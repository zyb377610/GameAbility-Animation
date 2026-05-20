#include "NePyGeneratedStruct.h"
#include "NePyGeneratedType.h"
#include "NePyGeneratedClass.h"
#include "NePyBase.h"
#include "NePyDynamicType.h"
#include "NePyUserStruct.h"
#include "NePyUserTableRow.h"
#include "NePySubclassing.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"
#include "NePyUtil.h"
#include "NePyReloadUtils.h"
#include "NePyDescriptor.h"
#include "UObject/LinkerLoad.h"
#include "UObject/StructOnScope.h"
#include "Blueprint/BlueprintSupport.h"
#include "Engine/DataTable.h"
#include "HAL/IConsoleManager.h"

extern TAutoConsoleVariable<int32> CVarSubclassingLog;

// NePyGeneratedStruct 有时候一些操作不会走ScriptStruct的虚函数，会直接调用 CppStructOps
// 黑一个自定义实现，现在用于初始化 SuperStruct 的虚函数指针。后面缺啥补啥
struct FNePyStructOps : public UScriptStruct::ICppStructOps
{
	static FNePyStructOps* CreateNePyStructOpsByStruct(const UScriptStruct* InStruct)
	{
		const UScriptStruct* SuperStruct = (UScriptStruct*)InStruct->GetSuperStruct();
		ICppStructOps* NativeStructOps = nullptr;
		while (NativeStructOps == nullptr && SuperStruct != nullptr)
		{
			if (Cast<const UNePyGeneratedStruct>(SuperStruct))
			{
				SuperStruct = (UScriptStruct*)SuperStruct->GetSuperStruct();
				continue;
			}
			NativeStructOps = SuperStruct->GetCppStructOps();
			SuperStruct = (UScriptStruct*)SuperStruct->GetSuperStruct();
		}

		FNePyStructOps* StructOps = new FNePyStructOps(InStruct, NativeStructOps);
		return StructOps;
	}

// 5.1 把各种 HasXXX 的虚函数改成了统一通过 GetCapabilities 来获取。
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6 // UE >= 5.1
	virtual FCapabilities GetCapabilities() const override
	{
		FCapabilities Capabilities;
		FMemory::Memzero(Capabilities);
		Capabilities.HasDestructor = true;
		Capabilities.HasStructuredSerializeFromMismatchedTag = true;
		return Capabilities;
	}
#else // UE < 5.1
	virtual bool HasNoopConstructor() override { return false; }
	virtual bool HasZeroConstructor() override { return false; }
	virtual bool HasDestructor() override { return true; } // return true
	virtual bool HasSerializer() override { return false; }
	virtual bool HasStructuredSerializer() override { return false; }
	virtual bool HasPostSerialize() override { return false; }
	virtual bool HasNetSerializer() override { return false; }
	virtual bool HasNetSharedSerialization() override { return false; }
	virtual bool HasNetDeltaSerializer() override { return false; }
	virtual bool HasPostScriptConstruct() override { return false; }
	virtual bool IsPlainOldData() override { return false; }
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
	virtual bool IsUECoreType() override { return false; }
	virtual bool IsUECoreVariant() override { return false; }
#endif
	virtual bool HasCopy() override { return false; }
	virtual bool HasIdentical() override { return false; }
	virtual bool HasExportTextItem() override { return false; }
	virtual bool HasImportTextItem() override { return false; }
	virtual bool HasAddStructReferencedObjects() override { return false; }
	virtual bool HasSerializeFromMismatchedTag() override { return false; }
	virtual bool HasStructuredSerializeFromMismatchedTag() override { return true; }
	virtual bool HasGetTypeHash() override { return false; }
	virtual EPropertyFlags GetComputedPropertyFlags() const override { return EPropertyFlags::CPF_None; }
	virtual bool IsAbstract() const override { return false; }
#endif

// UE 每个版本都往 ICppStructOps 里面加了一两个新接口
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 27 || ENGINE_MAJOR_VERSION >= 5
	virtual void ConstructForTests(void* Dest) override { return Construct(Dest); }
#endif // UE >= 4.27

#if ENGINE_MAJOR_VERSION >= 5 
	virtual void GetPreloadDependencies(void* Data, TArray<UObject*>& OutDeps) override {}
#endif // UE >= 5.0

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
#if WITH_EDITOR
	virtual bool CanEditChange(const FEditPropertyChain& PropertyChain, const void* Data) const override { return true; }
#endif // WITH_EDITOR
#endif // UE >= 5.1

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	virtual void InitializeIntrusiveUnsetOptionalValue(void* Data) const override {}
	virtual bool IsIntrusiveOptionalValueSet(const void* Data) const override { return false; }
	virtual void ClearIntrusiveOptionalValue(void* Data) const override {}
	virtual bool IsIntrusiveOptionalSafeForGC() const override { return true; }
#if ENGINE_MINOR_VERSION < 6
	virtual EPropertyVisitorControlFlow Visit(FPropertyVisitorPath& Path, const FPropertyVisitorData& Data, const TFunctionRef<EPropertyVisitorControlFlow(const FPropertyVisitorPath& /*Path*/, const FPropertyVisitorData& /*Data*/)> InFunc) const override { return InFunc(Path, Data); }
#else // UE >= 5.6
	virtual EPropertyVisitorControlFlow Visit(FPropertyVisitorContext& Context, const TFunctionRef<EPropertyVisitorControlFlow(const FPropertyVisitorContext& /*Conmtext*/)> InFunc) const override { return InFunc(Context); }
#endif
	virtual void* ResolveVisitedPathInfo(void* Data, const FPropertyVisitorInfo& Info) const override { return nullptr; }
#endif

	virtual void Construct(void* Dest) override
	{
		int32 InitializedSize = 0;
		int32 PropertiesSize = GetSize();
		if (NativeStructOps != nullptr)
		{
			if (!NativeStructOps->HasZeroConstructor())
			{
				void* PropertyDest = Dest;
				NativeStructOps->Construct(PropertyDest);
			}

			InitializedSize = NativeStructOps->GetSize();
		}

		if (PropertiesSize > InitializedSize)
		{
			bool bHitBase = false;
			for (FProperty* Property = Struct->PropertyLink; Property && !bHitBase; Property = Property->PropertyLinkNext)
			{
				if (!Property->IsInContainer(InitializedSize))
				{
					Property->InitializeValue_InContainer(Dest);
				}
				else
				{
					bHitBase = true;
				}
			}
		}
	}

	virtual void Destruct(void* Dest) override
	{
		int32 ClearedSize = 0;
		int32 PropertiesSize = GetSize();
		if (NativeStructOps)
		{
			if (NativeStructOps->HasDestructor())
			{
				NativeStructOps->Destruct((uint8*)Dest);
			}
			ClearedSize = NativeStructOps->GetSize();
		}

		if (PropertiesSize > ClearedSize)
		{
			bool bHitBase = false;
			for (FProperty* P = Struct->DestructorLink; P && !bHitBase; P = P->DestructorLinkNext)
			{
				if (!P->IsInContainer(ClearedSize))
				{
					if (!P->HasAnyPropertyFlags(CPF_NoDestructor))
					{
						P->DestroyValue_InContainer((uint8*)Dest);
					}
				}
				else
				{
					bHitBase = true;
				}
			}
		}
	}

	// 可能下面这些不应该使用Cpp的实现，可能需要自己添加额外逻辑。但我不确定，所以先抄一遍再说。
	virtual bool Copy(void* Dest, void const* Src, int32 ArrayDim) override { return false; }
	virtual bool Identical(const void* A, const void* B, uint32 PortFlags, bool& bOutResult) override { return false; }

	virtual bool Serialize(FArchive& Ar, void* Data) override { return false; }
	virtual bool Serialize(FStructuredArchive::FSlot Slot, void* Data) override { return false; }
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	virtual bool Serialize(FArchive& Ar, void* Data, UStruct* DefaultsStruct, const void* Defaults) override { return false; }
	virtual bool Serialize(FStructuredArchive::FSlot Slot, void* Data, UStruct* DefaultsStruct, const void* Defaults) override { return false; }
#endif

	virtual void PostSerialize(const FArchive& Ar, void* Data) override {}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess, void* Data) override { return false; }
	virtual bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms, void* Data) override { return false; }
	virtual bool ExportTextItem(FString& ValueStr, const void* PropertyValue, const void* DefaultValue, class UObject* Parent, int32 PortFlags, class UObject* ExportRootScope) override { return false; }
	virtual bool ImportTextItem(const TCHAR*& Buffer, void* Data, int32 PortFlags, class UObject* OwnerObject, FOutputDevice* ErrorText) override { return false; }
	virtual bool SerializeFromMismatchedTag(struct FPropertyTag const& Tag, FArchive& Ar, void* Data) override { return false; }

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6)
	// 简单的forward给super
	virtual bool FindInnerPropertyInstance(FName PropertyName, const void* Data, const FProperty*& OutProp, const void*& OutData) const override
	{
		if (NativeStructOps)
			return NativeStructOps->FindInnerPropertyInstance(PropertyName, Data, OutProp, OutData);
		return false;
	}
#endif

	// 处理旧结构体实例Reinstance
	virtual bool StructuredSerializeFromMismatchedTag(struct FPropertyTag const& Tag, FStructuredArchive::FSlot Slot, void* Data) override
	{
#if WITH_EDITOR
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4 || ENGINE_MAJOR_VERSION >= 6)
		UNePyGeneratedStruct* NewStruct = Cast<UNePyGeneratedStruct>(CastFieldChecked<FStructProperty>(Tag.GetProperty())->Struct);
		UNePyGeneratedStruct* OldStruct = FindObject<UNePyGeneratedStruct>(GetTransientPackage(), *Tag.GetType().GetParameterName(0).ToString(), true);
#else
		UNePyGeneratedStruct* NewStruct = Cast<UNePyGeneratedStruct>(CastFieldChecked<FStructProperty>(Tag.Prop)->Struct);
		UNePyGeneratedStruct* OldStruct = FindObject<UNePyGeneratedStruct>(GetTransientPackage(), *Tag.StructName.ToString(), true);
#endif
		if (!NewStruct || !OldStruct)
		{
			return false;
		}

		check(OldStruct->NewStruct == NewStruct);

		// 先将旧结构体数据解析出来
		FStructOnScope StructInstance(OldStruct);
		OldStruct->SerializeTaggedProperties(Slot, StructInstance.GetStructMemory(), OldStruct, OldStruct->DefaultStructInstance.GetStructMemory());

		// 随后将旧结构体数据拷贝到新结构体上
		FNePyCopyPropWriter Writer(OldStruct, StructInstance.GetStructMemory(), OldStruct->DefaultStructInstance.GetStructMemory());
		FNePyCopyPropReader Reader(Writer, NewStruct, (uint8*)Data, NewStruct->DefaultStructInstance.GetStructMemory());
		return true;
#else
		return false;
#endif
	}

	virtual void PostScriptConstruct(void* Data) override {}

	virtual uint32 GetStructTypeHash(const void* Src) override { return 0; }

	virtual TPointerToAddStructReferencedObjects AddStructReferencedObjects() override { return nullptr; }

protected:
	FNePyStructOps(const UScriptStruct* InStruct, ICppStructOps* InNativeStructOps = nullptr) : ICppStructOps(InStruct->GetStructureSize(), InStruct->GetMinAlignment()), Struct(InStruct), NativeStructOps(InNativeStructOps) {}

private:
	const UScriptStruct* Struct = nullptr;
	ICppStructOps* NativeStructOps = nullptr;
};

FNePyGeneratedStructOnScopeIgnoreDefaults::FNePyGeneratedStructOnScopeIgnoreDefaults(const UNePyGeneratedStruct* InUserStruct, uint8* InData) : FStructOnScope(InUserStruct, InData)
{
}

FNePyGeneratedStructOnScopeIgnoreDefaults::FNePyGeneratedStructOnScopeIgnoreDefaults(const UNePyGeneratedStruct* InUserStruct)
{
	// Can't call super constructor because we need to call our overridden initialize
	ScriptStruct = InUserStruct;
	SampleStructMemory = nullptr;
	OwnsMemory = false;
	FNePyGeneratedStructOnScopeIgnoreDefaults::Initialize();
}

void FNePyGeneratedStructOnScopeIgnoreDefaults::Recreate(const UNePyGeneratedStruct* InUserStruct)
{
	Destroy();
	ScriptStruct = InUserStruct;
	Initialize();
}

void FNePyGeneratedStructOnScopeIgnoreDefaults::Initialize()
{
	if (const UStruct* ScriptStructPtr = ScriptStruct.Get())
	{
		SampleStructMemory = (uint8*)FMemory::Malloc(ScriptStructPtr->GetStructureSize() ? ScriptStructPtr->GetStructureSize() : 1);
		((UNePyGeneratedStruct*)ScriptStruct.Get())->InitializeStructIgnoreDefaults(SampleStructMemory);
		OwnsMemory = true;
	}
}

UNePyGeneratedStruct::~UNePyGeneratedStruct()
{
	SetCppStructOps(nullptr);
}

void UNePyGeneratedStruct::BeginDestroy()
{
	ReleasePythonResources();
#if WITH_EDITOR
	if (Specifiers.Num() > 0)
	{
		FNePySpecifier::ReleaseSpecifiers(Specifiers);
	}
#endif // WITH_EDITOR
	Super::BeginDestroy();
}

void UNePyGeneratedStruct::ReleasePythonResources()
{
	if (PyType || PyInitDefaultFunc)
	{
		FNePyScopedGIL GIL;

		Py_XDECREF(PyType);
		PyType = nullptr;

		Py_XDECREF(PyInitDefaultFunc);
		PyInitDefaultFunc = nullptr;
	}
}

class FNePyGeneratedStructBuilder
{
public:
	FNePyGeneratedStructBuilder(const FString& InStructName, UScriptStruct* InSuperStruct, PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs)
		: StructName(InStructName)
		, PyType(InPyType)
	{
		UObject* StructOuter = GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Runtime);

		// Find any existing struct with the name we want to use
		OldStruct = FindObject<UNePyGeneratedStruct>(StructOuter, *StructName);

		// Create a new struct with a temporary name; we will rename it as part of Finalize
		const FString NewStructName = MakeUniqueObjectName(StructOuter, UNePyGeneratedStruct::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *StructName)).ToString();
		NewStruct = NewObject<UNePyGeneratedStruct>(StructOuter, *NewStructName, RF_Public | RF_NePyGeneratedTypeGCSafe);
		NewStruct->SetSuperStruct(InSuperStruct);

		// 如果存在旧类，复用旧类作为最终生成的类，这样我们就不需要修复对旧类的引用了。
		FinalStruct = OldStruct ? OldStruct : NewStruct;

		Specifiers = FNePySpecifier::ParseSpecifiers(InPySpecifierPairs, FNePySpecifier::Scope_Struct);
	}

	FNePyGeneratedStructBuilder(UNePyGeneratedStruct* InOldStruct)
		: StructName(InOldStruct->GetName())
		, PyType(InOldStruct->PyType)
		, OldStruct(InOldStruct)
	{
		UObject* StructOuter = GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Runtime);

		// Create a new struct with a temporary name; we will rename it as part of Finalize
		const FString NewStructName = MakeUniqueObjectName(StructOuter, UNePyGeneratedStruct::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *StructName)).ToString();
		NewStruct = NewObject<UNePyGeneratedStruct>(StructOuter, *NewStructName, RF_Public | RF_NePyGeneratedTypeGCSafe);
		NewStruct->SetSuperStruct(InOldStruct->GetSuperStruct());

		// 如果存在旧类，复用旧类作为最终生成的类，这样我们就不需要修复对旧类的引用了。
		FinalStruct = OldStruct ? OldStruct : NewStruct;

#if WITH_EDITOR
		if (FinalStruct->Specifiers.Num() > 0)
		{
			Specifiers = FinalStruct->Specifiers;
			FinalStruct->Specifiers.Empty();
		}
#endif // WITH_EDITOR
	}

	~FNePyGeneratedStructBuilder()
	{
#if WITH_EDITOR
		if (FinalStruct->Specifiers.Num() > 0)
		{
			FNePySpecifier::ReleaseSpecifiers(FinalStruct->Specifiers);
		}
		FinalStruct->Specifiers = Specifiers;
#else
		FNePySpecifier::ReleaseSpecifiers(Specifiers);
#endif // WITH_EDITOR

		// If NewStruct is still set at this point, if means Finalize wasn't called and we should destroy the partially built struct
		if (NewStruct)
		{
			NewStruct->ClearFlags(RF_AllFlags);
			NewStruct->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
			NewStruct = nullptr;

			Py_BEGIN_ALLOW_THREADS
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			Py_END_ALLOW_THREADS
		}
	}

	UNePyGeneratedStruct* Finalize()
	{
		int32 Level = CVarSubclassingLog.GetValueOnGameThread();

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("========================================"));
			UE_LOG(LogNePython, Log, TEXT("Finalizing Python Generated Struct: %s"), *StructName);
			UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s, tp_basicsize=%d"), 
				PyType, 
				UTF8_TO_TCHAR(PyType->tp_name),
				(int32)PyType->tp_basicsize);
			UE_LOG(LogNePython, Log, TEXT("  OldStruct=%p, NewStruct=%p, FinalStruct=%p"), 
				OldStruct, 
				NewStruct, 
				FinalStruct);
			UE_LOG(LogNePython, Log, TEXT("  SuperStruct=%s [%p]"), 
				NewStruct->GetSuperStruct() ? *NewStruct->GetSuperStruct()->GetName() : TEXT("None"),
				NewStruct->GetSuperStruct());
			UE_LOG(LogNePython, Log, TEXT("  NewStruct Flags: 0x%08X"), (uint32)NewStruct->StructFlags);
		}

		// Replace the definitions with real descriptors
		TArray<FNePyObjectPtr> PyDescriptors;
		if (!RegisterDescriptors(PyDescriptors))
		{
			return nullptr;
		}

		PyType->tp_alloc = FNePyUserStruct::Alloc; // 覆写heap type原生行为，使用我们提供的tp_alloc
		PyType->tp_dealloc = FNePyUserStruct::Dealloc; // 覆写heap type原生行为，使用我们提供的tp_dealloc

		// We can no longer fail, so prepare the old struct for removal and set the correct name on the new struct
		if (OldStruct)
		{
			ReplaceOldStructProperties();
			if (OldStruct->PyType != PyType)
			{
				FNePyWrapperTypeRegistry::Get().UnregisterWrappedStructType(OldStruct);
#if WITH_EDITOR
				if (Reload.IsValid())
				{
					FNePyWrapperTypeRegistry::Get().AddReinstancingPyTypeToStruct(Reload->DuplicatedStruct->PyType, FinalStruct);
				}
#endif
			}
			else
			{
				NePyType_CleanupDescriptors(PyType);
			}
		}
		
		for (auto& PyObjPtr : PyDescriptors)
		{
			FNePyDescriptorBase* PyDescr = (FNePyDescriptorBase*)PyObjPtr.Get();
			PyDict_SetItem(PyType->tp_dict, PyDescr->Name, PyDescr);
		}
		PyType_Modified(PyType);

		{
			Py_BEGIN_ALLOW_THREADS
			if (!OldStruct)
			{
				NewStruct->Rename(*StructName, nullptr, REN_DontCreateRedirectors);
			}

			// Finalize the struct
			FinalStruct->Bind();
			FinalStruct->StaticLink(true);
			FinalStruct->GenerateDefaultInstance();
			Py_END_ALLOW_THREADS
		}

		if (!OldStruct || OldStruct->PyType != PyType)
		{
			Py_INCREF(PyType);
			Py_XDECREF(FinalStruct->PyType);
			FinalStruct->PyType = PyType;

			// 避免用户直接在Python中定义__init__
			if (FinalStruct->PyType->tp_init && FinalStruct->PyType->tp_init != NePyUserStructGetType()->tp_init)
			{
				UE_LOG(LogNePython, Warning, TEXT(
					"User defined '__init__' in subclassing type '%s', which is not allowed and will take no effect."
					" Use '__init_default__' instead if you want to define default values in unreal type."
					), UTF8_TO_TCHAR(FinalStruct->PyType->tp_name));
				FinalStruct->PyType->tp_init = NePyUserStructGetType()->tp_init;
			}

			// Map the Unreal struct to the Python type
			FNePyStructTypeInfo TypeInfo = {
				PyType,
				ENePyTypeFlags::ScriptPyType,
				(NePyStructPropSet)FNePyDynamicStructType::PropSet,
				(NePyStructPropGet)FNePyDynamicStructType::PropGet,
			};
			FNePyWrapperTypeRegistry::Get().RegisterWrappedStructType(FinalStruct, TypeInfo);
		}

#if WITH_EDITORONLY_DATA
		FString Path = NePyGenUtil::GetPythonModuleRelativePathForPyType(PyType);
		if (!Path.IsEmpty())
		{
			FinalStruct->SetMetaData(TEXT("ModuleRelativePath"), *Path);
		}

		// Apply the doc string as the ustruct tooltip
		const FString DocString = NePyUtil::GetDocString((PyObject*)PyType);
		if (!DocString.IsEmpty())
		{
			FinalStruct->SetMetaData(TEXT("ToolTip"), *DocString);
		}
#endif // WITH_EDITORONLY_DATA

		// 应用说明符
		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Applying specifiers (%d)..."), Specifiers.Num());
		}
		FNePySpecifier::ApplyToStruct(Specifiers, FinalStruct);

		// Call InitDefault to init default struct instance
		FinalStruct->InitDefaultValueForProperties();
		FinalStruct->CallInitDefaultFunc();

#if WITH_EDITOR
		// Re-instance the old struct
		if (Reload.IsValid())
		{
			check(Reload->ReloadingStruct == FinalStruct);
			FNePyHouseKeeper::Get().HandleUserStructsReinstance(Reload->ReloadingStruct, Reload->DuplicatedStruct);
			Reload->Finalize();
			FNePyWrapperTypeRegistry::Get().RemoveReinstancingPyTypeToStruct(Reload->DuplicatedStruct->PyType);
			Reload.Reset();
		}
#endif

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		NotifyRegistrationEvent(*FinalStruct->GetPackage()->GetName(), *StructName, ENotifyRegistrationType::NRT_Struct, ENotifyRegistrationPhase::NRP_Finished, nullptr, false, FinalStruct);
#endif

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Finalization complete!"));
			UE_LOG(LogNePython, Log, TEXT("    FinalStruct=%p, Name=%s"), FinalStruct, *FinalStruct->GetName());
			UE_LOG(LogNePython, Log, TEXT("    Package=%s [%p]"), *FinalStruct->GetPackage()->GetName(), FinalStruct->GetPackage());
			UE_LOG(LogNePython, Log, TEXT("    DefaultInstance=%p"), FinalStruct->GetDefaultInstance());
			UE_LOG(LogNePython, Log, TEXT("    Structure Size: %d bytes"), FinalStruct->GetStructureSize());
			UE_LOG(LogNePython, Log, TEXT("    MinAlignment: %d"), FinalStruct->GetMinAlignment());
			UE_LOG(LogNePython, Log, TEXT("    StructFlags: 0x%08X"), (uint32)FinalStruct->StructFlags);
			UE_LOG(LogNePython, Log, TEXT("    PyType=%p, refcount=%d"), 
				FinalStruct->PyType, 
				(int32)Py_REFCNT(FinalStruct->PyType));
			UE_LOG(LogNePython, Log, TEXT("    PyInitDefaultFunc=%p"), FinalStruct->PyInitDefaultFunc);
			
			if (Level >= 2)
			{
				// Summary of property layout
				UE_LOG(LogNePython, Log, TEXT("  Property Layout Summary:"));
				for (TFieldIterator<FProperty> It(FinalStruct); It; ++It)
				{
					FProperty* Prop = *It;
					UE_LOG(LogNePython, Log, TEXT("    [%4d] %s: %s (%d bytes)"),
						Prop->GetOffset_ForInternal(),
						*Prop->GetName(),
						*NePyUtil::GetPropertyTypeString(Prop),
						Prop->GetSize());
				}
			}
			
			UE_LOG(LogNePython, Log, TEXT("========================================"));
		}

		if (!OldStruct)
		{
			// Null the NewStruct pointer so the destructor doesn't kill it
			NewStruct = nullptr;
		}
		return FinalStruct;
	}

	bool CreatePropertyFromDefinition(const char* InFieldName, const FNePyFPropertyDef* InPyPropDef)
	{
		UScriptStruct* SuperStruct = Cast<UScriptStruct>(NewStruct->GetSuperStruct());

		// Resolve the property name to match any previously exported properties from the parent type
		const FName PropName(InFieldName);
		if (SuperStruct && SuperStruct->FindPropertyByName(PropName))
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) cannot override a property from the base type", PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Structs cannot support getter/setter functions (or any functions)
		if (!InPyPropDef->GetterFuncName.IsEmpty() || !InPyPropDef->SetterFuncName.IsEmpty())
		{
			PyErr_Format(PyExc_Exception, "%s: Struct property '%s' (%s) has a getter or setter, which is not supported on structs", PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Create the property from its definition
		FProperty* Prop = NePySubclassingNewProperty(NewStruct, PropName, InPyPropDef->PropType, NewStruct->GetName(), InPyPropDef->UserDefineUPropertyFlags, InPyPropDef->DefaultValue);
		if (!Prop)
		{
			PyErr_Format(PyExc_Exception, "%s: Failed to create property for '%s' (%s)", PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// 应用说明符
		FNePySpecifier::ApplyToProperty(InPyPropDef->Specifiers, Prop);

		NewStruct->AddCppProperty(Prop);
		if (Prop->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
		{
			NewStruct->StructFlags = EStructFlags(NewStruct->StructFlags | STRUCT_HasInstancedReference);
		}

		// Build the definition data for the new property accessor
		NePyGenUtil::FPropertyDef& PropDef = *NewStruct->PropertyDefs.Add_GetRef(MakeShared<NePyGenUtil::FPropertyDef>());
		PropDef.GetSetName = NePyGenUtil::UTF8ToUTF8Buffer(InFieldName);
		PropDef.Prop = Prop;
		// Copy default value out
		if (InPyPropDef->DefaultValue)
		{
			PropDef.DefaultValue = NePyNewReferenceWithGIL(InPyPropDef->DefaultValue);
		}

		return true;
	}

	bool CopyPropertiesFromOldStruct()
	{
		check(OldStruct);

		NewStruct->PropertyDefs.Reserve(OldStruct->PropertyDefs.Num());
		for (const TSharedPtr<NePyGenUtil::FPropertyDef>& OldPropDef : OldStruct->PropertyDefs)
		{
			const FProperty* OldProp = OldPropDef->Prop;

			FProperty* Prop = CastFieldChecked<FProperty>(FField::Duplicate(OldProp, NewStruct, OldProp->GetFName()));
			if (!Prop)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to duplicate property for '%s'", PyType->tp_name, TCHAR_TO_UTF8(*OldProp->GetName()));
				return false;
			}

#if WITH_EDITORONLY_DATA
			FField::CopyMetaData(OldProp, Prop);
#endif // WITH_EDITORONLY_DATA
			NewStruct->AddCppProperty(Prop);
			if (Prop->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
			{
				NewStruct->StructFlags = EStructFlags(NewStruct->StructFlags | STRUCT_HasInstancedReference);
			}

			NePyGenUtil::FPropertyDef& PropDef = *NewStruct->PropertyDefs.Add_GetRef(MakeShared<NePyGenUtil::FPropertyDef>());
			PropDef.GetSetName = OldPropDef->GetSetName;
			PropDef.Prop = Prop;
			PropDef.DefaultValue = OldPropDef->DefaultValue;
		}

		return true;
	}

	void ExtractInitDefaultFunc()
	{
		NewStruct->PyInitDefaultFunc = NePyGenUtil::GetInitDefaultFunc(PyType);
	}

	bool HasOldStruct() const
	{
		return OldStruct != nullptr;
	}

private:
	bool RegisterDescriptors(TArray<FNePyObjectPtr>& OutDescriptors)
	{
		for (const TSharedPtr<NePyGenUtil::FPropertyDef>& PropDef : NewStruct->PropertyDefs)
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(PyType, PropDef->Prop, PropDef->GetSetName.GetData(), false));
			if (!Descr)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for '%s'", PyType->tp_name, PropDef->GetSetName.GetData());
				return false;
			}

			OutDescriptors.Add(MoveTemp(Descr));
		}

		{
			static PyMethodDef NePyGeneratedStructMethod_Struct = { "Struct", NePyCFunctionCast(&FNePyDynamicStructType::Struct), METH_NOARGS | METH_CLASS, "" };
			FNePyObjectPtr Descr = NePyStealReference(PyDescr_NewClassMethod(PyType, &NePyGeneratedStructMethod_Struct));
			if (!Descr || PyDict_SetItemString(PyType->tp_dict, "Struct", Descr) < 0)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for 'Struct'", PyType->tp_name);
				return false;
			}
		}

		return true;
	}

	// 将NewStruct的属性转移给OldStruct
	void ReplaceOldStructProperties()
	{
#if WITH_EDITOR
		Reload.Reset(new FNePyStructReload(OldStruct));
#endif

		// 抄自 FUserDefinedStructureCompilerInner::CleanAndSanitizeStruct
		{
			OldStruct->SetSuperStruct(nullptr);
			OldStruct->Children = nullptr;
			OldStruct->DestroyChildPropertiesAndResetPropertyLinks();
			OldStruct->Script.Empty();
			OldStruct->MinAlignment = 0;
			OldStruct->ScriptAndPropertyObjectReferences.Empty();
			OldStruct->SetStructTrashed(true);
		}

		OldStruct->SetSuperStruct(NewStruct->GetSuperStruct());
		OldStruct->StructFlags = NewStruct->StructFlags;
		OldStruct->PyInitDefaultFunc = NewStruct->PyInitDefaultFunc;
		NewStruct->PyInitDefaultFunc = nullptr;
		OldStruct->PropertyDefs = MoveTemp(NewStruct->PropertyDefs);

		check(!OldStruct->ChildProperties);
		OldStruct->ChildProperties = NewStruct->ChildProperties;
		NewStruct->ChildProperties = nullptr;

		FField* LastFField = OldStruct->ChildProperties;
		while (LastFField)
		{
			check(LastFField->Owner == NewStruct);
			LastFField->Owner = OldStruct;
			LastFField = LastFField->Next;
		}

		NePyGenUtil::UpdateReloadedPropertyStructReferences(OldStruct);
		OldStruct->ClearAndDeleteCppStructOps();
	}

private:
	FString StructName;
	PyTypeObject* PyType;
	TArray<FNePySpecifier*> Specifiers;
	UNePyGeneratedStruct* OldStruct;
	UNePyGeneratedStruct* NewStruct;
	UNePyGeneratedStruct* FinalStruct;
	FNePyObjectPtr InitDefaultFunc;
#if WITH_EDITOR
	TUniquePtr<FNePyStructReload> Reload;
#endif
};

UNePyGeneratedStruct::UNePyGeneratedStruct(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultStructInstance.SetPackage(GetOutermost());
}

UNePyGeneratedStruct* UNePyGeneratedStruct::GenerateStruct(PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs)
{
	PyTypeObject* PyBase = InPyType->tp_base;
	UScriptStruct* SuperStruct = (UScriptStruct*)FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyBase);

	// Builder used to generate the struct
	FNePyGeneratedStructBuilder PythonStructBuilder(InPyType->tp_name, SuperStruct, InPyType, InPySpecifierPairs);

#if !WITH_EDITOR
	if (PythonStructBuilder.HasOldStruct())
	{
		// Subclassing结构体目前只支持在编辑器模式下进行Reload
		PyErr_Format(PyExc_Exception, "Regenerate subclassing struct '%s' is not allowed in standalone build", InPyType->tp_name);
		return nullptr;
	}
#endif

	TArray<TPair<PyObject*, FNePyFPropertyDef*>> PyPropDefs;
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			if (FNePyFPropertyDef* PyPropDef = FNePyFPropertyDef::Check(FieldValue))
			{
				PyPropDefs.Emplace(FieldKey, PyPropDef);
			}
			else if (FNePyUValueDef::Check(FieldValue))
			{
				// Values are not supported on structs
				PyErr_Format(PyExc_Exception, "%s: Structs do not support values", InPyType->tp_name);
				return nullptr;
			}
			else if (FNePyUFunctionDef::Check(FieldValue))
			{
				// Functions are not supported on structs
				PyErr_Format(PyExc_Exception, "%s: Structs do not support methods", InPyType->tp_name);
				return nullptr;
			}
		}
	}

	PyPropDefs.Sort([](const TPair<PyObject*, FNePyFPropertyDef*>& Left, const TPair<PyObject*, FNePyFPropertyDef*>& Right) {
		return Left.Value->DefineOrder > Right.Value->DefineOrder;
		});

	// Add the fields to this struct
	for (const auto& Pair : PyPropDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}

		FNePyFPropertyDef* PyPropDef = Pair.Value;
		if (!PythonStructBuilder.CreatePropertyFromDefinition(FieldName, PyPropDef))
		{
			return nullptr;
		}
	}

	// Get InitDefault Function
	PythonStructBuilder.ExtractInitDefaultFunc();
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	// Finalize the struct with its post-init function
	return PythonStructBuilder.Finalize();
}

// 重新生成引用了结构体的所有相关的其它结构体和类
// 生成是按照一定顺序进行的：生成结构体 -> 生成引用了结构体的结构体 -> 生成引用了结构体的类
// 必须按照这个顺序来，不要弄乱
void UNePyGeneratedStruct::RegenerateReferencers(UNePyGeneratedStruct* InOldStruct)
{
	// 寻找所有持有了旧结构体的结构体
	TArray<UNePyGeneratedStruct*> AllStructs = FNePyWrapperTypeRegistry::Get().GetAllGeneratedStructs();
	TArray<UNePyGeneratedStruct*> ReferencerStructs;
	ReferencerStructs.Add(InOldStruct);
	for (int32 Index = 0; Index < ReferencerStructs.Num(); ++Index)
	{
		UNePyGeneratedStruct* TargetStruct = ReferencerStructs[Index];
		for (UNePyGeneratedStruct* Struct : AllStructs)
		{
			if (ReferencerStructs.Contains(Struct))
			{
				continue;
			}

			// 在结构体属性字段里寻找对旧结构体的引用
			// 不太确定未来是否会支持生成结构体之间的继承，因此还是搜索一下父类
#if ENGINE_MAJOR_VERSION >= 5
			auto PropIt = TFieldIterator<FStructProperty>(Struct, EFieldIterationFlags::Default);
#else
			auto PropIt = TFieldIterator<FStructProperty>(Struct, EFieldIteratorFlags::IncludeSuper, EFieldIteratorFlags::IncludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces);
#endif
			for (; PropIt; ++PropIt)
			{
				const FStructProperty* Prop = *PropIt;
				if (Prop->Struct == TargetStruct)
				{
					ReferencerStructs.Add(Struct);
					break;
				}
			}
		}
	}

	// 寻找所有持有了旧结构体的类
	TArray<UNePyGeneratedClass*> AllClasses = FNePyWrapperTypeRegistry::Get().GetAllGeneratedClasses();
	TArray<UNePyGeneratedClass*> ReferencerClasses;
	for (UNePyGeneratedStruct* TargetStruct : ReferencerStructs)
	{
		for (UNePyGeneratedClass* Class : AllClasses)
		{
			if (ReferencerClasses.Contains(Class))
			{
				continue;
			}

			// 是否已加入引用者列表了
			bool bAdded = false;

			// 在类属性字段里寻找对旧结构体的引用
			// 注意无需遍历父类的Property。原因是父类在Reload的时候，会Reparent子类，因此无需单独对子类进行处理。
#if ENGINE_MAJOR_VERSION >= 5
			auto PropIt = TFieldIterator<FStructProperty>(Class, EFieldIterationFlags::IncludeDeprecated);
#else
			auto PropIt = TFieldIterator<FStructProperty>(Class, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::IncludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces);
#endif
			for (; PropIt; ++PropIt)
			{
				const FStructProperty* Prop = *PropIt;
				if (Prop->Struct == TargetStruct)
				{
					ReferencerClasses.Add(Class);
					bAdded = true;
					break;
				}
			}

			if (bAdded)
			{
				continue;
			}

			// 在类方法的参数中寻找对旧结构体的引用，同样无需遍历父类的UFunction
			for (UNePyGeneratedFunction* Func : Class->PyGeneratedFuncs)
			{
#if ENGINE_MAJOR_VERSION >= 5
				auto ParamIt = TFieldIterator<FStructProperty>(Func, EFieldIterationFlags::IncludeDeprecated);
#else
				auto ParamIt = TFieldIterator<FStructProperty>(Func, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::IncludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces);
#endif
				for (; ParamIt; ++ParamIt)
				{
					const FStructProperty* Prop = *ParamIt;
					if (Prop->Struct == TargetStruct)
					{
						ReferencerClasses.Add(Class);
						bAdded = true;
						break;
					}
				}

				if (bAdded)
				{
					break;
				}
			}
		}
	}

	check(ReferencerStructs[0] == InOldStruct);
	ReferencerStructs.RemoveAt(0);

	// 重新编译所有受影响的结构体
	for (UNePyGeneratedStruct* Struct : ReferencerStructs)
	{
		UNePyGeneratedStruct::RegenerateStruct(Struct);
	}

	// 重新编译所有受影响的类
	for (UNePyGeneratedClass* Class : ReferencerClasses)
	{
		UNePyGeneratedClass::RegenerateNePyClass(Class);
	}
}

UNePyGeneratedStruct* UNePyGeneratedStruct::RegenerateStruct(UNePyGeneratedStruct* InOldStruct)
{
	// Builder used to re-generate the class
	FNePyGeneratedStructBuilder PythonStructBuilder(InOldStruct);

	// Copy the data from the old class
	if (!PythonStructBuilder.CopyPropertiesFromOldStruct())
	{
		return nullptr;
	}

	// Get InitDefault Function
	PythonStructBuilder.ExtractInitDefaultFunc();
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	return PythonStructBuilder.Finalize();
}

void UNePyGeneratedStruct::SerializeTaggedProperties(FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults, const UObject* BreakRecursionIfFullyLoad) const
{
	// 这个函数抄自 UserDefinedStruct，目前完全没改
	// 函数功能是保存Struct时，与默认值进行对比，使得未改动的字段能够跟随默认值改动而改动
	// 函数实现种，大部分逻辑都是 WITH_EDITOR，可能实际发包之后也会有修改的需求？后续观望应用情况之后再调整
	bool bTemporarilyEnableDelta = false;
	FArchive& Ar = Slot.GetUnderlyingArchive();

#if WITH_EDITOR
	// In the editor the default structure may change while the editor is running, so we need to always delta serialize
	UNePyGeneratedStruct* UDDefaultsStruct = Cast<UNePyGeneratedStruct>(DefaultsStruct);

	const bool bDuplicate = (0 != (Ar.GetPortFlags() & PPF_Duplicate));

	// When saving delta, we want the difference between current data and true structure's default values.
	// So if we don't have defaults we need to use the struct defaults
	const bool bUseNewDefaults = !Defaults
		&& UDDefaultsStruct
		&& (UDDefaultsStruct->GetDefaultInstance() != Data)
		&& !bDuplicate
		&& (Ar.IsSaving() || Ar.IsLoading())
		&& !Ar.IsCooking();

	if (bUseNewDefaults)
	{
		Defaults = const_cast<uint8*>(UDDefaultsStruct->GetDefaultInstance());
	}

	// Temporarily enable delta serialization if this is a CPFUO 
	bTemporarilyEnableDelta = bUseNewDefaults && Ar.IsIgnoringArchetypeRef() && Ar.IsIgnoringClassRef() && !Ar.DoDelta();
	if (bTemporarilyEnableDelta)
	{
		Ar.ArNoDelta = false;
	}
#endif // WITH_EDITOR

	Super::SerializeTaggedProperties(Slot, Data, DefaultsStruct, Defaults, BreakRecursionIfFullyLoad);

	if (bTemporarilyEnableDelta)
	{
		Ar.ArNoDelta = true;
	}
}

void UNePyGeneratedStruct::InitializeStruct(void* Dest, int32 ArrayDim) const
{
	InitializeStructIgnoreDefaults(Dest, ArrayDim);

	if (Dest)
	{
		const uint8* DefaultInstance = GetDefaultInstance();
		if (DefaultInstance)
		{
			int32 Stride = GetStructureSize();

			for (int32 ArrayIndex = 0; ArrayIndex < ArrayDim; ArrayIndex++)
			{
				void* DestStruct = (uint8*)Dest + (Stride * ArrayIndex);
				CopyScriptStruct(DestStruct, DefaultInstance);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#if WITH_EDITOR
				// When copying into another struct we need to register this raw struct pointer so any deferred dependencies will get fixed later
				FScopedPlaceholderRawContainerTracker TrackStruct(DestStruct);
				FBlueprintSupport::RegisterDeferredDependenciesInStruct(this, DestStruct);
#endif
#else
				// When copying into another struct we need to register this raw struct pointer so any deferred dependencies will get fixed later
				FScopedPlaceholderRawContainerTracker TrackStruct(DestStruct);
				FBlueprintSupport::RegisterDeferredDependenciesInStruct(this, DestStruct);
#endif
			}
		}
	}
}

void UNePyGeneratedStruct::PrepareCppStructOps()
{
	if (bPrepareCppStructOpsCompleted)
	{
		return;
	}

	// 在引擎里，CppStructOps 是通过 DeferredCppStructOps 接口，全局有一个 Map 统一管理的，FlattenedStructPathName 与 CppStructOps 一一对应。
	// 考虑到 NePyGeneratedStruct 是动态类，有 Reload，可能同时存在两个同名的。
	// 因此这里就简单处理，直接由 Struct 对象来管理 CppStructOps 的创建销毁。
	SetCppStructOps(FNePyStructOps::CreateNePyStructOpsByStruct(this));

	if (CppStructOps != nullptr)
	{
		Super::PrepareCppStructOps();
	}

	bPrepareCppStructOpsCompleted = true;
}

void UNePyGeneratedStruct::UpdateStructFlags()
{
	// 这个函数抄自 UserDefinedStruct
	// UserDefinedStruct 没有 PrepareCppStructOps，因此需要通过这个函数来初始化 Flags
	// 现在带有的 StructOps 会调用父类的 PrepareCppStructOps 来初始化 Flag。是否还需要调用这个函数存疑
	// 以下是原函数注释：
	// Adapted from PrepareCppStructOps, where we 'discover' zero constructability
	// for native types:
	bool bIsZeroConstruct = true;
	{
		if (CppStructOps != nullptr)
		{
			bIsZeroConstruct = CppStructOps->HasZeroConstructor();
		}

		if (bIsZeroConstruct)
		{
			uint8* StructData = DefaultStructInstance.GetStructMemory();
			if (StructData)
			{
				int32 Size = GetStructureSize();
				for (int32 Index = 0; Index < Size; Index++)
				{
					if (StructData[Index])
					{
						bIsZeroConstruct = false;
						break;
					}
				}
			}
		}

		if (bIsZeroConstruct)
		{
			for (TFieldIterator<FProperty> It(this); It; ++It)
			{
				FProperty* Property = *It;
				if (Property && !Property->HasAnyPropertyFlags(CPF_ZeroConstructor))
				{
					bIsZeroConstruct = false;
					break;
				}
			}
		}
	}

	// IsPOD/NoDtor could be derived earlier than bIsZeroConstruct because they do not depend on 
	// the structs default values, but it is convenient to calculate them all in one place:
	bool bIsPOD = true;
	{
		for (TFieldIterator<FProperty> It(this); It; ++It)
		{
			FProperty* Property = *It;
			if (Property && !Property->HasAnyPropertyFlags(CPF_IsPlainOldData))
			{
				bIsPOD = false;
				break;
			}
		}
	}

	bool bHasNoDtor = true;
	if (!bIsPOD)
	{
		if (bHasNoDtor && CppStructOps != nullptr)
		{
			bHasNoDtor = !CppStructOps->HasDestructor();
		}

		if (bHasNoDtor)
		{
			// we're not POD, but we still may have no destructor, check properties:
			bHasNoDtor = true;
			for (TFieldIterator<FProperty> It(this); It; ++It)
			{
				FProperty* Property = *It;
				if (Property && !Property->HasAnyPropertyFlags(CPF_NoDestructor))
				{
					bHasNoDtor = false;
					break;
				}
			}
		}
	}

	StructFlags = EStructFlags(StructFlags | STRUCT_ZeroConstructor | STRUCT_IsPlainOldData | STRUCT_NoDestructor);
	if (!bIsZeroConstruct)
	{
		StructFlags = EStructFlags(StructFlags & ~STRUCT_ZeroConstructor);
	}
	if (!bIsPOD)
	{
		StructFlags = EStructFlags(StructFlags & ~STRUCT_IsPlainOldData);
	}
	if (!bHasNoDtor)
	{
		StructFlags = EStructFlags(StructFlags & ~STRUCT_NoDestructor);
	}
}

void UNePyGeneratedStruct::InitializeStructIgnoreDefaults(void* Dest, int32 ArrayDim) const
{
	Super::InitializeStruct(Dest, ArrayDim);
}

void UNePyGeneratedStruct::GenerateDefaultInstance()
{
	DefaultStructInstance.Recreate(this);
}

const uint8* UNePyGeneratedStruct::GetDefaultInstance() const
{
	ensure(DefaultStructInstance.IsValid() && DefaultStructInstance.GetStruct() == this);
	return DefaultStructInstance.GetStructMemory();
}

void UNePyGeneratedStruct::CallInitDefaultFunc()
{
	if (!PyInitDefaultFunc)
	{
		return;
	}

	// 不知道为什么，我自己手动调用 tp_new + Init 就会crash，最终发现用 tp_call 这种构造方法才能正常。
	FNePyObjectPtr PyDefaultStruct = NePyStealReference(PyType_Type.tp_call((PyObject*)PyType, nullptr, nullptr));
	if (!PyDefaultStruct && PyErr_Occurred())
	{
		PyErr_Print();
		return;
	}

	bool bHasInitDefaultFunc = false;
	TArray<UNePyGeneratedStruct*, TInlineAllocator<8>> PyStructs;
	{
		UNePyGeneratedStruct* PyStruct = this;
		while (PyStruct)
		{
			bHasInitDefaultFunc = bHasInitDefaultFunc || !!PyStruct->PyInitDefaultFunc;
			PyStructs.Add(PyStruct);
			PyStruct = Cast<UNePyGeneratedStruct>(PyStruct->GetSuperStruct());
		}
	}

	FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(1));
	PyTuple_SetItem(PyArgs, 0, NePyNewReference(PyDefaultStruct).Release());

#if WITH_EDITOR
	NePyGenUtil::FInitDefaultChecker InitDefaultChecker(PyDefaultStruct);
#endif

	Algo::Reverse(PyStructs); // 按照从基结构到子结构的顺序调用__init_default__
	for (UNePyGeneratedStruct* PyStruct : PyStructs)
	{
		if (PyStruct->PyInitDefaultFunc)
		{
			FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyStruct->PyInitDefaultFunc, PyArgs));  // 返回值并不会被使用
			if (PyErr_Occurred())
			{
				PyErr_Print();
				UE_LOG(LogNePython, Warning, TEXT("Failed to init default object for '%s'"), UTF8_TO_TCHAR(PyType->tp_name));
			}
		}
	}

#if WITH_EDITOR
	InitDefaultChecker.DoCheck();
#endif

	void* DstValuePtr = const_cast<uint8*>(this->GetDefaultInstance());
	void* SrcValuePtr = ((FNePyStructBase*)PyDefaultStruct.Get())->Value;
	this->CopyScriptStruct(DstValuePtr, SrcValuePtr);
	this->UpdateStructFlags();
}

void UNePyGeneratedStruct::InitDefaultValueForProperties()
{
	// 直接使用 GetDefaultInstance 往对象写默认属性值
	void* DstValuePtr = const_cast<uint8*>(this->GetDefaultInstance());
	NePyGenUtil::AssignPropertyDefaultValue(DstValuePtr, PropertyDefs);
}

void UNePyGeneratedStruct::SetCppStructOps(ICppStructOps* InStructOps)
{
	if (CppStructOps != nullptr)
	{
		delete CppStructOps;
	}
	CppStructOps = InStructOps;
}

void UNePyGeneratedStruct::ClearAndDeleteCppStructOps()
{
	if (CppStructOps)
	{
		delete CppStructOps;
		CppStructOps = nullptr;
	}
	Super::ClearCppStructOps();
}

PyObject* NePyMethod_GenerateStruct(PyObject*, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:ustruct", &PyObj))
	{
		return nullptr;
	}

	if (!PyType_Check(PyObj))
	{
		PyErr_SetString(PyExc_TypeError, "@ue.ustruct() should be used on a python class.");
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!GNePyDisableGeneratedType)
	{
		PyTypeObject* PyBase = PyType->tp_base;
		if (!PyType_IsSubtype(PyBase, NePyStructBaseGetType()))
		{
			PyErr_Format(PyExc_Exception, "Type '%s' does not derive from 'ue.StructBase'",
				PyType->tp_name);
			return nullptr;
		}

		if (PyTuple_GET_SIZE(PyType->tp_bases) > 1)
		{
			PyErr_Format(PyExc_Exception, "Python generated struct '%s' is not allowed to have mutiple bases",
				PyType->tp_name);
			return nullptr;
		}

		TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;
		if (!NePyGenUtil::ParseSpecifiersFromPyDict(InKwds, PySpecifierPairs))
		{
			return nullptr;
		}

		if (!UNePyGeneratedStruct::GenerateStruct(PyType, PySpecifierPairs))
		{
			if (!PyErr_Occurred())
			{
				PyErr_Format(PyExc_Exception, "Failed to generate an Unreal struct for the Python type '%s'",
					PyType->tp_name);
			}
			return nullptr;
		}
		else
		{
			// 这个过程中可能产生了非阻碍性异常，需要打印出来，并清除错误指示器
			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
		}
	}

	Py_INCREF(PyType);
	return (PyObject*)PyType;
}


struct FNePyUStructDecorator : public PyObject
{
public:
	PyObject* CachedKwds;

public:
	static FNePyUStructDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		FNePyUStructDecorator* Self = (FNePyUStructDecorator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
			Self->CachedKwds = nullptr;
		}
		return Self;
	}

	static void Dealloc(FNePyUStructDecorator* InSelf)
	{
		Py_XDECREF(InSelf->CachedKwds);
		Py_TYPE(InSelf)->tp_free(InSelf);
	}

	static int Init(FNePyUStructDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (InKwds)
		{
			Py_INCREF(InKwds);
			InSelf->CachedKwds = InKwds;
		}
		return 0;
	}

	static PyObject* Call(FNePyUStructDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		return NePyMethod_GenerateStruct(nullptr, InArgs, InSelf->CachedKwds);
	}
};

static PyTypeObject NePyUStructDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	NePyInternalModuleName ".StructDecorator", /* tp_name */
	sizeof(FNePyUStructDecorator), /* tp_basicsize */
};

PyObject* NePyMethod_UStructDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUStructDecorator* PyRet = FNePyUStructDecorator::New(&NePyUStructDecoratorType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUStructDecorator::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyTypeObject* NePyInitGeneratedStruct()
{
	// @ue.ustruct装饰器
	PyTypeObject* PyType = &NePyUStructDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)&FNePyUStructDecorator::New;
	PyType->tp_dealloc = (destructor)&FNePyUStructDecorator::Dealloc;
	PyType->tp_init = (initproc)&FNePyUStructDecorator::Init;
	PyType->tp_call = (ternaryfunc)&FNePyUStructDecorator::Call;
	PyType_Ready(PyType);
	return PyType;
}
