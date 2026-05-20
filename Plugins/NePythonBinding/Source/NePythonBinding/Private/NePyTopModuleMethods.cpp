#include "NePyBase.h"
#include "NePyGIL.h"
#include "NePyObjectBase.h"
#include "NePyGeneratedType.h"
#include "NePyImporter.h"
#include "NePyObjectHolder.h"
#include "NePyTickerHandle.h"
#include "NePyTimerManagerWrapper.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyAutoExportVersion.h"
#include "NePythonBinding.h"
#include "NePyCallInfo.h"
#if WITH_NEPY_AUTO_EXPORT
#include "NePyStruct_Guid.h"
#include "NePyStruct_Vector.h"
#include "NePyStruct_Vector2D.h"
#include "NePyStruct_Color.h"
#endif // WITH_NEPY_AUTO_EXPORT
#include "RHI.h"

// #include "openssl/aes.h"
#include "Misc/AES.h"
#include "Misc/EngineVersionComparison.h"
#if WITH_EDITOR
#include "PackageTools.h"
#include "Editor.h"
#include "ReflectionExport/NePyExport.h"
#include "ObjectTools.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Containers/BinaryHeap.h"
#include "Engine/ObjectLibrary.h"
#include "IDesktopPlatform.h"
#include "DesktopPlatformModule.h"
#endif
#include "ShaderCodeLibrary.h"
#include "Blueprint/SlateBlueprintLibrary.h"

#include "UnrealEngine.h"
#include "Engine/GameViewportClient.h"

#include "HAL/PlatformMisc.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformOutputDevices.h"

#include "Runtime/Launch/Resources/Version.h"
#include "Runtime/Launch/Public/LaunchEngineLoop.h"

#include "Framework/Application/SlateApplication.h"
#include "UObject/UObjectIterator.h"

#include "Misc/FileHelper.h"

#include "Logging/LogMacros.h"

#include "IPlatformFilePak.h"
#include "IPlatformFileSandboxWrapper.h"

#include "UObject/SoftObjectPath.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Engine/Selection.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

#include "RenderingThread.h"
#include "UnrealClient.h"

// 反射兜底，在Shipping版中不应当开启（因为性能很差），在开发版中如果反射兜底成功，也会打Warning日志来警告用户
// 触发兜底的原因大概率是UProperty或UFunction的FName与用户在Python层使用的名称大小写不一致
bool GNePyEnableReflectionFallback = true;
bool GNePyEnableReflectionFallbackLog = true;

static PyObject* NePyMethod_IsInit(PyObject* InSelf, PyObject* InArgs)
{
	if (FNePythonBindingModule::Get().IsInitFuncCalled())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

static PyObject* NePyMethod_NeedRedirect(PyObject* InSelf, PyObject* InArgs)
{
	if (FNePythonBindingModule::Get().NeedRedirect())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* NePyMethod_Log(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "O:Log", &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	UE_LOG(LogNePython, Log, TEXT("%s"), UTF8_TO_TCHAR(Message));
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

PyObject* NePyMethod_LogDisplay(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "O:LogDisplay", &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	UE_LOG(LogNePython, Display, TEXT("%s"), UTF8_TO_TCHAR(Message));
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

PyObject* NePyMethod_LogWarning(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "O:LogWarning", &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	UE_LOG(LogNePython, Warning, TEXT("%s"), UTF8_TO_TCHAR(Message));
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

PyObject* NePyMethod_LogError(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "O:LogError", &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	UE_LOG(LogNePython, Error, TEXT("%s"), UTF8_TO_TCHAR(Message));
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

PyObject* NePyMethod_LogVerbose(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "O:LogVerbose", &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	UE_LOG(LogNePython, Verbose, TEXT("%s"), UTF8_TO_TCHAR(Message));
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

PyObject* NePyMethod_LogFatal(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "O:LogFatal", &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	UE_LOG(LogNePython, Fatal, TEXT("%s"), UTF8_TO_TCHAR(Message));
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

PyObject* NePyMethod_AddOnScreenDebugMessage(PyObject* InSelf, PyObject* InArgs)
{
	if (!GEngine)
	{
		Py_RETURN_NONE;
	}

	int Key;
	float TimeToDisplay;
	PyObject* PyMessage;
	if (!PyArg_ParseTuple(InArgs, "ifO:AddOnScreenDebugMessage", &Key, &TimeToDisplay, &PyMessage))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg3 'Message' must have type 'str'");
		return nullptr;
	}

	GEngine->AddOnScreenDebugMessage(Key, TimeToDisplay, FColor::Green, FString::Printf(TEXT("%s"), UTF8_TO_TCHAR(Message)));
	Py_RETURN_NONE;
}

PyObject* NePyMethod_PrintString(PyObject* InSelf, PyObject* InArgs)
{
#if WITH_NEPY_AUTO_EXPORT
	if (!GEngine)
	{
		Py_RETURN_NONE;
	}

	PyObject* PyMessage;
	float TimeToDisplay = 2.0;
	PyObject* PyColor = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O|fO:PrintString", &PyMessage, &TimeToDisplay, &PyColor))
	{
		return nullptr;
	}

	const char* Message;
	if (!NePyBase::ToCpp(PyMessage, Message))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Message' must have type 'str'");
		return nullptr;
	}

	FColor Color = FColor::Cyan;
	if (PyColor)
	{
		if (!NePyBase::ToCpp(PyColor, Color))
		{
			return PyErr_Format(PyExc_Exception, "argument is not a FColor");
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, TimeToDisplay, Color, FString(UTF8_TO_TCHAR(Message)));
#endif

	Py_RETURN_NONE;
}

PyObject* NePyMethod_RequestExit(PyObject* InSelf, PyObject* InArgs)
{
	bool bForce;
	if (!PyArg_ParseTuple(InArgs, "b|:RequestExit", &bForce))
	{
		return nullptr;
	}

#if PLATFORM_IOS || PLATFORM_ANDROID
	if (bForce)
	{
		_exit(0);
	}
#endif

	FPlatformMisc::RequestExit(bForce);
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetEngineDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::EngineDir()));
}

PyObject* NePyMethod_GetEngineContentDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::EngineContentDir()));
}

PyObject* NePyMethod_GetEngineConfigDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::EngineConfigDir()));
}

PyObject* NePyMethod_GetProjectDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::ProjectDir()));
}

PyObject* NePyMethod_GetContentDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::ProjectContentDir()));
}

PyObject* NePyMethod_GetDocumentDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*(FPaths::ProjectDir() + TEXT("Documents/"))));
}

PyObject* NePyMethod_GetConfigDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::ProjectConfigDir()));
}

PyObject* NePyMethod_GetLogDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::ProjectLogDir()));
}

#if NEPY_BUILD_WITH_NETEASE_UE
#include "HAL/ExceptionHandling.h"
#endif
PyObject* NePyMethod_GetLogFilename(PyObject* InSelf)
{
	FString LogFile = FPlatformOutputDevices::GetAbsoluteLogFilename();
	return PyUnicode_FromString(TCHAR_TO_UTF8(*LogFile));
}

PyObject* NePyMethod_GetGameSavedDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::ProjectSavedDir()));
}

PyObject* NePyMethod_GetGameUserDeveloperDir(PyObject* InSelf)
{
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::GameUserDeveloperDir()));
}

PyObject* NePyMethod_ConvertRelativePathToFull(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	if (!PyArg_ParseTuple(InArgs, "s:ConvertRelativePathToFull", &Path))
	{
		return nullptr;
	}
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(Path))));
}

PyObject* NePyMethod_ConvertAbsolutePathApp(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	if (!PyArg_ParseTuple(InArgs, "s:ConvertAbsolutePathApp", &Path))
	{
		return nullptr;
	}

	FString AbsPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(UTF8_TO_TCHAR(Path));
	return PyUnicode_FromString(TCHAR_TO_UTF8(*AbsPath));
}

PyObject* NePyMethod_ObjectPathToPackageName(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	if (!PyArg_ParseTuple(InArgs, "s:ObjectPathToPackageName", &Path))
	{
		return nullptr;
	}

	const FString PackageName = FPackageName::ObjectPathToPackageName((const FString)(UTF8_TO_TCHAR(Path)));
	return PyUnicode_FromString(TCHAR_TO_UTF8(*PackageName));
}

PyObject* NePyMethod_GetPath(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	if (!PyArg_ParseTuple(InArgs, "s:GetPath", &Path))
	{
		return nullptr;
	}
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::GetPath(UTF8_TO_TCHAR(Path))));
}

PyObject* NePyMethod_GetBaseFilename(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	if (!PyArg_ParseTuple(InArgs, "s:GetBaseFilename", &Path))
	{
		return nullptr;
	}
	return PyUnicode_FromString(TCHAR_TO_UTF8(*FPaths::GetBaseFilename(UTF8_TO_TCHAR(Path))));
}

PyObject* NePyMethod_FindFile(PyObject* InSelf, PyObject* InArgs)
{
	const char* FullName;
	const char* PathStr;
	if (!PyArg_ParseTuple(InArgs, "ss", &FullName, &PathStr))
	{
		return nullptr;
	}
	FString Path = UTF8_TO_TCHAR(PathStr);
	Path = Path / UTF8_TO_TCHAR(FullName);
	bool exist = FPaths::FileExists(Path);
	return Py_BuildValue("b", exist);
}

PyObject* NePyMethod_GetFile(PyObject* InSelf, PyObject* InArgs)
{
	const char* FullName;
	const char* PathStr;
	if (!PyArg_ParseTuple(InArgs, "ss", &FullName, &PathStr))
	{
		return nullptr;
	}

	FString Path = UTF8_TO_TCHAR(PathStr);
	Path = Path / UTF8_TO_TCHAR(FullName);
	TArray<uint8> ByteBuffer;
	if (!FFileHelper::LoadFileToArray(ByteBuffer, *Path))
	{
		return PyErr_Format(PyExc_Exception, "LoadFileToArray failed!");
	}
	return PyBytes_FromStringAndSize((const char*)ByteBuffer.GetData(), ByteBuffer.Num());
}

#if WITH_EDITOR
PyObject* NePyMethod_EncryptBuffer(PyObject* InSelf, PyObject* InArgs)
{
	typedef unsigned char BYTE;
#ifdef UE_BUILD_SHIPPING
	//VM_TIGER_RED_START // Themida shell
#endif
	PyObject* BytesObj;
	if (!PyArg_ParseTuple(InArgs, "O", &BytesObj))
	{
		return nullptr;
	}

	Py_ssize_t BufLen = PyBytes_Size(BytesObj);
	BYTE* Buf = (BYTE*)PyBytes_AS_STRING(BytesObj);

	FAES::FAESKey Key;
	NePyImporterEncryptKey(Key.Key);
	// 为了加速 AES 加密/解密, Data部分必须考虑16字节对齐
	// 实际分配内存大小为 16 + BlockSize* KeySize
	// BufferLen 占据 4 字节，预留 12 字节作为 Padding
	uint32 BlockCount = (BufLen - 1) / FAES::AESBlockSize + 1;
	uint32 AllocSize = BlockCount * FAES::AESBlockSize + 16;
	BYTE* OutBuf = (BYTE*)FMemory::Malloc(AllocSize, 16);
	*(uint32*)(OutBuf + 12) = BufLen;
	BYTE* p = OutBuf + 16;
	memcpy(p, Buf, BufLen);
	if (BlockCount * FAES::FAESKey::KeySize > BufLen)
	{
		// Padding 0
		memset(p + BufLen, 0, BlockCount * FAES::AESBlockSize - BufLen);
	}
	FAES::EncryptData(p, BlockCount * FAES::AESBlockSize, Key);

#if PY_MAJOR_VERSION >= 3
	PyObject* PyBuf = Py_BuildValue("y#", OutBuf + 12, BlockCount * FAES::AESBlockSize + sizeof(uint32));
#else
	PyObject* PyBuf = Py_BuildValue("s#", OutBuf + 12, BlockCount * FAES::AESBlockSize + sizeof(uint32));
#endif
	FMemory::Free(OutBuf);

#ifdef UE_BUILD_SHIPPING
	//VM_TIGER_RED_END
#endif
	return PyBuf;
}
#endif

PyObject* NePyMethod_GetTypeFromClass(PyObject* InSelf, PyObject* InArg)
{
	UClass* Class = NePyBase::ToCppClass(InArg);
	if (!Class)
	{
		PyErr_Format(PyExc_TypeError, "Parameter must be a 'Class' not '%s'", Py_TYPE(InArg)->tp_name);
		return nullptr;
	}

	const FNePyObjectTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(Class);
	if (!PyTypeInfo)
	{
		Py_RETURN_NONE;
	}

	Py_INCREF(PyTypeInfo->TypeObject);
	return (PyObject*)PyTypeInfo->TypeObject;
}

PyObject* NePyMethod_GetTypeFromStruct(PyObject* InSelf, PyObject* InArg)
{
	UScriptStruct* ScriptStruct = NePyBase::ToCppScriptStruct(InArg);
	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_TypeError, "Parameter must be a 'ScriptStruct' not '%s'", Py_TYPE(InArg)->tp_name);
		return nullptr;
	}

	const FNePyStructTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(ScriptStruct);
	if (!PyTypeInfo)
	{
		Py_RETURN_NONE;
	}

	Py_INCREF(PyTypeInfo->TypeObject);
	return (PyObject*)PyTypeInfo->TypeObject;
}

PyObject* NePyMethod_GetTypeFromEnum(PyObject* InSelf, PyObject* InArg)
{
	UEnum* Enum = NePyBase::ToCppEnum(InArg);
	if (!Enum)
	{
		PyErr_Format(PyExc_TypeError, "Parameter must be a 'Enum' not '%s'", Py_TYPE(InArg)->tp_name);
		return nullptr;
	}

	const FNePyEnumTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedEnumType(Enum);
	if (!PyTypeInfo)
	{
		Py_RETURN_NONE;
	}

	Py_INCREF(PyTypeInfo->TypeObject);
	return (PyObject*)PyTypeInfo->TypeObject;
}

PyObject* NePyMethod_MountPak(PyObject* InSelf, PyObject* InArgs)
{
	const char* PakFile;
	int Order;
	const char* MountPoint = nullptr;
	if (!PyArg_ParseTuple(InArgs, "si|s", &PakFile, &Order, &MountPoint))
	{
		return nullptr;
	}

	bool bResult = false;
	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile")));
	if (PakFileMgr == nullptr)
	{
		UE_LOG(LogNePython, Log, TEXT("MountPak failed: no PakFile found"));
		Py_RETURN_FALSE;
	}

	FString PakFilePath = UTF8_TO_TCHAR(PakFile);
	if (!FPaths::FileExists(PakFilePath))
	{
		bResult = false;
		UE_LOG(LogNePython, Error, TEXT("MountPak failed, %s not exists!"), *PakFilePath);
	}
	else if (MountPoint)
	{
		bResult = PakFileMgr->Mount(*PakFilePath, Order, UTF8_TO_TCHAR(MountPoint));
	}
	else
	{
		bResult = PakFileMgr->Mount(*PakFilePath, Order);
	}
	return Py_BuildValue("b", bResult);
}

PyObject* NePyMethod_UnmountPak(PyObject* InSelf, PyObject* InArgs)
{
	const char* PakFile = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s", &PakFile))
	{
		return nullptr;
	}

	bool bResult = false;
	FPakPlatformFile* PakFileMgr = (FPakPlatformFile*)(FPlatformFileManager::Get().FindPlatformFile(TEXT("PakFile")));
	if (PakFileMgr == nullptr)
	{
		UE_LOG(LogNePython, Log, TEXT("UnmountPak failed: no PakFile found"));
		Py_RETURN_FALSE;
	}

	FString PakFilePath = UTF8_TO_TCHAR(PakFile);
	if (!FPaths::FileExists(PakFilePath))
	{
		bResult = false;
		UE_LOG(LogNePython, Error, TEXT("UnmountPak failed, %s not exists!"), *PakFilePath);
	}
	else
	{
		bResult = PakFileMgr->Unmount(*PakFilePath);
	}
	return Py_BuildValue("b", bResult);
}

PyObject* NePyMethod_CreateWorld(PyObject* InSelf, PyObject* InArgs)
{
	int WorldType = EWorldType::None;
	if (!PyArg_ParseTuple(InArgs, "|i:CreateWorld", &WorldType))
	{
		return nullptr;
	}

	UWorld* World = UWorld::CreateWorld((EWorldType::Type)WorldType, false);
	return NePyBase::ToPy(World);
}

PyObject* NePyMethod_ParsePropertyFlags(PyObject* InSelf, PyObject* InArgs)
{
	uint64 Flags;
	if (!PyArg_ParseTuple(InArgs, "l:ParsePropertyFlags", &Flags))
	{
		return nullptr;
	}

	auto RetVal = ParsePropertyFlags((EPropertyFlags)Flags);

	PyObject* PyRetVal0 = NePyBase::ToPy(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_GetTransientPackage(PyObject* InSelf)
{
	auto RetVal = GetTransientPackage();

	PyObject* PyRetVal0 = NePyBase::ToPy(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_GetIniFilenameFromObjectsReference(PyObject* InSelf, PyObject* InArg)
{
	PyObject* PyArgs[1] = { InArg };

	FString ObjectsReferenceString;
	if (!NePyBase::ToCpp(PyArgs[0], ObjectsReferenceString))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'ObjectsReferenceString' must have type 'str'");
		return nullptr;
	}

	auto RetVal = GetIniFilenameFromObjectsReference(ObjectsReferenceString);

	PyObject* PyRetVal0;
	if (!NePyBase::ToPy(RetVal, PyRetVal0))
	{
		PyErr_SetString(PyExc_TypeError, "PyRetVal0 'RetVal' with type 'str' convert to PyObject failed!");
		return nullptr;
	}
	return PyRetVal0;
}

PyObject* NePyMethod_ResolveIniObjectsReference(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyArgs[3] = { nullptr, nullptr, nullptr };
	if (!PyArg_ParseTuple(InArgs, "O|OO:ResolveIniObjectsReference", &PyArgs[0], &PyArgs[1], &PyArgs[2]))
	{
		return nullptr;
	}

	FString ObjectReference;
	if (!NePyBase::ToCpp(PyArgs[0], ObjectReference))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'ObjectReference' must have type 'str'");
		return nullptr;
	}

	FString* IniFilename = nullptr;
	FString TempIniFilename;
	if (PyArgs[1] && PyArgs[1] != Py_None)
	{
		if (!NePyBase::ToCpp(PyArgs[1], TempIniFilename))
		{
			PyErr_SetString(PyExc_TypeError, "arg2 'IniFilename' must have type 'str'");
			return nullptr;
		}
		IniFilename = &TempIniFilename;
	}

	bool bThrow = false;
	if (PyArgs[2])
	{
		if (!NePyBase::ToCpp(PyArgs[2], bThrow))
		{
			PyErr_SetString(PyExc_TypeError, "arg3 'bThrow' must have type 'bool'");
			return nullptr;
		}
	}

	auto RetVal = ResolveIniObjectsReference(ObjectReference, IniFilename, bThrow);

	PyObject* PyRetVal0;
	if (!NePyBase::ToPy(RetVal, PyRetVal0))
	{
		PyErr_SetString(PyExc_TypeError, "PyRetVal0 'RetVal' with type 'str' convert to PyObject failed!");
		return nullptr;
	}
	return PyRetVal0;
}

PyObject* NePyMethod_LoadPackage(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "s:LoadPackage", &Name))
	{
		return nullptr;
	}

	UPackage* Package;
	Py_BEGIN_ALLOW_THREADS
	Package = LoadPackage(nullptr, UTF8_TO_TCHAR(Name), LOAD_None);
	Py_END_ALLOW_THREADS

	if (!Package)
	{
		return PyErr_Format(PyExc_Exception, "unable to load package %s", Name);
	}

	return NePyBase::ToPy(Package);
}

PyObject* NePyMethod_LoadPackageAsync(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyCallable;
	int32 PackagePriority = 0;
	uint32 PackageFlags = PKG_None;
	if (!PyArg_ParseTuple(InArgs, "sO|iI:LoadPackageAsync", &Name, &PyCallable, &PackagePriority, &PackageFlags))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_TypeError, "argument is not callable");
	}

	FNePyObjectPtrWithGIL PyCallablePtr = NePyNewReferenceWithGIL(PyCallable);
	FLoadPackageAsyncDelegate CompletionDelegate = FLoadPackageAsyncDelegate::CreateLambda([PyCallablePtr](const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
		{
			FNePyScopedGIL GIL;

			FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(3));
			PyTuple_SetItem(PyArgs, 0, NePyString_FromString(TCHAR_TO_ANSI(*(PackageName.ToString()))));
			PyTuple_SetItem(PyArgs, 1, NePyBase::ToPy(LoadedPackage));
			PyTuple_SetItem(PyArgs, 2, PyLong_FromLong(Result));
			FNePyObjectPtr PyResult = NePyStealReference(PyObject_Call(PyCallablePtr, PyArgs, nullptr));
			if (!PyResult)
			{
				PyErr_Print();
			}
		});
	LoadPackageAsync(Name, CompletionDelegate, PackagePriority, (EPackageFlags)PackageFlags);
	Py_RETURN_NONE;
}

#if WITH_EDITOR
PyObject* NePyMethod_UnloadPackage(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj;
	if (!PyArg_ParseTuple(InArgs, "O:UnloadPackage", &PyObj))
	{
		return nullptr;
	}

	UPackage* PackageToUnload = NePyBase::ToCppObject<UPackage>(PyObj);
	if (!PackageToUnload)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UPackage");
	}

	FText OutErrorMsg;
	if (!PackageTools::UnloadPackages({ PackageToUnload }, OutErrorMsg))
	{
		return PyErr_Format(PyExc_Exception, "%s", TCHAR_TO_UTF8(*OutErrorMsg.ToString()));
	}

	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetPackageFileName(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "s:GetPackageFileName", &Name))
	{
		return nullptr;
	}

	FString FileName;
#if ENGINE_MAJOR_VERSION >= 5
	if (!FPackageName::DoesPackageExist(FString(UTF8_TO_TCHAR(Name)), &FileName))
#else
	if (!FPackageName::DoesPackageExist(FString(UTF8_TO_TCHAR(Name)), nullptr, &FileName))
#endif
	{
		return PyErr_Format(PyExc_Exception, "package does not exist");
	}

	return PyUnicode_FromString(TCHAR_TO_UTF8(*FileName));
}
#endif

PyObject* NePyMethod_FindClass(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyOuter = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:FindClass", &Name, &PyOuter))
	{
		return nullptr;
	}

	UObject* Outer = nullptr;
	if (PyOuter && PyOuter != Py_None)
	{
		if (!NePyBase::ToCpp(PyOuter, Outer))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'Outer' must have type 'Object'");
			return nullptr;
		}
	}

	UClass* Class;
	Py_BEGIN_ALLOW_THREADS
	if (Outer)
	{
		Class = FindObject<UClass>(Outer, UTF8_TO_TCHAR(Name));
	}
	else
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		Class = FindFirstObject<UClass>(UTF8_TO_TCHAR(Name));
#else
		Class = FindObject<UClass>(ANY_PACKAGE, UTF8_TO_TCHAR(Name));
#endif
	}
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Class);
}

// UE5.4+出现Python生成类的蓝图子类CDO的Attachment为空问题
// 原因跟KismetEditorUtilities::ConformBlueprintFlagsAndComponents移除了蓝图类CDO的RootComponent（调用了ConformRemovedNativeComponents）
// 更底层原因应该跟FindNativeArchetype被重新实现有关
// 蓝图类在这里重建CDO
void NePyFixupUClassCDOAttachment(UClass* Class)
{
	if (Class && Class->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
	{
		AActor* ActorOldCDO = Cast<AActor>(Class->GetDefaultObject());
		if (ActorOldCDO == nullptr)
		{
			return;
		}
		if (ActorOldCDO->GetRootComponent() == nullptr)
		{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 6 || ENGINE_MAJOR_VERSION >= 6
			Class->SetDefaultObject(nullptr);
#else
			Class->ClassDefaultObject = nullptr;
#endif
			Class->GetDefaultObject(true);
		}
	}
}

PyObject* NePyMethod_LoadClass(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "s:LoadClass", &Name))
	{
		return nullptr;
	}

	UObject* Object;
	Py_BEGIN_ALLOW_THREADS
	Object = StaticLoadObject(UClass::StaticClass(), nullptr, UTF8_TO_TCHAR(Name));
	Py_END_ALLOW_THREADS

	NePyFixupUClassCDOAttachment(Cast<UClass>(Object));

	return NePyBase::ToPy(Object);
}

PyObject* NePyMethod_AsyncLoadClass(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyCallable;
	int32 Priority = FStreamableManager::DefaultAsyncLoadPriority;
	if (!PyArg_ParseTuple(InArgs, "sO|i:AsyncLoadClass", &Name, &PyCallable, &Priority))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_TypeError, "argument is not callable");
	}

	FSoftClassPath SoftPath(Name);

	FNePyObjectPtrWithGIL PyCallablePtr = NePyNewReferenceWithGIL(PyCallable);
	FStreamableDelegate StreamableDelegate = FStreamableDelegate::CreateLambda([SoftPath, PyCallablePtr]()
		{
			FNePyScopedGIL GIL;

			UClass* Class = SoftPath.ResolveClass();
			NePyFixupUClassCDOAttachment(Class);

			PyObject* PyName = NePyString_FromString(TCHAR_TO_ANSI(*(SoftPath.GetAssetPathString())));
			PyObject* PyClass = NePyBase::ToPy(Class);
			FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(2));
			PyTuple_SetItem(PyArgs, 0, PyName);
			PyTuple_SetItem(PyArgs, 1, PyClass);
			FNePyObjectPtr PyResult = NePyStealReference(PyObject_Call(PyCallablePtr, PyArgs, nullptr));
			if (!PyResult)
			{
				PyErr_Print();
			}
		});
	UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(SoftPath), MoveTemp(StreamableDelegate), Priority);
	Py_RETURN_NONE;
}

PyObject* NePyMethod_FindObject(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyOuter = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:FindObject", &Name, &PyOuter))
	{
		return nullptr;
	}

	UObject* Outer = nullptr;
	if (PyOuter && PyOuter != Py_None)
	{
		if (!NePyBase::ToCpp(PyOuter, Outer))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'Outer' must have type 'Object'");
			return nullptr;
		}
	}

	UObject* Object;
	Py_BEGIN_ALLOW_THREADS
	if (Outer)
	{
		Object = FindObject<UObject>(Outer, UTF8_TO_TCHAR(Name));
	}
	else
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		Object = FindFirstObject<UObject>(UTF8_TO_TCHAR(Name));
#else
		Object = FindObject<UObject>(ANY_PACKAGE, UTF8_TO_TCHAR(Name));
#endif
	}
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Object);
}

PyObject* NePyMethod_LoadObject(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyClass;
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "Os:LoadObject", &PyClass, &Name))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass);
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass");
	}

	UObject* Object;
	Py_BEGIN_ALLOW_THREADS
	Object = StaticLoadObject(Class, nullptr, UTF8_TO_TCHAR(Name));
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Object);
}

PyObject* NePyMethod_AsyncLoadObject(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyCallable;
	int32 Priority = FStreamableManager::DefaultAsyncLoadPriority;
	if (!PyArg_ParseTuple(InArgs, "sO|i:AsyncLoadObject", &Name, &PyCallable, &Priority))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallable))
	{
		return PyErr_Format(PyExc_TypeError, "argument is not callable");
	}

	FSoftObjectPath SoftPath(Name);

	FNePyObjectPtrWithGIL PyCallablePtr = NePyNewReferenceWithGIL(PyCallable);
	FStreamableDelegate StreamableDelegate = FStreamableDelegate::CreateLambda([SoftPath, PyCallablePtr]()
		{
			FNePyScopedGIL GIL;

			PyObject* PyName = NePyString_FromString(TCHAR_TO_ANSI(*(SoftPath.GetAssetPathString())));
			PyObject* PyObj = NePyBase::ToPy(SoftPath.ResolveObject());
			FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(2));
			PyTuple_SetItem(PyArgs, 0, PyName);
			PyTuple_SetItem(PyArgs, 1, PyObj);
			FNePyObjectPtr PyResult = NePyStealReference(PyObject_Call(PyCallablePtr, PyArgs, nullptr));
			if (!PyResult)
			{
				PyErr_Print();
			}
		});
	UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(SoftPath), MoveTemp(StreamableDelegate), Priority);
	Py_RETURN_NONE;
}

PyObject* NePyMethod_FindStruct(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyOuter = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:FindStruct", &Name, &PyOuter))
	{
		return nullptr;
	}

	UObject* Outer = nullptr;
	if (PyOuter && PyOuter != Py_None)
	{
		if (!NePyBase::ToCpp(PyOuter, Outer))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'Outer' must have type 'Object'");
			return nullptr;
		}
	}

	UScriptStruct* Struct;
	Py_BEGIN_ALLOW_THREADS
	if (Outer)
	{
		Struct = FindObject<UScriptStruct>(Outer, UTF8_TO_TCHAR(Name));
}
	else
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		Struct = FindFirstObject<UScriptStruct>(UTF8_TO_TCHAR(Name));
#else
		Struct = FindObject<UScriptStruct>(ANY_PACKAGE, UTF8_TO_TCHAR(Name));
#endif
	}
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Struct);
}

PyObject* NePyMethod_LoadStruct(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "s:LoadStruct", &Name))
	{
		return nullptr;
	}

	UObject* Object;
	Py_BEGIN_ALLOW_THREADS
	Object = StaticLoadObject(UScriptStruct::StaticClass(), nullptr, UTF8_TO_TCHAR(Name));
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Object);
}

PyObject* NePyMethod_FindEnum(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	PyObject* PyOuter = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:FindEnum", &Name, &PyOuter))
	{
		return nullptr;
	}

	UObject* Outer = nullptr;
	if (PyOuter && PyOuter != Py_None)
	{
		if (!NePyBase::ToCpp(PyOuter, Outer))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'Outer' must have type 'Object'");
			return nullptr;
		}
	}

	UEnum* Enum;
	Py_BEGIN_ALLOW_THREADS
	if (Outer)
	{
		Enum = FindObject<UEnum>(Outer, UTF8_TO_TCHAR(Name));
	}
	else
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		Enum = FindFirstObject<UEnum>(UTF8_TO_TCHAR(Name));
#else
		Enum = FindObject<UEnum>(ANY_PACKAGE, UTF8_TO_TCHAR(Name));
#endif
	}
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Enum);
}

PyObject* NePyMethod_LoadEnum(PyObject* InSelf, PyObject* InArgs)
{
	char* Name;
	if (!PyArg_ParseTuple(InArgs, "s:LoadEnum", &Name))
	{
		return nullptr;
	}

	UObject* Object;
	Py_BEGIN_ALLOW_THREADS
	Object = StaticLoadObject(UEnum::StaticClass(), nullptr, UTF8_TO_TCHAR(Name));
	Py_END_ALLOW_THREADS

	return NePyBase::ToPy(Object);
}

PyObject* NePyMethod_CancelAsyncLoading(PyObject* InSelf)
{
	CancelAsyncLoading();
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetAsyncLoadPercentage(PyObject* InSelf, PyObject* InArg)
{
	PyObject* PyArgs[1] = { InArg };

	FName PackageName;
	if (!NePyBase::ToCpp(PyArgs[0], PackageName))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'PackageName' must have type 'str'");
		return nullptr;
	}

	auto RetVal = GetAsyncLoadPercentage(PackageName);

	PyObject* PyRetVal0 = PyFloat_FromDouble(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_FlushAsyncLoading(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyArgs[1] = { nullptr };
	if (!PyArg_ParseTuple(InArgs, "|O:FlushAsyncLoading", &PyArgs[0]))
	{
		return nullptr;
	}

	int32 PackageID = INDEX_NONE;
	if (PyArgs[0])
	{
		if (!NePyBase::ToCpp(PyArgs[0], PackageID))
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'PackageID' must have type 'int'");
			return nullptr;
		}
	}

	FlushAsyncLoading(PackageID);
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetNumAsyncPackages(PyObject* InSelf)
{
	auto RetVal = GetNumAsyncPackages();

	PyObject* PyRetVal0 = PyLong_FromLong(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_IsLoading(PyObject* InSelf)
{
	auto RetVal = IsLoading();

	PyObject* PyRetVal0 = PyBool_FromLong(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_FindPackage(PyObject* InSelf, PyObject* InArgs)
{
	char* PackageName;
	if (!PyArg_ParseTuple(InArgs, "s:FindPackage", &PackageName))
	{
		return nullptr;
	}

	UPackage* RetVal;
	Py_BEGIN_ALLOW_THREADS
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	RetVal = FindPackage(nullptr, UTF8_TO_TCHAR(PackageName));
#else
	RetVal = FindPackage(ANY_PACKAGE, UTF8_TO_TCHAR(PackageName));
#endif
	Py_END_ALLOW_THREADS

	PyObject* PyRetVal0 = NePyBase::ToPy(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_CreatePackage(PyObject* InSelf, PyObject* InArgs)
{
	char* PackageName;
	if (!PyArg_ParseTuple(InArgs, "s:CreatePackage", &PackageName))
	{
		return nullptr;
	}

	auto RetVal = CreatePackage(UTF8_TO_TCHAR(PackageName));

	PyObject* PyRetVal0 = NePyBase::ToPy(RetVal);
	return PyRetVal0;
}

PyObject* NePyMethod_NewObject(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyClass;
	PyObject* PyOuter = nullptr;
	char* NameStr = nullptr;
	uint64 Flags = (uint64)(RF_Public);
	if (!PyArg_ParseTuple(InArgs, "O|OsK:NewObject", &PyClass, &PyOuter, &NameStr, &Flags))
	{
		return nullptr;
	}

	UClass* Class = NePyBase::ToCppClass(PyClass);
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "Class is not a UClass");
	}

	if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Interface))
	{
		if (Class->HasAnyClassFlags(CLASS_Interface))
		{
			return PyErr_Format(PyExc_Exception, "can't create instance of interface class '%s'",
				TCHAR_TO_UTF8(*Class->GetName()));
		}
		else
		{
			return PyErr_Format(PyExc_Exception, "can't create instance of abstract class '%s'",
				TCHAR_TO_UTF8(*Class->GetName()));
		}
	}

	FName Name = NAME_None;
	if (NameStr && strlen(NameStr) > 0)
	{
		Name = FName(UTF8_TO_TCHAR(NameStr));
	}

	UObject* Outer;
	if (PyOuter && PyOuter != Py_None)
	{
		Outer = NePyBase::ToCppObject(PyOuter);
		if (!Outer)
		{
			return PyErr_Format(PyExc_Exception, "Outer is not a UObject");
		}
	}
	else
	{
		Outer = GetTransientPackage();
	}

	UObject* RetObject = NewObject<UObject>(Outer, Class, Name, (EObjectFlags)Flags);
	if (!RetObject)
	{
		return PyErr_Format(PyExc_Exception, "unable to create object");
	}

	return NePyBase::ToPy(RetObject);
}

PyObject* NePyMethod_StringToGuid(PyObject* InSelf, PyObject* InArgs)
{
#if WITH_NEPY_AUTO_EXPORT
	char* Str;
	if (!PyArg_ParseTuple(InArgs, "s:StringToGuid", &Str))
	{
		return nullptr;
	}

	FGuid Guid;
	if (FGuid::Parse(FString(Str), Guid))
	{
		return NePyStructNew_Guid(Guid);
	}

	return PyErr_Format(PyExc_Exception, "unable to build FGuid");
#else
	Py_RETURN_NONE;
#endif
}

PyObject* NePyMethod_GuidToString(PyObject* InSelf, PyObject* InArgs)
{
#if WITH_NEPY_AUTO_EXPORT
	PyObject* PyArg;
	if (!PyArg_ParseTuple(InArgs, "O:GuidToString", &PyArg))
	{
		return nullptr;
	}

	FGuid* Guid;
	if (FNePyStruct_Guid* PyGuid = NePyStructCheck_Guid(PyArg))
	{
		Guid = (FGuid*)PyGuid->Value;
	}
	else
	{
		return PyErr_Format(PyExc_Exception, "object is not a FGuid");
	}

	return PyUnicode_FromString(TCHAR_TO_UTF8(*Guid->ToString()));
#else
	Py_RETURN_NONE;
#endif
}

PyObject* NePyMethod_TickSlate(PyObject* InSelf)
{
	FSlateApplication::Get().PumpMessages();
	FSlateApplication::Get().Tick();
	Py_RETURN_NONE;
}

PyObject* NePyMethod_TickEngine(PyObject* InSelf, PyObject* InArgs)
{
	float DeltaSeconds = FApp::GetDeltaTime();
	PyObject* PyIdle = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|fO:EngineTick", &DeltaSeconds, &PyIdle))
	{
		return nullptr;
	}

	bool bIdle = false;
	if (PyIdle && PyObject_IsTrue(PyIdle))
	{
		bIdle = true;
	}

	GEngine->Tick(DeltaSeconds, bIdle);

	Py_RETURN_NONE;
}

#if WITH_EDITOR
PyObject* NePyMethod_TickRenderingTickables(PyObject* InSelf)
{
	TickRenderingTickables();

	Py_RETURN_NONE;
}
#endif

PyObject* NePyMethod_AddTicker(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyCallback;
	float Delay = 0.f;
	if (!PyArg_ParseTuple(InArgs, "O|f:AddTicker", &PyCallback, &Delay))
	{
		return nullptr;
	}

	if (!PyCallable_Check(PyCallback))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InCallback' must have type 'callable'");
		return nullptr;
	}

	if (Delay < 0)
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'InDelay' must be not less than zero");
		return nullptr;
	}

	class FNePyTickerFunctor : public FNePyObjectHolder
	{
	public:
		using FNePyObjectHolder::FNePyObjectHolder;
		FNePyTickerFunctor(const FNePyTickerFunctor& Other) : FNePyObjectHolder(Other) {}
		FNePyTickerFunctor(FNePyTickerFunctor&& Other) : FNePyObjectHolder(Other) {}

		bool operator()(float DeltaTime)
		{
			FNePyScopedGIL GIL;
			FNePyObjectPtr PyArgs = NePyStealReference(Py_BuildValue("(f)", DeltaTime));
			FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(Value, PyArgs));
			if (!PyRet)
			{
				PyErr_Print();
			}

			bool RetVal = true;
			NePyBase::ToCpp(PyRet, RetVal, true);
			return RetVal;
		}

	private:
		FNePyTickerFunctor() = delete;
		FNePyTickerFunctor& operator=(const FNePyTickerFunctor& InOther) = delete;
		FNePyTickerFunctor& operator=(FNePyTickerFunctor&& InOther) = delete;
	};

	FTickerDelegate TickerDelegate;
	TickerDelegate.BindLambda(FNePyTickerFunctor(PyCallback));

#if ENGINE_MAJOR_VERSION < 5
	auto TickerHandle = FTicker::GetCoreTicker().AddTicker(TickerDelegate, Delay);
#else
	auto TickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickerDelegate, Delay);
#endif
	return NePyStructNew_TickerHandle(TickerHandle);
}

PyObject* NePyMethod_RemoveTicker(PyObject* InSelf, PyObject* InArg)
{
	FNePyTickerHandle* Handle;
	if (FNePyStruct_TickerHandle* PyHandle = NePyStructCheck_TickerHandle(InArg))
	{
		Handle = (FNePyTickerHandle*)PyHandle->Value;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InHandle' must have type 'TickerHandle'");
		return nullptr;
	}

#if ENGINE_MAJOR_VERSION < 5
	FTicker::GetCoreTicker().RemoveTicker(*Handle);
#else
	FTSTicker::GetCoreTicker().RemoveTicker(*Handle);
#endif
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetDeltaTime(PyObject* InSelf)
{
	return PyFloat_FromDouble(FApp::GetDeltaTime());
}

PyObject* NePyMethod_GetAllWorlds(PyObject* InSelf)
{
	FNePyObjectPtr Ret = NePyNewReference(PyList_New(0));
	for (TObjectIterator<UWorld> Itr; Itr; ++Itr)
	{
		PyObject* PyWorld = NePyBase::ToPy(*Itr);
		PyList_Append(Ret, PyWorld);
		Py_DECREF(PyWorld);
	}
	return Ret;
}

PyObject* NePyMethod_GetGameViewportSize(PyObject* InSelf)
{
	if (!GEngine->GameViewport)
	{
		return PyErr_Format(PyExc_Exception, "unable to get GameViewport");
	}

	FVector2D size;
	GEngine->GameViewport->GetViewportSize(size);

	return Py_BuildValue("(ff)", size.X, size.Y);
}

PyObject* NePyMethod_GetResolution(PyObject* InSelf)
{
	return Py_BuildValue("(ff)", GSystemResolution.ResX, GSystemResolution.ResY);
}

PyObject* NePyMethod_GetViewportScreenshot(PyObject* InSelf, PyObject* InArgs)
{
#if WITH_NEPY_AUTO_EXPORT
	if (!GEngine->GameViewport)
	{
		Py_RETURN_NONE;
	}

	bool bAsIntList = false;
	if (!PyArg_ParseTuple(InArgs, "|b:GetViewportScreenshot", &bAsIntList))
	{
		return nullptr;
	}

	FViewport* Viewport = GEngine->GameViewport->Viewport;
	TArray<FColor> Bitmap;
	bool bSuccess = GetViewportScreenShot(Viewport, Bitmap);
	if (!bSuccess)
	{
		Py_RETURN_NONE;
	}

	if (bAsIntList)
	{
		FNePyObjectPtr BitmapTuple = NePyNewReference(PyTuple_New(Bitmap.Num() * 4));
		for (int i = 0; i < Bitmap.Num(); i++)
		{
			PyTuple_SetItem(BitmapTuple, i * 4, PyLong_FromLong(Bitmap[i].R));
			PyTuple_SetItem(BitmapTuple, i * 4 + 1, PyLong_FromLong(Bitmap[i].G));
			PyTuple_SetItem(BitmapTuple, i * 4 + 2, PyLong_FromLong(Bitmap[i].B));
			PyTuple_SetItem(BitmapTuple, i * 4 + 3, PyLong_FromLong(Bitmap[i].A));
		}
		return BitmapTuple;
	}

	FNePyObjectPtr BitmapTuple = NePyNewReference(PyTuple_New(Bitmap.Num()));
	for (int i = 0; i < Bitmap.Num(); i++)
	{
		PyTuple_SetItem(BitmapTuple, i, NePyStructNew_Color(Bitmap[i]));
	}
	return BitmapTuple;
#else
	Py_RETURN_NONE;
#endif
}

PyObject* NePyMethod_GetStatUnit(PyObject* InSelf)
{
	if (!GEngine->GameViewport)
	{
		Py_RETURN_NONE;
	}

	// GEngine->GameViewport->IsStatEnabled(TEXT("Unit"));
	if (const TArray<FString>* CurrentStats = GEngine->GameViewport->GetEnabledStats())
	{
		TArray<FString> NewStats = *CurrentStats;
		NewStats.Add(TEXT("Unit"));
		GEngine->GameViewport->SetEnabledStats(NewStats);
		// GEngine->GameViewport->SetShowStats(false);
	}

	FStatUnitData* StatUnit = GEngine->GameViewport->GetStatUnitData();
	PyObject* PyDict = PyDict_New();

	/** Unit frame times filtered with a simple running average */
	PyDict_SetItem(PyDict, PyUnicode_FromString("RenderThreadTime"), PyFloat_FromDouble(StatUnit->RenderThreadTime));
	PyDict_SetItem(PyDict, PyUnicode_FromString("GameThreadTime"), PyFloat_FromDouble(StatUnit->GameThreadTime));
#define PyDict_SetItem_GPUFrameTimeArray(Num) \
		PyDict_SetItem(PyDict, PyUnicode_FromString("GPUFrameTime"#Num), PyFloat_FromDouble(StatUnit->GPUFrameTime[Num]))
	PyDict_SetItem_GPUFrameTimeArray(0);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	#if WITH_MGPU
	PyDict_SetItem_GPUFrameTimeArray(1);
	PyDict_SetItem_GPUFrameTimeArray(2);
	PyDict_SetItem_GPUFrameTimeArray(3);
	#endif
#else
	#if PLATFORM_DESKTOP
	PyDict_SetItem_GPUFrameTimeArray(1);
	PyDict_SetItem_GPUFrameTimeArray(2);
	PyDict_SetItem_GPUFrameTimeArray(3);
	#endif
#endif
	PyDict_SetItem(PyDict, PyUnicode_FromString("FrameTime"), PyFloat_FromDouble(StatUnit->FrameTime));
	PyDict_SetItem(PyDict, PyUnicode_FromString("RHITTime"), PyFloat_FromDouble(StatUnit->RHITTime));

	/** Raw equivalents of the above variables */
	PyDict_SetItem(PyDict, PyUnicode_FromString("RawRenderThreadTime"), PyFloat_FromDouble(StatUnit->RawRenderThreadTime));
	PyDict_SetItem(PyDict, PyUnicode_FromString("RawGameThreadTime"), PyFloat_FromDouble(StatUnit->RawGameThreadTime));
#define PyDict_SetItem_RawGPUFrameTimeArray(Num) \
		PyDict_SetItem(PyDict, PyUnicode_FromString("RawGPUFrameTime"#Num), PyFloat_FromDouble(StatUnit->RawGPUFrameTime[Num]))
	PyDict_SetItem_RawGPUFrameTimeArray(0);
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
	#if WITH_MGPU
	PyDict_SetItem_RawGPUFrameTimeArray(1);
	PyDict_SetItem_RawGPUFrameTimeArray(2);
	PyDict_SetItem_RawGPUFrameTimeArray(3);
	#endif
#else
	#if PLATFORM_DESKTOP
	PyDict_SetItem_RawGPUFrameTimeArray(1);
	PyDict_SetItem_RawGPUFrameTimeArray(2);
	PyDict_SetItem_RawGPUFrameTimeArray(3);
	#endif
#endif
	PyDict_SetItem(PyDict, PyUnicode_FromString("RawFrameTime"), PyFloat_FromDouble(StatUnit->RawFrameTime));
	PyDict_SetItem(PyDict, PyUnicode_FromString("RawRHITTime"), PyFloat_FromDouble(StatUnit->RawRHITTime));

	/** Time that has transpired since the last draw call */
	PyDict_SetItem(PyDict, PyUnicode_FromString("LastTime"), PyFloat_FromDouble(StatUnit->LastTime));

	return PyDict;
}

PyObject* NePyMethod_GetStatFps(PyObject* InSelf)
{
	extern ENGINE_API float GAverageFPS;
	return PyFloat_FromDouble(GAverageFPS);
}

PyObject* NePyMethod_GetStatRhi(PyObject* InSelf)
{
	PyObject* PyDict = PyDict_New();

	PyObject* PyArgs1 = PyTuple_New(MAX_NUM_GPUS);
	PyObject* PyArgs2 = PyTuple_New(MAX_NUM_GPUS);
	for (int i = 0; i < MAX_NUM_GPUS; ++i)
	{
		PyTuple_SetItem(PyArgs1, i, PyFloat_FromDouble(GNumPrimitivesDrawnRHI[i]));
		PyTuple_SetItem(PyArgs2, i, PyFloat_FromDouble(GNumDrawCallsRHI[i]));
	}
	PyDict_SetItem(PyDict, PyUnicode_FromString("GNumPrimitivesDrawnRHI"), PyArgs1);
	PyDict_SetItem(PyDict, PyUnicode_FromString("GNumDrawCallsRHI"), PyArgs2);

	return PyDict;
}

PyObject* NePyMethod_CopyPropertiesForUnrelatedObjects(PyObject* InSelf, PyObject* InArgs, PyObject* kwInArgs)
{
	PyObject* PyOldObject;
	PyObject* PyNewObject;

	PyObject* PyAggressiveDefaultSubobjectReplacement = nullptr;
	PyObject* PyCopyDeprecatedProperties = nullptr;
	PyObject* PyDoDelta = nullptr;
	PyObject* PyNotifyObjectReplacement = nullptr;
	PyObject* PyPreserveRootComponent = nullptr;
	PyObject* PyReplaceObjectClassReferences = nullptr;
	PyObject* PySkipCompilerGeneratedDefaults = nullptr;

	static char* kw_names[] = {
		(char*)"OldObject",
		(char*)"NewObject",
		(char*)"AggressiveDefaultSubobjectReplacement",
		(char*)"CopyDeprecatedProperties",
		(char*)"DoDelta",
		(char*)"NotifyObjectReplacement",
		(char*)"PreserveRootComponent",
		(char*)"ReplaceObjectClassReferences",
		(char*)"SkipCompilerGeneratedDefaults",
		nullptr
	};

	if (!PyArg_ParseTupleAndKeywords(InArgs, kwInArgs, "OO|OOOOOOO:CopyPropertiesForUnrelatedObjects", kw_names,
		&PyOldObject,
		&PyNewObject,
		&PyAggressiveDefaultSubobjectReplacement,
		&PyCopyDeprecatedProperties,
		&PyDoDelta,
		&PyNotifyObjectReplacement,
		&PyPreserveRootComponent,
		&PyReplaceObjectClassReferences,
		&PySkipCompilerGeneratedDefaults))
	{
		return nullptr;
	}

	UObject* OldObject = NePyBase::ToCppObject(PyOldObject);
	if (!OldObject)
	{
		return PyErr_Format(PyExc_Exception, "OldObject is not a UObject");
	}

	UObject* NewObject = NePyBase::ToCppObject(PyNewObject);
	if (!NewObject)
	{
		return PyErr_Format(PyExc_Exception, "NewObject is not a UObject");
	}

	UEngine::FCopyPropertiesForUnrelatedObjectsParams Params;
#if !(ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
	Params.bAggressiveDefaultSubobjectReplacement = (PyAggressiveDefaultSubobjectReplacement && PyObject_IsTrue(PyAggressiveDefaultSubobjectReplacement));
#endif
	Params.bCopyDeprecatedProperties = (PyCopyDeprecatedProperties && PyObject_IsTrue(PyCopyDeprecatedProperties));
	Params.bDoDelta = (PyDoDelta && PyObject_IsTrue(PyDoDelta));
	Params.bNotifyObjectReplacement = (PyNotifyObjectReplacement && PyObject_IsTrue(PyNotifyObjectReplacement));
	Params.bPreserveRootComponent = (PyPreserveRootComponent && PyObject_IsTrue(PyPreserveRootComponent));
	Params.bReplaceObjectClassReferences = (PyReplaceObjectClassReferences && PyObject_IsTrue(PyReplaceObjectClassReferences));
	Params.bSkipCompilerGeneratedDefaults = (PySkipCompilerGeneratedDefaults && PyObject_IsTrue(PySkipCompilerGeneratedDefaults));

	GEngine->CopyPropertiesForUnrelatedObjects(
		OldObject,
		NewObject,
		Params);

	Py_RETURN_NONE;
}

PyObject* NePyMethod_SetRandomSeed(PyObject* InSelf, PyObject* InArgs)
{
	int Seed;
	if (!PyArg_ParseTuple(InArgs, "i:SetRandomSeed", &Seed))
	{
		return nullptr;
	}

	// Thanks to Sven Mika (Ducandu GmbH) for spotting this
	FMath::RandInit(Seed);
	FGenericPlatformMath::SRandInit(Seed);
	FGenericPlatformMath::RandInit(Seed);

	Py_RETURN_NONE;
}

PyObject* NePyMethod_ClipboardCopy(PyObject* InSelf, PyObject* InArgs)
{
	char* Text;
	if (!PyArg_ParseTuple(InArgs, "s:ClipboardCopy", &Text))
	{
		return nullptr;
	}

	FPlatformApplicationMisc::ClipboardCopy(UTF8_TO_TCHAR(Text));
	Py_RETURN_NONE;
}

PyObject* NePyMethod_ClipboardPaste(PyObject* InSelf)
{
	FString Clipboard;
	FPlatformApplicationMisc::ClipboardPaste(Clipboard);
	return PyUnicode_FromString(TCHAR_TO_UTF8(*Clipboard));
}

PyObject* NePyMethodGetGameInstance(PyObject* InSelf)
{
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.OwningGameInstance)
		{
			return NePyBase::ToPy(WorldContext.OwningGameInstance);
		}
	}

	Py_RETURN_NONE;
}

PyObject* NePyMethodGetGameWorld(PyObject* InSelf)
{
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (WorldContext.OwningGameInstance && WorldContext.World())
		{
			return NePyBase::ToPy(WorldContext.World());
		}
	}

	Py_RETURN_NONE;
}

#if WITH_EDITOR
PyObject* NePyMethodDumpReflectionInfos(PyObject* InSelf, PyObject* InArgs)
{
	char* PyOutputDir = nullptr;
	if (!PyArg_ParseTuple(InArgs, "|s", &PyOutputDir))
	{
		return nullptr;
	}

	FString OutputDir;
	if (PyOutputDir)
	{
		OutputDir = PyOutputDir;
	}
	else
	{
		OutputDir = NePyGetDefaultDumpReflectionInfosDirectory();
	}

	NePyDumpReflectionInfosToFile(OutputDir);
	Py_RETURN_NONE;
}

PyObject* NePyMethodGetEditorWorld(PyObject* InSelf)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	return NePyBase::ToPy(World);
}

PyObject* NePyMethodGetEditorTimerManager(PyObject* InSelf)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}

	const auto& TimerManager = GEditor->GetTimerManager();
	return FNePySharedTimerManagerWrapper::New(TimerManager);
}

PyObject* NePyMethod_DeleteAsset(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	PyObject* PyShowConfirmation = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:DeleteAsset", &Path, &PyShowConfirmation))
	{
		return nullptr;
	}
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	bool bShowConfirmation = false;
	if (PyShowConfirmation && PyObject_IsTrue(PyShowConfirmation))
	{
		bShowConfirmation = true;
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	FAssetData Asset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(UTF8_TO_TCHAR(Path)));
#else
	FAssetData Asset = AssetRegistryModule.Get().GetAssetByObjectPath(UTF8_TO_TCHAR(Path));
#endif
	if (!Asset.IsValid())
	{
		return PyErr_Format(PyExc_Exception, "unable to find Asset %s", Path);
	}

	UObject* Object = Asset.GetAsset();
	TArray<UObject*> Objects;
	Objects.Add(Object);

	if (ObjectTools::DeleteObjects(Objects, bShowConfirmation) < 1)
	{
		if (ObjectTools::ForceDeleteObjects(Objects, bShowConfirmation) < 1)
		{
			return PyErr_Format(PyExc_Exception, "unable to delete Asset %s", Path);
		}
	}

	Py_RETURN_NONE;
}

/*以依赖顺序删除资源*/
PyObject* NePyMethod_DeleteAssets(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyToDeleteAssets = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:DeleteAssets", &PyToDeleteAssets))
	{
		return nullptr;
	}
	if (!PyList_Check(PyToDeleteAssets))
	{
		return PyErr_Format(PyExc_Exception, "PyToDeleteAssets is not a valid List");
	}
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	int Num = PyList_Size(PyToDeleteAssets);
	if (Num <= 0)
	{
		return PyErr_Format(PyExc_Exception, "PyToDeleteAssets is not a valid list");
	}

	FBinaryHeap<int> ReferencersHeap;
	TMultiMap<FName, uint32> PathToIndex;
	TMap<uint32, FName> IndexToPath;
	TArray<FName> RawObjectList;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	char* FileName = nullptr;
	FString SepStr(".");
	for (int i = 0; i < Num; ++i)
	{
		PyObject* StrObj = PyList_GetItem(PyToDeleteAssets, i);
#if PY_MAJOR_VERSION >= 3
		FileName = PyBytes_AS_STRING(PyUnicode_AsEncodedString(StrObj, "utf-8", "Error"));
#else
		FileName = PyString_AsString(FNePyObjectPtr::StealReference(PyObject_Str(StrObj)));
#endif

		TArray<FName> Referencers;
		FName ObjectPath(UTF8_TO_TCHAR(FileName));
		RawObjectList.Add(ObjectPath);
		FString PackagePathStr;
		FString AssetNameStr;
		ObjectPath.ToString().Split(SepStr, &PackagePathStr, &AssetNameStr);
		FName PackagePath(*PackagePathStr);
		AssetRegistryModule.Get().GetReferencers(PackagePath, Referencers);
		PathToIndex.Add(PackagePath, i);
		IndexToPath.Add(i, PackagePath);
		ReferencersHeap.Add(Referencers.Num(), i);
	}

	while (ReferencersHeap.Num())
	{
		int TopIndex = ReferencersHeap.Top();
		ReferencersHeap.Pop();

		FName PackagePath = IndexToPath[TopIndex];
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		FAssetData Asset = AssetRegistryModule.Get().GetAssetByObjectPath(FSoftObjectPath(RawObjectList[TopIndex].ToString()));
#else
		FAssetData Asset = AssetRegistryModule.Get().GetAssetByObjectPath(RawObjectList[TopIndex]);
#endif
		if (!Asset.IsValid())
		{
			continue;
		}

		UObject* Object = Asset.GetAsset();

		TArray<FName> Dependencies;
		AssetRegistryModule.Get().GetDependencies(PackagePath, Dependencies);

		TArray<UObject*> Objects;
		Objects.Add(Object);

		if (ObjectTools::DeleteObjects(Objects, false) < 1)
		{
			if (ObjectTools::ForceDeleteObjects(Objects, false) < 1)
			{
				UE_LOG(LogNePython, Error, TEXT("NePyMethod_DeleteAssets: delete object fail %s"), *PackagePath.ToString());
			}
		}
		int RefCnts;
		int Index;
		for (FName Outer : Dependencies)
		{
			if (!PathToIndex.Contains(Outer))
			{
				continue;
			}

			Index = PathToIndex.FindRef(Outer);
			// 考虑循环引用的情况这里需要先检查是否存在
			if (ReferencersHeap.IsPresent(Index))
			{
				RefCnts = ReferencersHeap.GetKey(Index);
				ReferencersHeap.Update(--RefCnts, Index);
			}
			else
			{
				UE_LOG(LogNePython, Warning, TEXT("NePyMethod_DeleteAssets: there may have loop ref for %s"), *PackagePath.ToString());
			}
		}
	}
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetAssets(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	PyObject* PyRecursive = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:GetAssets", &Path, &PyRecursive))
	{
		return nullptr;
	}
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	bool bRecursive = false;
	if (PyRecursive && PyObject_IsTrue(PyRecursive))
	{
		bRecursive = true;
	}

	TArray<FAssetData> Assets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(UTF8_TO_TCHAR(Path), Assets, bRecursive);

	PyObject* AssetsList = PyList_New(0);
	for (FAssetData Asset : Assets)
	{
		if (!Asset.IsValid())
		{
			continue;
		}
		PyObject* PyAsset = NePyBase::ToPy(Asset.GetAsset());
		if (PyAsset)
		{
			PyList_Append(AssetsList, PyAsset);
			Py_DECREF(PyAsset);
		}
	}

	return AssetsList;
}

PyObject* NePyMethod_GetAllAssetsPath(PyObject* InSelf, PyObject* InArgs)
{
	char* Path;
	PyObject* PyRecursive = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:GetAllAssetsPath", &Path, &PyRecursive))
	{
		return nullptr;
	}
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	bool bRecursive = false;
	if (PyRecursive && PyObject_IsTrue(PyRecursive))
	{
		bRecursive = true;
	}

	TArray<FAssetData> Assets;
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().GetAssetsByPath(UTF8_TO_TCHAR(Path), Assets, bRecursive);

	PyObject* AssetsList = PyList_New(0);
	for (FAssetData Asset : Assets)
	{
		if (!Asset.IsValid())
		{
			continue;
		}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		PyObject* PyAssetClass = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.AssetClassPath.ToString()));
		PyObject* PyObjectPath = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.GetSoftObjectPath().ToString()));
#else
		PyObject* PyAssetClass = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.AssetClass.ToString()));
		PyObject* PyObjectPath = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.ObjectPath.ToString()));
#endif
		PyList_Append(AssetsList, PyAssetClass);
		PyList_Append(AssetsList, PyObjectPath);
		Py_DECREF(PyAssetClass);
		Py_DECREF(PyObjectPath);
	}

	return AssetsList;
}

PyObject* NePyMethod_GetAssetUnused(PyObject* InSelf, PyObject* InArgs)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}

	FString ContentRelativeDir(TEXT("/Game"));
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(*ContentRelativeDir);
	TArray<FAssetData> AssetData;
	ObjectLibrary->GetAssetDataList(AssetData);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	PyObject* UnusedList = PyList_New(0);
	for (auto& Asset : AssetData)
	{
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(Asset.PackageName, Referencers);
		TArray<FName> Dependencies;
		AssetRegistryModule.Get().GetDependencies(Asset.PackageName, Dependencies);
		if (Referencers.Num() + Dependencies.Num() == 0)
		{
			//UE_LOG(LogNePython, Warning, TEXT("%s"), *Asset.PackageName.ToString());
			PyObject* PyPackageName = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.PackageName.ToString()));
			PyList_Append(UnusedList, PyPackageName);
			Py_DECREF(PyPackageName);
			continue;
		}
		if ((Referencers.Num() == 1) && (Dependencies.Num() == 0) && (Referencers[0] == Asset.PackageName))
		{
			PyObject* PyPackageName = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.PackageName.ToString()));
			PyList_Append(UnusedList, PyPackageName);
			Py_DECREF(PyPackageName);
		}
	}
	return UnusedList;
}

PyObject* NePyMethod_GetAssetUnreferenced(PyObject* InSelf, PyObject* InArgs)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}

	FString ContentRelativeDir(TEXT("/Game"));
	UObjectLibrary* ObjectLibrary = UObjectLibrary::CreateLibrary(UObject::StaticClass(), false, true);
	ObjectLibrary->LoadAssetDataFromPath(*ContentRelativeDir);
	TArray<FAssetData> AssetData;
	ObjectLibrary->GetAssetDataList(AssetData);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	PyObject* UnreferencedList = PyList_New(0);
	for (auto& Asset : AssetData) {
		TArray<FName> Referencers;
		AssetRegistry.GetReferencers(Asset.PackageName, Referencers);
		// UE_LOG(LogNePython, Warning, TEXT("%s"), *Asset.PackageName.ToString());
		if (Referencers.Num() == 0)
		{
			//UE_LOG(LogNePython, Warning, TEXT("%s"), *Asset.PackageName.ToString());
			PyObject* PyPackageName = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.PackageName.ToString()));
			PyList_Append(UnreferencedList, PyPackageName);
			Py_DECREF(PyPackageName);
			continue;
		}
		if ((Referencers.Num() == 1) && (Referencers[0] == Asset.PackageName))
		{
			PyObject* PyPackageName = PyUnicode_FromString(TCHAR_TO_UTF8(*Asset.PackageName.ToString()));
			PyList_Append(UnreferencedList, PyPackageName);
			Py_DECREF(PyPackageName);
		}
	}
	return UnreferencedList;
}

PyObject* NePyMethod_GetReachableAssets(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* RootList = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GetReachableAssets", &RootList))
	{
		return nullptr;
	}
	if (!PyList_Check(RootList))
	{
		return PyErr_Format(PyExc_Exception, "Root List is not a valid List");
	}
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	int NumLines = PyList_Size(RootList);
	if (NumLines <= 0)
	{
		return PyErr_Format(PyExc_Exception, "Root paths is not a valid list");
	}

	TSet<FName> ReachableAssets;
	TQueue<FName> OpenList;
	for (int i = 0; i < NumLines; ++i)
	{
		PyObject* StrObj = PyList_GetItem(RootList, i);
#if PY_MAJOR_VERSION >= 3
		char* FileName = PyBytes_AS_STRING(PyUnicode_AsEncodedString(StrObj, "utf-8", "Error"));
#else
		char* FileName = PyString_AsString(FNePyObjectPtr::StealReference(PyObject_Str(StrObj)));
#endif
		OpenList.Enqueue(UTF8_TO_TCHAR(FileName));
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");

	while (!OpenList.IsEmpty())
	{
		FName FileName;
		OpenList.Dequeue(FileName);
		// 提前添加，避免循环引用导致循环往队列添加同一资源
		ReachableAssets.Add(FileName);
		TArray<FName> Dependencies;
		AssetRegistryModule.Get().GetDependencies(FileName, Dependencies);
		for (FName Name : Dependencies)
		{
			if (ReachableAssets.Find(Name))
			{
				continue;
			}
			OpenList.Enqueue(Name);
		}
	}

	PyObject* AssetsList = PyList_New(0);
	for (FName Name : ReachableAssets)
	{
		PyObject* PyName = PyUnicode_FromString(TCHAR_TO_UTF8(*Name.ToString()));
		PyList_Append(AssetsList, PyName);
		Py_DECREF(PyName);
	}
	return AssetsList;
}

PyObject* NePyMethod_GetSelectedActors(PyObject* InSelf)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	FNePyObjectPtr Ret = NePyNewReference(PyList_New(0));
	USelection* Selection = GEditor->GetSelectedActors();
	for (int32 I = 0; I < Selection->Num(); ++I)
	{
		UObject* Obj = Selection->GetSelectedObject(I);
		if (!Obj->IsA<AActor>())
		{
			continue;
		}
		FNePyObjectPtr PyActor = NePyStealReference(NePyBase::ToPy((AActor*)Obj));
		PyList_Append(Ret, PyActor);
	}
	return Ret;
}

PyObject* NePyMethod_SelectNone(PyObject* InSelf)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}
	GEditor->SelectNone(true, true);
	Py_RETURN_TRUE;
}

PyObject* NePyMethod_SelectActor(PyObject* InSelf, PyObject* InArgs)
{
	if (!GEditor)
	{
		return PyErr_Format(PyExc_Exception, "no GEditor found");
	}

	PyObject* PyActor;
	bool bInSelected;
	bool bNotify;
	if (!PyArg_ParseTuple(InArgs, "Obb:SelectActor", &PyActor, &bInSelected, &bNotify))
	{
		return nullptr;
	}

	UObject* Object = nullptr;
	AActor* Actor = nullptr;
	if (NePyBase::ToCpp(PyActor, Object))
	{
		Actor = Cast<AActor>(Object);
	}

	if (!Actor)
	{
		PyErr_SetString(PyExc_TypeError, "'Actor' must have type 'AActor'");
		return nullptr;
	}

	GEditor->SelectActor(Actor, bInSelected, bNotify, true, true);
	Py_RETURN_TRUE;
}

#endif // WITH_EDITOR

// 是否正在以命令行形式启动UE
PyObject* NePyMethod_IsRunningCommandlet(PyObject* InSelf)
{
	if (IsRunningCommandlet())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

// 是否DedicatedServer
PyObject* NePyMethod_IsRunningDedicatedServer(PyObject* InSelf)
{
	if (IsRunningDedicatedServer())
	{
		Py_RETURN_TRUE;
	}
	Py_RETURN_FALSE;
}

PyObject* NePyMethod_ReloadShaderByteCode(PyObject* InSelf)
{
	FShaderCodeLibrary::OpenLibrary("Global", FPaths::ProjectContentDir());
	FShaderCodeLibrary::OpenLibrary(FApp::GetProjectName(), FPaths::ProjectContentDir());
	UE_LOG(LogNePython, Log, TEXT("reload_shader_byte_code..."));
	Py_RETURN_TRUE;
}

PyObject* NePyMethod_OpenShaderByteCodeLibrary(PyObject* InSelf, PyObject* Args)
{
	char* Name = nullptr;
	char* Dir = nullptr;

	if (!PyArg_ParseTuple(Args, "ss", &Name, &Dir))
	{
		return nullptr;
	}

	if (FShaderCodeLibrary::OpenLibrary(FString(UTF8_TO_TCHAR(Name)), FString(UTF8_TO_TCHAR(Dir))))
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

PyObject* NePyMethod_ConvertWorldToViewPort(PyObject* InSelf, PyObject* InArgs)
{
#if !WITH_NEPY_AUTO_EXPORT
	return nullptr;
#else
	PyObject* PyArgs[3] = { nullptr, nullptr, nullptr };
	if (!PyArg_ParseTuple(InArgs, "OO|O:ConvertWorldToViewPort", &PyArgs[0], &PyArgs[1], &PyArgs[2]))
	{
		return nullptr;
	}

	UWorld* World = NePyBase::TryGetWorld(PyArgs[0]);
	if (!World)
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'WorldContextObject' must have type 'UObject'");
		return nullptr;
	}

	FVector WorldPosition;
	if (!NePyBase::ToCpp(PyArgs[1], WorldPosition))
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'WorldPosition' must have type 'FVector'");
		return nullptr;
	}

	bool bPlayerViewportRelative = false;
	if (PyArgs[2] && !NePyBase::ToCpp(PyArgs[2], bPlayerViewportRelative))
	{
		PyErr_SetString(PyExc_TypeError, "arg3 'bPlayerViewportRelative' must have type 'bool'");
		return nullptr;
	}

	APlayerController* PlayerControl = World->GetFirstPlayerController();
	if (!PlayerControl)
	{
		UE_LOG(LogNePython, Warning, TEXT("ConvertWorldToViewPort Faied: get first player controller is None"));
		Py_RETURN_NONE;
	}

	FVector2D ScreenLocation;
	bool bSuccess = UGameplayStatics::ProjectWorldToScreen(PlayerControl, WorldPosition, ScreenLocation, bPlayerViewportRelative);
	if (!bSuccess)
	{
		// 正常情况也会投射失败，所以这里不要打印日志
		Py_RETURN_NONE;
	}

	FVector2D ViewportPosition2D;
	const FVector2D RoundedPosition2D(FMath::RoundToInt(ScreenLocation.X), FMath::RoundToInt(ScreenLocation.Y));
	USlateBlueprintLibrary::ScreenToViewport(PlayerControl, RoundedPosition2D, ViewportPosition2D);

	PyObject* PyPosition;
	NePyBase::ToPy(ViewportPosition2D, PyPosition);
	return PyPosition;
#endif
}

PyObject* NePyMethod_GetEnableReflectionFallback(PyObject* InSelf)
{
	if (GNePyEnableReflectionFallback)
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

PyObject* NePyMethod_SetEnableReflectionFallback(PyObject* InSelf, PyObject* InArgs)
{
	bool bEnable;
	if (!PyArg_ParseTuple(InArgs, "b:EnableReflectionFallback", &bEnable))
	{
		return nullptr;
	}
	GNePyEnableReflectionFallback = bEnable;
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetEnableReflectionFallbackLog(PyObject* InSelf)
{
	if (GNePyEnableReflectionFallbackLog)
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

PyObject* NePyMethod_SetEnableReflectionFallbackLog(PyObject* InSelf, PyObject* InArgs)
{
	bool bEnable;
	if (!PyArg_ParseTuple(InArgs, "b:EnableReflectionFallbackLog", &bEnable))
	{
		return nullptr;
	}
	GNePyEnableReflectionFallbackLog = bEnable;
	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetFrameNumber(PyObject* InSelf)
{
	return PyLong_FromLong(GFrameNumber);
}

PyObject* NePyMethod_IsInstanceOf(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj;
	PyObject* PyClassObj;
	if (!PyArg_ParseTuple(InArgs, "OO::IsInstanceOf", &PyObj, &PyClassObj))
	{
		return nullptr;
	}

	UObject* Object = nullptr;
	if (!NePyBase::ToCpp(PyObj, Object, nullptr))
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InObj' must have type 'Object'");
		return nullptr;
	}

	UClass* Class = nullptr;
	if (!NePyBase::ToCpp(PyClassObj, Class, nullptr))
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'InClass' must have type 'Class'");
		return nullptr;
	}

	if (Object->IsA(Class))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyObject* NePyMethod_IsSubclassOf(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyClassObj;
	PyObject* PySuperClassObj;
	if (!PyArg_ParseTuple(InArgs, "OO::IsSubclassOf", &PyClassObj, &PySuperClassObj))
	{
		return nullptr;
	}

	UStruct* Struct = NePyBase::ToCppStruct(PyClassObj);
	if (!Struct)
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'InClass' must have type 'Class'");
	}
	
	UStruct* SuperStruct = NePyBase::ToCppStruct(PySuperClassObj);
	if (!Struct)
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'InSuperClass' must have type 'Class'");
	}

	if (Struct->IsChildOf(SuperStruct))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

PyObject* NePyMethod_CreateLocalizedText(PyObject* InSelf, PyObject* Args)
{
	char* Namespace = nullptr;
	char* Key = nullptr;
	char* Source = nullptr;

	if (!PyArg_ParseTuple(Args, "sss:NSLOCTEXT", &Namespace, &Key, &Source))
	{
		return nullptr;
	}

#if UE_VERSION_NEWER_THAN(5, 5, 0)
	FText Ret = FText::AsLocalizable_Advanced(UTF8_TO_TCHAR(Source), UTF8_TO_TCHAR(Namespace), UTF8_TO_TCHAR(Key));
#else
	FText Ret = FInternationalization::Get().ForUseOnlyByLocMacroAndGraphNodeTextLiterals_CreateText(UTF8_TO_TCHAR(Source), UTF8_TO_TCHAR(Namespace), UTF8_TO_TCHAR(Key));
#endif

	PyObject* PyRet;
	NePyBase::ToPy(Ret, PyRet);
	return PyRet;
}

PyObject* NePyMethod_GetCVar(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	if (!PyArg_ParseTuple(InArgs, "s:GetCVar", &CVarName))
	{
		return nullptr;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		return PyErr_Format(PyExc_Exception, "Console variable %s does not exist", CVarName);
	}

	int32 Value = CVar->GetInt();
	return PyLong_FromLong(Value);
}

PyObject* NePyMethod_GetCVarFloat(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	if (!PyArg_ParseTuple(InArgs, "s:GetCVarFloat", &CVarName))
	{
		return nullptr;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		return PyErr_Format(PyExc_Exception, "Console variable %s does not exist", CVarName);
	}

	float Value = CVar->GetFloat();
	return PyFloat_FromDouble(Value);
}

PyObject* NePyMethod_GetCVarString(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	if (!PyArg_ParseTuple(InArgs, "s:GetCVarString", &CVarName))
	{
		return nullptr;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		return PyErr_Format(PyExc_Exception, "Console variable %s does not exist", CVarName);
	}

	FString Value = CVar->GetString();
	return PyUnicode_FromString(TCHAR_TO_UTF8(*Value));
}

PyObject* NePyMethod_GetCVarSafe(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	if (!PyArg_ParseTuple(InArgs, "s:GetCVarSafe", &CVarName))
	{
		return nullptr;
	}

	if (!CVarName)
	{
		Py_RETURN_NONE;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		Py_RETURN_NONE;
	}

	int32 Value = CVar->GetInt();
	return PyLong_FromLong(Value);
}

PyObject* NePyMethod_SetCVar(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	int32 Value;
	if (!PyArg_ParseTuple(InArgs, "si:SetCVar", &CVarName, &Value))
	{
		return nullptr;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		UE_LOG(LogNePython, Error, TEXT("Console variable %s does not exist"), UTF8_TO_TCHAR(CVarName));
		Py_RETURN_NONE;
	}

	CVar->Set(Value);
	Py_RETURN_NONE;
}

PyObject* NePyMethod_SetCVarFloat(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	float Value;
	if (!PyArg_ParseTuple(InArgs, "sf:SetCVarFloat", &CVarName, &Value))
	{
		return nullptr;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		UE_LOG(LogNePython, Error, TEXT("Console variable %s does not exist"), UTF8_TO_TCHAR(CVarName));
		Py_RETURN_NONE;
	}

	CVar->Set(Value);
	Py_RETURN_NONE;
}

PyObject* NePyMethod_SetCVarString(PyObject* InSelf, PyObject* InArgs)
{
	char* CVarName;
	char* Value;
	if (!PyArg_ParseTuple(InArgs, "ss:SetCVarString", &CVarName, &Value))
	{
		return nullptr;
	}

	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(UTF8_TO_TCHAR(CVarName));
	if (!CVar)
	{
		UE_LOG(LogNePython, Error, TEXT("Console variable %s does not exist"), UTF8_TO_TCHAR(CVarName));
		Py_RETURN_NONE;
	}

	CVar->Set(UTF8_TO_TCHAR(Value));
	Py_RETURN_NONE;
}

PyObject* NePyMethod_CallAllConsoleVariableSinks(PyObject* InSelf, PyObject* InArgs)
{
	IConsoleManager::Get().CallAllConsoleVariableSinks();
	Py_RETURN_NONE;
}

PyObject* NePyMethod_ExecuteConsoleCommand(PyObject* InSelf, PyObject* InArgs)
{
	char* CommandName;
	char* Arguments;
	if (!PyArg_ParseTuple(InArgs, "ss:ExecuteConsoleCommand", &CommandName, &Arguments))
	{
		return nullptr;
	}

	IConsoleObject* ConsoleObj = IConsoleManager::Get().FindConsoleObject(UTF8_TO_TCHAR(CommandName));
	if (!ConsoleObj)
	{
		return PyErr_Format(PyExc_Exception, "Console command %s does not exist", CommandName);
	}

	IConsoleCommand* ConsoleCmd = ConsoleObj->AsCommand();
	if (!ConsoleCmd)
	{
		return PyErr_Format(PyExc_Exception, "Console object %s is not a command", CommandName);
	}

	TArray<FString> Args;
	Args.Add(FString(UTF8_TO_TCHAR(Arguments)));
	ConsoleCmd->Execute(Args, nullptr, *GLog);

	Py_RETURN_NONE;
}

PyObject* NePyMethod_GetCurrentCallInfo(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyIncludeVariables = nullptr;
	PyObject* PyIncludeClosures = nullptr;
	int32 MaxDepth = 65536;

	if (!PyArg_ParseTuple(InArgs, "|OOi:GetCurrentCallInfo", &PyIncludeVariables, &PyIncludeClosures, &MaxDepth))
	{
		return nullptr;
	}

	bool bIncludeVariables = true;
	if (PyIncludeVariables && PyObject_IsTrue(PyIncludeVariables))
	{
		bIncludeVariables = true;
	}
	else if (PyIncludeVariables)
	{
		bIncludeVariables = false;
	}

	bool bIncludeClosures = true;
	if (PyIncludeClosures && PyObject_IsTrue(PyIncludeClosures))
	{
		bIncludeClosures = true;
	}
	else if (PyIncludeClosures)
	{
		bIncludeClosures = false;
	}

	FString Result = NePyCallInfo::GetCurrentCallInfo(bIncludeVariables, bIncludeClosures, MaxDepth);
	return PyUnicode_FromString(TCHAR_TO_UTF8(*Result));
}

PyObject* NePyMethod_GetTracebackCallInfo(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyTraceback;
	PyObject* PyIncludeVariables = nullptr;
	PyObject* PyIncludeClosures = nullptr;
	int32 MaxDepth = 65536;

	if (!PyArg_ParseTuple(InArgs, "O|OOi:GetTracebackCallInfo", &PyTraceback, &PyIncludeVariables, &PyIncludeClosures, &MaxDepth))
	{
		return nullptr;
	}

	// Check if traceback is None
	if (PyTraceback == Py_None)
	{
		return PyUnicode_FromString("<no traceback>");
	}

	// Check if it's a valid traceback object
	if (!PyTraceBack_Check(PyTraceback))
	{
		return PyErr_Format(PyExc_TypeError, "First argument must be a traceback object");
	}

	bool bIncludeVariables = true;
	if (PyIncludeVariables && PyObject_IsTrue(PyIncludeVariables))
	{
		bIncludeVariables = true;
	}

	bool bIncludeClosures = true;
	if (PyIncludeClosures && PyObject_IsTrue(PyIncludeClosures))
	{
		bIncludeClosures = true;
	}
	else if (PyIncludeClosures)
	{
		bIncludeClosures = false;
	}

	FString Result = NePyCallInfo::GetTracebackCallInfo(PyTraceback, bIncludeVariables, bIncludeClosures, MaxDepth);
	return PyUnicode_FromString(TCHAR_TO_UTF8(*Result));
}

// 顶层模块静态方法
PyMethodDef NePyTopModuleMethods[] = {
	{ "IsInit", NePyMethod_IsInit, METH_NOARGS, "Returns true if the on_init function has been called."},
	{ "NeedRedirect", NePyMethod_NeedRedirect, METH_NOARGS, "Returns true if redirected."},
	{ "Log", (PyCFunction)NePyMethod_Log, METH_VARARGS, "(Message: str) -> None" },
	{ "LogDisplay", (PyCFunction)NePyMethod_LogDisplay, METH_VARARGS, "(Message: str) -> None" },
	{ "LogWarning", (PyCFunction)NePyMethod_LogWarning, METH_VARARGS, "(Message: str) -> None" },
	{ "LogError", (PyCFunction)NePyMethod_LogError, METH_VARARGS, "(Message: str) -> None" },
	{ "LogVerbose", (PyCFunction)NePyMethod_LogVerbose, METH_VARARGS, "(Message: str) -> None" },
	{ "LogFatal", (PyCFunction)NePyMethod_LogFatal, METH_VARARGS, "(Message: str) -> None" },
	{ "AddOnScreenDebugMessage", (PyCFunction)NePyMethod_AddOnScreenDebugMessage, METH_VARARGS, "(Key: int, TimeToDisplay: float, Message: str) -> None" },
	{ "PrintString", (PyCFunction)NePyMethod_PrintString, METH_VARARGS, "(Message: str, TimeToDisplay: float = ..., Color: Color = ...) -> None" },
	{ "RequestExit", (PyCFunction)NePyMethod_RequestExit, METH_VARARGS, "(bForce: bool) -> None" },
	{ "GetEngineDir", (PyCFunction)NePyMethod_GetEngineDir, METH_NOARGS, "() -> str" },
	{ "GetEngineContentDir", (PyCFunction)NePyMethod_GetEngineContentDir, METH_NOARGS, "() -> str" },
	{ "GetEngineConfigDir", (PyCFunction)NePyMethod_GetEngineConfigDir, METH_NOARGS, "() -> str" },
	{ "GetProjectDir", (PyCFunction)NePyMethod_GetProjectDir, METH_NOARGS, "() -> str" },
	{ "GetContentDir", (PyCFunction)NePyMethod_GetContentDir, METH_NOARGS, "() -> str" },
	{ "GetDocumentDir", (PyCFunction)NePyMethod_GetDocumentDir, METH_NOARGS, "() -> str" },
	{ "GetConfigDir", (PyCFunction)NePyMethod_GetConfigDir, METH_NOARGS, "() -> str" },
	{ "GetLogDir", (PyCFunction)NePyMethod_GetLogDir, METH_NOARGS, "() -> str" },
	{ "GetLogFilename", (PyCFunction)NePyMethod_GetLogFilename, METH_NOARGS, "() -> str" },
	{ "GetGameSavedDir", (PyCFunction)NePyMethod_GetGameSavedDir, METH_NOARGS, "() -> str" },
	{ "GetGameUserDeveloperDir", (PyCFunction)NePyMethod_GetGameUserDeveloperDir, METH_NOARGS, "() -> str" },
	{ "ConvertRelativePathToFull", (PyCFunction)NePyMethod_ConvertRelativePathToFull, METH_VARARGS, "(Path: str) -> str" },
	{ "ConvertAbsolutePathApp", (PyCFunction)NePyMethod_ConvertAbsolutePathApp, METH_VARARGS, "(Path: str) -> str" },
	{ "ObjectPathToPackageName", (PyCFunction)NePyMethod_ObjectPathToPackageName, METH_VARARGS, "(Path: str) -> str" },
	{ "GetPath", (PyCFunction)NePyMethod_GetPath, METH_VARARGS, "(Path: str) -> str" },
	{ "GetBaseFilename", (PyCFunction)NePyMethod_GetBaseFilename, METH_VARARGS, "(Path: str) -> str" },
	{ "FindFile", (PyCFunction)NePyMethod_FindFile, METH_VARARGS, "(FullName: str, Path: str) -> bool" },
	{ "GetFile", (PyCFunction)NePyMethod_GetFile, METH_VARARGS, "(FullName: str, Path: str) -> bytes" },
#if WITH_EDITOR
	{ "EncryptBuffer", (PyCFunction)NePyMethod_EncryptBuffer, METH_VARARGS, "" },
#endif

	{ "IsOldStyleSubclassingEnabled", NePyCFunctionCast(&NePyMethod_IsOldStyleSubclassingEnabled), METH_NOARGS, "() -> bool" },
	{ "DisableOldStyleSubclassing", NePyCFunctionCast(&NePyMethod_DisableOldStyleSubclassing), METH_VARARGS, "(bDisable: bool) -> None" },
	{ "IsGeneratedTypeEnabled", NePyCFunctionCast(&NePyMethod_IsGeneratedTypeEnabled), METH_NOARGS, "() -> bool" },
	{ "DisableGeneratedType", NePyCFunctionCast(&NePyMethod_DisableGeneratedType), METH_VARARGS, "(bDisable: bool) -> None" },
	{ "RegenerateFunctions", NePyCFunctionCast(&NePyMethod_RegenerateFunctions), METH_O, "[T: Object](PyType: type[T]) -> None" },
	{ "uclass", NePyCFunctionCast(&NePyMethod_UClassDecorator), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "ustruct", NePyCFunctionCast(&NePyMethod_UStructDecorator), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "uenum", NePyCFunctionCast(&NePyMethod_UEnumDecorator), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "uvalue", NePyCFunctionCast(&NePyMethod_UValueFunction), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "uproperty", NePyCFunctionCast(&NePyMethod_UPropertyFunction), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "uparam", NePyCFunctionCast(&NePyMethod_UParamFunction), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "ufunction", NePyCFunctionCast(&NePyMethod_UFunctionDecorator), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "udelegate", NePyCFunctionCast(&NePyMethod_UDelegateFunction), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "ucomponent", NePyCFunctionCast(&NePyMethod_UComponentFunction), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "ref", NePyCFunctionCast(&NePyMethod_RefParamFunction), METH_VARARGS | METH_KEYWORDS, "WithoutStub" },  // type hints define in builtin_doc.pyi
	{ "clsref", NePyCFunctionCast(&NePyMethod_ClassReferenceFunction), METH_O, "[T: Object](ObjType: type[T]) -> type[TSubclassOf[T]];(ObjType: Class) -> type[TSubclassOf[Object]]" },
	{ "softobjref", NePyCFunctionCast(&NePyMethod_SoftObjectReferenceFunction), METH_O, "[T: Object](ObjType: type[T]) -> type[TSoftObjectPtr[T]];(ObjType: Class) -> type[SoftPtr]" },
	{ "softclsref", NePyCFunctionCast(&NePyMethod_SoftClassReferenceFunction), METH_O, "[T: Object](ObjType: type[T]) -> type[TSoftClassPtr[T]];(ObjType: Class) -> type[TSoftClassPtr[Object]]" },
	{ "weakobjref", NePyCFunctionCast(&NePyMethod_WeakObjectReferenceFunction), METH_O, "[T: Object](ObjType: type[T]) -> type[TWeakObjectPtr[T]];(ObjType: Class) -> type[TWeakObjectPtr[Object]]" },
	{ "GetTypeFromClass", NePyCFunctionCast(&NePyMethod_GetTypeFromClass), METH_O, "(Class: Class) -> type" },
	{ "GetTypeFromStruct", NePyCFunctionCast(&NePyMethod_GetTypeFromStruct), METH_O, "(Struct: ScriptStruct) -> type" },
	{ "GetTypeFromEnum", NePyCFunctionCast(&NePyMethod_GetTypeFromEnum), METH_O, "(Enum: Enum) -> type" },

	{ "MountPak", (PyCFunction)NePyMethod_MountPak, METH_VARARGS, "" },
	{ "UnmountPak", (PyCFunction)NePyMethod_UnmountPak, METH_VARARGS, "" },
	{ "CreateWorld", (PyCFunction)NePyMethod_CreateWorld, METH_VARARGS, "(WorldType: EWorldType = ...) -> World" },
	{ "ParsePropertyFlags", (PyCFunction)NePyMethod_ParsePropertyFlags, METH_VARARGS, "(Flags: int) -> list[str]" },
	{ "GetTransientPackage", (PyCFunction)NePyMethod_GetTransientPackage, METH_NOARGS, "() -> Package" },
	{ "GetIniFilenameFromObjectsReference", (PyCFunction)NePyMethod_GetIniFilenameFromObjectsReference, METH_O, "(ObjectsReferenceString: str) -> str" },
	{ "ResolveIniObjectsReference", (PyCFunction)NePyMethod_ResolveIniObjectsReference, METH_O, "(ObjectReference: str, IniFilename: str = ..., bThrow: bool = ...) -> str" },
	{ "LoadPackage", (PyCFunction)NePyMethod_LoadPackage, METH_VARARGS, "(Name: str) -> Package" },
	{ "LoadPackageAsync", (PyCFunction)NePyMethod_LoadPackageAsync, METH_VARARGS, "(Name: str, Callback: typing.Callable[[str, Package, EAsyncLoadingResult], typing.Any], Priority: int = ..., PackageFlags: EPackageFlags = ...) -> None" },
#if WITH_EDITOR
	{ "UnloadPackage", (PyCFunction)NePyMethod_UnloadPackage, METH_VARARGS, "(PackageToUnload: Package) -> None" },
	{ "GetPackageFileName", (PyCFunction)NePyMethod_GetPackageFileName, METH_VARARGS, "(Name: str) -> str" },
#endif
	{ "FindClass", (PyCFunction)NePyMethod_FindClass, METH_VARARGS, "(Name: str, Outer: Object = ...) -> Class" },
	{ "LoadClass", (PyCFunction)NePyMethod_LoadClass, METH_VARARGS, "(Name: str) -> Class" },
	{ "AsyncLoadClass", (PyCFunction)NePyMethod_AsyncLoadClass, METH_VARARGS, "(Name: str, Callback: typing.Callable[[str, Class], typing.Any], Priority: int = ...) -> None" },
	{ "FindObject", (PyCFunction)NePyMethod_FindObject, METH_VARARGS, "(Name: str, Outer: Object = ...) -> Object" },
	{ "LoadObject", (PyCFunction)NePyMethod_LoadObject, METH_VARARGS, "[T: Object](Class: type[T] | TSubclassOf[T], Name: str) -> T;(Class: Class, Name: str) -> Object" },
	{ "AsyncLoadObject", (PyCFunction)NePyMethod_AsyncLoadObject, METH_VARARGS, "(Name: str, Callback: typing.Callable[[str, Object], typing.Any], Priority: int = ...) -> None" },
	{ "FindStruct", (PyCFunction)NePyMethod_FindStruct, METH_VARARGS, "(Name: str, Outer: Object = ...) -> ScriptStruct" },
	{ "LoadStruct", (PyCFunction)NePyMethod_LoadStruct, METH_VARARGS, "(Name: str) -> ScriptStruct" },
	{ "FindEnum", (PyCFunction)NePyMethod_FindEnum, METH_VARARGS, "(Name: str, Outer: Object = ...) -> Enum" },
	{ "LoadEnum", (PyCFunction)NePyMethod_LoadEnum, METH_VARARGS, "(Name: str) -> Enum" },
	{ "CancelAsyncLoading", (PyCFunction)NePyMethod_CancelAsyncLoading, METH_NOARGS, "() -> None" },
	{ "GetAsyncLoadPercentage", (PyCFunction)NePyMethod_GetAsyncLoadPercentage, METH_O, "(PackageName: str) -> float" },
	{ "FlushAsyncLoading", (PyCFunction)NePyMethod_FlushAsyncLoading, METH_VARARGS, "(PackageID: int = ...) -> None" },
	{ "GetNumAsyncPackages", (PyCFunction)NePyMethod_GetNumAsyncPackages, METH_NOARGS, "() -> int" },
	{ "IsLoading", (PyCFunction)NePyMethod_IsLoading, METH_NOARGS, "() -> bool" },
	{ "FindPackage", (PyCFunction)NePyMethod_FindPackage, METH_VARARGS, "(PackageName: str) -> Package" },
	{ "CreatePackage", (PyCFunction)NePyMethod_CreatePackage, METH_VARARGS, "(PackageName: str) -> Package" },
	{ "NewObject", (PyCFunction)NePyMethod_NewObject, METH_VARARGS, "[T: Object](Class: type[T] | TSubclassOf[T], Outer: Object = ..., Name: str = ..., Flags: int = ...) -> T;(Class: Class, Outer: Object = ..., Name: str = ..., Flags: int = ...) -> Object" },
	{ "StringToGuid", (PyCFunction)NePyMethod_StringToGuid, METH_VARARGS, "(Str: str) -> Guid" },
	{ "GuidToString", (PyCFunction)NePyMethod_GuidToString, METH_VARARGS, "(Guid: Guid) -> str" },
	{ "TickSlate", (PyCFunction)NePyMethod_TickSlate, METH_NOARGS, "() -> None" },
	{ "TickEngine", (PyCFunction)NePyMethod_TickEngine, METH_VARARGS, "(DeltaSeconds: float, bIdle: bool) -> None" },
#if WITH_EDITOR
	{ "TickRenderingTickables", (PyCFunction)NePyMethod_TickRenderingTickables, METH_NOARGS, "() -> None" },
#endif
	{ "AddTicker", (PyCFunction)NePyMethod_AddTicker, METH_VARARGS, "(InCallback: typing.Callable[[float], bool], InDelay: float = ...) -> TickerHandle" },
	{ "RemoveTicker", (PyCFunction)NePyMethod_RemoveTicker, METH_O, "(InHandle: TickerHandle) -> None" },
	{ "GetDeltaTime", (PyCFunction)NePyMethod_GetDeltaTime, METH_NOARGS, "() -> float" },
	{ "GetAllWorlds", (PyCFunction)NePyMethod_GetAllWorlds, METH_NOARGS, "() -> list[World]" },
	{ "GetGameViewportSize", (PyCFunction)NePyMethod_GetGameViewportSize, METH_NOARGS, "() -> tuple[float, float]" },
	{ "GetResolution", (PyCFunction)NePyMethod_GetResolution, METH_NOARGS, "() -> tuple[float, float]" },
	{ "GetViewportScreenshot", (PyCFunction)NePyMethod_GetViewportScreenshot, METH_VARARGS, "(bAsIntList: bool) -> list[Color] | list[tuple[float, float, float, float]]" },
	{ "GetStatUnit", (PyCFunction)NePyMethod_GetStatUnit, METH_NOARGS, "() -> dict" },
	{ "GetStatFps", (PyCFunction)NePyMethod_GetStatFps, METH_NOARGS, "() -> float" },
	{ "GetStatRhi", (PyCFunction)NePyMethod_GetStatRhi, METH_NOARGS, "() -> dict" },
	{ "CopyPropertiesForUnrelatedObjects", NePyCFunctionCast(NePyMethod_CopyPropertiesForUnrelatedObjects), METH_VARARGS | METH_KEYWORDS, "" },
	{ "SetRandomSeed", (PyCFunction)NePyMethod_SetRandomSeed, METH_VARARGS, "(Seed: int) -> None" },
	{ "ClipboardCopy", (PyCFunction)NePyMethod_ClipboardCopy, METH_VARARGS, "(Text: str) -> None" },
	{ "ClipboardPaste", (PyCFunction)NePyMethod_ClipboardPaste, METH_NOARGS, "() -> str" },
	{ "GetGameInstance", (PyCFunction)NePyMethodGetGameInstance, METH_NOARGS, "() -> GameInstance" },
	{ "GetGameWorld", (PyCFunction)NePyMethodGetGameWorld, METH_NOARGS, "() -> World" },
#if WITH_EDITOR
	{ "DumpReflectionInfos", NePyMethodDumpReflectionInfos, METH_VARARGS, "" },
	{ "GetEditorWorld", (PyCFunction)NePyMethodGetEditorWorld, METH_NOARGS, "() -> World" },
	{ "GetEditorTimerManager", (PyCFunction)NePyMethodGetEditorTimerManager, METH_NOARGS, "() -> TimerManagerWrapper" },
	{ "DeleteAsset", NePyMethod_DeleteAsset, METH_VARARGS, "(Path: str, bShowConfirmation: bool = ...) -> None" },
	{ "DeleteAssets", NePyMethod_DeleteAssets, METH_VARARGS, "(Paths: list[str]) -> None " },
	{ "GetAssets", NePyMethod_GetAssets, METH_VARARGS, "" },
	{ "GetAllAssetsPath", NePyMethod_GetAllAssetsPath, METH_VARARGS, "" },
	{ "GetAssetUnused", NePyMethod_GetAssetUnused, METH_VARARGS, "" },
	{ "GetAssetUnreferenced", NePyMethod_GetAssetUnreferenced, METH_VARARGS, "" },
	{ "GetReachableAssets", NePyMethod_GetReachableAssets, METH_VARARGS, "" },
	{ "GetSelectedActors", (PyCFunction)NePyMethod_GetSelectedActors, METH_VARARGS, "() -> list[Actor]" },
	{ "SelectNone", (PyCFunction)NePyMethod_SelectNone, METH_VARARGS, "() -> bool" },
	{ "SelectActor", (PyCFunction)NePyMethod_SelectActor, METH_VARARGS, "(Actor: Actor, bInSelected: bool, bNotify: bool) -> bool" },

#endif
	{ "IsRunningCommandlet", (PyCFunction)NePyMethod_IsRunningCommandlet, METH_NOARGS, "() -> bool" },
	{ "IsRunningDedicatedServer", (PyCFunction)NePyMethod_IsRunningDedicatedServer, METH_NOARGS, "() -> bool" },
	{ "ReloadShaderByteCode", (PyCFunction)NePyMethod_ReloadShaderByteCode, METH_NOARGS, "() -> None" },
	{ "OpenShaderByteCodeLibrary", (PyCFunction)NePyMethod_OpenShaderByteCodeLibrary, METH_VARARGS, "(Name: str, Directory: str) -> bool" },
	{ "ConvertWorldToViewport", NePyMethod_ConvertWorldToViewPort, METH_VARARGS, "(World: World, WorldPosition: Vector, bPlayerViewportRelative: bool = ...) -> Vector" },
	{ "GetEnableReflectionFallback", (PyCFunction)NePyMethod_GetEnableReflectionFallback, METH_NOARGS, "() -> bool" },
	{ "SetEnableReflectionFallback", NePyMethod_SetEnableReflectionFallback, METH_VARARGS, "(bEnable: bool) -> None" },
	{ "GetEnableReflectionFallbackLog", (PyCFunction)NePyMethod_GetEnableReflectionFallbackLog, METH_NOARGS, "() -> bool" },
	{ "SetEnableReflectionFallbackLog", NePyMethod_SetEnableReflectionFallbackLog, METH_VARARGS, "(bEnable: bool) -> None" },
	{ "GetFrameNumber", (PyCFunction)NePyMethod_GetFrameNumber, METH_NOARGS, "() -> int" },
	{ "IsInstanceOf", (PyCFunction)NePyMethod_IsInstanceOf, METH_NOARGS, "[T: Object](InObj: Object, InClass: TSubclassOf[T] | type[T]) -> typing.TypeGuard[T];(InObj: Object, InClass: Class) -> bool" },
	{ "IsSubclassOf", (PyCFunction)NePyMethod_IsSubclassOf, METH_NOARGS, "[T: Object](InClass: Class, InSuperClass: type[T]) -> typing.TypeGuard[type[T]];[T: Object](InClass: Class, InSuperClass: TSubclassOf[T]) -> typing.TypeGuard[TSubclassOf[T]];(InClass: Class, InSuperClass: Class) -> bool" },
	{ "NSLOCTEXT", (PyCFunction)NePyMethod_CreateLocalizedText, METH_VARARGS, "(Namespace: str, Key: str, Source: str) -> Text" },

	{ "GetCVar", (PyCFunction)NePyMethod_GetCVar, METH_VARARGS, "(CVarName: str) -> int" },
	{ "GetCVarFloat", (PyCFunction)NePyMethod_GetCVarFloat, METH_VARARGS, "(CVarName: str) -> float" },
	{ "GetCVarString", (PyCFunction)NePyMethod_GetCVarString, METH_VARARGS, "(CVarName: str) -> str" },
	{ "GetCVarSafe", (PyCFunction)NePyMethod_GetCVarSafe, METH_VARARGS, "(CVarName: str) -> int | None" },
	{ "SetCVar", (PyCFunction)NePyMethod_SetCVar, METH_VARARGS, "(CVarName: str, Value: int) -> None" },
	{ "SetCVarFloat", (PyCFunction)NePyMethod_SetCVarFloat, METH_VARARGS, "(CVarName: str, Value: float) -> None" },
	{ "SetCVarString", (PyCFunction)NePyMethod_SetCVarString, METH_VARARGS, "(CVarName: str, Value: str) -> None" },
	{ "CallAllConsoleVariableSinks", (PyCFunction)NePyMethod_CallAllConsoleVariableSinks, METH_NOARGS, "() -> None" },
	{ "ExecuteConsoleCommand", (PyCFunction)NePyMethod_ExecuteConsoleCommand, METH_VARARGS, "(CommandName: str, Arguments: str) -> None" },

	{ "GetCurrentCallInfo", (PyCFunction)NePyMethod_GetCurrentCallInfo, METH_VARARGS, "(bIncludeVariables: bool = ..., bIncludeClosures: bool = ..., MaxDepth: int = ...) -> str" },
	{ "GetTracebackCallInfo", (PyCFunction)NePyMethod_GetTracebackCallInfo, METH_VARARGS, "(Traceback: traceback, bIncludeVariables: bool = ..., bIncludeClosures: bool = ..., MaxDepth: int = ...) -> str" },

	{NULL, NULL}  /* Sentinel */
};
