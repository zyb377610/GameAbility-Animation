#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Dom/JsonObject.h"

// 使用官方Python插件的方法，将反射信息导出成Json文件，供后续代码生成使用
// 参见 PyWrapperTypeRegistry.cpp FPyWrapperTypeRegistry::GenerateWrappedTypes()
NEPYTHONBINDING_API TSharedRef<FJsonObject> NePyDumpReflectionInfos(bool bForceExport);

NEPYTHONBINDING_API void NePyDumpReflectionInfosToFile(const FString& InOutputDirectory);

NEPYTHONBINDING_API FString NePyGetDefaultDumpReflectionInfosDirectory();

// 导出蓝图生成类的反射信息
NEPYTHONBINDING_API TSharedRef<FJsonObject> NePyDumpBlueprintInfos();

NEPYTHONBINDING_API void NePyDumpBlueprintInfosToFile(const FString& InOutputFile);

#endif
