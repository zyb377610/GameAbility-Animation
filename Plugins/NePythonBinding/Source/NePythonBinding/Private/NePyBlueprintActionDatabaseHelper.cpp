#if WITH_EDITOR
#include "NePyBlueprintActionDatabaseHelper.h"
#include "Runtime/Launch/Resources/Version.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.h"
#include "BlueprintBoundEventNodeSpawner.h"
#include "BlueprintVariableNodeSpawner.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "BlueprintTypePromotion.h"
#if ENGINE_MINOR_VERSION >= 3
#include "PropertyPermissionList.h"
#else
#include "PropertyEditorPermissionList.h"
#endif
#endif
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_Message.h"
#include "K2Node_AssignDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_ClassDynamicCast.h"
#include "K2Node_ActorBoundEvent.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "NePyBlueprintActionDatabaseHelper"

TArray<UClass*> FNePyBlueprintActionDatabaseHelper::PendingClasses;

// 此文件中的绝大部分函数都来自于BlueprintActionDatabase.cpp，除了命名空间之外，无任何其它改动
// 放在这里的原因是FBlueprintActionDatabase::RefreshClassActions发现我们的类是UBlueprintGeneratedClass时，会找ClassGeneratedBy（蓝图）去要Actions
// 如果ClassGeneratedBy为NULL，则会跳过注册，导致我们在蓝图编辑器中无法看到Subclassing声明的所有ufunction和uproperty
// 本文件中的RegisterClassActions函数目的就是平替FBlueprintActionDatabase::RefreshClassActions，将我们的ufunction和uproperty注册到BlueprintActionDatabase中

//------------------------------------------------------------------------------
UBlueprintNodeSpawner* FNePyBlueprintActionDatabaseHelper::MakeMessageNodeSpawner(UFunction* InterfaceFunction)
{
	check(InterfaceFunction != nullptr);
	check(FKismetEditorUtilities::IsClassABlueprintInterface(CastChecked<UClass>(InterfaceFunction->GetOuter())));

	UBlueprintFunctionNodeSpawner* NodeSpawner = UBlueprintFunctionNodeSpawner::Create(UK2Node_Message::StaticClass(), InterfaceFunction);
	check(NodeSpawner != nullptr);

	auto SetNodeFunctionLambda = [](UEdGraphNode* NewNode, FFieldVariant FuncField)
	{
		UK2Node_Message* MessageNode = CastChecked<UK2Node_Message>(NewNode);
		MessageNode->FunctionReference.SetFromField<UFunction>(FuncField.Get<UField>(), /*bIsConsideredSelfContext =*/false);
	};
	NodeSpawner->SetNodeFieldDelegate = UBlueprintFunctionNodeSpawner::FSetNodeFieldDelegate::CreateStatic(SetNodeFunctionLambda);

	NodeSpawner->DefaultMenuSignature.MenuName = FText::Format(LOCTEXT("MessageNodeMenuName", "{0} (Message)"), NodeSpawner->DefaultMenuSignature.MenuName);
	return NodeSpawner;
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner* FNePyBlueprintActionDatabaseHelper::MakeAssignDelegateNodeSpawner(FMulticastDelegateProperty* DelegateProperty)
{
	// @TODO: it'd be awesome to have both nodes spawned by this available for 
	//        context pin matching (the delegate inputs and the event outputs)
	return UBlueprintDelegateNodeSpawner::Create(UK2Node_AssignDelegate::StaticClass(), DelegateProperty);
}

UBlueprintNodeSpawner* FNePyBlueprintActionDatabaseHelper::MakeComponentBoundEventSpawner(FMulticastDelegateProperty* DelegateProperty)
{
	return UBlueprintBoundEventNodeSpawner::Create(UK2Node_ComponentBoundEvent::StaticClass(), DelegateProperty);
}

//------------------------------------------------------------------------------
UBlueprintNodeSpawner* FNePyBlueprintActionDatabaseHelper::MakeActorBoundEventSpawner(FMulticastDelegateProperty* DelegateProperty)
{
	return UBlueprintBoundEventNodeSpawner::Create(UK2Node_ActorBoundEvent::StaticClass(), DelegateProperty);
}

//------------------------------------------------------------------------------
bool FNePyBlueprintActionDatabaseHelper::IsPropertyBlueprintVisible(FProperty const* const Property)
{
	bool const bIsAccessible = Property->HasAllPropertyFlags(CPF_BlueprintVisible);

	bool const bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
	bool const bIsAssignableOrCallable = Property->HasAnyPropertyFlags(CPF_BlueprintAssignable | CPF_BlueprintCallable);

	bool bVisible = !Property->HasAnyPropertyFlags(CPF_Parm) && (bIsAccessible || (bIsDelegate && bIsAssignableOrCallable));
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	if (bVisible)
	{
		bVisible = FPropertyEditorPermissionList::Get().DoesPropertyPassFilter(Property->GetOwnerStruct(), Property->GetFName());
	}
#endif
	
	return bVisible;
}

//------------------------------------------------------------------------------
bool FNePyBlueprintActionDatabaseHelper::IsBlueprintInterfaceFunction(const UFunction* Function)
{
	bool bIsBpInterfaceFunc = false;
	if (UClass* FuncClass = Function->GetOwnerClass())
	{
		if (UBlueprint* BpOuter = Cast<UBlueprint>(FuncClass->ClassGeneratedBy))
		{
			FName FuncName = Function->GetFName();

			for (int32 InterfaceIndex = 0; (InterfaceIndex < BpOuter->ImplementedInterfaces.Num()) && !bIsBpInterfaceFunc; ++InterfaceIndex)
			{
				FBPInterfaceDescription& InterfaceDesc = BpOuter->ImplementedInterfaces[InterfaceIndex];
				if(!InterfaceDesc.Interface)
				{
					continue;
				}

				bIsBpInterfaceFunc = (InterfaceDesc.Interface->FindFunctionByName(FuncName) != nullptr);
			}
		}
	}
	return bIsBpInterfaceFunc;
}

//------------------------------------------------------------------------------
bool FNePyBlueprintActionDatabaseHelper::IsInheritedBlueprintFunction(const UFunction* Function)
{
	bool bIsBpInheritedFunc = false;
	if (UClass* FuncClass = Function->GetOwnerClass())
	{
		if (UBlueprint* BpOwner = Cast<UBlueprint>(FuncClass->ClassGeneratedBy))
		{
			FName FuncName = Function->GetFName();
			if (UClass* ParentClass = BpOwner->ParentClass)
			{
				bIsBpInheritedFunc = (ParentClass->FindFunctionByName(FuncName, EIncludeSuperFlag::IncludeSuper) != nullptr);
			}
		}
	}
	return bIsBpInheritedFunc;
}

//------------------------------------------------------------------------------
void FNePyBlueprintActionDatabaseHelper::GetClassMemberActions(UClass* const Class, FActionList& ActionListOut)
{
	// class field actions (nodes that represent and perform actions on
	// specific fields of the class... functions, properties, etc.)
	{
		AddClassFunctionActions(Class, ActionListOut);
		AddClassPropertyActions(Class, ActionListOut);
		AddClassDataObjectActions(Class, ActionListOut);
		// class UEnum actions are added by individual nodes via GetNodeSpecificActions()
		// class UScriptStruct actions are added by individual nodes via GetNodeSpecificActions()
	}

	AddClassCastActions(Class, ActionListOut);
}

//------------------------------------------------------------------------------
void FNePyBlueprintActionDatabaseHelper::AddClassFunctionActions(UClass const* const Class, FActionList& ActionListOut)
{
	check(Class != nullptr);

	// loop over all the functions in the specified class; exclude-super because 
	// we can always get the super functions by looking up that class separately 
	for (TFieldIterator<UFunction> FunctionIt(Class, EFieldIteratorFlags::ExcludeSuper); FunctionIt; ++FunctionIt)
	{
		UFunction* Function = *FunctionIt;

		bool const bIsInheritedFunction = IsInheritedBlueprintFunction(Function);
		if (bIsInheritedFunction)
		{
			// inherited functions will be captured when the parent class is ran
			// through this function (no need to duplicate)
			continue;
		}

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		// Apply general filtering for functions
		if(!FBlueprintActionDatabase::IsFunctionAllowed(Function, FBlueprintActionDatabase::EPermissionsContext::Node))
		{
			continue;
		}
#endif

		bool const bIsBpInterfaceFunc = IsBlueprintInterfaceFunction(Function);
		if (UEdGraphSchema_K2::FunctionCanBePlacedAsEvent(Function) && !bIsBpInterfaceFunc)
		{
			if (UBlueprintEventNodeSpawner* NodeSpawner = UBlueprintEventNodeSpawner::Create(Function))
			{
				ActionListOut.Add(NodeSpawner);
			}
		}

#if ENGINE_MAJOR_VERSION >= 5
		// If this is a promotable function, and it has already been registered
		// than do NOT add it to the asset action database. We should
		// probably have some better logic for this, like adding our own node spawner
		const bool bIsRegisteredPromotionFunc =
			TypePromoDebug::IsTypePromoEnabled() &&
			FTypePromotion::IsFunctionPromotionReady(Function) &&
			FTypePromotion::IsOperatorSpawnerRegistered(Function);
#endif

		if (UEdGraphSchema_K2::CanUserKismetCallFunction(Function))
		{
			// @TODO: if this is a Blueprint, and this function is from a 
			//        Blueprint "implemented interface", then we don't need to 
			//        include it (the function is accounted for in from the 
			//        interface class).
			UBlueprintFunctionNodeSpawner* FuncSpawner = UBlueprintFunctionNodeSpawner::Create(Function);

#if ENGINE_MAJOR_VERSION >= 5
			// Only add this action to the list of the operator function is not already registered. Otherwise we will 
			// get a bunch of duplicate operator actions
			if (!bIsRegisteredPromotionFunc)
#endif
			{
				ActionListOut.Add(FuncSpawner);
			}

			if (FKismetEditorUtilities::IsClassABlueprintInterface(Class))
			{
				// Use the default function name
				ActionListOut.Add(MakeMessageNodeSpawner(Function));
			}
		}
	}
}

//------------------------------------------------------------------------------
void FNePyBlueprintActionDatabaseHelper::AddClassPropertyActions(UClass const* const Class, FActionList& ActionListOut)
{
	bool const bIsComponent  = Class->IsChildOf<UActorComponent>();
	bool const bIsActorClass = Class->IsChildOf<AActor>();
	
	// loop over all the properties in the specified class; exclude-super because 
	// we can always get the super properties by looking up that class separately 
	for (TFieldIterator<FProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		if (!IsPropertyBlueprintVisible(Property))
		{
			continue;
		}

 		bool const bIsDelegate = Property->IsA(FMulticastDelegateProperty::StaticClass());
 		if (bIsDelegate)
 		{
			FMulticastDelegateProperty* DelegateProperty = CastFieldChecked<FMulticastDelegateProperty>(Property);
			if (DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintAssignable))
			{
				UBlueprintNodeSpawner* AddSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_AddDelegate::StaticClass(), DelegateProperty);
				ActionListOut.Add(AddSpawner);
				
				UBlueprintNodeSpawner* AssignSpawner = MakeAssignDelegateNodeSpawner(DelegateProperty);
				ActionListOut.Add(AssignSpawner);
			}
			
			if (DelegateProperty->HasAnyPropertyFlags(CPF_BlueprintCallable))
			{
				UBlueprintNodeSpawner* CallSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_CallDelegate::StaticClass(), DelegateProperty);
				ActionListOut.Add(CallSpawner);
			}
			
			UBlueprintNodeSpawner* RemoveSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_RemoveDelegate::StaticClass(), DelegateProperty);
			ActionListOut.Add(RemoveSpawner);
			UBlueprintNodeSpawner* ClearSpawner = UBlueprintDelegateNodeSpawner::Create(UK2Node_ClearDelegate::StaticClass(), DelegateProperty);
			ActionListOut.Add(ClearSpawner);

			if (bIsComponent)
			{
				ActionListOut.Add(MakeComponentBoundEventSpawner(DelegateProperty));
			}
			else if (bIsActorClass)
			{
				ActionListOut.Add(MakeActorBoundEventSpawner(DelegateProperty));
			}
 		}
		else
		{
			UBlueprintVariableNodeSpawner* GetterSpawner = UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(UK2Node_VariableGet::StaticClass(), Property);
			ActionListOut.Add(GetterSpawner);
			UBlueprintVariableNodeSpawner* SetterSpawner = UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(UK2Node_VariableSet::StaticClass(), Property);
			ActionListOut.Add(SetterSpawner);
		}
	}
}

//------------------------------------------------------------------------------
void FNePyBlueprintActionDatabaseHelper::AddClassDataObjectActions(UClass const* const Class, FActionList& ActionListOut)
{
	// loop over all the properties in the specified class; exclude-super because 
	// we can always get the super properties by looking up that class separately 
	const UStruct* SparseDataStruct = Class->GetSparseClassDataStruct();
	const UStruct* ParentSparseDataStruct = Class->GetSuperClass() ? Class->GetSuperClass()->GetSparseClassDataStruct() : nullptr;
	if (ParentSparseDataStruct != SparseDataStruct)
	{
		for (TFieldIterator<FProperty> PropertyIt(SparseDataStruct, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
		{
			FProperty* Property = *PropertyIt;
			if (!IsPropertyBlueprintVisible(Property))
			{
				continue;
			}

			UClass* NonConstClass = const_cast<UClass*>(Class);
			UBlueprintVariableNodeSpawner* GetterSpawner = UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(UK2Node_VariableGet::StaticClass(), Property, nullptr, NonConstClass);
			ActionListOut.Add(GetterSpawner);
//			UBlueprintVariableNodeSpawner* SetterSpawner = UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(UK2Node_VariableSet::StaticClass(), Property);
//			ActionListOut.Add(SetterSpawner);
		}
	}
}

//------------------------------------------------------------------------------
void FNePyBlueprintActionDatabaseHelper::AddClassCastActions(UClass* Class, FActionList& ActionListOut)
{
	Class = Class->GetAuthoritativeClass();
	check(Class);

	UEdGraphSchema_K2 const* K2Schema = GetDefault<UEdGraphSchema_K2>();
	bool bIsCastPermitted  = UEdGraphSchema_K2::IsAllowableBlueprintVariableType(Class);
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	bIsCastPermitted = bIsCastPermitted && FBlueprintActionDatabase::IsClassAllowed(Class, FBlueprintActionDatabase::EPermissionsContext::Node);
#endif
	
	if (bIsCastPermitted)
	{
		auto CustomizeCastNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, UClass* TargetType)
		{
			UK2Node_DynamicCast* CastNode = CastChecked<UK2Node_DynamicCast>(NewNode);
			CastNode->TargetType = TargetType;
		};

		UBlueprintNodeSpawner* CastObjNodeSpawner = UBlueprintNodeSpawner::Create<UK2Node_DynamicCast>();
		CastObjNodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeCastNodeLambda, Class);
		ActionListOut.Add(CastObjNodeSpawner);

		UBlueprintNodeSpawner* CastClassNodeSpawner = UBlueprintNodeSpawner::Create<UK2Node_ClassDynamicCast>();
		CastClassNodeSpawner->CustomizeNodeDelegate = CastObjNodeSpawner->CustomizeNodeDelegate;
		ActionListOut.Add(CastClassNodeSpawner);
	}
}

//------------------------------------------------------------------------------
// 从这里开始是我们自己实现的函数
void FNePyBlueprintActionDatabaseHelper::RegisterClassActions(UClass* Class)
{
	if (IsRunningDedicatedServer() || IsRunningGame())
	{
		return;
	}

	if (!GEditor)
	{
		PendingClasses.Add(Class);
		return;
	}

	FBlueprintActionDatabase& ActionDatabase = FBlueprintActionDatabase::Get();
	FBlueprintActionDatabase::FActionRegistry& ActionRegistry = const_cast<FBlueprintActionDatabase::FActionRegistry&>(ActionDatabase.GetAllActions());

	FActionList& ClassActionList = ActionRegistry.FindOrAdd(Class);
	ClassActionList.Empty();

	GetClassMemberActions(Class, ClassActionList);

	if (ClassActionList.Num() > 0)
	{
		// TODO: FBlueprintActionDatabase::RefreshClassActions会将Class加到ActionPrimingQueue中，似乎是某种优化操作，我们拿不到这个ActionPrimingQueue暂时没法加
		// 但是跑起来似乎又没啥问题
	}
	else
	{
		ActionRegistry.Remove(Class);
	}
}

void FNePyBlueprintActionDatabaseHelper::OnPostEngineInit()
{
	if (IsRunningDedicatedServer() || IsRunningGame())
	{
		return;
	}

	check(GEditor);

	for (UClass* const Class : PendingClasses)
	{
		RegisterClassActions(Class);
	}

	PendingClasses.Empty();
}

#endif // WITH_EDITOR
