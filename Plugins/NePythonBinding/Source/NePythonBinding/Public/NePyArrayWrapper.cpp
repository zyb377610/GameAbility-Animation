#include "NePyArrayWrapper.h"
#include "NePyBase.h"
#include "NePyPropertyConvert.h"
#include "NePyMemoryAllocator.h"
#include "NePySubclassing.h"

#if PY_MAJOR_VERSION >= 3
#define NEPY_CAST_SLICE(X) X
#else
#define NEPY_CAST_SLICE(X) ((PySliceObject*)X)
#endif

static PyTypeObject FNePyArrayWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ArrayWrapper", /* tp_name */
	sizeof(FNePyArrayWrapper), /* tp_basicsize */
};

static PyTypeObject FNePyStructArrayWrapperType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ArrayWrapper", /* tp_name */
	sizeof(FNePyStructArrayWrapper), /* tp_basicsize */
};

void FNePyArrayWrapper::InitPyType(PyObject* PyOuterModule)
{
#define DUAL_NAME_METHOD(lower, upper, func, flags, doc) \
	{ lower, func, flags, doc }, \
	{ upper, func, flags, doc }

	static PyMethodDef PyMethods[] = {
		DUAL_NAME_METHOD("is_valid", "IsValid", NePyCFunctionCast(&FNePyArrayWrapper::IsValid), METH_NOARGS, "() -> bool"),
		DUAL_NAME_METHOD("copy", "Copy", NePyCFunctionCast(&FNePyArrayWrapper::Copy), METH_NOARGS, "() -> list"),
		DUAL_NAME_METHOD("append", "Append", NePyCFunctionCast(&FNePyArrayWrapper::Append), METH_O, "(Item: typing.Any) -> None"),
		DUAL_NAME_METHOD("count", "Count", NePyCFunctionCast(&FNePyArrayWrapper::Count), METH_O, "(Item: typing.Any) -> int"),
		DUAL_NAME_METHOD("extend", "Extend", NePyCFunctionCast(&FNePyArrayWrapper::Extend), METH_O, "(Values: list[typing.Any]) -> None"),
		DUAL_NAME_METHOD("index", "Index", NePyCFunctionCast(&FNePyArrayWrapper::Index), METH_VARARGS | METH_KEYWORDS, "(Value: typing.Any, Start : int = ..., Stop : int = ...) -> int"),
		DUAL_NAME_METHOD("insert", "Insert", NePyCFunctionCast(&FNePyArrayWrapper::Insert), METH_VARARGS | METH_KEYWORDS, "(Index: int, Item : typing.Any)->None"),
		DUAL_NAME_METHOD("pop", "Pop", NePyCFunctionCast(&FNePyArrayWrapper::Pop), METH_VARARGS, "(Index: int = ...)->typing.Any"),
		DUAL_NAME_METHOD("remove", "Remove", NePyCFunctionCast(&FNePyArrayWrapper::Remove), METH_O, "(Item: typing.Any)->None"),
		DUAL_NAME_METHOD("reverse","Reverse", NePyCFunctionCast(&FNePyArrayWrapper::Reverse), METH_NOARGS, "() -> None"),
#if PY_MAJOR_VERSION < 3
		DUAL_NAME_METHOD("sort", "Sort", NePyCFunctionCast(&FNePyArrayWrapper::Sort), METH_VARARGS | METH_KEYWORDS, "(Cmp: typing.Callable = ..., Key : typing.Callable = ..., bReverse : bool = ...)->None"),
#else	// PY_MAJOR_VERSION < 3
		DUAL_NAME_METHOD("sort", "Sort", NePyCFunctionCast(&FNePyArrayWrapper::Sort), METH_VARARGS | METH_KEYWORDS, "(Key: typing.Callable = ..., bReverse: bool = ...) -> None"),
#endif	// PY_MAJOR_VERSION < 3
		DUAL_NAME_METHOD("clear", "Clear", NePyCFunctionCast(&FNePyArrayWrapper::Clear), METH_NOARGS, "() -> None"),
		NEPY_TRACKED_OBJECT_METHODS
		{ nullptr, nullptr, 0, nullptr }
	};

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FNePyArrayWrapper::Len;
	PySequence.sq_item = (ssizeargfunc)&FNePyArrayWrapper::GetItem;
	PySequence.sq_ass_item = (ssizeobjargproc)&FNePyArrayWrapper::SetItem;
	PySequence.sq_contains = (objobjproc)&FNePyArrayWrapper::Contains;
	PySequence.sq_concat = (binaryfunc)&FNePyArrayWrapper::Concat;
	PySequence.sq_inplace_concat = (binaryfunc)&FNePyArrayWrapper::ConcatInplace;
	PySequence.sq_repeat = (ssizeargfunc)&FNePyArrayWrapper::Repeat;
	PySequence.sq_inplace_repeat = (ssizeargfunc)&FNePyArrayWrapper::RepeatInplace;

	static PyMappingMethods PyMapping;
	PyMapping.mp_length = (lenfunc)FNePyArrayWrapper::Len;
	PyMapping.mp_subscript = (binaryfunc)FNePyArrayWrapper::GetSubscript;
	PyMapping.mp_ass_subscript = (objobjargproc)FNePyArrayWrapper::SetSubscript;

	{
		PyTypeObject* PyType = &FNePyArrayWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = (newfunc)FNePyArrayWrapper::NewIntenral;
		PyType->tp_dealloc = (destructor)&FNePyArrayWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyArrayWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePyArrayWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePyArrayWrapper::Repr;
		PyType->tp_richcompare = (richcmpfunc)&FNePyArrayWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType->tp_as_mapping = &PyMapping;
		PyType_Ready(PyType);
	}

	{
		PyTypeObject* PyType = &FNePyStructArrayWrapperType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_new = (newfunc)FNePyArrayWrapper::NewIntenral;
		PyType->tp_dealloc = (destructor)&FNePyStructArrayWrapper::Dealloc;
		PyType->tp_init = (initproc)&FNePyArrayWrapper::Init;
		PyType->tp_repr = (reprfunc)&FNePyArrayWrapper::Repr;
		PyType->tp_str = (reprfunc)&FNePyArrayWrapper::Repr;
		PyType->tp_richcompare = (richcmpfunc)&FNePyArrayWrapper::RichCompare;
		PyType->tp_methods = PyMethods;
		PyType->tp_as_sequence = &PySequence;
		PyType->tp_as_mapping = &PyMapping;
		PyType_Ready(PyType);
	}

	Py_INCREF(&FNePyArrayWrapperType);
	PyModule_AddObject(PyOuterModule, "ArrayWrapper", (PyObject*)&FNePyArrayWrapperType);

#undef DUAL_NAME_METHOD
}

FNePyArrayWrapper* FNePyArrayWrapper::NewIntenral(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyArrayWrapper* Self = (FNePyArrayWrapper*)InType->tp_alloc(InType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
	Self->Value = nullptr;
	Self->Prop = nullptr;
	Self->IsObjectProperty = false;
	Self->IsSelfCreateValue = false;
	NEPY_RECORD_OBJECT_CREATION(static_cast<FNePyTrackedObject*>(Self), "ArrayWrapper");
	return Self;
}

FNePyArrayWrapper* FNePyArrayWrapper::New(UObject* InObject, void* InMemberPtr, const char* PropName)
{
	UClass* Class = InObject->GetClass();

	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(Class, InObject, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' object has no property '%s'", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
	if (!ArrayProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not array property", TCHAR_TO_UTF8(*Class->GetName()), PropName);
		return nullptr;
	}

	return New(InObject, InMemberPtr, ArrayProp);
}

FNePyArrayWrapper* FNePyArrayWrapper::New(UObject* InObject, void* InMemberPtr, const FArrayProperty* InArrayProp)
{
	check(InMemberPtr == InArrayProp->ContainerPtrToValuePtr<void>(InObject));

	auto Constructor = [InMemberPtr, InArrayProp]() -> FNePyPropObject* {
		FNePyArrayWrapper* RetValue = PyObject_New(FNePyArrayWrapper, &FNePyArrayWrapperType);
		RetValue->Value = InMemberPtr;
		RetValue->Prop = InArrayProp;
		RetValue->IsObjectProperty = true;
		RetValue->IsSelfCreateValue = false;
		NEPY_RECORD_OBJECT_CREATION(static_cast<FNePyTrackedObject*>(RetValue), "ArrayWrapper(UObject's Member)");
		return RetValue;
	};

	FNePyArrayWrapper* RetValue = (FNePyArrayWrapper*)FNePyHouseKeeper::Get().NewNePyObjectMember(InObject, InMemberPtr, Constructor);
	return (FNePyArrayWrapper*)RetValue;
}

FNePyArrayWrapper* FNePyArrayWrapper::NewReference(void* InMemberPtr, const FArrayProperty* InArrayProp)
{
	check(InMemberPtr);

	FNePyArrayWrapper* RetValue = PyObject_New(FNePyArrayWrapper, &FNePyArrayWrapperType);
	RetValue->Prop = InArrayProp;
	RetValue->Value = InMemberPtr;
	RetValue->IsObjectProperty = false;
	RetValue->IsSelfCreateValue = false;
	NEPY_RECORD_OBJECT_CREATION(static_cast<FNePyTrackedObject*>(RetValue), "ArrayWrapper(Ref)");

	return RetValue;
}

FNePyArrayWrapper* FNePyArrayWrapper::NewCopy(void* InMemberPtr, const FArrayProperty* InArrayProp)
{
	check(InMemberPtr);

	FNePyArrayWrapper* RetValue = PyObject_New(FNePyArrayWrapper, &FNePyArrayWrapperType);
	RetValue->Prop = InArrayProp;
	RetValue->Value = new TScriptArray<FHeapAllocator>;
	RetValue->IsObjectProperty = false;
	RetValue->IsSelfCreateValue = true;
	NEPY_RECORD_OBJECT_CREATION(static_cast<FNePyTrackedObject*>(RetValue), "ArrayWrapper(Copy)");

	FScriptArrayHelper ArraySelf(InArrayProp, RetValue->Value);
	FScriptArrayHelper ArrayOther(InArrayProp, InMemberPtr);

	const int32 SizeSelf = ArraySelf.Num();
	const int32 SizeOther = ArrayOther.Num();

	ArraySelf.Resize(SizeOther);

	for (int32 Index = 0; Index < SizeOther; ++Index)
	{
		void* Src = ArrayOther.GetRawPtr(Index);
		void* Dest = ArraySelf.GetRawPtr(Index);
		InArrayProp->Inner->InitializeValue(Dest);
		InArrayProp->Inner->CopyCompleteValue(Dest, Src);
	}

	return RetValue;
}

FNePyArrayWrapper* FNePyArrayWrapper::NewImplicit(const FArrayProperty* InProp)
{
	FNePyArrayWrapper* RetValue = PyObject_New(FNePyArrayWrapper, &FNePyArrayWrapperType);
	RetValue->Prop = InProp;
	RetValue->Value = new TScriptArray<FHeapAllocator>;
	RetValue->IsObjectProperty = false;
	RetValue->IsSelfCreateValue = true;
	NEPY_RECORD_OBJECT_CREATION(static_cast<FNePyTrackedObject*>(RetValue), "ArrayWrapper(Implicit)");

	return RetValue;
}

FNePyArrayWrapper* FNePyArrayWrapper::Check(PyObject* InPyObj)
{
	if (InPyObj)
	{
		PyTypeObject* PyType = Py_TYPE(InPyObj);
		if (PyType == &FNePyArrayWrapperType || PyType == &FNePyStructArrayWrapperType)
		{
			return (FNePyArrayWrapper*)InPyObj;
		}
	}
	return nullptr;
}

bool FNePyArrayWrapper::Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InClass, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
	if (!ArrayProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not array property", TCHAR_TO_UTF8(*InClass->GetName()), PropName);
		return false;
	}

	UObject* InOwnerObject = InClass->IsA<UClass>() ? (UObject*)InInstance : nullptr;
	return FNePyArrayWrapper::Assign(InOther, ArrayProp, InMemberPtr, InOwnerObject);
}

bool FNePyArrayWrapper::Assign(PyObject* InOther, const FArrayProperty* InProp, void* InDest, UObject* InOwnerObject)
{
	FNePyObjectPtr PyList;
	if (PyList_Check(InOther) || PyTuple_Check(InOther))
	{
		PyList = NePyNewReference(InOther);
	}
	else if (FNePyArrayWrapper* OtherArray = FNePyArrayWrapper::Check(InOther))
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

	NePyPyObjectToPropertyFunc Converter = NePyGetPyObjectToPropertyConverter(InProp->Inner);
	if (!Converter)
	{
		NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InProp->Inner);
		return false;
	}

	FNePyPropValue TempArray(InProp);
	FScriptArrayHelper ArrayHelper(InProp, TempArray.Value);

	const int32 NewElementCount = (int32)PySequence_Size(PyList);
	if (NewElementCount > 0)
	{
		ArrayHelper.AddValues(NewElementCount);
		for (int32 Index = 0; Index < NewElementCount; ++Index)
		{
			PyObject* PyItem = PyList_GetItem(PyList, Index);
			if (!Converter(PyItem, InProp->Inner, ArrayHelper.GetRawPtr(Index), InOwnerObject))
			{
				NePyBase::SetConvertPyObjectToFPropertyError(PyItem, InProp->Inner);
				return false;
			}
		}
	}

	InProp->CopyCompleteValue(InDest, TempArray.Value);
	return true;
}

PyObject* FNePyArrayWrapper::ToPyList(FNePyArrayWrapper* InSelf)
{
	return ToPyList(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePyArrayWrapper::ToPyList(const FArrayProperty* InProp, const void* InValue, UObject* InOwnerObject)
{
	NePyPropertyToPyObjectFunc InnerConverter = NePyGetPropertyToPyObjectConverterNoDependency(InProp->Inner);
	if (!InnerConverter)
	{
		return nullptr;
	}

	FScriptArrayHelper ArrayHelper(InProp, InValue);
	PyObject* PyListObj = PyList_New(ArrayHelper.Num());
	for (int32 Index = 0; Index < ArrayHelper.Num(); Index++)
	{
		PyObject* PyItem = InnerConverter(InProp->Inner, ArrayHelper.GetRawPtr(Index), InOwnerObject);
		if (!PyItem)
		{
			Py_DECREF(PyListObj);
			return nullptr;
		}
		PyList_SetItem(PyListObj, Index, PyItem); // PyList_SteItem will steal reference
	}
	return PyListObj;
}

FNePyArrayWrapper* FNePyArrayWrapper::FromPyIterable(const FArrayProperty* InProp, PyObject* Object)
{
	// 获取迭代器
	PyObject* Iterator = PyObject_GetIter(Object);
	if (!Iterator)
	{
		UE_LOG(LogNePython, Error, TEXT("Object is not iterable!"));
		return nullptr;
	}

	// 如果是 range 对象，可以预先知道长度
	Py_ssize_t Length = -1;
	if (PyRange_Check(Object))
	{
		Length = PySequence_Size(Object);
	}
	else if (PySequence_Check(Object))
	{
		Length = PySequence_Size(Object);
	}

	NePyPyObjectToPropertyFunc Converter = NePyGetPyObjectToPropertyConverter(InProp->Inner);
	if (!Converter)
	{
		Py_DECREF(Iterator);
		NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InProp->Inner);
		return nullptr;
	}

	FNePyArrayWrapper* RetVal = FNePyArrayWrapper::NewImplicit(InProp);
	FScriptArrayHelper ArrayHelper(RetVal->Prop, RetVal->Value);

	// 如果知道长度，预分配空间
	if (Length > 0)
	{
		ArrayHelper.Resize(Length);
	}

	FNePyPropValue PropValue(InProp->Inner);
	PyObject* Item;
	int32 Index = 0;

	// 迭代处理每个元素
	while (true)
	{
		Item = PyIter_Next(Iterator);
		if (!Item)
		{
			break;
		}
		// 如果不知道长度，动态扩展
		if (Length < 0)
		{
			ArrayHelper.Resize(Index + 1);
		}

		if (!PropValue.SetValue(Item))
		{
			Py_DECREF(Item);
			Py_DECREF(Iterator);
			Py_XDECREF(RetVal);
			UE_LOG(LogNePython, Warning, TEXT("Can't convert item %d to property type %s"), Index, *InProp->Inner->GetClass()->GetName());
			return nullptr;
		}

		if (!Converter(Item, InProp->Inner, ArrayHelper.GetRawPtr(Index), nullptr))
		{
			NePyBase::SetConvertPyObjectToFPropertyError(Item, InProp->Inner);
			Py_DECREF(Item);
			Py_DECREF(Iterator);
			Py_XDECREF(RetVal);
			return nullptr;
		}

		Py_DECREF(Item);
		Index++;
	}

	Py_DECREF(Iterator);

	// 检查迭代是否因为错误而结束
	if (PyErr_Occurred())
	{
		Py_XDECREF(RetVal);
		return nullptr;
	}

	// 如果实际长度小于预分配长度，调整大小
	if (Length > 0 && Index != Length)
	{
		ArrayHelper.Resize(Index);
	}

	return RetVal;
}

void FNePyArrayWrapper::ClenupValue(FNePyArrayWrapper* InSelf)
{
	if (InSelf->IsSelfCreateValue)
	{
		delete (TScriptArray<FHeapAllocator>*)InSelf->Value;
		InSelf->Value = nullptr;
		InSelf->IsSelfCreateValue = false;
	}
}

UObject* FNePyArrayWrapper::GetOwnerObject(FNePyArrayWrapper* InSelf)
{
	if (!InSelf->Prop || !InSelf->Value)
	{
		return nullptr;
	}

	if (InSelf->IsSelfCreateValue)
	{
		return nullptr;
	}

	if (Py_TYPE(InSelf) == &FNePyStructArrayWrapperType)
	{
		return nullptr;
	}

	UObject* Owner = (UObject*)((uint8*)InSelf->Value - InSelf->Prop->GetOffset_ForInternal());
	return Owner;
}

void FNePyArrayWrapper::Dealloc(FNePyArrayWrapper* InSelf)
{
	// 走到这里，说明UObject已被GC，FNePyHouseKeeper::InvalidateTracker已被调用
	// 因此ArrayPtr必为空
	if (InSelf->IsObjectProperty)
	{
		check(!InSelf->Value);
	}
	FNePyArrayWrapper::ClenupValue(InSelf);
	Py_TYPE(InSelf)->tp_free(InSelf);
}

int FNePyArrayWrapper::Init(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	int ArgCount = PyTuple_Size(InArgs);
	if (ArgCount < 1 || ArgCount > 2)
	{
		PyErr_Format(PyExc_TypeError, "ArrayWrapper() takes 1 or 2 positional arguments but %d were given", ArgCount);
		return -1;
	}

	PyObject* PyInner = PyTuple_GetItem(InArgs, 0);
	PyObject* PyInitialData = nullptr;

	if (ArgCount == 2)
	{
		PyInitialData = PyTuple_GetItem(InArgs, 1);
	}

	if (!PyType_Check(PyInner))
	{
		PyErr_Format(PyExc_TypeError, "ArrayWrapper() first argument must be a type, but got '%s'", PyInner->ob_type->tp_name);
		return -1;
	}

	auto Constructor = [PyInner]() -> FArrayProperty* {
		FArrayProperty* ArrayProp = new FArrayProperty(nullptr, "ArrayWrapper", RF_NoFlags);

		FProperty* InnerProp = NePySubclassingNewProperty(ArrayProp,
			TEXT("ArrayWrapperInner"),
			PyInner,
			TEXT("NULL"),
			CPF_PropagateToArrayInner,
			nullptr,
			nullptr);

		if (!InnerProp)
		{
			UE_LOG(LogNePython, Error, TEXT("Can't create Inner Property for ArrayWrapper!"));
			// FArrayProperty的析构函数里没有判空……
			ArrayProp->Inner = new FProperty(ArrayProp, NAME_None, RF_NoFlags);
			delete ArrayProp;
			return nullptr;
		}

		if (InnerProp->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
		{
			ArrayProp->SetPropertyFlags(CPF_ContainsInstancedReference);
		}
		ArrayProp->Inner = InnerProp;

		return ArrayProp;
	};

	InSelf->Prop = FNePyHouseKeeper::Get().NewArrayPropertyByPyTypeObject((PyTypeObject*)PyInner, Constructor);
	if (!InSelf->Prop)
	{
		PyErr_Format(PyExc_RuntimeError, "Cannot create ArrayWrapper for type '%s'", ((PyTypeObject*)PyInner)->tp_name);
		return -1;
	}

	InSelf->Value = new TScriptArray<FHeapAllocator>;
	InSelf->IsObjectProperty = false;
	InSelf->IsSelfCreateValue = true;

	if (PyInitialData)
	{
		if (!PySequence_Check(PyInitialData))
		{
			PyErr_Format(PyExc_TypeError, "ArrayWrapper() second argument must be a sequence (list or tuple), but got '%s'", PyInitialData->ob_type->tp_name);
			return -1;
		}

		Py_ssize_t SeqLength = PySequence_Length(PyInitialData);
		if (SeqLength < 0)
		{
			PyErr_SetString(PyExc_RuntimeError, "Failed to get sequence length");
			return -1;
		}

		if (SeqLength > 0)
		{
			FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
			ArrayHelper.Resize(SeqLength);

			for (Py_ssize_t ArrayIndex = 0; ArrayIndex < SeqLength; ++ArrayIndex)
			{
				FNePyObjectPtr Item = NePyStealReference(PySequence_GetItem(PyInitialData, ArrayIndex));
				if (!Item)
				{
					PyErr_Format(PyExc_RuntimeError, "Failed to get item at index %d from initial data", (int)ArrayIndex);
					return -1;
				}
				if (!NePyBase::TryConvertPyObjectToFPropertyDirect(Item, InSelf->Prop->Inner, ArrayHelper.GetRawPtr(ArrayIndex), GetOwnerObject(InSelf)))
				{
					NePyBase::SetConvertPyObjectToFPropertyError(Item, InSelf->Prop->Inner);
					return -1;
				}
			}
		}
	}

	return 0;
}

PyObject* FNePyArrayWrapper::Repr(FNePyArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	FString ExportedArray;
	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		if (ElementIndex > 0)
		{
			ExportedArray += TEXT(", ");
		}
		const FProperty* InnerProp = InSelf->Prop->Inner;
		void* InnerValue = ArrayHelper.GetRawPtr(ElementIndex);
		FString PropValueStr = NePyBase::GetFriendlyPropertyValue(InnerProp, InnerValue, PPF_Delimited | PPF_IncludeTransient);
		ExportedArray += PropValueStr;
	}
	return PyUnicode_FromFormat("[%s]", TCHAR_TO_UTF8(*ExportedArray));
}

PyObject* FNePyArrayWrapper::RichCompare(FNePyArrayWrapper* InSelf, PyObject* InOther, int InOp)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	// 所有操作符：Py_LT, Py_LE, Py_EQ, Py_NE, Py_GT, Py_GE
	if (PyList_Check(InOther))
	{
		PyObject* This = ToPyList(InSelf);
		if (!This)
		{
			return nullptr;
		}
		PyObject* result = Py_TYPE(This)->tp_richcompare(This, InOther, InOp);
		Py_DECREF(This);
		return result;
	}

	if (FNePyArrayWrapper* OtherArray = Check(InOther))
	{
		bool bIsTypeSame = InSelf->Prop->SameType(OtherArray->Prop);
		if (bIsTypeSame)
		{
			return CompareArrays(InSelf, OtherArray, InOp);
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("ArrayWrapper rich compare: type mismatch between '%s' and '%s'"), *InSelf->Prop->Inner->GetName(), *OtherArray->Prop->Inner->GetName());
			Py_INCREF(Py_NotImplemented);
			return Py_NotImplemented;
		}
	}

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

PyObject* FNePyArrayWrapper::CompareArrays(FNePyArrayWrapper* InSelf, FNePyArrayWrapper* InOther, int InOp)
{
	FScriptArrayHelper SelfArrayHelper(InSelf->Prop, InSelf->Value);
	FScriptArrayHelper OtherArrayHelper(InOther->Prop, InOther->Value);
	int32 SelfLength = SelfArrayHelper.Num();
	int32 OtherLength = OtherArrayHelper.Num();

	// 处理相等和不等比较
	if (InOp == Py_EQ)
	{
		if (SelfLength != OtherLength)
		{
			Py_RETURN_FALSE;
		}
		bool bIsIdentical = InSelf->Prop->Identical(InSelf->Value, InOther->Value, PPF_None);
		return PyBool_FromLong(bIsIdentical);
	}
	else if (InOp == Py_NE)
	{
		if (SelfLength != OtherLength)
		{
			Py_RETURN_TRUE;
		}
		bool bIsIdentical = InSelf->Prop->Identical(InSelf->Value, InOther->Value, PPF_None);
		return PyBool_FromLong(!bIsIdentical);
	}

	// 处理所有顺序比较：<, <=, >, >=
	// 使用字典序比较（lexicographic comparison）
	int32 MinLength = FMath::Min(SelfLength, OtherLength);

	for (int32 Index = 0; Index < MinLength; ++Index)
	{
		const void* SelfElement = SelfArrayHelper.GetRawPtr(Index);
		const void* OtherElement = OtherArrayHelper.GetRawPtr(Index);

		// 使用更安全的元素比较
		int32 CmpResult = CompareElements(InSelf->Prop->Inner, SelfElement, OtherElement);

		if (CmpResult != 0)
		{
			// 找到第一个不同的元素，根据操作符返回结果
			switch (InOp)
			{
				case Py_LT: return PyBool_FromLong(CmpResult < 0);
				case Py_LE: return PyBool_FromLong(CmpResult < 0);  // 这里CmpResult != 0，所以<0就是<=
				case Py_GT: return PyBool_FromLong(CmpResult > 0);
				case Py_GE: return PyBool_FromLong(CmpResult > 0);  // 这里CmpResult != 0，所以>0就是>=
			}
		}
	}

	// 所有比较的元素都相等，比较长度
	switch (InOp)
	{
		case Py_LT: return PyBool_FromLong(SelfLength < OtherLength);
		case Py_LE: return PyBool_FromLong(SelfLength <= OtherLength);
		case Py_GT: return PyBool_FromLong(SelfLength > OtherLength);
		case Py_GE: return PyBool_FromLong(SelfLength >= OtherLength);
		default:
			PyErr_SetString(PyExc_SystemError, "Unknown comparison operator");
			return nullptr;
	}
}

int32 FNePyArrayWrapper::CompareElements(FProperty* InnerProp, const void* Element1, const void* Element2)
{
	// 根据属性类型进行类型安全的比较
	if (FIntProperty* IntProp = CastField<FIntProperty>(InnerProp))
	{
		int32 Val1 = IntProp->GetPropertyValue(Element1);
		int32 Val2 = IntProp->GetPropertyValue(Element2);
		return (Val1 < Val2) ? -1 : (Val1 > Val2) ? 1 : 0;
	}
	else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(InnerProp))
	{
		float Val1 = FloatProp->GetPropertyValue(Element1);
		float Val2 = FloatProp->GetPropertyValue(Element2);
		return (Val1 < Val2) ? -1 : (Val1 > Val2) ? 1 : 0;
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(InnerProp))
	{
		double Val1 = DoubleProp->GetPropertyValue(Element1);
		double Val2 = DoubleProp->GetPropertyValue(Element2);
		return (Val1 < Val2) ? -1 : (Val1 > Val2) ? 1 : 0;
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(InnerProp))
	{
		bool Val1 = BoolProp->GetPropertyValue(Element1);
		bool Val2 = BoolProp->GetPropertyValue(Element2);
		return (Val1 < Val2) ? -1 : (Val1 > Val2) ? 1 : 0;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(InnerProp))
	{
		const FString& Str1 = StrProp->GetPropertyValue(Element1);
		const FString& Str2 = StrProp->GetPropertyValue(Element2);
		return Str1.Compare(Str2);
	}
	else
	{
		// 对于复杂类型，回退到内存比较（但这可能不是语义正确的）
		UE_LOG(LogNePython, Warning, TEXT("Using memory comparison for property type: %s"), *InnerProp->GetClass()->GetName());
		return FMemory::Memcmp(Element1, Element2, InnerProp->GetSize());
	}
}

PyObject* FNePyArrayWrapper::IsValid(FNePyArrayWrapper* InSelf)
{
	if (((FNePyPropObject*)InSelf)->IsValid())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* FNePyArrayWrapper::Copy(FNePyArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	return ToPyList(InSelf->Prop, InSelf->Value, GetOwnerObject(InSelf));
}

PyObject* FNePyArrayWrapper::Append(FNePyArrayWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 NewValueIndex = ArrayHelper.AddValue();

	if (!NePyBase::TryConvertPyObjectToFPropertyDirect(InValue, InSelf->Prop->Inner, ArrayHelper.GetRawPtr(NewValueIndex), OwnerObject))
	{
		ArrayHelper.RemoveValues(NewValueIndex);
		NePyBase::SetConvertPyObjectToFPropertyError(InValue, InSelf->Prop->Inner);
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* FNePyArrayWrapper::Count(FNePyArrayWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyPropValue PropValue(InSelf->Prop->Inner);
	if (!PropValue.SetValue(InValue))
	{
		return nullptr;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	int32 ValueCount = 0;
	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		if (PropValue.Prop->Identical(ArrayHelper.GetRawPtr(ElementIndex), PropValue.Value, 0))
		{
			++ValueCount;
		}
	}

	PyObject* PyRet;
	NePyBase::ToPy(ValueCount, PyRet);
	return PyRet;
}

PyObject* FNePyArrayWrapper::Extend(FNePyArrayWrapper* InSelf, PyObject* InValue)
{
	PyObject* Concated = FNePyArrayWrapper::ConcatInplace(InSelf, InValue); 
	if (Concated)
	{
		Py_XDECREF(Concated);
		Py_RETURN_NONE;
	}
	else
	{
		return nullptr;
	}
}

PyObject* FNePyArrayWrapper::Index(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
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

	FNePyPropValue PropValue(InSelf->Prop->Inner);
	if (!PropValue.SetValue(PyValueObj))
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

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();
	const Py_ssize_t ResolvedStartIndex = NePyBase::ResolveContainerIndexParam(StartIndex, ElementCount);
	const Py_ssize_t ResolvedStopIndex = NePyBase::ResolveContainerIndexParam(StopIndex, ElementCount);

	StartIndex = FMath::Min((int32)ResolvedStartIndex, ElementCount);
	StopIndex = FMath::Max((int32)ResolvedStopIndex, ElementCount);

	int32 ReturnIndex = INDEX_NONE;
	for (int32 ElementIndex = StartIndex; ElementIndex < StopIndex; ++ElementIndex)
	{
		if (PropValue.Prop->Identical(ArrayHelper.GetRawPtr(ElementIndex), PropValue.Value, 0))
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

	PyObject* PyRet;
	NePyBase::ToPy(ReturnIndex, PyRet);
	return PyRet;
}

PyObject* FNePyArrayWrapper::Insert(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyIndexObj = nullptr;
	PyObject* PyValueObj = nullptr;

	static const char* ArgsKwdList[] = { "Index", "Value", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:Insert", (char**)ArgsKwdList, &PyIndexObj, &PyValueObj))
	{
		return nullptr;
	}

	int32 InsertIndex = 0;
	if (!NePyBase::ToCpp(PyIndexObj, InsertIndex))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Index' must have type 'int'");
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(InsertIndex, ElementCount);

	InsertIndex = FMath::Min((int32)ResolvedIndex, ElementCount);
	InsertIndex = FMath::Max(0, InsertIndex);
	ArrayHelper.InsertValues(InsertIndex);

	if (!NePyBase::TryConvertPyObjectToFPropertyDirect(PyValueObj, InSelf->Prop->Inner, ArrayHelper.GetRawPtr(InsertIndex), OwnerObject))
	{
		ArrayHelper.RemoveValues(InsertIndex);
		NePyBase::SetConvertPyObjectToFPropertyError(PyValueObj, InSelf->Prop->Inner);
		return nullptr;
	}

	Py_RETURN_NONE;
}

PyObject* FNePyArrayWrapper::Pop(FNePyArrayWrapper* InSelf, PyObject* InArgs)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|O:pop", &PyObj))
	{
		return nullptr;
	}

	int32 ValueIndex = INDEX_NONE;
	if (PyObj && !NePyBase::ToCpp(PyObj, ValueIndex))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'index' must have type 'int'");
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(ValueIndex, ElementCount);

	if (NePyBase::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->Prop) != 0)
	{
		return nullptr;
	}

	PyObject* PyReturnValue = NePyBase::TryConvertFPropertyToPyObjectDirectNoDependency(InSelf->Prop->Inner, ArrayHelper.GetRawPtr(ResolvedIndex), OwnerObject);
	if (!PyReturnValue)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->Inner);
		return nullptr;
	}

	ArrayHelper.RemoveValues(ResolvedIndex);

	return PyReturnValue;
}

PyObject* FNePyArrayWrapper::Remove(FNePyArrayWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyPropValue PropValue(InSelf->Prop->Inner);
	if (!PropValue.SetValue(InValue))
	{
		return nullptr;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	int32 ValueIndex = INDEX_NONE;
	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		if (PropValue.Prop->Identical(ArrayHelper.GetRawPtr(ElementIndex), PropValue.Value, 0))
		{
			ValueIndex = ElementIndex;
			break;
		}
	}

	if (ValueIndex == INDEX_NONE)
	{
		FString PropValueStr = NePyBase::GetFriendlyPropertyValue(PropValue.Prop, PropValue.Value, PPF_Delimited | PPF_IncludeTransient);
		PyErr_Format(PyExc_ValueError, "Remove failed, %s was not found in the array", TCHAR_TO_UTF8(*PropValueStr));
		return nullptr;
	}

	ArrayHelper.RemoveValues(ValueIndex);

	Py_RETURN_NONE;
}

PyObject* FNePyArrayWrapper::Reverse(FNePyArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	// Taken from Algo::Reverse
	for (int32 i = 0, i2 = ElementCount - 1; i < ElementCount / 2 /*rounding down*/; ++i, --i2)
	{
		ArrayHelper.SwapValues(i, i2);
	}

	Py_RETURN_NONE;
}

PyObject* FNePyArrayWrapper::Sort(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

#if PY_MAJOR_VERSION < 3
	PyObject* PyCmpObject = nullptr;
#endif	// PY_MAJOR_VERSION < 3
	PyObject* PyKeyObj = nullptr;
	PyObject* PyReverseObj = nullptr;

#if PY_MAJOR_VERSION < 3
	static const char* ArgsKwdList[] = { "cmp", "key", "reverse", nullptr };
	static const char* OldArgsKwdList[] = { "Cmp", "Key", "bReverse", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OOO:Sort", (char**)ArgsKwdList, &PyCmpObject, &PyKeyObj, &PyReverseObj) &&
		!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OOO:Sort", (char**)OldArgsKwdList, &PyCmpObject, &PyKeyObj, &PyReverseObj))
#else	// PY_MAJOR_VERSION < 3
	static const char* ArgsKwdList[] = { "key", "reverse", nullptr };
	static const char* OldArgsKwdList[] = { "Key", "bReverse", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:Sort", (char**)ArgsKwdList, &PyKeyObj, &PyReverseObj) &&
		!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:Sort", (char**)OldArgsKwdList, &PyKeyObj, &PyReverseObj))
#endif	// PY_MAJOR_VERSION < 3
	{
		return nullptr;
	}

	bool bReverse = false;
	if (PyReverseObj && !NePyBase::ToCpp(PyReverseObj, bReverse))
	{
#if PY_MAJOR_VERSION < 3
		PyErr_SetString(PyExc_TypeError, "arg3 'reverse' must have type 'int'");
#else	// PY_MAJOR_VERSION < 3
		PyErr_SetString(PyExc_TypeError, "arg2 'reverse' must have type 'int'");
#endif	// PY_MAJOR_VERSION < 3
		return nullptr;
	}

	NePyPropertyToPyObjectFunc ToPyConverter = NePyGetPropertyToPyObjectConverterNoDependency(InSelf->Prop->Inner);
	if (!ToPyConverter)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->Inner);
		return nullptr;
	}

	NePyPyObjectToPropertyFunc ToUEConverter = NePyGetPyObjectToPropertyConverter(InSelf->Prop->Inner);
	if (!ToUEConverter)
	{
		NePyBase::SetConvertPyObjectToFPropertyError(nullptr, InSelf->Prop->Inner);
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	// This isn't ideal, but we have no sorting algorithms that take untyped data, and it's the simplest way to deal with the key (and cmp) arguments that need processing in Python.
	FNePyObjectPtr PyList = NePyStealReference(PyList_New(ElementCount));
	if (!PyList)
	{
		return nullptr;
	}

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		PyObject* PyItemObj = ToPyConverter(InSelf->Prop->Inner, ArrayHelper.GetRawPtr(ElementIndex), OwnerObject);
		if (!PyItemObj)
		{
			NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->Inner);
			return nullptr;
		}
		PyList_SetItem(PyList, ElementIndex, PyItemObj); // PyList_SetItem steals this reference
	}

	// We need to call 'sort' directly since PyList_Sort doesn't expose the version that takes arguments
	FNePyObjectPtr PyListSortFunc = NePyStealReference(PyObject_GetAttrString(PyList, "sort"));
	FNePyObjectPtr PyListSortArgs = NePyStealReference(PyTuple_New(0));
	FNePyObjectPtr PyListSortKwds = NePyStealReference(PyDict_New());
#if PY_MAJOR_VERSION < 3
	PyDict_SetItemString(PyListSortKwds, "cmp", PyCmpObject ? PyCmpObject : Py_None);
#endif	// PY_MAJOR_VERSION < 3
	PyDict_SetItemString(PyListSortKwds, "key", PyKeyObj ? PyKeyObj : Py_None);
	PyDict_SetItemString(PyListSortKwds, "reverse", bReverse ? Py_True : Py_False);

	FNePyObjectPtr PyListSortResult = NePyStealReference(PyObject_Call(PyListSortFunc, PyListSortArgs, PyListSortKwds));
	if (!PyListSortResult)
	{
		return nullptr;
	}

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		PyObject* PyItemObj = PyList_GetItem(PyList, ElementIndex);
		ToUEConverter(PyItemObj, InSelf->Prop->Inner, ArrayHelper.GetRawPtr(ElementIndex), OwnerObject);
	}

	Py_RETURN_NONE;
}

PyObject* FNePyArrayWrapper::Clear(FNePyArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	ArrayHelper.EmptyValues();
	Py_RETURN_NONE;
}

Py_ssize_t FNePyArrayWrapper::Len(FNePyArrayWrapper* InSelf)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	return ArrayHelper.Num();
}

PyObject* FNePyArrayWrapper::GetItem(FNePyArrayWrapper* InSelf, Py_ssize_t InIndex)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(InIndex, ElementCount);

	if (NePyBase::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->Prop) != 0)
	{
		return nullptr;
	}

	PyObject* PyItemObj = NePyBase::TryConvertFPropertyToPyObjectDirectPyOuter(InSelf->Prop->Inner, ArrayHelper.GetRawPtr(ResolvedIndex), InSelf);
	if (!PyItemObj)
	{
		NePyBase::SetConvertFPropertyToPyObjectError(InSelf->Prop->Inner);
		return nullptr;
	}
	return PyItemObj;
}

int FNePyArrayWrapper::DeleteItem(FNePyArrayWrapper* InSelf, Py_ssize_t InIndex)
{
	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(InIndex, ElementCount);

	if (NePyBase::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->Prop) != 0)
	{
		return -1;
	}

	ArrayHelper.RemoveValues(ResolvedIndex, 1);
	return 0;
}

int FNePyArrayWrapper::SetItem(FNePyArrayWrapper* InSelf, Py_ssize_t InIndex, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	if (InValue == nullptr)
	{
		return DeleteItem(InSelf, InIndex);
	}

	UObject* OwnerObject = GetOwnerObject(InSelf);
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = NePyBase::ResolveContainerIndexParam(InIndex, ElementCount);

	if (NePyBase::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->Prop) != 0)
	{
		return -1;
	}

	if (!NePyBase::TryConvertPyObjectToFPropertyDirect(InValue, InSelf->Prop->Inner, ArrayHelper.GetRawPtr(ResolvedIndex), OwnerObject))
	{
		NePyBase::SetConvertPyObjectToFPropertyError(InValue, InSelf->Prop->Inner);
		return -1;
	}

	return 0;
}

PyObject* FNePyArrayWrapper::GetSlice(FNePyArrayWrapper* InSelf, PyObject* SliceObj)
{
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	Py_ssize_t Start, Stop, Step, SliceLength;
	if (PySlice_GetIndicesEx(NEPY_CAST_SLICE(SliceObj), ElementCount, &Start, &Stop, &Step, &SliceLength) < 0)
	{
		return nullptr;
	}

	for (Py_ssize_t Index = 0; Index < SliceLength; ++Index)
	{
		Py_ssize_t SourceIndex = Start + Index * Step;
		if (SourceIndex < 0 || SourceIndex >= ElementCount)
		{
			PyErr_SetString(PyExc_IndexError, "slice index out of range");
			return nullptr;
		}
	}

	FNePyArrayWrapper* RetValue = FNePyArrayWrapper::NewImplicit(InSelf->Prop);
	FScriptArrayHelper ArrayRet(RetValue->Prop, RetValue->Value);
	ArrayRet.Resize(SliceLength);

	for (Py_ssize_t Index = 0; Index < SliceLength; ++Index)
	{
		Py_ssize_t SourceIndex = Start + Index * Step;
		if (SourceIndex < 0 || SourceIndex >= ElementCount)
		{
			PyErr_SetString(PyExc_IndexError, "slice index out of range");
			for (Py_ssize_t CleanupIndex = 0; CleanupIndex < Index; ++CleanupIndex)
			{
				InSelf->Prop->Inner->DestroyValue(ArrayRet.GetRawPtr(CleanupIndex));
			}
			Py_DECREF(RetValue);
			return nullptr;
		}
		void* Src = ArrayHelper.GetRawPtr(SourceIndex);
		void* Dest = ArrayRet.GetRawPtr(Index);	
		InSelf->Prop->Inner->CopyCompleteValue(Dest, Src);		
	}

	return RetValue;
}

int FNePyArrayWrapper::SetSlice(FNePyArrayWrapper* InSelf, PyObject* SliceObj, PyObject* Value)
{
	if (Value == nullptr)
	{
		// 删除切片
		return DeleteSlice(InSelf, SliceObj);
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	Py_ssize_t Start = -1;
	Py_ssize_t Stop = -1;
	Py_ssize_t Step = -1;
	Py_ssize_t SliceLength = -1;

	if (PySlice_GetIndicesEx(NEPY_CAST_SLICE(SliceObj), ElementCount, &Start, &Stop, &Step, &SliceLength) < 0)
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to parse slice object: %s"), UTF8_TO_TCHAR(Py_TYPE(SliceObj)->tp_name));
		return -1;
	}

	// 检查Value是否为序列
	if (!PySequence_Check(Value))
	{
		PyErr_SetString(PyExc_TypeError, "Can only assign an iterable");
		return -1;
	}

	Py_ssize_t ValueLength = PySequence_Size(Value);
	if (ValueLength < 0)
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to get sequence size for value: %s"), UTF8_TO_TCHAR(Py_TYPE(Value)->tp_name));
		return -1;
	}

	if (Step == 1)
	{
		// 简单切片赋值（可能改变大小）
		return SetSimpleSlice(InSelf, Start, Stop, Value, ValueLength);
	}
	else
	{
		// 扩展切片赋值（不能改变大小）
		if (ValueLength != SliceLength)
		{
			PyErr_SetString(PyExc_ValueError, "attempt to assign sequence of wrong size");
			return -1;
		}
		return SetExtendedSlice(InSelf, Start, Step, SliceLength, Value);
	}
}

int FNePyArrayWrapper::SetSimpleSlice(FNePyArrayWrapper* InSelf, Py_ssize_t Start, Py_ssize_t Stop, PyObject* Value, Py_ssize_t ValueLength)
{
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 OriginalElementCount = ArrayHelper.Num();

	Py_ssize_t OldSliceLength = Stop - Start;
	Py_ssize_t SizeChange = ValueLength - OldSliceLength;

	for (Py_ssize_t Index = Start; Index < Stop; Index++)
	{
		InSelf->Prop->Inner->DestroyValue(ArrayHelper.GetRawPtr(Index));
	}

	if (SizeChange != 0)
	{
		if (SizeChange > 0)
		{
			ArrayHelper.Resize(OriginalElementCount + SizeChange);
			// 使用属性系统移动元素而不是内存拷贝
			if (Stop < OriginalElementCount)
			{
				// 从后往前移动，避免覆盖
				for (int32 Index = OriginalElementCount - 1; Index >= Stop; --Index)
				{
					void* OldPtr = ArrayHelper.GetRawPtr(Index);
					void* NewPtr = ArrayHelper.GetRawPtr(Index + SizeChange);					
					InSelf->Prop->Inner->InitializeValue(NewPtr);
						InSelf->Prop->Inner->CopyCompleteValue(NewPtr, OldPtr);
						InSelf->Prop->Inner->DestroyValue(OldPtr);
				}
			}
		}
		else
		{
			if (Stop < OriginalElementCount)
			{
				// 从前往后移动
				for (int32 Index = Stop; Index < OriginalElementCount; ++Index)
				{
					void* OldPtr = ArrayHelper.GetRawPtr(Index);
					void* NewPtr = ArrayHelper.GetRawPtr(Index + SizeChange);
					// 目标位置可能已经有值，先销毁
					if (Index + SizeChange >= Start + ValueLength)
					{
						InSelf->Prop->Inner->DestroyValue(NewPtr);
						InSelf->Prop->Inner->InitializeValue(NewPtr);
					}
					InSelf->Prop->Inner->CopyCompleteValue(NewPtr, OldPtr);
					InSelf->Prop->Inner->DestroyValue(OldPtr);
				}
			}
			ArrayHelper.Resize(OriginalElementCount + SizeChange);
		}
	}

	for (Py_ssize_t Index = 0; Index < ValueLength; Index++)
	{
		PyObject* Item = PySequence_GetItem(Value, Index);
		if (!Item)
		{
			UE_LOG(LogNePython, Error, TEXT("Failed to get item %lld from value sequence"), (long long)Index);
			return -1;
		}

		void* TargetPtr = ArrayHelper.GetRawPtr(Start + Index);
		InSelf->Prop->Inner->InitializeValue(TargetPtr);

		if (!NePyBase::TryConvertPyObjectToFPropertyDirect(Item, InSelf->Prop->Inner, TargetPtr, GetOwnerObject(InSelf)))
		{
			Py_DECREF(Item);
			InSelf->Prop->Inner->DestroyValue(TargetPtr);
			UE_LOG(LogNePython, Error, TEXT("Failed to convert Python object to property at index %lld"), (long long)Index);
			return -1;
		}
		Py_DECREF(Item);
	}

	return 0;
}

int FNePyArrayWrapper::SetExtendedSlice(FNePyArrayWrapper* Self, Py_ssize_t Start, Py_ssize_t Step, Py_ssize_t SliceLength, PyObject* Value)
{
	FScriptArrayHelper ArrayHelper(Self->Prop, Self->Value);

	// 转换Value为序列
	PyObject* Sequence = PySequence_Fast(Value, "can only assign an iterable");
	if (!Sequence)
	{
		return -1;
	}

	const Py_ssize_t ValueLength = PySequence_Fast_GET_SIZE(Sequence);

	if (ValueLength != SliceLength)
	{
		Py_DECREF(Sequence);
		PyErr_Format(PyExc_ValueError, "attempt to assign sequence of size %zd to extended slice of size %zd", ValueLength, SliceLength);
		return -1;
	}

	for (Py_ssize_t Iterator = 0; Iterator < SliceLength; ++Iterator)
	{
		const Py_ssize_t ArrayIndex = Start + Iterator * Step;
		PyObject* Item = PySequence_Fast_GET_ITEM(Sequence, Iterator);

		if (!NePyBase::TryConvertPyObjectToFPropertyDirect(Item, Self->Prop->Inner, ArrayHelper.GetRawPtr(ArrayIndex), GetOwnerObject(Self)))
		{
			Py_DECREF(Sequence);
			NePyBase::SetConvertPyObjectToFPropertyError(Item, Self->Prop->Inner);
			return -1;
		}
	}

	Py_DECREF(Sequence);
	return 0;
}

int FNePyArrayWrapper::DeleteSlice(FNePyArrayWrapper* InSelf, PyObject* SliceObj)
{
	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	Py_ssize_t Start, Stop, Step, SliceLength;
	if (PySlice_GetIndicesEx(NEPY_CAST_SLICE(SliceObj), ElementCount, &Start, &Stop, &Step, &SliceLength) < 0)
	{
		return -1;
	}

	for (Py_ssize_t Iterator = SliceLength - 1; Iterator >= 0; --Iterator)
	{
		Py_ssize_t Index = Start + Iterator * Step;
		ArrayHelper.RemoveValues(Index, 1);
	}

	return 0;
}

int FNePyArrayWrapper::Contains(FNePyArrayWrapper* InSelf, PyObject* InValue)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return -1;
	}

	FNePyPropValue PropValue(InSelf->Prop->Inner);
	if (!PropValue.SetValue(InValue))
	{
		PyErr_Clear();
		return 0;
	}

	FScriptArrayHelper ArrayHelper(InSelf->Prop, InSelf->Value);
	const int32 ElementCount = ArrayHelper.Num();

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		if (PropValue.Prop->Identical(ArrayHelper.GetRawPtr(ElementIndex), PropValue.Value))
		{
			return 1;
		}
	}

	return 0;
}

PyObject* FNePyArrayWrapper::Concat(FNePyArrayWrapper* InSelf, PyObject* InOther)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyArrayWrapper* OtherArray = nullptr;
	if (PyRange_Check(InOther) || PyIter_Check(InOther) || PySequence_Check(InOther))
	{
		OtherArray = FromPyIterable(InSelf->Prop, InOther);
	}
	else if (FNePyArrayWrapper::Check(InOther))
	{
		OtherArray = (FNePyArrayWrapper*)InOther;
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "Can't concat ArrayWrapper and '%s' objects", InOther->ob_type->tp_name);
	}

	if (!OtherArray)
	{
		return nullptr;
	}

	FNePyArrayWrapper* RetValue = FNePyArrayWrapper::NewImplicit(InSelf->Prop);

	FScriptArrayHelper ArraySelf(InSelf->Prop, InSelf->Value);
	FScriptArrayHelper ArrayOther(OtherArray->Prop, OtherArray->Value);
	FScriptArrayHelper ArrayRet(RetValue->Prop, RetValue->Value);

	const int32 SizeSelf = ArraySelf.Num();
	const int32 SizeOther = ArrayOther.Num();

	ArrayRet.Resize(SizeSelf + SizeOther);

	// Copy elements from both arrays into the new array
	for (int32 Index = 0; Index < SizeSelf; ++Index)
	{
		void* Src = ArraySelf.GetRawPtr(Index);
		void* Dest = ArrayRet.GetRawPtr(Index);
		InSelf->Prop->Inner->InitializeValue(Dest);
		InSelf->Prop->Inner->CopyCompleteValue(Dest, Src);
	}

	for (int32 Index = 0; Index < SizeOther; ++Index)
	{
		void* Src = ArrayOther.GetRawPtr(Index);
		void* Dest = ArrayRet.GetRawPtr(SizeSelf + Index);
		InSelf->Prop->Inner->InitializeValue(Dest);
		InSelf->Prop->Inner->CopyCompleteValue(Dest, Src);
	}

	return RetValue;
}

PyObject* FNePyArrayWrapper::ConcatInplace(FNePyArrayWrapper* InSelf, PyObject* InOther)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyArrayWrapper* OtherArray = nullptr;
	if (PyRange_Check(InOther) || PyIter_Check(InOther) || PySequence_Check(InOther))
	{
		OtherArray = FromPyIterable(InSelf->Prop, InOther);
	}
	else if (FNePyArrayWrapper::Check(InOther))
	{
		OtherArray = (FNePyArrayWrapper*)InOther;
	}
	else
	{
		PyErr_Format(PyExc_TypeError, "Can't concat ArrayWrapper and '%s' objects", InOther->ob_type->tp_name);
	}

	if (!OtherArray)
	{
		return nullptr;
	}

	FScriptArrayHelper ArraySelf(InSelf->Prop, InSelf->Value);
	FScriptArrayHelper ArrayOther(OtherArray->Prop, OtherArray->Value);

	const int32 SizeSelf = ArraySelf.Num();
	const int32 SizeOther = ArrayOther.Num();

	ArraySelf.Resize(SizeSelf + SizeOther);

	for (int32 Index = 0; Index < SizeOther; ++Index)
	{
		void* Src = ArrayOther.GetRawPtr(Index);
		void* Dest = ArraySelf.GetRawPtr(SizeSelf + Index);
		InSelf->Prop->Inner->InitializeValue(Dest);
		InSelf->Prop->Inner->CopyCompleteValue(Dest, Src);
	}

	Py_INCREF(InSelf);
	return InSelf;
}

PyObject* FNePyArrayWrapper::Repeat(FNePyArrayWrapper* InSelf, Py_ssize_t InTimes)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FNePyArrayWrapper* RetValue = FNePyArrayWrapper::NewImplicit(InSelf->Prop);

	if (InTimes < 0)
	{
		// Negative repetition results in an empty array
		return RetValue;
	}

	FScriptArrayHelper ArraySelf(InSelf->Prop, InSelf->Value);
	FScriptArrayHelper ArrayRet(RetValue->Prop, RetValue->Value);

	const int32 SizeSelf = ArraySelf.Num();
	ArrayRet.Resize(SizeSelf * InTimes);

	// Copy elements from the original array into the new array multiple times
	for (int32 Index = 0; Index < SizeSelf; ++Index)
	{
		void* Src = ArraySelf.GetRawPtr(Index);
		for (Py_ssize_t Time = 0; Time < InTimes; ++Time)
		{
			void* Dest = ArrayRet.GetRawPtr(Index + Time * SizeSelf);
			InSelf->Prop->Inner->CopyCompleteValue(Dest, Src);
		}
	}

	return RetValue;
}

PyObject* FNePyArrayWrapper::RepeatInplace(FNePyArrayWrapper* InSelf, Py_ssize_t InTimes)
{
	if (!InSelf->CheckValidAndSetPyErr())
	{
		return nullptr;
	}

	FScriptArrayHelper ArraySelf(InSelf->Prop, InSelf->Value);

	const int32 SizeSelf = ArraySelf.Num();
	ArraySelf.Resize(SizeSelf * InTimes);

	if (SizeSelf == 0 || InTimes <= 1)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	for (int32 Index = 0; Index < SizeSelf; ++Index)
	{
		void* Src = ArraySelf.GetRawPtr(Index);
		for (Py_ssize_t Time = 1; Time < InTimes; ++Time)
		{
			void* Dest = ArraySelf.GetRawPtr(Index + Time * SizeSelf);
			InSelf->Prop->Inner->InitializeValue(Dest);
			InSelf->Prop->Inner->CopyCompleteValue(Dest, Src);
		}
	}

	Py_INCREF(InSelf);
	return InSelf;
}

PyObject* FNePyArrayWrapper::GetSubscript(FNePyArrayWrapper* Self, PyObject* Key)
{
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(Key))
	{
		// 处理整数索引
		Py_ssize_t Index = PyInt_AsSsize_t(Key);
#else
	if (PyLong_Check(Key))
	{
		// 处理整数索引
		Py_ssize_t Index = PyLong_AsSsize_t(Key);
#endif
		if (PyErr_Occurred())
		{
			return nullptr;
		}
		return GetItem(Self, Index);
	}
	else if (PySlice_Check(Key))
	{
		// 处理切片
		return GetSlice(Self, Key);
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "indices must be integers or slices");
		return nullptr;
	}
}

int FNePyArrayWrapper::SetSubscript(FNePyArrayWrapper* Self, PyObject* Key, PyObject* Value)
{
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(Key))
	{
		// 处理整数索引
		Py_ssize_t Index = PyInt_AsSsize_t(Key);
#else
	if (PyLong_Check(Key))
	{
		// 处理整数索引
		Py_ssize_t Index = PyLong_AsSsize_t(Key);
#endif
		if (PyErr_Occurred())
		{
			return -1;
		}
		return SetItem(Self, Index, Value);
	}
	else if (PySlice_Check(Key))
	{
		// 处理切片
		return SetSlice(Self, Key, Value);
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "indices must be integers or slices");
		return -1;
	}
}

FNePyStructArrayWrapper* FNePyStructArrayWrapper::New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName)
{
	const FProperty* Prop = NePyBase::FindPropertyByMemberPtr(InStruct, InInstance, InMemberPtr);
	if (!Prop)
	{
		PyErr_Format(PyExc_AttributeError, "'%s' has no property '%s'", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	check(InMemberPtr == Prop->ContainerPtrToValuePtr<void>(InInstance));

	const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop);
	if (!ArrayProp)
	{
		PyErr_Format(PyExc_AttributeError, "'%s.%s' is not array property", TCHAR_TO_UTF8(*InStruct->GetName()), PropName);
		return nullptr;
	}

	return FNePyStructArrayWrapper::New(PyOuter, InMemberPtr, ArrayProp);
}

FNePyStructArrayWrapper* FNePyStructArrayWrapper::New(PyObject* PyOuter, void* InMemberPtr, const FArrayProperty* InArrayProp)
{
	FNePyStructArrayWrapper* RetValue = PyObject_New(FNePyStructArrayWrapper, &FNePyStructArrayWrapperType);
	RetValue->Value = InMemberPtr;
	RetValue->Prop = InArrayProp;
	RetValue->PyOuter = PyOuter;
	RetValue->IsObjectProperty = false;
	RetValue->IsSelfCreateValue = false;
	Py_INCREF(PyOuter);
	return RetValue;
}

void FNePyStructArrayWrapper::Dealloc(FNePyStructArrayWrapper* InSelf)
{
	if (InSelf->PyOuter)
	{
		Py_DECREF(InSelf->PyOuter);
		InSelf->PyOuter = nullptr;
	}
	Py_TYPE(InSelf)->tp_free(InSelf);
}

FNePyArrayWrapper* NePyArrayWrapperCheck(PyObject* InPyObj)
{
	return FNePyArrayWrapper::Check(InPyObj);
}

PyTypeObject* NePyArrayWrapperGetType()
{
	return &FNePyArrayWrapperType;
}