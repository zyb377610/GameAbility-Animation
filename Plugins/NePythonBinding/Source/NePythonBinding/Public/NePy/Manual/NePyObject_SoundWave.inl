#pragma once
#include "NePyBase.h"
#include "Sound/SoundWave.h"

PyObject* NePySoundWave_GetRawData(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetRawData'"))
	{
		return nullptr;
	}

	USoundWave* SoundWave = (USoundWave*)InSelf->Value;

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
#if WITH_EDITORONLY_DATA
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 4
	const UE::Serialization::FEditorBulkData & SoundRawData = SoundWave->RawData.RawData;
#else
	const UE::Serialization::FEditorBulkData& SoundRawData = SoundWave->RawData;
#endif
#endif
	PyObject* PyRawData = nullptr; // TODO:
	PyErr_Format(PyExc_Exception, "nepy not implement this function 'GetRawData' yet in UE 5.1");
#else
	FByteBulkData RawData = SoundWave->RawData;

	char* Data = (char*)RawData.Lock(LOCK_READ_ONLY);
	int32 DataSize = RawData.GetBulkDataSize();
	PyObject* PyRawData = PyBytes_FromStringAndSize(Data, DataSize);
	RawData.Unlock();
#endif

	return PyRawData;
}

PyObject* NePySoundWave_SetRawData(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetRawData'"))
	{
		return nullptr;
	}

	Py_buffer PySoundWaveBuffer;
	if (!PyArg_ParseTuple(InArgs, "y*:SoundSetData", &PySoundWaveBuffer))
	{
		return nullptr;
	}

	if (PySoundWaveBuffer.buf == nullptr)
	{
		PyBuffer_Release(&PySoundWaveBuffer);
		return PyErr_Format(PyExc_Exception, "Invalid SoundWave buffer.");
	}

#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
	PyBuffer_Release(&PySoundWaveBuffer);
	PyErr_Format(PyExc_Exception, "nepy not implement this function 'SetRawData' yet in UE 5.1");
#else
	USoundWave* SoundWave = (USoundWave*)InSelf->Value;
	SoundWave->FreeResources();
	SoundWave->InvalidateCompressedData();

	SoundWave->RawData.Lock(LOCK_READ_WRITE);
	void* data = SoundWave->RawData.Realloc(PySoundWaveBuffer.len);
	FMemory::Memcpy(data, PySoundWaveBuffer.buf, PySoundWaveBuffer.len);
	SoundWave->RawData.Unlock();

	PyBuffer_Release(&PySoundWaveBuffer);
#endif

	Py_RETURN_NONE;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetRawData", NePyCFunctionCast(&NePySoundWave_GetRawData), METH_NOARGS, "(self) -> bytes"}, \
{"SetRawData", NePyCFunctionCast(&NePySoundWave_SetRawData), METH_VARARGS, "(self, Buffer: bytes) -> None"}, \
