#include "NePyDynamicMulticastDelegateWrapper.h"
#include "NePyBase.h"
#include "NePyGenUtil.h"
#include "UObject/StructOnScope.h"

static PyTypeObject FNePyDynamicMulticastDelegateWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"DynamicMulticastDelegateWrapper", /* tp_name */
	sizeof(FNePyDynamicMulticastDelegateWrapper), /* tp_basicsize */
};

void FNePyDynamicMulticastDelegateWrapper::InitPyType()
{
	static PyMethodDef PyMethods[] = {
		{ "IsValid", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::IsValid), METH_NOARGS, "() -> bool" },
		{ "IsBound", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyIsBound), METH_NOARGS, "() -> bool" },
		{ "Contains", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyContains), METH_O, "(typing.Callable) -> bool" },
		{ "Add", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyAdd), METH_O, "(typing.Callable) -> None" },
		{ "AddUnique", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyAddUnique), METH_O, "(typing.Callable) -> None" },
		{ "AddDynamic", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyAddDynamic), METH_VARARGS, "(target: Object, callback: typing.Callable) -> None" },
		{ "AddDynamicUnique", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyAddDynamicUnique), METH_VARARGS, "(target: Object, callback: typing.Callable) -> None" },
		{ "RemoveDynamic", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyRemoveDynamic), METH_VARARGS, "(target: Object, callback: typing.Callable) -> None" },
		{ "Remove", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyRemove), METH_O, "(typing.Callable) -> None" },
		{ "Clear", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyClear), METH_NOARGS, "() -> None" },
		{ "GetPythonCallbacks", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::GetPythonCallbacks), METH_NOARGS, "() -> list[typing.Callable]" },
		{ "Broadcast", NePyCFunctionCast(&FNePyDynamicMulticastDelegateWrapper::PyBroadcast), METH_VARARGS, "(...) -> typing.Any" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &FNePyDynamicMulticastDelegateWrapperType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = PyType_GenericNew;
	PyType->tp_dealloc = (destructor)&FNePyDynamicMulticastDelegateWrapper::Dealloc;
	PyType->tp_init = (initproc)&FNePyDynamicMulticastDelegateWrapper::Init;
	PyType->tp_repr = (reprfunc)&FNePyDynamicMulticastDelegateWrapper::Repr;
	PyType->tp_str = (reprfunc)&FNePyDynamicMulticastDelegateWrapper::Repr;
	PyType->tp_methods = PyMethods;
	PyType_Ready(PyType);
}

FNePyDynamicMulticastDelegateWrapper* FNePyDynamicMulticastDelegateWrapper::New(UObject* InObject, void* InMemberPtr, const char* PropName)
{
	UClass* Class = InObject->GetClass();

	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(Class, InObject, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' object has no property '%s'", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InObject));

	const FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(Prop);
	if (!DelegateProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not multicast delegate property", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	auto Constructor = [InMemberPtr, DelegateProp]() -> FNePyPropObject* {
		FNePyDynamicMulticastDelegateWrapper* RetValue = PyObject_New(FNePyDynamicMulticastDelegateWrapper, &FNePyDynamicMulticastDelegateWrapperType);
		RetValue->Value = InMemberPtr;
		RetValue->Prop = DelegateProp;
		new (&RetValue->Delegates) decltype(RetValue->Delegates)();
		new (&RetValue->UObjectDelegates) decltype(RetValue->UObjectDelegates)();
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InMemberPtr, Constructor);
	return (FNePyDynamicMulticastDelegateWrapper*)RetValue;
}

FNePyDynamicMulticastDelegateWrapper* FNePyDynamicMulticastDelegateWrapper::New(UObject* InObject, const FMulticastDelegateProperty* InProp)
{
	void* MemberPtr = InProp->ContainerPtrToValuePtr<void>(InObject);

	auto Constructor = [MemberPtr, InProp]() -> FNePyPropObject* {
		FNePyDynamicMulticastDelegateWrapper* RetValue = PyObject_New(FNePyDynamicMulticastDelegateWrapper, &FNePyDynamicMulticastDelegateWrapperType);
		RetValue->Value = MemberPtr;
		RetValue->Prop = InProp;
		new (&RetValue->Delegates) decltype(RetValue->Delegates)();
		new (&RetValue->UObjectDelegates) decltype(RetValue->UObjectDelegates)();
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, MemberPtr, Constructor);
	return (FNePyDynamicMulticastDelegateWrapper*)RetValue;
}

FNePyDynamicMulticastDelegateWrapper* FNePyDynamicMulticastDelegateWrapper::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePyDynamicMulticastDelegateWrapperType)
		{
			return (FNePyDynamicMulticastDelegateWrapper*)InPyObj;
		}
	}
	return nullptr;
}

bool FNePyDynamicMulticastDelegateWrapper::IsBound()
{
	const FMulticastScriptDelegate* ScriptDelegate = Prop->GetMulticastDelegate(Value);
	return ScriptDelegate && ScriptDelegate->IsBound();
}

bool FNePyDynamicMulticastDelegateWrapper::Contains(PyObject* InPyCallable)
{
	int32 DelegateIndex = FindDelegateIndex(InPyCallable);
	return DelegateIndex != INDEX_NONE;
}

void FNePyDynamicMulticastDelegateWrapper::Add(PyObject* InPyCallable)
{
	UNePyDelegate* NewDelegate = NewObject<UNePyDelegate>();
	NewDelegate->Initialize(Prop->SignatureFunction, InPyCallable);
	Delegates.Add(NewDelegate);

	FScriptDelegate ScriptDelegate;
	ScriptDelegate.BindUFunction(NewDelegate, UNePyDelegate::FakeFuncName);
	Prop->AddDelegate(ScriptDelegate, nullptr, Value);
}

void FNePyDynamicMulticastDelegateWrapper::AddUnique(PyObject* InPyCallable)
{
	int32 DelegateIndex = FindDelegateIndex(InPyCallable);
	if (DelegateIndex == INDEX_NONE)
	{
		UNePyDelegate* NewDelegate = NewObject<UNePyDelegate>();
		NewDelegate->Initialize(Prop->SignatureFunction, InPyCallable);
		Delegates.Add(NewDelegate);

		FScriptDelegate ScriptDelegate;
		ScriptDelegate.BindUFunction(NewDelegate, UNePyDelegate::FakeFuncName);
		Prop->AddDelegate(ScriptDelegate, nullptr, Value);
	}
}

void FNePyDynamicMulticastDelegateWrapper::Remove(PyObject* InPyCallable)
{
	int32 DelegateIndex = FindDelegateIndex(InPyCallable);
	if (DelegateIndex != INDEX_NONE)
	{
		UNePyDelegate* FoundDelegate = Delegates[DelegateIndex];
		Delegates.RemoveAt(DelegateIndex);

		FScriptDelegate ScriptDelegate;
		ScriptDelegate.BindUFunction(FoundDelegate, UNePyDelegate::FakeFuncName);
		Prop->RemoveDelegate(ScriptDelegate, nullptr, Value);
		FoundDelegate->Finalize();
	}
}

void FNePyDynamicMulticastDelegateWrapper::AddUObjectDynamic(const UObject* InTarget, PyObject* InPyCallable)
{
	if (!UObjectDelegates.Contains(InTarget))
	{
		UObjectDelegates.Add(InTarget, {});
	}
	UNePyDelegate* NewDelegate = NewObject<UNePyDelegate>();
	NewDelegate->Initialize(Prop->SignatureFunction, InTarget, InPyCallable);
	Delegates.Add(NewDelegate);
	UObjectDelegates[InTarget].Add(NewDelegate);

	FScriptDelegate ScriptDelegate;
	ScriptDelegate.BindUFunction(NewDelegate, UNePyDelegate::FakeFuncName);
	Prop->AddDelegate(ScriptDelegate, nullptr, Value);

	FNePyHouseKeeper::Get().RegisterDynamicMulticastDelegateUsage(InTarget, this);
}

void FNePyDynamicMulticastDelegateWrapper::AddUObjectDynamicUnique(const UObject* InTarget, PyObject* InPyCallable)
{
	if (!UObjectDelegates.Contains(InTarget))
	{
		UObjectDelegates.Add(InTarget, {});
	}

	int32 DelegateIndex = FindDelegateIndex(InPyCallable);
	if (DelegateIndex == INDEX_NONE)
	{
		UNePyDelegate* NewDelegate = NewObject<UNePyDelegate>();
		NewDelegate->Initialize(Prop->SignatureFunction, InTarget, InPyCallable);
		Delegates.Add(NewDelegate);
		UObjectDelegates[InTarget].Add(NewDelegate);

		FScriptDelegate ScriptDelegate;
		ScriptDelegate.BindUFunction(NewDelegate, UNePyDelegate::FakeFuncName);
		Prop->AddDelegate(ScriptDelegate, nullptr, Value);

		FNePyHouseKeeper::Get().RegisterDynamicMulticastDelegateUsage(InTarget, this);
	}
}

void FNePyDynamicMulticastDelegateWrapper::RemoveUObjectDynamic(const UObject* InTarget, PyObject* InPyCallable)
{
	if (!UObjectDelegates.Contains(InTarget))
	{
		return;
	}
	int32 DelegateIndex = FindDelegateIndex(InPyCallable);
	if (DelegateIndex != INDEX_NONE)
	{
		UNePyDelegate* FoundDelegate = Delegates[DelegateIndex];
		if (UObjectDelegates[InTarget].Contains(FoundDelegate))
		{
			Delegates.RemoveAt(DelegateIndex);
			UObjectDelegates[InTarget].Remove(FoundDelegate);
			FScriptDelegate ScriptDelegate;
			ScriptDelegate.BindUFunction(FoundDelegate, UNePyDelegate::FakeFuncName);
			Prop->RemoveDelegate(ScriptDelegate, nullptr, Value);
			FoundDelegate->Finalize();

			if (UObjectDelegates[InTarget].Num() == 0)
			{
				UObjectDelegates.Remove(InTarget);
				FNePyHouseKeeper::Get().UnRegisterDynamicMulticastDelegateUsage(InTarget, this);
			}
		}
	}
}

void FNePyDynamicMulticastDelegateWrapper::RemoveUObjectAllDynamic(const UObject* InTarget)
{
	if (!UObjectDelegates.Contains(InTarget))
	{
		return;
	}
	for (UNePyDelegate* Delegate : UObjectDelegates[InTarget])
	{
		int32 DelegateIndex = Delegates.Find(Delegate);
		if (DelegateIndex != INDEX_NONE)
		{
			Delegates.RemoveAt(DelegateIndex);
			FScriptDelegate ScriptDelegate;
			ScriptDelegate.BindUFunction(Delegate, UNePyDelegate::FakeFuncName);
			Prop->RemoveDelegate(ScriptDelegate, nullptr, Value);
			Delegate->Finalize();
		}
	}
	UObjectDelegates.Remove(InTarget);
	FNePyHouseKeeper::Get().UnRegisterDynamicMulticastDelegateUsage(InTarget, this);
}

void FNePyDynamicMulticastDelegateWrapper::ReinstanceUObjectDynamic(const UObject* OldObject, const UObject* NewObject)
{
	if (!UObjectDelegates.Contains(OldObject))
	{
		return;
	}
	TArray<UNePyDelegate*> DelegatesToMove = UObjectDelegates[OldObject];
	UObjectDelegates.Remove(OldObject);
	UObjectDelegates.Add(NewObject, DelegatesToMove);
}

void FNePyDynamicMulticastDelegateWrapper::Clear()
{
	for (UNePyDelegate* Delegate : Delegates)
	{
		Delegate->Finalize();
	}
	Delegates.Empty();	
	for (const auto& Pair : UObjectDelegates)
	{
		const UObject* TargetObject = Pair.Key;
		FNePyHouseKeeper::Get().UnRegisterDynamicMulticastDelegateUsage(TargetObject, this);
	}
	UObjectDelegates.Empty();
	Prop->ClearDelegate(nullptr, Value);
}

void FNePyDynamicMulticastDelegateWrapper::Dealloc(FNePyDynamicMulticastDelegateWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此DelegatePtr必为空
	check(!InSelf->Value);

	for (UNePyDelegate* Delegate : InSelf->Delegates)
	{
		Delegate->Finalize();
	}
	InSelf->Delegates.~TArray();

	for (const auto& Pair : InSelf->UObjectDelegates)
	{
		const UObject* TargetObject = Pair.Key;
		FNePyHouseKeeper::Get().UnRegisterDynamicMulticastDelegateUsage(TargetObject, InSelf);
	}
	InSelf->UObjectDelegates.~TMap();

	InSelf->ob_type->tp_free(InSelf);
}

int FNePyDynamicMulticastDelegateWrapper::Init(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init DynamicMulticastDelegate directly!");
	return -1;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::Repr(FNePyDynamicMulticastDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return PyUnicode_FromFormat("<DynamicMulticastDelegate '%s' at %p>", TCHAR_TO_UTF8(*InSelf->Prop->GetName()), InSelf->Value);
}

PyObject* FNePyDynamicMulticastDelegateWrapper::IsValid(FNePyDynamicMulticastDelegateWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyIsBound(FNePyDynamicMulticastDelegateWrapper* InSelf)
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

PyObject* FNePyDynamicMulticastDelegateWrapper::PyContains(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (InSelf->Contains(InPyCallable))
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyAdd(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	InSelf->Add(InPyCallable);
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyAddUnique(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	InSelf->AddUnique(InPyCallable);
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyRemove(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	InSelf->Remove(InPyCallable);
	Py_RETURN_NONE;
}

bool FNePyDynamicMulticastDelegateWrapper::ParseDynamicDelegateArgs(PyObject* InArgs, const char* FunctionName, PyObject*& OutTargetPyObject, PyObject*& OutPyCallable)
{
	OutTargetPyObject = nullptr;
	OutPyCallable = nullptr;
	
	const Py_ssize_t ArgCount = PyTuple_GET_SIZE(InArgs);

	if (ArgCount == 1)
	{
		// 场景: Function(o.Func)
		// 参数是一个绑定方法 (bound method)
		PyObject* BoundMethod = PyTuple_GET_ITEM(InArgs, 0);
		if (PyMethod_Check(BoundMethod))
		{
			OutTargetPyObject = PyMethod_GET_SELF(BoundMethod);
			OutPyCallable = PyMethod_GET_FUNCTION(BoundMethod);
		}
		else
		{
			PyErr_Format(PyExc_TypeError, "%s with one argument requires a bound method.", FunctionName);
			return false;
		}
	}
	else if (ArgCount == 2)
	{
		// 场景: Function(o, o.Func) 或 Function(o, Class.Func)
		PyObject* Arg1 = PyTuple_GET_ITEM(InArgs, 0);
		PyObject* Arg2 = PyTuple_GET_ITEM(InArgs, 1);

		if (PyMethod_Check(Arg2))
		{
			// 如果第二个参数是绑定方法，优先使用它解包出的对象和函数
			OutTargetPyObject = PyMethod_GET_SELF(Arg2);
			OutPyCallable = PyMethod_GET_FUNCTION(Arg2);
			if (OutTargetPyObject != Arg1)
			{
				PyErr_SetString(PyExc_ValueError, "The target object does not match the bound method's self.");
				return false;
			}
		}
		else
		{
			// 否则，假定是 (对象, 可调用对象) 的形式
			OutTargetPyObject = Arg1;
			OutPyCallable = Arg2;
		}
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "%s takes 1 or 2 arguments, but %zd were given.", FunctionName, ArgCount);
		return false;
	}

	if (!OutTargetPyObject)
	{
		PyErr_Format(PyExc_ValueError, "Could not resolve a valid target object for %s.", FunctionName);
		return false;
	}

	if (!PyCallable_Check(OutPyCallable))
	{
		PyErr_Format(PyExc_TypeError, "The callback for %s must be a callable, but got '%s'.", 
			FunctionName, Py_TYPE(OutPyCallable)->tp_name);
		return false;
	}

	return true;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyAddDynamic(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* TargetPyObject = nullptr;
	PyObject* PyCallable = nullptr;

	if (!ParseDynamicDelegateArgs(InArgs, "AddDynamic", TargetPyObject, PyCallable))
	{
		return nullptr;
	}

	UObject* TargetUObject = NePyBase::ToCppObject<UObject>(TargetPyObject);
	if (!TargetUObject)
	{
		PyErr_Format(PyExc_TypeError, "The target for AddDynamic must be a UObject, but got '%s'.", 
			Py_TYPE(TargetPyObject)->tp_name);
		return nullptr;
	}

	InSelf->AddUObjectDynamic(TargetUObject, PyCallable);
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyAddDynamicUnique(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* TargetPyObject = nullptr;
	PyObject* PyCallable = nullptr;

	if (!ParseDynamicDelegateArgs(InArgs, "AddDynamicUnique", TargetPyObject, PyCallable))
	{
		return nullptr;
	}

	UObject* TargetUObject = NePyBase::ToCppObject<UObject>(TargetPyObject);
	if (!TargetUObject)
	{
		PyErr_Format(PyExc_TypeError, "The target for AddDynamicUnique must be a UObject, but got '%s'.", 
			Py_TYPE(TargetPyObject)->tp_name);
		return nullptr;
	}

	InSelf->AddUObjectDynamicUnique(TargetUObject, PyCallable);
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyRemoveDynamic(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* TargetPyObject = nullptr;
	PyObject* PyCallable = nullptr;

	if (!ParseDynamicDelegateArgs(InArgs, "RemoveDynamic", TargetPyObject, PyCallable))
	{
		return nullptr;
	}

	UObject* TargetUObject = NePyBase::ToCppObject<UObject>(TargetPyObject);
	if (!TargetUObject)
	{
		PyErr_Format(PyExc_TypeError, "The target for RemoveDynamic must be a UObject, but got '%s'.", 
			Py_TYPE(TargetPyObject)->tp_name);
		return nullptr;
	}

	InSelf->RemoveUObjectDynamic(TargetUObject, PyCallable);
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyClear(FNePyDynamicMulticastDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	InSelf->Clear();
	Py_RETURN_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::GetPythonCallbacks(FNePyDynamicMulticastDelegateWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyList = PyList_New(InSelf->Delegates.Num());
	for (int32 Index = 0; Index < InSelf->Delegates.Num(); ++Index)
	{
		UNePyDelegate* Delegate = InSelf->Delegates[Index];
		PyObject* PyCallback = Delegate->GetPyCallable();
		if (!PyCallback)
		{
			PyCallback = Py_None;
		}

		Py_INCREF(PyCallback);
		PyList_SetItem(PyList, Index, PyCallback);
	}
	return PyList;
}

int32 FNePyDynamicMulticastDelegateWrapper::FindDelegateIndex(PyObject* InPyCallable)
{
	int32 DelegateIndex = 0;
	for (UNePyDelegate* Delegate : Delegates)
	{
		if (Delegate->UsesPyCallable(InPyCallable))
		{
			return DelegateIndex;
		}
		DelegateIndex += 1;
	}

	return INDEX_NONE;
}

PyObject* FNePyDynamicMulticastDelegateWrapper::PyBroadcast(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (!InSelf->IsBound())
	{
		Py_RETURN_NONE;
	}

	return InSelf->BroadcastInternal(InArgs);
}

// 参考：PyWrapperDelegate.cpp FPyWrapperDelegateImpl::CallDelegate
PyObject* FNePyDynamicMulticastDelegateWrapper::BroadcastInternal(PyObject* InArgs)
{
	if (!PyTuple_Check(InArgs))
	{
		PyErr_BadInternalCall();
		return nullptr;
	}

	const FMulticastScriptDelegate* ScriptDelegate = Prop->GetMulticastDelegate(Value);

	if (Prop->SignatureFunction->ChildProperties == nullptr)
	{
		// Simple case, no parameters or return value
		const Py_ssize_t NumPyArgs = PyTuple_GET_SIZE(InArgs);
		if (NumPyArgs != 0)
		{
			PyErr_Format(PyExc_TypeError, "%s takes no arguments (%ld given)",
				TCHAR_TO_UTF8(*Prop->GetName()), NumPyArgs);
			return nullptr;
		}
		
		ScriptDelegate->ProcessMulticastDelegate<UObject>(nullptr);
		Py_RETURN_NONE;
	}

	// Complex case, parameters or return value
	TArray<const FProperty*> InputParams;
	TArray<const FProperty*> OutputParams;
	NePyGenUtil::ExtractFunctionParams(Prop->SignatureFunction, InputParams, OutputParams);

	const Py_ssize_t NumPyArgs = PyTuple_GET_SIZE(InArgs);
	if (NumPyArgs != InputParams.Num())
	{
		PyErr_Format(PyExc_TypeError, "%s takes %d arguments, got %ld",
			TCHAR_TO_UTF8(*Prop->GetName()), InputParams.Num(), NumPyArgs);
		return nullptr;
	}

	FStructOnScope DelegateParams(Prop->SignatureFunction);
	for (int32 ParamIndex = 0; ParamIndex < NumPyArgs; ++ParamIndex)
	{
		PyObject* PyArg = PyTuple_GetItem(InArgs, ParamIndex);
		if (!NePyBase::TryConvertPyObjectToFPropertyInContainer(PyArg, InputParams[ParamIndex], DelegateParams.GetStructMemory(), 0, nullptr))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyArg, InputParams[ParamIndex]);
			return nullptr;
		}
	}

	ScriptDelegate->ProcessMulticastDelegate<UObject>(DelegateParams.GetStructMemory());
	return NePyGenUtil::PackReturnValues(DelegateParams.GetStructMemory(), OutputParams, nullptr);
}
