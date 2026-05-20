#include "NePyTableRowBase.h"
#include "Misc/AssertionMacros.h"
#include "NePyHouseKeeper.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyWrapperInitializer.h"

static PyTypeObject FNePyTableRowBaseType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"TableRowBase", /* tp_name */
	sizeof(FNePyTableRowBase), /* tp_basicsize */
};

bool NePyStructPropSet_TableRowBase(const FStructProperty* InStructProp, FNePyStructBase* InSelf, void* InBuffer)
{
	if (FNePyTableRowBase* PyObj = NePyTableRowBaseCheck(InSelf))
	{
		InStructProp->Struct->CopyScriptStruct(InBuffer, PyObj->Value);
		return true;
	}
	return false;
}

FNePyStructBase* NePyStructPropGet_TableRowBase(const FStructProperty* InStructProp, const void* InBuffer)
{
	PyTypeObject* PyType = &FNePyTableRowBaseType;
	FNePyTableRowBase* PyObj = (FNePyTableRowBase*)PyType->tp_alloc(PyType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyObj);
#if NEPY_ENABLE_STRUCT_DEEP_ACCESS
	PyObj->Value = (FTableRowBase*)InBuffer;
	PyObj->SelfCreatedValue = false;
#else
	PyObj->Value = (FTableRowBase*)FMemory::Malloc(sizeof(FTableRowBase), alignof(FTableRowBase));
	NePyBase::EnsureCopyToStructSafely<FTableRowBase>(InStructProp, PyObj->Value);
	InStructProp->Struct->CopyScriptStruct(PyObj->Value, InBuffer);
	PyObj->SelfCreatedValue = true;
#endif
	return PyObj;
}

void NePyInitTableRowBase(PyObject* PyOuterModule)
{
	PyTypeObject* NePyStructType = &FNePyTableRowBaseType;
	TNePyStructBase<FTableRowBase>::InitTypeCommon(NePyStructType);
	NePyStructType->tp_base = NePyStructBaseGetType();
	PyType_Ready(NePyStructType);

	FNePyStructTypeInfo TypeInfo = {
		NePyStructType,
		ENePyTypeFlags::StaticPyType,
		(NePyStructPropSet)NePyStructPropSet_TableRowBase,
		(NePyStructPropGet)NePyStructPropGet_TableRowBase,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedStructType(FTableRowBase::StaticStruct(), TypeInfo);
}

FNePyTableRowBase* NePyTableRowBaseNew(const FTableRowBase& InValue)
{
	PyTypeObject* PyType = &FNePyTableRowBaseType;
	FNePyTableRowBase* RetValue = (FNePyTableRowBase*)PyType->tp_alloc(PyType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	new (RetValue->Value) FTableRowBase(InValue);
	return RetValue;
}

FNePyTableRowBase* NePyTableRowBaseCheck(PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyTableRowBaseType))
	{
		return (FNePyTableRowBase*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyTableRowBaseGetType()
{
	return &FNePyTableRowBaseType;
}

namespace NePyBase
{
	bool ToCpp(PyObject* InPyObj, FTableRowBase& OutVal)
	{
		if (FNePyTableRowBase* PyObj = NePyTableRowBaseCheck(InPyObj))
		{
			OutVal = *(FTableRowBase*)PyObj->Value;
			return true;
		}
		return false;
	}

	bool ToPy(const FTableRowBase& InVal, PyObject*& OutPyObj)
	{
		OutPyObj = NePyTableRowBaseNew(InVal);
		return true;
	}
}
#ifdef _MSC_VER
#pragma warning(default:4996)
#endif
