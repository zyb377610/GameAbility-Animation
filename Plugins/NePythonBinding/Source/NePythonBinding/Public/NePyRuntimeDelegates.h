#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "NePyRuntimeDelegates.generated.h"

// Declare the original parameter-less signature
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNePythonRuntimeStateSignature);

// Declare a new signature for OnTick that takes a float
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNePythonTickSignature, float, DeltaSeconds);

// Declare a new signature for OnDebugInput that takes an FString
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNePythonDebugInputSignature, const FString&, Command);


/**
 * A UObject container for holding global runtime delegates for the NePython module.
 * This allows delegates to be part of the UObject ecosystem, making them accessible
 * to the reflection system and potentially Blueprints.
 */
UCLASS()
class NEPYTHONBINDING_API UNePyRuntimeDelegates : public UObject
{
	GENERATED_BODY()

public:
	// 2. Define each delegate as a UPROPERTY.
	//    'BlueprintAssignable' allows these delegates to be bound in Blueprints.

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnInitPre;

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnInitPost;

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnPostEngineInit;

	// Modified OnTick to use the new signature with a float parameter
	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonTickSignature OnTick;

	// Modified OnDebugInput to use the new signature with an FString parameter
	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonDebugInputSignature OnDebugInput;

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnPatchRunPre;

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnPatchRunPost;

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnPatchShouldContinuePre;

	UPROPERTY(BlueprintAssignable, Category = "NePython|Runtime Delegates")
	FNePythonRuntimeStateSignature OnPatchShouldContinuePost;
};