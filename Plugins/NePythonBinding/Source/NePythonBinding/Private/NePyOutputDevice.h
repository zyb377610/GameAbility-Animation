#pragma once

#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "NePyIncludePython.h"
#include "NePyGIL.h"

class FNeOutputDevice : FOutputDevice
{

public:
	FNeOutputDevice(PyObject* InPyCallable)
	{
		this->SetPySerialize(InPyCallable);
		GLog->AddOutputDevice(this);
		GLog->SerializeBacklog(this);
	}

	~FNeOutputDevice()
	{
		if (GLog)
		{
			GLog->RemoveOutputDevice(this);
		}
		Py_XDECREF(PySerialize);
	}

	void SetPySerialize(PyObject* InPyCallable)
	{
		PySerialize = InPyCallable;
		Py_INCREF(PySerialize);
	}

protected:
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const class FName& Category) override
	{
		if (!PySerialize)
			return;
		FNePyScopedGIL gil;
		PyObject* ret = PyObject_CallFunction(PySerialize, (char*)"sis", TCHAR_TO_UTF8(V), Verbosity, TCHAR_TO_UTF8(*Category.ToString()));
		if (!ret)
		{
			PyErr_Print();
		}
		Py_XDECREF(ret);
	}

private:
	PyObject* PySerialize;
};

struct FNePyOutputDevice : public PyObject
{
	FNeOutputDevice* Device;
};

void NePyInitOutputDevice(PyObject* PyOuterModule);
