#pragma once
#include "NePyIncludePython.h"
#include "NePyStructBase.h"
#include "NePyObjectRef.generated.h"


// 这是一个辅助类，用于把PyObject对象传递到C++层
// UFUNCTION标记的函数无法直接使用 PyObject* 类型的参数，可使用 const FNePyObjectRef& 类型参数作为中转
USTRUCT(BlueprintType)
struct NEPYTHONBINDING_API FNePyObjectRef
{
	GENERATED_BODY();

	// 为PyObject生成FNePyObjectRef
	// - 对象装箱时，引用计数+1
	static bool Boxing(PyObject* InPyObj, FNePyObjectRef& OutPyObjRef);

	// 将内部对象返回给Python虚拟机，并置空Value
	// - 对象拆箱时，引用计数不变
	// - 如果内部对象为空，则返回Py_None，并使Py_None加1引用计数
	static PyObject* Unboxing(FNePyObjectRef& InPyObjRef);

	// 创建一个空PyObject引用
	FNePyObjectRef();

	// 创建一个PyObject引用
	explicit FNePyObjectRef(PyObject* InPyObj);

	// 拷贝构造
	FNePyObjectRef(const FNePyObjectRef& InOther);

	// 移动构造
	FNePyObjectRef(FNePyObjectRef&& InOther);

	// 析构
	// - 对象销毁时，引用计数-1
	~FNePyObjectRef();

	// 拷贝
	FNePyObjectRef& operator=(const FNePyObjectRef& InOther);

	// 移动
	FNePyObjectRef& operator=(FNePyObjectRef&& InOther);

	// 引用的Python对象
	PyObject* Value;
};

// 为FNePyObjectRef添加动态反射支持
struct FNePyStruct_NePyObjectRef : public TNePyStructBase<FNePyObjectRef>
{
};

void NePyInitObjectRef(PyObject* PyOuterModule);

NEPYTHONBINDING_API FNePyStruct_NePyObjectRef* NePyStructNew_NePyObjectRef(const FNePyObjectRef& InValue);

NEPYTHONBINDING_API FNePyStruct_NePyObjectRef* NePyStructCheck_NePyObjectRef(PyObject* InPyObj);

NEPYTHONBINDING_API PyTypeObject* NePyStructGetType_NePyObjectRef();
