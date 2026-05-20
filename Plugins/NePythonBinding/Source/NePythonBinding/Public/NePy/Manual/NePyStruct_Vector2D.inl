#pragma once
#include "NePyBase.h"
#include "Math/Vector2D.h"

PyObject* FNePyStruct_Vector2D_AsTuple_Manual(FNePyStruct_Vector2D* InSelf)
{
	PyObject* VecTuple = PyTuple_New(2);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		PyTuple_SetItem(VecTuple, Index, PyFloat_FromDouble((*(FVector2D*)InSelf->Value)[Index]));
	}
	return VecTuple;
}

PyObject* FNePyStruct_Vector2D_AsList_Manual(FNePyStruct_Vector2D* InSelf)
{
	PyObject* VecList = PyList_New(2);
	for (int32 Index = 0; Index < 2; ++Index)
	{
		PyList_SetItem(VecList, Index, PyFloat_FromDouble((*(FVector2D*)InSelf->Value)[Index]));
	}
	return VecList;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"AsTuple", NePyCFunctionCast(&FNePyStruct_Vector2D_AsTuple_Manual), METH_NOARGS, "(self) -> tuple[float, float]"}, \
{"AsList", NePyCFunctionCast(&FNePyStruct_Vector2D_AsList_Manual), METH_NOARGS, "(self) -> list[float]"}, \

