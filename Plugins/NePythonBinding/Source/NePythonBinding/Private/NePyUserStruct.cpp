#include "NePyUserStruct.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"
#include "NePyDynamicType.h"
#include "NePyGeneratedStruct.h"
#include "NePyMemoryAllocator.h"
#include "CoreMinimal.h"
#include "HAL/UnrealMemory.h"
#include "UObject/Class.h"

static PyTypeObject FNePyUserStructType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UserStruct", /* tp_name */
	sizeof(FNePyUserStruct), /* tp_basicsize */
};

// tp_alloc
PyObject* FNePyUserStruct::Alloc(PyTypeObject* PyType, Py_ssize_t NItems)
{
	const UScriptStruct* ScriptStruct = FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyType);
	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get ScriptStruct of type '%s'", PyType->tp_name);
		return nullptr;
	}

	PyTypeObject* NewestPyType = PyType;
	if (const UNePyGeneratedStruct* PyStruct = Cast<UNePyGeneratedStruct>(ScriptStruct))
	{
		NewestPyType = PyStruct->PyType;
	}

	FNePyUserStruct* PyRet = (FNePyUserStruct*)PyType_GenericAlloc(NewestPyType, NItems);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyRet);
	PyRet->PyOuter = nullptr;
	PyRet->Value = nullptr;
	PyRet->SelfCreatedValue = false;
	FNePyHouseKeeper::Get().AddUserStruct(ScriptStruct, PyRet);

	if (NewestPyType != PyType)
	{
		// 由于Reload的原因，使用了最新版本的PyType，与传入的PyType不同
		// 此时需要我们自行调用tp_init
		FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(0));
		if (NewestPyType->tp_init(PyRet, PyArgs, nullptr) < 0)
		{
			Py_DECREF(PyRet);
			return nullptr;
		}
	}

	return PyRet;
}

// tp_dealloc
void FNePyUserStruct::Dealloc(PyObject* InSelfBase)
{
	FNePyUserStruct* InSelf = (FNePyUserStruct*)InSelfBase;
	PyObject_GC_UnTrack((PyObject*)InSelf);
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	if (ScriptStruct)
	{
		FNePyHouseKeeper::Get().RemoveUserStruct(ScriptStruct, InSelf);
		if (InSelf->SelfCreatedValue)
		{
			ScriptStruct->DestroyStruct(InSelf->Value);
			FNePyStructBase::FreeValuePtr(InSelf);
			InSelf->SelfCreatedValue = false;
		}
	}
	if (InSelf->PyOuter)
	{
		Py_DECREF(InSelf->PyOuter);
		InSelf->PyOuter = nullptr;
	}
	PyObject_GC_Del(InSelf);
}

// tp_new
PyObject* FNePyUserStruct::New(PyTypeObject* InPyType, PyObject* InArgs, PyObject* InKwds)
{
	if (InPyType == &FNePyUserStructType)
	{
		PyErr_SetString(PyExc_RuntimeError, "Can't create instance of 'ue.StructBase', use one of it's subtypes instead.");
		return nullptr;
	}

	return PyType_GenericNew(InPyType, InArgs, InKwds);
}

void NePyInitUserStruct(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyUserStructType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_new = FNePyUserStruct::New;
	PyType->tp_alloc = FNePyUserStruct::Alloc;
	PyType->tp_dealloc = (destructor)FNePyUserStruct::Dealloc;
	PyType->tp_init = (initproc)FNePyDynamicStructType::Init;
	PyType->tp_base = NePyStructBaseGetType();
	PyType_Ready(PyType);

	// StructBase需要暴露给用户作为NePyGeneratedStruct的基类
	Py_INCREF(PyType);
	PyModule_AddObject(PyOuterModule, "StructBase", (PyObject*)PyType);
}

FNePyUserStruct* NePyUserStructCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&FNePyUserStructType))
	{
		return (FNePyUserStruct*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyUserStructGetType()
{
	return &FNePyUserStructType;
}
