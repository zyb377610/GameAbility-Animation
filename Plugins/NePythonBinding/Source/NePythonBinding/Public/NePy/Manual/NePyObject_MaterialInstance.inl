#pragma once
#include "NePyBase.h"
#include "NePyStruct_LinearColor.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstance.h"

PyObject* NePyMaterialInstance_GetScalarParameterValue(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetScalarParameterValue'"))
	{
		return nullptr;
	}

	char* ParamName = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s:GetScalarParameterValue", &ParamName))
	{
		return nullptr;
	}

	UMaterialInstance* MaterialInstance = (UMaterialInstance*)InSelf->Value;

	float Value = 0;
	MaterialInstance->GetScalarParameterValue(UTF8_TO_TCHAR(ParamName), Value);

	return PyFloat_FromDouble(Value);
}

PyObject* NePyMaterialInstance_GetVectorParameterValue(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetVectorParameterValue'"))
	{
		return nullptr;
	}

	char* ParamName = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s:GetVectorParameterValue", &ParamName))
	{
		return nullptr;
	}

	if (!InSelf->Value->IsA<UMaterialInstance>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstance");
	}

	UMaterialInstance* MaterialInstance = (UMaterialInstance*)InSelf->Value;

	FLinearColor Value(0, 0, 0);
	MaterialInstance->GetVectorParameterValue(UTF8_TO_TCHAR(ParamName), Value);

	return NePyStructNew_LinearColor(Value);
}

PyObject* NePyMaterialInstance_GetTextureParameterValue(FNePyObjectBase* InSelf, PyObject* InArgs)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetTextureParameterValue'"))
	{
		return nullptr;
	}

	char* ParamName = nullptr;
	if (!PyArg_ParseTuple(InArgs, "s:GetTextureParameterValue", &ParamName))
	{
		return nullptr;
	}

	if (!InSelf->Value->IsA<UMaterialInstance>())
	{
		return PyErr_Format(PyExc_Exception, "uobject is not a UMaterialInstance");
	}

	UMaterialInstance* MaterialInstance = (UMaterialInstance*)InSelf->Value;

	UTexture* Texture = nullptr;
	if (!MaterialInstance->GetTextureParameterValue(UTF8_TO_TCHAR(ParamName), Texture))
	{
		Py_RETURN_NONE;
	}

	return NePyBase::ToPy(Texture);
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetScalarParameterValue", NePyCFunctionCast(&NePyMaterialInstance_GetScalarParameterValue), METH_VARARGS, "(self, ParamName: str) -> float"}, \
{"GetVectorParameterValue", NePyCFunctionCast(&NePyMaterialInstance_GetVectorParameterValue), METH_VARARGS, "(self, ParamName: str) -> LinearColor"}, \
{"GetTextureParameterValue", NePyCFunctionCast(&NePyMaterialInstance_GetTextureParameterValue), METH_VARARGS, "(self, ParamName: str) -> Texture"}, 
