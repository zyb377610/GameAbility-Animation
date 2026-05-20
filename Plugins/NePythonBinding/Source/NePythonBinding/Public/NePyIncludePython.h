#pragma once
#include "CoreMinimal.h"

// 详情见https://peps.python.org/pep-0353/，搜索PY_SSIZE_T_CLEAN
#define PY_SSIZE_T_CLEAN

#if PLATFORM_WINDOWS
#include <pyconfig.h>
#ifndef SIZEOF_PID_T
#define SIZEOF_PID_T 4
#endif
#include <Python.h>
#include <structmember.h>
#else
#include <Python.h>
#include <structmember.h>
#endif

#ifdef __clang__
// 在导出Python函数时，例如 { "SupportedClass", (getter)FNePyObject_Exporter_Prop_GetSupportedClass, (setter)FNePyObject_Exporter_Prop_SetSupportedClass, nullptr, nullptr }
// 由于Python定义的结构体成员为char*而非const char*，导致clang编译报错
#pragma clang diagnostic ignored "-Wwritable-strings"
#endif

#if PY_MAJOR_VERSION >= 3

//for String stuffs
#define NePyString_Check PyUnicode_Check
#define NePyString_CheckExact PyUnicode_CheckExact
#define NePyString_FromStringAndSize PyUnicode_FromStringAndSize
#define NePyString_FromString PyUnicode_FromString
#define NePyString_FromFormatV PyUnicode_FromFormatV
#define NePyString_FromFormat PyUnicode_FromFormat
#define NePyString_AsString PyUnicode_AsUTF8
#define NePyString_Type PyUnicode_Type

#else

#define NePyString_Check PyString_Check
#define NePyString_CheckExact PyString_CheckExact
#define NePyString_FromStringAndSize PyString_FromStringAndSize
#define NePyString_FromString PyString_FromString
#define NePyString_FromFormatV PyString_FromFormatV
#define NePyString_FromFormat PyString_FromFormat
#define NePyString_AsString PyString_AsString
#define NePyString_Type PyString_Type

#endif

#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 2
typedef Py_hash_t FNePyHashType;
#else
typedef long FNePyHashType;
#endif
