#include "NePyEnum.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"

static PyTypeObject FNePyEnumType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Enum", /* tp_name */
	sizeof(FNePyEnum), /* tp_basicsize */
};


// tp_repr
static PyObject* NePyEnum_Repr(FNePyEnum* InSelf)
{
	if (!FNePyHouseKeeper::Get().IsValid(InSelf))
	{
		return PyUnicode_FromFormat("<Invalid UEnum at %p>", InSelf->Value);
	}
	return PyUnicode_FromFormat("<Enum '%s' at %p>", TCHAR_TO_UTF8(*InSelf->GetValue()->GetName()), InSelf->GetValue());
}

FNePyEnum* NePyEnumNew(UEnum* InValue, PyTypeObject* InPyType)
{
	check(PyType_IsSubtype(InPyType, &FNePyEnumType));
	FNePyEnum* RetValue = PyObject_New(FNePyEnum, InPyType);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyEnum* NePyEnumCheck(const PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyEnumType))
	{
		return (FNePyEnum*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyEnumGetType()
{
	return &FNePyEnumType;
}

void NePyInitEnum(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyEnumType;
	NePyObjectType_InitCommon(PyType);
	PyType->tp_base = NePyObjectBaseGetType();
	PyType->tp_repr = (reprfunc)&NePyEnum_Repr;
	PyType->tp_str = (reprfunc)&NePyEnum_Repr;
	PyType->tp_flags &= ~Py_TPFLAGS_BASETYPE;
	PyType_Ready(PyType);

	PyModule_AddObject(PyOuterModule, "Enum", (PyObject*)PyType);

	FNePyObjectTypeInfo TypeInfo = {
		PyType,
		ENePyTypeFlags::StaticPyType,
		(NePyObjectNewFunc)NePyEnumNew,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(UEnum::StaticClass(), TypeInfo);
}
