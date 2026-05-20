#include "NePythonEditorModule.h"
#include "NePySourceCodeNavigation.h"

IMPLEMENT_MODULE(FNePythonEditorModule, NePythonEditor);

void FNePythonEditorModule::StartupModule()
{
	NePyRegisterSourceNavigation();
}

void FNePythonEditorModule::ShutdownModule()
{
	NePyUnregisterSourceNavigation();
}