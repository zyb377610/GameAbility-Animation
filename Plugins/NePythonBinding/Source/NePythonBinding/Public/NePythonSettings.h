#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"

#include "NePythonSettings.generated.h"

UENUM()
enum class ENePyPropertyEditSpecifier : uint8
{
	NotEditable,
	EditAnywhere,
	EditInstanceOnly,
	EditDefaultsOnly,
	VisibleAnywhere UMETA(Hidden),
	VisibleInstanceOnly UMETA(Hidden),
	VisibleDefaultsOnly UMETA(Hidden),
};

UENUM()
enum class ENePyPropertyBlueprintSpecifier : uint8
{
	BlueprintHidden,
	BlueprintReadWrite,
	BlueprintReadOnly,
};

/**
 * Settings for NePythonBindings.
 */
UCLASS(Config=Engine, defaultconfig)
class NEPYTHONBINDING_API UNePythonSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UNePythonSettings();

#if WITH_EDITOR

	virtual FText GetSectionText() const override;

#endif

	/**
	 * 是否全局禁用旧式Subclassing。
	 */ 
	UPROPERTY(config, EditDefaultsOnly, AdvancedDisplay, Category=Subclassing, meta = (ConfigRestartRequired = true))
	bool bDisableOldStyleSubclassing;

	/* Whether ufunction()s should be BlueprintCallable by default without explicit BlueprintCallable specifier. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	bool bDefaultFunctionBlueprintCallable = true;

	/* Whether ustruct()s should be BlueprintType by default without explicit BlueprintType specifier. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	bool bDefaultStructBlueprintType = true;
	
	/* Whether uenum()s should be BlueprintType by default without explicit BlueprintType specifier. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	bool bDefaultEnumBlueprintType = true;

	/* Default Edit access specifier for uproperty()s without explicit Edit specifier on classes. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	ENePyPropertyEditSpecifier DefaultPropertyEditSpecifier = ENePyPropertyEditSpecifier::EditAnywhere;

	/* Default Edit access specifier for uproperty()s without explicit Edit specifier on structs. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	ENePyPropertyEditSpecifier DefaultPropertyEditSpecifierForStructs = ENePyPropertyEditSpecifier::EditAnywhere;

	/* Default Blueprint access specifier for uproperty()s without explicit Blueprint specifier on classes. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	ENePyPropertyBlueprintSpecifier DefaultPropertyBlueprintSpecifier = ENePyPropertyBlueprintSpecifier::BlueprintReadWrite;

	/* Default Blueprint access specifier for uproperty()s without explicit Blueprint specifier on structs. */
	UPROPERTY(Config, EditDefaultsOnly, Category = Subclassing, meta = (ConfigRestartRequired = true))
	ENePyPropertyBlueprintSpecifier DefaultPropertyBlueprintSpecifierForStructs = ENePyPropertyBlueprintSpecifier::BlueprintReadWrite;

};
