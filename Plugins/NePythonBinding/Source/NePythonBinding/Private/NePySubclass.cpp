#include "NePySubclass.h"
#include "NePySubclassing.h"
#include "NePyHouseKeeper.h"
#include "NePyMemoryAllocator.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyGIL.h"
#include "NePythonBinding.h"
#include "NePyUtil.h"
#include "NePyGenUtil.h"
#include "Misc/ScopeExit.h"
#include "Misc/AssertionMacros.h"

// -----------------------------------------
// UNePySubclass

void UNePySubclass::PySubclassConstructor(const FObjectInitializer& ObjectInitializer)
{
	// 从继承链上寻找UNePySubclass，以及Object的基类
	// 假如有以下继承链：
	//   AActor <- MyActor（Python） <- MyActorBP_C（蓝图）
	// 那么：
	//   ObjectClass 将会是 MyActorBP_C
	//   Class 将会是 MyActor
	//   NativeSuperClass 将会是 AActor
	UClass* ObjectClass = ObjectInitializer.GetClass();
	check(ObjectClass->ClassConstructor == &UNePySubclass::PySubclassConstructor);
	UNePySubclass* Class = Cast<UNePySubclass>(ObjectClass);
	UClass* NativeSuperClass = ObjectClass->GetSuperClass();
	while (NativeSuperClass->ClassConstructor == &UNePySubclass::PySubclassConstructor)
	{
		if (!Class)
		{
			Class = Cast<UNePySubclass>(NativeSuperClass);
		}
		NativeSuperClass = NativeSuperClass->GetSuperClass();
	}
	check(Class);
	check(Class->PyClass);
	NativeSuperClass->ClassConstructor(ObjectInitializer);

	// 调用__init__
	UObject* Object = ObjectInitializer.GetObj();
	if (!Object->HasAnyFlags(RF_ClassDefaultObject))
	{
		FNePyScopedGIL GIL;

		FNePyObjectPtr PyObj = NePyStealReference(FNePyHouseKeeper::Get().NewNePyObject(Object));
		//check(strcmp(PyObj->ob_type->tp_name, "PySubclassInstance") == 0);

		// 实现继承，使用第一个定义了__init__的父类的函数
		while (Class)
		{
			PyObject* PyInitFunc = PyDict_GetItemString(Class->PyClass->PyDict, "__init__");
			if (PyInitFunc)
			{
				check(PyFunction_Check(PyInitFunc));
				FNePyObjectPtr PyArgs = NePyStealReference((Py_BuildValue("(O)", PyObj.Get())));
				FNePyObjectPtr PyRet = NePyStealReference(PyObject_CallObject(PyInitFunc, PyArgs));
				if (!PyRet)
				{
					PyErr_Print();
				}
				break;
			}
			Class = Cast<UNePySubclass>(Class->GetSuperClass());
		}
	}
}

// 参考UPythonGeneratedClass::CallPythonFunction
DEFINE_FUNCTION(UNePySubclass::CallPythonFunction)
{
	bool bHasError = false;
	const UFunction* Func = Stack.CurrentNativeFunction;
	UNePySubclass* Class = CastChecked<UNePySubclass>(Func->GetOwnerClass());
	FNePySubclass* PyClass = Class->PyClass;
	if (!PyClass)
	{
		UE_LOG(LogNePython, Error, TEXT("Failed to call Python function for %s.%s, python class is invalid."), *Class->GetName(), *Func->GetName());
		bHasError = true;
	}

	{
		FNePyScopedGIL GIL;

		PyObject* PyFunc = nullptr;
		if (PyClass)
		{
			PyFunc = PyDict_GetItemString(PyClass->PyDict, TCHAR_TO_ANSI(*Func->GetName()));
			if (!PyFunc)
			{
				UE_LOG(LogNePython, Error, TEXT("Failed to call Python function for %s.%s, python function not found."), *Class->GetName(), *Func->GetName());
				bHasError = true;
			}
				
		}

		bool bStaticMethod = Func->HasAnyFunctionFlags(FUNC_Static);
		// 如果C++ Funciton为static，那么python对应的方法也应该是PyStaticMethod_Type
		check(bStaticMethod == (Py_TYPE(PyFunc) == &PyStaticMethod_Type));
		if (bStaticMethod)
		{
			PyFunc = PyObject_GetAttrString(PyFunc, "__func__");
		}

		FNePyObjectPtr PySelf;
		if (!bStaticMethod)
		{
			PySelf = NePyStealReference(FNePyHouseKeeper::Get().NewNePyObject(P_THIS_OBJECT));
		}

		if (!NePyGenUtil::InvokePythonCallableFromUnrealFunctionThunk(PySelf, PyFunc, Func, Context, Stack, RESULT_PARAM) || bHasError)
		{
			PyErr_Print();
		}

		if (bStaticMethod)
		{
			Py_XDECREF(PyFunc);
		}
	}
}

// -----------------------------------------
// FNePySubclass

static PyTypeObject FNePySubclassType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"PySubclass", /* tp_name */
	sizeof(FNePySubclass), /* tp_basicsize */
};

// tp_new
PyObject* NePySubclass_New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	if (PyTuple_Size(InArgs) != 3)
	{
		// 不是subclassing
		return PyType_GenericNew(InType, InArgs, InKwds);
	}

	return NePySubclassing(InType, InArgs, InKwds);
}

// tp_dealloc
void NePySubclass_Dealloc(FNePySubclass* InSelf)
{
	if (FNePythonModuleDelegates::IsModuleInited())
	{
		// 此类型永远不应该走到Dealloc
		// 如果走到了，那就是引用计数出错了
		// check(false);  // reload的时候还是有可能走到
	}
}

// tp_repr
static PyObject* NePySubclass_Repr(FNePyClass* InSelf)
{
	// PySubclass一旦创建用不释放
	check(FNePyHouseKeeper::Get().IsValid(InSelf));
	return PyUnicode_FromFormat("<PySubclass '%s' at %p>", TCHAR_TO_UTF8(*InSelf->GetValue()->GetName()), InSelf->GetValue());
}

void NePyInitSubclass(PyObject* PyOuterModule)
{
	PyTypeObject* PyType = &FNePySubclassType;
	// 检查下PyTypeObject的引用计数，以保证在注册此类型前，没有被子类意外地提前引用，造成污染
	checkf(PyType && (Py_REFCNT(PyType) == 1), TEXT("NePyObjectType_InitCommon PyTypeObject is nullptr or ref_count isnot equal 1."));
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = NePySubclass_New;
	PyType->tp_dealloc = (destructor)&NePySubclass_Dealloc;
	PyType->tp_base = NePyClassGetType();
	PyType->tp_repr = (reprfunc)&NePySubclass_Repr;
	PyType->tp_str = (reprfunc)&NePySubclass_Repr;
	PyType->tp_dictoffset = offsetof(FNePySubclass, PyDict);
	PyType_Ready(PyType);

	// 以下接口不需要继承自父类（NePyObjectBase），直接置空或覆盖
	PyType->tp_init = nullptr;
	//PyType->tp_getattro = PyObject_GenericGetAttr;
	//PyType->tp_setattro = nullptr;
}

PyTypeObject* NePySubclassGetType()
{
	return &FNePySubclassType;
}

FNePySubclass* NePySubclassNewInternalUseOnly(UNePySubclass* Class, UClass* SuperClass)
{
	//if (!NePyNewSubclassInstanceType(Class, SuperClass))
	//{
	//	return nullptr;
	//}

	// 创建新PyClass并与UClass进行绑定
	FNePySubclass* PyClass = (FNePySubclass*)FNePySubclassType.tp_alloc(&FNePySubclassType, 0);
	FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyClass);
	PyClass->PyDict = PyDict_New();
	PyDict_SetItemString(PyClass->PyDict, "__name__", NePyStealReference(NePyString_FromString(TCHAR_TO_ANSI(*Class->GetName()))));
	PyDict_SetItemString(PyClass->PyDict, "__dict__", PyClass->PyDict);
	check(!Class->PyClass);
	Class->PyClass = PyClass;
	check(!PyClass->Value);
	PyClass->Value = Class;
	FNePyHouseKeeper::Get().AddNePyObjectInternalUseOnly(PyClass);

	return PyClass;
}

// -----------------------------------------
// FNePySubclassInstance

void NePyObject_Dealloc(FNePyObjectBase* InSelf);

// tp_dealloc
void NePySubclassInstance_Dealloc(FNePySubclassInstance* InSelf)
{
	if (InSelf->PyDict)
	{
		PyDict_Clear(InSelf->PyDict); // __dict__.clear()解除循环引用
		Py_DECREF(InSelf->PyDict);
		InSelf->PyDict = nullptr;
	}

	NePyObject_Dealloc(InSelf);
}

PyTypeObject* NePyNewSubclassInstanceType(UNePySubclass* Class, UClass* SuperClass, PyObject* InDict)
{
	PyTypeObject* PySuperType;
	auto TypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedClassType(SuperClass);
	if (TypeInfo)
	{
		PySuperType = TypeInfo->TypeObject;
	}
	else
	{
		PySuperType = NePyObjectBaseGetType();
	}

	PyObject* PyName = NePyString_FromFormat("%s", TCHAR_TO_ANSI(*Class->GetName()));
	PyObject* PyParents = PyTuple_New(1);
	Py_INCREF(PySuperType);
	PyTuple_SetItem(PyParents, 0, (PyObject*)PySuperType);

	FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(3));
	PyTuple_SetItem(PyArgs, 0, (PyObject*)PyName);
	PyTuple_SetItem(PyArgs, 1, (PyObject*)PyParents);
	Py_INCREF(InDict);
	PyTuple_SetItem(PyArgs, 2, (PyObject*)InDict);

	PyTypeObject* NewPyType = (PyTypeObject*)PyType_Type.tp_new(Py_TYPE(PySuperType), PyArgs, nullptr);

	if (!NewPyType)
	{
		return nullptr;
	}

	// 向HouseKeeper注册新的Python类型
	FNePyObjectTypeInfo NewTypeInfo = {
		NewPyType,
		ENePyTypeFlags::ScriptPyType,
		(NePyObjectNewFunc)NePySubclassNew,
	};
	FNePyWrapperTypeRegistry::Get().RegisterWrappedClassType(Class, NewTypeInfo);
	return NewPyType;
}

FNePyObjectBase* NePySubclassNew(UObject* InObject, PyTypeObject* InPyType)
{
	//FNePyObjectBase* RetValue = PyObject_New(FNePyObjectBase, PyType);
	FNePyObjectPtr PyArgs = NePyStealReference(PyTuple_New(0));
	FNePyObjectBase* RetValue = (FNePyObjectBase*)PyBaseObject_Type.tp_new(InPyType, PyArgs, nullptr);
	RetValue->Value = InObject;
	return RetValue;
}
