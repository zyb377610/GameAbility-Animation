#include "NePyUserWidget.h"
#include "Widgets/Layout/SBox.h"
#include "UMGStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "MovieScene.h"
#include "NePyBase.h"
#include "NePyGIL.h"
#include "NePyHouseKeeper.h"
#include "NePyAutoExportVersion.h"
#if WITH_NEPY_AUTO_EXPORT
#include "NePyStruct_Geometry.h"
#include "NePyStruct_PointerEvent.h"
#include "NePyStruct_KeyEvent.h"
#include "NePyStruct_PaintContext.h"
#endif // WITH_NEPY_AUTO_EXPORT

namespace NePyUserWidget
{
#if WITH_NEPY_AUTO_EXPORT
	template <typename T>
	PyObject* ToPy(const T& InCppStruct)
	{
		PyObject* PyStruct = nullptr;
		NePyBase::ToPy(InCppStruct, PyStruct);
		return PyStruct;
	}
#endif // WITH_NEPY_AUTO_EXPORT
}

void UNePyUserWidget::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UNePyUserWidget::ReleasePythonResources()
{
	// 释放持有的py_proxy
	if (PyInstance)
	{
		FNePyScopedGIL GIL;
		Py_DECREF(PyInstance);
		PyInstance = nullptr;
	}
}

void UNePyUserWidget::NativeOnInitialized()
{
	ImportPythonInstance();
	Super::NativeOnInitialized();
}

void UNePyUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	WidgetTree->ForEachWidget([&](UWidget* Widget) {
		if (Widget->IsA<UNativeWidgetHost>())
		{
			NativeWidgetHost = Cast<UNativeWidgetHost>(Widget);
		}
	});

	if (PythonTickForceDisabled)
		bHasScriptImplementedTick = false;

	if (PythonPaintForceDisabled)
		bHasScriptImplementedPaint = false;

	FNePyScopedGIL GIL;
	if (!PyInstance)
	{
		PyErr_Print();
		return;
	}

	if (!PyObject_HasAttrString(PyInstance, (char*)"construct"))
		return;

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"construct", NULL));
	if (!Ret) 
	{
		PyErr_Print();
		return;
	}
}

void UNePyUserWidget::ImportPythonInstance()
{
	if (PythonModule.IsEmpty() || PythonClass.IsEmpty())
	{
		return;
	}

	FNePyScopedGIL GIL;
	FNePyObjectPtr PyWidgetModule = NePyStealReference(PyImport_ImportModule(TCHAR_TO_UTF8(*PythonModule)));
	if (!PyWidgetModule)
	{
		PyErr_Print();
		return;
	}

	PyObject* PyWidgetModuleDict = PyModule_GetDict(PyWidgetModule);
	PyObject* PyWidgetClass = PyDict_GetItemString(PyWidgetModuleDict, TCHAR_TO_UTF8(*PythonClass));
	if (!PyWidgetClass)
	{
		UE_LOG(LogNePython, Error, TEXT("Unable to find class %s in module %s"), *PythonClass, *PythonModule);
		return;
	}

	FNePyObjectPtr PyWidgetInstance = NePyStealReference(PyObject_CallObject(PyWidgetClass, nullptr));
	if (!PyWidgetInstance)
	{
		PyErr_Print();
		return;
	}

	SetPythonInstance(PyWidgetInstance);
}


// 目前是由python来创建UNePyUserWidget对象，并调用此方法绑定界面的python实例
void UNePyUserWidget::SetPythonInstance(PyObject* InPyInstance)
{
	if (!InPyInstance) 
	{
		PyErr_Print();
		return;
	}

	FNePyScopedGIL GIL;
	Py_XINCREF(InPyInstance);
	Py_XDECREF(PyInstance);
	PyInstance = InPyInstance;
	
	FNePyObjectPtr SelfObj = NePyStealReference(NePyBase::ToPy(this));
	if (SelfObj)
	{
		PyObject_SetAttrString(PyInstance, (char*)"uobject", (PyObject*)SelfObj);
	}
}

void UNePyUserWidget::NativeDestruct()
{
	if (PyInstance)
	{
		FNePyScopedGIL GIL;

		if (!PyObject_HasAttrString(PyInstance, (char*)"destruct"))
			return;

		FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"destruct", NULL));
		if (!Ret)
		{
			PyErr_Print();
			return;
		}

		Py_DECREF(PyInstance);
		PyInstance = nullptr;
	}

	Super::NativeDestruct();
}

// Called every frame
void UNePyUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance || PythonTickForceDisabled)
		return;

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_native_tick"))
		return;

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(MyGeometry));
	if (!PyGeometry)
		return;

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_native_tick", (char*)"Of", PyGeometry.Get(), InDeltaTime));
	if (!Ret) {
		PyErr_Print();
		return;
	}
#endif
}

FReply UNePyUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_mouse_button_down"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InMouseEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_mouse_button_down", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) {
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) {
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_mouse_button_up"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InMouseEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_mouse_button_up", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) {
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) {
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Super::NativeOnKeyUp(InGeometry, InKeyEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_key_up"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InKeyEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_key_up", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) 
	{
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) 
	{
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	Super::NativeOnKeyDown(InGeometry, InKeyEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_key_down"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InKeyEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_key_down", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) {
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) {
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

#if WITH_EDITOR

const FText UNePyUserWidget::GetPaletteCategory()
{
	return NSLOCTEXT("Python", "Python", "Python");
}
#endif

void UNePyUserWidget::SetSlateWidget(TSharedRef<SWidget> InContent)
{
	if (NativeWidgetHost.IsValid())
	{
		NativeWidgetHost->SetContent(InContent);
	}
}


void UNePyUserWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
}

void UNePyUserWidget::GetAnimations(TMap<FName, UWidgetAnimation*>& OutAnimationsMap)
{
	for (FProperty* PropertyIt = GetClass()->PropertyLink; PropertyIt != NULL; PropertyIt = PropertyIt->PropertyLinkNext)
	{
		if (PropertyIt->IsA(FObjectProperty::StaticClass()))
		{
			FObjectProperty* SubObject = CastFieldChecked<FObjectProperty>(PropertyIt);
			UWidgetAnimation* WidgetAnimation = Cast<UWidgetAnimation>(SubObject->GetObjectPropertyValue_InContainer(this));
			if (WidgetAnimation && WidgetAnimation->MovieScene)
			{
				FName AnimName = WidgetAnimation->MovieScene->GetFName();
				OutAnimationsMap.Emplace(AnimName, WidgetAnimation);
			}
		}
	}
}

TSharedRef<SWidget> UNePyUserWidget::RebuildWidget()
{
	return Super::RebuildWidget();
}

FReply UNePyUserWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseWheel(InGeometry, InMouseEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_mouse_wheel"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InMouseEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_mouse_wheel", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) {
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) {
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseMove(InGeometry, InMouseEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_mouse_move"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InMouseEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_mouse_move", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) {
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) {
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	Super::NativeOnTouchStarted(InGeometry, InGestureEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_touch_started"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InGestureEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_touch_started", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) 
	{
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret))
	{
		// capture此触摸事件，后续此触摸事件才能一直往本widget发，否则当鼠标移动到别的更高层级按钮上再弹起时，会收不到touchEnd
		TSharedPtr<SWidget> CapturingSlateWidget = this->GetCachedWidget();
		if (CapturingSlateWidget && CapturingSlateWidget.IsValid())
		{
			return FReply::Handled().CaptureMouse(CapturingSlateWidget.ToSharedRef());
		}
		else
		{
			return FReply::Handled();
		}
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	Super::NativeOnTouchMoved(InGeometry, InGestureEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_touch_moved"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InGestureEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_touch_moved", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) {
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) {
		return FReply::Handled();
	}
#endif
	return FReply::Unhandled();
}

FReply UNePyUserWidget::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	Super::NativeOnTouchEnded(InGeometry, InGestureEvent);

#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance)
		return FReply::Unhandled();

	FNePyScopedGIL GIL;

	if (!PyObject_HasAttrString(PyInstance, (char*)"on_touch_ended"))
		return FReply::Unhandled();

	FNePyObjectPtr PyGeometry = NePyStealReference(NePyUserWidget::ToPy(InGeometry));
	if (!PyGeometry)
		return FReply::Unhandled();

	FNePyObjectPtr PyEvent = NePyStealReference(NePyUserWidget::ToPy(InGestureEvent));
	if (!PyEvent)
		return FReply::Unhandled();

	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_touch_ended", (char*)"OO", PyGeometry.Get(), PyEvent.Get()));
	if (!Ret) 
	{
		PyErr_Print();
		return FReply::Unhandled();
	}
	if (PyObject_IsTrue(Ret)) 
	{
		return FReply::Handled().ReleaseMouseCapture();
	}
#endif
	return FReply::Unhandled();
}

void UNePyUserWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);

	if (!PyInstance)
		return;

	FNePyScopedGIL GIL;
	if (!PyObject_HasAttrString(PyInstance, (char*)"on_mouse_capture_lost"))
		return;
	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"on_mouse_capture_lost", (char*)"i", CaptureLostEvent.PointerIndex));
	if (!Ret) 
	{
		PyErr_Print();
		return;
	}
}

bool UNePyUserWidget::NativeIsInteractable() const
{
	if (!PyInstance)
		return false;

	FNePyScopedGIL GIL;
	if (!PyObject_HasAttrString(PyInstance, (char*)"is_interactable"))
		return false;
	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"is_interactable", nullptr));
	if (!Ret) 
	{
		PyErr_Print();
		return false;
	}
	if (PyObject_IsTrue(Ret)) 
	{
		return true;
	}

	return false;
}

int32 UNePyUserWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
#if WITH_NEPY_AUTO_EXPORT
	if (!PyInstance || PythonPaintForceDisabled)
		return LayerId;

	FNePyScopedGIL GIL;
	if (!PyObject_HasAttrString(PyInstance, (char*)"paint"))
		return LayerId;

	FPaintContext InContext(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	FNePyObjectPtr PyPaintContext = NePyStealReference(NePyUserWidget::ToPy(InContext));
	if (!PyPaintContext)
	{
		return LayerId;
	}
	FNePyObjectPtr Ret = NePyStealReference(PyObject_CallMethod(PyInstance, (char*)"paint", (char*)"O", PyPaintContext.Get()));
	if (!Ret) 
	{
		PyErr_Print();
		return LayerId;
	}
	int32 RetValue = LayerId;
	if (PyNumber_Check(Ret))
	{
		FNePyObjectPtr PyValue = NePyStealReference(PyNumber_Long(Ret));
		RetValue = PyLong_AsLong(PyValue);
	}

	return RetValue;
#else
	return 0;
#endif
}

UNePyUserWidget::UNePyUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PythonTickForceDisabled(true)
	, PythonPaintForceDisabled(true)
{}


void UNePyUserWidget::CallPythonUserWidgetMethod(FString InMethodName, FString InArgs)
{
	if (!PyInstance)
		return;

	FNePyScopedGIL GIL;
	FNePyObjectPtr Ret;
	if (InArgs.IsEmpty())
	{
		Ret = NePyStealReference(PyObject_CallMethod(PyInstance, TCHAR_TO_UTF8(*InMethodName), NULL));
	}
	else
	{
		Ret = NePyStealReference(PyObject_CallMethod(PyInstance, TCHAR_TO_UTF8(*InMethodName), (char*)"s", TCHAR_TO_UTF8(*InArgs)));
	}

	if (!Ret)
	{
		PyErr_Print();
		return;
	}
}

void UNePyUserWidget::BindWidget(PyObject* WidgetMap)
{
	if (!PyInstance)
	{
		return;
	}

	static const TCHAR IgnorePrefix = '_';
	FNePyScopedGIL GIL;

	WidgetTree->ForEachWidget([&](UWidget* Widget)
	{
		FString WidgetName = Widget->GetName();
		if (WidgetName[0] == IgnorePrefix)
			return;

		FNePyObjectPtr PyWidgetObj = NePyStealReference(NePyBase::ToPy(Widget));
		PyDict_SetItemString(WidgetMap, TCHAR_TO_UTF8(*WidgetName.ToLower()), PyWidgetObj);
	});

	UWidgetBlueprintGeneratedClass* WidgetClass = GetWidgetTreeOwningClass();
	static const FString InstPrefix = "_INST";
	for (int Idx = 0; Idx < WidgetClass->Animations.Num(); Idx++)
	{
		FString AnimName = WidgetClass->Animations[Idx]->GetName();
		AnimName = AnimName.Left(AnimName.Len() - InstPrefix.Len()); //去掉_INST后缀
		FProperty* AnimProperty = WidgetClass->FindPropertyByName(FName(*AnimName));

		FObjectProperty* AnimObjectProperty = CastField<FObjectProperty>(AnimProperty);
		if (!AnimObjectProperty)
			continue;

		UObject* AnimObject = AnimObjectProperty->GetObjectPropertyValue_InContainer(this, 0);
		if (!AnimObject)
			continue;

		FNePyObjectPtr PyAnimObj = NePyStealReference(NePyBase::ToPy(AnimObject));
		PyObject_SetAttrString(PyInstance, TCHAR_TO_UTF8(*AnimName), PyAnimObj);
	}
}
