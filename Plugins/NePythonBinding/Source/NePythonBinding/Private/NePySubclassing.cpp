#include "NePySubclassing.h"
#include "CoreMinimal.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "UObject/TextProperty.h"
#include "UObject/UnrealTypePrivate.h"
#include "Components/ActorComponent.h"
#include "NePyBase.h"
#include "NePySubclass.h"
#include "NePyObjectBase.h"
#include "NePyStructBase.h"
#include "NePyHouseKeeper.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyDescriptor.h"
#include "NePyGeneratedType.h"
#include "NePythonSettings.h"
#if WITH_EDITOR
#include "NePyBlueprintActionDatabaseHelper.h"
#include "Editor.h"
#include "EdGraphSchema_K2.h"
#include "EditorCategoryUtils.h"
#endif

// 使用指定名称和基类，创建一个新的UClass
UNePySubclass* NePySubclassingNewClassBegin(const FString& ClassName, UClass* SuperClass)
{
	UNePySubclass* NewClass = nullptr;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	UClass* OldClass = FindFirstObject<UClass>(*ClassName);
#else
	UClass* OldClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
#endif
	if (OldClass)
	{
		NewClass = Cast<UNePySubclass>(OldClass);
		if (!NewClass)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing failed! Try to overwrite existing class '%s', this is forbidden."), *ClassName);
			return nullptr;
		}
	}
	else
	{
		NewClass = NewObject<UNePySubclass>(SuperClass->GetOuter(), *ClassName, RF_Public | RF_NePyGeneratedTypeGCSafe);
		if (!NewClass)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing failed! Create new class '%s' failed."), *ClassName);
			return nullptr;
		}

		// 让HouseKeeper知道不需要追踪此类的GC
		NewClass->AddToRoot();
	}

	// 存在同名的旧类，说明是Reload
	if (OldClass)
	{
		if (OldClass->ClassDefaultObject)
		{
			OldClass->ClassDefaultObject->RemoveFromRoot();
			OldClass->ClassDefaultObject->ConditionalBeginDestroy();
			OldClass->ClassDefaultObject = nullptr;
			// 如果不立马回收ClassDefaultObject，那么之后finishdestroy用了新的UClass，内存布局有更新的话就会泄露并可能触发crash
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		}
		OldClass->PurgeClass(true);
	}

	NewClass->SetSuperStruct(SuperClass);
	NewClass->ClassWithin = SuperClass->ClassWithin;
	NewClass->ClassConfigName = SuperClass->ClassConfigName;

	NewClass->ClassFlags = (SuperClass->ClassFlags & CLASS_ScriptInherit);

	NewClass->ClassConstructor = &UNePySubclass::PySubclassConstructor;
	NewClass->Bind();
	check(NewClass->ClassConstructor == &UNePySubclass::PySubclassConstructor);
	check(NewClass->ClassVTableHelperCtorCaller == SuperClass->ClassVTableHelperCtorCaller);
#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
	check(NewClass->ClassAddReferencedObjects == SuperClass->ClassAddReferencedObjects);
#endif
	check(NewClass->ClassCastFlags == SuperClass->ClassCastFlags);

	NewClass->StaticLink(true);
	check(NewClass->PropertiesSize == SuperClass->PropertiesSize);
	check(NewClass->MinAlignment == SuperClass->MinAlignment);
	check(NewClass->PropertyLink == SuperClass->PropertyLink);
	check(NewClass->RefLink == SuperClass->RefLink);
	check(NewClass->DestructorLink == SuperClass->DestructorLink);
	check(NewClass->PostConstructLink == SuperClass->PostConstructLink);
	check(NewClass->ScriptAndPropertyObjectReferences.Num() == SuperClass->ScriptAndPropertyObjectReferences.Num());

	return NewClass;
}

// 完成新建UClass的收尾工作
void NePySubclassingNewClassFinish(UClass* NewClass, UClass* SuperClass)
{
	// 防止中间有创建CDO的，这里再清一次
	if (NewClass->ClassDefaultObject)
	{
		NewClass->ClassDefaultObject->RemoveFromRoot();
		NewClass->ClassDefaultObject->ConditionalBeginDestroy();
		NewClass->ClassDefaultObject = nullptr;
	}

	NewClass->StaticLink(true);

#if WITH_EDITOR
	// 以下代码抄自FBlueprintEditorUtils::RecreateClassMetaData
	{
		TArray<FString> AllHideCategories;
		if (!SuperClass->HasMetaData(FBlueprintMetadata::MD_IgnoreCategoryKeywordsInSubclasses))
		{
			// we want the categories just as they appear in the parent class 
			// (set bHomogenize to false) - especially since homogenization 
			// could inject spaces
			FEditorCategoryUtils::GetClassHideCategories(SuperClass, AllHideCategories, /*bHomogenize =*/false);
			if (SuperClass->HasMetaData(TEXT("ShowCategories")))
			{
				NewClass->SetMetaData(TEXT("ShowCategories"), *SuperClass->GetMetaData("ShowCategories"));
			}
			if (SuperClass->HasMetaData(TEXT("AutoExpandCategories")))
			{
				NewClass->SetMetaData(TEXT("AutoExpandCategories"), *SuperClass->GetMetaData("AutoExpandCategories"));
			}
			if (SuperClass->HasMetaData(TEXT("AutoCollapseCategories")))
			{
				NewClass->SetMetaData(TEXT("AutoCollapseCategories"), *SuperClass->GetMetaData("AutoCollapseCategories"));
			}
		}

		if (SuperClass->HasMetaData(TEXT("HideFunctions")))
		{
			NewClass->SetMetaData(TEXT("HideFunctions"), *SuperClass->GetMetaData("HideFunctions"));
		}

		if (SuperClass->IsChildOf(UActorComponent::StaticClass()))
		{
			static const FName NAME_ClassGroupNames(TEXT("ClassGroupNames"));
			NewClass->SetMetaData(FBlueprintMetadata::MD_BlueprintSpawnableComponent, TEXT("true"));

			FString ClassGroupCategory = NSLOCTEXT("BlueprintableComponents", "CategoryName", "PythonSubclassing").ToString();
			NewClass->SetMetaData(NAME_ClassGroupNames, *ClassGroupCategory);
		}

		if (SuperClass->HasMetaData(TEXT("SparseClassDataTypes")))
		{
			NewClass->SetMetaData(TEXT("SparseClassDataTypes"), *SuperClass->GetMetaData("SparseClassDataTypes"));
		}

		if (AllHideCategories.Num() > 0)
		{
			NewClass->SetMetaData(TEXT("HideCategories"), *FString::Join(AllHideCategories, TEXT(" ")));
		}
		else
		{
			NewClass->RemoveMetaData(TEXT("HideCategories"));
		}
	}
#endif

#if WITH_EDITOR
	NewClass->PostEditChange();
#endif

	NewClass->GetDefaultObject()->PostInitProperties();

#if WITH_EDITOR
	NewClass->PostLinkerChange();
#endif

	NewClass->AssembleReferenceTokenStream(true);

#if WITH_EDITOR
	// this is required for avoiding startup crashes
	// https://github.com/20tab/UnrealEnginePython/issues/450
	if (GEditor)
	{
		FNePyBlueprintActionDatabaseHelper::RegisterClassActions(NewClass);
	}
#endif
}

// 将PyType转化为FProperty
// 注意！仅转化基础类型和字符串类型！
FProperty* NePySubclassingPyTypeToFProperty(PyTypeObject* PyType, FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags, const FString& OwnerName)
{
	typedef FProperty* (*ConverterFun)(FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags);
	static TMap<PyTypeObject*, ConverterFun> ConverterMap = {
#if PY_MAJOR_VERSION < 3
		{
			&PyInt_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FIntProperty(Owner, PropName, ObjectFlags);
			}
		},
#endif
#if NEPY_SUBCLASSING_PY_LONG_AS_UE_INT64
		{
			&PyLong_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FInt64Property(Owner, PropName, ObjectFlags);
			}
		},
#else
		{
			&PyLong_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FIntProperty(Owner, PropName, ObjectFlags);
			}
		},
#endif
#if NEPY_SUBCLASSING_PY_FLOAT_AS_UE_DOUBLE
		{
			&PyFloat_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FDoubleProperty(Owner, PropName, ObjectFlags);
			}
		},
#else
		{
			&PyFloat_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FFloatProperty(Owner, PropName, ObjectFlags);
			}
		},
#endif
		{
			&PyBool_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				FBoolProperty* Prop = new FBoolProperty(Owner, PropName, ObjectFlags);
				Prop->SetBoolSize(sizeof(bool), true);
				return Prop;
			}
		},
		{
			&NePyString_Type,
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FStrProperty(Owner, PropName, ObjectFlags);
			}
		},
	};

	auto Result = ConverterMap.Find(PyType);
	if (!Result)
	{
		return nullptr;
	}

	auto Converter = *Result;
	FProperty* NewProp = Converter(Owner, PropName, ObjectFlags);
	return NewProp;
}

// 将UProperty转化为FProperty
// 注意！仅转化基础类型和字符串类型！
FProperty* NePySubclassingUPropertyToFProperty(const UClass* Class, FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags, const FString& OwnerName)
{
#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
	#include "UObject/UndefineUPropertyMacros.h"
#endif
#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6)
	typedef FProperty* (*ConverterFun)(FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags);
	static TMap<const UClass*, ConverterFun> ConverterMap = {
		{
			UByteProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FByteProperty(Owner, PropName, ObjectFlags);
			}
		},
		{
			UInt8Property::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FInt8Property(Owner, PropName, ObjectFlags);
			}
		},
		{
			UInt16Property::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FInt16Property(Owner, PropName, ObjectFlags);
			}
		},
		{
			UIntProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FIntProperty(Owner, PropName, ObjectFlags);
			}
		},
		{
			UInt64Property::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FInt64Property(Owner, PropName, ObjectFlags);
			}
		},
		{
			UUInt16Property::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FUInt16Property(Owner, PropName, ObjectFlags);
			}
		},
		{
			UUInt32Property::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FUInt32Property(Owner, PropName, ObjectFlags);
			}
		},
		{
			UUInt64Property::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FUInt64Property(Owner, PropName, ObjectFlags);
			}
		},
		{
			UFloatProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FFloatProperty(Owner, PropName, ObjectFlags);
			}
		},
		{
			UDoubleProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FDoubleProperty(Owner, PropName, ObjectFlags);
			}
		},
		{
			UBoolProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				FBoolProperty* Prop = new FBoolProperty(Owner, PropName, ObjectFlags);
				Prop->SetBoolSize(sizeof(bool), true);
				return Prop;
			}
		},
		{
			UNameProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FNameProperty(Owner, PropName, ObjectFlags);
			}
		},
		{
			UStrProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FStrProperty(Owner, PropName, ObjectFlags);
			}
		},
		{
			UTextProperty::StaticClass(),
			[](FFieldVariant Owner, const FName& PropName, EObjectFlags ObjectFlags) -> FProperty*
			{
				return new FTextProperty(Owner, PropName, ObjectFlags);
			}
		},
	};
#endif

	FProperty* NewProp = nullptr;

#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6)
	if (Class->IsChildOf<UProperty>())
	{
		auto Result = ConverterMap.Find(Class);
		if (!Result)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Can't convert %s to FProperty. Owner is %s"), *Class->GetName(), *OwnerName);
			return nullptr;
		}

		auto Converter = *Result;
		NewProp = Converter(Owner, PropName, ObjectFlags);
	}
#endif
#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
	#include "UObject/DefineUPropertyMacros.h"
#endif
	return NewProp;
}

// 根据Enum的值域，创建Enum底层属性类型
FProperty* NePyNewEnumUnderlyingProperty(FEnumProperty* EnumProp, EObjectFlags ObjectFlags)
{
	const UEnum* Enum = EnumProp->GetEnum();

	// 找到最大值
	// UEnum::GetMaxEnumValue()是int64类型的，没办法处理uint64
	uint64 MaxValue = 0;
	for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
	{
		uint64 Value = (uint64)Enum->GetValueByIndex(Index);
		if (MaxValue < Value)
		{
			MaxValue = Value;
		}
	}

	// UE会自动给Enum添加一个_MAX项，我们要减掉这一项
	// 否则原本值域为uint8（例如EViewModeIndex）的Enum，值域会被扩大成int
	if (MaxValue > 0 && Enum->ContainsExistingMax())
	{
		MaxValue -= 1;
	}

	FProperty* UnderlyingProp;
	if (MaxValue <= 0xff)
	{
		UnderlyingProp = new FByteProperty(EnumProp, FName("UnderlyingType"), ObjectFlags);
	}
	else if (MaxValue < 0xffffffff)
	{
		UnderlyingProp = new FIntProperty(EnumProp, FName("UnderlyingType"), ObjectFlags);
	}
	else if (MaxValue == 0xffffffff)
	{
		UnderlyingProp = new FUInt32Property(EnumProp, FName("UnderlyingType"), ObjectFlags);
	}
	else
	{
		UnderlyingProp = new FUInt64Property(EnumProp, FName("UnderlyingType"), ObjectFlags);
	}

	return UnderlyingProp;
}

// 根据Python对象，返回新创建的FProperty
// 如果无法创建则返回nullptr
FProperty* NePySubclassingNewProperty(FFieldVariant Owner, const FName& AttrName, PyObject* PyAttrVal, const FString& OwnerName, EPropertyFlags PropFlags, PyObject* DefaultValue, UClass* ThisClass)
{
	FProperty* NewProp = nullptr;
	EObjectFlags ObjectFlags = RF_Public | RF_NePyGeneratedTypeGCSafe;

	// 先尝试从默认值创建
	if (DefaultValue == PyAttrVal)
	{
		NewProp = NePyGenUtil::CreatePropertyFromDefaultValue(Owner, AttrName, PyAttrVal, ObjectFlags);
		if (!NewProp)
		{
			UE_LOG(LogNePython, Error, TEXT("If we have default value we should always be able to create property from this!"));
		}
	}

	// 形如:
	// class MyActor(ue.Actor):
	//		# UPARAM(ref) for subclassing
	//		@ue.ufunction(params=(ue.ref(ue.Object)))
	//		def func(refObj)
	if (FNePyRefParamDef::Check(PyAttrVal))
	{
		FNePyRefParamDef* RefParam = (FNePyRefParamDef*)PyAttrVal;
		if (ensureAlwaysMsgf(PropFlags & CPF_Parm, TEXT("Property must be a parameter")))
		{
			FProperty* RetProp = NePySubclassingNewProperty(Owner, AttrName, RefParam->TypeObject, OwnerName, PropFlags | CPF_OutParm | CPF_ReferenceParm, nullptr, ThisClass);
			return RetProp;
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("ref should only use for parameter"));
			return nullptr;
		}
	}

	// 形如:
	// class MyActor(ue.Actor):
	//     XXX = ue.uproperty([Val])
	if (!NewProp && PyList_Check(PyAttrVal))
	{
		if (PyList_Size(PyAttrVal) != 1)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Length of TArray for %s must have exactly one. Owner is %s"), *AttrName.ToString(), *OwnerName);
			return nullptr;
		}

		PyObject* PyInner = PyList_GetItem(PyAttrVal, 0);
		FArrayProperty* ArrayProp = new FArrayProperty(Owner, AttrName, ObjectFlags);
		FProperty* InnerProp = NePySubclassingNewProperty(ArrayProp, FName(AttrName.ToString() + TEXT("_Inner")), PyInner,
			OwnerName + TEXT(".") + AttrName.ToString(), PropFlags & CPF_PropagateToArrayInner, nullptr, ThisClass);

		if (!InnerProp)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Types in TArray for %s is not supported. Owner is %s"), *AttrName.ToString(), *OwnerName);
			// FArrayProperty的析构函数里没有判空……
			ArrayProp->Inner = new FProperty(ArrayProp, NAME_None, RF_NoFlags);
			delete ArrayProp;
			return nullptr;
		}

		if (InnerProp->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
		{
			ArrayProp->SetPropertyFlags(CPF_ContainsInstancedReference);
		}
		ArrayProp->Inner = InnerProp;
		NewProp = ArrayProp;
	}

	// 形如:
	// class MyActor(ue.Actor):
	//     XXX = ue.uproperty({Key: Val})
	if (!NewProp && PyDict_Check(PyAttrVal))
	{
		if (PyDict_Size(PyAttrVal) != 1)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Length of TMap for %s must have exactly one. Owner is %s"), *AttrName.ToString(), *OwnerName);
			return nullptr;
		}

		PyObject* PyElemKey = nullptr;
		PyObject* PyElemVal = nullptr;
		Py_ssize_t PyElemPos = 0;
		PyDict_Next(PyAttrVal, &PyElemPos, &PyElemKey, &PyElemVal);

		FMapProperty* MapProp = new FMapProperty(Owner, AttrName, ObjectFlags);
		FProperty* KeyProp = NePySubclassingNewProperty(MapProp, FName(AttrName.ToString() + TEXT("_Key")), PyElemKey,
			OwnerName + TEXT(".") + AttrName.ToString(), PropFlags & CPF_PropagateToMapKey, nullptr, ThisClass);
		FProperty* ValueProp = NePySubclassingNewProperty(MapProp, FName(AttrName.ToString() + TEXT("_Value")), PyElemVal,
			OwnerName + TEXT(".") + AttrName.ToString(), PropFlags & CPF_PropagateToMapValue, nullptr, ThisClass);

		MapProp->KeyProp = KeyProp;
		MapProp->ValueProp = ValueProp;

		if (!KeyProp || !ValueProp)
		{
			if (!KeyProp)
			{
				UE_LOG(LogNePython, Error, TEXT("Subclassing error! Key type in TMap for %s is not supported. Owner is %s"), *AttrName.ToString(), *OwnerName);
				// FMapProperty的析构函数里没有判空……
				MapProp->KeyProp = new FProperty(MapProp, NAME_None, RF_NoFlags);
			}
			if (!ValueProp)
			{
				UE_LOG(LogNePython, Error, TEXT("Subclassing error! Value type in TMap for %s is not supported. Owner is %s"), *AttrName.ToString(), *OwnerName);
				// FMapProperty的析构函数里没有判空……
				MapProp->ValueProp = new FProperty(MapProp, NAME_None, RF_NoFlags);
			}
			delete MapProp;
			return nullptr;
		}

		if (KeyProp->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference) ||
			ValueProp->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
		{
			MapProp->SetPropertyFlags(CPF_ContainsInstancedReference);
		}
		NewProp = MapProp;
	}

	// 形如:
	// class MyActor(ue.Actor):
	//     XXX = ue.uproperty({Val})
	if (!NewProp && PySet_Check(PyAttrVal))
	{
		if (PySet_Size(PyAttrVal) != 1)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Length of TSet for %s must have exactly one. Owner is %s"), *AttrName.ToString(), *OwnerName);
			return nullptr;
		}

		FNePyObjectPtr PyElem = NePyStealReference(PySet_Pop(PyAttrVal));
		FSetProperty* SetProp = new FSetProperty(Owner, AttrName, ObjectFlags);
		FProperty* ElementProp = NePySubclassingNewProperty(SetProp, FName(AttrName.ToString() + TEXT("_Elem")), PyElem,
			OwnerName + TEXT(".") + AttrName.ToString(), PropFlags & CPF_PropagateToSetElement, nullptr, ThisClass);

		if (!ElementProp)
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Elem type in TSet for %s is not supported. Owner is %s"), *AttrName.ToString(), *OwnerName);
			// FSetProperty的析构函数里没有判空……
			SetProp->ElementProp = new FProperty(SetProp, NAME_None, RF_NoFlags);
			delete SetProp;
			return nullptr;
		}

		if (ElementProp->HasAnyPropertyFlags(CPF_ContainsInstancedReference | CPF_InstancedReference))
		{
			SetProp->SetPropertyFlags(CPF_ContainsInstancedReference);
		}
		SetProp->ElementProp = ElementProp;
		NewProp = SetProp;
	}

	// 形如：
	// class MyActor(ue.Actor):
	//     XXX = ue.uproperty(int)
	//     XXX = ue.uproperty(long)
	//     XXX = ue.uproperty(float)
	//     XXX = ue.uproperty(bool)
	//     XXX = ue.uproperty(str)
	if (!NewProp && PyType_Check(PyAttrVal))
	{
		NewProp = NePySubclassingPyTypeToFProperty((PyTypeObject*)PyAttrVal, Owner, AttrName, ObjectFlags, OwnerName);
	}

	// 形如
	// class MyActor(ue.Actor):
	//     XXX = ue.uproperty(ue.Name)
	//     XXX = ue.uproperty(ue.Text)
	else if (FNePyTypeAlias* NePyTypeAlias = FNePyTypeAlias::Check(PyAttrVal))
	{
		if (NePyTypeAlias->AliasType == ENePyAliasType::FName)
		{
			NewProp = new FNameProperty(Owner, AttrName, ObjectFlags);
		}
		else if (NePyTypeAlias->AliasType == ENePyAliasType::FText)
		{
			NewProp = new FTextProperty(Owner, AttrName, ObjectFlags);
		}
	}

	// 形如：
	// @ue.ufunction(params=(@ue.udelegate(params=(int, bool))))
	// def FuncTakeDelegateParam(self, Delegate):
	//     ...
	if (!NewProp && (PropFlags & CPF_Parm))
	{
		if (FNePyUDelegateDef* DelegateDef = FNePyUDelegateDef::Check(PyAttrVal))
		{
			check(Owner.IsA(UFunction::StaticClass()));
			UFunction* OwnerFunc = CastChecked<UFunction>(Owner.ToUObject());
			UClass* OwnerClass = OwnerFunc->GetOwnerClass();

			const FName DelegateFuncName(FString::Printf(TEXT("%s_%s_SignatureFunction"), *OwnerFunc->GetName(), *AttrName.ToString()));
			UFunction* SignatureFunction = NewObject<UFunction>(OwnerClass, DelegateFuncName, RF_Public);
			SignatureFunction->FunctionFlags |= FUNC_Delegate;
			
			if (!NePyGenUtil::FillFuncWithParams(SignatureFunction, DelegateDef->FuncParamNames, DelegateDef->FuncParamTypes, nullptr))
			{
				return nullptr;
			}

			SignatureFunction->Next = OwnerClass->Children;
			OwnerClass->Children = SignatureFunction;

			SignatureFunction->Bind();
			SignatureFunction->StaticLink(true);
			
			FDelegateProperty* DelegateProp = new FDelegateProperty(Owner, AttrName, ObjectFlags);
			DelegateProp->SignatureFunction = SignatureFunction;

			NewProp = DelegateProp;
		}
	}

	if (!NewProp)
	{
		const UObject* Object = nullptr;
		ENePyObjectRefType ObjectRefType = ENePyObjectRefType::ObjectReference;

		if (PyType_Check(PyAttrVal))
		{
			// 形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.Actor)
			//     XXX = ue.uproperty(ue.Vector)
			//     XXX = ue.uproperty(ue.ECollisionChannel)
			// 这里起到一个“脱壳”的作用
			PyTypeObject* PyType = (PyTypeObject*)PyAttrVal;
			if (PyType_IsSubtype(PyType, NePyObjectBaseGetType()))
			{
				Object = FNePyWrapperTypeRegistry::Get().GetClassByPyType(PyType);
			}
			else if (PyType_IsSubtype(PyType, NePyStructBaseGetType()))
			{
				Object = FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyType);
			}
			else
			{
				Object = FNePyWrapperTypeRegistry::Get().GetEnumByPyType(PyType);
			}
		}
		else if (FNePyObjectRefDef* ObjectRefDef = FNePyObjectRefDef::Check(PyAttrVal))
		{
			// 形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.clsref(ue.SelfClass))
			//     XXX = ue.uproperty(ue.softobjref(ue.SelfClass))
			//     XXX = ue.uproperty(ue.softclsref(ue.SelfClass))
			if (ObjectRefDef->IsSelfClass)
			{
				Object = ThisClass;
			}
			// 形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.clsref(ue.Actor))
			//     XXX = ue.uproperty(ue.softobjref(ue.Actor))
			//     XXX = ue.uproperty(ue.softclsref(ue.Actor))
			else
			{
				Object = ObjectRefDef->Class;
			}
			ObjectRefType = ObjectRefDef->RefType;
		}
		else if (FNePyObjectBase* PyObjectAttr = NePyObjectBaseCheck(PyAttrVal))
		{
			Object = PyObjectAttr->Value;
		}
		else if (FNePySelfClassDef::Check(PyAttrVal))
		{
			// 形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.SelfClass)
			Object = ThisClass;
		}

		if (Object)
		{
			if (UClass* Class = const_cast<UClass*>(Cast<UClass>(Object)))
			{
				// 形如:
				// class MyActor(ue.Actor):
				//     XXX = ue.uproperty(ue.FloatProperty)
				if (FProperty* Prop = NePySubclassingUPropertyToFProperty(Class, Owner, AttrName, ObjectFlags, OwnerName))
				{
					NewProp = Prop;
				}
				// 形如:
				// class MyActor(ue.Actor):
				//     XXX = ue.uproperty(ue.Actor.Class())
				// 或形如:
				// class MyActor(ue.Actor):
				//     XXX = ue.uproperty(ue.FindClass('Actor'))
				else
				{
					switch (ObjectRefType)
					{
					case ENePyObjectRefType::ObjectReference:
					{
						FObjectProperty* ObjectProp = new FObjectProperty(Owner, AttrName, ObjectFlags);
						ObjectProp->SetPropertyClass(Class);
						NewProp = ObjectProp;
						break;
					}
					case ENePyObjectRefType::ClassReference:
					{
						FClassProperty* ClassProp = new FClassProperty(Owner, AttrName, ObjectFlags);
						ClassProp->SetPropertyClass(UClass::StaticClass());
						ClassProp->SetMetaClass(Class);
						NewProp = ClassProp;
						break;
					}
					case ENePyObjectRefType::SoftObjectReference:
					{
						FSoftObjectProperty* SoftObjectProp = new FSoftObjectProperty(Owner, AttrName, ObjectFlags);
						SoftObjectProp->SetPropertyClass(Class);
						NewProp = SoftObjectProp;
						break;
					}
					case ENePyObjectRefType::SoftClassReference:
					{
						FSoftClassProperty* SoftClassProp = new FSoftClassProperty(Owner, AttrName, ObjectFlags);
						SoftClassProp->SetPropertyClass(UClass::StaticClass());
						SoftClassProp->SetMetaClass(Class);
						NewProp = SoftClassProp;
						break;
					}
					case ENePyObjectRefType::WeakObjectReference:
					{
						FWeakObjectProperty* WeakObjectProp = new FWeakObjectProperty(Owner, AttrName, ObjectFlags);
						WeakObjectProp->SetPropertyClass(Class);
						NewProp = WeakObjectProp;
						break;
					}
					default:
						check(false);
						break;
					}
				}
			}
			// 形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.Vector.Struct())
			// 或形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.FindStruct('Vector'))
			else if (UScriptStruct* ScriptStruct = const_cast<UScriptStruct*>(Cast<UScriptStruct>(Object)))
			{
				FStructProperty* StructProp = new FStructProperty(Owner, AttrName, ObjectFlags);
				StructProp->Struct = ScriptStruct;
				if (!(PropFlags & CPF_Parm) && ScriptStruct->StructFlags & STRUCT_HasInstancedReference)
				{
					StructProp->SetPropertyFlags(CPF_ContainsInstancedReference);
				}
				NewProp = StructProp;
			}
			// 形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.ECollisionChannel.Enum())
			// 或形如:
			// class MyActor(ue.Actor):
			//     XXX = ue.uproperty(ue.FindEnum('ECollisionChannel'))
			else if (UEnum* Enum = const_cast<UEnum*>(Cast<UEnum>(Object)))
			{
				FEnumProperty* EnumProp = new FEnumProperty(Owner, AttrName, ObjectFlags);
				EnumProp->SetEnum(Enum);
				FProperty* UnderlyingProp = NePyNewEnumUnderlyingProperty(EnumProp, ObjectFlags);
				EnumProp->AddCppProperty(UnderlyingProp);
				NewProp = EnumProp;
			}
		}
	}

	if (NewProp)
	{
		NewProp->SetPropertyFlags(PropFlags);
		{
			// Need to manually call Link to fix-up some data (such as the C++ property flags and the set layout) that are only set during Link
			FArchive Ar;
			NewProp->LinkWithoutChangingOffset(Ar);
		}
	}
	else
	{
		PyTypeObject* PyType;
		if (PyType_Check(PyAttrVal))
		{
			PyType = (PyTypeObject*)PyAttrVal;
		}
		else
		{
			PyType = PyAttrVal->ob_type;
		}
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! Can't convert %s to FProperty. Owner is %s"), UTF8_TO_TCHAR(PyType->tp_name), *OwnerName);
	}

	return NewProp;
}

// 提取Python方法的参数名
// 参考：PyUtil::InspectFunctionArgs
bool NePySubclassingInspectFunctionArgs(UClass* NewClass, const FName& FuncName, PyObject* InFunc, EFunctionFlags InFunctionFlags, PyObject* InDisplayList, TArray<FName>& OutArgNames)
{
	FNePyObjectPtr PyInspectModule = NePyStealReference(PyImport_ImportModule("inspect"));
	if (!PyInspectModule)
	{
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! Cant import inspect module"));
		return false;
	}
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 5
	const char* GetArgSpecFuncName = "getfullargspec";
#else
	const char *GetArgSpecFuncName = "getargspec";
#endif
	PyObject* PyInspectDict = PyModule_GetDict(PyInspectModule);
	PyObject* PyGetArgSpecFunc = PyDict_GetItemString(PyInspectDict, GetArgSpecFuncName);
	if (!PyGetArgSpecFunc)
	{
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! Cant find inspect.%s()"), *FString(GetArgSpecFuncName));
		return false;
	}

	FNePyObjectPtr PyGetArgSpecResult = NePyStealReference(PyObject_CallFunctionObjArgs(PyGetArgSpecFunc, InFunc, nullptr));
	if (!PyGetArgSpecResult)
	{
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! Call inspect.%s() failed on function %s.%s"), *FString(GetArgSpecFuncName), *NewClass->GetName(), *FuncName.ToString());
		return false;
	}

	bool bIsStaticMethod = (InFunctionFlags & EFunctionFlags::FUNC_Static) != 0;
	PyObject* PyFuncArgNames = PyTuple_GetItem(PyGetArgSpecResult, 0);
	const int32 NumArgNames = (PyFuncArgNames && PyFuncArgNames != Py_None) ? PySequence_Size(PyFuncArgNames) : 0;
	if (NumArgNames == 0 && !bIsStaticMethod)
	{
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! %s.%s missing arg 'self'"), *NewClass->GetName(), *FuncName.ToString());
		return false;
	}

	OutArgNames.Reserve(NumArgNames);
	for (int32 ArgNameIndex = 0; ArgNameIndex < NumArgNames; ++ArgNameIndex)
	{
		PyObject* PyArgName = NULL;
		if (bIsStaticMethod)
		{
			PyArgName = PySequence_GetItem(InDisplayList, ArgNameIndex);
		}
		else if (ArgNameIndex > 0)
		{
			PyArgName = PySequence_GetItem(InDisplayList, ArgNameIndex - 1);
		}

		// 没有display名字，fallback到生成名字
		if (!PyArgName)
		{
			PyArgName = PySequence_GetItem(PyFuncArgNames, ArgNameIndex);
		}

		FName ArgName;
		NePyBase::ToCpp(PyArgName, ArgName);

		// 检查并跳过self
		if (ArgNameIndex == 0 && !bIsStaticMethod)
		{
			if (ArgName.ToString() != TEXT("self"))
			{
				UE_LOG(LogNePython, Error, TEXT("Subclassing error! Function %s missing arg 'self', Owner is %s"), *FuncName.ToString(), *NewClass->GetName());
				return false;
			}
		}
		else
		{
			OutArgNames.Add(ArgName);
		}
	}

	return true;
}

// 根据Python函数签名，创建一个新的UFunction
// 如果无法创建则返回nullptr
// 参考：FPythonGeneratedClassBuilder::CreateFunctionFromDefinition
UFunction* NePySubclassingNewFunction(UClass* NewClass, const FName& FuncName, PyFunctionObject* PyFunc, bool bStaticMethod=false)
{
	FNePyObjectPtr PyArgTypes = NePyStealReference(PyObject_GetAttrString((PyObject*)PyFunc, "bp_arg_types"));
	FNePyObjectPtr PyRetTypes = NePyStealReference(PyObject_GetAttrString((PyObject*)PyFunc, "bp_ret_types"));
	FNePyObjectPtr PyArgDisplayStrings = NePyStealReference(PyObject_GetAttrString((PyObject*)PyFunc, "bp_arg_display"));
	FNePyObjectPtr PyRetDisplayStrings = NePyStealReference(PyObject_GetAttrString((PyObject*)PyFunc, "bp_ret_display"));
	if (!PyArgTypes || !PyRetTypes)
	{
		// 此方法没有@bp_func标记，不是暴露给蓝图的方法
		return nullptr;
	}
	if (!PyTuple_Check(PyArgDisplayStrings) && !PyList_Check(PyArgDisplayStrings))
	{
		// arg的展示列表必须为此，即便是个空的
		return nullptr;
	}
	if (!PyTuple_Check(PyRetDisplayStrings) && !PyList_Check(PyRetDisplayStrings))
	{
		// ret的展示列表必须为此，即便是个空的
		return nullptr;
	}

	EFunctionFlags FunctionFlags(FUNC_None);
	if (bStaticMethod)
	{
		FunctionFlags |= FUNC_Static;
	}

	if (NewClass->FindFunctionByName(FuncName, EIncludeSuperFlag::ExcludeSuper))
	{
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! Function %s.%s is already registered"), *NewClass->GetName(), *FuncName.ToString());
		return nullptr;
	}

	UClass* SuperClass = NewClass->GetSuperClass();
	const UFunction* SuperFunc = SuperClass->FindFunctionByName(FuncName);
	if (SuperFunc && !SuperFunc->HasAnyFunctionFlags(FUNC_BlueprintEvent))
	{
		UE_LOG(LogNePython, Error, TEXT("Subclassing error! Cant override non blueprint event function %s.%s"), *NewClass->GetName(), *FuncName.ToString());
		return nullptr;
	}

	// 非覆写方法，提取参数名
	// 如果是覆写的方法则跳过这一步（使用原函数签名）
	TArray<FName> ArgNames;
	TArray<PyObject*> ArgTypes;
	TArray<PyObject*> RetTypes;
	if (!SuperFunc)
	{
		if (!NePySubclassingInspectFunctionArgs(NewClass, FuncName, (PyObject*)PyFunc, FunctionFlags, PyArgDisplayStrings, ArgNames))
		{
			return nullptr;
		}

		if (!PyTuple_Check(PyArgTypes) && !PyList_Check(PyArgTypes))
		{
			PyObject* PyArgTuple = PyTuple_New(1);
			PyTuple_SetItem(PyArgTuple, 0, PyArgTypes.Release()); // steals reference
			PyArgTypes.Get() = PyArgTuple;
		}

		if (!PyTuple_Check(PyRetTypes) && !PyList_Check(PyRetTypes))
		{
			PyObject* PyRetTuple = PyTuple_New(1);
			PyTuple_SetItem(PyRetTuple, 0, PyRetTypes.Release()); // steals reference
			PyRetTypes.Get() = PyRetTuple;
		}

		// 提取并检查参数类型
		int32 NumArgTypes = PySequence_Size(PyArgTypes);
		if (NumArgTypes != ArgNames.Num())
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Number of arg types incorrect (expected %d, got %d). Function is %s.%s"), ArgNames.Num(), NumArgTypes, *NewClass->GetName(), *FuncName.ToString());
			return nullptr;
		}
		for (int32 Index = 0; Index < NumArgTypes; ++Index)
		{
			ArgTypes.Add(PySequence_GetItem(PyArgTypes, Index));
		}

		// 提取并检查返回值类型
		int32 NumRetTypes = PySequence_Size(PyRetTypes);
		for (int32 Index = 0; Index < NumRetTypes; ++Index)
		{
			RetTypes.Add(PySequence_GetItem(PyRetTypes, Index));
		}
		if (NumRetTypes == 1 && RetTypes[0] == Py_None)
		{
			RetTypes.Empty();
		}
	}

	// Create the function, either from the definition, or from the super-function found to override
	NewClass->AddNativeFunction(*FuncName.ToString(), &UNePySubclass::CallPythonFunction); // Need to do this before the call to DuplicateObject in the case that the super-function already has FUNC_Native
	UFunction* NewFunc = nullptr;
	if (SuperFunc)
	{
		FObjectDuplicationParameters FuncDuplicationParams(const_cast<UFunction*>(SuperFunc), NewClass);
		FuncDuplicationParams.DestName = FuncName;
		// FuncDuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
		NewFunc = CastChecked<UFunction>(StaticDuplicateObjectEx(FuncDuplicationParams));
	}
	else
	{
		NewFunc = NewObject<UFunction>(NewClass, FuncName);
	}
	if (!SuperFunc)
	{
		NewFunc->FunctionFlags |= FUNC_Public;
		// The function is not in the linked list of class fields, insert it so that field iterators & funcs work
		NewFunc->Next = NewClass->Children;
		NewClass->Children = NewFunc;
	}
	NewFunc->FunctionFlags |= (FUNC_Native | FUNC_BlueprintCallable | FunctionFlags);
	NewClass->AddFunctionToFunctionMap(NewFunc, NewFunc->GetFName());

	// 为非覆写方法构造参数和返回值信息
	if (!SuperFunc)
	{
		{
			// Adding properties to a function inserts them into a linked list, so we loop backwards to get the order right
			check(ArgNames.Num() == ArgTypes.Num());
			int32 ArgIndex = ArgNames.Num() - 1;
			while (ArgIndex >= 0)
			{
				FProperty* ArgProp = NePySubclassingNewProperty(NewFunc, ArgNames[ArgIndex], ArgTypes[ArgIndex],
					NewClass->GetName() + TEXT(".") + FuncName.ToString(), CPF_Parm);
				if (!ArgProp)
				{
					return nullptr;
				}
				NewFunc->AddCppProperty(ArgProp);
				ArgIndex--;
			}
		}

		// Build the arguments struct if not overriding a function
		if (RetTypes.Num() > 0)
		{
			// 剩余的返回值通过引用参数返回
			// bug fix: AddCppProperty use inversed order, so we loop backwards to get iterating at the right order
			int32 ArgIndex = RetTypes.Num() - 1;
			while (ArgIndex >= 1)
			{
				FName AttrName = *FString::Printf(TEXT("OutValue%d"), ArgIndex);

				// 尝试获取display名字
				PyObject* PyDisplayName = PySequence_GetItem(PyRetDisplayStrings, ArgIndex);
				if (PyDisplayName)
				{
					NePyBase::ToCpp(PyDisplayName, AttrName);
				}

				FProperty* ArgProp = NePySubclassingNewProperty(NewFunc, AttrName, RetTypes[ArgIndex],
					NewClass->GetName() + TEXT(".") + FuncName.ToString(), CPF_Parm | CPF_OutParm);
				if (!ArgProp)
				{
					return nullptr;
				}
				NewFunc->AddCppProperty(ArgProp);
				NewFunc->FunctionFlags |= FUNC_HasOutParms;

				ArgIndex--;
			}
			
			// 将第一个返回值当做真正的返回值, 调换顺序，保证Return Val出现在第一个
			{
				FName AttrName = TEXT("ReturnValue");

				// 尝试获取display名字
				PyObject* PyDisplayName = PySequence_GetItem(PyRetDisplayStrings, 0);
				if (PyDisplayName)
				{
					NePyBase::ToCpp(PyDisplayName, AttrName);
				}

				FProperty* RetProp = NePySubclassingNewProperty(NewFunc, AttrName, RetTypes[0],
					NewClass->GetName() + TEXT(".") + FuncName.ToString(), CPF_Parm | CPF_ReturnParm);
				if (!RetProp)
				{
					return nullptr;
				}
				NewFunc->AddCppProperty(RetProp);
			}
		}
	}

	NewFunc->Bind();
	NewFunc->StaticLink(true);

	return NewFunc;
}

PyObject* NePySubclassing(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	if (PyTuple_Size(InArgs) != 3)
	{
		return nullptr;
	}

#if WITH_EDITOR
	if (!GetDefault<UNePythonSettings>()->bDisableOldStyleSubclassing)
	{
		FString DebugArg0 = UTF8_TO_TCHAR(NePyString_AsString(NePyStealReference(PyObject_Str(PyTuple_GetItem(InArgs, 0)))));
		FString DebugArg1 = UTF8_TO_TCHAR(NePyString_AsString(NePyStealReference(PyObject_Str(PyTuple_GetItem(InArgs, 1)))));
		FString DebugArg2 = UTF8_TO_TCHAR(NePyString_AsString(NePyStealReference(PyObject_Str(PyTuple_GetItem(InArgs, 2)))));
		UE_LOG(LogNePython, Log, TEXT("Begin Subclassing"));
		UE_LOG(LogNePython, Log, TEXT("    ClassName: %s"), *DebugArg0);
		UE_LOG(LogNePython, Log, TEXT("    Parents: %s"), *DebugArg1);
		UE_LOG(LogNePython, Log, TEXT("    ClassAttrs: %s"), *DebugArg2);
	}
#endif

	PyObject* PyClassAttrs = PyTuple_GetItem(InArgs, 2);
	FNePyObjectPtr PyClassName = NePyStealReference(PyObject_Str(PyTuple_GetItem(InArgs, 0)));

	const char* ClassNameStr;
	if (!NePyBase::ToCpp(PyClassName, ClassNameStr))
	{
		if (GetDefault<UNePythonSettings>()->bDisableOldStyleSubclassing)
		{
			UE_LOG(LogNePython, Error, TEXT("Generate unreal class failed! Can't retrieve class name."));
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing failed! Can't retrieve class name."));
		}
		return nullptr;
	}

	FString ClassName(UTF8_TO_TCHAR(ClassNameStr));
	PyObject* PyBases = PyTuple_GetItem(InArgs, 1);
	Py_ssize_t PyBasesCount = PyTuple_Size(PyBases);
	if (!PyTuple_Check(PyBases) || PyBasesCount < 1)
	{
		if (GetDefault<UNePythonSettings>()->bDisableOldStyleSubclassing)
		{
			UE_LOG(LogNePython, Error, TEXT("Generate unreal class failed! '%s' has no parents."), *ClassName);
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing failed! '%s' has no parents."), *ClassName);
		}
		return nullptr;
	}

	PyObject* PyBase = PyTuple_GetItem(PyBases, 0);
	UClass* SuperClass = NePyBase::ToCppClass(PyBase);
	if (!SuperClass)
	{
		if (GetDefault<UNePythonSettings>()->bDisableOldStyleSubclassing)
		{
			UE_LOG(LogNePython, Error, TEXT("Generate unreal class failed! First parent of '%s' should be a valid unreal class."), *ClassName);
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing failed! First parent of '%s' should be a valid UClass."), *ClassName);
		}
		return nullptr;
	}

	bool bDisableOldStyleSubclassing = GetDefault<UNePythonSettings>()->bDisableOldStyleSubclassing;
	if (!bDisableOldStyleSubclassing)
	{
		PyObject* PyDisableOldStyleSubclassing = PyDict_GetItemString(PyClassAttrs, "DISABLE_OLD_STYLE_SUBCLASSING");
		bDisableOldStyleSubclassing = PyDisableOldStyleSubclassing == Py_True;
	}

	// 禁用旧Subclassing机制，使用Python原生类继承构造方法构造
	if (bDisableOldStyleSubclassing)
	{
		auto BaseTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(SuperClass);
		if (!BaseTypeInfo)
		{
			UE_LOG(LogNePython, Error, TEXT("Generate unreal class failed! '%s'\'s Parent class '%s' don't have a python wrapper type."),
				*ClassName, *SuperClass->GetName());
			return nullptr;
		}

		PyTypeObject* PyBaseType = BaseTypeInfo->TypeObject;

		// todo: twx 现在只处理了第一个父类的UClass->PyTypeObject转换
		// 未来要支持python层使用UInterface，有可能会有多个基类需要UClass->PyTypeObject的转换
		FNePyObjectPtr NewBases = NePyStealReference(PyTuple_New(PyBasesCount));
		Py_INCREF(PyBaseType);
		PyTuple_SetItem(NewBases, 0, (PyObject*)PyBaseType);
		for (Py_ssize_t Index = 1; Index < PyBasesCount; ++Index)
		{
			PyObject* PyOtherBase = PyTuple_GetItem(PyBases, Index);
			Py_INCREF(PyOtherBase);
			PyTuple_SetItem(NewBases, Index, PyOtherBase);
		}

		FNePyObjectPtr NewArgs = NePyStealReference(PyTuple_New(3));
		PyTuple_SetItem(NewArgs, 0, PyClassName.Release());
		PyTuple_SetItem(NewArgs, 1, NewBases.Release());
		Py_INCREF(PyClassAttrs);
		PyTuple_SetItem(NewArgs, 2, PyClassAttrs);

		return PyType_Type.tp_new(Py_TYPE(PyBaseType), NewArgs, InKwds);
	}

	// hack: 修复运行Commandlet时，创建SubClassing类的CDO时，会走到“Attempt to process %s before it has been added.”，导致运行失败
	// 原始修复方案位于FUnrealEnginePythonModule::StartupModule()，由gzlihaonan@corp.netease.com提供
	struct FHackCommandletSubclassingFatalError
	{
		FHackCommandletSubclassingFatalError()
			: bOriginIsInitialLoad(GIsInitialLoad)
		{
			GIsInitialLoad = false;
		}
		~FHackCommandletSubclassingFatalError()
		{
			GIsInitialLoad = bOriginIsInitialLoad;
		}
	private:
		bool bOriginIsInitialLoad;
	};
	FHackCommandletSubclassingFatalError ScopedHackCommandletSubclassingFatalError;

	// 生成新的UClass
	UNePySubclass* NewClass = NePySubclassingNewClassBegin(ClassName, SuperClass);
	if (!NewClass)
	{
		return nullptr;
	}

	// 生成新的PySubclass，或者复用旧的（reload）
	FNePySubclass* PySubclass = NewClass->PyClass;
	PyTypeObject* PySubclassType;
	if (!PySubclass)
	{
		// 创建船新的SubclassPyTypeObject
		PySubclassType = NePyNewSubclassInstanceType(NewClass, SuperClass, PyClassAttrs);
		if (!PySubclassType)
		{
			return nullptr;
		}
		// 创建船新的PySubclass
		PySubclass = NePySubclassNewInternalUseOnly(NewClass, SuperClass);
		if (!PySubclass)
		{
			return nullptr;
		}
	}
	else
	{
		// 说明是Reload
		check(NewClass->PyClass == PySubclass);
		check(PySubclass->Value == NewClass);
		check(FNePyHouseKeeper::Get().IsValid(PySubclass));

		// clear the older
		const FNePyObjectTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedObjectType(NewClass);
		check(PyTypeInfo);
		FNePyWrapperTypeRegistry::Get().UnregisterWrappedClassType(NewClass);

		// 重新创建船新的SubclassPyTypeObject
		PySubclassType = NePyNewSubclassInstanceType(NewClass, SuperClass, PyClassAttrs);
		if (!PySubclassType)
		{
			return nullptr;
		}

		// 每次Reload都会减一次引用，这里要加一次引用
		Py_INCREF(PySubclass);

		// 清理__dict__
		FNePyObjectPtr PyClassDictIter = NePyStealReference(PyObject_GetIter(PySubclass->PyDict));
		TArray<const char*> DeletingKeys;
		while (true)
		{
			FNePyObjectPtr PyClassDictKey = NePyStealReference(PyIter_Next(PyClassDictIter));
			if (!PyClassDictKey)
			{
				break;
			}
			const char* KeyStr;
			if (!NePyBase::ToCpp(PyClassDictKey, KeyStr))
			{
				continue;
			}
			if (strlen(KeyStr) >= 2 && KeyStr[0] == '_' && KeyStr[1] == '_')
			{
				// 跳过内置成员
				continue;
			}
			DeletingKeys.Add(KeyStr);
		}
		for (const char* KeyStr : DeletingKeys)
		{
			PyDict_DelItemString(PySubclass->PyDict, KeyStr);
		}
		if (PyDict_GetItemString(PySubclass->PyDict, "__init__"))
		{
			PyDict_DelItemString(PySubclass->PyDict, "__init__");
		}

		// 现在 reload 会导致 PyType 重新生成，所以以下代码先注释掉看看
		// {
		// 	// PyType不会重新生成
		// 	const FNePyObjectTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(NewClass);
		// 	PyTypeObject* NePyType = PyTypeInfo->TypeObject;
		// 	TArray<FNePyObjectPtr> DeletingKeyPtrs;
		// 	PyObject* Key, * Value;
		// 	Py_ssize_t Pos = 0;
		// 	while (PyDict_Next(NePyType->tp_dict, &Pos, &Key, &Value))
		// 	{
		// 		// 清除旧的所有 DynamicDescriptor, @property, function, staticmethod
		// 		if (Py_TYPE(Value)->tp_descr_get)
		// 		{
		// 			DeletingKeyPtrs.Add(NePyNewReference(Key));
		// 		}
		// 	}
		// 	for (FNePyObjectPtr IterKey : DeletingKeyPtrs)
		// 	{
		// 		PyDict_DelItem(NePyType->tp_dict, IterKey);
		// 	}
		// 	PyType_Modified(NePyType);
		// }
	}

	check(FNePyWrapperTypeRegistry::Get().GetWrappedClassType(NewClass)->TypeObject == PySubclassType);

	// 根据Python提供的信息
	// 为新生成的UClass添加UFunction和UProperty
	FNePyObjectPtr PyClassAttrIter = NePyStealReference(PyObject_GetIter(PyClassAttrs));
	while (true)
	{
		FNePyObjectPtr PyAttrKey = NePyStealReference(PyIter_Next(PyClassAttrIter));
		if (!PyAttrKey)
		{
			break;
		}

		const char* AttrNameStr;
		if (!NePyBase::ToCpp(PyAttrKey, AttrNameStr))
		{
			continue;
		}

		if (strlen(AttrNameStr) >= 2 && AttrNameStr[0] == '_' && AttrNameStr[1] == '_')
		{
			// 忽略内置成员
			continue;
		}

		FName AttrName(UTF8_TO_TCHAR(AttrNameStr));
		PyObject* PyAttrVal = PyDict_GetItem(PyClassAttrs, PyAttrKey);

		// 是否需要将属性/方法加入Python类型的__dict__
		bool bAddToPyDict = true;

		if (FProperty* Property = NewClass->FindPropertyByName(AttrName))
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! Try to overwrite existing property '%s' (%s) on %s, this is forbidden."),
				*Property->GetName(), *Property->GetClass()->GetName(), *ClassName);
			bAddToPyDict = false;
		}
		else if (PyFunction_Check(PyAttrVal))
		{
			// 新增成员函数(UFUNCTION)
			// 由于是“类”而不是“实例”
			// 成员函数此时依旧是PyFunctionObject而不是PyMethodObject
			PyFunctionObject* PyFunc = (PyFunctionObject*)PyAttrVal;
			UFunction* FuncObj = NePySubclassingNewFunction(NewClass, AttrName, PyFunc);
			if (FuncObj)
			{
				FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewFunction(PySubclassType, FuncObj, AttrNameStr));
			}
			else
			{
				PyObject_SetAttr((PyObject*)PySubclassType, PyAttrKey, PyAttrVal);
			}
		}
		else if (Py_TYPE(PyAttrVal) == &PyStaticMethod_Type)
		{
			PyFunctionObject* PyFunc = (PyFunctionObject*)PyObject_GetAttrString(PyAttrVal, "__func__");
			UFunction* FuncObj = NePySubclassingNewFunction(NewClass, AttrName, PyFunc, true);
			if (FuncObj)
			{
				FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewFunction(PySubclassType, FuncObj, AttrNameStr));
			}
			else
			{
				PyObject_SetAttr((PyObject*)PySubclassType, PyAttrKey, PyAttrVal);
			}
			Py_XDECREF(PyFunc);
		}
		else
		{
			// 新增成员变量(uproperty)
			EPropertyFlags PropFlags = CPF_Edit | CPF_BlueprintVisible;
			FProperty* NewProp = NePySubclassingNewProperty(NewClass, AttrName, PyAttrVal, NewClass->GetName(), PropFlags);
			if (NewProp)
			{
				NewClass->AddCppProperty(NewProp);
				FNePyObjectPtr Descr = NePyStealReference(NePyType_AddNewProperty(PySubclassType, NewProp, AttrNameStr));
				bAddToPyDict = false;
			}
			// 其他情况没办法了，subclassing原来的设计就是不支持。
		}

		if (bAddToPyDict)
		{
			// 仍然加入到 USubClass 的 PyObject 中，保持接口不变
			PyDict_SetItem(PySubclass->PyDict, PyAttrKey, PyAttrVal);
		}
	}

	if (PyObject* PyInitFunc = PyDict_GetItemString(PyClassAttrs, "__init__"))
	{
		if (PyFunction_Check(PyInitFunc))
		{
			PyDict_SetItemString(PySubclass->PyDict, "__init__", PyInitFunc);
		}
		else
		{
			UE_LOG(LogNePython, Error, TEXT("Subclassing error! '%s.__init__' must be function, now is %s"), *ClassName, UTF8_TO_TCHAR(PyInitFunc->ob_type->tp_name));
		}
	}

	if (PyErr_Occurred())
	{
		PyErr_Clear();
	}

	NePySubclassingNewClassFinish(NewClass, SuperClass);

	return PySubclass;
}

