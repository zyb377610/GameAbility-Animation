#pragma once
#include "NePyIncludePython.h"

// 用于从.pak文件中读取脚本文件，供import使用
bool NePyImporterInit();

#if PY_MAJOR_VERSION >= 3
// Python3在虚拟机初始化（也就是Py_Initialize）的时候，就会加载外部py文件
// 必须要在此之前将Importer注入，才能正确地从.pak文件里读取py文件
bool NePyInjectImporterToInittab();
#endif // PY_MAJOR_VERSION >= 3

// 返回用于脚本加密解密的AES密钥
void NePyImporterEncryptKey(uint8* OutKey);
