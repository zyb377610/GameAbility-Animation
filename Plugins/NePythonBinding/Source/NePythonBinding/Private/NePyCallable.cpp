#include "NePyCallable.h"
#include "NePyObjectBase.h"
#include "NePyDescriptor.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyGenUtil.h"
#include "UObject/Class.h"
#include "UObject/StructOnScope.h"

static PyTypeObject NePyCallableType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Callable", /* tp_name */
	sizeof(FNePyCallable), /* tp_basicsize */
};

void FNePyCallable::InitPyType()
{
	PyTypeObject* PyType = &NePyCallableType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
	PyType->tp_dealloc = (destructor)FNePyCallable::Dealloc;
	PyType->tp_repr = (reprfunc)FNePyCallable::Repr;
	PyType->tp_str = (reprfunc)FNePyCallable::Repr;
	PyType->tp_call = (ternaryfunc)FNePyCallable::Call;
	PyType->tp_traverse = (traverseproc)FNePyCallable::Traverse;
	PyType->tp_clear = (inquiry)FNePyCallable::Clear;
	PyType->tp_richcompare = (richcmpfunc)FNePyCallable::RichCompare;
	PyType_Ready(PyType);
}

FNePyCallable* FNePyCallable::New(NePyGenUtil::FMethodDef* InMethodDef, PyObject* InSelfObject)
{
	check(InMethodDef);
	check(InSelfObject);
	FNePyCallable* PyRet = (FNePyCallable*)PyObject_GC_New(FNePyCallable, &NePyCallableType);
	Py_INCREF(InMethodDef);
	PyRet->MethodDef = InMethodDef;
	Py_INCREF(InSelfObject);
	PyRet->SelfObject = InSelfObject;
	PyObject_GC_Track(PyRet);
	return PyRet;
}

FNePyCallable* FNePyCallable::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyCallableType))
	{
		return (FNePyCallable*)InPyObj;
	}
	return nullptr;
}

PyObject* FNePyCallable::CallMethod(NePyGenUtil::FMethodDef* InMethodDef, PyObject* SelfObject, PyObject* InArgs, PyObject* InKwds)
{
	if (!IsValid(InMethodDef->Func))
	{
		PyErr_Format(PyExc_Exception, "method '%s' is in invalid state",
			InMethodDef->FuncName.GetData());
		return nullptr;
	}

	UObject* Object;
	if (InMethodDef->bIsStatic)
	{
		PyTypeObject* PyType = (PyTypeObject*)SelfObject;
		const UClass* Class = FNePyWrapperTypeRegistry::Get().GetClassByPyType(PyType);
		if (!Class)
		{
			PyErr_Format(PyExc_Exception, "cant retrieve UClass of type '%s' when calling method '%s'",
				PyType->tp_name, InMethodDef->FuncName.GetData());
			return nullptr;
		}
		Object = Class->GetDefaultObject();
	}
	else
	{
		Object = ((FNePyObjectBase*)SelfObject)->Value;
	}

	if (!IsValid(Object))
	{
		PyErr_Format(PyExc_Exception, "'self' underlying UObject is invalid when calling method '%s'",
			InMethodDef->FuncName.GetData());
		return nullptr;
	}

	if (InKwds && PyDict_Size(InKwds) > 0)
	{
		PyErr_Format(PyExc_TypeError, "%.200s() takes no keyword arguments",
			InMethodDef->FuncName.GetData());
		return nullptr;
	}

	if (InMethodDef->InputParams.Num() == 0)
	{
		return CallMethodNoArgs(InMethodDef, Object, InArgs, InKwds);
	}
	return CallMethodWithArgs(InMethodDef, Object, InArgs, InKwds);
}

void FNePyCallable::Dealloc(FNePyCallable* InSelf)
{
	PyObject_GC_UnTrack((PyObject*)InSelf);
	FNePyCallable::Clear(InSelf);
	PyObject_GC_Del(InSelf);
}

int FNePyCallable::Clear(FNePyCallable* InSelf)
{
	Py_CLEAR(InSelf->MethodDef);
	Py_CLEAR(InSelf->SelfObject);
	return 0;
}

PyObject* FNePyCallable::Repr(FNePyCallable* InSelf)
{
	FNePyObjectPtr SelfObjectRepr = NePyStealReference(PyObject_Repr(InSelf->SelfObject));
	PyObject* PyRet = NePyString_FromFormat("<method %s of %s>",
		InSelf->MethodDef->FuncName.GetData(), NePyString_AsString(SelfObjectRepr));
	return PyRet;
}

PyObject* FNePyCallable::Call(FNePyCallable* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return CallMethod(InSelf->MethodDef, InSelf->SelfObject, InArgs, InKwds);
}

int FNePyCallable::Traverse(FNePyCallable* InSelf, visitproc visit, void* arg)
{
	Py_VISIT(InSelf->MethodDef);
	Py_VISIT(InSelf->SelfObject);
	return 0;
}

PyObject* FNePyCallable::RichCompare(PyObject* InSelf, PyObject* InOther, int Op)
{
	if ((Op != Py_EQ && Op != Py_NE) ||
		!Check(InSelf) ||
		!Check(InOther))
	{
		Py_INCREF(Py_NotImplemented);
		return Py_NotImplemented;
	}

	FNePyCallable* Self = (FNePyCallable*)InSelf;
	FNePyCallable* Other = (FNePyCallable*)InOther;
	bool bEqual = Self->MethodDef == Other->MethodDef;
	if (bEqual)
	{
		bEqual = Self->SelfObject == Other->SelfObject;
	}

	PyObject* PyRet;
	if (Op == Py_EQ)
	{
		PyRet = bEqual ? Py_True : Py_False;
	}
	else
	{
		PyRet = bEqual ? Py_False : Py_True;
	}
	Py_INCREF(PyRet);
	return PyRet;
}

PyObject* FNePyCallable::CallMethodNoArgs(NePyGenUtil::FMethodDef* InMethodDef, UObject* InObject, PyObject* InArgs, PyObject* InKwds)
{
	Py_ssize_t ArgCount = PyTuple_GET_SIZE(InArgs);
	if (ArgCount > 0)
	{
		PyErr_Format(PyExc_TypeError, "%.200s() takes no arguments (%zd given)",
			InMethodDef->FuncName.GetData(), ArgCount);
		return nullptr;
	}

	if (InMethodDef->Func->ChildProperties == nullptr)
	{
		// Simple case, no parameters or return value
		FScopeCycleCounterUObject ObjectScope(InObject);
		FScopeCycleCounterUObject FunctionScope(InMethodDef->Func);
		InObject->ProcessEvent(InMethodDef->Func, nullptr);
		Py_RETURN_NONE;
	}

	// Return value requires that we create a params struct to hold the result
	FStructOnScope FuncParams(InMethodDef->Func);
	FScopeCycleCounterUObject ObjectScope(InObject);
	FScopeCycleCounterUObject FunctionScope(InMethodDef->Func);
	InObject->ProcessEvent(InMethodDef->Func, FuncParams.GetStructMemory());
	return NePyGenUtil::PackReturnValues(FuncParams.GetStructMemory(), InMethodDef->OutputParams, InObject);
}

PyObject* FNePyCallable::CallMethodWithArgs(NePyGenUtil::FMethodDef* InMethodDef, UObject* InObject, PyObject* InArgs, PyObject* InKwds)
{
	const Py_ssize_t NumPyArgs = PyTuple_GET_SIZE(InArgs);
	if (NumPyArgs != InMethodDef->InputParams.Num())
	{
		PyErr_Format(PyExc_TypeError, "%.200s() takes %d arguments (%ld given)",
			InMethodDef->FuncName.GetData(), InMethodDef->InputParams.Num(), NumPyArgs);
		return nullptr;
	}

	FStructOnScope FuncParams(InMethodDef->Func);
	for (int32 Index = 0; Index < NumPyArgs; ++Index)
	{
		auto& InputParam = InMethodDef->InputParams[Index];
		const FProperty* Prop = InputParam.Prop;
		auto& Converter = InputParam.Converter;
		PyObject* PyArg = PyTuple_GetItem(InArgs, Index);
		if (!Converter(PyArg, Prop, Prop->ContainerPtrToValuePtr<void>(FuncParams.GetStructMemory()), InObject))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyArg, Prop);
			return nullptr;
		}
	}

	FScopeCycleCounterUObject ObjectScope(InObject);
	FScopeCycleCounterUObject FunctionScope(InMethodDef->Func);
	InObject->ProcessEvent(InMethodDef->Func, FuncParams.GetStructMemory());
	return NePyGenUtil::PackReturnValues(FuncParams.GetStructMemory(), InMethodDef->OutputParams, InObject);
}