#pragma once
#include "NePyHouseKeeper.h"
#include "NePyMemoryAllocator.h"
#include "CoreMinimal.h"
#include "HAL/UnrealMemory.h"

// UStruct py base class
struct NEPYTHONBINDING_API FNePyStructBase : public FNePyPropObject
{
	// 对父结构的反向引用
	// UStruct不使用HouseKeeper管理生命周期。通过增加对父结构体的引用计数，防止父结构体被释放。
	PyObject* PyOuter;
	// 是否是非引用值
	bool SelfCreatedValue;

	// 获取PyType对应的UScriptStruct
	static const UScriptStruct* GetScriptStruct(FNePyStructBase* InSelf);

	// 设置子类成员变量“Value”的值
	static void SetValuePtr(FNePyStructBase* InSelf, void* Value);

	// 分配空间给“Value”并且设置“SelfCreateValue”为true
	static void* AllocateValuePtr(FNePyStructBase* InSelf, const UScriptStruct* InScriptStruct);

	// 释放“Value”的空间并且设置“SelfCreateValue”为false
	static void FreeValuePtr(FNePyStructBase* InSelf);

	// 克隆
	static FNePyStructBase* Clone(FNePyStructBase* InSelf);

	// 计算类结构体大小（包含PyObject部分）
	static int32 Size();
	static int32 CalcPythonStructSize(const UScriptStruct* InScriptStruct);

	// 根据包名和类名，查找并返回UScriptStruct
	static const UScriptStruct* SearchScriptStruct(const TCHAR* InPackageName, const TCHAR* InStructName);
};

void NePyInitStructBase(PyObject* PyOuterModule);
NEPYTHONBINDING_API FNePyStructBase* NePyStructBaseCheck(PyObject* InPyObj);
NEPYTHONBINDING_API PyTypeObject* NePyStructBaseGetType();


template<typename TCppType>
struct TNePyStructBase : public FNePyStructBase
{
	// common init for PyTypeObject which include a TNePyStructBase class
	static void InitTypeCommon(PyTypeObject* InPyType)
	{
#if PY_MAJOR_VERSION < 3
		InPyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES;
#else
		InPyType->tp_flags = Py_TPFLAGS_DEFAULT;
#endif
		InPyType->tp_flags |= Py_TPFLAGS_BASETYPE;
		
		InPyType->tp_new = PyType_GenericNew;
		InPyType->tp_init = (initproc)&TNePyStructBase<TCppType>::Init;
		InPyType->tp_repr = (reprfunc)&TNePyStructBase<TCppType>::Repr;
		InPyType->tp_str = (reprfunc)&TNePyStructBase<TCppType>::Repr;
		InPyType->tp_alloc = TNePyStructBase<TCppType>::Alloc;
		InPyType->tp_dealloc = (destructor)&TNePyStructBase<TCppType>::Dealloc;
		if (TNePyStructBase<TCppType>::HasHash)
		{
			InPyType->tp_hash = (hashfunc)&TNePyStructBase<TCppType>::Hash;
		}
	}

	static int Init(TNePyStructBase<TCppType>* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
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

		InSelf->Value = FMemory::Malloc(sizeof(TCppType), alignof(TCppType));
		FMemory::Memset(InSelf->Value, '\0', sizeof(TCppType));
		InSelf->SelfCreatedValue = true;

		InSelf->PyOuter = nullptr;

		_CppConstruct<TCppType>(InSelf);
		return 0;
	}

	static PyObject* Alloc(PyTypeObject* PyType, Py_ssize_t NItems)
	{
		TNePyStructBase<TCppType>* PyObj = (TNePyStructBase<TCppType>*)PyType_GenericAlloc(PyType, NItems);
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(PyObj);
		return PyObj;
	}

	static void Dealloc(TNePyStructBase<TCppType>* InSelf)
	{
		if (InSelf->SelfCreatedValue)
		{
			_CppDestruct<TCppType>(InSelf);
			FMemory::Free(InSelf->Value);
			InSelf->Value = nullptr;
			InSelf->SelfCreatedValue = false;
		}
		if (InSelf->PyOuter)
		{
			Py_DECREF(InSelf->PyOuter);
			InSelf->PyOuter = nullptr;
		}
		InSelf->ob_type->tp_free(InSelf);
	}

	static PyObject* Repr(TNePyStructBase<TCppType>* InSelf)
	{
		return _Repr<TCppType>(InSelf);
	}

	static FNePyHashType Hash(TNePyStructBase<TCppType>* InSelf)
	{
		return _Hash<TCppType>(InSelf);
	}

private:
	// 只匹配无参的ToString实现
	template<typename TCppCls> static uint8 ResolvedString(decltype(std::declval<TCppCls>().ToString()) (TCppCls::*)());
	template<typename TCppCls> static uint16 ResolvedString(...);

	template <typename TCppCls> static uint8 ResolvedHash(decltype(GetTypeHash(std::declval<TCppCls>())));
	template<typename TCppCls> static uint16 ResolvedHash(...);

	enum
	{
		HasToString = sizeof(ResolvedString<TCppType>(nullptr)) == sizeof(uint8),
		HasHash = sizeof(ResolvedHash<TCppType>(nullptr)) == sizeof(uint8),
		HasNonTrivialConstructible = !std::is_trivially_default_constructible<TCppType>::value,
		HasNonTrivialDestructible = !std::is_trivially_destructible<TCppType>::value
	};

	template<typename TCppCls>
	static typename TEnableIf<TNePyStructBase<TCppCls>::HasNonTrivialConstructible, void>::Type _CppConstruct(TNePyStructBase<TCppCls>* InSelf)
	{
		new (InSelf->Value) TCppCls();
	}

	template<typename TCppCls>
	static typename TEnableIf<!TNePyStructBase<TCppCls>::HasNonTrivialConstructible, void>::Type _CppConstruct(TNePyStructBase<TCppCls>* InSelf)
	{
	}

	template<typename TCppCls>
	static typename TEnableIf<TNePyStructBase<TCppCls>::HasNonTrivialDestructible, void>::Type _CppDestruct(TNePyStructBase<TCppCls>* InSelf)
	{
		((TCppType*)InSelf->Value)->~TCppCls();
	}

	template<typename TCppCls>
	static typename TEnableIf<!TNePyStructBase<TCppCls>::HasNonTrivialDestructible, void>::Type _CppDestruct(TNePyStructBase<TCppCls>* InSelf)
	{
	}

	template<typename TCppCls>
	static typename TEnableIf<TNePyStructBase<TCppCls>::HasToString, PyObject>::Type* _Repr(TNePyStructBase<TCppCls>* InSelf)
	{
		return PyUnicode_FromFormat("<%s {%s}>", InSelf->ob_type->tp_name, TCHAR_TO_UTF8(*((TCppCls*)InSelf->Value)->ToString()));
	}

	template<typename TCppCls>
	static typename TEnableIf<!TNePyStructBase<TCppCls>::HasToString, PyObject>::Type* _Repr(TNePyStructBase<TCppCls>* InSelf)
	{
		return PyUnicode_FromFormat("<%s at %p>", InSelf->ob_type->tp_name, InSelf);
	}

	template<typename TCppCls>
	static typename TEnableIf<TNePyStructBase<TCppCls>::HasHash, FNePyHashType>::Type _Hash(TNePyStructBase<TCppCls>* InSelf)
	{
		return GetTypeHash(InSelf->Value);
	}

	template<typename TCppCls>
	static typename TEnableIf<!TNePyStructBase<TCppCls>::HasHash, FNePyHashType>::Type _Hash(TNePyStructBase<TCppCls>* InSelf)
	{
		check(false);
		return 0;
	}
};