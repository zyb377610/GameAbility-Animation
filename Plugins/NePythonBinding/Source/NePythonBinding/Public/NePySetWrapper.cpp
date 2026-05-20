#include "NePySetWrapper.h"
#include "NePyBase.h"
#include "NePyPropertyConvert.h"

static PyTypeObject FNePySetWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"SetWrapper", /* tp_name */
	sizeof(FNePySetWrapper), /* tp_basicsize */
};

static PyTypeObject FNePyStructSetWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"SetWrapper", /* tp_name */
	sizeof(FNePyStructSetWrapper), /* tp_basicsize */
};

static PyTypeObject FNePySetWrapperKeyIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"SetWrapperKeyIterator", /* tp_name */
	sizeof(FNePySetWrapperKeyIterator), /* tp_basicsize */
};

void FNePySetWrapper::InitPyType()
{
#define DUAL_NAME_METHOD(lower, upper, func, flags, doc) \
	{ lower, func, flags, doc }, \
	{ upper, func, flags, doc }

	static PyMethodDef PyMethods[] = {
		DUAL_NAME_METHOD("is_valid", "IsValid", NePyCFunctionCast(&FNePySetWrapper::IsValid), METH_NOARGS, "() -> bool"),
		DUAL_NAME_METHOD("copy", "Copy", NePyCFunctionCast(&FNePySetWrapper::Copy), METH_NOARGS, "() -> set"),
		DUAL_NAME_METHOD("add", "Add", NePyCFunctionCast(&FNePySetWrapper::Add), METH_O, "(Value: typing.Any) -> None"),
		DUAL_NAME_METHOD("discard", "Discard", NePyCFunctionCast(&FNePySetWrapper::Discard), METH_O, "(Value: typing.Any) -> None"),
		DUAL_NAME_METHOD("remove", "Remove", NePyCFunctionCast(&FNePySetWrapper::Remove), METH_O, "(Value: typing.Any) -> None"),
		DUAL_NAME_METHOD("pop", "Pop", NePyCFunctionCast(&FNePySetWrapper::Pop), METH_NOARGS, "() -> typing.Any"),
		DUAL_NAME_METHOD("clear", "Clear", NePyCFunctionCast(&FNePySetWrapper::Clear), METH_NOARGS, "() -> None"),
		{ nullptr, nullptr, 0, nullptr }
	};

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FNePySetWrapper::Len;
	PySequence.sq_contains = (objobjproc)&FNePySetWrapper::Contains;

	{
		PyTypeObject* PyType = &FNePySetWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePySetWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePySetWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePySetWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePySetWrapper::Repr;
		PyType->tp_iter = (getiterfunc)&FNePySetWrapper::Iter;
		PyType->tp_richcompare = (richcmpfunc)&FNePySetWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePyStructSetWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyStructSetWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePySetWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePySetWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePySetWrapper::Repr;
		PyType->tp_iter = (getiterfunc)&FNePySetWrapper::Iter;
		PyType->tp_richcompare = (richcmpfunc)&FNePySetWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePySetWrapperKeyIteratorType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePySetWrapperKeyIterator::Dealloc;
		PyType->tp_init = (initproc)&FNePySetWrapperKeyIterator::Init;
		PyType->tp_iter = (getiterfunc)&FNePySetWrapperKeyIterator::Iter;
		PyType->tp_iternext = (iternextfunc)&FNePySetWrapperKeyIterator::IterNext;
		PyType_Ready(PyType);
	}

#undef DUAL_NAME_METHOD
}

FNePySetWrapper* FNePySetWrapper::New(UObject* InObject, void* InMemberPtr, const char* PropName)
{
	UClass* Class = InObject->GetClass();

	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(Class, InObject, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' object has no property '%s'", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	const FSetProperty* SetProp = CastField<FSetProperty>(Prop);
	if (!SetProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not Set property", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	return New(InObject, (void*)InMemberPtr, SetProp);
}

FNePySetWrapper* FNePySetWrapper::New(UObject* InObject, void* InMemberPtr, const FSetProperty* InSetProp)
{
	check(InMemberPtr == InSetProp->ContainerPtrToValuePtr<void>(InObject));

	auto Constructor = [InMemberPtr, InSetProp]() -> FNePyPropObject* {
		FNePySetWrapper* RetValue = PyObject_New(FNePySetWrapper, &FNePySetWrapperType);
		RetValue->Value = InMemberPtr;
		RetValue->Prop = InSetProp;
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InMemberPtr, Constructor);
	return (FNePySetWrapper*)RetValue;
}

FNePySetWrapper* FNePySetWrapper::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePySetWrapperType || PyType == &FNePyStructSetWrapperType)
		{

			return (FNePySetWrapper*)InPyObj;
		}
	}
	return nullptr;
}

bool FNePySetWrapper::Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InClass, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	const FSetProperty* SetProp = CastField<FSetProperty>(Prop);
	if (!SetProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not Set property", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	UObject* InOwnerObject = InClass->IsA<UClass>() ? (UObject*)InInstance : nullptr;
	return FNePySetWrapper::Assign(InOther, SetProp, InMemberPtr, InOwnerObject);
}

bool FNePySetWrapper::Assign(PyObject* InOther, const FSetProperty* InProp, void* InDest, UObject* InOwnerObject)
{
	FNePyObjectPtr PySet;
	if (PySet_Check(InOther))
	{
		PySet = NePyNewReference(InOther);
	}
	else if (FNePySetWrapper* OtherSet = FNePySetWrapper::Check(InOther))
	{
		PySet = NePyStealReference(ToPySet(OtherSet));
		if (!PySet)
		{
			return false;
		}
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "unable to assign '%s' to %s (%s)", InOther->ob_type->tp_name, TCHAR_TO_UTF8(*InProp->GetName()), TCHAR_TO_UTF8(*InProp->GetClass()->GetName()));
		return false;
	}

	NePyPyObjectToPropertyFunc Converter = NePyGetPyObjectToPropertyConverter(InProp->ElementProp);
	if (!Converter)
	{
		NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InProp->ElementProp);
		return false;
	}

	FNePyPropValue TempSet(InProp);
	FScriptSetHelper SetHelper(InProp, TempSet.Value);

	FNePyObjectPtr PySetIter = NePyStealReference(PyObject_GetIter(PySet));
	while (FNePyObjectPtr PyElem = NePyStealReference(PyIter_Next(PySetIter)))
	{
		int32 Index = SetHelper.AddDefaultValue_Invalid_NeedsRehash();
		if (!Converter(PyElem, SetHelper.ElementProp, SetHelper.GetElementPtr(Index), InOwnerObject))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyElem, SetHelper.ElementProp);
			return false;
		}
	}

	InProp->CopyCompleteValue(InDest, TempSet.Value);
	return true;
}

PyObject* FNePySetWrapper::ToPySet(FNePySetWrapper* InSelf)
{
	return ToPySet(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePySetWrapper::ToPySet(const FSetProperty* InProp, const void* InValue, UObject* InOwnerObject)
{
	NePyPropertyToPyObjectFunc ElementConverter = NePyGetPropertyToPyObjectConverterNoDependency(InProp->ElementProp);
	if (!ElementConverter)
	{
		return nullptr;
	}

	FScriptSetHelper SetHelper(InProp, InValue);
	PyObject* PySetObj = PySet_New(nullptr);
	for (int32 Index = 0; Index < SetHelper.Num(); Index++)
	{
		if (SetHelper.IsValidIndex(Index))
		{
			FNePyObjectPtr PyElem = NePyStealReference(ElementConverter(SetHelper.ElementProp, SetHelper.GetElementPtr(Index), InOwnerObject));
			if (!PyElem)
			{
				Py_DECREF(PySetObj);
				return nullptr;
			}
			PySet_Add(PySetObj, PyElem);
		}
	}
	return PySetObj;
}

UObject* FNePySetWrapper::GetOwnerObject(FNePySetWrapper* InSelf)
{
	if (!InSelf->Prop || !InSelf->Value)
	{
		return nullptr;
	}

	if (Py_TYPE(InSelf) == &FNePyStructSetWrapperType)
	{
		return nullptr;
	}

	UObject* Owner = (UObject*)((uint8*)InSelf->Value - InSelf->Prop->GetOffset_ForInternal());
	return Owner;
}

void FNePySetWrapper::Dealloc(FNePySetWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此SetPtr必为空
	check(!InSelf->Value);
	Py_TYPE(InSelf)->tp_free(InSelf);
}

int FNePySetWrapper::Init(FNePySetWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init SetWrapper directly!");
	return -1;
}

PyObject* FNePySetWrapper::Repr(FNePySetWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = SelfScriptSetHelper.Num();

	FString ExportedSet;
	for (int32 ElementIndex = 0, SparseIndex = 0; ElementIndex < ElementCount; ++SparseIndex)
	{
		if (!SelfScriptSetHelper.IsValidIndex(SparseIndex))
		{
			continue;
		}

		if (ElementIndex > 0)
		{
			ExportedSet += TEXT(", ");
		}
		ExportedSet += NePyBase::GetFriendlyPropertyValue(SelfScriptSetHelper.GetElementProperty(), SelfScriptSetHelper.GetElementPtr(SparseIndex), PPF_Delimited | PPF_IncludeTransient);
		++ElementIndex;
	}
	return PyUnicode_FromFormat("{%s}", TCHAR_TO_UTF8(*ExportedSet));
}

PyObject* FNePySetWrapper::Iter(FNePySetWrapper* InSelf)
{
	return FNePySetWrapperKeyIterator::New(InSelf);
}

PyObject* FNePySetWrapper::RichCompare(FNePySetWrapper* InSelf, PyObject* InOther, int InOp)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	if (InOp != Py_EQ && InOp != Py_NE)
	{
		PyErr_SetString(PyExc_TypeError, "only == and != comparison is supported");
		return nullptr;
	}

	if (PySet_Check(InOther))
	{
		PyObject* This = ToPySet(InSelf);
		if (!This)
		{
			return nullptr;
		}
		return Py_TYPE(This)->tp_richcompare(This, InOther, InOp);
	}

	if (FNePySetWrapper* OtherSet = Check(InOther))
	{
		bool bIsIdentical = InSelf->Prop->SameType(OtherSet->Prop);
		if (bIsIdentical)
		{
			bIsIdentical = InSelf->Prop->Identical(InSelf->Value, OtherSet->Value, PPF_None);
		}
		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

PyObject* FNePySetWrapper::IsValid(FNePySetWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePySetWrapper::Copy(FNePySetWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return ToPySet(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePySetWrapper::Add(FNePySetWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyPropValue ContainerEntryValue(InSelf->Prop->ElementProp);
	if (!ContainerEntryValue.SetValue(InValue))
	{
		return nullptr;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	SelfScriptSetHelper.AddElement(ContainerEntryValue.Value);

	Py_RETURN_NONE;
}

PyObject* FNePySetWrapper::Discard(FNePySetWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyPropValue ContainerEntryValue(InSelf->Prop->ElementProp);
	if (!ContainerEntryValue.SetValue(InValue))
	{
		return nullptr;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	const bool bWasRemoved = SelfScriptSetHelper.RemoveElement(ContainerEntryValue.Value);

	Py_RETURN_NONE;
}

PyObject* FNePySetWrapper::Remove(FNePySetWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyPropValue ContainerEntryValue(InSelf->Prop->ElementProp);
	if (!ContainerEntryValue.SetValue(InValue))
	{
		return nullptr;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	const bool bWasRemoved = SelfScriptSetHelper.RemoveElement(ContainerEntryValue.Value);

	if (!bWasRemoved)
	{
		PyErr_SetString(PyExc_KeyError, "The given value was not found in the set");
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* FNePySetWrapper::Pop(FNePySetWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	const int32 SelfElementCount = SelfScriptSetHelper.Num();

	if (SelfElementCount == 0)
	{
		PyErr_SetString(PyExc_KeyError, "Cannot pop from an empty set");
		return nullptr;
	}

	for (int32 SelfSparseIndex = 0; ; ++SelfSparseIndex)
	{
		if (!SelfScriptSetHelper.IsValidIndex(SelfSparseIndex))
		{
			continue;
		}

		PyObject* PyReturnValue = NePyBase::TryConvertFPropertyToPyObjectDirectNoDependency(SelfScriptSetHelper.GetElementProperty(), SelfScriptSetHelper.GetElementPtr(SelfSparseIndex), OwnerObject);
		if (!PyReturnValue)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptSetHelper.GetElementProperty());
			return nullptr;
		}

		SelfScriptSetHelper.RemoveAt(SelfSparseIndex);

		return PyReturnValue;
	}
}

PyObject* FNePySetWrapper::Clear(FNePySetWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	SelfScriptSetHelper.EmptyElements();
	Py_RETURN_NONE;
}

Py_ssize_t FNePySetWrapper::Len(FNePySetWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	return SelfScriptSetHelper.Num();
}

int FNePySetWrapper::Contains(FNePySetWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FNePyPropValue ContainerEntryValue(InSelf->Prop->ElementProp);
	if (!ContainerEntryValue.SetValue(InValue))
	{
		PyErr_Clear();
		return 0;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->Prop, InSelf->Value);
	return SelfScriptSetHelper.FindElementIndexFromHash(ContainerEntryValue.Value) != INDEX_NONE ? 1 : 0;
}

FNePyStructSetWrapper* FNePyStructSetWrapper::New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InStruct, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	const FSetProperty* SetProp = CastField<FSetProperty>(Prop);
	if (!SetProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not Set property", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	return FNePyStructSetWrapper::New(PyOuter, InMemberPtr, SetProp);
}

FNePyStructSetWrapper* FNePyStructSetWrapper::New(PyObject* PyOuter, void* InMemberPtr, const FSetProperty* InSetProp)
{
	FNePyStructSetWrapper* RetValue = PyObject_New(FNePyStructSetWrapper, &FNePyStructSetWrapperType);
	RetValue->Value = InMemberPtr;
	RetValue->Prop = InSetProp;
	RetValue->PyOuter = PyOuter;
	Py_INCREF(PyOuter);
	return RetValue;
}

void FNePyStructSetWrapper::Dealloc(FNePyStructSetWrapper* InSelf)
{
	if (InSelf->PyOuter)
	{
		Py_DECREF(InSelf->PyOuter);
		InSelf->PyOuter = nullptr;
	}
	Py_TYPE(InSelf)->tp_free(InSelf);
}

FNePySetWrapperKeyIterator* FNePySetWrapperKeyIterator::New(FNePySetWrapper* IterInstance)
{
	FNePySetWrapperKeyIterator* RetValue = PyObject_New(FNePySetWrapperKeyIterator, &FNePySetWrapperKeyIteratorType);
	Py_INCREF(IterInstance);
	RetValue->IterInstance = IterInstance;
	RetValue->IterIndex = GetElementIndex(RetValue, 0);
	return RetValue;
}

void FNePySetWrapperKeyIterator::Dealloc(FNePySetWrapperKeyIterator* InSelf)
{
	Py_XDECREF(InSelf->IterInstance);
	InSelf->IterInstance = nullptr;
	InSelf->IterIndex = 0;
	Py_TYPE(InSelf)->tp_free(InSelf);
}

int FNePySetWrapperKeyIterator::Init(FNePySetWrapperKeyIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init SetWrapperKeyIterator directly!");
	return -1;
}

PyObject* FNePySetWrapperKeyIterator::Iter(FNePySetWrapperKeyIterator* InSelf)
{
	Py_INCREF(InSelf);
	return InSelf;
}

PyObject* FNePySetWrapperKeyIterator::IterNext(FNePySetWrapperKeyIterator* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	UObject* OwnerObject = FNePySetWrapper::GetOwnerObject(InSelf->IterInstance);
	FScriptSetHelper ScriptSetHelper(InSelf->IterInstance->Prop, InSelf->IterInstance->Value);
	const int32 SparseCount = ScriptSetHelper.GetMaxIndex();

	if (InSelf->IterIndex < SparseCount)
	{
		const int32 ElementIndex = InSelf->IterIndex;
		InSelf->IterIndex = GetElementIndex(InSelf, InSelf->IterIndex + 1);

		if (!ScriptSetHelper.IsValidIndex(ElementIndex))
		{
			PyErr_SetString(PyExc_IndexError, "Iterator was on an invalid element index! Was the set changed while iterating?");
			return nullptr;
		}

		PyObject* PyItemObj = NePyBase::TryConvertFPropertyToPyObjectDirectPyOuter(ScriptSetHelper.GetElementProperty(), ScriptSetHelper.GetElementPtr(ElementIndex), InSelf);
		if (!PyItemObj)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(ScriptSetHelper.GetElementProperty());
			return nullptr;
		}
		return PyItemObj;
	}

	PyErr_SetObject(PyExc_StopIteration, Py_None);
	return nullptr;
}

bool FNePySetWrapperKeyIterator::ValidateInternalState(FNePySetWrapperKeyIterator* InSelf)
{
	if (!InSelf->IterInstance)
	{
		PyErr_SetString(PyExc_Exception, "Internal Error - IterInstance is null!");
		return false;
	}

	if (!InSelf->IterInstance->CheckValidAndSetPyErr())
	{
		return false;
	}

	return true;
}

int32 FNePySetWrapperKeyIterator::GetElementIndex(FNePySetWrapperKeyIterator* InSelf, int32 InSparseIndex)
{
	FScriptSetHelper ScriptSetHelper(InSelf->IterInstance->Prop, InSelf->IterInstance->Value);
	const int32 SparseCount = ScriptSetHelper.GetMaxIndex();

	int32 ElementIndex = InSparseIndex;
	for (; ElementIndex < SparseCount; ++ElementIndex)
	{
		if (ScriptSetHelper.IsValidIndex(ElementIndex))
		{
			break;
		}
	}

	return ElementIndex;
}
