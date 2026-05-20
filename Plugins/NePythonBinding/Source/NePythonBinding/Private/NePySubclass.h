#pragma once
// 处理Subclassing相关逻辑
// 即，可以在Python脚本中新建类直接继承ue类
#include "CoreMinimal.h"
#include "NePyClass.h"
#include "NePyObjectBase.h"
#include "UObject/Package.h"
#include "NePySubclass.generated.h"

struct FNePySubclass;
struct FNePyObjectBase;

// 通过Python提供的信息，在运行时构造出来的UE4类
UCLASS()
class UNePySubclass : public UClass
{
	GENERATED_BODY()

public:
	// 与此UClass一一对应的Python包装类
	// 由HouseKeeper保证PyClass的引用计数正确
	// 此处即不需要增加引用，也不需要在析构时减少引用
	FNePySubclass* PyClass = nullptr;

public:
	// 对象构造函数（ClassConstructor）
	static void PySubclassConstructor(const FObjectInitializer& ObjectInitializer);

	// 从C++调用Python的静态转发方法
	DECLARE_FUNCTION(CallPythonFunction);
};

// UNePySubclass的Python封装类
struct FNePySubclass : public FNePyClass
{
	// class的__dict__，用来放类成员函数
	PyObject* PyDict;

	inline UClass* GetValue()
	{
		return (UNePySubclass*)Value;
	}
};

// 注册PySubclass
void NePyInitSubclass(PyObject* PyOuterModule);

// 获取NePySubclass的Python类型
PyTypeObject* NePySubclassGetType();

// 为每个PySubclass生成独一无二的PyType，从而能正确继承基类
PyTypeObject* NePyNewSubclassInstanceType(UNePySubclass* Class, UClass* SuperClass, PyObject* InDict);

// 生成新的Subclass
// 仅供NePySubclassing()调用
FNePySubclass* NePySubclassNewInternalUseOnly(UNePySubclass* Class, UClass* SuperClass);

// UNePySubclass的Python封装类，现在纯Python生成的PyType，用不了也不用这个了
struct FNePySubclassInstance : public FNePyObjectBase
{
	// instance的__dict__，用来放实例成员
	PyObject* PyDict;
};

// 生成新的Subobject
// 每个Subobject有独一无二的tp_type
FNePyObjectBase* NePySubclassNew(UObject* InObject, PyTypeObject* InPyType);
