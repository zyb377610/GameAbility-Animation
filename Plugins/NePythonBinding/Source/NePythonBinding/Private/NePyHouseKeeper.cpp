#include "NePyHouseKeeper.h"
#include "Misc/AssertionMacros.h"
#include "HAL/UnrealMemory.h"
#include "UObject/Class.h"
#include "Engine/BlueprintGeneratedClass.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#include "StructUtils/UserDefinedStruct.h"
#else
#include "Engine/UserDefinedStruct.h"
#endif
#include "Engine/UserDefinedEnum.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR
#include "NePyGIL.h"
#include "NePyObjectBase.h"
#include "NePyReloadUtils.h"
#include "NePyUserStruct.h"
#include "NePyGeneratedStruct.h"
#include "NePyGeneratedClass.h"
#include "NePyGeneratedEnum.h"
#include "NePyDynamicMulticastDelegateWrapper.h"
#include "NePyWrapperTypeRegistry.h"

bool FNePyPropObject::IsValid() const
{
	return Value != nullptr;
}

bool FNePyPropObject::CheckValidAndSetPyErr() const
{
	if (!Value)
	{
		PyErr_SetString(PyExc_RuntimeError, "Self underlying UObject is invalid");
		return false;
	}
	return true;
}

FNePyObjectBase* FNePyHouseKeeper::NewNePyObject(UObject* InObject)
{
	check(IsInGameThread());
	check(InObject);

	FObjectReferenceTracker* Tracker = TrackerMap.Find(InObject);
	if (Tracker)
	{
		check(Tracker->PyObj);

		Py_INCREF(Tracker->PyObj);
		return Tracker->PyObj;
	}

	check(InObject->IsValidLowLevel());
#if ENGINE_MAJOR_VERSION >= 5
	ensure(::IsValid(InObject));
#else
	ensure(!InObject->IsPendingKillOrUnreachable());
#endif


	FNePyObjectBase* NewPyObj;
	const FNePyObjectTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedObjectType(InObject);
	if (PyTypeInfo)
	{
		NewPyObj = PyTypeInfo->NewObject(InObject);
	}
	else
	{
		NewPyObj = NePyObjectBaseNewInternalUseOnly(InObject);
	}

	check(NewPyObj->Value == InObject);

	AddNePyObjectInternalUseOnly(NewPyObj);

	if (PyTypeInfo)
	{
		 PyTypeInfo->InitObject(NewPyObj);
	}

	return NewPyObj;
}

void FNePyHouseKeeper::AddNePyObjectInternalUseOnly(FNePyObjectBase* NewPyObj)
{
	check(IsInGameThread());
	// 加入Tracker，增加引用计数
	check(NewPyObj->ob_refcnt == 1);
	Py_INCREF(NewPyObj);

	UObject* InObject = NewPyObj->Value;
	check(InObject->IsValidLowLevel());
	ensure(::IsValid(InObject));
	check(!TrackerMap.Find(InObject));
	TrackerMap.Add(InObject, FObjectReferenceTracker(NewPyObj));

#if !UE_BUILD_SHIPPING
	{
		FObjectReferenceTracker& Tracker = TrackerMap.FindChecked(InObject);
		Tracker.ObjectName = InObject->GetFName();
		Tracker.ClassName = InObject->GetClass()->GetFName();
	}
#endif // !UE_BUILD_SHIPPING
}

bool FNePyHouseKeeper::IsValid(FNePyObjectBase* InPyObject)
{
	if (!InPyObject || !::IsValid(InPyObject->Value))
	{
		return false;
	}

	check(TrackerMap.Contains(InPyObject->Value));
	return true;
}

FNePyPropObject* FNePyHouseKeeper::NewNePyObjectMember(UObject* InObject, void* InMemberPtr, const TFunction<FNePyPropObject* ()>& Constructor)
{
	check(IsInGameThread());
	FObjectReferenceTracker* Tracker = TrackerMap.Find(InObject);

	// 获取UObject成员变量的Python对象之前
	// UObject自身必须已生成Python对象
	check(Tracker);
	check(Tracker->PyObj);

	FNePyPropObject** PyObjectMember = Tracker->PyPropObjs.Find(InMemberPtr);
	if (PyObjectMember)
	{
		Py_INCREF(*PyObjectMember);
		return *PyObjectMember;
	}

	FNePyPropObject* RetValue = Constructor();

	// 加入Tracker，增加引用计数
	Py_INCREF(RetValue);
	Tracker->PyPropObjs.Add(InMemberPtr, (FNePyPropObject*)RetValue);

	return RetValue;
}

void FNePyHouseKeeper::RemoveNePyObjectMember(UObject* InObject, FNePyPropObject* ObjectMember)
{
	check(IsInGameThread());
	FObjectReferenceTracker* Tracker = TrackerMap.Find(InObject);
	if (!Tracker)
	{
		return;
	}
	FNePyPropObject** PyObjectMember = Tracker->PyPropObjs.Find(ObjectMember);
	if (PyObjectMember)
	{
		// 从Tracker移除，减少引用计数
		Py_DECREF(*PyObjectMember);
		Tracker->PyPropObjs.Remove(ObjectMember);
	}
}

FArrayProperty* FNePyHouseKeeper::NewArrayPropertyByPyTypeObject(PyTypeObject* PyTypeObject, const TFunction<FArrayProperty* ()>& Constructor)
{
	check(IsInGameThread());
	if (FArrayProperty** pProperty = ArrayPropertyCreationMap.Find(PyTypeObject))
	{
		return *pProperty;
	}

	FArrayProperty* NewProperty = Constructor();
	if (NewProperty)
	{
		// 增加引用计数
		Py_INCREF(PyTypeObject);
		ArrayPropertyCreationMap.Add(PyTypeObject, NewProperty);
	}

	return NewProperty;
}

void FNePyHouseKeeper::InvalidatePyTypeObjectForArrayProperty(PyTypeObject* PyTypeObject)
{
	check(IsInGameThread());
	if (FArrayProperty** pProperty = ArrayPropertyCreationMap.Find(PyTypeObject))
	{
		Py_XDECREF(PyTypeObject);
		ArrayPropertyCreationMap.Remove(PyTypeObject);
	}
}

void FNePyHouseKeeper::RegisterDynamicMulticastDelegateUsage(const UObject* InObject, FNePyDynamicMulticastDelegateWrapper* InWrapper)
{
	check(IsInGameThread());
	FObjectReferenceTracker* Tracker = TrackerMap.Find(InObject);
	check(Tracker);
	check(Tracker->PyObj);
	if (!Tracker->MulticastDelegateUsages.Contains(InWrapper))
	{
		// 此处绝对不能增加 InWrapper 的 Python 引用计数 (Py_INCREF)！
		// 循环引用链条分析:
		// 1. `InWrapper` (委托包装器) 为了执行回调，持有一个 `UNePyDelegate` 的 UObject 引用。
		// 2. `UNePyDelegate` 为了调用 Python 函数，持有一个 Python 回调函数 (Callable) 的强引用。
		// 3. Python 回调函数 (例如一个实例方法) 持有其所属对象 `InObject` 的 Python 包装器 (`self`) 的强引用。
		//    至此，引用链为： `InWrapper` -> `UNePyDelegate` -> Python Callable -> `InObject` 的 Python 包装器
		// 4. `FNePyHouseKeeper` 的 `TrackerMap` 通过 `InObject` 查找到对应的 `Tracker`。
		//    这个 `Tracker` 的 `MulticastDelegateUsages` 数组将要存储一个指向 `InWrapper` 的裸指针。
		// 5. 如果在这里对 `InWrapper` 执行 Py_INCREF，`Tracker` (代表 `InObject`) 就会持有一个 `InWrapper` 的强引用。
		// 最终形成的循环引用:
		// `InObject` -> `Tracker` -> `InWrapper` -> ... -> `InObject`
		// 这个循环引用会阻止 `InObject` 和 `InWrapper` 被 Python 的垃圾回收器正确销毁。
		// 因此，`MulticastDelegateUsages` 必须只存储裸指针。
		// ========== 以上注释为初次实现时UNePyDelegate直接存放实例方法下所记录，目前已经改为存放解包后函数 ==========
		// ========== 引用关系或许改变，但为保险起见，仍保留此注释与机制 ==========
		Tracker->MulticastDelegateUsages.Add(InWrapper);
	}
}

void FNePyHouseKeeper::UnRegisterDynamicMulticastDelegateUsage(const UObject* InObject, FNePyDynamicMulticastDelegateWrapper* InWrapper)
{
	check(IsInGameThread());
	FObjectReferenceTracker* Tracker = TrackerMap.Find(InObject);
	if (!Tracker)
	{
		return;
	}
	if (Tracker->MulticastDelegateUsages.Contains(InWrapper))
	{
		Tracker->MulticastDelegateUsages.Remove(InWrapper);
	}
}

bool FNePyHouseKeeper::IsOwnedByPython(FNePyObjectBase* InPyObject)
{
	check(IsInGameThread());
	if (!InPyObject->Value)
	{
		// CPP对象已被释放
		return false;
	}

	return PythonOwnedObjects.Contains(InPyObject->Value);
}

void FNePyHouseKeeper::ChangeOwnershipToPython(FNePyObjectBase* InPyObject)
{
	check(IsInGameThread());
	check(InPyObject->Value);
	if (PythonOwnedObjects.Contains(InPyObject->Value))
	{
		return;
	}

	PythonOwnedObjects.Add(InPyObject->Value);
	Py_DECREF(InPyObject);
}

void FNePyHouseKeeper::ChangeOwnershipToCpp(FNePyObjectBase* InPyObject)
{
	check(IsInGameThread());
	if (!InPyObject->Value)
	{
		// CPP对象已被释放
		return;
	}

	int32 RemoveCount = PythonOwnedObjects.Remove(InPyObject->Value);
	if (RemoveCount == 0)
	{
		return;
	}

	check(RemoveCount == 1);
	Py_INCREF(InPyObject);
}

void FNePyHouseKeeper::AddReferencedObjects(FReferenceCollector& InCollector)
{
	InCollector.AddReferencedObjects(PythonOwnedObjects);

#if WITH_EDITOR
	if (GIsReinstancing)
	{
		// Reinstance过程中只替换了Set的内容，但没有重新计算哈希
		// 我们需要通过这种手段迫使其重新计算一遍哈希，不然被替换过引用的对象就永远无法被移除了
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4 || ENGINE_MAJOR_VERSION >= 6
		TSet<TObjectPtr<const UObject>> TempObjects = MoveTemp(PythonOwnedObjects);
#else
		TSet<const UObject*> TempObjects = MoveTemp(PythonOwnedObjects);
#endif
		PythonOwnedObjects.Empty();
		PythonOwnedObjects.Append(TempObjects);
	}
#endif

	if (UserStructMap.Num() > 0)
	{
		TArray<const UScriptStruct*, TInlineAllocator<16>> StaledStructs;
		for (auto& Item : UserStructMap)
		{
			if (Item.Value.Num() > 0)
			{
				// 只要还存在Python实例，结构体类就不能释放
				InCollector.AddReferencedObject(Item.Key);
			}
			else
			{
				StaledStructs.Add(Item.Key);
			}
		}

		for (const UScriptStruct* StaledStruct : StaledStructs)
		{
			UserStructMap.Remove(StaledStruct);
		}
	}
}

FString FNePyHouseKeeper::GetReferencerName() const
{
	return TEXT("FNePyHouseKeeper");
}

void FNePyHouseKeeper::InvalidateTracker(const UObject* InObject, bool bObjectIsOwnedByPython)
{
	check(IsInGameThread());
	FObjectReferenceTracker Tracker = TrackerMap.FindAndRemoveChecked(InObject);

	for (auto& Pair : Tracker.PyPropObjs)
	{
		Pair.Value->Value = nullptr;
		// 从Tracker移除，减少引用计数
		Py_DECREF(Pair.Value);
	}

	for (FNePyDynamicMulticastDelegateWrapper* DelegateWrapper : TArray(Tracker.MulticastDelegateUsages))
	{
		if (DelegateWrapper->Value)
		{
			DelegateWrapper->RemoveUObjectAllDynamic(InObject);
		}
	}

	check(Tracker.PyObj->Value == InObject);
	Tracker.PyObj->Value = nullptr;

	if (bObjectIsOwnedByPython)
	{
		int32 RemoveCount = PythonOwnedObjects.Remove(InObject);
		check(RemoveCount == 1 || RemoveCount == 0);
	}
	else
	{
		check(!PythonOwnedObjects.Contains(InObject));
		// 从Tracker移除，减少引用计数
		Py_DECREF(Tracker.PyObj);
	}
}

void FNePyHouseKeeper::AddUserStruct(const UScriptStruct* InScriptStruct, FNePyUserStruct* UserStruct)
{
	check(IsInGameThread());
	TSet<FNePyUserStruct*>& UserStructs = UserStructMap.FindOrAdd(InScriptStruct);
	check(!UserStructs.Contains(UserStruct));
	UserStructs.Add(UserStruct);
}

void FNePyHouseKeeper::RemoveUserStruct(const UScriptStruct* InScriptStruct, FNePyUserStruct* UserStruct)
{
	check(IsInGameThread());
	TSet<FNePyUserStruct*>* UserStructs = UserStructMap.Find(InScriptStruct);
	if (UserStructs)
	{
		UserStructs->Remove(UserStruct);
	}
}

void FNePyHouseKeeper::InvalidateUserStructs(const UScriptStruct* InScriptStruct)
{
	check(IsInGameThread());
	check(!EnumHasAnyFlags(InScriptStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
	TSet<FNePyUserStruct*>* UserStructs = UserStructMap.Find(InScriptStruct);
	if (UserStructs)
	{
		for (FNePyUserStruct* UserStruct : *UserStructs)
		{
			check(UserStruct->Value);
			if (UserStruct->SelfCreatedValue)
			{
				InScriptStruct->DestroyStruct(UserStruct->Value);
				FNePyStructBase::FreeValuePtr(UserStruct);
				UserStruct->SelfCreatedValue = false;
			}
			if (UserStruct->PyOuter)
			{
				Py_DECREF(UserStruct->PyOuter);
				UserStruct->PyOuter = nullptr;
			}
		}
		UserStructMap.Remove(InScriptStruct);
	}
}

#if WITH_EDITOR
void FNePyHouseKeeper::HandleObjectsReinstanced(const TMap<UObject*, UObject*>& OldToNewInstanceMap)
{
	// 更新Python对UE对象的引用
	for (auto& ObjPair : OldToNewInstanceMap)
	{
		UObject* OldObject = ObjPair.Key;
		UObject* NewObject = ObjPair.Value;
		if (!NewObject || OldObject == NewObject)
		{
			continue;
		}

		if (TrackerMap.Contains(NewObject))
		{
			// Python中已持有新对象引用
			// 根据Python对象与UE对象一一对应原则，需要销毁旧对象（会延迟至NotifyUObjectDeleted函数中执行）。
			continue;
		}

		FObjectReferenceTracker Tracker;
		if (!TrackerMap.RemoveAndCopyValue(OldObject, Tracker))
		{
			continue;
		}

		check(Tracker.PyObj->Value == OldObject);
		Tracker.PyObj->Value = NewObject;

		if (Tracker.PyPropObjs.Num() > 0)
		{
			FNePyScopedGIL GIL;
			// todo: 现在直接清理掉属性引用，脚本层如果持有了就会变成Invalid状态
			//       未来也许应该想个办法把属性对应上，并更新py对象？
			for (auto& Pair : Tracker.PyPropObjs)
			{
				Pair.Value->Value = nullptr;
				// 从Tracker移除，减少引用计数
				Py_DECREF(Pair.Value);
			}
			Tracker.PyPropObjs.Empty();
		}

		if (Tracker.MulticastDelegateUsages.Num() > 0)
		{
			FNePyScopedGIL GIL;
			for (FNePyDynamicMulticastDelegateWrapper* DelegateWrapper : TArray(Tracker.MulticastDelegateUsages))
			{
				// 更新多播委托的UE对象引用
				if (DelegateWrapper->Value)
				{
					DelegateWrapper->ReinstanceUObjectDynamic(OldObject, NewObject);
				}
			}
		}

		UClass* NewClass = NewObject->GetClass();

#if !UE_BUILD_SHIPPING
		Tracker.ObjectName = NewObject->GetFName();
		Tracker.ClassName = NewClass->GetFName();
#endif // !UE_BUILD_SHIPPING

		// replace python type
		{
			FNePyScopedGIL GIL;
			PyTypeObject* OldPyType = Py_TYPE(Tracker.PyObj);
			PyTypeObject* NewPyType = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(NewClass)->TypeObject;
			if (OldPyType != NewPyType)
			{
				if (NewPyType->tp_flags & Py_TPFLAGS_HEAPTYPE)
				{
					Py_INCREF(NewPyType);
				}
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 10
				Py_SET_TYPE(Tracker.PyObj, NewPyType);
#else
				Py_TYPE(Tracker.PyObj) = NewPyType;
#endif
				if (OldPyType->tp_flags & Py_TPFLAGS_HEAPTYPE)
				{
					Py_DECREF(OldPyType);
				}
			}
		}

		check(!TrackerMap.Contains(NewObject));
		TrackerMap.Add(NewObject, MoveTemp(Tracker));
	}
}

void FNePyHouseKeeper::HandleUserStructsReinstance(UScriptStruct* StructToReinstance, UScriptStruct* DuplicatedStruct)
{
	check(IsInGameThread());
	check(!EnumHasAnyFlags(StructToReinstance->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));
	check(!EnumHasAnyFlags(DuplicatedStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));

#if DO_CHECK
	if (UNePyGeneratedStruct* PyDuplicatedStruct = Cast<UNePyGeneratedStruct>(DuplicatedStruct))
	{
		check(PyDuplicatedStruct->NewStruct == StructToReinstance);
	}
#endif
	
	TSet<FNePyUserStruct*>* UserStructs = UserStructMap.Find(StructToReinstance);
	if (!UserStructs)
	{
		return;
	}

	if (UserStructs->Num() == 0)
	{
		UserStructMap.Remove(StructToReinstance);
		return;
	}

	const uint8* OldDefaults;
	if (UNePyGeneratedStruct* PyDuplicatedStruct = Cast<UNePyGeneratedStruct>(DuplicatedStruct))
	{
		OldDefaults = PyDuplicatedStruct->DefaultStructInstance.GetStructMemory();
	}
	else
	{
		OldDefaults = CastChecked<UUserDefinedStruct>(DuplicatedStruct)->GetDefaultInstance();
	}

	const uint8* NewDefaults;
	if (UNePyGeneratedStruct* PyStructToReinstance = Cast<UNePyGeneratedStruct>(StructToReinstance))
	{
		NewDefaults = PyStructToReinstance->DefaultStructInstance.GetStructMemory();
	}
	else
	{
		NewDefaults = CastChecked<UUserDefinedStruct>(StructToReinstance)->GetDefaultInstance();
	}

	PyTypeObject* NewPyType = FNePyWrapperTypeRegistry::Get().GetWrappedStructTypeIfExist(StructToReinstance)->TypeObject;

	TArray<FNePyUserStruct*> NonSelfCreateUserStructs;
	TMap<void*, void*> OldToNewValuePtrMap;

	for (FNePyUserStruct* UserStruct : *UserStructs)
	{
		check(UserStruct->Value);

		if (!UserStruct->SelfCreatedValue)
		{
			// Skip non-self-created struct reinstance
			NonSelfCreateUserStructs.Add(UserStruct);
			continue;
		}

		void* OldValuePtr = UserStruct->Value;
		void* NewValuePtr = FMemory::Malloc(StructToReinstance->GetStructureSize() ? StructToReinstance->GetStructureSize() : 1, StructToReinstance->GetMinAlignment());
		StructToReinstance->InitializeStruct(NewValuePtr);

		// check UObject properties validity
		for (TFieldIterator<FProperty> It(DuplicatedStruct); It; ++It)
		{
			FProperty* Property = *It;
			if (FObjectProperty* ObjectProp = CastField<FObjectProperty>(Property))
			{
				UObject* Obj = ObjectProp->GetObjectPropertyValue_InContainer(OldValuePtr);
				if (Obj && (!Obj->IsValidLowLevel() || !::IsValid(Obj)))
				{
					UE_LOG(LogNePython, Warning, TEXT("Invalid UObject found in property '%s' of struct '%s'. It may have been garbage collected."), *ObjectProp->GetName(), *DuplicatedStruct->GetName());
					ObjectProp->SetObjectPropertyValue_InContainer(OldValuePtr, nullptr);
				}
			}
		}

		// copy properties
		FNePyCopyPropWriter Writer(DuplicatedStruct, (uint8*)OldValuePtr, OldDefaults);
		FNePyCopyPropReader Reader(Writer, StructToReinstance, (uint8*)NewValuePtr, NewDefaults);

		// replace python type
		PyTypeObject* OldPyType = Py_TYPE(UserStruct);
		if (OldPyType != NewPyType)
		{
			FNePyScopedGIL GIL;
			check(Py_TYPE(UserStruct) == OldPyType);
			check(OldPyType->tp_flags & Py_TPFLAGS_HEAPTYPE);
			check(NewPyType->tp_flags & Py_TPFLAGS_HEAPTYPE);
			Py_INCREF(NewPyType);
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 10
			Py_SET_TYPE(UserStruct, NewPyType);
#else
			Py_TYPE(UserStruct) = NewPyType;
#endif
			Py_DECREF(OldPyType);
		}

		DuplicatedStruct->DestroyStruct(OldValuePtr);
		FMemory::Free(OldValuePtr);
		UserStruct->Value = NewValuePtr;

		// record the mapping for properties copying
		OldToNewValuePtrMap.Add(OldValuePtr, NewValuePtr);
	}

	for (FNePyUserStruct* UserStruct : NonSelfCreateUserStructs)
	{
		check(UserStruct->Value);
		void** NewValuePtrPtr =	OldToNewValuePtrMap.Find(UserStruct->Value);
		if (NewValuePtrPtr)
		{
			UserStruct->Value = *NewValuePtrPtr;
		}
		else
		{
			UserStruct->Value = nullptr;
			UserStructs->Remove(UserStruct);
		}
	}
}
#endif // WITH_EDITOR

void FNePyHouseKeeper::InitSingleton()
{
	check(!Singleton);
	Singleton = new FNePyHouseKeeper();
	GUObjectArray.AddUObjectDeleteListener(&Singleton->ObjectDeleteListener);

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION < 5
	if (GEditor)
	{
		GEditor->OnObjectsReplaced().AddRaw(Singleton, &FNePyHouseKeeper::HandleObjectsReinstanced);
	}
#else
	FCoreUObjectDelegates::OnObjectsReplaced.AddRaw(Singleton, &FNePyHouseKeeper::HandleObjectsReinstanced);
#endif // ENGINE_MAJOR_VERSION
#endif // WITH_EDITOR

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	FCoreUObjectDelegates::OnObjectsReinstanced.AddRaw(Singleton, &FNePyHouseKeeper::HandleObjectsReinstanced);
#endif // ENGINE_MAJOR_VERSION
#endif // WITH_EDITOR
}

void FNePyHouseKeeper::OnPreExit()
{
	GUObjectArray.RemoveUObjectDeleteListener(&Singleton->ObjectDeleteListener);

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION < 5
	if (GEditor)
	{
		GEditor->OnObjectsReplaced().RemoveAll(Singleton);
	}
#else
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(Singleton);
#endif // ENGINE_MAJOR_VERSION
#endif // WITH_EDITOR

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	FCoreUObjectDelegates::OnObjectsReinstanced.RemoveAll(Singleton);
#endif // ENGINE_MAJOR_VERSION
#endif // WITH_EDITOR
}

FNePyHouseKeeper* FNePyHouseKeeper::Singleton;

void FNePyHouseKeeper::FObjectListener::NotifyUObjectDeleted(const UObjectBase* InObject, int32 InObjectIndex)
{
	check(IsInGameThread());
	FNePyHouseKeeper& HouseKeeper = FNePyHouseKeeper::Get();

	const UObject* Object = (const UObject*)InObject;
	if (Object->IsA<UField>()) // 性能优化：这个判断可以跳过绝大部分的Case
	{
		if (const UClass* Class = Cast<UClass>(Object))
		{
			if (Object->IsA<UBlueprintGeneratedClass>() || Object->IsA<UNePyGeneratedClass>())
			{
				FNePyWrapperTypeRegistry::Get().UnregisterWrappedClassType(Class);
			}
		}
		else if (const UScriptStruct* Struct = Cast<UScriptStruct>(Object))
		{
			if (Object->IsA<UUserDefinedStruct>() || Object->IsA<UNePyGeneratedStruct>())
			{
				HouseKeeper.InvalidateUserStructs(Struct);
				FNePyWrapperTypeRegistry::Get().UnregisterWrappedStructType(Struct);
			}
		}
		else if (const UEnum* Enum = Cast<UEnum>(Object))
		{
			if (Object->IsA<UUserDefinedEnum>() || Object->IsA<UNePyGeneratedEnum>())
			{
				FNePyWrapperTypeRegistry::Get().UnregisterWrappedEnumType(Enum);
			}
		}
	}
	
#if !UE_BUILD_SHIPPING
	TotalNotifyCount += 1;
#endif

	if (!HouseKeeper.TrackerMap.Contains(Object))
	{
		// TODO: twx TotalNotifyCount比CutoffPythonCount高出两个数量级，此处*也许*有性能热点
		// 一个解决思路是利用上ObjectFlags里空白的位，但需要改引擎
		return;
	}

#if !UE_BUILD_SHIPPING
	CutoffPythonCount += 1;
#endif

	FObjectReferenceTracker Tracker = HouseKeeper.TrackerMap.FindAndRemoveChecked(Object);

	{
		FNePyScopedGIL GIL;

		for (auto& Pair : Tracker.PyPropObjs)
		{
			Pair.Value->Value = nullptr;
			// 从Tracker移除，减少引用计数
			Py_DECREF(Pair.Value);
		}

		for (FNePyDynamicMulticastDelegateWrapper* DelegateWrapper : TArray(Tracker.MulticastDelegateUsages))
		{
			if (DelegateWrapper->Value)
			{
				DelegateWrapper->RemoveUObjectAllDynamic(Object);
			}
		}

		check(Tracker.PyObj->Value == InObject);
		Tracker.PyObj->Value = nullptr;

		if (!HouseKeeper.PythonOwnedObjects.Remove(Object))
		{
			// 从Tracker移除，减少引用计数
			Py_DECREF(Tracker.PyObj);
		}
	}
}

void FNePyHouseKeeper::FObjectListener::OnUObjectArrayShutdown()
{
	GUObjectArray.RemoveUObjectDeleteListener(this);
}
