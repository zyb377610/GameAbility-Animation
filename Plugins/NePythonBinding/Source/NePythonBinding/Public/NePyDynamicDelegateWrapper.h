#pragma once
#include "NePyBase.h"
#include "UObject/UnrealType.h"
#include "NePyHouseKeeper.h"
#include "NePyDelegate.h"

// 为UObject的委托成员变量提供引用类型的访问
struct NEPYTHONBINDING_API FNePyDynamicDelegateWrapper : FNePyPropObject
{
	// 初始化委托PyType类型
	static void InitPyType();

	// 为UObject的委托属性生成Python对象
	static FNePyDynamicDelegateWrapper* New(UObject* InObject, void* InMemberPtr, const char* PropName);

	// 为UObject的委托属性生成Python对象
	static FNePyDynamicDelegateWrapper* New(UObject* InObject, const FDelegateProperty* InProp);

	// 检查输入对象是否为FNePyDynamicDelegateWrapper类型，是则返回对象本身，否则返回空
	static FNePyDynamicDelegateWrapper* Check(PyObject* InPyObj);

	// 此委托是否已绑定过回调函数（Python或C++）
	bool IsBound();

	// 此委托是否已绑定至某Python回调函数
	bool IsBoundTo(PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数
	void Bind(PyObject* InPyCallable);

	// 从此委托移除一个Python回调函数
	void Unbind();

private:
	// tp_dealloc
	static void Dealloc(FNePyDynamicDelegateWrapper* InSelf);

	// tp_init
	static int Init(FNePyDynamicDelegateWrapper* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyDynamicDelegateWrapper* InSelf);

private:
	// 检测底层UObject是否已被销毁
	static PyObject* IsValid(FNePyDynamicDelegateWrapper* InSelf);

	// 此委托是否已绑定过回调函数（Python或C++）
	static PyObject* PyIsBound(FNePyDynamicDelegateWrapper* InSelf);

	// 此委托是否已绑定至某Python回调函数
	static PyObject* PyIsBoundTo(FNePyDynamicDelegateWrapper* InSelf, PyObject* InPyCallable);

	// 向此委托绑定一个Python回调函数
	static PyObject* PyBind(FNePyDynamicDelegateWrapper* InSelf, PyObject* InPyCallable);

	// 从此委托移除一个Python回调函数
	static PyObject* PyUnbind(FNePyDynamicDelegateWrapper* InSelf);

	// 返回此委托上绑定的Python回调函数
	static PyObject* GetPythonCallback(FNePyDynamicDelegateWrapper* InSelf);

	// 触发委托，如果未绑定回调函数则报错
	static PyObject* PyExecute(FNePyDynamicDelegateWrapper* InSelf, PyObject* InArgs);

	// 如果委托绑定了回调函数，则触发委托
	static PyObject* PyExecuteIfBound(FNePyDynamicDelegateWrapper* InSelf, PyObject* InArgs);

	// 触发委托
	PyObject* ExecuteInternal(PyObject* InArgs);

private:
	// 委托属性定义
	const FDelegateProperty* Prop;
	// 回调函数
	UNePyDelegate* Delegate;
};

// 为UFunction的委托类型参数提供引用类型的访问
// 用于将Python回调函数绑定到C++定义的方法中的委托类型参数
struct NEPYTHONBINDING_API FNePyDynamicDelegateParam : FNePyPropObject
{
	// 初始化委托PyType类型
	static void InitPyType();

	// 为UFunciton的委托参数生成Python对象
	static FNePyDynamicDelegateParam* New(UObject* InObject, const FDelegateProperty* InProp);

	// 为UFunciton的委托参数生成Python对象，并绑定Python回调函数
	static UNePyDelegate* FindOrAddNePyDelegate(UObject* InObject, const FName& InFuncName, const FName& InParamName, PyObject* InPyCallable);

	// 根据传入的Python回调，创建委托对象
	// 注意！处于性能考虑，同一个Python回调会返回同一个委托对象
	UNePyDelegate* FindOrAddDelegate(PyObject* InPyCallable);

private:
	// 根据函数名和参数名，查找对应的DelegateProperty
	static const FDelegateProperty* FindDelegateProperty(UObject* InObject, const FName& InFuncName, const FName& InParamName);

	// 返回委托参数定义
	inline const FDelegateProperty* GetParamProp()
	{
		return (const FDelegateProperty*)Value;
	}

private:
	// tp_dealloc
	static void Dealloc(FNePyDynamicDelegateParam* InSelf);

	// tp_init
	static int Init(FNePyDynamicDelegateParam* InSelf, PyObject* InArgs, PyObject* InKwds);

private:

private:
	// 回调函数列表
	TArray<UNePyDelegate*> Delegates;
};

// 用于将委托类型的参数传递至Python脚本
struct NEPYTHONBINDING_API FNePyDynamicDelegateArg : public PyObject
{
	// 初始化委托PyType类型
	static void InitPyType();

	// 为UFunciton的委托参数生成Python对象
	static FNePyDynamicDelegateArg* New(const FScriptDelegate& InDelegate, const FDelegateProperty* InDelegateProp);

private:
	// tp_dealloc
	static void Dealloc(FNePyDynamicDelegateArg* InSelf);

	// tp_init
	static int Init(FNePyDynamicDelegateArg* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	// tp_repr
	static PyObject* Repr(FNePyDynamicDelegateArg* InSelf);

private:
	// 此委托是否已绑定过回调函数（Python或C++）
	static PyObject* PyIsBound(FNePyDynamicDelegateArg* InSelf);
	
	// 触发委托，如果未绑定回调函数则报错
	static PyObject* PyExecute(FNePyDynamicDelegateArg* InSelf, PyObject* InArgs);

	// 如果委托绑定了回调函数，则触发委托
	static PyObject* PyExecuteIfBound(FNePyDynamicDelegateArg* InSelf, PyObject* InArgs);

	// 触发委托
	PyObject* ExecuteInternal(PyObject* InArgs);
	
private:
	// 委托
	FScriptDelegate Delegate;
};
