#pragma once
#include "NePyBase.h"
#include "UObject/UnrealType.h"
#include "NePyHouseKeeper.h"

// 为UObject的静态数组成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyFixedArrayWrapper : FNePyPropObject
{
	// 静态数组属性定义
	const FProperty* Prop;

	// 初始化静态数组PyType类型
	static void InitPyType();

	// 为UObject的静态数组属性生成Python对象
	static FNePyFixedArrayWrapper* New(UObject* InObject, void* InMemberPtr, const char* PropName);

	// 为UObject的静态数组属性生成Python对象
	static FNePyFixedArrayWrapper* New(UObject* InObject, void* InMemberPtr, const FProperty* InProp);

	// 检查输入对象是否为FNePyFixedArrayWrapper类型，是则返回对象本身，否则返回空
	static FNePyFixedArrayWrapper* Check(PyObject* InPyObj);

	// 为此数组赋值
	static bool Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为此数组赋值
	static bool Assign(PyObject* InOther, const FProperty* InProp, void* InDest, UObject* InOwnerObject);

	// 将自身转化为Python列表
	static PyObject* ToPyList(FNePyFixedArrayWrapper* InSelf);

	// 将属性转化为Python列表
	static PyObject* ToPyList(const FProperty* InProp, const void* InValue, UObject* InOwnerObject);

	// 获取此对象的 Owner UObject
	static UObject* GetOwnerObject(FNePyFixedArrayWrapper* InSelf);

private:
	// tp_dealloc
	static void Dealloc(FNePyFixedArrayWrapper* InSelf);

	// tp_init
	static int Init(FNePyFixedArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyFixedArrayWrapper* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePyFixedArrayWrapper* InSelf, PyObject* InOther, int InOp);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePyFixedArrayWrapper* InSelf);

	/** Make a shallow copy of this list */
	static PyObject* Copy(FNePyFixedArrayWrapper* InSelf);

	/** Count the number of times that the given value appears in this container (equivalent to 'x.count(v)' in Python) */
	static PyObject* Count(FNePyFixedArrayWrapper* InSelf, PyObject* InValue);

	/** Get the index of the first the given value appears in this container (equivalent to 'x.index(v)' in Python) */
	static PyObject* Index(FNePyFixedArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Get the raw pointer to the element at index N (negative indexing not supported) */
	static void* GetItemPtr(const FProperty* InProp, void* InValue, Py_ssize_t InIndex);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FNePyFixedArrayWrapper* InSelf);

	/** Get the item at index N (equivalent to 'x[N]' in Python, returns new reference) */
	static PyObject* GetItem(FNePyFixedArrayWrapper* InSelf, Py_ssize_t InIndex);

	/** Set the item at index N (equivalent to 'x[N] = v' in Python) */
	static int SetItem(FNePyFixedArrayWrapper* InSelf, Py_ssize_t InIndex, PyObject* InValue);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FNePyFixedArrayWrapper* InSelf, PyObject* InValue);

	/** Concatenate the other object to this one, returning a new container (equivalent to 'x + o' in Python, returns new reference) */
	static PyObject* Concat(FNePyFixedArrayWrapper* InSelf, PyObject* InOther);
};

// 为UStruct的静态数组成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyStructFixedArrayWrapper : FNePyFixedArrayWrapper
{
	// 对父结构的反向引用
	// UStruct不使用HouseKeeper管理生命周期。通过增加对父结构体的引用计数，防止父结构体被释放。
	PyObject* PyOuter;

	// 为UStruct的静态数组属性生成Python对象
	static FNePyStructFixedArrayWrapper* New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为UStruct的静态数组属性生成Python对象
	static FNePyStructFixedArrayWrapper* New(PyObject* PyOuter, void* InMemberPtr, const FProperty* InProp);

private:
	// tp_dealloc
	static void Dealloc(FNePyStructFixedArrayWrapper* InSelf);

	friend struct FNePyFixedArrayWrapper;
};
