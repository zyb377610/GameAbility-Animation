#pragma once

#include "Runtime/UMG/Public/Blueprint/UserWidget.h"
#include "Runtime/UMG/Public/Components/NativeWidgetHost.h"
#include "NePyIncludePython.h"
#include "NePyResourceOwner.h"
#include "NePyUserWidget.generated.h"


UCLASS(BlueprintType, Blueprintable)
class NEPYTHONBINDING_API UNePyUserWidget : public UUserWidget, public INePyResourceOwner
{
	GENERATED_BODY()

public:
	UNePyUserWidget(const FObjectInitializer& ObjectInitializer);

	// UObject interface
	virtual void BeginDestroy() override;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Called every frame
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool NativeIsInteractable() const override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent);

	UPROPERTY(EditAnywhere, Category = "Python")
	FString PythonModule;

	UPROPERTY(EditAnywhere, Category = "Python")
	FString PythonClass;

	UPROPERTY(EditAnywhere, Category = "Python", BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	bool PythonTickForceDisabled;

	UPROPERTY(EditAnywhere, Category = "Python", BlueprintReadWrite, meta = (ExposeOnSpawn = true))
	bool PythonPaintForceDisabled;

	UFUNCTION(BlueprintCallable, Category = "Python")
	void CallPythonUserWidgetMethod(FString InMethodName, FString InArgs);

	// 从property中import python脚本
	void ImportPythonInstance();
	// 给此界面蓝图绑定一个python对象
	void SetPythonInstance(PyObject* InPyInstance);

	UPROPERTY(EditAnywhere, Category = "Python", BlueprintReadWrite)
	TWeakObjectPtr<class UNativeWidgetHost> NativeWidgetHost;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif
	// 创建面板中控件的名字映射
	void BindWidget(PyObject *WidgetMap);

	PyObject* GetPyProxy()
	{
		return PyInstance;
	}
	
	void SetSlateWidget(TSharedRef<SWidget> InContent);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	void GetAnimations(TMap<FName, UWidgetAnimation*>& outAnimationsMap);

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

private:
	// 此界面对应的python实例
	PyObject* PyInstance;
};


