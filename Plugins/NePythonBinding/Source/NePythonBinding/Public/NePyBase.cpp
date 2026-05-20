#include "NePyBase.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "NePyPropertyConvert.h"
#include "NePyHouseKeeper.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyObjectBase.h"
#include "NePyStructBase.h"
#include "NePyEnumBase.h"
#include "NePyFieldPath.h"
#include "NePySoftPtr.h"
#include "NePyWeakPtr.h"

DEFINE_LOG_CATEGORY(LogNePython);

namespace Internal
{
	template <typename T>
	bool ToCppInt(PyObject* PyObj, T& OutVal, bool bStrict)
	{
		// Booleans subclass integer, so exclude those explicitly
		if (!PyBool_Check(PyObj))
		{
#if PY_MAJOR_VERSION < 3
			if (PyInt_Check(PyObj))
			{
				OutVal = (T)PyInt_AsLong(PyObj);
				return true;
			}
#endif

			if (PyLong_Check(PyObj))
			{
				OutVal = (T)PyLong_AsLongLong(PyObj);
				return true;
			}

			if (!bStrict)
			{
				if (PyFloat_Check(PyObj))
				{
					OutVal = (T)PyFloat_AsDouble(PyObj);
					return true;
				}
			}
		}

		return false;
	}

	template <typename T>
	bool ToPyInt(const T Val, PyObject*& OutPyObj)
	{
#if PY_MAJOR_VERSION < 3
		if (Val >= LONG_MIN && Val <= LONG_MAX)
		{
			OutPyObj = PyInt_FromLong(Val);
		}
		else
#endif
		{
			OutPyObj = PyLong_FromLongLong(Val);
		}

		return true;
	}

	template <typename T>
	bool ToCppUInt(PyObject* PyObj, T& OutVal, bool bStrict)
	{
		// Booleans subclass integer, so exclude those explicitly
		if (!PyBool_Check(PyObj))
		{
#if PY_MAJOR_VERSION < 3
			if (PyInt_Check(PyObj))
			{
				OutVal = (T)PyInt_AsSsize_t(PyObj);
				return true;
			}
#endif

			if (PyLong_Check(PyObj))
			{
				OutVal = (T)PyLong_AsUnsignedLongLong(PyObj);
				return true;
			}

			if (!bStrict)
			{
				if (PyFloat_Check(PyObj))
				{
					OutVal = (T)PyFloat_AsDouble(PyObj);
					return true;
				}
			}
		}

		return false;
	}

	template <typename T>
	bool ToPyUInt(const T Val, PyObject*& OutPyObj)
	{
#if PY_MAJOR_VERSION < 3
		if (Val <= LONG_MAX)
		{
			OutPyObj = PyInt_FromSsize_t(Val);
		}
		else
#endif
		{
			OutPyObj = PyLong_FromUnsignedLongLong(Val);
		}

		return true;
	}

	template <typename T>
	bool ToCppFloat(PyObject* PyObj, T& OutVal)
	{
		// Booleans subclass integer, so exclude those explicitly
		if (!PyBool_Check(PyObj))
		{
#if PY_MAJOR_VERSION < 3
			if (PyInt_Check(PyObj))
			{
				OutVal = (T)PyInt_AsSsize_t(PyObj);
				return true;
			}
#endif

			if (PyLong_Check(PyObj))
			{
				OutVal = (T)PyLong_AsLongLong(PyObj);
				return true;
			}

			if (PyFloat_Check(PyObj))
			{
				OutVal = (T)PyFloat_AsDouble(PyObj);
				return true;
			}
		}

		return false;
	}

	template <typename T>
	bool ToPyFloat(const T Val, PyObject*& OutPyObj)
	{
		OutPyObj = PyFloat_FromDouble(Val);
		return true;
	}
}

namespace NePyBase
{

	bool ToCpp(PyObject* PyObj, bool& OutVal, bool bStrict)
	{
		if (PyObj == Py_True)
		{
			OutVal = true;
			return true;
		}

		if (PyObj == Py_False)
		{
			OutVal = false;
			return true;
		}

		if (!bStrict)
		{
			if (PyObj == Py_None)
			{
				OutVal = false;
				return true;
			}

#if PY_MAJOR_VERSION < 3
			if (PyInt_Check(PyObj))
			{
				OutVal = PyInt_AsLong(PyObj) != 0;
				return true;
			}
#endif

			if (PyLong_Check(PyObj))
			{
				OutVal = PyLong_AsLongLong(PyObj) != 0;
				return true;
			}
		}

		return false;
	}

	bool ToPy(const bool Val, PyObject*& OutPyObj)
	{
		if (Val)
		{
			Py_INCREF(Py_True);
			OutPyObj = Py_True;
		}
		else
		{
			Py_INCREF(Py_False);
			OutPyObj = Py_False;
		}

		return true;
	}

	bool CheckValidAndSetPyErr(FNePyObjectBase* InPyObj)
	{
		if (FNePyHouseKeeper::Get().IsValid(InPyObj))
		{
			return true;
		}

		PyErr_Format(PyExc_RuntimeError, "'%s' underlying UObject is invalid", Py_TYPE(InPyObj)->tp_name);
		return false;
	}

	bool CheckValidAndSetPyErr(FNePyObjectBase* InPyObj, const char* InMemberName)
	{
		if (FNePyHouseKeeper::Get().IsValid(InPyObj))
		{
			return true;
		}

		PyErr_Format(PyExc_RuntimeError, "%s of '%s' is in invalid state", InMemberName, Py_TYPE(InPyObj)->tp_name);
		return false;
	}

	bool CheckValidAndSetPyErr(FNePyObjectBase* InPyObj, const FProperty* InProperty)
	{
		if (FNePyHouseKeeper::Get().IsValid(InPyObj))
		{
			return true;
		}
		PyErr_Format(PyExc_RuntimeError, "attribute '%s' of '%s' is in invalid state", TCHAR_TO_UTF8(*InProperty->GetName()), Py_TYPE(InPyObj)->tp_name);
		return false;
	}

	void SetBinopTypeError(PyObject* InLeft, PyObject* InRight, const char* InOpName)
	{
		PyErr_Format(PyExc_TypeError,
			"unsupported operand type(s) for %.100s: "
			"'%.100s' and '%.100s'",
			InOpName,
			InLeft->ob_type->tp_name,
			InRight->ob_type->tp_name);
	}

	int ValidateContainerIndexParam(const Py_ssize_t InIndex, const Py_ssize_t InLen, const FProperty* InProp)
	{
		if (InIndex < 0 || InIndex >= InLen)
		{
			PyErr_Format(PyExc_IndexError, "Index %d is out-of-bounds (len: %d) for property '%s' (%s)", (int32)InIndex, (int32)InLen, TCHAR_TO_UTF8(*InProp->GetName()), TCHAR_TO_UTF8(*InProp->GetClass()->GetName()));
			return -1;
		}

		return 0;
	}

	Py_ssize_t ResolveContainerIndexParam(const Py_ssize_t InIndex, const Py_ssize_t InLen)
	{
		return InIndex < 0 ? InIndex + InLen : InIndex;
	}

	FString GetFriendlyPropertyValue(const FProperty* InProp, const void* InPropValue, const uint32 InPortFlags)
	{
		FString FriendlyPropertyValue;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		InProp->ExportTextItem_Direct(FriendlyPropertyValue, InPropValue, InPropValue, nullptr, InPortFlags, nullptr);
#else
		InProp->ExportTextItem(FriendlyPropertyValue, InPropValue, InPropValue, nullptr, InPortFlags, nullptr);
#endif
		return FriendlyPropertyValue;
	}

	FProperty* FindPropertyByMemberPtr(const UStruct* InStruct, const void* InInstancePtr, const void* InMemberPtr)
	{
		check((uint64)InMemberPtr >= (uint64)InInstancePtr);
		int32 MemberOffset = (int32)((uint64)InMemberPtr - (uint64)InInstancePtr);
		for (FProperty* Property = InStruct->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
		{
			if (Property->GetOffset_ForInternal() == MemberOffset)
			{
				return Property;
			}
		}
		return nullptr;
	}

	bool ToCpp(PyObject* PyObj, int8& OutVal, bool bStrict)
	{
		return Internal::ToCppInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const int8 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, uint8& OutVal, bool bStrict)
	{
		return Internal::ToCppUInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const uint8 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyUInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, int16& OutVal, bool bStrict)
	{
		return Internal::ToCppInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const int16 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, uint16& OutVal, bool bStrict)
	{
		return Internal::ToCppUInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const uint16 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyUInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, int32& OutVal, bool bStrict)
	{
		return Internal::ToCppInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const int32 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, uint32& OutVal, bool bStrict)
	{
		return Internal::ToCppUInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const uint32 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyUInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, int64& OutVal, bool bStrict)
	{
		return Internal::ToCppInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const int64 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, uint64& OutVal, bool bStrict)
	{
		return Internal::ToCppUInt(PyObj, OutVal, bStrict);
	}

	bool ToPy(const uint64 Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyUInt(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, float& OutVal)
	{
		return Internal::ToCppFloat(PyObj, OutVal);
	}

	bool ToPy(const float Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyFloat(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, double& OutVal)
	{
		return Internal::ToCppFloat(PyObj, OutVal);
	}

	bool ToPy(const double Val, PyObject*& OutPyObj)
	{
		return Internal::ToPyFloat(Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, const char*& OutVal)
	{
# if PY_MAJOR_VERSION < 3
		if (PyUnicode_Check(PyObj))
		{
			FNePyObjectPtr PyBytesObj = FNePyObjectPtr::StealReference(PyUnicode_AsUTF8String(PyObj));
			if (PyBytesObj)
			{
				OutVal = PyBytes_AsString(PyBytesObj);
				return true;
			}
		}

		if (PyString_Check(PyObj))
		{
			OutVal = PyString_AsString(PyObj);
			return true;
		}
#else
		if (PyUnicode_Check(PyObj))
		{
			OutVal = PyUnicode_AsUTF8(PyObj);
			return true;
		}

		if (PyBytes_Check(PyObj))
		{
			OutVal = PyBytes_AsString(PyObj);
			return true;
		}
#endif
		return false;
	}

	bool ToPy(const char* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
		}
		else
		{
			OutPyObj = NePyString_FromString(Val);
		}
		return true;
	}

	bool ToPy(const TCHAR* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}

#if PY_MAJOR_VERSION < 3
		if (FCString::IsPureAnsi(Val))
		{
			OutPyObj = PyString_FromString(TCHAR_TO_ANSI(Val));
		}
		else
		{
			// 1. 先编码为unicode
			PyObject* UnicodeObject = PyUnicode_FromString(TCHAR_TO_UTF8(Val));
			// 2. 再将unicode encode 为str，保证函数返回值都是str obj
			OutPyObj = PyUnicode_AsUTF8String(UnicodeObject);
			Py_XDECREF(UnicodeObject);
		}
#else
		{
			OutPyObj = PyUnicode_FromString(TCHAR_TO_UTF8(Val));
		}
#endif	// PY_MAJOR_VERSION < 3

		return true;
	}

	bool ToCpp(PyObject* PyObj, FString& OutVal)
	{
		const char* TempVal;
		if (!ToCpp(PyObj, TempVal))
		{
			return false;
		}

		OutVal = UTF8_TO_TCHAR(TempVal);
		return true;
	}

	bool ToPy(const FString& Val, PyObject*& OutPyObj)
	{
		return ToPy(*Val, OutPyObj);
	}

	bool ToPy(const FString* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}
		return ToPy(*Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, FName& OutVal)
	{
		const char* TempVal;
		if (!ToCpp(PyObj, TempVal))
		{
			return false;
		}

		OutVal = UTF8_TO_TCHAR(TempVal);
		return true;
	}

	bool ToPy(const FName& Val, PyObject*& OutPyObj)
	{
		return ToPy(Val.ToString(), OutPyObj);
	}

	bool ToPy(const FName* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}
		return ToPy(*Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, FText& OutVal)
	{
		FString TextStr;
		if (ToCpp(PyObj, TextStr))
		{
			OutVal = FText::FromString(TextStr);
			return true;
		}
		return false;
	}

	bool ToPy(const FText& Val, PyObject*& OutPyObj)
	{
		return ToPy(Val.ToString(), OutPyObj);
	}

	bool ToPy(const FText* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}
		return ToPy(*Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, FFieldPath& OutVal)
	{
		if (FNePyFieldPath* PyFieldPath = FNePyFieldPath::Check(PyObj))
		{
			OutVal = PyFieldPath->Value;
			return true;
		}
		else
		{
			FString StrValue;
			if (ToCpp(PyObj, StrValue))
			{
				OutVal.Generate(*StrValue);
				return true;
			}
		}
		return false;
	}

	bool ToPy(const FFieldPath& Val, PyObject*& OutPyObj)
	{
		OutPyObj = FNePyFieldPath::New(Val);
		return true;
	}

	bool ToPy(const FFieldPath* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}
		return ToPy(*Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, FSoftObjectPtr& OutVal)
	{
		if (FNePySoftPtr* PySoftPtr = FNePySoftPtr::Check(PyObj))
		{
			OutVal = PySoftPtr->Value;
			return true;
		}
		return false;
	}

	bool ToPy(const FSoftObjectPtr& Val, PyObject*& OutPyObj)
	{
		OutPyObj = FNePySoftPtr::New(Val);
		return true;
	}

	bool ToPy(const FSoftObjectPtr* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}
		return ToPy(*Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, FWeakObjectPtr& OutVal)
	{
		if (FNePyWeakPtr* PySoftPtr = FNePyWeakPtr::Check(PyObj))
		{
			OutVal = PySoftPtr->Value;
			return true;
		}
		return false;
	}

	bool ToPy(const FWeakObjectPtr& Val, PyObject*& OutPyObj)
	{
		OutPyObj = FNePyWeakPtr::New(Val);
		return true;
	}

	bool ToPy(const FWeakObjectPtr* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}
		return ToPy(*Val, OutPyObj);
	}

	bool ToCpp(PyObject* PyObj, TArray<uint8>& OutVal)
	{
		char* BytesPtr;
		Py_ssize_t BytesLength;
		if (PyBytes_AsStringAndSize(PyObj, &BytesPtr, &BytesLength) != 0)
		{
			return false;
		}

		OutVal.Empty();
		OutVal.AddUninitialized((int32)BytesLength);
		FMemory::Memcpy(OutVal.GetData(), BytesPtr, BytesLength);
		return true;
	}

	bool ToPy(const TArray<uint8>& Val, PyObject*& OutPyObj)
	{
		OutPyObj = PyBytes_FromStringAndSize((const char*)Val.GetData(), (Py_ssize_t)Val.Num());
		return true;
	}


#if WITH_EDITORONLY_DATA
	PyObject* ToPy(FProperty* Val)
	{
		if (!Val)
		{
			Py_RETURN_NONE;
		}

		auto PropObj = Val->GetUPropertyWrapper();
		return ToPy(PropObj);
	}
#endif

	bool ToCpp(PyObject* PyObj, UObject*& OutVal, const UClass* ExpectedType)
	{
		if (ExpectedType && ExpectedType->IsChildOf<UField>())
		{
			return ToCpp(PyObj, (UField*&)OutVal, ExpectedType);
		}
		
		FNePyObjectBase* RetValue = NePyObjectBaseCheck(PyObj);
		if (!RetValue)
		{
			return false;
		}

		if (!FNePyHouseKeeper::Get().IsValid(RetValue))
		{
			return false;
		}

		if (ExpectedType && !RetValue->Value->IsA(ExpectedType))
		{
			return false;
		}

		OutVal = RetValue->Value;
		return true;
	}

	UObject* ToCppObject(PyObject* PyObj, const UClass* ExpectedType)
	{
		UObject* Object = nullptr;
		ToCpp(PyObj, Object, ExpectedType);
		return Object;
	}

	bool ToPy(const UObject* Val, PyObject*& OutPyObj)
	{
		if (!Val)
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
			return true;
		}

		OutPyObj = FNePyHouseKeeper::Get().NewNePyObject(const_cast<UObject*>(Val));
		return true;
	}

	PyObject* ToPy(const UObject* Val)
	{
		PyObject* Obj = nullptr;
		ToPy(Val, Obj);
		return Obj;
	}

	bool ToCpp(PyObject* PyObj, UField*& OutVal, const UClass* ExpectedType)
	{
		check(ExpectedType);
		if (ExpectedType == UClass::StaticClass())
		{
			return ToCpp(PyObj, (UClass*&)OutVal);
		}
		if (ExpectedType == UScriptStruct::StaticClass())
		{
			return ToCpp(PyObj, (UScriptStruct*&)OutVal);
		}
		if (ExpectedType == UEnum::StaticClass())
		{
			return ToCpp(PyObj, (UEnum*&)OutVal);
		}

		// ExpectedType为 UField|UStruct|UFunction
		// 首先还是尝试转换PyType
		if (ToCpp(PyObj, (UStruct*&)OutVal) || ToCpp(PyObj, (UEnum*&)OutVal))
		{
			return true;
		}

		// 尝试失败，回到UObject转换逻辑
		FNePyObjectBase* RetValue = NePyObjectBaseCheck(PyObj);
		if (!RetValue)
		{
			return false;
		}

		if (!FNePyHouseKeeper::Get().IsValid(RetValue))
		{
			return false;
		}

		if (ExpectedType && !RetValue->Value->IsA(ExpectedType))
		{
			return false;
		}

		check(RetValue->Value->IsA<UField>());
		OutVal = (UField*)RetValue->Value;
		return true;
	}

	UField* ToCppField(PyObject* PyObj, const UClass* ExpectedType)
	{
		UField* Field = nullptr;
		ToCpp(PyObj, Field, ExpectedType);
		return Field;
	}

	bool ToCpp(PyObject* PyObj, UStruct*& OutVal, const UClass* ExpectedType)
	{
		const UObject* RetValue = nullptr;

		if (PyObj && PyObject_TypeCheck(PyObj, &PyType_Type))
		{
			PyTypeObject* PyType = (PyTypeObject*)PyObj;
			if (PyType_IsSubtype(PyType, NePyObjectBaseGetType()))
			{
				RetValue = FNePyWrapperTypeRegistry::Get().GetClassByPyType(PyType);
			}
			else if (PyType_IsSubtype(PyType, NePyStructBaseGetType()))
			{
				RetValue = FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyType);
			}
		}
		else
		{
			FNePyObjectBase* NePyObj = NePyObjectBaseCheck(PyObj);
			if (!NePyObj)
			{
				return false;
			}

			if (!FNePyHouseKeeper::Get().IsValid(NePyObj))
			{
				return false;
			}

			RetValue = NePyObj->Value;
		}

		if (!RetValue || !RetValue->IsA<UStruct>())
		{
			return false;
		}

		if (ExpectedType && !((UStruct*)RetValue)->IsChildOf(ExpectedType))
		{
			return false;
		}

		OutVal = (UStruct*)RetValue;
		return true;
	}

	UStruct* ToCppStruct(PyObject* PyObj, const UClass* ExpectedType)
	{
		UStruct* Struct = nullptr;
		ToCpp(PyObj, Struct, ExpectedType);
		return Struct;
	}

	bool ToCpp(PyObject* PyObj, UClass*& OutVal, const UClass* ExpectedType)
	{
		UObject* RetValue = ToCppStruct(PyObj, ExpectedType);
		if (!RetValue || !RetValue->IsA<UClass>())
		{
			return false;
		}

		OutVal = (UClass*)RetValue;
		return true;
	}

	UClass* ToCppClass(PyObject* PyObj, const UClass* ExpectedType)
	{
		UClass* Class = nullptr;
		ToCpp(PyObj, Class, ExpectedType);
		return Class;
	}

	bool ToCpp(PyObject* PyObj, UScriptStruct*& OutVal, const UClass* ExpectedType)
	{
		UObject* RetValue = ToCppStruct(PyObj, ExpectedType);
		if (!RetValue || !RetValue->IsA<UScriptStruct>())
		{
			return false;
		}

		OutVal = (UScriptStruct*)RetValue;
		return true;
	}

	UScriptStruct* ToCppScriptStruct(PyObject* PyObj, const UClass* ExpectedType)
	{
		UScriptStruct* ScriptStruct = nullptr;
		ToCpp(PyObj, ScriptStruct, ExpectedType);
		return ScriptStruct;
	}

	bool ToCpp(PyObject* PyObj, UEnum*& OutVal)
	{
		const UObject* RetValue = nullptr;

		if (PyObj && PyObject_TypeCheck(PyObj, &PyType_Type))
		{
			PyTypeObject* PyType = (PyTypeObject*)PyObj;
			if (PyType_IsSubtype(PyType, NePyEnumBaseGetType()))
			{
				RetValue = FNePyWrapperTypeRegistry::Get().GetEnumByPyType(PyType);
			}
		}
		else
		{
			FNePyObjectBase* NePyObj = NePyObjectBaseCheck(PyObj);
			if (!NePyObj)
			{
				return false;
			}

			if (!FNePyHouseKeeper::Get().IsValid(NePyObj))
			{
				return false;
			}

			RetValue = NePyObj->Value;
		}

		if (!RetValue || !RetValue->IsA<UEnum>())
		{
			return false;
		}

		OutVal = (UEnum*)RetValue;
		return true;
	}

	UEnum* ToCppEnum(PyObject* PyObj)
	{
		UEnum* Enum = nullptr;
		ToCpp(PyObj, Enum);
		return Enum;
	}

	PyObject* TryConvertFPropertyToPyObjectDirect(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
	{
		NePyPropertyToPyObjectFunc Converter = NePyGetPropertyToPyObjectConverter(InProp);
		if (!Converter)
		{
			return nullptr;
		}
		return Converter(InProp, InBuffer, InOwnerObject);
	}

	PyObject* TryConvertFPropertyToPyObjectInContainer(const FProperty* InProp, const void* InBuffer, int32 InArrayIndex, UObject* InOwnerObject)
	{
		check(InArrayIndex < InProp->ArrayDim);
		return TryConvertFPropertyToPyObjectDirect(InProp, InProp->ContainerPtrToValuePtr<void>(InBuffer, InArrayIndex), InOwnerObject);
	}

	PyObject* TryConvertFPropertyToPyObjectDirectPyOuter(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter)
	{
		NePyPropertyToPyObjectFuncForStruct Converter = NePyGetPropertyToPyObjectConverterForStruct(InProp);
		if (!Converter)
		{
			return nullptr;
		}
		return Converter(InProp, InBuffer, InPyOuter);
	}

	PyObject* TryConvertFPropertyToPyObjectInContainerPyOuter(const FProperty* InProp, const void* InBuffer, int32 InArrayIndex, PyObject* InPyOuter)
	{
		check(InArrayIndex < InProp->ArrayDim);
		return TryConvertFPropertyToPyObjectDirectPyOuter(InProp, InProp->ContainerPtrToValuePtr<void>(InBuffer, InArrayIndex), InPyOuter);
	}

	PyObject* TryConvertFPropertyToPyObjectDirectNoDependency(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
	{
		NePyPropertyToPyObjectFunc Converter = NePyGetPropertyToPyObjectConverterNoDependency(InProp);
		if (!Converter)
		{
			return nullptr;
		}
		return Converter(InProp, InBuffer, InOwnerObject);
	}

	PyObject* TryConvertFPropertyToPyObjectInContainerNoDependency(const FProperty* InProp, const void* InBuffer, int32 InArrayIndex, UObject* InOwnerObject)
	{
		check(InArrayIndex < InProp->ArrayDim);
		return TryConvertFPropertyToPyObjectDirectNoDependency(InProp, InProp->ContainerPtrToValuePtr<void>(InBuffer, InArrayIndex), InOwnerObject);
	}

	bool TryConvertPyObjectToFPropertyDirect(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject)
	{
		NePyPyObjectToPropertyFunc Converter = NePyGetPyObjectToPropertyConverter(InProp);
		if (!Converter)
		{
			return false;
		}
		return Converter(InPyObj, InProp, InBuffer, InOwnerObject);
	}

	bool TryConvertPyObjectToFPropertyInContainer(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, int32 InArrayIndex, UObject* InOwnerObject)
	{
		check(InArrayIndex < InProp->ArrayDim);
		return TryConvertPyObjectToFPropertyDirect(InPyObj, InProp, InProp->ContainerPtrToValuePtr<void>(InBuffer, InArrayIndex), InOwnerObject);
	}

	void SetConvertFPropertyToPyObjectError(const FProperty* InProp)
	{
		FString PropClassName = InProp->GetClass()->GetName();
		if (InProp->ArrayDim > 1)
		{
			PropClassName += FString::Printf(TEXT("[%d]"), InProp->ArrayDim);
		}
		PyErr_Format(PyExc_TypeError, "unable to convert property %s (%s) to python object", TCHAR_TO_UTF8(*InProp->GetName()), TCHAR_TO_UTF8(*PropClassName));
	}

	void SetConvertPyObjectToFPropertyError(PyObject* InPyObj, const FProperty* InProp)
	{
		const char* TypeStr;
		FNePyObjectPtr PyTypeStr;
		if (!InPyObj)
		{
			TypeStr = "nullptr";
		}
		else
		{
			PyTypeStr = NePyStealReference(PyObject_Str((PyObject*)InPyObj->ob_type));
			if (PyTypeStr)
			{
				ToCpp(PyTypeStr, TypeStr);
			}
			else
			{
				TypeStr = "unknown";
			}
		}

		FString PropClassName = InProp->GetClass()->GetName();
		if (InProp->ArrayDim > 1)
		{
			PropClassName += FString::Printf(TEXT("[%d]"), InProp->ArrayDim);
		}
		PyErr_Format(PyExc_TypeError, "unable to convert %s to property %s (%s)", TypeStr, TCHAR_TO_UTF8(*InProp->GetName()), TCHAR_TO_UTF8(*PropClassName));
	}

	FString PyObjectToString(PyObject* InPyObj)
	{
		FString OutVal;

# if PY_MAJOR_VERSION < 3
		if (PyString_Check(InPyObj) || PyUnicode_Check(InPyObj))
		{
			ToCpp(InPyObj, OutVal);
			return OutVal;
		}

#else
		if (PyUnicode_Check(InPyObj) || PyBytes_Check(InPyObj))
		{
			ToCpp(InPyObj, OutVal);
			return OutVal;
		}
#endif

		if (FNePyObjectPtr PyStrObj = NePyStealReference(PyObject_Str(InPyObj)))
		{
			ToCpp(PyStrObj, OutVal);
			return OutVal;
		}

		return OutVal;
	}

	UWorld* TryGetWorld(PyObject* InPyObj)
	{
		FNePyObjectBase* PyObj = NePyObjectBaseCheck(InPyObj);
		if (!PyObj)
		{
			return nullptr;
		}

		if (!FNePyHouseKeeper::Get().IsValid(PyObj))
		{
			return nullptr;
		}

		if (PyObj->Value->IsA<UWorld>())
		{
			return (UWorld*)PyObj->Value;
		}

		if (PyObj->Value->IsA<AActor>())
		{
			AActor* Actor = (AActor*)PyObj->Value;
			return Actor->GetWorld();
		}

		if (PyObj->Value->IsA<UActorComponent>())
		{
			UActorComponent* Comp = (UActorComponent*)PyObj->Value;
			return Comp->GetWorld();
		}

		if (PyObj->Value->IsA<UGameInstance>())
		{
			UGameInstance* GameInstance = (UGameInstance*)PyObj->Value;
			return GameInstance->GetWorld();
		}

		return nullptr;
	}
}

FNePyPropValue::FNePyPropValue(const FProperty* InProp)
	: Prop(InProp)
{
	if (Prop->GetSize() <= sizeof(InlineBuffer) && Prop->GetMinAlignment() <= alignof(FNePyPropValue))
	{
		Value = &InlineBuffer;
	}
	else
	{
		Value = FMemory::Malloc(Prop->GetSize(), Prop->GetMinAlignment());
	}
	Prop->InitializeValue(Value);
}

FNePyPropValue::~FNePyPropValue()
{
	if (Value)
	{
		Prop->DestroyValue(Value);
		if (Value != &InlineBuffer)
		{
			FMemory::Free(Value);
		}
		Value = nullptr;
	}
}

bool FNePyPropValue::SetValue(PyObject* InPyObj)
{
	if (NePyBase::TryConvertPyObjectToFPropertyDirect(InPyObj, Prop, Value, nullptr))
	{
		return true;
	}

	NePyBase::SetConvertPyObjectToFPropertyError(InPyObj, Prop);
	return false;
}
