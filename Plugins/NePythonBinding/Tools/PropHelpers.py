# -*- encoding: utf-8 -*-
import copy
import ExportConfig
import UE4Flags
from GeneratorContext import GeneratorContext, ExportContext


# 类型是否应该被静态导出
def is_export_type(type_info):
	# type: (dict) -> bool
	return type_info and 'py_name' in type_info


# 属性是否为内置类型
def is_builtin_type(prop_info):
	# type: (dict) -> bool
	prop_type = prop_info['type']
	if prop_type == 'ByteProperty':
		# 在这种情况下，它其实是个Enum
		return 'enum_name' not in prop_info
	return prop_type in BUILTIN_TYPES

# 属性是否为枚举类型


def is_enum_type(prop_info):
	# type: (dict) -> bool
	prop_type = prop_info['type']
	if prop_type == 'ByteProperty':
		return 'enum_name' in prop_info
	return prop_type == 'EnumProperty'


# 属性是否为字符串类型
def is_string_type(prop_info):
	# type: (dict) -> bool
	return prop_info['type'] in STRING_TYPES


# 属性是否为Delegate类型
def is_delegate_type(prop_info):
	# type: (dict) -> bool
	return prop_info['type'] in DELEGATE_TYPES

# 属性是否为数组类型


def is_array_type(prop_info):
	# type: (dict) -> bool
	return prop_info['array_dim'] > 1 or prop_info['type'] == 'ArrayProperty'

# 是否为容器类型


def is_container_type(prop_info):
	# type: (dict) -> bool
	return prop_info['array_dim'] > 1 \
		or (prop_info['type'] in ('ArrayProperty', 'SetProperty', 'MapProperty'))

# 是否为指针类型


def is_pointer_type(prop_info):
	# type: (dict) -> bool
	return prop_info['array_dim'] == 1 \
		and (prop_info['type'] in ('ObjectProperty', 'ClassProperty', 'ClassPtrProperty'))


# 是否为字节缓冲类型
def is_bytes_type(prop_info):
	# type: (dict) -> bool
	if not is_array_type(prop_info):
		return False
	inner_prop_info = prop_info['inner_prop']
	inner_type_info = inner_prop_info['type_info']
	return not is_enum_type(inner_prop_info) and is_export_type(inner_type_info) and inner_type_info['cpp_name'] == 'uint8'


def is_soft_type(prop_info):
	# type: (dict) -> bool
	return prop_info['type'] in ("SoftObjectProperty", "SoftClassProperty")


# 是否允许为 none
def is_allow_none_type(prop_info):
	# 允许 None 的条件：1. 全局开启，2. UObject指针，3. 不含 CPF_NonNullable
	# 暂时不清楚 TScriptInterface 是什么情况，不支持。
	return ExportConfig.AllowPropertySetNone\
		and is_pointer_type(prop_info)\
		and not prop_info['prop_flags'] & UE4Flags.CPF_NonNullable


# 是否能通过TBaseStructure<>模板获取到UScriptStruct
def has_export_struct_type(type_info):
	# type: (dict) -> bool
	return not (type_info['struct_flags'] & UE4Flags.STRUCT_NoExport) \
		or (type_info['name'] in ExportConfig.NeedRegisterStruct)


# 提取属性的C++类型
def extract_cpp_type(prop_info):
	# type: (dict) -> str
	prop_type = prop_info['type']
	type_info = prop_info['type_info']
	if not is_export_type(type_info):
		return 'unsupported'

	if is_enum_type(prop_info):
		if prop_type == 'ByteProperty':
			return 'TEnumAsByte<%s>' % prop_info['enum_cpp_name']
		return prop_info['enum_cpp_name']

	if prop_type == 'ClassProperty':
		prop_flags = prop_info['prop_flags']
		if prop_flags & UE4Flags.CPF_UObjectWrapper:
			return 'TSubclassOf<%s>' % type_info['cpp_name']
		else:
			return 'UClass*'

	if prop_type == 'SoftObjectProperty':
		prop_flags = prop_info['prop_flags']
		return 'TSoftObjectPtr<%s>' % type_info['cpp_name']

	if prop_type == 'SoftClassProperty':
		prop_flags = prop_info['prop_flags']
		return 'TSoftClassPtr<%s>' % type_info['cpp_name']

	cpp_type = type_info['cpp_name']
	if is_pointer_type(prop_info):
		cpp_type += '*'
	return cpp_type


# 字符串是否是数字
def is_number(str):
	# type: (str) -> bool
	try:
		# 因为使用float有一个例外是'NaN'
		if str == 'NaN':
			return False
		float(str)
		return True
	except ValueError:
		return False


# 解析属性信息，返回处理过后的prop_info和type_info
# 如果遇到容器类型，则会递归处理
def parse_prop_info(prop_info, context):
	# type: (dict, ExportContext) -> dict
	prop_type = prop_info['type']

	# 先拷贝一份，我们会给prop_info加内容
	ret_prop_info = copy.copy(prop_info)
	# 属性对应的类型信息
	ret_prop_info['type_info'] = None

	if is_array_type(prop_info):
		if prop_info['array_dim'] > 1:
			inner_prop_info = copy.copy(prop_info)
			inner_prop_info['array_dim'] = 1
		else:  # ArrayProperty
			inner_prop_info = prop_info['inner_prop']
		ret_inner_prop_info = parse_prop_info(inner_prop_info, context)
		ret_prop_info['inner_prop'] = ret_inner_prop_info
		ret_inner_type_info = ret_inner_prop_info['type_info']
		if ret_inner_type_info:
			inner_py_name = ret_inner_type_info.get('name', ret_inner_type_info.get('py_name', 'unsupported'))
			inner_cpp_name = extract_cpp_type(ret_inner_prop_info)
		else:
			inner_py_name = 'unsupported'
			inner_cpp_name = 'unsupported'
		ret_prop_info['type_info'] = {
			'py_name': 'list(%s)' % inner_py_name,
			'cpp_name': 'TArray<%s>' % inner_cpp_name,
		}
		# 特殊处理bytes类型
		if is_bytes_type(ret_prop_info):
			ret_prop_info['type_info']['py_name'] = 'bytes'

	elif prop_type == 'SetProperty':
		ret_element_prop_info = parse_prop_info(prop_info['element_prop'], context)
		ret_prop_info['element_prop'] = ret_element_prop_info
		ret_element_type_info = ret_element_prop_info['type_info']
		if ret_element_type_info:
			element_py_name = ret_element_type_info.get('name', ret_element_type_info.get('py_name', 'unsupported'))
			element_cpp_name = extract_cpp_type(ret_element_prop_info)
		else:
			element_py_name = 'unsupported'
			element_cpp_name = 'unsupported'
		ret_prop_info['type_info'] = {
			'py_name': 'set(%s)' % element_py_name,
			'cpp_name': 'TSet<%s>' % element_cpp_name,
		}

	elif prop_type == 'MapProperty':
		ret_key_prop_info = parse_prop_info(prop_info['key_prop'], context)
		ret_prop_info['key_prop'] = ret_key_prop_info
		ret_key_type_info = ret_key_prop_info['type_info']
		ret_value_prop_info = parse_prop_info(prop_info['value_prop'], context)
		ret_prop_info['value_prop'] = ret_value_prop_info
		ret_value_type_info = ret_value_prop_info['type_info']
		if ret_key_type_info:
			key_py_name = ret_key_type_info.get('name', ret_key_type_info.get('py_name', 'unsupported'))
			key_cpp_name = extract_cpp_type(ret_key_prop_info)
		else:
			key_py_name = 'unsupported'
			key_cpp_name = 'unsupported'
		if ret_value_type_info:
			value_py_name = ret_value_type_info.get('name', ret_value_type_info.get('py_name', 'unsupported'))
			value_cpp_name = extract_cpp_type(ret_value_prop_info)
		else:
			value_py_name = 'unsupported'
			value_cpp_name = 'unsupported'
		ret_prop_info['type_info'] = {
			'py_name': 'dict(%s, %s)' % (key_py_name, value_py_name),
			'cpp_name': 'TMap<%s, %s>' % (key_cpp_name, value_cpp_name),
		}

	elif is_builtin_type(prop_info):
		ret_prop_info['type_info'] = BUILTIN_TYPES[prop_type]
	elif is_enum_type(prop_info):
		if prop_type == 'EnumProperty':
			underlying_type = prop_info['enum_underlying_type']
		else:  # ByteProperty
			underlying_type = prop_type
		ret_prop_info['type_info'] = BUILTIN_TYPES[underlying_type]
	elif is_string_type(prop_info):
		ret_prop_info['type_info'] = STRING_TYPES[prop_type]
	elif is_delegate_type(prop_info):
		type_info = DELEGATE_TYPES[prop_type]
		if 'func_info' in prop_info:  # UE5 UHT MulticastDelegate Bug，等UE5官方修复
			func_info = prop_info['func_info']
			type_info['cpp_name'] = 'F' + func_info['name'].split('_')[0]
			type_info['func_info'] = func_info
			module_relative_path = func_info.get('meta_data', {}).get('ModuleRelativePath', '')  # type: str
			header = module_relative_path.split('/', 1)[-1]  # 一般是 "Public/Animation/AnimCurveTypes.h" 这样的，需要去掉最前面的 Public
			type_info['header'] = header
			ret_prop_info['type_info'] = type_info
	elif prop_type == 'FieldPathProperty':
		ret_prop_info['type_info'] = {
			'py_name': 'FieldPath',
			'cpp_name': 'TFieldPath<F%s>' % prop_info['class_name'],
		}
	elif prop_type == 'StructProperty':
		ret_prop_info['type_info'] = context.current_gen.structs_by_name.get(prop_info['struct_name'])
	elif prop_type == 'ObjectProperty':
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(prop_info['class_name'])
	elif prop_type == 'WeakObjectProperty':
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(prop_info['class_name'])
	elif prop_type == 'InterfaceProperty':
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(prop_info['interface_name'])
	elif prop_type == 'ClassProperty':
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(prop_info['meta_class_name'])
	elif prop_type == 'ClassPtrProperty':
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(prop_info['class_name'])
	elif prop_type == "SoftObjectProperty":
		class_name = prop_info['class_name']
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(class_name)
	elif prop_type == "SoftClassProperty":
		ret_prop_info['type_info'] = context.current_gen.classes_by_name.get(prop_info['meta_class_name'])
	else:
		pass

	return ret_prop_info


def parse_py_default(prop_info):
	if 'default' not in prop_info:
		return
	prop_type = prop_info['type']
	default = prop_info['default']
	py_default = '...'
	if prop_type == 'BoolProperty':
		py_default = 'True' if default == 'true' else 'False'
	elif prop_type in ('ObjectProperty', 'ClassProperty'):
		py_default = default  # default 一定为 None
	elif prop_type in ('NameProperty', 'StrProperty'):
		py_default = '"%s"' % default  # default 好像一定为 None
	elif prop_type == 'StructProperty':
		if default == '':
			py_default = '%s()' % prop_info['struct_name']
		# python 的构造参数不一定有匹配的参数列表，不处理
		# elif default.startswith('('):
		# 	py_default = '%s%s' % (prop_info['struct_name'], default)
		# else:
		# 	py_default = '%s(%s)' % (prop_info['struct_name'], default)
	elif prop_type in ('EnumProperty', 'ByteProperty') and 'enum_name' in prop_info:
		if default.startswith('Type::'):
			default = default[6:]
		if default == 'None':
			default = 'NONE'
		py_default = '%s.%s' % (prop_info['enum_name'], default)
	elif prop_type in ('FloatProperty', 'DoubleProperty'):
		if default.endswith('f') or default.endswith('d'):
			default = default[:-1]
		if is_number(default):
			py_default = default
	elif prop_type in ('IntProperty', 'ByteProperty'):
		if is_number(default):
			py_default = default
	return py_default


# 检查prop_info中type_info是否不为空
def check_prop_type_info(prop_info):
	# type: (dict) -> bool
	prop_type = prop_info['type']
	if is_array_type(prop_info):
		return check_prop_type_info(prop_info['inner_prop'])
	elif prop_type == 'SetProperty':
		return check_prop_type_info(prop_info['element_prop'])
	elif prop_type == 'MapProperty':
		return check_prop_type_info(prop_info['key_prop']) \
			and check_prop_type_info(prop_info['value_prop'])
	return is_export_type(prop_info['type_info'])


# 如果枚举值刚好是Python的关键字，
# 则将它们替换为别的名字
# 注意：需要同时修改 FNePyEnumBase::ReplaceEnumItemName (NePyEnumBase.cpp)
def repl_enum_item_name(item_name):
	# type: (str) -> str
	if item_name == 'None':
		return 'NONE'
	elif item_name == 'Enum':
		return 'ENUM'
	return item_name


# 可与Python内置类型直接转换的UE4类型
BUILTIN_TYPES = {
	'BoolProperty': {
		'py_name': 'bool',
		'cpp_name': 'bool',
		'to_py_expr': 'PyBool_FromLong(%s)',
	},
	'Int8Property': {
		'py_name': 'int',
		'cpp_name': 'int8',
		'to_py_expr': 'PyLong_FromLong(%s)',
	},
	'ByteProperty': {
		'py_name': 'int',
		'cpp_name': 'uint8',
		'to_py_expr': 'PyLong_FromUnsignedLong(%s)',
	},
	'Int16Property': {
		'py_name': 'int',
		'cpp_name': 'int16',
		'to_py_expr': 'PyLong_FromLong(%s)',
	},
	'UInt16Property': {
		'py_name': 'int',
		'cpp_name': 'uint16',
		'to_py_expr': 'PyLong_FromUnsignedLong(%s)',
	},
	'IntProperty': {
		'py_name': 'int',
		'cpp_name': 'int32',
		'to_py_expr': 'PyLong_FromLong(%s)',
	},
	'UInt32Property': {
		'py_name': 'int',
		'cpp_name': 'uint32',
		'to_py_expr': 'PyLong_FromUnsignedLong(%s)',
	},
	'Int64Property': {
		'py_name': 'int',
		'cpp_name': 'int64',
		'to_py_expr': 'PyLong_FromLongLong(%s)',
	},
	'UInt64Property': {
		'py_name': 'int',
		'cpp_name': 'uint64',
		'to_py_expr': 'PyLong_FromUnsignedLongLong(%s)',
	},
	'FloatProperty': {
		'py_name': 'float',
		'cpp_name': 'float',
		'to_py_expr': 'PyFloat_FromDouble(%s)',
	},
	'DoubleProperty': {
		'py_name': 'float',
		'cpp_name': 'double',
		'to_py_expr': 'PyFloat_FromDouble(%s)',
	},
}

# 字符串类型
STRING_TYPES = {
	'StrProperty': {
		'py_name': 'str',
		'cpp_name': 'FString',
	},
	'NameProperty': {
		'py_name': 'Name',
		'cpp_name': 'FName',
	},
	'TextProperty': {
		'py_name': 'Text',
		'cpp_name': 'FText',
	},
}

# Delegate类型
DELEGATE_TYPES = {
	'DelegateProperty': {
		'py_name': 'Callable',
		'py_wrapper': 'DynamicDelegateWrapper',
	},
	'MulticastInlineDelegateProperty': {
		'py_name': 'Callable',
		'py_wrapper': 'DynamicMulticastDelegateWrapper',
	},
	'MulticastSparseDelegateProperty': {
		'py_name': 'Callable',
		'py_wrapper': 'DynamicMulticastDelegateWrapper',
	},
}
