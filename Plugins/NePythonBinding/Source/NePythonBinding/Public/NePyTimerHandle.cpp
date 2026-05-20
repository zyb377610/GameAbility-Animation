#include "NePyTimerHandle.h"
#include "HAL/UnrealMemory.h"
#include "NePyWrapperTypeRegistry.h"

static PyTypeObject FNePyStructType_TimerHandle = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"TimerHandle", /* tp_name */
	sizeof(FNePyStruct_TimerHandle), /* tp_basicsize */
};

int NePyStructInit_TimerHandle(FNePyStruct_TimerHandle* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|O:TimerHandle", &PyObj))
	{
		return -1;
	}
	
	if (!PyObj)
	{
		*(FTimerHandle*)InSelf->Value = FTimerHandle();
	}
	else if (FNePyStruct_TimerHandle* OtherRef = NePyStructCheck_TimerHandle(PyObj))
	{
		*(FTimerHandle*)InSelf->Value = *(FTimerHandle*)OtherRef->Value;
	}
	else
	{
		static_assert(sizeof(FTimerHandle) == sizeof(uint64), "FTimerHandle must has the same size with uint64.");

		uint64 Handle;
		if (!NePyBase::ToCpp(PyObj, Handle))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 must have type 'int'");
			return -1;
		}

		FMemory::Memcpy(InSelf->Value, &Handle, sizeof(Handle));
	}

	return 0;
}

PyObject* NePyStructRepr_TimerHandle(FNePyStruct_TimerHandle* InSelf)
{
	return PyUnicode_FromFormat("<TimerHandle {%s}>", TCHAR_TO_UTF8(*((FTimerHandle*)InSelf->Value)->ToString()));
}

PyObject* NePyStructRichCmp_TimerHandle(FNePyStruct_TimerHandle* InSelf, PyObject* InOther, int InOp)
{
	FTimerHandle* HandlePtr = nullptr;
	if (FNePyStruct_TimerHandle* PyOther = NePyStructCheck_TimerHandle(InOther))
	{
		HandlePtr = (FTimerHandle*)PyOther->Value;
	}

	switch (InOp)
	{
		case Py_EQ:
		{
			if (!HandlePtr)
			{
				Py_RETURN_FALSE;
			}
			if (((FTimerHandle*)InSelf->Value)->operator==(*HandlePtr))
			{
				Py_RETURN_TRUE;
			}
			else
			{
				Py_RETURN_FALSE;
			}
		}
		case Py_NE:
		{
			if (!HandlePtr)
			{
				Py_RETURN_TRUE;
			}
			if (((FTimerHandle*)InSelf->Value)->operator!=(*HandlePtr))
			{
				Py_RETURN_TRUE;
			}
			else
			{
				Py_RETURN_FALSE;
			}
		}
		default:
			break;
	}

	return PyErr_Format(PyExc_NotImplementedError, "this kind of compare is not implemented by TimerHandle");
}

bool NePyStructPropSet_TimerHandle(const FStructProperty* InStructProp, FNePyStructBase* InSelf, void* InBuffer)
{
	if (FNePyStruct_TimerHandle* PyObj = NePyStructCheck_TimerHandle(InSelf))
	{
		InStructProp->Struct->CopyScriptStruct(InBuffer, PyObj->Value);
		return true;
	}
	return false;
}

FNePyStructBase* NePyStructPropGet_TimerHandle(const FStructProperty* InStructProp, const void* InBuffer)
{
	FNePyStruct_TimerHandle* PyObj = PyObject_New(FNePyStruct_TimerHandle, &FNePyStructType_TimerHandle);
#if NEPY_ENABLE_STRUCT_DEEP_ACCESS
	PyObj->Value = (FTimerHandle*)InBuffer;
	PyObj->SelfCreatedValue = false;
#else
	PyObj->Value = (FTimerHandle*)FMemory::Malloc(sizeof(FTimerHandle), alignof(FTimerHandle));
	NePyBase::EnsureCopyToStructSafely<FTimerHandle>(InStructProp, PyObj->Value);
	InStructProp->Struct->CopyScriptStruct(PyObj->Value, InBuffer);
	PyObj->SelfCreatedValue = true;
#endif
	PyObj->PyOuter = nullptr;
	return PyObj;
}

PyObject* FNePyStruct_TimerHandle_IsValid(FNePyStruct_TimerHandle* InSelf)
{
	if (((FTimerHandle*)InSelf->Value)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyStruct_TimerHandle_Invalidate(FNePyStruct_TimerHandle* InSelf)
{
	((FTimerHandle*)InSelf->Value)->Invalidate();
	Py_RETURN_NONE;
}

void NePyInitTimerHandle(PyObject* PyOuterModule)
{
	static PyMethodDef PyMethods[] = {
		{"IsValid", NePyCFunctionCast(&FNePyStruct_TimerHandle_IsValid), METH_NOARGS, "(self) -> bool"},
		{"Invalidate", NePyCFunctionCast(&FNePyStruct_TimerHandle_Invalidate), METH_NOARGS, "(self) -> None"},
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &FNePyStructType_TimerHandle;
	TNePyStructBase<FNePyStruct_TimerHandle>::InitTypeCommon(PyType);
	PyType->tp_init = (initproc)&NePyStructInit_TimerHandle;
	PyType->tp_repr = (reprfunc)&NePyStructRepr_TimerHandle;
	PyType->tp_str = (reprfunc)&NePyStructRepr_TimerHandle;
	PyType->tp_richcompare = (richcmpfunc)&NePyStructRichCmp_TimerHandle;
	PyType->tp_methods = PyMethods;
	PyType->tp_base = NePyStructBaseGetType();
	PyType_Ready(PyType);

	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "TimerHandle", (PyObject*)PyType);

	FNePyStructTypeInfo TypeInfo = {
		PyType,
		ENePyTypeFlags::StaticPyType,
		(NePyStructPropSet)NePyStructPropSet_TimerHandle,
		(NePyStructPropGet)NePyStructPropGet_TimerHandle,
	};
	UScriptStruct* ScriptStruct = TBaseStructure<FTimerHandle>::Get();
	FNePyWrapperTypeRegistry::Get().RegisterWrappedStructType(ScriptStruct, TypeInfo);
}

NEPYTHONBINDING_API FNePyStruct_TimerHandle* NePyStructNew_TimerHandle(const FTimerHandle& InValue)
{
	FNePyStruct_TimerHandle* PyObj = PyObject_New(FNePyStruct_TimerHandle, &FNePyStructType_TimerHandle);
	PyObj->Value = (void*)FMemory::Malloc(sizeof(FTimerHandle), alignof(FTimerHandle));
	new(PyObj->Value) FTimerHandle(InValue);
	PyObj->SelfCreatedValue = true;
	PyObj->PyOuter = nullptr;
	return PyObj;
}

NEPYTHONBINDING_API FNePyStruct_TimerHandle* NePyStructCheck_TimerHandle(PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyStructType_TimerHandle))
	{
		return (FNePyStruct_TimerHandle*)InPyObj;
	}
	return nullptr;
}

NEPYTHONBINDING_API PyTypeObject* NePyStructGetType_TimerHandle()
{
	return &FNePyStructType_TimerHandle;
}

bool NePyBase::ToCpp(PyObject* InPyObj, FTimerHandle& OutVal)
{
	if (FNePyStruct_TimerHandle* PyObj = NePyStructCheck_TimerHandle(InPyObj))
	{
		OutVal = *(FTimerHandle*)PyObj->Value;
		return true;
	}
	return false;
}

bool NePyBase::ToPy(const FTimerHandle& InVal, PyObject*& OutPyObj)
{
	OutPyObj = NePyStructNew_TimerHandle(InVal);
	return true;
}

PyObject* NePyBase::ToPy(const FTimerHandle& InVal)
{
	return NePyStructNew_TimerHandle(InVal);
}
