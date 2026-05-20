#pragma once
#include "NePyBase.h"
#include "Animation/AnimationAsset.h"
#include "Animation/Skeleton.h"

PyObject* NePyAnimationAsset_GetSkeleton(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetSkeleton'"))
	{
		return nullptr;
	}

	UAnimationAsset* Anim = (UAnimationAsset*)InSelf->Value;
	USkeleton* Skeleton = Anim->GetSkeleton();

	return NePyBase::ToPy(Skeleton);
}

PyObject* NePyAnimationAsset_SetSkeleton(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'SetSkeleton'"))
	{
		return nullptr;
	}

	USkeleton* Skeleton = NePyBase::ToCppObject<USkeleton>(InArg);
	if (!Skeleton)
	{
		return PyErr_Format(PyExc_Exception, "argument is not a USkeleton.");
	}

	UAnimationAsset* Anim = (UAnimationAsset*)InSelf->Value;
	Anim->SetSkeleton(Skeleton);

	Py_RETURN_NONE;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetSkeleton", NePyCFunctionCast(&NePyAnimationAsset_GetSkeleton), METH_NOARGS, "(self) -> Skeleton"}, \
{"SetSkeleton", NePyCFunctionCast(&NePyAnimationAsset_SetSkeleton), METH_O, "(self, NewSkeleton: Skeleton) -> None"}, \
