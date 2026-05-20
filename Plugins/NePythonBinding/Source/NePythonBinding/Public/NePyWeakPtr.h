#pragma once
#include "NePyIncludePython.h"
#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "NePyHouseKeeper.h"

struct NEPYTHONBINDING_API FNePyWeakPtr : public PyObject
{
	FWeakObjectPtr Value;
	// 初始化 WeakPtr PyType 类型
	static void InitPyType(PyObject* PyOuterModule);
	static FNePyWeakPtr* New(const FWeakObjectPtr& InValue);
	static FNePyWeakPtr* Check(PyObject* InPyObj);

private:

	// tp_init
	static int Init(FNePyWeakPtr* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	// tp_str
	static PyObject* Repr(FNePyWeakPtr* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePyWeakPtr* InSelf, PyObject* InOther, int InOp);

	// tp_hash
	static FNePyHashType Hash(FNePyWeakPtr* InSelf);
private:
	static PyObject* IsValid(FNePyWeakPtr* InSelf);
	static PyObject* IsStale(FNePyWeakPtr* InSelf);
	static PyObject* Get(FNePyWeakPtr* InSelf);
};

NEPYTHONBINDING_API PyTypeObject* NePyWeakPtrGetType();