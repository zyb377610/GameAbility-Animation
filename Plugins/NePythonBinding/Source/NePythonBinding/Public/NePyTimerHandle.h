#pragma once
#include "Engine/EngineTypes.h"
#include "NePyStructBase.h"

// 针对FTimerHandle的手写封装
struct NEPYTHONBINDING_API FNePyStruct_TimerHandle : public TNePyStructBase<FTimerHandle>
{
};

void NePyInitTimerHandle(PyObject* PyOuterModule);

NEPYTHONBINDING_API FNePyStruct_TimerHandle* NePyStructNew_TimerHandle(const FTimerHandle& InValue);

NEPYTHONBINDING_API FNePyStruct_TimerHandle* NePyStructCheck_TimerHandle(PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyStructGetType_TimerHandle();

namespace NePyBase
{
	NEPYTHONBINDING_API bool ToCpp(PyObject* InPyObj, FTimerHandle& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FTimerHandle& InVal, PyObject*& OutPyObj);
	NEPYTHONBINDING_API PyObject* ToPy(const FTimerHandle& InVal);
}
