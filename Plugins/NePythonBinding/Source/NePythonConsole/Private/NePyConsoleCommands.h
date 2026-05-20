#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "HAL/IConsoleManager.h"
#include "Runtime/Launch/Resources/Version.h"

class FNePyConsoleCommands : public TCommands<FNePyConsoleCommands>
{
public:
	FNePyConsoleCommands()
		: TCommands<FNePyConsoleCommands>(
			TEXT("New Eden Python Console"),
			NSLOCTEXT("Contexts", "NePyConsoleCommands", "New Eden Python Console"),
			NAME_None,
			FName(TEXT("FTextBlockStyle"))
			)
	{
	}

	virtual void RegisterCommands() override;

private:
	static void DoShowConsole();

private:
	TSharedPtr<FUICommandInfo> ShowConsole;
	TSharedPtr<FUICommandInfo> ShowConsoleUE5;
};

class FNepyCommandExecutor : public IConsoleCommandExecutor
{
protected:
	bool IsFilePathFormat(const FString& Input);
public:
	FNepyCommandExecutor();

	static FName StaticName();
	virtual FName GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetDescription() const override;
	virtual FText GetHintText() const override;
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5 || ENGINE_MAJOR_VERSION >= 6
	virtual void GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out) override;
#else
	virtual void GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out) override;
#endif
	virtual void GetExecHistory(TArray<FString>& Out) override;
	virtual bool Exec(const TCHAR* Input) override;
	virtual bool AllowHotKeyClose() const override;
	virtual bool AllowMultiLine() const override;
	virtual FInputChord GetHotKey() const override;
#if ENGINE_MAJOR_VERSION >= 5
	virtual FInputChord GetIterateExecutorHotKey() const override;
#endif
};