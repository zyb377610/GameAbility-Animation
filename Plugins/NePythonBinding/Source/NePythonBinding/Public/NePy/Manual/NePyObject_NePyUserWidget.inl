#pragma once
#include "NePyBase.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "NePyUserWidget.h"


PyObject* NePyUserWidget_GetPyProxy(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetPyProxy'"))
	{
		return nullptr;
	}

	if (!InSelf->Value->IsA(UNePyUserWidget::StaticClass()))
	{
		PyErr_Format(PyExc_RuntimeError, "self(%p) is not a UNePyUserWidget", InSelf);
		return nullptr;
	}

	UNePyUserWidget* Widget = static_cast<UNePyUserWidget*>(InSelf->Value);
	PyObject* PyWidget = Widget->GetPyProxy();
	if (PyWidget)
	{
		Py_INCREF(PyWidget);
		return PyWidget;
	}

	Py_RETURN_NONE;
}

PyObject* NePyUserWidget_BindWidget(FNePyObjectBase* InSelf, PyObject* InArg)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'BindWidget'"))
	{
		return nullptr;
	}

	if (!InSelf->Value->IsA(UNePyUserWidget::StaticClass()))
	{
		PyErr_Format(PyExc_RuntimeError, "self(%p) is not a UNePyUserWidget", InSelf);
		return nullptr;
	}
	
	PyObject* WidgetMap;
	if (!PyArg_ParseTuple(InArg, "O:BindWidget", &WidgetMap))
	{
		return nullptr;
	}

	UNePyUserWidget* UserWidget = static_cast<UNePyUserWidget*>(InSelf->Value);
	UserWidget->BindWidget(WidgetMap);
	Py_RETURN_NONE;
}

PyObject* NepyUserWidget_GetAnimations(FNePyObjectBase* InSelf)
{
	if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method 'GetAnimations'"))
	{
		return nullptr;
	}

	UNePyUserWidget* UserWidget = (UNePyUserWidget*)InSelf->Value;
	TMap<FName, UWidgetAnimation*> AnimationMap;
	UserWidget->GetAnimations(AnimationMap);

	PyObject* AnimationDict = PyDict_New();
	for (auto& Elem : AnimationMap)
	{
		PyDict_SetItemString(AnimationDict, TCHAR_TO_ANSI(*Elem.Key.ToString()), NePyBase::ToPy(Elem.Value));
	}
	return AnimationDict;
}

#undef NePyManualExportFuncs
#define NePyManualExportFuncs \
{"GetPyProxy", NePyCFunctionCast(&NePyUserWidget_GetPyProxy), METH_NOARGS, "(self) -> object"}, \
{"BindWidget", NePyCFunctionCast(&NePyUserWidget_BindWidget), METH_VARARGS, "(self, WidgetMap: dict) -> None"}, \
{"GetAnimations", NePyCFunctionCast(&NepyUserWidget_GetAnimations), METH_VARARGS, "(self) -> dict"}, \
