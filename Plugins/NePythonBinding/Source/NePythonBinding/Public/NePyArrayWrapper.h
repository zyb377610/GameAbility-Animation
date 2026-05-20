#pragma once
#include "NePyBase.h"
#include "UObject/UnrealType.h"
#include "NePyHouseKeeper.h"

// 为UObject的数组成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyArrayWrapper : FNePyPropObject
{
	// 数组属性定义
	const FArrayProperty* Prop;

	// 是否是对象属性
	bool IsObjectProperty;

	// 为了新建数组，自己创建的Value
	bool IsSelfCreateValue;

	// 初始化数组PyType类型
	static void InitPyType(PyObject* PyOuterModule);

	// === 供属性提供的旧接口 ===
	// 1.为UObject的数组属性生成Python对象
	static FNePyArrayWrapper* New(UObject* InObject, void* InMemberPtr, const char* PropName);

	// 2.为UObject的数组属性生成Python对象
	static FNePyArrayWrapper* New(UObject* InObject, void* InMemberPtr, const FArrayProperty* InProp);

	// === 供引用改造新增的接口 ===
	// 3.为蓝图虚拟机变量引用生成Python对象
	static FNePyArrayWrapper* NewReference(void* InMemberPtr, const FArrayProperty* InProp);

	// 4.为蓝图虚拟机变量拷贝生成Python对象
	static FNePyArrayWrapper* NewCopy(void* InMemberPtr, const FArrayProperty* InProp);

	// === 供协议函数实现新增的接口 ===
	// 5.为协议函数生成的数组生成Python对象
	static FNePyArrayWrapper* NewImplicit(const FArrayProperty* InProp);

	// 检查输入对象是否为FNePyArrayWrapper类型，是则返回对象本身，否则返回空
	static FNePyArrayWrapper* Check(PyObject* InPyObj);

	// 为此数组赋值
	static bool Assign(PyObject* InOther, UStruct* InClass, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为此数组赋值
	static bool Assign(PyObject* InOther, const FArrayProperty* InProp, void* InDest, UObject* InOwnerObject);

	// 将自身转化为Python列表
	static PyObject* ToPyList(FNePyArrayWrapper* InSelf);

	// 清理自身的Value
	static void ClenupValue(FNePyArrayWrapper* InSelf);

	// 将属性转化为Python列表
	static PyObject* ToPyList(const FArrayProperty* InProp, const void* InValue, UObject* InOwnerObject);

	// 将Python可迭代对象转化为List
	static FNePyArrayWrapper* FromPyIterable(const FArrayProperty* InProp, PyObject* Object);

	// 获取此对象的 Owner UObject
	static UObject* GetOwnerObject(FNePyArrayWrapper* InSelf);

private:
	// tp_new
	static FNePyArrayWrapper* NewIntenral(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	// tp_dealloc
	static void Dealloc(FNePyArrayWrapper* InSelf);

	// tp_init
	static int Init(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyArrayWrapper* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePyArrayWrapper* InSelf, PyObject* InOther, int InOp);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePyArrayWrapper* InSelf);
	
	/** Make a shallow copy of this list */
	static PyObject* Copy(FNePyArrayWrapper* InSelf);

	/** Append the given value to the end this container (equivalent to 'x.append(v)' in Python) */
	static PyObject* Append(FNePyArrayWrapper* InSelf, PyObject* InValue);

	/** Count the number of times that the given value appears in this container (equivalent to 'x.count(v)' in Python) */
	static PyObject* Count(FNePyArrayWrapper* InSelf, PyObject* InValue);

	/** extend this Unreal array by appending elements from the given iterable (equivalent to TArray::Append in C++) */
	static PyObject* Extend(FNePyArrayWrapper* InSelf, PyObject* InValue);

	/** Get the index of the first the given value appears in this container (equivalent to 'x.index(v)' in Python) */
	static PyObject* Index(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Insert the given value into this container at the given index (equivalent to 'x.insert(i, v)' in Python) */
	static PyObject* Insert(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Pop and return the value at the given index (or the end) of this container (equivalent to 'x.pop()' in Python) */
	static PyObject* Pop(FNePyArrayWrapper* InSelf, PyObject* InArgs);

	/** Remove the given value from this container (equivalent to 'x.remove(v)' in Python) */
	static PyObject* Remove(FNePyArrayWrapper* InSelf, PyObject* InValue);

	/** Reverse this container (equivalent to 'x.reverse()' in Python) */
	static PyObject* Reverse(FNePyArrayWrapper* InSelf);

	/** Sort this container (equivalent to 'x.sort()' in Python) */
	static PyObject* Sort(FNePyArrayWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Clear all values in this container (equivalent to 'del x[:]' in Python */
	static PyObject* Clear(FNePyArrayWrapper* InSelf);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FNePyArrayWrapper* InSelf);

	/** Get the item at index N (equivalent to 'x[N]' in Python, returns new reference) */
	static PyObject* GetItem(FNePyArrayWrapper* InSelf, Py_ssize_t InIndex);

	/** Delete the item at index N (equivalent to 'del x[N]' in Python) */
	static int DeleteItem(FNePyArrayWrapper* InSelf, Py_ssize_t InIndex);

	/** Set the item at index N (equivalent to 'x[N] = v' in Python) */
	static int SetItem(FNePyArrayWrapper* InSelf, Py_ssize_t InIndex, PyObject* InValue);

	/** Get a slice of this container (equivalent to 'x[i:j]' in Python, returns new reference) */
	static PyObject* GetSlice(FNePyArrayWrapper* InSelf, PyObject* SliceObj);

	/** Set a slice of this container (equivalent to 'x[i:j] = v' in Python) */
	static int SetSlice(FNePyArrayWrapper* InSelf, PyObject* SliceObj, PyObject* Value);

	/** Set a simple slice of this container (equivalent to 'x[i:j] = v' in Python) */
	static int SetSimpleSlice(FNePyArrayWrapper* InSelf, Py_ssize_t Start, Py_ssize_t Stop, PyObject* Value, Py_ssize_t ValueLength);

	/** Set an extended slice of this container (equivalent to 'x[i:j:step] = v' in Python) */
	static int SetExtendedSlice(FNePyArrayWrapper* InSelf, Py_ssize_t Start, Py_ssize_t Step, Py_ssize_t SliceLength, PyObject* Value);

	/* Delete a slice of this container (equivalent to 'del x[i:j]' in Python) */
	static int DeleteSlice(FNePyArrayWrapper* InSelf, PyObject* SliceObj);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FNePyArrayWrapper* InSelf, PyObject* InValue);

	/** Concatenate the other object to this one, returning a new container (equivalent to 'x + o' in Python, returns new reference) */
	static PyObject* Concat(FNePyArrayWrapper* InSelf, PyObject* InOther);

	/** Concatenate the other object to this one in-place (equivalent to 'x += o' in Python) */
	static PyObject* ConcatInplace(FNePyArrayWrapper* InSelf, PyObject* InOther);

	/** Repeat this container the given number of times, returning a new container (equivalent to 'x * n' in Python, returns new reference) */
	static PyObject* Repeat(FNePyArrayWrapper* InSelf, Py_ssize_t InTimes);

	/** Repeat this container the given number of times in-place (equivalent to 'x *= n' in Python) */
	static PyObject* RepeatInplace(FNePyArrayWrapper* InSelf, Py_ssize_t InTimes);

	/** Get a slice of this container (equivalent to 'x[i:j]' in Python, returns new reference) */
	static PyObject* GetSubscript(FNePyArrayWrapper* InSelf, PyObject* Key);

	/** Set a slice of this container (equivalent to 'x[i:j] = v' in Python) */
	static int SetSubscript(FNePyArrayWrapper* InSelf, PyObject* Key, PyObject* Value);

	/** Compare two arrays, used for rich comparison */
	static PyObject* CompareArrays(FNePyArrayWrapper* InSelf, FNePyArrayWrapper* InOther, int InOp);

	/** Compare two elements, used for sorting */
	static int32 CompareElements(FProperty* InnerProp, const void* Element1, const void* Element2);
};

// 为UStruct的数组成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyStructArrayWrapper : FNePyArrayWrapper
{
	// 对父结构的反向引用
	// UStruct不使用HouseKeeper管理生命周期。通过增加对父结构体的引用计数，防止父结构体被释放。
	PyObject* PyOuter;

	// 为UStruct的数组属性生成Python对象
	static FNePyStructArrayWrapper* New(PyObject* PyOuter, UScriptStruct* InStruct, void* InInstance, void* InMemberPtr, const char* PropName);

	// 为UStruct的数组属性生成Python对象
	static FNePyStructArrayWrapper* New(PyObject* PyOuter, void* InMemberPtr, const FArrayProperty* InArrayProp);

private:
	// tp_dealloc
	static void Dealloc(FNePyStructArrayWrapper* InSelf);

	friend struct FNePyArrayWrapper;
};

FNePyArrayWrapper* NePyArrayWrapperCheck(PyObject* InPyObj);
PyTypeObject* NePyArrayWrapperGetType();