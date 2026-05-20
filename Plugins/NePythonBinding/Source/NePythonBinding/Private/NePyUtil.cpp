#include "NePyUtil.h"
#include "NePyBase.h"
#include "NePyArrayWrapper.h"
#include "NePyFixedArrayWrapper.h"
#include "NePySetWrapper.h"
#include "NePyMapWrapper.h"
#include "UObject/EnumProperty.h"

namespace NePyUtil
{
	bool IsInputParameter(const FProperty* InParam)
	{
		const bool bIsParam = InParam->HasAnyPropertyFlags(CPF_Parm);
		const bool bIsReturnParam = InParam->HasAnyPropertyFlags(CPF_ReturnParm);
		const bool bIsReferenceParam = InParam->HasAnyPropertyFlags(CPF_ReferenceParm);
		const bool bIsOutParam = InParam->HasAnyPropertyFlags(CPF_OutParm) && !InParam->HasAnyPropertyFlags(CPF_ConstParm);
		return bIsParam && !bIsReturnParam && (!bIsOutParam || bIsReferenceParam);
	}

	bool IsOutputParameter(const FProperty* InParam)
	{
		const bool bIsParam = InParam->HasAnyPropertyFlags(CPF_Parm);
		const bool bIsReturnParam = InParam->HasAnyPropertyFlags(CPF_ReturnParm);
		const bool bIsOutParam = InParam->HasAnyPropertyFlags(CPF_OutParm) && !InParam->HasAnyPropertyFlags(CPF_ConstParm);
		return bIsParam && !bIsReturnParam && bIsOutParam;
	}

	bool InspectFunctionArgs(PyObject* InFunc, TArray<FString>& OutArgNames, TArray<FNePyObjectPtr>* OutArgDefaults, bool* bHoutHasDefaults)
	{
		if (!PyFunction_Check(InFunc) && !PyMethod_Check(InFunc))
		{
			return false;
		}

		FNePyObjectPtr PyInspectModule = NePyStealReference(PyImport_ImportModule("inspect"));
		if (!PyInspectModule)
		{
			return false;
		}

		PyObject* PyInspectDict = PyModule_GetDict(PyInspectModule);
#if PY_MAJOR_VERSION >= 3
		PyObject* PyGetArgSpecFunc = PyDict_GetItemString(PyInspectDict, "getfullargspec");
#else
		PyObject* PyGetArgSpecFunc = PyDict_GetItemString(PyInspectDict, "getargspec");
#endif
		if (!PyGetArgSpecFunc)
		{
			return false;
		}

		FNePyObjectPtr PyGetArgSpecResult = NePyStealReference(PyObject_CallFunctionObjArgs(PyGetArgSpecFunc, InFunc, nullptr));
		if (!PyGetArgSpecResult)
		{
			return false;
		}

		PyObject* PyFuncArgNames = PyTuple_GetItem(PyGetArgSpecResult, 0);
		const int32 NumArgNames = (PyFuncArgNames && PyFuncArgNames != Py_None) ? PySequence_Size(PyFuncArgNames) : 0;

		PyObject* PyFuncArgDefaults = PyTuple_GetItem(PyGetArgSpecResult, 3);
		const int32 NumArgDefaults = (PyFuncArgDefaults && PyFuncArgDefaults != Py_None) ? PySequence_Size(PyFuncArgDefaults) : 0;

		OutArgNames.Reset(NumArgNames);
		if (OutArgDefaults)
		{
			OutArgDefaults->Reset(NumArgNames);
		}

		// Get the names
		for (int32 ArgNameIndex = 0; ArgNameIndex < NumArgNames; ++ArgNameIndex)
		{
			PyObject* PyArgName = PySequence_GetItem(PyFuncArgNames, ArgNameIndex);
			OutArgNames.Emplace(NePyBase::PyObjectToString(PyArgName));
		}

		// Get the defaults (padding the start of the array with empty strings)
		if (OutArgDefaults)
		{
			OutArgDefaults->AddDefaulted(NumArgNames - NumArgDefaults);
			for (int32 ArgDefaultIndex = 0; ArgDefaultIndex < NumArgDefaults; ++ArgDefaultIndex)
			{
				PyObject* PyArgDefault = PySequence_GetItem(PyFuncArgDefaults, ArgDefaultIndex);
				OutArgDefaults->Emplace(NePyNewReference(PyArgDefault));
			}
		}

		if (bHoutHasDefaults)
		{
			*bHoutHasDefaults = NumArgDefaults > 0;
		}

		check(!OutArgDefaults || OutArgNames.Num() == OutArgDefaults->Num());
		return true;
	}

	FString GetDocString(PyObject* InPyObj)
	{
		FString DocString;
		if (FNePyObjectPtr DocStringObj = NePyStealReference(PyObject_GetAttrString(InPyObj, "__doc__")))
		{
			if (DocStringObj != Py_None)
			{
				DocString = NePyBase::PyObjectToString(DocStringObj);
			}
		}
		return DocString;
	}

	FString GetFriendlyTypename(PyTypeObject* InPyType)
	{
		return UTF8_TO_TCHAR(InPyType->tp_name);
	}

	FString GetFriendlyTypename(PyObject* InPyObj)
	{
		if (FNePyArrayWrapper* PyArray = FNePyArrayWrapper::Check(InPyObj))
		{
			const FString PropTypeName = PyArray->Prop->Inner ? PyArray->Prop->Inner->GetClass()->GetName() : FString();
			return FString::Printf(TEXT("%s (%s)"), UTF8_TO_TCHAR(Py_TYPE(InPyObj)->tp_name), *PropTypeName);
		}

		if (FNePyFixedArrayWrapper* PyFixedArray = FNePyFixedArrayWrapper::Check(InPyObj))
		{
			const FString PropTypeName = PyFixedArray->Prop ? PyFixedArray->Prop->GetClass()->GetName() : FString();
			return FString::Printf(TEXT("%s (%s)"), UTF8_TO_TCHAR(Py_TYPE(InPyObj)->tp_name), *PropTypeName);
		}

		if (FNePySetWrapper* PySet = FNePySetWrapper::Check(InPyObj))
		{
			const FString PropTypeName = PySet->Prop ? PySet->Prop->ElementProp->GetClass()->GetName() : FString();
			return FString::Printf(TEXT("%s (%s)"), UTF8_TO_TCHAR(Py_TYPE(InPyObj)->tp_name), *PropTypeName);
		}

		if (FNePyMapWrapper* PyMap = FNePyMapWrapper::Check(InPyObj))
		{
			const FString PropKeyName = PyMap->Prop ? PyMap->Prop->KeyProp->GetClass()->GetName() : FString();
			const FString PropTypeName = PyMap->Prop ? PyMap->Prop->ValueProp->GetClass()->GetName() : FString();
			return FString::Printf(TEXT("%s (%s, %s)"), UTF8_TO_TCHAR(Py_TYPE(InPyObj)->tp_name), *PropKeyName, *PropTypeName);
		}

		return GetFriendlyTypename(PyType_Check(InPyObj) ? (PyTypeObject*)InPyObj : Py_TYPE(InPyObj));
	}

	FString GetPropertyTypeString(const FProperty* Prop)
	{
		if (!Prop)
		{
			return TEXT("nullptr");
		}

		FString TypeString = Prop->GetClass()->GetName();
		TypeString.RemoveFromEnd(TEXT("Property"));
		
		if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
		{
			TypeString = FString::Printf(TEXT("%s<%s>"), *TypeString, *ObjProp->PropertyClass->GetName());
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			TypeString = FString::Printf(TEXT("%s<%s>"), *TypeString, *StructProp->Struct->GetName());
		}
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			TypeString = FString::Printf(TEXT("TArray<%s>"), *GetPropertyTypeString(ArrayProp->Inner));
		}
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(Prop))
		{
			TypeString = FString::Printf(TEXT("TMap<%s, %s>"), 
				*GetPropertyTypeString(MapProp->KeyProp), 
				*GetPropertyTypeString(MapProp->ValueProp));
		}
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(Prop))
		{
			TypeString = FString::Printf(TEXT("TSet<%s>"), *GetPropertyTypeString(SetProp->ElementProp));
		}
		else if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
		{
			if (EnumProp->GetEnum())
			{
				TypeString = FString::Printf(TEXT("Enum<%s>"), *EnumProp->GetEnum()->GetName());
			}
		}

		return TypeString;
	}

	FString GetPropertyFlagsString(const FProperty* Prop)
	{
		if (!Prop)
		{
			return TEXT("");
		}

		TArray<FString> FlagStrings;
		
		if (Prop->HasAnyPropertyFlags(CPF_Edit))
			FlagStrings.Add(TEXT("Edit"));
		if (Prop->HasAnyPropertyFlags(CPF_BlueprintVisible))
			FlagStrings.Add(TEXT("BlueprintVisible"));
		if (Prop->HasAnyPropertyFlags(CPF_BlueprintReadOnly))
			FlagStrings.Add(TEXT("BlueprintReadOnly"));
		if (Prop->HasAnyPropertyFlags(CPF_Net))
			FlagStrings.Add(TEXT("Replicated"));
		if (Prop->HasAnyPropertyFlags(CPF_Transient))
			FlagStrings.Add(TEXT("Transient"));
		if (Prop->HasAnyPropertyFlags(CPF_Config))
			FlagStrings.Add(TEXT("Config"));
		if (Prop->HasAnyPropertyFlags(CPF_SaveGame))
			FlagStrings.Add(TEXT("SaveGame"));
		if (Prop->HasAnyPropertyFlags(CPF_EditConst))
			FlagStrings.Add(TEXT("EditConst"));

		return FString::Join(FlagStrings, TEXT(", "));
	}

	FString GetFunctionFlagsString(const UFunction* Func)
	{
		if (!Func)
		{
			return TEXT("");
		}

		TArray<FString> FlagStrings;
		
		if (Func->HasAnyFunctionFlags(FUNC_Static))
			FlagStrings.Add(TEXT("Static"));
		if (Func->HasAnyFunctionFlags(FUNC_BlueprintCallable))
			FlagStrings.Add(TEXT("BlueprintCallable"));
		if (Func->HasAnyFunctionFlags(FUNC_BlueprintPure))
			FlagStrings.Add(TEXT("BlueprintPure"));
		if (Func->HasAnyFunctionFlags(FUNC_BlueprintEvent))
			FlagStrings.Add(TEXT("BlueprintEvent"));
		if (Func->HasAnyFunctionFlags(FUNC_Net))
			FlagStrings.Add(TEXT("Net"));
		if (Func->HasAnyFunctionFlags(FUNC_Const))
			FlagStrings.Add(TEXT("Const"));

		return FString::Join(FlagStrings, TEXT(", "));
	}

	void LogFunctionSignature(const UFunction* Func, const FString& PyName, int32 Level)
	{
		if (!Func || Level < 2)
		{
			return;
		}

		TArray<FString> ParamStrings;
		FProperty* ReturnProperty = nullptr;

		for (TFieldIterator<FProperty> ParamIt(Func); ParamIt; ++ParamIt)
		{
			FProperty* Param = *ParamIt;
			
			if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnProperty = Param;
				continue;
			}

			FString ParamStr = FString::Printf(TEXT("%s: %s"), 
				*Param->GetName(), 
				*GetPropertyTypeString(Param));

			if (Param->HasAnyPropertyFlags(CPF_OutParm))
			{
				ParamStr += TEXT(" [Out]");
			}
			if (Param->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				ParamStr += TEXT(" [Ref]");
			}
			if (Param->HasAnyPropertyFlags(CPF_ConstParm))
			{
				ParamStr += TEXT(" [Const]");
			}

			ParamStrings.Add(ParamStr);
		}

		FString FuncFlags = GetFunctionFlagsString(Func);
		FString ReturnType = ReturnProperty ? GetPropertyTypeString(ReturnProperty) : TEXT("void");
		
		UE_LOG(LogNePython, Log, TEXT("      %s(%s) -> %s%s"), 
			*PyName,
			*FString::Join(ParamStrings, TEXT(", ")),
			*ReturnType,
			FuncFlags.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" [%s]"), *FuncFlags));
	}

	FString GetFunctionSignatureString(const UFunction* Func, const FString& PyName, int32 Level)
	{
		if (!Func || Level < 2)
		{
			return TEXT("");
		}

		TArray<FString> ParamStrings;
		FProperty* ReturnProperty = nullptr;

		for (TFieldIterator<FProperty> ParamIt(Func); ParamIt; ++ParamIt)
		{
			FProperty* Param = *ParamIt;
			
			if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				ReturnProperty = Param;
				continue;
			}

			FString ParamStr = FString::Printf(TEXT("%s: %s"), 
				*Param->GetName(), 
				*GetPropertyTypeString(Param));

			if (Param->HasAnyPropertyFlags(CPF_OutParm))
			{
				ParamStr += TEXT(" [Out]");
			}
			if (Param->HasAnyPropertyFlags(CPF_ReferenceParm))
			{
				ParamStr += TEXT(" [Ref]");
			}
			if (Param->HasAnyPropertyFlags(CPF_ConstParm))
			{
				ParamStr += TEXT(" [Const]");
			}

			ParamStrings.Add(ParamStr);
		}

		FString FuncFlags = GetFunctionFlagsString(Func);
		FString ReturnType = ReturnProperty ? GetPropertyTypeString(ReturnProperty) : TEXT("void");
		
		return FString::Printf(TEXT("%s(%s) -> %s%s"), 
			*PyName,
			*FString::Join(ParamStrings, TEXT(", ")),
			*ReturnType,
			FuncFlags.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" [%s]"), *FuncFlags));
	}

	FString GetPropertyInfoString(const FProperty* Prop, const char* PropName)
	{
		if (!Prop)
		{
			return FString::Printf(TEXT("%s: [nullptr]"), UTF8_TO_TCHAR(PropName));
		}

		FString TypeString = GetPropertyTypeString(Prop);
		FString FlagsString = GetPropertyFlagsString(Prop);
		
		// 处理委托属性，输出其签名函数信息
		FString DelegateInfo;
		if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(Prop))
		{
			if (DelegateProp->SignatureFunction)
			{
				TArray<FString> ParamStrings;
				FProperty* ReturnProperty = nullptr;

				// 收集参数信息
				for (TFieldIterator<FProperty> ParamIt(DelegateProp->SignatureFunction); ParamIt; ++ParamIt)
				{
					FProperty* Param = *ParamIt;
					
					if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
					{
						ReturnProperty = Param;
						continue;
					}

					FString ParamStr = FString::Printf(TEXT("%s: %s"), 
						*Param->GetName(), 
						*GetPropertyTypeString(Param));

					if (Param->HasAnyPropertyFlags(CPF_OutParm))
					{
						ParamStr += TEXT(" [Out]");
					}
					if (Param->HasAnyPropertyFlags(CPF_ReferenceParm))
					{
						ParamStr += TEXT(" [Ref]");
					}
					if (Param->HasAnyPropertyFlags(CPF_ConstParm))
					{
						ParamStr += TEXT(" [Const]");
					}

					ParamStrings.Add(ParamStr);
				}

#if ENGINE_MAJOR_VERSION < 5
				const void* SignatureFunction = DelegateProp;
#else
				const void* SignatureFunction = DelegateProp->SignatureFunction.Get();
#endif

				FString ReturnType = ReturnProperty ? GetPropertyTypeString(ReturnProperty) : TEXT("void");
				DelegateInfo = FString::Printf(TEXT(" Signature=(%s) -> %s [Func=%p]"),
					*FString::Join(ParamStrings, TEXT(", ")),
					*ReturnType,
					SignatureFunction);
			}
			else
			{
				DelegateInfo = TEXT(" Signature=[nullptr]");
			}
		}
		else if (const FMulticastDelegateProperty* MulticastDelegateProp = CastField<FMulticastDelegateProperty>(Prop))
		{
			if (MulticastDelegateProp->SignatureFunction)
			{
				TArray<FString> ParamStrings;
				FProperty* ReturnProperty = nullptr;

				// 收集参数信息
				for (TFieldIterator<FProperty> ParamIt(MulticastDelegateProp->SignatureFunction); ParamIt; ++ParamIt)
				{
					FProperty* Param = *ParamIt;
					
					if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
					{
						ReturnProperty = Param;
						continue;
					}

					FString ParamStr = FString::Printf(TEXT("%s: %s"), 
						*Param->GetName(), 
						*GetPropertyTypeString(Param));

					if (Param->HasAnyPropertyFlags(CPF_OutParm))
					{
						ParamStr += TEXT(" [Out]");
					}
					if (Param->HasAnyPropertyFlags(CPF_ReferenceParm))
					{
						ParamStr += TEXT(" [Ref]");
					}
					if (Param->HasAnyPropertyFlags(CPF_ConstParm))
					{
						ParamStr += TEXT(" [Const]");
					}

					ParamStrings.Add(ParamStr);
				}

				FString ReturnType = ReturnProperty ? GetPropertyTypeString(ReturnProperty) : TEXT("void");
				
				// 判断委托类型（Inline, Sparse等）
				FString DelegateTypeStr;
				if (CastField<FMulticastInlineDelegateProperty>(MulticastDelegateProp))
				{
					DelegateTypeStr = TEXT("InlineMulticast");
				}
				else if (CastField<FMulticastSparseDelegateProperty>(MulticastDelegateProp))
				{
					DelegateTypeStr = TEXT("SparseMulticast");
				}
				else
				{
					DelegateTypeStr = TEXT("Multicast");
				}

#if ENGINE_MAJOR_VERSION < 5
				const void* SignatureFunction = MulticastDelegateProp;
#else
				const void* SignatureFunction = MulticastDelegateProp->SignatureFunction.Get();
#endif

				DelegateInfo = FString::Printf(TEXT(" Type=%s Signature=(%s) -> %s [Func=%p]"),
					*DelegateTypeStr,
					*FString::Join(ParamStrings, TEXT(", ")),
					*ReturnType,
					SignatureFunction);
			}
			else
			{
				DelegateInfo = TEXT(" Signature=[nullptr]");
			}
		}
		
		return FString::Printf(TEXT("%s: %s [Prop=%p, Owner=%p]%s%s"),
			UTF8_TO_TCHAR(PropName),
			*TypeString,
			Prop,
			Prop->GetOwnerUObject() ? (void*)Prop->GetOwnerUObject() : (void*)Prop->GetOwnerStruct(),
			FlagsString.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" [%s]"), *FlagsString),
			*DelegateInfo);
	}

	FString GetFunctionInfoString(const UFunction* Func, const char* FuncName)
	{
		if (!Func)
		{
			return FString::Printf(TEXT("%s: [nullptr]"), UTF8_TO_TCHAR(FuncName));
		}

		FString FlagsString = GetFunctionFlagsString(Func);
		
		return FString::Printf(TEXT("%s [Func=%p, Outer=%p]%s"),
			UTF8_TO_TCHAR(FuncName),
			Func,
			Func->GetOuter(),
			FlagsString.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" [%s]"), *FlagsString));
	}
}
