#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Script.h"
#include "Templates/Function.h"

/** 记录由Python层定义的说明符 */
class FNePySpecifier
{
public:
	/** 说明符用途 */
	enum EUsage : uint8
	{
		/** 仅针对编辑器起作用 */
		Usage_Editor,
		/** 在游戏中也起作用 */
		Usage_Runtime,
	};
	
	/** 说明符作用域*/
	enum EScope : uint8
	{
		/** 说明符可作用于任意地方 */
		Scope_Any,
		/** 说明符可作用于ue.uclass()中 */
		Scope_Class = 1 << 0,
		/** 说明符可作用于ue.ustruct()中 */
		Scope_Struct = 1 << 1,
		/** 说明符可作用于ue.uenum()中 */
		Scope_Enum = 1 << 2,
		/** 预留 */
		Scope_Interface = 1 << 3,
		/** 说明符可作用于ue.uproperty()和ue.ucomponent()中 */
		Scope_Property = 1 << 4,
		/** 说明符可作用于ue.ufunction()中 */
		Scope_Function = 1 << 5,
		/** 说明符可作用于ue.udelegate()中 */
		Scope_Delegate = 1 << 6,
		/** 说明符可作用于ue.uparam()中 */
		Scope_Param = 1 << 7,

		Scope_Types = Scope_Class | Scope_Struct | Scope_Enum | Scope_Interface,
		Scope_Props = Scope_Property | Scope_Delegate,
		Scope_Members = Scope_Props | Scope_Function,
		Scope_ClassOrProps = Scope_Class | Scope_Props,
		Scope_FunctionOrDelegate = Scope_Function | Scope_Delegate,
	};

	/** 获取说明符名称 */
	virtual FName GetName() const;

	/**
	 * 解析Python参数，创建说明符数组
	 * 注意：外部需要自行管理说明符生命周期
	 * 参数：
	 *  InPySpecifierPairs 由Python传来的说明符字典
	 *  InScope 说明符作用域
	 */
	static TArray<FNePySpecifier*> ParseSpecifiers(const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs, EScope InScope);

	/**
	 * 释放并清理说明符数组
	 */
	static void ReleaseSpecifiers(TArray<FNePySpecifier*>& Specifiers);

	/**
	 * 应用说明符
	 */
	static void ApplyToClass(const TArray<FNePySpecifier*>& Specifiers, UClass* InClass);
	static void ApplyToStruct(const TArray<FNePySpecifier*>& Specifiers, UScriptStruct* InStruct);
	static void ApplyToEnum(const TArray<FNePySpecifier*>& Specifiers, UEnum* InEnum);
	static void ApplyToParam(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp);
	static void ApplyToProperty(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp);
	static void ApplyToDelegate(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp);
	static void ApplyToComponent(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp, bool bIsOverride=false);
	static void ApplyToFunction(const TArray<FNePySpecifier*>& Specifiers, UFunction* InFunc);
	
protected:
	/** 说明符应用场合 */
	enum EApplyCase: uint8
	{
		ApplyCase_Class = 0,
		ApplyCase_Struct = 1,
		ApplyCase_Enum = 2,
		ApplyCase_Interface = 3,
		ApplyCase_Function = 4,
		ApplyCase_PropertyInClass = 5,
		ApplyCase_PropertyInStruct = 6,
		ApplyCase_DelegateProperty = 7,
		ApplyCase_ComponentProperty = 8,
		ApplyCase_OverrideComponentProperty = 9,

		ApplyCase_MAX = 10,
	};

	static void ApplyTo(const TArray<FNePySpecifier*>& Specifiers, UField* Field);
	static void ApplyToPropertyInternal(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp, EApplyCase InApplyCase);
	virtual void ApplyTo(UField* Field) {};
	virtual void ApplyTo(FField* Field) {};

#if WITH_EDITORONLY_DATA
	void SetMetaData(UField* Field, const FName& Key, const FString& Value);
	void SetMetaData(FField* Field, const FName& Key, const FString& Value);
	void RemoveMetaData(UField* Field, const FName& Key);
	void RemoveMetaData(FField* Field, const FName& Key);
#endif
	void AddClassFlags(UField* Field, EClassFlags ClassFlags);
	void RemoveClassFlags(UField* Field, EClassFlags ClassFlags);
	void AddPropertyFlags(FField* Field, EPropertyFlags PropFlags);
	void RemovePropertyFlags(FField* Field, EPropertyFlags PropFlags);
	void AddFunctionFlags(UField* Field, EFunctionFlags FuncFlags);
	void RemoveFunctionFlags(UField* Field, EFunctionFlags FuncFlags);

	static void PrintPythonTraceback();

public:
	FNePySpecifier() = delete;
	explicit FNePySpecifier(PyObject*) {}
	virtual ~FNePySpecifier() = default;
	
protected:
	template <typename T>
	struct TNePySpecifierSelfRegister;

private:
	struct FNePySpecifierCreator
	{
		TFunction<FNePySpecifier*(PyObject*)> CreateFunc;
		EUsage Usage;
		EScope AcceptScope;

		inline bool MatchScope(EScope InScope)
		{
			return (AcceptScope == Scope_Any) || (InScope & AcceptScope);
		}
	};

	static TMap<FName, FNePySpecifierCreator> SpecifierCreators;

public:
	static void InitializeSpecifierDefaults();

protected:

	static TUniquePtr<FNePySpecifier> CreateUniqueSpecifier(const FName& InSpecifierName);
	static FNePySpecifier* GetUniqueSpecifier(const FName& InSpecifierName);

	static void ApplyPropertyDefaultEditSpecifier(FProperty* InProp, EApplyCase InApplyCase);
	static void ApplyPropertyDefaultBlueprintSpecifier(FProperty* InProp, EApplyCase InApplyCase);

private:
	static TMap<FName, TUniquePtr<FNePySpecifier>> DefaultUniqueSpecifiers;

	static TMap<EApplyCase, FNePySpecifier*> DefaultPropertyEditSpecifiers;
	static TMap<EApplyCase, FNePySpecifier*> DefaultPropertyBlueprintSpecifiers;

	static FNePySpecifier* DefaultFunctionBlueprintCallableSpecifier;
	static FNePySpecifier* DefaultStructBlueprintTypeSpecifier;
	static FNePySpecifier* DefaultEnumBlueprintTypeSpecifier;
};
