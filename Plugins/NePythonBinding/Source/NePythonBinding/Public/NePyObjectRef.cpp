#include "NePyObjectRef.h"
#include "NePyGIL.h"
#include "NePyWrapperTypeRegistry.h"

bool FNePyObjectRef::Boxing(PyObject* InPyObj, FNePyObjectRef& OutPyObjRef)
{
	OutPyObjRef = FNePyObjectRef(InPyObj);
	return true;
}

PyObject* FNePyObjectRef::Unboxing(FNePyObjectRef& InPyObjRef)
{
	PyObject* RetValue = InPyObjRef.Value;
	InPyObjRef.Value = nullptr;

	if (!RetValue)
	{
		Py_INCREF(Py_None);
		RetValue = Py_None;
	}
	return RetValue;
}

FNePyObjectRef::FNePyObjectRef()
	: Value(nullptr)
{
}

FNePyObjectRef::FNePyObjectRef(PyObject* InPyObj)
	: Value(InPyObj)
{
	Py_XINCREF(Value);
}

FNePyObjectRef::FNePyObjectRef(const FNePyObjectRef& InOther)
	: Value(InOther.Value)
{
	if (Value)
	{
		FNePyScopedGIL GIL;
		Py_INCREF(Value);
	}
}

FNePyObjectRef::FNePyObjectRef(FNePyObjectRef&& InOther)
	: Value(InOther.Value)
{
	InOther.Value = nullptr;
}

FNePyObjectRef::~FNePyObjectRef()
{
	if (Value)
	{
		if (Py_IsInitialized())
		{
			FNePyScopedGIL GIL;
			Py_DECREF(Value);
		}
		Value = nullptr;
	}
}

FNePyObjectRef& FNePyObjectRef::operator=(const FNePyObjectRef& InOther)
{
	if (this != &InOther)
	{
		FNePyScopedGIL GIL;
		Py_XDECREF(this->Value);
		this->Value = InOther.Value;
		Py_XINCREF(this->Value);
	}
	return *this;
}

FNePyObjectRef& FNePyObjectRef::operator=(FNePyObjectRef&& InOther)
{
	if (this != &InOther)
	{
		if (this->Value)
		{
			FNePyScopedGIL GIL;
			Py_DECREF(this->Value);
		}
		this->Value = InOther.Value;
		InOther.Value = nullptr;
	}
	return *this;
}

static PyTypeObject FNePyStructType_NePyObjectRef = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyObjectRef", /* tp_name */
	sizeof(FNePyStruct_NePyObjectRef), /* tp_basicsize */
};


// tp_dealloc
void FNePyStructDealloc_NePyObjectRef(FNePyStruct_NePyObjectRef* InSelf)
{
	FNePyObjectRef* ObjRef = (FNePyObjectRef*)InSelf->Value;
	if (ObjRef && ObjRef->Value)
	{
		Py_DECREF(ObjRef->Value);
		ObjRef->Value = nullptr;
	}
	InSelf->ob_type->tp_free(InSelf);
}

// tp_init
int FNePyStructInit_NePyObjectRef(FNePyStruct_NePyObjectRef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:NePyObjectRef", &PyObj))
	{
		return -1;
	}

	ensureMsgf(InSelf->Value, TEXT("FNePyStruct_NePyObjectRef Value is null in __init__"));

	if (FNePyStruct_NePyObjectRef* OtherRef = NePyStructCheck_NePyObjectRef(PyObj))
	{
		*(FNePyObjectRef*)InSelf->Value = *(FNePyObjectRef*)OtherRef->Value;
	}
	else
	{
		*(FNePyObjectRef*)InSelf->Value = FNePyObjectRef(PyObj);
	}
	return 0;
}

PyObject* FNePyStruct_NePyObjectRef_GetValue(FNePyStruct_NePyObjectRef* InSelf)
{
	PyObject* RetObj = ((FNePyObjectRef*)InSelf->Value)->Value;
	if (!RetObj)
	{
		RetObj = Py_None;
	}

	Py_INCREF(RetObj);
	return RetObj;
}

static PyMethodDef FNePyStructType_NePyObjectRef_methods[] = {
	{"GetValue", NePyCFunctionCast(&FNePyStruct_NePyObjectRef_GetValue), METH_NOARGS, "(self) -> typing.Any"},
	{ NULL } /* Sentinel */
};

bool NePyStructPropSet_NePyObjectRef(const FStructProperty* InStructProp, PyObject* InSelf, void* InBuffer)
{
	if (FNePyStruct_NePyObjectRef* PyObj = NePyStructCheck_NePyObjectRef(InSelf))
	{
		InStructProp->Struct->CopyScriptStruct(InBuffer, PyObj->Value);
	}
	else
	{
		FNePyObjectRef Temp;
		Temp.Value = InSelf;
		InStructProp->Struct->CopyScriptStruct(InBuffer, &Temp);
		Temp.Value = nullptr;
	}
	return true;
}

PyObject* NePyStructPropGet_NePyObjectRef(const FStructProperty* InStructProp, const void* InBuffer)
{
	FNePyObjectRef Temp;
	InStructProp->Struct->CopyScriptStruct(&Temp, InBuffer); // 这里会添加PyObj的引用计数
	PyObject* PyObj = Temp.Value;
	Temp.Value = nullptr;
	if (!PyObj)
	{
		Py_RETURN_NONE;
	}
	return PyObj;
}

void NePyInitObjectRef(PyObject* PyOuterModule)
{
	PyTypeObject* NePyStructType = &FNePyStructType_NePyObjectRef;
	TNePyStructBase<FNePyObjectRef>::InitTypeCommon(NePyStructType);
	NePyStructType->tp_dealloc = (destructor)&FNePyStructDealloc_NePyObjectRef;
	NePyStructType->tp_init = (initproc)&FNePyStructInit_NePyObjectRef;
	NePyStructType->tp_methods = FNePyStructType_NePyObjectRef_methods;
	NePyStructType->tp_base = NePyStructBaseGetType();
	PyType_Ready(NePyStructType);

	Py_INCREF(NePyStructType);
	PyModule_AddObject(PyOuterModule, "NePyObjectRef", (PyObject*)NePyStructType);

	FNePyStructTypeInfo TypeInfo = {
		NePyStructType,
		ENePyTypeFlags::StaticPyType,
		(NePyStructPropSet)NePyStructPropSet_NePyObjectRef,
		(NePyStructPropGet)NePyStructPropGet_NePyObjectRef,
	};
	UScriptStruct* ScriptStruct = TBaseStructure<FNePyObjectRef>::Get();
	FNePyWrapperTypeRegistry::Get().RegisterWrappedStructType(ScriptStruct, TypeInfo);
}

FNePyStruct_NePyObjectRef* NePyStructNew_NePyObjectRef(const FNePyObjectRef& InValue)
{
	FNePyStruct_NePyObjectRef* RetValue = PyObject_New(FNePyStruct_NePyObjectRef, &FNePyStructType_NePyObjectRef);
	if (FNePyStruct_NePyObjectRef::Init(RetValue, nullptr, nullptr) < 0)
	{
		Py_DECREF(RetValue);
		return nullptr;
	}
	new (RetValue->Value) FNePyObjectRef(InValue);
	return RetValue;
}

FNePyStruct_NePyObjectRef* NePyStructCheck_NePyObjectRef(PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyStructType_NePyObjectRef))
	{
		return (FNePyStruct_NePyObjectRef*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyStructGetType_NePyObjectRef()
{
	return &FNePyStructType_NePyObjectRef;
}
