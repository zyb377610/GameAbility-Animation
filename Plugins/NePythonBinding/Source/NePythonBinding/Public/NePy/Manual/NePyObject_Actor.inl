#pragma once
#include "NePyBase.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"

PyObject* NePyActor_DispatchBeginPlay(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'DispatchBeginPlay'"))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;

	Actor->DispatchBeginPlay();

	Py_RETURN_NONE;
}

PyObject* NePyActor_GetComponents(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetComponents'"))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;
	TInlineComponentArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	return NePyBase::ToPy(Components);
}

PyObject* NePyActor_GetComponent(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetComponent'"))
	{
		return nullptr;
	}

	char* CompName;
	if (!PyArg_ParseTuple(InArg, "s:GetComponent", &CompName))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;
	for (UActorComponent* Component : Actor->GetComponents())
	{
		if (Component->GetName().Equals(UTF8_TO_TCHAR(CompName)))
		{
			return NePyBase::ToPy(Component);
		}
	}

	Py_RETURN_NONE;
}

PyObject* NePyActor_AddInstanceComponent(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'AddInstanceComponent'"))
	{
		return nullptr;
	}

	UActorComponent* Component = NePyBase::ToCppObject<UActorComponent>(InArg);
	if (!Component)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UActorComponent");
	}

	AActor* Actor = (AActor*)InSelf->Value;
	Actor->AddInstanceComponent(Component);
	Py_RETURN_NONE;
}

PyObject* NePyActor_SetRootComponent(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetRootComponent'"))
	{
		return nullptr;
	}

	USceneComponent* Component = NePyBase::ToCppObject<USceneComponent>(InArg);
	if (!Component)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a USceneComponent");
	}

	AActor* Actor = (AActor*)InSelf->Value;
	Actor->SetRootComponent(Component);
	Py_RETURN_NONE;
}

PyObject* NePyActor_RemoveInstanceComponent(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'RemoveInstanceComponent'"))
	{
		return nullptr;
	}

	UActorComponent* Component = NePyBase::ToCppObject<UActorComponent>(InArg);
	if (!Component)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UActorComponent");
	}

	AActor* Actor = (AActor*)InSelf->Value;
	Actor->RemoveInstanceComponent(Component);
	Py_RETURN_NONE;
}

PyObject* NePyActor_ClearInstanceComponents(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'ClearInstanceComponents'"))
	{
		return nullptr;
	}

	bool bDestroyComponents;
	if (!NePyBase::ToCpp(InArg, bDestroyComponents))
	{
		return PyErr_Format(PyExc_Exception, "argument is not a boolean");
	}

	AActor* Actor = (AActor*)InSelf->Value;
	Actor->ClearInstanceComponents(bDestroyComponents);
	Py_RETURN_NONE;
}

PyObject* NePyActor_GetInstanceComponents(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetInstanceComponents'"))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;
	return NePyBase::ToPy(Actor->GetInstanceComponents());
}

PyObject* NePyActor_AddActorComponent(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'AddActorComponent'"))
	{
		return nullptr;
	}

	PyObject* PyClass;
	char* Name;
	PyObject* PyParentComponent = nullptr;
	if (!PyArg_ParseTuple(InArgs, "Os|O:AddActorComponent", &PyClass, &Name, &PyParentComponent))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass, UActorComponent::StaticClass());
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass derived from UActorComponent");
	}

	USceneComponent* ParentComponent = nullptr;
	if (PyParentComponent && PyParentComponent != Py_None)
	{
		ParentComponent = NePyBase::ToCppObject<USceneComponent>(PyParentComponent);
		if (!ParentComponent)
		{
			return PyErr_Format(PyExc_Exception, "argument is not a USceneComponent");
		}
	}

	AActor* Actor = (AActor*)InSelf->Value;
	UActorComponent* Component = NewObject<UActorComponent>(Actor, Class, FName(UTF8_TO_TCHAR(Name)), RF_Public);
	if (!Component)
	{ 
		return PyErr_Format(PyExc_Exception, "unable to create component");
	}

	if (PyParentComponent && Component->IsA<USceneComponent>())
	{
		USceneComponent* scene_component = (USceneComponent*)Component;
		scene_component->SetupAttachment(ParentComponent);
	}

	if (Actor->GetWorld() && !Component->IsRegistered())
	{
		Component->RegisterComponent();
	}

	if (Component->bWantsInitializeComponent && !Component->HasBeenInitialized() && Component->IsRegistered())
	{
		Component->InitializeComponent();
	}

	return NePyBase::ToPy(Component);
}

PyObject* NePyActor_AddActorRootComponent(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'AddActorRootComponent'"))
	{
		return nullptr;
	}

	PyObject* PyClass;
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "Os:AddActorRootComponent", &PyClass, &Name))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass, USceneComponent::StaticClass());
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass derived from USceneComponent");
	}

	AActor* Actor = (AActor*)InSelf->Value;
	USceneComponent* Component = NewObject<USceneComponent>(Actor, Class, FName(UTF8_TO_TCHAR(Name)), RF_Public);
	if (!Component)
	{
		return PyErr_Format(PyExc_Exception, "unable to create component");
	}

	Actor->SetRootComponent(Component);

	if (Actor->GetWorld() && !Component->IsRegistered())
	{
		Component->RegisterComponent();
	}

	if (Component->bWantsInitializeComponent && !Component->HasBeenInitialized() && Component->IsRegistered())
	{
		Component->InitializeComponent();
	}

	return NePyBase::ToPy(Component);
}

PyObject* NePyActor_GetWorld(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetWorld'"))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;
	UWorld* World = Actor->GetWorld();
	PyObject* PyWorld = NePyBase::ToPy(World);
	return PyWorld;
}

PyObject* NePyActor_GetMesh(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetMesh'"))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;
	auto Mesh = Actor->GetComponentByClass(UMeshComponent::StaticClass());
	PyObject* PyRetVal0 = NePyBase::ToPy(Mesh);
	return PyRetVal0;
}

PyObject* NePyActor_Destroy(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'Destroy'"))
	{
		return nullptr;
	}

	AActor* Actor = (AActor*)InSelf->Value;
	Actor->Destroy();

	Py_RETURN_NONE;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"DispatchBeginPlay", NePyCFunctionCast(&NePyActor_DispatchBeginPlay), METH_NOARGS, "(self) -> None"}, \
{"GetComponents", NePyCFunctionCast(&NePyActor_GetComponents), METH_NOARGS, "(self) -> list[ActorComponent]"}, \
{"GetComponent", NePyCFunctionCast(&NePyActor_GetComponent), METH_VARARGS, "(self, Name: str) -> ActorComponent"}, \
{"AddInstanceComponent", NePyCFunctionCast(&NePyActor_AddInstanceComponent), METH_O, "(self, Component: ActorComponent) -> None"}, \
{"RemoveInstanceComponent", NePyCFunctionCast(&NePyActor_RemoveInstanceComponent), METH_O, "(self, Component: ActorComponent) -> None"}, \
{"SetRootComponent", NePyCFunctionCast(&NePyActor_SetRootComponent), METH_O, "(self, Component: SceneComponent) -> None"}, \
{"ClearInstanceComponents", NePyCFunctionCast(&NePyActor_ClearInstanceComponents), METH_O, "(self, bDestroyComponents: bool) -> None"}, \
{"GetInstanceComponents", NePyCFunctionCast(&NePyActor_GetInstanceComponents), METH_NOARGS, "(self) -> list[ActorComponent]"}, \
{"AddActorComponent", NePyCFunctionCast(&NePyActor_AddActorComponent), METH_VARARGS, "[T: ActorComponent](self, Class: type[T], Name: str, ParentComponent: SceneComponent = ...) -> T;(self, Class: Class, Name: str, ParentComponent: SceneComponent = ...) -> ActorComponent"}, \
{"AddActorRootComponent", NePyCFunctionCast(&NePyActor_AddActorRootComponent), METH_VARARGS, "[T: SceneComponent](self, Class: type[T], Name: str) -> T;(self, Class: Class, Name: str) -> SceneComponent"}, \
{"GetWorld", NePyCFunctionCast(&NePyActor_GetWorld), METH_NOARGS, "(self) -> World"}, \
{"GetMesh", NePyCFunctionCast(&NePyActor_GetMesh), METH_NOARGS, "(self) -> MeshComponent"}, \
{"Destroy", NePyCFunctionCast(&NePyActor_Destroy), METH_NOARGS, "(self) -> None"}, \
