#include "NePyDelegateHelper.h"
#include "NePyGeneratedType.h"
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/UnrealTypePrivate.h"

// 创建FMulticastInlineDelegateProperty
// 如果无法创建则返回nullptr
FMulticastInlineDelegateProperty* NePySubclassingNewDelegate(FFieldVariant Owner, const FName& FuncName, UFunction* UFunc)
{
	EObjectFlags ObjectFlags = RF_Public | RF_MarkAsNative;
	FMulticastInlineDelegateProperty* NewPropertyDelegate = new FMulticastInlineDelegateProperty(Owner, FuncName, ObjectFlags);
	NewPropertyDelegate->SignatureFunction = UFunc;
	return NewPropertyDelegate;
}
