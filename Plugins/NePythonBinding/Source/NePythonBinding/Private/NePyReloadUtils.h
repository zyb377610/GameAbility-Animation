#pragma once
#include "CoreMinimal.h"
#include "Misc/Build.h"
#include "Misc/EngineVersionComparison.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

class UBlueprintGeneratedClass;
class UUserDefinedStruct;
class UNePyGeneratedClass;
class UNePyGeneratedStruct;
class FNePyReloadReinstancer;

#if WITH_EDITOR
#include "Kismet2/KismetReinstanceUtilities.h"

// 一个辅助类，用来处理Subclassing类Reload以后的善后工作:
// * 旧实例Reinstancing
//   将旧类型实例的值记录下来，并转移到新实例上
// * 更新所有引用到旧类的蓝图
// 只能在Editor环境下工作
class FNePyClassReload
{
public:
	explicit FNePyClassReload(UClass* InReloadingClass);

	//
	void Finalize();

private:
	// 为指定的类创建Reinstancer
	void CreateReinstancer(UClass* InClass);

	// 处理正在Reload的类，以及其子类的默认值Reinstance
	// 参考 FBlueprintCompilationManagerImpl::ReinstanceBatch
	void ReinstanceCDOs();

	// 创建新默认旧子对象（Default Sub-Object，DSO）的映射
	// 参考 FBlueprintCompilationManagerImpl::BuildDSOMap
	static void BuildDSOMap(UObject* OldObject, UObject* NewObject, TMap<UObject*, UObject*>& OutOldToNewDSO);

	// 处理正在Reload的类，以及其子类的普通实例Reinstance
	// 参考 FBlueprintCompilationManagerImpl::FlushReinstancingQueueImpl()
	void ReinstanceObjects();

	// 重新编译所有相关的蓝图
	void RecompileDependentBPs();
	
	// 在Reinstance之后，修复Python生成类以及其蓝图子类Actor中组件的挂接关系
	void FixupClassComponentAttachment();
	bool FixupOneClassComponentAttachment(UNePyGeneratedClass* PyClass, UBlueprintGeneratedClass* BpClass, TSet<UBlueprintGeneratedClass*>& ProcessedBpClasses);


private:
	// 需要处理的类
	UClass* ReloadingClass;

	// 继承链上所有类的Reinstancer
	TArray<TSharedPtr<FNePyReloadReinstancer>> Reinstancers;

	// 旧类到新类的映射
	TMap<UClass*, UClass*> OldToNewClassMap;
};

// 辅助类，用于将FBlueprintCompileReinstancer的私有成员暴露给FNePyClassReload
class FNePyReloadReinstancer : public FBlueprintCompileReinstancer
{
public:
	FNePyReloadReinstancer(UClass* InClassToReinstance, EBlueprintCompileReinstancerFlags Flags);

#if ENGINE_MAJOR_VERSION >= 5
	static void CopyPropertiesForUnrelatedObjects(UObject* OldObject, UObject* NewObject, bool bClearExternalReferences, bool bForceDeltaSerialization = false)
	{
		FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects(OldObject, NewObject, bClearExternalReferences, bForceDeltaSerialization);
	};
#else
	static void CopyPropertiesForUnrelatedObjects(UObject* OldObject, UObject* NewObject, bool bClearExternalReferences)
	{
		FBlueprintCompileReinstancer::CopyPropertiesForUnrelatedObjects(OldObject, NewObject, bClearExternalReferences);
	};
#endif

	friend class FNePyClassReload;
};

// 一个辅助类，用来处理Subclassing结构体Reload以后的善后工作:
// * 重新生成所有引用到了该结构体的类和结构体
class FNePyStructReload
{
public:
	explicit FNePyStructReload(UNePyGeneratedStruct* InReloadingStruct);

	~FNePyStructReload();

	//
	void Finalize();

private:
	// 创建指定结构体的复制品
	// 参考 FUserDefinedStructureCompilerInner::ReplaceStructWithTempDuplicate
	static UScriptStruct* CreateDuplicatedStruct(UScriptStruct* InStruct);

	// 将所有引用了正在Reload的结构体的属性替换为引用复制品
	// 参考 FUserDefinedStructureCompilerInner::ReplaceStructWithTempDuplicate
	void ReplaceStructWithTempDuplicate();

	// 参考 FUserDefinedStructureCompilerInner::ClearStructReferencesInBP
	void ClearStructReferencesInBP(UBlueprint* FoundBlueprint);

	// 使用最新Python结构体默认值更新蓝图结构体的默认值
	void UpdateUserDefinedStructDefaultValues(UUserDefinedStruct* InStruct);

public:
	// 需要处理的结构体
	UNePyGeneratedStruct* ReloadingStruct;

	// ReloadingStruct的复制品，用于保留旧类型的布局
	UNePyGeneratedStruct* DuplicatedStruct;

private:
	// 需要重新生成的Python类
	TArray<UNePyGeneratedClass*> PyClassesToRegenerate;

	// 需要重新生成的Python结构体
	TArray<UNePyGeneratedStruct*> PyStructsToRegenerate;

	// 需要重新编译的蓝图类
	TArray<UBlueprint*> BlueprintsToRecompile;

	// 需要重新生成的蓝图结构体
	TArray<UUserDefinedStruct*> BpStructsToRegenerate;
};

// 辅助类，用于记录结构体中与默认值不相同的字段
struct FNePyCopyPropWriter : public FMemoryWriter
{
	FNePyCopyPropWriter(const UStruct* InStruct, const uint8* InData, const uint8* InDefaults);

	virtual FArchive& operator<<(UObject*& Res) override;

	virtual FArchive& operator<<(FLazyObjectPtr& Value) override { return FArchiveUObject::SerializeLazyObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override { return FArchiveUObject::SerializeSoftObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPath& Value) override { return FArchiveUObject::SerializeSoftObjectPath(*this, Value); }
	virtual FArchive& operator<<(FWeakObjectPtr& Value) override { return FArchiveUObject::SerializeWeakObjectPtr(*this, Value); }

#if ENGINE_MAJOR_VERSION >= 5
	virtual FArchive& operator<<(FObjectPtr& Value) override { return FArchiveUObject::SerializeObjectPtr(*this, Value); }
#endif // UE >= 5

	// 包含源对象序列化后的数据
	TArray<uint8> SavedPropertyData;
};

// 辅助类，用于将旧结构体中与默认值不同的字段拷贝至新结构体
struct FNePyCopyPropReader : public FMemoryReader
{
	FNePyCopyPropReader(FNePyCopyPropWriter& DataSrc, const UStruct* InStruct, uint8* OutData, const uint8* InDefaults);

	virtual FArchive& operator<<(UObject*& Res) override;

	virtual FArchive& operator<<(FLazyObjectPtr& Value) override { return FArchiveUObject::SerializeLazyObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPtr& Value) override { return FArchiveUObject::SerializeSoftObjectPtr(*this, Value); }
	virtual FArchive& operator<<(FSoftObjectPath& Value) override { return FArchiveUObject::SerializeSoftObjectPath(*this, Value); }
	virtual FArchive& operator<<(FWeakObjectPtr& Value) override { return FArchiveUObject::SerializeWeakObjectPtr(*this, Value); }

#if ENGINE_MAJOR_VERSION >= 5
	virtual FArchive& operator<<(FObjectPtr& Value) override { return FArchiveUObject::SerializeObjectPtr(*this, Value); }
#endif // UE >= 5
};

#endif // WITH_EDITOR
