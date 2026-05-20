#include "NePyGameInstance.h"
#include "Tickable.h"
#include "NePyBase.h"
#include "NePyGIL.h"

void UNePyGameInstance::Init()
{
	Super::Init();

	if (PythonModule.IsEmpty())
	{
		UE_LOG(LogNePython, Error, TEXT("\"PythonModule\" of UNePyGameInstance is empty!"));
		return;
	}

	if (PythonClass.IsEmpty())
	{
		UE_LOG(LogNePython, Error, TEXT("\"PythonClass\" of UNePyGameInstance is empty!"));
		return;
	}

	{
		FNePyScopedGIL GIL;
		PyObject* SelfObj = NePyBase::ToPy(this);
		if (!SelfObj)
		{
			PyErr_Print();
			return;
		}

		FNePyObjectPtr PyGameModule = NePyStealReference(PyImport_ImportModule(TCHAR_TO_UTF8(*PythonModule)));
		if (!PyGameModule)
		{
			PyErr_Print();
			return;
		}

#if WITH_EDITOR
		PyGameModule = NePyStealReference(PyImport_ReloadModule(PyGameModule));
		if (!PyGameModule)
		{
			PyErr_Print();
			return;
		}
#endif

		PyObject* PyGameModuleDict = PyModule_GetDict(PyGameModule);
		PyObject* PyGameClass = PyDict_GetItemString(PyGameModuleDict, TCHAR_TO_UTF8(*PythonClass));
		if (!PyGameClass)
		{
			UE_LOG(LogNePython, Error, TEXT("Unable to find class %s in module %s"), *PythonClass, *PythonModule);
			return;
		}

		PyGameInstance = PyObject_CallObject(PyGameClass, nullptr);
		if (!PyGameInstance)
		{
			PyErr_Print();
			return;
		}

		PyObject_SetAttrString(PyGameInstance, (char*)"uobject", (PyObject*)SelfObj);

		if (!PyObject_HasAttrString(PyGameInstance, (char*)"init"))
		{
			return;
		}

		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, (char*)"init", nullptr));
		if (!PyRet)
		{
			PyErr_Print();
			return;
		}

		if (PyObject_HasAttrString(PyGameInstance, "on_pre_world_tick"))
		{
			PyGameInstanceOnPreWorldTick = PyObject_GetAttrString(PyGameInstance, "on_pre_world_tick");
		}

		if (PyObject_HasAttrString(PyGameInstance, "on_pre_actor_tick"))
		{
			PyGameInstanceOnPreActorTick = PyObject_GetAttrString(PyGameInstance, "on_pre_actor_tick");
		}

		if (PyObject_HasAttrString(PyGameInstance, "on_post_actor_tick"))
		{
			PyGameInstanceOnPostActorTick = PyObject_GetAttrString(PyGameInstance, "on_post_actor_tick");
		}

		if (PyObject_HasAttrString(PyGameInstance, "on_tick"))
		{
			PyGameInstanceOnTick = PyObject_GetAttrString(PyGameInstance, "on_tick");
		}
	}

	if (PyGameInstanceOnPreWorldTick)
	{
		check(!PreWorldTickDelegateHandler.IsValid());
		PreWorldTickDelegateHandler = FWorldDelegates::OnWorldTickStart.AddUObject(this, &UNePyGameInstance::PreWorldTick);
	}

	if (PyGameInstanceOnPreActorTick)
	{
		check(!PreActorTickDelegateHandler.IsValid());
		PreActorTickDelegateHandler = FWorldDelegates::OnWorldPreActorTick.AddUObject(this, &UNePyGameInstance::PreActorTick);
	}

	if (PyGameInstanceOnPostActorTick)
	{
		check(!PostActorTickDelegateHandler.IsValid());
		PostActorTickDelegateHandler = FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UNePyGameInstance::PostActorTick);
	}

	if (PyGameInstanceOnTick)
	{
#if ENGINE_MAJOR_VERSION >= 5
		TickDelegateHandler = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UNePyGameInstance::Tick), 0
		);
#else
		TickDelegateHandler = FTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UNePyGameInstance::Tick), 0
		);
#endif
	}
}

void UNePyGameInstance::Shutdown()
{
	Super::Shutdown();

	if (!PyGameInstance)
	{
		return;
	}

	if (PreWorldTickDelegateHandler.IsValid())
	{
		FWorldDelegates::OnWorldTickStart.Remove(PreWorldTickDelegateHandler);
	}

	if (PreActorTickDelegateHandler.IsValid())
	{
		FWorldDelegates::OnWorldPreActorTick.Remove(PreActorTickDelegateHandler);
	}

	if (PostActorTickDelegateHandler.IsValid())
	{
		FWorldDelegates::OnWorldPostActorTick.Remove(PostActorTickDelegateHandler);
	}

	if (TickDelegateHandler.IsValid())
	{
#if ENGINE_MAJOR_VERSION >= 5
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandler);
#else
		FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandler);
#endif
	}

	{
		FNePyScopedGIL GIL;

		if (PyObject_HasAttrString(PyGameInstance, (char*)"shutdown"))
		{
			PyObject* PyRet = PyObject_CallMethod(PyGameInstance, (char*)"shutdown", nullptr);
			if (!PyRet)
			{
				PyErr_Print();
			}
			else
			{
				Py_DECREF(PyRet);
			}
		}
	}

	ReleasePythonResources();
}

void UNePyGameInstance::OnStart()
{
	Super::OnStart();

	if (!PyGameInstance)
	{
		return;
	}

	FNePyScopedGIL GIL;
	if (!PyObject_HasAttrString(PyGameInstance, (char*)"on_start"))
	{
		return;
	}

	FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, (char*)"on_start", nullptr));
	if (!PyRet)
	{
		PyErr_Print();
		return;
	}
}

void UNePyGameInstance::ReleasePythonResources()
{
	FNePyScopedGIL GIL;

	if (PyGameInstanceOnPreActorTick)
	{
		Py_DECREF(PyGameInstanceOnPreActorTick);
		PyGameInstanceOnPreActorTick = nullptr;
	}

	if (PyGameInstanceOnPostActorTick)
	{
		Py_DECREF(PyGameInstanceOnPostActorTick);
		PyGameInstanceOnPostActorTick = nullptr;
	}

	if (PyGameInstanceOnTick)
	{
		Py_DECREF(PyGameInstanceOnTick);
		PyGameInstanceOnTick = nullptr;
	}

	// 释放持有的py_proxy
	if (PyGameInstance)
	{
		Py_DECREF(PyGameInstance);
		PyGameInstance = nullptr;
	}
}


void UNePyGameInstance::PreWorldTick(UWorld* World, ELevelTick Tick, float Delta)
{
	if (World != GetWorld())
	{
		return;
	}

	check(PyGameInstanceOnPreWorldTick)
	{
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(f)", Delta));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyGameInstanceOnPreWorldTick, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}
}

void UNePyGameInstance::PreActorTick(UWorld* World, ELevelTick Tick, float Delta)
{
	if (World != GetWorld())
	{
		return;
	}

	check(PyGameInstanceOnPreActorTick)
	{
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(f)", Delta));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyGameInstanceOnPreActorTick, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}
}

void UNePyGameInstance::PostActorTick(UWorld* World, ELevelTick Tick, float Delta)
{
	if (World != GetWorld())
	{
		return;
	}

	check(PyGameInstanceOnPostActorTick)
	{
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(f)", Delta));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyGameInstanceOnPostActorTick, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}
}

bool UNePyGameInstance::Tick(float DeltaSeconds)
{
	check(PyGameInstanceOnTick)
	{
		FNePyScopedGIL GIL;
		FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(f)", DeltaSeconds));
		FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyGameInstanceOnTick, PyArgs));
		if (!PyRet)
		{
			PyErr_Print();
		}
	}

	return true;
}

void UNePyGameInstance::CallPythonActorMethod(FString InMethodName, FString InArgs)
{
	if (!PyGameInstance)
	{
		return;
	}

	FNePyScopedGIL GIL;
	FNePyObjectPtr PyRet;
	if (InArgs.IsEmpty())
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, TCHAR_TO_UTF8(*InMethodName), nullptr));
	}
	else
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, TCHAR_TO_UTF8(*InMethodName), (char*)"s", TCHAR_TO_UTF8(*InArgs)));
	}

	if (!PyRet)
	{
		PyErr_Print();
		return;
	}
}

bool UNePyGameInstance::CallPythonActorMethodBool(FString InMethodName, FString InArgs)
{
	if (!PyGameInstance)
	{
		return false;
	}

	FNePyScopedGIL GIL;
	FNePyObjectPtr PyRet;
	if (InArgs.IsEmpty())
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, TCHAR_TO_UTF8(*InMethodName), nullptr));
	}
	else
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, TCHAR_TO_UTF8(*InMethodName), (char*)"s", TCHAR_TO_UTF8(*InArgs)));
	}
	if (!PyRet)
	{
		PyErr_Print();
		return false;
	}

	if (PyObject_IsTrue(PyRet))
	{
		return true;
	}

	return false;
}

FString UNePyGameInstance::CallPythonActorMethodString(FString InMethodName, FString InArgs)
{
	if (!PyGameInstance)
	{
		return FString();
	}

	FNePyScopedGIL GIL;
	FNePyObjectPtr PyRet;
	if (InArgs.IsEmpty())
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, TCHAR_TO_UTF8(*InMethodName), nullptr));
	}
	else
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, TCHAR_TO_UTF8(*InMethodName), (char*)"s", TCHAR_TO_UTF8(*InArgs)));
	}
	if (!PyRet)
	{
		PyErr_Print();
		return FString();
	}

	FNePyObjectPtr PyStr = NePyStealReference(PyObject_Str(PyRet));
	if (!PyStr)
	{
		return FString();
	}

	const char* StrValue = NePyString_AsString(PyStr);
	FString RetStr = FString(UTF8_TO_TCHAR(StrValue));

	return RetStr;
}

bool UNePyGameInstance::CallGameInstanceMethod(char* FunctionName, char* FormatList, ...)
{
	if (!PyGameInstance)
	{
		return false;
	}

	FNePyScopedGIL GIL;
	FNePyObjectPtr PyRet;
	if (FormatList == NULL || FormatList[0] == '\0')
	{
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, FunctionName, nullptr));
	}
	else
	{
		va_list args;
		va_start(args, FormatList);
		PyRet = NePyStealReference(PyObject_CallMethod(PyGameInstance, FunctionName, FormatList, args));
		va_end(args);
	}
	if (!PyRet)
	{
		PyErr_Print();
		return false;
	}

	return true;
}
