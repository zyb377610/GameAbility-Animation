#include "NePyDynamicDelegateWrapper.h"
#include "NePyBase.h"
#include "NePyGenUtil.h"
#include "UObject/StructOnScope.h"

static PyTypeObject FNePyDynamicDelegateWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"DynamicDelegateWrapper", /* tp_name */
	sizeof(FNePyDynamicDelegateWrapper), /* tp_basicsize */
};

void FNePyDynamicDelegateWrapper::InitPyType()
{
	static PyMethodDef PyMethods[] = {
		{ "IsValid", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::IsValid), METH_NOARGS, "() -> bool" },
		{ "IsBound", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyIsBound), METH_NOARGS, "() -> bool" },
		{ "IsBoundTo", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyIsBoundTo), METH_O, "(typing.Callable) -> bool" },
		{ "Bind", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyBind), METH_O, "(typing.Callable) -> None" },
		{ "Unbind", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyUnbind), METH_NOARGS, "() -> None" },
		{ "Clear", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyUnbind), METH_NOARGS, "() -> None" },
		{ "GetPythonCallback", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::GetPythonCallback), METH_NOARGS, "() -> typing.Callable|None" },
		{ "Execute", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyExecute), METH_VARARGS, "(...) -> typing.Any" },
		{ "ExecuteIfBound", NePyCFunctionCast(&FNePyDynamicDelegateWrapper::PyExecuteIfBound), METH_VARARGS, "(...) -> typing.Any"},
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &FNePyDynamicDelegateWrapperType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_dealloc = (destructor)&FNePyDynamicDelegateWrapper::Dealloc;
	PyType->tp_init = (initproc)&FNePyDynamicDelegateWrapper::Init;
	PyType->tp_repr = (reprfunc)&FNePyDynamicDelegateWrapper::Repr;
	PyType->tp_str = (reprfunc)&FNePyDynamicDelegateWrapper::Repr;
	PyType->tp_methods = PyMethods;
	PyType_Ready(PyType);
}

FNePyDynamicDelegateWrapper* FNePyDynamicDelegateWrapper::New(UObject* InObject, void* InMemberPtr, const char* PropName)
{
	UClass* Class = InObject->GetClass();

	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(Class, InObject, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' object has no property '%s'", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InObject));

	const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Prop);
	if (!DelegateProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not delegate property", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	auto Constructor = [InMemberPtr, DelegateProp]() -> FNePyPropObject* {
		FNePyDynamicDelegateWrapper* RetValue = PyObject_New(FNePyDynamicDelegateWrapper, &FNePyDynamicDelegateWrapperType);
		RetValue->Value = InMemberPtr;
		RetValue->Prop = DelegateProp;
		RetValue->Delegate = nullptr;
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, DelegateProp->SignatureFunction, Constructor);
	return (FNePyDynamicDelegateWrapper*)RetValue;
}

FNePyDynamicDelegateWrapper* FNePyDynamicDelegateWrapper::New(UObject* InObject, const FDelegateProperty* InProp)
{
	void* MemberPtr = InProp->ContainerPtrToValuePtr<void>(InObject);

	auto Constructor = [MemberPtr, InProp]() -> FNePyPropObject* {
		FNePyDynamicDelegateWrapper* RetValue = PyObject_New(FNePyDynamicDelegateWrapper, &FNePyDynamicDelegateWrapperType);
		RetValue->Value = MemberPtr;
		RetValue->Prop = InProp;
		RetValue->Delegate = nullptr;
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InProp->SignatureFunction, Constructor);
	return (FNePyDynamicDelegateWrapper*)RetValue;
}

FNePyDynamicDelegateWrapper* FNePyDynamicDelegateWrapper::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePyDynamicDelegateWrapperType)
		{
			return (FNePyDynamicDelegateWrapper*)InPyObj;
		}
	}
	return nullptr;
}

bool FNePyDynamicDelegateWrapper::IsBound()
{
	FScriptDelegate ScriptDelegate = Prop->GetPropertyValue(Value);
	return ScriptDelegate.IsBound();
}

bool FNePyDynamicDelegateWrapper::IsBoundTo(PyObject* InPyCallable)
{
	return Delegate && Delegate->UsesPyCallable(InPyCallable);
}

void FNePyDynamicDelegateWrapper::Bind(PyObject* InPyCallable)
{
	// 单播事件，先检查当前是否已经绑定过回调函数了
	FScriptDelegate ScriptDelegate = Prop->GetPropertyValue(Value);
	if (ScriptDelegate.IsBound())
	{
		FString DelegateStr = Delegate? Delegate->GetPyCallableStr() : ScriptDelegate.ToString<UObject>();
		UE_LOG(LogNePython, Error, TEXT("DynamicDelegate '%s' at %p already bound with '%s'"),
			*Prop->GetName(), Value, *DelegateStr);
		return;
	}

	check(!Delegate);
	Delegate = NewObject<UNePyDelegate>();
	Delegate->Initialize(Prop->SignatureFunction, InPyCallable);

	ScriptDelegate.BindUFunction(Delegate, UNePyDelegate::FakeFuncName);
	Prop->SetPropertyValue(Value, ScriptDelegate);
}

void FNePyDynamicDelegateWrapper::Unbind()
{
	if (Delegate)
	{
		Delegate->Finalize();
		Delegate = nullptr;
	}

	FScriptDelegate ScriptDelegate;
	Prop->SetPropertyValue(Value, ScriptDelegate);
}

void FNePyDynamicDelegateWrapper::Dealloc(FNePyDynamicDelegateWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此DelegatePtr必为空
	check(!InSelf->Value);

	if (InSelf->Delegate)
	{
		InSelf->Delegate->Finalize();
		InSelf->Delegate = nullptr;
	}

	InSelf->ob_type->tp_free(InSelf);
}

int FNePyDynamicDelegateWrapper::Init(FNePyDynamicDelegateWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init DynamicDelegate directly!");
	return -1;
}

PyObject* FNePyDynamicDelegateWrapper::Repr(FNePyDynamicDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return PyUnicode_FromFormat("<DynamicDelegate '%s' at %p>", TCHAR_TO_UTF8(*InSelf->Prop->GetName()), InSelf->Value);
}

PyObject* FNePyDynamicDelegateWrapper::IsValid(FNePyDynamicDelegateWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyDynamicDelegateWrapper::PyIsBound(FNePyDynamicDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (InSelf->IsBound())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyDynamicDelegateWrapper::PyIsBoundTo(FNePyDynamicDelegateWrapper* InSelf, PyObject* InPyCallable)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (InSelf->IsBoundTo(InPyCallable))
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyDynamicDelegateWrapper::PyBind(FNePyDynamicDelegateWrapper* InSelf, PyObject* InPyCallable)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	InSelf->Bind(InPyCallable);
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicDelegateWrapper::PyUnbind(FNePyDynamicDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	InSelf->Unbind();
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicDelegateWrapper::GetPythonCallback(FNePyDynamicDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (!InSelf->Delegate)
	{
		Py_RETURN_NONE;
	}

	PyObject* PyCallback = InSelf->Delegate->GetPyCallable();
	if (!PyCallback)
	{
		PyCallback = Py_None;
	}

	Py_INCREF(PyCallback);
	return PyCallback;
}

PyObject* FNePyDynamicDelegateWrapper::PyExecute(FNePyDynamicDelegateWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (!InSelf->IsBound())
	{
		PyErr_SetString(PyExc_Exception, "Cannot call an unbound delegate!");
		return nullptr;
	}

	return InSelf->ExecuteInternal(InArgs);
}

PyObject* FNePyDynamicDelegateWrapper::PyExecuteIfBound(FNePyDynamicDelegateWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (!InSelf->IsBound())
	{
		Py_RETURN_NONE;
	}

	return InSelf->ExecuteInternal(InArgs);
}

// 参考：PyWrapperDelegate.cpp FPyWrapperDelegateImpl::CallDelegate
PyObject* NePyDynamicDelegateExecute(PyObject* InArgs, const FScriptDelegate& ScriptDelegate, const UFunction* SignatureFunction)
{
	if (!PyTuple_Check(InArgs))
	{
		PyErr_BadInternalCall();
		return nullptr;
	}

	if (SignatureFunction->ChildProperties == nullptr)
	{
		// Simple case, no parameters or return value
		const Py_ssize_t NumPyArgs = PyTuple_GET_SIZE(InArgs);
		if (NumPyArgs != 0)
		{
			PyErr_Format(PyExc_TypeError, "%s takes no arguments (%ld given)",
				TCHAR_TO_UTF8(*SignatureFunction->GetName()), NumPyArgs);
			return nullptr;
		}

		ScriptDelegate.ProcessDelegate<UObject>(nullptr);
		Py_RETURN_NONE;
	}

	// Complex case, parameters or return value
	TArray<const FProperty*> InputParams;
	TArray<const FProperty*> OutputParams;
	NePyGenUtil::ExtractFunctionParams(SignatureFunction, InputParams, OutputParams);

	const Py_ssize_t NumPyArgs = PyTuple_GET_SIZE(InArgs);
	if (NumPyArgs != InputParams.Num())
	{
		PyErr_Format(PyExc_TypeError, "%s takes %d arguments, got %ld",
			TCHAR_TO_UTF8(*SignatureFunction->GetName()), InputParams.Num(), NumPyArgs);
		return nullptr;
	}

	FStructOnScope DelegateParams(SignatureFunction);
	for (int32 ParamIndex = 0; ParamIndex < NumPyArgs; ++ParamIndex)
	{
		PyObject* PyArg = PyTuple_GetItem(InArgs, ParamIndex);
		if (!NePyBase::TryConvertPyObjectToFPropertyInContainer(PyArg, InputParams[ParamIndex], DelegateParams.GetStructMemory(), 0, nullptr))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyArg, InputParams[ParamIndex]);
			return nullptr;
		}
	}

	ScriptDelegate.ProcessDelegate<UObject>(DelegateParams.GetStructMemory());
	return NePyGenUtil::PackReturnValues(DelegateParams.GetStructMemory(), OutputParams, nullptr);	
}

PyObject* FNePyDynamicDelegateWrapper::ExecuteInternal(PyObject* InArgs)
{
	FScriptDelegate ScriptDelegate = Prop->GetPropertyValue(Value);
	return NePyDynamicDelegateExecute(InArgs, ScriptDelegate, Prop->SignatureFunction);
}

static PyTypeObject FNePyDynamicDelegateParamType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"DynamicDelegateParam", /* tp_name */
	sizeof(FNePyDynamicDelegateParam), /* tp_basicsize */
};

void FNePyDynamicDelegateParam::InitPyType()
{
	PyTypeObject* PyType = &FNePyDynamicDelegateParamType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_dealloc = (destructor)&FNePyDynamicDelegateParam::Dealloc;
	PyType->tp_init = (initproc)&FNePyDynamicDelegateParam::Init;
	PyType_Ready(PyType);
}

FNePyDynamicDelegateParam* FNePyDynamicDelegateParam::New(UObject* InObject, const FDelegateProperty* InProp)
{
	auto Constructor = [InProp]() -> FNePyPropObject* {
		FNePyDynamicDelegateParam* RetValue = PyObject_New(FNePyDynamicDelegateParam, &FNePyDynamicDelegateParamType);
		RetValue->Value = (void*)InProp;
		new (&RetValue->Delegates) TArray<UNePyDelegate*>();
		return RetValue;
	};

	// NOTE: InProp源自于InObject上的UFunction，其指针位置在InObject的生命周期内不变且独一无二，因此可作为PyPropObjs的Key。
	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InProp->SignatureFunction, Constructor);
	return (FNePyDynamicDelegateParam*)RetValue;
}

UNePyDelegate* FNePyDynamicDelegateParam::FindOrAddNePyDelegate(UObject* InObject, const FName& InFuncName, const FName& InParamName, PyObject* InPyCallable)
{
	const FDelegateProperty* Prop = FNePyDynamicDelegateParam::FindDelegateProperty(InObject, InFuncName, InParamName);
	if (!Prop)
	{
		return nullptr;
	}

	FNePyDynamicDelegateParam* DelegateParam = FNePyDynamicDelegateParam::New(InObject, Prop);
	Py_DECREF(DelegateParam);
	UNePyDelegate* Delegate = DelegateParam->FindOrAddDelegate(InPyCallable);
	return Delegate;
}

UNePyDelegate* FNePyDynamicDelegateParam::FindOrAddDelegate(PyObject* InPyCallable)
{
	for (UNePyDelegate* Delegate : Delegates)
	{
		if (Delegate->UsesPyCallable(InPyCallable))
		{
			return Delegate;
		}
	}

	const FDelegateProperty* Prop = GetParamProp();
	UNePyDelegate* NewDelegate = NewObject<UNePyDelegate>();
	NewDelegate->Initialize(Prop->SignatureFunction, InPyCallable);
	Delegates.Add(NewDelegate);
	return NewDelegate;
}

const FDelegateProperty* FNePyDynamicDelegateParam::FindDelegateProperty(UObject* InObject, const FName& InFuncName, const FName& InParamName)
{
	UFunction* FuncValue = InObject->FindFunction(InFuncName);
	if (!FuncValue)
	{
		return nullptr;
	}

	for (TFieldIterator<FProperty> ParamIt(FuncValue); ParamIt && ParamIt->HasAnyPropertyFlags(CPF_Parm); ++ParamIt)
	{
		if (auto DelegateProperty = CastField<FDelegateProperty>(*ParamIt))
		{
			if (DelegateProperty->GetFName() == InParamName)
			{
				return DelegateProperty;
			}
		}
	}

	return nullptr;
}

void FNePyDynamicDelegateParam::Dealloc(FNePyDynamicDelegateParam* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此DelegatePtr必为空
	check(!InSelf->Value);

	for (UNePyDelegate* Delegate : InSelf->Delegates)
	{
		Delegate->Finalize();
	}
	InSelf->Delegates.~TArray();

	InSelf->ob_type->tp_free(InSelf);
}

int FNePyDynamicDelegateParam::Init(FNePyDynamicDelegateParam* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init DynamicDelegateParam directly!");
	return -1;
}

static PyTypeObject FNePyDynamicDelegateArgType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"DynamicDelegateArg", /* tp_name */
	sizeof(FNePyDynamicDelegateArg), /* tp_basicsize */
};

void FNePyDynamicDelegateArg::InitPyType()
{
	static PyMethodDef PyMethods[] = {
		{ "IsBound", NePyCFunctionCast(&FNePyDynamicDelegateArg::PyIsBound), METH_NOARGS, "() -> bool" },
		{ "Execute", NePyCFunctionCast(&FNePyDynamicDelegateArg::PyExecute), METH_VARARGS, "(...) -> typing.Any" },
		{ "ExecuteIfBound", NePyCFunctionCast(&FNePyDynamicDelegateArg::PyExecuteIfBound), METH_VARARGS, "(...) -> typing.Any"},
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &FNePyDynamicDelegateArgType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_dealloc = (destructor)&FNePyDynamicDelegateArg::Dealloc;
	PyType->tp_init = (initproc)&FNePyDynamicDelegateArg::Init;
	PyType->tp_repr = (reprfunc)&FNePyDynamicDelegateArg::Repr;
	PyType->tp_str = (reprfunc)&FNePyDynamicDelegateArg::Repr;
	PyType->tp_methods = PyMethods;
	PyType_Ready(PyType);
}

FNePyDynamicDelegateArg* FNePyDynamicDelegateArg::New(const FScriptDelegate& InDelegate, const FDelegateProperty* InDelegateProp)
{
	FNePyDynamicDelegateArg* RetValue = PyObject_New(FNePyDynamicDelegateArg, &FNePyDynamicDelegateArgType);
	new (&RetValue->Delegate) FScriptDelegate(InDelegate);
	return RetValue;
}

void FNePyDynamicDelegateArg::Dealloc(FNePyDynamicDelegateArg* InSelf)
{
	InSelf->Delegate.~FScriptDelegate();
	InSelf->ob_type->tp_free(InSelf);
}

int FNePyDynamicDelegateArg::Init(FNePyDynamicDelegateArg* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init DynamicDelegateArg directly!");
	return -1;
}

PyObject* FNePyDynamicDelegateArg::Repr(FNePyDynamicDelegateArg* InSelf)
{
	const FScriptDelegate& Delegate = InSelf->Delegate;
	if (Delegate.IsBound())
	{
		return PyUnicode_FromFormat("<DynamicDelegate '%s.%s' at %p>",
			TCHAR_TO_UTF8(*Delegate.GetUObject()->GetName()), TCHAR_TO_UTF8(*Delegate.GetFunctionName().ToString()), InSelf);
	}
	return PyUnicode_FromFormat("<DynamicDelegate '[unbound]' at %p>", InSelf);
}

PyObject* FNePyDynamicDelegateArg::PyIsBound(FNePyDynamicDelegateArg* InSelf)
{
	if (InSelf->Delegate.IsBound())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyDynamicDelegateArg::PyExecute(FNePyDynamicDelegateArg* InSelf, PyObject* InArgs)
{
	if (!InSelf->Delegate.IsBound())
	{
		PyErr_SetString(PyExc_Exception, "Cannot call an unbound delegate!");
		return nullptr;
	}

	return InSelf->ExecuteInternal(InArgs);
}

PyObject* FNePyDynamicDelegateArg::PyExecuteIfBound(FNePyDynamicDelegateArg* InSelf, PyObject* InArgs)
{
	if (!InSelf->Delegate.IsBound())
	{
		Py_RETURN_NONE;
	}
	
	return InSelf->ExecuteInternal(InArgs);
}

PyObject* FNePyDynamicDelegateArg::ExecuteInternal(PyObject* InArgs)
{
	UObject* OwnerObject = Delegate.GetUObject();
	check(::IsValid(OwnerObject));
	UFunction* DelegateFunction = OwnerObject->FindFunctionChecked(Delegate.GetFunctionName());
	return NePyDynamicDelegateExecute(InArgs, Delegate, DelegateFunction);
}