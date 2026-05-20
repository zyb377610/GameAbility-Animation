#include "NePyStructBase.h"
#include "NePyDescriptor.h"
#include "NePyUserStruct.h"
#include "NePyDynamicType.h"
#include "NePyWrapperTypeRegistry.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

static PyTypeObject FNePyStructBaseType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"StructBase", /* tp_name */
	sizeof(FNePyStructBase), /* tp_basicsize */
};

// tp_init
int NePyStructBase_Init(FNePyStructBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init StructBase directly!");
	return -1;
}

// tp_repr
static PyObject* NePyStruct_Repr(FNePyStructBase* InSelf)
{
	return PyUnicode_FromFormat("<%s at %p>", InSelf->ob_type->tp_name, InSelf);
}

// tp_getattro
static PyObject* NePyStruct_Getattro(FNePyStructBase* InSelf, PyObject* InAttrName)
{
#if PY_MAJOR_VERSION >= 3
	if (PyObject* PyRet = _PyObject_GenericGetAttrWithDict((PyObject*)InSelf, InAttrName, nullptr, 1))
#else
	if (PyObject* PyRet = PyObject_GenericGetAttr((PyObject*)InSelf, InAttrName))
#endif
	{
		return PyRet;
	}

	const char* StrAttr = NePyString_AsString(InAttrName);
	while (true)
	{
		const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
		if (!ScriptStruct)
		{
			break;
		}

		void* ValuePtr = InSelf->Value;
		if (!ValuePtr)
		{
			UE_LOG(LogNePython, Warning, TEXT("ValuePtr is null for struct %s of object %p"), *ScriptStruct->GetName(), InSelf);
			break;
		}

		FProperty* Property = ScriptStruct->FindPropertyByName(FName(UTF8_TO_TCHAR(StrAttr)));
		if (!Property)
		{
			UE_LOG(LogNePython, Warning, TEXT("Cant find property %s in struct %s"), UTF8_TO_TCHAR(StrAttr), *ScriptStruct->GetName());
			break;
		}

		UScriptStruct* OwnerScriptStruct = Cast<UScriptStruct>(Property->GetOwnerUObject());
		if (!OwnerScriptStruct)
		{
			UE_LOG(LogNePython, Warning, TEXT("Cant get owner struct of property %s in struct %s"), UTF8_TO_TCHAR(StrAttr), *ScriptStruct->GetName());
			break;
		}

		// swallow previous exception
		PyErr_Clear();

		if (const FNePyStructTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(OwnerScriptStruct))
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(TypeInfo->TypeObject, Property, StrAttr));
			if (!Descr)
			{
				PyErr_Format(PyExc_AttributeError, "attribute ' '%.400s' of '%.50s' is not readable", StrAttr, Py_TYPE(InSelf)->tp_name);
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

		break;
	}

	PyErr_Format(PyExc_AttributeError,
		"'%.50s' object has no attribute '%.400s'",
		Py_TYPE(InSelf)->tp_name, StrAttr);
	return nullptr;
}

// tp_setattro
static int NePyStruct_Setattro(FNePyStructBase* InSelf, PyObject* InAttrName, PyObject* InValue)
{
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

		PyErr_Format(PyExc_AttributeError,
			"'%.50s' object attribute '%.400s' is read-only",
			PyType->tp_name, StrAttr);
		return -1;
	}


	while (true)
	{
		const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
		if (!ScriptStruct)
		{
			break;
		}

		void* ValuePtr = InSelf->Value;
		if (!ValuePtr)
		{
			break;
		}

		FProperty* Property = ScriptStruct->FindPropertyByName(FName(UTF8_TO_TCHAR(StrAttr)));
		if (!Property)
		{
			break;
		}

		UScriptStruct* OwnerScriptStruct = Cast<UScriptStruct>(Property->GetOwnerUObject());
		if (!OwnerScriptStruct)
		{
			break;
		}

		if (!ScriptStruct->IsChildOf(OwnerScriptStruct))
		{
			break;
		}

		// swallow previous exception
		PyErr_Clear();

		if (const FNePyStructTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(ScriptStruct))
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(TypeInfo->TypeObject, Property, StrAttr));
			if (!Descr)
			{
				PyErr_Format(PyExc_AttributeError, "attribute ' '%.400s' of '%.50s' is read-only", StrAttr, Py_TYPE(InSelf)->tp_name);
				return -1;
			}
			else
			{
				descrsetfunc DescrSetFunc = Py_TYPE(Descr)->tp_descr_set;
				if (DescrSetFunc)
				{
					return DescrSetFunc(Descr, InSelf, InValue);
				}
				else
				{
					PyErr_Format(PyExc_AttributeError, "attribute ' '%.400s' of '%.50s' is read-only", StrAttr, Py_TYPE(InSelf)->tp_name);
					return -1;
				}
			}
		}
		else
		{
			PyErr_Format(PyExc_AttributeError, "Cant get py wrapper type of '%.50s' object ", Py_TYPE(InSelf)->tp_name);
			return -1;
		}

		break;
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
		"'%.100s' object has no attribute '%.200s'",
		PyType->tp_name, StrAttr);
	return -1;
}

const UScriptStruct* FNePyStructBase::GetScriptStruct(FNePyStructBase* InSelf)
{
	PyTypeObject* PyType = Py_TYPE(InSelf);
	check(PyType_IsSubtype(PyType, &FNePyStructBaseType));
	const UScriptStruct* ScriptStruct = FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyType);
	return ScriptStruct;
}

void FNePyStructBase::SetValuePtr(FNePyStructBase* InSelf, void* Value)
{
	if (InSelf->SelfCreatedValue)
	{
		FreeValuePtr(InSelf);
	}
	InSelf->Value = Value;
	InSelf->SelfCreatedValue = false;
}

void* FNePyStructBase::AllocateValuePtr(FNePyStructBase* InSelf, const UScriptStruct* InScriptStruct)
{
	FreeValuePtr(InSelf);

	InSelf->Value = FMemory::Malloc(InScriptStruct->GetPropertiesSize(), InScriptStruct->GetMinAlignment());
	FMemory::Memset(InSelf->Value, '\0', InScriptStruct->GetPropertiesSize());
	InSelf->SelfCreatedValue = true;

	return InSelf->Value;
}

void FNePyStructBase::FreeValuePtr(FNePyStructBase* InSelf)
{
	if (InSelf->SelfCreatedValue)
	{
		FMemory::Free(InSelf->Value);
		InSelf->Value = nullptr;
		InSelf->SelfCreatedValue = false;
	}
}

FNePyStructBase* FNePyStructBase::Clone(FNePyStructBase* InSelf)
{
	PyTypeObject* PyType = Py_TYPE(InSelf);
	check(PyType_IsSubtype(PyType, &FNePyStructBaseType));
	const UScriptStruct* ScriptStruct = FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyType);

	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get ScriptStruct of type '%s'", Py_TYPE(InSelf)->tp_name);
		return nullptr;
	}

	void* SrcValuePtr = InSelf->Value;
	if (!SrcValuePtr)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get value address of type '%s'", Py_TYPE(InSelf)->tp_name);
		return nullptr;
	}

	FNePyStructBase* PyRet = (FNePyStructBase*)PyType_GenericAlloc(Py_TYPE(InSelf), 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyRet);
	AllocateValuePtr(PyRet, ScriptStruct);
	void* DstValuePtr = PyRet->Value;
	check(DstValuePtr);

	ScriptStruct->InitializeStruct(DstValuePtr);
	ScriptStruct->CopyScriptStruct(DstValuePtr, SrcValuePtr);
	return PyRet;
}

int32 FNePyStructBase::CalcPythonStructSize(const UScriptStruct* InScriptStruct)
{
	check(EnumHasAnyFlags(InScriptStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
	return Size();
}

int32 FNePyStructBase::Size()
{
	return sizeof(FNePyStructBase);
}

const UScriptStruct* FNePyStructBase::SearchScriptStruct(const TCHAR* InPackageName, const TCHAR* InStructName)
{
	UPackage* Package = FindObjectChecked<UPackage>(nullptr, InPackageName);
	if (!Package)
	{
		return nullptr;
	}
	return FindObject<UScriptStruct>(Package, InStructName);
}

PyObject* NePyStruct_Fields(FNePyStructBase* InSelf, PyObject* InArgs)
{
	bool bIncludeSuper = true;
	if (!PyArg_ParseTuple(InArgs, "|b:Fields", &bIncludeSuper))
	{
		return nullptr;
	}

	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get ScriptStruct of type '%s'", Py_TYPE(InSelf)->tp_name);
		return nullptr;
	}

	PyObject* PyRet = PyList_New(0);
	for (TFieldIterator<FProperty> PropIt(ScriptStruct, bIncludeSuper ? EFieldIteratorFlags::IncludeSuper : EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Prop = *PropIt;
		FString PropName = ScriptStruct->GetAuthoredNameForField(Prop);
		FNePyObjectPtr PyPropName = NePyStealReference(NePyString_FromString(TCHAR_TO_UTF8(*PropName)));
		PyList_Append(PyRet, PyPropName);
	}
	return PyRet;
}

PyObject* NePyStruct_GetStruct(FNePyStructBase* InSelf)
{
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	return NePyBase::ToPy(ScriptStruct);
}

PyObject* NePyStruct_Clone(FNePyStructBase* InSelf)
{
	return FNePyStructBase::Clone(InSelf);
}

PyObject* NePyStruct_AsDict(FNePyStructBase* InSelf, PyObject* InArgs)
{
	bool bDisplayName = false;
	bool bIncludeSuper = true;
	if (!PyArg_ParseTuple(InArgs, "|bb:AsDict", &bDisplayName, &bIncludeSuper))
	{
		return nullptr;
	}

	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get ScriptStruct of type '%s'", Py_TYPE(InSelf)->tp_name);
		return nullptr;
	}

	void* ValuePtr = InSelf->Value;
	if (!ValuePtr)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get value address of type '%s'", Py_TYPE(InSelf)->tp_name);
		return nullptr;
	}

	PyObject* PyRet = PyDict_New();
	for (TFieldIterator<FProperty> PropIt(ScriptStruct, bIncludeSuper ? EFieldIteratorFlags::IncludeSuper : EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Prop = *PropIt;
		FNePyObjectPtr PyPropValue = NePyStealReference(NePyBase::TryConvertFPropertyToPyObjectInContainerNoDependency(Prop, ValuePtr, 0, nullptr));
		if (!PyPropValue)
		{
			continue;
		}

		FString PropName = ScriptStruct->GetAuthoredNameForField(Prop);
#if WITH_EDITOR
		if (bDisplayName)
		{
			static const FName DisplayNameKey(TEXT("DisplayName"));
			if (Prop->HasMetaData(DisplayNameKey))
			{
				FString DisplayName = Prop->GetMetaData(DisplayNameKey);
				if (DisplayName.Len() > 0)
					PropName = DisplayName;
			}
		}
#endif
		PyDict_SetItemString(PyRet, TCHAR_TO_UTF8(*PropName), PyPropValue);
	}
	return PyRet;
}

static PyMethodDef FNePyStructBase_methods[] = {
	{ "Fields", NePyCFunctionCast(NePyStruct_Fields), METH_VARARGS, "(self, bIncludeSuper: bool = ...) -> list[str]" },
	{ "GetStruct", NePyCFunctionCast(NePyStruct_GetStruct), METH_NOARGS, "(self) -> ScriptStruct" },
	{ "Clone", NePyCFunctionCast(NePyStruct_Clone), METH_NOARGS, "(self) -> typing.Any" },
	{ "AsDict", NePyCFunctionCast(NePyStruct_AsDict), METH_VARARGS, "(self, bDisplayName: bool = ..., bIncludeSuper: bool = ...) -> dict" },
	{ "Struct", NePyCFunctionCast(&FNePyDynamicStructType::Struct), METH_NOARGS | METH_CLASS, "(cls) -> ScriptStruct" },
	{ NULL } /* Sentinel */
};

void NePyInitStructBase(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyStructBaseType;
#if PY_MAJOR_VERSION < 3
	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
	PyType->tp_flags |= Py_TPFLAGS_BASETYPE;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_init = (initproc)&NePyStructBase_Init;
	PyType->tp_repr = (reprfunc)&NePyStruct_Repr;
	PyType->tp_str = (reprfunc)&NePyStruct_Repr;
	PyType->tp_getattro = (getattrofunc)NePyStruct_Getattro;
	PyType->tp_setattro = (setattrofunc)NePyStruct_Setattro;
	PyType->tp_methods = FNePyStructBase_methods;
	PyType_Ready(PyType);
}

FNePyStructBase* NePyStructBaseCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyStructBaseType))
	{
		return (FNePyStructBase*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyStructBaseGetType()
{
	return &FNePyStructBaseType;
}
