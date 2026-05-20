#pragma once
#include "NePyIncludePython.h"
#include "UObject/Interface.h"
#include "NePyResourceOwner.generated.h"


UINTERFACE(MinimalApi)
class UNePyResourceOwner : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/// <summary>
/// 对于长期持有 Python 对象的 UObject，需要实现该接口
/// </summary>
class INePyResourceOwner
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * 将 PyObject* 或 FNePyObjectPtr 的引用清理逻辑统一放在这个函数中。
	 * 该函数会在关闭 Python 虚拟机前统一调用，以确保 Python 对象都在虚拟机销毁前回收。
	 * 除此之外，UObject 自身也应该在合适的时机自行调用，执行清理工作。
	 * 例如，普通 UObject 的子类，一般需要重写 UObject::BeginDestroy() 并调用该函数执行清理，
	 * AActor 的子类，一般重写 AActor::Destroyed() 进行清理。
	 */
	virtual void ReleasePythonResources() = 0;
};
