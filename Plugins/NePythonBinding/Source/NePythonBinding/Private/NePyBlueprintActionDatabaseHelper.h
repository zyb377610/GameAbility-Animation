#pragma once
#if WITH_EDITOR
#include "CoreMinimal.h"
#include "BlueprintActionDatabase.h"

class FNePyBlueprintActionDatabaseHelper
{
public:
	static void RegisterClassActions(UClass* Class);
	static void OnPostEngineInit();
protected:
	typedef FBlueprintActionDatabase::FActionList FActionList;

	static UBlueprintNodeSpawner* MakeMessageNodeSpawner(UFunction* InterfaceFunction);
	static UBlueprintNodeSpawner* MakeAssignDelegateNodeSpawner(FMulticastDelegateProperty* DelegateProperty);
	static UBlueprintNodeSpawner* MakeComponentBoundEventSpawner(FMulticastDelegateProperty* DelegateProperty);
	static UBlueprintNodeSpawner* MakeActorBoundEventSpawner(FMulticastDelegateProperty* DelegateProperty);

	/**
	 * Mimics UEdGraphSchema_K2::CanUserKismetAccessVariable(); however, this
	 * omits the filtering that CanUserKismetAccessVariable() does (saves that
	 * for later with FBlueprintActionFilter).
	 *
	 * @param  Property		The property you want to check.
	 * @return True if the property can be seen from a blueprint.
	 */
	static bool IsPropertyBlueprintVisible(FProperty const* const Property);

	/**
	* Checks to see if the specified function is a blueprint owned function
	* that was inherited from an implemented interface.
	*
	* @param  Function	 The function to check.
	* @return True if the function is owned by a blueprint, and some (implemented) interface has a matching function name.
	*/
	static bool IsBlueprintInterfaceFunction(const UFunction* Function);

	/**
	* Checks to see if the specified function is a blueprint owned function
	* that was inherited from the blueprint's parent.
	*
	* @param  Function	 The function to check.
	* @return True if the function is owned by a blueprint, and some parent has a matching function name.
	*/
	static bool IsInheritedBlueprintFunction(const UFunction* Function);

	/**
	* Retrieves all the actions pertaining to a class and its fields (functions,
	* variables, delegates, etc.). Actions that are conceptually owned by the
	* class.
	*
	* @param  Class			The class you want actions for.
	* @param  ActionListOut	The array you want filled with the requested actions.
	*/
	static void GetClassMemberActions(UClass* const Class, FActionList& ActionListOut);

	/**
	* Loops over all of the class's functions and creates a node-spawners for
	* any that are viable for blueprint use. Evolved from
	* FK2ActionMenuBuilder::GetFuncNodesForClass(), plus a series of other
	* FK2ActionMenuBuilder methods (GetAllInterfaceMessageActions,
	* GetEventsForBlueprint, etc).
	*
	* Ideally, any node that is constructed from a UFunction should go in here
	* (so we only ever loop through the class's functions once). We handle
	* UK2Node_CallFunction alongside UK2Node_Event.
	*
	* @param  Class			The class whose functions you want node-spawners for.
	* @param  ActionListOut	The list you want populated with the new spawners.
	*/
	static void AddClassFunctionActions(UClass const* const Class, FActionList& ActionListOut);

	/**
	* Loops over all of the class's properties and creates node-spawners for
	* any that are viable for blueprint use. Evolved from certain parts of
	* FK2ActionMenuBuilder::GetAllActionsForClass().
	*
	* @param  Class			The class whose properties you want node-spawners for.
	* @param  ActionListOut	The list you want populated with the new spawners.
	*/
	static void AddClassPropertyActions(UClass const* const Class, FActionList& ActionListOut);

	/**
	* Loops over all of the class's data object's properties and creates node-spawners for
	* any that are viable for blueprint use.
	*
	* @param  Class			The class whose properties you want node-spawners for.
	* @param  ActionListOut	The list you want populated with the new spawners.
	*/
	static void AddClassDataObjectActions(UClass const* const Class, FActionList& ActionListOut);

	/**
	* Evolved from FClassDynamicCastHelper::GetClassDynamicCastNodes(). If the
	* specified class is a viable blueprint variable type, then two cast nodes
	* are added for it (UK2Node_DynamicCast, and UK2Node_ClassDynamicCast).
	*
	* @param  Class			The class who you want cast nodes for (they cast to this class).
	* @param  ActionListOut	The list you want populated with the new spawners.
	*/
	static void AddClassCastActions(UClass* const Class, FActionList& ActionListOut);

	static TArray<UClass*> PendingClasses;
};

#endif // WITH_EDITOR
