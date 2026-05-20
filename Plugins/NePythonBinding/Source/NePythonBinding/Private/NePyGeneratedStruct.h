#pragma once
#include "NePyIncludePython.h"
#include "NePyResourceOwner.h"
#include "NePyGenUtil.h"
#include "NePySpecifiers.h"
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/StructOnScope.h"
#include "NePyGeneratedStruct.generated.h"

class UNePyGeneratedStruct;

/** Wrapper for StructOnScope that tells it to ignore default values */
class FNePyGeneratedStructOnScopeIgnoreDefaults : public FStructOnScope
{
public:
	/** Constructor with no script struct, call Recreate later */
	FNePyGeneratedStructOnScopeIgnoreDefaults() : FStructOnScope() {}

	/** Constructor that initializes for you */
	FNePyGeneratedStructOnScopeIgnoreDefaults(const UNePyGeneratedStruct* InUserStruct);

	/** Initialize from existing data, will free when scope closes */
	FNePyGeneratedStructOnScopeIgnoreDefaults(const UNePyGeneratedStruct* InUserStruct, uint8* InData);

	/** Destroys and creates new struct */
	void Recreate(const UNePyGeneratedStruct* InUserStruct);

	virtual void Initialize() override;

#if WITH_EDITOR
	friend class FNePyStructReload;
#endif
};

// 通过Python提供的信息，在运行时构造出来的UE结构体
UCLASS()
class NEPYTHONBINDING_API UNePyGeneratedStruct : public UScriptStruct, public INePyResourceOwner
{
	GENERATED_UCLASS_BODY()
	~UNePyGeneratedStruct();

public:
	// UObject interface
	virtual void BeginDestroy() override;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

	/** Generate an Unreal struct from the given Python type */
	static UNePyGeneratedStruct* GenerateStruct(PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs);

	// 重新生成所有持有了旧结构体的结构体和类
	static void RegenerateReferencers(UNePyGeneratedStruct* InOldStruct);

	// 重新生成结构体
	static UNePyGeneratedStruct* RegenerateStruct(UNePyGeneratedStruct* InOldStruct);

	// UObject interface.
	
	virtual void SerializeTaggedProperties(FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults, const UObject* BreakRecursionIfFullyLoad = nullptr) const override;

	// End of UObject interface.

	// UScriptStruct interface.
	virtual void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;

	virtual void PrepareCppStructOps() override;
	// End of  UScriptStruct interface.

	/** Inspects properties and default values, setting appropriate StructFlags */
	void UpdateStructFlags();

	/** Specifically initialize this struct without using the default instance data */
	void InitializeStructIgnoreDefaults(void* Dest, int32 ArrayDim = 1) const;

	void GenerateDefaultInstance();

	/** Returns the raw memory of the default instance */
	const uint8* GetDefaultInstance() const;

	// 调用__init_default__方法以初始化CDO
	void CallInitDefaultFunc();

	// 获取 uproperty 直接定义的默认值以初始化CDO
	void InitDefaultValueForProperties();

protected:

	void SetCppStructOps(ICppStructOps* InStructOps);

	void ClearAndDeleteCppStructOps();

public:
	/** Default instance of this struct with default values filled in, used to initialize structure */
	FNePyGeneratedStructOnScopeIgnoreDefaults DefaultStructInstance;

	// 与此UScriptStruct一一对应的Python类
	// 持有它是为了确保当没有Python实例对象时，PyType不会被gc回收
	PyTypeObject* PyType = nullptr;

	// Python类的__init_default__方法
	PyObject* PyInitDefaultFunc = nullptr;

	// Reload后，引用新创建的Struct
	UNePyGeneratedStruct* NewStruct = nullptr;

	/** Array of properties generated for this class */
	TArray<TSharedPtr<NePyGenUtil::FPropertyDef>> PropertyDefs;

#if WITH_EDITOR
	// 记录 Class Specifier 用于 Reload
	TArray<FNePySpecifier*> Specifiers;
#endif //WITH_EDITOR

	friend class FNePyGeneratedStructBuilder;
#if WITH_EDITOR
	friend class FNePyStructReload;
#endif
};

PyTypeObject* NePyInitGeneratedStruct();
