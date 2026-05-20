#pragma once
#include "NePyIncludePython.h"
#include <type_traits>
#include "CoreMinimal.h"
#include "Templates/IsPointer.h"
#include "UObject/UnrealType.h"
#include "UObject/FieldPath.h"
#include "NePyPtr.h"
#include "Runtime/Launch/Resources/Version.h"

struct FNePyObjectBase;

#ifdef __clang__
// 有些函数被标记为UE_DEPRECATED，例如K2_AttachRootComponentToActor
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

/** Cast a function pointer to PyCFunction (via a void* to avoid a compiler warning) */
#define NePyCFunctionCast(FUNCPTR) (PyCFunction)(void*)(FUNCPTR)

NEPYTHONBINDING_API DECLARE_LOG_CATEGORY_EXTERN(LogNePython, Log, All);

// 提供各种静态方法
namespace NePyBase
{
	// 在拷贝FStructProperty前调用，确保新创建的结构体已正确构造
	template<typename TStruct>
	inline void EnsureCopyToStructSafely(const FStructProperty* InStructProp, void* StructPtr)
	{
#if (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1 || ENGINE_MAJOR_VERSION >= 6)
		// 'PLATFORM_COMPILER_HAS_IF_CONSTEXPR': name was marked as #pragma deprecated
		if constexpr (!TStructOpsTypeTraits<TStruct>::WithZeroConstructor && !TIsPODType<TStruct>::Value)
#else
#if PLATFORM_COMPILER_HAS_IF_CONSTEXPR
		//WithZeroConstructor 或 PODType都是安全的，可以不构造。
		if constexpr (!TStructOpsTypeTraits<TStruct>::WithZeroConstructor && !TIsPODType<TStruct>::Value)
#else
		if (!TStructOpsTypeTraits<TStruct>::WithZeroConstructor && !TIsPODType<TStruct>::Value)
#endif //PLATFORM_COMPILER_HAS_IF_CONSTEXPR
#endif // UE_VERSION
		{
			check(StructPtr);
			InStructProp->Struct->InitializeStruct(StructPtr);
		}
	}

	/** bool overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, bool& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const bool Val, PyObject*& OutPyObj);

	/** int8 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, int8& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const int8 Val, PyObject*& OutPyObj);

	/** uint8 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, uint8& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const uint8 Val, PyObject*& OutPyObj);

	/** int16 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, int16& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const int16 Val, PyObject*& OutPyObj);

	/** uint16 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, uint16& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const uint16 Val, PyObject*& OutPyObj);

	/** int32 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, int32& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const int32 Val, PyObject*& OutPyObj);

	/** uint32 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, uint32& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const uint32 Val, PyObject*& OutPyObj);

	/** int64 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, int64& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const int64 Val, PyObject*& OutPyObj);

	/** uint64 overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, uint64& OutVal, bool bStrict = true);
	NEPYTHONBINDING_API bool ToPy(const uint64 Val, PyObject*& OutPyObj);

	/** float overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, float& OutVal);
	NEPYTHONBINDING_API bool ToPy(const float Val, PyObject*& OutPyObj);

	/** double overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, double& OutVal);
	NEPYTHONBINDING_API bool ToPy(const double Val, PyObject*& OutPyObj);

	/** char overload*/
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, const char*& OutVal);
	NEPYTHONBINDING_API bool ToPy(const char* Val, PyObject*& OutPyObj);

	/** TCHAR overload*/
	NEPYTHONBINDING_API bool ToPy(const TCHAR* Val, PyObject*& OutPyObj);

	/** FString overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, FString& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FString& Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API bool ToPy(const FString* Val, PyObject*& OutPyObj);

	/** FName overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, FName& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FName& Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API bool ToPy(const FName* Val, PyObject*& OutPyObj);

	/** FText overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, FText& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FText& Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API bool ToPy(const FText* Val, PyObject*& OutPyObj);

	/** FFieldPath overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, FFieldPath& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FFieldPath& Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API bool ToPy(const FFieldPath* Val, PyObject*& OutPyObj);

	/** FSoftObjectPtr overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, FSoftObjectPtr& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FSoftObjectPtr& Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API bool ToPy(const FSoftObjectPtr* Val, PyObject*& OutPyObj);

	/** FWeakObjectPtr overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, FWeakObjectPtr& OutVal);
	NEPYTHONBINDING_API bool ToPy(const FWeakObjectPtr& Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API bool ToPy(const FWeakObjectPtr* Val, PyObject*& OutPyObj);

	/** TArray<uint8> overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, TArray<uint8>& OutVal);
	NEPYTHONBINDING_API bool ToPy(const TArray<uint8>& Val, PyObject*& OutPyObj);


#if WITH_EDITORONLY_DATA
	/** FProperty overload */
	NEPYTHONBINDING_API PyObject* ToPy(FProperty* Val);
#endif

	/** UObject overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, UObject*& OutVal, const UClass* ExpectedType = nullptr);
	NEPYTHONBINDING_API UObject* ToCppObject(PyObject* PyObj, const UClass* ExpectedType = nullptr);
	template <typename T>
	T* ToCppObject(PyObject* PyObj);
	NEPYTHONBINDING_API bool ToPy(const UObject* Val, PyObject*& OutPyObj);
	NEPYTHONBINDING_API PyObject* ToPy(const UObject* Val);

	/** UField overrload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, UField*& OutVal, const UClass* ExpectedType);
	NEPYTHONBINDING_API UField* ToCppField(PyObject* PyObj, const UClass* ExpectedType);

	/** UStruct overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, UStruct*& OutVal, const UClass* ExpectedType = nullptr);
	NEPYTHONBINDING_API UStruct* ToCppStruct(PyObject* PyObj, const UClass* ExpectedType = nullptr);

	/** UClass overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, UClass*& OutVal, const UClass* ExpectedType = nullptr);
	NEPYTHONBINDING_API UClass* ToCppClass(PyObject* PyObj, const UClass* ExpectedType = nullptr);

	/** UScriptStruct overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, UScriptStruct*& OutVal, const UClass* ExpectedType = nullptr);
	NEPYTHONBINDING_API UScriptStruct* ToCppScriptStruct(PyObject* PyObj, const UClass* ExpectedType = nullptr);

	/** UEnum overload */
	NEPYTHONBINDING_API bool ToCpp(PyObject* PyObj, UEnum*& OutVal);
	NEPYTHONBINDING_API UEnum* ToCppEnum(PyObject* PyObj);

	/** TWeakObjectPtr overload */
	template<typename ClassType>
	inline bool ToPy(const TWeakObjectPtr<ClassType>& InWeakObject, PyObject*& OutPyObj)
	{
		OutPyObj = ToPy(InWeakObject.Get());
		return true;
	}
	template<typename ClassType>
	inline PyObject* ToPy(const TWeakObjectPtr<ClassType>& InWeakObject)
	{
		PyObject* RetPyObj;
		ToPy(InWeakObject, RetPyObj);
		return RetPyObj;
	}

	/** TArray overload **/
	template<typename ElementType, typename AllocatorType>
	inline PyObject* ToPy(const TArray<ElementType, AllocatorType>& InArray)
	{
		int32 RetValNum = (int32)InArray.Num();
		PyObject* PyList = PyList_New(RetValNum);
		for (int32 Index = 0; Index < RetValNum; ++Index)
		{
			PyObject* PyItem;
			if (NePyBase::ToPy(InArray[Index], PyItem))
			{
				PyList_SetItem(PyList, Index, PyItem);
			}
		}
		return PyList;
	}

	/** TMap overload **/
	template<typename KeyType, typename ValueType, typename SetAllocator, typename KeyFuncs>
	inline PyObject* ToPy(const TMap<KeyType, ValueType, SetAllocator, KeyFuncs>& InMap)
	{
		PyObject* PyDict = PyDict_New();
		for (auto& Pair : InMap)
		{
			PyObject* PyKey = nullptr;
			NePyBase::ToPy(Pair.Key, PyKey);
			PyObject* PyValue = nullptr;
			NePyBase::ToPy(Pair.Value, PyValue);

			if (PyKey && PyValue)
			{
				PyDict_SetItem(PyDict, PyKey, PyValue);
			}

			Py_XDECREF(PyKey);
			Py_XDECREF(PyValue);
		}
		return PyDict;
	}

	/** TSet overload **/
	template<typename InElementType, typename KeyFuncs, typename Allocator>
	inline PyObject* ToPy(const TSet<InElementType, KeyFuncs, Allocator>& InSet)
	{
		PyObject* PySet = PySet_New(nullptr);
		for (auto& Elem : InSet)
		{
			PyObject* PyElem;
			if (NePyBase::ToPy(Elem, PyElem))
			{
				PySet_Add(PySet, PyElem);
				Py_DECREF(PyElem);
			}
		}
		return PySet;
	}

	// 将任意Python对象转化为UE字符串
	NEPYTHONBINDING_API FString PyObjectToString(PyObject* InPyObj);

	// 尝试获取UWorld
	NEPYTHONBINDING_API UWorld* TryGetWorld(PyObject* InPyObj);

	// 判断Python对象持有的UE对象是否合法
	// 若UE对象已被释放，则返回false，并设置Python脚本错
	NEPYTHONBINDING_API bool CheckValidAndSetPyErr(FNePyObjectBase* InPyObj);
	NEPYTHONBINDING_API bool CheckValidAndSetPyErr(FNePyObjectBase* InPyObj, const char* InMemberName);
	NEPYTHONBINDING_API bool CheckValidAndSetPyErr(FNePyObjectBase* InPyObj, const FProperty* InProperty);

	// 设置二元运算符重载错误
	NEPYTHONBINDING_API void SetBinopTypeError(PyObject* InLeft, PyObject* InRight, const char* InOpName);

	/** Validate that the given index is valid for the container length */
	NEPYTHONBINDING_API int ValidateContainerIndexParam(const Py_ssize_t InIndex, const Py_ssize_t InLen, const FProperty* InProp);

	/** Resolve a container index (taking into account negative indices) */
	NEPYTHONBINDING_API Py_ssize_t ResolveContainerIndexParam(const Py_ssize_t InIndex, const Py_ssize_t InLen);

	/**
	 * Get the friendly value of the given property that can be used when stringifying property values for Python.
	 */
	NEPYTHONBINDING_API FString GetFriendlyPropertyValue(const FProperty* InProp, const void* InPropValue, const uint32 InPropPortFlags);

	// 根据成员对象指针偏移值，寻找对应的Property
	NEPYTHONBINDING_API FProperty* FindPropertyByMemberPtr(const UStruct* InStruct, const void* InInstancePtr, const void* InMemberPtr);

	// convert a property to a python object
	NEPYTHONBINDING_API PyObject* TryConvertFPropertyToPyObjectDirect(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
	// convert a property to a python object
	NEPYTHONBINDING_API PyObject* TryConvertFPropertyToPyObjectInContainer(const FProperty* InProp, const void* InBuffer, int32 InArrayIndex, UObject* InOwnerObject);

	// === 返回的PyObject可能会通过拷贝一份数据从而不依赖原始内存 ===
	// convert a property to a python object
	NEPYTHONBINDING_API PyObject* TryConvertFPropertyToPyObjectDirectNoDependency(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
	// convert a property to a python object
	NEPYTHONBINDING_API PyObject* TryConvertFPropertyToPyObjectInContainerNoDependency(const FProperty* InProp, const void* InBuffer, int32 InArrayIndex, UObject* InOwnerObject);
	
	// === 返回的PyObject会通过引用PyOuter保证Array/Map/Set/Struct等容器的数据内存安全 ===
	// convert a property to a python object
	NEPYTHONBINDING_API PyObject* TryConvertFPropertyToPyObjectDirectPyOuter(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
	// convert a property to a python object
	NEPYTHONBINDING_API PyObject* TryConvertFPropertyToPyObjectInContainerPyOuter(const FProperty* InProp, const void* InBuffer, int32 InArrayIndex, PyObject* InPyOuter);

	// convert a python object to a property
	NEPYTHONBINDING_API bool TryConvertPyObjectToFPropertyDirect(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
	// convert a python object to a property
	NEPYTHONBINDING_API bool TryConvertPyObjectToFPropertyInContainer(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, int32 InArrayIndex, UObject* InOwnerObject);

	// set python error when convert property to python object failed
	NEPYTHONBINDING_API void SetConvertFPropertyToPyObjectError(const FProperty* InProp);
	// set python error when convert python object to property failed
	NEPYTHONBINDING_API void SetConvertPyObjectToFPropertyError(PyObject* InPyObj, const FProperty* InProp);
};

template <typename T>
T* NePyBase::ToCppObject(PyObject* PyObj)
{
	return (T*)NePyBase::ToCppObject(PyObj, T::StaticClass());
}

// 一个辅助用的结构体，用来创建和储存对应Property的临时变量
struct alignas(16) FNePyPropValue
{
	FNePyPropValue(const FProperty* InProp);
	~FNePyPropValue();

	// 将Python对象转化为C++对象
	bool SetValue(PyObject* InPyObj);

	// 属性定义
	const FProperty* Prop;
	// 属性对应的C++对象
	void* Value;
	// 内联内存，避免频繁内存分配
	uint8 InlineBuffer[sizeof(double) * 4];
};
