#include "NePyUserTableRow.h"
#include "NePyTableRowBase.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"
#include "NePyDynamicType.h"
#include "CoreMinimal.h"

static PyTypeObject FNePyUserTableRowType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UserTableRow", /* tp_name */
	sizeof(FNePyUserTableRow), /* tp_basicsize */
};

PyObject* FNePyUserTableRow::New(PyTypeObject* InPyType, PyObject* InArgs, PyObject* InKwds)
{
	if (InPyType == &FNePyUserTableRowType)
	{
		PyErr_SetString(PyExc_RuntimeError, "Can't create instance of 'ue.TableRowBase', use one of it's subtypes instead.");
		return nullptr;
	}

	return PyType_GenericNew(InPyType, InArgs, InKwds);
}

void NePyInitUserTableRow(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyUserTableRowType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_new = FNePyUserTableRow::New;
	PyType->tp_alloc = FNePyUserTableRow::Alloc;
	PyType->tp_dealloc = (destructor)FNePyUserTableRow::Dealloc;
	PyType->tp_init = (initproc)FNePyDynamicStructType::Init;
	PyType->tp_base = NePyTableRowBaseGetType();
	PyType_Ready(PyType);

	// TableRowBase 需要暴露给用户作为 NePyGeneratedStruct 的基类
	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "TableRowBase", (PyObject*)PyType);
}

FNePyUserTableRow* NePyUserTableRowCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyUserTableRowType))
	{
		return (FNePyUserTableRow*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyUserTableRowGetType()
{
	return &FNePyUserTableRowType;
}
