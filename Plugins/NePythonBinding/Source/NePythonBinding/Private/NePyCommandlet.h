#pragma once
#include "Commandlets/Commandlet.h"
#include "NePyCommandlet.generated.h"

UCLASS()
class UNePyDumpReflectionInfosCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	virtual int32 Main(const FString& InParamsStr) override;
};

UCLASS()
class UNePyDumpBlueprintInfosCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	virtual int32 Main(const FString& InParamsStr) override;
};

UCLASS()
class UNePyGmCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	virtual int32 Main(const FString& Params) override;
};

// 通用的执行外部Python脚本的Commandlet
UCLASS()
class UNePyRunScriptCommandlet : public UCommandlet
{
	GENERATED_UCLASS_BODY()
	virtual int32 Main(const FString& Params) override;
};
