#pragma once
#include "NePyIncludePython.h"
#include "NePyGenUtil.h"
#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

// 描述器基类，用于为UE对象的属性或方法生成动态绑定
struct NEPYTHONBINDING_API FNePyDescriptorBase : public PyObject
{
	// 持有描述器的PyType
	PyTypeObject* Type;

	// 描述器在Type中的名称
	PyObject* Name;
	// 虽然可以通过UProperty，UFunction GetName()获取，但这里还是存多一份
	// 一方面，与cpython实现保持一致；另一方面，可能存在Python接口与UE实际的属性、函数名称不相同的情况

	static void InitPyTypeCommon(PyTypeObject* InPyType);

	// 生成Python对象
	static FNePyDescriptorBase* New(PyTypeObject* InDescrType, PyTypeObject* InType, const char* InName);

	// tp_dealloc
	static void Dealloc(FNePyDescriptorBase* InSelf);

	// tp_init
	static int Init(FNePyDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_repr
	static PyObject* Repr(FNePyDescriptorBase* InSelf);

	// tp_traverse
	static int Traverse(FNePyDescriptorBase* InSelf, visitproc InVisit, void* InArg);

	// tp_clear
	static int Clear(FNePyDescriptorBase* InSelf);

public:

	static const char* DescrName(FNePyDescriptorBase* InSelf);

	static PyTypeObject* DescrType(FNePyDescriptorBase* InSelf);

	static int DescrCheck(FNePyDescriptorBase* InSelf, PyObject* InObject);

protected:

	static PyObject* StringFormat(FNePyDescriptorBase* InSelf, const char* InFormat);
};

// 用于为UE属性（UPROPERTY）生成动态绑定的描述器
struct NEPYTHONBINDING_API FNePyPropertyDescriptor : public FNePyDescriptorBase
{
	const FProperty* Prop;

	getter Getter;
	setter Setter;

	NePyGenUtil::FMethodDef* GetFunc;
	NePyGenUtil::FMethodDef* SetFunc;

	// 初始化PyType类型
	static void InitPyType();

	// 生成Python对象
	static FNePyPropertyDescriptor* New(PyTypeObject* InType, const char* InName, const FProperty* InBindProperty, getter InGetter, setter InSetter);

	// tp_dealloc
	static void Dealloc(FNePyPropertyDescriptor* InSelf);

	// 清空类成员
	static void Reset(FNePyPropertyDescriptor* InSelf);

	// tp_repr
	static PyObject* Repr(FNePyPropertyDescriptor* InSelf);

	// tp_descr_get
	static PyObject* DescrGet(FNePyPropertyDescriptor* InSelf, PyObject* InObject, PyTypeObject* InType);

	// tp_descr_set
	static int DescrSet(FNePyPropertyDescriptor* InSelf, PyObject* InObject, PyObject* InValue);

	// 判断Python对象是否为此类的实例
	static FNePyPropertyDescriptor* Check(PyObject* InPyObj);
};

// 用于为UE方法（UFUNCTION）生成动态绑定的描述器基类
struct NEPYTHONBINDING_API FNePyFunctionDescriptorBase : public FNePyDescriptorBase
{
	NePyGenUtil::FMethodDef* MethodDef;

	// 初始化PyType类型
	static void InitPyType();

	// tp_dealloc
	static void Dealloc(FNePyFunctionDescriptorBase* InSelf);

	// 清空类成员
	static void Reset(FNePyFunctionDescriptorBase* InSelf);

	// tp_repr
	static PyObject* Repr(FNePyFunctionDescriptorBase* InSelf);

	// 判断Python对象是否为此类的实例
	static FNePyFunctionDescriptorBase* Check(PyObject* InPyObj);
};

// 用于为UE成员方法生成动态绑定的描述器
struct NEPYTHONBINDING_API FNePyFunctionDescriptor : public FNePyFunctionDescriptorBase
{
	// 初始化PyType类型
	static void InitPyType();

	// 生成Python对象
	static FNePyFunctionDescriptor* New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction);

	// tp_call
	static PyObject* Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_descr_get
	static PyObject* DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType);
};

// 用于为UE静态方法生成动态绑定的描述器
struct NEPYTHONBINDING_API FNePyStaticFunctionDescriptor : public FNePyFunctionDescriptorBase
{
	// 初始化PyType类型
	static void InitPyType();

	// 生成Python对象
	static FNePyStaticFunctionDescriptor* New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction);

	// tp_call
	static PyObject* Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_descr_get
	static PyObject* DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType);
};

// 用于为Pyhon生成类成员方法生成动态绑定的描述器
struct NEPYTHONBINDING_API FNePyGeneratedFunctionDescriptor : public FNePyFunctionDescriptorBase
{
	// 初始化PyType类型
	static void InitPyType();

	// 生成Python对象
	static FNePyGeneratedFunctionDescriptor* New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction);

	// tp_call
	static PyObject* Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_descr_get
	static PyObject* DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType);
};

// 用于为Python生成类静态方法生成动态绑定的描述器
struct NEPYTHONBINDING_API FNePyGeneratedStaticFunctionDescriptor : public FNePyFunctionDescriptorBase
{
	// 初始化PyType类型
	static void InitPyType();

	// 生成Python对象
	static FNePyGeneratedStaticFunctionDescriptor* New(PyTypeObject* InType, const char* InName, UFunction* InBindFunction);

	// tp_call
	static PyObject* Call(FNePyFunctionDescriptorBase* InSelf, PyObject* InArgs, PyObject* InKwds);

	// tp_descr_get
	static PyObject* DescrGet(FNePyFunctionDescriptorBase* InSelf, PyObject* InObject, PyTypeObject* InType);
};


// 为FProperty创建Descriptor，并添加到PyType.tp_dict中
FNePyDescriptorBase* NePyType_AddNewProperty(PyTypeObject* InPyType, const FProperty* InBindProperty, const char* InAttrName, bool bAddToDict = true);

// 为UFunction创建Descriptor，并添加到PyType.tp_dict中
FNePyDescriptorBase* NePyType_AddNewFunction(PyTypeObject* InPyType, UFunction* InBindFunction, const char* InAttrName, bool bAddToDict = true);

// 清理PyType.tp_dict中的所有Descriptor
void NePyType_CleanupDescriptors(PyTypeObject* InPyType, bool bCleanupProps = true, bool bCleanupFuncs = true);

#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 3 || ENGINE_MAJOR_VERSION >= 6
// 为类中的所有属性添加FieldNotify支持
void NePyType_AddFieldNotifySupportForType(PyTypeObject* PyType, const UClass* InClass);

// 为单个类成员属性添加FieldNotify支持
void NePyType_AddFieldNotifySupportForDescr(PyObject* PyDescr, const UClass* InClass);
#endif