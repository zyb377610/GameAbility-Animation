#include "NePyScriptStruct.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"

static PyTypeObject FNePyScriptStructType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ScriptStruct", /* tp_name */
	sizeof(FNePyScriptStruct), /* tp_basicsize */
};

// tp_new
PyObject* NePyScriptStruct_New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	if (PyTuple_Size(InArgs) != 3)
	{
		return PyType_GenericNew(InType, InArgs, InKwds);
	}

	FNePyObjectPtr PyClassName = NePyStealReference(PyObject_Str(PyTuple_GetItem(InArgs, 0)));
	PyObject* PyClassAttrs = PyTuple_GetItem(InArgs, 2);
	PyObject* PyBases = PyTuple_GetItem(InArgs, 1);

	const char* StructNameStr;
	if (!NePyBase::ToCpp(PyClassName, StructNameStr))
	{
		UE_LOG(LogNePython, Error, TEXT("Generate unreal struct failed! Can't retrieve class name."));
		return nullptr;
	}

	FString StructName(UTF8_TO_TCHAR(StructNameStr));
	Py_ssize_t PyBasesCount = PyTuple_Size(PyBases);
	PyObject* PyBase = nullptr;
	if (PyTuple_Check(PyBases) && PyBasesCount >= 1)
	{
		PyBase = PyTuple_GetItem(PyBases, 0);
	}

	UScriptStruct* SuperStruct = nullptr;
	if (PyBase)
	{
		SuperStruct = NePyBase::ToCppScriptStruct(PyBase);
	}

	const FNePyStructTypeInfo* BaseTypeInfo = nullptr;
	if (SuperStruct)
	{
		BaseTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(SuperStruct);
		if (!BaseTypeInfo)
		{
			UE_LOG(LogNePython, Error, TEXT("Generate unreal struct failed! '%s'\'s Parent struct '%s' don't have a python wrapper type."),
				*StructName, *SuperStruct->GetName());
			return nullptr;
		}
	}

	FNePyObjectPtr NewArgs;
	if (BaseTypeInfo)
	{
		PyTypeObject* PyBaseType = BaseTypeInfo->TypeObject;
		FNePyObjectPtr NewBases = NePyStealReference(PyTuple_New(PyBasesCount));
		Py_INCREF(PyBaseType);
		PyTuple_SetItem(NewBases, 0, (PyObject*)PyBaseType);
		for (Py_ssize_t Index = 1; Index < PyBasesCount; ++Index)
		{
			PyObject* PyOtherBase = PyTuple_GetItem(PyBases, Index);
			Py_INCREF(PyOtherBase);
			PyTuple_SetItem(NewBases, Index, PyOtherBase);
		}

		NewArgs = NePyStealReference(PyTuple_New(3));
		PyTuple_SetItem(NewArgs, 0, PyClassName.Release());
		PyTuple_SetItem(NewArgs, 1, NewBases.Release());
		Py_INCREF(PyClassAttrs);
		PyTuple_SetItem(NewArgs, 2, PyClassAttrs);
	}
	else
	{
		NewArgs = NePyNewReference(InArgs);
	}

	return PyType_Type.tp_new(&PyType_Type, NewArgs, InKwds);
}

// tp_repr
static PyObject* NePyScriptStruct_Repr(FNePyScriptStruct* InSelf)
{
	if (!FNePyHouseKeeper::Get().IsValid(InSelf))
	{
		return PyUnicode_FromFormat("<Invalid UScriptStruct at %p>", InSelf->Value);
	}
	return PyUnicode_FromFormat("<ScriptStruct '%s' at %p>", TCHAR_TO_UTF8(*InSelf->GetValue()->GetName()), InSelf->GetValue());
}

PyObject* NePyScriptStruct_GetStructFlags(FNePyScriptStruct* InSelf)
{
	return PyLong_FromUnsignedLongLong((uint64)InSelf->GetValue()->StructFlags);
}

PyObject* NePyScriptStruct_SetStructFlags(FNePyScriptStruct* InSelf, PyObject* InArgs)
{
	uint64 Flags;
	if (!PyArg_ParseTuple(InArgs, "K:SetStructFlags", &Flags))
	{
		return nullptr;
	}

	InSelf->GetValue()->StructFlags = (EStructFlags)Flags;
	Py_RETURN_NONE;
}

PyObject* NePyScriptStruct_GetSuperStruct(FNePyScriptStruct* InSelf)
{
	UStruct* SuperStruct = InSelf->GetValue()->GetSuperStruct();
	return NePyBase::ToPy(SuperStruct);
}

PyObject* NePyScriptStruct_IsChildOf(FNePyScriptStruct* InSelf, PyObject* InArgs)
{
	PyObject* PyObj;
	if (!PyArg_ParseTuple(InArgs, "O", &PyObj))
	{
		return nullptr;
	}

	UStruct* Struct = NePyBase::ToCppStruct(PyObj);
	if (!Struct)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UStruct");
	}

	if (InSelf->GetValue()->IsChildOf(Struct))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyMethodDef FNePyScriptStructType_methods[] = {
	{"GetStructFlags", NePyCFunctionCast(&NePyScriptStruct_GetStructFlags), METH_NOARGS, "(self) -> int"},
	{"SetStructFlags", NePyCFunctionCast(&NePyScriptStruct_SetStructFlags), METH_VARARGS, "(self, InStructFlags: int) -> None"},
	{"GetSuperStruct", NePyCFunctionCast(&NePyScriptStruct_GetSuperStruct), METH_NOARGS, "(self) -> ScriptStruct"},
	{"IsChildOf", NePyCFunctionCast(&NePyScriptStruct_IsChildOf), METH_VARARGS, "(self, InScriptStruct: ScriptStruct) -> bool"},
	{ NULL } /* Sentinel */
};

FNePyScriptStruct* NePyScriptStructNew(UScriptStruct* InValue, PyTypeObject* InPyType)
{
	check(PyType_IsSubtype(InPyType, &FNePyScriptStructType));
	FNePyScriptStruct* RetValue = PyObject_New(FNePyScriptStruct, InPyType);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyScriptStruct* NePyScriptStructCheck(const PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyScriptStructType))
	{
		return (FNePyScriptStruct*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyScriptStructGetType()
{
	return &FNePyScriptStructType;
}

void NePyInitScriptStruct(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyScriptStructType;
	NePyObjectType_InitCommon(PyType);
	PyType->tp_new = NePyScriptStruct_New;
	PyType->tp_methods = FNePyScriptStructType_methods;
	PyType->tp_base = NePyObjectBaseGetType();
	PyType->tp_repr = (reprfunc)&NePyScriptStruct_Repr;
	PyType->tp_str = (reprfunc)&NePyScriptStruct_Repr;
	PyType->tp_flags &= ~Py_TPFLAGS_BASETYPE;
	PyType_Ready(PyType);

	PyModule_AddObject(PyOuterModule, "ScriptStruct", (PyObject*)PyType);

	FNePyObjectTypeInfo TypeInfo = {
		PyType,
		ENePyTypeFlags::StaticPyType,
		(NePyObjectNewFunc)NePyScriptStructNew,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(UScriptStruct::StaticClass(), TypeInfo);
}

