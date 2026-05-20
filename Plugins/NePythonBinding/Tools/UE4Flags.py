## -*- encoding: utf-8 -*-
# ------------------------------------------
# EStructFlags
#
STRUCT_Native = 0x00000001
# If set, this struct will be compared using native code
STRUCT_IdenticalNative = 0x00000002
#
STRUCT_HasInstancedReference = 0x00000004
#
STRUCT_NoExport = 0x00000008
# Indicates that this struct should always be serialized as a single unit
STRUCT_Atomic = 0x00000010
# Indicates that this struct uses binary serialization; it is unsafe to add/remove members from this struct without incrementing the package version
STRUCT_Immutable = 0x00000020
# If set, native code needs to be run to find referenced objects
STRUCT_AddStructReferencedObjects = 0x00000040
# Indicates that this struct should be exportable/importable at the DLL layer. Base structs must also be exportable for this to work.
STRUCT_RequiredAPI = 0x00000200
# If set, this struct will be serialized using the CPP net serializer
STRUCT_NetSerializeNative = 0x00000400
# If set, this struct will be serialized using the CPP serializer
STRUCT_SerializeNative = 0x00000800
# If set, this struct will be copied using the CPP operator=
STRUCT_CopyNative = 0x00001000
# If set, this struct will be copied using memcpy
STRUCT_IsPlainOldData = 0x00002000
# If set, this struct has no destructor and non will be called. STRUCT_IsPlainOldData implies STRUCT_NoDestructor
STRUCT_NoDestructor = 0x00004000
# If set, this struct will not be constructed because it is assumed that memory is zero before construction.
STRUCT_ZeroConstructor = 0x00008000
# If set, native code will be used to export text
STRUCT_ExportTextItemNative = 0x00010000
# If set, native code will be used to export text
STRUCT_ImportTextItemNative = 0x00020000
# If set, this struct will have PostSerialize called on it after CPP serializer or tagged property serialization is complete
STRUCT_PostSerializeNative = 0x00040000
# If set, this struct will have SerializeFromMismatchedTag called on it if a mismatched tag is encountered.
STRUCT_SerializeFromMismatchedTag = 0x00080000
# If set, this struct will be serialized using the CPP net delta serializer
STRUCT_NetDeltaSerializeNative = 0x00100000
# If set, this struct will be have PostScriptConstruct called on it after a temporary object is constructed in a running blueprint
STRUCT_PostScriptConstruct = 0x00200000
# If set, this struct can share net serialization state across connections
STRUCT_NetSharedSerialization = 0x00400000
# If set, this struct has been cleaned and sanitized (trashed) and should not be used
STRUCT_Trashed = 0x00800000
# ------------------------------------------
# EClassFlags
# Class is abstract and can't be instantiated directly.
CLASS_Abstract = 0x00000001
# Save object configuration only to Default INIs, never to local INIs. Must be combined with CLASS_Config
CLASS_DefaultConfig = 0x00000002
# Load object configuration at construction time.
CLASS_Config = 0x00000004
# This object type can't be saved; null it out at save time.
CLASS_Transient = 0x00000008
# Successfully parsed.
CLASS_Parsed = 0x00000010
#
CLASS_MatchedSerializers = 0x00000020
# All the properties on the class are shown in the advanced section (which is hidden by default) unless SimpleDisplay is specified on the property
CLASS_AdvancedDisplay = 0x00000040
# Class is a native class - native interfaces will have CLASS_Native set, but not RF_MarkAsNative
CLASS_Native = 0x00000080
# Don't export to C++ header.
CLASS_NoExport = 0x00000100
# Do not allow users to create in the editor.
CLASS_NotPlaceable = 0x00000200
# Handle object configuration on a per-object basis, rather than per-class.
CLASS_PerObjectConfig = 0x00000400
# Whether SetUpRuntimeReplicationData still needs to be called for this class
CLASS_ReplicationDataIsSetUp = 0x00000800
# Class can be constructed from editinline New button.
CLASS_EditInlineNew = 0x00001000
# Display properties in the editor without using categories.
CLASS_CollapseCategories = 0x00002000
# Class is an interface
CLASS_Interface = 0x00004000
# Do not export a constructor for this class, assuming it is in the cpptext
CLASS_CustomConstructor = 0x00008000
# all properties and functions in this class are const and should be exported as const
CLASS_Const = 0x00010000
# Class flag indicating the class is having its layout changed, and therefore is not ready for a CDO to be created
CLASS_LayoutChanging = 0x00020000
# Indicates that the class was created from blueprint source material
CLASS_CompiledFromBlueprint = 0x00040000
# Indicates that only the bare minimum bits of this class should be DLL exported/imported
CLASS_MinimalAPI = 0x00080000
# Indicates this class must be DLL exported/imported (along with all of it's members)
CLASS_RequiredAPI = 0x00100000
# Indicates that references to this class default to instanced. Used to be subclasses of UComponent, but now can be any UObject
CLASS_DefaultToInstanced = 0x00200000
# Indicates that the parent token stream has been merged with ours
CLASS_TokenStreamAssembled = 0x00400000
# Class has component properties
CLASS_HasInstancedReference = 0x00800000
# Don't show this class in the editor class browser or edit inline new menus
CLASS_Hidden = 0x01000000
# Don't save objects of this class when serializing
CLASS_Deprecated = 0x02000000
# Class not shown in editor drop down for class selection
CLASS_HideDropDown = 0x04000000
# Class settings are saved to <AppData>/..../Blah.ini (as opposed to CLASS_DefaultConfig)
CLASS_GlobalUserConfig = 0x08000000
# Class was declared directly in C++ and has no boilerplate generated by UnrealHeaderTool
CLASS_Intrinsic = 0x10000000
# Class has already been constructed (maybe in a previous DLL version before hot-reload).
CLASS_Constructed = 0x20000000
# Indicates that object configuration will not check against ini base/defaults when serialized
CLASS_ConfigDoNotCheckDefaults = 0x40000000
# Class has been consigned to oblivion as part of a blueprint recompile, and a newer version currently exists.
CLASS_NewerVersionExists = 0x80000000

# ------------------------------------------
# EFunctionFlags
FUNC_Final = 0x00000001 # Function is final (prebindable, non-overridable function).
FUNC_RequiredAPI = 0x00000002 # Indicates this function is DLL exported/imported.
FUNC_BlueprintAuthorityOnly = 0x00000004   # Function will only run if the object has network authority
FUNC_BlueprintCosmetic = 0x00000008   # Function is cosmetic in nature and should not be invoked on dedicated servers
# FUNC_ = 0x00000010   # unused.
# FUNC_ = 0x00000020   # unused.
FUNC_Net = 0x00000040   # Function is network-replicated.
FUNC_NetReliable = 0x00000080   # Function should be sent reliably on the network.
FUNC_NetRequest = 0x00000100 # Function is sent to a net service
FUNC_Exec = 0x00000200 # Executable from command line.
FUNC_Native = 0x00000400 # Native function.
FUNC_Event = 0x00000800   # Event function.
FUNC_NetResponse = 0x00001000   # Function response from a net service
FUNC_Static = 0x00002000   # Static function.
FUNC_NetMulticast = 0x00004000 # Function is networked multicast Server -> All Clients
FUNC_UbergraphFunction = 0x00008000   # Function is used as the merge 'ubergraph' for a blueprint, only assigned when using the persistent 'ubergraph' frame
FUNC_MulticastDelegate = 0x00010000 # Function is a multi-cast delegate signature (also requires FUNC_Delegate to be set!)
FUNC_Public = 0x00020000 # Function is accessible in all classes (if overridden, parameters must remain unchanged).
FUNC_Private = 0x00040000 # Function is accessible only in the class it is defined in (cannot be overridden, but function name may be reused in subclasses.  IOW: if overridden, parameters don't need to match, and Super.Func() cannot be accessed since it's private.)
FUNC_Protected = 0x00080000 # Function is accessible only in the class it is defined in and subclasses (if overridden, parameters much remain unchanged).
FUNC_Delegate = 0x00100000 # Function is delegate signature (either single-cast or multi-cast, depending on whether FUNC_MulticastDelegate is set.)
FUNC_NetServer = 0x00200000 # Function is executed on servers (set by replication code if passes check)
FUNC_HasOutParms = 0x00400000 # function has out (pass by reference) parameters
FUNC_HasDefaults = 0x00800000 # function has structs that contain defaults
FUNC_NetClient = 0x01000000 # function is executed on clients
FUNC_DLLImport = 0x02000000 # function is imported from a DLL
FUNC_BlueprintCallable = 0x04000000 # function can be called from blueprint code
FUNC_BlueprintEvent = 0x08000000 # function can be overridden/implemented from a blueprint
FUNC_BlueprintPure = 0x10000000 # function can be called from blueprint code, and is also pure (produces no side effects). If you set this, you should set FUNC_BlueprintCallable as well.
FUNC_EditorOnly = 0x20000000 # function can only be called from an editor scrippt.
FUNC_Const = 0x40000000 # function can be called from blueprint code, and only reads state (never writes state)
FUNC_NetValidate = 0x80000000 # function must supply a _Validate implementation

# ------------------------------------------
# EPropertyFlags
CPF_Edit = 0x0000000000000001 # Property is user-settable in the editor.
CPF_ConstParm = 0x0000000000000002 # This is a constant function parameter
CPF_BlueprintVisible = 0x0000000000000004 # This property can be read by blueprint code
CPF_ExportObject = 0x0000000000000008 # Object can be exported with actor.
CPF_BlueprintReadOnly = 0x0000000000000010 # This property cannot be modified by blueprint code
CPF_Net = 0x0000000000000020 # Property is relevant to network replication.
CPF_EditFixedSize = 0x0000000000000040 # Indicates that elements of an array can be modified but its size cannot be changed.
CPF_Parm = 0x0000000000000080 # Function/When call parameter.
CPF_OutParm = 0x0000000000000100 # Value is copied out after function call.
CPF_ZeroConstructor = 0x0000000000000200 # memset is fine for construction
CPF_ReturnParm = 0x0000000000000400 # Return value.
CPF_DisableEditOnTemplate = 0x0000000000000800 # Disable editing of this property on an archetype/sub-blueprint
CPF_NonNullable = 0x0000000000001000 # Object property can never be null
CPF_Transient = 0x0000000000002000 # Property is transient: shouldn't be saved or loaded except for Blueprint CDOs.
CPF_Config = 0x0000000000004000 # Property should be loaded/saved as permanent profile.
CPF_DisableEditOnInstance = 0x0000000000010000 # Disable editing on an instance of this class
CPF_EditConst = 0x0000000000020000 # Property is uneditable in the editor.
CPF_GlobalConfig = 0x0000000000040000 # Load config from base class not subclass.
CPF_InstancedReference = 0x0000000000080000 # Property is a component references.
CPF_DuplicateTransient = 0x0000000000200000 # Property should always be reset to the default value during any type of duplication (copy/paste binary duplication etc.)
CPF_SubobjectReference = 0x0000000000400000 # Property contains subobject references (TSubobjectPtr)
CPF_SaveGame = 0x0000000001000000 # Property should be serialized for save games this is only checked for game-specific archives with ArIsSaveGame
CPF_NoClear = 0x0000000002000000 # Hide clear (and browse) button.
CPF_ReferenceParm = 0x0000000008000000 # Value is passed by reference; CPF_OutParam and CPF_Param should also be set.
CPF_BlueprintAssignable = 0x0000000010000000 # MC Delegates only. Property should be exposed for assigning in blueprint code
CPF_Deprecated = 0x0000000020000000 # Property is deprecated. Read it from an archive but don't save it.
CPF_IsPlainOldData = 0x0000000040000000 # If this is set then the property can be memcopied instead of CopyCompleteValue / CopySingleValue
CPF_RepSkip = 0x0000000080000000 # Not replicated. For non replicated properties in replicated structs
CPF_RepNotify = 0x0000000100000000 # Notify actors when a property is replicated
CPF_Interp = 0x0000000200000000 # interpolatable property for use with matinee
CPF_NonTransactional = 0x0000000400000000 # Property isn't transacted
CPF_EditorOnly = 0x0000000800000000 # Property should only be loaded in the editor
CPF_NoDestructor = 0x0000001000000000 # No destructor
CPF_AutoWeak = 0x0000004000000000 # Only used for weak pointers means the export type is autoweak
CPF_ContainsInstancedReference = 0x0000008000000000 # Property contains component references.
CPF_AssetRegistrySearchable = 0x0000010000000000 # asset instances will add properties with this flag to the asset registry automatically
CPF_SimpleDisplay = 0x0000020000000000 # The property is visible by default in the editor details view
CPF_AdvancedDisplay = 0x0000040000000000 # The property is advanced and not visible by default in the editor details view
CPF_Protected = 0x0000080000000000 # property is protected from the perspective of script
CPF_BlueprintCallable = 0x0000100000000000 # MC Delegates only. Property should be exposed for calling in blueprint code
CPF_BlueprintAuthorityOnly = 0x0000200000000000 # MC Delegates only. This delegate accepts (only in blueprint) only events with BlueprintAuthorityOnly.
CPF_TextExportTransient = 0x0000400000000000 # Property shouldn't be exported to text format (e.g. copy/paste)
CPF_NonPIEDuplicateTransient = 0x0000800000000000 # Property should only be copied in PIE
CPF_ExposeOnSpawn = 0x0001000000000000 # Property is exposed on spawn
CPF_PersistentInstance = 0x0002000000000000 # A object referenced by the property is duplicated like a component. (Each actor should have an own instance.)
CPF_UObjectWrapper = 0x0004000000000000 # Property was parsed as a wrapper class like TSubclassOf<T> FScriptInterface etc. rather than a USomething*
CPF_HasGetValueTypeHash = 0x0008000000000000 # This property can generate a meaningful hash value.
CPF_NativeAccessSpecifierPublic = 0x0010000000000000 # Public native access specifier
CPF_NativeAccessSpecifierProtected = 0x0020000000000000 # Protected native access specifier
CPF_NativeAccessSpecifierPrivate = 0x0040000000000000 # Private native access specifier
CPF_SkipSerialization = 0x0080000000000000 # Property shouldn't be serialized can still be exported to text

# ------------------------------------------
# UEnum::ECppForm
ECF_Regular = 0
ECF_Namespaced = 1
ECF_EnumClass = 2


# ------------------------------------------
# 所有可标注的参数flag，参考自：VariableSpecifiers.def
ALL_VARIABLE_SPECIFIER = ['AdvancedDisplay', 'AssetRegistrySearchable', 'BlueprintAssignable', 'BlueprintAuthorityOnly', 'BlueprintCallable', 'BlueprintGetter', 'BlueprintReadOnly', 'BlueprintReadWrite', 'BlueprintSetter', 'Config', 'Const', 'DuplicateTransient', 'EditAnywhere', 'EditDefaultsOnly', 'EditFixedSize', 'EditInline', 'EditInstanceOnly', 'Export', 'GlobalConfig', 'Instanced', 'Interp', 'Localized', 'NoClear', 'NonPIEDuplicateTransient', 'NonPIETransient', 'NonTransactional', 'NotReplicated', 'Ref', 'Replicated', 'ReplicatedUsing', 'RepRetry', 'SaveGame', 'SimpleDisplay', 'SkipSerialization', 'TextExportTransient', 'Transient', 'VisibleAnywhere', 'VisibleDefaultsOnly', 'VisibleInstanceOnly']

# -------------------------------------------
# 所有可标注的函数flag，参考自：FunctionSpecifiers.def
ALL_FUNCTION_SPECIFIER = ['BlueprintAuthorityOnly', 'BlueprintCallable', 'BlueprintCosmetic', 'BlueprintGetter', 'BlueprintImplementableEvent', 'BlueprintNativeEvent', 'BlueprintPure', 'BlueprintSetter', 'Client', 'CustomThunk', 'Exec', 'NetMulticast', 'Reliable', 'SealedEvent', 'Server', 'ServiceRequest', 'ServiceResponse', 'Unreliable', 'WithValidation']


# 将EStructFlags转化为可读的字符串描述
def explain_struct_flags(struct_flags):
	if not _g_struct_flag_names_dict:
		_init_flag_names('STRUCT_', _g_struct_flag_names_dict, _g_struct_flag_names_list)

	result = []
	for flag_value, flag_name in _g_struct_flag_names_list:
		if struct_flags & flag_value:
			result.append(flag_name)
	return '|'.join(result)


# 将EClassFlags转化为可读的字符串描述
def explain_class_flags(class_flags):
	if not _g_class_flag_names_dict:
		_init_flag_names('CLASS_', _g_class_flag_names_dict, _g_class_flag_names_list)

	result = []
	for flag_value, flag_name in _g_class_flag_names_list:
		if class_flags & flag_value:
			result.append(flag_name)
	return '|'.join(result)


# 将EFunctionFlags转化为可读的字符串描述
def explain_func_flags(func_flags, check_specifier=False):
	if not _g_func_flag_names_dict:
		_init_flag_names('FUNC_', _g_func_flag_names_dict, _g_func_flag_names_list)

	result = []
	for flag_value, flag_name in _g_func_flag_names_list:
		if func_flags & flag_value:
			if check_specifier and flag_name not in ALL_FUNCTION_SPECIFIER:
				continue
			result.append(flag_name)
	return '|'.join(result)


# 将EPropertyFlags转化为可读的字符串描述
def explain_prop_flags(prop_flags, check_specifier=False):
	if not _g_prop_flag_names_dict:
		_init_flag_names('CPF_', _g_prop_flag_names_dict, _g_prop_flag_names_list)

	result = []
	for flag_value, flag_name in _g_prop_flag_names_list:
		if prop_flags & flag_value:
			if check_specifier and flag_name not in ALL_VARIABLE_SPECIFIER:
				continue
			result.append(flag_name)
	return '|'.join(result)


# 初始化枚举值到名字的映射表
def _init_flag_names(flag_prefix, out_dict, out_list):
	# need to be compatible with py2 and py3
	for key, val in globals().items():
		if not key.startswith(flag_prefix):
			continue
		key = key[len(flag_prefix):]
		out_dict[val] = key

	# need to be compatible with py2 and py3
	out_list.extend(sorted(out_dict.items()))


_g_struct_flag_names_dict = {}
_g_struct_flag_names_list = []
_g_class_flag_names_dict = {}
_g_class_flag_names_list = []
_g_func_flag_names_dict = {}
_g_func_flag_names_list = []
_g_prop_flag_names_dict = {}
_g_prop_flag_names_list = []


def test():
	from pprint import pprint
	explain_struct_flags(0)
	pprint(_g_struct_flag_names_list)
	explain_class_flags(0)
	pprint(_g_class_flag_names_list)
	explain_func_flags(0)
	pprint(_g_func_flag_names_list)
	explain_prop_flags(0)
	pprint(_g_prop_flag_names_list)

if __name__ == '__main__':
	test()
