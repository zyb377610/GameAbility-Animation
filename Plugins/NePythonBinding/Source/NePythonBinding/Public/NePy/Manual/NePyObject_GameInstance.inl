#pragma once
#include "NePyBase.h"
#include "NePyGameInstance.h"
#include "NePyTimerManagerWrapper.h"
#include "Engine/GameInstance.h"

PyObject* NePyGameInstance_GetPyProxy(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetPyProxy'"))
	{
		return nullptr;
	}

	if (!InSelf->Value->IsA(UNePyGameInstance::StaticClass()))
	{
		PyErr_Format(PyExc_RuntimeError, "self(%p) is not a UNePyGameInstance", InSelf);
		return nullptr;
	}

	UNePyGameInstance* GameInstance = (UNePyGameInstance*)InSelf->Value;
	PyObject* PyInstance = GameInstance->GetPyProxy();
	if (PyInstance)
	{
		Py_INCREF(PyInstance);
		return PyInstance;
	}

	Py_RETURN_NONE;
}

PyObject* NePyGameInstance_GetWorld(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetWorld'"))
	{
		return nullptr;
	}

	UGameInstance* GameInstance = (UGameInstance*)InSelf->Value;
	UWorld* World = GameInstance->GetWorld();
	PyObject* PyWorld = NePyBase::ToPy(World);
	return PyWorld;
}

PyObject* NePyGameInstance_GetTimerManager(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetTimerManager'"))
	{
		return nullptr;
	}

	UGameInstance* GameInstance = (UGameInstance*)InSelf->Value;
	FTimerManager& TimerManager = GameInstance->GetTimerManager();
	PyObject* PyTimerManager = FNePyTimerManagerWrapper::New(GameInstance, &TimerManager);
	return PyTimerManager;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetPyProxy", NePyCFunctionCast(&NePyGameInstance_GetPyProxy), METH_NOARGS, "(self) -> object"}, \
{"GetWorld", NePyCFunctionCast(&NePyGameInstance_GetWorld), METH_NOARGS, "(self) -> World"}, \
{"GetTimerManager", NePyCFunctionCast(&NePyGameInstance_GetTimerManager), METH_NOARGS, "(self) -> TimerManagerWrapper"}, \

