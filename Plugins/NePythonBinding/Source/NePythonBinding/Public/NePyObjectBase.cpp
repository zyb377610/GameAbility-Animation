#include "NePyObjectBase.h"
#include "NePyHouseKeeper.h"
#include "NePyMemoryAllocator.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "NePyCallable.h"
#include "NePy/Manual/NePyObjectBase.inl"
#include "NePySubclass.h"
#include "NePySubclassing.h"
#include "NePyDynamicDelegateWrapper.h"
#include "NePyDynamicMulticastDelegateWrapper.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyDescriptor.h"

extern bool GNePyEnableReflectionFallback;
extern bool GNePyEnableReflectionFallbackLog;

// tp_new
PyObject* NePyObjectMeta_New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	if (PyTuple_Size(InArgs) != 3)
	{
		// 不是subclassing
		return PyType_GenericNew(InType, InArgs, InKwds);
	}

	return NePySubclassing(InType, InArgs, InKwds);
}

// tp_dealloc
void NePyObject_Dealloc(FNePyObjectBase* InSelf)
{
	// 如果Python对象析构时，UE4对象依旧存在，说明其所有权已转给给Python
	if (InSelf->Value)
	{
		FNePyHouseKeeper::Get().InvalidateTracker(InSelf->Value, true);
	}

	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此Value必为空
	check(!InSelf->Value);

	InSelf->ob_type->tp_free(InSelf);
}

// tp_init
int NePyObject_Init(FNePyObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyTypeObject* PyType = Py_TYPE(InSelf);
	const UClass* Class = FNePyWrapperTypeRegistry::Get().GetClassByPyType(PyType);
	if (IsValid(Class))
	{
		if (Class->IsChildOf<AActor>())
		{
			PyErr_Format(PyExc_RuntimeError, "can't init '%s' directly, use World.SpawnActor() instead", PyType->tp_name);
		}
		else
		{
			PyErr_Format(PyExc_RuntimeError, "can't init '%s' directly, use ue.NewObject() instead", PyType->tp_name);
		}
	}
	else
	{
		PyErr_Format(PyExc_RuntimeError, "can't init '%s' directly", PyType->tp_name);
	}
	return -1;
}

// tp_getattro
static PyObject* NePyObject_Getattro(FNePyObjectBase* InSelf, PyObject* InAttrName)
{
	// 首先从Python对象身上取（tp_methods, __dict__之类的）
#if PY_MAJOR_VERSION >= 3
	if (PyObject* Ret = _PyObject_GenericGetAttrWithDict((PyObject*)InSelf, InAttrName, nullptr, 1))
#else
	if (PyObject* Ret = PyObject_GenericGetAttr((PyObject*)InSelf, InAttrName))
#endif
	{
		return Ret;
	}

	if (PyErr_Occurred())
	{
		// 当 _PyObject_GenericGetAttrWithDict 中引发报错时，直接返回错误信息
#if PY_MAJOR_VERSION >= 3
		return nullptr;
#else
		// Python 2 无论如何都会抛出 AttributeError，这种情况将继续进行后续的反射流程
		if (PyErr_ExceptionMatches(PyExc_AttributeError))
		{
			PyErr_Clear();
		}
		else
		{
			return nullptr;
		}
#endif
	}

	if (!NePyBase::CheckValidAndSetPyErr(InSelf))
	{
		return nullptr;
	}

	UClass* MayNePySubclass = InSelf->Value->GetClass();
	while (Cast<UBlueprintGeneratedClass>(MayNePySubclass))
	{
		MayNePySubclass = MayNePySubclass->GetSuperClass();
	}
	
	while (UNePySubclass* Subclass = Cast<UNePySubclass>(MayNePySubclass))
	{
		// 如果是Subclassing动态生成的类，再从Subclassing的Dict上取
		if (Subclass->PyClass)
		{
			if (PyObject* Ret = PyDict_GetItem(Subclass->PyClass->PyDict, InAttrName))
			{
				PyErr_Clear();

				if (PyFunction_Check(Ret))
				{
#if PY_MAJOR_VERSION >= 3
					return PyMethod_New(Ret, InSelf);
#else
					return PyMethod_New(Ret, InSelf, Subclass->PyClass);
#endif
				}

				Py_INCREF(Ret);
				return Ret;
			}
		}

		MayNePySubclass = MayNePySubclass->GetSuperClass();
	}

	if (!NePyString_Check(InAttrName))
	{
		PyErr_Format(PyExc_TypeError,
			"attribute name must be string, not '%.200s'",
			Py_TYPE(InAttrName)->tp_name);
		return nullptr;
	}

	const char* StrAttr = NePyString_AsString(InAttrName);
	{
		FName AttrName(UTF8_TO_TCHAR(StrAttr));

		if (InSelf->Value->IsA<UEnum>())
		{
			// swallow previous exception
			PyErr_Clear();

			UEnum* EnumValue = (UEnum*)InSelf->Value;
			if (const FNePyEnumTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedEnumType(EnumValue))
			{
				PyTypeObject* PyType = TypeInfo->TypeObject;
				PyTypeObject* PyMetaType = Py_TYPE(PyType);
				return PyMetaType->tp_getattro((PyObject*)PyType, InAttrName);
			}
			else
			{
				PyErr_Format(PyExc_AttributeError, "Cant get py wrapper type of '%.50s' object ", Py_TYPE(InSelf)->tp_name);
				return nullptr;
			}
		}

		// 反射兜底，在Shipping版中不应当开启（因为性能很差），在开发版中如果反射兜底成功，也会打Warning日志来警告用户
		// 触发兜底的原因大概率是UProperty或UFunction的FName与用户在Python层使用的名称大小写不一致
		if (GNePyEnableReflectionFallback)
		{
			FProperty* PropertyValue = InSelf->Value->GetClass()->FindPropertyByName(AttrName);
			if (PropertyValue)
			{
				if (GNePyEnableReflectionFallbackLog)
				{
					UE_LOG(LogNePython, Warning, TEXT("%hs Property fallback triggered, required name is: '%s', original FName is: '%s', this is harmful for performance!"), __FUNCTION__, UTF8_TO_TCHAR(StrAttr), *PropertyValue->GetName());
				}
				// swallow previous exception
				PyErr_Clear();

				UClass* OwnerClass = PropertyValue->GetOwnerClass();
				if (const FNePyObjectTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(OwnerClass))
				{
					FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(TypeInfo->TypeObject, PropertyValue, StrAttr));
					if (!Descr)
					{
						PyErr_Format(PyExc_AttributeError, "attribute ' '%.400s' of '%.50s' is not readable", StrAttr, Py_TYPE(InSelf)->tp_name);
						return nullptr;
					}
					else
					{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
						NePyType_AddFieldNotifySupportForDescr(Descr, OwnerClass);
#endif
						descrgetfunc DescrGetFunc = Py_TYPE(Descr)->tp_descr_get;
						return DescrGetFunc(Descr, InSelf, (PyObject*)Py_TYPE(InSelf));
					}
				}
				else
				{
					PyErr_Format(PyExc_AttributeError, "Cant get py wrapper type of '%.50s' object ", Py_TYPE(InSelf)->tp_name);
					return nullptr;
				}
			}

			UFunction* FuncValue = InSelf->Value->GetClass()->FindFunctionByName(AttrName);
			// retry wth K2_ prefix
			if (!FuncValue)
			{
				FString K2Name = FString("K2_") + UTF8_TO_TCHAR(StrAttr);
				FuncValue = InSelf->Value->GetClass()->FindFunctionByName(FName(*K2Name));
			}

			if (FuncValue)
			{
				if (GNePyEnableReflectionFallbackLog)
				{
					UE_LOG(LogNePython, Warning, TEXT("%hs Function fallback triggered, required name is: '%s', original FName is: '%s', this is harmful for performance!"), __FUNCTION__, UTF8_TO_TCHAR(StrAttr), *FuncValue->GetName());
				}
				// swallow previous exception
				PyErr_Clear();

				UClass* OwnerClass = FuncValue->GetOwnerClass();
				if (const FNePyObjectTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(OwnerClass))
				{
					FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewFunction(TypeInfo->TypeObject, FuncValue, StrAttr));
					if (!Descr)
					{
						PyErr_Format(PyExc_AttributeError, "method ' '%.400s' of '%.50s' is not readable", StrAttr, Py_TYPE(InSelf)->tp_name);
						return nullptr;
					}
					else
					{
						descrgetfunc DescrGetFunc = Py_TYPE(Descr)->tp_descr_get;
						return DescrGetFunc(Descr, InSelf, (PyObject*)Py_TYPE(InSelf));
					}
				}
				else
				{
					PyErr_Format(PyExc_AttributeError, "Cant get py wrapper type of '%.50s' object ", Py_TYPE(InSelf)->tp_name);
					return nullptr;
				}
			}
		}

		// 兼容旧版本nepy里 ue.FindClass('Xxx').Xxx 的写法，性能非常差
		if (InSelf->Value->IsA<UClass>())
		{
			UClass* ClassValue = (UClass*)InSelf->Value;
			UFunction* FuncValue = ClassValue->FindFunctionByName(AttrName);

			// retry wth K2_ prefix
			if (!FuncValue)
			{
				FString K2Name = FString("K2_") + UTF8_TO_TCHAR(StrAttr);
				FuncValue = ClassValue->FindFunctionByName(FName(*K2Name));
			}

			if (FuncValue && EnumHasAnyFlags(FuncValue->FunctionFlags, EFunctionFlags::FUNC_Static))
			{
				// swallow previous exception
				PyErr_Clear();

				if (const FNePyObjectTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(ClassValue))
				{
					NePyGenUtil::FMethodDef* MethodDef = NePyGenUtil::FMethodDef::New(FuncValue, TypeInfo->TypeObject->tp_name);
					PyObject* PyRet = FNePyCallable::New(MethodDef, (PyObject*)TypeInfo->TypeObject);
					Py_DECREF(MethodDef);
					return PyRet;
				}
				else
				{
					PyErr_Format(PyExc_AttributeError, "Can't get py wrapper type of '%.50s' object ", Py_TYPE(InSelf)->tp_name);
					return nullptr;
				}
			}
		}
	}

	PyErr_Format(PyExc_AttributeError,
		"'%.50s' object has no attribute '%.400s'",
		Py_TYPE(InSelf)->tp_name, StrAttr);
	return nullptr;
}

// tp_setattro
static int NePyObject_Setattro(FNePyObjectBase* InSelf, PyObject* InAttrName, PyObject* InValue)
{
	if (!InSelf)
	{
		PyErr_SetString(PyExc_Exception, "FNePyObjectBase is in invalid state");
		return -1;
	}

	if (!NePyString_Check(InAttrName))
	{
		PyErr_Format(PyExc_TypeError,
			"attribute name must be string, not '%.200s'",
			Py_TYPE(InAttrName)->tp_name);
		return -1;
	}

	const char* StrAttr = NePyString_AsString(InAttrName);

	// 如果存在属性setter，优先调用属性setter
	PyTypeObject* PyType = Py_TYPE(InSelf);
	check(PyType->tp_dict);
	if (FNePyObjectPtr PyDescr = NePyNewReference(_PyType_Lookup(PyType, InAttrName)))
	{
		if (descrsetfunc FuncPtr = Py_TYPE(PyDescr)->tp_descr_set)
		{
			return FuncPtr(PyDescr, InSelf, InValue);
		}
		// tp_descr_set为空不一定是read-only，也有可能是class身上的属性，然后instance尝试去设置它
	}

	FName AttrName(UTF8_TO_TCHAR(StrAttr));

	// 反射兜底，在Shipping版中不应当开启（因为性能很差），在开发版中如果反射兜底成功，也会打Warning日志来警告用户
	// 触发兜底的原因大概率是UProperty或UFunction的FName与用户在Python层使用的名称大小写不一致
	if (GNePyEnableReflectionFallback)
	{
		// first of all check for UProperty
		// access to UProperty is only available when the object is valid
		if (FNePyHouseKeeper::Get().IsValid(InSelf))
		{
			// first check for property
			auto* PropertyValue = InSelf->Value->GetClass()->FindPropertyByName(AttrName);
			if (PropertyValue)
			{
				if (GNePyEnableReflectionFallbackLog)
				{
					UE_LOG(LogNePython, Warning, TEXT("%hs Property fallback triggered, required name is: '%s', original FName is: '%s', this is harmful for performance!"), __FUNCTION__, UTF8_TO_TCHAR(StrAttr), *PropertyValue->GetName());
				}
				UClass* OwnerClass = PropertyValue->GetOwnerClass();
				if (const FNePyObjectTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(OwnerClass))
				{
					FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(TypeInfo->TypeObject, PropertyValue, StrAttr));
					if (!Descr || !Py_TYPE(Descr)->tp_descr_set)
					{
						PyErr_Format(PyExc_AttributeError, "attribute ' '%.400s' of '%.50s' is read-only", StrAttr, Py_TYPE(InSelf)->tp_name);
						return -1;
					}
					else
					{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
						NePyType_AddFieldNotifySupportForDescr(Descr, OwnerClass);
#endif
						descrsetfunc DescrSetFunc = Py_TYPE(Descr)->tp_descr_set;
						return DescrSetFunc(Descr, InSelf, InValue);
					}
				}
				else
				{
					PyErr_Format(PyExc_AttributeError, "Can't get py wrapper type of '%.50s' object ", Py_TYPE(InSelf)->tp_name);
					return -1;
				}
			}

			// now check for FuncValue name
			if (UFunction* FuncValue = InSelf->Value->GetClass()->FindFunctionByName(AttrName))
			{
				if (GNePyEnableReflectionFallbackLog)
				{
					UE_LOG(LogNePython, Warning, TEXT("%hs Function fallback triggered, required name is: '%s', original FName is: '%s', this is harmful for performance!"), __FUNCTION__, UTF8_TO_TCHAR(StrAttr), *FuncValue->GetName());
				}
				PyErr_Format(PyExc_ValueError, "you cannot overwrite a UFunction '%s'", StrAttr);
				return -1;
			}
		}
	}

	bool bHasPyDict = PyType->tp_dictoffset != 0;
	if (bHasPyDict)
	{
		// 对象存在__dict__，前面的属性设置失败了，将属性写入__dict__
		PyErr_Clear();
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 3
		FNePyObjectPtr PyDict = FNePyObjectPtr::StealReference(PyObject_GenericGetDict(InSelf, nullptr));
#else
		PyObject** PyDictPtr = _PyObject_GetDictPtr(InSelf);
		check(PyDictPtr);
		PyObject* PyDict = *PyDictPtr;
		if (!PyDict)
		{
			PyDict = PyDict_New();
			*PyDictPtr = PyDict;
		}
#endif
		if (InValue)
		{
			return PyDict_SetItem(PyDict, InAttrName, InValue);
		}
		else
		{
			return PyDict_DelItem(PyDict, InAttrName);
		}
	}

	PyErr_Format(PyExc_AttributeError,
		"'%.100s' object failed to set attribute '%.200s'",
		PyType->tp_name, StrAttr);
	return -1;
}

// tp_repr
static PyObject* NePyObject_Repr(FNePyObjectBase* InSelf)
{
	if (!FNePyHouseKeeper::Get().IsValid(InSelf))
	{
		return PyUnicode_FromFormat("<Invalid %s object at %p>", Py_TYPE(InSelf)->tp_name, InSelf->Value);
	}
	UObject* Object = InSelf->Value;
	UClass* Class = Object->GetClass();
	return PyUnicode_FromFormat("<%s '%s' at %p>", TCHAR_TO_UTF8(*Class->GetName()), TCHAR_TO_UTF8(*Object->GetName()), Object);
}

int NePyObject_Bool(FNePyObjectBase* InSelf)
{
	int RetVal = FNePyHouseKeeper::Get().IsValid(InSelf) ? 1 : 0;
	return RetVal;
}

static PyTypeObject FNePyObjectType_ObjectMeta = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"ObjectMeta", /* tp_name */
};

void NePyObjectType_InitCommon(PyTypeObject* InPyType)
{
	// 检查下PyTypeObject的引用计数，以保证在注册此类型前，没有被子类意外地提前引用，造成污染
	checkf(InPyType && (Py_REFCNT(InPyType) == 1), TEXT("NePyObjectType_InitCommon PyTypeObject is nullptr or ref_count isnot equal 1."));

	PyTypeObject* MetaType = &FNePyObjectType_ObjectMeta;
	if (MetaType->tp_dict == nullptr)
	{
		MetaType->tp_base = &PyType_Type;
#if PY_MAJOR_VERSION < 3
		MetaType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
		MetaType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
		MetaType->tp_new = NePyObjectMeta_New;
		PyType_Ready(MetaType);
	}

#if PY_MAJOR_VERSION < 3
	InPyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
	InPyType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
	InPyType->tp_flags |= Py_TPFLAGS_BASETYPE;

#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 10
	Py_SET_TYPE(InPyType, MetaType);
#else
	Py_TYPE(InPyType) = MetaType;
#endif

	InPyType->tp_new = PyType_GenericNew;
	InPyType->tp_dealloc = (destructor)&NePyObject_Dealloc;
	InPyType->tp_init = (initproc)&NePyObject_Init;
	InPyType->tp_getattro = (getattrofunc)NePyObject_Getattro;
	InPyType->tp_setattro = (setattrofunc)NePyObject_Setattro;
	InPyType->tp_repr = (reprfunc)&NePyObject_Repr;
	InPyType->tp_str = (reprfunc)&NePyObject_Repr;
}

static PyTypeObject FNePyObjectType_ObjectBase = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ObjectBase", /* tp_name */
	sizeof(FNePyObjectBase), /* tp_basicsize */
};

static PyMethodDef FNePyObjectType_ObjectBase_methods[] = {
	NePyManualExportFuncs
#if PY_MAJOR_VERSION == 2
	{"__nonzero__", NePyCFunctionCast(&NePyObject_IsValid), METH_NOARGS, ""},
#endif
	{ NULL } /* Sentinel */
};

void NePyInitObjectBase(PyObject* PyOuterModule)
{
	PyTypeObject* NePyObjectType = &FNePyObjectType_ObjectBase;
	NePyObjectType_InitCommon(NePyObjectType);
	NePyObjectType->tp_methods = FNePyObjectType_ObjectBase_methods;
	NePyObjectType->tp_base = nullptr;

	static PyNumberMethods PyNumber;
	NePyObjectType->tp_as_number = &PyNumber;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_bool = (inquiry)NePyObject_Bool;
#else
	PyNumber.nb_nonzero = (inquiry)NePyObject_Bool;
#endif

	PyType_Ready(NePyObjectType);

	// ObjectBase对象禁止用户构建
	//PyModule_AddObject(PyOuterModule, "ObjectBase", (PyObject*)NePyObjectType);
}

FNePyObjectBase* NePyObjectBaseNew(UObject* InValue, PyTypeObject* InPyType)
{
	FNePyObjectBase* RetValue = (FNePyObjectBase*)PyType_GenericAlloc(InPyType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyObjectBase* NePyObjectBaseCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyObjectType_ObjectBase))
	{
		return (FNePyObjectBase*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyObjectBaseGetType()
{
	return &FNePyObjectType_ObjectBase;
}

FNePyObjectBase* NePyObjectBaseNewInternalUseOnly(UObject* InValue)
{
	return NePyObjectBaseNew(InValue, &FNePyObjectType_ObjectBase);
}

// automatically bind events based on class methods names
void NePyObjectBaseAutoBindEventForPyClass(FNePyObjectBase* InPyObject, PyObject* InPyClass)
{
	PyObject* PyAttrs = PyObject_Dir(InPyClass);
	if (!PyAttrs)
	{
		return;
	}

	Py_ssize_t Len = PyList_Size(PyAttrs);
	for (Py_ssize_t i = 0; i < Len; i++)
	{
		PyObject* PyAttrName = PyList_GetItem(PyAttrs, i);
		if (!PyAttrName || !NePyString_Check(PyAttrName))
		{
			continue;
		}
		const char* StrAttr = NePyString_AsString(PyAttrName);
		FString AttrName(UTF8_TO_TCHAR(StrAttr));
		if (!AttrName.StartsWith("on_", ESearchCase::CaseSensitive))
		{
			continue;
		}
		// check if the attr is a callable
		PyObject* PyCallable = PyObject_GetAttrString(InPyClass, StrAttr);
		if (PyCallable && PyCallable_Check(PyCallable))
		{
			TArray<FString> Parts;
			if (AttrName.ParseIntoArray(Parts, UTF8_TO_TCHAR("_")) > 1)
			{
				FString EventName;
				for (FString part : Parts)
				{
					FString first_letter = part.Left(1).ToUpper();
					part.RemoveAt(0);
					EventName = EventName.Append(first_letter);
					EventName = EventName.Append(part);
				}
				// do not fail on wrong properties
				NePyObjectBaseBindEvent(InPyObject, EventName, PyCallable, false);
			}
		}
		Py_XDECREF(PyCallable);
	}

	Py_DECREF(PyAttrs);
}

void NePyObjectBaseBindEventsForPyClassByAttribute(UObject* InObject, PyObject* InPyClass)
{
	// attempt to register events
	PyObject* PyAttrs = PyObject_Dir(InPyClass);
	if (!PyAttrs)
	{
		return;
	}

	AActor* Actor = Cast<AActor>(InObject);
	if (!Actor)
	{
		UActorComponent* Comp = Cast<UActorComponent>(InObject);
		if (!Comp)
		{
			return;
		}
		Actor = Comp->GetOwner();
	}

	Py_ssize_t Len = PyList_Size(PyAttrs);
	for (Py_ssize_t i = 0; i < Len; i++)
	{
		PyObject* PyAttrName = PyList_GetItem(PyAttrs, i);
		if (!PyAttrName || !NePyString_Check(PyAttrName))
		{
			continue;
		}
		PyObject* PyCallable = PyObject_GetAttrString(InPyClass, NePyString_AsString(PyAttrName));
		if (PyCallable && PyCallable_Check(PyCallable))
		{
			// check for ue_event signature
			PyObject* PyEventSignature = PyObject_GetAttrString(PyCallable, (char*)"ue_event");
			if (PyEventSignature)
			{
				if (NePyString_Check(PyEventSignature))
				{
					FString EventName = FString(UTF8_TO_TCHAR(NePyString_AsString(PyEventSignature)));
					TArray<FString> Parts;
					int Count = EventName.ParseIntoArray(Parts, UTF8_TO_TCHAR("."));
					if (Count < 1 || Count > 2)
					{
						PyErr_SetString(PyExc_Exception, "invalid ue_event syntax, must be the name of an event or ComponentName.Event");
						PyErr_Print();
					}
					else
					{
						if (Count == 1)
						{
							if (!NePyObjectBaseBindEvent((FNePyObjectBase *)NePyBase::ToPy(Actor), Parts[0], PyCallable, true))
							{
								PyErr_Print();
							}
						}
						else
						{
							bool bFound = false;
							for (UActorComponent* Comp : Actor->GetComponents())
							{
								if (Comp->GetFName() == FName(*Parts[0]))
								{
									if (!NePyObjectBaseBindEvent((FNePyObjectBase *)NePyBase::ToPy(Comp), Parts[1], PyCallable, true))
									{
										PyErr_Print();
									}
									bFound = true;
									break;
								}
							}

							if (!bFound)
							{
								PyErr_SetString(PyExc_Exception, "unable to find Comp by name");
								PyErr_Print();
							}
						}
					}
				}
				else
				{
					PyErr_SetString(PyExc_Exception, "ue_event attribute must be a string");
					PyErr_Print();
				}
			}
			Py_XDECREF(PyEventSignature);
		}
		Py_XDECREF(PyCallable);
	}
	Py_DECREF(PyAttrs);

	PyErr_Clear();
}


PyObject* NePyObjectBaseBindEvent(FNePyObjectBase* InPyObject, FString InEventName, PyObject* InPyCallable, bool bFailOnWrongProp)
{
	FName EventName(*InEventName);
	const FProperty* PropValue = InPyObject->Value->GetClass()->FindPropertyByName(EventName);
	if (!PropValue)
	{
		if (bFailOnWrongProp)
		{
			return PyErr_Format(PyExc_Exception, "unable to find event property %s", TCHAR_TO_UTF8(*InEventName));
		}
		Py_RETURN_NONE;
	}

	if (auto MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(PropValue))
	{
		FNePyDynamicMulticastDelegateWrapper* PyDelegateWrapper = FNePyDynamicMulticastDelegateWrapper::New(InPyObject->Value, MulticastDelegateProperty);
		Py_DECREF(PyDelegateWrapper);
		PyDelegateWrapper->Add(InPyCallable);
	}
	else if (auto DelegateProperty = CastField<FDelegateProperty>(PropValue))
	{
		FNePyDynamicDelegateWrapper* PyDelegateWrapper = FNePyDynamicDelegateWrapper::New(InPyObject->Value, DelegateProperty);
		Py_DECREF(PyDelegateWrapper);
		PyDelegateWrapper->Bind(InPyCallable);
	}
	else
	{
		if (bFailOnWrongProp)
		{
			return PyErr_Format(PyExc_Exception, "property %s is not an event", TCHAR_TO_UTF8(*InEventName));
		}
	}

	Py_RETURN_NONE;
}

PyObject* NePyObjectBaseUnbindEvent(FNePyObjectBase* InPyObject, FString InEventName, PyObject* InPyCallable, bool bFailOnWrongProp)
{
	FName EventName(*InEventName);
	const FProperty* Prop = InPyObject->Value->GetClass()->FindPropertyByName(EventName);
	if (!Prop)
	{
		if (bFailOnWrongProp)
		{
			return PyErr_Format(PyExc_Exception, "unable to find event property %s", TCHAR_TO_UTF8(*InEventName));
		}
		Py_RETURN_NONE;
	}

	if (auto MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Prop))
	{
		FNePyDynamicMulticastDelegateWrapper* PyDelegateWrapper = FNePyDynamicMulticastDelegateWrapper::New(InPyObject->Value, MulticastDelegateProperty);
		Py_DECREF(PyDelegateWrapper);
		PyDelegateWrapper->Remove(InPyCallable);
	}
	else if (auto DelegateProperty = CastField<FDelegateProperty>(Prop))
	{
		FNePyDynamicDelegateWrapper* PyDelegateWrapper = FNePyDynamicDelegateWrapper::New(InPyObject->Value, DelegateProperty);
		Py_DECREF(PyDelegateWrapper);
		if (PyDelegateWrapper->IsBoundTo(InPyCallable))
		{
			PyDelegateWrapper->Unbind();
		}
	}
	else
	{
		if (bFailOnWrongProp)
		{
			return PyErr_Format(PyExc_Exception, "property %s is not an event", TCHAR_TO_UTF8(*InEventName));
		}
	}

	Py_RETURN_NONE;
}

PyObject* NePyObjectBaseUnbindAllEvent(FNePyObjectBase* InPyObject, FString InEventName, bool bFailOnWrongProp)
{
	FName EventName(*InEventName);
	const FProperty* Prop = InPyObject->Value->GetClass()->FindPropertyByName(EventName);
	if (!Prop)
	{
		if (bFailOnWrongProp)
		{
			return PyErr_Format(PyExc_Exception, "unable to find event property %s", TCHAR_TO_UTF8(*InEventName));
		}
		Py_RETURN_NONE;
	}

	if (auto MulticastDelegateProperty = CastField<FMulticastDelegateProperty>(Prop))
	{
		FNePyDynamicMulticastDelegateWrapper* PyDelegateWrapper = FNePyDynamicMulticastDelegateWrapper::New(InPyObject->Value, MulticastDelegateProperty);
		Py_DECREF(PyDelegateWrapper);
		PyDelegateWrapper->Clear();
	}
	else if (auto DelegateProperty = CastField<FDelegateProperty>(Prop))
	{
		FNePyDynamicDelegateWrapper* PyDelegateWrapper = FNePyDynamicDelegateWrapper::New(InPyObject->Value, DelegateProperty);
		Py_DECREF(PyDelegateWrapper);
		PyDelegateWrapper->Unbind();
	}
	else
	{
		if (bFailOnWrongProp)
		{
			return PyErr_Format(PyExc_Exception, "property %s is not an event", TCHAR_TO_UTF8(*InEventName));
		}
	}

	Py_RETURN_NONE;
}

