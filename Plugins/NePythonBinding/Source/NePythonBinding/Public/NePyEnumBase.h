#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"

// UEnum对象的封装基类，基类为PyLongObject
struct NEPYTHONBINDING_API FNePyEnumBase : public PyVarObject
{
	// 初始化Enum条目
	static void InitEnumEntries(PyTypeObject* InPyType, const UEnum* InEnum);

	// 获取PyType对应的UEnum
	static const UEnum* GetEnum(FNePyEnumBase* InSelf);

private:
	// 如果枚举值刚好是Python的关键字，则将它们替换为别的名字
	// 注意：需要同时修改 PropHelper.repl_enum_item_name (PropHelper.py)
	static void ReplaceEnumItemName(FString& InOutName);
};

void NePyInitEnumBase(PyObject* PyOuterModule);
NEPYTHONBINDING_API FNePyEnumBase* NePyEnumBaseCheck(PyObject* InPyObj);
NEPYTHONBINDING_API PyTypeObject* NePyEnumBaseGetType();
