#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNePythonBindingModuleInterface : public IModuleInterface
{
public:
	// 执行一段Python脚本
	virtual void RunString(const char* InStr) = 0;

	// 尝试执行一段console指令，如果python脚本中未能正常执行，则返回False
	virtual bool TryRunString(const char* InStr) = 0;

#if WITH_EDITOR
	// 执行一个外部Python脚本
	virtual bool RunFile(const TCHAR* InFile, const TCHAR* InArgs) = 0;
#endif

	// 获取运行时委托容器
	virtual class UNePyRuntimeDelegates* GetRuntimeDelegates() const = 0;

	// 是否需要运行Patch
	virtual bool NeedRunPatch() const = 0;
};