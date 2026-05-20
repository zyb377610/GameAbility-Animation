#include "NePyDelegate.h"
#include "NePyBase.h"
#include "NePyGenUtil.h"

UNePyDelegate::UNePyDelegate()
	: Signature(nullptr)
	, OwnerObject(nullptr)
	, PyCallableObj(nullptr)
{
}


void UNePyDelegate::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UNePyDelegate::ReleasePythonResources()
{
	if (PyCallableObj)
	{
		FNePyScopedGIL GIL;
		Py_DECREF(PyCallableObj);
		PyCallableObj = nullptr;
	}
}

void UNePyDelegate::Initialize(UFunction* InSignature, PyObject* InPyCallableObj)
{
	check(!Signature);
	check(!PyCallableObj);
	check(InPyCallableObj);

	// do not acquire the gil here as we set the callable in python call themselves
	Signature = InSignature;
	PyCallableObj = InPyCallableObj;
	Py_INCREF(PyCallableObj);

	AddToRoot();
}

void UNePyDelegate::Initialize(UFunction* InSignature, const UObject* InOwnerObject, PyObject* InPyCallableObj)
{
	check(!Signature);
	check(!OwnerObject);
	check(!PyCallableObj);

	check(InOwnerObject);
	check(InPyCallableObj);

	// do not acquire the gil here as we set the callable in python call themselves
	Signature = InSignature;
	OwnerObject = InOwnerObject;
	PyCallableObj = InPyCallableObj;
	Py_INCREF(PyCallableObj);

	AddToRoot();
}

void UNePyDelegate::Finalize()
{
	if (!Signature)
	{
		return;
	}

	Signature = nullptr;
	OwnerObject = nullptr;
	RemoveFromRoot();
}

bool UNePyDelegate::UsesPyCallable(PyObject* InPyCallableObj) const
{
	PyObject* PyCallable = GetPyCallable();
	if (!PyCallable)
	{
		return false;
	}

	if (!PyCallable_Check(InPyCallableObj))
	{
		return false;
	}

	if (PyCallable == InPyCallableObj)
	{
		return true;
	}

	// 1. Callable with PyMethod_Type
	if (PyMethod_Check(InPyCallableObj) && PyMethod_Check(PyCallable))
	{
		PyMethodObject* OtherCallable = (PyMethodObject*)InPyCallableObj;
		PyMethodObject* SelfCallable = (PyMethodObject*)PyCallable;
		return OtherCallable->im_func == SelfCallable->im_func && OtherCallable->im_self == SelfCallable->im_self;
	}

	// 2. Callable with PyFunction_Type
	if (PyFunction_Check(InPyCallableObj) && PyFunction_Check(PyCallable))
	{
		PyFunctionObject* OtherCallable = (PyFunctionObject*)InPyCallableObj;
		PyFunctionObject* SelfCallable = (PyFunctionObject*)PyCallable;
		return OtherCallable->func_code == SelfCallable->func_code && OtherCallable->func_closure == SelfCallable->func_closure;
	}

	return false;
}

void UNePyDelegate::ProcessEvent(UFunction* InFunc, void* InBuffer)
{
	if (!Signature)
	{
		return;
	}

	PyObject* PyCallable = GetPyCallable();
	if (!PyCallable)
	{
		UE_LOG(LogNePython, Error, TEXT("UNePyDelegate::ProcessEvent: PyCallable is null for delegate with signature %s"), *Signature->GetName());
		return;
	}

	if (!PyCallable_Check(PyCallable))
	{
		UE_LOG(LogNePython, Error, TEXT("UNePyDelegate::ProcessEvent: PyCallable is not callable for delegate with signature %s"), *Signature->GetName());
		return;
	}

	FNePyScopedGIL GIL;

	FNePyObjectPtr PyArgs;

	// Simple case, no parameters or return value
	if (Signature->ChildProperties == nullptr)
	{
		if (OwnerObject)
		{
			PyArgs = NePyStealReference(PyTuple_New(1));
			PyObject* PyOwnerObject = NePyBase::ToPy(OwnerObject);
			PyTuple_SetItem(PyArgs, 0, PyOwnerObject);
		}
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyCallable, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
		return;
	}

	// Complex case, parameters or return value
	TArray<const FProperty*> InputParams;
	TArray<const FProperty*> OutputParams;
	NePyGenUtil::ExtractFunctionParams(Signature, InputParams, OutputParams);

	if (InputParams.Num() > 0)
	{
		int ArgOffset = OwnerObject ? 1 : 0;
		int32 TotalArgs = ArgOffset + InputParams.Num();
		PyArgs = NePyStealReference(PyTuple_New(TotalArgs));
		if (OwnerObject)
		{
			PyObject* PyOwnerObject = NePyBase::ToPy(OwnerObject);
			PyTuple_SetItem(PyArgs, 0, PyOwnerObject);
		}
		for (int32 ArgIdx = 0; ArgIdx < InputParams.Num(); ++ArgIdx)
		{
			const FProperty* Prop = InputParams[ArgIdx];
			PyObject* PyArg = NePyBase::TryConvertFPropertyToPyObjectInContainerNoDependency(Prop, InBuffer, 0, nullptr);
			if (!PyArg)
			{
				PyErr_Print();
				return;
			}
			PyTuple_SetItem(PyArgs, ArgOffset + ArgIdx, PyArg);
		}
	}

	FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyCallable, PyArgs));
	if (!PyRet)
	{
		PyErr_Print();
		return;
	}

	if (OutputParams.Num() == 0)
	{
		return;
	}

	if (OutputParams.Num() == 1)
	{
		if (!NePyBase::TryConvertPyObjectToFPropertyInContainer(PyRet, OutputParams[0], InBuffer, 0, nullptr))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyRet, OutputParams[0]);
			PyErr_Print();
		}
		return;
	}

	if (!PyTuple_Check(PyRet))
	{
		PyErr_Format(PyExc_TypeError, "%s needs %d return values, but python callable returned %d value(s).",
			TCHAR_TO_UTF8(*Signature->GetName()), OutputParams.Num(), 1);
		PyErr_Print();
		return;
	}

	int32 NumPyRets = (int32)PyTuple_Size(PyRet);
	if (NumPyRets != OutputParams.Num())
	{
		PyErr_Format(PyExc_TypeError, "%s needs %d return values, but python callable returned %d value(s).",
			TCHAR_TO_UTF8(*Signature->GetName()), OutputParams.Num(), NumPyRets);
		PyErr_Print();
		return;
	}

	for (int32 OutParamIndex = 0; OutParamIndex < OutputParams.Num(); ++OutParamIndex)
	{
		PyObject* PyValue = PyTuple_GetItem(PyRet, OutParamIndex);
		if (!NePyBase::TryConvertPyObjectToFPropertyInContainer(PyValue, OutputParams[OutParamIndex], InBuffer, 0, nullptr))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyValue, OutputParams[OutParamIndex]);
			PyErr_Print();
			return;
		}
	}
}

void UNePyDelegate::PyFakeCallable()
{
}

PyObject* UNePyDelegate::GetPyCallable() const
{
	return PyCallableObj;
}

FString UNePyDelegate::GetPyCallableStr() const
{
	PyObject* PyCallable = GetPyCallable();
	if (!PyCallable)
	{
		return FString("<null>");
	}

	FNePyScopedGIL GIL;
	PyObject* PyStr = PyObject_Repr(PyCallable);
	const char* Str = NePyString_AsString(PyStr);
	return FString(Str);
}

FName UNePyDelegate::FakeFuncName = FName("PyFakeCallable");

void UNePyWeakDelegate::Initialize(UFunction* InSignature, PyObject* InPyCallableObj)
{
	check(!Signature);
	check(!PyCallableObj);

	// do not acquire the gil here as we set the callable in python call themselves
	Signature = InSignature;
	PyCallableObj = PyWeakref_NewRef(InPyCallableObj, nullptr);
}

PyObject* UNePyWeakDelegate::GetPyCallable() const
{
	PyObject* PyCallable = PyWeakref_GetObject(PyCallableObj);
	if (PyCallable == Py_None)
	{
		return nullptr;
	}
	return PyCallable;
}
