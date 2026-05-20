#pragma once
#include "NePyIncludePython.h"
#include "NePyGIL.h"
#include "NePyResourceOwner.h"
#include "NePyDelegate.generated.h"

UCLASS()
class NEPYTHONBINDING_API UNePyDelegate : public UObject, public INePyResourceOwner
{
	GENERATED_BODY()

public:
	UNePyDelegate();

	// UObject interface
	virtual void BeginDestroy() override;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

	// 初始化
	virtual void Initialize(UFunction* InSignature, PyObject* InPyCallableObj);
	virtual void Initialize(UFunction* InSignature, const UObject* InOwnerObject, PyObject* InPyCallableObj);

	// 清理
	void Finalize();

	// 判断此对象是否使用了传入的Python回调函数
	bool UsesPyCallable(PyObject* InPyCallableObj) const;

	//
	virtual void ProcessEvent(UFunction* InFunc, void* InBuffer) override;

	// 获取PyCallableObj
	virtual PyObject* GetPyCallable() const;

	// 获取PyCallableObj的字符串表示
	FString GetPyCallableStr() const;

	static FName FakeFuncName;

protected:
	UFunction* Signature;
	const UObject* OwnerObject;
	PyObject* PyCallableObj;

	UFUNCTION()
	void PyFakeCallable();
};

UCLASS()
class UNePyWeakDelegate : public UNePyDelegate
{
	GENERATED_BODY()

public:
	// 初始化
	virtual void Initialize(UFunction* InSignature, PyObject* InPyCallableObj);

	// 获取PyCallableObj
	virtual PyObject* GetPyCallable() const;
};

