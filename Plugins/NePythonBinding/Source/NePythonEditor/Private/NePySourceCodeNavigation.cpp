#include "NePySourceCodeNavigation.h"
#include "NePyGIL.h"
#include "NePyPtr.h"
#include "NePyBase.h"
#include "NePyGeneratedClass.h"
#include "NePyGeneratedStruct.h"
#include "SourceCodeNavigation.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "Internationalization/Regex.h"
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "NePySourceCodeNavigation"

// 为Python生成类提供跳转至源码的功能
class FNePySourceCodeNavigation : public ISourceCodeNavigationHandler
{
public:
	FNePySourceCodeNavigation()
	{
		InitializeVSCodeURL();
	}

	virtual bool CanNavigateToClass(const UClass* InClass) override
	{
		return ((const UObject*)InClass)->IsA<UNePyGeneratedClass>();
	}

	virtual bool NavigateToClass(const UClass* InClass) override
	{
		const UNePyGeneratedClass* Class = Cast<UNePyGeneratedClass>(InClass);
		if (!Class)
		{
			return false;
		}

		NavigateToSource(Class, (PyObject*)Class->PyType);
		return true;
	}

	virtual bool CanNavigateToStruct(const UScriptStruct* InStruct) override
	{
		return (InStruct)->IsA<UNePyGeneratedStruct>();
	}

	virtual bool NavigateToStruct(const UScriptStruct* InStruct) override
	{
		const UNePyGeneratedStruct* Struct = Cast<UNePyGeneratedStruct>(InStruct);
		if (!Struct)
		{
			return false;
		}

		NavigateToSource(Struct, (PyObject*)Struct->PyType);
		return true;
	}

	virtual bool CanNavigateToFunction(const UFunction* InFunction)
	{
		return (InFunction->GetOuter()->IsA<UNePyGeneratedClass>());
	}

	virtual bool NavigateToFunction(const UFunction* InFunction)
	{
		const UNePyGeneratedFunction* Function = Cast<UNePyGeneratedFunction>(InFunction);
		if (!Function)
		{
			return false;
		}

		if (Function->PyFunc)
		{
			NavigateToSource(InFunction, Function->PyFunc);
		}
		return true;
	}

	virtual bool CanNavigateToStruct(const UStruct* InStruct) override
	{
		if (const UClass* Class = Cast<UClass>(InStruct))
		{
			return CanNavigateToClass(Class);
		}
		if (const UScriptStruct* Struct = Cast<UScriptStruct>(InStruct))
		{
			return CanNavigateToStruct(Struct);
		}
		return false;
	}

	virtual bool NavigateToStruct(const UStruct* InStruct) override
	{
		if (const UClass* Class = Cast<UClass>(InStruct))
		{
			return NavigateToClass(Class);
		}
		if (const UScriptStruct* Struct = Cast<UScriptStruct>(InStruct))
		{
			return NavigateToStruct(Struct);
		}
		return false;
	}

private:
	void NavigateToSource(const UObject* Object, PyObject* PyObj)
	{
		if (!PyObj)
		{
			WarnError(Object, LOCTEXT("PyObjectInvalid", "python object is invalid"));
			return;
		}

		FString SourceFile;
		int32 LineNumber;
		{
			FNePyScopedGIL GIL;

			FNePyObjectPtr PyInspectModule = NePyStealReference(PyImport_ImportModule("inspect"));
			if (!PyInspectModule)
			{
				WarnError(Object, LOCTEXT("InspectNotFound", "module 'inspect' not found"));
				return;
			}

			PyObject* PyInspectDict = PyModule_GetDict(PyInspectModule);
			PyObject* PyGetFileFunc = PyDict_GetItemString(PyInspectDict, "getfile");
			if (!PyGetFileFunc)
			{
				WarnError(Object, LOCTEXT("FailedToFindFile", "failed to find source file"));
				return;
			}

			FNePyObjectPtr PyGetFileResult = NePyStealReference(PyObject_CallFunctionObjArgs(PyGetFileFunc, PyObj, nullptr));
			if (!PyGetFileResult)
			{
				WarnError(Object, LOCTEXT("FailedToFindFile", "failed to find source file"));
				return;
			}

			SourceFile = NePyBase::PyObjectToString(PyGetFileResult);
			if (SourceFile.IsEmpty())
			{
				WarnError(Object, LOCTEXT("FailedToFindFile", "failed to find source file"));
				return;
			}

			SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
			if (!FPaths::FileExists(SourceFile))
			{
				WarnError(Object, FText::Format(LOCTEXT("FileNotExist", "file '{0}' not exist"), FText::FromString(SourceFile)));
				return;
			}

			PyObject* PyFindSourceFunc = PyDict_GetItemString(PyInspectDict, "findsource");
			if (!PyFindSourceFunc)
			{
				WarnError(Object, LOCTEXT("FailedToFindLineNumber", "failed to find line number"));
				return;
			}

			FNePyObjectPtr PyFindSourceResult = NePyStealReference(PyObject_CallFunctionObjArgs(PyFindSourceFunc, PyObj, nullptr));
			if (!PyFindSourceResult || !PyTuple_Check(PyFindSourceResult) || PyTuple_Size(PyFindSourceResult) != 2)
			{
				WarnError(Object, LOCTEXT("FailedToFindLineNumber", "failed to find line number"));
				return;
			}

			PyObject* PyLineNumber = PyTuple_GetItem(PyFindSourceResult, 1);
			if (!NePyBase::ToCpp(PyLineNumber, LineNumber))
			{
				WarnError(Object, LOCTEXT("FailedToFindLineNumber", "failed to find line number"));
				return;
			}
		}

		// 首先尝试使用 Visual Studio Code 打开脚本源码
		bool bResult = false;
		if (!VSCodeURL.IsEmpty())
		{
			FProcHandle hProcess = FPlatformProcess::CreateProc(
				*VSCodeURL,
				*FString::Printf(TEXT("--goto \"%s:%d\""), *SourceFile, LineNumber),
				true, false, false,
				nullptr, 0, nullptr,
				nullptr, nullptr);
			bResult = hProcess.IsValid();
		}

		// 用户没有安装 Visual Studio Code，调用UE默认的编辑器打开（一般来说是 Visual Studio）
		if (!bResult)
		{
			FSourceCodeNavigation::OpenSourceFile(SourceFile, LineNumber);
		}
	}

	void WarnError(const UObject* Object, const FText& InReason)
	{
		FText Text = FText::Format(LOCTEXT("FailedToNavigate", "Can't open source code of '{0}', {1}."),
			FText::FromName(Object->GetFName()), InReason);

		FNotificationInfo Info(Text);
		Info.ExpireDuration = 2;
#if ENGINE_MAJOR_VERSION >= 5
		TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
		Notification->SetCompletionState(SNotificationItem::CS_Fail);
#else
		Info.Image = FCoreStyle::Get().GetBrush("NotificationList.FailImage");
		FSlateNotificationManager::Get().AddNotification(Info);
#endif // ENGINE_MAJOR_VERSION >= 5
	}

	void InitializeVSCodeURL()
	{
#if PLATFORM_WINDOWS
		FString IDEPath;

		if (!FWindowsPlatformMisc::QueryRegKey(HKEY_CURRENT_USER, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command\\"), TEXT(""), IDEPath))
		{
			FWindowsPlatformMisc::QueryRegKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command\\"), TEXT(""), IDEPath);
		}

		FString PatternString(TEXT("\"(.*)\" \".*\""));
		FRegexPattern Pattern(PatternString);
		FRegexMatcher Matcher(Pattern, IDEPath);
		if (Matcher.FindNext())
		{
			FString URL = Matcher.GetCaptureGroup(1);
			if (FPaths::FileExists(URL))
			{
				VSCodeURL = URL;
			}
		}
#endif
	}

private:
	FString VSCodeURL;
};

static FNePySourceCodeNavigation* NePySourceCodeNavigation = nullptr;

void NePyRegisterSourceNavigation()
{
	if (!NePySourceCodeNavigation)
	{
		NePySourceCodeNavigation = new FNePySourceCodeNavigation();
		FSourceCodeNavigation::AddNavigationHandler(NePySourceCodeNavigation);
	}
}

void NePyUnregisterSourceNavigation()
{
	if (NePySourceCodeNavigation)
	{
		FSourceCodeNavigation::RemoveNavigationHandler(NePySourceCodeNavigation);
		delete NePySourceCodeNavigation;
		NePySourceCodeNavigation = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
