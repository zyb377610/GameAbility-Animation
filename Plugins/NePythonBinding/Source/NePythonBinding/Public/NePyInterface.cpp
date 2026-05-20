#include "NePyInterface.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"

static PyTypeObject FNePyInterfaceType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UInterface", /* tp_name */
	sizeof(FNePyInterface), /* tp_basicsize */
};

FNePyInterface* NePyInterfaceNew(UInterface* InValue, PyTypeObject* InPyType)
{
	check(PyType_IsSubtype(InPyType, &FNePyInterfaceType));
	FNePyInterface* RetValue = PyObject_New(FNePyInterface, InPyType);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyInterface* NePyInterfaceCheck(const PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyInterfaceType))
	{
		return (FNePyInterface*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyInterfaceGetType()
{
	return &FNePyInterfaceType;
}

void NePyInitInterface(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyInterfaceType;
	NePyObjectType_InitCommon(PyType);
	PyType->tp_base = NePyObjectBaseGetType();
	PyType_Ready(PyType);

	// 不需要加入到模块中，我们不希望用户能直接从Python中构建Interface对象
	//PyModule_AddObject(PyOuterModule, "Interface", (PyObject*)PyType);

	FNePyObjectTypeInfo TypeInfo = {
		PyType,
		ENePyTypeFlags::StaticPyType,
		(NePyObjectNewFunc)NePyInterfaceNew,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(UInterface::StaticClass(), TypeInfo);
}

