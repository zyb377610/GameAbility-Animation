#include "NePyClass.h"
#include "NePySubclassing.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyHouseKeeper.h"
#include "NePyMemoryAllocator.h"

#if ENGINE_MAJOR_VERSION >= 5
#define CLASS_GENERATED_BY_WITH_EDITORONLY_DATA WITH_EDITORONLY_DATA
#else
#define CLASS_GENERATED_BY_WITH_EDITORONLY_DATA 1
#endif

static PyTypeObject FNePyClassType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Class", /* tp_name */
	sizeof(FNePyClass), /* tp_basicsize */
};

// tp_new
static PyObject* NePyClass_New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	if (PyTuple_Size(InArgs) != 3)
	{
		// 不是subclassing
		return PyType_GenericNew(InType, InArgs, InKwds);
	}

	return NePySubclassing(InType, InArgs, InKwds);
}

// tp_repr
static PyObject* NePyClass_Repr(FNePyClass* InSelf)
{
	if (!FNePyHouseKeeper::Get().IsValid(InSelf))
	{
		return PyUnicode_FromFormat("<Invalid UClass at %p>", InSelf->Value);
	}
	return PyUnicode_FromFormat("<Class '%s' at %p>", TCHAR_TO_UTF8(*InSelf->GetValue()->GetName()), InSelf->GetValue());
}

#if CLASS_GENERATED_BY_WITH_EDITORONLY_DATA
static PyObject* NePyClass_ClassGeneratedBy(FNePyClass* InSelf)
{
	UObject* Object = InSelf->GetValue()->ClassGeneratedBy;
	if (!Object)
	{
		Py_RETURN_NONE;
	}

	return NePyBase::ToPy(Object);
}
#endif // CLASS_GENERATED_BY_WITH_EDITORONLY_DATA

static PyObject* NePyClass_GetClassFlags(FNePyClass* InSelf)
{
	return PyLong_FromUnsignedLongLong((uint64)InSelf->GetValue()->GetClassFlags());
}

static PyObject* NePyClass_SetClassFlags(FNePyClass* InSelf, PyObject* InArgs)
{
	uint64 Flags;
	if (!PyArg_ParseTuple(InArgs, "K:SetClassFlags", &Flags))
	{
		return nullptr;
	}

	InSelf->GetValue()->ClassFlags = (EClassFlags)Flags;
	Py_RETURN_NONE;
}

static PyObject* NePyClass_GetSuperClass(FNePyClass* InSelf)
{
	UClass* SuperClass = InSelf->GetValue()->GetSuperClass();
	return NePyBase::ToPy(SuperClass);
}

static PyObject* NePyClass_IsChildOf(FNePyClass* InSelf, PyObject* InArg)
{
	UStruct* Struct = NePyBase::ToCppStruct(InArg);
	if (!Struct)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UStruct");
	}

	if (InSelf->GetValue()->IsChildOf(Struct))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyObject* NePyClass_ImplementsInterface(FNePyClass* InSelf, PyObject* InArg)
{
	UClass* Class = NePyBase::ToCppClass(InArg);
	if (!Class)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a UClass");
	}

	if (InSelf->GetValue()->ImplementsInterface(Class))
	{
		Py_RETURN_TRUE;
	}

	Py_RETURN_FALSE;
}

static PyObject* NePyClass_GetDefaultObject(FNePyClass* InSelf)
{
	return NePyBase::ToPy(InSelf->GetValue()->GetDefaultObject());
}

static PyObject* NePyClass_Class(PyObject* InSelf) {
	UClass* Class = UClass::StaticClass();
	return NePyBase::ToPy(Class);
}

static PyMethodDef FNePyClassType_methods[] = {
#if CLASS_GENERATED_BY_WITH_EDITORONLY_DATA
	{"ClassGeneratedBy", NePyCFunctionCast(&NePyClass_ClassGeneratedBy), METH_NOARGS, "(self) -> Object"},
#endif // CLASS_GENERATED_BY_WITH_EDITORONLY_DATA
	{"GetClassFlags", NePyCFunctionCast(&NePyClass_GetClassFlags), METH_NOARGS, "(self) -> int"},
	{"SetClassFlags", NePyCFunctionCast(&NePyClass_SetClassFlags), METH_VARARGS, "(self, InClassFlags: int) -> None"},
	{"GetSuperClass", NePyCFunctionCast(&NePyClass_GetSuperClass), METH_NOARGS, "(self) -> Class"},
	{"IsChildOf", NePyCFunctionCast(&NePyClass_IsChildOf), METH_O, "(self, InClass: type[T]) -> typing.TypeGuard[type[T]];(self, InClass: TSubclassOf[T]) -> typing.TypeGuard[TSubclassOf[T]];(self, InClass: Class) -> bool"},
	{"ImplementsInterface", NePyCFunctionCast(&NePyClass_ImplementsInterface), METH_O, "[T: Interface](self, InClass: type[T]) -> typing.TypeGuard[type[T]];[T: Interface](self, InClass: TSubclassOf[T]) -> typing.TypeGuard[TSubclassOf[T]];(self, InClass: Class) -> bool"},
	{"GetDefaultObject", NePyCFunctionCast(&NePyClass_GetDefaultObject), METH_NOARGS, "(self) -> Object"},
	{"Class", NePyCFunctionCast(&NePyClass_Class), METH_NOARGS | METH_STATIC, "() -> Class"},
	{ NULL } /* Sentinel */
};

FNePyClass* NePyClassNew(UClass* InValue, PyTypeObject* InPyType)
{
	check(PyType_IsSubtype(InPyType, &FNePyClassType));
	FNePyClass* RetValue = (FNePyClass*)PyType_GenericAlloc(InPyType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(RetValue);
	RetValue->Value = InValue;
	return RetValue;
}

FNePyClass* NePyClassCheck(const PyObject* InPyObj)
{
	if (InPyObj && PyObject_TypeCheck(InPyObj, &FNePyClassType))
	{
		return (FNePyClass*)InPyObj;
	}
	return nullptr;
}

PyTypeObject* NePyClassGetType()
{
	return &FNePyClassType;
}

void NePyInitClass(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePyClassType;
	NePyObjectType_InitCommon(PyType);
	PyType->tp_new = NePyClass_New;
	PyType->tp_methods = FNePyClassType_methods;
	PyType->tp_base = NePyObjectBaseGetType();
	PyType->tp_repr = (reprfunc)&NePyClass_Repr;
	PyType->tp_str = (reprfunc)&NePyClass_Repr;
	PyType->tp_flags &= ~Py_TPFLAGS_BASETYPE;
	PyType_Ready(PyType);

	PyModule_AddObject(PyOuterModule, "Class", (PyObject*)PyType);

	FNePyObjectTypeInfo TypeInfo = {
		PyType,
		ENePyTypeFlags::StaticPyType,
		(NePyObjectNewFunc)NePyClassNew,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(UClass::StaticClass(), TypeInfo);
}

#undef CLASS_GENERATED_BY_WITH_EDITORONLY_DATA

