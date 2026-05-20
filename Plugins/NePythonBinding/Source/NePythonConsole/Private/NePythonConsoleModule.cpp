// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NePythonConsoleModule.h"
#include "SPythonConsole.h"
#include "SPythonLog.h"
#include "NePyConsoleCommands.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/Slate/Public/Widgets/Docking/SDockTab.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructureModule.h"
#include "Editor/WorkspaceMenuStructure/Public/WorkspaceMenuStructure.h"

IMPLEMENT_MODULE(FNePythonConsoleModule, NePythonConsole);

namespace NePythonConsoleModule
{
	static const FName PythonLogTabName = FName(TEXT("NePythonLog"));
}

/** This class is to capture all log output even if the log window is closed */
class FPythonLogHistory : public FOutputDevice
{
public:

	FPythonLogHistory()
	{
		GLog->AddOutputDevice(this);
		GLog->SerializeBacklog(this);
	}

	~FPythonLogHistory()
	{
		// At shutdown, GLog may already be null
		if (GLog != NULL)
		{
			GLog->RemoveOutputDevice(this);
		}
	}

	/** Gets all captured messages */
	const TArray<TSharedPtr<FLogMessage>>& GetMessages() const
	{
		return Messages;
	}

protected:

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		// Capture all incoming messages and store them in history
		SPythonLog::CreateLogMessages(V, Verbosity, Category, Messages);
	}

private:

	/** All log messsges since this module has been started */
	TArray<TSharedPtr<FLogMessage>> Messages;
};

/** Our global output log app spawner */
static TSharedPtr<FPythonLogHistory> PythonLogHistory;

static TSharedPtr<FNepyCommandExecutor> PythonCommandExecutor;

TSharedRef<SDockTab> SpawnPythonLog(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
#if ENGINE_MAJOR_VERSION < 5
		.Icon(FEditorStyle::GetBrush("Log.TabIcon"))
#endif
		.TabRole(ETabRole::NomadTab)
		.Label(NSLOCTEXT("NePythonConsole", "TabTitle", "NewEdenPythonConsole"))
		[
			SNew(SPythonLog).Messages(PythonLogHistory->GetMessages())
		];
}

void FNePythonConsoleModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(NePythonConsoleModule::PythonLogTabName, FOnSpawnTab::CreateStatic(&SpawnPythonLog))
		.SetDisplayName(NSLOCTEXT("UnrealEditor", "NePythonLogTab", "NewEdenPythonConsole"))
		.SetTooltipText(NSLOCTEXT("UnrealEditor", "NePythonLogTooltipText", "Open the NewEdenPythonConsole tab."))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsLogCategory())
#if ENGINE_MAJOR_VERSION >= 5
		.SetIcon(FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Log.TabIcon"));
#else
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Log.TabIcon"));
#endif

	PythonLogHistory = MakeShareable(new FPythonLogHistory);

	PythonCommandExecutor = MakeShareable(new FNepyCommandExecutor);
	IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), PythonCommandExecutor.Get());

	FNePyConsoleCommands::Register();
}

void FNePythonConsoleModule::ShutdownModule()
{
	IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), PythonCommandExecutor.Get());

	if (FSlateApplication::IsInitialized())
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(NePythonConsoleModule::PythonLogTabName);
	}
}

TSharedRef<SWidget> FNePythonConsoleModule::MakeConsoleInputBox(TSharedPtr< SEditableTextBox >& OutExposedEditableTextBox) const
{
	TSharedRef<SPythonConsoleInputBox> NewConsoleInputBox = SNew(SPythonConsoleInputBox);
	OutExposedEditableTextBox = NewConsoleInputBox->GetEditableTextBox();
	return NewConsoleInputBox;
}

void FNePythonConsoleModule::InvokePythonConsoleTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(NePythonConsoleModule::PythonLogTabName);
}
