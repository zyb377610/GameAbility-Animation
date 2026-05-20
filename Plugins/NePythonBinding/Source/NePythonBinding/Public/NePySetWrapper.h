#pragma once
#include "NePyBase.h"
#include "UObject/UnrealType.h"
#include "NePyHouseKeeper.h"

// 为UObject的集合成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePySetWrapper : FNePyPropObject
{
	// 集合属性定义
	const FSetProperty* Prop;

	// 初始化集合PyType类型
	static void InitPyType();

	// 为UObject的集合属性生成Python对象
	static FNePySetWrapper* New(UObject* InObject, void* InMemberPtr, const char* PropName);

	// 为UObject的集合属性生成Python对象
	static FNePySetWrapper* New(UObject* InObject, void* InMemberPtr, const FSetProperty* InSetProp);

	// 检查输入对象是否为FNePySetWrapper类型，是则返回对象本身，否则返回空
	static FNePySetWrapper* Check(PyObject* InPyObj);

	// 为此集合赋值
	static bool Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为此集合赋值
	static bool Assign(PyObject* InOther, const FSetProperty* InProp, void* InDest, UObject* InOwnerObject);

	// 将自身转化为Python集合
	static PyObject* ToPySet(FNePySetWrapper* InSelf);

	// 将属性转化为Python集合
	static PyObject* ToPySet(const FSetProperty* InProp, const void* InValue, UObject* InOwnerObject);

	// 获取此对象的 Owner UObject
	static UObject* GetOwnerObject(FNePySetWrapper* InSelf);

private:
	// tp_dealloc
	static void Dealloc(FNePySetWrapper* InSelf);

	// tp_init
	static int Init(FNePySetWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePySetWrapper* InSelf);

	// tp_iter
	static PyObject* Iter(FNePySetWrapper* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePySetWrapper* InSelf, PyObject* InOther, int InOp);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePySetWrapper* InSelf);

	/** Make a shallow copy of this set */
	static PyObject* Copy(FNePySetWrapper* InSelf);

	/** Add the given value to this container (equivalent to 'x.add(v)' in Python) */
	static PyObject* Add(FNePySetWrapper* InSelf, PyObject* InValue);

	/** Remove the given value from this container, doing nothing if it's not present (equivalent to 'x.discard(v)' in Python) */
	static PyObject* Discard(FNePySetWrapper* InSelf, PyObject* InValue);

	/** Remove the given value from this container (equivalent to 'x.remove(v)' in Python) */
	static PyObject* Remove(FNePySetWrapper* InSelf, PyObject* InValue);

	/** Remove and return an arbitrary value from this container (equivalent to 'x.pop()' in Python, returns new reference) */
	static PyObject* Pop(FNePySetWrapper* InSelf);

	/** Remove all values from this container (equivalent to 'x.clear()' in Python) */
	static PyObject* Clear(FNePySetWrapper* InSelf);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FNePySetWrapper* InSelf);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FNePySetWrapper* InSelf, PyObject* InValue);
};

// 为UStruct的集合成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyStructSetWrapper : FNePySetWrapper
{
	// 对父结构的反向引用
	// UStruct不使用HouseKeeper管理生命周期。通过增加对父结构体的引用计数，防止父结构体被释放。
	PyObject* PyOuter;

	// 为UStruct的集合属性生成Python对象
	static FNePyStructSetWrapper* New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为UStruct的集合属性生成Python对象
	static FNePyStructSetWrapper* New(PyObject* PyOuter, void* InMemberPtr, const FSetProperty* InSetProp);

private:
	// tp_dealloc
	static void Dealloc(FNePyStructSetWrapper* InSelf);

	friend struct FNePySetWrapper;
};

/** Iterator used with map keys */
struct FNePySetWrapperKeyIterator : public PyObject
{
private:
	/** Instance being iterated over */
	FNePySetWrapper* IterInstance;

	/** Current iteration index */
	int32 IterIndex;

	//
	static FNePySetWrapperKeyIterator* New(FNePySetWrapper* IterInstance);

	// tp_dealloc
	static void Dealloc(FNePySetWrapperKeyIterator* InSelf);

	// tp_init
	static int Init(FNePySetWrapperKeyIterator* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_iter
	static PyObject* Iter(FNePySetWrapperKeyIterator* InSelf);

	// tp_iternext
	static PyObject* IterNext(FNePySetWrapperKeyIterator* InSelf);

	/** Called to validate the internal state of this iterator instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FNePySetWrapperKeyIterator* InSelf);

	/** Given a sparse index, get the first element index from this point in the map (including the given index) */
	static int32 GetElementIndex(FNePySetWrapperKeyIterator* InSelf, int32 InSparseIndex);

	friend struct FNePySetWrapper;
};
