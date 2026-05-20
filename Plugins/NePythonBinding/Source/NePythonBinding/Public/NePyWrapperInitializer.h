#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"

class UClass;
class UScriptStruct;

typedef PyTypeObject* (*NePyWrappedClassInitFunc)(const UClass* InClass, PyTypeObject* InPyBase, PyObject* InPyBases);
typedef PyTypeObject* (*NePyWrappedStructInitFunc)(const UScriptStruct* InStruct, PyTypeObject* InPyBase);


// 用于在插件启动时，生成所有静态导出类型
class NEPYTHONBINDING_API FNePyWrapperInitializer
{
public:
	static FNePyWrapperInitializer& Get();

	// 为静态导出的UE类注册Python类型初始化函数
	void RegisterWrappedClassInitFunc(const UClass* InClass, NePyWrappedClassInitFunc InInitFunc);

	// 为静态导出的UE结构体注册Python类型初始化函数
	void RegisterWrappedStructInitFunc(const UScriptStruct* InStruct, NePyWrappedStructInitFunc InInitFunc);
	
	void GenerateWrappedTypes();

private:
	// 为UE类生成对应的Python类型，如果没生成静态导出代码，则走动态导出
	void GenerateWrappedClassType(const UClass* InClass);
	PyTypeObject* DoGenerateWrappedClassType(const UClass* InClass, PyTypeObject* InPyBase);

	// 为UE结构体生成对应的Python类型，如果没生成静态导出代码，则走动态导出
	void GenerateWrappedStructType(const UScriptStruct* InStruct);
	PyTypeObject* DoGenerateWrappedStructType(const UScriptStruct* InStruct, PyTypeObject* InPyBase);

private:
	TMap<const UClass*, NePyWrappedClassInitFunc> ClassInitFuncs;
	TMap<const UScriptStruct*, NePyWrappedStructInitFunc> StructInitFuncs;
};

