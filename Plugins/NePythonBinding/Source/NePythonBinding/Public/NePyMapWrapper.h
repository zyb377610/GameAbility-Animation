#pragma once
#include "NePyBase.h"
#include "UObject/UnrealType.h"
#include "NePyHouseKeeper.h"

// 为UObject的字典成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyMapWrapper : FNePyPropObject
{
	// 字典属性定义
	const FMapProperty* Prop;

	// 初始化字典PyType类型
	static void InitPyType();

	// 为UObject的字典属性生成Python对象
	static FNePyMapWrapper* New(UObject* InObject, void* InMemberPtr, const char* PropName);

	// 为UObject的字典属性生成Python对象
	static FNePyMapWrapper* New(UObject* InObject, void* InMemberPtr, const FMapProperty* InMapProp);

	// 检查输入对象是否为FNePyMapWrapper类型，是则返回对象本身，否则返回空
	static FNePyMapWrapper* Check(PyObject* InPyObj);

	// 为此字典赋值
	static bool Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为此字典赋值
	static bool Assign(PyObject* InOther, const FMapProperty* InProp, void* InDest, UObject* InOwnerObject);

	// 将自身转化为Python字典
	static PyObject* ToPyDict(FNePyMapWrapper* InSelf);

	// 将属性转化为Python字典
	static PyObject* ToPyDict(const FMapProperty* InProp, const void* InValue, UObject* InOwnerObject);

	// 获取此对象的 Owner UObject
	static UObject* GetOwnerObject(FNePyMapWrapper* InSelf);

private:
	// tp_dealloc
	static void Dealloc(FNePyMapWrapper* InSelf);

	// tp_init
	static int Init(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyMapWrapper* InSelf);

	// tp_iter
	static PyObject* Iter(FNePyMapWrapper* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePyMapWrapper* InSelf, PyObject* InOther, int InOp);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePyMapWrapper* InSelf);

	/** Make a shallow copy of this map */
	static PyObject* Copy(FNePyMapWrapper* InSelf);

	/** Remove all values from this container (equivalent to 'x.clear()' in Python) */
	static PyObject* Clear(FNePyMapWrapper* InSelf);

	/** Get the item with key K (equivalent to 'x.get(K)' in Python) */
	static PyObject* Get(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Get a Python list containing the items from this map as key->value pairs */
	static PyObject* Items(FNePyMapWrapper* InSelf);

	/** Get a Python list containing the keys from this map */
	static PyObject* Keys(FNePyMapWrapper* InSelf);

	/** Get a Python list containing the values from this map */
	static PyObject* Values(FNePyMapWrapper* InSelf);

	/** Remove and return the value for key K if present, otherwise return the default, or raise KeyError if no default is given (equivalent to 'x.popitem()' in Python, returns new reference) */
	static PyObject* Pop(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Remove and return an arbitrary pair from this map (equivalent to 'x.popitem()' in Python, returns new reference) */
	static PyObject* PopItem(FNePyMapWrapper* InSelf);

	/** Set the item with key K if K isn't in the map and return the value of K (equivalent to 'x.setdefault(K, v)' in Python) */
	static PyObject* SetDefault(FNePyMapWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Update this map from another (equivalent to 'x.update(o)' in Python) */
	static PyObject* Update(FNePyMapWrapper* InSelf, PyObject* InOther);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FNePyMapWrapper* InSelf, PyObject* InKey);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FNePyMapWrapper* InSelf);

	/** Get the item with key K (equivalent to 'x[K]' in Python, returns new reference) */
	static PyObject* GetItem(FNePyMapWrapper* InSelf, PyObject* InKey);

	/** Get the item with key K (equivalent to 'x.get(K, D)' in Python, returns new reference) */
	static PyObject* DoGetItem(FNePyMapWrapper* InSelf, PyObject* InKey, PyObject* InDefault, bool bRemove = false);

	/** Set the item with key K (equivalent to 'x[K] = v' in Python) */
	static int SetItem(FNePyMapWrapper* InSelf, PyObject* InKey, PyObject* InValue);
};

// 为UStruct的字典成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyStructMapWrapper : FNePyMapWrapper
{
	// 对父结构的反向引用
	// UStruct不使用HouseKeeper管理生命周期。通过增加对父结构体的引用计数，防止父结构体被释放。
	PyObject* PyOuter;

	// 为UStruct的字典属性生成Python对象
	static FNePyStructMapWrapper* New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为UStruct的字典属性生成Python对象
	static FNePyStructMapWrapper* New(PyObject* PyOuter, void* InMemberPtr, const FMapProperty* InMapProp);

private:
	// tp_dealloc
	static void Dealloc(FNePyStructMapWrapper* InSelf);

	friend struct FNePyMapWrapper;
};

/** Iterator used with map keys */
struct FNePyMapWrapperKeyIterator : public PyObject
{
private:
	/** Instance being iterated over */
	FNePyMapWrapper* IterInstance;

	/** Current iteration index */
	int32 IterIndex;

	//
	static FNePyMapWrapperKeyIterator* New(FNePyMapWrapper* IterInstance);

	// tp_dealloc
	static void Dealloc(FNePyMapWrapperKeyIterator* InSelf);

	// tp_init
	static int Init(FNePyMapWrapperKeyIterator* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_iter
	static PyObject* Iter(FNePyMapWrapperKeyIterator* InSelf);

	// tp_iternext
	static PyObject* IterNext(FNePyMapWrapperKeyIterator* InSelf);

	/** Called to validate the internal state of this iterator instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FNePyMapWrapperKeyIterator* InSelf);

	/** Given a sparse index, get the first element index from this point in the map (including the given index) */
	static int32 GetElementIndex(FNePyMapWrapperKeyIterator* InSelf, int32 InSparseIndex);

	friend struct FNePyMapWrapper;
};
