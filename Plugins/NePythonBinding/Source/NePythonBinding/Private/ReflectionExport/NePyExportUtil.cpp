#include "NePyExportUtil.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Class.h"
#include "UObject/Stack.h"
#include "UObject/Package.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/CoreRedirects.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/UnrealType.h"
#include "Engine/BlueprintCore.h"
#include "Engine/BlueprintGeneratedClass.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5 || ENGINE_MAJOR_VERSION >= 6
#include "StructUtils/UserDefinedStruct.h"
#else
#include "Engine/UserDefinedStruct.h"
#endif
#include "Engine/UserDefinedEnum.h"
#include "GameFramework/Actor.h"

#if WITH_EDITOR
namespace NePyExportUtil
{
	const FName OriginalNameMetaDataKey = TEXT("OriginalName");
	const FName ScriptNameMetaDataKey = TEXT("ScriptName");
	const FName ScriptNoExportMetaDataKey = TEXT("ScriptNoExport");
	const FName ScriptMethodMetaDataKey = TEXT("ScriptMethod");
	const FName ScriptMethodSelfReturnMetaDataKey = TEXT("ScriptMethodSelfReturn");
	const FName ScriptOperatorMetaDataKey = TEXT("ScriptOperator");
	const FName ScriptConstantMetaDataKey = TEXT("ScriptConstant");
	const FName ScriptConstantHostMetaDataKey = TEXT("ScriptConstantHost");
	const FName BlueprintTypeMetaDataKey = TEXT("BlueprintType");
	const FName NotBlueprintTypeMetaDataKey = TEXT("NotBlueprintType");
	const FName BlueprintSpawnableComponentMetaDataKey = TEXT("BlueprintSpawnableComponent");
	const FName BlueprintGetterMetaDataKey = TEXT("BlueprintGetter");
	const FName BlueprintSetterMetaDataKey = TEXT("BlueprintSetter");
	const FName BlueprintInternalUseOnlyMetaDataKey = TEXT("BlueprintInternalUseOnly");
	const FName CustomThunkMetaDataKey = TEXT("CustomThunk");
	const FName DeprecatedPropertyMetaDataKey = TEXT("DeprecatedProperty");
	const FName DeprecatedFunctionMetaDataKey = TEXT("DeprecatedFunction");
	const FName DeprecationMessageMetaDataKey = TEXT("DeprecationMessage");
	const FName HasNativeMakeMetaDataKey = TEXT("HasNativeMake");
	const FName HasNativeBreakMetaDataKey = TEXT("HasNativeBreak");
	const FName NativeBreakFuncMetaDataKey = TEXT("NativeBreakFunc");
	const FName NativeMakeFuncMetaDataKey = TEXT("NativeMakeFunc");
	const FName ReturnValueKey = TEXT("ReturnValue");
	const FName FriendFunctionKey = TEXT("bFriendFunction");
	const FName ScriptPackageKey = TEXT("Package");
	const TCHAR* HiddenMetaDataKey = TEXT("Hidden");

	bool IsScriptExposedClass(const UClass* InClass)
	{
		for (const UClass* ParentClass = InClass; ParentClass; ParentClass = ParentClass->GetSuperClass())
		{
			if (IsBlueprintGeneratedClass(ParentClass))
			{
				return true;
			}

			if (ParentClass->GetBoolMetaData(BlueprintTypeMetaDataKey) || ParentClass->HasMetaData(BlueprintSpawnableComponentMetaDataKey))
			{
				return true;
			}

			if (ParentClass->GetBoolMetaData(NotBlueprintTypeMetaDataKey))
			{
				return false;
			}
		}

		return false;
	}

	bool IsScriptExposedStruct(const UScriptStruct* InStruct)
	{
		for (const UScriptStruct* ParentStruct = InStruct; ParentStruct; ParentStruct = Cast<UScriptStruct>(ParentStruct->GetSuperStruct()))
		{
			if (IsBlueprintGeneratedStruct(ParentStruct))
			{
				return true;
			}

			if (ParentStruct->GetBoolMetaData(BlueprintTypeMetaDataKey))
			{
				return true;
			}

			if (ParentStruct->GetBoolMetaData(NotBlueprintTypeMetaDataKey))
			{
				return false;
			}
		}

		return false;
	}

	bool IsScriptExposedEnum(const UEnum* InEnum)
	{
		if (IsBlueprintGeneratedEnum(InEnum))
		{
			return true;
		}

		if (InEnum->GetBoolMetaData(BlueprintTypeMetaDataKey))
		{
			return true;
		}

		if (InEnum->GetBoolMetaData(NotBlueprintTypeMetaDataKey))
		{
			return false;
		}

		return true;
	}

	bool IsScriptExposedEnumEntry(const UEnum* InEnum, int32 InEnumEntryIndex)
	{
		return !InEnum->HasMetaData(HiddenMetaDataKey, InEnumEntryIndex);
	}

	bool IsScriptExposedProperty(const FProperty* InProp)
	{
		return !InProp->HasMetaData(ScriptNoExportMetaDataKey)
			&& InProp->HasAnyPropertyFlags(CPF_BlueprintVisible | CPF_BlueprintAssignable);
	}

	bool IsScriptExposedFunction(const UFunction* InFunc)
	{
		return !InFunc->HasMetaData(ScriptNoExportMetaDataKey)
			&& InFunc->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintEvent)
			&& !InFunc->HasMetaData(BlueprintGetterMetaDataKey)
			&& !InFunc->HasMetaData(BlueprintSetterMetaDataKey)
			&& !InFunc->HasMetaData(BlueprintInternalUseOnlyMetaDataKey)
			&& !InFunc->HasMetaData(CustomThunkMetaDataKey)
			&& !InFunc->HasMetaData(NativeBreakFuncMetaDataKey)
			&& !InFunc->HasMetaData(NativeMakeFuncMetaDataKey);
	}

	bool IsApiClass(const UClass* InClass)
	{
		return InClass->HasAnyClassFlags(CLASS_RequiredAPI) || InClass->HasAnyClassFlags(CLASS_MinimalAPI);
	}

	bool HasScriptExposedFields(const UStruct* InStruct)
	{
		for (TFieldIterator<const UField> FieldIt(InStruct); FieldIt; ++FieldIt)
		{
			if (const UFunction* Func = Cast<const UFunction>(*FieldIt))
			{
				if (IsScriptExposedFunction(Func))
				{
					return true;
				}
			}
		}

		for (TFieldIterator<const FField> FieldIt(InStruct); FieldIt; ++FieldIt)
		{
			if (const FProperty* Prop = CastField<const FProperty>(*FieldIt))
			{
				if (IsScriptExposedProperty(Prop))
				{
					return true;
				}
			}
		}

		return false;
	}

	bool IsBlueprintGeneratedClass(const UClass* InClass)
	{
		// Need to use IsA rather than IsChildOf since we want to test the type of InClass itself *NOT* the class instance represented by InClass
		const UObject* ClassObject = InClass;
		return ClassObject->IsA<UBlueprintGeneratedClass>();
	}

	bool IsBlueprintGeneratedStruct(const UScriptStruct* InStruct)
	{
		return InStruct->IsA<UUserDefinedStruct>();
	}

	bool IsBlueprintGeneratedEnum(const UEnum* InEnum)
	{
		return InEnum->IsA<UUserDefinedEnum>();
	}

	bool IsDeprecatedClass(const UClass* InClass)
	{
		return InClass->HasAnyClassFlags(CLASS_Deprecated);
	}

	bool IsDeprecatedProperty(const FProperty* InProp)
	{
		return InProp->HasMetaData(DeprecatedPropertyMetaDataKey);
	}

	bool IsDeprecatedFunction(const UFunction* InFunc)
	{
		return InFunc->HasMetaData(DeprecatedFunctionMetaDataKey);
	}

	bool IsFriendFunction(const UFunction* InFunc)
	{
		return InFunc->GetBoolMetaData(FriendFunctionKey);
	}

	FString GetNoExportTypePackage(const UClass* InClass)
	{
		return InClass->GetMetaData(ScriptPackageKey);
	}

	bool ShouldExportClass(const UClass* InClass)
	{
		return IsApiClass(InClass) && (IsScriptExposedClass(InClass) || HasScriptExposedFields(InClass));
	}

	bool ShouldExportStruct(const UScriptStruct* InStruct)
	{
		return IsScriptExposedStruct(InStruct) || HasScriptExposedFields(InStruct);
	}

	bool ShouldExportEnum(const UEnum* InEnum)
	{
		return IsScriptExposedEnum(InEnum);
	}

	bool ShouldExportEnumEntry(const UEnum* InEnum, int32 InEnumEntryIndex)
	{
		return IsScriptExposedEnumEntry(InEnum, InEnumEntryIndex);
	}

	bool ShouldExportProperty(const FProperty* InProp)
	{
		const bool bCanScriptExport = !InProp->HasMetaData(ScriptNoExportMetaDataKey); // Need to test this again here as IsScriptExposedProperty checks it internally, but IsDeprecatedProperty doesn't
		return bCanScriptExport && (IsScriptExposedProperty(InProp) || IsDeprecatedProperty(InProp));
	}

	bool ShouldExportEditorOnlyProperty(const FProperty* InProp)
	{
		const bool bCanScriptExport = !InProp->HasMetaData(ScriptNoExportMetaDataKey);
		return bCanScriptExport && GIsEditor && (InProp->HasAnyPropertyFlags(CPF_Edit) || IsDeprecatedProperty(InProp));
	}

	bool ShouldExportFunction(const UFunction* InFunc)
	{
		return IsScriptExposedFunction(InFunc);
	}

	bool IsValidName(const FString& InName)
	{
		if (InName.Len() == 0)
		{
			//if (OutError)
			//{
			//	*OutError = NSLOCTEXT("PyGenUtil", "InvalidName_EmptyName", "Name is empty");
			//}
			return false;
		}

		// Names must be a valid symbol (alnum or _ and doesn't start with a digit)

		const TCHAR* InvalidChar = InName.GetCharArray().FindByPredicate(
			[](const TCHAR InChar)
			{
				return !FChar::IsAlnum(InChar) && InChar != TEXT('_') && InChar != 0;
			});

		if (InvalidChar)
		{
			//if (OutError)
			//{
			//	*OutError = FText::Format(NSLOCTEXT("PyGenUtil", "InvalidName_RestrictedCharacter", "Name contains '{0}' which is invalid for Python"), FText::AsCultureInvariant(FString(1, InvalidChar)));
			//}
			return false;
		}

		if (FChar::IsDigit(InName[0]))
		{
			//if (OutError)
			//{
			//	*OutError = NSLOCTEXT("PyGenUtil", "InvalidName_LeadingDigit", "Name starts with a digit which is invalid for Python");
			//}
			return false;
		}

		return true;
	}

	bool GetFieldPythonNameFromMetaDataImpl(const FFieldVariant& InField, const FName InMetaDataKey, FString& OutFieldName)
	{
		// See if we have a name override in the meta-data
		if (!InMetaDataKey.IsNone())
		{
			OutFieldName = InField.IsUObject() ? InField.Get<UField>()->GetMetaData(InMetaDataKey) : InField.Get<FField>()->GetMetaData(InMetaDataKey);

			// This may be a semi-colon separated list - the first item is the one we want for the current name
			if (!OutFieldName.IsEmpty())
			{
				int32 SemiColonIndex = INDEX_NONE;
				if (OutFieldName.FindChar(TEXT(';'), SemiColonIndex))
				{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
					OutFieldName.RemoveAt(SemiColonIndex, OutFieldName.Len() - SemiColonIndex, EAllowShrinking::No);
#else
					OutFieldName.RemoveAt(SemiColonIndex, OutFieldName.Len() - SemiColonIndex, /*bAllowShrinking*/false);
#endif
				}

				if (!IsValidName(OutFieldName))
				{
					//REPORT_PYTHON_GENERATION_ISSUE(Error, TEXT("'%s' has an invalid '%s' meta-data value '%s': %s."), *FieldPathName, *InMetaDataKey.ToString(), *OutFieldName, *ValidationError.ToString());
					return false;
				}

				return true;
			}
		}

		return false;
	}

	FString GetFieldPythonNameImpl(const FFieldVariant& InField, const FName InMetaDataKey)
	{
		FString FieldName;

		// First see if we have a name override in the meta-data
		if (GetFieldPythonNameFromMetaDataImpl(InField, InMetaDataKey, FieldName))
		{
			return FieldName;
		}

		// Just use the field name if we have no meta-data
		{
			FieldName = InField.GetName();

			// Strip the "E" prefix from enum names
			if (InField.IsA<UEnum>() && FieldName.Len() >= 2 && FieldName[0] == TEXT('E') && FChar::IsUpper(FieldName[1]))
			{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
				FieldName.RemoveAt(0, 1, EAllowShrinking::No);
#else
				FieldName.RemoveAt(0, 1, /*bAllowShrinking*/false);
#endif
			}

			// Classes, structs, and enums will no longer have their C++ prefix at this point
			// These prefixes can prevent types that start with numbers from being a compile error in C++
			// Ideally we don't want those prefixes to be used for Python, but we must ensure a valid symbol name
			if (UObject* FieldObject = InField.ToUObject())
			{
				if (FieldName.Len() >= 0 && FChar::IsDigit(FieldName[0]))
				{
					if (const UClass* Class = Cast<UClass>(FieldObject))
					{
						FieldName.InsertAt(0, Class->IsChildOf<AActor>() ? TEXT('A') : TEXT('U'));
					}
					else if (FieldObject->IsA<UScriptStruct>())
					{
						FieldName.InsertAt(0, TEXT('F'));
					}
					else if (FieldObject->IsA<UEnum>())
					{
						FieldName.InsertAt(0, TEXT('E'));
					}
				}
			}
		}

		return FieldName;
	}

	FString GetClassPythonName(const UClass* InClass)
	{
		return GetFieldPythonNameImpl(InClass, ScriptNameMetaDataKey);
	}

	FString GetStructPythonName(const UScriptStruct* InStruct)
	{
		return GetFieldPythonNameImpl(InStruct, ScriptNameMetaDataKey);
	}

	FString GetEnumPythonName(const UEnum* InEnum)
	{
		return GetFieldPythonNameImpl(InEnum, ScriptNameMetaDataKey);
	}

	FString GetEnumEntryPythonName(const UEnum* InEnum, const int32 InEntryIndex)
	{
		FString EnumEntryName;

		// First see if we have a name override in the meta-data
		{
			EnumEntryName = InEnum->GetMetaData(TEXT("ScriptName"), InEntryIndex);

			// This may be a semi-colon separated list - the first item is the one we want for the current name
			if (!EnumEntryName.IsEmpty())
			{
				int32 SemiColonIndex = INDEX_NONE;
				if (EnumEntryName.FindChar(TEXT(';'), SemiColonIndex))
				{
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
					EnumEntryName.RemoveAt(SemiColonIndex, EnumEntryName.Len() - SemiColonIndex, EAllowShrinking::No);
#else
					EnumEntryName.RemoveAt(SemiColonIndex, EnumEntryName.Len() - SemiColonIndex, /*bAllowShrinking*/false);
#endif
				}

				if (!IsValidName(EnumEntryName))
				{
					//REPORT_PYTHON_GENERATION_ISSUE(Error, TEXT("'%s' has an invalid 'ScriptName' meta-data value '%s': %s."), *InEnum->GetPathName(), *EnumEntryName, *ValidationError.ToString());
					EnumEntryName.Empty();
				}
			}
		}

		// Just use the entry name if we have no meta-data
		if (EnumEntryName.IsEmpty())
		{
			EnumEntryName = InEnum->GetNameStringByIndex(InEntryIndex);
		}

		return EnumEntryName;
	}

	FString GetDelegatePythonName(const UFunction* InDelegateSignature)
	{
		return InDelegateSignature->GetName().LeftChop(19); // Trim the "__DelegateSignature" suffix from the name
	}

	FString GetFunctionPythonName(const UFunction* InFunc)
	{
		FString FuncName = GetFieldPythonNameImpl(InFunc, ScriptNameMetaDataKey);
		return FuncName;
	}

	FString GetFunctionCPPName(const UFunction* InFunc)
	{
		FString FuncName = InFunc->GetMetaData(OriginalNameMetaDataKey);
		if (FuncName.IsEmpty())
		{
			FuncName = InFunc->GetName();
		}
		return FuncName;
	}

	FString GetScriptMethodPythonName(const UFunction* InFunc)
	{
		FString ScriptMethodName;
		if (GetFieldPythonNameFromMetaDataImpl(InFunc, ScriptMethodMetaDataKey, ScriptMethodName))
		{
			return ScriptMethodName;
		}
		return GetFunctionPythonName(InFunc);
	}

	FString GetScriptConstantPythonName(const UFunction* InFunc)
	{
		FString ScriptConstantName;
		if (!GetFieldPythonNameFromMetaDataImpl(InFunc, ScriptConstantMetaDataKey, ScriptConstantName))
		{
			ScriptConstantName = GetFieldPythonNameImpl(InFunc, ScriptNameMetaDataKey);
		}
		return ScriptConstantName;
	}

	FString GetPropertyPythonName(const FProperty* InProp)
	{
		FString PropName = GetFieldPythonNameImpl(InProp, ScriptNameMetaDataKey);
		return PropName;
	}

	FString GetPropertyTypePythonName(const FProperty* InProp, const bool InIncludeUnrealNamespace, const bool InIsForDocString)
	{
#define GET_PROPERTY_TYPE(TYPE, VALUE)				\
		if (CastField<const TYPE>(InProp) != nullptr)	\
		{											\
			return (VALUE);							\
		}

		const TCHAR* UnrealNamespace = InIncludeUnrealNamespace ? TEXT("unreal.") : TEXT("");

		GET_PROPERTY_TYPE(FBoolProperty, TEXT("bool"))
			GET_PROPERTY_TYPE(FInt8Property, InIsForDocString ? TEXT("int8") : TEXT("int"))
			GET_PROPERTY_TYPE(FInt16Property, InIsForDocString ? TEXT("int16") : TEXT("int"))
			GET_PROPERTY_TYPE(FUInt16Property, InIsForDocString ? TEXT("uint16") : TEXT("int"))
			GET_PROPERTY_TYPE(FIntProperty, InIsForDocString ? TEXT("int32") : TEXT("int"))
			GET_PROPERTY_TYPE(FUInt32Property, InIsForDocString ? TEXT("uint32") : TEXT("int"))
			GET_PROPERTY_TYPE(FInt64Property, InIsForDocString ? TEXT("int64") : TEXT("int"))
			GET_PROPERTY_TYPE(FUInt64Property, InIsForDocString ? TEXT("uint64") : TEXT("int"))
			GET_PROPERTY_TYPE(FFloatProperty, TEXT("float"))
			GET_PROPERTY_TYPE(FDoubleProperty, InIsForDocString ? TEXT("double") : TEXT("float"))
			GET_PROPERTY_TYPE(FStrProperty, TEXT("str"))
			GET_PROPERTY_TYPE(FNameProperty, InIncludeUnrealNamespace ? TEXT("unreal.Name") : TEXT("Name"))
			GET_PROPERTY_TYPE(FTextProperty, InIncludeUnrealNamespace ? TEXT("unreal.Text") : TEXT("Text"))
			if (const FByteProperty* ByteProp = CastField<const FByteProperty>(InProp))
			{
				if (ByteProp->Enum)
				{
					return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetEnumPythonName(ByteProp->Enum));
				}
				else
				{
					return InIsForDocString ? TEXT("uint8") : TEXT("int");
				}
			}
		if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(InProp))
		{
			return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetEnumPythonName(EnumProp->GetEnum()));
		}
		if (InIsForDocString)
		{
			if (const FClassProperty* ClassProp = CastField<const FClassProperty>(InProp))
			{
				return FString::Printf(TEXT("type(%s%s)"), UnrealNamespace, *GetClassPythonName(ClassProp->PropertyClass));
			}
		}
		if (const FObjectPropertyBase* ObjProp = CastField<const FObjectPropertyBase>(InProp))
		{
			return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetClassPythonName(ObjProp->PropertyClass));
		}
		if (const FInterfaceProperty* InterfaceProp = CastField<const FInterfaceProperty>(InProp))
		{
			return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetClassPythonName(InterfaceProp->InterfaceClass));
		}
		if (const FStructProperty* StructProp = CastField<const FStructProperty>(InProp))
		{
			return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetStructPythonName(StructProp->Struct));
		}
		if (const FDelegateProperty* DelegateProp = CastField<const FDelegateProperty>(InProp))
		{
			return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetDelegatePythonName(DelegateProp->SignatureFunction));
		}
		if (const FMulticastDelegateProperty* MulticastDelegateProp = CastField<const FMulticastDelegateProperty>(InProp))
		{
			return FString::Printf(TEXT("%s%s"), UnrealNamespace, *GetDelegatePythonName(MulticastDelegateProp->SignatureFunction));
		}
		if (const FArrayProperty* ArrayProperty = CastField<const FArrayProperty>(InProp))
		{
			return FString::Printf(TEXT("%sArray(%s)"), UnrealNamespace, *GetPropertyTypePythonName(ArrayProperty->Inner, InIncludeUnrealNamespace, InIsForDocString));
		}
		if (const FSetProperty* SetProperty = CastField<const FSetProperty>(InProp))
		{
			return FString::Printf(TEXT("%sSet(%s)"), UnrealNamespace, *GetPropertyTypePythonName(SetProperty->ElementProp, InIncludeUnrealNamespace, InIsForDocString));
		}
		if (const FMapProperty* MapProperty = CastField<const FMapProperty>(InProp))
		{
			return FString::Printf(TEXT("%sMap(%s, %s)"), UnrealNamespace, *GetPropertyTypePythonName(MapProperty->KeyProp, InIncludeUnrealNamespace, InIsForDocString), *GetPropertyTypePythonName(MapProperty->ValueProp, InIncludeUnrealNamespace, InIsForDocString));
		}

		return InIsForDocString ? TEXT("'undefined'") : TEXT("type");

#undef GET_PROPERTY_TYPE
	}

	template <typename FieldType>
	FString GetFieldTooltipImpl(const FieldType* InField)
	{
		FString DocString = *FTextInspector::GetSourceString(InField->GetToolTipText());
		DocString = DocString.Replace(TEXT("\""), TEXT("'"));
		DocString = DocString.Replace(TEXT("\\"), TEXT("/"));
		return DocString;
	}

	FString GetFieldTooltip(const UField* InField)
	{
		return GetFieldTooltipImpl(InField);
	}

	FString GetFieldTooltip(const FField* InField)
	{
		return GetFieldTooltipImpl(InField);
	}

	FString GetEnumTooltip(const UEnum* InEnum, int32 NameIndex)
	{
		FString DocString = InEnum->GetMetaData(TEXT("ToolTip"), NameIndex);
		DocString = DocString.Replace(TEXT("\""), TEXT("'"));
		DocString = DocString.Replace(TEXT("\\"), TEXT("/"));
		return DocString;
	}

}

#endif
