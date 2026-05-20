#pragma once
#include "Runtime/Slate/Public/SlateBasics.h"

class FNePythonConsoleModule : public IModuleInterface
{
public:
	virtual void StartupModule();
	virtual void ShutdownModule();

	/** Generates a console input box widget.  Remember, this widget will become invalid if the
		output log DLL is unloaded on the fly. */
	virtual TSharedRef<SWidget> MakeConsoleInputBox(TSharedPtr<SEditableTextBox>& OutExposedEditableTextBox) const;

	static void InvokePythonConsoleTab();
};