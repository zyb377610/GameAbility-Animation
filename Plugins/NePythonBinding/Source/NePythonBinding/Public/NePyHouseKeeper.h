#pragma once
#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/UObjectArray.h"
#include "Containers/Map.h"
#include "Containers/Set.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Runtime/Launch/Resources/Version.h"
#include "NePyIncludePython.h"
#include "NePyTrackedObject.h"

struct FNePyObjectBase;
struct FNePyUserStruct;
struct FNePyDynamicMulticastDelegateWrapper;
class UScriptStruct;

// UObject成员变量的PythonWrapper类型
struct NEPYTHONBINDING_API FNePyPropObject : PyObject, FNePyTrackedObject
{
	// 对象指针
	void* Value;

	// 此对象是否依旧有效
	bool IsValid() const;

	// 判断对象有效性，如果失效则设置Python异常
	bool CheckValidAndSetPyErr() const;
};

class NEPYTHONBINDING_API FNePyHouseKeeper : public FGCObject
{
	// 参照UEP，一旦Python对象创建，则一直存在，直到对应的UObject销毁后才销毁。
	// 这样可避免频繁地创建和销毁Python对象，性能较好。
	// 
	// Python对象的引用计数管理如下：
	// 1. 首次创建Python对象时，加入TrackerMap，并使其引用计数+1。
	// 2. 当脚本层不再持有Python对象，引用计数最少降为1，此时Python对象不会销毁。
	// 3. 再次创建Python对象时，从TrackerMap中查找现有Python对象，将其引用计数+1并返回。
	// 4. 当UObject被Destroy或GC时，将Python对象从TrackerMap移除，使其引用计数-1。
	//    若此时Python对象引用计数为0，则被销毁。
	//    若此时Python对象引用计数不为0，则切断与UObject的联系，使其IsValid接口返回False。
	//
	// 如果希望由Python控制对象生命周期，而不受UE GC的影响：
	// 1. 调用ChangeOwnershipToPython，加入PythonOwnedObjects列表，并将引用计数-1
	// 2. PythonOwnedObjects中的对象不会被UE GC。
	// 3. 当脚本层不再持有Python对象，引用计数降为0，Python对象被销毁，
	//    且UE对象从PythonOwnedObjects列表中移除，可被正常GC
	struct FObjectReferenceTracker
	{
		// UObject的Python对象
		FNePyObjectBase* PyObj;
		// UObject成员变量的Python对象
		TMap<void*, FNePyPropObject*> PyPropObjs;
		// 此对象被添加到的多播委托列表
		TArray<FNePyDynamicMulticastDelegateWrapper*> MulticastDelegateUsages;

		FObjectReferenceTracker() = default;

		explicit FObjectReferenceTracker(FNePyObjectBase* InPyObj)
			: PyObj(InPyObj)
		{}

#if !UE_BUILD_SHIPPING
		// 调试用，UE对象名称
		FName ObjectName;
		// 调试用，UE类名称
		FName ClassName;
#endif // !UE_BUILD_SHIPPING
	};

public:
	static inline FNePyHouseKeeper& Get()
	{
		return *Singleton;
	}

	// 首先尝试获取UObject已有的Python对象
	// 若不存在，则创建并返回新的Python对象
	// 此接口会增加Python对象引用计数
	FNePyObjectBase* NewNePyObject(UObject* InObject);

	// 将新创建的PyObject加入生命周期管理
	// 内部接口，禁止随意使用！
	void AddNePyObjectInternalUseOnly(FNePyObjectBase* NewPyObj);

	// 看看Python Wrapper持有的UObject是否还有效
	bool IsValid(FNePyObjectBase* InPyObject);

	// 首先尝试获取UObject成员变量已有的Python对象
	// 若不存在，则根据传入的Constructor，创建并返回新的Python对象
	// 此接口会增加Python对象引用计数
	FNePyPropObject* NewNePyObjectMember(UObject* InObject, void* InMemberPtr, const TFunction<FNePyPropObject* ()>& Constructor);

	// 移除UObject成员变量的Python对象
	void RemoveNePyObjectMember(UObject* InObject, FNePyPropObject* ObjectMember);

	// 由Python类型新建ArrayWrapper时调用
	FArrayProperty* NewArrayPropertyByPyTypeObject(PyTypeObject* PyTypeObject, const TFunction<FArrayProperty*()>& Constructor);
	// 当PyTypeObject不可用时，清除对应创建的FProperty缓存
	void InvalidatePyTypeObjectForArrayProperty(PyTypeObject* PyTypeObject);

	// 注册UObject对DynamicMulticastDelegate的使用
	void RegisterDynamicMulticastDelegateUsage(const UObject* InObject, FNePyDynamicMulticastDelegateWrapper* InWrapper);

	// 反注册UObject对DynamicMulticastDelegate的使用
	void UnRegisterDynamicMulticastDelegateUsage(const UObject* InObject, FNePyDynamicMulticastDelegateWrapper* InWrapper);

	// Object生命是否正在由Python管理
	bool IsOwnedByPython(FNePyObjectBase* InPyObject);

	// 将Object的生命周期交由Python管理
	void ChangeOwnershipToPython(FNePyObjectBase* InPyObject);

	// 将Object的生命周期还给C++管理
	void ChangeOwnershipToCpp(FNePyObjectBase* InPyObject);

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& InCollector) override;
	virtual FString GetReferencerName() const override;

	// UE对象被GC时，切断与所有Python对象联系
	// 该方法仅供内部使用
	void InvalidateTracker(const UObject* InObject, bool bObjectIsOwnedByPython);

	// 记录UserDefinedStruct和NePyGeneratedStruct的结构体实例
	// 该方法仅供内部使用
	void AddUserStruct(const UScriptStruct* InScriptStruct, FNePyUserStruct* UserStruct);

	// 移除对UserDefinedStruct和NePyGeneratedStruct的结构体实例的记录
	// 该方法仅供内部使用
	void RemoveUserStruct(const UScriptStruct* InScriptStruct, FNePyUserStruct* UserStruct);

	// 当UserDefinedStruct或NePyGeneratedStruct销毁时，切断与所有Python对象联系
	// 该方法仅供内部使用
	void InvalidateUserStructs(const UScriptStruct* InScriptStruct);

#if WITH_EDITOR
	// 处理编辑器UObject对象Reload
	// 该方法仅供内部使用
	void HandleObjectsReinstanced(const TMap<UObject*, UObject*>& OldToNewInstanceMap);

	// 处理编辑器结构体实例Reload
	// 该方法仅供内部使用
	void HandleUserStructsReinstance(UScriptStruct* StructToReinstance, UScriptStruct* DuplicatedStruct);
#endif // WITH_EDITOR

private:
	// 记录了UObject被哪些Python对象引用
	TMap<const UObject*, FObjectReferenceTracker> TrackerMap;

	// 由Python持有，并管理生命周期的对象列表
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4 || ENGINE_MAJOR_VERSION >= 6
	TSet<TObjectPtr<const UObject>> PythonOwnedObjects;
#else
	TSet<const UObject*> PythonOwnedObjects;
#endif

	// 使用UserDefinedStruct和NePyGeneratedStruct创建的结构体实例
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4 || ENGINE_MAJOR_VERSION >= 6
	TMap<TObjectPtr<const UScriptStruct>, TSet<FNePyUserStruct*>> UserStructMap;
#else
	TMap<const UScriptStruct*, TSet<FNePyUserStruct*>> UserStructMap;
#endif

	// Python类型到ArrayProperty的映射
	TMap<PyTypeObject*, FArrayProperty*> ArrayPropertyCreationMap;

private:
	struct FObjectListener : public FUObjectArray::FUObjectDeleteListener
	{
		virtual void NotifyUObjectDeleted(const UObjectBase* InObject, int32 InObjectIndex) override;
		virtual void OnUObjectArrayShutdown();

#if !UE_BUILD_SHIPPING
		// 调试用，NotifyUObjectDeleted一共被调用了多少次
		uint64_t TotalNotifyCount = 0;
		// 调试用，切断了多少次Python连接
		uint64_t CutoffPythonCount = 0;
#endif
	};

	// 允许Listener直接操作HouseKeeper成员
	friend struct FObjectListener;

	// 监听UObject对象销毁事件，切断与Python Wrapper的联系
	FObjectListener ObjectDeleteListener;

private:
	static void InitSingleton();
	static void OnPreExit();
	static FNePyHouseKeeper* Singleton;

	friend class FNePythonBindingModule;
	friend class UNePyObjectTools;
};
