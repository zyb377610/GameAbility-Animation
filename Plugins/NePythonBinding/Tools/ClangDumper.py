# -*- encoding: utf-8 -*-
import os
import time
import json
import UE4Flags
import ExportConfig
import SourceHelpers
import OperatorOverloadHelper
import DebugLog as dlog
from FilterRules import FunctionFilterRules
from clang.cindex import TranslationUnit
from clang.cindex import AccessSpecifier
from clang.cindex import CursorKind
from clang.cindex import TypeKind
from clang.cindex import Cursor
from version_helper import open_and_read_str, open_and_write_str # noqa

try:
	import typing
except:
	pass

# 编译需要定义的一些宏
MACRO_DEFS = {
	'WITH_EDITOR': '0',
	'WITH_ENGINE': '1',
	'WITH_UNREAL_DEVELOPER_TOOLS': '0',
	'WITH_PLUGIN_SUPPORT': '1',
	'IS_MONOLITHIC': '0',
	'IS_PROGRAM': '0',
	'UE_BUILD_DEVELOPMENT': '1',
	'UBT_COMPILED_PLATFORM': 'Windows',
	'PLATFORM_WINDOWS': '1',
	'CORE_API': '',
}

# 另一些需要定义的宏
UHT_MACROS = '''
// override macros that interfere with definitions.
#define DEPRECATED(...)
#define FORCEINLINE(...)

// use variants of macros defined in ObjectMacros.h that are meant to be active during edit.
#define UCLASS(...)
#define UINTERFACE(...)
#define UPROPERTY(...)
#define UFUNCTION(...)
#define USTRUCT(...)
#define UMETA(...)
#define UPARAM(...)
#define UENUM(...)
#define UDELEGATE(...)

#define GENERATED_BODY_LEGACY(...)
#define GENERATED_BODY(...)
#define GENERATED_USTRUCT_BODY(...)
#define GENERATED_UCLASS_BODY(...)
#define GENERATED_UINTERFACE_BODY(...)
#define GENERATED_IINTERFACE_BODY(...)
'''

# 编译时的include路径
INCLUDE_PATHS = [
	os.path.join(ExportConfig.EnginePath, 'Source', 'Runtime', 'Core', 'Public'),
	os.path.join(ExportConfig.EnginePath, 'Source', 'Runtime', 'Core', 'Private'),
]

# clang类型到UE4内置属性的映射
BUILTIN_TYPES = {
	TypeKind.BOOL: 'BoolProperty',
	TypeKind.CHAR_U: 'ByteProperty',
	TypeKind.UCHAR: 'ByteProperty',
	TypeKind.USHORT: 'UInt16Property',
	TypeKind.UINT: 'UInt32Property',
	TypeKind.ULONG: 'UInt32Property',
	TypeKind.ULONGLONG: 'UInt64Property',
	TypeKind.CHAR_S: 'Int8Property',
	TypeKind.SCHAR: 'Int8Property',
	TypeKind.SHORT: 'Int16Property',
	TypeKind.INT: 'IntProperty',
	TypeKind.LONG: 'IntProperty',
	TypeKind.LONGLONG: 'Int64Property',
	TypeKind.FLOAT: 'FloatProperty',
	TypeKind.DOUBLE: 'DoubleProperty',
}

# UE4自定义类型到UE4内置属性的映射
TYPEDEF_TYPES = {
	'uint8': 'ByteProperty',
	'uint16': 'UInt16Property',
	'uint32': 'UInt32Property',
	'uint64': 'UInt64Property',
	'int8': 'Int8Property',
	'int16': 'Int16Property',
	'int32': 'IntProperty',
	'int64': 'Int64Property',
}

class StructContext(object):
	def __init__(self):
		self.name = None # type: str
		self.cpp_name = None # type: str
		self.package_name = None # type: str
		self.flags = 0 # type: int
		self.prop_infos = [] # type: list[dict]
		self.func_infos = [] # type: list[dict]
	
	def dump(self):
		return {
			'name': self.name,
			'cpp_name': self.cpp_name,
			'package': self.package_name,
			'struct_flags': self.flags,
			'props': self.prop_infos,
			'funcs': self.func_infos,
		}


# 解析单个文件
# clang_args: 传给clang的参数
# package_name: 源码所在的UE4包
# file_path: 文件路径
# struct_rules: 需要导出的结构体规则
def parse_one_file(clang_args, package_name, file_path, struct_rules):
	# type: (list[str], str, str, dict) -> dict
	# need to be compatible with py2 and py3
	source_code = open_and_read_str(file_path)
	source_code = SourceHelpers.replace_math_templates(file_path, source_code)
	source_code = SourceHelpers.preprocess_source_for_clang(source_code)
	tu = create_translation_unit(clang_args, file_path, source_code)
	return parse_one_tu(package_name, struct_rules, tu)


# 调用Clang将源码解析成AST
# clang_args: 传给clang的参数
# file_path: 文件路径
# source_code: 源码数据
# 	如果为None，则读取file_path对应的源码
def create_translation_unit(clang_args, file_path, source_code):
	# type: (list[str], str, str) -> TranslationUnit
	# fix: error: invalid argument '-std=c++17' not allowed with 'C'
	if file_path.endswith('.h'):
		file_path = file_path[:-2] + '.cpp'

	source_code = UHT_MACROS + source_code

	unsaved_files = [(file_path, source_code)]
	options = TranslationUnit.PARSE_INCOMPLETE | TranslationUnit.PARSE_SKIP_FUNCTION_BODIES
	tu = TranslationUnit.from_source(file_path, args=clang_args, unsaved_files=unsaved_files, options=options)

	# print('-------------------------------------')
	# for i in tu.diagnostics:
	# 	print(i)

	# print('-------------------------------------')
	# for i in tu.get_includes():
	# 	print(i.include)

	# print('-------------------------------------')
	# walk_and_print(tu.cursor)
	# exit(0)

	return tu


# 解析AST
# package_name: 源码所在的UE4包
# struct_rules: 需要导出的结构体规则
# tu: TranslationUnit，是由Clang解析源码获得的AST
def parse_one_tu(package_name, struct_rules, tu):
	# type: (str, dict, TranslationUnit) -> dict
	struct_infos = []
	# need to be compatible with py2 and py3
	for struct_name, struct_rule in struct_rules.items():
		func_filter = None
		top_module_methods = None
		if isinstance(struct_rule, dict):
			func_rules = struct_rule.get('funcs', {})
			include_func_rule = func_rules.get('include')
			exclude_func_rule = func_rules.get('exclude')
			if include_func_rule is not None or exclude_func_rule is not None:
				func_filter = FunctionFilterRules(include_func_rule, exclude_func_rule)
			top_module_methods = bool(struct_rule.get('top_module_methods'))

		if top_module_methods:
			target_cursor = tu.cursor # type: Cursor
		else:
			target_cursor = find_cursor(tu.cursor, struct_name, cursor_kind=CursorKind.STRUCT_DECL, must_have_children=True)

		if target_cursor is None:
			dlog.debug('[error] cant find target_cursor for struct "%s"' % struct_name)
			continue

		# print('-------------------------------------')
		# walk_and_print(target_cursor)
		# exit(0)

		if top_module_methods:
			struct_info = parse_top_module_methods(package_name, target_cursor, func_filter)
		else:
			struct_info = parse_struct(package_name, target_cursor, func_filter)

		struct_infos.append(struct_info)

	return {
		'structs': struct_infos
	}


# 遍历整个树，直到找到对应的节点
# root_cursor: 根节点
# cursor_name: 要查找的节点名称
# cursor_kind: 要查找的节点类型
# must_have_children: 目标节点是否必须含有子节点
def find_cursor(root_cursor, cursor_name, cursor_kind=None, must_have_children=False):
	# type: (Cursor, str, CursorKind, bool) -> Cursor|None

	def _breadth_first_walk(root_cursor):
		# type: (Cursor) -> typing.Generator[Cursor]
		for cursor in root_cursor.get_children():
			yield cursor
		for cursor in root_cursor.get_children():
			for child_cursor in _breadth_first_walk(cursor):
				yield child_cursor

	def _is_target(cursor):
		# type: (Cursor) -> bool
		if cursor.spelling != cursor_name:
			return False
		if cursor_kind is not None and cursor.kind != cursor_kind:
			return False
		if must_have_children and len(list(cursor.get_children())) == 0:
			return False
		return True

	if _is_target(root_cursor):
		return root_cursor
	
	for cursor in _breadth_first_walk(root_cursor):
		if _is_target(cursor):
			return cursor

	return None


# 查找输入节点的直接子节点，返回对应的节点
# cursor: 父节点
# child_cursor_name: 要查找的子节点名称
# child_cursor_kind: 要查找的子节点类型
def find_child_cursor(cursor, child_cursor_name=None, child_cursor_kind=None):
	# type: (Cursor, str, CursorKind) -> Cursor|None

	def _is_target(child_cursor):
		# type: (Cursor) -> bool
		if child_cursor_name is not None and child_cursor.spelling != child_cursor_name:
			return False
		if child_cursor_kind is not None and child_cursor.kind != child_cursor_kind:
			return False
		return True
	
	for child_cursor in cursor.get_children():
		if _is_target(child_cursor):
			return child_cursor
		
	return None


# 遍历并返回特定的子节点
# child_cursor_kind: 要查找的子节点类型
# recursive: 是否递归遍历子节点的子节点
def iter_child_cursor(cursor, child_cursor_kind=None, recursive=False):
	# type: (Cursor, CursorKind, bool) -> typing.Generator[Cursor]
	for child_cursor in cursor.get_children():
		if child_cursor_kind is None or child_cursor.kind == child_cursor_kind:
			yield child_cursor
		if recursive:
			yield iter_child_cursor(child_cursor, child_cursor_kind, recursive)


# 遍历，并打印整个节点树
def walk_and_print(cursor, curr_depth=0):
	# type: (Cursor, int) -> None
	typing.Generator
	print('%s[%s] %s' % (('    ' * curr_depth), cursor.kind, cursor.spelling))
	for child in cursor.get_children():
		walk_and_print(child, curr_depth + 1)


# 调试用，打印节点信息
def explain(node):
	# type: (Cursor) -> None
	print('-------------------------------------')
	print('~~~~~~~', node.spelling)
	for k in dir(node):
		if k.startswith('_'):
			continue

		try:
			v = getattr(node, k)
			if callable(v):
				try:
					print('%s:%s' % (k, v()))
				except:
					print('%s:%s' % (k, v))
			else:
				print('%s:%s' % (k, v))
		except:
			print('%s: xxxxx' % (k, ))
			pass
	print('-------------------------------------')


# 导出单个结构体信息
def parse_struct(package_name, cursor, func_filter):
	# type: (str, Cursor, typing.Callable) -> dict
	assert cursor.kind == CursorKind.STRUCT_DECL

	dlog.debug('struct:', cursor.spelling)
	dlog.indent()

	context = StructContext()
	context.name = cursor.spelling[1:] # skip F
	context.cpp_name = cursor.spelling
	context.package_name = package_name
	context.flags = UE4Flags.STRUCT_Native

	for child_cursor in cursor.get_children():
		if child_cursor.access_specifier != AccessSpecifier.PUBLIC:
			# 只导出public的成员
			continue

		if child_cursor.kind == CursorKind.UNION_DECL:
			# union { struct { float x, float y, float z}; float xyz[3] };
			for grand_child_cursor in child_cursor.get_children():
				if grand_child_cursor.kind == CursorKind.STRUCT_DECL:
					for grand_grand_child_cursor in grand_child_cursor.get_children():
						if grand_grand_child_cursor.kind == CursorKind.FIELD_DECL:
							dlog.debug('field:', grand_grand_child_cursor.spelling)
							dlog.indent()
							parse_field(grand_grand_child_cursor, context)
							dlog.unindent()
			continue

		if child_cursor.kind == CursorKind.FIELD_DECL:
			dlog.debug('field:', child_cursor.spelling)
			dlog.indent()
			parse_field(child_cursor, context)
			dlog.unindent()
			continue

		if child_cursor.kind == CursorKind.CONSTRUCTOR:
			dlog.debug('constructor:', child_cursor.displayname)
			dlog.indent()
			parse_constructor(child_cursor, context, func_filter)
			dlog.unindent()
			continue

		if child_cursor.kind == CursorKind.CXX_METHOD:
			dlog.debug('method:', child_cursor.displayname)
			dlog.indent()
			parse_method(child_cursor, context, func_filter)
			dlog.unindent()
			continue

		if child_cursor.kind == CursorKind.FRIEND_DECL:
			for grand_child_cursor in child_cursor.get_children():
				if grand_child_cursor.kind == CursorKind.FUNCTION_DECL:
					parse_friend_func(grand_child_cursor, context, func_filter)
				break
			continue

	dlog.unindent()

	return context.dump()

def parse_top_module_methods(package_name, cursor, func_filter):
	# type: (str, Cursor, typing.Callable) -> dict
	context = StructContext()
	context.name = 'TopModule'
	context.cpp_name = 'TopModule'
	context.package_name = package_name

	for child_cursor in cursor.get_children():
		if child_cursor.kind == CursorKind.FUNCTION_DECL:
			dlog.debug('method:', child_cursor.displayname)
			dlog.indent()
			parse_method(child_cursor, context, func_filter)
			dlog.unindent()

	return context.dump()


# 解析成员变量
def parse_field(cursor, context):
	# type: (Cursor, StructContext) -> None
	prop_type_info = get_prop_type_info(cursor)
	if not prop_type_info:
		return

	dlog.debug(prop_type_info)

	prop_info = {
		'name': cursor.spelling,
		'pretty_name': cursor.spelling,
		'prop_flags': UE4Flags.CPF_NativeAccessSpecifierPublic,
		'array_dim': 1,
	}

	prop_info.update(prop_type_info)

	context.prop_infos.append(prop_info)


# 解析构造函数
def parse_constructor(cursor, context, func_filter):
	# type: (Cursor, StructContext, typing.Callable) -> None
	if not isinstance(context, StructContext):
		# 只有UStruct才能在Python中构造创建
		# 因此只有UStruct需要解析构造函数
		dlog.debug('skip non-struct constructor:', cursor.displayname)
		return

	# Python无法区分int和enum，因此无法使用这种类型的构造函数
	skip_enums = ('EForceInit', 'ENoInit')
	for skip_enum in skip_enums:
		if skip_enum in cursor.displayname:
			dlog.debug('skip %s constructor: %s' % (skip_enum, cursor.displayname))
			return

	parse_method(cursor, context, func_filter)


# 解析友元方法
def parse_friend_func(cursor, context, func_filter):
	# type: (Cursor, StructContext, typing.Callable) -> None
	# 第一个参数类型与自身相同，则能被导出
	is_friend_func = False
	for child_cursor in iter_child_cursor(cursor, CursorKind.PARM_DECL):
		param_info = get_prop_type_info(child_cursor)
		if param_info:
			if param_info['type'] == 'StructProperty':
				if param_info['struct_name'] == context.name:
					is_friend_func = True
		break

	if is_friend_func:
		dlog.debug('friend func:', cursor.displayname)
		dlog.indent()
		parse_method(cursor, context, func_filter, True)
		dlog.unindent()


# 解析成员函数
def parse_method(cursor, context, func_filter, is_friend_func=False):
	# type: (Cursor, StructContext, typing.Callable, bool) -> None
	if func_filter:
		if not func_filter.should_export(cursor.displayname):
			dlog.debug('skip by func filter:', cursor.displayname)
			return
		if not func_filter.should_export(cursor.spelling):
			dlog.debug('skip by func filter:', cursor.spelling)
			return

	func_name = cursor.spelling
	pretty_func_name = cursor.spelling
	required_arg_count = -1

	if func_name.startswith('operator'):
		operator_info = OperatorOverloadHelper.OPERATOR_BY_CPP_NAME.get(func_name)
		if not operator_info:
			dlog.debug('[error] unsupported operator overload:', func_name)
			return
		pretty_func_name = operator_info['py_name']
		required_arg_count = operator_info['arg_count']

	param_infos = []
	for arg_idx, child_cursor in enumerate(iter_child_cursor(cursor, CursorKind.PARM_DECL)):
		param_info = get_prop_type_info(child_cursor)
		if not param_info:
			return

		if is_friend_func and arg_idx == 0:
			# 忽略友元方法的第一个参数，FuncBuilder中会将其替换为InSelf->Value
			continue

		is_const_qualified = False
		is_reference = False
		is_pointer = False
		has_default_value = False
		default_value = ''
		for token in child_cursor.get_tokens():
			if has_default_value:
				default_value += token.spelling
			if token.spelling == 'const':
				is_const_qualified = True
			elif token.spelling == '&':
				is_reference = True
			elif token.spelling == '*':
				is_pointer = True
			elif token.spelling == '=':
				has_default_value = True
			
		prop_flags = UE4Flags.CPF_Parm
		if is_reference:
			prop_flags |= UE4Flags.CPF_OutParm
		if is_const_qualified:
			prop_flags |= UE4Flags.CPF_ConstParm
		if is_reference and is_const_qualified:
			prop_flags |= UE4Flags.CPF_ReferenceParm

		param_name = child_cursor.spelling
		if not param_name:
			param_name = 'InAnonymous' + str(arg_idx)

		param_info.update({
			'name': param_name,
			'pretty_name': param_name,
			'prop_flags': prop_flags,
			'array_dim': 1,
		})
		if is_pointer:
			param_info['is_pointer'] = True
		if has_default_value:
			param_info['default'] = default_value
		param_infos.append(param_info)
		dlog.debug('arg%d: %s' % (arg_idx, param_info))

	if required_arg_count != -1 and len(param_infos) != required_arg_count:
		dlog.debug('[error] %s should have exactly %d args, now is %d' % (func_name, required_arg_count, len(param_infos)))
		return

	if cursor.result_type.kind != TypeKind.VOID:
		ret_info = get_prop_type_info(cursor)
		if not ret_info:
			return

		prop_flags = UE4Flags.CPF_Parm | UE4Flags.CPF_OutParm | UE4Flags.CPF_ReturnParm
		ret_info.update({
			'name': 'ReturnValue',
			'pretty_name': 'ReturnValue',
			'prop_flags': prop_flags,
			'array_dim': 1,
		})

		param_infos.append(ret_info)
		dlog.debug('ret:', ret_info)
	
	func_flags = UE4Flags.FUNC_Native | UE4Flags.FUNC_Public
	if cursor.is_static_method():
		func_flags |= UE4Flags.FUNC_Static

	func_info = {
		'name': func_name,
		'pretty_name': pretty_func_name,
		'func_flags': func_flags,
		'params': param_infos,
		'is_friend_func': is_friend_func,
	}
	context.func_infos.append(func_info)


# 获取节点对应的UE4属性类型
# 如果输入节点是CXX_METHOD，获取的是返回值类型
def get_prop_type_info(cursor):
	# type: (Cursor) -> dict
	if cursor.kind in (CursorKind.CXX_METHOD, CursorKind.FUNCTION_DECL):
		typ = cursor.result_type
	else:
		typ = cursor.type
	type_kind = typ.kind

	# if type_kind == TypeKind.POINTER:
	# 	if typ.spelling == 'const TCHAR *':
	# 		return {
	# 			'type': 'StrProperty',
	# 		}

	# 首先把typedef还原成真正的类型
	if type_kind == TypeKind.TYPEDEF:
		typedef_prop_type = TYPEDEF_TYPES.get(typ.get_typedef_name())
		if typedef_prop_type:
			return {
				'type': typedef_prop_type,
			}

	# 内置类型
	builtin_prop_type = BUILTIN_TYPES.get(type_kind)
	if builtin_prop_type:
		return {
			'type': builtin_prop_type,
		}

	# 枚举类型
	if type_kind == TypeKind.ENUM:
		if typ.get_size() == 1:
			enum_underlying_type = 'ByteProperty'
		elif typ.get_size() == 2:
			enum_underlying_type = 'Int16Property'
		else:
			enum_underlying_type = 'IntProperty'
		return {
			'type': 'EnumProperty',
			'enum_name': typ.spelling,
			'enum_cpp_name': typ.spelling,
			'enum_cpp_form': UE4Flags.ECF_Regular, # todo: 想办法解析成真正的类型
			'enum_underlying_type': enum_underlying_type,
		}

	type_ref_cursor = find_child_cursor(cursor, child_cursor_kind=CursorKind.TYPE_REF)
	if type_ref_cursor:
		type_ref_infos = type_ref_cursor.spelling.split(' ')

		# 结构体类型
		if type_ref_infos[0] == 'struct':
			struct_name = type_ref_infos[1]
			struct_name = struct_name.rpartition(':')[-1] # skip UE::Math::
			struct_name = struct_name[1:] # skip F
			return {
				'type': 'StructProperty',
				'struct_name': struct_name,
			}
		
		# 类类型
		if type_ref_infos[0] == 'class':
			class_name = type_ref_infos[1]
			if class_name == 'FString':
				return {
					'type': 'StrProperty',
				}
			elif class_name == 'FName':
				return {
					'type': 'NameProperty',
				}
			elif class_name == 'FText':
				return {
					'type': 'TextProperty',
				}
			elif class_name == 'UClass':
				return {
					'type': 'ClassProperty',
					'class_name': 'Class',
					'meta_class_name': 'Class',
				}
			elif class_name[0] in ('U', 'A'):
				return {
					'type': 'ObjectProperty',
					'class_name': class_name[1:], # skip U or A
				}

	dlog.debug('[error] unknown prop type:', type_kind, typ.spelling)
	return None


def main():
	from clang.cindex import Config
	Config.set_library_path('clang/native/')

	clang_infos = {
		'structs': [],
	}

	package_name_to_source_dir_dict = SourceHelpers.get_package_name_to_source_dir_dict()

	# 对规则进行预处理和检查
	package_rules = []

	# need to be compatible with py2 and py3
	for package_name, _source_rules in ExportConfig.ClangParsePackages.items():
		source_dir = package_name_to_source_dir_dict.get(package_name)
		if not source_dir:
			dlog('[error] package "%s" either not defined in ExportConfig.ExportModules, or dont has "source_dir" filed.' % package_name)
			continue

		source_rules = []
		for source_rule in _source_rules:
			file_path = source_rule['header']
			if not file_path.endswith('.h'):
				dlog('[error] "%s" is not a header file!' % file_path)
				continue
			source_rules.append(source_rule)
	
		package_rules.append((package_name, source_rules))


	clang_args = []
	# c++标准
	clang_args.append('-std=c++17')
	# 忽略所有warning
	clang_args.append('-Wno-everything')

	# 头文件
	for include_path in INCLUDE_PATHS:
		clang_args.append('-I' + include_path)

	# 加上每个包的include目录
	# need to be compatible with py2 and py3
	for source_dir in package_name_to_source_dir_dict.values():
		clang_args.append('-I' + os.path.join(source_dir, 'Public'))
		clang_args.append('-I' + os.path.join(source_dir, 'Private'))
		clang_args.append('-I' + os.path.join(source_dir, 'Classes'))

	# 各种UBT内置宏
	# need to be compatible with py2 and py3
	for macro_def in MACRO_DEFS.items():
		clang_args.append('-D%s=%s' % macro_def)

	# 根据包名，自动推断并加上XXX_API这种宏
	# 例如将/Script/CoreUObject转化为COREUOBJECT_API
	for package_name in package_name_to_source_dir_dict:
		slash_index = package_name.rfind('/')
		api_macro = package_name[slash_index + 1:]
		api_macro = api_macro.upper() + '_API'
		clang_args.append('-D%s=' % api_macro)

	temp_dir = os.path.abspath('temp')
	if not os.path.isdir('temp'):
		os.path.mkdirs(temp_dir)

	# 一次解析一个文件
	for package_name, source_rules in package_rules:
		source_dir = package_name_to_source_dir_dict[package_name]
		for source_rule in source_rules:
			file_path = source_rule['header']
			struct_rules = source_rule.get('structs')
			if struct_rules:
				result = parse_one_file(clang_args, package_name, file_path, struct_rules)
				struct_infos = result.get('structs', [])
				for struct_info in struct_infos:
					struct_info['header'] = SourceHelpers.get_relative_header_path(source_rule['header'], source_dir)
				clang_infos['structs'].extend(struct_infos)

	# UE5我们为了把数学库的模板还原回非模板，做了很多黑科技，导致用不了UnityBuild了
	# # 使用UnityBuild，将文件整合成一个一起解析，加快解析速度
	# for package_name, source_rules in package_rules:
	# 	source_dir = package_name_to_source_dir_dict[package_name]

	# 	unity_build_temp_files = []
	# 	unity_build_source = []
	# 	for source_idx, source_rule in enumerate(source_rules):
	# 		file_path = source_rule['header'].replace('\\', '/')
	# 		temp_file_path = os.path.join(temp_dir, 'unity_build_source_%s.h' % source_idx)
	# 		unity_build_temp_files.append(temp_file_path)

	# 		# need to be compatible with py2 and py3
	# 		source_code = open_and_read_str(file_path)
	# 		source_code = SourceHelpers.replace_math_templates(file_path, source_code)
	# 		source_code = SourceHelpers.preprocess_source_for_clang(source_code)

	# 		# need to be compatible with py2 and py3
	# 		open_and_write_str(temp_file_path, source_code)
	# 		temp_file_path = temp_file_path.replace('\\', '/')
	# 		unity_build_source.append('#include "%s"' % temp_file_path)
	# 	unity_build_source = '\n'.join(unity_build_source)

	# 	tu = create_translation_unit(clang_args, 'unity.cpp', unity_build_source)
	# 	for source_rule in source_rules:
	# 		struct_rules = source_rule.get('structs')
	# 		result = parse_one_tu(package_name, struct_rules, tu)
	# 		struct_infos = result.get('structs', [])
	# 		for struct_info in struct_infos:
	# 			struct_info['header'] = SourceHelpers.get_relative_header_path(source_rule['header'], source_dir)
	# 		clang_infos['structs'].extend(struct_infos)

	clang_infos_file_path = os.path.join(temp_dir, 'clang_infos.json')
	with open(clang_infos_file_path, 'w') as f:
		json.dump(clang_infos, f, indent=4, sort_keys=True)


if __name__ == '__main__':
	dlog.set_log_level(dlog.LOG_LEVEL_DEBUG)
	begin = time.time()
	main()
	print('cost time: %.2f' % (time.time() - begin))
