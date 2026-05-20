#pragma once
#include "NePyBase.h"
#include "UObject/UnrealType.h"
#include "NePyHouseKeeper.h"
#include "NePyDelegate.h"

// 为UObject的委托成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyDynamicMulticastDelegateWrapper : FNePyPropObject
{
	// 初始化委托PyType类型
	static void InitPyType();

	// 为UObject的委托属性生成Python对象
	static FNePyDynamicMulticastDelegateWrapper* New(UObject* InObject, void* InMemberPtr, const char* PropName);

	// 为UObject的委托属性生成Python对象
	static FNePyDynamicMulticastDelegateWrapper* New(UObject* InObject, const FMulticastDelegateProperty* InProp);

	// 检查输入对象是否为FNePyDynamicMulticastDelegateWrapper类型，是则返回对象本身，否则返回空
	static FNePyDynamicMulticastDelegateWrapper* Check(PyObject* InPyObj);

	// 此委托是否已绑定过回调函数（Python或C++）
	bool IsBound();

	// 此委托是否已绑定过某Python回调函数
	bool Contains(PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数
	void Add(PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数，重复绑定无效
	void AddUnique(PyObject* InPyCallable);

	// 从此委托移除一个Python回调函数
	void Remove(PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数到指定的目标对象上
	void AddUObjectDynamic(const UObject* InTarget, PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数到指定的目标对象上，重复绑定无效
	void AddUObjectDynamicUnique(const UObject* InTarget, PyObject* InPyCallable);

	// 从此委托从指定的目标对象上移除一个Python回调函数
	void RemoveUObjectDynamic(const UObject* InTarget, PyObject* InPyCallable);

	// 从此委托从指定的目标对象上移除所有绑定的Python回调函数
	void RemoveUObjectAllDynamic(const UObject* InTarget);

	// 当UObject被重实例化后，更新绑定的回调函数
	void ReinstanceUObjectDynamic(const UObject* OldObject, const UObject* NewObject);

	// 清除委托上绑定的所有Python回调函数
	void Clear();

private:
	// tp_dealloc
	static void Dealloc(FNePyDynamicMulticastDelegateWrapper* InSelf);

	// tp_init
	static int Init(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyDynamicMulticastDelegateWrapper* InSelf);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePyDynamicMulticastDelegateWrapper* InSelf);

	// 此委托是否已绑定过回调函数（Python或C++）
	static PyObject* PyIsBound(FNePyDynamicMulticastDelegateWrapper* InSelf);

	// 此委托是否已绑定过某Python回调函数
	static PyObject* PyContains(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数
	static PyObject* PyAdd(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数，重复绑定无效
	static PyObject* PyAddUnique(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable);

	// 从此委托移除一个Python回调函数
	static PyObject* PyRemove(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数到指定的目标对象上
	static PyObject* PyAddDynamic(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs);

	// 向此委托绑定一个Python回调函数到指定的目标对象上，重复绑定无效
	static PyObject* PyAddDynamicUnique(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs);

	// 从此委托从指定的目标对象上移除一个Python回调函数
	static PyObject* PyRemoveDynamic(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs);

	// 清除委托上绑定的所有Python回调函数
	static PyObject* PyClear(FNePyDynamicMulticastDelegateWrapper* InSelf);

	// 返回此委托上绑定的Python回调函数列表
	static PyObject* GetPythonCallbacks(FNePyDynamicMulticastDelegateWrapper* InSelf);

	// 根据Python回调函数，查找在Delegates数组中的索引位置
	int32 FindDelegateIndex(PyObject* InPyCallable);

	// 触发委托
	static PyObject* PyBroadcast(FNePyDynamicMulticastDelegateWrapper* InSelf, PyObject* InArgs);

	// 辅助函数：从 PyObject 参数中解析 target 和 callable
	static bool ParseDynamicDelegateArgs(PyObject* InArgs, const char* FunctionName, PyObject*& OutTargetPyObject, PyObject*& OutPyCallable);

	// 触发委托
	PyObject* BroadcastInternal(PyObject* InArgs);

private:
	// 委托属性定义
	const FMulticastDelegateProperty* Prop;
	// 回调函数列表
	TArray<UNePyDelegate*> Delegates;
	// 绑定到UObject的回调函数列表
	TMap<const UObject*, TArray<UNePyDelegate*>> UObjectDelegates;
};
