#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FNePythonEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule();
	virtual void ShutdownModule();
};