/** NOTE:
 * 本文件的实现参考了以下几个文件
 * UhtDefaultSpecifiers.cs
 * UhtClassSpecifiers.cs
 * UhtPropertyMemberSpecifiers.cs
 * UhtFunctionSpecifiers.cs
 */
#include "NePySpecifiers.h"
#include <frameobject.h>
#include "NePyBase.h"
#include "NePyGeneratedClass.h"
#include "Runtime/Launch/Resources/Version.h"
#include "UObject/Field.h"
#include "UObject/Class.h"
#include "NePythonSettings.h"
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
#include "INotifyFieldValueChanged.h"
#endif


FName FNePySpecifier::GetName() const
{
	return NAME_None;
}

TArray<FNePySpecifier*> FNePySpecifier::ParseSpecifiers(const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs, EScope InScope)
{
	check(PyGILState_Check());

	auto ScopeName = [InScope]()
	{
		switch (InScope)
		{
		case Scope_Class:
			return "class";
		case Scope_Property:
			return "property";
		case Scope_Function:
			return "function";
		case Scope_Delegate:
			return "delegate property";
		case Scope_Param:
			return "param";
		default:
			break;
		}
		return "unknown";
	};

	TArray<FNePySpecifier*> OutSpecifiers;
	for (auto& Pair : InPySpecifierPairs)
	{
		const char* SpecifierNameStr = NePyString_AsString(Pair.Key);
		FName SpecifierName(SpecifierNameStr);
		FNePySpecifierCreator* Creator = SpecifierCreators.Find(SpecifierName);
		if (!Creator || !Creator->MatchScope(InScope))
		{
			PyErr_Format(PyExc_TypeError, "'%s' is not a valid unreal %s specifier", SpecifierNameStr, ScopeName());
			PrintPythonTraceback();
			continue;
		}

#if !WITH_EDITOR
		if (Creator->Usage == Usage_Editor)
		{
			continue;
		}
#endif

		FNePySpecifier* Specifier = Creator->CreateFunc(Pair.Value);
		OutSpecifiers.Add(Specifier);
	}

	return OutSpecifiers;
}

void FNePySpecifier::ReleaseSpecifiers(TArray<FNePySpecifier*>& Specifiers)
{
	for (FNePySpecifier* Specifier : Specifiers)
	{
		delete Specifier;
	}
	Specifiers.Empty();
}

void FNePySpecifier::ApplyTo(const TArray<FNePySpecifier*>& Specifiers, UField* Field)
{
	for (FNePySpecifier* Specifier : Specifiers)
	{
		Specifier->ApplyTo(Field);
	}
}

#if WITH_EDITORONLY_DATA
void FNePySpecifier::SetMetaData(UField* Field, const FName& Key, const FString& Value)
{
	if (!Value.IsEmpty())
	{
		Field->SetMetaData(Key, *Value);
	}
}

void FNePySpecifier::SetMetaData(FField* Field, const FName& Key, const FString& Value)
{
	if (!Value.IsEmpty())
	{
		Field->SetMetaData(Key, *Value);
	}
}

void FNePySpecifier::RemoveMetaData(UField* Field, const FName& Key)
{
	Field->RemoveMetaData(Key);
}

void FNePySpecifier::RemoveMetaData(FField* Field, const FName& Key)
{
	Field->RemoveMetaData(Key);
}
#else
#define SetMetaData(...)
#define RemoveMetaData(...)
#endif

void FNePySpecifier::AddClassFlags(UField* Field, EClassFlags ClassFlags)
{
	if (UClass* Class = Cast<UClass>(Field))
	{
		Class->ClassFlags |= ClassFlags;
	}
}

void FNePySpecifier::RemoveClassFlags(UField* Field, EClassFlags ClassFlags)
{
	if (UClass* Class = Cast<UClass>(Field))
	{
		Class->ClassFlags &= ~ClassFlags;
	}
}

void FNePySpecifier::AddPropertyFlags(FField* Field, EPropertyFlags PropFlags)
{
	if (FProperty* Prop = CastField<FProperty>(Field))
	{
		Prop->PropertyFlags |= PropFlags;
	}
}

void FNePySpecifier::RemovePropertyFlags(FField* Field, EPropertyFlags PropFlags)
{
	if (FProperty* Prop = CastField<FProperty>(Field))
	{
		Prop->PropertyFlags &= ~PropFlags;
	}
}

void FNePySpecifier::AddFunctionFlags(UField* Field, EFunctionFlags FuncFlags)
{
	if (UFunction* Func = Cast<UFunction>(Field))
	{
		Func->FunctionFlags |= FuncFlags;
	}
}

void FNePySpecifier::RemoveFunctionFlags(UField* Field, EFunctionFlags FuncFlags)
{
	if (UFunction* Func = Cast<UFunction>(Field))
	{
		Func->FunctionFlags &= ~FuncFlags;
	}
}

void FNePySpecifier::PrintPythonTraceback()
{
	if (PyFrameObject* FrameObject = PyEval_GetFrame())
	{
		PyTraceBack_Here(FrameObject);
	}
	PyErr_Print();
}

TMap<FName, FNePySpecifier::FNePySpecifierCreator> FNePySpecifier::SpecifierCreators;


/** 带有一个FString类型值的说明符 */
class FNePySpecifierStringValue : public FNePySpecifier
{
public:
	explicit FNePySpecifierStringValue(PyObject* InPyValue)
		: FNePySpecifier(InPyValue)
	{
		if (NePyBase::ToCpp(InPyValue, Value))
		{
			Value.TrimStartAndEndInline();
		}
	}

protected:
	FString Value;

	friend class FNePySpecifier;
};

/** 单个字符串，或是一个字符串列表 */
class FNePySpecifierStringListValueBase : public FNePySpecifier
{
public:
	using FNePySpecifier::FNePySpecifier;

protected:
	TArray<FString> ParseToList(PyObject* InPyValue)
	{
		PyObject* (*GetItemFunc)(PyObject*, Py_ssize_t) = nullptr;
		if (PyTuple_Check(InPyValue))
		{
			GetItemFunc = PyTuple_GetItem;
		}
		else if (PyList_Check(InPyValue))
		{
			GetItemFunc = PyList_GetItem;
		}

		TArray<FString> TempValues;
		if (GetItemFunc)
		{
			int32 Length = (int32)PySequence_Size(InPyValue);
			FString TempValue;
			for (int32 Index = 0; Index < Length; ++Index)
			{
				PyObject* TempPyValue = GetItemFunc(InPyValue, Index);
				if (NePyBase::ToCpp(TempPyValue, TempValue))
				{
					TempValue.TrimStartAndEndInline();
					if (!TempValue.IsEmpty())
					{
						TempValues.Add(TempValue);
					}
				}
			}
		}

		return TempValues;
	}

protected:
	FString Value;

	friend class FNePySpecifier;
};

/** 单个字符串，或是一个字符串列表（以空格作为分隔符） */
class FNePySpecifierStringListValue : public FNePySpecifierStringListValueBase
{
public:
	explicit FNePySpecifierStringListValue(PyObject* InPyValue)
		: FNePySpecifierStringListValueBase(InPyValue)
	{
		if (NePyBase::ToCpp(InPyValue, Value))
		{
			Value.TrimStartAndEndInline();
			return;
		}

		TArray<FString> TempValues = ParseToList(InPyValue);
		if (TempValues.Num() > 0)
		{
			Value = FString::Join(TempValues, TEXT(" "));
		}
	}
};

/** 单个字符串，或是一个字符串列表（以‘|’作为分隔符） */
class FNePySpecifierStringListValueBarSep : public FNePySpecifierStringListValueBase
{
public:
	explicit FNePySpecifierStringListValueBarSep(PyObject* InPyValue)
		: FNePySpecifierStringListValueBase(InPyValue)
	{
		if (NePyBase::ToCpp(InPyValue, Value))
		{
			Value.TrimStartAndEndInline();
			return;
		}

		TArray<FString> TempValues = ParseToList(InPyValue);
		if (TempValues.Num() > 0)
		{
			Value = FString::Join(TempValues, TEXT("|"));
		}
	}
};

template <typename T>
struct FNePySpecifier::TNePySpecifierSelfRegister
{
	TNePySpecifierSelfRegister()
	{
		FNePySpecifier::FNePySpecifierCreator Creator;
		Creator.CreateFunc = [](PyObject* InPyValue) { return new T(InPyValue); };
		Creator.Usage = T::Usage;
		Creator.AcceptScope = T::AcceptScope;

		FNePySpecifier::SpecifierCreators.Emplace(T::SpecifierName, Creator);
	}
};

#define NEPY_SPECIFIER_CLASS_BEGIN(_Name, _Parent, _Usage, _AcceptScope) \
static FName NAME_NePySpecifier##_Name(#_Name); \
namespace NePy##_Name##Specifier { \
class FNePy##_Name##Specifier; \
using Type = FNePy##_Name##Specifier; \
class FNePy##_Name##Specifier : public _Parent \
{ \
	using _Parent::_Parent; \
	using SelfRegisterType = FNePySpecifier::TNePySpecifierSelfRegister<FNePy##_Name##Specifier>; \
	static SelfRegisterType SelfRegister; \
\
public: \
	inline static FName SpecifierName{#_Name}; \
	inline static EUsage Usage{_Usage}; \
	inline static EScope AcceptScope{_AcceptScope}; \
\
	virtual FName GetName() const override \
	{ return SpecifierName; } \

#define NEPY_SPECIFIER_CLASS_END \
}; \
Type::SelfRegisterType Type::SelfRegister; \
} \

/** Meta = "..." */
NEPY_SPECIFIER_CLASS_BEGIN(Meta, FNePySpecifier, Usage_Editor, Scope_Any)
	explicit FNePyMetaSpecifier(PyObject* InPyValue)
		: FNePySpecifier(InPyValue)
	{
		if (!PyDict_Check(InPyValue))
		{
			PyErr_Format(PyExc_TypeError, "'meta' must be dict, not '%.200s'",
				Py_TYPE(InPyValue)->tp_name);
			PrintPythonTraceback();
			return;
		}

		PyObject* MetaDataKey = nullptr;
		PyObject* MetaDataValue = nullptr;
		Py_ssize_t MetaDataIndex = 0;
		while (PyDict_Next(InPyValue, &MetaDataIndex, &MetaDataKey, &MetaDataValue))
		{
			const FString MetaDataKeyStr = NePyBase::PyObjectToString(MetaDataKey);
			const FString MetaDataValueStr = NePyBase::PyObjectToString(MetaDataValue);
			Pairs.Emplace(FName(MetaDataKeyStr), MetaDataValueStr);
		}
	}

	virtual void ApplyTo(UField* Field) override
	{
		for (auto& Pair : Pairs)
		{
			SetMetaData(Field, Pair.Key, Pair.Value);
		}
	}

	virtual void ApplyTo(FField* Field) override
	{
		for (auto& Pair : Pairs)
		{
			SetMetaData(Field, Pair.Key, Pair.Value);
		}
	}

private:
	TArray<TPair<FName, FString>> Pairs;
NEPY_SPECIFIER_CLASS_END

#pragma region Specifiers

/** DisplayName = "..." */
NEPY_SPECIFIER_CLASS_BEGIN(DisplayName, FNePySpecifierStringValue, Usage_Editor, Scope_Any)
	virtual void ApplyTo(UField* Field) override
	{
		SetMetaData(Field, SpecifierName, Value);
	}

	virtual void ApplyTo(FField* Field) override
	{
		SetMetaData(Field, SpecifierName, Value);
	}
NEPY_SPECIFIER_CLASS_END

/** Category = "..." */
NEPY_SPECIFIER_CLASS_BEGIN(Category, FNePySpecifierStringValue, Usage_Editor, Scope_Members)
	virtual void ApplyTo(UField* Field) override
	{
		SetMetaData(Field, SpecifierName, Value);
	}

	virtual void ApplyTo(FField* Field) override
	{
		SetMetaData(Field, SpecifierName, Value);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintType */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintType, FNePySpecifier, Usage_Editor, Scope_Types)
	virtual void ApplyTo(UField* Field) override
	{
		SetMetaData(Field, SpecifierName, "true");
	}
NEPY_SPECIFIER_CLASS_END

/** NotBlueprintType */
NEPY_SPECIFIER_CLASS_BEGIN(NotBlueprintType, FNePySpecifier, Usage_Editor, Scope_Types)
	virtual void ApplyTo(UField* Field) override
	{
		SetMetaData(Field, SpecifierName, "true");
		RemoveMetaData(Field, NAME_NePySpecifierBlueprintType);
	}
NEPY_SPECIFIER_CLASS_END

/** Blueprintable */
NEPY_SPECIFIER_CLASS_BEGIN(Blueprintable, FNePySpecifier, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		static FName NAME_IsBlueprintBase("IsBlueprintBase");
		SetMetaData(Field, NAME_IsBlueprintBase, "true");
		SetMetaData(Field, NAME_NePySpecifierBlueprintType, "true");
	}
NEPY_SPECIFIER_CLASS_END

/** NotBlueprintable */
NEPY_SPECIFIER_CLASS_BEGIN(NotBlueprintable, FNePySpecifier, Usage_Editor, Scope_Types)
	virtual void ApplyTo(UField* Field) override
	{
		static FName NAME_IsBlueprintBase("IsBlueprintBase");
		SetMetaData(Field, NAME_IsBlueprintBase, "false");
		RemoveMetaData(Field, NAME_NePySpecifierBlueprintType);
	}
NEPY_SPECIFIER_CLASS_END

/** CallInEditor */
NEPY_SPECIFIER_CLASS_BEGIN(CallInEditor, FNePySpecifier, Usage_Editor, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		SetMetaData(Field, SpecifierName, "true");
	}
NEPY_SPECIFIER_CLASS_END

/** EditInlineNew */
NEPY_SPECIFIER_CLASS_BEGIN(EditInlineNew, FNePySpecifier, Usage_Runtime, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_EditInlineNew);
	}
NEPY_SPECIFIER_CLASS_END

/** NotEditInlineNew */
NEPY_SPECIFIER_CLASS_BEGIN(NotEditInlineNew, FNePySpecifier, Usage_Runtime, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		RemoveClassFlags(Field, CLASS_EditInlineNew);
	}
NEPY_SPECIFIER_CLASS_END

/** NotPlaceable */
NEPY_SPECIFIER_CLASS_BEGIN(NotPlaceable, FNePySpecifier, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_NotPlaceable);
	}
NEPY_SPECIFIER_CLASS_END

/** DefaultToInstanced */
NEPY_SPECIFIER_CLASS_BEGIN(DefaultToInstanced, FNePySpecifier, Usage_Runtime, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_DefaultToInstanced);
	}
NEPY_SPECIFIER_CLASS_END

/** HideDropdown */
NEPY_SPECIFIER_CLASS_BEGIN(HideDropdown, FNePySpecifier, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_HideDropDown);
	}
NEPY_SPECIFIER_CLASS_END

/** Abstract */
NEPY_SPECIFIER_CLASS_BEGIN(Abstract, FNePySpecifier, Usage_Runtime, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_Abstract);
	}
NEPY_SPECIFIER_CLASS_END

/** Deprecated */
NEPY_SPECIFIER_CLASS_BEGIN(Deprecated, FNePySpecifier, Usage_Runtime, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_Deprecated | CLASS_NotPlaceable);
	}
NEPY_SPECIFIER_CLASS_END

/** Transient */
NEPY_SPECIFIER_CLASS_BEGIN(Transient, FNePySpecifier, Usage_Runtime, Scope_ClassOrProps)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_Transient);
	}

	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Transient);
	}
NEPY_SPECIFIER_CLASS_END

/**
 * 对于UClass而言 Config = "..."
 * 对于FProperty而言 Config
 */
NEPY_SPECIFIER_CLASS_BEGIN(Config, FNePySpecifierStringValue, Usage_Runtime, Scope_ClassOrProps)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			Class->ClassConfigName = FName(Value);
			AddClassFlags(Class, CLASS_Config);
		}
	}

	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Config);
		auto Class = Field->GetOwnerClass();
	}
NEPY_SPECIFIER_CLASS_END

/* PerObjectConfig */
NEPY_SPECIFIER_CLASS_BEGIN(PerObjectConfig, FNePySpecifier, Usage_Runtime, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		AddClassFlags(Field, CLASS_PerObjectConfig);
	}
NEPY_SPECIFIER_CLASS_END

/** ShowCategories = ("...", "...", ...) */
NEPY_SPECIFIER_CLASS_BEGIN(ShowCategories, FNePySpecifierStringListValue, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
#if WITH_EDITORONLY_DATA
			const FString& SuperValue = Class->GetMetaData(SpecifierName);
			if (!SuperValue.IsEmpty())
			{
				Value = SuperValue + ' ' + Value;
			}
#endif
			SetMetaData(Class, SpecifierName, Value);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** HideCategories = ("...", "...", ...) */
NEPY_SPECIFIER_CLASS_BEGIN(HideCategories, FNePySpecifierStringListValue, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
#if WITH_EDITORONLY_DATA
			const FString& SuperValue = Class->GetMetaData(SpecifierName);
			if (!SuperValue.IsEmpty())
			{
				Value = SuperValue + ' ' + Value;
			}
#endif
			SetMetaData(Class, SpecifierName, Value);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** ClassGroup = "..." */
NEPY_SPECIFIER_CLASS_BEGIN(ClassGroup, FNePySpecifierStringValue, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			static FName NAME_ClassGroupNames("ClassGroupNames");
			SetMetaData(Class, NAME_ClassGroupNames, Value);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** AutoExpandCategories = ("...", "...", ...) */
NEPY_SPECIFIER_CLASS_BEGIN(AutoExpandCategories, FNePySpecifierStringListValue, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			SetMetaData(Class, SpecifierName, Value);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** AutoCollapseCategories = ("...", "...", ...) */
NEPY_SPECIFIER_CLASS_BEGIN(AutoCollapseCategories, FNePySpecifierStringListValue, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			SetMetaData(Class, SpecifierName, Value);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** CollapseCategories = True */
NEPY_SPECIFIER_CLASS_BEGIN(CollapseCategories, FNePySpecifier, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			AddClassFlags(Class, CLASS_CollapseCategories);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** DontCollapseCategories = True */
NEPY_SPECIFIER_CLASS_BEGIN(DontCollapseCategories, FNePySpecifier, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			RemoveClassFlags(Class, CLASS_CollapseCategories);
		}
	}
NEPY_SPECIFIER_CLASS_END

/** AdvancedClassDisplay */
NEPY_SPECIFIER_CLASS_BEGIN(AdvancedClassDisplay, FNePySpecifier, Usage_Editor, Scope_Class)
	virtual void ApplyTo(UField* Field) override
	{
		if (UClass* Class = Cast<UClass>(Field))
		{
			SetMetaData(Class, SpecifierName, "true");
		}
	}
NEPY_SPECIFIER_CLASS_END

/** NotEditable */
NEPY_SPECIFIER_CLASS_BEGIN(NotEditable, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		// Do nothing.
		//RemovePropertyFlags(Field, CPF_Edit | CPF_EditConst);
	}
NEPY_SPECIFIER_CLASS_END

/** EditAnywhere */
NEPY_SPECIFIER_CLASS_BEGIN(EditAnywhere, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit);
	}
NEPY_SPECIFIER_CLASS_END

/** EditInstanceOnly */
NEPY_SPECIFIER_CLASS_BEGIN(EditInstanceOnly, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit | CPF_DisableEditOnTemplate);
	}
NEPY_SPECIFIER_CLASS_END

/** EditDefaultsOnly */
NEPY_SPECIFIER_CLASS_BEGIN(EditDefaultsOnly, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit | CPF_DisableEditOnInstance);
	}
NEPY_SPECIFIER_CLASS_END

/** VisibleAnywhere */
NEPY_SPECIFIER_CLASS_BEGIN(VisibleAnywhere, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit | CPF_EditConst);
	}
NEPY_SPECIFIER_CLASS_END

/** VisibleInstanceOnly */
NEPY_SPECIFIER_CLASS_BEGIN(VisibleInstanceOnly, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit | CPF_EditConst | CPF_DisableEditOnTemplate);
	}
NEPY_SPECIFIER_CLASS_END

/** VisibleDefaultsOnly */
NEPY_SPECIFIER_CLASS_BEGIN(VisibleDefaultsOnly, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit | CPF_EditConst | CPF_DisableEditOnInstance);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintHidden */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintHidden, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		// Do nothing.
		//RemovePropertyFlags(Field, CPF_BlueprintVisible | CPF_BlueprintReadOnly);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintReadWrite */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintReadWrite, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintVisible);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintReadOnly */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintReadOnly, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintVisible | CPF_BlueprintReadOnly);
	}
NEPY_SPECIFIER_CLASS_END

/** 
 * BlueprintSetter
 * 对于 UProperty 是 BlueprintSetter = "..."
 * 对于 UFunction 是 BlueprintSetter
*/
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintSetter, FNePySpecifierStringValue, Usage_Runtime, Scope_Props | Scope_Function)

	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintVisible);
		SetMetaData(Field, NAME_NePySpecifierBlueprintSetter, Value);
	}

	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_BlueprintCallable);
		SetMetaData(Field, NAME_NePySpecifierBlueprintSetter, "");
	}

NEPY_SPECIFIER_CLASS_END

/** 
 * BlueprintGetter
 * 对于 UProperty 是 BlueprintGetter = "FunctionName"
 * 对于 UFunction 是 BlueprintGetter
*/
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintGetter, FNePySpecifierStringValue, Usage_Runtime, Scope_Props | Scope_Function)

	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintVisible);
		SetMetaData(Field, NAME_NePySpecifierBlueprintGetter, Value);
	}

	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_BlueprintCallable);
		AddFunctionFlags(Field, FUNC_BlueprintPure);
		SetMetaData(Field, NAME_NePySpecifierBlueprintGetter, "");
	}

NEPY_SPECIFIER_CLASS_END

/** GlobalConfig */
NEPY_SPECIFIER_CLASS_BEGIN(GlobalConfig, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Config | CPF_GlobalConfig);
	}
NEPY_SPECIFIER_CLASS_END

/** DuplicateTransient */
NEPY_SPECIFIER_CLASS_BEGIN(DuplicateTransient, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_DuplicateTransient);
	}
NEPY_SPECIFIER_CLASS_END

/** TextExportTransient */
NEPY_SPECIFIER_CLASS_BEGIN(TextExportTransient, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_TextExportTransient);
	}
NEPY_SPECIFIER_CLASS_END

/** NonPIEDuplicateTransient */
NEPY_SPECIFIER_CLASS_BEGIN(NonPIEDuplicateTransient, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_NonPIEDuplicateTransient);
	}
NEPY_SPECIFIER_CLASS_END

/** NoClear */
NEPY_SPECIFIER_CLASS_BEGIN(NoClear, FNePySpecifier, Usage_Editor, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_NoClear);
	}
NEPY_SPECIFIER_CLASS_END

/** EditFixedSize */
NEPY_SPECIFIER_CLASS_BEGIN(EditFixedSize, FNePySpecifier, Usage_Editor, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_EditFixedSize);
	}
NEPY_SPECIFIER_CLASS_END

/** Replicated */
NEPY_SPECIFIER_CLASS_BEGIN(Replicated, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Net);
	}
NEPY_SPECIFIER_CLASS_END

/** ReplicatedUsing = "..." */
NEPY_SPECIFIER_CLASS_BEGIN(ReplicatedUsing, FNePySpecifierStringValue, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		if (!Value.IsEmpty())
		{
			if (FProperty* Prop = CastField<FProperty>(Field))
			{
				Prop->PropertyFlags |= CPF_RepNotify;
				Prop->RepNotifyFunc = FName(Value);
			}
		}
	}
NEPY_SPECIFIER_CLASS_END

/** NotReplicated */
NEPY_SPECIFIER_CLASS_BEGIN(NotReplicated, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_RepSkip);
	}
NEPY_SPECIFIER_CLASS_END

/** Interp */
NEPY_SPECIFIER_CLASS_BEGIN(Interp, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_Edit | CPF_BlueprintVisible | CPF_Interp);
	}
NEPY_SPECIFIER_CLASS_END

/** NonTransactional */
NEPY_SPECIFIER_CLASS_BEGIN(NonTransactional, FNePySpecifier, Usage_Editor, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_NonTransactional);
	}
NEPY_SPECIFIER_CLASS_END

/** Instanced */
NEPY_SPECIFIER_CLASS_BEGIN(Instanced, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		// 此处逻辑摘自 AngelScript
		auto ApplyInstancedPropertyFlags = [this](FField* Property, FField* InnerProperty) {
			const FName NAME_EditInline(TEXT("EditInline"));
			if (Property)
			{
				AddPropertyFlags(Property, CPF_ContainsInstancedReference);
				SetMetaData(Property, NAME_EditInline, "true");
			}
			if (InnerProperty)
			{
				AddPropertyFlags(InnerProperty, CPF_InstancedReference | CPF_ExportObject | CPF_PersistentInstance);
				SetMetaData(InnerProperty, NAME_EditInline, "true");
			}
		};

		if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Field))
		{
			ApplyInstancedPropertyFlags(ArrayProp, ArrayProp->Inner);
		}
		else if (FMapProperty* MapProp = CastField<FMapProperty>(Field))
		{
			ApplyInstancedPropertyFlags(MapProp, MapProp->ValueProp);
			ApplyInstancedPropertyFlags(MapProp, MapProp->KeyProp);
		}
		else if (FSetProperty* SetProp = CastField<FSetProperty>(Field))
		{
			ApplyInstancedPropertyFlags(SetProp, SetProp->ElementProp);
		}
		else
		{
			ApplyInstancedPropertyFlags(nullptr, Field);
		}
	}

NEPY_SPECIFIER_CLASS_END

/** BlueprintAssignable */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintAssignable, FNePySpecifier, Usage_Runtime, Scope_Delegate)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintAssignable);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintCallable */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintCallable, FNePySpecifier, Usage_Runtime, Scope_FunctionOrDelegate)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintCallable);
	}

	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_BlueprintCallable);
	}
NEPY_SPECIFIER_CLASS_END

/** NotBlueprintCallable */
NEPY_SPECIFIER_CLASS_BEGIN(NotBlueprintCallable, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		// Do nothing.
		//RemoveFunctionFlags(Field, FUNC_BlueprintCallable);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintPure **/
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintPure, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_BlueprintCallable);
		AddFunctionFlags(Field, FUNC_BlueprintPure);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintAuthorityOnly */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintAuthorityOnly, FNePySpecifier, Usage_Runtime, Scope_FunctionOrDelegate)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_BlueprintAuthorityOnly);
	}

	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_BlueprintAuthorityOnly);
	}
NEPY_SPECIFIER_CLASS_END

/** AssetRegistrySearchable */
NEPY_SPECIFIER_CLASS_BEGIN(AssetRegistrySearchable, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_AssetRegistrySearchable);
	}
NEPY_SPECIFIER_CLASS_END

/** SimpleDisplay */
NEPY_SPECIFIER_CLASS_BEGIN(SimpleDisplay, FNePySpecifier, Usage_Editor, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_SimpleDisplay);
	}
NEPY_SPECIFIER_CLASS_END

/** AdvancedDisplay */
NEPY_SPECIFIER_CLASS_BEGIN(AdvancedDisplay, FNePySpecifier, Usage_Editor, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_AdvancedDisplay);
	}
NEPY_SPECIFIER_CLASS_END

/** SaveGame */
NEPY_SPECIFIER_CLASS_BEGIN(SaveGame, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_SaveGame);
	}
NEPY_SPECIFIER_CLASS_END

/** SkipSerialization */
NEPY_SPECIFIER_CLASS_BEGIN(SkipSerialization, FNePySpecifier, Usage_Runtime, Scope_Props)
	virtual void ApplyTo(FField* Field) override
	{
		AddPropertyFlags(Field, CPF_SkipSerialization);
	}
NEPY_SPECIFIER_CLASS_END

/** FieldNotify
 * 对于属性而言，还可以指定联动通知对象：
 *   FieldNotify = '...'
 *   FieldNotify = ('...', '...', ...)
 */
NEPY_SPECIFIER_CLASS_BEGIN(FieldNotify, FNePySpecifierStringListValueBarSep, Usage_Runtime, Scope_Property | Scope_Function)
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3
	virtual void ApplyTo(FField* Field) override
	{
#if WITH_EDITORONLY_DATA
		Field->SetMetaData(SpecifierName, *Value);
#endif

		UNePyGeneratedClass* PyClass = CastChecked<UNePyGeneratedClass>(Field->GetOwnerUObject());
		ensure(PyClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass()));
		ensure(!PyClass->FieldNotifies.Contains(FFieldNotificationId(Field->GetFName())));
		PyClass->FieldNotifies.Add(FFieldNotificationId(Field->GetFName()));

		if (!Value.IsEmpty())
		{
			TArray<FString> OtherFieldNotifyToTrigger;
			Value.ParseIntoArray(OtherFieldNotifyToTrigger, TEXT("|"), true);

			TArray<FName> OtherFieldNotifyNameToTrigger;
			for (const FString& OtherFieldNotify : OtherFieldNotifyToTrigger)
			{
				FName OtherName(OtherFieldNotify);
				if (OtherName != Field->GetFName())
				{
					OtherFieldNotifyNameToTrigger.AddUnique(OtherName);
				}
			}
			
			if (OtherFieldNotifyNameToTrigger.Num() > 0)
			{
				PyClass->OtherFieldNotifyToTriggerMap.Emplace(Field->GetFName(), MoveTemp(OtherFieldNotifyNameToTrigger));
			}
		}
	}

	virtual void ApplyTo(UField* Field) override
	{
#if WITH_EDITORONLY_DATA
		Field->SetMetaData(SpecifierName, TEXT(""));
#endif

		AddFunctionFlags(Field, FUNC_BlueprintPure | FUNC_Const);
		RemoveFunctionFlags(Field, FUNC_Event);

		UNePyGeneratedClass* PyClass = CastChecked<UNePyGeneratedClass>(Field->GetOuter());
		ensure(PyClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass()));
		ensure(!PyClass->FieldNotifies.Contains(FFieldNotificationId(Field->GetFName())));
		PyClass->FieldNotifies.Add(FFieldNotificationId(Field->GetFName()));
	}
#endif
NEPY_SPECIFIER_CLASS_END

/** BlueprintCosmetic */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintCosmetic, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_BlueprintCosmetic);
	}
NEPY_SPECIFIER_CLASS_END

/** BlueprintEvent */
NEPY_SPECIFIER_CLASS_BEGIN(BlueprintEvent, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
{
	AddFunctionFlags(Field, FUNC_Event | FUNC_BlueprintEvent);
}
NEPY_SPECIFIER_CLASS_END

/** Client */
NEPY_SPECIFIER_CLASS_BEGIN(Client, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_Net | FUNC_NetClient);
	}
NEPY_SPECIFIER_CLASS_END

/** NetMulticast */
NEPY_SPECIFIER_CLASS_BEGIN(NetMulticast, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_Net | FUNC_NetMulticast);
	}
NEPY_SPECIFIER_CLASS_END

/** Reliable */
NEPY_SPECIFIER_CLASS_BEGIN(Reliable, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_NetReliable);
	}
NEPY_SPECIFIER_CLASS_END

/** Unreliable 是否真的需要这个？ */
NEPY_SPECIFIER_CLASS_BEGIN(Unreliable, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		RemoveFunctionFlags(Field, FUNC_NetReliable);
	}
NEPY_SPECIFIER_CLASS_END

/** Server */
NEPY_SPECIFIER_CLASS_BEGIN(Server, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_Net | FUNC_NetServer);
	}
NEPY_SPECIFIER_CLASS_END

/** Exec */
NEPY_SPECIFIER_CLASS_BEGIN(Exec, FNePySpecifier, Usage_Runtime, Scope_Function)
	virtual void ApplyTo(UField* Field) override
	{
		AddFunctionFlags(Field, FUNC_Exec);
	}
NEPY_SPECIFIER_CLASS_END

#pragma endregion

#undef NEPY_SPECIFIER_CLASS_BEGIN
#undef NEPY_SPECIFIER_CLASS_END

#if !WITH_EDITORONLY_DATA
#undef SetMetaData
#undef RemoveMetaData
#endif


void FNePySpecifier::ApplyToClass(const TArray<FNePySpecifier*>& Specifiers, UClass* InClass)
{
#if WITH_EDITORONLY_DATA
	UClass* SuperClass = InClass->GetSuperClass();
	static FName InheritedMetaDataKeys[] = {
		NAME_NePySpecifierHideCategories,
		NAME_NePySpecifierShowCategories,
	};

	for (const FName& InheritedMetaDataKey : InheritedMetaDataKeys)
	{
		const FString MetaDataValue = SuperClass->GetMetaData(InheritedMetaDataKey);
		if (!MetaDataValue.IsEmpty())
		{
			InClass->SetMetaData(InheritedMetaDataKey, *MetaDataValue);
		}
		else
		{
			// 目前可继承的 Metadata 不会在前序流程进行初始化赋值
			// 由于后续处理流程利用了这里的信息，因此在 Reload 时应消除之前已有的 MetaData
			InClass->RemoveMetaData(InheritedMetaDataKey);
		}
	}
#endif
	ApplyTo(Specifiers, InClass);
}

void FNePySpecifier::ApplyToStruct(const TArray<FNePySpecifier*>& Specifiers, UScriptStruct* InStruct)
{
	bool bSeenBlueprintType = false;
	for (FNePySpecifier* Specifier : Specifiers)
	{
		const FName SpecifierName = Specifier->GetName();
		if (SpecifierName == NAME_NePySpecifierBlueprintType
			|| SpecifierName == NAME_NePySpecifierNotBlueprintType)
		{
			if (bSeenBlueprintType)
			{
				PyErr_Format(PyExc_TypeError, "%s: Cannot specify a ustruct as being both BlueprintType and NotBlueprintType.",
					TCHAR_TO_UTF8(*InStruct->GetName()));
				PrintPythonTraceback();
				continue;
			}
			bSeenBlueprintType = true;
		}
		Specifier->ApplyTo(InStruct);
	}

	if (!bSeenBlueprintType && DefaultStructBlueprintTypeSpecifier)
	{
		DefaultStructBlueprintTypeSpecifier->ApplyTo(InStruct);
	}
}

void FNePySpecifier::ApplyToEnum(const TArray<FNePySpecifier*>& Specifiers, UEnum* InEnum)
{
	bool bSeenBlueprintType = false;
	for (FNePySpecifier* Specifier : Specifiers)
	{
		const FName SpecifierName = Specifier->GetName();
		if (SpecifierName == NAME_NePySpecifierBlueprintType
			|| SpecifierName == NAME_NePySpecifierNotBlueprintType)
		{
			if (bSeenBlueprintType)
			{
				PyErr_Format(PyExc_TypeError, "%s: Cannot specify a uenum as being both BlueprintType and NotBlueprintType.",
					TCHAR_TO_UTF8(*InEnum->GetName()));
				PrintPythonTraceback();
				continue;
			}
			bSeenBlueprintType = true;
		}
		Specifier->ApplyTo(InEnum);
	}

	if (!bSeenBlueprintType && DefaultEnumBlueprintTypeSpecifier)
	{
		DefaultEnumBlueprintTypeSpecifier->ApplyTo(InEnum);
	}
}

void FNePySpecifier::ApplyToParam(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp)
{
	for (FNePySpecifier* Specifier : Specifiers)
	{
		Specifier->ApplyTo(InProp);
	}
}

void FNePySpecifier::ApplyToProperty(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp)
{
	UObject* Owner = InProp->GetOwnerUObject();
	EApplyCase ApplyCase = Owner && Owner->IsA<UScriptStruct>() ? ApplyCase_PropertyInStruct : ApplyCase_PropertyInClass;
	ApplyToPropertyInternal(Specifiers, InProp, ApplyCase);
}

void FNePySpecifier::ApplyToDelegate(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp)
{
	ApplyToPropertyInternal(Specifiers, InProp, ApplyCase_DelegateProperty);
}

void FNePySpecifier::ApplyToComponent(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp, bool bIsOverride)
{
	EApplyCase ApplyCase = bIsOverride ? ApplyCase_OverrideComponentProperty : ApplyCase_ComponentProperty;
	ApplyToPropertyInternal(Specifiers, InProp, ApplyCase);
}

void FNePySpecifier::ApplyToPropertyInternal(const TArray<FNePySpecifier*>& Specifiers, FProperty* InProp, EApplyCase InApplyCase)
{
	bool bSeenEditSpecifier = false;
	bool bSeenBlueprintReadOnlySpecifier = false;
	bool bSeenBlueprintWriteSpecifier = false;
	bool bSeenBlueprintGetterSpecifier = false;

	auto ContextName = [InProp]()
	{
		UObject* Owner = InProp->GetOwnerUObject();
		if (Owner)
		{
			return FString::Printf(TEXT("%s.%s"), *Owner->GetName(), *InProp->GetName());
		}
		return InProp->GetName();
	};

	for (FNePySpecifier* Specifier : Specifiers)
	{
		const FName SpecifierName = Specifier->GetName();
		if (SpecifierName == NAME_NePySpecifierNotEditable
			|| SpecifierName == NAME_NePySpecifierEditAnywhere
			|| SpecifierName == NAME_NePySpecifierEditInstanceOnly
			|| SpecifierName == NAME_NePySpecifierEditDefaultsOnly
			|| SpecifierName == NAME_NePySpecifierVisibleAnywhere
			|| SpecifierName == NAME_NePySpecifierVisibleInstanceOnly
			|| SpecifierName == NAME_NePySpecifierVisibleDefaultsOnly)
		{
			if (bSeenEditSpecifier)
			{
				PyErr_Format(PyExc_TypeError, "%s: Found more than one edit/visibility specifier (%s), only one is allowed",
					TCHAR_TO_UTF8(*ContextName()), TCHAR_TO_UTF8(*SpecifierName.ToString()));
				PrintPythonTraceback();
				continue;
			}
			bSeenEditSpecifier = true;
		}
		else if (SpecifierName == NAME_NePySpecifierBlueprintSetter
			|| SpecifierName == NAME_NePySpecifierBlueprintReadWrite)
		{
			if (bSeenBlueprintReadOnlySpecifier)
			{
				PyErr_Format(PyExc_TypeError, "%s: Cannot specify a property as being both BlueprintReadOnly and BlueprintReadWrite or BlueprintSetter.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}
			bSeenBlueprintWriteSpecifier = true;
		}
		else if (SpecifierName == NAME_NePySpecifierBlueprintHidden
			|| SpecifierName == NAME_NePySpecifierBlueprintReadOnly)
		{
			if (bSeenBlueprintWriteSpecifier)
			{
				PyErr_Format(PyExc_TypeError, "%s: Cannot specify a property as being both BlueprintReadOnly and BlueprintReadWrite or BlueprintSetter.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}
			bSeenBlueprintReadOnlySpecifier = true;
		}
		else if (SpecifierName == NAME_NePySpecifierBlueprintGetter)
		{
			bSeenBlueprintGetterSpecifier = true;
		}
		else if (SpecifierName == NAME_NePySpecifierReplicated
			|| SpecifierName == NAME_NePySpecifierReplicatedUsing)
		{
			if (InApplyCase == ApplyCase_PropertyInStruct)
			{
				PyErr_Format(PyExc_TypeError, "%s: Struct members cannot be replicated",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}
		}
		else if (SpecifierName == NAME_NePySpecifierNotReplicated)
		{
			if (InApplyCase != ApplyCase_PropertyInStruct)
			{
				PyErr_Format(PyExc_TypeError, "%s: Only Struct members can be marked NotReplicated",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}
		}
		else if (SpecifierName == NAME_NePySpecifierFieldNotify)
		{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
			if (InApplyCase != ApplyCase_PropertyInClass)
			{
				PyErr_Format(PyExc_TypeError, "%s: FieldNofity property are only valid as UClass member variable.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}

			UObject* Owner = InProp->GetOwnerUObject();
			UClass* OwnerClass = Cast<UClass>(Owner);
			if (!OwnerClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass()))
			{
				PyErr_Format(PyExc_TypeError, "%s: UClass need to implement the interface INotifyFieldValueChanged' to support FieldNotify.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}
#else
			PyErr_Format(PyExc_TypeError, "%s: FieldNotify is only available for UE5.3 and above",
				TCHAR_TO_UTF8(*ContextName()));
			PrintPythonTraceback();
			continue;
#endif
		}

		Specifier->ApplyTo(InProp);
	}

	if (bSeenBlueprintGetterSpecifier && !bSeenBlueprintWriteSpecifier)
	{
		InProp->PropertyFlags |= CPF_BlueprintReadOnly;
	}

	// 处理默认值
	if (!bSeenEditSpecifier)
	{
		ApplyPropertyDefaultEditSpecifier(InProp, InApplyCase);
	}
	if (!(bSeenBlueprintReadOnlySpecifier || bSeenBlueprintWriteSpecifier || bSeenBlueprintGetterSpecifier))
	{
		ApplyPropertyDefaultBlueprintSpecifier(InProp, InApplyCase);
	}
}

void FNePySpecifier::ApplyToFunction(const TArray<FNePySpecifier*>& Specifiers, UFunction* InFunc)
{
	bool bSeenBlueprintCallableSpecifier = false;

	auto ContextName = [InFunc]()
	{
		UObject* Owner = InFunc->GetOwnerStruct();
		if (Owner)
		{
			return FString::Printf(TEXT("%s.%s"), *Owner->GetName(), *InFunc->GetName());
		}
		return InFunc->GetName();
	};

	for (FNePySpecifier* Specifier : Specifiers)
	{
		const FName SpecifierName = Specifier->GetName();
		if (SpecifierName == NAME_NePySpecifierClient
			|| SpecifierName == NAME_NePySpecifierServer
			|| SpecifierName == NAME_NePySpecifierNetMulticast)
		{
			if (InFunc->HasAnyFunctionFlags(FUNC_Exec))
			{
				PyErr_Format(PyExc_TypeError, "%s: Exec functions cannot be replicated!",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;	
			}
		}
		else if (SpecifierName == NAME_NePySpecifierExec)
		{
			if (InFunc->HasAnyFunctionFlags(FUNC_Net))
			{
				PyErr_Format(PyExc_TypeError, "%s: Exec functions cannot be replicated!",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;	
			}
		}
		else if (SpecifierName == NAME_NePySpecifierFieldNotify)
		{
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
			UClass* OwnerClass = InFunc->GetOuterUClass();
			if (!OwnerClass)
			{
				PyErr_Format(PyExc_TypeError, "%s: FieldNofity function is only valid as UClass member function.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;	
			}

			if (!OwnerClass->ImplementsInterface(UNotifyFieldValueChanged::StaticClass()))
			{
				PyErr_Format(PyExc_TypeError, "%s: UClass need to implement the interface INotifyFieldValueChanged' to support FieldNotify.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;	
			}

			int32 NumRets = 0;
			int32 NumOuts = 0;
			int32 NumArgs = 0;
			for (TFieldIterator<FProperty> PropIt(InFunc, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
			{
				FProperty* Prop = *PropIt;
				if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					NumRets += 1;
				}
				else if (Prop->HasAnyPropertyFlags(CPF_OutParm))
				{
					NumOuts += 1;	
				}
				else if (Prop->HasAnyPropertyFlags(CPF_Parm))
				{
					NumArgs += 1;
				}
			}

			if (NumArgs > 0)
			{
				PyErr_Format(PyExc_TypeError, "%s: FieldNotify function must not have parameters.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;		
			}

			if (NumRets != 1 || NumOuts > 0)
			{
				PyErr_Format(PyExc_TypeError, "%s: FieldNotify function must return ONE value.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;		
			}

			if (!((NePyFieldNotifySpecifier::Type*)Specifier)->Value.IsEmpty())
			{
				PyErr_Format(PyExc_TypeError, "%s: FieldNotify function can not be a trigger source.",
					TCHAR_TO_UTF8(*ContextName()));
				PrintPythonTraceback();
				continue;
			}
#else
			PyErr_Format(PyExc_TypeError, "%s: FieldNotify is only available for UE5.3 and above",
				TCHAR_TO_UTF8(*ContextName()));
			PrintPythonTraceback();
			continue;	
#endif
		}
		else if (SpecifierName == NAME_NePySpecifierBlueprintCallable
			|| SpecifierName == NAME_NePySpecifierNotBlueprintCallable)
		{
			bSeenBlueprintCallableSpecifier = true;
		}
		
		Specifier->ApplyTo(InFunc);
	}

	if (!bSeenBlueprintCallableSpecifier && DefaultFunctionBlueprintCallableSpecifier)
	{
		DefaultFunctionBlueprintCallableSpecifier->ApplyTo(InFunc);
	}
}

void FNePySpecifier::InitializeSpecifierDefaults()
{
	static FName UniqueSpecifierNames[] = {
		//NAME_NePySpecifierNotEditable,
		NAME_NePySpecifierEditAnywhere,
		NAME_NePySpecifierEditDefaultsOnly,
		NAME_NePySpecifierEditInstanceOnly,
		NAME_NePySpecifierVisibleAnywhere,
		NAME_NePySpecifierVisibleDefaultsOnly,
		NAME_NePySpecifierVisibleInstanceOnly,

		//NAME_NePySpecifierBlueprintHidden,
		NAME_NePySpecifierBlueprintReadOnly,
		NAME_NePySpecifierBlueprintReadWrite,

		NAME_NePySpecifierBlueprintType,
		//NAME_NePySpecifierNotBlueprintType,

		NAME_NePySpecifierBlueprintCallable,
		//NAME_NePySpecifierNotBlueprintCallable
	};
	for (auto& Name : UniqueSpecifierNames)
	{
		auto Specifier = CreateUniqueSpecifier(Name);
		if (Specifier)
		{
			DefaultUniqueSpecifiers.Add(Name, MoveTemp(Specifier));
		}
	}

	auto NePythonSettings = UNePythonSettings::StaticClass()->GetDefaultObject<UNePythonSettings>();
	if (NePythonSettings->bDefaultFunctionBlueprintCallable)
	{
		DefaultFunctionBlueprintCallableSpecifier = GetUniqueSpecifier(NAME_NePySpecifierBlueprintCallable);
	}
	if (NePythonSettings->bDefaultStructBlueprintType)
	{
		DefaultStructBlueprintTypeSpecifier = GetUniqueSpecifier(NAME_NePySpecifierBlueprintType);
	}
	if (NePythonSettings->bDefaultEnumBlueprintType)
	{
		DefaultEnumBlueprintTypeSpecifier = GetUniqueSpecifier(NAME_NePySpecifierBlueprintType);
	}

	auto EditSpecifierName = [](ENePyPropertyEditSpecifier EditSpecifier) -> FName
	{
		switch (EditSpecifier)
		{
		case ENePyPropertyEditSpecifier::EditAnywhere:
			return NAME_NePySpecifierEditAnywhere;
		case ENePyPropertyEditSpecifier::EditDefaultsOnly:
			return NAME_NePySpecifierEditDefaultsOnly;
		case ENePyPropertyEditSpecifier::EditInstanceOnly:
			return NAME_NePySpecifierEditInstanceOnly;
		case ENePyPropertyEditSpecifier::VisibleAnywhere:
			return NAME_NePySpecifierVisibleAnywhere;
		case ENePyPropertyEditSpecifier::VisibleDefaultsOnly:
			return NAME_NePySpecifierVisibleDefaultsOnly;
		case ENePyPropertyEditSpecifier::VisibleInstanceOnly:
			return NAME_NePySpecifierVisibleInstanceOnly;
		default:
			return NAME_NePySpecifierNotEditable;
		}
	};

	auto BlueprintSpecifierName = [](ENePyPropertyBlueprintSpecifier BlueprintSpecifier) -> FName
	{
		switch (BlueprintSpecifier)
		{
		case ENePyPropertyBlueprintSpecifier::BlueprintReadWrite:
			return NAME_NePySpecifierBlueprintReadWrite;
		case ENePyPropertyBlueprintSpecifier::BlueprintReadOnly:
			return NAME_NePySpecifierBlueprintReadOnly;
		default:
			return NAME_NePySpecifierBlueprintHidden;
		}
	};

	DefaultPropertyEditSpecifiers = {
		{ApplyCase_PropertyInClass, GetUniqueSpecifier(EditSpecifierName(NePythonSettings->DefaultPropertyEditSpecifier))},
		{ApplyCase_PropertyInStruct, GetUniqueSpecifier(EditSpecifierName(NePythonSettings->DefaultPropertyEditSpecifierForStructs))},
		{ApplyCase_DelegateProperty, GetUniqueSpecifier(NAME_NePySpecifierNotEditable)},
		{ApplyCase_ComponentProperty, GetUniqueSpecifier(NAME_NePySpecifierEditDefaultsOnly)},
		{ApplyCase_OverrideComponentProperty, GetUniqueSpecifier(NAME_NePySpecifierNotEditable)},
	};

	DefaultPropertyBlueprintSpecifiers = {
		{ApplyCase_PropertyInClass, GetUniqueSpecifier(BlueprintSpecifierName(NePythonSettings->DefaultPropertyBlueprintSpecifier))},
		{ApplyCase_PropertyInStruct, GetUniqueSpecifier(BlueprintSpecifierName(NePythonSettings->DefaultPropertyBlueprintSpecifierForStructs))},
		{ApplyCase_DelegateProperty, GetUniqueSpecifier(NAME_NePySpecifierBlueprintReadWrite)},
		{ApplyCase_ComponentProperty, GetUniqueSpecifier(NAME_NePySpecifierBlueprintReadOnly)},
		{ApplyCase_OverrideComponentProperty, GetUniqueSpecifier(NAME_NePySpecifierBlueprintHidden)},
	};
}

TUniquePtr<FNePySpecifier> FNePySpecifier::CreateUniqueSpecifier(const FName& InSpecifierName)
{
	FNePySpecifierCreator* Creator = SpecifierCreators.Find(InSpecifierName);

#if !WITH_EDITOR
	if (Creator->Usage == Usage_Editor)
	{
		return TUniquePtr<FNePySpecifier>();
	}
#endif

	FNePySpecifier* Specifier = Creator->CreateFunc(Py_True);
	return TUniquePtr<FNePySpecifier>(Specifier);
}

FNePySpecifier* FNePySpecifier::GetUniqueSpecifier(const FName& InSpecifierName)
{
	TUniquePtr<FNePySpecifier>* Specifier = DefaultUniqueSpecifiers.Find(InSpecifierName);
	if (Specifier)
	{
		return Specifier->Get();
	}
	return nullptr;
}

void FNePySpecifier::ApplyPropertyDefaultEditSpecifier(FProperty* InProp, EApplyCase InApplyCase)
{
	FNePySpecifier* DefaultEditSpecifier = DefaultPropertyEditSpecifiers.FindRef(InApplyCase);
	if (DefaultEditSpecifier)
	{
		DefaultEditSpecifier->ApplyTo(InProp);
	}
}

void FNePySpecifier::ApplyPropertyDefaultBlueprintSpecifier(FProperty* InProp, EApplyCase InApplyCase)
{
	FNePySpecifier* DefaultEditSpecifier = DefaultPropertyBlueprintSpecifiers.FindRef(InApplyCase);
	if (DefaultEditSpecifier)
	{
		DefaultEditSpecifier->ApplyTo(InProp);
	}
}

TMap<FName, TUniquePtr<FNePySpecifier>> FNePySpecifier::DefaultUniqueSpecifiers;
TMap<FNePySpecifier::EApplyCase, FNePySpecifier*> FNePySpecifier::DefaultPropertyEditSpecifiers;
TMap<FNePySpecifier::EApplyCase, FNePySpecifier*> FNePySpecifier::DefaultPropertyBlueprintSpecifiers;
FNePySpecifier* FNePySpecifier::DefaultFunctionBlueprintCallableSpecifier;
FNePySpecifier* FNePySpecifier::DefaultStructBlueprintTypeSpecifier;
FNePySpecifier* FNePySpecifier::DefaultEnumBlueprintTypeSpecifier;
