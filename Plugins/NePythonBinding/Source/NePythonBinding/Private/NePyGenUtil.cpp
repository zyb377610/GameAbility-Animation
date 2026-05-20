#include "NePyGenUtil.h"
#include "NePyBase.h"
#include "NePyUtil.h"
#include "NePyGeneratedType.h"
#include "NePyGeneratedStruct.h"
#include "NePyEnumBase.h"
#include "NePySubclassing.h"
#include "NePyStructBase.h"
#include "NePyMemoryAllocator.h"
#include "Misc/ScopeExit.h"
#include "UObject/Class.h"
#include "UObject/ScriptMacros.h"

namespace NePyGenUtil
{
	const FName BlueprintGetterMetaDataKey = TEXT("BlueprintGetter");
	const FName BlueprintSetterMetaDataKey = TEXT("BlueprintSetter");
	const char* InitDefaultFuncName = "__init_default__";
	const char* InitPythonObjectFuncName = "__init_pyobj__";

#pragma region FMethodDef
	static PyTypeObject NePyMethodDefType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"MethodDef", /* tp_name */
		sizeof(FMethodDef), /* tp_basicsize */
	};

	void FMethodDef::InitPyType()
	{
		PyTypeObject* PyType = &NePyMethodDefType;
		PyType->tp_flags = Py_TPFLAGS_DEFAULT;
		PyType->tp_dealloc = (destructor)FMethodDef::Dealloc;
		PyType_Ready(PyType);
	}

	FMethodDef* FMethodDef::New(UFunction* InFunction, const char* InTypeName)
	{
		FMethodDef* MethodDef = (FMethodDef*)PyType_GenericAlloc(&NePyMethodDefType, 0);
		FNePyMemoryAllocator::Get().BindOwnerIfTracked(MethodDef);
		if (MethodDef)
		{
			MethodDef->FuncName = TCHARToUTF8Buffer(*InFunction->GetName());
			MethodDef->Func = InFunction;
			if (!MethodDef->ExtractParams(InTypeName))
			{
				Py_DECREF(MethodDef);
				return nullptr;
			}
		}
		return MethodDef;
	}

	void FMethodDef::Dealloc(FMethodDef* InSelf)
	{
		InSelf->Func = nullptr;
		InSelf->FuncName.~FUTF8Buffer();
		InSelf->InputParams.~TArray();
		InSelf->OutputParams.~TArray();
		Py_TYPE(InSelf)->tp_free(InSelf);
	}

	FMethodDef* FMethodDef::Check(PyObject* InPyObj)
	{
		if (InPyObj && PyObject_IsInstance(InPyObj, (PyObject*)&NePyMethodDefType))
		{
			return (FMethodDef*)InPyObj;
		}
		return nullptr;
	}

	void FMethodDef::Reset()
	{
		Func = nullptr;
		InputParams.Empty();
		OutputParams.Empty();
	}

	bool FMethodDef::ExtractParams(const char* InTypeName)
	{
		TArray<const FProperty*> InputProps;
		TArray<const FProperty*> OutputProps;
		NePyGenUtil::ExtractFunctionParams(Func, InputProps, OutputProps);

		InputParams.Reserve(InputProps.Num());
		for (int32 Index = 0; Index < InputProps.Num(); ++Index)
		{
			const FProperty* Prop = InputProps[Index];
			auto Converter = NePyGetPyObjectToPropertyConverter(Prop);
			if (!Converter)
			{
				PyErr_Format(PyExc_TypeError, "'%s.%s' cannot get converter of input param %d '%s' (%s).",
					InTypeName, TCHAR_TO_UTF8(*Func->GetName()), Index, TCHAR_TO_UTF8(*Prop->GetName()), TCHAR_TO_UTF8(*Prop->GetClass()->GetName()));
				return false;
			}

			InputParams.Add({
					Prop,
					Converter
				});
		}

		OutputParams.Reserve(OutputProps.Num());
		for (int32 Index = 0; Index < OutputProps.Num(); ++Index)
		{
			const FProperty* Prop = OutputProps[Index];
			// 一定要用NoDependency版本，避免因为返回结构体导致指针悬空
			auto Converter = NePyGetPropertyToPyObjectConverterNoDependency(Prop);
			if (!Converter)
			{
				PyErr_Format(PyExc_TypeError, "'%s.%s' cannot get converter of output param %d '%s' (%s).",
					InTypeName, TCHAR_TO_UTF8(*Func->GetName()), Index, TCHAR_TO_UTF8(*Prop->GetName()), TCHAR_TO_UTF8(*Prop->GetClass()->GetName()));
				return false;
			}

			OutputParams.Add({
					Prop,
					Converter
				});
		}

		bIsStatic = EnumHasAnyFlags(Func->FunctionFlags, EFunctionFlags::FUNC_Static);
		return true;
	}
#pragma endregion

#if WITH_EDITOR
	FInitDefaultChecker::FInitDefaultChecker(PyObject* InPyDefaultObject)
		: PyDefaultObject(InPyDefaultObject)
	{
		PyObject** PyDictPtr = _PyObject_GetDictPtr(PyDefaultObject);
		if (PyDictPtr && *PyDictPtr)
		{
			PyDictBefore = NePyStealReference(PyDict_Copy(*PyDictPtr));
		}
	}

	void FInitDefaultChecker::DoCheck()
	{
		PyObject* PyDictAfter = nullptr;
		PyObject** PyDictPtr = _PyObject_GetDictPtr(PyDefaultObject);
		if (PyDictPtr && *PyDictPtr)
		{
			PyDictAfter = *PyDictPtr;
		}

		bool bDictChanged = false;
		if ((PyDictBefore.Get() && !PyDictAfter)
			|| (!PyDictBefore.Get() && PyDictAfter))
		{
			bDictChanged = true;
		}
		else if (PyDictBefore.Get() && PyDictAfter)
		{
			FNePyObjectPtr PyResult = NePyStealReference(PyObject_RichCompare(PyDictBefore, PyDictAfter, Py_EQ));
			if (PyResult != Py_True)
			{
				bDictChanged = true;
			}
		}

		if (bDictChanged)
		{
			PyTypeObject* PyType = Py_TYPE(PyDefaultObject);
			UE_LOG(LogNePython, Warning, TEXT(
				"you are trying to initialize python members in '%s.__init_default__()', which will take no effect."
			), UTF8_TO_TCHAR(PyType->tp_name));

			if (PyDictAfter)
			{
				UE_LOG(LogNePython, Warning, TEXT("members in __dict__: %s"), *NePyBase::PyObjectToString(PyDictAfter));
			}
		}
	}
#endif

	FUTF8Buffer TCHARToUTF8Buffer(const TCHAR* InStr)
	{
		return UTF8ToUTF8Buffer(TCHAR_TO_UTF8(InStr));
	}

	FUTF8Buffer UTF8ToUTF8Buffer(const char* InUTF8Str)
	{
		int32 UTF8StrLen = strlen(InUTF8Str) + 1; // Count includes the null terminator
		FUTF8Buffer UTF8Buffer;
		UTF8Buffer.Append(InUTF8Str, UTF8StrLen);
		return UTF8Buffer;
	}

	PyObject* GetInitDefaultFunc(PyTypeObject* InPyType)
	{
		FNePyObjectPtr InitDefaultFunc = NePyStealReference(PyObject_GetAttrString((PyObject*)InPyType, InitDefaultFuncName));
		if (!InitDefaultFunc)
		{
			PyErr_Clear();
			return nullptr;
		}

		if (!PyCallable_Check(InitDefaultFunc))
		{
			PyErr_Format(PyExc_TypeError, "'%s.%s' is not callable", InPyType->tp_name, InitDefaultFuncName);
			return nullptr;
		}

		return InitDefaultFunc.Release();
	}

	void ExtractFunctionParams(const UFunction* InFunc, TArray<const FProperty*>& OutInputParams, TArray<const FProperty*>& OutOutputParams)
	{
		if (const FProperty* ReturnProp = InFunc->GetReturnProperty())
		{
			OutOutputParams.Add(ReturnProp);
		}

		TArray<const FProperty*> NonRefOutputs, RefOutputs;

		for (TFieldIterator<const FProperty> ParamIt(InFunc); ParamIt; ++ParamIt)
		{
			const FProperty* Param = *ParamIt;

			if (NePyUtil::IsInputParameter(Param))
			{
				OutInputParams.Add(Param);
			}

			if (NePyUtil::IsOutputParameter(Param))
			{
				(Param->HasAnyPropertyFlags(CPF_ReferenceParm) ? RefOutputs : NonRefOutputs).Add(Param);
			}
		}

		// 先非引用输出，后引用输出
		OutOutputParams.Append(NonRefOutputs);
		OutOutputParams.Append(RefOutputs);
	}

	PyObject* PackReturnValues(const void* InBaseParamsAddr, const TArray<const FProperty*>& InOutputParams, UObject* InOwnerObject)
	{
		if (!InOutputParams.Num())
		{
			Py_RETURN_NONE;
		}

		// Do we need to return a packed tuple, or just a single value?
		const int32 NumPropertiesToPack = InOutputParams.Num();
		if (NumPropertiesToPack == 1)
		{
			PyObject* PyRet = NePyBase::TryConvertFPropertyToPyObjectInContainerNoDependency(InOutputParams[0], InBaseParamsAddr, 0, InOwnerObject);
			if (!PyRet)
			{
				NePyBase::SetConvertFPropertyToPyObjectError(InOutputParams[0]);
				return nullptr;
			}
			return PyRet;
		}
		else
		{
			FNePyObjectPtr OutParamTuple = NePyStealReference(PyTuple_New(NumPropertiesToPack));
			for (int32 OutParamIndex = 0; OutParamIndex < InOutputParams.Num(); ++OutParamIndex)
			{
				PyObject* OutParamPyObj = NePyBase::TryConvertFPropertyToPyObjectInContainerNoDependency(InOutputParams[OutParamIndex], InBaseParamsAddr, 0, InOwnerObject);
				if (!OutParamPyObj)
				{
					NePyBase::SetConvertFPropertyToPyObjectError(InOutputParams[OutParamIndex]);
					return nullptr;
				}
				PyTuple_SetItem(OutParamTuple, OutParamIndex, OutParamPyObj); // SetItem steals the reference
			}
			return OutParamTuple.Release();
		}
	}

	PyObject* PackReturnValues(const void* InBaseParamsAddr, const TArray<OutputParamDef>& InOutputParams, UObject* InOwnerObject)
	{
		if (!InOutputParams.Num())
		{
			Py_RETURN_NONE;
		}

		// Do we need to return a packed tuple, or just a single value?
		const int32 NumPropertiesToPack = InOutputParams.Num();
		if (NumPropertiesToPack == 1)
		{
			const FProperty* Prop = InOutputParams[0].Prop;
			auto& Converter = InOutputParams[0].Converter;
			PyObject* PyRet = Converter(Prop, Prop->ContainerPtrToValuePtr<void>(InBaseParamsAddr), InOwnerObject);
			if (!PyRet)
			{
				NePyBase::SetConvertFPropertyToPyObjectError(Prop);
				return nullptr;
			}
			return PyRet;
		}
		else
		{
			FNePyObjectPtr OutParamTuple = NePyStealReference(PyTuple_New(NumPropertiesToPack));
			for (int32 OutParamIndex = 0; OutParamIndex < InOutputParams.Num(); ++OutParamIndex)
			{
				const FProperty* Prop = InOutputParams[OutParamIndex].Prop;
				auto& Converter = InOutputParams[OutParamIndex].Converter;
				PyObject* OutParamPyObj = Converter(Prop, Prop->ContainerPtrToValuePtr<void>(InBaseParamsAddr), InOwnerObject);
				if (!OutParamPyObj)
				{
					NePyBase::SetConvertFPropertyToPyObjectError(Prop);
					return nullptr;
				}
				PyTuple_SetItem(OutParamTuple, OutParamIndex, OutParamPyObj); // SetItem steals the reference
			}
			return OutParamTuple.Release();
		}
	}

	bool UnpackReturnValues(PyObject* InRetVals, const FOutParmRec* InOutputParms, const UFunction* InFunc, const UObject* InThisObject)
	{
		if (!InOutputParms)
		{
			return true;
		}

		const FOutParmRec* OutParamRec = InOutputParms;

		// Do we need to expect a packed tuple, or just a single value?
		if (OutParamRec->NextOutParm == nullptr)
		{
			if (!NePyBase::TryConvertPyObjectToFPropertyDirect(InRetVals, OutParamRec->Property, OutParamRec->PropAddr, const_cast<UObject*>(InThisObject)))
			{
				UE_LOG(LogNePython, Error, TEXT("Failed to convert return property '%s' (%s) when calling function '%s' on '%s'"),
					*OutParamRec->Property->GetName(), *OutParamRec->Property->GetClass()->GetName(), *InFunc->GetName(), *InThisObject->GetName());
				return false;
			}
		}
		else
		{
			if (!PyTuple_Check(InRetVals))
			{
				UE_LOG(LogNePython, Error, TEXT("Expected a 'tuple' return type, but got '%s' when calling function '%s' on '%s'"),
					UTF8_TO_TCHAR(InRetVals->ob_type->tp_name), *InFunc->GetName(), *InThisObject->GetName());
				return false;
			}

			check(OutParamRec->NextOutParm);
			int32 NumPropertiesToUnpack = 2; // We can start from 2 since we know we already have at least 2 output parameters to get to this code
			for (const FOutParmRec* Tmp = OutParamRec->NextOutParm->NextOutParm; Tmp; Tmp = Tmp->NextOutParm)
			{
				++NumPropertiesToUnpack;
			}

			const int32 RetTupleSize = PyTuple_Size(InRetVals);
			if (RetTupleSize != NumPropertiesToUnpack)
			{
				UE_LOG(LogNePython, Error, TEXT("Expected a 'tuple' return type containing '%d' items but got one containing '%d' items when calling function '%s' on '%s'"),
					NumPropertiesToUnpack, RetTupleSize, *InFunc->GetName(), *InThisObject->GetName());
				return false;
			}

			int32 RetTupleIndex = 0;
			do
			{
				PyObject* RetVal = PyTuple_GetItem(InRetVals, RetTupleIndex++);
				if (!NePyBase::TryConvertPyObjectToFPropertyDirect(RetVal, OutParamRec->Property, OutParamRec->PropAddr, const_cast<UObject*>(InThisObject)))
				{
					UE_LOG(LogNePython, Error, TEXT("Failed to convert return property '%s' (%s) when calling function '%s' on '%s'"),
						*OutParamRec->Property->GetName(), *OutParamRec->Property->GetClass()->GetName(), *InFunc->GetName(), *InThisObject->GetName());
					return false;
				}
				OutParamRec = OutParamRec->NextOutParm;
			} while (OutParamRec);
		}

		return true;
	}

	bool InvokePythonCallableFromUnrealFunctionThunk(FNePyObjectPtr& InSelf, PyObject* InCallable, const UFunction* InFunc, UObject* Context, FFrame& Stack, RESULT_DECL)
	{
		// Allocate memory to store our local argument data
		void* LocalStruct = FMemory_Alloca(FMath::Max<int32>(1, InFunc->GetStructureSize()));
		InFunc->InitializeStruct(LocalStruct);
		ON_SCOPE_EXIT
		{
			InFunc->DestroyStruct(LocalStruct);
		};

		// Stores information about inputs and outputs
		FOutParmRec* OutParms = nullptr;
		FOutParmRec* OutRefParms = nullptr;
		TArray<FNePyObjectPtr, TInlineAllocator<4>> PyParams;

		// Add any return property to the output params chain
		if (FProperty* ReturnProp = InFunc->GetReturnProperty())
		{
			FOutParmRec* Out = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));
			Out->Property = ReturnProp;
			Out->PropAddr = (uint8*)RESULT_PARAM;

			// Link it to the head of the list, as UnpackReturnValues expects the return value to be first in the list
			Out->NextOutParm = OutParms;
			OutParms = Out;
		}

		// Get the value of the input params for the Python args, and cache the addresses that return and output data should be unpacked to
		bool bProcessedInputs = true;
		{
			int32 ArgIndex = 0;
			FOutParmRec** LastOut = &OutParms;
			FOutParmRec** LastRefOut = &OutRefParms;

			// We iterate the fields directly here as we need to process input and output properties in the 
			// correct stack order, as we're potentially popping data off the bytecode stack
			for (TFieldIterator<FProperty> ParamIt(InFunc); ParamIt; ++ParamIt)
			{
				FProperty* Param = *ParamIt;

				// Skip the return value; it never has data on the bytecode stack and was added to the output params chain before this loop
				if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					continue;
				}

				// Step the property data to populate the local value
				Stack.MostRecentPropertyAddress = nullptr;
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6
				Stack.MostRecentPropertyContainer = nullptr;
#endif
				void* LocalValue = Param->ContainerPtrToValuePtr<void>(LocalStruct);
				Stack.StepCompiledIn(LocalValue, Param->GetClass());

				// Output parameters (even const ones) need to read their data from the property address (if available) rather than the local struct
				void* ValueAddress = LocalValue;
				if (Param->HasAnyPropertyFlags(CPF_OutParm) && Stack.MostRecentPropertyAddress)
				{
					ValueAddress = Stack.MostRecentPropertyAddress;
				}

				// Add any output parameters to the output params chain
				if (NePyUtil::IsOutputParameter(Param))
				{
					CA_SUPPRESS(6263) // using _alloca in a loop
					FOutParmRec* Out = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));
					Out->Property = Param;
					Out->PropAddr = (uint8*)ValueAddress;
					Out->NextOutParm = nullptr;

					if (Param->HasAnyPropertyFlags(CPF_ReferenceParm))
					{
						// Link it to the end of the list (ref)
						if (*LastRefOut)
						{
							(*LastRefOut)->NextOutParm = Out;
							LastRefOut = &(*LastRefOut)->NextOutParm;
						}
						else
						{
							*LastRefOut = Out;
						}
					}
					else
					{
						// Link it to the end of the list
						if (*LastOut)
						{
							(*LastOut)->NextOutParm = Out;
							LastOut = &(*LastOut)->NextOutParm;
						}
						else
						{
							*LastOut = Out;
						}
					}
				}

				// Convert any input parameters for use with Python
				if (NePyUtil::IsInputParameter(Param))
				{
					FNePyObjectPtr& PyParam = PyParams.AddDefaulted_GetRef();
					PyParam.Get() = NePyBase::TryConvertFPropertyToPyObjectDirectNoDependency(Param, (uint8*)ValueAddress, Context); // 此处相当于Steal
					if (!PyParam.Get())
					{
						UE_LOG(LogNePython, Error, TEXT("Failed to convert argument at pos '%d' when calling function '%s' on '%s'"),
							ArgIndex + 1, *InFunc->GetName(), *P_THIS_OBJECT->GetName());
						bProcessedInputs = false;
					}
					++ArgIndex;
				}
			}
		}

		// Validate we reached the end of the parameters when stepping the bytecode stack
		if (Stack.Code)
		{
			checkSlow(*Stack.Code == EX_EndFunctionParms);
			++Stack.Code;
		}

		// If any errors happened during parameter processing then we can bail now that we've finished stepping the bytecode stack
		// We can also bail if we have no Python callable to invoke
		if (!bProcessedInputs || !InCallable)
		{
			return false;
		}

		// Prepare the arguments tuple for the Python callable
		FNePyObjectPtr PyArgs;
		if (InSelf || PyParams.Num() > 0)
		{
			const int32 PyParamOffset = (InSelf ? 1 : 0);
			PyArgs = NePyStealReference(PyTuple_New(PyParams.Num() + PyParamOffset));
			if (InSelf)
			{
				PyTuple_SetItem(PyArgs, 0, InSelf.Release()); // SetItem steals the reference
			}
			for (int32 PyParamIndex = 0; PyParamIndex < PyParams.Num(); ++PyParamIndex)
			{
				PyTuple_SetItem(PyArgs, PyParamIndex + PyParamOffset, PyParams[PyParamIndex].Release()); // SetItem steals the reference
			}
		}

		// Invoke the Python callable
		FNePyObjectPtr RetVals = NePyStealReference(PyObject_CallObject(InCallable, PyArgs));
		if (!RetVals)
		{
			return false;
		}

		// Append the out reference params at the end of out params
		if (OutParms)
		{
			FOutParmRec* OutParmsLast = OutParms;
			while (OutParmsLast->NextOutParm)
			{
				OutParmsLast = OutParmsLast->NextOutParm;
			}
			OutParmsLast->NextOutParm = OutRefParms;
		}
		else
		{
			OutParms = OutRefParms;
		}

		// Unpack any output values
		if (!UnpackReturnValues(RetVals, OutParms, InFunc, P_THIS_OBJECT))
		{
			return false;
		}

		return true;
	}

	void UpdateReloadedPropertyStructReferences(const UStruct* InStruct)
	{
#if ENGINE_MAJOR_VERSION >= 5
		auto PropIt = TFieldIterator<FStructProperty>(InStruct, EFieldIterationFlags::IncludeDeprecated);
#else
		auto PropIt = TFieldIterator<FStructProperty>(InStruct, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::IncludeDeprecated, EFieldIteratorFlags::ExcludeInterfaces);
#endif
		for (; PropIt; ++PropIt)
		{
			FStructProperty* Prop = *PropIt;
			UNePyGeneratedStruct* OldStruct = Cast<UNePyGeneratedStruct>(Prop->Struct);
			if (!OldStruct)
			{
				continue;
			}

			UNePyGeneratedStruct* NewStruct = OldStruct->NewStruct;
			if (!NewStruct)
			{
				continue;
			}

			// 找到最新的类型
			while (NewStruct->NewStruct)
			{
				NewStruct = NewStruct->NewStruct;
			}
			Prop->Struct = NewStruct;
			Prop->ClearPropertyFlags(CPF_ComputedFlags);
		}
	}

	bool ParseSpecifiersFromPyDict(PyObject* InKwds, TArray<TPair<PyObject*, PyObject*>>& OutPySpecifierPairs)
	{
		check(PyGILState_Check());

		OutPySpecifierPairs.Empty();
		if (!InKwds)
		{
			return true;
		}
		
		PyObject* KwdKey = nullptr;
		PyObject* KwdVal = nullptr;
		Py_ssize_t KwdIndex = 0;
		while (PyDict_Next(InKwds, &KwdIndex, &KwdKey, &KwdVal))
		{
			OutPySpecifierPairs.Emplace(KwdKey, KwdVal);
		}
		return true;
	}

	void RegularizeUFunctionName(FString& InName)
	{
		constexpr TCHAR K2Prefix[] = TEXT("K2_");
		constexpr size_t K2PrefixLen = sizeof(K2Prefix) / sizeof(TCHAR) - 1;

		if (InName.StartsWith(K2Prefix))
		{
			InName.MidInline(K2PrefixLen);
		}
	}

	bool FillFuncWithParams(UFunction* Func, const TArray<FString>& FuncArgNames, PyObject* FuncParamTypes, PyObject* FuncRetType, UClass* ThisClass)
	{
		const FString OwnerName = Func->GetOuter()->GetName() + TEXT(".") + Func->GetName();
		
		// Adding properties to a function inserts them into a linked list, so we add the return and output values first so that they appear at the end
		if (FuncRetType && FuncRetType != Py_None)
		{
			// 提取返回值类型
			TArray<PyObject*> RetTypes;
			Py_ssize_t NumRetTypes = PySequence_Size(FuncRetType);
			for (Py_ssize_t Index = 0; Index < NumRetTypes; ++Index)
			{
				RetTypes.Add(PySequence_GetItem(FuncRetType, Index));
			}
			if (RetTypes.Num() == 1 && RetTypes[0] == Py_None)
			{
				RetTypes.Empty();
			}

			if (RetTypes.Num() > 0)
			{
				FName AttrName = TEXT("ReturnValue");
				PyObject* RetType = RetTypes[0];
				FNePyUParamDef* RetParamDef = FNePyUParamDef::Check(RetType);
				if (RetParamDef)
				{
					RetType = RetParamDef->PropType;
				}
				FProperty* RetProp = NePySubclassingNewProperty(Func, AttrName, RetType,
					OwnerName, CPF_Parm | CPF_OutParm | CPF_ReturnParm, nullptr, ThisClass);
				if (!RetProp)
				{
					PyErr_Format(PyExc_Exception, "Failed to create return property (%s) for function '%s'", TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(RetType)), TCHAR_TO_UTF8(*OwnerName));
					return false;
				}
				if (RetParamDef)
				{
					FNePySpecifier::ApplyToParam(RetParamDef->Specifiers, RetProp);
				}
				Func->AddCppProperty(RetProp);
			}

			for (int32 ArgIndex = 1; ArgIndex < RetTypes.Num(); ++ArgIndex)
			{
				FName AttrName = *FString::Printf(TEXT("OutValue%d"), ArgIndex);
				PyObject* ArgType = RetTypes[ArgIndex];
				FNePyUParamDef* ArgParamDef = FNePyUParamDef::Check(ArgType);
				if (ArgParamDef)
				{
					ArgType = ArgParamDef->PropType;
				}
				FProperty* ArgProp = NePySubclassingNewProperty(Func, AttrName, ArgType,
					OwnerName, CPF_Parm | CPF_OutParm, nullptr, ThisClass);
				if (!ArgProp)
				{
					PyErr_Format(PyExc_Exception, "Failed to create output property (%s) for function '%s' at index %d", TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(ArgType)), TCHAR_TO_UTF8(*OwnerName), ArgIndex);
					return false;
				}
				if (ArgParamDef)
				{
					FNePySpecifier::ApplyToParam(ArgParamDef->Specifiers, ArgProp);
				}
				Func->AddCppProperty(ArgProp);
				Func->FunctionFlags |= FUNC_HasOutParms;
			}
		}

		// Adding properties to a function inserts them into a linked list, so we loop backwards to get the order right
		{
			int32 ArgIndex = FuncArgNames.Num() - 1;
			while (ArgIndex >= 0)
			{
				PyObject* ArgTypeObj = PySequence_GetItem(FuncParamTypes, ArgIndex);
				FNePyUParamDef* ArgParamDef = FNePyUParamDef::Check(ArgTypeObj);
				if (ArgParamDef)
				{
					ArgTypeObj = ArgParamDef->PropType;
				}
				FProperty* ArgProp = NePySubclassingNewProperty(Func, *FuncArgNames[ArgIndex], ArgTypeObj,
					OwnerName, CPF_Parm, nullptr, ThisClass);
				if (!ArgProp)
				{
					PyErr_Format(PyExc_Exception, "Failed to create property (%s) for function '%s' argument '%s'", TCHAR_TO_UTF8(*NePyUtil::GetFriendlyTypename(ArgTypeObj)), TCHAR_TO_UTF8(*OwnerName), TCHAR_TO_UTF8(*FuncArgNames[ArgIndex]));
					return false;
				}
				if (ArgParamDef)
				{
					FNePySpecifier::ApplyToParam(ArgParamDef->Specifiers, ArgProp);
				}
				Func->AddCppProperty(ArgProp);
				if (ArgProp->GetPropertyFlags() & CPF_OutParm)
				{
					Func->FunctionFlags |= FUNC_HasOutParms;
				}
				ArgIndex--;
			}
		}

		if (Func->HasAnyFunctionFlags(FUNC_Delegate))
		{
			for (TFieldIterator<FProperty> ParamIt(Func); ParamIt; ++ParamIt)
			{
				FProperty* Param = *ParamIt;
				if (FArrayProperty* ArrayParam = CastField<FArrayProperty>(Param))
				{
					// 蓝图中默认对数组类型的参数开启了‘引用传参’（估计又是个历史遗留问题）
					// 如果Function是个Delegate，且我们不进行同样的处理，则会因为函数签名不同而无法绑定
					// 参见 FBlueprintCompilationManagerImpl::FastGenerateSkeletonClass
					ArrayParam->PropertyFlags |= (CPF_OutParm | CPF_ReferenceParm);
					Func->FunctionFlags |= FUNC_HasOutParms;
				}
			}
		}
		return true;
	}

	bool SupportDefaultPropertyValue(PyObject* PyAttrVal)
	{
		if (PyBool_Check(PyAttrVal)
#if PY_MAJOR_VERSION < 3
			|| PyInt_Check(PyAttrVal)
#endif
			|| PyLong_Check(PyAttrVal) || PyFloat_Check(PyAttrVal) || NePyString_Check(PyAttrVal) || NePyEnumBaseCheck(PyAttrVal))
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	FProperty* CreatePropertyFromDefaultValue(FFieldVariant Owner, const FName& AttrName, PyObject* PyAttrVal, EObjectFlags ObjectFlags)
	{
		FProperty* NewProp = nullptr;

		// 形如:
		// class MyActor(ue.Actor):
		//     XXX = ue.uproperty(ue.ECollisionChannel.ECC_Camera)
		if (!NewProp)
		{
			if (FNePyEnumBase* EnumEntry = NePyEnumBaseCheck(PyAttrVal))
			{
				UEnum* Enum = const_cast<UEnum*>(FNePyEnumBase::GetEnum(EnumEntry));
				FEnumProperty* EnumProp = new FEnumProperty(Owner, AttrName, ObjectFlags);
				EnumProp->SetEnum(Enum);
				FProperty* UnderlyingProp = NePyNewEnumUnderlyingProperty(EnumProp, ObjectFlags);
				EnumProp->AddCppProperty(UnderlyingProp);
				NewProp = EnumProp;
			}
		}

		// 形如:
		// class MyActor(ue.Actor):
		//     XXX = ue.uproperty(True)
		if (!NewProp && PyBool_Check(PyAttrVal))
		{
			NewProp = new FBoolProperty(Owner, AttrName, ObjectFlags);
			((FBoolProperty*)NewProp)->SetBoolSize(sizeof(bool), true);
		}

		// 形如:
		// class MyActor(ue.Actor):
		//     XXX = ue.uproperty(1)
#if PY_MAJOR_VERSION < 3
		if (!NewProp && PyInt_Check(PyAttrVal))
		{
			NewProp = new FIntProperty(Owner, AttrName, ObjectFlags);
		}
#endif

		if (!NewProp && PyLong_Check(PyAttrVal))
		{
#if NEPY_SUBCLASSING_PY_LONG_AS_UE_INT64
			NewProp = new FInt64Property(Owner, AttrName, ObjectFlags);
#else
			NewProp = new FIntProperty(Owner, AttrName, ObjectFlags);
#endif
		}

		// 形如:
		// class MyActor(ue.Actor):
		//     XXX = ue.uproperty(1.0)
		if (!NewProp && PyFloat_Check(PyAttrVal))
		{
#if NEPY_SUBCLASSING_PY_FLOAT_AS_UE_DOUBLE
			NewProp = new FDoubleProperty(Owner, AttrName, ObjectFlags);
#else
			NewProp = new FFloatProperty(Owner, AttrName, ObjectFlags);
#endif
		}

		// 形如:
		// class MyActor(ue.Actor):
		//     XXX = ue.uproperty("True")
		if (!NewProp && NePyString_Check(PyAttrVal))
		{
			NewProp = new FStrProperty(Owner, AttrName, ObjectFlags);
		}

		return NewProp;
	}

	bool AssignPropertyDefaultValue(void* Container, TArray<TSharedPtr<NePyGenUtil::FPropertyDef>>& PropertyDefs)
	{
		if (!Container)
		{
			return false;
		}

		bool Success = true;

		for (const TSharedPtr<NePyGenUtil::FPropertyDef>& PropDef : PropertyDefs)
		{
			if (PropDef->DefaultValue)
			{
				FProperty* Property = PropDef->Prop;
				PyObject* PyAttrVal = PropDef->DefaultValue;

				// 检查顺序要跟CreatePropertyFromDefaultValue严格对应
				bool Assigned = false;

				if (!Assigned && NePyEnumBaseCheck(PyAttrVal))
				{
					FEnumProperty* EnumProperty = (FEnumProperty*)Property;
					FNePyObjectPtr PyLong = NePyStealReference(PyNumber_Long(PyAttrVal));
					uint64 Val = PyLong_AsUnsignedLongLong(PyLong);
					void* Buffer = EnumProperty->ContainerPtrToValuePtr<void>(Container);
					EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(Buffer, Val);

					Assigned = true;
				}

				if (!Assigned && PyBool_Check(PyAttrVal))
				{
					FBoolProperty* BoolProperty = (FBoolProperty*)Property;
					bool Val = PyAttrVal == Py_True;
#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
					BoolProperty->SetPropertyValue_InContainer(Container, Val);
#else
					BoolProperty->SetValue_InContainer(Container, &Val);
#endif
					Assigned = true;
				}

#if PY_MAJOR_VERSION < 3
				if (!Assigned && PyInt_Check(PyAttrVal))
				{
					int Val = (int)PyInt_AsLong(PyAttrVal);
					FIntProperty* IntProperty = (FIntProperty*)Property;
#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
					IntProperty->SetPropertyValue_InContainer(Container, Val);
#else
					IntProperty->SetValue_InContainer(Container, Val);
#endif
					Assigned = true;
				}
#endif

				if (!Assigned && PyLong_Check(PyAttrVal))
				{
#if NEPY_SUBCLASSING_PY_LONG_AS_UE_INT64
					FInt64Property* IntProperty = (FInt64Property*)Property;
					int64 Val = PyLong_AsLong(PyAttrVal);
#else
					FIntProperty* IntProperty = (FIntProperty*)Property;
					int32 Val = (int32)PyLong_AsLong(PyAttrVal);
#endif
#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
					IntProperty->SetPropertyValue_InContainer(Container, Val);
#else
					IntProperty->SetValue_InContainer(Container, Val);
#endif
					Assigned = true;
				}

				if (!Assigned && PyFloat_Check(PyAttrVal))
				{
#if NEPY_SUBCLASSING_PY_FLOAT_AS_UE_DOUBLE
					FDoubleProperty* FloatProperty = (FDoubleProperty*)Property;
					double Val = PyFloat_AsDouble(PyAttrVal);
#else
					FFloatProperty* FloatProperty = (FFloatProperty*)Property;
					float Val = (float)PyFloat_AsDouble(PyAttrVal);
#endif
#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
					FloatProperty->SetPropertyValue_InContainer(Container, Val);
#else
					FloatProperty->SetValue_InContainer(Container, Val);
#endif
					Assigned = true;
				}

				if (!Assigned && NePyString_Check(PyAttrVal))
				{
					FStrProperty* StrProperty = (FStrProperty*)Property;
					const char* Val = NePyString_AsString(PyAttrVal);
#if ENGINE_MAJOR_VERSION < 5 || ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
					StrProperty->SetPropertyValue_InContainer(Container, Val);
#else
					StrProperty->SetValue_InContainer(Container, Val);
#endif
					Assigned = true;
				}

				if (!Success)
				{
					UE_LOG(LogNePython, Error, TEXT("Assign property default value fail for %s, should never reach."), *PropDef->Prop->GetName());
				}

				Success &= Assigned;
			}
		}

		return Success;
	}

	void StaticReparentPythonType(PyTypeObject* InPyType, PyTypeObject* InNewBasePyType)
	{
		auto UpdateTuple = [](PyObject* InTuple, PyTypeObject* InOldType, PyTypeObject* InNewType)
		{
			if (InTuple)
			{
				const int32 TupleSize = PyTuple_Size(InTuple);
				for (int32 TupleIndex = 0; TupleIndex < TupleSize; ++TupleIndex)
				{
					if (PyTuple_GetItem(InTuple, TupleIndex) == (PyObject*)InOldType)
					{
						FNePyTypeObjectPtr NewType = NePyNewReference(InNewType);
						PyTuple_SetItem(InTuple, TupleIndex, (PyObject*)NewType.Release()); // PyTuple_SetItem steals the reference
					}
				}
			}
		};

		FNePyTypeObjectPtr OldBasePyType = NePyStealReference(InPyType->tp_base);
		InPyType->tp_base = NePyNewReference(InNewBasePyType).Release();
		UpdateTuple(InPyType->tp_bases, OldBasePyType, InNewBasePyType);

		FNePyObjectPtr MroName = NePyStealReference(NePyString_FromString("mro"));
		PyObject* MroMethod = _PyType_Lookup(&PyType_Type, MroName);
		if (MroMethod)
		{
			PyObject* OldMro = InPyType->tp_mro;
			if (OldMro && OldMro->ob_refcnt > 1)
			{
				// 还有别人持有tp_mro，我们无法安全地替换
				PyErr_BadInternalCall();
				return;
			}

			FNePyObjectPtr Args = NePyStealReference(PyTuple_Pack(1, InPyType));
			FNePyObjectPtr NewMro = NePyStealReference(PyObject_Call(MroMethod, Args, nullptr));
			check(NewMro);
			InPyType->tp_mro = PySequence_Tuple(NewMro);
			Py_XDECREF(OldMro);
		}
		else
		{
			// 这是什么版本的python，居然没有mro这个方法
			// fallback回保底方案，只替换mro里的直接基类，不处理基类的基类了
			UpdateTuple(InPyType->tp_mro, OldBasePyType, InNewBasePyType);
		}

		PyType_Modified(InPyType);
	}
}
