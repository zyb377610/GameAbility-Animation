#include "NePySoftPtr.h"
#include "NePyObjectBase.h"
#include "NePyMemoryAllocator.h"
//#include "NePy/Auto/Core/NePyStruct_SoftObjectPath.h"


static PyTypeObject FNePySoftPtrType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"SoftPtr", /* tp_name */
	sizeof(FNePySoftPtr), /* tp_basicsize */
};

void FNePySoftPtr::InitPyType(PyObject * PyOuterModule)
{
	static PyMethodDef PyMethods[] = {
		{ "IsValid", NePyCFunctionCast(&FNePySoftPtr::IsValid), METH_NOARGS, "() -> bool" },
		{ "IsNull", NePyCFunctionCast(&FNePySoftPtr::IsNull), METH_NOARGS, "() -> bool" },
		{ "IsPending", NePyCFunctionCast(&FNePySoftPtr::IsPending), METH_NOARGS, "() -> bool" },
		{ "IsStale", NePyCFunctionCast(&FNePySoftPtr::IsStale), METH_NOARGS, "() -> bool" },
		{ "Get", NePyCFunctionCast(&FNePySoftPtr::Get), METH_NOARGS, "() -> Object" },
		{ "GetAssetName", NePyCFunctionCast(&FNePySoftPtr::GetAssetName), METH_NOARGS, "() -> str" },
		{ "GetLongPackageName", NePyCFunctionCast(&FNePySoftPtr::GetLongPackageName), METH_NOARGS, "() -> str" },
		//{ "ToSoftObjectPath", NePyCFunctionCast(&FNePySoftPtr::ToSoftObjectPath), METH_NOARGS, "() -> SoftObjectPath" },
		{ "LoadSynchronous", NePyCFunctionCast(&FNePySoftPtr::LoadSynchronous), METH_NOARGS, "() -> None" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &FNePySoftPtrType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_init = (initproc)&FNePySoftPtr::Init;
	PyType->tp_repr = (reprfunc)&FNePySoftPtr::Repr;
	PyType->tp_str = (reprfunc)&FNePySoftPtr::Repr;
	PyType->tp_richcompare = (richcmpfunc)&FNePySoftPtr::RichCompare;
	PyType->tp_hash = (hashfunc)&FNePySoftPtr::Hash;
	PyType->tp_methods = PyMethods;
	PyType_Ready(PyType);

	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "SoftPtr", (PyObject*)PyType);
}

FNePySoftPtr* FNePySoftPtr::New(const FSoftObjectPtr& InValue)
{
	FNePySoftPtr* RetValue = (FNePySoftPtr*)FNePySoftPtrType.tp_alloc(&FNePySoftPtrType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	RetValue->Value = InValue;
	return RetValue;
}

FNePySoftPtr* FNePySoftPtr::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePySoftPtrType)
		{
			return (FNePySoftPtr*)InPyObj;
		}
	}
	return nullptr;
}

int FNePySoftPtr::Init(FNePySoftPtr* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|O:call", &PyObj))
	{
		return -1;
	}
	if (PyObj)
	{
		if (FNePySoftPtr* OtherSoftObject = FNePySoftPtr::Check(PyObj))
		{
			InSelf->Value = OtherSoftObject->Value;
		}
		else if (FNePyObjectBase* OtherObject = NePyObjectBaseCheck(PyObj))
		{
			InSelf->Value = FSoftObjectPtr(OtherObject->Value);
		}
		//else if (FNePyStruct_SoftObjectPath* OtherSoftPath = NePyStructCheck_SoftObjectPath(PyObj))
		//{
		//	InSelf->Value = FSoftObjectPtr(OtherSoftPath->Value);
		//}
		else
		{
			PyErr_Format(PyExc_TypeError, "Failed to convert init argument '%s' to 'SoftObjectPtr'", Py_TYPE(PyObj)->tp_name);
			return -1;
		}
	}
	return 0;
}

PyObject* FNePySoftPtr::Repr(FNePySoftPtr* InSelf)
{
	return NePyString_FromString(TCHAR_TO_UTF8(*FString::Printf(TEXT("SoftPtr(\"%s\")"), *InSelf->Value.ToString())));
}


PyObject* FNePySoftPtr::RichCompare(FNePySoftPtr* InSelf, PyObject* InOther, int InOp)
{
	FSoftObjectPtr Other;
	if (!NePyBase::ToCpp(InOther, Other))
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	if (InOp != Py_EQ && InOp != Py_NE)
	{
		PyErr_Format(PyExc_TypeError, "SoftPtr: Only == and != comparison is supported");
		return nullptr;
	}

	bool bIdentical = InSelf->Value == Other;
	return PyBool_FromLong(InOp == Py_EQ ? bIdentical : !bIdentical);
}

FNePyHashType FNePySoftPtr::Hash(FNePySoftPtr* InSelf)
{
	uint32 SoftObjectPtrHash = GetTypeHash(InSelf->Value);
	return SoftObjectPtrHash != -1 ? SoftObjectPtrHash : 0;
}

PyObject* FNePySoftPtr::IsValid(FNePySoftPtr* InSelf)
{
	if (InSelf->Value.IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePySoftPtr::IsNull(FNePySoftPtr* InSelf)
{
	if (InSelf->Value.IsNull())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePySoftPtr::IsPending(FNePySoftPtr* InSelf)
{
	if (InSelf->Value.IsPending())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePySoftPtr::IsStale(FNePySoftPtr* InSelf)
{
	if (InSelf->Value.IsStale())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePySoftPtr::Get(FNePySoftPtr* InSelf)
{
	PyObject* PyRet = nullptr;
	NePyBase::ToPy(InSelf->Value.Get(), PyRet);
	return PyRet;
}

PyObject* FNePySoftPtr::GetAssetName(FNePySoftPtr* InSelf)
{
	PyObject* PyRet = nullptr;
	NePyBase::ToPy(InSelf->Value.GetAssetName(), PyRet); 
	return PyRet;
}

PyObject* FNePySoftPtr::GetLongPackageName(FNePySoftPtr* InSelf)
{
	PyObject* PyRet = nullptr;
	NePyBase::ToPy(InSelf->Value.GetLongPackageName(), PyRet);
	return PyRet;
}

//PyObject* FNePySoftPtr::ToSoftObjectPath(FNePySoftPtr* InSelf)
//{
//	PyObject* PyRet = nullptr; 
//	NePyBase::ToPy(InSelf->Value.ToSoftObjectPath(), PyRet); 
//	return PyRet;
//}

PyObject* FNePySoftPtr::LoadSynchronous(FNePySoftPtr* InSelf)
{
	PyObject* PyRet = nullptr;
	NePyBase::ToPy(InSelf->Value.LoadSynchronous(), PyRet); 
	return PyRet;
}

NEPYTHONBINDING_API PyTypeObject* NePySoftPtrGetType()
{
	return &FNePySoftPtrType;
}
