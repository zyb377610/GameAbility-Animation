#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NePyObjectBase.h"

// 存放着UInterface的指针
struct NEPYTHONBINDING_API FNePyInterface : public FNePyObjectBase
{
	inline UInterface* GetValue()
	{
		return (UInterface*)Value;
	}
};

// 初始化Interface类对象
void NePyInitInterface(PyObject* PyOuterModule);

// 检查Python对象类型是否为FNePyInterface
NEPYTHONBINDING_API FNePyInterface* NePyInterfaceCheck(const PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyInterfaceGetType();

