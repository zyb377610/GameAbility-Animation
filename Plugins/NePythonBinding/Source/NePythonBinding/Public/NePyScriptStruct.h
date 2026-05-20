#pragma once
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"

// 存放着UScriptStruct的指针
struct NEPYTHONBINDING_API FNePyScriptStruct : public FNePyObjectBase
{
	inline UScriptStruct* GetValue()
	{
		return (UScriptStruct*)Value;
	}
};

// 初始化StaticStruct类对象
void NePyInitScriptStruct(PyObject* PyOuterModule);

// 检查Python对象类型是否为FNePyScriptStruct
NEPYTHONBINDING_API FNePyScriptStruct* NePyScriptStructCheck(const PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyScriptStructGetType();

