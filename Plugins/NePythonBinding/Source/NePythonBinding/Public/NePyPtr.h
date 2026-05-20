// Copyright Epic Games, Inc. All Rights Reserved.
// 这个文件是ue_4.25\Engine\Plugins\Experimental\PythonScriptPlugin\Source\PythonScriptPlugin\Private\PyPtr.h拷贝过来的
#pragma once

#include "NePyIncludePython.h"
#include "Misc/AssertionMacros.h"
#include "NePyGIL.h"

/** Wrapper that handles the ref-counting of the contained Python object */
template <typename TPythonType, bool TNeedGIL>
class TNePyPtr
{
public:
	/** Create a null pointer */
	TNePyPtr()
		: Ptr(nullptr)
	{
	}

	/** Copy a pointer */
	TNePyPtr(const TNePyPtr& InOther)
		: Ptr(InOther.Ptr)
	{
		if (TNeedGIL)
		{
			FNePyScopedGIL GIL;
			Py_XINCREF(Ptr);
		}
		else
		{
			Py_XINCREF(Ptr);
		}
	}

	/** Move a pointer */
	TNePyPtr(TNePyPtr&& InOther)
		: Ptr(InOther.Ptr)
	{
		InOther.Ptr = nullptr;
	}

	/** Release our reference to this object */
	~TNePyPtr()
	{
		if (TNeedGIL)
		{
			FNePyScopedGIL GIL;
			if (GIL.IsHoldGIL())
			{
				Py_XDECREF(Ptr);
			}
		}
		else
		{
			Py_XDECREF(Ptr);
		}
	}

	/** Create a pointer from the given object, incrementing its reference count */
	static TNePyPtr NewReference(TPythonType* InPtr)
	{
		return TNePyPtr(InPtr, true);
	}

	/** Create a pointer from the given object, leaving its reference count alone */
	static TNePyPtr StealReference(TPythonType* InPtr)
	{
		return TNePyPtr(InPtr, false);
	}

	/** Copy a pointer */
	TNePyPtr& operator=(const TNePyPtr& InOther)
	{
		if (this != &InOther)
		{
			if (TNeedGIL)
			{
				FNePyScopedGIL GIL;
				Py_XDECREF(Ptr);
				Ptr = InOther.Ptr;
				Py_XINCREF(Ptr);
			}
			else
			{
				Py_XDECREF(Ptr);
				Ptr = InOther.Ptr;
				Py_XINCREF(Ptr);
			}
		}
		return *this;
	}

	/** Move a pointer */
	TNePyPtr& operator=(TNePyPtr&& InOther)
	{
		if (this != &InOther)
		{
			if (TNeedGIL)
			{
				FNePyScopedGIL GIL;
				Py_XDECREF(Ptr);
			}
			else
			{
				Py_XDECREF(Ptr);
			}
			Ptr = InOther.Ptr;
			InOther.Ptr = nullptr;
		}
		return *this;
	}

	explicit operator bool() const
	{
		return Ptr != nullptr;
	}

	bool IsValid() const
	{
		return Ptr != nullptr;
	}

	operator TPythonType*() const
	{
		return Ptr;
	}

	TPythonType& operator*()
	{
		check(Ptr);
		return *Ptr;
	}

	const TPythonType& operator*() const
	{
		check(Ptr);
		return *Ptr;
	}

	TPythonType* operator->()
	{
		check(Ptr);
		return Ptr;
	}

	const TPythonType* operator->() const
	{
		check(Ptr);
		return Ptr;
	}

	TPythonType*& Get()
	{
		return Ptr;
	}

	TPythonType* GetPtr() const
	{
		return Ptr;
	}

	TPythonType* Release()
	{
		TPythonType* RetPtr = Ptr;
		Ptr = nullptr;
		return RetPtr;
	}

	void Reset()
	{
		if (TNeedGIL)
		{
			FNePyScopedGIL GIL;
			if (GIL.IsHoldGIL())
			{
				Py_XDECREF(Ptr);
			}
		}
		else
		{
			Py_XDECREF(Ptr);
		}
		Ptr = nullptr;
	}

private:
	TNePyPtr(TPythonType* InPtr, const bool bIncRef)
		: Ptr(InPtr)
	{
		if (bIncRef)
		{
			if (TNeedGIL)
			{
				FNePyScopedGIL GIL;
				Py_XINCREF(Ptr);
			}
			else
			{
				Py_XINCREF(Ptr);
			}
		}
	}

	TPythonType* Ptr;
};

typedef TNePyPtr<PyObject, false> FNePyObjectPtr;
typedef TNePyPtr<PyObject, true> FNePyObjectPtrWithGIL;
typedef TNePyPtr<PyTypeObject, false> FNePyTypeObjectPtr;
typedef TNePyPtr<PyTypeObject, true> FNePyTypeObjectPtrWithGIL;

static inline FNePyObjectPtr NePyNewReference(PyObject* PyObj)
{
	return FNePyObjectPtr::NewReference(PyObj);
}

static inline FNePyObjectPtr NePyStealReference(PyObject* PyObj)
{
	return FNePyObjectPtr::StealReference(PyObj);
}

static inline FNePyObjectPtrWithGIL NePyNewReferenceWithGIL(PyObject* PyObj)
{
	return FNePyObjectPtrWithGIL::NewReference(PyObj);
}

static inline FNePyObjectPtrWithGIL NePyStealReferenceWithGIL(PyObject* PyObj)
{
	return FNePyObjectPtrWithGIL::StealReference(PyObj);
}

static inline FNePyTypeObjectPtr NePyNewReference(PyTypeObject* PyTypeObj)
{
	return FNePyTypeObjectPtr::NewReference(PyTypeObj);
}

static inline FNePyTypeObjectPtr NePyStealReference(PyTypeObject* PyTypeObj)
{
	return FNePyTypeObjectPtr::StealReference(PyTypeObj);
}

static inline FNePyTypeObjectPtrWithGIL NePyNewReferenceWithGIL(PyTypeObject* PyTypeObj)
{
	return FNePyTypeObjectPtrWithGIL::NewReference(PyTypeObj);
}

static inline FNePyTypeObjectPtrWithGIL NePyStealReferenceWithGIL(PyTypeObject* PyTypeObj)
{
	return FNePyTypeObjectPtrWithGIL::StealReference(PyTypeObj);
}
