#pragma once
#include "NePyBase.h"
#include "Components/ActorComponent.h"

PyObject* NePyActorComponent_IsRegistered(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'IsRegistered'"))
	{
		return nullptr;
	}

	UActorComponent* Component = (UActorComponent*)InSelf->Value;
	if (Component->IsRegistered())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* NePyActorComponent_RegisterComponent(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'RegisterComponent'"))
	{
		return nullptr;
	}

	UActorComponent* Component = (UActorComponent*)InSelf->Value;
	Component->RegisterComponent();
	Py_RETURN_NONE;
}

PyObject* NePyActorComponent_UnregisterComponent(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'UnregisterComponent'"))
	{
		return nullptr;
	}

	UActorComponent* Component = (UActorComponent*)InSelf->Value;
	Component->UnregisterComponent();
	Py_RETURN_NONE;
}

PyObject* NePyActorComponent_GetWorld(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetWorld'"))
	{
		return nullptr;
	}

	UActorComponent* Component = (UActorComponent*)InSelf->Value;
	UWorld* World = Component->GetWorld();
	PyObject* PyWorld = NePyBase::ToPy((UObject*)World);
	return PyWorld;
}

PyObject* NePyActorComponent_DestroySelfComponent(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'DestroySelfComponent'"))
	{
		return nullptr;
	}

	UActorComponent* Component = (UActorComponent*)InSelf->Value;
	Component->DestroyComponent();
	Py_RETURN_NONE;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"IsRegistered", NePyCFunctionCast(&NePyActorComponent_IsRegistered), METH_NOARGS, "(self) -> bool"}, \
{"RegisterComponent", NePyCFunctionCast(&NePyActorComponent_RegisterComponent), METH_NOARGS, "(self) -> None"}, \
{"UnregisterComponent", NePyCFunctionCast(&NePyActorComponent_UnregisterComponent), METH_NOARGS, "(self) -> None"}, \
{"GetWorld", NePyCFunctionCast(&NePyActorComponent_GetWorld), METH_NOARGS, "(self) -> World"}, \
{"DestroySelfComponent", NePyCFunctionCast(&NePyActorComponent_DestroySelfComponent), METH_NOARGS, "(self) -> None"}, \
