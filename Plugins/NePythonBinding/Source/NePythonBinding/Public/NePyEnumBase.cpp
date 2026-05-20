#include "NePyEnumBase.h"
#include "NePyBase.h"
#include "Engine/UserDefinedEnum.h"
#include "NePyWrapperTypeRegistry.h"
#include "UObject/Class.h"
#include "NePyUtil.h"

static PyTypeObject FNePyEnumMetaType = {
	PyVarObject_HEAD_INIT(&PyType_Type, 0)
	"EnumMeta", /* tp_name */
};

static PyTypeObject FNePyEnumBaseType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"EnumBase", /* tp_name */
	sizeof(FNePyEnumBase), /* tp_basicsize */
};

static PyObject* NePyEnumBase_MemberMap_Name;
static PyObject* NePyEnumBase_ValueToMemberMap_Name;
static PyObject* NePyEnumBase_EntryToNameMap_Name;

// tp_repr
static PyObject* NePyEnumMeta_Repr(PyTypeObject* InPyType)
{
	return NePyString_FromFormat("<enum %s>", InPyType->tp_name);
}

static PyObject* NePyEnumMetaMethod_Enum(PyTypeObject* InPyType)
{
	const UEnum* Enum = FNePyWrapperTypeRegistry::Get().GetEnumByPyType(InPyType);
	return NePyBase::ToPy(Enum);
}

static PyObject* NePyEnumMetaMethod_Iter(PyTypeObject* InPyType)
{
	PyObject* PyEntryMap = PyDict_GetItem(InPyType->tp_dict, NePyEnumBase_EntryToNameMap_Name);
	if (PyEntryMap)
	{
		return PyObject_GetIter(PyEntryMap);
	}
	return nullptr;
}

static int NePyEnumMetaMethod_Contain(PyTypeObject* InPyType, PyObject* InKey)
{
	PyObject* PyValueMap = PyDict_GetItem(InPyType->tp_dict, NePyEnumBase_ValueToMemberMap_Name);
	if (PyValueMap)
	{
		return PyDict_Contains(PyValueMap, InKey);
	}
	return -1;
}

static Py_ssize_t NePyEnumMetaMethod_Len(PyTypeObject* InPyType)
{
	PyObject* PyMemberMap = PyDict_GetItem(InPyType->tp_dict, NePyEnumBase_MemberMap_Name);
	if (PyMemberMap)
	{
		auto Count = PyDict_Size(PyMemberMap);
		return Count;
	}
	return -1;
}

static PyObject* NePyEnumMetaMethod_GetItem(PyTypeObject* InPyType, PyObject* InKey)
{
	PyObject* PyMemberMap = PyDict_GetItem(InPyType->tp_dict, NePyEnumBase_MemberMap_Name);
	if (PyMemberMap)
	{
		PyObject* PyEnumEntry = PyDict_GetItem(PyMemberMap, InKey);
		if (!PyEnumEntry)
		{
			PyErr_Format(PyExc_KeyError, "Enum name: %s is not found in %s", TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InKey)), TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(InPyType)));
			return nullptr;
		}
		Py_INCREF(PyEnumEntry);
		return PyEnumEntry;
	}
	return nullptr;
}

static int NePyEnumMetaMethod_Bool(PyTypeObject* InPyType)
{
	return 1;
}

// tp_new
PyObject* NePyEnumBase_New(PyTypeObject* InPyType, PyObject* InArgs, PyObject* InKwds)
{
	if (InPyType == &FNePyEnumBaseType)
	{
		PyErr_SetString(PyExc_RuntimeError, "Can't create instance of 'ue.EnumBase', use one of it's subtypes instead.");
		return nullptr;
	}

	PyObject* PyValue;
	if (!PyArg_ParseTuple(InArgs, "O:__new__", &PyValue))
	{
		return nullptr;
	}

	if (Py_TYPE(PyValue) == InPyType)
	{
		Py_INCREF(PyValue);
		return PyValue;
	}

	PyObject* PyValueToMemberMap = PyDict_GetItem(InPyType->tp_dict, NePyEnumBase_ValueToMemberMap_Name);
	PyObject* PyEnumEntry = PyDict_GetItem(PyValueToMemberMap, PyValue);
	if (!PyEnumEntry)
	{
		FString ValueStr = NePyBase::PyObjectToString(PyValue);
		PyErr_Format(PyExc_ValueError, "%s is not a valid %s", TCHAR_TO_UTF8(*ValueStr), InPyType->tp_name);
		return nullptr;
	}

	Py_INCREF(PyEnumEntry);
	return PyEnumEntry;
}

// tp_repr
static PyObject* NePyEnumBase_Repr(FNePyEnumBase* InSelf)
{
	PyTypeObject* PyType = Py_TYPE(InSelf);
	PyObject* PyEntryToNameMap = PyDict_GetItem(PyType->tp_dict, NePyEnumBase_EntryToNameMap_Name);
	PyObject* PyEntryName = PyDict_GetItem(PyEntryToNameMap, (PyObject*)InSelf);
	uint64 EntryValue = PyLong_AsUnsignedLongLong((PyObject*)InSelf);
	if (PyEntryName)
	{
		const char* EntryName = NePyString_AsString(PyEntryName);
		return NePyString_FromFormat("<%s.%s: %llu>", PyType->tp_name, EntryName, EntryValue);
	}
	return NePyString_FromFormat("<%s.[unknown]: %llu>", PyType->tp_name, EntryValue);
}

PyObject* NePyEnumBaseMethod_GetDisplayName(FNePyEnumBase* InSelf)
{
	const UEnum* Enum = FNePyEnumBase::GetEnum(InSelf);
	if (!Enum)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get Enum of type '%s'", Py_TYPE(InSelf)->tp_name);
		return nullptr;
	}

	int64 EnumEntryValue = PyLong_AsLongLong((PyObject*)InSelf);
	FText DisplayName = Enum->GetDisplayNameTextByValue(EnumEntryValue);
	PyObject* RetValue;
	NePyBase::ToPy(DisplayName, RetValue);
	return RetValue;
}

PyObject* NePyEnumBaseMethod_GetName(FNePyEnumBase* InSelf)
{
	PyTypeObject* PyType = Py_TYPE(InSelf);
	PyObject* PyEntryToNameMap = PyDict_GetItem(PyType->tp_dict, NePyEnumBase_EntryToNameMap_Name);
	PyObject* PyEntryName = PyDict_GetItem(PyEntryToNameMap, (PyObject*)InSelf);
	if (!PyEntryName)
	{
		PyErr_SetString(PyExc_Exception, "Can't get name of enum entry");
		return nullptr;
	}
	Py_INCREF(PyEntryName);
	return PyEntryName;	
}

PyObject* NePyEnumBaseMethod_GetValue(FNePyEnumBase* InSelf)
{
	Py_INCREF(InSelf);
	return (PyObject*)InSelf;	
}

static PyMethodDef NePyEnumBase_methods[] = {
	{"GetDisplayName", NePyCFunctionCast(&NePyEnumBaseMethod_GetDisplayName), METH_NOARGS, "() -> str"},
	{"GetName", NePyCFunctionCast(&NePyEnumBaseMethod_GetName), METH_NOARGS, "() -> str"},
	{"GetValue", NePyCFunctionCast(&NePyEnumBaseMethod_GetValue), METH_NOARGS, "() -> int"},
	{"Enum", NePyCFunctionCast(&NePyEnumMetaMethod_Enum), METH_NOARGS | METH_CLASS, "() -> Enum"},
	{ NULL } /* Sentinel */
};

void FNePyEnumBase::InitEnumEntries(PyTypeObject* InPyType, const UEnum* InEnum)
{
	PyObject* PyMemberMap = PyDict_New();
	PyDict_SetItem(InPyType->tp_dict, NePyEnumBase_MemberMap_Name, PyMemberMap);
	PyObject* PyValueToMemberMap = PyDict_New();
	PyDict_SetItem(InPyType->tp_dict, NePyEnumBase_ValueToMemberMap_Name, PyValueToMemberMap);
	PyObject* PyEntryToNameMap = PyDict_New();
	PyDict_SetItem(InPyType->tp_dict, NePyEnumBase_EntryToNameMap_Name, PyEntryToNameMap);

	const UUserDefinedEnum* UserDefinedEnum = Cast<UUserDefinedEnum>(InEnum);
	FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(1));
	for (int32 Index = 0; Index < InEnum->NumEnums(); ++Index)
	{
		FString EntryName = InEnum->GetNameStringByIndex(Index);
		if (EntryName.EndsWith(TEXT("_MAX"), ESearchCase::CaseSensitive))
		{
			continue;
		}

		if (UserDefinedEnum)
		{
			EntryName = InEnum->GetDisplayNameTextByIndex(Index).ToString();
		}
		else
		{
			ReplaceEnumItemName(EntryName);
		}

		int64 EntryValue = InEnum->GetValueByIndex(Index);
		PyObject* PyEntryValue = PyLong_FromLongLong(EntryValue);
		PyTuple_SetItem(PyArgs, 0, PyEntryValue); // steal reference
		FNePyObjectPtr PyEnumEntry = NePyStealReference(PyLong_Type.tp_new(InPyType, PyArgs, nullptr));
		FNePyObjectPtr PyEntryName = NePyStealReference(NePyString_FromString(TCHAR_TO_UTF8(*EntryName)));

		PyDict_SetItem(PyMemberMap, PyEntryName, PyEnumEntry);
		PyDict_SetItem(InPyType->tp_dict, PyEntryName, PyEnumEntry);
		PyDict_SetItem(PyValueToMemberMap, PyEntryValue, PyEnumEntry);
		PyDict_SetItem(PyEntryToNameMap, PyEnumEntry, PyEntryName);
	}
}

const UEnum* FNePyEnumBase::GetEnum(FNePyEnumBase* InSelf)
{
	PyTypeObject* PyType = Py_TYPE(InSelf);
	check(PyType_IsSubtype(PyType, &FNePyEnumBaseType));
	const UEnum* Enum = FNePyWrapperTypeRegistry::Get().GetEnumByPyType(PyType);
	return Enum;
}

void FNePyEnumBase::ReplaceEnumItemName(FString& InOutName)
{
	if (InOutName == TEXT("None"))
	{
		InOutName = TEXT("NONE");
	}
	else if (InOutName == TEXT("Enum"))
	{
		InOutName = TEXT("ENUM");
	}
}

void NePyInitEnumBase(PyObject* PyOuterModule)
{
	NePyEnumBase_MemberMap_Name = NePyString_FromString("_member_map_");
	NePyEnumBase_ValueToMemberMap_Name = NePyString_FromString("_value2member_map_");
	NePyEnumBase_EntryToNameMap_Name = NePyString_FromString("_entry2name_map_");

	static PySequenceMethods PySequence;
	PySequence.sq_contains = (objobjproc)NePyEnumMetaMethod_Contain;

	static PyMappingMethods PyMapping;
	PyMapping.mp_length = (lenfunc)NePyEnumMetaMethod_Len;
	PyMapping.mp_subscript = (binaryfunc)NePyEnumMetaMethod_GetItem;

	static PyNumberMethods PyNumber;
#if PY_MAJOR_VERSION >= 3
	PyNumber.nb_bool = (inquiry)NePyEnumMetaMethod_Bool;
#else
	PyNumber.nb_nonzero = (inquiry)NePyEnumMetaMethod_Bool;
#endif

	PyTypeObject* MetaType = &FNePyEnumMetaType;
	MetaType->tp_base = &PyType_Type;
	MetaType->tp_flags = Py_TPFLAGS_DEFAULT;
	MetaType->tp_repr = (reprfunc)NePyEnumMeta_Repr;
	MetaType->tp_iter = (getiterfunc)NePyEnumMetaMethod_Iter;
	MetaType->tp_as_sequence = &PySequence;
	MetaType->tp_as_mapping = &PyMapping;
	MetaType->tp_as_number = &PyNumber;
	PyType_Ready(MetaType);

	PyTypeObject* PyType = &FNePyEnumBaseType;
	PyType->tp_base = &PyLong_Type;
#if PY_MAJOR_VERSION == 3 && PY_MINOR_VERSION >= 10
	Py_SET_TYPE(PyType, MetaType);
#else
	Py_TYPE(PyType) = MetaType;
#endif
	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_new = NePyEnumBase_New;
	PyType->tp_repr = (reprfunc)NePyEnumBase_Repr;
	PyType->tp_methods = NePyEnumBase_methods;
	PyType_Ready(PyType);

	// EnumBase需要暴露给用户作为NePyGeneratedEnum的基类
	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "EnumBase", (PyObject*)PyType);
}

FNePyEnumBase* NePyEnumBaseCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyEnumBaseType))
	{
		return (FNePyEnumBase*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyEnumBaseGetType()
{
	return &FNePyEnumBaseType;
}
