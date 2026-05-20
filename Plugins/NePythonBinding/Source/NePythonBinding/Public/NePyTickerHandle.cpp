#include "NePyTickerHandle.h"
#include "HAL/UnrealMemory.h"
#include "NePyWrapperTypeRegistry.h"

static PyTypeObject FNePyStructType_TickerHandle = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"TickerHandle", /* tp_name */
	sizeof(FNePyStruct_TickerHandle), /* tp_basicsize */
};


static void NePyStructDealloc_TickerHandle(FNePyStruct_TickerHandle* InSelf)
{
	FNePyStruct_TickerHandle::Dealloc(InSelf);
}

int NePyStructInit_TickerHandle(FNePyStruct_TickerHandle* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init TickerHandle directly!");
	return -1;
}

PyObject* FNePyStruct_TickerHandle_IsValid(FNePyStruct_TickerHandle* InSelf)
{
	if (((FNePyTickerHandle*)InSelf->Value)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyStruct_TickerHandle_Reset(FNePyStruct_TickerHandle* InSelf)
{
	((FNePyTickerHandle*)InSelf->Value)->Reset();
	Py_RETURN_NONE;
}

void NePyInitTickerHandle(PyObject* PyOuterModule)
{
	static PyMethodDef PyMethods[] = {
		{"IsValid", NePyCFunctionCast(&FNePyStruct_TickerHandle_IsValid), METH_NOARGS, "(self) -> bool"},
		{"Reset", NePyCFunctionCast(&FNePyStruct_TickerHandle_Reset), METH_NOARGS, "(self) -> None"},
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &FNePyStructType_TickerHandle;
	FNePyStruct_TickerHandle::InitTypeCommon(PyType);
#if ENGINE_MAJOR_VERSION >= 5
	PyType->tp_dealloc = (destructor)&NePyStructDealloc_TickerHandle;
#endif
	PyType->tp_init = (initproc)&NePyStructInit_TickerHandle;
	PyType->tp_methods = PyMethods;
	PyType->tp_base = NePyStructBaseGetType();
	PyType_Ready(PyType);
}

NEPYTHONBINDING_API FNePyStruct_TickerHandle* NePyStructNew_TickerHandle(const FNePyTickerHandle& InValue)
{
	FNePyStruct_TickerHandle* RetValue = PyObject_New(FNePyStruct_TickerHandle, &FNePyStructType_TickerHandle);
	if (FNePyStruct_TickerHandle::Init(RetValue, nullptr, nullptr) < 0)
	{
		Py_DECREF(RetValue);
		return nullptr;
	}
#if ENGINE_MAJOR_VERSION >= 5
	new (RetValue->Value) FNePyTickerHandle(InValue);
#else
	*(FNePyTickerHandle*)RetValue->Value = InValue;
#endif
	return RetValue;
}

NEPYTHONBINDING_API FNePyStruct_TickerHandle* NePyStructCheck_TickerHandle(PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyStructType_TickerHandle))
	{
		return (FNePyStruct_TickerHandle*)InPyObj;
	}
	return nullptr;
}

NEPYTHONBINDING_API PyTypeObject* NePyStructGetType_TickerHandle()
{
	return &FNePyStructType_TickerHandle;
}
