#pragma once
#include "NePyBase.h"
#include "Kismet/DataTableFunctionLibrary.h"
#include "NePyStructBase.h"


PyObject* FNePyObject_DataTableFunctionLibrary_Generic_GetDataTableRowFromName(FNePyObject_DataTableFunctionLibrary* InSelf, PyObject* InArgs)
{
	PyObject* PyArgs[2] = { nullptr, nullptr };
	if (!PyArg_ParseTuple(InArgs, "OO:Generic_GetDataTableRowFromName", &PyArgs[0], &PyArgs[1]))
	{
		return nullptr;
	}

	UDataTable* Table;
	if (FNePyObject_DataTable* PyTable = NePyObjectCheck_DataTable(PyArgs[0]))
	{
		if (FNePyHouseKeeper::Get().IsValid(PyTable))
		{
			Table = PyTable->GetValue();
		}
		else
		{
			PyErr_SetString(PyExc_TypeError, "arg1 'Table' underlying UObject is invalid");
			return nullptr;
		}
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, "arg1 'Table' must have type 'DataTable'");
		return nullptr;
	}

	FName RowName;
	if (!NePyBase::ToCpp(PyArgs[1], RowName))
	{
		PyErr_SetString(PyExc_TypeError, "arg2 'RowName' must have type 'str'");
		return nullptr;
	}

	// 前面都是套路，这里开始是特殊处理部分
	const UScriptStruct* ScriptStruct = Table->GetRowStruct();
	PyTypeObject* StructPyType = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(ScriptStruct)->TypeObject;
	PyObject* RetStruct = StructPyType->tp_new(StructPyType, nullptr, nullptr);
	void* OutRow = ((FNePyStructBase*)RetStruct)->Value;
	ScriptStruct->InitializeStruct(OutRow);

	auto RetVal = UDataTableFunctionLibrary::Generic_GetDataTableRowFromName(Table, RowName, OutRow);

	PyObject* PyRetVal0 = PyBool_FromLong(RetVal);
	PyObject* PyRetVal1;
	if (RetVal)
	{
		PyRetVal1 = RetStruct;
	}
	else
	{
		Py_DECREF(RetStruct);
		Py_INCREF(Py_None);
		PyRetVal1 = Py_None;
	}

	PyObject* PyRetVals = PyTuple_New(2);
	PyTuple_SetItem(PyRetVals, 0, PyRetVal0);
	PyTuple_SetItem(PyRetVals, 1, PyRetVal1);
	return PyRetVals;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetDataTableRowFromName", NePyCFunctionCast(&FNePyObject_DataTableFunctionLibrary_Generic_GetDataTableRowFromName), METH_VARARGS | METH_STATIC, "(Table: DataTable, RowName: str) -> tuple[bool, StructBase]"},
