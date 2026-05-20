#include "NePyFixedArrayWrapper.h"
#include "NePyBase.h"
#include "NePyPropertyConvert.h"
#include "Misc/EngineVersionComparison.h"

static PyTypeObject FNePyFixedArrayWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FixedArrayWrapper", /* tp_name */
	sizeof(FNePyFixedArrayWrapper), /* tp_basicsize */
};

static PyTypeObject FNePyStructFixedArrayWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FixedArrayWrapper", /* tp_name */
	sizeof(FNePyStructFixedArrayWrapper), /* tp_basicsize */
};

void FNePyFixedArrayWrapper::InitPyType()
{
#define DUAL_NAME_METHOD(lower, upper, func, flags, doc) \
	{ lower, func, flags, doc }, \
	{ upper, func, flags, doc }

	static PyMethodDef PyMethods[] = {
		DUAL_NAME_METHOD("is_valid", "IsValid", NePyCFunctionCast(&FNePyFixedArrayWrapper::IsValid), METH_NOARGS, "() -> bool"),
		DUAL_NAME_METHOD("copy", "Copy", NePyCFunctionCast(&FNePyFixedArrayWrapper::Copy), METH_NOARGS, "()->list" ),
		DUAL_NAME_METHOD("count", "Count", NePyCFunctionCast(&FNePyFixedArrayWrapper::Count), METH_O, "(Item: typing.Any) -> int" ),
		DUAL_NAME_METHOD("index", "Index", NePyCFunctionCast(&FNePyFixedArrayWrapper::Index), METH_VARARGS | METH_KEYWORDS, "(Value: typing.Any, Start : int = ..., Stop : int = ...) -> int" ),
		{ nullptr, nullptr, 0, nullptr }
	};

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FNePyFixedArrayWrapper::Len;
	PySequence.sq_item = (ssizeargfunc)&FNePyFixedArrayWrapper::GetItem;
	PySequence.sq_ass_item = (ssizeobjargproc)&FNePyFixedArrayWrapper::SetItem;
	PySequence.sq_contains = (objobjproc)&FNePyFixedArrayWrapper::Contains;
	PySequence.sq_concat = (binaryfunc)&FNePyFixedArrayWrapper::Concat;

	{
		PyTypeObject* PyType = &FNePyFixedArrayWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyFixedArrayWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyFixedArrayWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePyFixedArrayWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePyFixedArrayWrapper::Repr;
		PyType->tp_richcompare = (richcmpfunc)&FNePyFixedArrayWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePyStructFixedArrayWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = PyType_GenericNew;
		PyType->tp_dealloc = (destructor)&FNePyStructFixedArrayWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyFixedArrayWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePyFixedArrayWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePyFixedArrayWrapper::Repr;
		PyType->tp_richcompare = (richcmpfunc)&FNePyFixedArrayWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType_Ready(PyType);
	}

#undef DUAL_NAME_METHOD
}

FNePyFixedArrayWrapper* FNePyFixedArrayWrapper::New(UObject* InObject, void* InMemberPtr, const char* PropName)
{
	UClass* Class = InObject->GetClass();

	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(Class, InObject, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' object has no property '%s'", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	return New(InObject, (void*)InMemberPtr, Prop);
}

FNePyFixedArrayWrapper* FNePyFixedArrayWrapper::New(UObject* InObject, void* InMemberPtr, const FProperty* InProp)
{
	check(InMemberPtr == InProp->ContainerPtrToValuePtr<void>(InObject));

	auto Constructor = [InMemberPtr, InProp]() -> FNePyPropObject* {
		FNePyFixedArrayWrapper* RetValue = PyObject_New(FNePyFixedArrayWrapper, &FNePyFixedArrayWrapperType);
		RetValue->Value = InMemberPtr;
		RetValue->Prop = InProp;
		return RetValue;
	};

	PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InMemberPtr, Constructor);
	return (FNePyFixedArrayWrapper*)RetValue;
}

FNePyFixedArrayWrapper* FNePyFixedArrayWrapper::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePyFixedArrayWrapperType || PyType == &FNePyStructFixedArrayWrapperType)
		{

			return (FNePyFixedArrayWrapper*)InPyObj;
		}
	}
	return nullptr;
}

class FNePyScopedArrayDimEraser
{
public:
	FNePyScopedArrayDimEraser(FProperty* InProp)
		: Prop(InProp), OldArrayDim(InProp->ArrayDim)
	{
		Prop->ArrayDim = 1;
	}

	~FNePyScopedArrayDimEraser()
	{
		Prop->ArrayDim = OldArrayDim;
	}

private:
	FProperty* Prop;
	int32 OldArrayDim;
};

bool FNePyFixedArrayWrapper::Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InClass, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	UObject* InOwnerObject = InClass->IsA<UClass>() ? (UObject*)InInstance : nullptr;
	return FNePyFixedArrayWrapper::Assign(InOther, Prop, InMemberPtr, InOwnerObject);
}

bool FNePyFixedArrayWrapper::Assign(PyObject* InOther, const FProperty* InProp, void* InDest, UObject* InOwnerObject)
{
	FNePyObjectPtr PyList;
	if (PyList_Check(InOther) || PyTuple_Check(InOther))
	{
		PyList = NePyNewReference(InOther);
	}
	else if (FNePyFixedArrayWrapper* OtherArray = FNePyFixedArrayWrapper::Check(InOther))
	{
		PyList = NePyStealReference(ToPyList(OtherArray));
		if (!PyList)
		{
			return false;
		}
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "unable to assign '%s' to %s (%s)", InOther->ob_type->tp_name, TCHAR_TO_UTF8(*InProp->GetName()), TCHAR_TO_UTF8(*InProp->GetClass()->GetName()));
		return false;
	}

	const int32 NewElementCount = (int32)PySequence_Size(PyList);
	if (NewElementCount != InProp->ArrayDim)
	{
		PyErr_Format(PyExc_ValueError, "assign list to FixedArray with error length: %d", NewElementCount);
		return false;
	}

	FNePyPropValue TempArray(InProp);
	{
		// 将属性数组大小临时改为1，防止递归
		FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InProp));
		check(InProp->ArrayDim == 1);

		NePyPyObjectToPropertyFunc Converter = NePyGetPyObjectToPropertyConverter(InProp);
		if (!Converter)
		{
			NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InProp);
			return false;
		}

		for (int32 Index = 0; Index < NewElementCount; ++Index)
		{
			PyObject* PyItem = PyList_GetItem(PyList, Index);
			if (!Converter(PyItem, InProp, GetItemPtr(InProp, TempArray.Value, Index), InOwnerObject))
			{
				NePyBase::SetConvertPyObjectToFPropertyError(PyItem, InProp);
				return false;
			}
		}
	}

	InProp->CopyCompleteValue(InDest, TempArray.Value);
	return true;
}

PyObject* FNePyFixedArrayWrapper::ToPyList(FNePyFixedArrayWrapper* InSelf)
{
	return ToPyList(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePyFixedArrayWrapper::ToPyList(const FProperty* InProp, const void* InValue, UObject* InOwnerObject)
{
	const int32 ElementCount = InProp->ArrayDim;
	FNePyObjectPtr PyList = NePyStealReference(PyList_New(ElementCount));
	{
		// 将属性数组大小临时改为1，防止递归
		FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InProp));
		check(InProp->ArrayDim == 1);

		NePyPropertyToPyObjectFunc Converter = NePyGetPropertyToPyObjectConverter(InProp);
		if (!Converter)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(InProp);
			return nullptr;
		}

		for (int32 ArrayIndex = 0; ArrayIndex < ElementCount; ++ArrayIndex)
		{
			PyObject* PyItemObj = Converter(InProp, GetItemPtr(InProp, const_cast<void*>(InValue), ArrayIndex), InOwnerObject);
			if (!PyItemObj)
			{
				NePyBase::SetConvertFPropertyToPyObjectError(InProp);
				return nullptr;
			}
			PyList_SetItem(PyList, ArrayIndex, PyItemObj); // PyList_SetItem steals this reference
		}
	}

	return PyList.Release();
}

UObject* FNePyFixedArrayWrapper::GetOwnerObject(FNePyFixedArrayWrapper* InSelf)
{
	if (!InSelf->Prop || !InSelf->Value)
	{
		return nullptr;
	}

	if (Py_TYPE(InSelf) == &FNePyStructFixedArrayWrapperType)
	{
		return nullptr;
	}

	UObject* Owner = (UObject*)((uint8*)InSelf->Value - InSelf->Prop->GetOffset_ForInternal());
	return Owner;
}

void FNePyFixedArrayWrapper::Dealloc(FNePyFixedArrayWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此ArrayPtr必为空
	check(!InSelf->Value);
	Py_TYPE(InSelf)->tp_free(InSelf);
}

int FNePyFixedArrayWrapper::Init(FNePyFixedArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyErr_SetString(PyExc_RuntimeError, "You can not init FixedArrayWrapper directly!");
	return -1;
}

PyObject* FNePyFixedArrayWrapper::Repr(FNePyFixedArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FString ExportedArray;
	for (int32 ArrayIndex = 0; ArrayIndex < InSelf->Prop->ArrayDim; ++ArrayIndex)
	{
		if (ArrayIndex > 0)
		{
			ExportedArray += TEXT(", ");
		}
		ExportedArray += NePyBase::GetFriendlyPropertyValue(InSelf->Prop, GetItemPtr(InSelf->Prop, InSelf->Value, ArrayIndex), PPF_Delimited | PPF_IncludeTransient);
	}
	return PyUnicode_FromFormat("[%s]", TCHAR_TO_UTF8(*ExportedArray));
}

PyObject* FNePyFixedArrayWrapper::RichCompare(FNePyFixedArrayWrapper* InSelf, PyObject* InOther, int InOp)
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

	if (PyList_Check(InOther))
	{
		PyObject* This = ToPyList(InSelf);
		if (!This)
		{
			return nullptr;
		}
		return Py_TYPE(This)->tp_richcompare(This, InOther, InOp);
	}

	if (FNePyFixedArrayWrapper* OtherArray = Check(InOther))
	{
		bool bIsIdentical = InSelf->Prop->SameType(OtherArray->Prop);
		if (bIsIdentical)
		{
			bIsIdentical = InSelf->Prop->Identical(InSelf->Value, OtherArray->Value, PPF_None);
		}
		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

PyObject* FNePyFixedArrayWrapper::IsValid(FNePyFixedArrayWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyFixedArrayWrapper::Copy(FNePyFixedArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return FNePyFixedArrayWrapper::ToPyList(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

void* FNePyFixedArrayWrapper::GetItemPtr(const FProperty* InProp, void* InValue, Py_ssize_t InIndex)
{
	// This doesn't use ContainerPtrToValuePtr as the ArrayInstance has already been adjusted to 
	// point to the first element in the array and we just need to adjust it for the element size
#if UE_VERSION_OLDER_THAN(5,5,0)
	return static_cast<uint8*>(InValue) + (InProp->ElementSize * InIndex);
#else
	return static_cast<uint8*>(InValue) + (InProp->GetElementSize() * InIndex);
#endif
}

PyObject* FNePyFixedArrayWrapper::Count(FNePyFixedArrayWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	int32 ValueCount = 0;
	{
		// 将属性数组大小临时改为1，防止递归
		FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InSelf->Prop));
		check(InSelf->Prop->ArrayDim == 1);

		FNePyPropValue PropValue(InSelf->Prop);
		if (!PropValue.SetValue(InValue))
		{
			return nullptr;
		}

		for (int32 ArrayIndex = 0; ArrayIndex < InSelf->Prop->ArrayDim; ++ArrayIndex)
		{
			if (PropValue.Prop->Identical(GetItemPtr(InSelf->Prop, InSelf->Value, ArrayIndex), PropValue.Value, 0))
			{
				++ValueCount;
			}
		}
	}

	PyObject* PyRet;
	NePyBase::ToPy(ValueCount, PyRet);
	return PyRet;
}

PyObject* FNePyFixedArrayWrapper::Index(FNePyFixedArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyValueObj = nullptr;
	PyObject* PyStartObj = nullptr;
	PyObject* PyStopObj = nullptr;

	static const char* ArgsKwdList[] = { "Value", "Start", "Stop", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OO:Index", (char**)ArgsKwdList, &PyValueObj, &PyStartObj, &PyStopObj))
	{
		return nullptr;
	}

	int32 StartIndex = 0;
	if (PyStartObj && !NePyBase::ToCpp(PyStartObj, StartIndex))
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'start' must have type 'int'");
		return nullptr;
	}

	int32 StopIndex = -1;
	if (PyStopObj && !NePyBase::ToCpp(PyStopObj, StopIndex))
	{
		PyErr_SetString(PyExc_TypeError, "arg3 'stop' must have type 'int'");
		return nullptr;
	}

	const int32 ElementCount = InSelf->Prop->ArrayDim;
	const Py_ssize_t ResolvedStartIndex = NePyBase::ResolveContainerIndexParam(StartIndex, ElementCount);
	const Py_ssize_t ResolvedStopIndex = NePyBase::ResolveContainerIndexParam(StopIndex, ElementCount);

	StartIndex = FMath::Min((int32)ResolvedStartIndex, ElementCount);
	StopIndex = FMath::Max((int32)ResolvedStopIndex, ElementCount);

	int32 ReturnIndex = INDEX_NONE;
	{
		// 将属性数组大小临时改为1，防止递归
		FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InSelf->Prop));
		check(InSelf->Prop->ArrayDim == 1);

		FNePyPropValue PropValue(InSelf->Prop);
		if (!PropValue.SetValue(PyValueObj))
		{
			return nullptr;
		}

		for (int32 ElementIndex = StartIndex; ElementIndex < StopIndex; ++ElementIndex)
		{
			if (PropValue.Prop->Identical(GetItemPtr(InSelf->Prop, InSelf->Value, ElementIndex), PropValue.Value, 0))
			{
				ReturnIndex = ElementIndex;
				break;
			}
		}

		if (ReturnIndex == INDEX_NONE)
		{
			FString PropValueStr = NePyBase::GetFriendlyPropertyValue(PropValue.Prop, PropValue.Value, PPF_Delimited | PPF_IncludeTransient);
			PyErr_Format(PyExc_ValueError, "%s value was not found in the array", TCHAR_TO_UTF8(*PropValueStr));
			return nullptr;
		}
	}

	PyObject* PyRet;
	NePyBase::ToPy(ReturnIndex, PyRet);
	return PyRet;
}

Py_ssize_t FNePyFixedArrayWrapper::Len(FNePyFixedArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	return InSelf->Prop->ArrayDim;
}

PyObject* FNePyFixedArrayWrapper::GetItem(FNePyFixedArrayWrapper* InSelf, Py_ssize_t InIndex)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(InIndex, InSelf->Prop->ArrayDim);
	if (NePyBase::ValidateContainerIndexParam(ResolvedIndex, InSelf->Prop->ArrayDim, InSelf->Prop) != 0)
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	PyObject* PyItemObj = nullptr;
	{
		// 将属性数组大小临时改为1，防止递归
		FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InSelf->Prop));
		check(InSelf->Prop->ArrayDim == 1);
		auto Converter = NePyGetPropertyToPyObjectConverter(InSelf->Prop);
		PyItemObj = Converter(InSelf->Prop, GetItemPtr(InSelf->Prop, InSelf->Value, ResolvedIndex), OwnerObject);
	}

	if (!PyItemObj)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop);
		return nullptr;
	}
	return PyItemObj;
}

int FNePyFixedArrayWrapper::SetItem(FNePyFixedArrayWrapper* InSelf, Py_ssize_t InIndex, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(InIndex, InSelf->Prop->ArrayDim);
	const int ValidateIndexResult = NePyBase::ValidateContainerIndexParam(ResolvedIndex, InSelf->Prop->ArrayDim, InSelf->Prop);
	if (ValidateIndexResult != 0)
	{
		return ValidateIndexResult;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	bool bResult = false;
	{
		// 将属性数组大小临时改为1，防止递归
		FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InSelf->Prop));
		check(InSelf->Prop->ArrayDim == 1);

		auto Converter = NePyGetPyObjectToPropertyConverter(InSelf->Prop);
		bResult = Converter(InValue, InSelf->Prop, GetItemPtr(InSelf->Prop, InSelf->Value, ResolvedIndex), OwnerObject);
	}

	if (!bResult)
	{
		NePyBase::SetConvertPyObjectToFPropertyError(InValue, InSelf->Prop);
		return -1;
	}
	return 0;
}

int FNePyFixedArrayWrapper::Contains(FNePyFixedArrayWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	// 将属性数组大小临时改为1，防止递归
	FNePyScopedArrayDimEraser ArrayDimEraser(const_cast<FProperty*>(InSelf->Prop));
	check(InSelf->Prop->ArrayDim == 1);

	FNePyPropValue ContainerEntryValue(InSelf->Prop);
	if (!ContainerEntryValue.SetValue(InValue))
	{
		return -1;
	}

	for (int32 ArrIndex = 0; ArrIndex < InSelf->Prop->ArrayDim; ++ArrIndex)
	{
		if (ContainerEntryValue.Prop->Identical(GetItemPtr(InSelf->Prop, InSelf->Value, ArrIndex), ContainerEntryValue.Value))
		{
			return 1;
		}
	}

	return 0;
}

PyObject* FNePyFixedArrayWrapper::Concat(FNePyFixedArrayWrapper* InSelf, PyObject* InOther)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyObjectPtr PyList = NePyStealReference(FNePyFixedArrayWrapper::Copy(InSelf));
	if (!PyList)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop);
		return nullptr;
	}

	PyObject* RetValue = PySequence_InPlaceConcat(PyList, InOther);
	if (!RetValue)
	{
		PyErr_Format(PyExc_TypeError, "unable to concat FixedArrayWrapper and %s", InOther->ob_type->tp_name);
		return nullptr;
	}

	return RetValue;
}

FNePyStructFixedArrayWrapper* FNePyStructFixedArrayWrapper::New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InStruct, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	return FNePyStructFixedArrayWrapper::New(PyOuter, InMemberPtr, Prop);
}

FNePyStructFixedArrayWrapper* FNePyStructFixedArrayWrapper::New(PyObject* PyOuter, void* InMemberPtr, const FProperty* InProp)
{
	FNePyStructFixedArrayWrapper* RetValue = PyObject_New(FNePyStructFixedArrayWrapper, &FNePyStructFixedArrayWrapperType);
	RetValue->Value = InMemberPtr;
	RetValue->Prop = InProp;
	RetValue->PyOuter = PyOuter;
	Py_INCREF(PyOuter);
	return RetValue;
}

void FNePyStructFixedArrayWrapper::Dealloc(FNePyStructFixedArrayWrapper* InSelf)
{
	if (InSelf->PyOuter)
	{
		Py_DECREF(InSelf->PyOuter);
		InSelf->PyOuter = nullptr;
	}
	Py_TYPE(InSelf)->tp_free(InSelf);
}
