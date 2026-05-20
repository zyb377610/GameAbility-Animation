#include "NePyObjectHolder.h"
#include "NePyGIL.h"

FNePyObjectHolder::FNePyObjectHolder()
	: Value(nullptr)
{
}

FNePyObjectHolder::FNePyObjectHolder(PyObject* InPyObj)
	: Value(InPyObj)
{
	Py_XINCREF(Value);
}

FNePyObjectHolder::FNePyObjectHolder(const FNePyObjectHolder& InOther)
	: Value(InOther.Value)
{
	if (Value)
	{
		FNePyScopedGIL GIL;
		Py_INCREF(Value);
	}
}

FNePyObjectHolder::FNePyObjectHolder(FNePyObjectHolder&& InOther)
	: Value(InOther.Value)
{
	InOther.Value = nullptr;
}

FNePyObjectHolder& FNePyObjectHolder::operator=(const FNePyObjectHolder& InOther)
{
	if (this != &InOther)
	{
		FNePyScopedGIL GIL;
		Py_XDECREF(this->Value);
		this->Value = InOther.Value;
		Py_XINCREF(this->Value);
	}
	return *this;
}

FNePyObjectHolder& FNePyObjectHolder::operator=(FNePyObjectHolder&& InOther)
{
	if (this != &InOther)
	{
		if (this->Value)
		{
			FNePyScopedGIL GIL;
			Py_DECREF(this->Value);
		}
		this->Value = InOther.Value;
		InOther.Value = nullptr;
	}
	return *this;
}

FNePyObjectHolder::~FNePyObjectHolder()
{
	if (Value)
	{
		if (Py_IsInitialized())
		{
			FNePyScopedGIL GIL;
			Py_DECREF(Value);
		}
		Value = nullptr;
	}
}
