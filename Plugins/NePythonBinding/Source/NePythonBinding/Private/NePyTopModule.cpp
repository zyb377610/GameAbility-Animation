#include "NePyTopModule.h"
#include "NePyPtr.h"
#include "NePyBase.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyGeneratedClass.h"
#include "NePyGeneratedStruct.h"
#include "NePyGeneratedEnum.h"
#include "Engine/BlueprintGeneratedClass.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5
#include "StructUtils/UserDefinedStruct.h"
#else
#include "Engine/UserDefinedStruct.h"
#endif
#include "Engine/UserDefinedEnum.h"


// 顶层模块静态方法
// 本体在NePyTopModuleMethods.cpp中
extern PyMethodDef NePyTopModuleMethods[];

// 顶层模块的__dict__
// 只引用，不增减引用计数
static PyObject* NePyTopModuleDict;

static PyTypeObject FNePyTopModuleType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"NePyTopModule", /* tp_name */
};

// 按照UClass->UStruct->UEnum的优先级，在整个引擎中查找UE类型，并生成PyType
// 返回 new reference
static PyObject* NePyTopModule_SearchAndRegisterPyTypeSlow(PyObject* InAttrName)
{
	FString SearchName(NePyString_AsString(InAttrName));

	UClass* Class = nullptr;
	UScriptStruct* Struct = nullptr;
	UEnum* Enum = nullptr;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
	TArray<UObject*> FoundObjects;
	StaticFindAllObjects(FoundObjects, nullptr, *SearchName);

	auto FindObjectOfType = [&FoundObjects](UClass* Type) -> UObject* {
		for (UObject* Object : FoundObjects)
		{
			if (Object->IsA(Type))
			{
				return Object;
			}
		}
		return nullptr;
	};

	Class = (UClass*)FindObjectOfType(UClass::StaticClass());
	if (!Class)
	{
		Struct = (UScriptStruct*)FindObjectOfType(UScriptStruct::StaticClass());
		if (!Struct)
		{
			Enum = (UEnum*)FindObjectOfType(UEnum::StaticClass());
		}
	}
#else
	Class = FindObject<UClass>(ANY_PACKAGE, *SearchName);
	if (!Class)
	{
		Struct = FindObject<UScriptStruct>(ANY_PACKAGE, *SearchName);
		if (!Struct)
		{
			Enum = FindObject<UEnum>(ANY_PACKAGE, *SearchName);
		}
	}
#endif

	PyTypeObject* PyType = nullptr;
	if (Class)
	{
		if (Class->HasAnyInternalFlags(EInternalObjectFlags::Native) && !((const UObject*)Class)->IsA<UNePyGeneratedClass>())
		{
			check(!((const UObject*)Class)->IsA<UBlueprintGeneratedClass>());
			auto TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(Class);
			if (TypeInfo)
			{
				PyType = TypeInfo->TypeObject;
				if (EnumHasAnyFlags(TypeInfo->TypeFlags, ENePyTypeFlags::DynamicPyType))
				{
					PyDict_SetItemString(NePyTopModuleDict, PyType->tp_name, (PyObject*)PyType);
				}
			}
		}
	}
	else if (Struct)
	{
		if (Struct->HasAnyInternalFlags(EInternalObjectFlags::Native) && !((const UObject*)Struct)->IsA<UNePyGeneratedStruct>())
		{
			check(!Struct->IsA<UUserDefinedStruct>());
			auto TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(Struct);
			if (TypeInfo)
			{
				PyType = TypeInfo->TypeObject;
				if (EnumHasAnyFlags(TypeInfo->TypeFlags, ENePyTypeFlags::DynamicPyType))
				{
					PyDict_SetItemString(NePyTopModuleDict, PyType->tp_name, (PyObject*)PyType);
				}
			}
		}
	}
	else if (Enum)
	{
		if (Enum->HasAnyInternalFlags(EInternalObjectFlags::Native) && !((const UObject*)Enum)->IsA<UNePyGeneratedEnum>())
		{
			check(!Enum->IsA<UUserDefinedEnum>());
			auto TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedEnumType(Enum);
			if (TypeInfo)
			{
				PyType = TypeInfo->TypeObject;
				if (EnumHasAnyFlags(TypeInfo->TypeFlags, ENePyTypeFlags::DynamicPyType))
				{
					PyDict_SetItemString(NePyTopModuleDict, PyType->tp_name, (PyObject*)PyType);
				}
			}
		}
	}

	Py_XINCREF(PyType);
	return (PyObject*)PyType;
}

static PyObject* NePyTopModule_Getattro(PyObject* InSelf, PyObject* InAttrName)
{
	PyObject* PyRet = FNePyTopModuleType.tp_base->tp_getattro(InSelf, InAttrName);
	if (PyRet)
	{
		return PyRet;
	}

	PyErr_Clear();
	PyRet = NePyTopModule_SearchAndRegisterPyTypeSlow(InAttrName);
	if (PyRet)
	{
		return PyRet;
	}

	const char* StrAttr = NePyString_AsString(InAttrName);
	PyErr_Format(PyExc_AttributeError,
		"'module' object has no attribute '%.400s'", StrAttr);
	return nullptr;
}

PyObject* NePyInitTopModule(const char* RootModuleName)
{
	PyTypeObject* PyType = &FNePyTopModuleType;
	PyType->tp_base = &PyModule_Type;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_getattro = NePyTopModule_Getattro;
	if (PyType_Ready(PyType) < 0)
	{
		UE_LOG(LogNePython, Fatal, TEXT("Unable to init NePyTopModule type!"));
		return nullptr;
	}

#if PY_MAJOR_VERSION >= 3
	PyObject* PyModule = PyImport_AddModule(RootModuleName); // Borrowed reference
	PyModule_AddFunctions(PyModule, NePyTopModuleMethods);
#else
	PyObject* PyModule = Py_InitModule(RootModuleName, NePyTopModuleMethods);  // Borrowed reference
#endif

	// 黑科技：替换Module的ob_type，使我们的getattro函数可以生效
	// 理论上应该调用object_set_class函数完成这件事，但py2的这个函数会中一个检查。
	// 所以退而求其次，直接设置类型。
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 10
	Py_SET_TYPE(PyModule, PyType);
#else
	Py_TYPE(PyModule) = PyType;
#endif

	// 记录ue根模块的__dict__
	NePyTopModuleDict = PyModule_GetDict(PyModule);

	return PyModule;
}

void NePyAddTypeToTopModule(PyTypeObject* InPyType)
{
	PyObject* OldObj = PyDict_GetItemString(NePyTopModuleDict, InPyType->tp_name);
	if (OldObj)
	{
		if (OldObj == (PyObject*)InPyType)
		{
			return;
		}

		FString OldObjectStr = NePyBase::PyObjectToString(OldObj);
		FString NewObjectStr = NePyBase::PyObjectToString((PyObject*)InPyType);
		UE_LOG(LogNePython, Fatal, TEXT("Name collision detected in module 'ue'! old_object: '%s' new_object: '%s'"),
			*OldObjectStr, *NewObjectStr);
	}

	PyDict_SetItemString(NePyTopModuleDict, InPyType->tp_name, (PyObject*)InPyType);
}
