#pragma once
#include "NePyBase.h"
#include <type_traits>
#include "CoreMinimal.h"

// UObject对象封装基类
struct NEPYTHONBINDING_API FNePyObjectBase : public PyObject
{
	// C++对象
	UObject* Value;
};

// 初始化Python类型对象公共属性
NEPYTHONBINDING_API void NePyObjectType_InitCommon(PyTypeObject* InPyType);

// UObject的python封装基类
void NePyInitObjectBase(PyObject* PyOuterModule);
NEPYTHONBINDING_API FNePyObjectBase* NePyObjectBaseNew(UObject* InValue, PyTypeObject* InPyType);
NEPYTHONBINDING_API FNePyObjectBase* NePyObjectBaseCheck(PyObject* InPyObj);
NEPYTHONBINDING_API PyTypeObject* NePyObjectBaseGetType();

// 仅供FNePyHouseKeeper::NewNePyObject()调用
// 用户必须保证UObject不存在PythonWrapper
FNePyObjectBase* NePyObjectBaseNewInternalUseOnly(UObject* InValue);

// 事件绑定相关
NEPYTHONBINDING_API void NePyObjectBaseAutoBindEventForPyClass(FNePyObjectBase* InPyObject, PyObject* InPyClass);
NEPYTHONBINDING_API void NePyObjectBaseBindEventsForPyClassByAttribute(UObject* InObject, PyObject* InPyClass);
NEPYTHONBINDING_API PyObject* NePyObjectBaseBindEvent(FNePyObjectBase* InPyObject, FString InEventName, PyObject* InPyCallable, bool bFailOnWrongProp);
NEPYTHONBINDING_API PyObject* NePyObjectBaseUnbindEvent(FNePyObjectBase* InPyObject, FString InEventName, PyObject* InPyCallable, bool bFailOnWrongProp);
NEPYTHONBINDING_API PyObject* NePyObjectBaseUnbindAllEvent(FNePyObjectBase* InPyObject, FString InEventName, bool bFailOnWrongProp);

// object dealloc function base.
NEPYTHONBINDING_API void NePyObject_Dealloc(FNePyObjectBase* InSelf);