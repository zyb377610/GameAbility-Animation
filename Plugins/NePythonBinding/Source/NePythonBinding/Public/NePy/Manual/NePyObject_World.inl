#pragma once
#include "NePyBase.h"
#include "NePyObjectBase.h"
#include "NePyAutoExportVersion.h"
#include "NePyStruct_Vector.h"
#include "NePyStruct_Rotator.h"
#include "NePyStruct_Transform.h"
#include "UObject/UObjectIterator.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/LevelScriptActor.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

PyObject* NePyWorld_GetAllObjects(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetAllObjects'"))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;

	FNePyObjectPtr Ret = NePyNewReference(PyList_New(0));
	for (TObjectIterator<UObject> Itr; Itr; ++Itr)
	{
		UObject* Object = *Itr;
		if (Object->GetWorld() != World)
		{
			continue;
		}

		FNePyObjectPtr PyObj = NePyStealReference(NePyBase::ToPy(Object));
		PyList_Append(Ret, PyObj);
	}
	return Ret;
}

PyObject* NePyWorld_GetAllActors(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetAllActors'"))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;

	FNePyObjectPtr Ret = NePyNewReference(PyList_New(0));
	for (TActorIterator<AActor> Itr(World); Itr; ++Itr)
	{
		UObject* Object = *Itr;
		FNePyObjectPtr PyObj = NePyStealReference(NePyBase::ToPy(Object));
		PyList_Append(Ret, PyObj);

	}
	return Ret;
}

PyObject* NePyWorld_GetLevels(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetLevels'"))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;

	FNePyObjectPtr Ret = NePyNewReference(PyList_New(0));

	for (ULevel* Level : World->GetLevels())
	{
		FNePyObjectPtr PyObj = NePyStealReference(NePyBase::ToPy(Level));
		PyList_Append(Ret, PyObj);

	}
	return Ret;
}

PyObject* NePyWorld_GetCurrentLevel(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetCurrentLevel'"))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;

	ULevel* Level = World->GetCurrentLevel();
	if (!Level)
	{
		Py_RETURN_NONE;
	}

	return NePyBase::ToPy(Level);
}

PyObject* NePyWorld_DestroyActor(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'DestroyActor'"))
	{
		return nullptr;
	}

	PyObject* PyActor;
	bool bNetForce = false;
	bool bShouldModifyLevel = true;
	if (!PyArg_ParseTuple(InArgs, "O|bb:DestroyActor", &PyActor, &bNetForce, &bShouldModifyLevel))
	{
		return nullptr;
	}

	AActor* Actor = NePyBase::ToCppObject<AActor>(PyActor);
	if (!Actor)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a AActor");
	}

	UWorld* World = (UWorld*)InSelf->Value;
	if (World->DestroyActor(Actor, bNetForce, bShouldModifyLevel))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject* NePyWorld_RemoveActor(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'RemoveActor'"))
	{
		return nullptr;
	}

	PyObject* PyActor;
	bool bShouldModifyLevel;
	if (!PyArg_ParseTuple(InArgs, "Ob:RemoveActor", &PyActor, &bShouldModifyLevel))
	{
		return nullptr;
	}

	AActor* Actor = NePyBase::ToCppObject<AActor>(PyActor);
	if (!Actor)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a AActor");
	}

	UWorld* World = (UWorld*)InSelf->Value;
	World->RemoveActor(Actor, bShouldModifyLevel);

	Py_RETURN_NONE;
}

PyObject* NePyWorld_SpawnActor(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SpawnActor'"))
	{
		return nullptr;
	}

	PyObject* PyClass;
	PyObject* PyLocation = nullptr;
	PyObject* PyRotation = nullptr;
	int32 SpawnCollisionHandlingOverride = (int32)ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const char* Name = nullptr;
	int32 NameMode = (int32)FActorSpawnParameters::ESpawnActorNameMode::Required_Fatal;
	if (!PyArg_ParseTuple(InArgs, "O|OOisi:ActorSpawn", &PyClass, &PyLocation, &PyRotation, &SpawnCollisionHandlingOverride, &Name, &NameMode))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass, AActor::StaticClass());
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass derived from AActor");
	}

	FVector Location = FVector::ZeroVector;
	if (PyLocation && PyLocation != Py_None)
	{
		if (!NePyBase::ToCpp(PyLocation, Location))
		{
			return PyErr_Format(PyExc_Exception, "location must be an FVector");
		}
	}

	FRotator Rotation = FRotator::ZeroRotator;
	if (PyRotation && PyRotation != Py_None)
	{
		if (!NePyBase::ToCpp(PyRotation, Rotation))
		{
			return PyErr_Format(PyExc_Exception, "location must be an FRotator");
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = (ESpawnActorCollisionHandlingMethod)SpawnCollisionHandlingOverride;
	if (Name)
	{
		SpawnParameters.Name = FName(UTF8_TO_TCHAR(Name));
	}
	if (NameMode)
	{
		SpawnParameters.NameMode = (FActorSpawnParameters::ESpawnActorNameMode)NameMode;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	AActor* Actor = World->SpawnActor(Class, &Location, &Rotation, SpawnParameters);
	if (!Actor)
	{
		return PyErr_Format(PyExc_Exception, "unable to spawn a new Actor");
	}

	return NePyBase::ToPy(Actor);
}

PyObject* NePyWorld_SpawnActorAbsolute(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SpawnActorAbsolute'"))
	{
		return nullptr;
	}

	PyObject* PyClass;
	PyObject* PyAbsoluteTransform;
	int32 SpawnCollisionHandlingOverride = (int32)ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const char* Name = nullptr;
	int32 NameMode = (int32)FActorSpawnParameters::ESpawnActorNameMode::Required_Fatal;
	if (!PyArg_ParseTuple(InArgs, "OO|isi:SpawnActorAbsolute", &PyClass, &PyAbsoluteTransform, &SpawnCollisionHandlingOverride, &Name, &NameMode))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass, AActor::StaticClass());
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass derived from AActor");
	}

	FTransform AbsoluteTransform;
	if (!NePyBase::ToCpp(PyAbsoluteTransform, AbsoluteTransform))
	{
		return PyErr_Format(PyExc_Exception, "AbsoluteTransform must be an FTransform");
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = (ESpawnActorCollisionHandlingMethod)SpawnCollisionHandlingOverride;
	if (Name)
	{
		SpawnParameters.Name = FName(UTF8_TO_TCHAR(Name));
	}
	if (NameMode)
	{
		SpawnParameters.NameMode = (FActorSpawnParameters::ESpawnActorNameMode)NameMode;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	AActor* Actor = World->SpawnActorAbsolute(Class, AbsoluteTransform, SpawnParameters);
	if (!Actor)
	{
		return PyErr_Format(PyExc_Exception, "unable to spawn a new Actor");
	}

	return NePyBase::ToPy(Actor);
}

PyObject* NePyWorld_DuplicateActor(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'DuplicateActor'"))
	{
		return nullptr;
	}

	PyObject* PyTemplateActor;
	PyObject* PyLocation = nullptr;
	PyObject* PyRotation = nullptr;
	int32 SpawnCollisionHandlingOverride = (int32)ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const char* Name = nullptr;
	int32 NameMode = (int32)FActorSpawnParameters::ESpawnActorNameMode::Required_Fatal;
	if (!PyArg_ParseTuple(InArgs, "O|OOisi:ActorDuplicate", &PyTemplateActor, &PyLocation, &PyRotation, &SpawnCollisionHandlingOverride, &Name, &NameMode))
	{
		return nullptr;
	}

	AActor* TemplateActor = NePyBase::ToCppObject<AActor>(PyTemplateActor);
	if (!TemplateActor)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a AActor");
	}

	FVector Location = FVector::ZeroVector;
	if (PyLocation && PyLocation != Py_None)
	{
		if (!NePyBase::ToCpp(PyLocation, Location))
		{
			return PyErr_Format(PyExc_Exception, "location must be an FVector");
		}
	}

	FRotator Rotation = FRotator::ZeroRotator;
	if (PyRotation && PyRotation != Py_None)
	{
		if (!NePyBase::ToCpp(PyRotation, Rotation))
		{
			return PyErr_Format(PyExc_Exception, "location must be an FRotator");
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Template = TemplateActor;
	SpawnParameters.SpawnCollisionHandlingOverride = (ESpawnActorCollisionHandlingMethod)SpawnCollisionHandlingOverride;
	if (Name)
	{
		SpawnParameters.Name = FName(UTF8_TO_TCHAR(Name));
	}
	if (NameMode)
	{
		SpawnParameters.NameMode = (FActorSpawnParameters::ESpawnActorNameMode)NameMode;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	AActor* Actor = World->SpawnActor(TemplateActor->GetClass(), &Location, &Rotation, SpawnParameters);
	if (!Actor)
	{
		return PyErr_Format(PyExc_Exception, "unable to spawn a new Actor");
	}

	return NePyBase::ToPy(Actor);
}

PyObject* NePyWorld_DuplicateActorAbsolute(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'DuplicateActorAbsolute'"))
	{
		return nullptr;
	}

	PyObject* PyTemplateActor;
	PyObject* PyAbsoluteTransform;
	int32 SpawnCollisionHandlingOverride = (int32)ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const char* Name = nullptr;
	int32 NameMode = (int32)FActorSpawnParameters::ESpawnActorNameMode::Required_Fatal;
	if (!PyArg_ParseTuple(InArgs, "OO|isi:SpawnActorAbsolute", &PyTemplateActor, &PyAbsoluteTransform, &SpawnCollisionHandlingOverride, &Name, &NameMode))
	{
		return nullptr;
	}

	AActor* TemplateActor = NePyBase::ToCppObject<AActor>(PyTemplateActor);
	if (!TemplateActor)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a AActor");
	}

	FTransform AbsoluteTransform;
	if (!NePyBase::ToCpp(PyAbsoluteTransform, AbsoluteTransform))
	{
		return PyErr_Format(PyExc_Exception, "AbsoluteTransform must be an FTransform");
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Template = TemplateActor;
	SpawnParameters.SpawnCollisionHandlingOverride = (ESpawnActorCollisionHandlingMethod)SpawnCollisionHandlingOverride;
	if (Name)
	{
		SpawnParameters.Name = FName(UTF8_TO_TCHAR(Name));
	}
	if (NameMode)
	{
		SpawnParameters.NameMode = (FActorSpawnParameters::ESpawnActorNameMode)NameMode;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	AActor* Actor = World->SpawnActorAbsolute(TemplateActor->GetClass(), AbsoluteTransform, SpawnParameters);
	if (!Actor)
	{
		return PyErr_Format(PyExc_Exception, "unable to spawn a new Actor");
	}

	return NePyBase::ToPy(Actor);
}

PyObject* NePyWorld_GetPlayerController(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetPlayerController'"))
	{
		return nullptr;
	}

	int ControllerId = 0;
	if (!PyArg_ParseTuple(InArgs, "|i:GetPlayerController", &ControllerId))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	APlayerController* Controller = UGameplayStatics::GetPlayerController(World, ControllerId);
	if (!Controller)
	{
		return PyErr_Format(PyExc_Exception, "unable to retrieve controller %d", ControllerId);
	}

	return NePyBase::ToPy(Controller);
}

PyObject* NePyWorld_GetWorldType(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetWorldType'"))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	PyObject* RetValue = PyLong_FromUnsignedLong((uint8)World->WorldType);
	return RetValue;
}

PyObject* NePyWorld_GetLevelScriptActor(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetLevelScriptActor'"))
	{
		return nullptr;
	}

	PyObject* PyLevel = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|O:GetLevelScriptActor", &PyLevel))
	{
		return nullptr;
	}

	ULevel* OwnerLevel = nullptr;
	if (PyLevel && PyLevel != Py_None)
	{
		FNePyObjectBase* PyLevelTemp = NePyObjectBaseCheck(PyLevel);
		if (PyLevelTemp && !FNePyHouseKeeper::Get().IsValid(PyLevelTemp))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'OwnerLevel' underlying UObject is invalid");
			return nullptr;
		}
		if (!PyLevelTemp || !Cast<ULevel>(PyLevelTemp->Value))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'OwnerLevel' must be 'Level'");
			return nullptr;
		}
		OwnerLevel = Cast<ULevel>(PyLevelTemp->Value);
	}

	UWorld* World = (UWorld*)InSelf->Value;
	ALevelScriptActor* LevelScriptActor = World->GetLevelScriptActor(OwnerLevel);
	return NePyBase::ToPy(LevelScriptActor);
}

PyObject* NePyWorld_GetCurrentLevelScriptActor(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetCurrentLevelScriptActor'"))
	{
		return nullptr;
	}

	UWorld* World = (UWorld*)InSelf->Value;
	ALevelScriptActor* LevelScriptActor = World->GetLevelScriptActor();
	return NePyBase::ToPy(LevelScriptActor);
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetAllObjects", NePyCFunctionCast(&NePyWorld_GetAllObjects), METH_NOARGS, "(self) -> list[Object]"}, \
{"GetAllActors", NePyCFunctionCast(&NePyWorld_GetAllActors), METH_NOARGS, "(self) -> list[Actor]"}, \
{"GetLevels", NePyCFunctionCast(&NePyWorld_GetLevels), METH_NOARGS, "(self) -> list[Level]"}, \
{"GetCurrentLevel", NePyCFunctionCast(&NePyWorld_GetCurrentLevel), METH_NOARGS, "(self) -> Level"}, \
{"DestroyActor", NePyCFunctionCast(&NePyWorld_DestroyActor), METH_VARARGS, "(self, ThisActor: Actor, bNetForce: bool = ..., bShouldModifyLevel: bool = ...) -> bool"}, \
{"RemoveActor", NePyCFunctionCast(&NePyWorld_RemoveActor), METH_VARARGS, "(self, ThisActor: Actor, bShouldModifyLevel: bool) -> None"}, \
{"SpawnActor", NePyCFunctionCast(&NePyWorld_SpawnActor), METH_VARARGS, "[T: Actor](self, ActorClass: type[T] | TSubclassOf[T], Location: Vector = ..., Rotation: Rotator = ..., SpawnCollisionHandlingOverride: ESpawnActorCollisionHandlingMethod = ..., Name: str = ..., NameMode: int = ...) -> T;(self, ActorClass: Class, Location: Vector = ..., Rotation: Rotator = ..., SpawnCollisionHandlingOverride: ESpawnActorCollisionHandlingMethod = ..., Name: str = ..., NameMode: int = ...) -> Actor"}, \
{"SpawnActorAbsolute", NePyCFunctionCast(&NePyWorld_SpawnActorAbsolute), METH_VARARGS, "[T: Actor](self, ActorClass: type[T] | TSubclassOf[T], AbsoluteTransform: Transform, SpawnCollisionHandlingOverride: ESpawnActorCollisionHandlingMethod = ..., Name: str = ..., NameMode: int = ...) -> T;(self, ActorClass: Class, AbsoluteTransform: Transform, SpawnCollisionHandlingOverride: ESpawnActorCollisionHandlingMethod = ..., Name: str = ..., NameMode: int = ...) -> Actor"}, \
{"DuplicateActor", NePyCFunctionCast(&NePyWorld_DuplicateActor), METH_VARARGS, "(self, ActorTemplate: Actor, Location: Vector = ..., Rotation: Rotator = ..., SpawnCollisionHandlingOverride: ESpawnActorCollisionHandlingMethod = ..., Name: str = ..., NameMode: int = ...) -> Actor"}, \
{"DuplicateActorAbsolute", NePyCFunctionCast(&NePyWorld_DuplicateActorAbsolute), METH_VARARGS, "(self, ActorTemplate: Actor, AbsoluteTransform: Transform, SpawnCollisionHandlingOverride: ESpawnActorCollisionHandlingMethod = ..., Name: str = ..., NameMode: int = ...) -> Actor"}, \
{"GetPlayerController", NePyCFunctionCast(&NePyWorld_GetPlayerController), METH_VARARGS, "(self) -> PlayerController"}, \
{"GetWorldType", NePyCFunctionCast(&NePyWorld_GetWorldType), METH_NOARGS, "(self) -> int"}, \
{"GetLevelScriptActor", NePyCFunctionCast(&NePyWorld_GetLevelScriptActor), METH_VARARGS, "(self, Level: Level = ...) -> LevelScriptActor"}, \
{"GetCurrentLevelScriptActor", NePyCFunctionCast(&NePyWorld_GetCurrentLevelScriptActor), METH_NOARGS, "(self) -> LevelScriptActor"}, \

