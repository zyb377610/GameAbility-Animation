#include "NePyGeneratedClass.h"
#include "NePyGeneratedType.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"
#include "NePyDynamicType.h"
#include "NePySubclass.h"
#include "NePySubclassing.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyUtil.h"
#include "NePyReloadUtils.h"
#include "NePyHouseKeeper.h"
#include "NePyDescriptor.h"
#include "NePyDelegateHelper.h"
#include "NePyBlueprintActionDatabaseHelper.h"
#include "NePyMemoryAllocator.h"
#include "NePyBindingModuleInterface.h"
#include "Algo/Reverse.h"
#include "UObject/MetaData.h"
#include "UObject/Field.h"
#include "UObject/LinkerLoad.h"
#include "UObject/UObjectThreadContext.h"
#include "Subsystems/Subsystem.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Net/Core/PushModel/PushModel.h"
#if WITH_EDITORONLY_DATA
#include "EdGraphSchema_K2.h"
#endif // WITH_EDITORONLY_DATA

extern TAutoConsoleVariable<int32> CVarSubclassingLog;

static FNePyObjectBase* NePyGeneratedClassNew(UObject* InValue, PyTypeObject* InPyType)
{
	check(InPyType->tp_basicsize >= sizeof(FNePyObjectBase));
	FNePyObjectBase* RetValue = (FNePyObjectBase*)PyType_GenericAlloc(InPyType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	RetValue->Value = InValue;
	return RetValue;
}

static void NePyGeneratedClassInit(PyObject* InPyObject)
{
	FNePyObjectPtr InitFunc = NePyStealReference(PyObject_GetAttrString(InPyObject, NePyGenUtil::InitPythonObjectFuncName));
	if (!InitFunc)
	{
		check(false);  // 理论上，Builder会赋初值，不会空指针报错
		PyErr_Clear();
		return;
	}
	if (InitFunc.Get() == Py_None)
	{
		return;
	}
	FNePyObjectPtr InitFuncRet = NePyStealReference(PyObject_CallObject(InitFunc, nullptr));
	if (!InitFuncRet)
	{
		PyErr_Print();
	}
}

class FNePyGeneratedClassBuilder
{
public:
	FNePyGeneratedClassBuilder(const FString& InClassName, UClass* InSuperClass, PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs)
		: ClassName(InClassName)
		, PyType(InPyType)
	{
		UObject* ClassOuter = GetGenClassOuter(InSuperClass);

		// Find any existing class with the name we want to use
		OldClass = FindOldClass(ClassName);

		// Create a new class with a temporary name; we will rename it as part of Finalize
		const FString NewClassName = MakeUniqueObjectName(ClassOuter, UNePyGeneratedClass::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *ClassName)).ToString();
		NewClass = NewObject<UNePyGeneratedClass>(ClassOuter, *NewClassName, RF_Public | RF_NePyGeneratedTypeGCSafe);
		NewClass->SetSuperStruct(InSuperClass);
		NewClass->ClassFlags = (InSuperClass->ClassFlags & CLASS_ScriptInherit);

		// 如果存在旧类，复用旧类作为最终生成的类，这样我们就不需要修复对旧类的引用了。
		FinalClass = OldClass ? OldClass : NewClass;

		Specifiers = FNePySpecifier::ParseSpecifiers(InPySpecifierPairs, FNePySpecifier::Scope_Class);
	}

	FNePyGeneratedClassBuilder(UNePyGeneratedClass* InOldClass)
		: ClassName(InOldClass->GetName())
		, PyType(InOldClass->PyType)
		, OldClass(InOldClass)
	{
		UObject* ClassOuter = GetGenClassOuter(InOldClass->GetSuperClass());

		// Create a new class with a temporary name; we will rename it as part of Finalize
		const FString NewClassName = MakeUniqueObjectName(ClassOuter, UNePyGeneratedClass::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *ClassName)).ToString();
		NewClass = NewObject<UNePyGeneratedClass>(ClassOuter, *NewClassName, RF_Public | RF_NePyGeneratedTypeGCSafe);
		NewClass->SetSuperStruct(InOldClass->GetSuperClass());
		NewClass->ClassFlags = (InOldClass->GetSuperClass()->ClassFlags & CLASS_ScriptInherit);

		// 如果存在旧类，复用旧类作为最终生成的类，这样我们就不需要修复对旧类的引用了。
		FinalClass = OldClass ? OldClass : NewClass;
#if WITH_EDITOR
		if (FinalClass->Specifiers.Num() > 0)
		{
			Specifiers = FinalClass->Specifiers;
			FinalClass->Specifiers.Empty();
		}
#endif // WITH_EDITOR
	}

	~FNePyGeneratedClassBuilder()
	{
#if WITH_EDITOR
		if (FinalClass->Specifiers.Num() > 0)
		{
			FNePySpecifier::ReleaseSpecifiers(FinalClass->Specifiers);
		}
		FinalClass->Specifiers = Specifiers;
#else
		FNePySpecifier::ReleaseSpecifiers(Specifiers);
#endif // WITH_EDITOR

		// If NewClass is still set at this point, if means Finalize wasn't called and we should destroy the partially built class
		if (NewClass)
		{
			NewClass->ClearFlags(RF_AllFlags);
			NewClass->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
			NewClass->ClearPythonGeneratedFunctions();
			NewClass = nullptr;

			Py_BEGIN_ALLOW_THREADS
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			Py_END_ALLOW_THREADS
			if (GIsInitialLoad)
			{
				// 进入这个分支的情况：生成类在NePython初始化时候就已经报错了
				UE_LOG(LogNePython, Fatal, TEXT("SubClassing encounters an error during initialization that cannot recover!"));
			}
		}
	}

	UNePyGeneratedClass* Finalize()
	{
		int32 Level = CVarSubclassingLog.GetValueOnGameThread();

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("========================================"));
			UE_LOG(LogNePython, Log, TEXT("Finalizing Python Generated Class: %s"), *ClassName);
			UE_LOG(LogNePython, Log, TEXT("  PyType=%p, tp_name=%s"), PyType, UTF8_TO_TCHAR(PyType->tp_name));
			UE_LOG(LogNePython, Log, TEXT("  OldClass=%p, NewClass=%p, FinalClass=%p"), OldClass, NewClass, FinalClass);
			UE_LOG(LogNePython, Log, TEXT("  SuperClass=%s [%p]"), 
				NewClass->GetSuperClass() ? *NewClass->GetSuperClass()->GetName() : TEXT("None"),
				NewClass->GetSuperClass());
		}

		// Replace the definitions with real descriptors
		TArray<FNePyObjectPtr> PyDescriptors;
		if (!RegisterDescriptors(PyDescriptors))
		{
			return nullptr;
		}

		// We can no longer fail, so prepare the old class for removal and set the correct name on the new class
		if (OldClass)
		{
			ReplaceOldClassProperties();
			if (OldClass->PyType != PyType)
			{
				FNePyWrapperTypeRegistry::Get().UnregisterWrappedClassType(OldClass);
			}
			else
			{
				NePyType_CleanupDescriptors(PyType);
			}
		}

		// 这一步把PyType->tp_dict里原来的FNePyXXXDef对象给替换了
		for (auto& PyObjPtr : PyDescriptors)
		{
			FNePyDescriptorBase* PyDescr = (FNePyDescriptorBase*)PyObjPtr.Get();
			PyDict_SetItem(PyType->tp_dict, PyDescr->Name, PyDescr);
		}
		PyType_Modified(PyType);

		{
			Py_BEGIN_ALLOW_THREADS
			if (!OldClass)
			{
				NewClass->Rename(*ClassName, nullptr, REN_DontCreateRedirectors);
			}

			// 记录C++基类
			UClass* SuperClass = FinalClass->GetSuperClass();
			if (UNePyGeneratedClass* GeneratedSuperClass = Cast<UNePyGeneratedClass>(SuperClass))
			{
				FinalClass->NativeSuperClass = GeneratedSuperClass->NativeSuperClass;
			}
			else
			{
				FinalClass->NativeSuperClass = SuperClass;
			}

			if (Level >= 1)
			{
				UE_LOG(LogNePython, Log, TEXT("  NativeSuperClass: %s [%p]"),
					FinalClass->NativeSuperClass ? *FinalClass->NativeSuperClass->GetName() : TEXT("None"),
					FinalClass->NativeSuperClass);
			}

			// Actor和ActorComponent要使用特殊的构造函数
			if (FinalClass->IsChildOf(AActor::StaticClass()))
			{
				FinalClass->ClassConstructor = UNePyGeneratedClass::StaticActorConstructor;
				if (Level >= 1)
				{
					UE_LOG(LogNePython, Log, TEXT("  Using StaticActorConstructor"));
				}
			}
			else if (FinalClass->IsChildOf(UActorComponent::StaticClass()))
			{
				FinalClass->ClassConstructor = UNePyGeneratedClass::StaticComponentConstructor;
				if (Level >= 1)
				{
					UE_LOG(LogNePython, Log, TEXT("  Using StaticComponentConstructor"));
				}
			}
			else
			{
				FinalClass->ClassConstructor = UNePyGeneratedClass::StaticObjectConstructor;
				if (Level >= 1)
				{
					UE_LOG(LogNePython, Log, TEXT("  Using StaticObjectConstructor"));
				}
			}

#if WITH_EDITORONLY_DATA
			if (FinalClass->IsChildOf(UActorComponent::StaticClass()))
			{
				static const FName NAME_ClassGroupNames(TEXT("ClassGroupNames"));
				FinalClass->SetMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent, TEXT("true"));

				FString ClassGroupCategory = NSLOCTEXT("BlueprintableComponents", "CategoryName", "PythonSubclassing").ToString();
				FinalClass->SetMetaData(NAME_ClassGroupNames, *ClassGroupCategory);
			}
#endif // WITH_EDITORONLY_DATA

			// Finalize the class
			FinalClass->ClassConfigName = SuperClass->ClassConfigName;
			FinalClass->Bind();
			FinalClass->StaticLink(true);
			FinalClass->AssembleReferenceTokenStream(true);
			Py_END_ALLOW_THREADS
		}

		if (!OldClass || OldClass->PyType != PyType)
		{
			Py_INCREF(PyType);
			Py_XDECREF(FinalClass->PyType);
			FinalClass->PyType = PyType;

			// 避免用户直接在Python中定义__init__
			if (FinalClass->PyType->tp_init && FinalClass->PyType->tp_init != NePyObjectBaseGetType()->tp_init)
			{
				UE_LOG(LogNePython, Warning, TEXT(
					"User defined '__init__' in subclassing type '%s', which is not allowed and will take no effect."
					" Use '__init_default__' instead if you want to define default values in unreal type."
					" Use '__init_pyobj__' instead if you want to define python variables."
					), UTF8_TO_TCHAR(FinalClass->PyType->tp_name));
				FinalClass->PyType->tp_init = NePyObjectBaseGetType()->tp_init;
			}

			// Map the Unreal class to the Python type
			FNePyObjectTypeInfo NewTypeInfo = {
				PyType,
				ENePyTypeFlags::ScriptPyType,
				NePyGeneratedClassNew,
				NePyGeneratedClassInit,
			};
			FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(FinalClass, NewTypeInfo);
		}

#if WITH_EDITORONLY_DATA
		// 需要设置这个，生成类才会出现在编辑器蓝图基类列表里
		// 现在由于 UNePyGeneratedClass 继承自 UBlueprintGeneratedClass，无论是否设置都可被蓝图继承
		FinalClass->SetMetaData(FBlueprintMetadata::MD_IsBlueprintBase, TEXT("true"));
		// 既然已经是 Blueprintable 了，那么就把 BlueprintType 一起带上吧
		FinalClass->SetMetaData(TEXT("BlueprintType"), TEXT("true"));

		FString Path = NePyGenUtil::GetPythonModuleRelativePathForPyType(PyType);
		if (!Path.IsEmpty())
		{
			FName ModuleRelativePathMetaKey = TEXT("ModuleRelativePath");
			FinalClass->SetMetaData(ModuleRelativePathMetaKey, *Path);
			for (auto Func : FinalClass->PyGeneratedFuncs)
			{
				Func->SetMetaData(ModuleRelativePathMetaKey, *Path);
			}
		}

		// Apply the doc string as the uclass tooltip
		const FString DocString = NePyUtil::GetDocString((PyObject*)PyType);
		if (!DocString.IsEmpty())
		{
			FinalClass->SetMetaData(TEXT("ToolTip"), *DocString);
		}
#endif // WITH_EDITORONLY_DATA

		// 应用说明符
		FNePySpecifier::ApplyToClass(Specifiers, FinalClass);

		// Ensure the CDO exists
		{
			// Fix: 修复创建Actor对象的CDO时，设置好RootComponent后，又被FObjectInitializer::InitializeProperties()置空的问题。
			// 做法是将原本位于FObjectInitializer::PostConstructInit()中的FObjectInitializer::InitializeProperties()给跳过，
			// 并将FObjectInitializer::InitializeProperties()的调用时机提前到RootComponent设置之前执行。
			TGuardValue<EClassFlags> ClassFlagsGuardValue(FinalClass->ClassFlags, FinalClass->ClassFlags | CLASS_Native);
			Py_BEGIN_ALLOW_THREADS
			FinalClass->GetDefaultObject();
			Py_END_ALLOW_THREADS
		}

		// Call InitDefault to init CDO property values
		FinalClass->CallInitDefaultFunc();
		FinalClass->UpdateCustomPropertyListForPostConstruction();

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
		FinalClass->InitializeFieldNotifies();
		NePyType_AddFieldNotifySupportForType(PyType, FinalClass);
#endif

#if WITH_EDITOR
		// Re-instance the old class and re-parent any derived classes to this new type
		if (Reload.IsValid())
		{
			Reload->Finalize();
			Reload.Reset();
		}
#endif

		// 继承自UBlueprintGeneratedClass后，需要把bCooked置为true，否则UE中很多逻辑会尝试去找它身上的ClassGeneratedBy，这个变量在NePy中它是个伪造的蓝图或nullptr，很多情况下并不能被安全使用
		FinalClass->bCooked = true;

#if WITH_EDITORONLY_DATA
		Py_BEGIN_ALLOW_THREADS
		// Notify Blueprints that there is a new class to add to the action list
		FNePyBlueprintActionDatabaseHelper::RegisterClassActions(FinalClass);
		Py_END_ALLOW_THREADS
#endif // WITH_EDITORONLY_DATA

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		{
			TCHAR PackageName[FName::StringBufferSize];
			TCHAR CDOName[FName::StringBufferSize];
			FinalClass->GetPackage()->GetFName().ToString(PackageName);
			FinalClass->GetDefaultObject()->GetFName().ToString(CDOName);
			NotifyRegistrationEvent(PackageName, *ClassName, ENotifyRegistrationType::NRT_Class, ENotifyRegistrationPhase::NRP_Finished, nullptr, false, FinalClass);
			NotifyRegistrationEvent(PackageName, CDOName, ENotifyRegistrationType::NRT_ClassCDO, ENotifyRegistrationPhase::NRP_Finished, nullptr, false, FinalClass->GetDefaultObject());
		}
#endif

		if (Level >= 1)
		{
			UE_LOG(LogNePython, Log, TEXT("  Finalization complete!"));
			UE_LOG(LogNePython, Log, TEXT("    FinalClass=%p, Name=%s"), FinalClass, *FinalClass->GetName());
			UE_LOG(LogNePython, Log, TEXT("    CDO=%p"), FinalClass->GetDefaultObject(false));
			UE_LOG(LogNePython, Log, TEXT("    PyType=%p, refcount=%d"),
				FinalClass->PyType,
				(int32)Py_REFCNT(FinalClass->PyType));
			UE_LOG(LogNePython, Log, TEXT("========================================"));
		}

		if (!OldClass)
		{
			// Null the NewClass pointer so the destructor doesn't kill it
			NewClass = nullptr;
		}

		return FinalClass;
	}

	bool CreatePropertyFromDefinition(const char* InFieldName, const FNePyFPropertyDef* InPyPropDef)
	{
		UClass* SuperClass = NewClass->GetSuperClass();

		// Resolve the property name to match any previously exported properties from the parent type
		const FName PropName(InFieldName);
		if (SuperClass->FindPropertyByName(PropName))
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) cannot override a property from the base type", PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Create the property from its definition
		FProperty* Prop = NePySubclassingNewProperty(NewClass, PropName, InPyPropDef->PropType, NewClass->GetName(), InPyPropDef->UserDefineUPropertyFlags, InPyPropDef->DefaultValue, FinalClass);
		if (!Prop)
		{
			PyErr_Format(PyExc_Exception, "%s: Failed to create property for '%s' (%s)", PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// 应用说明符
		FNePySpecifier::ApplyToProperty(InPyPropDef->Specifiers, Prop);

		NewClass->AddCppProperty(Prop);
		if (Prop->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
		{
			NewClass->ClassFlags |= CLASS_HasInstancedReference;
		}

		// Build the definition data for the new property accessor
		NePyGenUtil::FPropertyDef& PropDef = *NewClass->PropertyDefs.Add_GetRef(MakeShared<NePyGenUtil::FPropertyDef>());
		PropDef.GetSetName = NePyGenUtil::UTF8ToUTF8Buffer(InFieldName);
		PropDef.Prop = Prop;
		if (!InPyPropDef->GetterFuncName.IsEmpty())
		{
			PropDef.GetFunc = NewClass->FindFunctionByName(FName(InPyPropDef->GetterFuncName));
		}
		if (!InPyPropDef->SetterFuncName.IsEmpty())
		{
			PropDef.SetFunc = NewClass->FindFunctionByName(FName(InPyPropDef->SetterFuncName));
		}
		// Copy default value out
		if (InPyPropDef->DefaultValue)
		{
			PropDef.DefaultValue = NePyNewReferenceWithGIL(InPyPropDef->DefaultValue);
		}

		// If this property has a getter or setter, also make an internal version with the get/set function cleared so that Python can read/write the internal property value
		if (PropDef.GetFunc || PropDef.SetFunc)
		{
			NePyGenUtil::FPropertyDef& InternalPropDef = *NewClass->PropertyDefs.Add_GetRef(MakeShared<NePyGenUtil::FPropertyDef>());
			InternalPropDef.GetSetName = NePyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("_%s"), UTF8_TO_TCHAR(InFieldName)));
			InternalPropDef.Prop = Prop;
			InternalPropDef.DefaultValue = PropDef.DefaultValue;
		}

		return true;
	}

	bool CreateFunctionFromDefinition(const char* InFieldName, const FNePyUFunctionDef* InPyFuncDef)
	{
		UClass* SuperClass = NewClass->GetSuperClass();

		// Validate the function definition makes sense
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, ENePyUFunctionDefFlags::Override))
		{
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, ENePyUFunctionDefFlags::Static))
			{
				PyErr_Format(PyExc_Exception, "%s: Method '%s' specified as 'override' cannot also specify '@staticmethod'", PyType->tp_name, InFieldName);
				return false;
			}
			if (InPyFuncDef->FuncRetType != Py_None || InPyFuncDef->FuncParamTypes != Py_None)
			{
				PyErr_Format(PyExc_Exception, "%s: Method '%s' specified as 'override' cannot also specify 'ret' or 'params'", PyType->tp_name, InFieldName);
				return false;
			}
		}

		// Resolve the function name to match any previously exported functions from the parent type
		const FName FuncName(InFieldName);
		const UFunction* SuperFunc = SuperClass->FindFunctionByName(FuncName);
		if (SuperFunc && !EnumHasAllFlags(InPyFuncDef->FuncFlags, ENePyUFunctionDefFlags::Override))
		{
			PyErr_Format(PyExc_Exception, "%s: Method '%s' cannot override a method from the base type (did you forget to specify 'override=True'?)", PyType->tp_name, InFieldName);
			return false;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, ENePyUFunctionDefFlags::Override))
		{
			if (!SuperFunc)
			{
				PyErr_Format(PyExc_Exception, "%s: Method '%s' was set to 'override', but no method was found to override", PyType->tp_name, InFieldName);
				return false;
			}
			if (!SuperFunc->HasAnyFunctionFlags(FUNC_BlueprintEvent))
			{
				PyErr_Format(PyExc_Exception, "%s: Method '%s' was set to 'override', but the method found to override was not a blueprint event", PyType->tp_name, InFieldName);
				return false;
			}
		}

		// Inspect the argument names and defaults from the Python function
		TArray<FString> FuncArgNames;
		//TArray<FNePyObjectPtr> FuncArgDefaults;
		bool bHasDefaults = false;
		if (!NePyUtil::InspectFunctionArgs(InPyFuncDef->Func, FuncArgNames, nullptr, &bHasDefaults))
		{
			PyErr_Format(PyExc_Exception, "%s: Failed to inspect the arguments for '%s'", PyType->tp_name, InFieldName);
			return false;
		}

		// 目前我们不支持在Python中定义的UFunction有默认参数
		if (bHasDefaults)
		{
			PyErr_Format(PyExc_Exception, "%s: Method '%s' has default arguments, which is not supported", PyType->tp_name, InFieldName);
			return false;
		}

		// Create the function, either from the definition, or from the super-function found to override
		UNePyGeneratedFunction* Func = nullptr;
		if (SuperFunc)
		{
			FObjectDuplicationParameters FuncDuplicationParams(const_cast<UFunction*>(SuperFunc), NewClass);
			FuncDuplicationParams.DestName = FuncName;
			FuncDuplicationParams.DestClass = UNePyGeneratedFunction::StaticClass();
			Func = CastChecked<UNePyGeneratedFunction>(StaticDuplicateObjectEx(FuncDuplicationParams));
		}
		else
		{
			Func = NewObject<UNePyGeneratedFunction>(NewClass, FuncName, RF_Public | RF_NePyGeneratedTypeGCSafe);
			Func->FunctionFlags |= FUNC_Public;
		}

		// Link into class
		Func->Next = NewClass->Children;
		NewClass->Children = Func;

		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, ENePyUFunctionDefFlags::Static))
		{
			Func->FunctionFlags |= FUNC_Static;
		}

		Func->FunctionFlags |= FUNC_Native;

		NewClass->AddFunctionToFunctionMap(Func, Func->GetFName());
		if (!Func->HasAnyFunctionFlags(FUNC_Static))
		{
			// Check for a malformed function rather than assert in the remove
			if (FuncArgNames.Num() > 0/* && FuncArgDefaults.Num() > 0*/)
			{
				// Strip the zero'th 'self' argument when processing a non-static function
#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4)
				FuncArgNames.RemoveAt(0, 1, EAllowShrinking::No);
				//FuncArgDefaults.RemoveAt(0, 1, EAllowShrinking::No);
#else
				FuncArgNames.RemoveAt(0, 1, /*bAllowShrinking*/false);
				//FuncArgDefaults.RemoveAt(0, 1, /*bAllowShrinking*/false);
#endif
			}
			else
			{
				PyErr_Format(PyExc_Exception, "%s: Incorrect number of arguments specified for '%s' (missing self?)", PyType->tp_name, InFieldName);
				Func->ClearFlags(RF_AllFlags);
				Func->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
				return false;
			}
		}
		// Build the arguments struct if not overriding a function
		if (!SuperFunc)
		{
			// Make sure the number of function arguments matches the number of argument types specified
			const int32 NumArgTypes = (InPyFuncDef->FuncParamTypes && InPyFuncDef->FuncParamTypes != Py_None) ? PySequence_Size(InPyFuncDef->FuncParamTypes) : 0;
			if (NumArgTypes != FuncArgNames.Num())
			{
				PyErr_Format(PyExc_Exception, "%s: Incorrect number of arguments specified for '%s' (expected %d, got %d)", PyType->tp_name, InFieldName, NumArgTypes, FuncArgNames.Num());
				Func->ClearFlags(RF_AllFlags);
				Func->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
				return false;
			}

			bool bFillParamsSucc = FillFuncWithParams(Func, FuncArgNames, InPyFuncDef->FuncParamTypes, InPyFuncDef->FuncRetType);
			if (!bFillParamsSucc)
			{
				PyErr_Format(PyExc_Exception, "%s: Can't fill this func params for '%s'", PyType->tp_name, InFieldName);
				Func->ClearFlags(RF_AllFlags);
				Func->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
				return false;
			}
		}

#if WITH_EDITORONLY_DATA
		// Apply the doc string as the function tooltip
		const FString DocString = NePyUtil::GetDocString(InPyFuncDef->Func);
		if (!DocString.IsEmpty())
		{
			Func->SetMetaData(TEXT("ToolTip"), *DocString);
		}
#endif // WITH_EDITORONLY_DATA

		// 应用说明符
		FNePySpecifier::ApplyToFunction(InPyFuncDef->Specifiers, Func);

//		// Apply the defaults to the function arguments and build the Python method params
//		{
//			int32 InputArgIndex = -1;
//			for (TFieldIterator<const FProperty> ParamIt(Func); ParamIt; ++ParamIt)
//			{
//				InputArgIndex += 1;
//
//				const FProperty* Param = *ParamIt;
//				if (!NePyUtil::IsInputParameter(Param))
//				{
//					continue;
//				}
//
//				TOptional<FString> ResolvedDefaultValue;
//				if (FuncArgDefaults.IsValidIndex(InputArgIndex) && FuncArgDefaults[InputArgIndex])
//				{
//					// Convert the default value to the given property...
//					FNePyPropValue DefaultValue(Param);
//					if (!DefaultValue.SetValue(FuncArgDefaults[InputArgIndex]))
//					{
//						PyErr_Format(PyExc_Exception, "%s: Failed to convert default value for function '%s' argument '%s' (%s)",
//							PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*FuncArgNames[InputArgIndex]), TCHAR_TO_UTF8(*Param->GetClass()->GetName()));
//						return false;
//					}
//
//					// ... and export it as meta-data
//					FString ExportedDefaultValue;
//					if (!DefaultValue.Prop->ExportText_Direct(ExportedDefaultValue, DefaultValue.Value, DefaultValue.Value, nullptr, PPF_None))
//					{
//						PyErr_Format(PyExc_Exception, "%s: Failed to export default value for function '%s' argument '%s' (%s)",
//							PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*FuncArgNames[InputArgIndex]), TCHAR_TO_UTF8(*Param->GetClass()->GetName()));
//						return false;
//					}
//
//					ResolvedDefaultValue = ExportedDefaultValue;
//				}
//
//				if (ResolvedDefaultValue.IsSet())
//				{
//#if WITH_EDITORONLY_DATA
//					const FName DefaultValueMetaDataKey = *FString::Printf(TEXT("CPP_Default_%s"), *Param->GetName());
//					Func->SetMetaData(DefaultValueMetaDataKey, *ResolvedDefaultValue.GetValue());
//#endif // WITH_EDITORONLY_DATA
//					Func->FunctionFlags |= FUNC_HasDefaults;
//					// todo: twx 将默认参数添加到函数定义上
//				}
//			}
//		}

		bool bEnableFastPath = !EnumHasAnyFlags(Func->FunctionFlags, FUNC_Net);
		// 对引用返回值进行蓝图虚拟机调用可以保证用户一定使用返回值获取引用结果，但是会增加虚拟机穿透开销代价
		if (bEnableFastPath)
		{
			for (TFieldIterator<const FProperty> ParamIt(Func); ParamIt; ++ParamIt)
			{
				const FProperty* Param = *ParamIt;
				if (Param->HasAllPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
				{
					// 引用返回的参数在导出至python时，应该被处理成值拷贝并通过常规返回值返回
					bEnableFastPath = false;
					break;
				}
			}
		}

		Func->Bind();
		Func->StaticLink(true);
		Func->InitializePythonFunction(InPyFuncDef->Func, InFieldName);
		Func->bEnableFastPath = bEnableFastPath;
		NewClass->PyGeneratedFuncs.Add(Func);

		return true;
	}

	bool FillFuncWithParams(UFunction* Func, const TArray<FString>& FuncArgNames, PyObject* FuncParamTypes, PyObject* FuncRetType)
	{
		return NePyGenUtil::FillFuncWithParams(Func, FuncArgNames, FuncParamTypes, FuncRetType, FinalClass);
	}

	bool CreateComponentFromDefinition(const char* InFieldName, const FNePyUComponentDef* InPyPropDef)
	{
		UClass* SuperClass = NewClass->GetSuperClass();

		// Resolve the property name to match any previously exported properties from the parent type
		const FName PropName(InFieldName);
		if (InPyPropDef->OverrideName.IsEmpty() && SuperClass->FindPropertyByName(PropName))
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) cannot directly override component from the base type, please set override = 'ComponentName' in the parent class", PyType->tp_name, InFieldName,
				TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		if (!InPyPropDef->OverrideName.IsEmpty() && (!InPyPropDef->AttachName.IsEmpty() || InPyPropDef->bRoot))
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) does not support attach and root for override component", PyType->tp_name, InFieldName,
				TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		if (!InPyPropDef->OverrideName.IsEmpty())
		{
			if (!SuperClass->HasAnyClassFlags(CLASS_Abstract) && !Cast<UActorComponent>(FindObjectFast<UObject>(SuperClass->ClassDefaultObject, FName(InPyPropDef->OverrideName))))
			{
				UE_LOG(LogNePython, Warning, TEXT("%s: Property '%s' (%s) override component Warning: The component object '%s' is not found in the SuperClass."), UTF8_TO_TCHAR(PyType->tp_name), UTF8_TO_TCHAR(InFieldName),
					*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType), *InPyPropDef->OverrideName);
			}
		}

		UClass* CompClass = NePyBase::ToCppClass(InPyPropDef->PropType);
		if (!CompClass)
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) cannot become an uproperty, because %s is not UClass or sub-type of ActorComponent", PyType->tp_name, InFieldName,
				TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)), TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}
		if (!CompClass->IsChildOf<UActorComponent>())
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) cannot become an uproperty, because %s does not derive from ActorComponent", PyType->tp_name, InFieldName,
				TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)), TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		if (InPyPropDef->bRoot && !InPyPropDef->AttachName.IsEmpty())
		{
			PyErr_Format(PyExc_Exception, "%s: Property '%s' (%s) can't set as root component and has attach name at the same time", PyType->tp_name, InFieldName,
				TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Create the property from its definition
		FProperty* Prop = NePySubclassingNewProperty(NewClass, PropName, InPyPropDef->PropType, NewClass->GetName(), CPF_ExportObject | CPF_InstancedReference, nullptr, FinalClass);
		if (!Prop)
		{
			PyErr_Format(PyExc_Exception, "%s: Failed to create property for '%s' (%s)", PyType->tp_name, InFieldName, TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// 应用说明符
		FNePySpecifier::ApplyToComponent(InPyPropDef->Specifiers, Prop, !InPyPropDef->OverrideName.IsEmpty());

		NewClass->AddCppProperty(Prop);
		NewClass->ClassFlags |= CLASS_HasInstancedReference;

		FObjectProperty* ObjectProp = CastField<FObjectProperty>(Prop);
		if (!ObjectProp)
		{
			PyErr_Format(PyExc_Exception, "Property %s is not FObjectProperty!", InFieldName);
			delete Prop;
			return false;
		}

		UClass* ComponentClass = ObjectProp->PropertyClass;
		if (!ComponentClass->IsChildOf(UActorComponent::StaticClass()))
		{
			PyErr_Format(PyExc_Exception, "The class of property %s is not child of UActorComponent!", InFieldName);
			delete Prop;
			return false;
		}

		NePyGenUtil::FComponentDef& PropDef = *NewClass->ComponentDefs.Add_GetRef(MakeShared<NePyGenUtil::FComponentDef>());
		PropDef.Prop = Prop;
		PropDef.ComponentClass = ComponentClass;
		PropDef.bRoot = InPyPropDef->bRoot;
		PropDef.FieldName = NePyGenUtil::UTF8ToUTF8Buffer(InFieldName);
		PropDef.AttachName = FName(InPyPropDef->AttachName);
		PropDef.SocketName = FName(InPyPropDef->SocketName);
		PropDef.OverrideName = FName(InPyPropDef->OverrideName);

		return true;
	}

	bool CreateDelegateFromDefinition(const char* InFieldName, const FNePyUDelegateDef* InPyPropDef)
	{
		const FName FuncName(FString(InFieldName) + "_SignatureFunction");
		const FName DelegateName(InFieldName);

		UFunction* Func = NewObject<UFunction>(NewClass, FuncName, RF_Public);
		Func->FunctionFlags |= (FUNC_Delegate | FUNC_MulticastDelegate);

		// Link into class
		Func->Next = NewClass->Children;
		NewClass->Children = Func;

		bool bFillParamsSucc = FillFuncWithParams(Func, InPyPropDef->FuncParamNames, InPyPropDef->FuncParamTypes, nullptr);
		if (!bFillParamsSucc)
		{
			PyErr_Format(PyExc_Exception, "%s: Can't fill this func params for '%s'", PyType->tp_name, TCHAR_TO_UTF8(*FuncName.ToString()));
			return false;
		}

		Func->Bind();
		Func->StaticLink(true);

		// Create the FDelegateProp from this func
		FMulticastInlineDelegateProperty* DelegateProp = NePySubclassingNewDelegate(NewClass, DelegateName, Func);
		if (!DelegateProp)
		{
			PyErr_Format(PyExc_Exception, "%s: Failed to create delegate for '%s'", PyType->tp_name, InFieldName);
			return false;
		}
		EPropertyFlags UserDefineFlags = CPF_BlueprintVisible | CPF_BlueprintAssignable;
		DelegateProp->SetPropertyFlags(UserDefineFlags);

		// 应用说明符
		FNePySpecifier::ApplyToDelegate(InPyPropDef->Specifiers, DelegateProp);

		NewClass->AddCppProperty(DelegateProp);

		NePyGenUtil::FDelegateDef& PropDef = *NewClass->DelegateDefs.Add_GetRef(MakeShared<NePyGenUtil::FDelegateDef>());
		PropDef.GetSetName = NePyGenUtil::UTF8ToUTF8Buffer(InFieldName);
		PropDef.Prop = DelegateProp;
		return true;
	}

	bool CopyPropertiesFromOldClass()
	{
		check(OldClass);

		NewClass->PropertyDefs.Reserve(OldClass->PropertyDefs.Num());
		for (const TSharedPtr<NePyGenUtil::FPropertyDef>& OldPropDef : OldClass->PropertyDefs)
		{
			const FProperty* OldProp = OldPropDef->Prop;
			const UFunction* OldGetFunc = OldPropDef->GetFunc;
			const UFunction* OldSetFunc = OldPropDef->SetFunc;

			FProperty* Prop = CastFieldChecked<FProperty>(FField::Duplicate(OldProp, NewClass, OldProp->GetFName()));
			if (!Prop)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to duplicate property for '%s'", PyType->tp_name, OldPropDef->GetSetName.GetData());
				return false;
			}

#if WITH_EDITORONLY_DATA
			FField::CopyMetaData(OldProp, Prop);
#endif // WITH_EDITORONLY_DATA
			NewClass->AddCppProperty(Prop);
			if (Prop->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
			{
				NewClass->ClassFlags |= CLASS_HasInstancedReference;
			}

			NePyGenUtil::FPropertyDef& PropDef = *NewClass->PropertyDefs.Add_GetRef(MakeShared<NePyGenUtil::FPropertyDef>());
			PropDef.GetSetName = OldPropDef->GetSetName;
			PropDef.Prop = Prop;
			PropDef.DefaultValue = OldPropDef->DefaultValue;
			if (OldGetFunc)
			{
				PropDef.GetFunc = NewClass->FindFunctionByName(OldGetFunc->GetFName());
			}
			if (OldSetFunc)
			{
				PropDef.SetFunc = NewClass->FindFunctionByName(OldSetFunc->GetFName());
			}
		}

		return true;
	}

	bool CopyFunctionsFromOldClass()
	{
		check(OldClass);

		NewClass->PyGeneratedFuncs.Reserve(OldClass->PyGeneratedFuncs.Num());
		for (const UNePyGeneratedFunction* OldFunc : OldClass->PyGeneratedFuncs)
		{
			UNePyGeneratedFunction* Func = DuplicateObject<UNePyGeneratedFunction>(OldFunc, NewClass, OldFunc->GetFName());
			if (!Func)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to duplicate function for '%s'", PyType->tp_name, OldFunc->PyFuncName.GetData());
				return false;
			}

			Func->Next = NewClass->Children;
			NewClass->Children = Func;

#if WITH_EDITORONLY_DATA
#if (ENGINE_MAJOR_VERSION > 5) || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6)
			FField::CopyMetaData(((UFunction*)OldFunc)->GetAssociatedFField(), Func->GetAssociatedFField());
#else
			UMetaData::CopyMetadata((UFunction*)OldFunc, Func);
#endif
#endif // WITH_EDITORONLY_DATA
			NewClass->AddFunctionToFunctionMap(Func, Func->GetFName());

			Func->Bind();
			Func->StaticLink(true);
			Func->InitializePythonFunction(OldFunc->PyFunc, OldFunc->PyFuncName.GetData());
			Func->bEnableFastPath = OldFunc->bEnableFastPath;
			NewClass->PyGeneratedFuncs.Add(Func);
		}

		return true;
	}

	bool CopyComponentsFromOldClass()
	{
		check(OldClass);

		NewClass->ComponentDefs.Reserve(OldClass->ComponentDefs.Num());
		for (const TSharedPtr<NePyGenUtil::FComponentDef>& OldPropDef : OldClass->ComponentDefs)
		{
			const FProperty* OldProp = OldPropDef->Prop;			

			FProperty* Prop = CastFieldChecked<FProperty>(FField::Duplicate(OldProp, NewClass, OldProp->GetFName()));
			if (!Prop)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to duplicate property for '%s'", PyType->tp_name, OldPropDef->FieldName.GetData());
				return false;
			}

#if WITH_EDITORONLY_DATA
			FField::CopyMetaData(OldProp, Prop);
#endif // WITH_EDITORONLY_DATA
			NewClass->AddCppProperty(Prop);
			NewClass->ClassFlags |= CLASS_HasInstancedReference;

			NePyGenUtil::FComponentDef& PropDef = *NewClass->ComponentDefs.Add_GetRef(MakeShared<NePyGenUtil::FComponentDef>());
			PropDef.Prop = Prop;
			PropDef.ComponentClass = CastFieldChecked<FObjectProperty>(Prop)->PropertyClass;
			PropDef.bRoot = OldPropDef->bRoot;
			PropDef.FieldName = OldPropDef->FieldName;
			PropDef.AttachName = OldPropDef->AttachName;
			PropDef.SocketName = OldPropDef->SocketName;
			PropDef.OverrideName = OldPropDef->OverrideName;
		}

		return true;
	}

	bool CopyDelegatesFromOldClass()
	{
		check(OldClass);

		NewClass->DelegateDefs.Reserve(OldClass->DelegateDefs.Num());
		for (const TSharedPtr<NePyGenUtil::FDelegateDef>& OldPropDef : OldClass->DelegateDefs)
		{
			const FMulticastInlineDelegateProperty* OldProp = OldPropDef->Prop;

			FMulticastInlineDelegateProperty* Prop = CastFieldChecked<FMulticastInlineDelegateProperty>(FField::Duplicate(OldProp, NewClass, OldProp->GetFName()));
			if (!Prop)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to duplicate property for '%s'", PyType->tp_name, OldPropDef->GetSetName.GetData());
				return false;
			}
			NePyGenUtil::FDelegateDef& PropDef = *NewClass->DelegateDefs.Add_GetRef(MakeShared<NePyGenUtil::FDelegateDef>());
			PropDef.GetSetName = OldPropDef->GetSetName;
			PropDef.Prop = Prop;
			// Do Func Bind
			auto NewFunc = Prop->SignatureFunction;
			NewFunc->Bind();
			NewFunc->StaticLink(true);
#if WITH_EDITORONLY_DATA
			FField::CopyMetaData(OldProp, Prop);
#endif // WITH_EDITORONLY_DATA
			NewClass->AddCppProperty(Prop);
		}
		return true;
	}

	void ExtractInitDefaultFunc()
	{
		if (PyType)
		{
			NewClass->PyInitDefaultFunc = NePyGenUtil::GetInitDefaultFunc(PyType);
		}
	}

	// 预处理 __init_pyobj__ 函数
	// 并不会缓存下来，只是格式检查，并初始化这个类变量，防止后续 PyObject_GetAttrString 无法命中导致属性搜索时间过长
	void PreprocessInitPythonObjectFunc()
	{
		FNePyObjectPtr InitFunc = NePyStealReference(PyObject_GetAttrString((PyObject*)PyType, NePyGenUtil::InitPythonObjectFuncName));
		if (!InitFunc)
		{
			PyErr_Clear();
			// 这里做了一个性能优化，在没有这个函数的类型中塞入一个 None，用于初始化 __init_pyobj__ 变量，加速后续 GetAttrString 的查找速度
			PyObject_SetAttrString((PyObject*)PyType, NePyGenUtil::InitPythonObjectFuncName, Py_None);
			return;
		}

		if (InitFunc.Get() == Py_None)  // 一般是父类已经赋空值的情况
		{
			return;
		}

		if (!PyCallable_Check(InitFunc))
		{
			PyErr_Format(PyExc_TypeError, "'%s.%s' is not callable", PyType->tp_name, NePyGenUtil::InitPythonObjectFuncName);
			return;
		}
	}

	bool HasOldClass() const
	{
		return OldClass != nullptr;
	}

protected:
	bool RegisterDescriptors(TArray<FNePyObjectPtr>& OutDescriptors)
	{
		for (const TSharedPtr<NePyGenUtil::FPropertyDef>& PropDef : NewClass->PropertyDefs)
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(PyType, PropDef->Prop, PropDef->GetSetName.GetData(), false));
			if (!Descr)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for '%s'", PyType->tp_name, PropDef->GetSetName.GetData());
				return false;
			}

			FNePyPropertyDescriptor* PropDescr = (FNePyPropertyDescriptor*)(Descr.Get());
			if (PropDef->GetFunc)
			{
				PropDescr->GetFunc = NePyGenUtil::FMethodDef::New(PropDef->GetFunc, PyType->tp_name);
			}
			if (PropDef->SetFunc)
			{
				PropDescr->SetFunc = NePyGenUtil::FMethodDef::New(PropDef->SetFunc, PyType->tp_name);
			}

			OutDescriptors.Add(MoveTemp(Descr));
		}

		for (const TSharedPtr<NePyGenUtil::FDelegateDef>& PropDef : NewClass->DelegateDefs)
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(PyType, PropDef->Prop, PropDef->GetSetName.GetData(), false));
			if (!Descr)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for '%s'", PyType->tp_name, PropDef->GetSetName.GetData());
				return false;
			}

			OutDescriptors.Add(MoveTemp(Descr));
		}

		for (const TSharedPtr<NePyGenUtil::FComponentDef>& PropDef : NewClass->ComponentDefs)
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(PyType, PropDef->Prop, PropDef->FieldName.GetData(), false));
			if (!Descr)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for '%s'", PyType->tp_name, PropDef->FieldName.GetData());
				return false;
			}

			OutDescriptors.Add(MoveTemp(Descr));
		}

		for (UNePyGeneratedFunction* Func : NewClass->PyGeneratedFuncs)
		{
			FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewFunction(PyType, Func, Func->PyFuncName.GetData(), false));
			if (!Descr)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for '%s'", PyType->tp_name, Func->PyFuncName.GetData());
				return false;
			}

			OutDescriptors.Add(MoveTemp(Descr));
		}

		{
			static PyMethodDef NePyGeneratedClassMethod_Class = { "Class", NePyCFunctionCast(&FNePyDynamicClassType::Class), METH_NOARGS | METH_CLASS, "" };
			FNePyObjectPtr Descr = NePyStealReference(PyDescr_NewClassMethod(PyType, &NePyGeneratedClassMethod_Class));
			if (!Descr || PyDict_SetItemString(PyType->tp_dict, "Class", Descr) < 0)
			{
				PyErr_Format(PyExc_Exception, "%s: Failed to create descriptor for 'Class'", PyType->tp_name);
				return false;
			}
		}

		return true;
	}

	// 将NewClass的属性转移给OldClass
	void ReplaceOldClassProperties()
	{
#if WITH_EDITOR
		Reload.Reset(new FNePyClassReload(OldClass));
#endif
		OldClass->PurgeClass(true);

		OldClass->SetSuperStruct(NewClass->GetSuperClass());
		OldClass->ClassFlags = NewClass->ClassFlags;
		OldClass->PyInitDefaultFunc = NewClass->PyInitDefaultFunc;
		NewClass->PyInitDefaultFunc = nullptr;
		OldClass->PropertyDefs = MoveTemp(NewClass->PropertyDefs);
		OldClass->ComponentDefs = MoveTemp(NewClass->ComponentDefs);
		OldClass->DelegateDefs = MoveTemp(NewClass->DelegateDefs);
		OldClass->ClearPythonGeneratedFunctions(true);
		OldClass->PyGeneratedFuncs = MoveTemp(NewClass->PyGeneratedFuncs);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
		OldClass->FieldNotifies = MoveTemp(NewClass->FieldNotifies);
		OldClass->OtherFieldNotifyToTriggerMap = MoveTemp(NewClass->OtherFieldNotifyToTriggerMap);
#endif

		check(!OldClass->ChildProperties);
		OldClass->ChildProperties = NewClass->ChildProperties;
		NewClass->ChildProperties = nullptr;

		FField* LastFField = OldClass->ChildProperties;
		while (LastFField)
		{
			check(LastFField->Owner == NewClass);
			LastFField->Owner = OldClass;
			LastFField = LastFField->Next;
		}

		check(!OldClass->Children);
		OldClass->Children = NewClass->Children;
		NewClass->Children = nullptr;

		UField* LastUField = OldClass->Children;
		while (LastUField)
		{
			check(LastUField->GetOuter() == NewClass);
			LastUField->Rename(nullptr, OldClass,
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
			if (UFunction* Func = Cast<UFunction>(LastUField))
			{
				NePyGenUtil::UpdateReloadedPropertyStructReferences(Func);
				Func->Bind();
				Func->StaticLink(true);
				if (!Func->HasAnyFunctionFlags(FUNC_Delegate | FUNC_MulticastDelegate))
				{
					OldClass->AddFunctionToFunctionMap(Func, Func->GetFName());
				}
			}
			LastUField = LastUField->Next;
		}

		for (auto& PropDef : OldClass->PropertyDefs)
		{
			if (PropDef->GetFunc)
			{
				PropDef->GetFunc = OldClass->FindFunctionByName(PropDef->GetFunc->GetFName());
			}
			if (PropDef->SetFunc)
			{
				PropDef->SetFunc = OldClass->FindFunctionByName(PropDef->SetFunc->GetFName());
			}
		}

		NePyGenUtil::UpdateReloadedPropertyStructReferences(OldClass);
	}

	// 目前只有 GeneratedClass 会根据基类所在包而切换到 EditorPackage
	static UObject* GetGenClassOuter(const UClass* SuperClass)
	{
		auto SuperOuter = SuperClass->GetPackage();
		if (SuperOuter->HasAnyPackageFlags(PKG_EditorOnly))
		{
			return GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Editor);
		}
		else
		{
			return GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Runtime);
		}
	}

	static UNePyGeneratedClass* FindOldClass(const FString& ClassName)
	{
		for (ENePyGeneratedTypeContainerType ContainerType : TEnumRange<ENePyGeneratedTypeContainerType>())
		{
			UNePyGeneratedClass* OldClass = FindObject<UNePyGeneratedClass>(GetNePyGeneratedTypeContainer(ContainerType), *ClassName);
			if (OldClass != nullptr)
			{
				return OldClass;
			}
		}
		return nullptr;
	}

protected:
	FString ClassName;
	PyTypeObject* PyType;
	TArray<FNePySpecifier*> Specifiers;
	UNePyGeneratedClass* OldClass;
	UNePyGeneratedClass* NewClass;
	UNePyGeneratedClass* FinalClass;
#if WITH_EDITOR
	TUniquePtr<FNePyClassReload> Reload;
#endif
};

void UNePyGeneratedClass::BeginDestroy()
{
	ReleasePythonResources();
	ClearPythonGeneratedFunctions();
#if WITH_EDITOR
	if (Specifiers.Num() > 0)
	{
		FNePySpecifier::ReleaseSpecifiers(Specifiers);
	}
#endif // WITH_EDITOR
	Super::BeginDestroy();
}

void UNePyGeneratedClass::PostLoad()
{
	Super::PostLoad();
	// 继承自UBlueprintGeneratedClass后，需要清除RF_Standalone这个Flag，否则会导致它无法被GC回收，进而导致很诡异的Reload Crash
	ClearFlags(RF_Standalone);
}

void UNePyGeneratedClass::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	// 由于黑了 bCooked，Duplicate 后的 REINST_ 对象进到 Super::PostLoad 之后会跑一堆本来不该跑的逻辑，但是 FProperty 没有完全被复制，需要先 Link 一次
	FArchive Ar;
	for (FField* Field = ChildProperties; (Field != NULL) && (Field->GetOwner<UObject>() == this); Field = Field->Next)
	{
		if (FProperty* Property = CastField<FProperty>(Field))
		{
			Property->LinkWithoutChangingOffset(Ar);
		}
	}
}

void UNePyGeneratedClass::GetLifetimeBlueprintReplicationList(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	for (TFieldIterator<FProperty> It(this, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		FProperty* Prop = *It;
		if (Prop != NULL && Prop->GetPropertyFlags() & CPF_Net)
		{
			OutLifetimeProps.AddUnique(FLifetimeProperty(Prop->RepIndex, Prop->GetBlueprintReplicationCondition(), REPNOTIFY_OnChanged, PUSH_MAKE_BP_PROPERTIES_PUSH_MODEL()));
		}
	}

	UBlueprintGeneratedClass* SuperBPClass = Cast<UBlueprintGeneratedClass>(GetSuperStruct());
	if (SuperBPClass != NULL)
	{
		SuperBPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

void UNePyGeneratedClass::ReleasePythonResources()
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

bool UNePyGeneratedClass::IsFunctionImplementedInScript(FName InFunctionName) const
{
	UFunction* Function = FindFunctionByName(InFunctionName);
	return Function && Function->GetOuter() && Function->GetOuter()->IsA(UNePyGeneratedClass::StaticClass());
}

#if ENGINE_MAJOR_VERSION < 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0)
void UNePyGeneratedClass::PostInitInstance(UObject* InObj)
{
	Super::PostInitInstance(InObj);
#else
void UNePyGeneratedClass::PostInitInstance(UObject* InObj, FObjectInstancingGraph* InstanceGraph)
{
	Super::PostInitInstance(InObj, InstanceGraph);
#endif // ENGINE_MAJOR_VERSION

	// 下面这段摘自 FObjectInitializer::PostConstructInit()
	// 原逻辑中因为 GeneratedClass 没有通过 HasAnyClassFlags(CLASS_CompiledFromBlueprint)的校验，导致没有执行 InstanceSubobjects
	const bool bIsCDO = InObj->HasAnyFlags(RF_ClassDefaultObject);
	const bool bShouldInitializePropsFromArchetype = !this->HasAnyClassFlags(CLASS_Native | CLASS_Intrinsic);
	const bool bNeedInstancing = (!bIsCDO || bShouldInitializePropsFromArchetype) && this->HasAnyClassFlags(CLASS_HasInstancedReference);
	if (bNeedInstancing)
	{
		FObjectInitializer* CurrentInitializer = FUObjectThreadContext::Get().TopInitializer();
		FScriptIntegrationObjectHelper::InstanceSubobjects(*CurrentInitializer, this, bNeedInstancing, false);
	}

	// 为了避免在Instance Component时会析构UObject，在Instance前给Subclassing的Component Class临时加了个Native的flag
	// 所以在要Instance后把这个临时添加的flag移除
	UClass* Class = InObj->GetClass();
	const UNePyGeneratedClass* PyClass = GetFirstGeneratedClass(Class);
	if (PyClass)
	{
		for (int32 Index = PyClass->ComponentDefs.Num() - 1; Index >= 0; --Index)
		{
			const TSharedPtr<NePyGenUtil::FComponentDef>& CompDef = PyClass->ComponentDefs[Index];
			if (GetFirstGeneratedClass(CompDef->ComponentClass))
			{
				CompDef->ComponentClass->ClassFlags &= ~(CLASS_Native|CLASS_CompiledFromBlueprint);
			}
		}
	}	
}

UNePyGeneratedClass* UNePyGeneratedClass::GenerateClass(PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs)
{
	UClass* SuperClass = NePyBase::ToCppClass((PyObject*)InPyType->tp_base);
	check(SuperClass);

	if (SuperClass->IsChildOf(USubsystem::StaticClass()))
	{
		FNePythonBindingModuleInterface* PythonModule = FModuleManager::GetModulePtr<FNePythonBindingModuleInterface>("NePythonBinding");
		if (PythonModule && PythonModule->NeedRunPatch())
		{
			// 限制:不支持继承USubsystem
			// 由于Patch模式下NePython初始化延迟,在GameInstance初始化USubsystem时
			// Python生成的Subsystem子类尚未注册,导致无法被正确遍历和初始化
			PyErr_Format(PyExc_TypeError, "Type '%s' cannot be subclassed because it derives from USubsystem, which is not supported in patch mode.", InPyType->tp_name);
			return nullptr;
		}
	}

	// Builder used to generate the class
	FNePyGeneratedClassBuilder PythonClassBuilder(InPyType->tp_name, SuperClass, InPyType, InPySpecifierPairs);

#if !WITH_EDITOR
	if (PythonClassBuilder.HasOldClass())
	{
		// Subclassing类目前只支持在编辑器模式下进行Reload
		PyErr_Format(PyExc_Exception, "Regenerate subclassing class '%s' is not allowed in standalone build", InPyType->tp_name);
		return nullptr;
	}
#endif

	TArray<TPair<PyObject*, FNePyUFunctionDef*>> PyFuncDefs;
	TArray<TPair<PyObject*, FNePyFPropertyDef*>> PyPropDefs;
	TArray<TPair<PyObject*, FNePyUComponentDef*>> PyCompDefs;
	TArray<TPair<PyObject*, FNePyUDelegateDef*>> PyDelegDefs;
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			if (FNePyUFunctionDef* PyFuncDef = FNePyUFunctionDef::Check(FieldValue))
			{
				PyFuncDefs.Emplace(FieldKey, PyFuncDef);
			}
			else if (FNePyFPropertyDef* PyPropDef = FNePyFPropertyDef::Check(FieldValue))
			{
				PyPropDefs.Emplace(FieldKey, PyPropDef);
			}
			else if (FNePyUComponentDef* PyComDef = FNePyUComponentDef::Check(FieldValue))
			{
				if (!SuperClass->IsChildOf(AActor::StaticClass()))
				{
					// UActorComponent is only support on AActor
					PyErr_Format(PyExc_Exception, "%s: UActorComponent is only support on AActor", InPyType->tp_name);
					return nullptr;
				}

				PyCompDefs.Emplace(FieldKey, PyComDef);
			}
			else if(FNePyUDelegateDef * PyDeleDef = FNePyUDelegateDef::Check(FieldValue))
			{
				PyDelegDefs.Emplace(FieldKey, PyDeleDef);
			}
			else if (FNePyUValueDef::Check(FieldValue))
			{
				// Values are not supported on classes
				PyErr_Format(PyExc_Exception, "%s: Classes do not support values", InPyType->tp_name);
				return nullptr;
			}
			// 检查一下类成员是不是Python生成类对象
			// 形如: 下面实例的BombClass
			// @ue.uclass()
			// class BombWeapon(BaseWeapon) :
			//		BombClass = ue.LoadClass('/Game/Gameplay/Attack/Blueprints/Projectile/BP_Bomb.BP_Bomb_C')
			// 如果生成类对应的蓝图是从Python继承的，因为有可能因为SubClassing初始化顺序导致问题，输出警告
			else if (UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(NePyBase::ToCppObject(FieldValue, UBlueprintGeneratedClass::StaticClass())))
			{
				for (UStruct* SuperStruct = BPGC->GetSuperStruct(); SuperStruct; SuperStruct = SuperStruct->GetSuperStruct())
				{
					// 这个BPGC是从Python生成的
					if (UNePyGeneratedClass* SuperBPGC = Cast<UNePyGeneratedClass>(SuperStruct))
					{
						const char* FieldName;
						if (NePyBase::ToCpp(FieldKey, FieldName))
						{
							UE_LOG(LogNePython, Warning, TEXT("Field '%s' in class '%s' is BPGC which Blueprint inherits from PythonGeneratedClass '%s'. This may cause asset issues by initialization order."),
								UTF8_TO_TCHAR(FieldName),
								UTF8_TO_TCHAR(InPyType->tp_name),
								*SuperBPGC->GetName());
						}
						break;
					}
				}
			}
		}
	}

	PyFuncDefs.Sort([](const TPair<PyObject*, FNePyUFunctionDef*>& Left, const TPair<PyObject*, FNePyUFunctionDef*>& Right) {
		return Left.Value->DefineOrder > Right.Value->DefineOrder;
		});

	PyPropDefs.Sort([](const TPair<PyObject*, FNePyFPropertyDef*>& Left, const TPair<PyObject*, FNePyFPropertyDef*>& Right) {
		return Left.Value->DefineOrder > Right.Value->DefineOrder;
		});

	PyCompDefs.Sort([](const TPair<PyObject*, FNePyUComponentDef*>& Left, const TPair<PyObject*, FNePyUComponentDef*>& Right) {
		return Left.Value->DefineOrder > Right.Value->DefineOrder;
		});

	PyDelegDefs.Sort([](const TPair<PyObject*, FNePyUDelegateDef*>& Left, const TPair<PyObject*, FNePyUDelegateDef*>& Right) {
		return Left.Value->DefineOrder > Right.Value->DefineOrder;
		});

	// Add the functions to this class
	// We have to process these first as properties may reference them as get/set functions
	for (const auto& Pair : PyFuncDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}
		
		FNePyUFunctionDef* PyFuncDef = Pair.Value;
		if (!PythonClassBuilder.CreateFunctionFromDefinition(FieldName, PyFuncDef))
		{
			return nullptr;
		}
	}

	// Add the properties to this class
	for (const auto& Pair : PyPropDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}

		FNePyFPropertyDef* PyPropDef = Pair.Value;
		if (!PythonClassBuilder.CreatePropertyFromDefinition(FieldName, PyPropDef))
		{
			return nullptr;
		}
	}

	// Add the components to this class
	for (const auto& Pair : PyCompDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}

		FNePyUComponentDef* PyComDef = Pair.Value;
		if (!PythonClassBuilder.CreateComponentFromDefinition(FieldName, PyComDef))
		{
			return nullptr;
		}
	}

	// Add the delegates to this class
	for (const auto& Pair : PyDelegDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}

		FNePyUDelegateDef* PyDelegDef = Pair.Value;
		if (!PythonClassBuilder.CreateDelegateFromDefinition(FieldName, PyDelegDef))
		{
			return nullptr;
		}
	}

	// Get InitDefault Function
	PythonClassBuilder.ExtractInitDefaultFunc();
	PythonClassBuilder.PreprocessInitPythonObjectFunc();
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	// Finalize the class with its post-init function
	return PythonClassBuilder.Finalize();
}

UNePyGeneratedClass* UNePyGeneratedClass::RegenerateNePyClass(UNePyGeneratedClass* InOldClass)
{
	// Builder used to re-generate the class
	FNePyGeneratedClassBuilder PythonClassBuilder(InOldClass);

	// Copy the data from the old class
	if (!PythonClassBuilder.CopyFunctionsFromOldClass())
	{
		return nullptr;
	}
	if (!PythonClassBuilder.CopyPropertiesFromOldClass())
	{
		return nullptr;
	}
	if (!PythonClassBuilder.CopyComponentsFromOldClass())
	{
		return nullptr;
	}
	if (!PythonClassBuilder.CopyDelegatesFromOldClass())
	{
		return nullptr;
	}

	// Get InitDefault Function
	PythonClassBuilder.ExtractInitDefaultFunc();
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	return PythonClassBuilder.Finalize();
}

void UNePyGeneratedClass::CallInitDefaultFunc()
{
	UObject* DefaultObject = GetDefaultObject(false);
	check(DefaultObject);

	bool bHasInitDefaultFunc = false;
	TArray<UNePyGeneratedClass*, TInlineAllocator<8>> PyClasses;

	{
		UNePyGeneratedClass* PyClass = this;
		while (PyClass)
		{
			bHasInitDefaultFunc = bHasInitDefaultFunc || !!PyClass->PyInitDefaultFunc;
			PyClasses.Add(PyClass);
			PyClass = Cast<UNePyGeneratedClass>(PyClass->GetSuperClass());
		}
	}

	if (!bHasInitDefaultFunc)
	{
		return;
	}

	FNePyObjectPtr PyDefaultObject = NePyStealReference(NePyBase::ToPy(DefaultObject));
	FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(1));
	PyTuple_SetItem(PyArgs, 0, NePyNewReference(PyDefaultObject).Release());

#if WITH_EDITOR
	NePyGenUtil::FInitDefaultChecker InitDefaultChecker(PyDefaultObject);
#endif

	Algo::Reverse(PyClasses); // 按照从基类到子类的顺序调用__init_default__
	for (UNePyGeneratedClass* PyClass : PyClasses)
	{
		if (PyClass->PyInitDefaultFunc)
		{
			FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyClass->PyInitDefaultFunc, PyArgs));  // 返回值并不会被使用
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
}

void UNePyGeneratedClass::InitDefaultValueForProperties()
{
	UNePyGeneratedClass* PyClass = this;
	UObject* DefaultObject = GetDefaultObject(false);
	check(DefaultObject);
	while (PyClass)
	{
		// 赋值属性默认值，因为 uproperty 不能重复定义覆盖，因此不需要逆序让基类优先
		NePyGenUtil::AssignPropertyDefaultValue(DefaultObject, PyClass->PropertyDefs);
		PyClass = Cast<UNePyGeneratedClass>(PyClass->GetSuperClass());
	}
}

void UNePyGeneratedClass::ClearPythonGeneratedFunctions(bool bMoveAside)
{
	for (UNePyGeneratedFunction* Func : PyGeneratedFuncs)
	{
		if (!Func->IsValidLowLevel()
#if ENGINE_MAJOR_VERSION >= 5
			|| !IsValid(Func)
#else
			|| Func->IsPendingKill()
#endif
			|| Func->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
		{
			continue;
		}

		Func->ClearFlags(RF_AllFlags);
		Func->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
		
		if (bMoveAside)
		{
			Func->Rename(nullptr, GetTransientPackage(),
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);

			for (TFieldIterator<const FDelegateProperty> ParamIt(Func); ParamIt; ++ParamIt)
			{
				const FDelegateProperty* DelegateProp = *ParamIt;
				if (DelegateProp->SignatureFunction)
				{
					DelegateProp->SignatureFunction->Rename(nullptr, GetTransientPackage(),
						REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
				}
			}
		}
	}
	PyGeneratedFuncs.Empty();
}

void UNePyGeneratedClass::StaticActorConstructor(const FObjectInitializer& Initializer)
{
	AActor* Actor = Cast<AActor>(Initializer.GetObj());
	const UNePyGeneratedClass* PyClass = GetFirstGeneratedClass(Actor->GetClass());
	StaticActorConstructorInternal(Initializer, PyClass);
	BuildComponentAttachment(Actor, PyClass);
}

void UNePyGeneratedClass::StaticComponentConstructor(const FObjectInitializer& Initializer)
{
	// todo: 需要针对ActorComponent做特殊处理
	StaticObjectConstructor(Initializer);
}

void UNePyGeneratedClass::StaticObjectConstructor(const FObjectInitializer& Initializer)
{
	UObject* Object = Initializer.GetObj();
	const UNePyGeneratedClass* PyClass = GetFirstGeneratedClass(Object->GetClass());
	check(PyClass);
	check(PyClass->PyType);

	// 调用C++类的构造函数
	PyClass->NativeSuperClass->ClassConstructor(Initializer);

	UClass* Class = Object->GetClass();
	if (Class->HasAnyClassFlags(CLASS_Native) && Object->HasAnyFlags(RF_ClassDefaultObject))
	{
		if (PyClass == Object->GetClass())
		{
			check(IsInGameThread());
			FNePyScopedGIL GIL;
			Cast<UNePyGeneratedClass>(Object->GetClass())->InitDefaultValueForProperties();
		}
		// 由于我们通过设置CLASS_Native的方式跳过了位于FObjectInitializer::PostConstructInit()中的FObjectInitializer::InitializeProperties()
		// 我们这里需要手动初始化Python上定义的属性
		TGuardValue<EClassFlags> ClassFlagsGuardValue(Class->ClassFlags, Class->ClassFlags & ~CLASS_Native);
		for (FProperty* Prop = Class->PropertyLink; Prop; Prop = Prop->PropertyLinkNext)
		{
			if (Prop->GetOwnerClass()->HasAnyClassFlags(CLASS_Native | CLASS_Intrinsic))
			{
				// PropertyLink是按照子类到基类的顺序链接的
				// 如果我们遇到了基类的属性，就可以安全地终止遍历
				break;
			}

			if (!Prop->HasAnyPropertyFlags(CPF_ZeroConstructor)) // CDO创建时已经memzero过了
			{
				Prop->InitializeValue_InContainer(Object);
			}
		}
	}
}

const UNePyGeneratedClass* UNePyGeneratedClass::GetFirstGeneratedClass(const UClass* InClass)
{
	while (InClass)
	{
		if (const UNePyGeneratedClass* Class = Cast<UNePyGeneratedClass>(InClass))
		{
			return Class;
		}
		InClass = InClass->GetSuperClass();
	}
	return nullptr;
}

void UNePyGeneratedClass::FixupComponentAttachmentAfterReinstanceCDO(AActor* Actor)
{
	if (!Actor) return;
	USceneComponent* OldRootComp = Actor->GetRootComponent();
	if (this->NativeSuperClass && OldRootComp)
	{
		if (!FindObjectFast<UActorComponent>(this->NativeSuperClass->ClassDefaultObject, OldRootComp->GetFName()))
		{
			Actor->SetRootComponent(nullptr);
		}
	}
	else
	{
		Actor->SetRootComponent(nullptr);
	}
	ClearComponentAttachment(Actor, this);
	BuildRootComponent(Actor, this);
	if (USceneComponent* RootComp = Actor->GetRootComponent())
	{
		RootComp->SetupAttachment(nullptr);
	}
	else
	{
		if (OldRootComp)
		{
			Actor->SetRootComponent(OldRootComp);
		}
		else
		{
			// 保证蓝图的DefaultSceneRoot不会消失
			USceneComponent* RootComponent = NewObject<USceneComponent>(Actor, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
			Actor->SetRootComponent(RootComponent);
		}
	}

	BuildComponentAttachment(Actor, this);
}

void UNePyGeneratedClass::SetActorComponentPropertyDefaultValue(const FObjectInitializer& Initializer, const UObject* Archetype, AActor* Actor, UActorComponent* ActorComp, FProperty* Prop)
{
	if (ActorComp)
	{
		if (const FObjectProperty* ActorProp = CastField<FObjectProperty>(Prop))
		{
			// set property to container component, so it can be edited in blueprint.
			ActorProp->SetObjectPropertyValue_InContainer(Actor, ActorComp);

			if (Actor != Archetype)
			{
				// copy property values from CDO.
				if (UObject* CDOComp = ActorProp->GetObjectPropertyValue_InContainer(Archetype))
				{
					FScriptIntegrationObjectHelper::InitProperties(Initializer, ActorComp, CDOComp->GetClass(), CDOComp);
				}
			}
		}
	}
}

void UNePyGeneratedClass::ClearComponentAttachment(AActor* Actor, const UNePyGeneratedClass* PyClass)
{
	UClass* SuperClass = PyClass->GetSuperClass();
	if (SuperClass)
	{
		UNePyGeneratedClass* SuperPyClass = Cast<UNePyGeneratedClass>(SuperClass);
		if (SuperPyClass)
		{
			ClearComponentAttachment(Actor, SuperPyClass);
		}
	}
	for (int32 Index = PyClass->ComponentDefs.Num() - 1; Index >= 0; --Index)
	{
		const TSharedPtr<NePyGenUtil::FComponentDef>& CompDef = PyClass->ComponentDefs[Index];
		if (CompDef->OverrideName.IsNone())
		{
			UActorComponent* ActorComp = FindObjectFast<UActorComponent>(Actor, FName(CompDef->FieldName.GetData()));
			if (USceneComponent* SceneComp = Cast<USceneComponent>(ActorComp))
			{
				// 不要直接用SetupAttachment(nullptr)以防已经有Attachment
				SceneComp->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepRelative, false));
			}
		}
	}
}

void UNePyGeneratedClass::BuildRootComponent(AActor* Actor, const UNePyGeneratedClass* PyClass)
{
	UClass* SuperClass = PyClass->GetSuperClass();
	if (SuperClass)
	{
		UNePyGeneratedClass* SuperPyClass = Cast<UNePyGeneratedClass>(SuperClass);
		if (SuperPyClass)
		{
			BuildRootComponent(Actor, SuperPyClass);
		}
	}
	for (int32 Index = PyClass->ComponentDefs.Num() - 1; Index >= 0; --Index)
	{
		const TSharedPtr<NePyGenUtil::FComponentDef>& CompDef = PyClass->ComponentDefs[Index];
		if (CompDef->OverrideName.IsNone())
		{
			UActorComponent* ActorComp = FindObjectFast<UActorComponent>(Actor, FName(CompDef->FieldName.GetData()));
			if (USceneComponent* SceneComp = Cast<USceneComponent>(ActorComp))
			{
				if (CompDef->bRoot)
				{
					USceneComponent* PreviousRoot = Actor->GetRootComponent();

					// Component should become the root component, since we don't have any root component or Component was set to be root component
					SceneComp->SetupAttachment(nullptr);
					Actor->SetRootComponent(SceneComp);

					// Attach previous root component to this component				
					if (PreviousRoot != nullptr && PreviousRoot != SceneComp)
					{
						PreviousRoot->SetupAttachment(SceneComp);
					}
				}
				else if (Actor->GetRootComponent() == nullptr && CompDef->AttachName.IsNone())
				{
					// Component should become the root component, since we don't have any
					SceneComp->SetupAttachment(nullptr);
					Actor->SetRootComponent(SceneComp);
				}
			}
		}
	}
}

void UNePyGeneratedClass::BuildComponentAttachmentByCurrentPyClass(AActor* Actor, const UNePyGeneratedClass* PyClass)
{
	USceneComponent* RootComp = Actor->GetRootComponent();
	if (!RootComp)
	{
		RootComp = (USceneComponent*)Actor->CreateDefaultSubobject(
			USceneComponent::GetDefaultSceneRootVariableName(),
			USceneComponent::StaticClass(),
			USceneComponent::StaticClass(),
			true,
			false);
		Actor->SetRootComponent(RootComp);
	}

	for (int32 Index = PyClass->ComponentDefs.Num() - 1; Index >= 0; --Index)
	{
		const TSharedPtr<NePyGenUtil::FComponentDef>& CompDef = PyClass->ComponentDefs[Index];
		if (CompDef->OverrideName.IsNone())
		{
			if (UActorComponent* ActorComp = FindObjectFast<UActorComponent>(Actor, FName(CompDef->FieldName.GetData())))
			{
				USceneComponent* SceneComp = Cast<USceneComponent>(ActorComp);
				if (!CompDef->AttachName.IsNone())
				{
					USceneComponent* TargetSceneComp = FindObjectFast<USceneComponent>(Actor, CompDef->AttachName);
					if (SceneComp && TargetSceneComp)
					{
						if (SceneComp != TargetSceneComp)
						{
							if (TargetSceneComp->GetFName() == RootComp->GetFName())
							{
								SceneComp->SetupAttachment(RootComp);
							}
							else
							{
								SceneComp->SetupAttachment(TargetSceneComp, CompDef->SocketName);
							}
						}
						else
						{
							UE_LOG(LogNePython, Warning, TEXT("Component %s cannot attach to itself!"), ANSI_TO_TCHAR(CompDef->FieldName.GetData()));
							SceneComp->SetupAttachment(RootComp);
						}
					}
					else
					{
						if (!SceneComp)
						{
							UE_LOG(LogNePython, Warning, TEXT("Component %s is not USceneComponent,so it can not to attach!"), ANSI_TO_TCHAR(CompDef->FieldName.GetData()));
						}
						else if (SceneComp != RootComp)
						{
							SceneComp->SetupAttachment(RootComp);
						}

						if (!TargetSceneComp)
						{
							UE_LOG(LogNePython, Warning, TEXT("Can not found Component %s or it is not a USceneComponent!"), *CompDef->AttachName.ToString());
						}
					}
				}
				else
				{
					if (SceneComp && SceneComp != RootComp)
					{
						SceneComp->SetupAttachment(RootComp);
					}
				}
			}
		}
	}
}

void UNePyGeneratedClass::BuildComponentAttachment(AActor* Actor, const UNePyGeneratedClass* PyClass)
{
	UClass* SuperClass = PyClass->GetSuperClass();
	if (SuperClass)
	{
		UNePyGeneratedClass* SuperPyClass = Cast<UNePyGeneratedClass>(SuperClass);
		if (SuperPyClass)
		{
			BuildComponentAttachment(Actor, SuperPyClass);
		}
	}
	BuildComponentAttachmentByCurrentPyClass(Actor, PyClass);
}

void UNePyGeneratedClass::StaticActorConstructorInternal(const FObjectInitializer& Initializer, const UNePyGeneratedClass* PyClass)
{
	for (int32 Index = PyClass->ComponentDefs.Num() - 1; Index >= 0; --Index)
	{
		const TSharedPtr<NePyGenUtil::FComponentDef>& CompDef = PyClass->ComponentDefs[Index];
		if (!CompDef->OverrideName.IsNone())
		{
			Initializer.SetDefaultSubobjectClass(CompDef->OverrideName, CompDef->ComponentClass);
		}

		// Component属性在Instance时，假如Component的Class不是Native，那么在Instance完会析构构造出来的UObject。
		// Instance的操作，在打包后的包体中是在非Game线程里执行的，也就是会在非Game线程里执行UObject的析构，从而导致Assert。
		// 所以在Instance Component前先给Suclassing的Component的ClassFlags设置为CLASS_Native，这样在Instance时，不会析构。
		if (GetFirstGeneratedClass(CompDef->ComponentClass))
		{
			CompDef->ComponentClass->ClassFlags |= (CLASS_Native|CLASS_CompiledFromBlueprint);
		}
	}

	bool bStaticObjectConstructor = true;
	UClass* SuperClass = PyClass->GetSuperClass();
	if (SuperClass)
	{
		UNePyGeneratedClass* SuperPyClass = Cast<UNePyGeneratedClass>(SuperClass);
		if (SuperPyClass)
		{
			bStaticObjectConstructor = false;
			StaticActorConstructorInternal(Initializer, SuperPyClass);
		}
	}
	if (bStaticObjectConstructor)
	{
		StaticObjectConstructor(Initializer);
	}

	AActor* Actor = Cast<AActor>(Initializer.GetObj());

	// const UObject* Archetype = PyClass->GetDefaultObject();
	const UObject* Archetype = GetFirstGeneratedClass(Actor->GetClass())->GetDefaultObject();

	for (int32 Index = PyClass->ComponentDefs.Num() - 1; Index >= 0; --Index)
	{
		const TSharedPtr<NePyGenUtil::FComponentDef>& CompDef = PyClass->ComponentDefs[Index];
		if (!CompDef->OverrideName.IsNone())
		{
			UActorComponent* ActorComp = FindObjectFast<UActorComponent>(Actor, CompDef->OverrideName);
			SetActorComponentPropertyDefaultValue(Initializer, Archetype, Actor, ActorComp, CompDef->Prop);
		}
		else
		{
			UActorComponent* ActorComp = Cast<UActorComponent>(Initializer.CreateDefaultSubobject(
				Actor,
				FName(CompDef->FieldName.GetData()),
				CompDef->ComponentClass,
				CompDef->ComponentClass,
				true,
				false
			));
			SetActorComponentPropertyDefaultValue(Initializer, Archetype, Actor, ActorComp, CompDef->Prop);
			if (ActorComp)
			{
				if (USceneComponent* SceneComp = Cast<USceneComponent>(ActorComp))
				{
					if (CompDef->bRoot)
					{
						USceneComponent* PreviousRoot = Actor->GetRootComponent();

						// Component should become the root component, since we don't have any root component or Component was set to be root component
						SceneComp->SetupAttachment(nullptr);
						Actor->SetRootComponent(SceneComp);

						// Attach previous root component to this component				
						if (PreviousRoot != nullptr && PreviousRoot != SceneComp)
						{
							PreviousRoot->SetupAttachment(SceneComp);
						}
					}
					else if (Actor->GetRootComponent() == nullptr && CompDef->AttachName.IsNone())
					{
						// Component should become the root component, since we don't have any
						SceneComp->SetupAttachment(nullptr);
						Actor->SetRootComponent(SceneComp);
					}
				}
			}
		}
	}
}

class FNePyRegeneratedFunctionsBuilder: public FNePyGeneratedClassBuilder
{
public:
	FNePyRegeneratedFunctionsBuilder(UNePyGeneratedClass* InOldClass)
		: FNePyGeneratedClassBuilder(InOldClass)
	{
	}

	UNePyGeneratedClass* Finalize()
	{
		// Replace the definitions with real descriptors
		TArray<FNePyObjectPtr> PyDescriptors;
		if (!RegisterDescriptors(PyDescriptors))
		{
			return nullptr;
		}

		// 清理并替换脚本层的Descriptor
		NePyType_CleanupDescriptors(PyType, false);
		for (auto& PyObjPtr : PyDescriptors)
		{
			FNePyDescriptorBase* PyDescr = (FNePyDescriptorBase*)PyObjPtr.Get();
			PyDict_SetItem(PyType->tp_dict, PyDescr->Name, PyDescr);
		}
		PyType_Modified(PyType);

		check(OldClass);
		ReplaceOldClassFunctions();

		FinalClass->AssembleReferenceTokenStream(true);

		return FinalClass;
	}

private:
	// 将NewClass的方法转移给OldClass
	void ReplaceOldClassFunctions()
	{
		for (UNePyGeneratedFunction* PyFunc : OldClass->PyGeneratedFuncs)
		{
			OldClass->RemoveFunctionFromFunctionMap(PyFunc);
		}

		OldClass->ClearPythonGeneratedFunctions(true);
		OldClass->PyGeneratedFuncs = MoveTemp(NewClass->PyGeneratedFuncs);

		OldClass->Children = NewClass->Children;
		NewClass->Children = nullptr;

		UField* LastUField = OldClass->Children;
		while (LastUField)
		{
			check(LastUField->GetOuter() == NewClass);
			LastUField->Rename(nullptr, OldClass,
				REN_DoNotDirty | REN_DontCreateRedirectors | REN_ForceNoResetLoaders);
			if (UFunction* Func = Cast<UFunction>(LastUField))
			{
				if (!Func->HasAnyFunctionFlags(FUNC_Delegate | FUNC_MulticastDelegate))
				{
					OldClass->AddFunctionToFunctionMap(Func, Func->GetFName());
				}
			}
			LastUField = LastUField->Next;
		}

		for (auto& PropDef : OldClass->PropertyDefs)
		{
			if (PropDef->GetFunc)
			{
				PropDef->GetFunc = OldClass->FindFunctionByName(PropDef->GetFunc->GetFName());
			}
			if (PropDef->SetFunc)
			{
				PropDef->SetFunc = OldClass->FindFunctionByName(PropDef->SetFunc->GetFName());
			}
		}
	}
};

PyObject* NePyMethod_RegenerateFunctions(PyObject*, PyObject* InArg)
{
	if (!PyType_Check(InArg))
	{
		PyErr_Format(PyExc_TypeError, "arg1 'PyType' must be type, not %.200s",
			Py_TYPE(InArg)->tp_name);
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)InArg;
	UClass* Class = NePyBase::ToCppClass(InArg);
	if (!Class)
	{
		PyErr_Format(PyExc_TypeError, "cant find unreal class for '%.200s'",
			Py_TYPE(InArg)->tp_name);
		return nullptr;
	}

	UNePyGeneratedClass* PyClass = Cast<UNePyGeneratedClass>(Class);
	if (!PyClass)
	{
		PyErr_Format(PyExc_TypeError, "'%.200s' is not a unreal python subclass",
			Py_TYPE(InArg)->tp_name);
		return nullptr;
	}

	FNePyRegeneratedFunctionsBuilder PythonFunctionsBuilder(PyClass);

	TArray<TPair<PyObject*, FNePyUFunctionDef*>> PyFuncDefs;
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(PyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			if (FNePyUFunctionDef* PyFuncDef = FNePyUFunctionDef::Check(FieldValue))
			{
				PyFuncDefs.Emplace(FieldKey, PyFuncDef);
			}
		}
	}

	PyFuncDefs.Sort([](const TPair<PyObject*, FNePyUFunctionDef*>& Left, const TPair<PyObject*, FNePyUFunctionDef*>& Right) {
		return Left.Value->DefineOrder > Right.Value->DefineOrder;
		});

	for (const auto& Pair : PyFuncDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}
		
		FNePyUFunctionDef* PyFuncDef = Pair.Value;
		if (!PythonFunctionsBuilder.CreateFunctionFromDefinition(FieldName, PyFuncDef))
		{
			return nullptr;
		}
	}

	PythonFunctionsBuilder.Finalize();
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GenerateClass(PyObject*, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:uclass", &PyObj))
	{
		return nullptr;
	}

	if (PyObject_IsInstance(PyObj, (PyObject*)NePySubclassGetType()))
	{
		PyErr_SetString(PyExc_TypeError,
			"Can not generate class on a NePySubClass object. " \
			"Consider to call 'ue.DisableOldStyleSubclassing()' to disable old style subclassing globally, " \
			"or to add class member 'DISABLE_OLD_STYLE_SUBCLASSING = True' in the class you want to disable old style subclassing.");
		return nullptr;
	}

	if (!PyType_Check(PyObj))
	{
		PyErr_SetString(PyExc_TypeError, "@ue.uclass() should be used on a python class.");
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!GNePyDisableGeneratedType)
	{
		PyTypeObject* PyBase = PyType->tp_base;
		UClass* SuperClass = NePyBase::ToCppClass((PyObject*)PyBase);
		if (!SuperClass)
		{
			PyErr_Format(PyExc_Exception, "Type '%s' does not derive from an Unreal class type",
				PyType->tp_name);
			return nullptr;
		}

		TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;
		if (!NePyGenUtil::ParseSpecifiersFromPyDict(InKwds, PySpecifierPairs))
		{
			return nullptr;
		}

		if (!UNePyGeneratedClass::GenerateClass(PyType, PySpecifierPairs))
		{
			if (!PyErr_Occurred())
			{
				PyErr_Format(PyExc_Exception, "Failed to generate an Unreal class for the Python type '%s'",
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


struct FNePyUClassDecorator : public PyObject
{
public:
	PyObject* CachedKwds;

public:
	static FNePyUClassDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		FNePyUClassDecorator* Self = (FNePyUClassDecorator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
			Self->CachedKwds = nullptr;
		}
		return Self;
	}

	static void Dealloc(FNePyUClassDecorator* InSelf)
	{
		Py_XDECREF(InSelf->CachedKwds);
		Py_TYPE(InSelf)->tp_free(InSelf);
	}

	static int Init(FNePyUClassDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (InKwds)
		{
			Py_INCREF(InKwds);
			InSelf->CachedKwds = InKwds;
		}
		return 0;
	}

	static PyObject* Call(FNePyUClassDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		return NePyMethod_GenerateClass(nullptr, InArgs, InSelf->CachedKwds);
	}
};

static PyTypeObject NePyUClassDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	NePyInternalModuleName ".ClassDecorator", /* tp_name */
	sizeof(FNePyUClassDecorator), /* tp_basicsize */
};

PyObject* NePyMethod_UClassDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUClassDecorator* PyRet = FNePyUClassDecorator::New(&NePyUClassDecoratorType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUClassDecorator::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyTypeObject* NePyInitGeneratedClass()
{
	// @ue.uclass装饰器
	PyTypeObject* PyType = &NePyUClassDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)&FNePyUClassDecorator::New;
	PyType->tp_dealloc = (destructor)&FNePyUClassDecorator::Dealloc;
	PyType->tp_init = (initproc)&FNePyUClassDecorator::Init;
	PyType->tp_call = (ternaryfunc)&FNePyUClassDecorator::Call;
	PyType_Ready(PyType);
	return PyType;
}

void UNePyGeneratedFunction::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UNePyGeneratedFunction::Bind()
{
	SetNativeFunc(UNePyGeneratedFunction::CallPythonFunction);
}

void UNePyGeneratedFunction::ReleasePythonResources()
{
	if (PyFunc)
	{
		FNePyScopedGIL GIL;
		Py_DECREF(PyFunc);
		PyFunc = nullptr;
	}
}

void UNePyGeneratedFunction::InitializePythonFunction(PyObject* InPyFunc, const char* InPyFuncName)
{
	check(!PyFunc);
	check(InPyFunc);

	Py_INCREF(InPyFunc);
	PyFunc = InPyFunc;
	PyFuncName = NePyGenUtil::UTF8ToUTF8Buffer(InPyFuncName);
}

DEFINE_FUNCTION(UNePyGeneratedFunction::CallPythonFunction)
{
	// Note: This function *must not* return until InvokePythonCallableFromUnrealFunctionThunk has been called, as we need to step over the correct amount of data from the bytecode stack!

	bool bHasError = false;
	const UNePyGeneratedFunction* Func = CastChecked<UNePyGeneratedFunction>(Stack.CurrentNativeFunction);
	if (!Func->PyFunc)
	{
		const UNePyGeneratedClass* Class = CastChecked<UNePyGeneratedClass>(Func->GetOwnerClass());
		UE_LOG(LogNePython, Error, TEXT("Failed to call Python function for %s.%s, python funtion not found."), *Class->GetName(), ANSI_TO_TCHAR(Func->PyFuncName.GetData()));
		bHasError = true;
	}

	{
		FNePyScopedGIL GIL;

		FNePyObjectPtr PySelf;
		if (!Func->HasAnyFunctionFlags(FUNC_Static))
		{
			PySelf = NePyStealReference(FNePyHouseKeeper::Get().NewNePyObject(P_THIS_OBJECT));

		}

		if (!NePyGenUtil::InvokePythonCallableFromUnrealFunctionThunk(PySelf, Func->PyFunc, Func, Context, Stack, RESULT_PARAM) || bHasError)
		{
			PyErr_Print();
		}
	}
}
