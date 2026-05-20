#pragma once
#include "NePyIncludePython.h"

// 这是一个辅助类，持有一个PyObject
// 使用RAII管理内部PyObject对象的引用计数
struct FNePyObjectHolder
{
	// 默认构造
	FNePyObjectHolder();

	// 构造
	explicit FNePyObjectHolder(PyObject* InPyObj);

	// 拷贝构造
	FNePyObjectHolder(const FNePyObjectHolder& InOther);

	// 移动构造
	FNePyObjectHolder(FNePyObjectHolder&& InOther);

	// 拷贝
	FNePyObjectHolder& operator=(const FNePyObjectHolder& InOther);

	// 移动
	FNePyObjectHolder& operator=(FNePyObjectHolder&& InOther);

	// 析构
	~FNePyObjectHolder();

	// 引用的Python对象
	PyObject* Value;
};
