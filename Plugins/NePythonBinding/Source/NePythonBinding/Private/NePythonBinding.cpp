#include "NePythonBinding.h"

#include <fcntl.h>
#include <string>
#include <signal.h>

#include "NePyIncludePython.h"
#include "NePyGIL.h"
#include "NePyBase.h"
#include "NePyHouseKeeper.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyWrapperInitializer.h"
#include "NePyTopModule.h"
#include "NePyObjectBase.h"
#include "NePyObject.h"
#include "NePyClass.h"
#include "NePyEnum.h"
#include "NePySubclass.h"
#include "NePyInterface.h"
#include "NePyStructBase.h"
#include "NePyUserStruct.h"
#include "NePyTableRowBase.h"
#include "NePyUserTableRow.h"
#include "NePyEnumBase.h"
#include "NePyFieldPath.h"
#include "NePyArrayWrapper.h"
#include "NePyFixedArrayWrapper.h"
#include "NePyMapWrapper.h"
#include "NePySetWrapper.h"
#include "NePyDynamicDelegateWrapper.h"
#include "NePyDynamicMulticastDelegateWrapper.h"
#include "NePyDescriptor.h"
#include "NePyTickerHandle.h"
#include "NePyTimerHandle.h"
#include "NePyTimerManagerWrapper.h"
#include "NePyObjectRef.h"
#include "NePyCallable.h"
#include "NePyOutputDevice.h"
#include "NePyScriptStruct.h"
#include "NePyGenUtil.h"
#include "NePyGeneratedType.h"
#include "NePyFileMonitor.h"
#include "NePyAutoExport.h"
#include "NePyImporter.h"
#include "NePyInsightProfiler.h"
#include "NePyMemoryAllocator.h"
#include "NePyBlueprintActionDatabaseHelper.h"
#include "NePyRuntimeDelegates.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "UObject/UObjectIterator.h"
#include "NePySoftPtr.h"
#include "NePyWeakPtr.h"
#include "NePySpecifiers.h"
#include "NePyCallInfo.h"
#include "HAL/IConsoleManager.h"
#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR

// 当需要打通Insight的Python栈时，开启这个宏为1
#define NEPY_ENABLE_INSIGHT_PROFILER 0

#define LOCTEXT_NAMESPACE "FNePythonBindingModule"

static TAutoConsoleVariable<int32> CVarEnableNePyCrashHandler(
	TEXT("nepy.EnableCrashHandler"),
	0,
	TEXT("If > 0, enables the NePythonBinding crash handler to print Python call stacks on system errors and ensures."),
	ECVF_Default);

#if PLATFORM_MAC
TMap<int32, struct sigaction> SignalActions;
#endif

void SignalHandler(int32 Signal)
{
	FNePythonBindingModule& PythonModule = FModuleManager::GetModuleChecked<FNePythonBindingModule>("NePythonBinding");
	PythonModule.LogPythonCallStackSafe();
#if PLATFORM_MAC
	sigaction(Signal, &SignalActions[Signal], nullptr);
#endif
}

// 注册一个console指令执行器，用于在Consloe及OutputLog窗口中执行python指令
static bool NePythonCustomConsoleExec(UWorld* InWorld, const TCHAR* InCmd, FOutputDevice& InDevice)
{
	bool bIsHandled = false;
	if (FNePythonBindingModule* PythonModule = FModuleManager::GetModulePtr<FNePythonBindingModule>("NePythonBinding"))
	{
		bIsHandled = PythonModule->TryRunString(TCHAR_TO_UTF8(InCmd));
	}

	return bIsHandled;
}
FStaticSelfRegisteringExec NePythonCustomConsoleExecRegistration(NePythonCustomConsoleExec);

void FNePythonBindingModule::StartupModule()
{
	if (CVarEnableNePyCrashHandler.GetValueOnAnyThread() > 0)
	{
		FCoreDelegates::OnHandleSystemError.AddRaw(this, &FNePythonBindingModule::OnSystemError);
		FCoreDelegates::OnHandleSystemEnsure.AddRaw(this, &FNePythonBindingModule::OnSystemError);
		static TSet<int32> SignalTypes = {
			// interrupt
			SIGINT,
			// illegal instruction - invalid function image
			SIGILL,
			// floating point exception
			SIGFPE,
			// segment violation
			SIGSEGV,
			// Software termination signal from kill
			SIGTERM,
	#if PLATFORM_WINDOWS
			// Ctrl-Break sequence
			SIGBREAK,
	#endif
			// abnormal termination triggered by abort call
			SIGABRT,
		};
		for (const auto SignalType : SignalTypes)
		{
#if PLATFORM_MAC
			struct sigaction SigAction;
			FMemory::Memzero(&SigAction, sizeof(struct sigaction));
			SigAction.sa_handler = SignalHandler;
			sigemptyset(&SigAction.sa_mask);
			if (!SignalActions.Contains(SignalType))
			{
				sigaction(SignalType, &SigAction, &SignalActions.Add(SignalType));
			}
			else
			{
				sigaction(SignalType, &SigAction, nullptr);
			}
#else
			signal(SignalType, SignalHandler);
#endif
		}
	}
	RuntimeDelegateContainer = NewObject<UNePyRuntimeDelegates>(GetTransientPackage(), TEXT("RuntimeDelegates"));
	RuntimeDelegateContainer->AddToRoot();
	FNePyHouseKeeper::InitSingleton();
	InitializePython();
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FNePythonBindingModule::OnPostEngineInit);
	FCoreDelegates::OnPreExit.AddRaw(this, &FNePythonBindingModule::OnPreExit);
}

void FNePythonBindingModule::ShutdownModule()
{
	FCoreDelegates::OnPreExit.RemoveAll(this);
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);
	ShutdownPython();
	// 退出时Unreal已经销毁Root对象，无需手动销毁
	RuntimeDelegateContainer = nullptr;
	if (CVarEnableNePyCrashHandler.GetValueOnAnyThread() > 0)
	{
		FCoreDelegates::OnHandleSystemEnsure.RemoveAll(this);
		FCoreDelegates::OnHandleSystemError.RemoveAll(this);
	}
}

void FNePythonBindingModule::InitPythonModules()
{
	PyObject* PyModule = NePyInitTopModule(NePyRootModuleName);
	Py_INCREF(PyModule);
	PyRootModule = PyModule;

	FString InternalModuleName = FString(UTF8_TO_TCHAR(NePyRootModuleName)) + FString(TEXT(".internal"));
	PyInternalModule = PyImport_AddModule(TCHAR_TO_UTF8(*InternalModuleName));
	Py_INCREF(PyInternalModule);
	PyModule_AddObject(PyRootModule, "internal", PyInternalModule);
	Py_INCREF(PyInternalModule);

	PyObject* PyModuleDict = PyModule_GetDict(PyModule);

#if UE_BUILD_SHIPPING
	PyDict_SetItemString(PyModuleDict, "UE_SHIPPING", Py_True);
#else
	PyDict_SetItemString(PyModuleDict, "UE_SHIPPING", Py_False);
#endif
	PyDict_SetItemString(PyModuleDict, "GIsEditor", FNePyObjectPtr::StealReference(PyBool_FromLong(GIsEditor)));
	PyDict_SetItemString(PyModuleDict, "GIsClient", FNePyObjectPtr::StealReference(PyBool_FromLong(GIsClient)));
	PyDict_SetItemString(PyModuleDict, "GIsServer", FNePyObjectPtr::StealReference(PyBool_FromLong(GIsServer)));

#if PLATFORM_WINDOWS
	NePyInitFileMonitor(PyModule);
#endif

	// !!! 描述器应最早创建，否则后续各种Init流程可能用到描述器会因为TypeObject不完整导致GC报错 !!!
	FNePyPropertyDescriptor::InitPyType();
	FNePyFunctionDescriptorBase::InitPyType();
	FNePyFunctionDescriptor::InitPyType();

	NePyInitStructBase(PyModule);
	NePyInitUserStruct(PyModule);
	NePyInitTableRowBase(PyModule);
	NePyInitUserTableRow(PyModule);
	NePyInitObjectBase(PyModule);
	NePyInitObject(PyModule);
	NePyInitClass(PyModule);
	NePyInitSubclass(PyModule);
	NePyInitInterface(PyModule);
	NePyInitEnum(PyModule);
	NePyInitEnumBase(PyModule);
	FNePyFieldPath::InitPyType(PyModule);
	FNePyArrayWrapper::InitPyType(PyModule);
	FNePyFixedArrayWrapper::InitPyType();
	FNePyMapWrapper::InitPyType();
	FNePySetWrapper::InitPyType();
	FNePyDynamicDelegateWrapper::InitPyType();
	FNePyDynamicDelegateParam::InitPyType();
	FNePyDynamicDelegateArg::InitPyType();
	FNePyDynamicMulticastDelegateWrapper::InitPyType();
	NePyGenUtil::FMethodDef::InitPyType();
	FNePyStaticFunctionDescriptor::InitPyType();
	FNePyGeneratedFunctionDescriptor::InitPyType();
	FNePyGeneratedStaticFunctionDescriptor::InitPyType();
	NePyInitTickerHandle(PyModule);
	NePyInitTimerHandle(PyModule);
	FNePyTimerManagerWrapper::InitPyType();
	NePyInitObjectRef(PyModule);
	FNePyCallable::InitPyType();
	NePyInitOutputDevice(PyModule);
	NePyInitScriptStruct(PyModule);
	FNePySoftPtr::InitPyType(PyModule);
	FNePyWeakPtr::InitPyType(PyModule);
	NePyInitGeneratedTypes(PyModule, PyInternalModule);
	NePyAutoExport

	FNePythonModuleDelegates::OnInitPythonModule.Broadcast(PyModule); // 通知其它模块进行InitPythonModules

	FNePyWrapperInitializer::Get().GenerateWrappedTypes();

	PyObject* PyDelegateContainer = NePyBase::ToPy(RuntimeDelegateContainer);
	PyDict_SetItemString(PyModuleDict, "GRuntimeDelegates", PyDelegateContainer);
}

PyObject* FNePythonBindingModule::GetPythonRootModule()
{
	return PyRootModule;
}


void FNePythonBindingModule::AppendPythonTopModuleMethods(PyMethodDef PyTopModuleMethods[])
{
	if (!PyRootModule)
	{
		UE_LOG(LogNePython, Error, TEXT("AppendPythonTopModuleMethods Error! PyRootModule is nullptr"));
		return;
	}

	const char* RootModuleName = PyModule_GetName(PyRootModule);
	FNePyObjectPtr PyRootModuleName = NePyStealReference(NePyString_FromString(RootModuleName));
	PyObject* PyRootModuleDict = PyModule_GetDict(PyRootModule);

	for (PyMethodDef* MethodPtr = PyTopModuleMethods; MethodPtr->ml_name != nullptr; MethodPtr++)
	{
		// 模块上的方法不可以标记为METH_CLASS或METH_STATIC
		check(!((MethodPtr->ml_flags & METH_CLASS) || (MethodPtr->ml_flags & METH_STATIC)));

		FNePyObjectPtr PyFunc = NePyStealReference(PyCFunction_NewEx(MethodPtr, nullptr, PyRootModuleName));
		if (!PyFunc)
		{
			UE_LOG(LogNePython, Error, TEXT("AppendPythonTopModuleMethods Error! PyFunc(%s) is nullptr"), UTF8_TO_TCHAR(MethodPtr->ml_name));
			continue;
		}
		if (PyDict_SetItemString(PyRootModuleDict, MethodPtr->ml_name, PyFunc) != 0)
		{
			UE_LOG(LogNePython, Error, TEXT("AppendPythonTopModuleMethods Error! PyDict_SetItemString(%s) return fail"), UTF8_TO_TCHAR(MethodPtr->ml_name));
			continue;
		}
	}
}

void FNePythonBindingModule::RunString(const char* InStr)
{
	FNePyScopedGIL GIL;

	RuntimeDelegateContainer->OnDebugInput.Broadcast(FString(UTF8_TO_TCHAR(InStr)));

	if (PyOnDebugInputFunc)
	{
		// 如果有脚本回调，则优先使用脚本回调
		FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(s)", InStr));
		FNePyObjectPtr PyResult = NePyStealReference(PyObject_CallObject(PyOnDebugInputFunc, PyArgs));
		if (!PyResult)
		{
			PyErr_Print();
			return;
		}
		else if (PyObject_IsTrue(PyResult))
		{
			return;
		}
	}

	{
		// 没有脚本回调，或脚本回调没有返回True，则使用默认的eval
		// 自动回显结果（Py_eval_input），如果是声明/import等则使用 Py_file_input
		FNePyObjectPtr PyResult = NePyStealReference(PyRun_String(InStr, Py_eval_input, (PyObject*)PyMainDict, (PyObject*)PyMainDict));
		if (!PyResult)
		{
			if (PyErr_ExceptionMatches(PyExc_SyntaxError))
			{
				PyErr_Clear();
				PyResult = NePyStealReference(PyRun_String(InStr, Py_file_input, (PyObject*)PyMainDict, (PyObject*)PyMainDict));
			}
			if (PyErr_ExceptionMatches(PyExc_SystemExit))
			{
				PyErr_Clear();	
			}
			else if(!PyResult)
			{
				PyErr_Print();
			}
		}
		else
		{
			FString ResStr = NePyBase::PyObjectToString(PyResult);
			UE_LOG(LogNePython, Log, TEXT("%s"), *ResStr);
		}
	}
}

bool FNePythonBindingModule::TryRunString(const char* InStr)
{
	FNePyScopedGIL GIL;

	if (PyOnDebugInputFunc)
	{
		FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(s)", InStr));
		FNePyObjectPtr PyResult = NePyStealReference(PyObject_CallObject(PyOnDebugInputFunc, PyArgs));
		if (!PyResult)
		{
			PyErr_Print();
			return false;
		}
		else if (PyObject_IsTrue(PyResult))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
bool FNePythonBindingModule::RunFile(const TCHAR* InFile, const TCHAR* InArgs)
{
	if (FString(InFile).IsEmpty())
	{
		UE_LOG(LogNePython, Error, TEXT("Python script file not specified."));
		return false;
	}

	// 从PythonScriptPlugin抄过来的，确定脚本文件绝对路径
	auto ResolveFilePath = [this, InFile]() -> FString
	{
		// Favor the CWD
		if (FPaths::FileExists(InFile))
		{
			return FPaths::ConvertRelativePathToFull(InFile);
		}

		// Execute Python code within this block
		{
			FNePyScopedGIL GIL;

			// Then test against each system path in order (as Python would)
			const TArray<FString> PySysPaths = GetPythonSysPaths();
			for (const FString& PySysPath : PySysPaths)
			{
				const FString PotentialFilePath = PySysPath / InFile;
				if (FPaths::FileExists(PotentialFilePath))
				{
					return PotentialFilePath;
				}
			}
		}

		// Didn't find a match... we know this file doesn't exist, but we'll use this path in the error reporting
		return FPaths::ConvertRelativePathToFull(InFile);
	};

	const FString ResolvedFilePath = ResolveFilePath();

	FString CodeString;
	bool bLoaded = FFileHelper::LoadFileToString(CodeString, *ResolvedFilePath);
	if (!bLoaded)
	{
		FString ErrorMsg = FString::Printf(TEXT("Could not load Python file '%s' (resolved from '%s')"), *ResolvedFilePath, InFile);
		UE_LOG(LogNePython, Error, TEXT("%s"), *ErrorMsg);
		return false;
	}

	// 执行外部文件前，设置一下执行环境（例如sys.argv、sys.path）
	// 执行完毕后恢复
	struct FScopedRunFileEnv
	{
		FScopedRunFileEnv(FNePythonBindingModule* InOuter, const TCHAR* InCommandLine, const FString& ResolvedFilePath)
			: Outer(InOuter)
			, OldSysPaths(InOuter->GetPythonSysPaths())
		{
			Outer->SetPythonSysArgv(InCommandLine);

			TArray<FString> NewSysPaths = OldSysPaths;
			NewSysPaths.Insert(FPaths::GetPath(ResolvedFilePath), 0);
			Outer->SetPythonSysPaths(NewSysPaths);
		}
		
		~FScopedRunFileEnv()
		{
			const TCHAR* CommandLine = FCommandLine::GetOriginal();
			Outer->SetPythonSysArgv(CommandLine);

			Outer->SetPythonSysPaths(OldSysPaths);
		}

		FNePythonBindingModule* Outer;
		TArray<FString> OldSysPaths;
	};

	{
		FNePyScopedGIL GIL;
		FScopedRunFileEnv Env(this, InArgs, ResolvedFilePath);

		FNePyObjectPtr PyFileDict = NePyStealReference(PyDict_Copy(PyMainDict));
		FNePyObjectPtr PyResolvedFilePath = NePyStealReference(NePyString_FromString(TCHAR_TO_UTF8(*ResolvedFilePath)));
		PyDict_SetItemString(PyFileDict, "__file__", PyResolvedFilePath);

		FNePyObjectPtr PyCode = NePyStealReference(Py_CompileString(TCHAR_TO_UTF8(*CodeString), TCHAR_TO_UTF8(*ResolvedFilePath), Py_file_input));
		if (!PyCode)
		{
			PyErr_Print();
			return false;
		}

#if PY_MAJOR_VERSION < 3
		FNePyObjectPtr PyResult = NePyStealReference(PyEval_EvalCode((PyCodeObject*)PyCode.Get(), PyFileDict, PyFileDict));
#else
		FNePyObjectPtr PyResult = NePyStealReference(PyEval_EvalCode(PyCode, PyFileDict, PyFileDict));
#endif
		if (!PyResult)
		{
			PyErr_Print();
			return false;
		}

		FNePyObjectPtr PyResultStr = NePyStealReference(PyObject_Repr(PyResult));
		const char* ResultStr = NePyString_AsString(PyResultStr);
		UE_LOG(LogNePython, Log, TEXT("%s"), UTF8_TO_TCHAR(ResultStr));
	}

	return true;
}
#endif // WITH_EDITOR

UNePyRuntimeDelegates* FNePythonBindingModule::GetRuntimeDelegates() const
{
	return RuntimeDelegateContainer;
}

#ifdef USE_NGTECH_PYTHON
extern "C" void init_python_ext(void);
#endif

// 逻辑参考UnrealEnginePython插件
// 逻辑还参考了官方Python插件 FPythonScriptPlugin::InitializePython()
void FNePythonBindingModule::InitializePython()
{
	FNePyMemoryAllocator::Get().Initialize();

	// 确定python脚本目录
	// 首先尝试使用项目目录外的py源码
	FString PythonScriptPath = FPaths::Combine(*FPaths::ProjectDir(), UTF8_TO_TCHAR("../script"));
	bNeedRedirect = false;
	if (!FPaths::DirectoryExists(PythonScriptPath))
	{
		// 然后看看项目目录下有没有py源码，这是策划的需求，他们想要看见源码
		PythonScriptPath = FPaths::Combine(*FPaths::ProjectDir(), UTF8_TO_TCHAR("RawScripts"));
		if (!FPaths::DirectoryExists(PythonScriptPath))
		{
			// 找不到则使用项目内的pyc文件
			PythonScriptPath = FPaths::Combine(*FPaths::ProjectContentDir(), UTF8_TO_TCHAR("Scripts"));
			bNeedRedirect = true;
		}
	}

	FString PythonScriptPathOverride;
	if (GConfig->GetString(TEXT("NEPY"), TEXT("PythonScriptPath"), PythonScriptPathOverride, GGameIni))
	{
		PythonScriptPath = FPaths::Combine(*FPaths::ProjectDir(), PythonScriptPathOverride);
	}

	bool bNeedRedirectOverride;
	if (GConfig->GetBool(TEXT("NEPY"), TEXT("NeedRedirect"), bNeedRedirectOverride, GGameIni))
	{
		bNeedRedirect = bNeedRedirectOverride;
	}

	PythonScriptPath = FPaths::ConvertRelativePathToFull(PythonScriptPath);

#if PY_MAJOR_VERSION >= 3
	// Python 3 changes the console mode from O_TEXT to O_BINARY which affects other uses of the console
	// So change the console mode back to its current setting after Py_Initialize has been called
#if PLATFORM_WINDOWS
	// We call _setmode here to cache the current state
	CA_SUPPRESS(6031)
	fflush(stdin);
	const int StdInMode  = _setmode(_fileno(stdin), _O_TEXT);
	CA_SUPPRESS(6031)
	fflush(stdout);
	const int StdOutMode = _setmode(_fileno(stdout), _O_TEXT);
	CA_SUPPRESS(6031)
	fflush(stderr);
	const int StdErrMode = _setmode(_fileno(stderr), _O_TEXT);
#endif	// PLATFORM_WINDOWS
#endif
	
#if PY_MAJOR_VERSION >= 3
#if PY_MINOR_VERSION >= 11
	PyConfig config;
	PyStatus initStatus;
	PyConfig_InitPythonConfig(&config);
	config.module_search_paths_set = 1;
	config.site_import = 0;
	config.use_environment = 0;
	config.write_bytecode = 0;
#if WITH_EDITOR
	// 防止os模块被frozen，导致无法取得os.__file__，导致脚本断点调试器失效
	config.use_frozen_modules = 0;
#endif // WITH_EDITOR
	PyWideStringList_Append(&config.module_search_paths, TCHAR_TO_WCHAR(*PythonScriptPath));
	PyWideStringList_Append(&config.module_search_paths, TCHAR_TO_WCHAR(*FPaths::Combine(*PythonScriptPath, UTF8_TO_TCHAR("Lib"))));
	PyWideStringList_Append(&config.module_search_paths, TCHAR_TO_WCHAR(*FPaths::Combine(*PythonScriptPath, UTF8_TO_TCHAR("lib"))));
#else
	Py_SetPythonHome(TCHAR_TO_WCHAR(*PythonScriptPath));
	// 这里设置一下PYTHONPATH="{PythonScriptPath}/Lib"
	// 因为非Win平台默认会定位到"{PythonScriptPath}/lib/python{PY_MAJOR_VERSION}.{PY_MINOR_VERSION}"目录
	Py_SetPath(TCHAR_TO_WCHAR(*FPaths::Combine(*PythonScriptPath, UTF8_TO_TCHAR("Lib"))));
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_DontWriteBytecodeFlag = 1;
#endif //endif py3.11

#else
	// python2需要设置PythonHome，才能正确加载.pyd
	Py_SetPythonHome(TCHAR_TO_UTF8(*PythonScriptPath));
	UE_LOG(LogNePython, Warning, TEXT("Py_SetPythonHome %s"), *PythonScriptPath);
	Py_NoSiteFlag = 1;
	Py_IgnoreEnvironmentFlag = 1;
	Py_DontWriteBytecodeFlag = 1;
#endif

	bool bPyIsInitializedBefore = false;
	PyGILState_STATE PyGILState = PyGILState_LOCKED;
	if (Py_IsInitialized())
	{
		// 与其它python插件（例如PythonScriptPlugin）共用同一个虚拟机
		// 且虚拟机已被其它插件初始化过了
		bPyIsInitializedBefore = true;
		PyGILState = PyGILState_Ensure();

#if PY_MAJOR_VERSION >= 3
		if (bNeedRedirect)
		{
			NePyImporterInit();
		}
#endif
	}
	else
	{
#if PY_MAJOR_VERSION >= 3
		if (bNeedRedirect)
		{
			NePyInjectImporterToInittab();
		}
#endif

#ifdef USE_NGTECH_PYTHON
		init_python_ext();
#endif

#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11)
		initStatus = Py_InitializeFromConfig(&config);
#else
		Py_Initialize();
#endif

#if PY_MAJOR_VERSION >= 3
#if PLATFORM_WINDOWS
		// We call _setmode here to restore the previous state
		// Restore stdio state after Py_Initialize set it to O_BINARY, otherwise
		// everything that the engine will output is going to be encoded in UTF-16.
		// The behaviour is described here: https://bugs.python.org/issue16587
		if (StdInMode != -1)
		{
			CA_SUPPRESS(6031)
			fflush(stdin);
			CA_SUPPRESS(6031)
			_setmode(_fileno(stdin), StdInMode);
		}
		if (StdOutMode != -1)
		{
			CA_SUPPRESS(6031)
			fflush(stdout);
			CA_SUPPRESS(6031)
			_setmode(_fileno(stdout), StdOutMode);
		}
		if (StdErrMode != -1)
		{
			CA_SUPPRESS(6031)
			fflush(stderr);
			CA_SUPPRESS(6031)
			_setmode(_fileno(stderr), StdErrMode);
		}
#endif	// PLATFORM_WINDOWS
#endif
		if (!Py_IsInitialized())
		{
			UE_LOG(LogNePython, Fatal, TEXT("Python VM initialize failed"));
		}

#if PY_MAJOR_VERSION < 3 || PY_MINOR_VERSION < 9
		PyEval_InitThreads();
#endif
	}

#if PY_MAJOR_VERSION < 3
	if (bNeedRedirect)
	{
		NePyImporterInit();
	}
#endif

	// 设置sys.path
	{
		TArray<FString> ScriptsPaths;
		ScriptsPaths.Add(PythonScriptPath);
		ScriptsPaths.Add(FPaths::Combine(*PythonScriptPath, UTF8_TO_TCHAR("Lib")));
		SetPythonSysPaths(ScriptsPaths);
	}

	// 解析命令行参数，传递给python
	{
		const TCHAR* CommandLine = FCommandLine::GetOriginal();
		SetPythonSysArgv(CommandLine);
	}

	UE_LOG(LogNePython, Log, TEXT("Python VM initialized: %s"), UTF8_TO_TCHAR(Py_GetVersion()));

	PyMainModule = PyImport_AddModule("__main__");
	Py_INCREF(PyMainModule);
	PyMainDict = PyModule_GetDict(PyMainModule);
	Py_INCREF(PyMainDict);

	FNePySpecifier::InitializeSpecifierDefaults();
	InitPythonModules();

	// 提供默认的stdout/stderr实现
	{
		check(!PyErr_Occurred());
		char const* CodeStr = "import sys\n"
			"import " NePyRootModuleName " as ue\n"
			"class NePythonBindingOutput:\n"
			"    def __init__(self, logger):\n"
			"        self.logger = logger\n"
			"        self.buffer = []\n"
			"    def write(self, data):\n"
			"        if data == '\\n':\n"
			"            self.flush()\n"
			"        elif data.endswith('\\n'):\n"
			"            self.buffer.append(data[:-1])\n"
			"            self.flush()\n"
			"        else:\n"
			"            self.buffer.append(data)\n"
			"    def flush(self):\n"
			"        buffer = self.buffer\n"
			"        self.buffer = []\n"
			"        self.logger(''.join(buffer))\n"
			"    def isatty(self):\n"
			"        return False\n"
			"sys.stdout = NePythonBindingOutput(ue.Log)\n"
			"sys.stderr = NePythonBindingOutput(ue.LogError)";
		PyRun_SimpleString(CodeStr);
	}

	// 运行builtin script
	NePyRunBuiltinScript();

#if WITH_EDITOR
	FNePyWrapperTypeRegistry::Get().RegisterBlueprintDelegates();
#endif

	bNeedRunPatch = false;
	PatchModuleName = "nepypatch";
	PyPatchModule = PyImport_ImportModule(PatchModuleName);

	if (PyPatchModule)
	{
		UE_LOG(LogNePython, Log, TEXT("%s Python patch module successfully imported"), UTF8_TO_TCHAR(PatchModuleName));

		bool bBrutalPatchExceptionExitOverride = false;
		if (GConfig->GetBool(TEXT("NEPY"), TEXT("BrutalPatchExceptionExit"), bBrutalPatchExceptionExitOverride, GGameIni))
		{
			bBrutalPatchExceptionExit = bBrutalPatchExceptionExitOverride;
		}

		// 获取 should_continue 函数
		PyPatchShouldContinueFunc = PyObject_GetAttrString(PyPatchModule, "should_continue");
		if (PyPatchShouldContinueFunc && PyCallable_Check(PyPatchShouldContinueFunc))
		{
			UE_LOG(LogNePython, Log, TEXT("Found callable should_continue function in %s"), UTF8_TO_TCHAR(PatchModuleName));
		}
		else
		{
			if (PyPatchShouldContinueFunc)
			{
				Py_DECREF(PyPatchShouldContinueFunc);
				PyPatchShouldContinueFunc = nullptr;
			}
			PyErr_Clear();
			UE_LOG(LogNePython, Log, TEXT("should_continue function not found or not callable in %s"), UTF8_TO_TCHAR(PatchModuleName));
		}

		// 获取 on_run 函数
		PyPatchRunFunc = PyObject_GetAttrString(PyPatchModule, "on_run");
		if (PyPatchRunFunc && PyCallable_Check(PyPatchRunFunc))
		{
			UE_LOG(LogNePython, Log, TEXT("Found callable on_run function in %s"), UTF8_TO_TCHAR(PatchModuleName));
			bNeedRunPatch = true;
		}
		else
		{
			if (PyPatchRunFunc)
			{
				Py_DECREF(PyPatchRunFunc);
				PyPatchRunFunc = nullptr;
			}
			PyErr_Clear();
			UE_LOG(LogNePython, Warning, TEXT("on_run function not found or not callable in %s, patch is not available."), UTF8_TO_TCHAR(PatchModuleName));
		}
	}
	else
	{
		PyErr_Clear();
		UE_LOG(LogNePython, Log, TEXT("No Python patch module needed"), UTF8_TO_TCHAR(PatchModuleName));
	}

	// 启动nepyinit(原ue_site)，作为python逻辑入口
	// 为了兼容旧逻辑，先尝试import ue_site，如果失败则再尝试import nepyinit
	InitModuleName = "ue_site";
	PyInitModule = PyImport_ImportModule(InitModuleName);
	if (!PyInitModule)
	{
		PyErr_Clear();
		InitModuleName = "nepyinit";
		PyInitModule = PyImport_ImportModule(InitModuleName);
	}

	if (PyInitModule)
	{
		UE_LOG(LogNePython, Log, TEXT("%s Python module successfully imported"), UTF8_TO_TCHAR(InitModuleName));

		PyOnDebugInputFunc = PyObject_GetAttrString(PyInitModule, "on_debug_input");
		if (!PyOnDebugInputFunc)
		{
			PyErr_Clear();
		}

		bool bCallInitInTick = false;

		FNePyObjectPtr PyCallInitInTick =  NePyStealReference(PyObject_GetAttrString(PyInitModule, "INIT_CALL_IN_TICK"));
		if (!PyCallInitInTick)
		{
			PyErr_Clear();
		}
		else
		{
			bCallInitInTick = PyCallInitInTick.Get() == Py_True;
		}

		PyOnInitFunc = PyObject_GetAttrString(PyInitModule, "on_init");
		if (PyOnInitFunc)
		{
			// 如果需要运行patch，init将延后到tick调用
			if (!bCallInitInTick && !bNeedRunPatch)
			{
				bInitFuncCalled = true;
				RuntimeDelegateContainer->OnInitPre.Broadcast();

				PyObject* Result = PyObject_CallObject(PyOnInitFunc, nullptr);
				if (Result)
				{
					Py_XDECREF(Result);
				}
				else
				{
					UE_LOG(LogNePython, Error, TEXT("Call %s.on_init() Failed!"), UTF8_TO_TCHAR(InitModuleName));
					PyErr_Print();
					PyErr_Clear();
				}

				RuntimeDelegateContainer->OnInitPost.Broadcast();
			}
		}
		else
		{
			PyErr_Print();
			PyErr_Clear();
		}

		PyOnPostEngineInit = PyObject_GetAttrString(PyInitModule, "on_post_engine_init");
		if (!PyOnPostEngineInit)
		{
			PyErr_Clear();
		}

		PyOnTickFunc = PyObject_GetAttrString(PyInitModule, "on_tick");
		if (PyOnTickFunc || (PyOnInitFunc != nullptr && !bInitFuncCalled))
		{
#if ENGINE_MAJOR_VERSION >= 5
			TickDelegateHandler = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FNePythonBindingModule::Tick), 0
			);
#else
			TickDelegateHandler = FTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FNePythonBindingModule::Tick), 0
			);
#endif
		}
		else
		{
			PyErr_Clear();
		}
	}
	else
	{
		PyErr_Print();
		PyErr_Clear();
	}

	// release the GIL
	if (bPyIsInitializedBefore)
	{
		PyGILState_Release(PyGILState);
	}
	else
	{
		PyEval_SaveThread();
	}

#if WITH_EDITOR
	// 纯数据蓝图可以通过点击编译进行组件修复
	// FNePyWrapperTypeRegistry::Get().ApplyDataOnlyBlueprintAttachmentFixup();
#endif

#if NEPY_ENABLE_INSIGHT_PROFILER
	NEPY_INSIGHT_ENABLE_PYTHON_PROFILE;
#endif

	FNePythonModuleDelegates::bModuleInited = true;
	FNePythonModuleDelegates::OnPythonModuleState_PostInit.Broadcast();

#if WITH_EDITOR
	// 编辑器模式下强制触发一次Tick以便兼容编辑器的Patch模块先运行
	Tick(0);
#endif
}

void FNePythonBindingModule::ShutdownPython()
{
	if (!FNePythonModuleDelegates::bModuleInited)
	{
		return;
	}

	FNePyHouseKeeper::OnPreExit();
	FNePythonModuleDelegates::OnPythonModuleState_PreRelease.Broadcast();
	FNePythonModuleDelegates::bModuleInited = false;

	// 移除tick
	if (TickDelegateHandler.IsValid())
	{

#if ENGINE_MAJOR_VERSION >= 5
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandler);
#else
		FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandler);
#endif
	}

#if NEPY_ENABLE_INSIGHT_PROFILER
	NEPY_INSIGHT_DISABLE_PYTHON_PROFILE;
#endif

	// 脚本层清理工作
	if (PyInitModule)
	{
		FNePyScopedGIL GIL;
		PyObject* PyOnShutDown = PyObject_GetAttrString(PyInitModule, "on_shutdown");
		if (PyOnShutDown)
		{
			PyObject* Result = PyObject_CallObject(PyOnShutDown, nullptr);
			Py_XDECREF(Result);
			Py_XDECREF(PyOnShutDown);
		}

		// 无论方法是否存在，调用是否成功，都不报错
		PyErr_Clear();
	}

	UE_LOG(LogNePython, Log, TEXT("Goodbye Python"));

	if (!bBrutalFinalize && Py_IsInitialized())
	{
		PyGILState_Ensure();

		NePyPurgeGeneratedTypes();

		PatchModuleName = nullptr;
		InitModuleName = nullptr;

		Py_CLEAR(PyPatchRunFunc);
		Py_CLEAR(PyPatchShouldContinueFunc);
		Py_CLEAR(PyPatchModule);
		Py_CLEAR(PyOnInitFunc);
		Py_CLEAR(PyOnTickFunc);
		Py_CLEAR(PyOnDebugInputFunc);
		Py_CLEAR(PyOnPostEngineInit);
		Py_CLEAR(PyInitModule);
		Py_CLEAR(PyMainDict);
		Py_CLEAR(PyMainModule);
		Py_CLEAR(PyInternalModule);
		Py_CLEAR(PyRootModule);

		Py_Finalize();

		FNePyMemoryAllocator::Get().Shutdown();
	}
}

void FNePythonBindingModule::OnPreExit()
{
	FCoreDelegates::OnPreExit.RemoveAll(this);

	TArray<const UClass*> NePyResourceOwnerImplClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* ClassObj = *It;
		if (ClassObj->ImplementsInterface(UNePyResourceOwner::StaticClass()))//判断实现了某个接口
		{
			NePyResourceOwnerImplClasses.Add(ClassObj);
		}
	}

	ForEachObjectOfClasses(NePyResourceOwnerImplClasses, [](UObject* PyResourceOwner)
		{
			INePyResourceOwner* OwnerObject = Cast<INePyResourceOwner>(PyResourceOwner);
			OwnerObject->ReleasePythonResources();
		}, RF_NoFlags);
}

void FNePythonBindingModule::LogPythonCallStackSafe()
{
	NePyCallInfo::SafePrintCurrentCallInfo();
	if (GLog)
	{
		GLog->Flush();
	}
}

void FNePythonBindingModule::OnSystemError()
{
	LogPythonCallStackSafe();
}

void FNePythonBindingModule::OnPostEngineInit()
{
	if (PyOnPostEngineInit)
	{
		FNePyScopedGIL GIL;
		PyObject* Result = PyObject_CallObject(PyOnPostEngineInit, nullptr);
		if (!Result)
		{
			PyErr_Print();
		}
		Py_XDECREF(Result);
	}

#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION < 5
	if (GEditor)
	{
		GEditor->OnObjectsReplaced().RemoveAll(&FNePyHouseKeeper::Get());
		GEditor->OnObjectsReplaced().AddRaw(&FNePyHouseKeeper::Get(), &FNePyHouseKeeper::HandleObjectsReinstanced);
	}
#endif // ENGINE_MAJOR_VERSION

	// BlueprintActionDatabase只能在GEditor被初始化之后才能访问
	// 因此将NePyGeneratedClass的Blueprint Action延迟到这里注册
	FNePyBlueprintActionDatabaseHelper::OnPostEngineInit();
#endif // WITH_EDITOR

	RuntimeDelegateContainer->OnPostEngineInit.Broadcast();
}

bool FNePythonBindingModule::Tick(float DeltaSeconds)
{
	if (bNeedRunPatch)
	{
		FNePyScopedGIL GIL;

		auto PatchExceptionExitIfNeed = [this]() -> void
		{
			if (bBrutalPatchExceptionExit)
			{
				// 如果设置了暴力退出，则直接退出
				UE_LOG(LogNePython, Fatal, TEXT("Brutal patch exception exit triggered!"));
			}
		};

		auto ExecuteOnRunSafeDone = [this]() -> bool
		{
			// 调用一次on_run
			PyObject* RunResult = PyObject_CallObject(PyPatchRunFunc, nullptr);
			if (RunResult)
			{
				Py_DECREF(RunResult);
				return true;
			}
			else
			{
				// 调用失败，打印错误信息
				PyErr_Print();
				UE_LOG(LogNePython, Error, TEXT("Failed to call on_run function in module %s"), UTF8_TO_TCHAR(PatchModuleName));
				return false;
			}
		};

		// 先执行on_run函数
		if (PyPatchRunFunc)
		{
			RuntimeDelegateContainer->OnPatchRunPre.Broadcast();
			if (!ExecuteOnRunSafeDone())
			{
				// 运行出现异常 patch终止
				bNeedRunPatch = false;
				PatchExceptionExitIfNeed();
			}
			RuntimeDelegateContainer->OnPatchRunPost.Broadcast();
		}

		// 然后判断是否有should_continue函数
		if (PyPatchShouldContinueFunc)
		{
			RuntimeDelegateContainer->OnPatchShouldContinuePre.Broadcast();
			// 有should_continue函数，调用并根据结果判断是否继续
			PyObject* ShouldContinueResult = PyObject_CallObject(PyPatchShouldContinueFunc, nullptr);
			if (ShouldContinueResult)
			{
				if (!PyObject_IsTrue(ShouldContinueResult))
				{
					// should_continue返回False，patch逻辑结束
					bNeedRunPatch = false;
				}
				Py_DECREF(ShouldContinueResult);
			}
			else
			{
				// should_continue函数调用失败
				PyErr_Print();
				UE_LOG(LogNePython, Error, TEXT("Failed to call should_continue function in module %s"), UTF8_TO_TCHAR(PatchModuleName));
				bNeedRunPatch = false;
				PatchExceptionExitIfNeed();
			}
			RuntimeDelegateContainer->OnPatchShouldContinuePost.Broadcast();
		}
		else
		{
			// 没有should_continue函数，直接结束
			bNeedRunPatch = false;
		}

		if (bNeedRunPatch)
		{
			return true;
		}
	}

	if (!bInitFuncCalled)
	{
		bInitFuncCalled = true;

		if (PyOnInitFunc != nullptr)
		{
			FNePyScopedGIL GIL;
			RuntimeDelegateContainer->OnInitPre.Broadcast();
			PyObject* Result = PyObject_CallObject(PyOnInitFunc, nullptr);
			if (Result)
			{
				Py_XDECREF(Result);
			}
			else
			{
				UE_LOG(LogNePython, Error, TEXT("Call %s.on_init() Failed!"), UTF8_TO_TCHAR(InitModuleName));
				PyErr_Print();
			}
			RuntimeDelegateContainer->OnInitPost.Broadcast();
		}

		if (PyOnTickFunc == nullptr)
		{
			// 移除tick
			if (TickDelegateHandler.IsValid())
			{

#if ENGINE_MAJOR_VERSION >= 5
				FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandler);
#else
				FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandler);
#endif
			}

			return true;
		}
	}
	
	check(PyOnTickFunc);
	{
		FNePyScopedGIL GIL;
		PyObject* PyArgs = Py_BuildValue("(f)", DeltaSeconds);
		PyObject* Result = PyObject_CallObject(PyOnTickFunc, PyArgs);
		if (!Result)
		{
			PyErr_Print();
		}
		Py_XDECREF(Result);
		Py_XDECREF(PyArgs);
	}

	RuntimeDelegateContainer->OnTick.Broadcast(DeltaSeconds);

	return true;
}

TArray<FString> FNePythonBindingModule::GetPythonSysPaths()
{
	FNePyScopedGIL GIL;
	TArray<FString> Paths;

	if (PyObject* PyPathList = PySys_GetObject((char*)"path"))
	{
		const Py_ssize_t PyPathLen = PyList_Size(PyPathList);
		for (Py_ssize_t PyPathIndex = 0; PyPathIndex < PyPathLen; ++PyPathIndex)
		{
			PyObject* PyPathItem = PyList_GetItem(PyPathList, PyPathIndex);
			Paths.Add(UTF8_TO_TCHAR(NePyString_AsString(PyPathItem)));
		}
	}

	return Paths;
}

void FNePythonBindingModule::SetPythonSysPaths(TArray<FString> Paths)
{
	FNePyScopedGIL GIL;

	FNePyObjectPtr PySysModule = NePyStealReference(PyImport_ImportModule("sys"));
	PyObject* PySysDict = PyModule_GetDict(PySysModule);

	FNePyObjectPtr PyPathList = NePyStealReference(PyList_New(Paths.Num()));
	int32 PathIndex = 0;
	for (auto& ScriptPath : Paths)
	{
		PyObject* PyScriptPath = NePyString_FromString(TCHAR_TO_UTF8(*ScriptPath));
		PyList_SetItem(PyPathList, PathIndex++, PyScriptPath);
		UE_LOG(LogNePython, Log, TEXT("Python Scripts search path: %s"), *ScriptPath);
	}
	PyDict_SetItemString(PySysDict, "path", PyPathList);
}

void FNePythonBindingModule::SetPythonSysArgv(const TCHAR* CommandLine)
{
	FString CommandLineCopy = CommandLine;
	const TCHAR* ParsedCmdLine = *CommandLineCopy;

	TArray<FString> Args;
	for (;;)
	{
		FString Arg = FParse::Token(ParsedCmdLine, 0);
		if (Arg.Len() <= 0)
			break;
		Args.Add(Arg);
	}

#if PY_MAJOR_VERSION > 3 || (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 11)
	PyObject* SysModule = PyImport_ImportModule("sys");
	if (!SysModule)
	{
		PyErr_Print();
		return;
	}
	PyObject* PyArgVList = PyList_New(Args.Num());
	if (!PyArgVList)
	{
		Py_DECREF(SysModule);
	}
	else
	{
		for (int32 i = 0; i < Args.Num(); i++)
		{
			PyObject* PyArg = PyUnicode_FromKindAndData((int)sizeof(TCHAR), *Args[i], Args[i].Len());
			PyList_SetItem(PyArgVList, i, PyArg);
		}
		PyObject_SetAttrString(SysModule, "argv", PyArgVList);
		Py_DECREF(PyArgVList);
		Py_DECREF(SysModule);
	}
#else
#if PY_MAJOR_VERSION >= 3
	wchar_t** argv = (wchar_t**)FMemory::Malloc(sizeof(wchar_t*) * (Args.Num() + 1));
#else
	char** argv = (char**)FMemory::Malloc(sizeof(char*) * (Args.Num() + 1));
#endif
	for (int32 i = 0; i < Args.Num(); i++)
	{
#if PY_MAJOR_VERSION >= 3
		std::wstring CurArg = TCHAR_TO_WCHAR(*Args[i]);
		int32 CurArgStrSize = sizeof(wchar_t) * (CurArg.length() + 1);
		argv[i] = (wchar_t*)FMemory::Malloc(CurArgStrSize);
		FMemory::Memcpy(argv[i], CurArg.c_str(), CurArgStrSize);
#else
		std::string CurArg = TCHAR_TO_UTF8(*Args[i]);
		int32 CurArgStrSize = sizeof(char) * (FCStringAnsi::Strlen(CurArg.c_str()) + 1);
		argv[i] = (char*)FMemory::Malloc(CurArgStrSize);
		FMemory::Memcpy(argv[i], CurArg.c_str(), CurArgStrSize);
#endif
	}
	argv[Args.Num()] = nullptr;

	PySys_SetArgv(Args.Num(), argv);

	for (int32 i = 0; i < Args.Num(); i++)
	{
		FMemory::Free(argv[i]);
	}

	FMemory::Free(argv);
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FNePythonBindingModule, NePythonBinding)

FNePythonModuleDelegates::FNePythonModuleState FNePythonModuleDelegates::OnPythonModuleState_PostInit;

FNePythonModuleDelegates::FNePythonModuleState FNePythonModuleDelegates::OnPythonModuleState_PreRelease;

FNePythonModuleDelegates::FNePythonInitPythonModuleDelegate FNePythonModuleDelegates::OnInitPythonModule;

bool FNePythonModuleDelegates::bModuleInited;
