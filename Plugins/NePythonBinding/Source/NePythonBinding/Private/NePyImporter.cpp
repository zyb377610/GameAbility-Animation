#include "NePyImporter.h"
#include "NePyBase.h"
#include "NePyMemoryAllocator.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
// #include "openssl/aes.h"
#include "Misc/AES.h"
#include "marshal.h"

// --------------------------------------------------

struct FNePyImportOptions
{
	// 输入是.py源码
	bool bIsPy;
	// 输入是.pyc
	bool bIsPyc;
};

struct FNePyImportLoader : public PyObject
{
	// import file name
	FString FilePath;
	//
	bool bIsPackage;
	//
	FNePyImportOptions Options;
};

static PyTypeObject FNePyImportLoader_Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyImportLoader", /* tp_name */
	sizeof(FNePyImportLoader), /* tp_basicsize */
};

// tp_new
static PyObject* PyObjectFNePyImportLoader_New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyImportLoader* RetVal = (FNePyImportLoader*)InType->tp_alloc(InType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetVal);
	new (&RetVal->FilePath) FString();
	return RetVal;
}

// tp_dealloc
void PyObjectFNePyImportLoader_Dealloc(FNePyImportLoader* InSelf)
{
	InSelf->FilePath.~FString();
	InSelf->ob_type->tp_free(InSelf);
}

// tp_repr
static PyObject* PyObjectFNePyImportLoader_Repr(FNePyImportLoader* InSelf)
{
	return NePyString_FromFormat("<NePyImportLoader from '%s'>",
		TCHAR_TO_UTF8(*InSelf->FilePath));
}

// Loader.create_module(spec)
static PyObject* FNePyImportLoader_CreateModule(FNePyImportLoader* InSelf, PyObject* InArgs)
{
	// Use default module creation semantics.
	Py_RETURN_NONE;
}

// Loader.exec_module(module)
static PyObject* FNePyImportLoader_ExecModule(FNePyImportLoader* InSelf, PyObject* InPyModule)
{
	PyObject* PyModuleDict = PyModule_GetDict(InPyModule);
	PyObject* PyModuleName = PyDict_GetItemString(PyModuleDict, "__name__");
	const char* ModuleNameStr = NePyString_AsString(PyModuleName);

	FNePyObjectPtr PyCode;
	if (InSelf->Options.bIsPy)
	{
		FString CodeString;
		if (!FFileHelper::LoadFileToString(CodeString, *InSelf->FilePath))
		{
			PyErr_Format(PyExc_RuntimeError, "import '%s' failed! can't read file: %s",
				ModuleNameStr, TCHAR_TO_UTF8(*InSelf->FilePath));
			return nullptr;
		}

		PyCode = NePyStealReference(Py_CompileString(TCHAR_TO_UTF8(*CodeString), TCHAR_TO_UTF8(*InSelf->FilePath), Py_file_input));
		if (!PyCode)
		{
			//PyErr_Print();
			//PyErr_Format(PyExc_Exception, "import '%s' failed! compile error", ModuleNameStr);
			return nullptr;
		}
	}
	else if (InSelf->Options.bIsPyc)
	{
		TArray<uint8> ByteBuffer;
		if (!FFileHelper::LoadFileToArray(ByteBuffer, *InSelf->FilePath))
		{
			PyErr_Format(PyExc_RuntimeError, "import '%s' failed! can't read file: %s",
				ModuleNameStr, TCHAR_TO_UTF8(*InSelf->FilePath));
			return nullptr;
		}

		const uint8* BufPtr = ByteBuffer.GetData();
		uint32 BufLen = ByteBuffer.Num();

		// 参考 pythonrun.c run_pyc_file()
		int32 magic = *(int32*)BufPtr;
		if (magic != PyImport_GetMagicNumber())
		{
			PyErr_Format(PyExc_RuntimeError, "import '%s' failed! bad magic number in .pyc file: %s",
				ModuleNameStr, TCHAR_TO_UTF8(*InSelf->FilePath));
			return nullptr;
		}

		/* Skip the rest of the header. */
		constexpr uint32 SkipBytes = 4 * sizeof(int32);
		if (SkipBytes > BufLen)
		{
			PyErr_Format(PyExc_RuntimeError, "import '%s' failed! corrupted .pyc file: %s",
				ModuleNameStr, TCHAR_TO_UTF8(*InSelf->FilePath));
			return nullptr;
		}
		BufPtr += SkipBytes;
		BufLen -= SkipBytes;

		PyCode = NePyStealReference(PyMarshal_ReadObjectFromString((char*)BufPtr, BufLen));
		if (!PyCode)
		{
			//PyErr_Print();
			//PyErr_Format(PyExc_Exception, "import '%s' failed! marshal.loads error", ModuleNameStr);
			return nullptr;
		}
	}
	else
	{
		TArray<uint8> ByteBuffer;
		if (!FFileHelper::LoadFileToArray(ByteBuffer, *InSelf->FilePath))
		{
			PyErr_Format(PyExc_RuntimeError, "import '%s' failed! can't read file: %s",
				ModuleNameStr, TCHAR_TO_UTF8(*InSelf->FilePath));
			return nullptr;
		}

		// Decrypt
		uint8* CodeBuffer = nullptr;
		uint32 CodeLen;
		{
			const uint8* BufPtr = ByteBuffer.GetData();
			uint32 BufLen = ByteBuffer.Num();
			CodeLen = *(uint32*)BufPtr;
			BufPtr += sizeof(uint32);
			BufLen -= sizeof(uint32);

			uint32 BlockCount = BufLen / FAES::AESBlockSize;
			CodeBuffer = (uint8*)FMemory::Malloc(BufLen, 16);
			memcpy(CodeBuffer, BufPtr, BufLen);

			FAES::FAESKey Key;
			NePyImporterEncryptKey(Key.Key);
			FAES::DecryptData(CodeBuffer, BufLen, Key);
		}

		PyCode = NePyStealReference(PyMarshal_ReadObjectFromString((char*)CodeBuffer, CodeLen));
		FMemory::Free(CodeBuffer);
		if (!PyCode)
		{
			//PyErr_Print();
			//PyErr_Format(PyExc_Exception, "import '%s' failed! marshal.loads error", ModuleNameStr);
			return nullptr;
		}
	}

	if (PyDict_SetItemString(PyModuleDict, "__file__", ((PyCodeObject*)PyCode.Get())->co_filename) != 0)
	{
		PyErr_Format(PyExc_Exception, "import '%s' failed! set __file__ error", ModuleNameStr);
		return nullptr;
	}

	if (InSelf->bIsPackage)
	{
		FString ParentPath = FPaths::GetPath(InSelf->FilePath);
		FNePyObjectPtr PyPath = NePyStealReference(PyList_New(1));
		PyList_SetItem(PyPath, 0, NePyString_FromString(TCHAR_TO_UTF8(*ParentPath)));
		if (PyDict_SetItemString(PyModuleDict, "__path__", PyPath) != 0)
		{
			PyErr_Format(PyExc_Exception, "import '%s' failed! set __path__ error", ModuleNameStr);
			return nullptr;
		}
	}

	FNePyObjectPtr PyExecResult = NePyStealReference(PyImport_ExecCodeModule((char*)ModuleNameStr, PyCode));
	if (!PyExecResult)
	{
		//PyErr_Print();
		//PyErr_Format(PyExc_Exception, "import '%s' failed! exec error", ModuleNameStr);
		return nullptr;
	}

	Py_RETURN_NONE;
}

#if PY_MAJOR_VERSION < 3
static PyObject* FNePyImportLoader_LoadModule(FNePyImportLoader* InSelf, PyObject* InArgs)
{
	const char* ModuleNameStr;
	if (!PyArg_ParseTuple(InArgs, "s:load_module", &ModuleNameStr))
	{
		return nullptr;
	}

	PyObject* PyModule = PyImport_AddModule(ModuleNameStr);
	if (!PyModule)
	{
		return PyErr_Format(PyExc_Exception, "PyImport_AddModule failed: %s", ModuleNameStr);
	}

	if (!FNePyImportLoader_ExecModule(InSelf, PyModule))
	{
		PyObject* PyModules = PyImport_GetModuleDict();
		if (PyDict_GetItemString(PyModules, ModuleNameStr))
		{
			if (PyDict_DelItemString(PyModules, ModuleNameStr) < 0)
			{
				Py_FatalError("import:  deleting existing key in sys.modules failed");
			}
		}
		return nullptr;
	}

	Py_INCREF(PyModule);
	return PyModule;
}
#endif // PY_MAJOR_VERSION < 3

static PyMethodDef FNePyImportLoader_methods[] = {
#if PY_MAJOR_VERSION >= 3
	{"create_module", NePyCFunctionCast(&FNePyImportLoader_CreateModule), METH_VARARGS, nullptr},
	{"exec_module", NePyCFunctionCast(&FNePyImportLoader_ExecModule), METH_O, nullptr},
#else
	{"load_module", NePyCFunctionCast(&FNePyImportLoader_LoadModule), METH_VARARGS, nullptr},
#endif
	{ NULL } /* Sentinel */
};

// --------------------------------------------------

static TArray<TPair<FString, FNePyImportOptions>> NePyValidExts;

struct FNePyImportFinder : public PyObject
{
	// import search path
	FString Path;
};

static PyTypeObject FNePyImportFinder_Type = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyImportFinder", /* tp_name */
	sizeof(FNePyImportFinder), /* tp_basicsize */
};

// tp_new
static PyObject* PyObjectFNePyImportFinder_New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyImportFinder* RetVal = (FNePyImportFinder*)InType->tp_alloc(InType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetVal);
	new (&RetVal->Path) FString();
	return RetVal;
}


// tp_dealloc
void PyObjectFNePyImportFinder_Dealloc(FNePyImportFinder* InSelf)
{
	InSelf->Path.~FString();
	InSelf->ob_type->tp_free(InSelf);
}

// tp_init
static int PyObjectFNePyImportFinder_Init(FNePyImportFinder* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	const char* PathStr;
	if (!PyArg_ParseTuple(InArgs, "s", &PathStr))
	{
		return -1;
	}

	FString FindPath = FPaths::ConvertRelativePathToFull(UTF8_TO_TCHAR(PathStr));
	FString ContentPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir());
	if (FindPath.StartsWith(ContentPath, ESearchCase::CaseSensitive))
	{
		FindPath.RightChopInline(ContentPath.Len());
		FindPath = FPaths::Combine(FPaths::ProjectContentDir(), FindPath);
	}
	else if (ContentPath.StartsWith(TEXT("../")) && FindPath.StartsWith(TEXT("/")))
	{
		FString ContentPath2 = ContentPath.RightChop(2);
		while (ContentPath2.StartsWith(TEXT("/../")))
		{
			ContentPath2.RightChopInline(3);
		}
		if (FindPath.StartsWith(ContentPath2, ESearchCase::CaseSensitive))
		{
			FindPath.RightChopInline(ContentPath2.Len());
			FindPath = FPaths::Combine(FPaths::ProjectContentDir(), FindPath);
		}
	}
	InSelf->Path = MoveTemp(FindPath);
	return 0;
}

// tp_repr
static PyObject* PyObjectFNePyImportFinder_Repr(FNePyImportFinder* InSelf)
{
	return NePyString_FromFormat("<NePyImportFinder from '%s'>",
		TCHAR_TO_UTF8(*InSelf->Path));
}

static FNePyImportLoader* _FNePyImportFinder_CreateLoader(const FString& Path, const char* ModuleNameStr)
{
	FString ModuleName = UTF8_TO_TCHAR(ModuleNameStr);
	FString TailModuleName = ModuleName;
	int32 TailPos;
	if (TailModuleName.FindLastChar(TEXT('.'), TailPos))
	{
		TailModuleName.RightChopInline(TailPos + 1);
	}

	FString FilePath;
	bool bIsPackage = false;
	FNePyImportOptions Options = {false, false};

	for (const auto& Pair : NePyValidExts)
	{
		const FString& Ext = Pair.Key;
		Options = Pair.Value;

		FString TempFilePath = Path / TailModuleName / (FString(TEXT("__init__")) + Ext);
		if (FPaths::FileExists(TempFilePath))
		{
			FilePath = TempFilePath;
			bIsPackage = true;
			break;
		}

		TempFilePath = Path / (TailModuleName + Ext);
		if (FPaths::FileExists(TempFilePath))
		{
			FilePath = TempFilePath;
			bIsPackage = false;
			break;
		}
	}

	if (FilePath.Len() == 0)
	{
		return nullptr;
	}


	FNePyImportLoader* PyLoader = (FNePyImportLoader*)PyObject_Call(
		(PyObject*)(&FNePyImportLoader_Type), NePyStealReference(PyTuple_New(0)), nullptr);
	PyLoader->FilePath = FilePath;
	PyLoader->bIsPackage = bIsPackage;
	PyLoader->Options = Options;
	return PyLoader;
}

#if PY_MAJOR_VERSION >= 3
// PathEntryFinder.find_spec(fullname, target=None)
static PyObject* FNePyImportFinder_FindSpec(FNePyImportFinder* InSelf, PyObject* InArgs)
{
	const char* ModuleNameStr;
	PyObject* PyTarget = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:find_spec", &ModuleNameStr, &PyTarget))
	{
		return nullptr;
	}

	FNePyImportLoader* PyLoader = _FNePyImportFinder_CreateLoader(InSelf->Path, ModuleNameStr);
	if (!PyLoader)
	{
		Py_RETURN_NONE;
	}

	FNePyObjectPtr PyBootstarp = NePyStealReference(PyImport_ImportModule("_frozen_importlib"));
	PyObject* PyBootstarpDict = PyModule_GetDict(PyBootstarp);
	PyObject* PyModuleSpec = PyDict_GetItemString(PyBootstarpDict, "ModuleSpec");

	FNePyObjectPtr PyCallArgs = NePyStealReference(PyTuple_New(2));
	PyTuple_SetItem(PyCallArgs, 0, NePyString_FromString(ModuleNameStr));
	PyTuple_SetItem(PyCallArgs, 1, PyLoader);
	FNePyObjectPtr PyCallKwds = NePyStealReference(PyDict_New());
	PyDict_SetItemString(PyCallKwds, "is_package", PyLoader->bIsPackage ? Py_True : Py_False);

	PyObject* PyRetValue = PyObject_Call(PyModuleSpec, PyCallArgs, PyCallKwds);
	return PyRetValue;
}
#endif // PY_MAJOR_VERSION >= 3

#if PY_MAJOR_VERSION < 3
static PyObject* FNePyImportFinder_FindModule(FNePyImportFinder* InSelf, PyObject* InArgs)
{
	const char* ModuleNameStr;
	PyObject* PyPath = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s|O:find_module", &ModuleNameStr, &PyPath))
	{
		return nullptr;
	}

	FNePyImportLoader* PyLoader = _FNePyImportFinder_CreateLoader(InSelf->Path, ModuleNameStr);
	if (!PyLoader)
	{
		Py_RETURN_NONE;
	}

	return PyLoader;
}
#endif // PY_MAJOR_VERSION < 3

static PyMethodDef FNePyImportFinder_methods[] = {
#if PY_MAJOR_VERSION >= 3
	{"find_spec", NePyCFunctionCast(&FNePyImportFinder_FindSpec), METH_VARARGS, nullptr},
#else
	{"find_module", NePyCFunctionCast(&FNePyImportFinder_FindModule), METH_VARARGS, nullptr},
#endif
	{ NULL } /* Sentinel */
};

// --------------------------------------------------

bool NePyImporterInit()
{
#if PY_MAJOR_VERSION < 3
#if PY_MINOR_VERSION < 7
	UE_LOG(LogNePython, Error, TEXT("NePyImporter only works with python 2.7 or above."));
	return false;
#endif
#else
#if PY_MINOR_VERSION < 5
	UE_LOG(LogNePython, Error, TEXT("NePyImporter only works with python 3.5 or above."));
	return false;
#endif
#endif

	PyTypeObject* PyLoaderType = &FNePyImportLoader_Type;
	PyLoaderType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyLoaderType->tp_new = &PyObjectFNePyImportLoader_New;
	PyLoaderType->tp_dealloc = (destructor)&PyObjectFNePyImportLoader_Dealloc;
	PyLoaderType->tp_repr = (reprfunc)&PyObjectFNePyImportLoader_Repr;
	PyLoaderType->tp_methods = FNePyImportLoader_methods;
	if (PyType_Ready(PyLoaderType) < 0)
	{
		UE_LOG(LogNePython, Error, TEXT("NePyImportLoader init python type failed!"));
		return false;
	}

	PyTypeObject* PyFinderType = &FNePyImportFinder_Type;
	PyFinderType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyFinderType->tp_new = &PyObjectFNePyImportFinder_New;
	PyFinderType->tp_dealloc = (destructor)&PyObjectFNePyImportFinder_Dealloc;
	PyFinderType->tp_init = (initproc)&PyObjectFNePyImportFinder_Init;
	PyFinderType->tp_repr = (reprfunc)&PyObjectFNePyImportFinder_Repr;
	PyFinderType->tp_methods = FNePyImportFinder_methods;
	if (PyType_Ready(PyFinderType) < 0)
	{
		UE_LOG(LogNePython, Error, TEXT("NePyImportFinder init python type failed!"));
		return false;
	}

	bool bImporterSupportPy = true;
	GConfig->GetBool(TEXT("NEPY"), TEXT("ImporterSupportPy"), bImporterSupportPy, GGameIni);
	if (bImporterSupportPy)
	{
		NePyValidExts.Emplace(FString(TEXT(".py")), FNePyImportOptions({ true, false }));
	}
	bool bImporterSupportPyc = true;
	GConfig->GetBool(TEXT("NEPY"), TEXT("ImporterSupportPyc"), bImporterSupportPyc, GGameIni);
	if (bImporterSupportPyc)
	{
		NePyValidExts.Emplace(FString(TEXT(".pyc")), FNePyImportOptions({ false, true }));
	}
	bool bImporterSupportUes = true;
	GConfig->GetBool(TEXT("NEPY"), TEXT("ImporterSupportUes"), bImporterSupportUes, GGameIni);
	if (bImporterSupportUes)
	{
		NePyValidExts.Emplace(FString(TEXT(".ues")), FNePyImportOptions({ false, false }));
	}

	PyObject* PyPathHooks = PySys_GetObject((char*)"path_hooks");
	if (!PyPathHooks)
	{
		UE_LOG(LogNePython, Error, TEXT("NePyImporterInit failed. Cant get 'sys.path_hooks'!"));
		return false;
	}

	if (PyList_Insert(PyPathHooks, 0, (PyObject*)PyFinderType) < 0)
	{
		UE_LOG(LogNePython, Error, TEXT("NePyImporterInit failed. Cant add NePyImporter to 'sys.path_hooks'!"));
		return false;
	}

	PyObject* PyPathImporterCache = PySys_GetObject((char*)"path_importer_cache");
	if (!PyPathImporterCache)
	{
		UE_LOG(LogNePython, Error, TEXT("NePyImporterInit failed. Cant get 'sys.path_importer_cache'!"));
		return false;
	}

	PyDict_Clear(PyPathImporterCache);

	return true;
}
// --------------------------------------------------
#if PY_MAJOR_VERSION >= 3

static _inittab NePyInjectedEntry;

static PyObject* NePyInjectImporterFunc()
{
	if (!NePyImporterInit())
	{
		PyErr_Clear();
	}
	return NePyInjectedEntry.initfunc();
}

bool NePyInjectImporterToInittab()
{
#if PY_MAJOR_VERSION < 3 || PY_MINOR_VERSION < 5
	UE_LOG(LogNePython, Error, TEXT("NePyImporter only works with python 3.5 or above."));
	return false;
#endif

	_inittab* Inittab = PyImport_Inittab;

	// _PyImport_Inittab中放置的是Python内置模块，会在 create_builtin() 函数中调用
	// _io 模块的初始化时间是在 importlib 初始化之后，在 encodings 初始化之前
	// 因此是个非常合适的注入点
	const char* InjectPoint = "_io";

	int32 InjectIndex = INDEX_NONE;
	for (int32 i = 0; Inittab[i].name != nullptr; ++i)
	{
		if (strcmp(InjectPoint, Inittab[i].name) == 0)
		{
			InjectIndex = i;
			NePyInjectedEntry.name = Inittab[i].name;
			NePyInjectedEntry.initfunc = Inittab[i].initfunc;
			break;
		}
	}

	if (InjectIndex == INDEX_NONE)
	{
		UE_LOG(LogNePython, Error, TEXT("NePyInjectImporterToInittab failed. Cant find inject point '%s'!"), ANSI_TO_TCHAR(InjectPoint));
		return false;
	}

	Inittab[InjectIndex].initfunc = NePyInjectImporterFunc;
	return true;
}

#endif // PY_MAJOR_VERSION >= 3
// --------------------------------------------------

void NePyImporterEncryptKey(uint8* OutKey)
{
	// WARNING! 各项目组应该将此处修改为独一无二的密钥
	constexpr uint8 DefaultKey[] = "w5q6^C04SW!@ed}ad#*7YouGoHell~`zq";
	memcpy(OutKey, DefaultKey, FAES::FAESKey::KeySize);
}

// --------------------------------------------------
