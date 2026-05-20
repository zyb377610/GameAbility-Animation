#pragma once
#include "NePyBase.h"
#include "Components/SceneComponent.h"

PyObject* NePySceneComponent_SetupAttachment(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetupAttachment'"))
	{
		return nullptr;
	}

	PyObject* PyParentComponent;
	char* PySocketName = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O|s:SetupAttachment", &PyParentComponent, &PySocketName))
	{
		return nullptr;
	}

	USceneComponent* ParentComponent = NePyBase::ToCppObject<USceneComponent>(PyParentComponent);
	if (!ParentComponent)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a USceneComponent");
	}

	FName SocketName = NAME_None;
	if (PySocketName)
	{
		SocketName = FName(UTF8_TO_TCHAR(PySocketName));
	}

	USceneComponent* Component = (USceneComponent*)InSelf->Value;
	Component->SetupAttachment(ParentComponent, SocketName);

	Py_RETURN_NONE;
}

PyObject* NePySceneComponent_GetRelativeLocation(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetRelativeLocation'"))
	{
		return nullptr;
	}

	USceneComponent* Component = (USceneComponent*)InSelf->Value;
	auto Vector = Component->GetRelativeLocation();
	PyObject* PyVector = nullptr;
	NePyBase::ToPy(Vector, PyVector);
	return PyVector;
}

PyObject* NePySceneComponent_SetRelativeRotation(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetRelativeRotation'"))
	{
		return nullptr;
	}

	PyObject* PyArgs[3] = { nullptr, nullptr, nullptr };
	if (!PyArg_ParseTuple(InArgs, "O|OO:SetRelativeRotation", &PyArgs[0], &PyArgs[1], &PyArgs[2]))
	{
		return nullptr;
	}

	FRotator* NewRotation;
	if (FNePyStruct_Rotator* PyNewRotation = NePyStructCheck_Rotator(PyArgs[0]))
	{
		NewRotation = (FRotator*)PyNewRotation->Value;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'NewRotation' must have type 'Rotator'");
		return nullptr;
	}

	bool bSweep = false;
	if (PyArgs[1] && !NePyBase::ToCpp(PyArgs[1], bSweep))
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'bSweep' must have type 'bool'");
		return nullptr;
	}

	FHitResult SweepHitResult;

	bool bTeleport = false;
	if (PyArgs[2] && !NePyBase::ToCpp(PyArgs[2], bTeleport))
	{
		PyErr_SetString(PyExc_TypeError, "arg3 'bTeleport' must have type 'bool'");
		return nullptr;
	}

	USceneComponent* Component = (USceneComponent*)InSelf->Value;
	Component->K2_SetRelativeRotation(*NewRotation, bSweep, SweepHitResult, bTeleport);

	PyObject* PyRetVal0 = NePyStructNew_HitResult(SweepHitResult);
	return PyRetVal0;
}

PyObject* NePySceneComponent_GetRelativeRotation(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetRelativeRotation'"))
	{
		return nullptr;
	}

	USceneComponent* Component = (USceneComponent*)InSelf->Value;
	auto Rotator = Component->GetRelativeRotation();
	PyObject* PyRotator = nullptr;
	NePyBase::ToPy(Rotator, PyRotator);
	return PyRotator;
}

PyObject* NePySceneComponent_SetRelativeLocation(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetRelativeLocation'"))
	{
		return nullptr;
	}

	PyObject* PyArgs[3] = { nullptr, nullptr, nullptr };
	if (!PyArg_ParseTuple(InArgs, "O|OO:SetRelativeRotation", &PyArgs[0], &PyArgs[1], &PyArgs[2]))
	{
		return nullptr;
	}

	FVector* NewLocation;
	if (FNePyStruct_Vector* PyNewLocation = NePyStructCheck_Vector(PyArgs[0]))
	{
		NewLocation = (FVector*)PyNewLocation->Value;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'NewLocation' must have type 'Vector'");
		return nullptr;
	}

	bool bSweep = false;
	if (PyArgs[1] && !NePyBase::ToCpp(PyArgs[1], bSweep))
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'bSweep' must have type 'bool'");
		return nullptr;
	}

	FHitResult SweepHitResult;

	bool bTeleport = false;
	if (PyArgs[2] && !NePyBase::ToCpp(PyArgs[2], bTeleport))
	{
		PyErr_SetString(PyExc_TypeError, "arg3 'bTeleport' must have type 'bool'");
		return nullptr;
	}

	USceneComponent* Component = (USceneComponent*)InSelf->Value;
	Component->K2_SetRelativeLocation(*NewLocation, bSweep, SweepHitResult, bTeleport);

	PyObject* PyRetVal0 = NePyStructNew_HitResult(SweepHitResult);
	return PyRetVal0;
}

PyObject* NePySceneComponent_GetRelativeScale3D(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetRelativeScale3D'"))
	{
		return nullptr;
	}

	USceneComponent* Component = (USceneComponent*)InSelf->Value;
	auto Vector = Component->GetRelativeScale3D();
	PyObject* PyVector = nullptr;
	NePyBase::ToPy(Vector, PyVector);
	return PyVector;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"SetupAttachment", NePyCFunctionCast(&NePySceneComponent_SetupAttachment), METH_VARARGS, "(self, ParentComponent: SceneComponent, SocketName: str) -> None"}, \
{"GetRelativeLocation", NePyCFunctionCast(&NePySceneComponent_GetRelativeLocation), METH_NOARGS, "(self) -> Vector"}, \
{"SetRelativeLocation", NePyCFunctionCast(&NePySceneComponent_SetRelativeLocation), METH_VARARGS, "(self, NewLocation: Vector, bSweep: bool = ..., bTeleport: bool = ...) -> HitResult"}, \
{"GetRelativeRotation", NePyCFunctionCast(&NePySceneComponent_GetRelativeRotation), METH_NOARGS, "(self) -> Rotator" }, \
{"SetRelativeRotation", NePyCFunctionCast(&NePySceneComponent_SetRelativeRotation), METH_VARARGS, "(self, NewRotation: Rotator, bSweep: bool = ..., bTeleport: bool = ...) -> HitResult"}, \
{"GetRelativeScale3D", NePyCFunctionCast(&NePySceneComponent_GetRelativeScale3D), METH_NOARGS, "(self) -> Vector" }, \
