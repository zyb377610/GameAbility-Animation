#include "NePyWrapperInitializer.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyObjectBase.h"
#include "NePyStructBase.h"
#include "NePyTopModule.h"
#include "Algo/Reverse.h"
#include "UObject/Class.h"

FNePyWrapperInitializer& FNePyWrapperInitializer::Get()
{
	static FNePyWrapperInitializer Instance;
	return Instance;
}

void FNePyWrapperInitializer::RegisterWrappedClassInitFunc(const UClass* InClass, NePyWrappedClassInitFunc InInitFunc)
{
	check(!ClassInitFuncs.Contains(InClass));
	ClassInitFuncs.Add(InClass, InInitFunc);
}

void FNePyWrapperInitializer::RegisterWrappedStructInitFunc(const UScriptStruct* InStruct, NePyWrappedStructInitFunc InInitFunc)
{
	check(!StructInitFuncs.Contains(InStruct));
	StructInitFuncs.Add(InStruct, InInitFunc);
}

void FNePyWrapperInitializer::GenerateWrappedTypes()
{
	TArray<const UClass*> NeedGenerateClasses;
	ClassInitFuncs.GetKeys(NeedGenerateClasses);

	TArray<const UScriptStruct*> NeedGenerateStructs;
	StructInitFuncs.GetKeys(NeedGenerateStructs);

	for (const UClass* Class : NeedGenerateClasses)
	{
		GenerateWrappedClassType(Class);
	}

	for (const UScriptStruct* Struct : NeedGenerateStructs)
	{
		GenerateWrappedStructType(Struct);
	}

	ClassInitFuncs.Empty();
	StructInitFuncs.Empty();
}

void FNePyWrapperInitializer::GenerateWrappedClassType(const UClass* InClass)
{
	FNePyWrapperTypeRegistry& TypeRegistry = FNePyWrapperTypeRegistry::Get();
	if (TypeRegistry.HasWrappedClassType(InClass))
	{
		// 之前已生成过了
		return;
	}

	// 递归查找所有尚未生成PyType的UE基类
	TArray<const UClass*, TInlineAllocator<16>> ClassWithoutPyTypes;
	PyTypeObject* PyBaseType = nullptr;
	ClassWithoutPyTypes.Add(InClass);
	while (const UClass* SuperClass = ClassWithoutPyTypes.Last()->GetSuperClass())
	{
		auto PyBaseTypeInfo = TypeRegistry.GetWrappedClassTypeIfExist(SuperClass);
		if (PyBaseTypeInfo)
		{
			PyBaseType = PyBaseTypeInfo->TypeObject;
			check(PyBaseType);
			break;
		}

		ClassWithoutPyTypes.Add(SuperClass);
	}

	if (!PyBaseType)
	{
		PyBaseType = NePyObjectBaseGetType();
	}

	// 按照继承顺序为所有类生成PyType
	Algo::Reverse(ClassWithoutPyTypes);
	for (const UClass* Class : ClassWithoutPyTypes)
	{
		PyBaseType = DoGenerateWrappedClassType(Class, PyBaseType);
		check(PyBaseType);
	}
}

PyTypeObject* FNePyWrapperInitializer::DoGenerateWrappedClassType(const UClass* InClass, PyTypeObject* InPyBase)
{
	check(InClass);
	check(InPyBase);
	FNePyWrapperTypeRegistry& TypeRegistry = FNePyWrapperTypeRegistry::Get();
	if (const FNePyObjectTypeInfo* TypeInfo = TypeRegistry.GetWrappedClassTypeIfExist(InClass))
	{
		// 之前已生成过了
		return TypeInfo->TypeObject;
	}

	for (const FImplementedInterface& Interface : InClass->Interfaces)
	{
		// 确保所有的接口类型都已经生成完毕
		GenerateWrappedClassType(Interface.Class);
	}

	PyTypeObject* NewPyType = nullptr;
	NePyWrappedClassInitFunc* InitFuncPtr = ClassInitFuncs.Find(InClass);
	if (InitFuncPtr)
	{
		PyObject* PyBases = nullptr;
		if (InClass->Interfaces.Num() > 0)
		{
			PyBases = PyTuple_New(InClass->Interfaces.Num() + 1);
			PyTuple_SetItem(PyBases, 0, (PyObject*)NePyNewReference(InPyBase).Release());
			for (int32 Index = 0; Index < InClass->Interfaces.Num(); ++Index)
			{
				const FImplementedInterface& Interface = InClass->Interfaces[Index];
				const FNePyObjectTypeInfo* InterfaceTypeInfo = TypeRegistry.GetWrappedClassTypeIfExist(Interface.Class);

				check(InterfaceTypeInfo && InterfaceTypeInfo->TypeObject);
				PyTuple_SetItem(PyBases, Index + 1, (PyObject*)NePyNewReference(InterfaceTypeInfo->TypeObject).Release());
			}
		}
		
		// 静态绑定
		NewPyType = (*InitFuncPtr)(InClass, InPyBase, PyBases);
		check(NewPyType);
	}
	else
	{
		// 动态绑定
		auto TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(InClass);
		check(TypeInfo);
		check(TypeInfo->TypeObject);
		check(EnumHasAnyFlags(TypeInfo->TypeFlags, ENePyTypeFlags::DynamicPyType));
		NewPyType = TypeInfo->TypeObject;
	}

	NePyAddTypeToTopModule(NewPyType);
	return NewPyType;
}

void FNePyWrapperInitializer::GenerateWrappedStructType(const UScriptStruct* InStruct)
{
	FNePyWrapperTypeRegistry& TypeRegistry = FNePyWrapperTypeRegistry::Get();
	if (TypeRegistry.HasWrappedStructType(InStruct))
	{
		// 之前已生成过了
		return;
	}

	// 递归查找所有尚未生成PyType的UE基类
	TArray<const UScriptStruct*, TInlineAllocator<16>> StructWithoutPyTypes;
	PyTypeObject* PyBaseType = nullptr;
	StructWithoutPyTypes.Add(InStruct);
	while (const UScriptStruct* SuperStruct = Cast<UScriptStruct>(StructWithoutPyTypes.Last()->GetSuperStruct()))
	{
		auto PyBaseTypeInfo = TypeRegistry.GetWrappedStructTypeIfExist(SuperStruct);
		if (PyBaseTypeInfo)
		{
			PyBaseType = PyBaseTypeInfo->TypeObject;
			check(PyBaseType);
			break;
		}

		StructWithoutPyTypes.Add(SuperStruct);
	}

	if (!PyBaseType)
	{
		PyBaseType = NePyStructBaseGetType();
	}

	// 按照继承顺序为所有类生成PyType
	Algo::Reverse(StructWithoutPyTypes);
	for (const UScriptStruct* Struct : StructWithoutPyTypes)
	{
		PyBaseType = DoGenerateWrappedStructType(Struct, PyBaseType);
		check(PyBaseType);
	}
}

PyTypeObject* FNePyWrapperInitializer::DoGenerateWrappedStructType(const UScriptStruct* InStruct, PyTypeObject* InPyBase)
{
	check(InStruct);
	check(InPyBase);

	PyTypeObject* NewPyType = nullptr;
	NePyWrappedStructInitFunc* InitFuncPtr = StructInitFuncs.Find(InStruct);
	if (InitFuncPtr)
	{
		// 静态绑定
		NewPyType = (*InitFuncPtr)(InStruct, InPyBase);
		check(NewPyType);
	}
	else
	{
		// 动态绑定
		auto TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(InStruct);
		check(TypeInfo);
		check(TypeInfo->TypeObject);
		check(EnumHasAnyFlags(TypeInfo->TypeFlags, ENePyTypeFlags::DynamicPyType));
		NewPyType = TypeInfo->TypeObject;
	}

	NePyAddTypeToTopModule(NewPyType);
	return NewPyType;
}
