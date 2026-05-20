#include "NePyGeneratedType.h"
#include "NePyResourceOwner.h"
#include "NePyGeneratedClass.h"
#include "NePyGeneratedStruct.h"
#include "NePyGeneratedEnum.h"
#include "NePyMemoryAllocator.h"
#include "NePyDelegate.h"
#include "NePyBase.h"
#include "UObject/Package.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/UObjectHash.h"
#include "NePythonSettings.h"
#include "NePySoftPtr.h"
#include "NePyWeakPtr.h"
#include "NePyClass.h"
#include "Misc/Paths.h"

bool GNePyDisableGeneratedType = false;

static UPackage* GNePyGeneratedType_RuntimeContainer = nullptr;
static UPackage* GNePyGeneratedType_EditorContainer = nullptr;

const TCHAR* GNePyGeneratedType_RuntimeContainerName = TEXT("/Script/NePythonBinding/PythonTypes");
const TCHAR* GNePyGeneratedType_EditorContainerName = TEXT("/Script/NePythonBinding/PythonEditorTypes");

UPackage* GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType ContainerType)
{
	switch (ContainerType)
	{
	case ENePyGeneratedTypeContainerType::Runtime:
		return GNePyGeneratedType_RuntimeContainer;
	case ENePyGeneratedTypeContainerType::Editor:
		return GNePyGeneratedType_EditorContainer;
	}
	return nullptr;
}

const TCHAR* GetNePyGeneratedTypeContainerName(ENePyGeneratedTypeContainerType ContainerType)
{
	switch (ContainerType)
	{
	case ENePyGeneratedTypeContainerType::Runtime:
		return GNePyGeneratedType_RuntimeContainerName;
	case ENePyGeneratedTypeContainerType::Editor:
		return GNePyGeneratedType_EditorContainerName;
	}
	return TEXT("");
}

static UPackage* CreateNePyInitGeneratedTypeContainer(ENePyGeneratedTypeContainerType ContainerType)
{
	EObjectFlags ObjectFlags =
		RF_Public;
		//RF_Transient; // 如果有RF_Transient标记，则继承Python类的蓝图无法存盘

	EPackageFlags PackageFlags =
		PKG_ContainsScript |
		PKG_RuntimeGenerated |  // 如果没有PKG_RuntimeGenerated标记，在FNePythonBindingModule::InitializePython回调脚本创建的NePyGeneratedClass会创建CDO报错
		PKG_CompiledIn; // 如果没有PKG_CompiledIn标记，则继承自Python类的蓝图无法加载

	if (ContainerType == ENePyGeneratedTypeContainerType::Editor)
	{
		PackageFlags |= PKG_EditorOnly;
	}

	check(!FindPackage(nullptr, GetNePyGeneratedTypeContainerName(ContainerType)));
	check(!GetNePyGeneratedTypeContainer(ContainerType));
	
	auto Package = NewObject<UPackage>(nullptr, GetNePyGeneratedTypeContainerName(ContainerType), ObjectFlags);
	Package->SetPackageFlags(PackageFlags);
	Package->AddToRoot();

	return Package;
}

static void NePyInitGeneratedTypeContainer()
{
	GNePyGeneratedType_RuntimeContainer = CreateNePyInitGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Runtime);
	GNePyGeneratedType_EditorContainer = CreateNePyInitGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Editor);
}

namespace NePyGenUtil
{
	bool ConvertOptionalBool(PyObject* InObj, bool& OutBool, const char* InErrorCtxt, const char* InErrorMsg)
	{
		if (InObj && InObj != Py_None)
		{
			if (!NePyBase::ToCpp(InObj, OutBool, false))
			{
				PyErr_Format(PyExc_TypeError, "%s: %s", InErrorCtxt, InErrorMsg);
				return false;
			}
		}
		return true;
	}

	bool ConvertOptionalString(PyObject* InObj, FString& OutString, const char* InErrorCtxt, const char* InErrorMsg)
	{
		OutString.Reset();
		if (InObj && InObj != Py_None)
		{
			if (!NePyBase::ToCpp(InObj, OutString))
			{
				PyErr_Format(PyExc_TypeError, "%s: %s", InErrorCtxt, InErrorMsg);
				return false;
			}
		}
		return true;
	}

	bool ConvertOptionalFunctionFlag(PyObject* InObj, ENePyUFunctionDefFlags& OutFlags, const ENePyUFunctionDefFlags InTrueFlagBit, const ENePyUFunctionDefFlags InFalseFlagBit, const char* InErrorCtxt, const char* InErrorMsg)
	{
		if (InObj && InObj != Py_None)
		{
			bool bFlagValue = false;
			if (!NePyBase::ToCpp(InObj, bFlagValue))
			{
				PyErr_Format(PyExc_TypeError, "%s: %s", InErrorCtxt, InErrorMsg);
				return false;
			}
			OutFlags |= (bFlagValue ? InTrueFlagBit : InFalseFlagBit);
		}
		return true;
	}

#if WITH_EDITORONLY_DATA
	void ApplyMetaData(PyObject* InMetaData, const TFunctionRef<void(const FString&, const FString&)>& InPredicate)
	{
		if (InMetaData && PyDict_Check(InMetaData))
		{
			PyObject* MetaDataKey = nullptr;
			PyObject* MetaDataValue = nullptr;
			Py_ssize_t MetaDataIndex = 0;
			while (PyDict_Next(InMetaData, &MetaDataIndex, &MetaDataKey, &MetaDataValue))
			{
				const FString MetaDataKeyStr = NePyBase::PyObjectToString(MetaDataKey);
				const FString MetaDataValueStr = NePyBase::PyObjectToString(MetaDataValue);
				InPredicate(MetaDataKeyStr, MetaDataValueStr);
			}
		}
	}

	FString GetPythonModuleRelativePathForPyType(PyTypeObject* InPyType)
	{
		// 拼一个看上去像是这么一回事的路径，用于给本地化数据收集器使用
		FNePyObjectPtr PyModuleName = NePyStealReference(PyObject_GetAttrString((PyObject*)InPyType, "__module__"));
		if (!PyModuleName)
		{
			PyErr_Clear();
			return FString();
		}
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 7
		FNePyObjectPtr PySysModule = NePyStealReference(PyImport_GetModule(PyModuleName));
		if (!PySysModule)
		{
			PyErr_Clear();
			return FString();
		}
#else
		FNePyObjectPtr PySysModuleDict = NePyNewReference(PyImport_GetModuleDict());
		FNePyObjectPtr PySysModule = NePyNewReference(PyDict_GetItem(PySysModuleDict, PyModuleName));
		if (!PySysModule)
		{
			PyErr_Clear();
			return FString();
		}
#endif // PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 7
		FNePyObjectPtr PyModuleFilePath = NePyStealReference(PyObject_GetAttrString(PySysModule, "__file__"));
		if (!PyModuleFilePath)
		{
			PyErr_Clear();
			return FString();
		}
		// 如果相对路径包含 ".." 会导致本地化系统识别不到
		// 此处相对路径取相对于 Project 目录的路径
		FString Path = NePyBase::PyObjectToString(PyModuleFilePath);
		FPaths::MakePathRelativeTo(Path, *FPaths::GetProjectFilePath());
		return Path;
	}
#endif // WITH_EDITORONLY_DATA

	static FString GetObjectLocationInfo(PyObject* Object)
	{
		// Try to get code object
		PyObject* Code = PyObject_GetAttrString(Object, "__code__");
		if (!Code)
		{
			PyErr_Clear();
			return FString(TEXT(""));
		}

		// Get filename and line number
		PyObject* FileName = PyObject_GetAttrString(Code, "co_filename");
		PyObject* LineNo = PyObject_GetAttrString(Code, "co_firstlineno");

		FString Location;

		if (FileName && LineNo)
		{
#if PY_MAJOR_VERSION >= 3
			const char* FileNameStr = PyUnicode_AsUTF8(FileName);
#else
			const char* FileNameStr = PyString_AsString(FileName);
#endif
			long LineNoValue = PyLong_AsLong(LineNo);

			if (FileNameStr && LineNoValue >= 0)
			{
				// Only show filename part, not full path
				const char* BaseFileName = strrchr(FileNameStr, '/');
				if (!BaseFileName)
				{
					BaseFileName = strrchr(FileNameStr, '\\');
				}
				BaseFileName = BaseFileName ? BaseFileName + 1 : FileNameStr;
				Location = FString::Printf(TEXT(" (at %s:%ld)"), UTF8_TO_TCHAR(BaseFileName), LineNoValue);
			}
		}

		Py_XDECREF(FileName);
		Py_XDECREF(LineNo);
		Py_DECREF(Code);

		return Location;
	}

	static bool CheckDuplicateObjectInScope(PyObject* Object)
	{
		if (!Object)
		{
			return true;
		}

		// Get object name
		PyObject* ObjectName = PyObject_GetAttrString(Object, "__name__");
		if (!ObjectName)
		{
			PyErr_Clear();
			return true;  // No __name__ attribute, no duplicate possible
		}

		// Get current frame's local variables
#if (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9) || PY_MAJOR_VERSION > 3
		PyFrameObject* Frame = PyEval_GetFrame();
#else
		PyFrameObject* Frame = PyThreadState_Get()->frame;
#endif
		if (!Frame)
		{
			Py_DECREF(ObjectName);
			return true;  // Cannot get frame info
		}

#if (PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 9) || PY_MAJOR_VERSION > 3
		PyObject* Locals = PyFrame_GetLocals(Frame);
#else
		PyObject* Locals = Frame->f_locals;
		Py_XINCREF(Locals);
#endif
		if (!Locals)
		{
			Py_DECREF(ObjectName);
			return true;  // Cannot get local variables
		}

		// Check if object with same name already exists
		PyObject* ExistingItem = PyDict_GetItem(Locals, ObjectName);

		if (ExistingItem && ExistingItem != Object)
		{
			// Found duplicate, get object name as string
#if PY_MAJOR_VERSION >= 3
			const char* NameStr = PyUnicode_AsUTF8(ObjectName);
#else
			const char* NameStr = PyString_AsString(ObjectName);
#endif
			if (!NameStr)
			{
				NameStr = "<unknown>";
			}

			// Get location information for existing object
			FString ExistingLocation = GetObjectLocationInfo(ExistingItem);

			// Get location information for new object
			FString NewLocation = GetObjectLocationInfo(Object);

			// Get object type descriptions
			FString ExistingType = UTF8_TO_TCHAR(Py_TYPE(ExistingItem)->tp_name);
			FString NewType = UTF8_TO_TCHAR(Py_TYPE(Object)->tp_name);

			// Throw Python exception with detailed information
			PyErr_Format(PyExc_SyntaxError,
				"Duplicate definition of '%s':\n"
				"  Existing: %s%s\n"
				"  New: %s%s\n"
				"Please use different names to avoid conflicts.",
				NameStr,
				TCHAR_TO_UTF8(*ExistingType), TCHAR_TO_UTF8(*ExistingLocation),
				TCHAR_TO_UTF8(*NewType), TCHAR_TO_UTF8(*NewLocation)
			);

			UE_LOG(LogNePython, Error, TEXT("Duplicate definition of '%s': Existing: %s%s New: %s%s"),
				UTF8_TO_TCHAR(NameStr),
				*ExistingType, *ExistingLocation,
				*NewType, *NewLocation
			);

			Py_DECREF(ObjectName);
			Py_DECREF(Locals);
			return false;
		}

		Py_DECREF(ObjectName);
		Py_DECREF(Locals);
		return true;
	}
}

PyTypeObject InitializeNePySelfClassDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".SelfClassDef", /* tp_name */
		sizeof(FNePySelfClassDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePySelfClassDef::New;
	PyType.tp_dealloc = (destructor)&FNePySelfClassDef::Dealloc;
	PyType.tp_init = (initproc)&FNePySelfClassDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "type used to define self class for subclassing";

	return PyType;
}

PyTypeObject InitializeNePyRefParamDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".RefParamDef", /* tp_name */
		sizeof(FNePyRefParamDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyRefParamDef::New;
	PyType.tp_dealloc = (destructor)&FNePyRefParamDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyRefParamDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "type used to define UPARAM(ref) for subclassing";

	return PyType;
}

PyTypeObject InitializeNePyUValueDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".ValueDef", /* tp_name */
		sizeof(FNePyUValueDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyUValueDef::New;
	PyType.tp_dealloc = (destructor)&FNePyUValueDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyUValueDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define FProperty fields from Python";

	return PyType;
}

PyTypeObject InitializeNePyFPropertyDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".PropertyDef", /* tp_name */
		sizeof(FNePyFPropertyDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyFPropertyDef::New;
	PyType.tp_dealloc = (destructor)&FNePyFPropertyDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyFPropertyDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define FProperty fields from Python";

	return PyType;
}

PyTypeObject InitializeNePyUParamDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".ParamDef", /* tp_name */
		sizeof(FNePyUParamDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyUParamDef::New;
	PyType.tp_dealloc = (destructor)&FNePyUParamDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyUParamDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define UPARAM fields from Python";

	return PyType;
}

PyTypeObject InitializeNePyUFunctionDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".FunctionDef", /* tp_name */
		sizeof(FNePyUFunctionDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyUFunctionDef::New;
	PyType.tp_dealloc = (destructor)&FNePyUFunctionDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyUFunctionDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define UFunction fields from Python";

	return PyType;
}

PyTypeObject InitializeNePyUComponentDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".ComponentDef", /* tp_name */
		sizeof(FNePyUComponentDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyUComponentDef::New;
	PyType.tp_dealloc = (destructor)&FNePyUComponentDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyUComponentDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define UActorComponent fields from Python";

	return PyType;
}

PyTypeObject InitializeFNePyDelegateDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".DelegateDef", /* tp_name */
		sizeof(FNePyUDelegateDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyUDelegateDef::New;
	PyType.tp_dealloc = (destructor)&FNePyUDelegateDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyUDelegateDef::Init;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define UDelegate fields from Python";

	return PyType;
}


PyTypeObject InitializeNePyObjectRefDefType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".ObjectRefDef", /* tp_name */
		sizeof(FNePyObjectRefDef), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FNePyObjectRefDef::New;
	PyType.tp_dealloc = (destructor)&FNePyObjectRefDef::Dealloc;
	PyType.tp_init = (initproc)&FNePyObjectRefDef::Init;
	PyType.tp_call = (ternaryfunc)&FNePyObjectRefDef::Call;
	PyType.tp_str = (reprfunc)&FNePyObjectRefDef::Str;
	PyType.tp_repr = (reprfunc)&FNePyObjectRefDef::Repr;

	static PyNumberMethods PyNumber;
	PyNumber.nb_or = (binaryfunc)&FNePyObjectRefDef::Or;
	PyType.tp_as_number = &PyNumber;
	
	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type used to define object reference from Python";

	return PyType;
}

PyTypeObject InitializeNePyObjectRefMakerType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".ObjectRefMaker", /* tp_name */
		sizeof(FNePyObjectRefMaker), /* tp_basicsize */
	};

	static PyMappingMethods PyMapping;
	PyMapping.mp_subscript = (binaryfunc)&FNePyObjectRefMaker::GetItem;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_as_mapping = &PyMapping;

	return PyType;
}

PyTypeObject InitializeNePyTypeAliasType()
{
	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		NePyInternalModuleName ".NePyTypeAlias", /* tp_name */
		sizeof(FNePyTypeAlias), /* tp_basicsize */
	};

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_call = (ternaryfunc)&FNePyTypeAlias::Call;
	PyType.tp_getattro = (getattrofunc)&FNePyTypeAlias::GetAttro;

	static PyNumberMethods PyNumber;
	PyNumber.nb_or = (binaryfunc)&FNePyTypeAlias::Or;
	PyType.tp_as_number = &PyNumber;

	return PyType;
}

static PyTypeObject NePySelfClassDefType = InitializeNePySelfClassDefType();
static PyTypeObject NePyRefParamDefType = InitializeNePyRefParamDefType();
static PyTypeObject NePyUValueDefType = InitializeNePyUValueDefType();
static PyTypeObject NePyFPropertyDefType = InitializeNePyFPropertyDefType();
static PyTypeObject NePyUParamDefType = InitializeNePyUParamDefType();
static PyTypeObject NePyUFunctionDefType = InitializeNePyUFunctionDefType();
static PyTypeObject NePyUComponentDefType = InitializeNePyUComponentDefType();
static PyTypeObject FNePyDelegateDefType = InitializeFNePyDelegateDefType();
static PyTypeObject NePyObjectRefDefType = InitializeNePyObjectRefDefType();
static PyTypeObject NePyObjectRefMakerType = InitializeNePyObjectRefMakerType();
static PyTypeObject NePyTypeAliasType = InitializeNePyTypeAliasType();

FNePySelfClassDef* FNePySelfClassDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePySelfClassDef* Self = (FNePySelfClassDef*)InType->tp_alloc(InType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
	return Self;
}

void FNePySelfClassDef::Dealloc(FNePySelfClassDef* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePySelfClassDef::Init(FNePySelfClassDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return 0;
}

void FNePySelfClassDef::Deinit(FNePySelfClassDef* InSelf)
{
}

FNePySelfClassDef* FNePySelfClassDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePySelfClassDefType))
	{
		return (FNePySelfClassDef*)InPyObj;
	}
	return nullptr;
}

FNePyRefParamDef* FNePyRefParamDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyRefParamDef* Self = (FNePyRefParamDef*)InType->tp_alloc(InType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
	return Self;
}

void FNePyRefParamDef::Dealloc(FNePyRefParamDef* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyRefParamDef::Init(FNePyRefParamDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyTypeObj = nullptr;
	PyArg_ParseTuple(InArgs, "O:refparam", &PyTypeObj);
	if (!PyTypeObj)
	{
		PyErr_Format(PyExc_TypeError, "%s: missing required argument 'type'", Py_TYPE(InSelf)->tp_name);
		return -1;
	}
	InSelf->TypeObject = PyTypeObj;
	Py_XINCREF(InSelf->TypeObject);
	return 0;
}

void FNePyRefParamDef::Deinit(FNePyRefParamDef* InSelf)
{
	Py_XDECREF(InSelf->TypeObject);
	InSelf->TypeObject = nullptr;
}

FNePyRefParamDef* FNePyRefParamDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyRefParamDefType))
	{
		return (FNePyRefParamDef*)InPyObj;
	}
	return nullptr;
}

FNePyUValueDef* FNePyUValueDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUValueDef* Self = (FNePyUValueDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
		Self->Value = 0;
		Self->MetaData = nullptr;
	}
	return Self;
}

void FNePyUValueDef::Dealloc(FNePyUValueDef* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyUValueDef::Init(FNePyUValueDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyValueObj = nullptr;
	PyObject* PyMetaObj = nullptr;

	static const char* ArgsKwdList[] = { "value", "meta", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:uvalue", (char**)ArgsKwdList, &PyValueObj, &PyMetaObj))
	{
		return -1;
	}

	int64 Value;
	if (!NePyBase::ToCpp(PyValueObj, Value, true))
	{
		PyErr_Format(PyExc_Exception, "%s: 'value' must have type 'int', not '%s'",
			Py_TYPE(InSelf)->tp_name, Py_TYPE(PyValueObj)->tp_name);
		return -1;
	}

	Deinit(InSelf);

	InSelf->Value = Value;

	Py_XINCREF(PyMetaObj);
	InSelf->MetaData = PyMetaObj;

	InSelf->DefineOrder = FNePyUValueDef::GlobalDefineOrder.fetch_add(1);

	return 0;
}

void FNePyUValueDef::Deinit(FNePyUValueDef* InSelf)
{
	Py_XDECREF(InSelf->MetaData);
	InSelf->MetaData = nullptr;
}

#if WITH_EDITORONLY_DATA
void FNePyUValueDef::ApplyMetaData(FNePyUValueDef* InSelf, const TFunctionRef<void(const FString&, const FString&)>& InPredicate)
{
	NePyGenUtil::ApplyMetaData(InSelf->MetaData, InPredicate);
}
#endif // WITH_EDITORONLY_DATA

FNePyUValueDef* FNePyUValueDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyUValueDefType))
	{
		return (FNePyUValueDef*)InPyObj;
	}
	return nullptr;
}

std::atomic<uint32> FNePyUValueDef::GlobalDefineOrder(0);


FNePyFPropertyDef* FNePyFPropertyDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyFPropertyDef* Self = (FNePyFPropertyDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
		Self->PropType = nullptr;
		Self->DefaultValue = nullptr;
		new(&Self->GetterFuncName) FString();
		new(&Self->SetterFuncName) FString();
	}
	return Self;
}

void FNePyFPropertyDef::Dealloc(FNePyFPropertyDef* InSelf)
{
	Deinit(InSelf);

	InSelf->GetterFuncName.~FString();
	InSelf->SetterFuncName.~FString();
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyFPropertyDef::Init(FNePyFPropertyDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyPropTypeObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:uproperty", &PyPropTypeObj))
	{
		return -1;
	}

	PyObject* PyPropGetterObj = nullptr;
	PyObject* PyPropSetterObj = nullptr;
	PyObject* PyPropFlagsObj = nullptr;
	PyObject* PyGetterKeyString = nullptr;
	PyObject* PySetterKeyString = nullptr;
	TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;

	if (InKwds)
	{
		PyObject* KwdKey = nullptr;
		PyObject* KwdVal = nullptr;
		Py_ssize_t KwdIndex = 0;
		while (PyDict_Next(InKwds, &KwdIndex, &KwdKey, &KwdVal))
		{
			const char* KeyStr = NePyString_AsString(KwdKey);
			if (strcmp(KeyStr, "BlueprintGetter") == 0)
			{
				PyPropGetterObj = KwdVal;
				PySpecifierPairs.Emplace(KwdKey, KwdVal);
			}
			else if (strcmp(KeyStr, "BlueprintSetter") == 0)
			{
				PyPropSetterObj = KwdVal;
				PySpecifierPairs.Emplace(KwdKey, KwdVal);
			}
			else if (strcmp(KeyStr, "getter") == 0)
			{
				if (!PyGetterKeyString)
				{
					PyGetterKeyString = NePyString_FromString("BlueprintGetter");
				}
				PyPropGetterObj = KwdVal;
				PySpecifierPairs.Emplace(PyGetterKeyString, KwdVal);
				UE_LOG(LogNePython, Warning, TEXT("param 'getter' has been deprecated. Use 'BlueprintGetter' instead."));
			}
			else if (strcmp(KeyStr, "setter") == 0)
			{
				if (!PySetterKeyString)
				{
					PySetterKeyString = NePyString_FromString("BlueprintSetter");
				}
				PyPropSetterObj = KwdVal;
				PySpecifierPairs.Emplace(PySetterKeyString, KwdVal);
				UE_LOG(LogNePython, Warning, TEXT("param 'setter' has been deprecated. Use 'BlueprintSetter' instead."));
			}
			else if (strcmp(KeyStr, "flags") == 0)
			{
				PyPropFlagsObj = KwdVal;
				UE_LOG(LogNePython, Warning, TEXT("'flags' has been deprecated, use Specifier instead."));
			}
			else
			{
				PySpecifierPairs.Emplace(KwdKey, KwdVal);
			}
		}
	}

	const char* ErrorCtxt = Py_TYPE(InSelf)->tp_name;

	FString PropGetter;
	if (!NePyGenUtil::ConvertOptionalString(PyPropGetterObj, PropGetter, ErrorCtxt, "Failed to convert parameter 'BlueprintGetter' to a string (expected 'None' or 'str')"))
	{
		return -1;
	}

	FString PropSetter;
	if (!NePyGenUtil::ConvertOptionalString(PyPropSetterObj, PropSetter, ErrorCtxt, "Failed to convert parameter 'BlueprintSetter' to a string (expected 'None' or 'str')"))
	{
		return -1;
	}

	uint64 UserDefineFlags = CPF_None;
	if (PyPropFlagsObj && PyPropFlagsObj != Py_None)
	{
		NePyBase::ToCpp(PyPropFlagsObj, UserDefineFlags);
	}

	Deinit(InSelf);

	Py_INCREF(PyPropTypeObj);
	InSelf->PropType = PyPropTypeObj;

	if (NePyGenUtil::SupportDefaultPropertyValue(PyPropTypeObj))
	{
		Py_INCREF(PyPropTypeObj);
		InSelf->DefaultValue = PyPropTypeObj;
	}

	InSelf->GetterFuncName = MoveTemp(PropGetter);
	InSelf->SetterFuncName = MoveTemp(PropSetter);
	InSelf->UserDefineUPropertyFlags = (EPropertyFlags)UserDefineFlags;
	InSelf->Specifiers = FNePySpecifier::ParseSpecifiers(PySpecifierPairs, FNePySpecifier::Scope_Property);

	InSelf->DefineOrder = FNePyFPropertyDef::GlobalDefineOrder.fetch_add(1);

	Py_XDECREF(PyGetterKeyString);
	Py_XDECREF(PySetterKeyString);
	return 0;
}

void FNePyFPropertyDef::Deinit(FNePyFPropertyDef* InSelf)
{
	Py_XDECREF(InSelf->PropType);
	InSelf->PropType = nullptr;

	Py_XDECREF(InSelf->DefaultValue);
	InSelf->DefaultValue = nullptr;

	InSelf->GetterFuncName.Reset();
	InSelf->SetterFuncName.Reset();

	FNePySpecifier::ReleaseSpecifiers(InSelf->Specifiers);
}

FNePyFPropertyDef* FNePyFPropertyDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyFPropertyDefType))
	{
		return (FNePyFPropertyDef*)InPyObj;
	}
	return nullptr;
}

std::atomic<uint32> FNePyFPropertyDef::GlobalDefineOrder(0);


FNePyUParamDef* FNePyUParamDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUParamDef* Self = (FNePyUParamDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
		Self->PropType = nullptr;
	}
	return Self;
}

void FNePyUParamDef::Dealloc(FNePyUParamDef* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyUParamDef::Init(FNePyUParamDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyPropTypeObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:uparam", &PyPropTypeObj))
	{
		return -1;
	}

	TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;

	if (InKwds)
	{
		PyObject* KwdKey = nullptr;
		PyObject* KwdVal = nullptr;
		Py_ssize_t KwdIndex = 0;
		while (PyDict_Next(InKwds, &KwdIndex, &KwdKey, &KwdVal))
		{
			const char* KeyStr = NePyString_AsString(KwdKey);
			PySpecifierPairs.Emplace(KwdKey, KwdVal);
		}
	}

	const char* ErrorCtxt = Py_TYPE(InSelf)->tp_name;

	Deinit(InSelf);

	Py_INCREF(PyPropTypeObj);
	InSelf->PropType = PyPropTypeObj;

	InSelf->Specifiers = FNePySpecifier::ParseSpecifiers(PySpecifierPairs, FNePySpecifier::Scope_Param);

	return 0;
}

void FNePyUParamDef::Deinit(FNePyUParamDef* InSelf)
{
	Py_XDECREF(InSelf->PropType);
	InSelf->PropType = nullptr;

	FNePySpecifier::ReleaseSpecifiers(InSelf->Specifiers);
}

FNePyUParamDef* FNePyUParamDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyUParamDefType))
	{
		return (FNePyUParamDef*)InPyObj;
	}
	return nullptr;
}

FNePyUFunctionDef* FNePyUFunctionDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUFunctionDef* Self = (FNePyUFunctionDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
		Self->Func = nullptr;
		Self->FuncRetType = nullptr;
		Self->FuncParamTypes = nullptr;
		Self->FuncFlags = ENePyUFunctionDefFlags::None;
	}
	return Self;
}

void FNePyUFunctionDef::Dealloc(FNePyUFunctionDef* InSelf)
{
	Deinit(InSelf);

	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyUFunctionDef::Init(FNePyUFunctionDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyFuncObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:ufunction", &PyFuncObj))
	{
		return -1;
	}

	PyObject* PyFuncRetTypeObj = Py_None;
	PyObject* PyFuncParamTypesObj = Py_None;
	PyObject* PyFuncOverrideObj = Py_None;
	PyObject* PyFuncStaticObj = Py_None;
	PyObject* PyPureKeyString = nullptr;
	PyObject* PyGetterKeyString = nullptr;
	PyObject* PySetterKeyString = nullptr;
	TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;

	const char* ErrorCtxt = Py_TYPE(InSelf)->tp_name;

	if (InKwds)
	{
		PyObject* KwdKey = nullptr;
		PyObject* KwdVal = nullptr;
		Py_ssize_t KwdIndex = 0;
		while (PyDict_Next(InKwds, &KwdIndex, &KwdKey, &KwdVal))
		{
			const char* KeyStr = NePyString_AsString(KwdKey);
			if (strcmp(KeyStr, "ret") == 0)
			{
				PyFuncRetTypeObj = KwdVal;
			}
			else if (strcmp(KeyStr, "params") == 0)
			{
				PyFuncParamTypesObj = KwdVal;
			}
			else if (strcmp(KeyStr, "override") == 0)
			{
				PyFuncOverrideObj = KwdVal;
			}
			else if (strcmp(KeyStr, "static") == 0)
			{
				PyFuncStaticObj = KwdVal;
				UE_LOG(LogNePython, Warning, TEXT("param 'static' has been deprecated. Mark method with '@staticmethod' under '@ue.ufunction()' instead."));
			}
			else if (strcmp(KeyStr, "pure") == 0)
			{
				if (!PyPureKeyString)
				{
					PyPureKeyString = NePyString_FromString("BlueprintPure");
				}
				PySpecifierPairs.Emplace(PyPureKeyString, KwdVal);
				UE_LOG(LogNePython, Warning, TEXT("param 'pure' has been deprecated. Use 'BlueprintPure' instead."));
			}
			else if (strcmp(KeyStr, "getter") == 0)
			{
				if (!PyGetterKeyString)
				{
					PyGetterKeyString = NePyString_FromString("BlueprintGetter");
				}
				PySpecifierPairs.Emplace(PyGetterKeyString, KwdVal);
				UE_LOG(LogNePython, Warning, TEXT("param 'getter' has been deprecated. Use 'BlueprintGetter' instead."));
			}
			else if (strcmp(KeyStr, "setter") == 0)
			{
				if (!PySetterKeyString)
				{
					PySetterKeyString = NePyString_FromString("BlueprintSetter");
				}
				PySpecifierPairs.Emplace(PySetterKeyString, KwdVal);
				UE_LOG(LogNePython, Warning, TEXT("param 'setter' has been deprecated. Use 'BlueprintSetter' instead."));
			}
			else
			{
				PySpecifierPairs.Emplace(KwdKey, KwdVal);
			}
		}
	}

	if (Py_TYPE(PyFuncObj) == &PyClassMethod_Type)
	{
		PyErr_Format(PyExc_TypeError, "%s: cant accept 'classmethod', did you means 'staticmethod'?", ErrorCtxt);
		return -1;
	}
	if (Py_TYPE(PyFuncObj) == &PyStaticMethod_Type)
	{
		PyFuncObj = PyStaticMethod_Type.tp_descr_get(PyFuncObj, nullptr, nullptr);
		if (!PyFuncObj)
		{
			PyErr_Format(PyExc_RuntimeError, "%s: uninitialized staticmethod object", ErrorCtxt);
			return -1;
		}
		Py_DECREF(PyFuncObj);

		PyFuncStaticObj = Py_True;
	}
	if (!PyFunction_Check(PyFuncObj))
	{
		PyErr_Format(PyExc_TypeError, "%s: parameter 'func' must have type 'function', not '%s'", ErrorCtxt, Py_TYPE(PyFuncObj)->tp_name);
		return -1;
	}

	ENePyUFunctionDefFlags FuncFlags = ENePyUFunctionDefFlags::None;
	if (!NePyGenUtil::ConvertOptionalFunctionFlag(PyFuncOverrideObj, FuncFlags, ENePyUFunctionDefFlags::Override, ENePyUFunctionDefFlags::None, ErrorCtxt, "Failed to convert parameter 'override' to a flag (expected 'None' or 'bool')"))
	{
		return -1;
	}
	if (!NePyGenUtil::ConvertOptionalFunctionFlag(PyFuncStaticObj, FuncFlags, ENePyUFunctionDefFlags::Static, ENePyUFunctionDefFlags::None, ErrorCtxt, "Failed to convert parameter 'static' to a flag (expected 'None' or 'bool')"))
	{
		return -1;
	}

	if (!NePyGenUtil::CheckDuplicateObjectInScope(PyFuncObj))
	{
		return -1;
	}

	Deinit(InSelf);

	Py_INCREF(PyFuncObj);
	InSelf->Func = PyFuncObj;

	if (PyFuncRetTypeObj == Py_None || PyTuple_Check(PyFuncRetTypeObj))
	{
		Py_INCREF(PyFuncRetTypeObj);
		InSelf->FuncRetType = PyFuncRetTypeObj;
	}
	else
	{
		InSelf->FuncRetType = PyTuple_New(1);
		Py_INCREF(PyFuncRetTypeObj);
		PyTuple_SetItem(InSelf->FuncRetType, 0, PyFuncRetTypeObj);
	}

	if (PyFuncParamTypesObj == Py_None || PyTuple_Check(PyFuncParamTypesObj))
	{
		Py_INCREF(PyFuncParamTypesObj);
		InSelf->FuncParamTypes = PyFuncParamTypesObj;
	}
	else
	{
		InSelf->FuncParamTypes = PyTuple_New(1);
		Py_INCREF(PyFuncParamTypesObj);
		PyTuple_SetItem(InSelf->FuncParamTypes, 0, PyFuncParamTypesObj);
	}

	InSelf->FuncFlags = FuncFlags;
	InSelf->Specifiers = FNePySpecifier::ParseSpecifiers(PySpecifierPairs, FNePySpecifier::Scope_Function);

	InSelf->DefineOrder = FNePyUFunctionDef::GlobalDefineOrder.fetch_add(1);

	Py_XDECREF(PyPureKeyString);
	Py_XDECREF(PyGetterKeyString);
	Py_XDECREF(PySetterKeyString);
	return 0;
}

void FNePyUFunctionDef::Deinit(FNePyUFunctionDef* InSelf)
{
	Py_XDECREF(InSelf->Func);
	InSelf->Func = nullptr;

	Py_XDECREF(InSelf->FuncRetType);
	InSelf->FuncRetType = nullptr;

	Py_XDECREF(InSelf->FuncParamTypes);
	InSelf->FuncParamTypes = nullptr;

	InSelf->FuncFlags = ENePyUFunctionDefFlags::None;

	FNePySpecifier::ReleaseSpecifiers(InSelf->Specifiers);
}

FNePyUFunctionDef* FNePyUFunctionDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyUFunctionDefType))
	{
		return (FNePyUFunctionDef*)InPyObj;
	}
	return nullptr;
}

std::atomic<uint32> FNePyUFunctionDef::GlobalDefineOrder(0);


void FNePyUDelegateDef::Dealloc(FNePyUDelegateDef* InSelf)
{
	Deinit(InSelf);

	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyUDelegateDef::Init(FNePyUDelegateDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyFuncParamTypesObj = Py_None;
	TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;

	if (InKwds)
	{
		PyObject* KwdKey = nullptr;
		PyObject* KwdVal = nullptr;
		Py_ssize_t KwdIndex = 0;
		while (PyDict_Next(InKwds, &KwdIndex, &KwdKey, &KwdVal))
		{
			const char* KeyStr = NePyString_AsString(KwdKey);
			if (strcmp(KeyStr, "params") == 0)
			{
				PyFuncParamTypesObj = KwdVal;
			}
			else
			{
				PySpecifierPairs.Emplace(KwdKey, KwdVal);
			}
		}
	}

	Deinit(InSelf);

	if (PyFuncParamTypesObj == Py_None)
	{
		Py_INCREF(PyFuncParamTypesObj);
		InSelf->FuncParamTypes = PyFuncParamTypesObj;
	}
	else
	{
		FNePyObjectPtr NewTuple;
		if (!(PyTuple_Check(PyFuncParamTypesObj) || PyList_Check(PyFuncParamTypesObj)))
		{
			NewTuple = NePyStealReference(PyTuple_New(1));
			Py_INCREF(PyFuncParamTypesObj);
			PyTuple_SetItem(NewTuple, 0, PyFuncParamTypesObj);
			PyFuncParamTypesObj = NewTuple.Get();
		}

		Py_ssize_t NumParmsTypes = PySequence_Size(PyFuncParamTypesObj);
		InSelf->FuncParamTypes = PyTuple_New(NumParmsTypes);

		for (Py_ssize_t Index = 0; Index < NumParmsTypes; ++Index)
		{
			PyObject* InnerItem = PySequence_GetItem(PyFuncParamTypesObj, Index);
			// 形如 params=((str, "param1"), (bool, "param2"))
			if (PyTuple_Check(InnerItem) || PyList_Check(InnerItem))
			{
				PyObject* InnerTuple = InnerItem;
				Py_ssize_t InnerTupleLen = PySequence_Size(InnerTuple);
				if (InnerTupleLen != 2)
				{
					PyErr_Format(PyExc_Exception, "delegate parameter define must be tuple like (str, \"param1\") but receive %s", TCHAR_TO_UTF8(*NePyBase::PyObjectToString(InnerTuple)));
					return -1;
				}
				PyObject* ParmsType = PySequence_GetItem(InnerTuple, 0);
				PyObject* ParmsName = PySequence_GetItem(InnerTuple, 1);
				Py_INCREF(ParmsType);
				PyTuple_SetItem(InSelf->FuncParamTypes, Index, ParmsType);
				InSelf->FuncParamNames.Emplace(NePyBase::PyObjectToString(ParmsName));
			}
			// 形如 ue.udelegate(params=(int,"A")) 或者 ue.udelegate(params=((int,"A")))
			else if (NumParmsTypes == 2 && Index == 0
				&& PyType_Check(PySequence_GetItem(PyFuncParamTypesObj, Index))
				&& (PyUnicode_Check(PySequence_GetItem(PyFuncParamTypesObj, Index + 1)) 
# if PY_MAJOR_VERSION < 3
					|| PyString_Check(PySequence_GetItem(PyFuncParamTypesObj, Index + 1))))
#else
					|| PyBytes_Check(PySequence_GetItem(PyFuncParamTypesObj, Index + 1))))
#endif
			{
				PyObject* ParmsName = PySequence_GetItem(PyFuncParamTypesObj, Index + 1);

				Py_INCREF(InnerItem);
				Py_INCREF(ParmsName);

				PyTuple_SetItem(InSelf->FuncParamTypes, Index, InnerItem);
				InSelf->FuncParamNames.Emplace(NePyBase::PyObjectToString(ParmsName));
				break;
			}
			// 形如 params=(str, bool)
			else
			{
				Py_INCREF(InnerItem);
				PyTuple_SetItem(InSelf->FuncParamTypes, Index, InnerItem);
				InSelf->FuncParamNames.Emplace(FString::Printf(TEXT("Param%d"), (int32)(Index + 1)));
			}
		}
	}

	InSelf->FuncFlags = ENePyUFunctionDefFlags::None;
	InSelf->Specifiers = FNePySpecifier::ParseSpecifiers(PySpecifierPairs, FNePySpecifier::Scope_Delegate);
	
	InSelf->DefineOrder = FNePyUDelegateDef::GlobalDefineOrder.fetch_add(1);

	return 0;
}

void FNePyUDelegateDef::Deinit(FNePyUDelegateDef* InSelf)
{
	Py_XDECREF(InSelf->FuncParamTypes);
	InSelf->FuncParamTypes = nullptr;

	InSelf->FuncFlags = ENePyUFunctionDefFlags::None;

	InSelf->FuncParamNames.Empty();

	FNePySpecifier::ReleaseSpecifiers(InSelf->Specifiers);
}

FNePyUDelegateDef* FNePyUDelegateDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUDelegateDef* Self = (FNePyUDelegateDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
		Self->FuncParamTypes = nullptr;
		Self->FuncFlags = ENePyUFunctionDefFlags::None;
	}
	return Self;
}

FNePyUDelegateDef* FNePyUDelegateDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyDelegateDefType))
	{
		return (FNePyUDelegateDef*)InPyObj;
	}
	return nullptr;
}

std::atomic<uint32> FNePyUDelegateDef::GlobalDefineOrder(0);


FNePyUComponentDef* FNePyUComponentDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUComponentDef* Self = (FNePyUComponentDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
		Self->PropType = nullptr;
		Self->bRoot = false;
		new(&Self->AttachName) FString();
		new(&Self->SocketName) FString();
		new(&Self->OverrideName) FString();
	}
	return Self;
}

void FNePyUComponentDef::Dealloc(FNePyUComponentDef* InSelf)
{
	Deinit(InSelf);

	InSelf->AttachName.~FString();
	InSelf->SocketName.~FString();
	InSelf->OverrideName.~FString();
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyUComponentDef::Init(FNePyUComponentDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyPropTypeObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:ucomponent", &PyPropTypeObj))
	{
		return -1;
	}

	PyObject* PyRootObj = nullptr;
	PyObject* PyAttachNameObj = nullptr;
	PyObject* PySocketNameObj = nullptr;
	PyObject* PyOverrideObj = nullptr;
	TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;

	if (InKwds)
	{
		PyObject* KwdKey = nullptr;
		PyObject* KwdVal = nullptr;
		Py_ssize_t KwdIndex = 0;
		while (PyDict_Next(InKwds, &KwdIndex, &KwdKey, &KwdVal))
		{
			const char* KeyStr = NePyString_AsString(KwdKey);
			if (strcmp(KeyStr, "root") == 0)
			{
				PyRootObj = KwdVal;
			}
			else if (strcmp(KeyStr, "attach") == 0)
			{
				PyAttachNameObj = KwdVal;
			}
			else if (strcmp(KeyStr, "attach_socket") == 0)
			{
				PySocketNameObj = KwdVal;
			}
			else if (strcmp(KeyStr, "override") == 0)
			{
				PyOverrideObj = KwdVal;
			}
			else
			{
				PySpecifierPairs.Emplace(KwdKey, KwdVal);
			}
		}
	}

	const char* ErrorCtxt = Py_TYPE(InSelf)->tp_name;

	bool bRoot = false;
	if (!NePyGenUtil::ConvertOptionalBool(PyRootObj, bRoot, ErrorCtxt, "Failed to convert parameter 'root' to a bool (expected 'None' or 'bool')"))
	{
		return -1;
	}

	FString AttachName;
	if (!NePyGenUtil::ConvertOptionalString(PyAttachNameObj, AttachName, ErrorCtxt, "Failed to convert parameter 'attach_socket' to a string (expected 'None' or 'str')"))
	{
		return -1;
	}

	FString SocketName;
	if (!NePyGenUtil::ConvertOptionalString(PySocketNameObj, SocketName, ErrorCtxt, "Failed to convert parameter 'attach_socket' to a string (expected 'None' or 'str')"))
	{
		return -1;
	}

	FString OverrideName;
	if (!NePyGenUtil::ConvertOptionalString(PyOverrideObj, OverrideName, ErrorCtxt, "Failed to convert parameter 'override' to a string (expected 'None' or 'str')"))
	{
		return -1;
	}

	Deinit(InSelf);

	Py_INCREF(PyPropTypeObj);
	InSelf->PropType = PyPropTypeObj;

	InSelf->bRoot = bRoot;
	InSelf->AttachName = MoveTemp(AttachName);
	InSelf->SocketName = MoveTemp(SocketName);
	InSelf->OverrideName = MoveTemp(OverrideName);
	InSelf->Specifiers = FNePySpecifier::ParseSpecifiers(PySpecifierPairs, FNePySpecifier::Scope_Property);
	
	InSelf->DefineOrder = FNePyUComponentDef::GlobalDefineOrder.fetch_add(1);

	return 0;
}

void FNePyUComponentDef::Deinit(FNePyUComponentDef* InSelf)
{
	Py_XDECREF(InSelf->PropType);
	InSelf->PropType = nullptr;

	InSelf->AttachName.Reset();
	InSelf->SocketName.Reset();

	FNePySpecifier::ReleaseSpecifiers(InSelf->Specifiers);
}

FNePyUComponentDef* FNePyUComponentDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyUComponentDefType))
	{
		return (FNePyUComponentDef*)InPyObj;
	}
	return nullptr;
}

std::atomic<uint32> FNePyUComponentDef::GlobalDefineOrder(0);

FNePyObjectRefDef* FNePyObjectRefDef::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyObjectRefDef* Self = (FNePyObjectRefDef*)InType->tp_alloc(InType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
	return Self;
}

void FNePyObjectRefDef::Dealloc(FNePyObjectRefDef* InSelf)
{
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FNePyObjectRefDef::Init(FNePyObjectRefDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj;
	int32 RefType;
	if (!PyArg_ParseTuple(InArgs, "Oi:ObjectRef", &PyObj, &RefType))
	{
		return -1;
	}

	if (RefType < 0 || RefType > 3)
	{
		PyErr_SetString(PyExc_ValueError, "value of RefType is invalid");
		return -1;
	}

	if (!DoInit(InSelf, PyObj, (ENePyObjectRefType)RefType))
	{
		return -1;
	}

	return 0;
}

bool FNePyObjectRefDef::DoInit(FNePyObjectRefDef* InSelf, PyObject* InArg, ENePyObjectRefType InRefType)
{
	// 传入的特殊对象SelfClass
	if (FNePySelfClassDef::Check(InArg))
	{
		InSelf->IsSelfClass = true;
		InSelf->Class = nullptr;
	}
	// 常规情况
	else
	{
		UClass* Class = NePyBase::ToCppClass(InArg);
		if (!Class)
		{
			PyErr_Format(PyExc_TypeError, "param of ObjectRefDef must be a unreal class, not '%s'", TCHAR_TO_UTF8(*NePyBase::PyObjectToString(InArg)));
			return false;
		}
		InSelf->IsSelfClass = false;
		InSelf->Class = Class;
	}

	InSelf->RefType = InRefType;
	
	return true;
}

PyObject* FNePyObjectRefDef::Repr(FNePyObjectRefDef* InSelf)
{
	return NePyString_FromFormat("<%s[%s]>", GetRefTypeName(InSelf->RefType), TCHAR_TO_UTF8(*InSelf->Class->GetName()));
}

PyObject* FNePyObjectRefDef::Str(FNePyObjectRefDef* InSelf)
{
	return NePyString_FromFormat("%s[%s]", GetRefTypeName(InSelf->RefType), TCHAR_TO_UTF8(*InSelf->Class->GetName()));
}

FNePyObjectRefDef* FNePyObjectRefDef::Check(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyObjectRefDefType))
	{
		return (FNePyObjectRefDef*)InPyObj;
	}
	return nullptr;
}

PyObject* FNePyObjectRefDef::Call(FNePyObjectRefDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	if (InSelf->RefType == ENePyObjectRefType::ClassReference)
	{
		PyErr_Format(PyExc_RuntimeError, "can't init %s[%s] directly, use ue.NewObject() instead", GetRefTypeName(InSelf->RefType), TCHAR_TO_UTF8(*InSelf->Class->GetName()));
		return nullptr;
	}
	else if (InSelf->RefType == ENePyObjectRefType::SoftObjectReference || InSelf->RefType == ENePyObjectRefType::SoftClassReference)
	{
		return PyObject_Call((PyObject*)NePySoftPtrGetType(), InArgs, InKwds);
	}
	else if (InSelf->RefType == ENePyObjectRefType::WeakObjectReference)
	{
		return PyObject_Call((PyObject*)NePyWeakPtrGetType(), InArgs, InKwds);
	}
	else
	{
		return nullptr;
	}
}

const char* FNePyObjectRefDef::GetRefTypeName(ENePyObjectRefType InRefType)
{
	switch (InRefType)
	{
		case ENePyObjectRefType::ObjectReference:
			return "TObjectPtr";
		case ENePyObjectRefType::ClassReference:
			return "TSubclassOf";
		case ENePyObjectRefType::SoftObjectReference:
			return "TSoftObjectPtr";
		case ENePyObjectRefType::SoftClassReference:
			return "TSoftClassPtr";
		case ENePyObjectRefType::WeakObjectReference:
			return "TWeakObjectPtr";
		default:
			return "ObjectRef";
	}
}

PyObject* FNePyObjectRefDef::Or(PyObject* InLHS, PyObject* InRHS)
{
	PyObject* PyOther;
	FNePyObjectRefDef* PySelf = FNePyObjectRefDef::Check(InLHS);
	if (PySelf)
	{
		PyOther = InRHS;
	}
	else
	{
		PySelf = FNePyObjectRefDef::Check(InRHS);
		PyOther = InLHS;
	}

	if (PySelf->RefType != ENePyObjectRefType::ClassReference)
	{
		PyErr_Format(PyExc_TypeError, "Can not use '|' operator with %s[%s]", GetRefTypeName(PySelf->RefType), TCHAR_TO_UTF8(*PySelf->Class->GetName()));
		return nullptr;
	}

	// 此处应该使用 type[ue.Class] 进行 Union 才合理
	PyTypeObject* PyType = NePyClassGetType();

	PyObject* PyRet = PyNumber_Or((PyObject*)PyType, PyOther);
	return PyRet;
}

FNePyObjectRefDef* FNePyObjectRefMaker::GetItem(FNePyObjectRefMaker* InSelf, PyObject* InKey)
{
	FNePyObjectRefDef* PyRet = FNePyObjectRefDef::New(&NePyObjectRefDefType, nullptr, nullptr);
	if (PyRet)
	{
		if (!FNePyObjectRefDef::DoInit(PyRet, InKey, InSelf->RefType))
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

FNePyObjectRefMaker* FNePyObjectRefMaker::NewObjectRefMaker(ENePyObjectRefType InRefType)
{
	FNePyObjectRefMaker* PyMaker = PyObject_New(FNePyObjectRefMaker, &NePyObjectRefMakerType);
	PyMaker->RefType = InRefType;
	return PyMaker;
}

FNePyObjectRefMaker* FNePyObjectRefMaker::Check(PyObject* InPyObj)
{
	// 由于不可继承，直接判断 ob_type
	if (InPyObj && Py_TYPE(InPyObj) == &NePyObjectRefDefType)
	{
		return (FNePyObjectRefMaker*)InPyObj;
	}
	return nullptr;
}


const char* FNePyTypeAlias::GetAliasTypeName(ENePyAliasType InAliasType)
{
	switch (InAliasType)
	{
	case ENePyAliasType::FName:
		return "Name";
	case ENePyAliasType::FText:
		return "Text";
	default:
		return "NePyTypeAlias";
	}
}

PyObject* FNePyTypeAlias::Call(FNePyTypeAlias* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyRet = PyObject_Call((PyObject*)InSelf->OriginalType, InArgs, InKwds);
	// 在 tp_call 中产生的 PyErr 都会返回到上层，因此不需要手动处理
	return PyRet;
}

PyObject* FNePyTypeAlias::GetAttro(FNePyTypeAlias* InSelf, PyObject* InAttrName)
{
	PyObject* PyAttr = PyObject_GenericGetAttr(InSelf, InAttrName);
	if (PyAttr)
	{
		return PyAttr;
	}
	PyErr_Clear();
	PyErr_Format(PyExc_AttributeError, "Can not use '%s' as a normal '%s' type.", GetAliasTypeName(InSelf->AliasType), InSelf->OriginalType->tp_name);
	return nullptr;
}

FNePyTypeAlias* FNePyTypeAlias::New(ENePyAliasType InAliasType, PyTypeObject* InOriginalType)
{
	FNePyTypeAlias* PyTypeAlias = PyObject_New(FNePyTypeAlias, &NePyTypeAliasType);
	PyTypeAlias->AliasType = InAliasType;
	PyTypeAlias->OriginalType = InOriginalType;  // 此处有个小破绽，我没有增加引用计数，因为实际只用了常驻的 str 类型，不会被回收
	Py_INCREF(PyTypeAlias->OriginalType);
	return PyTypeAlias;
}

PyObject* FNePyTypeAlias::Or(PyObject* InLHS, PyObject* InRHS)
{
	PyObject* PyOther;
	FNePyTypeAlias* PySelf = FNePyTypeAlias::Check(InLHS);
	if (PySelf)
	{
		PyOther = InRHS;
	}
	else
	{
		PySelf = FNePyTypeAlias::Check(InRHS);
		PyOther = InLHS;
	}

	PyObject* PyRet = PyNumber_Or((PyObject*)PySelf->OriginalType, PyOther);
	return PyRet;
}

FNePyTypeAlias* FNePyTypeAlias::Check(PyObject* InPyObj)
{
	// 由于不可继承，直接判断 ob_type
	if (InPyObj && Py_TYPE(InPyObj) == &NePyTypeAliasType)
	{
		return (FNePyTypeAlias*)InPyObj;
	}
	return nullptr;
}
PyObject* NePyMethod_IsOldStyleSubclassingEnabled(PyObject* InSelf)
{
	if (GetDefault<UNePythonSettings>()->bDisableOldStyleSubclassing)
	{
		Py_RETURN_FALSE;
	}
	Py_RETURN_TRUE;
}

PyObject* NePyMethod_DisableOldStyleSubclassing(PyObject* InSelf, PyObject* InArgs)
{
	bool bDisable;
	if (!PyArg_ParseTuple(InArgs, "b:DisableOldStyleSubclassing", &bDisable))
	{
		return nullptr;
	}
	PyErr_SetString(PyExc_RuntimeError, "Deprecated. Set 'bDisableOldStyleSubclassing' in ProjectSettings instead.");
	Py_RETURN_NONE;
}

PyObject* NePyMethod_IsGeneratedTypeEnabled(PyObject* InSelf)
{
	if (GNePyDisableGeneratedType)
	{
		Py_RETURN_FALSE;
	}
	Py_RETURN_TRUE;
}

PyObject* NePyMethod_DisableGeneratedType(PyObject* InSelf, PyObject* InArgs)
{
	bool bDisable;
	if (!PyArg_ParseTuple(InArgs, "b:DisableGeneratedType", &bDisable))
	{
		return nullptr;
	}
	GNePyDisableGeneratedType = bDisable;
	Py_RETURN_NONE;
}


struct FNePyUFunctionDecorator : public PyObject
{
	PyObject* Args;
	PyObject* Kwds;
};

static PyTypeObject NePyUFunctionDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	NePyInternalModuleName ".FunctionDecorator", /* tp_name */
	sizeof(FNePyUFunctionDecorator), /* tp_basicsize */
};

static void NePyUFunctionDecorator_Dealloc(FNePyUFunctionDecorator* InSelf)
{
	Py_CLEAR(InSelf->Args);
	Py_CLEAR(InSelf->Kwds);
}

static PyObject* NePyUFunctionDecorator_Call(FNePyUFunctionDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	Py_ssize_t SelfArgsSize = InSelf->Args ? PyTuple_Size(InSelf->Args) : 0;

	FNePyUFunctionDef* PyRet;
	if (!InArgs || PyTuple_Size(InArgs) != 1)
	{
		PyErr_SetString(PyExc_TypeError, "Invalid argument count.");
		return nullptr;
	}
	else if (SelfArgsSize == 0)
	{
		PyRet = FNePyUFunctionDef::New(&NePyUFunctionDefType, InSelf->Args, InSelf->Kwds);
		if (FNePyUFunctionDef::Init(PyRet, InArgs, InSelf->Kwds) != 0)
		{
			return nullptr;
		}
	}
	else
	{
		FNePyObjectPtr NewArgs = NePyStealReference(PyTuple_New(SelfArgsSize + 1));
		FNePyObjectPtr FirstArg = NePyNewReference(PyTuple_GetItem(InArgs, 0));
		PyTuple_SetItem(NewArgs, 0, FirstArg.Release());
		for (Py_ssize_t Index = 0; Index < SelfArgsSize; ++Index)
		{
			FNePyObjectPtr Arg = NePyNewReference(PyTuple_GetItem(InSelf->Args, Index));
			PyTuple_SetItem(NewArgs, Index + 1, Arg.Release());
		}

		PyRet = FNePyUFunctionDef::New(&NePyUFunctionDefType, NewArgs, InSelf->Kwds);
		if (FNePyUFunctionDef::Init(PyRet, InArgs, InSelf->Kwds) != 0)
		{
			return nullptr;
		}
	}

	return PyRet;
}

PyObject* NePyMethod_UValueFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUValueDef* PyRet = FNePyUValueDef::New(&NePyUValueDefType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUValueDef::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_UPropertyFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyFPropertyDef* PyRet = FNePyFPropertyDef::New(&NePyFPropertyDefType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyFPropertyDef::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_UParamFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUParamDef* PyRet = FNePyUParamDef::New(&NePyUParamDefType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUParamDef::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_UFunctionDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUFunctionDecorator* PyRet = PyObject_New(FNePyUFunctionDecorator, &NePyUFunctionDecoratorType);
	PyRet->Args = NePyNewReference(InArgs).Release();
	PyRet->Kwds = NePyNewReference(InKwds).Release();
	return PyRet;
}

PyObject* NePyMethod_UDelegateFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUDelegateDef* PyRet = FNePyUDelegateDef::New(&FNePyDelegateDefType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUDelegateDef::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_UComponentFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUComponentDef* PyRet = FNePyUComponentDef::New(&NePyUComponentDefType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUComponentDef::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_RefParamFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyRefParamDef* PyRet = FNePyRefParamDef::New(&NePyRefParamDefType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyRefParamDef::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_ClassReferenceFunction(PyObject* InSelf, PyObject* InArg)
{
	FNePyObjectRefDef* PyRet = FNePyObjectRefDef::New(&NePyObjectRefDefType, nullptr, nullptr);
	if (PyRet)
	{
		if (!FNePyObjectRefDef::DoInit(PyRet, InArg, ENePyObjectRefType::ClassReference))
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_SoftObjectReferenceFunction(PyObject* InSelf, PyObject* InArg)
{
	FNePyObjectRefDef* PyRet = FNePyObjectRefDef::New(&NePyObjectRefDefType, nullptr, nullptr);
	if (PyRet)
	{
		if (!FNePyObjectRefDef::DoInit(PyRet, InArg, ENePyObjectRefType::SoftObjectReference))
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_SoftClassReferenceFunction(PyObject* InSelf, PyObject* InArg)
{
	FNePyObjectRefDef* PyRet = FNePyObjectRefDef::New(&NePyObjectRefDefType, nullptr, nullptr);
	if (PyRet)
	{
		if (!FNePyObjectRefDef::DoInit(PyRet, InArg, ENePyObjectRefType::SoftClassReference))
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyObject* NePyMethod_WeakObjectReferenceFunction(PyObject* InSelf, PyObject* InArg)
{
	FNePyObjectRefDef* PyRet = FNePyObjectRefDef::New(&NePyObjectRefDefType, nullptr, nullptr);
	if (PyRet)
	{
		if (!FNePyObjectRefDef::DoInit(PyRet, InArg, ENePyObjectRefType::WeakObjectReference))
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

void NePyInitGeneratedTypes(PyObject* PyRootModule, PyObject* PyInternalModule)
{
	NePyInitGeneratedTypeContainer();

	const char* InternalModuleName = PyModule_GetName(PyInternalModule);
	FNePyObjectPtr PyInternalModuleName = NePyStealReference(NePyString_FromString(InternalModuleName));

	if (PyType_Ready(&NePySelfClassDefType) == 0)
	{
		Py_INCREF(&NePySelfClassDefType);
		PyModule_AddObject(PyInternalModule, NePySelfClassDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePySelfClassDefType);
		FNePySelfClassDef* SelfClass = FNePySelfClassDef::New(&NePySelfClassDefType, nullptr, nullptr);
		PyModule_AddObject(PyRootModule, "SelfClass", SelfClass);
	}

	if (PyType_Ready(&NePyRefParamDefType) == 0)
	{
		Py_INCREF(&NePyRefParamDefType);
		PyModule_AddObject(PyInternalModule, NePyRefParamDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyRefParamDefType);
	}

	if (PyType_Ready(&NePyUValueDefType) == 0)
	{
		Py_INCREF(&NePyUValueDefType);
		PyModule_AddObject(PyInternalModule, NePyUValueDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyUValueDefType);
	}

	if (PyType_Ready(&NePyFPropertyDefType) == 0)
	{
		Py_INCREF(&NePyFPropertyDefType);
		PyModule_AddObject(PyInternalModule, NePyFPropertyDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyFPropertyDefType);
	}

	if (PyType_Ready(&NePyUParamDefType) == 0)
	{
		Py_INCREF(&NePyUParamDefType);
		PyModule_AddObject(PyInternalModule, NePyUParamDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyUParamDefType);
	}

	if (PyType_Ready(&NePyUFunctionDefType) == 0)
	{
		Py_INCREF(&NePyUFunctionDefType);
		PyModule_AddObject(PyInternalModule, NePyUFunctionDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyUFunctionDefType);
	}

	if (PyType_Ready(&NePyUComponentDefType) == 0)
	{
		Py_INCREF(&NePyUComponentDefType);
		PyModule_AddObject(PyInternalModule, NePyUComponentDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyUComponentDefType);
	}

	if (PyType_Ready(&NePyObjectRefDefType) == 0)
	{
		Py_INCREF(&NePyObjectRefDefType);
		PyModule_AddObject(PyInternalModule, NePyObjectRefDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&NePyObjectRefDefType);
	}

	if (PyType_Ready(&FNePyDelegateDefType) == 0)
	{
		Py_INCREF(&FNePyDelegateDefType);
		PyModule_AddObject(PyInternalModule, FNePyDelegateDefType.tp_name + NePyInternalModuleNameSize, (PyObject*)&FNePyDelegateDefType);
	}

	// @ue.ufuntion装饰器
	{
		PyTypeObject* PyType = &NePyUFunctionDecoratorType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_dealloc = (destructor)&NePyUFunctionDecorator_Dealloc;
		PyType->tp_call = (ternaryfunc)&NePyUFunctionDecorator_Call;
		PyType_Ready(PyType);

		Py_INCREF(PyType);
		PyModule_AddObject(PyInternalModule, PyType->tp_name + NePyInternalModuleNameSize, (PyObject*)PyType);
	}

	// @ue.uclass装饰器
	{
		PyTypeObject* PyType = NePyInitGeneratedClass();
		Py_INCREF(PyType);
		PyModule_AddObject(PyInternalModule, PyType->tp_name + NePyInternalModuleNameSize, (PyObject*)PyType);
	}

	// @ue.ustruct装饰器
	{
		PyTypeObject* PyType = NePyInitGeneratedStruct();
		Py_INCREF(PyType);
		PyModule_AddObject(PyInternalModule, PyType->tp_name + NePyInternalModuleNameSize, (PyObject*)PyType);
	}

	// @ue.uenum装饰器
	{
		PyTypeObject* PyType = NePyInitGeneratedEnum();
		Py_INCREF(PyType);
		PyModule_AddObject(PyInternalModule, PyType->tp_name + NePyInternalModuleNameSize, (PyObject*)PyType);
	}

	// TSubclassOf, TSoftObjectPtr, TSoftClassPtr
	if (PyType_Ready(&NePyObjectRefMakerType) == 0)
	{
		// 类型本身不加入 Module
		PyObject* PyTSubclassOf = FNePyObjectRefMaker::NewObjectRefMaker(ENePyObjectRefType::ClassReference);
		Py_INCREF(PyTSubclassOf);
		PyModule_AddObject(PyRootModule, FNePyObjectRefDef::GetRefTypeName(ENePyObjectRefType::ClassReference), PyTSubclassOf);

		PyObject* PyTSoftObjectPtr = FNePyObjectRefMaker::NewObjectRefMaker(ENePyObjectRefType::SoftObjectReference);
		Py_INCREF(PyTSoftObjectPtr);
		PyModule_AddObject(PyRootModule, FNePyObjectRefDef::GetRefTypeName(ENePyObjectRefType::SoftObjectReference), PyTSoftObjectPtr);

		PyObject* PyTSoftClassPtr = FNePyObjectRefMaker::NewObjectRefMaker(ENePyObjectRefType::SoftClassReference);
		Py_INCREF(PyTSoftClassPtr);
		PyModule_AddObject(PyRootModule, FNePyObjectRefDef::GetRefTypeName(ENePyObjectRefType::SoftClassReference), PyTSoftClassPtr);

		PyObject* PyTWeakObjectPtr = FNePyObjectRefMaker::NewObjectRefMaker(ENePyObjectRefType::WeakObjectReference);
		Py_INCREF(PyTWeakObjectPtr);
		PyModule_AddObject(PyRootModule, FNePyObjectRefDef::GetRefTypeName(ENePyObjectRefType::WeakObjectReference), PyTWeakObjectPtr);
	}

	// Name, Text
	if (PyType_Ready(&NePyTypeAliasType) == 0)
	{
		PyObject* PyTypeAliasName = FNePyTypeAlias::New(ENePyAliasType::FName, &NePyString_Type);
		Py_INCREF(PyTypeAliasName);
		PyModule_AddObject(PyRootModule, FNePyTypeAlias::GetAliasTypeName(ENePyAliasType::FName), PyTypeAliasName);

		PyObject* PyTypeAliasText = FNePyTypeAlias::New(ENePyAliasType::FText, &NePyString_Type);
		Py_INCREF(PyTypeAliasText);
		PyModule_AddObject(PyRootModule, FNePyTypeAlias::GetAliasTypeName(ENePyAliasType::FText), PyTypeAliasText);
	}
}

void NePyPurgeGeneratedTypes()
{
	TArray<FWeakObjectPtr> WeakReferencesToPurgedObjects;

	auto FlagObjectForPurge = [&WeakReferencesToPurgedObjects](UObject* InObject, const bool bMarkPendingKill)
	{
		InObject->RemoveFromRoot();
		InObject->ClearFlags(RF_Public | RF_Standalone);
		InObject->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);

		if (!InObject->HasAnyFlags(RF_ClassDefaultObject))
		{
			if (bMarkPendingKill)
			{
#if ENGINE_MAJOR_VERSION >= 5
				InObject->MarkAsGarbage();
#else
				InObject->MarkPendingKill();
#endif
			}
			WeakReferencesToPurgedObjects.Add(InObject);
		}
	};

	// Clean-up Python generated class types and instances
	// The class types are instances of UNePyGeneratedClass
	{
		ForEachObjectOfClass(UNePyGeneratedClass::StaticClass(), [&FlagObjectForPurge](UObject* InObject)
		{
			FlagObjectForPurge(InObject, /*bMarkPendingKill*/false);

			UNePyGeneratedClass* PythonGeneratedClass = CastChecked<UNePyGeneratedClass>(InObject);
			ForEachObjectOfClass(PythonGeneratedClass, [&FlagObjectForPurge](UObject* InInnerObject)
			{
				FlagObjectForPurge(InInnerObject, /*bMarkPendingKill*/true);
			}, false, RF_NoFlags, EInternalObjectFlags::None);
		}, false, RF_ClassDefaultObject, EInternalObjectFlags::None);
	}

	// Clean-up Python generated struct types
	// The struct types are instances of UNePyGeneratedStruct
	{
		ForEachObjectOfClass(UNePyGeneratedStruct::StaticClass(), [&FlagObjectForPurge](UObject* InObject)
		{
			FlagObjectForPurge(InObject, /*bMarkPendingKill*/false);
		}, false, RF_ClassDefaultObject, EInternalObjectFlags::None);
	}

	// Clean-up Python generated enum types
	// The enum types are instances of UNePyGeneratedEnum
	{
		ForEachObjectOfClass(UNePyGeneratedEnum::StaticClass(), [&FlagObjectForPurge](UObject* InObject)
		{
			FlagObjectForPurge(InObject, /*bMarkPendingKill*/false);
		}, false, RF_ClassDefaultObject, EInternalObjectFlags::None);
	}

	// Clean-up Python callable instances
	{
		ForEachObjectOfClass(UNePyDelegate::StaticClass(), [&FlagObjectForPurge](UObject* InObject)
		{
			FlagObjectForPurge(InObject, /*bMarkPendingKill*/true);
		}, false, RF_NoFlags, EInternalObjectFlags::None);

		ForEachObjectOfClass(UNePyWeakDelegate::StaticClass(), [&FlagObjectForPurge](UObject* InObject)
		{
			FlagObjectForPurge(InObject, /*bMarkPendingKill*/true);
		}, false, RF_NoFlags, EInternalObjectFlags::None);
	}

	if (WeakReferencesToPurgedObjects.Num() > 0)
	{
		Py_BEGIN_ALLOW_THREADS
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		Py_END_ALLOW_THREADS

		for (const FWeakObjectPtr& WeakReferencesToPurgedObject : WeakReferencesToPurgedObjects)
		{
			if (UObject* FailedToPurgeObject = WeakReferencesToPurgedObject.Get())
			{
				if (INePyResourceOwner* PythonResourceOwner = Cast<INePyResourceOwner>(FailedToPurgeObject))
				{
					UE_LOG(LogNePython, Display, TEXT("Object '%s' was externally referenced when shutting down Python. Forcibly releasing its Python resources!"), *FailedToPurgeObject->GetPathName());
					PythonResourceOwner->ReleasePythonResources();
				}
				else
				{
					UE_LOG(LogNePython, Warning, TEXT("Object '%s' was externally referenced when shutting down Python and could not gracefully cleaned-up. This may lead to crashes!"), *FailedToPurgeObject->GetPathName());
				}
			}
		}
	}
}
