#pragma once
#include <atomic>
#include "Misc/EnumRange.h"
#include "NePyIncludePython.h"
#include "NePyAutoExportVersion.h"
#include "NePySpecifiers.h"
#include "UObject/Interface.h"
#include "UObject/Package.h"

// ue.internal内部模块名字
#define NePyInternalModuleName NePyRootModuleName ".internal"
// ue.internal字符串长度（包含结尾\0）
constexpr int32 NePyInternalModuleNameSize = sizeof(NePyInternalModuleName);

// 是否全局禁用NePyGeneratedType。
// 如果使用了func_code reloader，由于其特殊的类替换机制，
// 需要在reload之前首先禁用NePyGeneratedClass，
// 在reload过后重新打开NePyGenerated功能，
// 并手动调用ue.GenerateClass/ue.GenerateStruct/ue.GenerateEnum接口触发UE类型更新。
extern bool GNePyDisableGeneratedType;

#if ENGINE_MAJOR_VERSION >= 5 && NEPY_GENERATED_TYPE_SUPPORT_WORLD_PARTATION
#define RF_NePyGeneratedTypeGCSafe RF_MarkAsRootSet
#define EInternalObjectFlags_NePyGeneratedTypeGCSafe EInternalObjectFlags::RootSet
#else
#define RF_NePyGeneratedTypeGCSafe (RF_MarkAsNative | RF_Transient)
#define EInternalObjectFlags_NePyGeneratedTypeGCSafe EInternalObjectFlags::Native
#endif

enum class ENePyGeneratedTypeContainerType : uint8
{
	Runtime,
	Editor
};
ENUM_RANGE_BY_FIRST_AND_LAST(ENePyGeneratedTypeContainerType, ENePyGeneratedTypeContainerType::Runtime, ENePyGeneratedTypeContainerType::Editor);

// 获取存放Python生成类的Package
UPackage* GetNePyGeneratedTypeContainer(ENePyGeneratedTypeContainerType ContainerType);

/** Type used to define self class for subclassing */
//	@ue.uclass()
//	class Class(ue.Object) :
//		# instance = ue.uproperty(Class) --> [Wrong: Can't pass python syntax test]
//		instance = ue.uproperty(ue.SelfClass) --> [Right: Pass python syntax test]
struct FNePySelfClassDef : public PyObject
{
	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FNePySelfClassDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePySelfClassDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FNePySelfClassDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FNePySelfClassDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePySelfClassDef* Check(PyObject* InPyObj);
};

/** Type used to define UPARAM(ref) parameter for subclassing */
//	Used to define a parameter that is passed by reference in a function
//	ue.ref(XXX) is a special type that indicates the parameter of type 'XXX' is passed by reference
//	Example:
//	@ue.ufunction(params=(ue.ArrayWrapper(int))) --> Normal non-reference parameter
//	Equivalent C++ declaration:
//	void Func(TArray<int>);
//	@ue.ufunction(params=(ue.ref(ue.ArrayWrapper(int)))) --> UPARAM(ref) reference parameter
//	Equivalent C++ declaration:
//	void Func(UPARAM(ref) TArray<int>&);
struct FNePyRefParamDef : public PyObject
{
	//** Type of this parameter */
	PyObject* TypeObject;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FNePyRefParamDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyRefParamDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FNePyRefParamDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FNePyRefParamDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyRefParamDef* Check(PyObject* InPyObj);
};

/** Type used to define constant values from Python */
struct FNePyUValueDef : public PyObject
{
	/** Value of this definition */
	int64 Value;

	/** Dictionary of meta-data associated with this value */
	PyObject* MetaData;

	// Python中定义的顺序
	uint32 DefineOrder;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FNePyUValueDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyUValueDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FNePyUValueDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FNePyUValueDef* InSelf);

#if WITH_EDITORONLY_DATA
	/** Apply the meta-data on this instance via the given predicate */
	static void ApplyMetaData(FNePyUValueDef* InSelf, const TFunctionRef<void(const FString&, const FString&)>& InPredicate);
#endif // WITH_EDITORONLY_DATA

	// 判断Python对象是否为此类的实例
	static FNePyUValueDef* Check(PyObject* InPyObj);

private:
	static std::atomic<uint32> GlobalDefineOrder;
};

/** Type used to define FProperty fields from Python */
struct FNePyFPropertyDef : public PyObject
{
	/** Type of this property */
	PyObject* PropType;

	/** Default value of this property */
	PyObject* DefaultValue;

	/** Getter function to use with this property */
	FString GetterFuncName;

	/** Setter function to use with this property */
	FString SetterFuncName;

	// 用户定义的 UPF
	EPropertyFlags UserDefineUPropertyFlags;

	// 说明符列表
	TArray<FNePySpecifier*> Specifiers;

	// Python中定义的顺序
	uint32 DefineOrder;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FNePyFPropertyDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyFPropertyDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FNePyFPropertyDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FNePyFPropertyDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyFPropertyDef* Check(PyObject* InPyObj);

private:
	static std::atomic<uint32> GlobalDefineOrder;
};

/** Type used to define FComponent fields from Python */
struct FNePyUComponentDef : public PyObject
{
	/** Type of this property */
	PyObject* PropType;

	/** Is is RootComponent? */
	bool bRoot;

	/** The component which to attach with */
	FString AttachName;

	/** The name of socket which to attach with */
	FString SocketName;

	/** The name of component which to override in the parent class */
	FString OverrideName;

	// 说明符列表
	TArray<FNePySpecifier*> Specifiers;

	// Python中定义的顺序
	uint32 DefineOrder;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FNePyUComponentDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyUComponentDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FNePyUComponentDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this FNePyFComponentDef (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FNePyUComponentDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyUComponentDef* Check(PyObject* InPyObj);

private:
	static std::atomic<uint32> GlobalDefineOrder;
};

/** Flags used to define the attributes of a UFunction field from Python */
enum class ENePyUFunctionDefFlags : uint8
{
	None = 0,
	Override = 1 << 0,
	Static = 1 << 1,
};
ENUM_CLASS_FLAGS(ENePyUFunctionDefFlags);

struct FNePyUParamDef : public PyObject
{
	/** Type of this param */
	PyObject* PropType;

	// 说明符列表
	TArray<FNePySpecifier*> Specifiers;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FNePyUParamDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyUParamDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FNePyUParamDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FNePyUParamDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyUParamDef* Check(PyObject* InPyObj);

};

/** Type used to define UFunction fields from Python */
struct FNePyUFunctionDef : public PyObject
{
	/** Python function to call */
	PyObject* Func;

	/** Return type of this function */
	PyObject* FuncRetType;

	/** List of types for each parameter of this function */
	PyObject* FuncParamTypes;

	/** Flags used to define this function */
	ENePyUFunctionDefFlags FuncFlags;
	
	// 说明符列表
	TArray<FNePySpecifier*> Specifiers;

	// Python中定义的顺序
	uint32 DefineOrder;

	/** New this instance (called via tp_new for Python, or directly in C++)*/
	static FNePyUFunctionDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyUFunctionDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++)*/
	static int Init(FNePyUFunctionDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Dealloc to restore the instance to its New state) */
	static void Deinit(FNePyUFunctionDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyUFunctionDef* Check(PyObject* InPyObj);

private:
	static std::atomic<uint32> GlobalDefineOrder;
};

struct FNePyUDelegateDef : public PyObject
{
	/** List of types for each parameter of this delegate */
	PyObject* FuncParamTypes;

	/** List of names for each parameter of this delegate */
	TArray<FString> FuncParamNames;

	/** Flags used to define this delegate realted to function */
	ENePyUFunctionDefFlags FuncFlags;
	
	// 说明符列表
	TArray<FNePySpecifier*> Specifiers;

	// Python中定义的顺序
	uint32 DefineOrder;

	/** New this instance (called via tp_new for Python, or directly in C++)*/
	static FNePyUDelegateDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Dealloc(FNePyUDelegateDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++)*/
	static int Init(FNePyUDelegateDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Dealloc to restore the instance to its New state) */
	static void Deinit(FNePyUDelegateDef* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyUDelegateDef* Check(PyObject* InPyObj);

private:
	static std::atomic<uint32> GlobalDefineOrder;
};

// 用于定义对象引用类型的枚举值
enum class ENePyObjectRefType : uint8
{
	ObjectReference = 0,
	ClassReference = 1,
	SoftObjectReference = 2,
	SoftClassReference = 3,
	WeakObjectReference = 4,
};

// 用于在python定义对象引用，需要配合ue.uproperty使用
struct FNePyObjectRefDef : public PyObject
{
	// 对象引用类型
	ENePyObjectRefType RefType;

	// 引用的对象
	UClass* Class;

	// 是否构造的Python对象是SelfClass特殊对象
	bool IsSelfClass;

	// tp_new
	static FNePyObjectRefDef* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

	// tp_dealloc
	static void Dealloc(FNePyObjectRefDef* InSelf);

	// tp_init
	static int Init(FNePyObjectRefDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	//
	static bool DoInit(FNePyObjectRefDef* InSelf, PyObject* InArg, ENePyObjectRefType InRefType);

	static PyObject* Repr(FNePyObjectRefDef* InSelf);

	static PyObject* Str(FNePyObjectRefDef* InSelf);
	
	// 判断Python对象是否为此类的实例
	static FNePyObjectRefDef* Check(PyObject* InPyObj);

	static PyObject* Call(FNePyObjectRefDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	static const char* GetRefTypeName(ENePyObjectRefType InRefType);

	static PyObject* Or(PyObject* InLHS, PyObject* InRHS);
};

struct FNePyObjectRefMaker : public PyObject
{
	// 对象引用类型
	ENePyObjectRefType RefType;

	static FNePyObjectRefDef* GetItem(FNePyObjectRefMaker* InSelf, PyObject* InKey);

	static FNePyObjectRefMaker* NewObjectRefMaker(ENePyObjectRefType InRefType);

	// 判断Python对象是否为此类的实例
	static FNePyObjectRefMaker* Check(PyObject* InPyObj);
};

// 用于定义别名类型的枚举值
enum class ENePyAliasType : uint8
{
	FName,
	FText,
};

/*
* 用于在python定义某些不会导出的 UE 类型的 uproperty 时使用，如 FText, FName
*/
struct FNePyTypeAlias : public PyObject
{
	// 对象引用类型
	ENePyAliasType AliasType;
	PyTypeObject* OriginalType;

	static const char* GetAliasTypeName(ENePyAliasType InAliasType);

	// tp_call
	static PyObject* Call(FNePyTypeAlias* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_getattro
	static PyObject* GetAttro(FNePyTypeAlias* InSelf, PyObject* InAttrName);

	static FNePyTypeAlias* New(ENePyAliasType InAliasType, PyTypeObject* InOriginalType);

	static PyObject* Or(PyObject* InLHS, PyObject* InRHS);

	// 判断Python对象是否为此类的实例
	static FNePyTypeAlias* Check(PyObject* InPyObj);
};

// 旧式Subclassing功能当前是否启用
PyObject* NePyMethod_IsOldStyleSubclassingEnabled(PyObject* InSelf);

// 全局禁用旧式Subclassing功能
PyObject* NePyMethod_DisableOldStyleSubclassing(PyObject* InSelf, PyObject* InArgs);

// PythonGeneratedType功能当前是否启用
PyObject* NePyMethod_IsGeneratedTypeEnabled(PyObject* InSelf);

// 全局禁用PythonGeneratedType功能
PyObject* NePyMethod_DisableGeneratedType(PyObject* InSelf, PyObject* InArgs);

// 重新生成一个类的所有UFunction，但不触发Reinstance
PyObject* NePyMethod_RegenerateFunctions(PyObject* InSelf, PyObject* InArg);

// 返回用于在Python脚本中定义UCLASS的装饰器
PyObject* NePyMethod_UClassDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义USTRUCT的装饰器
PyObject* NePyMethod_UStructDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义UENUM的装饰器
PyObject* NePyMethod_UEnumDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义枚举值的方法
PyObject* NePyMethod_UValueFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义UPROPERTY的方法
PyObject* NePyMethod_UPropertyFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义UPARAM的方法
PyObject* NePyMethod_UParamFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义UFUNCTION的装饰器
PyObject* NePyMethod_UFunctionDecorator(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义UDelegate的方法
PyObject* NePyMethod_UDelegateFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义ClassReference的方法
PyObject* NePyMethod_ClassReferenceFunction(PyObject* InSelf, PyObject* InArg);

// 返回用于在Python脚本中定义SoftObjectReference的方法
PyObject* NePyMethod_SoftObjectReferenceFunction(PyObject* InSelf, PyObject* InArg);

// 返回用于在Python脚本中定义SoftClassReference的方法
PyObject* NePyMethod_SoftClassReferenceFunction(PyObject* InSelf, PyObject* InArg);

// 返回用于在Python脚本中定义WeakObjectReference的方法
PyObject* NePyMethod_WeakObjectReferenceFunction(PyObject* InSelf, PyObject* InArg);

// 返回用于在Python脚本中定义UActorComponent的方法
PyObject* NePyMethod_UComponentFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 返回用于在Python脚本中定义UPARAM(ref)的方法
PyObject* NePyMethod_RefParamFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

// 初始化Python生成类所需的Python类型
void NePyInitGeneratedTypes(PyObject* PyRootModule, PyObject* PyInternalModule);

// 销毁所有Python生成类，切断它们与Python虚拟机的关联
void NePyPurgeGeneratedTypes();
