#include "NePyDynamicType.h"
#include "NePyObjectBase.h"
#include "NePyStructBase.h"
#include "NePyEnumBase.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyMemoryAllocator.h"
#include "NePyDescriptor.h"
#include "HAL/UnrealMemory.h"
#include "UObject/Class.h"

FNePyDynamicType::FNePyDynamicType()
{
	TypeObject = { PyVarObject_HEAD_INIT(nullptr, 0) };
}

PyTypeObject* FNePyDynamicType::GetPyType()
{
	return &TypeObject;
}

const char* FNePyDynamicType::GetName() const
{
	return TypeName.GetData();
}

static PyMethodDef NePyDynamicClassType_methods[] = {
	{"Class", NePyCFunctionCast(&FNePyDynamicClassType::Class), METH_NOARGS | METH_CLASS, ""},
	{ NULL } /* Sentinel */
};

void FNePyDynamicClassType::InitDynamicClassType(const UClass* InClass, const FNePyObjectTypeInfo* SuperTypeInfo, PyTypeObject* InPyBase, PyObject* InPyBases)
{
	check(SuperTypeInfo);
	check(InPyBase);
	check(SuperTypeInfo->TypeObject == InPyBase);
	
	// 获取类型名称并拷贝保存
	TypeName = NePyGenUtil::TCHARToUTF8Buffer(*InClass->GetName());

	// InitPyType
	PyTypeObject* NePyType = GetPyType();
	NePyType->tp_name = GetName();
	NePyType->tp_basicsize = InPyBase->tp_basicsize;

	// 因为父类可能是手写的导出类，手写的不一定走InitCommon，所以这里基本什么都不做，继承父类设定即可
	// NePyObjectType_InitCommon(NePyType);

	NePyType->tp_base = InPyBase;
	NePyType->tp_bases = InPyBases;
	NePyType->tp_repr = InPyBase->tp_repr;
	NePyType->tp_str = InPyBase->tp_str;
	NePyType->tp_methods = NePyDynamicClassType_methods;

	// flags就没办法继承父类了，cpython里面自己塞了太多东西
#if PY_MAJOR_VERSION < 3
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
	NePyType->tp_flags |= Py_TPFLAGS_BASETYPE;
	NePyType->tp_dictoffset = InPyBase->tp_dictoffset;  // 不确定是否需要，先加上
	PyType_Ready(NePyType);

	// 注册TypeInfo
	FNePyObjectTypeInfo TypeInfo = *SuperTypeInfo; // NewFunc与父类相同
	TypeInfo.TypeObject = NePyType;
	TypeInfo.TypeFlags = ENePyTypeFlags::DynamicPyType;
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(InClass, TypeInfo);
}

PyObject* FNePyDynamicClassType::Class(PyObject* InSelf)
{
	const UClass* Class = FNePyWrapperTypeRegistry::Get().GetClassByPyType((PyTypeObject*)InSelf);
	return NePyBase::ToPy(Class);
}

// tp_alloc
PyObject* FNePyDynamicStructType::Alloc(PyTypeObject* PyType, Py_ssize_t NItems)
{
	FNePyStructBase* PyRet = (FNePyStructBase*)PyType_GenericAlloc(PyType, NItems);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyRet);
	return PyRet;
}

// tp_dealloc
void FNePyDynamicStructType::Dealloc(FNePyStructBase* InSelf)
{
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	if (ScriptStruct && !GExitPurge)
	{
		if (InSelf->SelfCreatedValue)
		{
			FNePyStructBase::FreeValuePtr(InSelf);
			InSelf->SelfCreatedValue = false;
		}
		if (InSelf->PyOuter)
		{
			Py_DECREF(InSelf->PyOuter);
			InSelf->PyOuter = nullptr;
		}
	}
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

// tp_new
FNePyDynamicStructType* FNePyDynamicStructType::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FNePyDynamicStructType* Self = (FNePyDynamicStructType*)InType->tp_alloc(InType, 0);
	return Self;
}

// tp_init
int FNePyDynamicStructType::Init(FNePyStructBase* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InSelf);
	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get ScriptStruct of type '%s'", Py_TYPE(InSelf)->tp_name);
		return -1;
	}

	if (InKwds && PyDict_Size(InKwds) > 0)
	{
		PyErr_Format(PyExc_TypeError, "%.200s.__init__() takes no keyword arguments", Py_TYPE(InSelf)->tp_name);
		return -1;
	}

	if (InArgs && PyTuple_GET_SIZE(InArgs) > 0)
	{
		PyErr_Format(PyExc_TypeError, "%.200s.__init__() takes no arguments(%zd given)", Py_TYPE(InSelf)->tp_name, PyTuple_GET_SIZE(InArgs));
		return -1;
	}

	if (!InSelf->SelfCreatedValue)
	{
		void* ValuePtr = FNePyStructBase::AllocateValuePtr(InSelf, ScriptStruct);
		check(ValuePtr);
		ScriptStruct->InitializeStruct(ValuePtr);
	}

	return 0;
}

bool FNePyDynamicStructType::PropSet(const FStructProperty* InStructProp, FNePyStructBase* InPyObj, void* InBuffer)
{
	const UScriptStruct* ScriptStruct = FNePyStructBase::GetScriptStruct(InPyObj);
	if (!ScriptStruct)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get ScriptStruct of type '%s'", Py_TYPE(InPyObj)->tp_name);
		return false;
	}

	if (ScriptStruct->IsChildOf(InStructProp->Struct))
	{
		void* ValuePtr = InPyObj->Value;
		check(ValuePtr);
		InStructProp->Struct->CopyScriptStruct(InBuffer, ValuePtr);
		return true;
	}

	return false;
}

FNePyStructBase* FNePyDynamicStructType::PropGet(const FStructProperty* InStructProp, void* InBuffer)
{
	const UScriptStruct* ScriptStruct = InStructProp->Struct;
	const FNePyStructTypeInfo* TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(ScriptStruct);
	if (!TypeInfo)
	{
		PyErr_Format(PyExc_RuntimeError, "Can't get PyType of ScriptStruct '%s'", TCHAR_TO_UTF8(*ScriptStruct->GetName()));
		return nullptr;
	}

	PyTypeObject* PyType = TypeInfo->TypeObject;
	FNePyStructBase* PyRet = (FNePyStructBase*)PyType->tp_alloc(PyType, 0);
	if (!PyRet)
	{
		return nullptr;
	}

	if (PyType->tp_init != (initproc)FNePyDynamicStructType::Init)
	{
		if (PyType->tp_init(PyRet, nullptr, nullptr) < 0)
		{
			Py_DECREF(PyRet);
			return nullptr;
		}
	}

	FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyRet);

#if NEPY_ENABLE_STRUCT_DEEP_ACCESS
	if (InBuffer)
	{
		FNePyStructBase::SetValuePtr(PyRet, InBuffer);
		PyRet->SelfCreatedValue = false;
	}
	else
	{
		void* ValuePtr = FNePyStructBase::AllocateValuePtr(PyRet, ScriptStruct);
		check(ValuePtr);
		ScriptStruct->InitializeStruct(ValuePtr);
	}
#else
	if (InBuffer)
	{
		void* ValuePtr = FNePyStructBase::AllocateValuePtr(PyRet, ScriptStruct);
		ScriptStruct->CopyScriptStruct(ValuePtr, InBuffer);
	}
#endif

	return PyRet;
}

static PyMethodDef NePyDynamicStructType_methods[] = {
	{"Struct", NePyCFunctionCast(&FNePyDynamicStructType::Struct), METH_NOARGS | METH_CLASS, ""},
	{ NULL } /* Sentinel */
};

void FNePyDynamicStructType::InitDynamicStructType(const UScriptStruct* InScriptStruct)
{
	// 获取类型名称并拷贝保存
	TypeName = NePyGenUtil::TCHARToUTF8Buffer(*InScriptStruct->GetName());

	// 获取父类PyType
	PyTypeObject* SuperPyType = nullptr;
	const FNePyStructTypeInfo* SuperTypeInfo = nullptr;
	UScriptStruct* SuperStruct = Cast<UScriptStruct>(InScriptStruct->GetSuperStruct());
	if (SuperStruct)
	{
		SuperTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(SuperStruct);
	}
	if (!SuperTypeInfo)
	{
		SuperPyType = NePyStructBaseGetType();
	}
	else
	{
		SuperPyType = SuperTypeInfo->TypeObject;
	}

	// InitPyType
	PyTypeObject* NePyType = GetPyType();
	NePyType->tp_name = GetName();
	NePyType->tp_basicsize = FNePyStructBase::CalcPythonStructSize(InScriptStruct);
	NePyType->tp_base = SuperPyType;
	NePyType->tp_methods = NePyDynamicStructType_methods;
#if PY_MAJOR_VERSION < 3
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
	NePyType->tp_flags |= Py_TPFLAGS_BASETYPE;
	NePyType->tp_new = (newfunc)&FNePyDynamicStructType::New;
	NePyType->tp_init = (initproc)FNePyDynamicStructType::Init;

	NePyType->tp_alloc = (allocfunc)FNePyDynamicStructType::Alloc;
	NePyType->tp_dealloc = (destructor)FNePyDynamicStructType::Dealloc;

	PyType_Ready(NePyType);

	// 注册TypeInfo
	FNePyStructTypeInfo TypeInfo = {
		NePyType,
		ENePyTypeFlags::DynamicPyType,
		(NePyStructPropSet)FNePyDynamicStructType::PropSet,
		(NePyStructPropGet)FNePyDynamicStructType::PropGet,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedStructType(InScriptStruct, TypeInfo);
}

PyObject* FNePyDynamicStructType::Struct(PyObject* InSelf)
{
	const UScriptStruct* ScriptStruct = FNePyWrapperTypeRegistry::Get().GetStructByPyType((PyTypeObject*)InSelf);
	return NePyBase::ToPy(ScriptStruct);
}

void FNePyDynamicEnumType::InitDynamicEnumType(const UEnum* InEnum)
{
	// 获取类型名称并拷贝保存
	TypeName = NePyGenUtil::TCHARToUTF8Buffer(*InEnum->GetName());

	// InitPyType
	PyTypeObject* NePyType = GetPyType();
	NePyType->tp_name = GetName();
	NePyType->tp_basicsize = sizeof(FNePyEnumBase);
	NePyType->tp_base = NePyEnumBaseGetType();
#if PY_MAJOR_VERSION < 3
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
	NePyType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
	PyType_Ready(NePyType);

	FNePyEnumBase::InitEnumEntries(NePyType, InEnum);

	// 注册TypeInfo
	FNePyEnumTypeInfo TypeInfo = {
		NePyType,
		ENePyTypeFlags::DynamicPyType
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedEnumType(InEnum, TypeInfo);
}
