#pragma once
#include "NePyIncludePython.h"
#include "UObject/Field.h"

struct FNePySubclass;

// 处理Subclassing相关逻辑
// 即，可以在Python脚本中新建类直接继承ue类
// tp_new
PyObject* NePySubclassing(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);

// 根据Python对象，返回新创建的FProperty
// 如果无法创建则返回nullptr
FProperty* NePySubclassingNewProperty(FFieldVariant Owner, const FName& AttrName, PyObject* PyAttrVal, const FString& OwnerName, EPropertyFlags PropFlags, PyObject* DefaultValue = nullptr, UClass* ThisClass = nullptr);

FProperty* NePyNewEnumUnderlyingProperty(FEnumProperty* EnumProp, EObjectFlags ObjectFlags);
