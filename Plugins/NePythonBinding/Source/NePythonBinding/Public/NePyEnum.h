#pragma once
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"

// 存放着UEnum的指针
struct NEPYTHONBINDING_API FNePyEnum : public FNePyObjectBase
{
	inline UEnum* GetValue()
	{
		return (UEnum*)Value;
	}
};

// 初始化StaticEnum类对象
void NePyInitEnum(PyObject* PyOuterModule);

// 检查Python对象类型是否为FNePyEnum
NEPYTHONBINDING_API FNePyEnum* NePyEnumCheck(const PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyEnumGetType();

