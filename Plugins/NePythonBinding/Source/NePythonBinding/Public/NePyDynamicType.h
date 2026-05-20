#pragma once

#include "NePyIncludePython.h"
#include "CoreMinimal.h"
#include "NePyGenUtil.h"

struct FNePyObjectTypeInfo;
struct FNePyStructBase;

// 为动态生成的类所需要的常量提供存储空间
struct FNePyDynamicType
{
	// PyTypeObject 使用的内存空间
	PyTypeObject TypeObject;

	// tp_name 使用的内存空间
	NePyGenUtil::FUTF8Buffer TypeName;

public:
	FNePyDynamicType();
	~FNePyDynamicType() = default;

	FNePyDynamicType(FNePyDynamicType&&) = default;
	FNePyDynamicType(const FNePyDynamicType&) = delete;
	FNePyDynamicType& operator=(FNePyDynamicType&&) = default;
	FNePyDynamicType& operator=(const FNePyDynamicType&) = delete;

	// 获取PyTypeObject*
	PyTypeObject* GetPyType();

	// 获取类型名称
	const char* GetName() const;
};

struct FNePyDynamicClassType : FNePyDynamicType
{
public:
	// 根据UClass初始化动态类型，与静态导出类的InitPyType不太一样
	void InitDynamicClassType(const UClass* InClass, const FNePyObjectTypeInfo* SuperTypeInfo, PyTypeObject* InPyBase, PyObject* InPyBases);

	static PyObject* Class(PyObject* InSelf);
};

struct FNePyDynamicStructType : FNePyDynamicType
{
public:
	void InitDynamicStructType(const UScriptStruct* InScriptStruct);

	// tp_alloc
	static PyObject* Alloc(PyTypeObject* PyType, Py_ssize_t NItems);
	// tp_dealloc
	static void Dealloc(FNePyStructBase* InSelf);
	// tp_new
	static FNePyDynamicStructType* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);
	// tp_init
	static int Init(FNePyStructBase* InSelf, PyObject* InArgs, PyObject* InKwds);

	static bool PropSet(const FStructProperty* InStructProp, FNePyStructBase* InPyObj, void* InBuffer);
	static FNePyStructBase* PropGet(const FStructProperty* InStructProp, void* InBuffer);

	static PyObject* Struct(PyObject* InSelf);
};

struct FNePyDynamicEnumType : FNePyDynamicType
{
public:
	void InitDynamicEnumType(const UEnum* InEnum);
};
