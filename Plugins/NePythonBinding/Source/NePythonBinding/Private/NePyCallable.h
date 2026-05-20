#pragma once
#include "NePyIncludePython.h"
#include "CoreMinimal.h"

namespace NePyGenUtil
{
	struct FMethodDef;
}

// 类似PyMethodObject，持有一个函数对象，以及调用者的self指针
struct FNePyCallable : public PyObject
{
	// 函数对象
	NePyGenUtil::FMethodDef* MethodDef;
	// self指针
	PyObject* SelfObject;

	// 初始化PyType类型
	static void InitPyType();

	// 生成Python对象
	static FNePyCallable* New(NePyGenUtil::FMethodDef* InMethodDef, PyObject* InSelfObject);

	// 判断Python对象是否为此类的实例
	static FNePyCallable* Check(PyObject* InPyObj);

	// 从Python端调用UE函数
	static PyObject* CallMethod(NePyGenUtil::FMethodDef* InMethodDef, PyObject* SelfObject, PyObject* InArgs, PyObject* InKwds);

private:
	// tp_dealloc
	static void Dealloc(FNePyCallable* InSelf);

	// tp_clear
	static int Clear(FNePyCallable* InSelf);

	// tp_repr
	static PyObject* Repr(FNePyCallable* InSelf);

	// tp_call
	static PyObject* Call(FNePyCallable* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_traverse
	static int Traverse(FNePyCallable* InSelf, visitproc visit, void* arg);

	// tp_richcompare
	static PyObject* RichCompare(PyObject* InSelf, PyObject* InOther, int Op);

private:
	static PyObject* CallMethodNoArgs(NePyGenUtil::FMethodDef* InMethodDef, UObject* InObject, PyObject* InArgs, PyObject* InKwds);
	static PyObject* CallMethodWithArgs(NePyGenUtil::FMethodDef* InMethodDef, UObject* InObject, PyObject* InArgs, PyObject* InKwds);
};
