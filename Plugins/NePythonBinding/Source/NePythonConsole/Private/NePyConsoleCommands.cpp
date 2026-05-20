#include "NePyConsoleCommands.h"
#include "NePythonConsoleModule.h"
#include "NePyBindingModuleInterface.h"
#include "NePyBase.h"
#include "Toolkits/GlobalEditorCommonCommands.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Runtime/Launch/Resources/Version.h"

#define LOCTEXT_NAMESPACE "NePyConsoleCommands"

void FNePyConsoleCommands::RegisterCommands()
{
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	auto& ActionList = MainFrameModule.GetMainFrameCommandBindings();

#if ENGINE_MAJOR_VERSION <= 4
	UI_COMMAND(ShowConsole, "NewEdenPythonConsole", "NewEdenPythonConsole", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Alt, EKeys::Tilde));
	ActionList->MapAction(Get().ShowConsole, FExecuteAction::CreateStatic(&FNePyConsoleCommands::DoShowConsole));
#endif

	// UE5把"Alt+`"这个快捷键占用了（参见FGlobalEditorCommonCommands::RegisterCommands()），我们改用"Shift+`"
	UI_COMMAND(ShowConsoleUE5, "NewEdenPythonConsole", "NewEdenPythonConsole", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Shift, EKeys::Tilde));
	ActionList->MapAction(Get().ShowConsoleUE5, FExecuteAction::CreateStatic(&FNePyConsoleCommands::DoShowConsole));
}

void FNePyConsoleCommands::DoShowConsole()
{
	FNePythonConsoleModule::InvokePythonConsoleTab();
}

FNepyCommandExecutor::FNepyCommandExecutor()
{
}

FName FNepyCommandExecutor::StaticName()
{
	static const FName CmdExecName = TEXT("Nepy");
	return CmdExecName;
}

FName FNepyCommandExecutor::GetName() const
{
	return StaticName();
}

FText FNepyCommandExecutor::GetDisplayName() const
{
	return LOCTEXT("NepyCommandExecutorDisplayName", "Nepy");
}

FText FNepyCommandExecutor::GetDescription() const
{
	return LOCTEXT("NepyCommandExecutorDescription", "Execute Nepy Python scripts (including files)");
}

FText FNepyCommandExecutor::GetHintText() const
{
	return LOCTEXT("NepyCommandExecutorHintText", "Enter Nepy Python script or a filename");
}

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5 || ENGINE_MAJOR_VERSION >= 6
void FNepyCommandExecutor::GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out)
{
}
#else
void FNepyCommandExecutor::GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out)
{
}
#endif

void FNepyCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(TEXT("Nepy"), Out);
}

bool FNepyCommandExecutor::IsFilePathFormat(const FString& Input)
{
	int32 LastDotIndex = -1;
	if (Input.FindLastChar(TEXT('.'), LastDotIndex))
	{
		FString Extension = Input.Mid(LastDotIndex + 1);
		return Extension == TEXT("py");
	}
	return false;
}

bool FNepyCommandExecutor::Exec(const TCHAR* Input)
{
	IConsoleManager::Get().AddConsoleHistoryEntry(TEXT("Nepy"), Input);

	FNePythonBindingModuleInterface* PythonModule = FModuleManager::GetModulePtr<FNePythonBindingModuleInterface>("NePythonBinding");
	ensureMsgf(PythonModule, TEXT("NePythonBinding plugin must be initialized first"));

	UE_LOG(LogNePython, Log, TEXT("%s"), Input);

	if (IsFilePathFormat(Input))
	{
		PythonModule->RunFile(Input, nullptr);
	}
	else
	{
		PythonModule->RunString(TCHAR_TO_UTF8(Input));
	}

	return true;
}

bool FNepyCommandExecutor::AllowHotKeyClose() const
{
	return true;
}

bool FNepyCommandExecutor::AllowMultiLine() const
{
	return true;
}

FInputChord FNepyCommandExecutor::GetHotKey() const
{
#if WITH_EDITOR
	return FGlobalEditorCommonCommands::Get().OpenConsoleCommandBox->GetActiveChord(EMultipleKeyBindingIndex::Primary).Get();
#else
	return FInputChord();
#endif
}

#if ENGINE_MAJOR_VERSION >= 5
FInputChord FNepyCommandExecutor::GetIterateExecutorHotKey() const
{
#if WITH_EDITOR
	return FGlobalEditorCommonCommands::Get().SelectNextConsoleExecutor->GetActiveChord(EMultipleKeyBindingIndex::Primary).Get();
#else
	return FInputChord();
#endif
}
#endif

#undef  LOCTEXT_NAMESPACE