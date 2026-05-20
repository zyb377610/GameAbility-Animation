#pragma once
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"

// 存放着UClass的指针
struct NEPYTHONBINDING_API FNePyClass : public FNePyObjectBase
{
	inline UClass* GetValue()
	{
		return (UClass*)Value;
	}
};

// 初始化StaticClass类对象
void NePyInitClass(PyObject* PyOuterModule);

// 检查Python对象类型是否为FNePyClass
NEPYTHONBINDING_API FNePyClass* NePyClassCheck(const PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyClassGetType();

