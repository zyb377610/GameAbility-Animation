#pragma once
#include "Containers/Ticker.h"
#include "NePyStructBase.h"

#if ENGINE_MAJOR_VERSION < 5
using FNePyTickerHandle = FDelegateHandle;
#else
using FNePyTickerHandle = FTSTicker::FDelegateHandle;
#endif

// 针对FTSTicker::FDelegateHandle的手写封装
struct NEPYTHONBINDING_API FNePyStruct_TickerHandle : public TNePyStructBase<FNePyTickerHandle>
{
};

void NePyInitTickerHandle(PyObject* PyOuterModule);

NEPYTHONBINDING_API FNePyStruct_TickerHandle* NePyStructNew_TickerHandle(const FNePyTickerHandle& InValue);

NEPYTHONBINDING_API FNePyStruct_TickerHandle* NePyStructCheck_TickerHandle(PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyStructGetType_TickerHandle();
