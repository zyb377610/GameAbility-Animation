#include "NePyFieldPath.h"
#include "NePyBase.h"
#include "NePyMemoryAllocator.h"

static PyTypeObject NePyFieldPathType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FieldPath", /* tp_name */
	sizeof(FNePyFieldPath), /* tp_basicsize */
};

void FNePyFieldPath::InitPyType(PyObject* PyOuterModule)
{
	static PyMethodDef PyMethods[] = {
		{ "IsValid", NePyCFunctionCast(&FNePyFieldPath::IsValid), METH_NOARGS, "() -> bool" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &NePyFieldPathType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_init = (initproc)&FNePyFieldPath::Init;
	PyType->tp_repr = (reprfunc)&FNePyFieldPath::Repr;
	PyType->tp_str = (reprfunc)&FNePyFieldPath::Str;
	PyType->tp_richcompare = (richcmpfunc)&FNePyFieldPath::RichCompare;
	PyType->tp_hash = (hashfunc)&FNePyFieldPath::Hash;
	PyType->tp_methods = PyMethods;
	PyType_Ready(PyType);

	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "FieldPath", (PyObject*)PyType);
}

FNePyFieldPath* FNePyFieldPath::New(const FFieldPath& InValue)
{
	FNePyFieldPath* RetValue = (FNePyFieldPath*)NePyFieldPathType.tp_alloc(&NePyFieldPathType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyFieldPath* FNePyFieldPath::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyFieldPathType))
	{
		return (FNePyFieldPath*)InPyObj;
	}
	return nullptr;
}

int FNePyFieldPath::Init(FNePyFieldPath* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|O:call", &PyObj))
	{
		return -1;
	}

	if (PyObj)
	{
		if (!NePyBase::ToCpp(PyObj, InSelf->Value))
		{
			PyErr_Format(PyExc_TypeError, "Failed to convert init argument '%s' to 'FieldPath'", Py_TYPE(PyObj)->tp_name);
			return -1;
		}
	}
	// else: Default initialization. "f = unreal.FieldPath()".

	return 0;
}

PyObject* FNePyFieldPath::Repr(FNePyFieldPath* InSelf)
{
	return NePyString_FromString(TCHAR_TO_UTF8(*FString::Printf(TEXT("FieldPath(\"%s\")"), *InSelf->Value.ToString())));
}

PyObject* FNePyFieldPath::Str(FNePyFieldPath* InSelf)
{
	return NePyString_FromString(TCHAR_TO_UTF8(*InSelf->Value.ToString()));
}

PyObject* FNePyFieldPath::RichCompare(FNePyFieldPath* InSelf, PyObject* InOther, int InOp)
{
	FFieldPath Other;
	if (!NePyBase::ToCpp(InOther, Other))
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	if (InOp != Py_EQ && InOp != Py_NE)
	{
		PyErr_Format(PyExc_TypeError, "FieldPath: Only == and != comparison is supported");
		return nullptr;
	}

	bool bIdentical = InSelf->Value == Other;
	return PyBool_FromLong(InOp == Py_EQ ? bIdentical : !bIdentical);
}

FNePyHashType FNePyFieldPath::Hash(FNePyFieldPath* InSelf)
{
	uint32 FieldPathHash = GetTypeHash(InSelf->Value);
	return FieldPathHash != -1 ? FieldPathHash : 0;
}

PyObject* FNePyFieldPath::IsValid(FNePyFieldPath* InSelf)
{
	if (InSelf->Value.TryToResolvePath(nullptr) != nullptr)
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}
