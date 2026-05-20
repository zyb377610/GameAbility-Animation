#pragma once
#include "NePyBase.h"
#include "Sound/SoundWaveProcedural.h"

PyObject* NePySoundWaveProcedural_QueueAudio(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	// Writes from a Python buffer object to a USoundWaveProcedural class
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'QueueAudio'"))
	{
		return nullptr;
	}

	Py_buffer PySoundWaveBuffer;
	if (!PyArg_ParseTuple(InArgs, "y*:QueueAudio", &PySoundWaveBuffer))
	{
		return nullptr;
	}

	// Convert the buffer
	uint8* Buffer = (uint8*)PySoundWaveBuffer.buf;
	if (Buffer == nullptr)
	{
		// Clean up
		PyBuffer_Release(&PySoundWaveBuffer);
		return PyErr_Format(PyExc_Exception, "Invalid SoundWave buffer.");
	}

	// Add the audio to the Sound Wave's audio buffer
	USoundWaveProcedural* SoundWaveProcedural = (USoundWaveProcedural*)InSelf->Value;
	SoundWaveProcedural->QueueAudio(Buffer, PySoundWaveBuffer.len);

	// Clean up
	PyBuffer_Release(&PySoundWaveBuffer);

	Py_RETURN_NONE;
}

PyObject* NePySoundWaveProcedural_GetAvailableAudioByteCount(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetAvailableAudioByteCount'"))
	{
		return nullptr;
	}

	USoundWaveProcedural* SoundWaveProcedural = (USoundWaveProcedural*)InSelf->Value;

	return PyLong_FromLong(SoundWaveProcedural->GetAvailableAudioByteCount());
}

PyObject* NePySoundWaveProcedural_ResetAudio(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'ResetAudio'"))
	{
		return nullptr;
	}

	USoundWaveProcedural* SoundWaveProcedural = (USoundWaveProcedural*)InSelf->Value;
	SoundWaveProcedural->ResetAudio();

	Py_RETURN_NONE;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"QueueAudio", NePyCFunctionCast(&NePySoundWaveProcedural_QueueAudio), METH_VARARGS, "(self, Buffer: bytes) -> None"}, \
{"GetAvailableAudioByteCount", NePyCFunctionCast(&NePySoundWaveProcedural_GetAvailableAudioByteCount), METH_NOARGS, "(self) -> int"}, \
{"ResetAudio", NePyCFunctionCast(&NePySoundWaveProcedural_ResetAudio), METH_NOARGS, "(self) -> None"}, \
