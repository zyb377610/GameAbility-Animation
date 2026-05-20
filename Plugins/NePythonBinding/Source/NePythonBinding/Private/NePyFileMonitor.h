#pragma once
#include "NePyIncludePython.h"
#include "CoreMinimal.h"

#if PLATFORM_WINDOWS
// 提供接口监听文件变化，用于脚本自动热更
void NePyInitFileMonitor(PyObject* PyOuterModule);
#endif
