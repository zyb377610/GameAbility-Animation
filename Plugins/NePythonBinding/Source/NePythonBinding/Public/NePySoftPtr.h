#pragma once
#include "NePyIncludePython.h"
#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "NePyHouseKeeper.h"

struct NEPYTHONBINDING_API FNePySoftPtr : public PyObject
{
	FSoftObjectPtr Value;
	// 初始化 SoftPtr PyType 类型
	static void InitPyType(PyObject* PyOuterModule);
	static FNePySoftPtr* New(const FSoftObjectPtr& InValue);
	static FNePySoftPtr* Check(PyObject* InPyObj);

private:

	// tp_init
	static int Init(FNePySoftPtr* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	// tp_str
	static PyObject* Repr(FNePySoftPtr* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePySoftPtr* InSelf, PyObject* InOther, int InOp);

	// tp_hash
	static FNePyHashType Hash(FNePySoftPtr* InSelf);
private:
	static PyObject* IsValid(FNePySoftPtr* InSelf);
	static PyObject* IsNull(FNePySoftPtr* InSelf);
	static PyObject* IsPending(FNePySoftPtr* InSelf);
	static PyObject* IsStale(FNePySoftPtr* InSelf);
	static PyObject* Get(FNePySoftPtr* InSelf);
	static PyObject* GetAssetName(FNePySoftPtr* InSelf);
	static PyObject* GetLongPackageName(FNePySoftPtr* InSelf);
	//static PyObject* ToSoftObjectPath(FNePySoftPtr* InSelf);
	static PyObject* LoadSynchronous(FNePySoftPtr* InSelf);
};

NEPYTHONBINDING_API PyTypeObject* NePySoftPtrGetType();
