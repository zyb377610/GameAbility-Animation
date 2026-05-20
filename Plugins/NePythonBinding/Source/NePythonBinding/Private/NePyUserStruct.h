#pragma once
#include "NePyStructBase.h"

// UUserDefinedStruct和UNePyGeneratedStruct的实例
struct FNePyUserStruct : public FNePyStructBase
{
	// tp_alloc
	static PyObject* Alloc(PyTypeObject* PyType, Py_ssize_t NItems);
	// tp_dealloc
	static void Dealloc(PyObject* InSelf);
	// tp_new
	static PyObject* New(PyTypeObject* InPyType, PyObject* InArgs, PyObject* InKwds);
};

void NePyInitUserStruct(PyObject* PyOuterModule);
FNePyUserStruct* NePyUserStructCheck(PyObject* InPyObj);
PyTypeObject* NePyUserStructGetType();
