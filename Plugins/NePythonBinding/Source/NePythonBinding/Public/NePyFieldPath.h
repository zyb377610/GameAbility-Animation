#pragma once
#include "NePyIncludePython.h"
#include "CoreMinimal.h"
#include "UObject/FieldPath.h"

// 为FFieldPath提供python导出
struct NEPYTHONBINDING_API FNePyFieldPath : public PyObject
{
	// 属性路径值
	FFieldPath Value;

	// 初始化属性路径PyType类型
	static void InitPyType(PyObject* PyOuterModule);

	// 生成属性路径Python对象
	static FNePyFieldPath* New(const FFieldPath& InValue);

	// 检查输入对象是否为FNePyFieldPath类型，是则返回对象本身，否则返回空
	static FNePyFieldPath* Check(PyObject* InPyObj);

private:
	// tp_init
	static int Init(FNePyFieldPath* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyFieldPath* InSelf);

	// tp_str
	static PyObject* Str(FNePyFieldPath* InSelf);

	// tp_richcompare
	static PyObject* RichCompare(FNePyFieldPath* InSelf, PyObject* InOther, int InOp);

	// tp_hash
	static FNePyHashType Hash(FNePyFieldPath* InSelf);

private:
	// 检测属性路径引用的FField（或UField）是否存在
	static PyObject* IsValid(FNePyFieldPath* InSelf);
};
