#pragma once
#include "NePyIncludePython.h"
#include "NePyResourceOwner.h"
#include "NePyGenUtil.h"
#include "NePySpecifiers.h"
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "NePyGeneratedClass.generated.h"

class UActorComponent;

// 通过Python提供的信息，在运行时构造出来的UE类
UCLASS()
class NEPYTHONBINDING_API UNePyGeneratedClass : public UBlueprintGeneratedClass, public INePyResourceOwner
{
	GENERATED_BODY()

public:
	// UObject interface
	virtual void BeginDestroy() override;

	virtual void PostLoad() override;

	virtual void PostDuplicate(bool bDuplicateForPIE) override;

	/** called to gather blueprint replicated properties */
	virtual void GetLifetimeBlueprintReplicationList(TArray<class FLifetimeProperty>& OutLifetimeProps) const /*override*/;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

	// UClass interface
	virtual bool IsFunctionImplementedInScript(FName InFunctionName) const override;

#if WITH_EDITOR
	virtual void ConditionalRecompileClass(FUObjectSerializeContext* InLoadContext) override {}
#endif //WITH_EDITOR

#if ENGINE_MAJOR_VERSION < 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0)
	virtual void PostInitInstance(UObject* InObj) override;
#else
	virtual void PostInitInstance(UObject* InObj, FObjectInstancingGraph* InstanceGraph) override;
#endif // ENGINE_MAJOR_VERSION

	/** Generate an Unreal class from the given Python type */
	static UNePyGeneratedClass* GenerateClass(PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs);

	// 重新生成类
	// NOTE: 本应该叫做RegenerateClass，但与UObject::RegenerateClass重名，必须换个名字
	static UNePyGeneratedClass* RegenerateNePyClass(UNePyGeneratedClass* InOldClass);

	// 调用__init_default__方法以初始化CDO
	void CallInitDefaultFunc();
	
	// 获取 uproperty 直接定义的默认值以初始化CDO
	void InitDefaultValueForProperties();

	// 清理Python中定义的UFunction
	void ClearPythonGeneratedFunctions(bool bMoveAside = false);

	// 对象构造函数（ClassConstructor）
	static void StaticActorConstructor(const FObjectInitializer& Initializer);
	static void StaticComponentConstructor(const FObjectInitializer& Initializer);
	static void StaticObjectConstructor(const FObjectInitializer& Initializer);

	// 递归查找父类，返回第一个找到的NePyGeneratedClass
	static const UNePyGeneratedClass* GetFirstGeneratedClass(const UClass* InClass);

	// 在CDO Reinstance之后修复ActorComponent之间的挂接关系
	void FixupComponentAttachmentAfterReinstanceCDO(AActor* Actor);

private:
	/** Helper method to Set the Property's DefaultValue corresponding to the Actor's Component*/
	static void SetActorComponentPropertyDefaultValue(const FObjectInitializer& Initializer, const UObject* Archetype, AActor* Actor, UActorComponent* ActorComp, FProperty* Prop);

	/** Helper method to clear Actor's ComponentAttachment defined by PyClass*/
	static void ClearComponentAttachment(AActor* Actor, const UNePyGeneratedClass* PyClass);

	/** Helper method to build Actor's RootComponent by PyClass*/
	static void BuildRootComponent(AActor* Actor, const UNePyGeneratedClass* PyClass);

	/** Helper method to build component attachment for Actor only by current PyClass*/
	static void BuildComponentAttachmentByCurrentPyClass(AActor* Actor, const UNePyGeneratedClass* PyClass);

	/** Helper method to build component attachment for Actor by PyClass and all its SuperClasses*/
	static void BuildComponentAttachment(AActor* Actor, const UNePyGeneratedClass* PyClass);

	/** Internal helper method for StaticActorConstructor*/
	static void StaticActorConstructorInternal(const FObjectInitializer& Initializer, const UNePyGeneratedClass* PyClass);

public:
	// 与此UClass一一对应的Python类
	// 持有它是为了确保当没有Python实例对象时，PyType不会被gc回收
	PyTypeObject* PyType = nullptr;

	// Python类的__init_default__方法
	PyObject* PyInitDefaultFunc = nullptr;

	// 继承链上最近的C++类
	UClass* NativeSuperClass = nullptr;

	/** Array of properties generated for this class */
	TArray<TSharedPtr<NePyGenUtil::FPropertyDef>> PropertyDefs;

	/** Array of components generated for this class */
	TArray<TSharedPtr<NePyGenUtil::FComponentDef>> ComponentDefs;

	/** Array of delegates generated for this class */
	TArray<TSharedPtr<NePyGenUtil::FDelegateDef>> DelegateDefs;

	// 记录包装后的Python函数
	TArray<UNePyGeneratedFunction*> PyGeneratedFuncs;

#if WITH_EDITOR
	// 记录 Class Specifier 用于 Reload
	TArray<FNePySpecifier*> Specifiers;
#endif //WITH_EDITOR

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
	// 针对每个FieldNotify，记录需要联动触发的其它FieldNotify
	TMap<FName, TArray<FName>> OtherFieldNotifyToTriggerMap;
#endif
};


// 通过Python提供的信息，在运行时构造出来的UE方法
UCLASS()
class NEPYTHONBINDING_API UNePyGeneratedFunction : public UFunction, public INePyResourceOwner
{
	GENERATED_BODY()

public:
	// UObject interface
	virtual void BeginDestroy() override;

	// UField interface.
	virtual void Bind() override;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

	// 初始化Python方法
	void InitializePythonFunction(PyObject* InPyFunc, const char* InPyFuncName);

	// 从C++调用Python的静态转发方法
	DECLARE_FUNCTION(CallPythonFunction);

public:
	// 持有的Python方法
	PyObject* PyFunc = nullptr;

	// Python方法名
	NePyGenUtil::FUTF8Buffer PyFuncName;

	// 是否允许从python调用生成类方法时，跳过蓝图虚拟机
	bool bEnableFastPath = false;
};


PyTypeObject* NePyInitGeneratedClass();
