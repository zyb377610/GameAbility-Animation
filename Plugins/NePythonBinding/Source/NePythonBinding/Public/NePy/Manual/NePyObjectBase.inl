#pragma once
#include "UObject/ObjectMacros.h"
#include "UObject/Package.h"
#include "Components/ActorComponent.h"
#include "Engine/UserDefinedEnum.h"
#include "Containers/StringConv.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"
#include "NePyHouseKeeper.h"

PyObject* NePyObject_GetUniqueID(FNePyObjectBase* InSelf)
{
	uint32 UniqueID = 0xffffffff;
	if (FNePyHouseKeeper::Get().IsValid(InSelf))
	{
		UniqueID = InSelf->Value->GetUniqueID();
	}

	return PyLong_FromUnsignedLongLong(UniqueID);
}

PyObject* NePyObject_GetClass(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetClass'"))
	{
		return nullptr;
	}

	return NePyBase::ToPy(InSelf->Value->GetClass());
}

PyObject* NePyObject_GetFlags(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetFlags'"))
	{
		return nullptr;
	}

	return PyLong_FromUnsignedLongLong((uint64)InSelf->Value->GetFlags());
}

PyObject* NePyObject_SetFlags(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetFlags'"))
	{
		return nullptr;
	}

	uint64 Flags;
	PyObject* PyReset = nullptr;
	if (!PyArg_ParseTuple(InArgs, "K|O", &Flags, &PyReset))
	{
		return nullptr;
	}

	if (PyReset && PyObject_IsTrue(PyReset))
	{
		InSelf->Value->ClearFlags(InSelf->Value->GetFlags());
	}

	InSelf->Value->SetFlags((EObjectFlags)Flags);
	Py_RETURN_NONE;
}

PyObject* NePyObject_ClearFlags(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'ClearFlags'"))
	{
		return nullptr;
	}

	uint64 Flags;
	if (!PyArg_ParseTuple(InArgs, "K", &Flags))
	{
		return nullptr;
	}

	InSelf->Value->ClearFlags((EObjectFlags)Flags);
	Py_RETURN_NONE;
}

PyObject* NePyObject_ResetFlags(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'ResetFlags'"))
	{
		return nullptr;
	}

	InSelf->Value->ClearFlags(InSelf->Value->GetFlags());
	Py_RETURN_NONE;
}

PyObject* NePyObject_GetOuter(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetOuter'"))
	{
		return nullptr;
	}

	UObject* Outer = InSelf->Value->GetOuter();
	if (!Outer)
	{
		Py_RETURN_NONE;
	}

	return NePyBase::ToPy(Outer);
}

PyObject* NePyObject_SetOuter(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetOuter'"))
	{
		return nullptr;
	}

	PyObject* PyOuter;
	if (!PyArg_ParseTuple(InArgs, "O", &PyOuter))
	{
		return nullptr;
	}

	UPackage* Package = NePyBase::ToCppObject<UPackage>(PyOuter);
	if (!Package)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UPackage");
	}

	if (!InSelf->Value->Rename(nullptr, Package, REN_Test))
	{
		return PyErr_Format(PyExc_Exception, "cannot move to package %s", TCHAR_TO_UTF8(*Package->GetPathName()));
	}

	if (InSelf->Value->Rename(nullptr, Package))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject* NePyObject_GetOutermost(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetOutermost'"))
	{
		return nullptr;
	}

	UObject* Outermost = InSelf->Value->GetOutermost();
	if (!Outermost)
	{
		Py_RETURN_NONE;
	}

	return NePyBase::ToPy(Outermost);
}

PyObject* NePyObject_CreateDefaultSubobject(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'CreateDefaultSubobject'"))
	{
		return nullptr;
	}

	PyObject* PyClass;
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "Os", &PyClass, &Name))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass, UObject::StaticClass());
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass derived from UObject");
	}

	UObject* Ret = InSelf->Value->CreateDefaultSubobject(FName(UTF8_TO_TCHAR(Name)), Class, Class, true, false);
	return NePyBase::ToPy(Ret);
}

PyObject* NePyObject_IsValid(FNePyObjectBase* InSelf)
{
	if (FNePyHouseKeeper::Get().IsValid(InSelf))
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* NePyObject_IsA(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'IsA'"))
	{
		return nullptr;
	}

	UClass* Class = nullptr;
	if (!NePyBase::ToCpp(InArg, Class, nullptr))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InType' must have type 'Class'");
		return nullptr;
	}

	if (InSelf->Value->IsA(Class))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject* NePyObject_Implements(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'Implements'"))
	{
		return nullptr;
	}

	UClass* Class = nullptr;
	if (!NePyBase::ToCpp(InArg, Class, nullptr))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InType' must have type 'Class'");
		return nullptr;
	}

	if (InSelf->Value->GetClass()->ImplementsInterface(Class))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject* NePyObject_IsClassDefaultObject(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'IsClassDefaultObject'"))
	{
		return nullptr;
	}

	return PyBool_FromLong(InSelf->Value->HasAnyFlags(RF_ClassDefaultObject));
}

PyObject* NePyObject_IsArchetypeObject(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'IsArchetypeObject'"))
	{
		return nullptr;
	}

	return PyBool_FromLong(InSelf->Value->HasAnyFlags(RF_ArchetypeObject));
}

PyObject* NePyObject_GetName(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetName'"))
	{
		return nullptr;
	}

	return PyUnicode_FromString(TCHAR_TO_UTF8(*(InSelf->Value->GetName())));
}

PyObject* NePyObject_SetName(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetName'"))
	{
		return nullptr;
	}

	char* Name;
	if (!PyArg_ParseTuple(InArgs, "s", &Name))
	{
		return nullptr;
	}

	if (!InSelf->Value->Rename(UTF8_TO_TCHAR(Name), InSelf->Value->GetOutermost(), REN_Test))
	{
		return PyErr_Format(PyExc_Exception, "cannot set name %s", Name);
	}

	if (InSelf->Value->Rename(UTF8_TO_TCHAR(Name)))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject* NePyObject_GetFullName(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetFullName'"))
	{
		return nullptr;
	}

	return PyUnicode_FromString(TCHAR_TO_UTF8(*(InSelf->Value->GetFullName())));
}

PyObject* NePyObject_GetPathName(FNePyObjectBase* InSelf)
{

	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetPathName'"))
	{
		return nullptr;
	}

	return PyUnicode_FromString(TCHAR_TO_UTF8(*(InSelf->Value->GetPathName())));
}

PyObject* NePyObject_BindEvent(FNePyObjectBase* InSelf, PyObject* InArgs)
{

	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindEvent'"))
	{
		return nullptr;
	}

	char* EventName;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "sO:BindEvent", &EventName, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	return NePyObjectBaseBindEvent(InSelf, FString(EventName), PyCallable, true);
}

PyObject* NePyObject_UnbindEvent(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'UnbindEvent'"))
	{
		return nullptr;
	}

	char* EventName;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "sO:UnbindEvent", &EventName, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	return NePyObjectBaseUnbindEvent(InSelf, FString(EventName), PyCallable, true);
}

PyObject* NePyObject_UnbindAllEvent(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'UnbindAllEvent'"))
	{
		return nullptr;
	}

	FString EventName;
	if (!NePyBase::ToCpp(InArg, EventName))
	{
		return PyErr_Format(PyExc_Exception, "argument is not a str");
	}

	return NePyObjectBaseUnbindAllEvent(InSelf, EventName, true);
}

PyObject* NePyObject_SetProperty(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetProperty'"))
	{
		return nullptr;
	}

	char* PropName;
	PyObject* PropValue;
	int Index = 0;
	if (!PyArg_ParseTuple(InArgs, "sO|i:SetProperty", &PropName, &PropValue, &Index))
	{
		return nullptr;
	}

	UStruct* Struct = nullptr;
	if (InSelf->Value->IsA<UStruct>())
	{
		Struct = (UStruct*)InSelf->Value;
	}
	else
	{
		Struct = (UStruct*)InSelf->Value->GetClass();
	}

	auto* Prop = Struct->FindPropertyByName(FName(UTF8_TO_TCHAR(PropName)));
	if (!Prop)
	{
		return PyErr_Format(PyExc_Exception, "unable to find property %s", PropName);
	}

	if (!NePyBase::TryConvertPyObjectToFPropertyInContainer(PropValue, Prop, InSelf->Value, Index, InSelf->Value))
	{
		return PyErr_Format(PyExc_Exception, "unable to set property %s", PropName);
	}

	Py_RETURN_NONE;
}

// 判断UE4对象当前是否由Python来管理生命周期
// 如果对象由Python管理生命周期，则只要Python持有着该对象，它就不会被UE4垃圾回收掉
PyObject* NePyObject_IsOwnedByPython(FNePyObjectBase* InSelf)
{
	if (FNePyHouseKeeper::Get().IsOwnedByPython(InSelf))
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

// 将所有权转移给Python，让Python来管理对象的生命周期
PyObject* NePyObject_OwnByPython(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'OwnByPython'"))
	{
		return nullptr;
	}

	FNePyHouseKeeper::Get().ChangeOwnershipToPython(InSelf);
	Py_RETURN_NONE;
}

// 将所有权转移给UE4，让UE4来管理对象的生命周期
PyObject* NePyObject_DisownByPython(FNePyObjectBase* InSelf)
{
	FNePyHouseKeeper::Get().ChangeOwnershipToCpp(InSelf);
	Py_RETURN_NONE;
}

PyObject* NePyObject_AsDict(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'AsDict'"))
	{
		return nullptr;
	}

	bool bDisplayName = false;
	bool bIncludeSuper = true;
	if (!PyArg_ParseTuple(InArgs, "|bb:AsDict", &bDisplayName, &bIncludeSuper))
	{
		return nullptr;
	}

	const UClass* Class = InSelf->Value->GetClass();

	PyObject* PyRet = PyDict_New();
	for (TFieldIterator<FProperty> PropIt(Class, bIncludeSuper ? EFieldIteratorFlags::IncludeSuper : EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Prop = *PropIt;
		FNePyObjectPtr PyPropValue = NePyStealReference(NePyBase::TryConvertFPropertyToPyObjectInContainer(Prop, InSelf->Value, 0, InSelf->Value));
		if (!PyPropValue)
		{
			continue;
		}

		FString PropName = Class->GetAuthoredNameForField(Prop);
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

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetUniqueID", NePyCFunctionCast(&NePyObject_GetUniqueID), METH_NOARGS, "(self) -> int"}, \
{"GetClass", NePyCFunctionCast(&NePyObject_GetClass), METH_NOARGS, "(self) -> Class"}, \
{"GetFlags", NePyCFunctionCast(&NePyObject_GetFlags), METH_NOARGS, "(self) -> int"}, \
{"SetFlags", NePyCFunctionCast(&NePyObject_SetFlags), METH_VARARGS, "(self, Flags: int, Reset: bool) -> None"}, \
{"ClearFlags", NePyCFunctionCast(&NePyObject_ClearFlags), METH_VARARGS, "(self, Flags: int) -> None"}, \
{"ResetFlags", NePyCFunctionCast(&NePyObject_ResetFlags), METH_NOARGS, "(self) -> None"}, \
{"GetOuter", NePyCFunctionCast(&NePyObject_GetOuter), METH_NOARGS, "(self) -> Object"}, \
{"SetOuter", NePyCFunctionCast(&NePyObject_SetOuter), METH_VARARGS, "(self, InPackage: Package) -> bool"}, \
{"GetOutermost", NePyCFunctionCast(&NePyObject_GetOutermost), METH_NOARGS, "(self) -> Package"}, \
{"CreateDefaultSubobject", NePyCFunctionCast(&NePyObject_CreateDefaultSubobject), METH_VARARGS, "(self, Class: Class, Name: str) -> Object"}, \
{"IsValid", NePyCFunctionCast(&NePyObject_IsValid), METH_NOARGS, "(self) -> bool"}, \
{"IsA", NePyCFunctionCast(&NePyObject_IsA), METH_O, "[T: Object](self, InClass: TSubclassOf[T] | type[T]) -> typing.TypeGuard[T];(self, InClass: Class) -> bool"}, \
{"Implements", NePyCFunctionCast(&NePyObject_Implements), METH_O, "[T: Interface](self, InClass: TSubclassOf[T] | type[T]) -> typing.TypeGuard[T];(self, InClass: Class) -> bool"}, \
{"IsClassDefaultObject", NePyCFunctionCast(&NePyObject_IsClassDefaultObject), METH_NOARGS, "(self) -> bool"}, \
{"IsArchetypeObject", NePyCFunctionCast(&NePyObject_IsArchetypeObject), METH_NOARGS, "(self) -> bool"}, \
{"GetName", NePyCFunctionCast(&NePyObject_GetName), METH_NOARGS, "(self) -> str"}, \
{"SetName", NePyCFunctionCast(&NePyObject_SetName), METH_VARARGS, "(self, InName: str) -> bool"}, \
{"GetFullName", NePyCFunctionCast(&NePyObject_GetFullName), METH_NOARGS, "(self) -> str"}, \
{"GetPathName", NePyCFunctionCast(&NePyObject_GetPathName), METH_NOARGS, "(self) -> str"}, \
{"BindEvent", NePyCFunctionCast(&NePyObject_BindEvent), METH_VARARGS, "(self, EventName: str, Callback: typing.Callable) -> None"}, \
{"UnbindEvent", NePyCFunctionCast(&NePyObject_UnbindEvent), METH_VARARGS, "(self, EventName: str, Callback: typing.Callable) -> None"}, \
{"UnbindAllEvent", NePyCFunctionCast(&NePyObject_UnbindAllEvent), METH_O, "(self, EventName: str) -> None"}, \
{"SetProperty", NePyCFunctionCast(&NePyObject_SetProperty), METH_VARARGS, "(self, PropName: str, PropValue: typing.Any) -> None"}, \
{"IsOwnedByPython", NePyCFunctionCast(&NePyObject_IsOwnedByPython), METH_NOARGS, "(self) -> bool"}, \
{"OwnByPython", NePyCFunctionCast(&NePyObject_OwnByPython), METH_NOARGS, "(self) -> None"}, \
{"DisownByPython", NePyCFunctionCast(&NePyObject_DisownByPython), METH_NOARGS, "(self) -> None"}, \
{"AsDict", NePyCFunctionCast(&NePyObject_AsDict), METH_VARARGS, "(self, bDisplayName: bool = ..., bIncludeSuper: bool = ...) -> dict[str, typing.Any]"}, \
