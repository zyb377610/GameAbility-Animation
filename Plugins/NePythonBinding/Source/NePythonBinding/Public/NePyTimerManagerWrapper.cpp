#include "NePyTimerManagerWrapper.h"
#include "HAL/UnrealMemory.h"
#include "NePyGIL.h"
#include "NePyObjectHolder.h"
#include "NePyTimerHandle.h"

// 用于绑定Python回调的帮助类
// 使用RAII来管理Python回调引用计数
class FNePyTimerFunctor : public FNePyObjectHolder
{
public:
	using FNePyObjectHolder::FNePyObjectHolder;
	FNePyTimerFunctor(const FNePyTimerFunctor& Other) : FNePyObjectHolder(Other) {}
	FNePyTimerFunctor(FNePyTimerFunctor&& Other) : FNePyObjectHolder(Other) {}

	void operator()()
	{
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyResult = NePyStealReference(PyObject_CallObject(this->Value, nullptr));
		if (!PyResult)
		{
			PyErr_Print();
		}
	}

private:
	FNePyTimerFunctor() = delete;
	FNePyTimerFunctor& operator=(const FNePyTimerFunctor& InOther) = delete;
	FNePyTimerFunctor& operator=(FNePyTimerFunctor&& InOther) = delete;
};

static PyTypeObject FNePyTimerManagerWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"TimerManagerWrapper", /* tp_name */
	sizeof(FNePyTimerManagerWrapper), /* tp_basicsize */
};

static PyTypeObject FNePySharedTimerManagerWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"TimerManagerWrapper", /* tp_name */
	sizeof(FNePySharedTimerManagerWrapper), /* tp_basicsize */
};

void FNePyTimerManagerWrapper::InitPyType()
{
	static PyMethodDef PyMethods[] = {
		{ "IsValid", NePyCFunctionCast(&FNePyTimerManagerWrapper::IsValid), METH_NOARGS, "(self) -> bool" },
		{ "SetTimer", NePyCFunctionCast(&FNePyTimerManagerWrapper::SetTimer), METH_VARARGS, "(self, InTimerMethod: typing.Callable, InRate: float, InbLoop: bool = ..., InFirstDelay: float = ...) -> TimerHandle" },
		{ "SetTimerForNextTick", NePyCFunctionCast(&FNePyTimerManagerWrapper::SetTimerForNextTick), METH_O, "(self, InTimerMethod: typing.Callable) -> TimerHandle" },
		{ "ClearTimer", NePyCFunctionCast(&FNePyTimerManagerWrapper::ClearTimer), METH_O, "(self, InHandle: TimerHandle) -> None" },
		{ "PauseTimer", NePyCFunctionCast(&FNePyTimerManagerWrapper::PauseTimer), METH_O, "(self, InHandle: TimerHandle) -> None" },
		{ "UnPauseTimer", NePyCFunctionCast(&FNePyTimerManagerWrapper::UnPauseTimer), METH_O, "(self, InHandle: TimerHandle) -> None" },
		{ "GetTimerRate", NePyCFunctionCast(&FNePyTimerManagerWrapper::GetTimerRate), METH_O, "(self, InHandle: TimerHandle) -> float" },
		{ "IsTimerActive", NePyCFunctionCast(&FNePyTimerManagerWrapper::IsTimerActive), METH_O, "(self, InHandle: TimerHandle) -> bool" },
		{ "IsTimerPaused", NePyCFunctionCast(&FNePyTimerManagerWrapper::IsTimerPaused), METH_O, "(self, InHandle: TimerHandle) -> bool" },
		{ "IsTimerPending", NePyCFunctionCast(&FNePyTimerManagerWrapper::IsTimerPending), METH_O, "(self, InHandle: TimerHandle) -> bool" },
		{ "TimerExists", NePyCFunctionCast(&FNePyTimerManagerWrapper::TimerExists), METH_O, "(self, InHandle: TimerHandle) -> bool" },
		{ "GetTimerElapsed", NePyCFunctionCast(&FNePyTimerManagerWrapper::GetTimerElapsed), METH_O, "(self, InHandle: TimerHandle) -> float" },
		{ "GetTimerRemaining", NePyCFunctionCast(&FNePyTimerManagerWrapper::GetTimerRemaining), METH_O, "(self, InHandle: TimerHandle) -> float" },
		{ nullptr, nullptr, 0, nullptr }
	};

	{
		PyTypeObject* PyType = &FNePyTimerManagerWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyTimerManagerWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyTimerManagerWrapper::Init;
		PyType->tp_methods = PyMethods;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePySharedTimerManagerWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyTimerManagerWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyTimerManagerWrapper::Init;
		PyType->tp_methods = PyMethods;
		PyType_Ready(PyType);
	}
}

FNePyTimerManagerWrapper* FNePyTimerManagerWrapper::New(UObject* InObject, FTimerManager* InTimerManager)
{
	auto Constructor = [InTimerManager]() -> FNePyPropObject* {
		FNePyTimerManagerWrapper* RetValue = PyObject_New(FNePyTimerManagerWrapper, &FNePyTimerManagerWrapperType);
		RetValue->Value = InTimerManager;
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InTimerManager, Constructor);
	return (FNePyTimerManagerWrapper*)RetValue;
}

void FNePyTimerManagerWrapper::Dealloc(FNePyTimerManagerWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此Value必为空
	check(!InSelf->Value);
	InSelf->ob_type->tp_free(InSelf);
}

int FNePyTimerManagerWrapper::Init(FNePyTimerManagerWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init TimerManagerWrapper directly!");
	return -1;
}

PyObject* FNePyTimerManagerWrapper::IsValid(FNePyTimerManagerWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyTimerManagerWrapper::SetTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyTimerMethod;
	float Rate;
	bool bLoop = false;
	float FirstDelay = -1.f;
	if (!PyArg_ParseTuple(InArgs, "Of|bf:SetTimer", &PyTimerMethod, &Rate, &bLoop, &FirstDelay))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyTimerMethod))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InTimerMethod' must have type 'callable'");
		return nullptr;
	}

	if (Rate <= 0)
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'InRate' must greater than zero");
		return nullptr;
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda(FNePyTimerFunctor(PyTimerMethod));

	FTimerHandle TimerHandle;
	InSelf->GetValue()->SetTimer(TimerHandle, TimerDelegate, Rate, bLoop, FirstDelay);

	return FNePyTimerManagerWrapper::ToPyHandle(TimerHandle);
}

PyObject* FNePyTimerManagerWrapper::SetTimerForNextTick(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (!PyCallable_Check(InArg))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InTimerMethod' must have type 'callable'");
		return nullptr;
	}

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindLambda(FNePyTimerFunctor(InArg));

	FTimerHandle TimerHandle;
	InSelf->GetValue()->SetTimerForNextTick(TimerDelegate);

	return FNePyTimerManagerWrapper::ToPyHandle(TimerHandle);
}

PyObject* FNePyTimerManagerWrapper::ClearTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	InSelf->GetValue()->ClearTimer(Handle);
	Py_RETURN_NONE;
}

PyObject* FNePyTimerManagerWrapper::PauseTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	InSelf->GetValue()->PauseTimer(Handle);
	Py_RETURN_NONE;
}

PyObject* FNePyTimerManagerWrapper::UnPauseTimer(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	InSelf->GetValue()->UnPauseTimer(Handle);
	Py_RETURN_NONE;
}

PyObject* FNePyTimerManagerWrapper::GetTimerRate(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	float RetVal = InSelf->GetValue()->GetTimerRate(Handle);
	return PyFloat_FromDouble(RetVal);
}

PyObject* FNePyTimerManagerWrapper::IsTimerActive(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	bool bRetVal = InSelf->GetValue()->IsTimerActive(Handle);
	return PyBool_FromLong(bRetVal);
}

PyObject* FNePyTimerManagerWrapper::IsTimerPaused(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	bool bRetVal = InSelf->GetValue()->IsTimerPaused(Handle);
	return PyBool_FromLong(bRetVal);
}

PyObject* FNePyTimerManagerWrapper::IsTimerPending(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	bool bRetVal = InSelf->GetValue()->IsTimerPending(Handle);
	return PyBool_FromLong(bRetVal);
}

PyObject* FNePyTimerManagerWrapper::TimerExists(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	bool bRetVal = InSelf->GetValue()->TimerExists(Handle);
	return PyBool_FromLong(bRetVal);
}

PyObject* FNePyTimerManagerWrapper::GetTimerElapsed(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	float RetVal = InSelf->GetValue()->GetTimerElapsed(Handle);
	return PyFloat_FromDouble(RetVal);
}

PyObject* FNePyTimerManagerWrapper::GetTimerRemaining(FNePyTimerManagerWrapper* InSelf, PyObject* InArg)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FTimerHandle Handle;
	if (!FNePyTimerManagerWrapper::ToCppHandle(InArg, Handle))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TimerHandle'");
		return nullptr;
	}

	float RetVal = InSelf->GetValue()->GetTimerRemaining(Handle);
	return PyFloat_FromDouble(RetVal);
}

bool FNePyTimerManagerWrapper::ToCppHandle(PyObject* InPyObj, FTimerHandle& OutHandle)
{
	return NePyBase::ToCpp(InPyObj, OutHandle);
}

PyObject* FNePyTimerManagerWrapper::ToPyHandle(FTimerHandle InHandle)
{
	return NePyBase::ToPy(InHandle);
}

FNePySharedTimerManagerWrapper* FNePySharedTimerManagerWrapper::New(const TSharedRef<FTimerManager>& InTimerManager)
{
	FNePySharedTimerManagerWrapper** RetValuePtr = FNePySharedTimerManagerWrapper::PythonWrappers.Find(InTimerManager);
	FNePySharedTimerManagerWrapper* RetValue;
	if (RetValuePtr)
	{
		RetValue = *RetValuePtr;
	}
	else
	{
		RetValue = PyObject_New(FNePySharedTimerManagerWrapper, &FNePySharedTimerManagerWrapperType);
		RetValue->Value = &InTimerManager.Get();
		FNePySharedTimerManagerWrapper::PythonWrappers.Add(InTimerManager, RetValue);
	}

	Py_INCREF(RetValue);
	return RetValue;
}

TMap<TSharedRef<FTimerManager>, FNePySharedTimerManagerWrapper*> FNePySharedTimerManagerWrapper::PythonWrappers;
