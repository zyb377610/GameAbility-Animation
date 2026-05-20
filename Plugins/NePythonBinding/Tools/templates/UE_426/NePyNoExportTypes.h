#pragma once
#include "CoreMinimal.h"
#include "NePyNoExportTypes.generated.h"

// 此文件用于手动添加标注，给UHT提供元信息，以导出属性、函数信息
// 编译时，请删除此文件

#if !CPP

// Base class for all mouse and keyevents.
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_InputEvent : public UObject
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	void FInputEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsRepeat();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsShiftDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsLeftShiftDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsRightShiftDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsControlDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsLeftControlDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsRightControlDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAltDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsLeftAltDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsRightAltDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsCommandDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsLeftCommandDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsRightCommandDown();

	UFUNCTION(meta=(bFriendFunction = false))
	bool AreCapsLocked();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 GetUserIndex();

	UFUNCTION(meta=(bFriendFunction = false))
	FText ToText();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsPointerEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsKeyEvent();

};

/**
* FKeyEvent describes a key action (keyboard/controller key/button pressed or released.)
* It is passed to event handlers dealing with key input.
*/
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_KeyEvent : public UNePyNoExportType_InputEvent
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	void FKeyEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	FKey GetKey();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 GetCharacter();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 GetKeyCode();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ToText", OriginalName = "ToText"))
	FText KeyEvent_ToText();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsKeyEvent", OriginalName = "IsKeyEvent"))
	bool KeyEvent_IsKeyEvent();

};

/**
* FAnalogEvent describes a analog key value.
* It is passed to event handlers dealing with analog keys.
*/
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_AnalogInputEvent : public UNePyNoExportType_KeyEvent
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	void FAnalogInputEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetAnalogValue();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ToText", OriginalName = "ToText"))
	FText AnalogInputEvent_ToText();

};

/**
* A rectangular 2D Box.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Box2D.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Box2D : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	FVector2D Min;

	UPROPERTY()
	FVector2D Max;

	UPROPERTY()
	bool bIsValid;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FBox2D", OriginalName = "FBox2D"))
	void FBox2D_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FBox2D", OriginalName = "FBox2D"))
	void FBox2D_Overload2(const FVector2D& InMin, const FVector2D& InMax);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FBox2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FBox2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__iadd__", OriginalName = "operator+="))
	FBox2D __iadd___Overload1(const FVector2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FBox2D __add___Overload1(const FVector2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__iadd__", OriginalName = "operator+="))
	FBox2D __iadd___Overload2(const FBox2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FBox2D __add___Overload2(const FBox2D& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	float ComputeSquaredDistanceToPoint(const FVector2D& Point);

	UFUNCTION(meta=(bFriendFunction = false))
	FBox2D ExpandBy(float W);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetArea();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetCenter();

	UFUNCTION(meta=(bFriendFunction = false))
	void GetCenterAndExtents(FVector2D& center, FVector2D& Extents);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetClosestPointTo(const FVector2D& Point);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetExtent();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetSize();

	UFUNCTION(meta=(bFriendFunction = false))
	void Init();

	UFUNCTION(meta=(bFriendFunction = false))
	bool Intersect(const FBox2D& other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsInside", OriginalName = "IsInside"))
	bool IsInside_Overload1(const FVector2D& TestPoint);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsInside", OriginalName = "IsInside"))
	bool IsInside_Overload2(const FBox2D& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	FBox2D ShiftBy(const FVector2D& Offset);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

};

/**
* A bounding box and bounding sphere with the same origin.
* @note The full C++ class is located here : Engine/Source/Runtime/Core/Public/Math/BoxSphereBounds.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_BoxSphereBounds : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	FVector Origin;

	UPROPERTY()
	FVector BoxExtent;

	UPROPERTY()
	float SphereRadius;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FBoxSphereBounds", OriginalName = "FBoxSphereBounds"))
	void FBoxSphereBounds_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FBoxSphereBounds", OriginalName = "FBoxSphereBounds"))
	void FBoxSphereBounds_Overload2(const FVector& InOrigin, const FVector& InBoxExtent, float InSphereRadius);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FBoxSphereBounds", OriginalName = "FBoxSphereBounds"))
	void FBoxSphereBounds_Overload3(const FBox& Box);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FBoxSphereBounds __add__(const FBoxSphereBounds& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FBoxSphereBounds& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FBoxSphereBounds& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	float ComputeSquaredDistanceFromBoxToPoint(const FVector& Point);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	static bool SpheresIntersect(const FBoxSphereBounds& A, const FBoxSphereBounds& B, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool BoxesIntersect(const FBoxSphereBounds& A, const FBoxSphereBounds& B);

	UFUNCTION(meta=(bFriendFunction = false))
	FBox GetBox();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetBoxExtrema(uint32 Extrema);

	UFUNCTION(meta=(bFriendFunction = false))
	FBoxSphereBounds ExpandBy(float ExpandAmount);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "TransformBy", OriginalName = "TransformBy"))
	FBoxSphereBounds TransformBy_Overload1(const FMatrix& M);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "TransformBy", OriginalName = "TransformBy"))
	FBoxSphereBounds TransformBy_Overload2(const FTransform& M);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = true))
	FBoxSphereBounds Union(const FBoxSphereBounds& InSelf, const FBoxSphereBounds& B);

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

};

// FCharacterEvent describes a keyboard action where the utf-16 code is given.  Used for OnKeyChar messages
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_CharacterEvent : public UNePyNoExportType_InputEvent
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	void FCharacterEvent();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ToText", OriginalName = "ToText"))
	FText CharacterEvent_ToText();

};

/**
* Stores a color with 8 bits of precision per channel. (BGRA).
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Color.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Color : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	uint8 B;

	UPROPERTY()
	uint8 G;

	UPROPERTY()
	uint8 R;

	UPROPERTY()
	uint8 A;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FColor", OriginalName = "FColor"))
	void FColor_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FColor", OriginalName = "FColor", CPP_Default_InA = "255"))
	void FColor_Overload2(uint8 InR, uint8 InG, uint8 InB, uint8 InA);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FColor", OriginalName = "FColor"))
	void FColor_Overload3(uint32 InColor);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FColor& C);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FColor& C);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	void __iadd__(const FColor& C);

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor FromRGBE();

	UFUNCTION(meta=(bFriendFunction = false))
	static FColor FromHex(const FString& HexString);

	UFUNCTION(meta=(bFriendFunction = false))
	static FColor MakeRandomColor();

	UFUNCTION(meta=(bFriendFunction = false))
	static FColor MakeRedToGreenColorFromScalar(float Scalar);

	UFUNCTION(meta=(bFriendFunction = false))
	static FColor MakeFromColorTemperature(float Temp);

	UFUNCTION(meta=(bFriendFunction = false))
	FColor WithAlpha(uint8 Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor ReinterpretAsLinear();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToHex();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 ToPackedARGB();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 ToPackedABGR();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 ToPackedRGBA();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 ToPackedBGRA();

};

/**
* A value representing a specific point date and time over a wide range of years.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Misc/DateTime.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_DateTime : public UObject
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FDateTime", OriginalName = "FDateTime"))
	void FDateTime_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FDateTime", OriginalName = "FDateTime"))
	void FDateTime_Overload2(int64 InTicks);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FDateTime", OriginalName = "FDateTime", CPP_Default_Hour = "0", CPP_Default_Minute = "0", CPP_Default_Second = "0", CPP_Default_Millisecond = "0"))
	void FDateTime_Overload3(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FDateTime __add___Overload1(const FTimespan& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FDateTime __iadd__(const FTimespan& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FDateTime __add___Overload2(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FTimespan __sub___Overload1(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FDateTime __sub___Overload2(const FTimespan& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FDateTime __isub__(const FTimespan& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator>"))
	bool __gt__(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator>="))
	bool __ge__(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator<"))
	bool __lt__(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator<="))
	bool __le__(const FDateTime& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	FDateTime GetDate();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetDay();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetDayOfYear();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetHour();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetHour12();

	UFUNCTION(meta=(bFriendFunction = false))
	double GetJulianDay();

	UFUNCTION(meta=(bFriendFunction = false))
	double GetModifiedJulianDay();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetMillisecond();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetMinute();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetMonth();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetSecond();

	UFUNCTION(meta=(bFriendFunction = false))
	int64 GetTicks();

	UFUNCTION(meta=(bFriendFunction = false))
	FTimespan GetTimeOfDay();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetYear();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAfternoon();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsMorning();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToHttpDate();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToIso8601();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	int64 ToUnixTimestamp();

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 DaysInMonth(int32 Year, int32 Month);

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 DaysInYear(int32 Year);

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime FromJulianDay(double JulianDay);

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime FromUnixTimestamp(int64 UnixTime);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool IsLeapYear(int32 Year);

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime MaxValue();

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime MinValue();

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime Now();

	UFUNCTION(meta=(bFriendFunction = false))
	static bool Parse(const FString& DateTimeString, FDateTime& OutDateTime);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool ParseHttpDate(const FString& HttpDate, FDateTime& OutDateTime);

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime Today();

	UFUNCTION(meta=(bFriendFunction = false))
	static FDateTime UtcNow();

	UFUNCTION(meta=(bFriendFunction = false))
	static bool Validate(int32 Year, int32 Month, int32 Day, int32 Hour, int32 Minute, int32 Second, int32 Millisecond);

	UFUNCTION(meta=(bFriendFunction = true))
	uint32 GetTypeHash(const FDateTime& InSelf);

};

/**
* Represents the position, size, and absolute position of a Widget in Slate.
* The absolute location of a geometry is usually screen space or
* window space depending on where the geometry originated.
* Geometries are usually paired with a SWidget pointer in order
* to provide information about a specific widget (see FArrangedWidget).
* A Geometry's parent is generally thought to be the Geometry of the
* the corresponding parent widget.
*/
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_Geometry : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	FVector2D Size;

	UPROPERTY()
	float Scale;

	UPROPERTY()
	FVector2D AbsolutePosition;

	UPROPERTY()
	FVector2D Position;

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FGeometry& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FGeometry& Other);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_ChildScale = "1.0f"))
	FGeometry MakeChild(const FVector2D& ChildOffset, const FVector2D& InLocalSize, float ChildScale);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsUnderLocation(const FVector2D& AbsoluteCoordinate);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D AbsoluteToLocal(FVector2D AbsoluteCoordinate);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D LocalToAbsolute(FVector2D LocalCoordinate);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D LocalToRoundedLocal(FVector2D LocalCoordinate);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetDrawSize();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetLocalSize();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetAbsolutePosition();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetAbsoluteSize();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetAbsolutePositionAtCoordinates(const FVector2D& NormalCoordinates);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetLocalPositionAtCoordinates(const FVector2D& NormalCoordinates);

	UFUNCTION(meta=(bFriendFunction = false))
	bool HasRenderTransform();

};

// A globally unique identifier (mirrored from Guid.h)
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Guid : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	uint32 A;

	UPROPERTY()
	uint32 B;

	UPROPERTY()
	uint32 C;

	UPROPERTY()
	uint32 D;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FGuid", OriginalName = "FGuid"))
	void FGuid_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FGuid", OriginalName = "FGuid"))
	void FGuid_Overload2(uint32 InA, uint32 InB, uint32 InC, uint32 InD);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FGuid", OriginalName = "FGuid"))
	void FGuid_Overload3(const FString& InGuidStr);

	UFUNCTION(meta=(bFriendFunction = true, OriginalName = "operator=="))
	bool __eq__(const FGuid& InSelf, const FGuid& Y);

	UFUNCTION(meta=(bFriendFunction = true, OriginalName = "operator!="))
	bool __ne__(const FGuid& InSelf, const FGuid& Y);

	UFUNCTION(meta=(bFriendFunction = true, OriginalName = "operator<"))
	bool __lt__(const FGuid& InSelf, const FGuid& Y);

	UFUNCTION(meta=(bFriendFunction = true))
	FString LexToString(const FGuid& InSelf);

	UFUNCTION(meta=(bFriendFunction = false))
	void Invalidate();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ToString", OriginalName = "ToString"))
	FString ToString_Overload1();

	UFUNCTION(meta=(bFriendFunction = true))
	uint32 GetTypeHash(const FGuid& InSelf);

	UFUNCTION(meta=(bFriendFunction = false))
	static FGuid NewGuid();

	UFUNCTION(meta=(bFriendFunction = false))
	static bool Parse(const FString& GuidString, FGuid& OutGuid);

};

// Key
UCLASS(meta=(Package = "/Script/InputCore"))
class UNePyNoExportType_Key : public UObject
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FKey", OriginalName = "FKey"))
	void FKey_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FKey", OriginalName = "FKey"))
	void FKey_Overload2(FName InName);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsModifierKey();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsGamepadKey();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsTouch();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsMouseButton();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsButtonAxis();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAxis1D();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAxis2D();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAxis3D();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsFloatAxis();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsVectorAxis();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsDigital();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAnalog();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsBindableInBlueprints();

	UFUNCTION(meta=(bFriendFunction = false))
	bool ShouldUpdateAxisWithoutSamples();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsBindableToActions();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsDeprecated();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_bLongDisplayName = "true"))
	FText GetDisplayName(bool bLongDisplayName);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	FName GetFName();

	UFUNCTION(meta=(bFriendFunction = false))
	FName GetMenuCategory();

	UFUNCTION(meta=(bFriendFunction = false))
	FKey GetPairedAxisKey();

	UFUNCTION(meta=(bFriendFunction = true, OriginalName = "operator=="))
	bool __eq__(const FKey& InSelf, const FKey& KeyB);

	UFUNCTION(meta=(bFriendFunction = true, OriginalName = "operator!="))
	bool __ne__(const FKey& InSelf, const FKey& KeyB);

	UFUNCTION(meta=(bFriendFunction = true, OriginalName = "operator<"))
	bool __lt__(const FKey& InSelf, const FKey& KeyB);

	UFUNCTION(meta=(bFriendFunction = true))
	uint32 GetTypeHash(const FKey& InSelf);

};

/**
* A linear, 32-bit/component floating point RGBA color.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Color.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_LinearColor : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float R;

	UPROPERTY()
	float G;

	UPROPERTY()
	float B;

	UPROPERTY()
	float A;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FLinearColor", OriginalName = "FLinearColor"))
	void FLinearColor_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FLinearColor", OriginalName = "FLinearColor", CPP_Default_InA = "1.0f"))
	void FLinearColor_Overload2(float InR, float InG, float InB, float InA);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FLinearColor", OriginalName = "FLinearColor"))
	void FLinearColor_Overload3(const FColor& Color);

	UFUNCTION(meta=(bFriendFunction = false))
	FColor ToRGBE();

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor FromSRGBColor(const FColor& Color);

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor FromPow22Color(const FColor& Color);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FLinearColor __add__(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FLinearColor __iadd__(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-"))
	FLinearColor __sub__(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FLinearColor __isub__(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FLinearColor __mul___Overload1(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FLinearColor __imul___Overload1(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FLinearColor __mul___Overload2(float Scalar);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FLinearColor __imul___Overload2(float Scalar);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FLinearColor __div___Overload1(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FLinearColor __idiv___Overload1(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FLinearColor __div___Overload2(float Scalar);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FLinearColor __idiv___Overload2(float Scalar);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_InMin = "0.0f", CPP_Default_InMax = "1.0f"))
	FLinearColor GetClamped(float InMin, float InMax);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FLinearColor& ColorB);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FLinearColor& Other);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FLinearColor& ColorB, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor CopyWithNewOpacity(float NewOpacicty);

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor FGetHSV(uint8 H, uint8 S, uint8 V);

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor MakeFromHSV8(uint8 H, uint8 S, uint8 V);

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor MakeRandomColor();

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor MakeFromColorTemperature(float Temp);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Dist(const FLinearColor& V1, const FLinearColor& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor LinearRGBToHSV();

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor HSVToLinearRGB();

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor LerpUsingHSV(const FLinearColor& From, const FLinearColor& To, float Progress);

	UFUNCTION(meta=(bFriendFunction = false))
	FColor Quantize();

	UFUNCTION(meta=(bFriendFunction = false))
	FColor QuantizeRound();

	UFUNCTION(meta=(bFriendFunction = false))
	FColor ToFColor(bool bSRGB);

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor Desaturate(float Desaturation);

	UFUNCTION(meta=(bFriendFunction = false))
	float ComputeLuminance();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMax();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAlmostBlack();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMin();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetLuminance();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

};

// 
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Math : public UObject
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	static int32 RandHelper(int32 A);

	UFUNCTION(meta=(bFriendFunction = false))
	static int64 RandHelper64(int64 A);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RandRange", OriginalName = "RandRange"))
	static int32 RandRange_Overload1(int32 Min, int32 Max);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RandRange", OriginalName = "RandRange"))
	static int64 RandRange_Overload2(int64 Min, int64 Max);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RandRange", OriginalName = "RandRange"))
	static float RandRange_Overload3(float InMin, float InMax);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FRandRange(float InMin, float InMax);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool RandBool();

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector VRand();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "VRandCone", OriginalName = "VRandCone"))
	static FVector VRandCone_Overload1(const FVector& Dir, float ConeHalfAngleRad);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "VRandCone", OriginalName = "VRandCone"))
	static FVector VRandCone_Overload2(const FVector& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D RandPointInCircle(float CircleRadius);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector RandPointInBox(const FBox& Box);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector GetReflectionVector(const FVector& Direction, const FVector& SurfaceNormal);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsNearlyEqual", OriginalName = "IsNearlyEqual", CPP_Default_ErrorTolerance = "SMALL_NUMBER"))
	static bool IsNearlyEqual_Overload1(float A, float B, float ErrorTolerance);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsNearlyZero", OriginalName = "IsNearlyZero", CPP_Default_ErrorTolerance = "SMALL_NUMBER"))
	static bool IsNearlyZero_Overload1(float Value, float ErrorTolerance);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsNearlyEqualByULP", OriginalName = "IsNearlyEqualByULP", CPP_Default_MaxUlps = "4"))
	static bool IsNearlyEqualByULP_Overload1(float A, float B, int32 MaxUlps);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "Floor", OriginalName = "Floor"))
	static float Floor_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "Floor", OriginalName = "Floor"))
	static double Floor_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Max3(float A, float B, float C);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Min3(float A, float B, float C);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Square(float A);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Clamp(float X, float Min, float Max);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Wrap(float X, float Min, float Max);

	UFUNCTION(meta=(bFriendFunction = false))
	static float GridSnap(float Location, float Grid);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DivideAndRoundUp(float Dividend, float Divisor);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DivideAndRoundDown(float Dividend, float Divisor);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DivideAndRoundNearest(float Dividend, float Divisor);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Log2(float Value);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FastAsin(float Value);

	UFUNCTION(meta=(bFriendFunction = false))
	static float ClampAngle(float AngleDegrees, float MinAngleDegrees, float MaxAngleDegrees);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FindDeltaAngleDegrees(float A1, float A2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FindDeltaAngleRadians(float A1, float A2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FindDeltaAngle(float A1, float A2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float UnwindRadians(float A);

	UFUNCTION(meta=(bFriendFunction = false))
	static float UnwindDegrees(float A);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FixedTurn(float InCurrent, float InDesired, float InDeltaRate);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool GetDotDistance(FVector2D& OutDotDist, const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D GetAzimuthAndElevation(const FVector& Direction, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "GetRangePct", OriginalName = "GetRangePct"))
	static float GetRangePct_Overload1(float MinValue, float MaxValue, float Value);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "GetRangePct", OriginalName = "GetRangePct"))
	static float GetRangePct_Overload2(const FVector2D& Range, float Value);

	UFUNCTION(meta=(bFriendFunction = false))
	static float GetRangeValue(const FVector2D& Range, float Pct);

	UFUNCTION(meta=(bFriendFunction = false))
	static float GetMappedRangeValueClamped(const FVector2D& InputRange, const FVector2D& OutputRange, float Value);

	UFUNCTION(meta=(bFriendFunction = false))
	static float GetMappedRangeValueUnclamped(const FVector2D& InputRange, const FVector2D& OutputRange, float Value);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector VInterpNormalRotationTo(const FVector& Current, const FVector& Target, float DeltaTime, float RotationSpeedDegrees);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector VInterpConstantTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector VInterpTo(const FVector& Current, const FVector& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D Vector2DInterpConstantTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D Vector2DInterpTo(const FVector2D& Current, const FVector2D& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FRotator RInterpConstantTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FRotator RInterpTo(const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FInterpConstantTo(float Current, float Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static float FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FLinearColor CInterpTo(const FLinearColor& Current, const FLinearColor& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat QInterpConstantTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat QInterpTo(const FQuat& Current, const FQuat& Target, float DeltaTime, float InterpSpeed);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector RayPlaneIntersection(const FVector& RayOrigin, const FVector& RayDirection, const FPlane& Plane);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "LinePlaneIntersection", OriginalName = "LinePlaneIntersection"))
	static FVector LinePlaneIntersection_Overload1(const FVector& Point1, const FVector& Point2, const FVector& PlaneOrigin, const FVector& PlaneNormal);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "LinePlaneIntersection", OriginalName = "LinePlaneIntersection"))
	static FVector LinePlaneIntersection_Overload2(const FVector& Point1, const FVector& Point2, const FPlane& Plane);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool PlaneAABBIntersection(const FPlane& P, const FBox& AABB);

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 PlaneAABBRelativePosition(const FPlane& P, const FBox& AABB);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool PointBoxIntersection(const FVector& Point, const FBox& Box);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "LineBoxIntersection", OriginalName = "LineBoxIntersection"))
	static bool LineBoxIntersection_Overload1(const FBox& Box, const FVector& Start, const FVector& End, const FVector& Direction);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "LineBoxIntersection", OriginalName = "LineBoxIntersection"))
	static bool LineBoxIntersection_Overload2(const FBox& Box, const FVector& Start, const FVector& End, const FVector& Direction, const FVector& OneOverDirection);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool SphereConeIntersection(const FVector& SphereCenter, float SphereRadius, const FVector& ConeAxis, float ConeAngleSin, float ConeAngleCos);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector ClosestPointOnLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector ClosestPointOnInfiniteLine(const FVector& LineStart, const FVector& LineEnd, const FVector& Point);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool IntersectPlanes3(FVector& I, const FPlane& P1, const FPlane& P2, const FPlane& P3);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool IntersectPlanes2(FVector& I, FVector& D, const FPlane& P1, const FPlane& P2);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "PointDistToLine", OriginalName = "PointDistToLine"))
	static float PointDistToLine_Overload1(const FVector& Point, const FVector& Direction, const FVector& Origin, FVector& OutClosestPoint);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "PointDistToLine", OriginalName = "PointDistToLine"))
	static float PointDistToLine_Overload2(const FVector& Point, const FVector& Direction, const FVector& Origin);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector ClosestPointOnSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D ClosestPointOnSegment2D(const FVector2D& Point, const FVector2D& StartPoint, const FVector2D& EndPoint);

	UFUNCTION(meta=(bFriendFunction = false))
	static float PointDistToSegment(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);

	UFUNCTION(meta=(bFriendFunction = false))
	static float PointDistToSegmentSquared(const FVector& Point, const FVector& StartPoint, const FVector& EndPoint);

	UFUNCTION(meta=(bFriendFunction = false))
	static void SegmentDistToSegment(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);

	UFUNCTION(meta=(bFriendFunction = false))
	static void SegmentDistToSegmentSafe(FVector A1, FVector B1, FVector A2, FVector B2, FVector& OutP1, FVector& OutP2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float GetTForSegmentPlaneIntersect(const FVector& StartPoint, const FVector& EndPoint, const FPlane& Plane);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool SegmentPlaneIntersection(const FVector& StartPoint, const FVector& EndPoint, const FPlane& Plane, FVector& out_IntersectionPoint);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool SegmentTriangleIntersection(const FVector& StartPoint, const FVector& EndPoint, const FVector& A, const FVector& B, const FVector& C, FVector& OutIntersectPoint, FVector& OutTriangleNormal);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool SegmentIntersection2D(const FVector& SegmentStartA, const FVector& SegmentEndA, const FVector& SegmentStartB, const FVector& SegmentEndB, FVector& out_IntersectionPoint);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector ClosestPointOnTriangleToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector ClosestPointOnTetrahedronToPoint(const FVector& Point, const FVector& A, const FVector& B, const FVector& C, const FVector& D);

	UFUNCTION(meta=(bFriendFunction = false))
	static void SphereDistToLine(FVector SphereOrigin, float SphereRadius, FVector LineOrigin, FVector LineDir, FVector& OutClosestPoint);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "TruncateToHalfIfClose", OriginalName = "TruncateToHalfIfClose", CPP_Default_Tolerance = "SMALL_NUMBER"))
	static float TruncateToHalfIfClose_Overload1(float F, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundHalfToEven", OriginalName = "RoundHalfToEven"))
	static float RoundHalfToEven_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundHalfToEven", OriginalName = "RoundHalfToEven"))
	static double RoundHalfToEven_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundHalfFromZero", OriginalName = "RoundHalfFromZero"))
	static float RoundHalfFromZero_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundHalfFromZero", OriginalName = "RoundHalfFromZero"))
	static double RoundHalfFromZero_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundHalfToZero", OriginalName = "RoundHalfToZero"))
	static float RoundHalfToZero_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundHalfToZero", OriginalName = "RoundHalfToZero"))
	static double RoundHalfToZero_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundFromZero", OriginalName = "RoundFromZero"))
	static float RoundFromZero_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundFromZero", OriginalName = "RoundFromZero"))
	static double RoundFromZero_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundToZero", OriginalName = "RoundToZero"))
	static float RoundToZero_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundToZero", OriginalName = "RoundToZero"))
	static double RoundToZero_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundToNegativeInfinity", OriginalName = "RoundToNegativeInfinity"))
	static float RoundToNegativeInfinity_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundToNegativeInfinity", OriginalName = "RoundToNegativeInfinity"))
	static double RoundToNegativeInfinity_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundToPositiveInfinity", OriginalName = "RoundToPositiveInfinity"))
	static float RoundToPositiveInfinity_Overload1(float F);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "RoundToPositiveInfinity", OriginalName = "RoundToPositiveInfinity"))
	static double RoundToPositiveInfinity_Overload2(double F);

	UFUNCTION(meta=(bFriendFunction = false))
	static FString FormatIntToHumanReadable(int32 Val);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector GetBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector ComputeBaryCentric2D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector4 ComputeBaryCentric3D(const FVector& Point, const FVector& A, const FVector& B, const FVector& C, const FVector& D);

	UFUNCTION(meta=(bFriendFunction = false))
	static float SmoothStep(float A, float B, float X);

	UFUNCTION(meta=(bFriendFunction = false))
	static uint8 Quantize8UnsignedByte(float x);

	UFUNCTION(meta=(bFriendFunction = false))
	static uint8 Quantize8SignedByte(float x);

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 GreatestCommonDivisor(int32 a, int32 b);

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 LeastCommonMultiplier(int32 a, int32 b);

	UFUNCTION(meta=(bFriendFunction = false))
	static float PerlinNoise1D(float Value);

	UFUNCTION(meta=(bFriendFunction = false))
	static float PerlinNoise2D(const FVector2D& Location);

	UFUNCTION(meta=(bFriendFunction = false))
	static float PerlinNoise3D(const FVector& Location);

	UFUNCTION(meta=(bFriendFunction = false))
	static float WeightedMovingAverage(float CurrentSample, float PreviousSample, float Weight);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DynamicWeightedMovingAverage(float CurrentSample, float PreviousSample, float MaxDistance, float MinWeight, float MaxWeight);

};

/**
* A 4x4 matrix.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Matrix.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Matrix : public UObject
{
	GENERATED_BODY()

	public:
	// XPlane
	UPROPERTY(SaveGame)
	FPlane XPlane;

	// YPlane
	UPROPERTY(SaveGame)
	FPlane YPlane;

	// ZPlane
	UPROPERTY(SaveGame)
	FPlane ZPlane;

	// WPlane
	UPROPERTY(SaveGame)
	FPlane WPlane;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FMatrix", OriginalName = "FMatrix"))
	void FMatrix_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FMatrix", OriginalName = "FMatrix"))
	void FMatrix_Overload2(const FPlane& InX, const FPlane& InY, const FPlane& InZ, const FPlane& InW);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FMatrix", OriginalName = "FMatrix"))
	void FMatrix_Overload3(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetIdentity();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FMatrix __mul___Overload1(const FMatrix& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	void __imul___Overload1(const FMatrix& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FMatrix __add__(const FMatrix& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	void __iadd__(const FMatrix& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FMatrix __mul___Overload2(float Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	void __imul___Overload2(float Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FMatrix& Other);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FMatrix& Other, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FMatrix& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 TransformFVector4(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 TransformPosition(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector InverseTransformPosition(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 TransformVector(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector InverseTransformVector(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix GetTransposed();

	UFUNCTION(meta=(bFriendFunction = false))
	float Determinant();

	UFUNCTION(meta=(bFriendFunction = false))
	float RotDeterminant();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix InverseFast();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix Inverse();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix TransposeAdjoint();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	void RemoveScaling(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FMatrix GetMatrixWithoutScale(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FVector ExtractScaling(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FVector GetScaleVector(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix RemoveTranslation();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix ConcatTranslation(const FVector& Translation);

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false))
	void ScaleTranslation(const FVector& Scale3D);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMaximumAxisScale();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix ApplyScale(float Scale);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetOrigin();

	UFUNCTION(meta=(bFriendFunction = false))
	void GetScaledAxes(FVector& X, FVector& Y, FVector& Z);

	UFUNCTION(meta=(bFriendFunction = false))
	void GetUnitAxes(FVector& X, FVector& Y, FVector& Z);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetAxis(int32 i, const FVector& Axis);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetOrigin(const FVector& NewOrigin);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetColumn(int32 i);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetColumn(int32 i, FVector Value);

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Rotator();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat ToQuat();

	UFUNCTION(meta=(bFriendFunction = false))
	bool GetFrustumNearPlane(FPlane& OutPlane);

	UFUNCTION(meta=(bFriendFunction = false))
	bool GetFrustumFarPlane(FPlane& OutPlane);

	UFUNCTION(meta=(bFriendFunction = false))
	bool GetFrustumLeftPlane(FPlane& OutPlane);

	UFUNCTION(meta=(bFriendFunction = false))
	bool GetFrustumRightPlane(FPlane& OutPlane);

	UFUNCTION(meta=(bFriendFunction = false))
	bool GetFrustumTopPlane(FPlane& OutPlane);

	UFUNCTION(meta=(bFriendFunction = false))
	bool GetFrustumBottomPlane(FPlane& OutPlane);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 ComputeHash();

};

/**
* FMotionEvent describes a touch pad action (press, move, lift)
* It is passed to event handlers dealing with touch input.
*/
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_MotionEvent : public UNePyNoExportType_InputEvent
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FMotionEvent", OriginalName = "FMotionEvent"))
	void FMotionEvent_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FMotionEvent", OriginalName = "FMotionEvent"))
	void FMotionEvent_Overload2(uint32 InUserIndex, const FVector& InTilt, const FVector& InRotationRate, const FVector& InGravity, const FVector& InAcceleration);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "GetUserIndex", OriginalName = "GetUserIndex"))
	uint32 MotionEvent_GetUserIndex();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetTilt();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetRotationRate();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetGravity();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetAcceleration();

};

/**
* FNavigationEvent describes a navigation action (Left, Right, Up, Down)
* It is passed to event handlers dealing with navigation.
*/
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_NavigationEvent : public UNePyNoExportType_InputEvent
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	void FNavigationEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	EUINavigation GetNavigationType();

	UFUNCTION(meta=(bFriendFunction = false))
	ENavigationGenesis GetNavigationGenesis();

};

/**
* A point or direction FVector in 3d space.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Vector.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Vector : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float X;

	UPROPERTY()
	float Y;

	UPROPERTY()
	float Z;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FVector", OriginalName = "FVector"))
	void FVector_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FVector", OriginalName = "FVector"))
	void FVector_Overload2(float InF);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FVector", OriginalName = "FVector"))
	void FVector_Overload3(float InX, float InY, float InZ);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector CrossProduct(const FVector& A, const FVector& B);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DotProduct(const FVector& A, const FVector& B);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FVector __add___Overload1(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FVector __sub___Overload1(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FVector __sub___Overload2(float Bias);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FVector __add___Overload2(float Bias);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector __mul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FVector __div___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector __mul___Overload2(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FVector __div___Overload2(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FVector& V, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool AllComponentsEqual(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FVector __iadd__(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FVector __isub__(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FVector __imul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FVector __idiv___Overload1(float V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FVector __imul___Overload2(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FVector __idiv___Overload2(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	float Component(int32 Index);

	UFUNCTION(meta=(bFriendFunction = false))
	void Set(float InX, float InY, float InZ);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMax();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetAbsMax();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMin();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetAbsMin();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector ComponentMin(const FVector& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector ComponentMax(const FVector& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetAbs();

	UFUNCTION(meta=(bFriendFunction = false))
	float Size();

	UFUNCTION(meta=(bFriendFunction = false))
	float SizeSquared();

	UFUNCTION(meta=(bFriendFunction = false))
	float Size2D();

	UFUNCTION(meta=(bFriendFunction = false))
	float SizeSquared2D();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool IsNearlyZero(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsZero();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_LengthSquaredTolerance = "KINDA_SMALL_NUMBER"))
	bool IsUnit(float LengthSquaredTolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsNormalized();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	bool Normalize(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetUnsafeNormal();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FVector GetSafeNormal(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FVector GetSafeNormal2D(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetSignVector();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector Projection();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetUnsafeNormal2D();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector BoundToCube(float Radius);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector BoundToBox(const FVector& Min, FVector Max);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetClampedToSize(float Min, float Max);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetClampedToSize2D(float Min, float Max);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetClampedToMaxSize(float MaxSize);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetClampedToMaxSize2D(float MaxSize);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Radius = "MAX_int16"))
	void AddBounded(const FVector& V, float Radius);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector Reciprocal();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool IsUniform(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector MirrorByVector(const FVector& MirrorNormal);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector MirrorByPlane(const FPlane& Plane);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector RotateAngleAxis(float AngleDeg, const FVector& Axis);

	UFUNCTION(meta=(bFriendFunction = false))
	float CosineAngle2D(FVector B);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector ProjectOnTo(const FVector& A);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector ProjectOnToNormal(const FVector& Normal);

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator ToOrientationRotator();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat ToOrientationQuat();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Rotation();

	UFUNCTION(meta=(bFriendFunction = false))
	void FindBestAxisVectors(FVector& Axis1, FVector& Axis2);

	UFUNCTION(meta=(bFriendFunction = false))
	void UnwindEuler();

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D UnitCartesianToSpherical();

	UFUNCTION(meta=(bFriendFunction = false))
	float HeadingAngle();

	UFUNCTION(meta=(bFriendFunction = false))
	static void CreateOrthonormalBasis(FVector& XAxis, FVector& YAxis, FVector& ZAxis);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool PointsAreSame(const FVector& P, const FVector& Q);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool PointsAreNear(const FVector& Point1, const FVector& Point2, float Dist);

	UFUNCTION(meta=(bFriendFunction = false))
	static float PointPlaneDist(const FVector& Point, const FVector& PlaneBase, const FVector& PlaneNormal);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "PointPlaneProject", OriginalName = "PointPlaneProject"))
	static FVector PointPlaneProject_Overload1(const FVector& Point, const FPlane& Plane);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "PointPlaneProject", OriginalName = "PointPlaneProject"))
	static FVector PointPlaneProject_Overload2(const FVector& Point, const FVector& A, const FVector& B, const FVector& C);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "PointPlaneProject", OriginalName = "PointPlaneProject"))
	static FVector PointPlaneProject_Overload3(const FVector& Point, const FVector& PlaneBase, const FVector& PlaneNormal);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector VectorPlaneProject(const FVector& V, const FVector& PlaneNormal);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Dist(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Distance(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DistXY(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Dist2D(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DistSquared(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DistSquaredXY(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DistSquared2D(const FVector& V1, const FVector& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float BoxPushOut(const FVector& Normal, const FVector& Size);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_ParallelCosineThreshold = "THRESH_NORMALS_ARE_PARALLEL"))
	static bool Parallel(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_ParallelCosineThreshold = "THRESH_NORMALS_ARE_PARALLEL"))
	static bool Coincident(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_OrthogonalCosineThreshold = "THRESH_NORMALS_ARE_ORTHOGONAL"))
	static bool Orthogonal(const FVector& Normal1, const FVector& Normal2, float OrthogonalCosineThreshold);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_ParallelCosineThreshold = "THRESH_NORMALS_ARE_PARALLEL"))
	static bool Coplanar(const FVector& Base1, const FVector& Normal1, const FVector& Base2, const FVector& Normal2, float ParallelCosineThreshold);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector RadiansToDegrees(const FVector& RadVector);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector DegreesToRadians(const FVector& DegVector);

};

/**
* A plane definition in 3D space.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Plane.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Plane : public UNePyNoExportType_Vector
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float W;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPlane", OriginalName = "FPlane"))
	void FPlane_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPlane", OriginalName = "FPlane"))
	void FPlane_Overload2(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPlane", OriginalName = "FPlane"))
	void FPlane_Overload3(float InX, float InY, float InZ, float InW);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPlane", OriginalName = "FPlane"))
	void FPlane_Overload4(FVector InNormal, float InW);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPlane", OriginalName = "FPlane"))
	void FPlane_Overload5(FVector InBase, const FVector& InNormal);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPlane", OriginalName = "FPlane"))
	void FPlane_Overload6(FVector A, FVector B, FVector C);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetOrigin();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetNormal();

	UFUNCTION(meta=(bFriendFunction = false))
	float PlaneDot(const FVector& P);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "Normalize", OriginalName = "Normalize", CPP_Default_Tolerance = "SMALL_NUMBER"))
	bool Plane_Normalize(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FPlane Flip();

	UFUNCTION(meta=(bFriendFunction = false))
	FPlane TransformBy(const FMatrix& M);

	UFUNCTION(meta=(bFriendFunction = false))
	FPlane TransformByUsingAdjointT(const FMatrix& M, float DetM, const FMatrix& TA);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__eq__", OriginalName = "operator=="))
	bool Plane___eq__(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__ne__", OriginalName = "operator!="))
	bool Plane___ne__(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "Equals", OriginalName = "Equals", CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Plane_Equals(const FPlane& V, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FPlane Plane___add__(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FPlane Plane___sub__(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FPlane Plane___div__(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FPlane Plane___mul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FPlane Plane___mul___Overload2(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__iadd__", OriginalName = "operator+="))
	FPlane Plane___iadd__(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__isub__", OriginalName = "operator-="))
	FPlane Plane___isub__(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FPlane Plane___imul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FPlane Plane___imul___Overload2(const FPlane& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FPlane Plane___idiv__(float V);

};

/**
* FPointerEvent describes a mouse or touch action (e.g. Press, Release, Move, etc).
* It is passed to event handlers dealing with pointer-based input.
*/
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_PointerEvent : public UNePyNoExportType_InputEvent
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetScreenSpacePosition();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetLastScreenSpacePosition();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetCursorDelta();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsMouseButtonDown(FKey MouseButton);

	UFUNCTION(meta=(bFriendFunction = false))
	FKey GetEffectingButton();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetWheelDelta();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "GetUserIndex", OriginalName = "GetUserIndex"))
	int32 PointerEvent_GetUserIndex();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 GetPointerIndex();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 GetTouchpadIndex();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetTouchForce();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsTouchEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsTouchForceChangedEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsTouchFirstMoveEvent();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetGestureType();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetGestureDelta();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsDirectionInvertedFromDevice();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ToText", OriginalName = "ToText"))
	FText PointerEvent_ToText();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "IsPointerEvent", OriginalName = "IsPointerEvent"))
	bool PointerEvent_IsPointerEvent();

};

/**
* This identifies an object as a 'primary' asset that can be searched for by the AssetManager and used in various tools
* @note The full C++ class is located here: Engine/Source/Runtime/CoreUObject/Public/UObject/PrimaryAssetId.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_PrimaryAssetId : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	FPrimaryAssetType PrimaryAssetType;

	UPROPERTY()
	FName PrimaryAssetName;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPrimaryAssetId", OriginalName = "FPrimaryAssetId"))
	void FPrimaryAssetId_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPrimaryAssetId", OriginalName = "FPrimaryAssetId"))
	void FPrimaryAssetId_Overload2(FPrimaryAssetType InAssetType, FName InAssetName);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ParseTypeAndName", OriginalName = "ParseTypeAndName"))
	static FPrimaryAssetId ParseTypeAndName_Overload1(FName TypeAndName);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ParseTypeAndName", OriginalName = "ParseTypeAndName"))
	static FPrimaryAssetId ParseTypeAndName_Overload2(const FString& TypeAndName);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPrimaryAssetId", OriginalName = "FPrimaryAssetId"))
	void FPrimaryAssetId_Overload3(const FString& TypeAndName);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	static FPrimaryAssetId FromString(const FString& String);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FPrimaryAssetId& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FPrimaryAssetId& Other);

	UFUNCTION(meta=(bFriendFunction = true))
	uint32 GetTypeHash(const FPrimaryAssetId& InSelf);

};

/**
* A type of primary asset, used by the Asset Manager system.
* @note The full C++ class is located here: Engine/Source/Runtime/CoreUObject/Public/UObject/PrimaryAssetId.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_PrimaryAssetType : public UObject
{
	GENERATED_BODY()

	public:
	// The Type of this object, by default its base class's name
	UPROPERTY(SaveGame)
	FName Name;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPrimaryAssetType", OriginalName = "FPrimaryAssetType"))
	void FPrimaryAssetType_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FPrimaryAssetType", OriginalName = "FPrimaryAssetType"))
	void FPrimaryAssetType_Overload2(FName InName);

	UFUNCTION(meta=(bFriendFunction = false))
	FName GetName();

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FPrimaryAssetType& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FPrimaryAssetType& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = true))
	uint32 GetTypeHash(const FPrimaryAssetType& InSelf);

};

/**
* Quaternion.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Quat.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Quat : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float X;

	UPROPERTY()
	float Y;

	UPROPERTY()
	float Z;

	UPROPERTY()
	float W;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FQuat", OriginalName = "FQuat"))
	void FQuat_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FQuat", OriginalName = "FQuat"))
	void FQuat_Overload2(float InX, float InY, float InZ, float InW);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FQuat", OriginalName = "FQuat"))
	void FQuat_Overload3(const FMatrix& M);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FQuat", OriginalName = "FQuat"))
	void FQuat_Overload4(const FRotator& R);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FQuat", OriginalName = "FQuat"))
	void FQuat_Overload5(FVector Axis, float AngleRad);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FQuat __add__(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FQuat __iadd__(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-"))
	FQuat __sub__(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FQuat& Q, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	bool IsIdentity(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FQuat __isub__(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FQuat __mul___Overload1(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FQuat __imul___Overload1(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector __mul___Overload2(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FMatrix __mul___Overload3(const FMatrix& M);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FQuat __imul___Overload2(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FQuat __mul___Overload4(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator/="))
	FQuat __idiv__(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator/"))
	FQuat __div__(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat MakeFromEuler(const FVector& Euler);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector Euler();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	void Normalize(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FQuat GetNormalized(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsNormalized();

	UFUNCTION(meta=(bFriendFunction = false))
	float Size();

	UFUNCTION(meta=(bFriendFunction = false))
	float SizeSquared();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetAngle();

	UFUNCTION(meta=(bFriendFunction = false))
	void ToSwingTwist(const FVector& InTwistAxis, FQuat& OutSwing, FQuat& OutTwist);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetTwistAngle(const FVector& TwistAxis);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector RotateVector(FVector V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector UnrotateVector(FVector V);

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat Log();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat Exp();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat Inverse();

	UFUNCTION(meta=(bFriendFunction = false))
	void EnforceShortestArcWith(const FQuat& OtherQuat);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetAxisX();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetAxisY();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetAxisZ();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetForwardVector();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetRightVector();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetUpVector();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector Vector();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Rotator();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetRotationAxis();

	UFUNCTION(meta=(bFriendFunction = false))
	float AngularDistance(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat FindBetween(const FVector& Vector1, const FVector& Vector2);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat FindBetweenNormals(const FVector& Normal1, const FVector& Normal2);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat FindBetweenVectors(const FVector& Vector1, const FVector& Vector2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Error(const FQuat& Q1, const FQuat& Q2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float ErrorAutoNormalize(const FQuat& A, const FQuat& B);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat FastLerp(const FQuat& A, const FQuat& B, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat FastBilerp(const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, float FracX, float FracY);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat Slerp_NotNormalized(const FQuat& Quat1, const FQuat& Quat2, float Slerp);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat Slerp(const FQuat& Quat1, const FQuat& Quat2, float Slerp);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat SlerpFullPath_NotNormalized(const FQuat& quat1, const FQuat& quat2, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat SlerpFullPath(const FQuat& quat1, const FQuat& quat2, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat Squad(const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	static FQuat SquadFullPath(const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	static void CalcTangents(const FQuat& PrevP, const FQuat& P, const FQuat& NextP, float Tension, FQuat& OutTan);

};

/**
* Thread-safe random number generator that can be manually seeded.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/RandomStream.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_RandomStream : public UObject
{
	GENERATED_BODY()

	public:
	// Holds the initial seed.
	UPROPERTY(SaveGame)
	int32 InitialSeed;

	// Holds the current seed.
	UPROPERTY()
	int32 Seed;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRandomStream", OriginalName = "FRandomStream"))
	void FRandomStream_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRandomStream", OriginalName = "FRandomStream"))
	void FRandomStream_Overload2(int32 InSeed);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRandomStream", OriginalName = "FRandomStream"))
	void FRandomStream_Overload3(FName InName);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "Initialize", OriginalName = "Initialize"))
	void Initialize_Overload1(int32 InSeed);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "Initialize", OriginalName = "Initialize"))
	void Initialize_Overload2(FName InName);

	UFUNCTION(meta=(bFriendFunction = false))
	void Reset();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetInitialSeed();

	UFUNCTION(meta=(bFriendFunction = false))
	void GenerateNewSeed();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetFraction();

	UFUNCTION(meta=(bFriendFunction = false))
	uint32 GetUnsignedInt();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetUnitVector();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 GetCurrentSeed();

	UFUNCTION(meta=(bFriendFunction = false))
	float FRand();

	UFUNCTION(meta=(bFriendFunction = false))
	int32 RandHelper(int32 A);

	UFUNCTION(meta=(bFriendFunction = false))
	int32 RandRange(int32 Min, int32 Max);

	UFUNCTION(meta=(bFriendFunction = false))
	float FRandRange(float InMin, float InMax);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector VRand();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "VRandCone", OriginalName = "VRandCone"))
	FVector VRandCone_Overload1(const FVector& Dir, float ConeHalfAngleRad);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "VRandCone", OriginalName = "VRandCone"))
	FVector VRandCone_Overload2(const FVector& Dir, float HorizontalConeHalfAngleRad, float VerticalConeHalfAngleRad);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

};

/**
* An orthogonal rotation in 3d space.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Rotator.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Rotator : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float Pitch;

	UPROPERTY()
	float Yaw;

	UPROPERTY()
	float Roll;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRotator", OriginalName = "FRotator"))
	void FRotator_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRotator", OriginalName = "FRotator"))
	void FRotator_Overload2(float InF);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRotator", OriginalName = "FRotator"))
	void FRotator_Overload3(float InPitch, float InYaw, float InRoll);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FRotator", OriginalName = "FRotator"))
	void FRotator_Overload4(const FQuat& Quat);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FRotator __add__(const FRotator& R);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-"))
	FRotator __sub__(const FRotator& R);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator*"))
	FRotator __mul__(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator*="))
	FRotator __imul__(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FRotator& R);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FRotator& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FRotator __iadd__(const FRotator& R);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FRotator __isub__(const FRotator& R);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool IsNearlyZero(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsZero();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FRotator& R, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Add(float DeltaPitch, float DeltaYaw, float DeltaRoll);

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator GetInverse();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator GridSnap(const FRotator& RotGrid);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector Vector();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat Quaternion();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector Euler();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector RotateVector(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector UnrotateVector(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Clamp();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator GetNormalized();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator GetDenormalized();

	UFUNCTION(meta=(bFriendFunction = false))
	void Normalize();

	UFUNCTION(meta=(bFriendFunction = false))
	void GetWindingAndRemainder(FRotator& Winding, FRotator& Remainder);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetManhattanDistance(const FRotator& Rotator);

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator GetEquivalentRotator();

	UFUNCTION(meta=(bFriendFunction = false))
	void SetClosestToMe(FRotator& MakeClosest);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false))
	static float ClampAxis(float Angle);

	UFUNCTION(meta=(bFriendFunction = false))
	static float NormalizeAxis(float Angle);

	UFUNCTION(meta=(bFriendFunction = false))
	static uint8 CompressAxisToByte(float Angle);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DecompressAxisFromByte(uint8 Angle);

	UFUNCTION(meta=(bFriendFunction = false))
	static uint16 CompressAxisToShort(float Angle);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DecompressAxisFromShort(uint16 Angle);

	UFUNCTION(meta=(bFriendFunction = false))
	static FRotator MakeFromEuler(const FVector& Euler);

};

// A Slate color can be a directly specified value, or the color can be pulled from a WidgetStyle.
UCLASS(meta=(Package = "/Script/SlateCore"))
class UNePyNoExportType_SlateColor : public UObject
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSlateColor", OriginalName = "FSlateColor"))
	void FSlateColor_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSlateColor", OriginalName = "FSlateColor"))
	void FSlateColor_Overload2(const FLinearColor& InColor);

	UFUNCTION(meta=(bFriendFunction = false))
	FLinearColor GetSpecifiedColor();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsColorSpecified();

	UFUNCTION(meta=(bFriendFunction = false))
	void Unlink();

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FSlateColor& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FSlateColor& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	static FSlateColor UseForeground();

	UFUNCTION(meta=(bFriendFunction = false))
	static FSlateColor UseSubduedForeground();

};

/**
* A struct that contains a string reference to an object, either a top level asset or a subobject.
* @note The full C++ class is located here: Engine/Source/Runtime/CoreUObject/Public/UObject/SoftObjectPath.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_SoftObjectPath : public UObject
{
	GENERATED_BODY()

	public:
	// Asset path, patch to a top level object in a package
	UPROPERTY()
	FName AssetPathName;

	// Optional FString for subobject within an asset
	UPROPERTY()
	FString SubPathString;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftObjectPath", OriginalName = "FSoftObjectPath"))
	void FSoftObjectPath_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftObjectPath", OriginalName = "FSoftObjectPath"))
	void FSoftObjectPath_Overload2(const FSoftObjectPath& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftObjectPath", OriginalName = "FSoftObjectPath"))
	void FSoftObjectPath_Overload3(const FString& Path);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftObjectPath", OriginalName = "FSoftObjectPath"))
	void FSoftObjectPath_Overload4(FName InAssetPathName, FString InSubPathString);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftObjectPath", OriginalName = "FSoftObjectPath"))
	void FSoftObjectPath_Overload5(const UObject* InObject);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "ToString", OriginalName = "ToString"))
	FString ToString_Overload1();

	UFUNCTION(meta=(bFriendFunction = false))
	FName GetAssetPathName();

	UFUNCTION(meta=(bFriendFunction = false))
	FString GetAssetPathString();

	UFUNCTION(meta=(bFriendFunction = false))
	FString GetSubPathString();

	UFUNCTION(meta=(bFriendFunction = false))
	FString GetLongPackageName();

	UFUNCTION(meta=(bFriendFunction = false))
	FString GetAssetName();

	UFUNCTION(meta=(bFriendFunction = false))
	void SetPath(const FString& Path);

	UFUNCTION(meta=(bFriendFunction = false))
	UObject* ResolveObject();

	UFUNCTION(meta=(bFriendFunction = false))
	void Reset();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsNull();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsAsset();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsSubobject();

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FSoftObjectPath& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FSoftObjectPath& Other);

	UFUNCTION(meta=(bFriendFunction = true))
	uint32 GetTypeHash(const FSoftObjectPath& InSelf);

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 GetCurrentTag();

	UFUNCTION(meta=(bFriendFunction = false))
	static int32 InvalidateTag();

	UFUNCTION(meta=(bFriendFunction = false))
	static FSoftObjectPath GetOrCreateIDForObject(const UObject* Object);

	UFUNCTION(meta=(bFriendFunction = false))
	static void AddPIEPackageName(FName NewPIEPackageName);

	UFUNCTION(meta=(bFriendFunction = false))
	static void ClearPIEPackageNames();

};

/**
* A struct that contains a string reference to a class, can be used to make soft references to classes.
* @note The full C++ class is located here: Engine/Source/Runtime/CoreUObject/Public/UObject/SoftObjectPath.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_SoftClassPath : public UNePyNoExportType_SoftObjectPath
{
	GENERATED_BODY()

	public:
	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftClassPath", OriginalName = "FSoftClassPath"))
	void FSoftClassPath_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftClassPath", OriginalName = "FSoftClassPath"))
	void FSoftClassPath_Overload2(const FSoftClassPath& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftClassPath", OriginalName = "FSoftClassPath"))
	void FSoftClassPath_Overload3(const FString& PathString);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FSoftClassPath", OriginalName = "FSoftClassPath"))
	void FSoftClassPath_Overload4(const UClass* InClass);

	UFUNCTION(meta=(bFriendFunction = false))
	UClass* ResolveClass();

	UFUNCTION(meta=(bFriendFunction = false))
	static FSoftClassPath GetOrCreateIDForClass(const UClass* InClass);

};

/**
* Transform composed of Quat/Translation/Scale.
* @note This is implemented in either TransformVectorized.h or TransformNonVectorized.h depending on the platform.
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Transform : public UObject
{
	GENERATED_BODY()

	public:
	// Rotation of this transformation, as a quaternion.
	UPROPERTY(SaveGame)
	FQuat Rotation;

	// Translation of this transformation, as a vector.
	UPROPERTY(SaveGame)
	FVector Translation;

	// 3D scale (always applied in local space) as a vector.
	UPROPERTY(SaveGame)
	FVector Scale3D;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform"))
	void FTransform_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform"))
	void FTransform_Overload2(const FVector& InTranslation);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform"))
	void FTransform_Overload3(const FQuat& InRotation);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform"))
	void FTransform_Overload4(const FRotator& InRotation);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform", CPP_Default_InScale3D = "FVector::OneVector"))
	void FTransform_Overload5(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform", CPP_Default_InScale3D = "FVector::OneVector"))
	void FTransform_Overload6(const FRotator& InRotation, const FVector& InTranslation, const FVector& InScale3D);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform"))
	void FTransform_Overload7(const FMatrix& InMatrix);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FTransform", OriginalName = "FTransform"))
	void FTransform_Overload8(const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InTranslation);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToHumanReadableString();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix ToMatrixWithScale();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix ToInverseMatrixWithScale();

	UFUNCTION(meta=(bFriendFunction = false))
	FTransform Inverse();

	UFUNCTION(meta=(bFriendFunction = false))
	FMatrix ToMatrixNoScale();

	UFUNCTION(meta=(bFriendFunction = false))
	void Blend(const FTransform& Atom1, const FTransform& Atom2, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false))
	void BlendWith(const FTransform& OtherAtom, float Alpha);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FTransform __add__(const FTransform& Atom);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FTransform __iadd__(const FTransform& Atom);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FTransform __mul___Overload1(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	void __imul___Overload1(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FTransform __mul___Overload2(const FQuat& Other);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	void __imul___Overload2(const FQuat& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	static bool AnyHasNegativeScale(const FVector& InScale3D, const FVector& InOtherScale3D);

	UFUNCTION(meta=(bFriendFunction = false))
	void ScaleTranslation(const FVector& InScale3D);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	void RemoveScaling(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMaximumAxisScale();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMinimumAxisScale();

	UFUNCTION(meta=(bFriendFunction = false))
	FTransform GetRelativeTransform(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	FTransform GetRelativeTransformReverse(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetToRelativeTransform(const FTransform& ParentTransform);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 TransformFVector4(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 TransformFVector4NoScale(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector TransformPosition(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector TransformPositionNoScale(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector InverseTransformPosition(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector InverseTransformPositionNoScale(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector TransformVector(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector TransformVectorNoScale(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector InverseTransformVector(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector InverseTransformVectorNoScale(const FVector& V);

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat TransformRotation(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat InverseTransformRotation(const FQuat& Q);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "GetScaled", OriginalName = "GetScaled"))
	FTransform GetScaled_Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "GetScaled", OriginalName = "GetScaled"))
	FTransform GetScaled_Overload2(FVector Scale);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	static FVector GetSafeScaleReciprocal(const FVector& InScale, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetLocation();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Rotator();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetDeterminant();

	UFUNCTION(meta=(bFriendFunction = false))
	void SetLocation(const FVector& Origin);

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsValid();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	static bool AreRotationsEqual(const FTransform& A, const FTransform& B, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	static bool AreTranslationsEqual(const FTransform& A, const FTransform& B, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	static bool AreScale3DsEqual(const FTransform& A, const FTransform& B, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool RotationEquals(const FTransform& Other, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool TranslationEquals(const FTransform& Other, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Scale3DEquals(const FTransform& Other, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FTransform& Other, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool EqualsNoScale(const FTransform& Other, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetComponents(const FQuat& InRotation, const FVector& InTranslation, const FVector& InScale3D);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetIdentity();

	UFUNCTION(meta=(bFriendFunction = false))
	void MultiplyScale3D(const FVector& Scale3DMultiplier);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetTranslation(const FVector& NewTranslation);

	UFUNCTION(meta=(bFriendFunction = false))
	void CopyTranslation(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	void ConcatenateRotation(const FQuat& DeltaRotation);

	UFUNCTION(meta=(bFriendFunction = false))
	void AddToTranslation(const FVector& DeltaTranslation);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector AddTranslations(const FTransform& A, const FTransform& B);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector SubtractTranslations(const FTransform& A, const FTransform& B);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetRotation(const FQuat& NewRotation);

	UFUNCTION(meta=(bFriendFunction = false))
	void CopyRotation(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetScale3D(const FVector& NewScale3D);

	UFUNCTION(meta=(bFriendFunction = false))
	void CopyScale3D(const FTransform& Other);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetTranslationAndScale3D(const FVector& NewTranslation, const FVector& NewScale3D);

	UFUNCTION(meta=(bFriendFunction = false))
	void Accumulate(const FTransform& SourceAtom);

	UFUNCTION(meta=(bFriendFunction = false))
	void NormalizeRotation();

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsRotationNormalized();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat GetRotation();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetTranslation();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector GetScale3D();

	UFUNCTION(meta=(bFriendFunction = false))
	void CopyRotationPart(const FTransform& SrcBA);

	UFUNCTION(meta=(bFriendFunction = false))
	void CopyTranslationAndScale3D(const FTransform& SrcBA);

	UFUNCTION(meta=(bFriendFunction = false))
	void SetFromMatrix(const FMatrix& InMatrix);

};

/**
* A vector in 2-D space composed of components (X, Y) with floating point precision.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Vector2D.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Vector2D : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float X;

	UPROPERTY()
	float Y;

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FVector2D", OriginalName = "FVector2D"))
	void FVector2D_Overload1();

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FVector2D", OriginalName = "FVector2D"))
	void FVector2D_Overload2(float InX, float InY);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "FVector2D", OriginalName = "FVector2D"))
	void FVector2D_Overload3(float InF);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FVector2D __add___Overload1(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FVector2D __sub___Overload1(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector2D __mul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FVector2D __div___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__add__", OriginalName = "operator+"))
	FVector2D __add___Overload2(float A);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__sub__", OriginalName = "operator-"))
	FVector2D __sub___Overload2(float A);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector2D __mul___Overload2(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FVector2D __div___Overload2(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator<"))
	bool __lt__(const FVector2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator>"))
	bool __gt__(const FVector2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator<="))
	bool __le__(const FVector2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator>="))
	bool __ge__(const FVector2D& Other);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FVector2D __iadd__(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FVector2D __isub__(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FVector2D __imul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FVector2D __idiv___Overload1(float V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FVector2D __imul___Overload2(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__idiv__", OriginalName = "operator/="))
	FVector2D __idiv___Overload2(const FVector2D& V);

	UFUNCTION(meta=(bFriendFunction = false))
	float Component(int32 Index);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DotProduct(const FVector2D& A, const FVector2D& B);

	UFUNCTION(meta=(bFriendFunction = false))
	static float DistSquared(const FVector2D& V1, const FVector2D& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float Distance(const FVector2D& V1, const FVector2D& V2);

	UFUNCTION(meta=(bFriendFunction = false))
	static float CrossProduct(const FVector2D& A, const FVector2D& B);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D Max(const FVector2D& A, const FVector2D& B);

	UFUNCTION(meta=(bFriendFunction = false))
	static FVector2D Min(const FVector2D& A, const FVector2D& B);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FVector2D& V, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	void Set(float InX, float InY);

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMax();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetAbsMax();

	UFUNCTION(meta=(bFriendFunction = false))
	float GetMin();

	UFUNCTION(meta=(bFriendFunction = false))
	float Size();

	UFUNCTION(meta=(bFriendFunction = false))
	float SizeSquared();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetRotated(float AngleDeg);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FVector2D GetSafeNormal(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	void Normalize(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool IsNearlyZero(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	bool IsZero();

	UFUNCTION(meta=(bFriendFunction = false))
	FIntPoint IntPoint();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D RoundToVector();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D ClampAxes(float MinAxisVal, float MaxAxisVal);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetSignVector();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector2D GetAbs();

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false))
	FVector SphericalToUnitCartesian();

};

/**
* A 4-D homogeneous vector.
* @note The full C++ class is located here: Engine/Source/Runtime/Core/Public/Math/Vector4.h
*/
UCLASS(meta=(Package = "/Script/CoreUObject"))
class UNePyNoExportType_Vector4 : public UObject
{
	GENERATED_BODY()

	public:
	UPROPERTY()
	float X;

	UPROPERTY()
	float Y;

	UPROPERTY()
	float Z;

	UPROPERTY()
	float W;

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_InX = "0.0f", CPP_Default_InY = "0.0f", CPP_Default_InZ = "0.0f", CPP_Default_InW = "1.0f"))
	void FVector4(float InX, float InY, float InZ, float InW);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+"))
	FVector4 __add__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator+="))
	FVector4 __iadd__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-"))
	FVector4 __sub__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator-="))
	FVector4 __isub__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector4 __mul___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FVector4 __div___Overload1(float Scale);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__div__", OriginalName = "operator/"))
	FVector4 __div___Overload2(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__mul__", OriginalName = "operator*"))
	FVector4 __mul___Overload2(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FVector4 __imul___Overload1(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator/="))
	FVector4 __idiv__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, ScriptName = "__imul__", OriginalName = "operator*="))
	FVector4 __imul___Overload2(float S);

	UFUNCTION(meta=(bFriendFunction = true))
	float Dot3(const FVector4& InSelf, const FVector4& V2);

	UFUNCTION(meta=(bFriendFunction = true))
	float Dot4(const FVector4& InSelf, const FVector4& V2);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator=="))
	bool __eq__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, OriginalName = "operator!="))
	bool __ne__(const FVector4& V);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool Equals(const FVector4& V, float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_LengthSquaredTolerance = "KINDA_SMALL_NUMBER"))
	bool IsUnit3(float LengthSquaredTolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FString ToString();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "SMALL_NUMBER"))
	FVector4 GetSafeNormal(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 GetUnsafeNormal3();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator ToOrientationRotator();

	UFUNCTION(meta=(bFriendFunction = false))
	FQuat ToOrientationQuat();

	UFUNCTION(meta=(bFriendFunction = false))
	FRotator Rotation();

	UFUNCTION(meta=(bFriendFunction = false))
	void Set(float InX, float InY, float InZ, float InW);

	UFUNCTION(meta=(bFriendFunction = false))
	float Size3();

	UFUNCTION(meta=(bFriendFunction = false))
	float SizeSquared3();

	UFUNCTION(meta=(bFriendFunction = false))
	float Size();

	UFUNCTION(meta=(bFriendFunction = false))
	float SizeSquared();

	UFUNCTION(meta=(bFriendFunction = false))
	bool ContainsNaN();

	UFUNCTION(meta=(bFriendFunction = false, CPP_Default_Tolerance = "KINDA_SMALL_NUMBER"))
	bool IsNearlyZero3(float Tolerance);

	UFUNCTION(meta=(bFriendFunction = false))
	FVector4 Reflect3(const FVector4& Normal);

	UFUNCTION(meta=(bFriendFunction = false))
	void FindBestAxisVectors3(FVector4& Axis1, FVector4& Axis2);

};

#endif
