#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"

// 初始化根模块“ue”
// 返回根模块 Borrowed reference
PyObject* NePyInitTopModule(const char* RootModuleName);

// 向根模块注册Python类型
void NePyAddTypeToTopModule(PyTypeObject* InPyType);