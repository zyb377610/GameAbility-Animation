#pragma once
#include "NePyBase.h"
#include "Math/Vector4.h"

PyObject* FNePyStruct_Vector4_AsTuple_Manual(FNePyStruct_Vector4* InSelf)
{
	PyObject* VecTuple = PyTuple_New(4);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		PyTuple_SetItem(VecTuple, Index, PyFloat_FromDouble((*(FVector4*)InSelf->Value)[Index]));
	}
	return VecTuple;
}

PyObject* FNePyStruct_Vector4_AsList_Manual(FNePyStruct_Vector4* InSelf)
{
	PyObject* VecList = PyList_New(4);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		PyList_SetItem(VecList, Index, PyFloat_FromDouble((*(FVector4*)InSelf->Value)[Index]));
	}
	return VecList;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"AsTuple", NePyCFunctionCast(&FNePyStruct_Vector4_AsTuple_Manual), METH_NOARGS, "(self) -> tuple[float, float, float, float]"}, \
{"AsList", NePyCFunctionCast(&FNePyStruct_Vector4_AsList_Manual), METH_NOARGS, "(self) -> list[float]"}, \

