// Python与UE动态交互的工具函数会放在这个文件里
// 这个文件参考的是 Plugins\Experimental\PythonScriptPlugin\Source\PythonScriptPlugin\Private\PyUtil.h
// 从PyUtil.h搬运方法时，请尽量保证和PyUtil.h中定义的顺序一致
#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"
#include "NePyPtr.h"

namespace NePyUtil
{
	/** Check to see if the given property is an input parameter for a function */
	bool IsInputParameter(const FProperty* InParam);

	/** Check to see if the given property is an output parameter for a function */
	bool IsOutputParameter(const FProperty* InParam);

	/** Given a Python function, get the names of the arguments along with their default values */
	bool InspectFunctionArgs(PyObject* InFunc, TArray<FString>& OutArgNames, TArray<FNePyObjectPtr>* OutArgDefaults = nullptr, bool* bHoutHasDefaults = nullptr);

	/**
	 * Get the doc string of the given object (if any).
	 */
	FString GetDocString(PyObject* InPyObj);

	/**
	 * Get the friendly typename of the given object that can be used in error reporting.
	 */
	FString GetFriendlyTypename(PyTypeObject* InPyType);

	/**
	 * Get the friendly typename of the given object that can be used in error reporting.
	 * @note Passing a PyTypeObject returns the name of that object, rather than 'type'.
	 */
	FString GetFriendlyTypename(PyObject* InPyObj);

	template <typename T>
	FString GetFriendlyTypename(T* InPyObj)
	{
		return GetFriendlyTypename((PyObject*)InPyObj);
	}

	// Helper function to get property type as string
	FString GetPropertyTypeString(const FProperty* Prop);

	// Helper function to get property flags as string
	FString GetPropertyFlagsString(const FProperty* Prop);

	// Helper function to get function flags as string
	FString GetFunctionFlagsString(const UFunction* Func);

	// Get function signature as formatted string (returns empty string if Func is null or Level < 2)
	FString GetFunctionSignatureString(const UFunction* Func, const FString& PyName, int32 Level = 2);

	// Get property info string with pointer information
	FString GetPropertyInfoString(const FProperty* Prop, const char* PropName);

	// Get function info string with pointer information
	FString GetFunctionInfoString(const UFunction* Func, const char* FuncName);
}
