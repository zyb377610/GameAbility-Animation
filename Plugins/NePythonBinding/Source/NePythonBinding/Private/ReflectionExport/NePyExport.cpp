#include "ReflectionExport/NePyExport.h"

#if WITH_EDITOR
#include "ReflectionExport/NePyExportUtil.h"
#include "Containers/Array.h"
#include "Containers/Set.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFilemanager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "UObject/UObjectHash.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#include "StructUtils/UserDefinedStruct.h"
#endif
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/UserDefinedEnum.h"
#include "Interfaces/IPluginManager.h"
#include "NePyBase.h"

struct NePyDumpReflectionContext
{
	// 根据反射信息生成的类json描述
	TArray<TSharedPtr<FJsonValue>> ClassInfos;

	// 根据反射信息生成的结构体json描述
	TArray<TSharedPtr<FJsonValue>> StructInfos;

	// 根据反射信息生成的枚举json描述
	TArray<TSharedPtr<FJsonValue>> EnumInfos;

	// 根据反射信息生成的手动标注UHT头的结构体json描述
	TArray<TSharedPtr<FJsonValue>> NoExportTypeInfos;

	// 已被导出的类
	TSet<FName> ExportedClasses;

	// 已被导出的结构体
	TSet<FName> ExportedStructs;

	// 已被导出的枚举
	TSet<FName> ExportedEnums;

	//TSet<FName> ExportedDelegates;
};

static FString NePyNoExportTypePrefix = TEXT("NePyNoExportType_");

TSharedPtr<FJsonObject> NePyDumpOneFunctionInternal(const UFunction* InFunc, bool bAllowBlueprintFunctions);

TSharedPtr<FJsonObject> NePyDumpReflectionOneFunction(const UFunction* InFunc)
{
	return NePyDumpOneFunctionInternal(InFunc, false);
}

TSharedPtr<FJsonObject> NePyDumpBlueprintOneFunction(const UFunction* InFunc)
{
	return NePyDumpOneFunctionInternal(InFunc, true);
}

void NePyDumpOneClassInnerInternal(const UClass* InClass, NePyDumpReflectionContext& RefContext, bool bExportBlueprintFunctions);

void NePyDumpReflectionOneClassInner(const UClass* InClass, NePyDumpReflectionContext& RefContext)
{
	NePyDumpOneClassInnerInternal(InClass, RefContext, false);
}

void NePyDumpBlueprintOneClassInner(const UClass* InClass, NePyDumpReflectionContext& RefContext)
{
	NePyDumpOneClassInnerInternal(InClass, RefContext, true);
}

void NePyDumpReflectionOneNoExportTypeInner(const UClass* InClass, NePyDumpReflectionContext& RefContext);

TSharedPtr<FJsonObject> NePyDumpReflectionOneProperty(const FProperty* InProp, bool bForceExport)
{
	if (!bForceExport)
	{
		if (NePyExportUtil::IsDeprecatedProperty(InProp))
		{
			return nullptr;
		}

		//if (!NePyExportUtil::ShouldExportProperty(InProp))
		//{
		//	return nullptr;
		//}
	}

	TSharedRef<FJsonObject> PropInfo = MakeShared<FJsonObject>();

	PropInfo->SetStringField("name", InProp->GetName());
	PropInfo->SetStringField("pretty_name", NePyExportUtil::GetPropertyPythonName(InProp));
	PropInfo->SetNumberField("prop_flags", InProp->PropertyFlags);
	PropInfo->SetNumberField("array_dim", InProp->ArrayDim);

	FString PropDoc = NePyExportUtil::GetFieldTooltip(InProp);
	PropInfo->SetStringField("doc", PropDoc);

	const FString GetterName = InProp->GetMetaData(NePyExportUtil::BlueprintGetterMetaDataKey);
	if (!GetterName.IsEmpty())
	{
		PropInfo->SetStringField("getter", GetterName);
	}

	const FString SetterName = InProp->GetMetaData(NePyExportUtil::BlueprintSetterMetaDataKey);
	if (!SetterName.IsEmpty())
	{
		PropInfo->SetStringField("setter", SetterName);
	}

	PropInfo->SetStringField("type", InProp->GetClass()->GetName());
	if (const FByteProperty* ByteProp = CastField<const FByteProperty>(InProp))
	{
		if (ByteProp->Enum)
		{
			PropInfo->SetStringField("enum_name", ByteProp->Enum->GetName());
			PropInfo->SetStringField("enum_cpp_name", ByteProp->Enum->CppType);
			PropInfo->SetNumberField("enum_cpp_form", (int32)ByteProp->Enum->GetCppForm());
		}
	}
	if (const FEnumProperty* EnumProp = CastField<const FEnumProperty>(InProp))
	{
		PropInfo->SetStringField("enum_name", EnumProp->GetEnum()->GetName());
		PropInfo->SetStringField("enum_cpp_name", EnumProp->GetEnum()->CppType);
		PropInfo->SetNumberField("enum_cpp_form", (int32)EnumProp->GetEnum()->GetCppForm());
		PropInfo->SetStringField("enum_underlying_type", EnumProp->GetUnderlyingProperty()->GetClass()->GetName());
	}
	if (const FObjectPropertyBase* ObjProp = CastField<const FObjectPropertyBase>(InProp))
	{
		PropInfo->SetStringField("class_name", ObjProp->PropertyClass->GetName());
	}
	if (const FClassProperty* ClassProp = CastField<const FClassProperty>(InProp))
	{
		PropInfo->SetStringField("meta_class_name", ClassProp->MetaClass->GetName());
	}
	if (const FSoftClassProperty* SoftClassProp = CastField<const FSoftClassProperty>(InProp))
	{
		PropInfo->SetStringField("meta_class_name", SoftClassProp->MetaClass->GetName());
	}
	if (const FInterfaceProperty* InterfaceProp = CastField<const FInterfaceProperty>(InProp))
	{
		PropInfo->SetStringField("interface_name", InterfaceProp->InterfaceClass->GetName());
	}
	if (const FStructProperty* StructProp = CastField<const FStructProperty>(InProp))
	{
		PropInfo->SetStringField("struct_name", StructProp->Struct->GetName());
	}
	if (const FDelegateProperty* DelegateProp = CastField<const FDelegateProperty>(InProp))
	{
		auto FuncInfo = NePyDumpReflectionOneFunction(DelegateProp->SignatureFunction);
		if (FuncInfo)
		{
			PropInfo->SetObjectField("func_info", FuncInfo);
		}
	}
	if (const FMulticastDelegateProperty* MulticastDelegateProp = CastField<const FMulticastDelegateProperty>(InProp))
	{
		auto FuncInfo = NePyDumpReflectionOneFunction(MulticastDelegateProp->SignatureFunction);
		if (FuncInfo)
		{
			PropInfo->SetObjectField("func_info", FuncInfo);
		}
	}
	if (const FArrayProperty* ArrayProperty = CastField<const FArrayProperty>(InProp))
	{
		auto InnerPropInfo = NePyDumpReflectionOneProperty(ArrayProperty->Inner, true);
		if (InnerPropInfo)
		{
			PropInfo->SetObjectField("inner_prop", InnerPropInfo);
		}
	}
	if (const FSetProperty* SetProperty = CastField<const FSetProperty>(InProp))
	{
		auto ElementProp = NePyDumpReflectionOneProperty(SetProperty->ElementProp, true);
		if (ElementProp)
		{
			PropInfo->SetObjectField("element_prop", ElementProp);
		}
	}
	if (const FMapProperty* MapProperty = CastField<const FMapProperty>(InProp))
	{
		auto KeyProp = NePyDumpReflectionOneProperty(MapProperty->KeyProp, true);
		auto ValueProp = NePyDumpReflectionOneProperty(MapProperty->ValueProp, true);
		if (KeyProp && ValueProp)
		{
			PropInfo->SetObjectField("key_prop", KeyProp);
			PropInfo->SetObjectField("value_prop", ValueProp);
		}
	}

	return PropInfo;
}

TSharedPtr<FJsonObject> NePyDumpOneFunctionInternal(const UFunction* InFunc, bool bAllowBlueprintFunctions)
{
	if (!InFunc)
	{
		return nullptr;
	}

	// 非C++方法不导出
	if (!bAllowBlueprintFunctions && !InFunc->HasAnyFunctionFlags(FUNC_Native))
	{
		// Delegate比较特殊，它没有C++实现，但依旧需要导出
		if (!InFunc->HasAnyFunctionFlags(FUNC_Delegate))
		{
			return nullptr;
		}
	}

	if (NePyExportUtil::IsDeprecatedFunction(InFunc))
	{
		return nullptr;
	}

	TSharedRef<FJsonObject> FuncInfo = MakeShared<FJsonObject>();

	FuncInfo->SetStringField("name", NePyExportUtil::GetFunctionCPPName(InFunc));
	FuncInfo->SetStringField("pretty_name", NePyExportUtil::GetFunctionPythonName(InFunc));
	FuncInfo->SetNumberField("func_flags", InFunc->FunctionFlags);

	FString FuncDoc = NePyExportUtil::GetFieldTooltip(InFunc);
	FuncInfo->SetStringField("doc", FuncDoc);

	// 对于蓝图函数，添加额外的标记
	if (bAllowBlueprintFunctions)
	{
		FuncInfo->SetBoolField("is_blueprint_func", true);
	}

	TArray<TSharedPtr<FJsonValue>> ParamInfos;
	for (TFieldIterator<const FProperty> ParamIt(InFunc); ParamIt; ++ParamIt)
	{
		const FProperty* Param = *ParamIt;
		auto PropInfo = NePyDumpReflectionOneProperty(Param, true);
		if (PropInfo)
		{
			const FName DefaultValueMetaDataKey = *FString::Printf(TEXT("CPP_Default_%s"), *Param->GetName());
			if (InFunc->HasMetaData(DefaultValueMetaDataKey))
			{
				const FString& ParamDefaultValue = InFunc->GetMetaData(DefaultValueMetaDataKey);
				PropInfo->SetStringField("default", ParamDefaultValue);
			}
			ParamInfos.Add(MakeShared<FJsonValueObject>(PropInfo));
		}
	}
	FuncInfo->SetArrayField("params", ParamInfos);

	return FuncInfo;
}

void NePyDumpReflectionOneClass(const UClass* InClass, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	if (NePyExportUtil::IsBlueprintGeneratedClass(InClass))
	{
		// 蓝图类不导出
		return;
	}

	if (!bForceExport && NePyExportUtil::IsDeprecatedClass(InClass))
	{
		// 废弃类不导出
		return;
	}

	if (!bForceExport && !NePyExportUtil::ShouldExportClass(InClass))
	{
		// 没有标记为导出的类不导出
		return;
	}

	FName ClassName = InClass->GetFName();
	if (RefContext.ExportedClasses.Contains(ClassName))
	{
		// 已经导出过了
		return;
	}

	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		// 如果有基类则先导出基类
		NePyDumpReflectionOneClass(SuperClass, RefContext, true);
	}

	check(!RefContext.ExportedClasses.Contains(ClassName));
	RefContext.ExportedClasses.Add(ClassName);

	// 导出手写UHT标注的类
	
	if (ClassName.ToString().StartsWith(NePyNoExportTypePrefix, ESearchCase::CaseSensitive))
	{
		NePyDumpReflectionOneNoExportTypeInner(InClass, RefContext);
	}
	else
	{
		NePyDumpReflectionOneClassInner(InClass, RefContext);
	}
	
}

void NePyDumpOneClassInnerInternal(const UClass* InClass, NePyDumpReflectionContext& RefContext, bool bExportBlueprintFunctions)
{
	FName ClassName = InClass->GetFName();
	TSharedRef<FJsonObject> ClassInfo = MakeShared<FJsonObject>();

	ClassInfo->SetStringField("name", ClassName.ToString());
	ClassInfo->SetStringField("pretty_name", NePyExportUtil::GetClassPythonName(InClass));
	ClassInfo->SetStringField("package", InClass->GetOuter()->GetName());
	ClassInfo->SetNumberField("class_flags", InClass->ClassFlags);

	FString ClassDoc = NePyExportUtil::GetFieldTooltip(InClass);
	ClassInfo->SetStringField("doc", ClassDoc);

	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		ClassInfo->SetStringField("super", NePyExportUtil::GetClassPythonName(SuperClass));
	}

	// 标记是否为蓝图类
	if (bExportBlueprintFunctions && NePyExportUtil::IsBlueprintGeneratedClass(InClass))
	{
		ClassInfo->SetBoolField("is_blueprint_class", true);
	}

	// 导出属性
	TArray<TSharedPtr<FJsonValue>> PropInfos;
	for (TFieldIterator<const FField> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	{
		if (const FProperty* Prop = CastField<const FProperty>(*FieldIt))
		{
			auto PropInfo = NePyDumpReflectionOneProperty(Prop, bExportBlueprintFunctions);
			if (PropInfo)
			{
				PropInfos.Add(MakeShared<FJsonValueObject>(PropInfo));
			}
		}
	}
	ClassInfo->SetArrayField("props", PropInfos);

	// 导出方法
	TArray<TSharedPtr<FJsonValue>> FuncInfos;
	for (TFieldIterator<const UField> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	{
		if (const UFunction* Func = Cast<const UFunction>(*FieldIt))
		{
			TSharedPtr<FJsonObject> FuncInfo;
			if (bExportBlueprintFunctions)
			{
				FuncInfo = NePyDumpBlueprintOneFunction(Func);
			}
			else
			{
				FuncInfo = NePyDumpReflectionOneFunction(Func);
			}

			if (FuncInfo)
			{
				FuncInfos.Add(MakeShared<FJsonValueObject>(FuncInfo));
			}
		}
	}
	ClassInfo->SetArrayField("funcs", FuncInfos);

	RefContext.ClassInfos.Add(MakeShared<FJsonValueObject>(ClassInfo));
}

void NePyDumpReflectionOneNoExportTypeInner(const UClass* InClass, NePyDumpReflectionContext& RefContext)
{
	FName ClassName = InClass->GetFName();
	FString ClassRealName = ClassName.ToString().Mid(NePyNoExportTypePrefix.Len());
	TSharedRef<FJsonObject> ClassInfo = MakeShared<FJsonObject>();

	ClassInfo->SetStringField("name", ClassRealName);
	ClassInfo->SetStringField("pretty_name", ClassRealName);
	ClassInfo->SetStringField("package", NePyExportUtil::GetNoExportTypePackage(InClass));
	ClassInfo->SetNumberField("struct_flags", InClass->ClassFlags);

	FString ClassDoc = NePyExportUtil::GetFieldTooltip(InClass);
	ClassInfo->SetStringField("doc", ClassDoc);

	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		if (SuperClass->GetName().StartsWith(NePyNoExportTypePrefix, ESearchCase::CaseSensitive))
		{
			ClassInfo->SetStringField("super", SuperClass->GetName().Mid(NePyNoExportTypePrefix.Len()));
		}
	}

	// 导出属性
	TArray<TSharedPtr<FJsonValue>> PropInfos;
	for (TFieldIterator<const FField> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	{
		if (const FProperty* Prop = CastField<const FProperty>(*FieldIt))
		{
			auto PropInfo = NePyDumpReflectionOneProperty(Prop, false);
			if (PropInfo)
			{
				PropInfos.Add(MakeShared<FJsonValueObject>(PropInfo));
			}
		}
	}
	ClassInfo->SetArrayField("props", PropInfos);

	// 导出方法
	TArray<TSharedPtr<FJsonValue>> FuncInfos;
	for (TFieldIterator<const UField> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	{
		if (const UFunction* Func = Cast<const UFunction>(*FieldIt))
		{
			auto FuncInfo = NePyDumpReflectionOneFunction(Func);
			if (FuncInfo)
			{
				bool bFriendFunction = NePyExportUtil::IsFriendFunction(Func);
				FuncInfo->SetBoolField("is_friend_func", bFriendFunction);
				// 忽略友元函数的第一个参数
				if (bFriendFunction)
				{
					const TArray<TSharedPtr<FJsonValue>>* ParamInfos = nullptr;
					if (FuncInfo->TryGetArrayField(TEXT("params"), ParamInfos))
					{
						if (ParamInfos->Num() > 0)
						{
							(const_cast<TArray<TSharedPtr<FJsonValue>>*>(ParamInfos))->RemoveAt(0);
						}
					}
				}
				FuncInfos.Insert(MakeShared<FJsonValueObject>(FuncInfo), 0);
			}
		}
	}
	ClassInfo->SetArrayField("funcs", FuncInfos);

	RefContext.NoExportTypeInfos.Add(MakeShared<FJsonValueObject>(ClassInfo));

}

void NePyDumpReflectionOneStruct(const UScriptStruct* InStruct, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	if (NePyExportUtil::IsBlueprintGeneratedStruct(InStruct))
	{
		// 蓝图类不导出
		return;
	}

	if (!bForceExport && !NePyExportUtil::ShouldExportStruct(InStruct))
	{
		// 没有标记为导出的类不导出
		return;
	}

	FName StructName = InStruct->GetFName();
	if (RefContext.ExportedStructs.Contains(StructName))
	{
		// 已经导出过了
		return;
	}

	if (const UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct()))
	{
		// 如果有基类则先导出基类
		NePyDumpReflectionOneStruct(SuperStruct, RefContext, true);
	}

	check(!RefContext.ExportedStructs.Contains(StructName));
	RefContext.ExportedStructs.Add(StructName);

	TSharedRef<FJsonObject> StructInfo = MakeShared<FJsonObject>();

	StructInfo->SetStringField("name", StructName.ToString());
	StructInfo->SetStringField("pretty_name", NePyExportUtil::GetStructPythonName(InStruct));
	StructInfo->SetStringField("package", InStruct->GetOuter()->GetName());
	StructInfo->SetNumberField("struct_flags", InStruct->StructFlags);

	FString StructDoc = NePyExportUtil::GetFieldTooltip(InStruct);
	StructInfo->SetStringField("doc", StructDoc);

	if (const UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct()))
	{
		StructInfo->SetStringField("super", NePyExportUtil::GetStructPythonName(SuperStruct));
	}

	// 导出属性
	TArray<TSharedPtr<FJsonValue>> PropInfos;
	for (TFieldIterator<const FField> FieldIt(InStruct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	{
		if (const FProperty* Prop = CastField<const FProperty>(*FieldIt))
		{
			auto PropInfo = NePyDumpReflectionOneProperty(Prop, false);
			if (PropInfo)
			{
				PropInfos.Add(MakeShared<FJsonValueObject>(PropInfo));
			}
		}
	}
	StructInfo->SetArrayField("props", PropInfos);

	// Struct 好像没有方法
	//TArray<TSharedPtr<FJsonValue>> FuncInfos;
	//for (TFieldIterator<const UField> FieldIt(InStruct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	//{
	//	if (const UFunction* Func = Cast<const UFunction>(*FieldIt))
	//	{
	//		NePyDumpReflectionOneFunction(Func, FuncInfos);
	//	}
	//}
	//StructInfo->SetArrayField("funcs", FuncInfos);

	RefContext.StructInfos.Add(MakeShared<FJsonValueObject>(StructInfo));
}

void NePyDumpReflectionOneEnum(const UEnum* InEnum, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	if (NePyExportUtil::IsBlueprintGeneratedEnum(InEnum))
	{
		// 蓝图类不导出
		return;
	}

	if (!bForceExport && !NePyExportUtil::ShouldExportEnum(InEnum))
	{
		// 没有标记为导出的类不导出
		return;
	}

	FName EnumName = InEnum->GetFName();
	if (RefContext.ExportedEnums.Contains(EnumName))
	{
		// 已经导出过了
		return;
	}

	RefContext.ExportedEnums.Add(EnumName);

	TSharedRef<FJsonObject> EnumInfo = MakeShared<FJsonObject>();

	EnumInfo->SetStringField("name", EnumName.ToString());
	EnumInfo->SetStringField("pretty_name", NePyExportUtil::GetEnumPythonName(InEnum));
	EnumInfo->SetStringField("package", InEnum->GetOuter()->GetName());

	FString EnumDoc = NePyExportUtil::GetFieldTooltip(InEnum);
	EnumInfo->SetStringField("doc", EnumDoc);

	TArray<TSharedPtr<FJsonValue>> PairInfos;
	TArray<TSharedPtr<FJsonValue>> DocInfos;
	int32 NumEnums = InEnum->NumEnums();
	for (int32 i = 0; i < NumEnums; ++i)
	{
		TArray<TSharedPtr<FJsonValue>> PairInfo;
		FString Name = InEnum->GetNameStringByIndex(i);
		PairInfo.Add(MakeShared<FJsonValueString>(Name));
		int64 Value = InEnum->GetValueByIndex(i);
		PairInfo.Add(MakeShared<FJsonValueNumber>(Value));
		PairInfos.Add(MakeShared<FJsonValueArray>(PairInfo));
		FString ItemDoc = NePyExportUtil::GetEnumTooltip(InEnum, i);
		DocInfos.Add(MakeShared<FJsonValueString>(ItemDoc));
	}
	EnumInfo->SetArrayField("pairs", PairInfos);
	EnumInfo->SetArrayField("item_docs", DocInfos);

	RefContext.EnumInfos.Add(MakeShared<FJsonValueObject>(EnumInfo));
}

void DumpReflectionForObject(const UObject* InObj, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	if (const UClass* Class = Cast<const UClass>(InObj))
	{
		NePyDumpReflectionOneClass(Class, RefContext, bForceExport);
	}

	if (const UScriptStruct* Struct = Cast<const UScriptStruct>(InObj))
	{
		NePyDumpReflectionOneStruct(Struct, RefContext, bForceExport);
	}

	if (const UEnum* Enum = Cast<const UEnum>(InObj))
	{
		return NePyDumpReflectionOneEnum(Enum, RefContext, bForceExport);
	}

	//if (const UFunction* Func = Cast<const UFunction>(InObj))
	//{
	//	if (Func->HasAnyFunctionFlags(FUNC_Delegate))
	//	{
	//		return GenerateWrappedDelegateType(Func, OutGeneratedWrappedTypeReferences, OutDirtyModules, InGenerationFlags);
	//	}
	//}
}

TSharedRef<FJsonObject> NePyDumpReflectionInfos(bool bForceExport)
{
	NePyDumpReflectionContext Context;

	// 收集UObject的所有子类
	TArray<UObject*> ObjectsToProcess;
	GetObjectsOfClass(UObject::StaticClass(), ObjectsToProcess);

	for (UObject* ObjectToProcess : ObjectsToProcess)
	{
		DumpReflectionForObject(ObjectToProcess, Context, bForceExport);
	}

	TSharedRef<FJsonObject> ReflectionInfos = MakeShared<FJsonObject>();
	ReflectionInfos->SetArrayField("classes", Context.ClassInfos);
	ReflectionInfos->SetArrayField("structs", Context.StructInfos);
	ReflectionInfos->SetArrayField("enums", Context.EnumInfos);
	ReflectionInfos->SetArrayField("noexport", Context.NoExportTypeInfos);

	return ReflectionInfos;
}

void NePyDumpReflectionInfosToFile(const FString& InOutputDirectory)
{
	if (!FPaths::DirectoryExists(InOutputDirectory))
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.CreateDirectoryTree(*InOutputDirectory);
	}

	{
		TSharedRef<FJsonObject> ReflectionInfos = NePyDumpReflectionInfos(false);

		FString OutputString;
		auto Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ReflectionInfos, Writer);

		FString DumpFilePath = FPaths::Combine(*InOutputDirectory, TEXT("reflection_infos.json"));
		FFileHelper::SaveStringToFile(OutputString, *DumpFilePath, FFileHelper::EEncodingOptions::ForceUTF8);
	}

	{
		TSharedRef<FJsonObject> ReflectionInfos = NePyDumpReflectionInfos(true);

		FString OutputString;
		auto Writer = TJsonWriterFactory<>::Create(&OutputString);
		FJsonSerializer::Serialize(ReflectionInfos, Writer);

		FString DumpFilePath = FPaths::Combine(*InOutputDirectory, TEXT("reflection_infos_full.json"));
		FFileHelper::SaveStringToFile(OutputString, *DumpFilePath, FFileHelper::EEncodingOptions::ForceUTF8);
	}
}

FString NePyGetDefaultDumpReflectionInfosDirectory()
{
	// 获取 NePythonBinding 插件的目录
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("NePythonBinding"));
	if (!Plugin.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find NePythonBinding plugin"));
		// 如果找不到插件，返回备用路径
		FString FallbackPath = FPaths::Combine(*FPaths::ProjectPluginsDir(), TEXT("NePythonBinding/Tools/temp"));
		return FPaths::ConvertRelativePathToFull(FallbackPath);
	}

	// 构建路径：插件目录/Tools/temp
	FString PluginBaseDir = Plugin->GetBaseDir();
	FString OutputDirectory = FPaths::Combine(PluginBaseDir, TEXT("Tools/temp"));
	OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
	
	return OutputDirectory;
}

static FString NePyGeneratedPrefix = TEXT("NePyGenerated");

void NePyDumpBlueprintOneClass(const UClass* InClass, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	// 只处理蓝图生成类
	if (!NePyExportUtil::IsBlueprintGeneratedClass(InClass))
	{
		return;
	}

	if (InClass->GetClass()->GetName().StartsWith(NePyGeneratedPrefix))
	{
		// 跳过Python生成的类
		return;
	}

	// 跳过已废弃和临时的类
	if (InClass->HasAnyClassFlags(CLASS_NewerVersionExists | CLASS_Deprecated))
	{
		return;
	}

	FName ClassName = InClass->GetFName();
	if (RefContext.ExportedClasses.Contains(ClassName))
	{
		// 已经导出过了
		return;
	}

	// 先导出基类
	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		if (NePyExportUtil::IsBlueprintGeneratedClass(SuperClass))
		{
			// 基类也是蓝图类，递归导出
			NePyDumpBlueprintOneClass(SuperClass, RefContext, true);
		}
	}

	check(!RefContext.ExportedClasses.Contains(ClassName));
	RefContext.ExportedClasses.Add(ClassName);

	// 使用蓝图特定的导出逻辑来导出蓝图类
	NePyDumpBlueprintOneClassInner(InClass, RefContext);
}

void NePyDumpBlueprintOneStruct(const UScriptStruct* InStruct, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	// 只处理蓝图生成结构体
	if (!NePyExportUtil::IsBlueprintGeneratedStruct(InStruct))
	{
		return;
	}

	if (InStruct->GetClass()->GetName().StartsWith(NePyGeneratedPrefix))
	{
		// 跳过Python生成的结构
		return;
	}

	FName StructName = InStruct->GetFName();
	if (RefContext.ExportedStructs.Contains(StructName))
	{
		// 已经导出过了
		return;
	}

	// 先导出基类
	if (const UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct()))
	{
		if (NePyExportUtil::IsBlueprintGeneratedStruct(SuperStruct))
		{
			// 基类也是蓝图结构体，递归导出
			NePyDumpBlueprintOneStruct(SuperStruct, RefContext, true);
		}
	}

	check(!RefContext.ExportedStructs.Contains(StructName));
	RefContext.ExportedStructs.Add(StructName);

	// 导出蓝图结构体的信息
	TSharedRef<FJsonObject> StructInfo = MakeShared<FJsonObject>();

	StructInfo->SetStringField("name", StructName.ToString());
	StructInfo->SetStringField("pretty_name", NePyExportUtil::GetStructPythonName(InStruct));
	StructInfo->SetStringField("package", InStruct->GetOuter()->GetName());
	StructInfo->SetNumberField("struct_flags", InStruct->StructFlags);

	FString StructDoc = NePyExportUtil::GetFieldTooltip(InStruct);
	StructInfo->SetStringField("doc", StructDoc);

	if (const UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct()))
	{
		StructInfo->SetStringField("super", NePyExportUtil::GetStructPythonName(SuperStruct));
	}

	// 导出属性
	TArray<TSharedPtr<FJsonValue>> PropInfos;
	for (TFieldIterator<const FField> FieldIt(InStruct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::ExcludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces); FieldIt; ++FieldIt)
	{
		if (const FProperty* Prop = CastField<const FProperty>(*FieldIt))
		{
			auto PropInfo = NePyDumpReflectionOneProperty(Prop, true);
			if (PropInfo)
			{
				PropInfos.Add(MakeShared<FJsonValueObject>(PropInfo));
			}
		}
	}
	StructInfo->SetArrayField("props", PropInfos);

	RefContext.StructInfos.Add(MakeShared<FJsonValueObject>(StructInfo));
}

void NePyDumpBlueprintOneEnum(const UEnum* InEnum, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	// 只处理蓝图生成枚举
	if (!NePyExportUtil::IsBlueprintGeneratedEnum(InEnum))
	{
		return;
	}

	if (InEnum->GetClass()->GetName().StartsWith(NePyGeneratedPrefix))
	{
		// 跳过Python生成的枚举
		return;
	}

	FName EnumName = InEnum->GetFName();
	if (RefContext.ExportedEnums.Contains(EnumName))
	{
		// 已经导出过了
		return;
	}

	RefContext.ExportedEnums.Add(EnumName);

	TSharedRef<FJsonObject> EnumInfo = MakeShared<FJsonObject>();

	EnumInfo->SetStringField("name", EnumName.ToString());
	EnumInfo->SetStringField("pretty_name", NePyExportUtil::GetEnumPythonName(InEnum));
	EnumInfo->SetStringField("package", InEnum->GetOuter()->GetName());

	FString EnumDoc = NePyExportUtil::GetFieldTooltip(InEnum);
	EnumInfo->SetStringField("doc", EnumDoc);

	TArray<TSharedPtr<FJsonValue>> PairInfos;
	TArray<TSharedPtr<FJsonValue>> DocInfos;
	int32 NumEnums = InEnum->NumEnums();
	for (int32 i = 0; i < NumEnums; ++i)
	{
		TArray<TSharedPtr<FJsonValue>> PairInfo;
		FString Name = InEnum->GetNameStringByIndex(i);
		PairInfo.Add(MakeShared<FJsonValueString>(Name));
		int64 Value = InEnum->GetValueByIndex(i);
		PairInfo.Add(MakeShared<FJsonValueNumber>(Value));
		PairInfos.Add(MakeShared<FJsonValueArray>(PairInfo));
		FString ItemDoc = NePyExportUtil::GetEnumTooltip(InEnum, i);
		DocInfos.Add(MakeShared<FJsonValueString>(ItemDoc));
	}
	EnumInfo->SetArrayField("pairs", PairInfos);
	EnumInfo->SetArrayField("item_docs", DocInfos);

	RefContext.EnumInfos.Add(MakeShared<FJsonValueObject>(EnumInfo));
}

void DumpBlueprintForObject(const UObject* InObj, NePyDumpReflectionContext& RefContext, bool bForceExport)
{
	if (InObj->GetClass()->GetName().StartsWith(NePyGeneratedPrefix))
	{
		// 跳过Python生成的类/结构体/枚举
		return;
	}

	if (const UClass* Class = Cast<const UClass>(InObj))
	{
		NePyDumpBlueprintOneClass(Class, RefContext, bForceExport);
	}

	if (const UScriptStruct* Struct = Cast<const UScriptStruct>(InObj))
	{
		NePyDumpBlueprintOneStruct(Struct, RefContext, bForceExport);
	}

	if (const UEnum* Enum = Cast<const UEnum>(InObj))
	{
		NePyDumpBlueprintOneEnum(Enum, RefContext, bForceExport);
	}
}

static void LoadAllBlueprintAssetsIntoMemory()
{
	// 获取 Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// 等待 Asset Registry 扫描完成
	if (AssetRegistry.IsLoadingAssets())
	{
		UE_LOG(LogNePython, Display, TEXT("Waiting for Asset Registry to finish loading assets..."));
		AssetRegistry.WaitForCompletion();
	}

	// 查找所有蓝图资源
	TArray<FAssetData> BlueprintAssets;
	FARFilter Filter;	
#if ENGINE_MAJOR_VERSION >= 5
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
#else
	Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
#endif
	Filter.bRecursiveClasses = true;

	UE_LOG(LogNePython, Display, TEXT("Finding all Blueprint assets..."));

	AssetRegistry.GetAssets(Filter, BlueprintAssets);

	// 加载所有蓝图资源到内存
	int32 AssetIndex = 0;
	int32 AssetNum = 0;
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		// 获取蓝图的 GeneratedClass 路径
		FString GeneratedClassPath = AssetData.GetTagValueRef<FString>(FBlueprintTags::GeneratedClassPath);
		if (!GeneratedClassPath.IsEmpty())
		{
			UE_LOG(LogNePython, Display, TEXT("Find Blueprint GeneratedClass: %s"), *GeneratedClassPath);
			++AssetNum;
		}
	}
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		// 获取蓝图的 GeneratedClass 路径
		FString GeneratedClassPath = AssetData.GetTagValueRef<FString>(FBlueprintTags::GeneratedClassPath);
		if (!GeneratedClassPath.IsEmpty())
		{
			// 加载类到内存
			UClass* LoadedClass = LoadObject<UClass>(nullptr, *GeneratedClassPath);
			UE_LOG(LogNePython, Display, TEXT("Loaded Blueprint GeneratedClass: %s (%d/%d)"), *GeneratedClassPath, ++AssetIndex, AssetNum);
		}
	}

	UE_LOG(LogNePython, Display, TEXT("Finished loading all Blueprint assets into memory."));
}

TSharedRef<FJsonObject> NePyDumpBlueprintInfos()
{
	NePyDumpReflectionContext Context;

	// 先确保所有蓝图资源都加载到内存
	LoadAllBlueprintAssetsIntoMemory();

	// 收集所有蓝图生成类
	TArray<UObject*> BlueprintClasses;
	GetObjectsOfClass(UBlueprintGeneratedClass::StaticClass(), BlueprintClasses, true, RF_NoFlags);

	BlueprintClasses.RemoveAll([](UObject* ObjectToProcess)
	{
		return ObjectToProcess->GetClass()->GetName().StartsWith(NePyGeneratedPrefix);
	});

	for (UObject* ObjectToProcess : BlueprintClasses)
	{
		DumpBlueprintForObject(ObjectToProcess, Context, true);
	}

	// 收集蓝图生成的结构体
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	TArray<UObject*> BlueprintStructs;
	GetObjectsOfClass(UUserDefinedStruct::StaticClass(), BlueprintStructs, true, RF_NoFlags);

	BlueprintStructs.RemoveAll([](UObject* ObjectToProcess)
	{
		return ObjectToProcess->GetClass()->GetName().StartsWith(NePyGeneratedPrefix);
	});

	for (UObject* ObjectToProcess : BlueprintStructs)
	{
		DumpBlueprintForObject(ObjectToProcess, Context, true);
	}
#endif

	// 收集蓝图生成的枚举
	TArray<UObject*> BlueprintEnums;
	GetObjectsOfClass(UUserDefinedEnum::StaticClass(), BlueprintEnums, true, RF_NoFlags);

	BlueprintEnums.RemoveAll([](UObject* ObjectToProcess)
	{
		return ObjectToProcess->GetClass()->GetName().StartsWith(NePyGeneratedPrefix);
	});

	for (UObject* ObjectToProcess : BlueprintEnums)
	{
		DumpBlueprintForObject(ObjectToProcess, Context, true);
	}

	TSharedRef<FJsonObject> ReflectionInfos = MakeShared<FJsonObject>();
	ReflectionInfos->SetArrayField("classes", Context.ClassInfos);
	ReflectionInfos->SetArrayField("structs", Context.StructInfos);
	ReflectionInfos->SetArrayField("enums", Context.EnumInfos);
	ReflectionInfos->SetArrayField("noexport", Context.NoExportTypeInfos);

	return ReflectionInfos;
}

void NePyDumpBlueprintInfosToFile(const FString& InOutputFile)
{
	FString OutputFile = InOutputFile;
	if (OutputFile.IsEmpty())
	{
		FString OutputDirectory = NePyGetDefaultDumpReflectionInfosDirectory();
		OutputFile = FPaths::Combine(*OutputDirectory, TEXT("blueprint_infos_full.json"));
	}

	// 确保输出目录存在
	FString OutputDir = FPaths::GetPath(OutputFile);
	if (!FPaths::DirectoryExists(OutputDir))
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		PlatformFile.CreateDirectoryTree(*OutputDir);
	}

	TSharedRef<FJsonObject> BlueprintInfos = NePyDumpBlueprintInfos();

	FString OutputString;
	auto Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(BlueprintInfos, Writer);

	FFileHelper::SaveStringToFile(OutputString, *OutputFile, FFileHelper::EEncodingOptions::ForceUTF8);
}

#endif // WITH_EDITOR
