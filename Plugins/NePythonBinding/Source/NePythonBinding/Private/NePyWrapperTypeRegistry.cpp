#include "NePyWrapperTypeRegistry.h"
#include "NePyObjectBase.h"
#include "NePyStructBase.h"
#include "NePyUserStruct.h"
#include "NePyEnumBase.h"
#include "NePyGeneratedClass.h"
#include "NePyGeneratedStruct.h"
#include "NePyGeneratedEnum.h"
#include "NePyDynamicType.h"
#include "NePyDescriptor.h"
#include "NePyHouseKeeper.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5 || ENGINE_MAJOR_VERSION >= 6
#include "StructUtils/UserDefinedStruct.h"
#else
#include "Engine/UserDefinedStruct.h"
#endif
#include "Engine/UserDefinedEnum.h"
#if WITH_EDITOR
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintCompilationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "Kismet2/ReloadUtilities.h"
#include "Kismet2/KismetReinstanceUtilities.h"
#endif // ENGINE_MAJOR_VERSION >= 5
#endif // WITH_EDITOR

// Console variable to control subclassing generation logging
TAutoConsoleVariable<int32> CVarSubclassingLog(
	TEXT("nepy.subclassing.log"),
	0,
	TEXT("Controls logging of Python subclassing generation.\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Log summary (class/struct name, counts of added descriptors)\n")
	TEXT("2: Log detailed information for each added property and function"),
	ECVF_Default
);

FNePyObjectBase* FNePyObjectTypeInfo::NewObject(UObject* InObject) const
{
	if (NewFunc)
	{
		return NewFunc(InObject, TypeObject);
	}
	return nullptr;
}

void FNePyObjectTypeInfo::InitObject(FNePyObjectBase* InPyObject) const
{
	if (InitFunc)
	{
		InitFunc(InPyObject);
	}
	return;
}

FNePyWrapperTypeRegistry::FNePyWrapperTypeRegistry()
{
}

FNePyWrapperTypeRegistry& FNePyWrapperTypeRegistry::Get()
{
	static FNePyWrapperTypeRegistry Instance;
	return Instance;
}

void FNePyWrapperTypeRegistry::RegisterWrappedClassType(const UClass* InClass, const FNePyObjectTypeInfo& InPyTypeInfo)
{
	check(IsInGameThread());
	checkf(!PythonWrappedClasses.Contains(InClass), TEXT("RegisterWrappedClassType multiple times!"));

	// Create shared pointer
	TSharedPtr<FNePyObjectTypeInfo> SharedTypeInfo = MakeShared<FNePyObjectTypeInfo>(InPyTypeInfo);

	// Add to registry
	PythonWrappedClasses.Add(InClass, SharedTypeInfo);

	// Log registration with pointer information
	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("RegisterWrappedClassType: %s [UClass=%p]"), 
			*InClass->GetName(), 
			InClass);
		UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s, refcount=%d"), 
			InPyTypeInfo.TypeObject, 
			UTF8_TO_TCHAR(InPyTypeInfo.TypeObject->tp_name),
			(int32)Py_REFCNT(InPyTypeInfo.TypeObject));
		UE_LOG(LogNePython, Log, TEXT("  TypeFlags=%d, SharedPtr=%p, UseCount=%d"), 
			(int32)InPyTypeInfo.TypeFlags,
			SharedTypeInfo.Get(),
			SharedTypeInfo.GetSharedReferenceCount());
		UE_LOG(LogNePython, Log, TEXT("  NewFunc=%p, InitFunc=%p"), 
			(void*)InPyTypeInfo.NewFunc,
			(void*)InPyTypeInfo.InitFunc);
		UE_LOG(LogNePython, Log, TEXT("  Total Wrapped Classes: %d"), 
			PythonWrappedClasses.Num());
	}

	checkf(!PyTypeToClass.Contains(InPyTypeInfo.TypeObject), TEXT("RegisterWrappedClassType multiple times!"));
	PyTypeToClass.Add(InPyTypeInfo.TypeObject, InClass);

	if(EnumHasAnyFlags(InPyTypeInfo.TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType))
	{
		Py_INCREF(InPyTypeInfo.TypeObject);

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Incremented PyType refcount, new refcount=%d"), 
				(int32)Py_REFCNT(InPyTypeInfo.TypeObject));
		}
	}

	// 静态导出的类型有可能会因为更新不及时而缺失很多域，并且静态导出本来就由于各种限制无法导出完整的类
	// 因此在这里做一个运行时的域补全
	if (EnumHasAnyFlags(InPyTypeInfo.TypeFlags, ENePyTypeFlags::StaticPyType))
	{
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Static type detected, generating attributes..."));
		}
		GeneratePyAttrDescForClass(InClass, InPyTypeInfo.TypeObject, true);
	}
}

void FNePyWrapperTypeRegistry::RegisterWrappedStructType(const UScriptStruct* InScriptStruct, const FNePyStructTypeInfo& InPyTypeInfo)
{
	check(IsInGameThread());
	checkf(!PythonWrappedStructs.Contains(InScriptStruct), TEXT("RegisterWrappedStructType multiple times!"));

	// Create shared pointer
	TSharedPtr<FNePyStructTypeInfo> SharedTypeInfo = MakeShared<FNePyStructTypeInfo>(InPyTypeInfo);

	// Add to registry
	PythonWrappedStructs.Add(InScriptStruct, SharedTypeInfo);

	// Log registration with pointer information
	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("RegisterWrappedStructType: %s [UScriptStruct=%p]"), 
			*InScriptStruct->GetName(), 
			InScriptStruct);
		UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s, tp_basicsize=%d"), 
			InPyTypeInfo.TypeObject, 
			UTF8_TO_TCHAR(InPyTypeInfo.TypeObject->tp_name),
			(int32)InPyTypeInfo.TypeObject->tp_basicsize);
		UE_LOG(LogNePython, Log, TEXT("  TypeFlags=%d, SharedPtr=%p, UseCount=%d"), 
			(int32)InPyTypeInfo.TypeFlags,
			SharedTypeInfo.Get(),
			SharedTypeInfo.GetSharedReferenceCount());
		UE_LOG(LogNePython, Log, TEXT("  PropSetFunc=%p, PropGetFunc=%p"), 
			(void*)InPyTypeInfo.PropSetFunc,
			(void*)InPyTypeInfo.PropGetFunc);
		UE_LOG(LogNePython, Log, TEXT("  Struct Size: %d bytes, Calculated Python Size: %d bytes"), 
			InScriptStruct->GetStructureSize(),
			FNePyStructBase::Size());
		UE_LOG(LogNePython, Log, TEXT("  Total Wrapped Structs: %d"), 
			PythonWrappedStructs.Num());
	}

	checkf(!PyTypeToStruct.Contains(InPyTypeInfo.TypeObject), TEXT("RegisterWrappedStructType multiple times!"));
	PyTypeToStruct.Add(InPyTypeInfo.TypeObject, InScriptStruct);

	if(EnumHasAnyFlags(InPyTypeInfo.TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType))
	{
		check(!EnumHasAnyFlags(InScriptStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
		Py_INCREF(InPyTypeInfo.TypeObject);

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Incremented PyType refcount, new refcount=%d"), 
				(int32)Py_REFCNT(InPyTypeInfo.TypeObject));
		}
	}
	else
	{
		check(EnumHasAnyFlags(InScriptStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
		// NEPY会将结构体*存放*到PyObject中，而不是旧实现的*内联*到PyObject里，因此下面断言的语义跟之前不同
		// 旧实现需要关注的情况（备忘）：
		//		一些类（例如FFloatRK4SpringInterpolator）并没有将自己的所有成员导出，造成了sizeof(T) > ScriptStruct->GetPropertiesSize()的情况
		check(InPyTypeInfo.TypeObject->tp_basicsize >= FNePyStructBase::CalcPythonStructSize(InScriptStruct));
	}
}

void FNePyWrapperTypeRegistry::RegisterWrappedEnumType(const UEnum* InEnum, const FNePyEnumTypeInfo& InPyTypeInfo)
{
	check(IsInGameThread());
	checkf(!PythonWrappedEnums.Contains(InEnum), TEXT("RegisterWrappedEnumType multiple times!"));

	// Create shared pointer
	TSharedPtr<FNePyEnumTypeInfo> SharedTypeInfo = MakeShared<FNePyEnumTypeInfo>(InPyTypeInfo);

	// Add to registry
	PythonWrappedEnums.Add(InEnum, SharedTypeInfo);

	// Log registration with pointer information
	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("RegisterWrappedEnumType: %s [UEnum=%p]"), 
			*InEnum->GetName(), 
			InEnum);
		UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s"), 
			InPyTypeInfo.TypeObject, 
			UTF8_TO_TCHAR(InPyTypeInfo.TypeObject->tp_name));
		UE_LOG(LogNePython, Log, TEXT("  TypeFlags=%d, SharedPtr=%p, UseCount=%d"), 
			(int32)InPyTypeInfo.TypeFlags,
			SharedTypeInfo.Get(),
			SharedTypeInfo.GetSharedReferenceCount());
		UE_LOG(LogNePython, Log, TEXT("  Enum Values: %d"), 
			InEnum->NumEnums());
		UE_LOG(LogNePython, Log, TEXT("  Total Wrapped Enums: %d"), 
			PythonWrappedEnums.Num());
	}

	checkf(!PyTypeToEnum.Contains(InPyTypeInfo.TypeObject), TEXT("RegisterWrappedEnumType multiple times!"));
	PyTypeToEnum.Add(InPyTypeInfo.TypeObject, InEnum);

	if (EnumHasAnyFlags(InPyTypeInfo.TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType))
	{
		Py_INCREF(InPyTypeInfo.TypeObject);

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Incremented PyType refcount, new refcount=%d"), 
				(int32)Py_REFCNT(InPyTypeInfo.TypeObject));
		}
	}
}

void FNePyWrapperTypeRegistry::UnregisterWrappedClassType(const UClass* InClass)
{	
	check(IsInGameThread());	
	
	TSharedPtr<FNePyObjectTypeInfo> PyTypeInfo = nullptr;
	if (!PythonWrappedClasses.RemoveAndCopyValue(InClass, PyTypeInfo))
	{
		return;
	}

	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("UnregisterWrappedClassType: %s [UClass=%p]"), 
			*InClass->GetName(), 
			InClass);
		UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s, refcount=%d"), 
			PyTypeInfo->TypeObject, 
			UTF8_TO_TCHAR(PyTypeInfo->TypeObject->tp_name),
			(int32)Py_REFCNT(PyTypeInfo->TypeObject));
		UE_LOG(LogNePython, Log, TEXT("  SharedPtr=%p, UseCount=%d"), 
			PyTypeInfo.Get(),
			PyTypeInfo.GetSharedReferenceCount());
		UE_LOG(LogNePython, Log, TEXT("  Remaining Wrapped Classes: %d"), 
			PythonWrappedClasses.Num());
	}

	check(PyTypeInfo->TypeObject);
	check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType));
	PyTypeToClass.FindAndRemoveChecked(PyTypeInfo->TypeObject);

	{
		FNePyScopedGIL GIL;
		NePyType_CleanupDescriptors(PyTypeInfo->TypeObject);
		Py_DECREF(PyTypeInfo->TypeObject);
		
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Decremented PyType refcount, new refcount=%d"), 
				(int32)Py_REFCNT(PyTypeInfo->TypeObject));
		}
	}
}

void FNePyWrapperTypeRegistry::UnregisterWrappedStructType(const UScriptStruct* InScriptStruct)
{
	check(IsInGameThread());	
	TSharedPtr<FNePyStructTypeInfo> PyTypeInfo = nullptr;
	if (!PythonWrappedStructs.RemoveAndCopyValue(InScriptStruct, PyTypeInfo))
	{
		return;
	}

	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("UnregisterWrappedStructType: %s [UScriptStruct=%p]"), 
			*InScriptStruct->GetName(), 
			InScriptStruct);
		UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s, refcount=%d"), 
			PyTypeInfo->TypeObject, 
			UTF8_TO_TCHAR(PyTypeInfo->TypeObject->tp_name),
			(int32)Py_REFCNT(PyTypeInfo->TypeObject));
		UE_LOG(LogNePython, Log, TEXT("  SharedPtr=%p, UseCount=%d"), 
			PyTypeInfo.Get(),
			PyTypeInfo.GetSharedReferenceCount());
		UE_LOG(LogNePython, Log, TEXT("  Remaining Wrapped Structs: %d"), 
			PythonWrappedStructs.Num());
	}

	check(PyTypeInfo->TypeObject);
	check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType));
	check(!EnumHasAnyFlags(InScriptStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
	PyTypeToStruct.FindAndRemoveChecked(PyTypeInfo->TypeObject);

	{
		FNePyScopedGIL GIL;
		NePyType_CleanupDescriptors(PyTypeInfo->TypeObject);
		Py_DECREF(PyTypeInfo->TypeObject);

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Decremented PyType refcount, new refcount=%d"), 
				(int32)Py_REFCNT(PyTypeInfo->TypeObject));
		}
	}
}

void FNePyWrapperTypeRegistry::UnregisterWrappedEnumType(const UEnum* InEnum)
{
	check(IsInGameThread());	
	TSharedPtr<FNePyEnumTypeInfo> PyTypeInfo = nullptr;
	if (!PythonWrappedEnums.RemoveAndCopyValue(InEnum, PyTypeInfo))
	{
		return;
	}

	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("UnregisterWrappedEnumType: %s [UEnum=%p]"), 
			*InEnum->GetName(), 
			InEnum);
		UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s, refcount=%d"), 
			PyTypeInfo->TypeObject, 
			UTF8_TO_TCHAR(PyTypeInfo->TypeObject->tp_name),
			(int32)Py_REFCNT(PyTypeInfo->TypeObject));
		UE_LOG(LogNePython, Log, TEXT("  SharedPtr=%p, UseCount=%d"), 
			PyTypeInfo.Get(),
			PyTypeInfo.GetSharedReferenceCount());
		UE_LOG(LogNePython, Log, TEXT("  Remaining Wrapped Enums: %d"), 
			PythonWrappedEnums.Num());
	}

	check(PyTypeInfo->TypeObject);
	check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType));
	PyTypeToEnum.FindAndRemoveChecked(PyTypeInfo->TypeObject);

	{
		FNePyScopedGIL GIL;
		Py_DECREF(PyTypeInfo->TypeObject);

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Decremented PyType refcount, new refcount=%d"), 
				(int32)Py_REFCNT(PyTypeInfo->TypeObject));
		}
	}
}

bool FNePyWrapperTypeRegistry::HasWrappedObjectType(const UObject* InObject) const
{
	check(IsInGameThread());
	const UClass* Class = InObject->GetClass();
	return HasWrappedClassType(Class);
}

bool FNePyWrapperTypeRegistry::HasWrappedClassType(const UClass* InClass) const
{
	check(IsInGameThread());
	return PythonWrappedClasses.Contains(InClass);
}

bool FNePyWrapperTypeRegistry::HasWrappedStructType(const UScriptStruct* InScriptStruct) const
{
	check(IsInGameThread());
	return PythonWrappedStructs.Contains(InScriptStruct);
}

bool FNePyWrapperTypeRegistry::HasWrappedEnumType(const UEnum* InEnum)
{
	check(IsInGameThread());
	return PythonWrappedEnums.Contains(InEnum);
}

const FNePyObjectTypeInfo* FNePyWrapperTypeRegistry::GetWrappedObjectType(const UObject* InObject)
{
	check(IsInGameThread());
	check(InObject);
	const UClass* Class = InObject->GetClass();
	return GetWrappedClassType(Class);
}

const FNePyObjectTypeInfo* FNePyWrapperTypeRegistry::GetWrappedClassType(const UClass* InClass)
{
	check(IsInGameThread());
	check(InClass);
	const UClass* Class = InClass;
	const FNePyObjectTypeInfo* PyTypeInfo = PythonWrappedClasses.Contains(InClass) ? PythonWrappedClasses.Find(Class)->Get() : nullptr;
	if (PyTypeInfo)
	{
		return PyTypeInfo;
	}

	PyTypeInfo = GenerateDynamicTypeForClass(Class);
	if (PyTypeInfo)
	{
		return PyTypeInfo;
	}

	return nullptr;
}

const FNePyObjectTypeInfo* FNePyWrapperTypeRegistry::GetWrappedClassTypeIfExist(const UClass* InClass) const
{
	check(IsInGameThread());
	return PythonWrappedClasses.Contains(InClass) ? PythonWrappedClasses.Find(InClass)->Get() : nullptr;
}

const FNePyStructTypeInfo* FNePyWrapperTypeRegistry::GetWrappedStructType(const UScriptStruct* InScriptStruct)
{
	check(IsInGameThread());
	check(InScriptStruct);
	const UScriptStruct* ScriptStruct = InScriptStruct;
	const FNePyStructTypeInfo* PyTypeInfo = PythonWrappedStructs.Contains(InScriptStruct) ? PythonWrappedStructs.Find(ScriptStruct)->Get() : nullptr;
	if (PyTypeInfo)
	{
		return PyTypeInfo;
	}

	PyTypeInfo = GenerateDynamicTypeForStruct(ScriptStruct);
	if (PyTypeInfo)
	{
		return PyTypeInfo;
	}

	return nullptr;
}

const FNePyStructTypeInfo* FNePyWrapperTypeRegistry::GetWrappedStructTypeIfExist(const UScriptStruct* InScriptStruct) const
{
	check(IsInGameThread());
	return PythonWrappedStructs.Contains(InScriptStruct) ? PythonWrappedStructs.Find(InScriptStruct)->Get() : nullptr;
}

const FNePyEnumTypeInfo* FNePyWrapperTypeRegistry::GetWrappedEnumType(const UEnum* InEnum)
{
	check(IsInGameThread());
	check(InEnum);
	const UEnum* Enum = InEnum;
	const FNePyEnumTypeInfo* PyTypeInfo = PythonWrappedEnums.Contains(Enum) ? PythonWrappedEnums.Find(Enum)->Get() : nullptr;
	if (PyTypeInfo)
	{
		return PyTypeInfo;
	}

	PyTypeInfo = GenerateDynamicTypeForEnum(Enum);
	if (PyTypeInfo)
	{
		return PyTypeInfo;
	}

	return nullptr;
}

const FNePyEnumTypeInfo* FNePyWrapperTypeRegistry::GetWrappedEnumTypeIfExist(const UEnum* InEnum) const
{
	check(IsInGameThread());
	return PythonWrappedEnums.Contains(InEnum) ? PythonWrappedEnums.Find(InEnum)->Get() : nullptr;
}

const UClass* FNePyWrapperTypeRegistry::GetClassByPyType(const PyTypeObject* InPyType) const
{
	check(IsInGameThread());
	auto Result = PyTypeToClass.Find(InPyType);
	if (Result)
	{
		return *Result;
	}
	return nullptr;
}

const UScriptStruct* FNePyWrapperTypeRegistry::GetStructByPyType(const PyTypeObject* InPyType) const
{
	check(IsInGameThread());
	auto Result = PyTypeToStruct.Find(InPyType);
	if (Result)
	{
		return *Result;
	}
	Result = ReinstancingPyTypeToStruct.Find(InPyType);
	if (Result)
	{
		return *Result;
	}
	return nullptr;
}

const UEnum* FNePyWrapperTypeRegistry::GetEnumByPyType(const PyTypeObject* InPyType) const
{
	check(IsInGameThread());
	auto Result = PyTypeToEnum.Find(InPyType);
	if (Result)
	{
		return *Result;
	}
	return nullptr;
}

void FNePyWrapperTypeRegistry::ResetPyTypeDescriptors(const UClass* Class, bool bRecreateDescriptors) const
{
	check(IsInGameThread());
	const FNePyObjectTypeInfo* PyTypeInfo = GetWrappedClassTypeIfExist(Class);
	if (!PyTypeInfo)
	{
		return;
	}

	check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType));
	check(PyTypeInfo->TypeObject);
	PyTypeObject* PyType = PyTypeInfo->TypeObject;

	FNePyScopedGIL GIL;

	// 清理旧的Descriptor
	NePyType_CleanupDescriptors(PyType);

	// 重新生成Descriptor
	if (bRecreateDescriptors)
	{
		check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::BlueprintPyType));
		GeneratePyAttrDescForClass(Class, PyType, false);
	}

	PyType_Modified(PyType);
}

void FNePyWrapperTypeRegistry::ResetPyTypeDescriptors(const UScriptStruct* ScriptStruct, bool bRecreateDescriptors) const
{
	check(IsInGameThread());
	const FNePyStructTypeInfo* PyTypeInfo = GetWrappedStructTypeIfExist(ScriptStruct);
	if (!PyTypeInfo)
	{
		return;
	}

	check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::ScriptPyType | ENePyTypeFlags::BlueprintPyType));
	check(PyTypeInfo->TypeObject);
	PyTypeObject* PyType = PyTypeInfo->TypeObject;

	FNePyScopedGIL GIL;

	// 清理旧的Descriptor
	NePyType_CleanupDescriptors(PyType);

	// 重新生成Descriptor
	if (bRecreateDescriptors)
	{
		check(EnumHasAnyFlags(PyTypeInfo->TypeFlags, ENePyTypeFlags::BlueprintPyType));
		check(ScriptStruct->IsA<UUserDefinedStruct>());
		for (TFieldIterator<FProperty> FieldIt(ScriptStruct, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
		{
			FProperty* Prop = *FieldIt;
			FString PropName = ScriptStruct->GetAuthoredNameForField(Prop);
			NePyStealReference(NePyType_AddNewProperty(PyType, Prop, TCHAR_TO_UTF8(*PropName)));
		}
	}

	PyType_Modified(PyType);
}

TArray<UNePyGeneratedClass*> FNePyWrapperTypeRegistry::GetAllGeneratedClasses() const
{
	TArray<UNePyGeneratedClass*> Result;
	for (auto& Pair : PythonWrappedClasses)
	{
		if (const UNePyGeneratedClass* GeneratedClass = Cast<UNePyGeneratedClass>(Pair.Key))
		{
			Result.Add(const_cast<UNePyGeneratedClass*>(GeneratedClass));
		}
	}
	return Result;
}

TArray<UNePyGeneratedStruct*> FNePyWrapperTypeRegistry::GetAllGeneratedStructs() const
{
	TArray<UNePyGeneratedStruct*> Result;
	for (auto& Pair : PythonWrappedStructs)
	{
		if (const UNePyGeneratedStruct* GeneratedStruct = Cast<UNePyGeneratedStruct>(Pair.Key))
		{
			Result.Add(const_cast<UNePyGeneratedStruct*>(GeneratedStruct));
		}
	}
	return Result;
}

TArray<UNePyGeneratedEnum*> FNePyWrapperTypeRegistry::GetAllGeneratedEnums() const
{
	TArray<UNePyGeneratedEnum*> Result;
	for (auto& Pair : PythonWrappedEnums)
	{
		if (const UNePyGeneratedEnum* GeneratedEnum = Cast<UNePyGeneratedEnum>(Pair.Key))
		{
			Result.Add(const_cast<UNePyGeneratedEnum*>(GeneratedEnum));
		}
	}
	return Result;
}

void FNePyWrapperTypeRegistry::AddReinstancingPyTypeToStruct(const PyTypeObject* OldPyType, const UScriptStruct* NewStruct)
{
	check(IsInGameThread());
	ReinstancingPyTypeToStruct.Add(OldPyType, NewStruct);
}

void FNePyWrapperTypeRegistry::RemoveReinstancingPyTypeToStruct(const PyTypeObject* OldPyType)
{
	check(IsInGameThread());
	ReinstancingPyTypeToStruct.Remove(OldPyType);
}

void FNePyWrapperTypeRegistry::AddReinstancingStruct(UScriptStruct* OldStruct, UScriptStruct* NewStruct)
{
	check(IsInGameThread());
	ReinstancingStructMap.Add(OldStruct, NewStruct);
}

void FNePyWrapperTypeRegistry::RemoveReinstancingStruct(UScriptStruct* OldStruct)
{
	check(IsInGameThread());
	ReinstancingStructMap.Remove(OldStruct);
}

UScriptStruct* FNePyWrapperTypeRegistry::GetReinstancedStruct(UScriptStruct* OldStruct)
{
	check(IsInGameThread());
	if (UScriptStruct** FoundStruct = ReinstancingStructMap.Find(OldStruct))
	{
		return *FoundStruct;
	}
	return nullptr;
}

void FNePyWrapperTypeRegistry::GeneratePyAttrDescForClass(const UClass* InClass, PyTypeObject* TypeObject, bool bCheckExistence) const
{
	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	int32 PropertyCount = 0;
	int32 FunctionCount = 0;

	// Log header if enabled
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("========================================"));
		UE_LOG(LogNePython, Log, TEXT("Generating Descriptors for Class: %s [UClass=%p]"), 
			*InClass->GetName(), 
			InClass);
		UE_LOG(LogNePython, Log, TEXT("  Python Type: %s [PyType=%p]"), 
			UTF8_TO_TCHAR(TypeObject->tp_name), 
			TypeObject);
		UE_LOG(LogNePython, Log, TEXT("  Base Class: %s [%p]"), 
			InClass->GetSuperClass() ? *InClass->GetSuperClass()->GetName() : TEXT("None"),
			InClass->GetSuperClass());
		UE_LOG(LogNePython, Log, TEXT("  Package: %s [%p]"), 
			*InClass->GetPackage()->GetName(),
			InClass->GetPackage());
		
		if (Level >= 2)
		{
			UE_LOG(LogNePython, Log, TEXT("  Properties:"));
		}
	}

	// Generate property descriptors
	for (TFieldIterator<FProperty> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
	{
		FProperty* Property = *FieldIt;
		FString PropName = Property->GetName();
		FTCHARToUTF8 PropNameUTF8(*PropName);
		if (!bCheckExistence || !PyDict_GetItemString(TypeObject->tp_dict, PropNameUTF8.Get()))
		{
			if (NePyStealReference(NePyType_AddNewProperty(TypeObject, Property, PropNameUTF8.Get())))
			{
				PropertyCount++;
			}
		}
	}
	
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
	NePyType_AddFieldNotifySupportForType(TypeObject, InClass);
#endif

	// Log function header
	if (Level >= 2 && PropertyCount > 0)
	{
		UE_LOG(LogNePython, Log, TEXT("  Functions:"));
	}

	// 总是预先生成函数绑定，不走NePyObject_Getattro延迟绑定，让子类函数能正确覆写基类函数
	TSet<FString> HasK2FunctionNames;
	for (TFieldIterator<UFunction> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
	{
		UFunction* Func = *FieldIt;
		FString FuncName = Func->GetName();
		if (FuncName.StartsWith(TEXT("K2_")))
		{
			HasK2FunctionNames.Add(FuncName.Mid(3));
		}
	}

	for (TFieldIterator<UFunction> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
	{
		UFunction* Func = *FieldIt;
		FString FuncName = Func->GetName();
		// 已经有K2_前缀的函数了，跳过无前缀的函数
		if (HasK2FunctionNames.Contains(FuncName))
		{
			continue;
		}
		NePyGenUtil::RegularizeUFunctionName(FuncName);
		FTCHARToUTF8 FuncNameUTF8(*FuncName);
		if (!bCheckExistence || !PyDict_GetItemString(TypeObject->tp_dict, FuncNameUTF8.Get()))
		{
			if (NePyStealReference(NePyType_AddNewFunction(TypeObject, Func, FuncNameUTF8.Get())))
			{
				FunctionCount++;
			}
		}
	}

	// Log summary
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("  Summary: Added %d properties and %d functions"), PropertyCount, FunctionCount);
		UE_LOG(LogNePython, Log, TEXT("  tp_dict=%p, tp_dict refcount=%d"), 
			TypeObject->tp_dict, 
			TypeObject->tp_dict ? (int32)Py_REFCNT(TypeObject->tp_dict) : 0);
		UE_LOG(LogNePython, Log, TEXT("========================================"));
	}
}

const FNePyObjectTypeInfo* FNePyWrapperTypeRegistry::GenerateDynamicTypeForClass(const UClass* InClass)
{
	check(IsInGameThread());
	checkf(!PythonWrappedClasses.Contains(InClass), TEXT("Generate DynamicType for a exist PyType!"));

	int32 Level = CVarSubclassingLog.GetValueOnGameThread();
	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("========================================"));
		UE_LOG(LogNePython, Log, TEXT("Starting Dynamic Type Generation for Class: %s [UClass=%p]"), 
			*InClass->GetName(), 
			InClass);
	}

	// 获取类型名称
	FName Name = InClass->GetFName();

	// 查找上一级父类的PyType，找不到则递归生成
	const FNePyObjectTypeInfo* SuperTypeInfo = nullptr;
	const UClass* SuperUClass = InClass->GetSuperClass();
	if (SuperUClass)
	{
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Looking up Super Class: %s [%p]"), 
				*SuperUClass->GetName(), 
				SuperUClass);
		}

		SuperTypeInfo = PythonWrappedClasses.Contains(SuperUClass) ? PythonWrappedClasses.Find(SuperUClass)->Get() : nullptr;
		if (!SuperTypeInfo)
		{
			if (Level >= 1)
			{
				UE_LOG(LogNePython, Log, TEXT("  Super Class not found, generating recursively..."));
			}
			SuperTypeInfo = GenerateDynamicTypeForClass(SuperUClass);
			check(SuperTypeInfo);
		}
	}
	check(SuperTypeInfo);

	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("  Super Type Info: PyType=%p, TypeFlags=%d"), 
			SuperTypeInfo->TypeObject, 
			(int32)SuperTypeInfo->TypeFlags);
	}

	PyTypeObject* PyBase = SuperTypeInfo->TypeObject;
	PyObject* PyBases = nullptr;
	if (InClass->Interfaces.Num() > 0)
	{
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Processing %d interfaces..."), InClass->Interfaces.Num());
		}

		PyBases = PyTuple_New(InClass->Interfaces.Num() + 1);
		PyTuple_SetItem(PyBases, 0, (PyObject*)NePyNewReference(PyBase).Release());
		for (int32 Index = 0; Index < InClass->Interfaces.Num(); ++Index)
		{
			const FImplementedInterface& Interface = InClass->Interfaces[Index];
			const FNePyObjectTypeInfo* InterfaceTypeInfo = GetWrappedClassType(Interface.Class);
			check(InterfaceTypeInfo && InterfaceTypeInfo->TypeObject);

			if (Level >= 2)
			{
#if ENGINE_MAJOR_VERSION < 5
				void* InterfaceClassPtr = Interface.Class;
#else
				void* InterfaceClassPtr = Interface.Class.Get();
#endif
				UE_LOG(LogNePython, Log, TEXT("    Interface[%d]: %s [UClass=%p, PyType=%p, TypeFlags=%d]"),
					Index,
					*Interface.Class->GetName(),
					InterfaceClassPtr,
					InterfaceTypeInfo->TypeObject,
					(int32)InterfaceTypeInfo->TypeFlags);
			}

			PyTuple_SetItem(PyBases, Index + 1, (PyObject*)NePyNewReference(InterfaceTypeInfo->TypeObject).Release());
		}
	}

	PyTypeObject* NewPyType;
	if (((const UObject*)InClass)->IsA<UBlueprintGeneratedClass>())
	{
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Creating Heap Type for Blueprint Class..."));
		}

		// 为了在蓝图类卸载后正确销毁PyType，这里使用heap type
		FNePyObjectPtr PyClassBases = NePyStealReference(PyTuple_New(1));
		PyTuple_SetItem(PyClassBases, 0, NePyNewReference((PyObject*)SuperTypeInfo->TypeObject).Release());
		FNePyObjectPtr PyClassDict = NePyStealReference(PyDict_New());
		PyDict_SetItemString(PyClassDict, "__slots__", NePyStealReference(PyTuple_New(0))); // 防止类实例创建__dict__和__weakref__，与静态导出类的行为保持一致
		PyObject* PyBuiltins = PyEval_GetBuiltins();
		PyObject* PyBuiltinsName = PyDict_GetItemString(PyBuiltins, "__name__");
		PyDict_SetItemString(PyClassDict, "__module__", PyBuiltinsName); // 动态类型都应视作为builtin模块
		FNePyObjectPtr PyClassArgs = NePyStealReference(PyTuple_New(3));
		PyTuple_SetItem(PyClassArgs, 0, NePyString_FromString(TCHAR_TO_UTF8(*Name.ToString())));
		PyTuple_SetItem(PyClassArgs, 1, PyClassBases.Release());
		PyTuple_SetItem(PyClassArgs, 2, PyClassDict.Release());

		FNePyTypeObjectPtr DynamicType = NePyStealReference((PyTypeObject*)PyType_Type.tp_new(Py_TYPE(SuperTypeInfo->TypeObject), PyClassArgs, nullptr));
		if (!DynamicType)
		{
			UE_LOG(LogNePython, Error, TEXT("  Failed to create heap type!"));
			PyErr_Print();
			return nullptr;
		}
		
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Created Heap Type: %s [PyType=%p, refcount=%d]"), 
				UTF8_TO_TCHAR(DynamicType->tp_name),
				DynamicType.Get(),
				(int32)Py_REFCNT(DynamicType.Get()));
		}

		NewPyType = DynamicType.Get();
		{
			static PyMethodDef NePyDynamicTypeMethod_Class = { "Class", NePyCFunctionCast(&FNePyDynamicClassType::Class), METH_NOARGS | METH_CLASS, "" };
			FNePyObjectPtr Descr = NePyStealReference(PyDescr_NewClassMethod(DynamicType, &NePyDynamicTypeMethod_Class));
			PyDict_SetItemString(DynamicType->tp_dict, "Class", Descr);
		}

		FNePyObjectTypeInfo TypeInfo = *SuperTypeInfo;  // NewFunc与父类相同
		TypeInfo.TypeObject = DynamicType.Get();
		TypeInfo.TypeFlags = ENePyTypeFlags::BlueprintPyType;
		RegisterWrappedClassType(InClass, TypeInfo);
	}
	else
	{
		check(EnumHasAnyFlags(InClass->ClassFlags, EClassFlags::CLASS_Native));

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Creating Dynamic Type for Native Class..."));
		}
		
		// 为PyTypeObject分配内存
		TSharedPtr<FNePyDynamicClassType> DynamicPyTypePtr = MakeShared<FNePyDynamicClassType>();
		FNePyDynamicClassType* DynamicType = DynamicPyTypePtr.Get();
		DynamicGeneratedPyTypes.Add(InClass, DynamicPyTypePtr);

		DynamicType->InitDynamicClassType(InClass, SuperTypeInfo, PyBase, PyBases);
		NewPyType = DynamicType->GetPyType();

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Created Dynamic Type: %s [PyType=%p, DynamicType=%p]"), 
				UTF8_TO_TCHAR(NewPyType->tp_name),
				NewPyType,
				DynamicType);
		}
	}

	GeneratePyAttrDescForClass(InClass, NewPyType, false);

	if (Level >= 1)
	{
		UE_LOG(LogNePython, Log, TEXT("Completed Dynamic Type Generation for Class: %s"), *InClass->GetName());
		UE_LOG(LogNePython, Log, TEXT("========================================"));
	}

	return PythonWrappedClasses.Contains(InClass) ? PythonWrappedClasses.Find(InClass)->Get() : nullptr;
}

const FNePyStructTypeInfo* FNePyWrapperTypeRegistry::GenerateDynamicTypeForStruct(const UScriptStruct* InScriptStruct)
{
	check(IsInGameThread());
	checkf(!PythonWrappedStructs.Contains(InScriptStruct), TEXT("Generate DynamicType for a exist PyType!"));

	// 获取类型名称
	FName Name = InScriptStruct->GetFName();

	// 查找上一级父类的PyType，找不到则递归生成
	const FNePyStructTypeInfo* SuperTypeInfo = nullptr;
	const UScriptStruct* SuperUStruct = Cast<UScriptStruct>(InScriptStruct->GetSuperStruct());
	if (SuperUStruct)
	{
		SuperTypeInfo = PythonWrappedStructs.Contains(SuperUStruct) ? PythonWrappedStructs.Find(SuperUStruct)->Get() : nullptr;
		if (!SuperTypeInfo)
		{
			SuperTypeInfo = GenerateDynamicTypeForStruct(SuperUStruct);
			check(SuperTypeInfo);
		}
	}

	if (InScriptStruct->IsA<UUserDefinedStruct>())
	{
		// UserDefinedStruct没有基类
		check(!SuperUStruct);
		PyTypeObject* PyBaseType = NePyUserStructGetType();

		// 为了在蓝图类卸载后正确销毁PyType，这里使用heap type
		FNePyObjectPtr PyClassBases = NePyStealReference(PyTuple_New(1));
		PyTuple_SetItem(PyClassBases, 0, NePyNewReference((PyObject*)PyBaseType).Release());
		FNePyObjectPtr PyClassDict = NePyStealReference(PyDict_New());
		PyDict_SetItemString(PyClassDict, "__slots__", NePyStealReference(PyTuple_New(0))); // 防止类实例创建__dict__和__weakref__，与静态导出类的行为保持一致
		PyObject* PyBuiltins = PyEval_GetBuiltins();
		PyObject* PyBuiltinsName = PyDict_GetItemString(PyBuiltins, "__name__");
		PyDict_SetItemString(PyClassDict, "__module__", PyBuiltinsName); // 动态类型都应视作为builtin模块
		FNePyObjectPtr PyClassArgs = NePyStealReference(PyTuple_New(3));
		PyTuple_SetItem(PyClassArgs, 0, NePyString_FromString(TCHAR_TO_UTF8(*Name.ToString())));
		PyTuple_SetItem(PyClassArgs, 1, PyClassBases.Release());
		PyTuple_SetItem(PyClassArgs, 2, PyClassDict.Release());

		FNePyTypeObjectPtr DynamicType = NePyStealReference((PyTypeObject*)PyType_Type.tp_new(Py_TYPE(PyBaseType), PyClassArgs, nullptr));
		if (!DynamicType)
		{
			PyErr_Print();
			return nullptr;
		}
		DynamicType->tp_alloc = FNePyUserStruct::Alloc; // 覆写heap type原生行为，使用我们提供的tp_alloc

		// UserDefinedStruct成员变量的原始名称里带有一长串GUID，用起来很不方便
		// 我们预先为UserDefinedStruct生成PropertyDescriptor，从而能去除GUID
		for (TFieldIterator<FProperty> FieldIt(InScriptStruct, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
		{
			FProperty* Prop = *FieldIt;
			FString PropName = InScriptStruct->GetAuthoredNameForField(Prop);
			NePyStealReference(NePyType_AddNewProperty(DynamicType, Prop, TCHAR_TO_UTF8(*PropName)));
		}

		FNePyStructTypeInfo TypeInfo = {
			DynamicType.Get(),
			ENePyTypeFlags::BlueprintPyType,
			(NePyStructPropSet)FNePyDynamicStructType::PropSet,
			(NePyStructPropGet)FNePyDynamicStructType::PropGet,
		};
		TypeInfo.TypeObject = DynamicType.Get();
		TypeInfo.TypeFlags = ENePyTypeFlags::BlueprintPyType;
		RegisterWrappedStructType(InScriptStruct, TypeInfo);
	}
	else
	{
		check(EnumHasAnyFlags(InScriptStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
		// 为PyTypeObject分配内存
		TSharedPtr<FNePyDynamicStructType> DynamicPyTypePtr = MakeShared<FNePyDynamicStructType>();
		FNePyDynamicStructType* DynamicType = DynamicPyTypePtr.Get();
		DynamicGeneratedPyTypes.Add(InScriptStruct, DynamicPyTypePtr);

		DynamicType->InitDynamicStructType(InScriptStruct);
	}

	return PythonWrappedStructs.Contains(InScriptStruct) ? PythonWrappedStructs.Find(InScriptStruct)->Get() : nullptr;
}

const FNePyEnumTypeInfo* FNePyWrapperTypeRegistry::GenerateDynamicTypeForEnum(const UEnum* InEnum)
{
	check(IsInGameThread());
	checkf(!PythonWrappedEnums.Contains(InEnum), TEXT("Generate DynamicType for a exist PyType!"));

	// 获取类型名称
	FName Name = InEnum->GetFName();

	if (InEnum->IsA<UUserDefinedEnum>())
	{
		// UserDefinedEnum没有基类
		PyTypeObject* PyBaseType = NePyEnumBaseGetType();

		// 为了在蓝图类卸载后正确销毁PyType，这里使用heap type
		FNePyObjectPtr PyClassBases = NePyStealReference(PyTuple_New(1));
		PyTuple_SetItem(PyClassBases, 0, NePyNewReference((PyObject*)PyBaseType).Release());
		FNePyObjectPtr PyClassDict = NePyStealReference(PyDict_New());
		PyDict_SetItemString(PyClassDict, "__slots__", NePyStealReference(PyTuple_New(0))); // 防止类实例创建__dict__和__weakref__，与静态导出类的行为保持一致
		PyObject* PyBuiltins = PyEval_GetBuiltins();
		PyObject* PyBuiltinsName = PyDict_GetItemString(PyBuiltins, "__name__");
		PyDict_SetItemString(PyClassDict, "__module__", PyBuiltinsName); // 动态类型都应视作为builtin模块
		FNePyObjectPtr PyClassArgs = NePyStealReference(PyTuple_New(3));
		PyTuple_SetItem(PyClassArgs, 0, NePyString_FromString(TCHAR_TO_UTF8(*Name.ToString())));
		PyTuple_SetItem(PyClassArgs, 1, PyClassBases.Release());
		PyTuple_SetItem(PyClassArgs, 2, PyClassDict.Release());

		FNePyTypeObjectPtr DynamicType = NePyStealReference((PyTypeObject*)PyType_Type.tp_new(Py_TYPE(PyBaseType), PyClassArgs, nullptr));
		if (!DynamicType)
		{
			PyErr_Print();
			return nullptr;
		}
		FNePyEnumBase::InitEnumEntries(DynamicType, InEnum);

		FNePyEnumTypeInfo TypeInfo = {
			DynamicType.Get(),
			ENePyTypeFlags::BlueprintPyType,
		};
		TypeInfo.TypeObject = DynamicType.Get();
		TypeInfo.TypeFlags = ENePyTypeFlags::BlueprintPyType;
		RegisterWrappedEnumType(InEnum, TypeInfo);
	}
	else
	{
		// 为PyTypeObject分配内存
		TSharedPtr<FNePyDynamicEnumType> DynamicPyTypePtr = MakeShared<FNePyDynamicEnumType>();
		FNePyDynamicEnumType* DynamicType = DynamicPyTypePtr.Get();
		DynamicGeneratedPyTypes.Add(InEnum, DynamicPyTypePtr);

		DynamicType->InitDynamicEnumType(InEnum);
	}

	return PythonWrappedEnums.Contains(InEnum) ? PythonWrappedEnums.Find(InEnum)->Get() : nullptr;
}

#if WITH_EDITOR
void FNePyWrapperTypeRegistry::RegisterBlueprintDelegates()
{
	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().AddStatic(&FNePyWrapperTypeRegistry::OnBlueprintPreCompile);
	}
	else
	{
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FNePyWrapperTypeRegistry::OnPostEngineInit);
	}
}

void FNePyWrapperTypeRegistry::ApplyDataOnlyBlueprintAttachmentFixup()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> BlueprintAssets;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName).Get();
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets, true);
#else
	AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), BlueprintAssets, true);
#endif

	for (const FAssetData& Asset : BlueprintAssets)
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset());
		if (!Blueprint)
		{
			continue;
		}

		if (!Blueprint->SkeletonGeneratedClass || !Blueprint->GeneratedClass)
		{
			// 蓝图没有生成类或骨架生成类，跳过
			continue;
		}

		// 用FBlueprintEditorUtils判断是否为纯数据蓝图
		if (!FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint))
		{
			continue;
		}

		// 检查是否是python的生成类
		if (!Cast<UNePyGeneratedClass>(Blueprint->ParentClass))
		{
			continue;
		}

		// 触发编译
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
	}
}

void FNePyWrapperTypeRegistry::OnPostEngineInit()
{
	if (GEditor)
	{
		GEditor->OnBlueprintPreCompile().AddStatic(&FNePyWrapperTypeRegistry::OnBlueprintPreCompile);
	}
}

void FNePyWrapperTypeRegistry::OnBlueprintPreCompile(UBlueprint* Blueprint)
{
	UClass* Class = Blueprint->GeneratedClass;
	if (Class && FNePyWrapperTypeRegistry::Get().HasWrappedClassType(Class))
	{
		Blueprint->OnCompiled().AddRaw(&FNePyWrapperTypeRegistry::Get(), &FNePyWrapperTypeRegistry::OnBlueprintCompiled);
	}
}

void FNePyWrapperTypeRegistry::OnBlueprintCompiled(UBlueprint* Blueprint)
{
	Blueprint->OnCompiled().RemoveAll(this);

	UClass* Class = Blueprint->GeneratedClass;
	if (!Class)
	{
		return;
	}

	ResetPyTypeDescriptors(Class, true);
}

void FNePyWrapperTypeRegistry::PreChange(const UUserDefinedStruct* Changed, FStructureEditorUtils::EStructureEditorChangeInfo ChangedType)
{
	ResetPyTypeDescriptors(Changed, false);
}

void FNePyWrapperTypeRegistry::PostChange(const UUserDefinedStruct* Changed, FStructureEditorUtils::EStructureEditorChangeInfo ChangedType)
{
	ResetPyTypeDescriptors(Changed, true);
}
#endif // WITH_EDITOR
