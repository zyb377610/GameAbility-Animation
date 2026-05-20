#pragma once
#include "NePyBase.h"
#include "Animation/BlendSpaceBase.h"

//// unsupported FBlendParameter
//PyObject* NePyBlendSpaceBase_GetBlendParameter(FNePyObjectBase* InSelf, PyObject* InArgs)
//{
//	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetBlendParameter'"))
//	{
//		return nullptr;
//	}
//
//	int Index;
//	if (!PyArg_ParseTuple(InArgs, "i:GetBlendParameter", &Index))
//	{
//		return nullptr;
//	}
//
//	if (Index < 0 || Index > 2)
//	{
//		return PyErr_Format(PyExc_Exception, "Invalid Blend Parameter Index");
//	}
//
//	UBlendSpaceBase* BlendSpace = (UBlendSpaceBase*)InSelf->Value;
//	const FBlendParameter& parameter = BlendSpace->GetBlendParameter(Index);
//
//	return NePyBlendSpaceBase_new_owned_uscriptstruct(FBlendParameter::StaticStruct(), (uint8*)&parameter);
//}
//
//// unsupported FBlendParameter
//PyObject* NePyBlendSpaceBase_SetBlendParameter(FNePyObjectBase* InSelf, PyObject* InArgs)
//{
//	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetBlendParameter'"))
//	{
//		return nullptr;
//	}
//
//	int Index;
//	PyObject* PyParameter;
//	if (!PyArg_ParseTuple(InArgs, "iO:GetBlendParameter", &Index, &PyParameter))
//	{
//		return nullptr;
//	}
//
//	if (Index < 0 || Index > 2)
//	{
//		return PyErr_Format(PyExc_Exception, "Invalid Blend Parameter Index");
//	}
//
//	FBlendParameter* Parameter = ue_py_check_struct<FBlendParameter>(PyParameter);
//	if (!Parameter)
//	{
//		return PyErr_Format(PyExc_Exception, "argument is not a FBlendParameter");
//	}
//
//
//	UBlendSpaceBase* BlendSpace = (UBlendSpaceBase*)InSelf->Value;
//	const FBlendParameter& OriginParameter = BlendSpace->GetBlendParameter(Index);
//
//	FMemory::Memcpy((uint8*)&OriginParameter, Parameter, FBlendParameter::StaticStruct()->GetStructureSize());
//
//	Py_RETURN_NONE;
//}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
