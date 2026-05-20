#include "NePyObject.h"
#include "NePyHouseKeeper.h"
#include "NePyMemoryAllocator.h"
#include "NePyWrapperTypeRegistry.h"
#include "UObject/ObjectMacros.h"
#include "Components/ActorComponent.h"
#include "Engine/UserDefinedEnum.h"
#include "Containers/StringConv.h"
#include "NePyBase.h"
#include "NePyObjectBase.h"
#include "NePyHouseKeeper.h"

static PyTypeObject FNePyObjectType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Object", /* tp_name */
	sizeof(FNePyObject), /* tp_basicsize */
};

static PyObject* FNePyObject_Class(PyObject* InSelf) {
	UClass* Class = UObject::StaticClass();
	return NePyBase::ToPy(Class);
}

static PyMethodDef FNePyObjectType_methods[] = {
	{"Class", NePyCFunctionCast(&FNePyObject_Class), METH_NOARGS | METH_CLASS, "(cls) -> TSubclassOf[typing.Self]"},
	{ NULL } /* Sentinel */
};

FNePyObject* NePyObjectNew(UObject* InValue, PyTypeObject* InPyType)
{
	check(PyType_IsSubtype(InPyType, &FNePyObjectType));
	FNePyObject* RetValue = (FNePyObject*)PyType_GenericAlloc(InPyType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyObject* NePyObjectCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyObjectType))
	{
		return (FNePyObject*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyObjectGetType()
{
	return &FNePyObjectType;
}

void NePyInitObject(PyObject* PyOuterModule)
{
	PyTypeObject* NePyObjectType = &FNePyObjectType;
	NePyObjectType_InitCommon(NePyObjectType);
	NePyObjectType->tp_methods = FNePyObjectType_methods;
	NePyObjectType->tp_base = NePyObjectBaseGetType();
	PyType_Ready(NePyObjectType);

	Py_INCREF(NePyObjectType);
	PyModule_AddObject(PyOuterModule, "Object", (PyObject*)NePyObjectType);

	FNePyObjectTypeInfo TypeInfo = {
		NePyObjectType,
		ENePyTypeFlags::StaticPyType,
		(NePyObjectNewFunc)NePyObjectNew,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(UObject::StaticClass(), TypeInfo);
}

