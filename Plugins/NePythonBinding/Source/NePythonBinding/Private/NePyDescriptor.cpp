#include "NePyDescriptor.h"
#include "NePyObjectBase.h"
#include "NePyStructBase.h"
#include "NePyCallable.h"
#include "NePyPropertyConvert.h"
#include "NePyGeneratedClass.h"
#include "NePyUtil.h"
#include "Engine/BlueprintGeneratedClass.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
#include "INotifyFieldValueChanged.h"
#endif
#include "HAL/IConsoleManager.h"

extern TAutoConsoleVariable<int32> CVarSubclassingLog;

#pragma region FNePyDescriptorBase

static PyMemberDef FNePyDescriptorBase_members[] = {
	{(char*)"__objclass__", T_OBJECT, offsetof(FNePyDescriptorBase, Type), READONLY},
	{(char*)"__name__", T_OBJECT, offsetof(FNePyDescriptorBase, Name), READONLY},
	{ nullptr } /* Sentinel */
};

void FNePyDescriptorBase::InitPyTypeCommon(PyTypeObject* InPyType)
{
	InPyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
	InPyType->tp_new = PyType_GenericNew;
	InPyType->tp_getattro = PyObject_GenericGetAttr;
	InPyType->tp_dealloc = (destructor)FNePyDescriptorBase::Dealloc;
	InPyType->tp_init = (initproc)FNePyDescriptorBase::Init;
	InPyType->tp_repr = (reprfunc)FNePyDescriptorBase::Repr;
	InPyType->tp_traverse = (traverseproc)FNePyDescriptorBase::Traverse;
	InPyType->tp_clear = (inquiry)FNePyDescriptorBase::Clear;
	InPyType->tp_members = FNePyDescriptorBase_members;
}

FNePyDescriptorBase* FNePyDescriptorBase::New(PyTypeObject* InDescrType, PyTypeObject* InType, const char* InName)
{
	FNePyDescriptorBase* Descr = (FNePyDescriptorBase*)PyObject_GC_New(FNePyDescriptorBase, InDescrType);
	if (Descr)
	{
		Py_XINCREF(InType);
		Descr->Type = InType;
		Descr->Name = NePyString_FromString(InName);
		if (Descr->Name == nullptr)
		{
			Py_DECREF(Descr);
			Descr = nullptr;
		}
		else
		{
			PyObject_GC_Track(Descr);
		}
	}
	return Descr;
}

void FNePyDescriptorBase::Dealloc(FNePyDescriptorBase* InSelf)
{
	PyObject_GC_UnTrack(InSelf);
	Clear(InSelf);
	PyObject_GC_Del(InSelf);
}

int FNePyDescriptorBase::Clear(FNePyDescriptorBase* InSelf)
{
	Py_CLEAR(InSelf->Type);
	Py_CLEAR(InSelf->Name);
	return 0;
}

int FNePyDescriptorBase::Init(FNePyDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init PropertyDescriptor directly!");
	return -1;
}

PyObject* FNePyDescriptorBase::Repr(FNePyDescriptorBase* InSelf)
{
	return FNePyDescriptorBase::StringFormat(InSelf, "<member '%s' of '%s' objects>");
}

int FNePyDescriptorBase::Traverse(FNePyDescriptorBase* InSelf, visitproc InVisit, void* InArg)
{
	// Aliases for Py_VISIT
	visitproc visit = InVisit;
	void* arg = InArg;

	Py_VISIT(InSelf->Type);
	return 0;
}

const char* FNePyDescriptorBase::DescrName(FNePyDescriptorBase* InSelf)
{
	if (InSelf->Name != nullptr && NePyString_Check(InSelf->Name))
	{
		return NePyString_AsString(InSelf->Name);
	}
	else
	{
		return "?";
	}
}

PyTypeObject* FNePyDescriptorBase::DescrType(FNePyDescriptorBase* InSelf)
{
	return InSelf->Type;
}

int FNePyDescriptorBase::DescrCheck(FNePyDescriptorBase* InSelf, PyObject* InObject)
{
	if (!PyObject_TypeCheck(InObject, InSelf->Type))
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' for '%.100s' objects doesn't apply to a ' %.100s' object", 
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name, Py_TYPE(InObject)->tp_name);
		return -1;
	}
	return 0;
}

PyObject* FNePyDescriptorBase::StringFormat(FNePyDescriptorBase* InSelf, const char* InFormat)
{
	return NePyString_FromFormat(InFormat, FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
}

#pragma endregion

#pragma region FNePyPropertyDescriptor

static PyTypeObject PyType_NePyPropertyDescriptor = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyPropertyDescriptor", /* tp_name */
	sizeof(FNePyPropertyDescriptor), /* tp_basicsize */
};

void FNePyPropertyDescriptor::InitPyType()
{
	PyTypeObject* PyObjectType = &PyType_NePyPropertyDescriptor;
	FNePyDescriptorBase::InitPyTypeCommon(PyObjectType);
	PyObjectType->tp_dealloc = (destructor)FNePyPropertyDescriptor::Dealloc;
	PyObjectType->tp_repr = (reprfunc)FNePyPropertyDescriptor::Repr;
	PyObjectType->tp_descr_get = (descrgetfunc)FNePyPropertyDescriptor::DescrGet;
	PyObjectType->tp_descr_set = (descrsetfunc)FNePyPropertyDescriptor::DescrSet;
	PyType_Ready(PyObjectType);
}

FNePyPropertyDescriptor* FNePyPropertyDescriptor::New(PyTypeObject* InType, const char* InName, const FProperty* InBindProperty, getter InGetter, setter InSetter)
{
	FNePyPropertyDescriptor* Descr = (FNePyPropertyDescriptor*)FNePyDescriptorBase::New(&PyType_NePyPropertyDescriptor, InType, InName);
	if (!Descr)
	{
		return nullptr;
	}
	Descr->Prop = InBindProperty;
	Descr->Getter = InGetter;
	Descr->Setter = InSetter;
	Descr->GetFunc = nullptr;
	Descr->SetFunc = nullptr;
	return Descr;
}

void FNePyPropertyDescriptor::Dealloc(FNePyPropertyDescriptor* InSelf)
{
	FNePyPropertyDescriptor::Reset(InSelf);
	FNePyDescriptorBase::Dealloc(InSelf);
}

void FNePyPropertyDescriptor::Reset(FNePyPropertyDescriptor* InSelf)
{
	InSelf->Prop = nullptr;
	InSelf->Getter = nullptr;
	InSelf->Setter = nullptr;
	if (InSelf->GetFunc)
	{
		InSelf->GetFunc->Reset();
		Py_DECREF(InSelf->GetFunc);
		InSelf->GetFunc = nullptr;
	}
	if (InSelf->SetFunc)
	{
		InSelf->SetFunc->Reset();
		Py_DECREF(InSelf->SetFunc);
		InSelf->SetFunc = nullptr;
	}
}

PyObject* FNePyPropertyDescriptor::Repr(FNePyPropertyDescriptor* InSelf)
{
	return FNePyPropertyDescriptor::StringFormat(InSelf, "<attribute '%s' of '%s' objects>");
}

PyObject* FNePyPropertyDescriptor::DescrGet(FNePyPropertyDescriptor* InSelf, PyObject* InObject, PyTypeObject* InType)
{
	if (InObject == nullptr)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	if (!InSelf->Prop)
	{
		PyErr_Format(PyExc_Exception, "attribute '%s' of '%.100s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	if (InSelf->GetFunc)
	{
		// 只有UObject的属性可以有Getter方法
		check(InSelf->Prop->Owner.IsUObject());
		FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(0));
		return FNePyCallable::CallMethod(InSelf->GetFunc, InObject, PyArgs, nullptr);
	}

	return InSelf->Getter(InObject, (void*)InSelf->Prop);
}

int FNePyPropertyDescriptor::DescrSet(FNePyPropertyDescriptor* InSelf, PyObject* InObject, PyObject* InValue)
{
	if (FNePyPropertyDescriptor::DescrCheck(InSelf, InObject) < 0)
	{
		return -1;
	}

	if (!InSelf->Prop)
	{
		PyErr_Format(PyExc_Exception, "attribute '%s' of '%.100s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return -1;
	}

	if (!InSelf->Setter)
	{
		PyErr_Format(PyExc_TypeError, "attribute '%s' of '%.100s' objects is read-only", 
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return -1;
	}

	if (InSelf->SetFunc)
	{
		// 只有UObject的属性可以有Setter方法
		check(InSelf->Prop->Owner.IsUObject());

		FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(1));
		PyTuple_SetItem(PyArgs, 0, NePyNewReference(InValue).Release());
		FNePyObjectPtr PyRet = NePyStealReference(FNePyCallable::CallMethod(InSelf->SetFunc, InObject, PyArgs, nullptr));
		return PyRet ? 0 : -1;
	}

	return InSelf->Setter(InObject, InValue, (void*)InSelf->Prop);
}

FNePyPropertyDescriptor* FNePyPropertyDescriptor::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&PyType_NePyPropertyDescriptor))
	{
		return (FNePyPropertyDescriptor*)InPyObj;
	}
	return nullptr;
}

static PyObject* NePyPropertyGetter_Object(FNePyObjectBase* InObject, const FProperty* InClosureProperty)
{
	if (!NePyBase::CheckValidAndSetPyErr(InObject, InClosureProperty))
	{
		return nullptr;
	}
	PyObject* PyRet = NePyBase::TryConvertFPropertyToPyObjectInContainer(InClosureProperty, InObject->Value, 0, InObject->Value);
	if (!PyRet)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InClosureProperty);
	}
	return PyRet;
}

static int NePyPropertySetter_Object(FNePyObjectBase* InObject, PyObject* InValue, const FProperty* InClosureProperty)
{
	if (!NePyBase::CheckValidAndSetPyErr(InObject, InClosureProperty))
	{
		return -1;
	}
	if (!NePyBase::TryConvertPyObjectToFPropertyInContainer(InValue, InClosureProperty, InObject->Value, 0, InObject->Value))
	{
		NePyBase::SetConvertPyObjectToFPropertyError(InValue, InClosureProperty);
		return -1;
	}
	return 0;
}

static PyObject* NePyPropertyGetter_Struct(FNePyStructBase* InSelf, const FProperty* InClosureProperty)
{
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	void* ValuePtr = ScriptStruct ? InSelf->Value : nullptr;
	if (!ValuePtr)
	{
		PyErr_Format(PyExc_AttributeError,
			"'%.100s' object has no attribute '%.200s'",
			Py_TYPE(InSelf)->tp_name, TCHAR_TO_UTF8(*InClosureProperty->GetName()));
		return nullptr;
	}

	PyObject* PyRet = nullptr;
	auto Converter = NePyGetPropertyToPyObjectConverterForStruct(InClosureProperty);
	if (Converter)
	{
		PyRet = Converter(InClosureProperty, InClosureProperty->ContainerPtrToValuePtr<void>(ValuePtr), InSelf);
	}
	if (!PyRet)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InClosureProperty);
	}
	return PyRet;
}

static int NePyPropertySetter_Struct(FNePyStructBase* InSelf, PyObject* InValue, const FProperty* InClosureProperty)
{
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	void* ValuePtr = ScriptStruct ? InSelf->Value : nullptr;
	if (!ValuePtr)
	{
		PyErr_Format(PyExc_AttributeError,
			"'%.100s' object has no attribute '%.200s'",
			Py_TYPE(InSelf)->tp_name, TCHAR_TO_UTF8(*InClosureProperty->GetName()));
		return -1;
	}

	auto Converter = NePyGetPyObjectToPropertyConverterForStruct(InClosureProperty);
	if (!Converter || !Converter(InValue, InClosureProperty, InClosureProperty->ContainerPtrToValuePtr<void>(ValuePtr), InSelf))
	{
		NePyBase::SetConvertPyObjectToFPropertyError(InValue, InClosureProperty);
		return -1;
	}
	return 0;
}

#pragma endregion

#pragma region FNePyFunctionDescriptorBase

static PyTypeObject PyType_NePyFunctionDescriptorBase = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyFunctionDescriptorBase", /* tp_name */
	sizeof(FNePyFunctionDescriptorBase), /* tp_basicsize */
};

void FNePyFunctionDescriptorBase::InitPyType()
{
	PyTypeObject* PyObjectType = &PyType_NePyFunctionDescriptorBase;
	FNePyFunctionDescriptorBase::InitPyTypeCommon(PyObjectType);
	PyObjectType->tp_dealloc = (destructor)FNePyFunctionDescriptorBase::Dealloc;
	PyObjectType->tp_repr = (reprfunc)FNePyFunctionDescriptorBase::Repr;
	PyType_Ready(PyObjectType);
}

void FNePyFunctionDescriptorBase::Dealloc(FNePyFunctionDescriptorBase* InSelf)
{
	FNePyFunctionDescriptorBase::Reset(InSelf);
	FNePyDescriptorBase::Dealloc(InSelf);
}

void FNePyFunctionDescriptorBase::Reset(FNePyFunctionDescriptorBase* InSelf)
{
	if (InSelf->MethodDef)
	{
		InSelf->MethodDef->Reset();
		Py_DecRef(InSelf->MethodDef);
		InSelf->MethodDef = nullptr;
	}
}

PyObject* FNePyFunctionDescriptorBase::Repr(FNePyFunctionDescriptorBase* InSelf)
{
	if (InSelf->MethodDef && InSelf->MethodDef->bIsStatic)
	{
		FNePyFunctionDescriptorBase::StringFormat(InSelf, "<static method '%s' of '%s' objects>");
	}
	return FNePyFunctionDescriptorBase::StringFormat(InSelf, "<method '%s' of '%s' objects>");
}

FNePyFunctionDescriptorBase* FNePyFunctionDescriptorBase::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&PyType_NePyFunctionDescriptorBase))
	{
		return (FNePyFunctionDescriptorBase*)InPyObj;
	}
	return nullptr;
}

#pragma endregion

#pragma region FNePyFunctionDescriptor

static PyTypeObject PyType_NePyFunctionDescriptor = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyFunctionDescriptor", /* tp_name */
	sizeof(FNePyFunctionDescriptor), /* tp_basicsize */
};

void FNePyFunctionDescriptor::InitPyType()
{
	PyTypeObject* PyObjectType = &PyType_NePyFunctionDescriptor;
	FNePyFunctionDescriptorBase::InitPyTypeCommon(PyObjectType);
	PyObjectType->tp_base = &PyType_NePyFunctionDescriptorBase;
	PyObjectType->tp_dealloc = (destructor)FNePyFunctionDescriptorBase::Dealloc;
	PyObjectType->tp_repr = (reprfunc)FNePyFunctionDescriptorBase::Repr;
	PyObjectType->tp_call = (ternaryfunc)FNePyFunctionDescriptor::Call;
	PyObjectType->tp_descr_get = (descrgetfunc)FNePyFunctionDescriptor::DescrGet;
	PyType_Ready(PyObjectType);
}

FNePyFunctionDescriptor* FNePyFunctionDescriptor::New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction)
{
	FNePyFunctionDescriptor* Descr = (FNePyFunctionDescriptor*)FNePyDescriptorBase::New(&PyType_NePyFunctionDescriptor, InType, InName);
	if (!Descr)
	{
		return nullptr;
	}
	Descr->MethodDef = NePyGenUtil::FMethodDef::New(InBindFunction, InType->tp_name);
	return Descr;
}

PyObject* FNePyFunctionDescriptor::Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	Py_ssize_t ArgCount = PyTuple_GET_SIZE(InArgs);
	if (ArgCount < 1)
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' of '%s' object needs an argument",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	PyObject* SelfObject = PyTuple_GET_ITEM(InArgs, 0);
	if (PyObject_IsSubclass((PyObject*)Py_TYPE(SelfObject), (PyObject*)InSelf->Type) != 1)
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' requires a '%s' object but received a '%s'",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name, Py_TYPE(SelfObject)->tp_name);
		return nullptr;
	}

	FNePyObjectPtr Args = NePyStealReference(PyTuple_GetSlice(InArgs, 1, ArgCount));
	return FNePyCallable::CallMethod(InSelf->MethodDef, SelfObject, Args, InKwds);
}

PyObject* FNePyFunctionDescriptor::DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	if (!InObject)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	if (FNePyPropertyDescriptor::DescrCheck(InSelf, InObject) < 0)
	{
		return nullptr;
	}

	// 调用对象函数的情况
	PyObject* Ret = FNePyCallable::New(InSelf->MethodDef, InObject);
	return Ret;
}

#pragma endregion

#pragma region FNePyStaticFunctionDescriptor

static PyTypeObject PyType_NePyStaticFunctionDescriptor = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyStaticFunctionDescriptor", /* tp_name */
	sizeof(FNePyStaticFunctionDescriptor), /* tp_basicsize */
};

void FNePyStaticFunctionDescriptor::InitPyType()
{
	PyTypeObject* PyObjectType = &PyType_NePyStaticFunctionDescriptor;
	FNePyFunctionDescriptorBase::InitPyTypeCommon(PyObjectType);
	PyObjectType->tp_base = &PyType_NePyFunctionDescriptorBase;
	PyObjectType->tp_dealloc = (destructor)FNePyFunctionDescriptorBase::Dealloc;
	PyObjectType->tp_repr = (reprfunc)FNePyFunctionDescriptorBase::Repr;
	PyObjectType->tp_call = (ternaryfunc)FNePyStaticFunctionDescriptor::Call;
	PyObjectType->tp_descr_get = (descrgetfunc)FNePyStaticFunctionDescriptor::DescrGet;
	PyType_Ready(PyObjectType);
}

FNePyStaticFunctionDescriptor* FNePyStaticFunctionDescriptor::New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction)
{
	FNePyStaticFunctionDescriptor* Descr = (FNePyStaticFunctionDescriptor*)FNePyDescriptorBase::New(&PyType_NePyStaticFunctionDescriptor, InType, InName);
	if (!Descr)
	{
		return nullptr;
	}
	Descr->MethodDef = NePyGenUtil::FMethodDef::New(InBindFunction, InType->tp_name);
	return Descr;
}

PyObject* FNePyStaticFunctionDescriptor::Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	Py_ssize_t ArgCount = PyTuple_GET_SIZE(InArgs);
	if (ArgCount < 1)
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' of '%s' object needs an argument",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	PyObject* SelfObject = PyTuple_GET_ITEM(InArgs, 0);
	if (!PyType_Check(SelfObject))
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' requires a type but received a '%s'",
			FNePyDescriptorBase::DescrName(InSelf), Py_TYPE(SelfObject)->tp_name);
		return nullptr;
	}

	if (!PyType_IsSubtype((PyTypeObject*)SelfObject, InSelf->Type))
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' requires a subtype of '%s' but received '%s'",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name, ((PyTypeObject*)SelfObject)->tp_name);
		return nullptr;
	}

	FNePyObjectPtr Args = NePyStealReference(PyTuple_GetSlice(InArgs, 1, ArgCount));
	return FNePyCallable::CallMethod(InSelf->MethodDef, SelfObject, Args, InKwds);
}

PyObject* FNePyStaticFunctionDescriptor::DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	PyObject* Type = (PyObject*)InType;
	if (!Type && InObject)
	{
		Type = (PyObject*)Py_TYPE(InObject);
	}

	if (!Type)
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' for type '%s' needs either an object or a type",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	if (!PyType_Check(Type))
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' for type '%s' needs a type, not a '%s' as arg 2",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name, Py_TYPE(Type)->tp_name);
		return nullptr;
	}

	if (!PyType_IsSubtype((PyTypeObject*)Type, InSelf->Type))
	{
		PyErr_Format(PyExc_TypeError, "descriptor '%s' for type '%s' doesn't apply to type '%s'",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name, ((PyTypeObject*)Type)->tp_name);
		return nullptr;
	}

	// 调用对象函数的情况
	PyObject* Ret = FNePyCallable::New(InSelf->MethodDef, Type);
	return Ret;
}

#pragma endregion

#pragma region FNePyGeneratedFunctionDescriptor

static PyTypeObject PyType_NePyGeneratedFunctionDescriptor = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyGeneratedFunctionDescriptor", /* tp_name */
	sizeof(FNePyGeneratedFunctionDescriptor), /* tp_basicsize */
};

void FNePyGeneratedFunctionDescriptor::InitPyType()
{
	PyTypeObject* PyObjectType = &PyType_NePyGeneratedFunctionDescriptor;
	FNePyFunctionDescriptorBase::InitPyTypeCommon(PyObjectType);
	PyObjectType->tp_base = &PyType_NePyFunctionDescriptorBase;
	PyObjectType->tp_dealloc = (destructor)FNePyFunctionDescriptorBase::Dealloc;
	PyObjectType->tp_repr = (reprfunc)FNePyFunctionDescriptorBase::Repr;
	PyObjectType->tp_call = (ternaryfunc)FNePyGeneratedFunctionDescriptor::Call;
	PyObjectType->tp_descr_get = (descrgetfunc)FNePyGeneratedFunctionDescriptor::DescrGet;
	PyType_Ready(PyObjectType);
}

FNePyGeneratedFunctionDescriptor* FNePyGeneratedFunctionDescriptor::New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction)
{
	FNePyGeneratedFunctionDescriptor* Descr = (FNePyGeneratedFunctionDescriptor*)FNePyDescriptorBase::New(&PyType_NePyGeneratedFunctionDescriptor, InType, InName);
	if (!Descr)
	{
		return nullptr;
	}
	Descr->MethodDef = NePyGenUtil::FMethodDef::New(InBindFunction, InType->tp_name);
	return Descr;
}

PyObject* FNePyGeneratedFunctionDescriptor::Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	UNePyGeneratedFunction* GenFunc = CastChecked<UNePyGeneratedFunction>(InSelf->MethodDef->Func);
	if (!GenFunc->bEnableFastPath)
	{
		return FNePyFunctionDescriptor::Call(InSelf, InArgs, InKwds);
	}

	return PyFunction_Type.tp_call(GenFunc->PyFunc, InArgs, InKwds);
}

PyObject* FNePyGeneratedFunctionDescriptor::DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	UNePyGeneratedFunction* GenFunc = CastChecked<UNePyGeneratedFunction>(InSelf->MethodDef->Func);
	if (!GenFunc->bEnableFastPath)
	{
		return FNePyFunctionDescriptor::DescrGet(InSelf, InObject, InType);
	}

	return PyFunction_Type.tp_descr_get(GenFunc->PyFunc, InObject, (PyObject*)InType);
}

#pragma endregion

#pragma region FNePyGeneratedStaticFunctionDescriptor

static PyTypeObject PyType_NePyGeneratedStaticFunctionDescriptor = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyGeneratedStaticFunctionDescriptor", /* tp_name */
	sizeof(FNePyGeneratedStaticFunctionDescriptor), /* tp_basicsize */
};

void FNePyGeneratedStaticFunctionDescriptor::InitPyType()
{
	PyTypeObject* PyObjectType = &PyType_NePyGeneratedStaticFunctionDescriptor;
	FNePyFunctionDescriptorBase::InitPyTypeCommon(PyObjectType);
	PyObjectType->tp_base = &PyType_NePyFunctionDescriptorBase;
	PyObjectType->tp_dealloc = (destructor)FNePyFunctionDescriptorBase::Dealloc;
	PyObjectType->tp_repr = (reprfunc)FNePyFunctionDescriptorBase::Repr;
	PyObjectType->tp_call = (ternaryfunc)FNePyGeneratedStaticFunctionDescriptor::Call;
	PyObjectType->tp_descr_get = (descrgetfunc)FNePyGeneratedStaticFunctionDescriptor::DescrGet;
	PyType_Ready(PyObjectType);
}

FNePyGeneratedStaticFunctionDescriptor* FNePyGeneratedStaticFunctionDescriptor::New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction)
{
	FNePyGeneratedStaticFunctionDescriptor* Descr = (FNePyGeneratedStaticFunctionDescriptor*)FNePyDescriptorBase::New(&PyType_NePyGeneratedStaticFunctionDescriptor, InType, InName);
	if (!Descr)
	{
		return nullptr;
	}
	Descr->MethodDef = NePyGenUtil::FMethodDef::New(InBindFunction, InType->tp_name);
	return Descr;
}

PyObject* FNePyGeneratedStaticFunctionDescriptor::Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	UNePyGeneratedFunction* GenFunc = CastChecked<UNePyGeneratedFunction>(InSelf->MethodDef->Func);
	if (!GenFunc->bEnableFastPath)
	{
		return FNePyStaticFunctionDescriptor::Call(InSelf, InArgs, InKwds);
	}

	return PyFunction_Type.tp_call(GenFunc->PyFunc, InArgs, InKwds);
}

PyObject* FNePyGeneratedStaticFunctionDescriptor::DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType)
{
	if (!InSelf->MethodDef || !IsValid(InSelf->MethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' of '%s' is in invalid state",
			FNePyDescriptorBase::DescrName(InSelf), InSelf->Type->tp_name);
		return nullptr;
	}

	UNePyGeneratedFunction* GenFunc = CastChecked<UNePyGeneratedFunction>(InSelf->MethodDef->Func);
	if (!GenFunc->bEnableFastPath)
	{
		return FNePyStaticFunctionDescriptor::DescrGet(InSelf, InObject, InType);
	}

	Py_INCREF(GenFunc->PyFunc);
	return GenFunc->PyFunc;
}

#pragma endregion


FNePyDescriptorBase* NePyType_AddNewProperty(PyTypeObject* InPyType, const FProperty* InBindProperty, const char* InAttrName, bool bAddToDict)
{
	if (!InBindProperty)
	{
		return nullptr;
	}

	getter Getter = nullptr;
	setter Setter = nullptr;

	// For NePyObjectBase
	if (PyType_IsSubtype(InPyType, NePyObjectBaseGetType()))
	{
		Getter = (getter)NePyPropertyGetter_Object;
		// 当前不进行访问性限制，如果要判断UProperty的可设置性，可参见官方插件 PropertyAccessUitl.cpp CanSetPropertyValue
		Setter = (setter)NePyPropertySetter_Object;
	}
	else if (PyType_IsSubtype(InPyType, NePyStructBaseGetType()))
	{
		Getter = (getter)NePyPropertyGetter_Struct;
		Setter = (setter)NePyPropertySetter_Struct;
	}
	else
	{
		return nullptr;
	}

	// 创建描述器
	FNePyObjectPtr Descr;
	Descr = NePyStealReference(FNePyPropertyDescriptor::New(InPyType, InAttrName, InBindProperty, Getter, Setter));
	if (!Descr)
	{
		return nullptr;
	}

	if (bAddToDict)
	{
		if (PyObject* LastItem = PyDict_GetItemString(InPyType->tp_dict, InAttrName))
		{
			FString LastItremStr = NePyBase::PyObjectToString(LastItem);
			UE_LOG(LogNePython, Warning, TEXT("NePyType_AddNewProperty: attribute '%s' of '%s' [PyType=%p] already exists. Current property [%p], Existing item: %s"),
				UTF8_TO_TCHAR(InAttrName),
				UTF8_TO_TCHAR(InPyType->tp_name),
				InPyType,
				InBindProperty,
				*LastItremStr);
		}
		if (PyDict_SetItemString(InPyType->tp_dict, InAttrName, Descr) < 0)
		{
			return nullptr;
		}
		PyType_Modified(InPyType);
	}

	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 2)
	{
		FString PropertyInfo = NePyUtil::GetPropertyInfoString(InBindProperty, InAttrName);
		UE_LOG(LogNePython, Log, TEXT("    Added Property: %s [Descr=%p]"), 
			*PropertyInfo,
			Descr.Get());
	}

	return (FNePyDescriptorBase*)Descr.Release();
}

FNePyDescriptorBase* NePyType_AddNewFunction(PyTypeObject* InPyType, UFunction* InBindFunction, const char* InAttrName, bool bAddToDict)
{
	if (!IsValid(InBindFunction))
	{
		return nullptr;
	}

	bool bIsGeneratedFunc = InBindFunction->IsA<UNePyGeneratedFunction>();
	bool bIsStatic = EnumHasAnyFlags(InBindFunction->FunctionFlags, EFunctionFlags::FUNC_Static);

	FNePyObjectPtr Descr;
	if (bIsGeneratedFunc)
	{
		if (bIsStatic)
		{
			Descr = NePyStealReference(FNePyGeneratedStaticFunctionDescriptor::New(InPyType, InAttrName, InBindFunction));
		}
		else
		{
			Descr = NePyStealReference(FNePyGeneratedFunctionDescriptor::New(InPyType, InAttrName, InBindFunction));
		}
	}
	else
	{
		if (bIsStatic)
		{
			Descr = NePyStealReference(FNePyStaticFunctionDescriptor::New(InPyType, InAttrName, InBindFunction));
		}
		else
		{
			Descr = NePyStealReference(FNePyFunctionDescriptor::New(InPyType, InAttrName, InBindFunction));
		}
	}

	if (!Descr)
	{
		return nullptr;
	}

	if (bAddToDict)
	{
		if (PyObject* LastItem = PyDict_GetItemString(InPyType->tp_dict, InAttrName))
		{
			FString LastItremStr = NePyBase::PyObjectToString(LastItem);
			UE_LOG(LogNePython, Warning, TEXT("NePyType_AddNewFunction: attribute '%s' of '%s' [PyType=%p] already exists. Current function [%p], Existing item: %s"),
				UTF8_TO_TCHAR(InAttrName),
				UTF8_TO_TCHAR(InPyType->tp_name),
				InPyType,
				InBindFunction,
				*LastItremStr);
		}
		if (PyDict_SetItemString(InPyType->tp_dict, InAttrName, Descr) < 0)
		{
			return nullptr;
		}
		PyType_Modified(InPyType);
	}

	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 2)
	{
		FString FunctionInfo = NePyUtil::GetFunctionInfoString(InBindFunction, InAttrName);
		UE_LOG(LogNePython, Log, TEXT("    Added Function: %s [Descr=%p]"), 
			*FunctionInfo,
			Descr.Get());
		
		FString SignatureString = NePyUtil::GetFunctionSignatureString(InBindFunction, UTF8_TO_TCHAR(InAttrName), Level);
		if (!SignatureString.IsEmpty())
		{
			UE_LOG(LogNePython, Log, TEXT("      %s"), *SignatureString);
		}
	}

	return (FNePyDescriptorBase*)Descr.Release();
}

void NePyType_CleanupDescriptors(PyTypeObject* InPyType, bool bCleanupProps, bool bCleanupFuncs)
{
	TArray<FNePyObjectPtr> DeletingKeys;
	PyObject* Key;
	PyObject* Value;
	Py_ssize_t Pos = 0;

	while (PyDict_Next(InPyType->tp_dict, &Pos, &Key, &Value))
	{
		if (FNePyPropertyDescriptor* PropDescr = FNePyPropertyDescriptor::Check(Value))
		{
			if (bCleanupProps)
			{
				FNePyPropertyDescriptor::Reset(PropDescr);
				DeletingKeys.Add(NePyNewReference(Key));
			}
		}
		else if (FNePyFunctionDescriptorBase* FuncDescr = FNePyFunctionDescriptorBase::Check(Value))
		{
			if (bCleanupFuncs)
			{
				FNePyFunctionDescriptorBase::Reset(FuncDescr);
				DeletingKeys.Add(NePyNewReference(Key));
			}
		}
	}

	for (auto& IterKey : DeletingKeys)
	{
		PyDict_DelItem(InPyType->tp_dict, IterKey);
	}
}

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
// 当带有FieldNotify标记的属性发生变化时，触发通知
// 参考 UFieldNotificationLibrary::execSetPropertyValueAndBroadcast
// 参考 UE::FieldNotification::Private::SetPropertyValueAndBroadcast
template <bool bNotifyOtherFields>
static int NePyPropertySetter_ObjectFieldNotify(FNePyObjectBase* InObject, PyObject* InValue, const FProperty* InClosureProperty)
{
	if (!NePyBase::CheckValidAndSetPyErr(InObject, "Self"))
	{
		return -1;
	}

	FNePyPropValue TempValue(InClosureProperty);
	if (!TempValue.SetValue(InValue))
	{
		return -1;
	}

	void* SourceValuePtr = TempValue.Value;
	void* TargetValuePtr = InClosureProperty->ContainerPtrToValuePtr<void>(InObject->Value, 0);
	if (InClosureProperty->Identical(TargetValuePtr, SourceValuePtr))
	{
		return 0;
	}

	InClosureProperty->SetValue_InContainer(InObject->Value, SourceValuePtr);
	
	TScriptInterface<INotifyFieldValueChanged> NotifyFieldSelf(InObject->Value);
	if (NotifyFieldSelf.GetInterface() != nullptr)
	{
		const UE::FieldNotification::FFieldId FieldId = NotifyFieldSelf->GetFieldNotificationDescriptor().GetField(InObject->Value->GetClass(), InClosureProperty->GetFName());
		if (FieldId.IsValid())
		{
			NotifyFieldSelf->BroadcastFieldValueChanged(FieldId);
		}

		if (bNotifyOtherFields)
		{
			const UNePyGeneratedClass* PyClass = CastChecked<const UNePyGeneratedClass>(InObject->Value->GetClass());
			while (IsValid(PyClass))
			{
				if (const TArray<FName>* OtherFieldNotifyToTrigger = PyClass->OtherFieldNotifyToTriggerMap.Find(InClosureProperty->GetFName()))
				{
					for (const FName& OtherFieldName : *OtherFieldNotifyToTrigger)
					{
						const UE::FieldNotification::FFieldId OtherFieldId = NotifyFieldSelf->GetFieldNotificationDescriptor().GetField(InObject->Value->GetClass(), OtherFieldName);
						if (OtherFieldId.IsValid())
						{
							NotifyFieldSelf->BroadcastFieldValueChanged(OtherFieldId);
						}
					}
					break;
				}
				PyClass = Cast<UNePyGeneratedClass>(PyClass->GetSuperClass());
			}
		}
	}
	
	return 0;
}

void NePyType_DoAddFieldNotifySupportForDescr(FNePyPropertyDescriptor* PropDescr, const UBlueprintGeneratedClass* InBpClass)
{
	if (!PropDescr->Setter)
	{
		return;
	}

	if (!InBpClass->FieldNotifies.Contains(FFieldNotificationId(PropDescr->Prop->GetFName())))
	{
		return;
	}

	bool bNotifyOtherFields = false;
	const UNePyGeneratedClass* PyClass = Cast<const UNePyGeneratedClass>(InBpClass);
	if (PyClass)
	{
		const TArray<FName>* OtherFieldNotifyToTrigger = PyClass->OtherFieldNotifyToTriggerMap.Find(PropDescr->Prop->GetFName());
		if (OtherFieldNotifyToTrigger && OtherFieldNotifyToTrigger->Num() > 0)
		{
			bNotifyOtherFields = true;
		}
	}

	if (bNotifyOtherFields)
	{
		PropDescr->Setter = (setter)NePyPropertySetter_ObjectFieldNotify<true>;
	}
	else
	{
		PropDescr->Setter = (setter)NePyPropertySetter_ObjectFieldNotify<false>;
	}
}

void NePyType_AddFieldNotifySupportForType(PyTypeObject* PyType, const UClass* InClass)
{
	const UBlueprintGeneratedClass* BpClass = Cast<UBlueprintGeneratedClass>(InClass);
	if (!BpClass)
	{
		// 所有C++ Class都自行处理了属性变化时的通知问题（通过UE_MVVM_SET_PROPERTY_VALUE或UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED）
		// 我们只需要处理蓝图类和Python类即可，他们都是UBlueprintGeneratedClass。
		return;
	}

	if (BpClass->FieldNotifies.Num() == 0)
	{
		return;
	}

	PyObject* PyKey;
	PyObject* PyValue;
	Py_ssize_t PyPos = 0;
	while (PyDict_Next(PyType->tp_dict, &PyPos, &PyKey, &PyValue))
	{
		if (FNePyPropertyDescriptor* PropDescr = FNePyPropertyDescriptor::Check(PyValue))
		{
			NePyType_DoAddFieldNotifySupportForDescr(PropDescr, BpClass);
		}
	}
}

void NePyType_AddFieldNotifySupportForDescr(PyObject* PyDescr, const UClass* InClass)
{
	FNePyPropertyDescriptor* PropDescr = FNePyPropertyDescriptor::Check(PyDescr);
	if (!PropDescr)
	{
		return;
	}

	const UBlueprintGeneratedClass* BpClass = Cast<UBlueprintGeneratedClass>(InClass);
	if (!BpClass)
	{
		// 所有C++ Class都自行处理了属性变化时的通知问题（通过UE_MVVM_SET_PROPERTY_VALUE或UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED）
		// 我们只需要处理蓝图类和Python类即可，他们都是UBlueprintGeneratedClass。
		return;
	}

	if (BpClass->FieldNotifies.Num() == 0)
	{
		return;
	}

	NePyType_DoAddFieldNotifySupportForDescr(PropDescr, BpClass);
}
#endif
