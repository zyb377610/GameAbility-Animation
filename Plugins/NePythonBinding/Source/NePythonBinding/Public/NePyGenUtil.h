// Python与UE动态交互的工具函数会放在这个文件里
// 这个文件参考的是 Plugins\Experimental\PythonScriptPlugin\Source\PythonScriptPlugin\Private\PyGenUtil.h
// 从PyGenUtil.h搬运方法时，请尽量保证和PyGenUtil.h中定义的顺序一致
#pragma once
#include "CoreMinimal.h"
#include "NePyIncludePython.h"
#include "NePyPtr.h"
#include "NePyPropertyConvert.h"
#include "UObject/Stack.h"

namespace NePyGenUtil
{
	extern const FName BlueprintGetterMetaDataKey;
	extern const FName BlueprintSetterMetaDataKey;
	extern const char* InitDefaultFuncName;
	extern const char* InitPythonObjectFuncName;

	/** Buffer for storing the UTF-8 strings used by Python types */
	typedef TArray<char> FUTF8Buffer;

	// 输入参数定义
	struct InputParamDef
	{
		// 属性
		const FProperty* Prop;
		// 转换函数
		NePyPyObjectToPropertyFunc Converter;
	};

	// 返回值与输出参数定义
	struct OutputParamDef
	{
		// 属性
		const FProperty* Prop;
		// 转换函数
		NePyPropertyToPyObjectFunc Converter;
	};

	// 动态绑定的函数定义
	struct FMethodDef : public PyObject
	{
		// 函数名
		FUTF8Buffer FuncName;
		// 对应的UE函数
		UFunction* Func;
		// 输入参数列表
		TArray<InputParamDef> InputParams;
		// 输出参数列表
		TArray<OutputParamDef> OutputParams;
		// 是否为静态函数
		bool bIsStatic;

		// 初始化PyType类型
		static void InitPyType();

		// 生成Python对象
		static FMethodDef* New(UFunction* InFunction, const char* InTypeName);

		// tp_dealloc
		static void Dealloc(FMethodDef* InSelf);

		// 判断Python对象是否为此类的实例
		static FMethodDef* Check(PyObject* InPyObj);

		// 切断与UFunction的联系
		void Reset();

	private:
		// 提取函数参数和返回值信息
		bool ExtractParams(const char* InTypeName);
	};

	/** Definition data for an Unreal property generated from a Python type */
	struct FPropertyDef
	{
		FPropertyDef() = default;
		FPropertyDef(FPropertyDef&&) = default;
		FPropertyDef(const FPropertyDef&) = delete;
		FPropertyDef& operator=(FPropertyDef&&) = default;
		FPropertyDef& operator=(const FPropertyDef&) = delete;

		/** The name of the get/set */
		FUTF8Buffer GetSetName;

		/** The Unreal property for this get/set */
		FProperty* Prop = nullptr;

		/** The Unreal function for the get (if any) */
		UFunction* GetFunc = nullptr;

		/** The Unreal function for the set (if any) */
		UFunction* SetFunc = nullptr;

		/** Default value of this property */
		FNePyObjectPtrWithGIL DefaultValue;
	};

	/** Definition data for an Unreal component generated from a Python type */
	struct FComponentDef
	{
		FComponentDef() = default;
		FComponentDef(FComponentDef&&) = default;
		FComponentDef(const FComponentDef&) = delete;
		FComponentDef& operator=(FComponentDef&&) = default;
		FComponentDef& operator=(const FComponentDef&) = delete;

		/** The Unreal property for UActorComponent */
		FProperty* Prop = nullptr;

		/** The Unreal UActorComponent */
		UClass* ComponentClass = nullptr;

		/** Is is RootComponent? */
		bool bRoot;

		/** The name of component field */
		FUTF8Buffer FieldName;

		/** The name of component which to attach with */
		FName AttachName;

		/** The name of socket which to attach with */
		FName SocketName;

		/** The name of component which to Override in the parent class */
		FName OverrideName;
	};

	/** Definition data for an Unreal delegate generated from a Python type */
	struct FDelegateDef
	{
		FDelegateDef() = default;
		FDelegateDef(FDelegateDef&&) = default;
		FDelegateDef(const FDelegateDef&) = delete;
		FDelegateDef& operator=(FDelegateDef&&) = default;
		FDelegateDef& operator=(const FDelegateDef&) = delete;

		/** The name of the get/set */
		FUTF8Buffer GetSetName;

		/** The Unreal property for this get/set */
		FMulticastInlineDelegateProperty* Prop = nullptr;
	};

	// 辅助类，用于检测用户是否在__init_default__函数中初始化了普通python成员
#if WITH_EDITOR
	class FInitDefaultChecker
	{
	public:
		explicit FInitDefaultChecker(PyObject* InPyDefaultObject);
		void DoCheck();

	private:
		PyObject* PyDefaultObject;
		FNePyObjectPtr PyDictBefore;
	};
#endif


	/** Convert TCHAR* to a UTF-8 buffer */
	FUTF8Buffer TCHARToUTF8Buffer(const TCHAR* InStr);

	/** Copy char* to a UTF-8 buffer */
	FUTF8Buffer UTF8ToUTF8Buffer(const char* InUTF8Str);

	/** Get the InitDefault function from this Python type */
	PyObject* GetInitDefaultFunc(PyTypeObject* InPyType);

	/** Given a function, extract all of its parameter information (input and output) */
	void ExtractFunctionParams(const UFunction* InFunc, TArray<const FProperty*>& OutInputParams, TArray<const FProperty*>& OutOutputParams);

	/** Ensure that the memory referenced within object is valid */
	PyObject* ValidateObjectMemory(PyObject* InObject);

	/** Given a set of return values and the struct data associated with them, pack them appropriately for returning to Python */
	PyObject* PackReturnValues(const void* InBaseParamsAddr, const TArray<const FProperty*>& InOutputParams, UObject* InOwnerObject);

	/** Given a set of return values and the struct data associated with them, pack them appropriately for returning to Python */
	PyObject* PackReturnValues(const void* InBaseParamsAddr, const TArray<OutputParamDef>& InOutputParams, UObject* InOwnerObject);

	/** Given a Python return value, unpack the values into the struct data associated with them */
	bool UnpackReturnValues(PyObject* InRetVals, const FOutParmRec* InOutputParms, const UFunction* InFunc, const UObject* InThisObject);

	/** Invoke a Python callable from an Unreal function thunk (ie, DEFINE_FUNCTION), ensuring that the calling convention is correct for both native and script function callers */
	bool InvokePythonCallableFromUnrealFunctionThunk(FNePyObjectPtr& InSelf, PyObject* InCallable, const UFunction* InFunc, UObject* Context, FFrame& Stack, RESULT_DECL);

	// 在 UNePyGeneratedStruct reload 以后，更新属性列表中的引用
	void UpdateReloadedPropertyStructReferences(const UStruct* InStruct);
	
	// 从字典参数中解析出Specifier数组
	bool ParseSpecifiersFromPyDict(PyObject* InKwds, TArray<TPair<PyObject*, PyObject*>>& OutPySpecifierPairs);

#if WITH_EDITORONLY_DATA
	void ApplyMetaData(PyObject* InMetaData, const TFunctionRef<void(const FString&, const FString&)>& InPredicate);

	// 获取 Python 模块相对路径
	FString GetPythonModuleRelativePathForPyType(PyTypeObject* InPyType);
#endif // WITH_EDITORONLY_DATA

	// 去掉UFuncion名称中的'K2_'前缀
	void RegularizeUFunctionName(FString& InName);

	// 为UFunction创建参数和返回值
	bool FillFuncWithParams(UFunction* Func, const TArray<FString> &FuncArgNames, PyObject* FuncParamTypes, PyObject* FuncRetType = nullptr, UClass* ThisClass = nullptr);
	
	// 是否为支持的FProperty默认值类型
	bool SupportDefaultPropertyValue(PyObject* DefaultValue);

	// 从默认值尝试创建FProperty，返回为NULL则创建失败
	FProperty* CreatePropertyFromDefaultValue(FFieldVariant Owner, const FName& AttrName, PyObject* DefaultValue, EObjectFlags ObjectFlags);

	// 给UClass/UStruct属性赋默认值
	bool AssignPropertyDefaultValue(void* Container, TArray<TSharedPtr<NePyGenUtil::FPropertyDef>>& PropertyDefs);

	// 调整Python类型的基类
	void StaticReparentPythonType(PyTypeObject* InPyType, PyTypeObject* InNewBasePyType);
}
