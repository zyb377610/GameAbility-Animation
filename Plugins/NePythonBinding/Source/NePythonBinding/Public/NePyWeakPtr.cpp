#include "NePyWeakPtr.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"
#include "NePyMemoryAllocator.h"

static PyTypeObject GNePyWeakPtrType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"WeakPtr", /* tp_name */
	sizeof(FNePyWeakPtr), /* tp_basicsize */
};

void FNePyWeakPtr::InitPyType(PyObject* PyOuterModule)
{
	static PyMethodDef PyMethods[] = {
		{ "IsValid", NePyCFunctionCast(&FNePyWeakPtr::IsValid), METH_NOARGS, "() -> bool" },
		{ "IsStale", NePyCFunctionCast(&FNePyWeakPtr::IsStale), METH_NOARGS, "() -> bool" },
		{ "Get", NePyCFunctionCast(&FNePyWeakPtr::Get), METH_NOARGS, "() -> Object" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &GNePyWeakPtrType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_init = (initproc)FNePyWeakPtr::Init;
	PyType->tp_repr = (reprfunc)FNePyWeakPtr::Repr;
	PyType->tp_str = (reprfunc)FNePyWeakPtr::Repr;
	PyType->tp_richcompare = (richcmpfunc)FNePyWeakPtr::RichCompare;
	PyType->tp_hash = (hashfunc)FNePyWeakPtr::Hash;
	PyType->tp_methods = PyMethods;
	PyType_Ready(PyType);

	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "WeakPtr", (PyObject*)PyType);
}

FNePyWeakPtr* FNePyWeakPtr::New(const FWeakObjectPtr& InValue)
{
	FNePyWeakPtr* RetValue = (FNePyWeakPtr*)GNePyWeakPtrType.tp_alloc(&GNePyWeakPtrType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	new(&RetValue->Value) FWeakObjectPtr(InValue);
	return RetValue;
}

FNePyWeakPtr* FNePyWeakPtr::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &GNePyWeakPtrType)
		{
			return (FNePyWeakPtr*)InPyObj;
		}
	}
	return nullptr;
}

int FNePyWeakPtr::Init(FNePyWeakPtr* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	new(&InSelf->Value) FWeakObjectPtr();

	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|O:call", &PyObj))
	{
		return -1;
	}
	if (PyObj)
	{
		if (FNePyWeakPtr* OtherWeakPtr = FNePyWeakPtr::Check(PyObj))
		{
			InSelf->Value = OtherWeakPtr->Value;
		}
		else if (FNePyObjectBase* OtherObject = NePyObjectBaseCheck(PyObj))
		{
			InSelf->Value = OtherObject->Value;
		}
		else
		{
			PyErr_Format(PyExc_TypeError, "Failed to convert init argument '%s' to 'WeakPtr'", Py_TYPE(PyObj)->tp_name);
			return -1;
		}
	}
	return 0;
}

PyObject* FNePyWeakPtr::Repr(FNePyWeakPtr* InSelf)
{
	UObject* Obj = InSelf->Value.Get();
	if (Obj)
	{
		return NePyString_FromString(TCHAR_TO_UTF8(*FString::Printf(TEXT("WeakPtr(\"%s\")"), *Obj->GetPathName())));
	}
	return NePyString_FromString("WeakPtr(stale)");
}


PyObject* FNePyWeakPtr::RichCompare(FNePyWeakPtr* InSelf, PyObject* InOther, int InOp)
{
	FWeakObjectPtr Other;
	if (!NePyBase::ToCpp(InOther, Other))
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	if (InOp != Py_EQ && InOp != Py_NE)
	{
		PyErr_Format(PyExc_TypeError, "WeakPtr: Only == and != comparison is supported");
		return nullptr;
	}

	bool bIdentical = InSelf->Value == Other;
	return PyBool_FromLong(InOp == Py_EQ ? bIdentical : !bIdentical);
}

FNePyHashType FNePyWeakPtr::Hash(FNePyWeakPtr* InSelf)
{
	uint32 WeakObjectPtrHash = GetTypeHash(InSelf->Value);
	return WeakObjectPtrHash != -1 ? WeakObjectPtrHash : 0;
}

PyObject* FNePyWeakPtr::IsValid(FNePyWeakPtr* InSelf)
{
	if (InSelf->Value.IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyWeakPtr::IsStale(FNePyWeakPtr* InSelf)
{
	if (InSelf->Value.IsStale())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyWeakPtr::Get(FNePyWeakPtr* InSelf)
{
	PyObject* PyRet = nullptr;
	NePyBase::ToPy(InSelf->Value.Get(), PyRet);
	return PyRet;
}

NEPYTHONBINDING_API PyTypeObject* NePyWeakPtrGetType()
{
	return &GNePyWeakPtrType;
}