#pragma once
#include "NePyIncludePython.h"
#include "UObject/UnrealType.h"
#include "UObject/Field.h"

struct FNePySubclass;

// 创建FMulticastInlineDelegateProperty
// 如果无法创建则返回nullptr
FMulticastInlineDelegateProperty* NePySubclassingNewDelegate(FFieldVariant Owner, const FName& AttrName, UFunction* UFunc);