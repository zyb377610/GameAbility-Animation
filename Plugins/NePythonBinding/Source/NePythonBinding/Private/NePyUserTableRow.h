#pragma once
#include "NePyUserStruct.h"
#include "UObject/Class.h"

// UUserDefinedStruct和UNePyGeneratedStruct的实例
struct FNePyUserTableRow : public FNePyUserStruct
{
	// tp_new
	static PyObject* New(PyTypeObject* InPyType, PyObject* InArgs, PyObject* InKwds);
};

void NePyInitUserTableRow(PyObject* PyOuterModule);
FNePyUserTableRow* NePyUserTableRowCheck(PyObject* InPyObj);
PyTypeObject* NePyUserTableRowGetType();
