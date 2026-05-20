#include "NePyMapWrapper.h"
#include "NePyBase.h"
#include "NePyPropertyConvert.h"

static PyTypeObject FNePyMapWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"MapWrapper", /* tp_name */
	sizeof(FNePyMapWrapper), /* tp_basicsize */
};

static PyTypeObject FNePyStructMapWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"MapWrapper", /* tp_name */
	sizeof(FNePyStructMapWrapper), /* tp_basicsize */
};

static PyTypeObject FNePyMapWrapperKeyIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"MapWrapperKeyIterator", /* tp_name */
	sizeof(FNePyMapWrapperKeyIterator), /* tp_basicsize */
};

void FNePyMapWrapper::InitPyType()
{
#define DUAL_NAME_METHOD(lower, upper, func, flags, doc) \
	{ lower, func, flags, doc }, \
	{ upper, func, flags, doc }

	static PyMethodDef PyMethods[] = {
		DUAL_NAME_METHOD("is_valid", "IsValid", NePyCFunctionCast(&FNePyMapWrapper::IsValid), METH_NOARGS, "() -> bool"),
		DUAL_NAME_METHOD("copy", "Copy", NePyCFunctionCast(&FNePyMapWrapper::Copy), METH_NOARGS, "() -> dict"),
		DUAL_NAME_METHOD("clear", "Clear", NePyCFunctionCast(&FNePyMapWrapper::Clear), METH_NOARGS, "() -> None"),
		DUAL_NAME_METHOD("get", "Get", NePyCFunctionCast(&FNePyMapWrapper::Get), METH_VARARGS | METH_KEYWORDS, "(Key: typing.Any, Default: typing.Any = ...) -> typing.Any"),
		DUAL_NAME_METHOD("items", "Items", NePyCFunctionCast(&FNePyMapWrapper::Items), METH_NOARGS, "() -> list"),
		DUAL_NAME_METHOD("keys", "Keys", NePyCFunctionCast(&FNePyMapWrapper::Keys), METH_NOARGS, "() -> list"),
		DUAL_NAME_METHOD("values", "Values", NePyCFunctionCast(&FNePyMapWrapper::Values), METH_NOARGS, "() -> list"),
		DUAL_NAME_METHOD("pop", "Pop", NePyCFunctionCast(&FNePyMapWrapper::Pop), METH_VARARGS | METH_KEYWORDS, "(Key: typing.Any, Default: typing.Any = ...) -> typing.Any"),
		DUAL_NAME_METHOD("pop_item", "PopItem", NePyCFunctionCast(&FNePyMapWrapper::PopItem), METH_NOARGS, "() -> typing.Any"),
		DUAL_NAME_METHOD("set_default", "SetDefault", NePyCFunctionCast(&FNePyMapWrapper::SetDefault), METH_VARARGS | METH_KEYWORDS, "(Key: typing.Any, Default: typing.Any = ...) -> typing.Any"),
		DUAL_NAME_METHOD("update", "Update", NePyCFunctionCast(&FNePyMapWrapper::Update), METH_O , "(dict) -> None"),
		{ nullptr, nullptr, 0, nullptr }
	};

	static PySequenceMethods PySequence;
	PySequence.sq_contains = (objobjproc)&FNePyMapWrapper::Contains;

	static PyMappingMethods PyMapping;
	PyMapping.mp_length = (lenfunc)&FNePyMapWrapper::Len;
	PyMapping.mp_subscript = (binaryfunc)&FNePyMapWrapper::GetItem;
	PyMapping.mp_ass_subscript = (objobjargproc)&FNePyMapWrapper::SetItem;

	{
		PyTypeObject* PyType = &FNePyMapWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyMapWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyMapWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePyMapWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePyMapWrapper::Repr;
		PyType->tp_iter = (getiterfunc)&FNePyMapWrapper::Iter;
		PyType->tp_richcompare = (richcmpfunc)&FNePyMapWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType->tp_as_mapping = &PyMapping;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePyStructMapWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyStructMapWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyMapWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePyMapWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePyMapWrapper::Repr;
		PyType->tp_iter = (getiterfunc)&FNePyMapWrapper::Iter;
		PyType->tp_richcompare = (richcmpfunc)&FNePyMapWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType->tp_as_mapping = &PyMapping;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePyMapWrapperKeyIteratorType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyMapWrapperKeyIterator::Dealloc;
		PyType->tp_init = (initproc)&FNePyMapWrapperKeyIterator::Init;
		PyType->tp_iter = (getiterfunc)&FNePyMapWrapperKeyIterator::Iter;
		PyType->tp_iternext = (iternextfunc)&FNePyMapWrapperKeyIterator::IterNext;
		PyType_Ready(PyType);
	}

#undef DUAL_NAME_METHOD
}

FNePyMapWrapper* FNePyMapWrapper::New(UObject* InObject, void* InMemberPtr, const char* PropName)
{
	UClass* Class = InObject->GetClass();

	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(Class, InObject, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' object has no property '%s'", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	const FMapProperty* MapProp = CastField<FMapProperty>(Prop);
	if (!MapProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not Map property", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	return New(InObject, (void*)InMemberPtr, MapProp);
}

FNePyMapWrapper* FNePyMapWrapper::New(UObject* InObject, void* InMemberPtr, const FMapProperty* InMapProp)
{
	check(InMemberPtr == InMapProp->ContainerPtrToValuePtr<void>(InObject));

	auto Constructor = [InMemberPtr, InMapProp]() -> FNePyPropObject* {
		FNePyMapWrapper* RetValue = PyObject_New(FNePyMapWrapper, &FNePyMapWrapperType);
		RetValue->Value = InMemberPtr;
		RetValue->Prop = InMapProp;
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InMemberPtr, Constructor);
	return (FNePyMapWrapper*)RetValue;
}

FNePyMapWrapper* FNePyMapWrapper::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePyMapWrapperType || PyType == &FNePyStructMapWrapperType)
		{

			return (FNePyMapWrapper*)InPyObj;
		}
	}
	return nullptr;
}

bool FNePyMapWrapper::Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InClass, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	const FMapProperty* MapProp = CastField<FMapProperty>(Prop);
	if (!MapProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not Map property", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	UObject* InOwnerObject = InClass->IsA<UClass>() ? (UObject*)InInstance : nullptr;
	return FNePyMapWrapper::Assign(InOther, MapProp, InMemberPtr, InOwnerObject);
}

bool FNePyMapWrapper::Assign(PyObject* InOther, const FMapProperty* InProp, void* InDest, UObject* InOwnerObject)
{
	FNePyObjectPtr PyDict;
	if (PyDict_Check(InOther))
	{
		PyDict = NePyNewReference(InOther);
	}
	else if (FNePyMapWrapper* OtherMap = FNePyMapWrapper::Check(InOther))
	{
		PyDict = NePyStealReference(ToPyDict(OtherMap));
		if (!PyDict)
		{
			return false;
		}
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "unable to assign '%s' to %s (%s)", InOther->ob_type->tp_name, TCHAR_TO_UTF8(*InProp->GetName()), TCHAR_TO_UTF8(*InProp->GetClass()->GetName()));
		return false;
	}

	NePyPyObjectToPropertyFunc KeyConverter = NePyGetPyObjectToPropertyConverter(InProp->KeyProp);
	if (!KeyConverter)
	{
		NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InProp->KeyProp);
		return false;
	}

	NePyPyObjectToPropertyFunc ValueConverter = NePyGetPyObjectToPropertyConverter(InProp->ValueProp);
	if (!ValueConverter)
	{
		NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InProp->ValueProp);
		return false;
	}

	FNePyPropValue TempMap(InProp);
	FScriptMapHelper MapHelper(InProp, TempMap.Value);

	PyObject* PyKey = nullptr;
	PyObject* PyValue = nullptr;
	Py_ssize_t PyPos = 0;
	while (PyDict_Next(PyDict, &PyPos, &PyKey, &PyValue))
	{
		int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
		if (!KeyConverter(PyKey, MapHelper.KeyProp, MapHelper.GetKeyPtr(Index), InOwnerObject))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyKey, MapHelper.KeyProp);
			return false;
		}
		if (!ValueConverter(PyValue, MapHelper.ValueProp, MapHelper.GetValuePtr(Index), InOwnerObject))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(PyValue, MapHelper.ValueProp);
			return false;
		}
	}

	InProp->CopyCompleteValue(InDest, TempMap.Value);
	return true;
}

PyObject* FNePyMapWrapper::ToPyDict(FNePyMapWrapper* InSelf)
{
	return ToPyDict(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePyMapWrapper::ToPyDict(const FMapProperty* InProp, const void* InValue, UObject* InOwnerObject)
{
	NePyPropertyToPyObjectFunc KeyConverter = NePyGetPropertyToPyObjectConverterNoDependency(InProp->KeyProp);
	NePyPropertyToPyObjectFunc ValueConverter = NePyGetPropertyToPyObjectConverterNoDependency(InProp->ValueProp);
	if (!KeyConverter || !ValueConverter)
	{
		return nullptr;
	}

	FScriptMapHelper MapHelper(InProp, InValue);
	PyObject* PyDictObj = PyDict_New();
	for (int32 Index = 0; Index < MapHelper.Num(); Index++)
	{
		if (MapHelper.IsValidIndex(Index))
		{
			FNePyObjectPtr PyKey = NePyStealReference(KeyConverter(MapHelper.KeyProp, MapHelper.GetKeyPtr(Index), InOwnerObject));
			if (!PyKey)
			{
				Py_DECREF(PyDictObj);
				return nullptr;
			}
			FNePyObjectPtr PyValue = NePyStealReference(ValueConverter(MapHelper.ValueProp, MapHelper.GetValuePtr(Index), InOwnerObject));
			if (!PyValue)
			{
				Py_DECREF(PyDictObj);
				return nullptr;
			}
			PyDict_SetItem(PyDictObj, PyKey, PyValue);
		}
	}
	return PyDictObj;
}

UObject* FNePyMapWrapper::GetOwnerObject(FNePyMapWrapper* InSelf)
{
	if (!InSelf->Prop || !InSelf->Value)
	{
		return nullptr;
	}

	if (Py_TYPE(InSelf) == &FNePyStructMapWrapperType)
	{
		return nullptr;
	}

	UObject* Owner = (UObject*)((uint8*)InSelf->Value - InSelf->Prop->GetOffset_ForInternal());
	return Owner;
}

void FNePyMapWrapper::Dealloc(FNePyMapWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此MapPtr必为空
	check(!InSelf->Value);
	Py_TYPE(InSelf)->tp_free(InSelf);
}

int FNePyMapWrapper::Init(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init MapWrapper directly!");
	return -1;
}

PyObject* FNePyMapWrapper::Repr(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = SelfScriptMapHelper.Num();

	FString ExportedMap;
	for (int32 ElementIndex = 0, SparseIndex = 0; ElementIndex < ElementCount; ++SparseIndex)
	{
		if (!SelfScriptMapHelper.IsValidIndex(SparseIndex))
		{
			continue;
		}

		if (ElementIndex > 0)
		{
			ExportedMap += TEXT(", ");
		}
		ExportedMap += NePyBase::GetFriendlyPropertyValue(SelfScriptMapHelper.GetKeyProperty(), SelfScriptMapHelper.GetKeyPtr(SparseIndex), PPF_Delimited | PPF_IncludeTransient);
		ExportedMap += TEXT(": ");
		ExportedMap += NePyBase::GetFriendlyPropertyValue(SelfScriptMapHelper.GetValueProperty(), SelfScriptMapHelper.GetValuePtr(SparseIndex), PPF_Delimited | PPF_IncludeTransient);
		++ElementIndex;
	}
	return PyUnicode_FromFormat("{%s}", TCHAR_TO_UTF8(*ExportedMap));
}

PyObject* FNePyMapWrapper::Iter(FNePyMapWrapper* InSelf)
{
	return FNePyMapWrapperKeyIterator::New(InSelf);
}

PyObject* FNePyMapWrapper::RichCompare(FNePyMapWrapper* InSelf, PyObject* InOther, int InOp)
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

	if (PyDict_Check(InOther))
	{
		PyObject* This = ToPyDict(InSelf);
		if (!This)
		{
			return nullptr;
		}
		return Py_TYPE(This)->tp_richcompare(This, InOther, InOp);
	}

	if (FNePyMapWrapper* OtherMap = Check(InOther))
	{
		bool bIsIdentical = InSelf->Prop->SameType(OtherMap->Prop);
		if (bIsIdentical)
		{
			bIsIdentical = InSelf->Prop->Identical(InSelf->Value, OtherMap->Value, PPF_None);
		}
		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

PyObject* FNePyMapWrapper::IsValid(FNePyMapWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyMapWrapper::Copy(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return ToPyDict(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePyMapWrapper::Clear(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	SelfScriptMapHelper.EmptyValues();
	Py_RETURN_NONE;
}

PyObject* FNePyMapWrapper::Get(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyKeyObj = nullptr;
	PyObject* PyDefaultObj = nullptr;

	static const char* ArgsKwdList[] = { "Key", "Default", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:Get", (char**)ArgsKwdList, &PyKeyObj, &PyDefaultObj))
	{
		return nullptr;
	}

	return DoGetItem(InSelf, PyKeyObj, PyDefaultObj);
}

PyObject* FNePyMapWrapper::Items(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	NePyPropertyToPyObjectFunc KeyConverter = NePyGetPropertyToPyObjectConverter(InSelf->Prop->KeyProp);
	if (!KeyConverter)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->KeyProp);
		return nullptr;
	}

	NePyPropertyToPyObjectFunc ValueConverter = NePyGetPropertyToPyObjectConverter(InSelf->Prop->ValueProp);
	if (!ValueConverter)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->ValueProp);
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	const int32 SelfElementCount = SelfScriptMapHelper.Num();
	FNePyObjectPtr PyList = NePyStealReference(PyList_New(SelfElementCount));
	const int32 SparseCount = SelfScriptMapHelper.GetMaxIndex();

	Py_ssize_t ListIndex = 0;
	for (int32 SelfSparseIndex = 0; SelfSparseIndex < SparseCount; ++SelfSparseIndex)
	{
		if (!SelfScriptMapHelper.IsValidIndex(SelfSparseIndex))
		{
			continue;
		}

		FNePyObjectPtr PyReturnKey = NePyStealReference(KeyConverter(SelfScriptMapHelper.GetKeyProperty(), SelfScriptMapHelper.GetKeyPtr(SelfSparseIndex), OwnerObject));
		if (!PyReturnKey)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetKeyProperty());
			return nullptr;
		}

		FNePyObjectPtr PyReturnValue = NePyStealReference(ValueConverter(SelfScriptMapHelper.GetValueProperty(), SelfScriptMapHelper.GetValuePtr(SelfSparseIndex), OwnerObject));
		if (!PyReturnValue)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetValueProperty());
			return nullptr;
		}

		PyObject* PyTuple = PyTuple_Pack(2, PyReturnKey.Get(), PyReturnValue.Get());

		check(ListIndex < SelfElementCount);
		PyList_SetItem(PyList, ListIndex++, PyTuple); // PyList_SetItem steals this reference
	}

	return PyList.Release();
}

PyObject* FNePyMapWrapper::Keys(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	NePyPropertyToPyObjectFunc Converter = NePyGetPropertyToPyObjectConverter(InSelf->Prop->KeyProp);
	if (!Converter)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->KeyProp);
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	const int32 SelfElementCount = SelfScriptMapHelper.Num();
	FNePyObjectPtr PyList = NePyStealReference(PyList_New(SelfElementCount));
	const int32 SparseCount = SelfScriptMapHelper.GetMaxIndex();

	Py_ssize_t ListIndex = 0;
	for (int32 SelfSparseIndex = 0; SelfSparseIndex < SparseCount; ++SelfSparseIndex)
	{
		if (!SelfScriptMapHelper.IsValidIndex(SelfSparseIndex))
		{
			continue;
		}

		PyObject* PyReturnKey = Converter(SelfScriptMapHelper.GetKeyProperty(), SelfScriptMapHelper.GetKeyPtr(SelfSparseIndex), OwnerObject);
		if (!PyReturnKey)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetKeyProperty());
			return nullptr;
		}

		check(ListIndex < SelfElementCount);
		PyList_SetItem(PyList, ListIndex++, PyReturnKey); // PyList_SetItem steals this reference
	}

	return PyList.Release();
}

PyObject* FNePyMapWrapper::Values(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	NePyPropertyToPyObjectFunc Converter = NePyGetPropertyToPyObjectConverter(InSelf->Prop->ValueProp);
	if (!Converter)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->ValueProp);
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	const int32 SelfElementCount = SelfScriptMapHelper.Num();
	FNePyObjectPtr PyList = NePyStealReference(PyList_New(SelfElementCount));
	const int32 SparseCount = SelfScriptMapHelper.GetMaxIndex();

	Py_ssize_t ListIndex = 0;
	for (int32 SelfSparseIndex = 0; SelfSparseIndex < SparseCount; ++SelfSparseIndex)
	{
		if (!SelfScriptMapHelper.IsValidIndex(SelfSparseIndex))
		{
			continue;
		}

		PyObject* PyReturnValue = Converter(SelfScriptMapHelper.GetValueProperty(), SelfScriptMapHelper.GetValuePtr(SelfSparseIndex), OwnerObject);
		if (!PyReturnValue)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetValueProperty());
			return nullptr;
		}

		check(ListIndex < SelfElementCount);
		PyList_SetItem(PyList, ListIndex++, PyReturnValue); // PyList_SetItem steals this reference
	}

	return PyList.Release();
}

PyObject* FNePyMapWrapper::Pop(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyKeyObj = nullptr;
	PyObject* PyDefaultObj = nullptr;

	static const char* ArgsKwdList[] = { "Key", "Default", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:Pop", (char**)ArgsKwdList, &PyKeyObj, &PyDefaultObj))
	{
		return nullptr;
	}

	return DoGetItem(InSelf, PyKeyObj, PyDefaultObj, true);
}

PyObject* FNePyMapWrapper::PopItem(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	const int32 SelfElementCount = SelfScriptMapHelper.Num();

	if (SelfElementCount == 0)
	{
		PyErr_SetString(PyExc_KeyError, "Cannot pop from an empty map");
		return nullptr;
	}

	for (int32 SelfSparseIndex = 0; ; ++SelfSparseIndex)
	{
		if (!SelfScriptMapHelper.IsValidIndex(SelfSparseIndex))
		{
			continue;
		}

		FNePyObjectPtr PyReturnKey = NePyStealReference(NePyBase::TryConvertFPropertyToPyObjectDirectNoDependency(SelfScriptMapHelper.GetKeyProperty(), SelfScriptMapHelper.GetKeyPtr(SelfSparseIndex), OwnerObject));
		if (!PyReturnKey)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetKeyProperty());
			return nullptr;
		}

		FNePyObjectPtr PyReturnValue = NePyStealReference(NePyBase::TryConvertFPropertyToPyObjectDirectNoDependency(SelfScriptMapHelper.GetValueProperty(), SelfScriptMapHelper.GetValuePtr(SelfSparseIndex), OwnerObject));
		if (!PyReturnValue)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetValueProperty());
			return nullptr;
		}

		SelfScriptMapHelper.RemoveAt(SelfSparseIndex);

		return PyTuple_Pack(2, PyReturnKey.Get(), PyReturnValue.Get());
	}
}

PyObject* FNePyMapWrapper::SetDefault(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyKeyObj = nullptr;
	PyObject* PyDefaultObj = nullptr;

	static const char* ArgsKwdList[] = { "Key", "Default", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:SetDefault", (char**)ArgsKwdList, &PyKeyObj, &PyDefaultObj))
	{
		return nullptr;
	}

	FNePyPropValue MapKey(InSelf->Prop->KeyProp);
	if (!MapKey.SetValue(PyKeyObj))
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);

	const void* ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.Value);
	if (!ValuePtr)
	{
		FNePyPropValue MapValue(InSelf->Prop->ValueProp);
		if (PyDefaultObj && !MapValue.SetValue(PyDefaultObj))
		{
			return nullptr;
		}

		SelfScriptMapHelper.AddPair(MapKey.Value, MapValue.Value);

		ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.Value);
		check(ValuePtr);
	}

	PyObject* PyItemObj = NePyBase::TryConvertFPropertyToPyObjectDirectNoDependency(SelfScriptMapHelper.GetValueProperty(), ValuePtr, OwnerObject);
	if (!PyItemObj)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetValueProperty());
		return nullptr;
	}
	return PyItemObj;
}

PyObject* FNePyMapWrapper::Update(FNePyMapWrapper* InSelf, PyObject* InOther)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyPropValue Other = FNePyPropValue(InSelf->Prop);
	UObject* OwnerObject = GetOwnerObject(InSelf);
	if (!Assign(InOther, InSelf->Prop, Other.Value, OwnerObject))
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	FScriptMapHelper OtherScriptMapHelper(InSelf->Prop, Other.Value);

	const int32 OtherSparseCount = OtherScriptMapHelper.Num();
	for (int32 OtherSparseIndex = 0; OtherSparseIndex < OtherSparseCount; ++OtherSparseIndex)
	{
		if (!OtherScriptMapHelper.IsValidIndex(OtherSparseIndex))
		{
			continue;
		}

		SelfScriptMapHelper.AddPair(OtherScriptMapHelper.GetKeyPtr(OtherSparseIndex), OtherScriptMapHelper.GetValuePtr(OtherSparseIndex));
	}

	Py_RETURN_NONE;
}

int FNePyMapWrapper::Contains(FNePyMapWrapper* InSelf, PyObject* InKey)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FNePyPropValue MapKey(InSelf->Prop->KeyProp);
	if (!MapKey.SetValue(InKey))
	{
		PyErr_Clear();
		return 0;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	return SelfScriptMapHelper.FindValueFromHash(MapKey.Value) ? 1 : 0;
}

Py_ssize_t FNePyMapWrapper::Len(FNePyMapWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	return SelfScriptMapHelper.Num();
}

PyObject* FNePyMapWrapper::GetItem(FNePyMapWrapper* InSelf, PyObject* InKey)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return DoGetItem(InSelf, InKey, nullptr);
}

PyObject* FNePyMapWrapper::DoGetItem(FNePyMapWrapper* InSelf, PyObject* InKey, PyObject* InDefault, bool bRemove)
{
	FNePyPropValue MapKey(InSelf->Prop->KeyProp);
	if (!MapKey.SetValue(InKey))
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);

	const void* ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.Value);
	if (!ValuePtr)
	{
		if (!InDefault)
		{
			FString KeyStr = NePyBase::GetFriendlyPropertyValue(MapKey.Prop, MapKey.Value, PPF_Delimited | PPF_IncludeTransient);
			PyErr_Format(PyExc_KeyError, "Key %s was not found in the map", TCHAR_TO_UTF8(*KeyStr));
			return nullptr;
		}

		Py_INCREF(InDefault);
		return InDefault;
	}

	PyObject* PyItemObj = NePyBase::TryConvertFPropertyToPyObjectDirectPyOuter(SelfScriptMapHelper.GetValueProperty(), ValuePtr, InSelf);
	if (!PyItemObj)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(SelfScriptMapHelper.GetValueProperty());
		return nullptr;
	}

	if (bRemove)
	{
		SelfScriptMapHelper.RemovePair(MapKey.Value);
	}

	return PyItemObj;
}

int FNePyMapWrapper::SetItem(FNePyMapWrapper* InSelf, PyObject* InKey, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FNePyPropValue MapKey(InSelf->Prop->KeyProp);
	if (!MapKey.SetValue(InKey))
	{
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->Prop, InSelf->Value);
	if (InValue)
	{
		FNePyPropValue  MapValue(InSelf->Prop->ValueProp);
		if (!MapValue.SetValue(InValue))
		{
			return -1;
		}

		SelfScriptMapHelper.AddPair(MapKey.Value, MapValue.Value);
	}
	else
	{
		SelfScriptMapHelper.RemovePair(MapKey.Value);
	}

	return 0;
}

FNePyStructMapWrapper* FNePyStructMapWrapper::New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InStruct, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	const FMapProperty* MapProp = CastField<FMapProperty>(Prop);
	if (!MapProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not Map property", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	return FNePyStructMapWrapper::New(PyOuter, InMemberPtr, MapProp);
}

FNePyStructMapWrapper* FNePyStructMapWrapper::New(PyObject* PyOuter, void* InMemberPtr, const FMapProperty* InMapProp)
{
	FNePyStructMapWrapper* RetValue = PyObject_New(FNePyStructMapWrapper, &FNePyStructMapWrapperType);
	RetValue->Value = InMemberPtr;
	RetValue->Prop = InMapProp;
	RetValue->PyOuter = PyOuter;
	Py_INCREF(PyOuter);
	return RetValue;
}

void FNePyStructMapWrapper::Dealloc(FNePyStructMapWrapper* InSelf)
{
	if (InSelf->PyOuter)
	{
		Py_DECREF(InSelf->PyOuter);
		InSelf->PyOuter = nullptr;
	}
	Py_TYPE(InSelf)->tp_free(InSelf);
}

FNePyMapWrapperKeyIterator* FNePyMapWrapperKeyIterator::New(FNePyMapWrapper* IterInstance)
{
	FNePyMapWrapperKeyIterator* RetValue = PyObject_New(FNePyMapWrapperKeyIterator, &FNePyMapWrapperKeyIteratorType);
	Py_INCREF(IterInstance);
	RetValue->IterInstance = IterInstance;
	RetValue->IterIndex = GetElementIndex(RetValue, 0);
	return RetValue;
}

void FNePyMapWrapperKeyIterator::Dealloc(FNePyMapWrapperKeyIterator* InSelf)
{
	Py_XDECREF(InSelf->IterInstance);
	InSelf->IterInstance = nullptr;
	InSelf->IterIndex = 0;
	Py_TYPE(InSelf)->tp_free(InSelf);
}

int FNePyMapWrapperKeyIterator::Init(FNePyMapWrapperKeyIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init MapWrapperKeyIterator directly!");
	return -1;
}

PyObject* FNePyMapWrapperKeyIterator::Iter(FNePyMapWrapperKeyIterator* InSelf)
{
	Py_INCREF(InSelf);
	return InSelf;
}

PyObject* FNePyMapWrapperKeyIterator::IterNext(FNePyMapWrapperKeyIterator* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	UObject* OwnerObject = FNePyMapWrapper::GetOwnerObject(InSelf->IterInstance);
	FScriptMapHelper ScriptMapHelper(InSelf->IterInstance->Prop, InSelf->IterInstance->Value);
	const int32 SparseCount = ScriptMapHelper.GetMaxIndex();

	if (InSelf->IterIndex < SparseCount)
	{
		const int32 ElementIndex = InSelf->IterIndex;
		InSelf->IterIndex = GetElementIndex(InSelf, InSelf->IterIndex + 1);

		if (!ScriptMapHelper.IsValidIndex(ElementIndex))
		{
			PyErr_SetString(PyExc_IndexError, "Iterator was on an invalid element index! Was the map changed while iterating?");
			return nullptr;
		}

		PyObject* PyItemObj = NePyBase::TryConvertFPropertyToPyObjectDirectPyOuter(ScriptMapHelper.GetKeyProperty(), ScriptMapHelper.GetKeyPtr(ElementIndex), InSelf);
		if (!PyItemObj)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(ScriptMapHelper.GetKeyProperty());
			return nullptr;
		}
		return PyItemObj;
	}

	PyErr_SetObject(PyExc_StopIteration, Py_None);
	return nullptr;
}

bool FNePyMapWrapperKeyIterator::ValidateInternalState(FNePyMapWrapperKeyIterator* InSelf)
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

int32 FNePyMapWrapperKeyIterator::GetElementIndex(FNePyMapWrapperKeyIterator* InSelf, int32 InSparseIndex)
{
	FScriptMapHelper ScriptMapHelper(InSelf->IterInstance->Prop, InSelf->IterInstance->Value);
	const int32 SparseCount = ScriptMapHelper.GetMaxIndex();

	int32 ElementIndex = InSparseIndex;
	for (; ElementIndex < SparseCount; ++ElementIndex)
	{
		if (ScriptMapHelper.IsValidIndex(ElementIndex))
		{
			break;
		}
	}

	return ElementIndex;
}
