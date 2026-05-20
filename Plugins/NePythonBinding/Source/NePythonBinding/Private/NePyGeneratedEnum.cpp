#include "NePyGeneratedEnum.h"
#include "NePyGeneratedType.h"
#include "NePyEnumBase.h"
#include "NePyBase.h"
#include "NePyGenUtil.h"
#include "NePyUtil.h"
#include "NePyWrapperTypeRegistry.h"
#include "NePyMemoryAllocator.h"
#include "NePySpecifiers.h"
#include "UObject/LinkerLoad.h"

void UNePyGeneratedEnum::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UNePyGeneratedEnum::ReleasePythonResources()
{
	if (PyType)
	{
		FNePyScopedGIL GIL;
		Py_CLEAR(PyType);
	}
}

class FNePyGeneratedEnumBuilder
{
public:
	FNePyGeneratedEnumBuilder(const FString& InEnumName, PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs)
		: EnumName(InEnumName)
		, PyType(InPyType)
		, NewEnum(nullptr)
		, bDidExist(false)
	{
		UObject* EnumOuter = GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType::Runtime);

		// Enum instances are re-used if they already exist
		NewEnum = FindObject<UNePyGeneratedEnum>(EnumOuter, *EnumName);
		if (NewEnum)
		{
			bDidExist = true;
		}
		else
		{
			NewEnum = NewObject<UNePyGeneratedEnum>(EnumOuter, *EnumName, RF_Public | RF_Standalone | RF_NePyGeneratedTypeGCSafe);

			NewEnum->AddToRoot();
		}

		Specifiers = FNePySpecifier::ParseSpecifiers(InPySpecifierPairs, FNePySpecifier::Scope_Enum);
	}

	~FNePyGeneratedEnumBuilder()
	{
		FNePySpecifier::ReleaseSpecifiers(Specifiers);

		// If NewEnum is still set at this point, if means Finalize wasn't called and we should destroy the partially built enum
		if (NewEnum)
		{
			NewEnum->RemoveFromRoot();
			NewEnum->ClearFlags(RF_AllFlags);
			NewEnum->ClearInternalFlags(EInternalObjectFlags_NePyGeneratedTypeGCSafe);
			NewEnum = nullptr;

			Py_BEGIN_ALLOW_THREADS
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			Py_END_ALLOW_THREADS
		}
	}

	UNePyGeneratedEnum* Finalize()
	{
		// Populate the enum with its values, and replace the definitions with real descriptors
		if (!RegisterDescriptors())
		{
			return nullptr;
		}

		// Let Python know that we've changed its type
		PyType_Modified(PyType);

		// Finalize the enum
		NewEnum->Bind();

		PyTypeObject* OldPyType = NewEnum->PyType;
		if (NewEnum->PyType != PyType)
		{
			Py_INCREF(PyType);
			Py_XDECREF(NewEnum->PyType);
			NewEnum->PyType = PyType;
		}

		// Map the Unreal enum to the Python type
		if (bDidExist && OldPyType != PyType)
		{
			FNePyWrapperTypeRegistry::Get().UnregisterWrappedEnumType(NewEnum);
		}
		if (!bDidExist || OldPyType != PyType)
		{
			FNePyEnumTypeInfo TypeInfo = {
				PyType,
				ENePyTypeFlags::ScriptPyType,
			};
			FNePyWrapperTypeRegistry::Get().RegisterWrappedEnumType(NewEnum, TypeInfo);
		}

#if WITH_EDITORONLY_DATA
		FString Path = NePyGenUtil::GetPythonModuleRelativePathForPyType(PyType);
		if (!Path.IsEmpty())
		{
			NewEnum->SetMetaData(TEXT("ModuleRelativePath"), *Path);
		}

		const FString DocString = NePyUtil::GetDocString((PyObject*)PyType);
		if (!DocString.IsEmpty())
		{
			NewEnum->SetMetaData(TEXT("ToolTip"), *DocString);
		}
#endif // WITH_EDITORONLY_DATA

		// 应用说明符
		FNePySpecifier::ApplyToEnum(Specifiers, NewEnum);

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
		NotifyRegistrationEvent(*NewEnum->GetPackage()->GetName(), *EnumName, ENotifyRegistrationType::NRT_Enum, ENotifyRegistrationPhase::NRP_Finished, nullptr, false, NewEnum);
#endif

		// Null the NewEnum pointer so the destructor doesn't kill it
		UNePyGeneratedEnum* FinalizedEnum = NewEnum;
		NewEnum = nullptr;
		return FinalizedEnum;
	}

	bool CreateValueFromDefinition(const FString& InFieldName, FNePyUValueDef* InPyValueDef)
	{
		// Build the definition data for the new enum value
		FEnumValueDef& EnumValueDef = EnumValueDefs.AddDefaulted_GetRef();
		EnumValueDef.PyValueDef = InPyValueDef;
		EnumValueDef.Name = InFieldName;

		return true;
	}

private:
	bool RegisterDescriptors()
	{
		// Populate the enum with its values
		{
			TArray<TPair<FName, int64>> ValueNames;
			for (const FEnumValueDef& EnumValueDef : EnumValueDefs)
			{
				const FString NamespacedValueName = FString::Printf(TEXT("%s::%s"), *EnumName, *EnumValueDef.Name);
				ValueNames.Emplace(MakeTuple(FName(*NamespacedValueName), EnumValueDef.PyValueDef->Value));
			}
			if (!NewEnum->SetEnums(ValueNames, UEnum::ECppForm::Namespaced))
			{
				PyErr_Format(PyExc_Exception, "Failed to set enum values %s", PyType->tp_name);
				return false;
			}

#if WITH_EDITORONLY_DATA
			// Can't set the meta-data until SetEnums has been called
			for (int32 EnumValueIndex = 0; EnumValueIndex < EnumValueDefs.Num(); ++EnumValueIndex)
			{
				const FEnumValueDef& EnumValueDef = EnumValueDefs[EnumValueIndex];
				FNePyUValueDef::ApplyMetaData(EnumValueDef.PyValueDef, [this, EnumValueIndex](const FString& InMetaDataKey, const FString& InMetaDataValue)
					{
						NewEnum->SetMetaData(*InMetaDataKey, *InMetaDataValue, EnumValueIndex);
					});
			}
#endif // WITH_EDITORONLY_DATA
		}

		// Replace the definitions with real descriptors
		// Now we replace the value defines with enum entries in InitEnumEntries
		FNePyEnumBase::InitEnumEntries(PyType, NewEnum);

		return true;
	}

private:
	FString EnumName;
	PyTypeObject* PyType;
	TArray<FNePySpecifier*> Specifiers;
	UNePyGeneratedEnum* NewEnum;
	bool bDidExist;

	/** Definition data for an Unreal enum value generated from a Python type */
	struct FEnumValueDef
	{
		/** Python definition of the enum value */
		FNePyUValueDef* PyValueDef;

		/** Name of the enum value */
		FString Name;
	};

	/** Array of values generated for this enum */
	TArray<FEnumValueDef> EnumValueDefs;
};


UNePyGeneratedEnum* UNePyGeneratedEnum::GenerateEnum(PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs)
{
	// Builder used to generate the enum
	FNePyGeneratedEnumBuilder PythonEnumBuilder(InPyType->tp_name, InPyType, InPySpecifierPairs);

	TArray<TPair<PyObject*, FNePyUValueDef*>> PyValueDefs;
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			if (FNePyUValueDef* PyValueDef = FNePyUValueDef::Check(FieldValue))
			{
				PyValueDefs.Emplace(FieldKey, PyValueDef);
			}
			else if (FNePyFPropertyDef::Check(FieldValue))
			{
				// Properties are not supported on enums
				PyErr_Format(PyExc_Exception, "%s: Enums do not support properties", InPyType->tp_name);
				return nullptr;
			}
			else if (FNePyUFunctionDef::Check(FieldValue))
			{
				// Functions are not supported on enums
				PyErr_Format(PyExc_Exception, "%s: Enums do not support methods", InPyType->tp_name);
				return nullptr;
			}
		}
	}

	PyValueDefs.Sort([](const TPair<PyObject*, FNePyUValueDef*>& Left, const TPair<PyObject*, FNePyUValueDef*>& Right) {
		return Left.Value->DefineOrder < Right.Value->DefineOrder;
		});

	// Add the values to this enum
	for (const auto& Pair : PyValueDefs)
	{
		const char* FieldName;
		if (!NePyBase::ToCpp(Pair.Key, FieldName))
		{
			continue;
		}

		FNePyUValueDef* PyValueDef = Pair.Value;
		if (!PythonEnumBuilder.CreateValueFromDefinition(FieldName, PyValueDef))
		{
			return nullptr;
		}
	}

	// Finalize the struct with its value meta-data
	return PythonEnumBuilder.Finalize();
}

PyObject* NePyMethod_GenerateEnum(PyObject*, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:uenum", &PyObj))
	{
		return nullptr;
	}

	if (!PyType_Check(PyObj))
	{
		PyErr_SetString(PyExc_TypeError, "@ue.uenum() should be used on a python class.");
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!GNePyDisableGeneratedType)
	{
		PyTypeObject* PyBase = PyType->tp_base;
		if (!PyType_IsSubtype(PyBase, NePyEnumBaseGetType()))
		{
			PyErr_Format(PyExc_Exception, "Type '%s' does not derive from ue.EnumBase",
				PyType->tp_name);
			return nullptr;
		}

		if (PyTuple_GET_SIZE(PyType->tp_bases) > 1)
		{
			PyErr_Format(PyExc_Exception, "Python generated enum '%s' is not allowed to have mutiple bases",
				PyType->tp_name);
			return nullptr;
		}

		TArray<TPair<PyObject*, PyObject*>> PySpecifierPairs;
		if (!NePyGenUtil::ParseSpecifiersFromPyDict(InKwds, PySpecifierPairs))
		{
			return nullptr;
		}

		if (!UNePyGeneratedEnum::GenerateEnum(PyType, PySpecifierPairs))
		{
			if (!PyErr_Occurred())
			{
				PyErr_Format(PyExc_Exception, "Failed to generate an Unreal enum for the Python type '%s'",
					PyType->tp_name);
			}
			return nullptr;
		}
		else
		{
			// 这个过程中可能产生了非阻碍性异常，需要打印出来，并清除错误指示器
			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
		}
	}

	Py_INCREF(PyType);
	return (PyObject*)PyType;
}


struct FNePyUEnumDecorator : public PyObject
{
public:
	PyObject* CachedKwds;

public:
	static FNePyUEnumDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		FNePyUEnumDecorator* Self = (FNePyUEnumDecorator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			FNePyMemoryAllocator::Get().BindOwnerIfTracked(Self);
			Self->CachedKwds = nullptr;
		}
		return Self;
	}

	static void Dealloc(FNePyUEnumDecorator* InSelf)
	{
		Py_XDECREF(InSelf->CachedKwds);
		Py_TYPE(InSelf)->tp_free(InSelf);
	}

	static int Init(FNePyUEnumDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (InKwds)
		{
			Py_INCREF(InKwds);
			InSelf->CachedKwds = InKwds;
		}
		return 0;
	}

	static PyObject* Call(FNePyUEnumDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		return NePyMethod_GenerateEnum(nullptr, InArgs, InSelf->CachedKwds);
	}
};

static PyTypeObject NePyUEnumDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	NePyInternalModuleName ".EnumDecorator", /* tp_name */
	sizeof(FNePyUEnumDecorator), /* tp_basicsize */
};

PyObject* NePyMethod_UEnumDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FNePyUEnumDecorator* PyRet = FNePyUEnumDecorator::New(&NePyUEnumDecoratorType, InArgs, InKwds);
	if (PyRet)
	{
		if (FNePyUEnumDecorator::Init(PyRet, InArgs, InKwds) != 0)
		{
			Py_CLEAR(PyRet);
		}
	}
	return PyRet;
}

PyTypeObject* NePyInitGeneratedEnum()
{
	// @ue.enum装饰器
	PyTypeObject* PyType = &NePyUEnumDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)&FNePyUEnumDecorator::New;
	PyType->tp_dealloc = (destructor)&FNePyUEnumDecorator::Dealloc;
	PyType->tp_init = (initproc)&FNePyUEnumDecorator::Init;
	PyType->tp_call = (ternaryfunc)&FNePyUEnumDecorator::Call;
	PyType_Ready(PyType);
	return PyType;
}
