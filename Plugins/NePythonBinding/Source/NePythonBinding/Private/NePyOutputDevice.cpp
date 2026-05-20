#include "NePyOutputDevice.h"


static PyMethodDef FNePyOutputDevice_methods[] = {
	{ NULL }  /* Sentinel */
};

static PyObject* FNePyOutputDevice_Str(FNePyOutputDevice* InSelf)
{
	return PyUnicode_FromFormat("<FNeOutputDevice '%p'>",
		InSelf->Device);
}

static void FNePyOutputDevice_Dealloc(FNePyOutputDevice* InSelf)
{
	if (InSelf->Device)
	{
		delete(InSelf->Device);
	}
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

static PyTypeObject FNePyOutputDeviceType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"FNeOutputDevice", /* tp_name */
	sizeof(FNePyOutputDevice), /* tp_basicsize */
};

static int FNePyOutputDevice_Init(FNePyOutputDevice* InSelf, PyObject* InArgs, PyObject* InKwargs)
{
	InSelf->Device = nullptr;
	PyObject* PySerialize;
	if (!PyArg_ParseTuple(InArgs, "O", &PySerialize))
	{
		return -1;
	}

	if (!PyCallable_Check(PySerialize))
	{
		PyErr_SetString(PyExc_TypeError, "argument is not a callable");
		return -1;
	}

	InSelf->Device = new FNeOutputDevice(PySerialize);
	return 0;
}

void NePyInitOutputDevice(PyObject* PyOuterModule)
{
	PyTypeObject* NePyType = &FNePyOutputDeviceType;
	NePyType->tp_new = PyType_GenericNew;
	NePyType->tp_init = (initproc)FNePyOutputDevice_Init;
	NePyType->tp_dealloc = (destructor)FNePyOutputDevice_Dealloc;
	NePyType->tp_repr = (reprfunc)FNePyOutputDevice_Str;
	NePyType->tp_str = (reprfunc)FNePyOutputDevice_Str;
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT;
	NePyType->tp_methods = FNePyOutputDevice_methods;
	if (PyType_Ready(NePyType) < 0)
		return;

	Py_INCREF(NePyType);
	PyModule_AddObject(PyOuterModule, "FNeOutputDevice", (PyObject*)NePyType);
}

