#pragma once
#include "NePyBase.h"
#include "InputCoreTypes.h"
#include "Components/InputComponent.h"
#include "NePyObjectHolder.h"
#include "NePyStruct_Key.h"
#include "NePyStruct_Vector.h"

class FNePyInputBindingFunctor : public FNePyObjectHolder
{
public:
	using FNePyObjectHolder::FNePyObjectHolder;
	FNePyInputBindingFunctor(const FNePyInputBindingFunctor& Other) : FNePyObjectHolder(Other) {}
	FNePyInputBindingFunctor(FNePyInputBindingFunctor&& Other) : FNePyObjectHolder(Other) {}

	void operator()()
	{
		check(this->Value)
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(this->Value, nullptr));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}

	void operator()(float AxisValue)
	{
		check(this->Value);
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(1));
		PyTuple_SetItem(PyArgs, 0, PyFloat_FromDouble(AxisValue));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(this->Value, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}

	void operator()(FVector AxisValue)
	{
		check(this->Value);
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(1));
		PyTuple_SetItem(PyArgs, 0, NePyStructNew_Vector(AxisValue));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(this->Value, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}

	void operator()(ETouchIndex::Type FingerIndex, FVector Location)
	{
		check(this->Value);
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(2));
#if PY_MAJOR_VERSION < 3
		PyTuple_SetItem(PyArgs, 0, PyInt_FromLong(FingerIndex));
#else
		PyTuple_SetItem(PyArgs, 0, PyLong_FromLong(FingerIndex));
#endif
		PyTuple_SetItem(PyArgs, 1, NePyStructNew_Vector(Location));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(this->Value, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}

private:
	FNePyInputBindingFunctor() = delete;
	FNePyInputBindingFunctor& operator=(const FNePyInputBindingFunctor& InOther) = delete;
	FNePyInputBindingFunctor& operator=(FNePyInputBindingFunctor&& InOther) = delete;
};

PyObject* NePyInputComponent_GetAxisValue(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetAxisValue'"))
	{
		return nullptr;
	}

	char* AxisName;
	if (!PyArg_ParseTuple(InArgs, "s:GetAxisValue", &AxisName))
	{
		return nullptr;
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	return Py_BuildValue("f", InputComponent->GetAxisValue(FName(UTF8_TO_TCHAR(AxisName))));
}

PyObject* NePyInputComponent_GetAxisKeyValue(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetAxisKeyValue'"))
	{
		return nullptr;
	}

	FKey AxisKey;
	if (FNePyStruct_Key* PyAxisKey = NePyStructCheck_Key(InArg))
	{
		AxisKey = *(FKey*)PyAxisKey->Value;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "argument type must be 'FKey'");
		return nullptr;
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	return Py_BuildValue("f", InputComponent->GetAxisKeyValue(AxisKey));
}


PyObject* NePyInputComponent_GetVectorAxisValue(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetVectorAxisValue'"))
	{
		return nullptr;
	}

	FKey AxisKey;
	if (FNePyStruct_Key* PyAxisKey = NePyStructCheck_Key(InArg))
	{
		AxisKey = *(FKey*)PyAxisKey->Value;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "argument type must be 'FKey'");
		return nullptr;
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	FVector AxisValue = InputComponent->GetVectorAxisValue(AxisKey);
	return NePyStructNew_Vector(AxisValue);
}

PyObject* NePyInputComponent_HasBindings(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'HasBindings'"))
	{
		return nullptr;
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (InputComponent->HasBindings())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* NePyInputComponent_BindAction(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindAction'"))
	{
		return nullptr;
	}

	char* ActionNameStr;
	int KeyEvent;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "siO:BindAction", &ActionNameStr, &KeyEvent, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent *)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}

	FName ActionName(UTF8_TO_TCHAR(ActionNameStr));
	FInputActionBinding ActionBinding(ActionName, (const EInputEvent)KeyEvent);
	ActionBinding.ActionDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->AddActionBinding(ActionBinding);

	Py_RETURN_NONE;
}

PyObject* NePyInputComponent_BindAxis(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindAxis'"))
	{
		return nullptr;
	}

	char* AxisNameStr;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "sO:BindAxis", &AxisNameStr, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}

	FName AxisName(UTF8_TO_TCHAR(AxisNameStr));
	FInputAxisBinding AxisBinding(AxisName);
	AxisBinding.AxisDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->AxisBindings.Add(AxisBinding);

	Py_RETURN_NONE;
}

PyObject* NePyInputComponent_BindAxisKey(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindAxisKey'"))
	{
		return nullptr;
	}

	char* AxisKeyNameStr;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "sO:BindAxisKey", &AxisKeyNameStr, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}

	FInputAxisKeyBinding AxisKeyBinding(FKey(UTF8_TO_TCHAR(AxisKeyNameStr)));
	AxisKeyBinding.AxisDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->AxisKeyBindings.Add(AxisKeyBinding);

	Py_RETURN_NONE;
}

PyObject* NePyInputComponent_BindVectorAxis(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindVectorAxis'"))
	{
		return nullptr;
	}

	char* AxisKeyNameStr;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "sO:BindVectorAxis", &AxisKeyNameStr, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}


	FInputVectorAxisBinding VectorAxisBinding(FKey(UTF8_TO_TCHAR(AxisKeyNameStr)));
	VectorAxisBinding.AxisDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->VectorAxisBindings.Add(VectorAxisBinding);

	Py_RETURN_NONE;
}

PyObject* NePyInputComponent_BindKey(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindKey'"))
	{
		return nullptr;
	}

	char* KeyNameStr;
	int KeyEvent;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "siO:BindKey", &KeyNameStr, &KeyEvent, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}

	FInputKeyBinding KeyBinding(FKey(UTF8_TO_TCHAR(KeyNameStr)), (const EInputEvent)KeyEvent);
	KeyBinding.KeyDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->KeyBindings.Add(KeyBinding);

	Py_RETURN_NONE;
}

PyObject* NePyInputComponent_BindTouch(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindTouch'"))
	{
		return nullptr;
	}

	int KeyEvent;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "iO:BindTouch", &KeyEvent, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}

	FInputTouchBinding TouchBinding((const EInputEvent)KeyEvent);
	TouchBinding.TouchDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->TouchBindings.Add(TouchBinding);

	Py_RETURN_NONE;
}

PyObject* NePyInputComponent_BindGesture(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindGesture'"))
	{
		return nullptr;
	}

	char* GestureKeyStr;
	PyObject* PyCallable;
	if (!PyArg_ParseTuple(InArgs, "sO:BindGesture", &GestureKeyStr, &PyCallable))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_Exception, "object is not a callable");
	}

	UInputComponent* InputComponent = (UInputComponent*)InSelf->Value;
	if (!InputComponent)
	{
		return PyErr_Format(PyExc_Exception, "InputComponent is nullptr");
	}

	FInputGestureBinding GestureBinding(FKey(UTF8_TO_TCHAR(GestureKeyStr)));
	GestureBinding.GestureDelegate.GetDelegateForManualSet().BindLambda(FNePyInputBindingFunctor(PyCallable));
	InputComponent->GestureBindings.Add(GestureBinding);

	Py_RETURN_NONE;
}


#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetAxisValue", NePyCFunctionCast(&NePyInputComponent_GetAxisValue), METH_VARARGS, "(self, AxisName: str) -> float"}, \
{"GetAxisKeyValue", NePyCFunctionCast(&NePyInputComponent_GetAxisKeyValue), METH_O, "(self, AxisKey: Key) -> float"}, \
{"GetVectorAxisValue", NePyCFunctionCast(&NePyInputComponent_GetVectorAxisValue), METH_O, "(self, AxisKey: Key) -> Vector"}, \
{"HasBindings", NePyCFunctionCast(&NePyInputComponent_HasBindings), METH_NOARGS, "(self) -> bool"}, \
{"BindAction", NePyCFunctionCast(&NePyInputComponent_BindAction), METH_VARARGS, "(self, ActionName: str, KeyEvent: EInputEvent, Callback: typing.Callable[[], None]) -> None"}, \
{"BindAxis", NePyCFunctionCast(&NePyInputComponent_BindAxis), METH_VARARGS, "(self, AxisName: str, Callback: typing.Callable[[float], None]) -> None"}, \
{"BindAxisKey", NePyCFunctionCast(&NePyInputComponent_BindAxisKey), METH_VARARGS, "(self, AxisKeyName: str, Callback: typing.Callable[[float], None]) -> None"}, \
{"BindVectorAxis", NePyCFunctionCast(&NePyInputComponent_BindVectorAxis), METH_VARARGS, "(self, AxisKeyName: str, Callback: typing.Callable[[Vector], None]) -> None"}, \
{"BindKey", NePyCFunctionCast(&NePyInputComponent_BindKey), METH_VARARGS, "(self, KeyName: str, KeyEvent: EInputEvent, Callback: typing.Callable[[], None]) -> None"}, \
{"BindTouch", NePyCFunctionCast(&NePyInputComponent_BindTouch), METH_VARARGS, "(self, KeyEvent: EInputEvent, Callback: typing.Callable[[ETouchIndex, Vector], None]) -> None"}, \
{"BindGesture", NePyCFunctionCast(&NePyInputComponent_BindGesture), METH_VARARGS, "(self, GestureName: str, Callback: typing.Callable[[float], None]) -> None"}, \
