#include "NePyCommandlet.h"
#if WITH_EDITOR
#include "ReflectionExport/NePyExport.h"
#include "Editor.h"
#endif
#include "NePyGIL.h"
#include "NePythonBinding.h"
#include "HAL/PlatformFile.h"
#include "Misc/FileHelper.h"


//-----------------------------------------------------------------------------------------------------------------
UNePyDumpReflectionInfosCommandlet::UNePyDumpReflectionInfosCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LogToConsole = 1;
}

int32 UNePyDumpReflectionInfosCommandlet::Main(const FString& InParamsStr)
{
#if WITH_EDITOR
	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*InParamsStr, Tokens, Switches, Params);

	FString OutputDir;
	if (Params.Contains(TEXT("OutputDir")))
	{
		OutputDir = *Params.Find(TEXT("OutputDir"));
	}
	else
	{
		OutputDir = NePyGetDefaultDumpReflectionInfosDirectory();
	}

	NePyDumpReflectionInfosToFile(OutputDir);
	return 0;
#else
	return -1;
#endif
}


//------------------------------------------------------------------------------------------------------------------
UNePyGmCommandlet::UNePyGmCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LogToConsole = 1;
}


int32 UNePyGmCommandlet::Main(const FString& CommandLine)
{
#if WITH_EDITOR

	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*CommandLine, Tokens, Switches, Params);

	FString gm_str = Tokens[0];
	FNePythonBindingModule& PythonModule = FModuleManager::GetModuleChecked<FNePythonBindingModule>("NePythonBinding");
	PythonModule.RunString(TCHAR_TO_UTF8(*gm_str));
#endif
	return 0;
}


//------------------------------------------------------------------------------------------------------------------
UNePyRunScriptCommandlet::UNePyRunScriptCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LogToConsole = 1;
}

int32 UNePyRunScriptCommandlet::Main(const FString& CommandLine)
{
#if WITH_EDITOR
	FString NextToken;
	TArray<FString> Tokens;
	FString PythonFilePath;
	const FString ScriptTag = TEXT("-Script=");

	const TCHAR* CommandLineStr = *CommandLine;
	while (FParse::Token(CommandLineStr, NextToken, false))
	{
		if (PythonFilePath.IsEmpty() && NextToken.StartsWith(ScriptTag, ESearchCase::IgnoreCase))
		{
			const TCHAR* ScriptTagValue = &NextToken[ScriptTag.Len()];
			if (*ScriptTagValue == TEXT('"'))
			{
				FParse::QuotedString(ScriptTagValue, PythonFilePath);
			}
			else
			{
				PythonFilePath = ScriptTagValue;
			}

			new(Tokens) FString(PythonFilePath);
			continue;
		}

		if (!PythonFilePath.IsEmpty())
		{
			new(Tokens) FString(NextToken);
		}
	}

	FString PythonArgs;
	if (Tokens.Num() > 0)
	{
		PythonArgs = FString::Join(Tokens, TEXT(" "));
	}
	
	FNePythonBindingModule& PythonModule = FModuleManager::GetModuleChecked<FNePythonBindingModule>("NePythonBinding");
	if (!PythonModule.RunFile(*PythonFilePath, *PythonArgs))
	{
		return -1;
	}
#endif
	return 0;
}

UNePyDumpBlueprintInfosCommandlet::UNePyDumpBlueprintInfosCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LogToConsole = 1;
}

int32 UNePyDumpBlueprintInfosCommandlet::Main(const FString& InParamsStr)
{
#if WITH_EDITOR
	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*InParamsStr, Tokens, Switches, Params);

	FString OutputFile;
	if (Params.Contains(TEXT("OutputFile")))
	{
		OutputFile = *Params.Find(TEXT("OutputFile"));
	}
	else
	{
		// 使用默认输出路径
		FString OutputDirectory = NePyGetDefaultDumpReflectionInfosDirectory();
		OutputFile = FPaths::Combine(*OutputDirectory, TEXT("blueprint_infos_full.json"));
	}

	NePyDumpBlueprintInfosToFile(OutputFile);

	return 0;
#else
	return -1;
#endif
}
