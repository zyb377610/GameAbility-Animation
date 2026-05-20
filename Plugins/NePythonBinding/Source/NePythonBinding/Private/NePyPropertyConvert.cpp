#include "NePyPropertyConvert.h"
#include "NePyBase.h"
#include "NePyStructBase.h"
#include "NePyArrayWrapper.h"
#include "NePyMapWrapper.h"
#include "NePySetWrapper.h"
#include "NePyFixedArrayWrapper.h"
#include "NePySoftPtr.h"
#include "NePyDynamicDelegateWrapper.h"
#include "NePyDynamicMulticastDelegateWrapper.h"
#include "NePyObjectRef.h"
#include "NePyWrapperTypeRegistry.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"
#include "UObject/FieldPathProperty.h"


// ue -> py
PyObject* NePyBoolPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyInt8PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyInt16PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyIntPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyInt64PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyBytePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyUInt16PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyUInt32PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyUInt64PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyFloatPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyDoublePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyStrPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyNamePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyTextPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyFieldPathPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyEnumPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyArrayPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePyMapPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePySetPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePyFixedArrayPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePyClassPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePySoftClassPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyObjectPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyInterfacePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);
PyObject* NePyStructPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePyDelegatePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePyMulticastDelegatePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePySoftObjectPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);
PyObject* NePyWeakObjectPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject);


// py -> ue
bool NePyPyObjectToBoolProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToInt8Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToInt16Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToIntProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToInt64Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToByteProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToUInt16Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToUInt32Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToUInt64Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToFloatProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToDoubleProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToStrProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToNameProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToTextProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToFieldPathProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToEnumProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToDelegateProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
bool NePyPyObjectToArrayProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
bool NePyPyObjectToMapProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
bool NePyPyObjectToSetProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
bool NePyPyObjectToFixedArrayProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject);
bool NePyPyObjectToClassProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToSoftClassProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToObjectProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToInterfaceProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToStructProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToSoftObjectProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);
bool NePyPyObjectToWeakObjectProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* /*unused*/);


// ue -> py
PyObject* NePyBoolPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyInt8PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyInt16PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyIntPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyInt64PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyBytePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyUInt16PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyUInt32PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyUInt64PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyFloatPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyDoublePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyStrPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyNamePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyTextPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyFieldPathPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyEnumPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyArrayPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
PyObject* NePyMapPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
PyObject* NePySetPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
PyObject* NePyFixedArrayPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
PyObject* NePyClassPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePySoftClassPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyObjectPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyInterfacePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyStructPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter);
PyObject* NePySoftObjectPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);
PyObject* NePyWeakObjectPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* /*unused*/);


// py -> ue
bool NePyPyObjectToBoolPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToInt8PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToInt16PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToIntPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToInt64PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToBytePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToUInt16PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToUInt32PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToUInt64PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToFloatPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToDoublePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToStrPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToNamePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToTextPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToFieldPathPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToEnumPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToArrayPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToMapPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToSetPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToFixedArrayPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToClassPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToSoftClassPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToObjectPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToInterfacePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToStructPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToSoftObjectPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);
bool NePyPyObjectToWeakObjectPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject* /*unused*/);


// ue -> py
PyObject* NePyStructPropertyToPyObjectClone(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/);


NePyPropertyToPyObjectFunc NePyGetPropertyToPyObjectConverter(const FProperty* InProp)
{
	if (InProp->ArrayDim > 1)
	{
		return NePyFixedArrayPropertyToPyObject;
	}

	static TMap<FFieldClass*, NePyPropertyToPyObjectFunc> ConverterMap = {
		{ FBoolProperty::StaticClass(), NePyBoolPropertyToPyObject },
		{ FInt8Property::StaticClass(), NePyInt8PropertyToPyObject },
		{ FInt16Property::StaticClass(), NePyInt16PropertyToPyObject },
		{ FIntProperty::StaticClass(), NePyIntPropertyToPyObject },
		{ FInt64Property::StaticClass(), NePyInt64PropertyToPyObject },
		{ FByteProperty::StaticClass(), NePyBytePropertyToPyObject },
		{ FUInt16Property::StaticClass(), NePyUInt16PropertyToPyObject },
		{ FUInt32Property::StaticClass(), NePyUInt32PropertyToPyObject },
		{ FUInt64Property::StaticClass(), NePyUInt64PropertyToPyObject },
		{ FFloatProperty::StaticClass(), NePyFloatPropertyToPyObject },
		{ FDoubleProperty::StaticClass(), NePyDoublePropertyToPyObject },
		{ FStrProperty::StaticClass(), NePyStrPropertyToPyObject },
		{ FNameProperty::StaticClass(), NePyNamePropertyToPyObject },
		{ FTextProperty::StaticClass(), NePyTextPropertyToPyObject },
		{ FFieldPathProperty::StaticClass(), NePyFieldPathPropertyToPyObject },
		{ FEnumProperty::StaticClass(), NePyEnumPropertyToPyObject },
		{ FArrayProperty::StaticClass(), NePyArrayPropertyToPyObject },
		{ FMapProperty::StaticClass(), NePyMapPropertyToPyObject },
		{ FSetProperty::StaticClass(), NePySetPropertyToPyObject },
	};

	auto ConverterRef = ConverterMap.Find(InProp->GetClass());
	if (ConverterRef)
	{
		return *ConverterRef;
	}

	if (CastField<FClassProperty>(InProp))
	{
		return NePyClassPropertyToPyObject;
	}

	//if (CastField<FSoftClassProperty>(InProp))
	//{
	//	return NePySoftClassPropertyToPyObject;
	//}

	if (CastField<FSoftObjectProperty>(InProp))
	{
		return NePySoftObjectPropertyToPyObject;
	}

	if (CastField<FWeakObjectProperty>(InProp))
	{
		return NePyWeakObjectPropertyToPyObject;
	}

	if (CastField<FObjectPropertyBase>(InProp))
	{
		return NePyObjectPropertyToPyObject;
	}

	if (CastField<FInterfaceProperty>(InProp))
	{
		return NePyInterfacePropertyToPyObject;
	}

	if (CastField<FStructProperty>(InProp))
	{
		return NePyStructPropertyToPyObject;
	}

	if (CastField<FDelegateProperty>(InProp))
	{
		return NePyDelegatePropertyToPyObject;
	}

	if (CastField<FMulticastDelegateProperty>(InProp))
	{
		return NePyMulticastDelegatePropertyToPyObject;
	}

	return nullptr;
}

NePyPyObjectToPropertyFunc NePyGetPyObjectToPropertyConverter(const FProperty* InProp)
{
	if (InProp->ArrayDim > 1)
	{
		return NePyPyObjectToFixedArrayProperty;
	}

	static TMap<FFieldClass*, NePyPyObjectToPropertyFunc> ConverterMap = {
		{ FBoolProperty::StaticClass(), NePyPyObjectToBoolProperty },
		{ FInt8Property::StaticClass(), NePyPyObjectToInt8Property },
		{ FInt16Property::StaticClass(), NePyPyObjectToInt16Property },
		{ FIntProperty::StaticClass(), NePyPyObjectToIntProperty },
		{ FInt64Property::StaticClass(), NePyPyObjectToInt64Property },
		{ FByteProperty::StaticClass(), NePyPyObjectToByteProperty },
		{ FUInt16Property::StaticClass(), NePyPyObjectToUInt16Property },
		{ FUInt32Property::StaticClass(), NePyPyObjectToUInt32Property },
		{ FUInt64Property::StaticClass(), NePyPyObjectToUInt64Property },
		{ FFloatProperty::StaticClass(), NePyPyObjectToFloatProperty },
		{ FDoubleProperty::StaticClass(), NePyPyObjectToDoubleProperty },
		{ FStrProperty::StaticClass(), NePyPyObjectToStrProperty },
		{ FNameProperty::StaticClass(), NePyPyObjectToNameProperty },
		{ FTextProperty::StaticClass(), NePyPyObjectToTextProperty },
		{ FFieldPathProperty::StaticClass(), NePyPyObjectToFieldPathProperty },
		{ FEnumProperty::StaticClass(), NePyPyObjectToEnumProperty },
		{ FDelegateProperty::StaticClass(), NePyPyObjectToDelegateProperty },
		{ FArrayProperty::StaticClass(), NePyPyObjectToArrayProperty },
		{ FMapProperty::StaticClass(), NePyPyObjectToMapProperty },
		{ FSetProperty::StaticClass(), NePyPyObjectToSetProperty },
	};

	auto ConverterRef = ConverterMap.Find(InProp->GetClass());
	if (ConverterRef)
	{
		return *ConverterRef;
	}

	if (CastField<FClassProperty>(InProp))
	{
		return NePyPyObjectToClassProperty;
	}

	if (CastField<FSoftClassProperty>(InProp))
	{
		return NePyPyObjectToSoftClassProperty;
	}

	if (CastField<FSoftObjectProperty>(InProp))
	{
		return NePyPyObjectToSoftObjectProperty;
	}

	if (CastField<FWeakObjectProperty>(InProp))
	{
		return NePyPyObjectToWeakObjectProperty;
	}

	if (CastField<FObjectPropertyBase>(InProp))
	{
		return NePyPyObjectToObjectProperty;
	}

	if (CastField<FInterfaceProperty>(InProp))
	{
		return NePyPyObjectToInterfaceProperty;
	}

	if (CastField<FStructProperty>(InProp))
	{
		return NePyPyObjectToStructProperty;
	}

	return nullptr;
}

NePyPropertyToPyObjectFuncForStruct NePyGetPropertyToPyObjectConverterForStruct(const FProperty* InProp)
{
	if (InProp->ArrayDim > 1)
	{
		return NePyFixedArrayPropertyToPyObjectForStruct;
	}

	static TMap<FFieldClass*, NePyPropertyToPyObjectFuncForStruct> ConverterMap = {
		{ FBoolProperty::StaticClass(), NePyBoolPropertyToPyObjectForStruct },
		{ FInt8Property::StaticClass(), NePyInt8PropertyToPyObjectForStruct },
		{ FInt16Property::StaticClass(), NePyInt16PropertyToPyObjectForStruct },
		{ FIntProperty::StaticClass(), NePyIntPropertyToPyObjectForStruct },
		{ FInt64Property::StaticClass(), NePyInt64PropertyToPyObjectForStruct },
		{ FByteProperty::StaticClass(), NePyBytePropertyToPyObjectForStruct },
		{ FUInt16Property::StaticClass(), NePyUInt16PropertyToPyObjectForStruct },
		{ FUInt32Property::StaticClass(), NePyUInt32PropertyToPyObjectForStruct },
		{ FUInt64Property::StaticClass(), NePyUInt64PropertyToPyObjectForStruct },
		{ FFloatProperty::StaticClass(), NePyFloatPropertyToPyObjectForStruct },
		{ FDoubleProperty::StaticClass(), NePyDoublePropertyToPyObjectForStruct },
		{ FStrProperty::StaticClass(), NePyStrPropertyToPyObjectForStruct },
		{ FNameProperty::StaticClass(), NePyNamePropertyToPyObjectForStruct },
		{ FTextProperty::StaticClass(), NePyTextPropertyToPyObjectForStruct },
		{ FFieldPathProperty::StaticClass(), NePyFieldPathPropertyToPyObjectForStruct },
		{ FEnumProperty::StaticClass(), NePyEnumPropertyToPyObjectForStruct },
		{ FArrayProperty::StaticClass(), NePyArrayPropertyToPyObjectForStruct },
		{ FMapProperty::StaticClass(), NePyMapPropertyToPyObjectForStruct },
		{ FSetProperty::StaticClass(), NePySetPropertyToPyObjectForStruct },
	};

	auto ConverterRef = ConverterMap.Find(InProp->GetClass());
	if (ConverterRef)
	{
		return *ConverterRef;
	}

	if (CastField<FClassProperty>(InProp))
	{
		return NePyClassPropertyToPyObjectForStruct;
	}

	//if (CastField<FSoftClassProperty>(InProp))
	//{
	//	return NePySoftClassPropertyToPyObjectForStruct;
	//}

	if (CastField<FSoftObjectProperty>(InProp))
	{
		return NePySoftObjectPropertyToPyObjectForStruct;
	}

	if (CastField<FWeakObjectProperty>(InProp))
	{
		return NePyWeakObjectPropertyToPyObjectForStruct;
	}

	if (CastField<FObjectPropertyBase>(InProp))
	{
		return NePyObjectPropertyToPyObjectForStruct;
	}

	if (CastField<FInterfaceProperty>(InProp))
	{
		return NePyInterfacePropertyToPyObjectForStruct;
	}

	if (CastField<FStructProperty>(InProp))
	{
		return NePyStructPropertyToPyObjectForStruct;
	}

	return nullptr;
}

NePyPyObjectToPropertyFuncForStruct NePyGetPyObjectToPropertyConverterForStruct(const FProperty* InProp)
{
	if (InProp->ArrayDim > 1)
	{
		return NePyPyObjectToFixedArrayPropertyForStruct;
	}

	static TMap<FFieldClass*, NePyPyObjectToPropertyFuncForStruct> ConverterMap = {
		{ FBoolProperty::StaticClass(), NePyPyObjectToBoolPropertyForStruct },
		{ FInt8Property::StaticClass(), NePyPyObjectToInt8PropertyForStruct },
		{ FInt16Property::StaticClass(), NePyPyObjectToInt16PropertyForStruct },
		{ FIntProperty::StaticClass(), NePyPyObjectToIntPropertyForStruct },
		{ FInt64Property::StaticClass(), NePyPyObjectToInt64PropertyForStruct },
		{ FByteProperty::StaticClass(), NePyPyObjectToBytePropertyForStruct },
		{ FUInt16Property::StaticClass(), NePyPyObjectToUInt16PropertyForStruct },
		{ FUInt32Property::StaticClass(), NePyPyObjectToUInt32PropertyForStruct },
		{ FUInt64Property::StaticClass(), NePyPyObjectToUInt64PropertyForStruct },
		{ FFloatProperty::StaticClass(), NePyPyObjectToFloatPropertyForStruct },
		{ FDoubleProperty::StaticClass(), NePyPyObjectToDoublePropertyForStruct },
		{ FStrProperty::StaticClass(), NePyPyObjectToStrPropertyForStruct },
		{ FNameProperty::StaticClass(), NePyPyObjectToNamePropertyForStruct },
		{ FTextProperty::StaticClass(), NePyPyObjectToTextPropertyForStruct },
		{ FFieldPathProperty::StaticClass(), NePyPyObjectToFieldPathPropertyForStruct },
		{ FEnumProperty::StaticClass(), NePyPyObjectToEnumPropertyForStruct },
		{ FArrayProperty::StaticClass(), NePyPyObjectToArrayPropertyForStruct },
		{ FMapProperty::StaticClass(), NePyPyObjectToMapPropertyForStruct },
		{ FSetProperty::StaticClass(), NePyPyObjectToSetPropertyForStruct },
	};

	auto ConverterRef = ConverterMap.Find(InProp->GetClass());
	if (ConverterRef)
	{
		return *ConverterRef;
	}

	if (CastField<FClassProperty>(InProp))
	{
		return NePyPyObjectToClassPropertyForStruct;
	}

	//if (CastField<FSoftClassProperty>(InProp))
	//{
	//	return NePyPyObjectToSoftClassPropertyForStruct;
	//}

	if (CastField<FSoftObjectProperty>(InProp))
	{
		return NePyPyObjectToSoftObjectPropertyForStruct;
	}

	if (CastField<FWeakObjectProperty>(InProp))
	{
		return NePyPyObjectToWeakObjectPropertyForStruct;
	}

	if (CastField<FObjectPropertyBase>(InProp))
	{
		return NePyPyObjectToObjectPropertyForStruct;
	}

	if (CastField<FInterfaceProperty>(InProp))
	{
		return NePyPyObjectToInterfacePropertyForStruct;
	}

	if (CastField<FStructProperty>(InProp))
	{
		return NePyPyObjectToStructPropertyForStruct;
	}

	return nullptr;
}

NePyPropertyToPyObjectFunc NePyGetPropertyToPyObjectConverterNoDependency(const FProperty* InProp)
{
	if (CastField<FStructProperty>(InProp))
	{
		return NePyStructPropertyToPyObjectClone;
	}
	
	return NePyGetPropertyToPyObjectConverter(InProp);
}

PyObject* NePyBoolPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FBoolProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyInt8PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FInt8Property>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyInt16PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FInt16Property>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyIntPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FIntProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyInt64PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FInt64Property>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyBytePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FByteProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyUInt16PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FUInt16Property>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyUInt32PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FUInt32Property>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyUInt64PropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FUInt64Property>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyFloatPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FFloatProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyDoublePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FDoubleProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyStrPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FStrProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyNamePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FNameProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyTextPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FTextProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyFieldPathPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FFieldPathProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePySoftObjectPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto&& Value = CastFieldChecked<FSoftObjectProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyWeakObjectPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto&& Value = CastFieldChecked<FWeakObjectProperty>(InProp)->GetPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyEnumPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto&& Value = CastFieldChecked<FEnumProperty>(InProp)->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(InBuffer);
	PyObject* PyValue = nullptr;
	NePyBase::ToPy(Value, PyValue);
	return PyValue;
}

PyObject* NePyArrayPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FArrayProperty>(InProp);

	// dedicated conversion for TArray<uint8>
	auto CastedValueProp = CastField<FByteProperty>(CastedProp->Inner);
	if (CastedValueProp)
	{
		PyObject* PyRetVal;
		const TArray<uint8>* ValuePtr = reinterpret_cast<const TArray<uint8>*>(InBuffer);
		NePyBase::ToPy(*ValuePtr, PyRetVal);
		return PyRetVal;
	}

	if (InOwnerObject && !InProp->GetOwnerUObject()->IsA<UFunction>())
	{
		return FNePyArrayWrapper::New(InOwnerObject, (void*)InBuffer, CastedProp);
	}
	return FNePyArrayWrapper::NewCopy((void*)InBuffer, CastedProp);
}

PyObject* NePyMapPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FMapProperty>(InProp);
	if (InOwnerObject && !InProp->GetOwnerUObject()->IsA<UFunction>())
	{
		return FNePyMapWrapper::New(InOwnerObject, (void*)InBuffer, CastedProp);
	}
	return FNePyMapWrapper::ToPyDict(CastedProp, InBuffer, InOwnerObject);
}

PyObject* NePySetPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FSetProperty>(InProp);
	if (InOwnerObject && !InProp->GetOwnerUObject()->IsA<UFunction>())
	{
		return FNePySetWrapper::New(InOwnerObject, (void*)InBuffer, CastedProp);
	}
	return FNePySetWrapper::ToPySet(CastedProp, InBuffer, InOwnerObject);
}

PyObject* NePyFixedArrayPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	if (InOwnerObject && !InProp->GetOwnerUObject()->IsA<UFunction>())
	{
		return FNePyFixedArrayWrapper::New(InOwnerObject, (void*)InBuffer, InProp);
	}
	return FNePyFixedArrayWrapper::ToPyList(InProp, InBuffer, InOwnerObject);
}

PyObject* NePyClassPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FClassProperty>(InProp);
	UClass* Value = Cast<UClass>(CastedProp->GetObjectPropertyValue(InBuffer));
	return NePyBase::ToPy(Value);
}

PyObject* NePySoftClassPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FSoftClassProperty>(InProp);
	UClass* Value = Cast<UClass>(CastedProp->GetObjectPropertyValue(InBuffer));
	return NePyBase::ToPy(Value);
}

PyObject* NePyObjectPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FObjectPropertyBase>(InProp);
	UObject* Value = CastedProp->GetObjectPropertyValue(InBuffer);
	return NePyBase::ToPy(Value);
}

PyObject* NePyInterfacePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FInterfaceProperty>(InProp);
	UObject* Value = CastedProp->GetPropertyValue(InBuffer).GetObject();
	return NePyBase::ToPy(Value);
}

PyObject* NePyStructPropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto Constructor = [InProp, InBuffer]() -> FNePyPropObject*
	{
		FNePyPropObject* RetValue = nullptr;
		auto CastedProp = CastFieldChecked<FStructProperty>(InProp);
		if (const UScriptStruct* CastedStruct = Cast<UScriptStruct>(CastedProp->Struct))
		{
			const FNePyStructTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(CastedStruct);
			if (PyTypeInfo)
			{
				RetValue = (FNePyPropObject*)PyTypeInfo->PropGetFunc(CastedProp, InBuffer);
				NEPY_RECORD_OBJECT_CREATION(static_cast<FNePyTrackedObject*>(RetValue), "StructProperty");
			}
		}
		return RetValue;
	};

#if NEPY_ENABLE_STRUCT_DEEP_ACCESS
	if (InOwnerObject && !InProp->GetOwnerUObject()->IsA<UFunction>())
	{
		// 这是对象成员，用HouseKeeper保证生命周期，防止野指针
		if (InProp->GetOwner<UClass>() == InOwnerObject->GetClass())
		{
			check(InBuffer == InProp->ContainerPtrToValuePtr<void>(InOwnerObject));
			PyObject* RetValue = FNePyHouseKeeper::Get().NewNePyObjectMember(InOwnerObject, (void*)InBuffer, Constructor);
			if (((FNePyStructBase*)RetValue)->SelfCreatedValue)
			{
				FNePyHouseKeeper::Get().RemoveNePyObjectMember(InOwnerObject, (FNePyStructBase*)RetValue);
			}
			return RetValue;
		}
	}
#endif

	return Constructor();
}

PyObject* NePyDelegatePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FDelegateProperty>(InProp);
	if (InProp->HasAnyPropertyFlags(CPF_Parm))
	{
		if (InBuffer)
		{
			const FScriptDelegate* Delegate = (const FScriptDelegate*)InBuffer;
			return FNePyDynamicDelegateArg::New(*Delegate, CastedProp);
		}
	}
	else
	{
		if (InOwnerObject)
		{
			return FNePyDynamicDelegateWrapper::New(InOwnerObject, CastedProp);
		}
	}
	return nullptr;
}

PyObject* NePyMulticastDelegatePropertyToPyObject(const FProperty* InProp, const void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FMulticastDelegateProperty>(InProp);
	if (InOwnerObject)
	{
		return FNePyDynamicMulticastDelegateWrapper::New(InOwnerObject, CastedProp);
	}
	return nullptr;
}

bool NePyPyObjectToBoolProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	bool Value;
	if (!NePyBase::ToCpp(InPyObj, Value, false))
	{
		return false;
	}
	CastFieldChecked<FBoolProperty>(InProp)->SetPropertyValue(InBuffer, Value);
	return true;
}

bool NePyPyObjectToInt8Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	int8* Value = CastFieldChecked<FInt8Property>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToInt16Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	int16* Value = CastFieldChecked<FInt16Property>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToIntProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	int32* Value = CastFieldChecked<FIntProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToInt64Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	int64* Value = CastFieldChecked<FInt64Property>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToByteProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	uint8* Value = CastFieldChecked<FByteProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToUInt16Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	uint16* Value = CastFieldChecked<FUInt16Property>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToUInt32Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	uint32* Value = CastFieldChecked<FUInt32Property>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToUInt64Property(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	uint64* Value = CastFieldChecked<FUInt64Property>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value, false);
}

bool NePyPyObjectToFloatProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	float* Value = CastFieldChecked<FFloatProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value);
}

bool NePyPyObjectToDoubleProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	double* Value = CastFieldChecked<FDoubleProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value);
}

bool NePyPyObjectToStrProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	FString* Value = CastFieldChecked<FStrProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value);
}

bool NePyPyObjectToNameProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	FName* Value = CastFieldChecked<FNameProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value);
}

bool NePyPyObjectToTextProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	FText* Value = CastFieldChecked<FTextProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value);
}

bool NePyPyObjectToFieldPathProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	FFieldPath* Value = CastFieldChecked<FFieldPathProperty>(InProp)->GetPropertyValuePtr(InBuffer);
	return NePyBase::ToCpp(InPyObj, *Value);
}

bool NePyPyObjectToEnumProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	uint64 Value;
	if (!NePyBase::ToCpp(InPyObj, Value))
	{
		return false;
	}
	auto CastedProp = CastFieldChecked<FEnumProperty>(InProp);
	CastedProp->GetUnderlyingProperty()->SetIntPropertyValue(InBuffer, Value);
	return true;
}

bool NePyPyObjectToDelegateProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject)
{
	if (!InOwnerObject)
	{
		return false;
	}

	auto DelegateProperty = CastFieldChecked<FDelegateProperty>(InProp);
	// 目前只有Delegate作为函数参数时，才会进入此分支
	check(DelegateProperty->GetOwnerUObject()->IsA<UFunction>());

	if (InOwnerObject->HasAnyFlags(RF_ClassDefaultObject))
	{
		NePyStealReference((PyObject*)FNePyHouseKeeper::Get().NewNePyObject(InOwnerObject));
	}
	FNePyDynamicDelegateParam* DelegateParm = FNePyDynamicDelegateParam::New(InOwnerObject, DelegateProperty);
	Py_DECREF(DelegateParm);
	UNePyDelegate* PyDelegate = DelegateParm->FindOrAddDelegate(InPyObj);

	FScriptDelegate ScriptDelegate = DelegateProperty->GetPropertyValue(InBuffer);
	ScriptDelegate.BindUFunction(PyDelegate, UNePyDelegate::FakeFuncName);
	DelegateProperty->SetPropertyValue(InBuffer, ScriptDelegate);
	return true;
}

bool NePyPyObjectToArrayProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FArrayProperty>(InProp);

	// dedicated conversion for TArray<uint8>
	auto CastedValueProp = CastField<FByteProperty>(CastedProp->Inner);
	if (CastedValueProp)
	{
		TArray<uint8>* ValuePtr = reinterpret_cast<TArray<uint8>*>(InBuffer);
		return NePyBase::ToCpp(InPyObj, *ValuePtr);
	}

	return FNePyArrayWrapper::Assign(InPyObj, CastedProp, InBuffer, InOwnerObject);
}

bool NePyPyObjectToMapProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FMapProperty>(InProp);
	return FNePyMapWrapper::Assign(InPyObj, CastedProp, InBuffer, InOwnerObject);
}

bool NePyPyObjectToSetProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject)
{
	auto CastedProp = CastFieldChecked<FSetProperty>(InProp);
	return FNePySetWrapper::Assign(InPyObj, CastedProp, InBuffer, InOwnerObject);
}

bool NePyPyObjectToFixedArrayProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject* InOwnerObject)
{
	return FNePyFixedArrayWrapper::Assign(InPyObj, InProp, InBuffer, InOwnerObject);
}

bool NePyPyObjectToClassProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FClassProperty>(InProp);
	if (InPyObj == Py_None)
	{
		CastedProp->SetObjectPropertyValue(InBuffer, nullptr);
		return true;
	}

	UClass* Value = NePyBase::ToCppClass(InPyObj, CastedProp->MetaClass);
	if (!Value)
	{
		return false;
	}
	CastedProp->SetObjectPropertyValue(InBuffer, Value);
	return true;
}

bool NePyPyObjectToSoftClassProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FSoftClassProperty>(InProp);
	// 第一种情况，传入 nullptr 表示置空
	if (InPyObj == Py_None)
	{
		CastedProp->SetObjectPropertyValue(InBuffer, nullptr);
		return true;
	}
	// 第二种情况，直接传入 SoftPtr 对象
	FSoftObjectPtr* PtrValue = CastedProp->GetPropertyValuePtr(InBuffer);
	if (NePyBase::ToCpp(InPyObj, *PtrValue))
	{
		return true;
	}
	// 第三种情况，传入 UClass 或 PyType，转换赋值给 FSoftObjectPtr
	UClass* ClassValue = NePyBase::ToCppClass(InPyObj, CastedProp->MetaClass);
	if (!ClassValue)
	{
		return false;
	}
	CastedProp->SetObjectPropertyValue(InBuffer, ClassValue);
	return true;
}

bool NePyPyObjectToObjectProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FObjectPropertyBase>(InProp);
	if (InPyObj == Py_None)
	{
		CastedProp->SetObjectPropertyValue(InBuffer, nullptr);
		return true;
	}

	UObject* Value = NePyBase::ToCppObject(InPyObj, CastedProp->PropertyClass);
	if (!Value)
	{
		return false;
	}
	CastedProp->SetObjectPropertyValue(InBuffer, Value);
	return true;
}

bool NePyPyObjectToSoftObjectProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FSoftObjectProperty>(InProp);
	// 第一种情况，传入 nullptr 表示置空
	if (InPyObj == Py_None)
	{
		CastedProp->SetObjectPropertyValue(InBuffer, nullptr);
		return true;
	}
	// 第二种情况，直接传入 SoftPtr 对象
	FSoftObjectPtr* PtrValue = CastedProp->GetPropertyValuePtr(InBuffer);
	if (NePyBase::ToCpp(InPyObj, *PtrValue))
	{
		return true;
	}
	// 第三种情况，传入 UObject，转换赋值给 FSoftObjectPtr
	UObject* ObjectValue = NePyBase::ToCppObject(InPyObj, CastedProp->PropertyClass);
	if (!ObjectValue)
	{
		return false;
	}
	CastedProp->SetObjectPropertyValue(InBuffer, ObjectValue);
	return true;
}

bool NePyPyObjectToWeakObjectProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	// 第一种情况，传入 nullptr 表示置空
	auto CastedProp = CastFieldChecked<FWeakObjectProperty>(InProp);
	if (InPyObj == Py_None)
	{
		CastedProp->SetObjectPropertyValue(InBuffer, nullptr);
		return true;
	}
	// 第二种情况，直接传入 WeakPtr 对象
	FWeakObjectPtr* PtrValue = CastedProp->GetPropertyValuePtr(InBuffer);
	if (NePyBase::ToCpp(InPyObj, *PtrValue))
	{
		return true;
	}
	// 第三种情况，传入 UObject，转换赋值给 FWeakObjectPtr
	UObject* Value = NePyBase::ToCppObject(InPyObj, CastedProp->PropertyClass);
	if (!Value)
	{
		return false;
	}
	CastedProp->SetObjectPropertyValue(InBuffer, Value);
	return true;
}

bool NePyPyObjectToInterfaceProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FInterfaceProperty>(InProp);
	if (InPyObj == Py_None)
	{
		CastedProp->SetPropertyValue(InBuffer, FScriptInterface());
		return true;
	}

	UObject* Value = NePyBase::ToCppObject(InPyObj);
	if (!Value)
	{
		return false;
	}
	if (!Value->GetClass()->ImplementsInterface(CastedProp->InterfaceClass))
	{
		return false;
	}
	CastedProp->SetPropertyValue(InBuffer, FScriptInterface(Value, Value->GetInterfaceAddress(CastedProp->InterfaceClass)));
	return true;
}

bool NePyPyObjectToStructProperty(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, UObject*)
{
	auto CastedProp = CastFieldChecked<FStructProperty>(InProp);
	if (NePyStructBaseCheck(InPyObj)
		|| CastedProp->Struct == FNePyObjectRef::StaticStruct()) // 对NePyObjectRef的特殊处理
	{
		const FNePyStructTypeInfo* PyTypeInfo = FNePyWrapperTypeRegistry::Get().GetWrappedStructType(CastedProp->Struct);
		if (PyTypeInfo)
		{
			bool bSucc = PyTypeInfo->PropSetFunc(CastedProp, InPyObj, InBuffer);
			return bSucc;
		}
	}
	return false;
}

PyObject* NePyBoolPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyBoolPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyInt8PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyInt8PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyInt16PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyInt16PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyIntPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyIntPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyInt64PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyInt64PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyBytePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyBytePropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyUInt16PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyUInt16PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyUInt32PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyUInt32PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyUInt64PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyUInt64PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyFloatPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyFloatPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyDoublePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyDoublePropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyStrPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyStrPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyNamePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyNamePropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyTextPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyTextPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyFieldPathPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyFieldPathPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyEnumPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyEnumPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyArrayPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter)
{
	auto CastedProp = CastFieldChecked<FArrayProperty>(InProp);

	// dedicated conversion for TArray<uint8>
	auto CastedValueProp = CastField<FByteProperty>(CastedProp->Inner);
	if (CastedValueProp)
	{
		PyObject* PyRetVal;
		const TArray<uint8>* ValuePtr = reinterpret_cast<const TArray<uint8>*>(InBuffer);
		NePyBase::ToPy(*ValuePtr, PyRetVal);
		return PyRetVal;
	}

	if (InPyOuter)
	{
		return FNePyStructArrayWrapper::New(InPyOuter, (void*)InBuffer, CastedProp);
	}
	return FNePyArrayWrapper::NewCopy((void*)InBuffer, CastedProp);
}

PyObject* NePyMapPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter)
{
	auto CastedProp = CastFieldChecked<FMapProperty>(InProp);
	if (InPyOuter)
	{
		return FNePyStructMapWrapper::New(InPyOuter, (void*)InBuffer, CastedProp);
	}
	return FNePyMapWrapper::ToPyDict(CastedProp, InBuffer, nullptr);
}

PyObject* NePySetPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter)
{
	auto CastedProp = CastFieldChecked<FSetProperty>(InProp);
	if (InPyOuter)
	{
		return FNePyStructSetWrapper::New(InPyOuter, (void*)InBuffer, CastedProp);
	}
	return FNePySetWrapper::ToPySet(CastedProp, InBuffer, nullptr);
}

PyObject* NePyFixedArrayPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter)
{
	if (InPyOuter)
	{
		return FNePyStructFixedArrayWrapper::New(InPyOuter, (void*)InBuffer, InProp);
	}
	return FNePyFixedArrayWrapper::ToPyList(InProp, InBuffer, nullptr);
}

PyObject* NePySoftObjectPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePySoftObjectPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyClassPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyClassPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePySoftClassPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePySoftClassPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyObjectPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyObjectPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyWeakObjectPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyWeakObjectPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyInterfacePropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject*)
{
	return NePyInterfacePropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyStructPropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer, PyObject* InPyOuter)
{
	PyObject* PyStruct = NePyStructPropertyToPyObject(InProp, InBuffer, nullptr);
	if (FNePyStructBase* PyStructBase = NePyStructBaseCheck(PyStruct))
	{
		// 视乎具体的代码路径，SelfCreateValue会不同
		const UScriptStruct* ScriptStruct = FNePyWrapperTypeRegistry::Get().GetStructByPyType(PyStruct->ob_type);
		// 这个一个对Outer内存引用创建的对象，增加对Outer的引用，避免野指针问题
		if (ScriptStruct && !PyStructBase->SelfCreatedValue)
		{
			PyStructBase->PyOuter = InPyOuter;
			Py_XINCREF(PyStructBase->PyOuter);
		}
	}
	return PyStruct;
}

bool NePyPyObjectToBoolPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToBoolProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToInt8PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToInt8Property(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToInt16PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToInt16Property(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToIntPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToIntProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToInt64PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToInt64Property(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToBytePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToByteProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToUInt16PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToUInt16Property(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToUInt32PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToUInt32Property(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToUInt64PropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToUInt64Property(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToFloatPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToFloatProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToDoublePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToDoubleProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToStrPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToStrProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToNamePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToNameProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToTextPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToTextProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToFieldPathPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToFieldPathProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToEnumPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToEnumProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToArrayPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToArrayProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToMapPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToMapProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToSetPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToSetProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToFixedArrayPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToFixedArrayProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToClassPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToClassProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToSoftClassPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToSoftClassProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToSoftObjectPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToSoftObjectProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToWeakObjectPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToWeakObjectProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToObjectPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToObjectProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToInterfacePropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToInterfaceProperty(InPyObj, InProp, InBuffer, nullptr);
}

bool NePyPyObjectToStructPropertyForStruct(PyObject* InPyObj, const FProperty* InProp, void* InBuffer, PyObject*)
{
	return NePyPyObjectToStructProperty(InPyObj, InProp, InBuffer, nullptr);
}

PyObject* NePyBoolPropertyToPyObjectClone(const FProperty* InProp, const void* InBuffer)
{
	return NePyBoolPropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyInt8PropertyToPyObjectClone(const FProperty* InProp, const void* InBuffer)
{
	return NePyInt8PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyInt16PropertyToPyObjectForStruct(const FProperty* InProp, const void* InBuffer)
{
	return NePyInt16PropertyToPyObject(InProp, InBuffer, nullptr);
}

PyObject* NePyStructPropertyToPyObjectClone(const FProperty* InProp, const void* InBuffer, UObject* /*unused*/)
{
	// 对于非对象成员（如函数返回值、数组/Map/Set中的元素），必须返回一个拥有独立内存的克隆（深拷贝），
	// 以防止其宿主容器（Host Container）被GC后，导致结构体内部指针悬空。
	//
	// 真实崩溃用例 (Use-After-Free):
	// Python代码:
	//   my_list = [item for item in cpp_func_returns_array_wrapper()]
	//
	// 崩溃流程:
	// 1. `cpp_func_returns_array_wrapper()` 返回一个临时的 ArrayWrapper (我们称之为 Host)，它拥有C++数据的内存。
	// 2. 列表推导式开始迭代这个 Host(ArrayWrapper)。在每次迭代中，`item` 变量被创建，这个创建过程会调用到本函数 `NePyStructPropertyToPyObject`。
	// 3. 【旧的错误逻辑】: 本函数直接调用 `Constructor()` 返回一个包装器，其内部 `Value` 指针指向 Host(ArrayWrapper) 的内存，`SelfCreatedValue` 为 false。这只是一个引用，不是拷贝。
	// 4. `item` 对象（一个危险的引用）被存入 `my_list`。
	// 5. 列表推导式结束后，临时的 Host(ArrayWrapper) 因为不再被任何变量引用，被Python GC回收，其拥有的C++内存被释放。
	// 6. 【崩溃点】: `my_list` 现在包含了一系列指向已释放内存的悬空指针。当后续代码访问 `my_list` 中任何元素的成员时（如 `my_list[0].some_property`），发生 use-after-free。
	//
	// 【修复方案】:
	// 通过检查 `SelfCreatedValue` 标志，我们识别出那些仅为引用的包装器 (`SelfCreatedValue` == false)，
	// 并强制调用 `FNePyStructBase::Clone()` 创建一个拥有独立内存的深拷贝。这样，即使原始的宿主容器被回收，返回给Python的结构体对象依然安全有效。

	PyObject* PyStruct = NePyStructPropertyToPyObject(InProp, InBuffer, nullptr);
	if (FNePyStructBase* PyStructBase = NePyStructBaseCheck(PyStruct))
	{
		// 视乎具体的代码路径，SelfCreateValue会不同
		if (!PyStructBase->SelfCreatedValue)
		{
			PyStruct = FNePyStructBase::Clone(PyStructBase);
			Py_XDECREF(PyStructBase);
		}
	}
	return PyStruct;
}