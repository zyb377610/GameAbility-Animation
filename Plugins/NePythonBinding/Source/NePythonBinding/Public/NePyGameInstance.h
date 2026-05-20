#pragma once
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Containers/Ticker.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "Engine/World.h"
#include "NePyIncludePython.h"
#include "NePyResourceOwner.h"
#include "NePyGameInstance.generated.h"

UCLASS(BlueprintType)
class NEPYTHONBINDING_API UNePyGameInstance : public UPlatformGameInstance, public INePyResourceOwner
{
	GENERATED_BODY()

public:
	// Override functions from UGameInstance
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void OnStart() override;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

	//
	void PreWorldTick(UWorld* World, ELevelTick Tick, float Delta);

	//
	void PreActorTick(UWorld* World, ELevelTick Tick, float Delta);

	//
	void PostActorTick(UWorld* World, ELevelTick Tick, float Delta);

	//
	bool Tick(float DeltaSeconds);

	UPROPERTY(EditAnywhere, Category = "Python", BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FString PythonModule;

	UPROPERTY(EditAnywhere, Category = "Python", BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	FString PythonClass;

	UFUNCTION(BlueprintCallable, Category = "Python")
	void CallPythonActorMethod(FString InMethodName, FString InArgs);

	UFUNCTION(BlueprintCallable, Category = "Python")
	bool CallPythonActorMethodBool(FString InMethodName, FString InArgs);

	UFUNCTION(BlueprintCallable, Category = "Python")
	FString CallPythonActorMethodString(FString InMethodName, FString InArgs);

	bool CallGameInstanceMethod(char* FunctionName, char* FormatList, ...);

	PyObject* GetPyProxy()
	{
		return PyGameInstance;
	}

private:
	PyObject* PyGameInstance = nullptr;

	// 在World开始时Tick
	PyObject* PyGameInstanceOnPreWorldTick = nullptr;
	// 在所有Actor之前Tick
	PyObject* PyGameInstanceOnPreActorTick = nullptr;
	// 在所有Actor之后Tick
	PyObject* PyGameInstanceOnPostActorTick = nullptr;
	// 在渲染Tick之后Tick
	PyObject* PyGameInstanceOnTick = nullptr;

	//
	FDelegateHandle PreWorldTickDelegateHandler;
	//
	FDelegateHandle PreActorTickDelegateHandler;
	//
	FDelegateHandle PostActorTickDelegateHandler;
#if ENGINE_MAJOR_VERSION >= 5
	FTSTicker::FDelegateHandle TickDelegateHandler;
#else
	FDelegateHandle TickDelegateHandler;
#endif
};
