#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"
#include "NePyBindingModuleInterface.h"
#include "NePyPtr.h"
#if ENGINE_MAJOR_VERSION >= 5
#include "UObject/ObjectPtr.h"
#endif

class NEPYTHONBINDING_API FNePythonBindingModule : public FNePythonBindingModuleInterface
{
public:
	static inline FNePythonBindingModule& Get()
	{
		return FModuleManager::GetModuleChecked<FNePythonBindingModule>("NePythonBinding");
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// 初始化python模块
	void InitPythonModules();

	// 获取Python根模块，用于给Game注册新的模块
	PyObject* GetPythonRootModule();

	// 向Python根模块添加方法
	void AppendPythonTopModuleMethods(PyMethodDef PyTopModuleMethods[]);

	/** FNePythonBindingModuleInterface implementation */

	// 执行一段Python脚本
	void RunString(const char* InStr) override;

	// 尝试执行一段console指令，如果python脚本中未能正常执行，则返回False
	bool TryRunString(const char* InStr) override;

#if WITH_EDITOR
	// 执行一个外部Python脚本
	bool RunFile(const TCHAR* InFile, const TCHAR* InArgs) override;
#endif

	class UNePyRuntimeDelegates* GetRuntimeDelegates() const override;

	bool NeedRunPatch() const override { return bNeedRunPatch; }

	bool NeedRedirect() const { return bNeedRedirect; }

	void LogPythonCallStackSafe();
private:
	// 初始化Python虚拟机
	void InitializePython();

	// 销毁Python虚拟机
	void ShutdownPython();

	// 引擎初始化完毕的回调
	void OnPostEngineInit();

	// 引擎准备要退出时的回调
	void OnPreExit();

	// 引擎发生崩溃错误时的回调
	void OnSystemError();

	bool Tick(float DeltaSeconds);

	// 获取sys.path
	TArray<FString> GetPythonSysPaths();

	// 设置sys.path
	void SetPythonSysPaths(TArray<FString> Paths);

	// 设置sys.argv
	void SetPythonSysArgv(const TCHAR* CommandLine);

private:
	// nepyinit.py（原ue_site.py）
	const char* InitModuleName = nullptr;

	// 对应ue模块
	PyObject* PyRootModule = nullptr;
	// 对应ue.internal模块
	PyObject* PyInternalModule = nullptr;
	// 对应__main__
	PyObject* PyMainModule = nullptr;
	// 对应__main__.__dict__
	PyObject* PyMainDict = nullptr;
	// 对应nepyinit.py（原ue_site.py）
	PyObject* PyInitModule = nullptr;
	// 对应nepyinit.on_post_engine_init（原ue_site.on_post_engine_init）
	PyObject* PyOnPostEngineInit = nullptr;
	// 对应nepyinit.on_debug_input（原ue_site.on_debug_input）
	PyObject* PyOnDebugInputFunc = nullptr;
	// 对应nepyinit.on_tick（原ue_site.on_tick）
	PyObject* PyOnTickFunc = nullptr;
	// 对应nepyinit.on_init（原ue_site.on_init）
	PyObject* PyOnInitFunc = nullptr;

	// nepypatch.py
	const char* PatchModuleName = nullptr;
	// 对应nepypatch.py
	PyObject* PyPatchModule = nullptr;
	// 对应nepypatch.should_continue
	PyObject* PyPatchShouldContinueFunc = nullptr;
	// 对应nepypatch.on_run
	PyObject* PyPatchRunFunc = nullptr;

	// 运行时委托容器
#if ENGINE_MAJOR_VERSION >= 5
	TObjectPtr<UNePyRuntimeDelegates> RuntimeDelegateContainer = nullptr;
#else
	UNePyRuntimeDelegates* RuntimeDelegateContainer = nullptr;
#endif

	// tick
#if ENGINE_MAJOR_VERSION >= 5
	FTSTicker::FDelegateHandle TickDelegateHandler;
#else
	FDelegateHandle TickDelegateHandler;
#endif

	// 是否跳过Python虚拟机销毁逻辑
	bool bBrutalFinalize = false;
	// 是否运行过init函数
	bool bInitFuncCalled = false;
	// 是否需要运行patch
	bool bNeedRunPatch = false;
	// patch运行异常是否需要强制退出
	bool bBrutalPatchExceptionExit = true;
	// 是否需要Redirect
	bool bNeedRedirect = false;


public:
	bool IsInitFuncCalled() const { return bInitFuncCalled; }
};

class NEPYTHONBINDING_API FNePythonModuleDelegates
{
public:
	DECLARE_MULTICAST_DELEGATE(FNePythonModuleState);
	DECLARE_MULTICAST_DELEGATE_OneParam(FNePythonInitPythonModuleDelegate, PyObject*);
	static FNePythonModuleState OnPythonModuleState_PostInit;
	static FNePythonModuleState OnPythonModuleState_PreRelease;
	static FNePythonInitPythonModuleDelegate OnInitPythonModule; // 用于通知其它模块进行python模块的注册
	static bool IsModuleInited() { return bModuleInited; }

private:
	static bool bModuleInited;
	friend class FNePythonBindingModule;
	FNePythonModuleDelegates() {}
};

