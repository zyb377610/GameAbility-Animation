#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"

typedef PyObject* (*NePyPropertyToPyObjectFunc)(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
typedef bool (*NePyPyObjectToPropertyFunc)(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
typedef PyObject* (*NePyPropertyToPyObjectFuncForStruct)(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
typedef bool (*NePyPyObjectToPropertyFuncForStruct)(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* InPyOuter);

// 获取从UE属性到Python对象的转换函数
NePyPropertyToPyObjectFunc NePyGetPropertyToPyObjectConverter(const FProperty* InProp);

// 获取从Python对象到UE属性的转换函数
NePyPyObjectToPropertyFunc NePyGetPyObjectToPropertyConverter(const FProperty* InProp);

// 获取从UE属性到Python对象的转换函数
NePyPropertyToPyObjectFuncForStruct NePyGetPropertyToPyObjectConverterForStruct(const FProperty* InProp);

// 获取从Python对象到UE属性的转换函数
NePyPyObjectToPropertyFuncForStruct NePyGetPyObjectToPropertyConverterForStruct(const FProperty* InProp);

// 获取从UE属性到Python对象的转换函数,确保返回的PyObject不依赖原始内存
NePyPropertyToPyObjectFunc NePyGetPropertyToPyObjectConverterNoDependency(const FProperty* InProp);
