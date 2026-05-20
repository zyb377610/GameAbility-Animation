#pragma once
#include "NePyIncludePython.h"
#include "NePyResourceOwner.h"
#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "NePyGeneratedEnum.generated.h"

// 通过Python提供的信息，在运行时构造出来的UE枚举
UCLASS()
class NEPYTHONBINDING_API UNePyGeneratedEnum : public UEnum, public INePyResourceOwner
{
	GENERATED_BODY()

	// UObject interface
	virtual void BeginDestroy() override;

	//~ INePyResourceOwner interface
	virtual void ReleasePythonResources() override;

public:
	/** Generate an Unreal enum from the given Python type */
	static UNePyGeneratedEnum* GenerateEnum(PyTypeObject* InPyType, const TArray<TPair<PyObject*, PyObject*>>& InPySpecifierPairs);

public:
	// 与此UEnum一一对应的Python类
	// 持有它是为了确保当没有Python实例对象时，PyType不会被gc回收
	PyTypeObject* PyType = nullptr;
};

PyTypeObject* NePyInitGeneratedEnum();
