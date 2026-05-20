#pragma once
#include "NePyIncludePython.h"
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Containers/Ticker.h"
#include "UObject/UObjectGlobals.h"
#if WITH_EDITOR
#include "Kismet2/StructureEditorUtils.h"
#endif

typedef bool (*NePyStructPropSet)(const FStructProperty*, PyObject*, void*);
typedef PyObject* (*NePyStructPropGet)(const FStructProperty*, const void*);

class UBlueprint;
class UBlueprintGeneratedClass;
struct FNePyDynamicType;
struct FNePyObjectBase;
class UNePyGeneratedClass;
class UNePyGeneratedStruct;
class UNePyGeneratedEnum;
#if WITH_EDITOR
class FNePyStructReload;
#endif

// NePy扩展的额外TypeFlags
enum class ENePyTypeFlags : uint16
{
	PyTypeFlag_None = 0,

	// 1 << 0 ~ 1 << 3 指示 PyType 的来源

	// 为C++类生成的静态PyType
	StaticPyType = 1 << 0,

	// 为C++类生成的动态PyType
	DynamicPyType = 1 << 1,

	// Python脚本中定义的PyType（heap type）
	ScriptPyType = 1 << 2,

	// 为蓝图生成的动态PyType（heap type）
	BlueprintPyType = 1 << 3,
};

ENUM_CLASS_FLAGS(ENePyTypeFlags)

struct FNePyTypeInfoBase
{
	PyTypeObject* TypeObject;
	ENePyTypeFlags TypeFlags;

public:
	FNePyTypeInfoBase(PyTypeObject* TypeObject, ENePyTypeFlags TypeFlags)
		: TypeObject(TypeObject), TypeFlags(TypeFlags) {}
};

struct FNePyStructTypeInfo: FNePyTypeInfoBase
{
	NePyStructPropSet PropSetFunc;
	NePyStructPropGet PropGetFunc;

public:
	FNePyStructTypeInfo(PyTypeObject* TypeObject, ENePyTypeFlags ExtraTypeFlags, NePyStructPropSet PropSetFunc, NePyStructPropGet PropGetFunc)
		: FNePyTypeInfoBase(TypeObject, ExtraTypeFlags), PropSetFunc(PropSetFunc), PropGetFunc(PropGetFunc) {}
};

typedef FNePyObjectBase* (*NePyObjectNewFunc)(UObject*, PyTypeObject*);
typedef void (*NePyObjectInitFunc)(PyObject*);

struct FNePyObjectTypeInfo : FNePyTypeInfoBase
{
private:
	friend class FNePyWrapperTypeRegistry;

	// 这项不要直接使用
	NePyObjectNewFunc NewFunc;

	NePyObjectInitFunc InitFunc;

public:
	FNePyObjectTypeInfo(PyTypeObject* TypeObject, ENePyTypeFlags ExtraTypeFlags, NePyObjectNewFunc NewFunc, NePyObjectInitFunc InitFunc=nullptr)
		: FNePyTypeInfoBase(TypeObject, ExtraTypeFlags), NewFunc(NewFunc), InitFunc(InitFunc){}

	// 调用NewFunc，创建一个新的PyObject
	FNePyObjectBase* NewObject(UObject* InObject) const;

	// 调用InitFunc，初始化 PyObject
	void InitObject(FNePyObjectBase* InPyObject) const;
};

struct FNePyEnumTypeInfo : FNePyTypeInfoBase
{
	FNePyEnumTypeInfo(PyTypeObject* TypeObject, ENePyTypeFlags TypeFlags)
		: FNePyTypeInfoBase(TypeObject, TypeFlags) {}
};


/** 单例管理类，用于管理UE类型到Python封装类型的映射*/
class NEPYTHONBINDING_API FNePyWrapperTypeRegistry
#if WITH_EDITOR
	: public FStructureEditorUtils::INotifyOnStructChanged
#endif
{
public:
	static FNePyWrapperTypeRegistry& Get();

	// 注册各种类型，缓存下UE和Python封装类型之间的映射关系
	void RegisterWrappedClassType(const UClass* InClass, const FNePyObjectTypeInfo& InPyTypeInfo);
	void RegisterWrappedStructType(const UScriptStruct* InScriptStruct, const FNePyStructTypeInfo& InPyTypeInfo);
	void RegisterWrappedEnumType(const UEnum* InEnum, const FNePyEnumTypeInfo& InPyTypeInfo);

	// 反注册各种类型
	void UnregisterWrappedClassType(const UClass* InClass);
	void UnregisterWrappedStructType(const UScriptStruct* InScriptStruct);
	void UnregisterWrappedEnumType(const UEnum* InEnum);

	// 查找UE对象是否有对应的Python封装类型
	bool HasWrappedObjectType(const UObject* InObject) const;
	bool HasWrappedClassType(const UClass* InClass) const;
	bool HasWrappedStructType(const UScriptStruct* InScriptStruct) const;
	bool HasWrappedEnumType(const UEnum* InEnum);

	// 获取UE对象对应的Python封装类型
	const FNePyObjectTypeInfo* GetWrappedObjectType(const UObject* InObject);
	const FNePyObjectTypeInfo* GetWrappedClassType(const UClass* InClass);
	const FNePyObjectTypeInfo* GetWrappedClassTypeIfExist(const UClass* InClass) const;
	const FNePyStructTypeInfo* GetWrappedStructType(const UScriptStruct* InScriptStruct);
	const FNePyStructTypeInfo* GetWrappedStructTypeIfExist(const UScriptStruct* InScriptStruct) const;
	const FNePyEnumTypeInfo* GetWrappedEnumType(const UEnum* InEnum);
	const FNePyEnumTypeInfo* GetWrappedEnumTypeIfExist(const UEnum* InEnum) const;

	// 根据Python封装类型获取UE类型
	const UClass* GetClassByPyType(const PyTypeObject* InPyType) const;
	const UScriptStruct* GetStructByPyType(const PyTypeObject* InPyType) const;
	const UEnum* GetEnumByPyType(const PyTypeObject* InPyType) const;

	// 清理并重新生成NePyDescriptor
	void ResetPyTypeDescriptors(const UClass* Class, bool bRecreateDescriptors) const;
	void ResetPyTypeDescriptors(const UScriptStruct* Class, bool bRecreateDescriptors) const;

	// 获取python生成类型列表
	TArray<UNePyGeneratedClass*> GetAllGeneratedClasses() const;
	TArray<UNePyGeneratedStruct*> GetAllGeneratedStructs() const;
	TArray<UNePyGeneratedEnum*> GetAllGeneratedEnums() const;

	// 将旧结构体的PyType指向新结构体
	void AddReinstancingPyTypeToStruct(const PyTypeObject* OldPyType, const UScriptStruct* NewStruct);
	// 移除旧结构体PyType对新结构体的指向
	void RemoveReinstancingPyTypeToStruct(const PyTypeObject* OldPyType);

	// 添加正在Reload的结构体映射
	void AddReinstancingStruct(UScriptStruct* OldStruct, UScriptStruct* NewStruct);
	// 移除正在Reload的结构体映射
	void RemoveReinstancingStruct(UScriptStruct* OldStruct);
	// 获取正在Reload的结构体映射
	UScriptStruct* GetReinstancedStruct(UScriptStruct* OldStruct);
private:

	FNePyWrapperTypeRegistry();

	// UClass到FNePyObjectTypeInfo的映射
	TMap<const UClass*, TSharedPtr<FNePyObjectTypeInfo>> PythonWrappedClasses;

	// UScriptStruct到FNePyStructTypeInfo的映射
	TMap<const UScriptStruct*, TSharedPtr<FNePyStructTypeInfo>> PythonWrappedStructs;

	// UEnum到FNePyEnumTypeInfo的映射
	TMap<const UEnum*, TSharedPtr<FNePyEnumTypeInfo>> PythonWrappedEnums;

	// PyTypeObject到UClass的映射
	TMap<const PyTypeObject*, const UClass*> PyTypeToClass;

	// PyTypeObject到UScriptStruct的映射
	TMap<const PyTypeObject*, const UScriptStruct*> PyTypeToStruct;

	// PyTypeObject到UEnum的映射
	TMap<const PyTypeObject*, const UEnum*> PyTypeToEnum;

	// 存储动态生成的PyTypeObject
	TMap<const UField*, TSharedPtr<FNePyDynamicType>> DynamicGeneratedPyTypes;

	// 正在Reload的旧PyType到新UScriptStruct的映射
	TMap<const PyTypeObject*, const UScriptStruct*> ReinstancingPyTypeToStruct;

	// 正在Reload的旧UScriptStruct到新UScriptStruct的映射
	TMap<UScriptStruct*, UScriptStruct*> ReinstancingStructMap;

private:
	// 为UClass的UProperty和UFunction创建Python层的Descriptor并添加到类型的tp_dict中
	void GeneratePyAttrDescForClass(const UClass* InClass, PyTypeObject* TypeObject, bool bCheckExistence) const;

	// 动态生成UClass对应的Python封装类型
	const FNePyObjectTypeInfo* GenerateDynamicTypeForClass(const UClass* InClass);

	// 动态生成UStruct对应的Python封装类型
	const FNePyStructTypeInfo* GenerateDynamicTypeForStruct(const UScriptStruct* InScriptStruct);

	// 动态生成UEnum对应的Python封装类型
	const FNePyEnumTypeInfo* GenerateDynamicTypeForEnum(const UEnum* InEnum);

#if WITH_EDITOR
public:
	void RegisterBlueprintDelegates();

	// 对纯数据的Python生成蓝图的Attachment进行修复
	void ApplyDataOnlyBlueprintAttachmentFixup();

private:
	void OnPostEngineInit();

	static void OnBlueprintPreCompile(UBlueprint* Blueprint);

	// 蓝图重新编译后，可能会复用旧的UClass，但替换其中的UProperty和UFunction。
	// 此时需要清理并重新生成NePyDescriptor。
	void OnBlueprintCompiled(UBlueprint* Blueprint);

	/* FStructureEditorUtils::INotifyOnStructChanged Interface, used to respond to changes to user defined structs */
	virtual void PreChange(const UUserDefinedStruct* Changed, FStructureEditorUtils::EStructureEditorChangeInfo ChangedType) override;
	virtual void PostChange(const UUserDefinedStruct* Changed, FStructureEditorUtils::EStructureEditorChangeInfo ChangedType) override;
#endif // WITH_EDITOR
};