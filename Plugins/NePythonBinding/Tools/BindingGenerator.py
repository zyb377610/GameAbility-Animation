# -*- encoding: utf-8 -*-
import sys
import os
import re
import json
import codecs
import time
import shutil
import itertools
import argparse
import ExportConfig
import UE4Flags
import DebugLog as dlog
import ExportConfig
from GeneratorContext import GeneratorContext, ExportContext
from DocGenerator import DocGenerator
from BlueprintDocGenerator import BlueprintDocGenerator
from FilterRules import FilterRules
from PropBuilder import PropBuilder
from FuncBuilderBase import FuncBuilderBase
from FuncBuilder import FuncBuilder
from OverloadFuncBuilder import OverloadFuncBuilder
import PropHelpers
import TemplateHelper
from version_helper import open_and_read_str


# 拥有这些标志位的属性不导出
NO_EXPORT_PROP_FLAGS = UE4Flags.CPF_NativeAccessSpecifierPrivate \
	| UE4Flags.CPF_NativeAccessSpecifierProtected

# 拥有这些标志位的函数不导出
NO_EXPORT_FUNC_FLAGS = UE4Flags.FUNC_NetServer \
	| UE4Flags.FUNC_NetRequest \
	| UE4Flags.FUNC_NetResponse \
	| UE4Flags.FUNC_Protected \
	| UE4Flags.FUNC_Private \
	| UE4Flags.FUNC_Delegate


# 解析反射信息
def load_reflection_infos(context, reflection_infos_file_path, full_reflection_infos_file_path):
	# type: (GeneratorContext, str, str) -> None
	try:
		reflection_infos = json.load(codecs.open(reflection_infos_file_path, 'r', 'utf-8-sig'))
	except Exception:
		reflection_infos = json.load(codecs.open(reflection_infos_file_path, 'r', 'utf-8'))
	try:
		full_reflection_infos = json.load(codecs.open(full_reflection_infos_file_path, 'r', 'utf-8-sig'))
	except Exception:
		full_reflection_infos = json.load(codecs.open(full_reflection_infos_file_path, 'r', 'utf-8'))

	# 记录需要静态导出的类型名称
	export_class_names = set(ExportConfig.ForceExportClasses)
	export_struct_names = set(ExportConfig.ForceExportStructs)
	export_enum_names = set(ExportConfig.ForceExportEnums)
	for class_info in reflection_infos.get('classes', []):
		export_class_names.add(class_info['name'])
	for struct_info in reflection_infos.get('structs', []):
		export_struct_names.add(struct_info['name'])
	for enum_info in reflection_infos.get('enums', []):
		export_enum_names.add(enum_info['name'])

	# 将类型信息填充到context.[type]_by_name中
	# 其中需要被静态导出的类型信息会填充到context.[type]_by_package

	global_names = set() # 用于检测命名冲突

	full_class_infos = full_reflection_infos.get('classes', [])
	for class_info in full_class_infos:
		class_name = class_info['name']
		if class_info['class_flags'] & UE4Flags.CLASS_Interface:
			if class_info['doc'] == 'I' + class_name:
				continue
		assert class_name not in context.classes_by_name
		assert class_name not in global_names
		global_names.add(class_name)
		context.classes_by_name[class_name] = class_info
		if class_name in export_class_names:
			package_name = class_info['package']
			if package_name in context.classes_by_package:
				context.classes_by_package[package_name].append(class_info)
			else:
				context.classes_by_package[package_name] = [class_info, ]

	full_struct_infos = full_reflection_infos.get('structs', [])
	for struct_info in full_struct_infos:
		struct_name = struct_info['name']
		assert struct_name not in context.structs_by_name
		assert struct_name not in global_names
		global_names.add(struct_name)
		context.structs_by_name[struct_name] = struct_info
		if struct_name in export_struct_names:
			package_name = struct_info['package']
			if package_name in context.structs_by_package:
				context.structs_by_package[package_name].append(struct_info)
			else:
				context.structs_by_package[package_name] = [struct_info, ]

	full_enum_infos = full_reflection_infos.get('enums', [])
	for enum_info in full_enum_infos:
		enum_name = enum_info['name']
		assert enum_name not in context.enums_by_name
		assert enum_name not in global_names
		global_names.add(enum_name)
		context.enums_by_name[enum_name] = enum_info
		if enum_name in export_enum_names:
			package_name = enum_info['package']
			if package_name in context.enums_by_package:
				context.enums_by_package[package_name].append(enum_info)
			else:
				context.enums_by_package[package_name] = [enum_info, ]

	# 注入手写类的类型信息
	for type_name, manual_type_info in ExportConfig.ManualTypeInfos.items():
		package_name = manual_type_info['package']
		cpp_name = manual_type_info['cpp_name'] # type: str
		if cpp_name.startswith('U'):
			if type_name in context.classes_by_name:
				type_info = context.classes_by_name[type_name]
			else:
				type_info = {
					'class_flags': 272629888,
				}
				context.classes_by_name[type_name] = type_info
				context.classes_by_package.setdefault(package_name, []).append(type_info)
		else:
			if type_name in context.structs_by_name:
				type_info = context.structs_by_name[type_name]
			else:
				type_info = {
					'struct_flags': UE4Flags.STRUCT_Native | UE4Flags.STRUCT_NoExport,
				}
				context.structs_by_name[type_name] = type_info
				context.structs_by_package.setdefault(package_name, []).append(type_info)

		type_info['name'] = type_name
		type_info['pretty_name'] = type_name
		type_info['cpp_name'] = cpp_name
		type_info['package'] = package_name
		type_info['header'] = manual_type_info['header']
		type_info['doc'] = manual_type_info.get('doc', type_info.get('doc', ''))
		type_info['props'] = type_info.get('props', [])
		type_info['funcs'] = type_info.get('funcs', [])


# 解析人工标注UHT头的struct信息
def load_noexport_type_infos(context, noexport_reflection_infos_file_path):
	# type: (GeneratorContext, str) -> None
	try:
		noexport_reflection_infos = json.load(codecs.open(noexport_reflection_infos_file_path, 'r', 'utf-8-sig'))
	except Exception:
		noexport_reflection_infos = json.load(codecs.open(noexport_reflection_infos_file_path, 'r', 'utf-8'))
	for noexport_struct_info in noexport_reflection_infos.get('noexport', []):
		struct_name = noexport_struct_info['name']
		if struct_name in context.classes_by_name:
			class_info = context.classes_by_name[struct_name]
			assert noexport_struct_info['package'] == class_info['package']
			prop_infos = noexport_struct_info.get('props')
			if prop_infos:
				class_info['props'] += prop_infos
			func_infos = noexport_struct_info.get('funcs')
			if func_infos:
				class_info['funcs'] += func_infos
		elif struct_name in context.structs_by_name:
			struct_info = context.structs_by_name[struct_name]
			assert noexport_struct_info['package'] == struct_info['package']
			prop_infos = noexport_struct_info.get('props')
			if prop_infos:
				struct_info['props'] = prop_infos
			func_infos = noexport_struct_info.get('funcs')
			if func_infos:
				struct_info['funcs'] = func_infos
		else:
			# 反射信息里没有此类型，则其无法进行注册，需要修正其flag
			noexport_struct_info['struct_flags'] = noexport_struct_info.get('struct_flags', UE4Flags.STRUCT_Native) | UE4Flags.STRUCT_NoExport
			context.structs_by_name[struct_name] = noexport_struct_info
			package_name = noexport_struct_info['package']
			if package_name in context.structs_by_package:
				context.structs_by_package[package_name].append(noexport_struct_info)
			else:
				context.structs_by_package[package_name] = [noexport_struct_info, ]


# 解析源码信息
def load_source_infos(context, source_infos_file_path):
	# type: (GeneratorContext, str) -> None
	with open(source_infos_file_path, 'r') as f:
		source_infos = json.load(f)

	# need to be compatible with py2 and py3
	for class_name, class_info in context.classes_by_name.items():
		package = class_info['package']
		cpp_info = source_infos.get(package, {}).get('classes', {}).get(class_name)
		if cpp_info:
			class_info['cpp_name'] = cpp_info['cpp_name']
			class_info['header'] = cpp_info['header']

	# need to be compatible with py2 and py3
	for struct_name, struct_info in context.structs_by_name.items():
		package = struct_info['package']
		cpp_info = source_infos.get(package, {}).get('structs', {}).get(struct_name)
		if cpp_info:
			struct_info['cpp_name'] = cpp_info['cpp_name']
			struct_info['header'] = cpp_info['header']

	# need to be compatible with py2 and py3
	for enum_name, enum_info in context.enums_by_name.items():
		package = enum_info['package']
		cpp_info = source_infos.get(package, {}).get('enums', {}).get(enum_name)
		if cpp_info:
			# enum_info['cpp_name'] = cpp_info['cpp_name']
			enum_info['header'] = cpp_info['header']


# 解析clang输出的文件信息
def load_clang_infos(context, clang_infos_file_path):
	# type: (GeneratorContext, str) -> None
	if not os.path.isfile(clang_infos_file_path):
		return

	with open(clang_infos_file_path, 'r') as f:
		clang_infos = json.load(f)

	for clang_struct_info in clang_infos.get('structs', []):
		struct_name = clang_struct_info['name']
		if struct_name not in context.structs_by_name: # 反射信息里没有此Struct，纯clang解析
			# 反射里没有此struct，则其无法进行注册，需要修正其flag
			clang_struct_info['struct_flags'] = clang_struct_info.get('struct_flags', UE4Flags.STRUCT_Native) | UE4Flags.STRUCT_NoExport
			context.structs_by_name[struct_name] = clang_struct_info
			package_name = clang_struct_info['package']
			if package_name in context.structs_by_package:
				context.structs_by_package[package_name].append(clang_struct_info)
			else:
				context.structs_by_package[package_name] = [clang_struct_info, ]
		else:
			struct_info = context.structs_by_name[struct_name]
			assert clang_struct_info['package'] == struct_info['package']
			prop_infos = clang_struct_info.get('props')
			if prop_infos:
				struct_info['props'] = prop_infos
			func_infos = clang_struct_info.get('funcs')
			if func_infos:
				struct_info['funcs'] = func_infos


# 清理代码生成输出目录
def cleanup_generation_paths():
	all_gen_paths = set()

	for module_rule in ExportConfig.ExportModules:
		all_gen_paths.add(module_rule['gen_path'])

	for gen_path in all_gen_paths:
		if os.path.isdir(gen_path):
			shutil.rmtree(gen_path, ignore_errors=True)
		os.makedirs(gen_path)


# 是否为Game模块
def check_is_game_module(module_rule):
	# type: (dict) -> bool
	a = os.path.normpath(os.path.abspath(module_rule['gen_path']))
	b = os.path.normpath(os.path.abspath(ExportConfig.GameBindingGeneratePath))
	return a == b


#
def get_module_api_name(is_game_module):
	# type: (bool) -> str
	return '' if is_game_module else ('%s ' % ExportConfig.PluginModuleAPIName)


# 处理属性导出Flags
def get_prop_filter_flags(module_rule):
	# type: (dict) -> int
	flags = NO_EXPORT_PROP_FLAGS
	if not module_rule.get('with_editor_props'):
		flags |= UE4Flags.CPF_EditorOnly
	return flags


# 处理函数导出Flags
def get_func_filter_flags(module_rule):
	# type: (dict) -> int
	flags = NO_EXPORT_FUNC_FLAGS
	if not module_rule.get('with_editor_funcs'):
		flags |= UE4Flags.FUNC_EditorOnly
	return flags


# 处理WITH_EDITOR宏
def get_with_editor_macros(module_rule):
	# type: (dict) -> tuple[str, str]
	if module_rule.get('with_editor', False):
		return '#if WITH_EDITOR', '#endif // WITH_EDITOR'
	return '__REMOVE_THIS_LINE__', '__REMOVE_THIS_LINE__'


# 导出所有模块
def export_modules(context):
	# type: (ExportContext) -> None
	engine_export_modules_list = []
	game_export_modules_list = []

	if not context.has_gen_difference():
		dlog.debug('no difference, skip export')
		context.current_gen_guid = context.last_gen_guid
		return

	import uuid
	context.current_gen_guid = str(uuid.uuid4())

	# 然后才是真正生成代码
	for module_rule in ExportConfig.ExportModules:
		header_name, init_func_name = export_module(context, module_rule)
		if check_is_game_module(module_rule):
			game_export_modules_list.append((header_name, init_func_name))
		else:
			engine_export_modules_list.append((header_name, init_func_name))
	header_name, init_func_name = export_uproperty_classes()
	engine_export_modules_list.append((header_name, init_func_name))

	builtin_code = export_builtin_script()

	# NePyAutoExport.h
	for export_modules_list in (engine_export_modules_list, game_export_modules_list):
		if len(export_modules_list) == 0:
			continue

		child_include_list = []
		child_init_list = []
		for header_name, init_func_name in export_modules_list:
			child_include_list.append('#include "%s"' % header_name)
			child_init_list.append('%s(PyModule);' % init_func_name)
		substitute_dict = {
			'child_include_list': '\n'.join(child_include_list),
			'export_macro_list': ' \\\n'.join(['#define NePyAutoExport'] + child_init_list),
			'has_builtin_script': False,
		}

		if export_modules_list is engine_export_modules_list:
			template_name = 'NePyAutoExport.h'
			header_name = template_name
			gen_path = ExportConfig.EngineBindingGeneratePath
			substitute_dict['has_builtin_script'] = True
			substitute_dict['builtin_script'] = builtin_code
		else:
			template_name = 'NePyAutoExport.h'
			header_name = 'NePyAutoExport_Game.h'
			gen_path = ExportConfig.GameBindingGeneratePath
		TemplateHelper.build_template('templates/' + template_name, os.path.join(gen_path, header_name), substitute_dict)

	# NePyAutoExportVersion.h
	substitute_dict = {
		'root_module_name': ExportConfig.RootModuleName,
		'guid': context.current_gen_guid,
	}
	TemplateHelper.build_template('templates/NePyAutoExportVersion.h', os.path.join(ExportConfig.EngineBindingGeneratePath, 'NePyAutoExportVersion.h'), substitute_dict)


# 预处理一遍Package下的所有类、结构体、枚举
# 收集信息供后续代码生成使用
def preprocess_package_for_module(context, module_rule, package_rule):
	# type: (ExportContext, dict, dict) -> None
	gen_path = module_rule['gen_path']
	gen_path = os.path.normpath(gen_path).replace('\\', '/')
	module_rule['gen_path'] = gen_path

	py_module_name = module_rule['module']
	package_name = package_rule['name']
	context.curr_package_name = package_name
	context.curr_package_filter_rules = FilterRules(package_rule.get('exclude', {}))
	context.curr_py_module_name = py_module_name

	dlog.debug('[pre-process] package:', package_name)
	dlog.indent()

	# 自动生成代码的include路径
	auto_header_prefix = '%s/%s/' % (gen_path[gen_path.rfind('NePy'):], py_module_name)
	# 手写代码搜索路径
	manual_code_dir = gen_path[:gen_path.rfind('Auto')] + 'Manual'

	enum_infos = context.current_gen.enums_by_package.get(package_name, [])
	for enum_info in enum_infos:
		enum_info['type'] = 'enum'

	struct_infos = context.current_gen.structs_by_package.get(package_name, [])
	for struct_info in struct_infos:
		struct_name = struct_info['name']
		if not context.curr_package_filter_rules.should_export_struct(struct_name):
			dlog.debug('!exclude struct by filter:', struct_name)
			continue
		if 'cpp_name' not in struct_info:
			dlog.debug('!exclude struct no cpp name:', struct_name)
			continue
		assert 'py_module_name' not in struct_info, 'struct "%s" already exported in py module "%s"!' % (struct_name, py_module_name)
		struct_info['type'] = 'struct'
		# 结构体实例
		struct_info['py_module_name'] = py_module_name
		struct_info['py_name'] = 'FNePyStruct_' + struct_name
		struct_info['py_new_func_name'] = 'NePyStructNew_' + struct_name
		struct_info['py_check_func_name'] = 'NePyStructCheck_' + struct_name
		struct_info['py_get_type_func_name'] = 'NePyStructGetType_' + struct_name
		struct_info['py_header_name'] = 'NePyStruct_%s.h' % struct_name
		struct_info['py_source_name'] = 'NePyStruct_%s.cpp' % struct_name
		struct_info['py_header_path'] = auto_header_prefix + struct_info['py_header_name']
		# 手写代码
		manual_code_path = '%s/NePyStruct_%s.inl' % (manual_code_dir, struct_name)
		if preprocess_manual_code_info(manual_code_path, struct_info):
			struct_info['manual_code_path'] = manual_code_path
			struct_info['manual_header_path'] = 'NePy/Manual/NePyStruct_%s.inl' % struct_name
		# 手写类型信息覆写
		if struct_name in ExportConfig.ManualTypeInfos:
			struct_info.update(ExportConfig.ManualTypeInfos[struct_name].get('extra_overrides', {}))

	class_infos = context.current_gen.classes_by_package.get(package_name, [])
	for class_info in class_infos:
		class_name = class_info['name']
		if not context.curr_package_filter_rules.should_export_class(class_name):
			dlog.debug('!exclude class by filter:', class_name)
			continue
		if 'cpp_name' not in class_info:
			dlog.debug('!exclude class no cpp name:', class_name)
			continue
		assert 'py_module_name' not in class_info, 'class "%s" already exported in py module "%s"!' % (class_name, py_module_name)
		class_info['type'] = 'class'
		# 类实例
		class_info['py_module_name'] = py_module_name
		class_info['py_name'] = 'FNePyObject_' + class_name
		class_info['py_new_func_name'] = 'NePyObjectNew_' + class_name
		class_info['py_dealloc_func_name'] = 'NePyObjectDealloc_' + class_name
		class_info['py_check_func_name'] = 'NePyObjectCheck_' + class_name
		class_info['py_get_type_func_name'] = 'NePyObjectGetType_' + class_name
		class_info['py_header_name'] = 'NePyObject_%s.h' % class_name
		class_info['py_source_name'] = 'NePyObject_%s.cpp' % class_name
		class_info['py_header_path'] = auto_header_prefix + class_info['py_header_name']
		class_info['has_pydict'] = context.curr_package_filter_rules.should_add_pydict_for_class(class_name)
		# 手写代码
		manual_code_path = '%s/NePyObject_%s.inl' % (manual_code_dir, class_name)
		if preprocess_manual_code_info(manual_code_path, class_info):
			class_info['manual_code_path'] = manual_code_path
			class_info['manual_header_path'] = 'NePy/Manual/NePyObject_%s.inl' % class_name
		# 手写类型信息覆写
		if class_name in ExportConfig.ManualTypeInfos:
			class_info.update(ExportConfig.ManualTypeInfos[class_name].get('extra_overrides', {}))

	dlog.unindent()


# 收集手写代码信息
def preprocess_manual_code_info(manual_code_path, type_info):
	# type: (str, dict) -> bool
	if not os.path.isfile(manual_code_path):
		return False

	has_manual_export = False
	# need to be compatible with py2 and py3
	data = open_and_read_str(manual_code_path)
	if data:
		if data.find('NePyManualExportFuncs') > 0:
			type_info['has_manual_export_funcs'] = True
			has_manual_export = True

	return has_manual_export


# 处理noexport类代码include路径
def fix_no_export_types_include_path(context):
	# type: (GeneratorContext) -> None
	pat = re.compile(r'The full C\+\+ class is located here:.*/Public/(.+\.h)')
	for struct_info in context.structs_by_name.values():
		if struct_info.get('header') == 'UObject/NoExportTypes.h':
			doc = struct_info.get('doc')
			if doc:
				result = pat.search(doc)
				if result:
					struct_info['header'] = result.group(1).strip()
					# FIX: 处理Timecode注释里的大小写错误问题
					if struct_info['header'].endswith('TimeCode.h'):
						struct_info['header'] = struct_info['header'].replace('TimeCode.h', 'Timecode.h')
	for class_info in context.classes_by_name.values():
		if class_info.get('header') == 'UObject/NoExportTypes.h':
			doc = class_info.get('doc')
			if doc:
				result = pat.search(doc)
				if result:
					class_info['header'] = result.group(1).strip()


# 为结构体添加Make/Break方法
def add_make_break_func_to_struct(context):
	# type: (GeneratorContext) -> None

	def find_external_func(func_meta):
		# type: (str) -> tuple
		# func_meta形如：/Script/Engine.KismetMathLibrary.MakeVector
		func_meta = func_meta.replace(":", ".")  # 兼容/Script/WwiseResourceLoader.WwiseObjectInfoLibrary:MakeStruct的情况
		metas = func_meta.split('.')
		if len(metas) < 3:
			return None, None
		func_name = metas[-1]
		class_name = metas[-2]
		class_info = context.classes_by_name.get(class_name)
		if not class_info:
			return None, None

		for func_info in class_info.get('funcs', ()):
			if func_info['name'] == func_name and func_info['func_flags'] & UE4Flags.FUNC_Static:
				return class_info, func_info
		return None, None

	for struct_info in context.structs_by_name.values():
		cpp_name = struct_info.get('cpp_name')
		if not cpp_name:
			continue

		make_func_meta = struct_info.get('make_func')
		break_func_meta = struct_info.get('break_func')
		if not (make_func_meta or break_func_meta):
			continue

		func_names = set()
		for func_info in struct_info.get('funcs', ()):
			func_names.add(func_info['name'])

		# make_func相当于结构体的构造函数
		# 假如结构体已有构造函数，则不采用make_func
		if make_func_meta and cpp_name not in func_names:
			# 补上默认无参构造函数
			default_func_info = {
				'name': cpp_name,
				'pretty_name': cpp_name,
				'func_flags': UE4Flags.FUNC_Native | UE4Flags.FUNC_Public,
				'doc': '',
				'params': [],
				'meta_data': {},
			}
			struct_info.setdefault('funcs', []).append(default_func_info)

			hosted_class_info, external_func_info = find_external_func(make_func_meta)
			new_func_info = {
				'name': cpp_name,
				'pretty_name': cpp_name,
				'func_flags': external_func_info['func_flags'] & ~(UE4Flags.FUNC_Static),
				'doc': external_func_info.get('doc', ''),
				'params': external_func_info['params'][:-1], # skip return value
				'hosted_class': hosted_class_info,
				'external_func': external_func_info,
				'meta_data': external_func_info['meta_data'],
			}
			struct_info.setdefault('funcs', []).append(new_func_info)

		# break_func相当于结构体的AsTuple函数
		# 假如结构体已有AsTuple函数，则不采用break_func
		if break_func_meta and 'AsTuple' not in func_names:
			hosted_class_info, external_func_info = find_external_func(break_func_meta)
			new_func_info = {
				'name': 'AsTuple',
				'pretty_name': 'AsTuple',
				'func_flags': external_func_info['func_flags'] & ~(UE4Flags.FUNC_Static),
				'doc': external_func_info.get('doc', ''),
				'params': external_func_info['params'][1:], # skip self
				'hosted_class': hosted_class_info,
				'external_func': external_func_info,
				'meta_data': external_func_info['meta_data'],
			}
			struct_info.setdefault('funcs', []).append(new_func_info)


# 收集函数重载信息
def collect_overload_funcs(context):
	# type: (GeneratorContext) -> None
	# need to be compatible with py2 and py3
	for type_info in itertools.chain(iter(context.classes_by_name.values()), iter(context.structs_by_name.values())):
		func_infos = type_info.get('funcs')
		if not func_infos:
			continue

		# key: func_name
		# val: count
		func_overload_count = {}

		has_overload_func = False
		for func_info in func_infos:
			func_name = func_info['pretty_name']
			if func_name not in func_overload_count:
				func_overload_count[func_name] = 1
			else:
				func_overload_count[func_name] += 1
				has_overload_func = True

		if not has_overload_func:
			continue

		# key: func_name
		# val: overload_func_info
		overload_func_infos = {}

		func_index = 0
		while func_index < len(func_infos):
			func_info = func_infos[func_index]
			func_name = func_info['pretty_name']
			overload_count = func_overload_count[func_name]
			if overload_count == 1:
				# 没有重载，无事发生
				func_index += 1
				continue

			# 使用函数重载信息替换原始函数信息
			if func_name not in overload_func_infos:
				overload_func_info = {
					'name': func_info['name'],
					'pretty_name': func_info['pretty_name'],
					'overloads': [],
					'is_static': True,
				}
				overload_func_infos[func_name] = overload_func_info
				func_infos[func_index] = overload_func_info
				func_index += 1
			else:
				overload_func_info = overload_func_infos[func_name]
				func_infos.pop(func_index)

			if not (func_info['func_flags'] & UE4Flags.FUNC_Static):
				# 只要有一个函数不为static，则不可导出为Python类静态方法
				overload_func_info['is_static'] = False

			overload_func_info['overloads'].append(func_info)
			overload_index = len(overload_func_info['overloads'])
			func_info['overload_index'] = overload_index
			func_info['pretty_name'] = '%s_Overload%d' % (func_info['pretty_name'], overload_index)

# 导出单个模块
def export_module(context, module_rule):
	# type: (ExportContext, dict) -> tuple[str, str]
	context.curr_module_rule = module_rule
	context.curr_prop_filter_flags = get_prop_filter_flags(module_rule)
	context.curr_func_filter_flags = get_func_filter_flags(module_rule)
	is_game_module = check_is_game_module(module_rule)
	module_name = module_rule['module']

	gen_path = module_rule['gen_path']
	gen_path = os.path.join(gen_path, module_name)
	if not os.path.isdir(gen_path):
		os.makedirs(gen_path)

	child_header_names = []
	child_init_func_names = []

	need_gen_file = False

	# 类与结构体
	package_rules = module_rule['packages']
	for package_rule in package_rules:
		result = export_package_for_module(context, module_name, gen_path, package_rule, is_game_module)
		child_header_names.extend(result['header_names'])
		child_init_func_names.extend(result['init_func_names'])
		need_gen_file |= result['need_gen_file']

	# 手写的结构体
	for struct_name in module_rule.get('manual_structs', []):
		manual_header_name, manual_init_func_name = export_manual_struct_for_module(context, module_name, gen_path, struct_name)
		if manual_header_name and manual_init_func_name:
			child_header_names.append(manual_header_name)
			child_init_func_names.append(manual_init_func_name)

	child_include_list = []
	for child_header_name in child_header_names:
		child_include_list.append('#include "%s"' % child_header_name)

	child_init_list = []
	for child_init_func_name in child_init_func_names:
		child_init_list.append('%s(PyModule);' % child_init_func_name)

	init_func_name = 'NePyInitModule_%s' % module_name
	header_name = 'NePyModule_%s.h' % module_name
	source_name = 'NePyModule_%s.cpp' % module_name
	with_editor_begin, with_editor_end = get_with_editor_macros(module_rule)

	substitute_dict = {
		'module_name': module_name,
		'init_func_name': init_func_name,
		'with_editor_begin': with_editor_begin,
		'with_editor_end': with_editor_end,

		# .h
		'child_include_list_h': '\n'.join(child_include_list),

		# .cpp
		'header_name': header_name,
		'child_include_list_cpp': '__REMOVE_THIS_LINE__',
		'full_module_name': '%s.%s' % (ExportConfig.RootModuleName, module_name),
		'child_init_list': '\n\t'.join(child_init_list),
	}

	if need_gen_file:
		TemplateHelper.build_template('templates/module.h', os.path.join(gen_path, header_name), substitute_dict)
		TemplateHelper.build_template('templates/module.cpp', os.path.join(gen_path, source_name), substitute_dict)
	else:
		dlog.debug('!module %s skip gen:' % (module_name,))

	return '%s/%s' % (module_name, header_name), init_func_name


# 导出一个手写的结构体
def export_manual_struct_for_module(context, module_name, gen_path, struct_name):
	# type: (ExportContext, str, str, str) -> tuple[str|None, str]
	header_name = None
	manual_code_dir = gen_path[:gen_path.rfind('Auto')] + 'Manual' # 手写代码搜索路径
	manual_code_path = '%s/NePyStruct_%s.h' % (manual_code_dir, struct_name)
	init_func_name = 'NePyInitStruct_' + struct_name
	if check_manual_code_func(manual_code_path, init_func_name):
		header_name = 'NePy/Manual/NePyStruct_%s.h' % struct_name
	return header_name, init_func_name


# 检查类型是否有手写的方法
def check_manual_code_func(manual_code_path, func_name):
	# type: (str, str) -> bool
	if not os.path.isfile(manual_code_path):
		return False

	has_func = False

	# need to be compatible with py2 and py3
	data = open_and_read_str(manual_code_path)
	if data:
		if data.find(func_name) > 0:
			has_func = True

	return has_func


# 导出一个UE4 Package下的所有类、结构体、枚举
def export_package_for_module(context, py_module_name, gen_path, package_rule, is_game_module):
	# type: (ExportContext, str, str, dict, bool) -> dict[str, list[str]]
	package_name = package_rule['name']
	context.curr_package_name = package_name
	context.curr_package_filter_rules = FilterRules(package_rule.get('exclude', {}))
	context.curr_py_module_name = py_module_name

	dlog.debug('package:', package_name)
	dlog.indent()

	header_names = []
	init_func_names = []

	# 所有结构体（以F打头的）
	struct_infos = context.current_gen.structs_by_package.get(package_name, [])
	for struct_info in struct_infos:
		if not PropHelpers.is_export_type(struct_info):
			continue
		header_name, init_func_name = export_struct(context, gen_path, struct_info, is_game_module)
		manual_type_info = ExportConfig.ManualTypeInfos.get(struct_info['name'])
		if not manual_type_info or manual_type_info.get('need_build_template'):
			header_names.append(header_name)
			init_func_names.append(init_func_name)

	# 所有继承自UObject的对象（以U或A打头的）
	class_infos = context.current_gen.classes_by_package.get(package_name, [])
	for class_info in class_infos:
		if not PropHelpers.is_export_type(class_info):
			continue
		header_name, init_func_name = export_class(context, gen_path, class_info, is_game_module)
		manual_type_info = ExportConfig.ManualTypeInfos.get(class_info['name'])
		if not manual_type_info or manual_type_info.get('need_build_template'):
			header_names.append(header_name)
			init_func_names.append(init_func_name)

	removed_structs = context.structs_dont_gen_this_time()
	removed_classes = context.classes_dont_gen_this_time()

	# 所有结构体导出代码的删除
	dec_struct_infos = context.dec_gen.structs_by_package.get(package_name, [])
	for struct_info in dec_struct_infos:
		if struct_info['name'] in removed_structs:
			remove_exported_type(context, gen_path, struct_info, is_game_module)

	# 所有类导出代码的删除
	dec_class_infos = context.dec_gen.classes_by_package.get(package_name, [])
	for class_info in dec_class_infos:
		if class_info['name'] in removed_classes:
			remove_exported_type(context, gen_path, class_info)

	dlog.unindent()

	return {
		'header_names': header_names,
		'init_func_names': init_func_names,
		'need_gen_file': not context.package_dont_need_to_gen_this_time(package_name),
	}


# 导出一个结构体
def export_struct(context, gen_path, struct_info, is_game_module):
	# type: (ExportContext, str, dict, bool) -> tuple[str, str]
	struct_name = struct_info['name']
	dlog.debug('struct:', struct_name)
	dlog.indent()
	dlog.debug('flags:', UE4Flags.explain_struct_flags(struct_info['struct_flags']))

	extra_info = {
		'is_struct': True,
		'is_game_module': is_game_module,
		'init_func_name': 'NePyInitStruct_' + struct_name,
		'template_name': 'struct',
		'prop_filter': context.curr_package_filter_rules.should_export_struct_prop,
		'func_filter': context.curr_package_filter_rules.should_export_struct_func,
	}

	return export_type(context, gen_path, struct_info, extra_info)


# 导出一个类
def export_class(context, gen_path, class_info, is_game_module):
	# type: (ExportContext, str, dict, bool) -> tuple[str, str]
	class_name = class_info['name']
	class_flags = class_info['class_flags']
	dlog.debug('class:', class_name)
	dlog.indent()
	dlog.debug('flags:', UE4Flags.explain_class_flags(class_flags))

	extra_info = {
		'is_object': not (class_flags & UE4Flags.CLASS_Interface),
		'is_interface': bool(class_flags & UE4Flags.CLASS_Interface),
		'is_game_module': is_game_module,
		'init_func_name': 'NePyInitObject_' + class_name,
		'template_name': 'object',
		'prop_filter': context.curr_package_filter_rules.should_export_class_prop,
		'func_filter': context.curr_package_filter_rules.should_export_class_func,
	}

	if ExportConfig.AutoCreateSubClassPyDict:
		extra_info['has_pydict'] = check_type_has_pydict(context, class_name, True)
		extra_info['clear_pydict'] = False
	else:
		if check_type_has_pydict(context, class_name, False):
			extra_info['has_pydict'] = True
			extra_info['clear_pydict'] = False
		else:
			extra_info['has_pydict'] = False
			extra_info['clear_pydict'] = check_type_has_pydict(context, class_name, True)

	return export_type(context, gen_path, class_info, extra_info)


# 检查类型是否需要添加PyDict
def check_type_has_pydict(context, class_name, check_super_class):
	# type: (GeneratorContext, str, bool) -> bool
	class_info = context.current_gen.classes_by_name.get(class_name)
	if class_info is None or not PropHelpers.is_export_type(class_info):
		return False

	if class_info.get('has_pydict', False):
		return True

	if check_super_class:
		super_class_name = class_info.get('super')
		if super_class_name:
			return check_type_has_pydict(context, super_class_name, True)

	return False

# 处理已导出的类型的导出代码删除
def remove_exported_type(context, gen_path, type_info):
	# type: (ExportContext, str, dict) -> None
	type_name = type_info['name']
	header_name = type_info['py_header_name']
	source_name = type_info['py_source_name']

	manual_type_info = ExportConfig.ManualTypeInfos.get(type_name)
	if not manual_type_info or manual_type_info.get('need_build_template'):
		auto_gen_h_path = os.path.join(gen_path, header_name)
		auto_gen_cpp_path = os.path.join(gen_path, source_name)
		if os.path.isfile(auto_gen_h_path):
			os.remove(auto_gen_h_path)
			dlog.debug('remove auto gen file:', auto_gen_h_path)
		if os.path.isfile(auto_gen_cpp_path):
			os.remove(auto_gen_cpp_path)
			dlog.debug('remove auto gen file:', auto_gen_cpp_path)

# 导出一个类型（UClass或UStruct）
def export_type(context, gen_path, type_info, extra_info):
	# type: (ExportContext, str, dict, dict) -> tuple[str, str]
	type_name = type_info['name']
	cpp_type_name = type_info['cpp_name']
	py_type_name = type_info['py_name']
	py_new_func_name = type_info['py_new_func_name']
	dealloc_func_name = type_info.get('py_dealloc_func_name')
	py_check_func_name = type_info['py_check_func_name']
	py_get_type_func_name = type_info['py_get_type_func_name']
	header_name = type_info['py_header_name']
	source_name = type_info['py_source_name']

	is_struct = extra_info.get('is_struct', False)
	is_object = extra_info.get('is_object', False)
	is_interface = extra_info.get('is_interface', False)
	is_game_module = extra_info['is_game_module']
	init_func_name = extra_info['init_func_name']
	template_name = extra_info['template_name']
	prop_filter = extra_info.get('prop_filter', lambda *args: True)
	context.curr_prop_filter_func = prop_filter
	func_filter = extra_info.get('func_filter', lambda *args: True)
	context.curr_func_filter_func = func_filter
	has_pydict = extra_info.get('has_pydict', False)
	clear_pydict = extra_info.get('clear_pydict', False)

	auto_gen_h_path = os.path.join(gen_path, header_name)
	auto_gen_cpp_path = os.path.join(gen_path, source_name)

	auto_export = False
	manual_type_info = ExportConfig.ManualTypeInfos.get(type_name)
	if not manual_type_info or manual_type_info.get('need_build_template'):
		auto_export = True

	# 判断是否可以跳过本次代码生成
	if auto_export and os.path.isfile(auto_gen_h_path) and os.path.isfile(auto_gen_cpp_path):
		if is_struct and context.struct_dont_need_to_gen_this_time(type_name):
			dlog.debug('!skip struct gen file:', type_name)
			dlog.unindent()
			return header_name, init_func_name
		if is_object and context.class_dont_need_to_gen_this_time(type_name):
			dlog.debug('!skip object gen file:', type_name)
			dlog.unindent()
			return header_name, init_func_name

	# 需要在.h文件中include的文件列表
	include_h_set = set()
	include_h_list = []

	def _include_h(header):
		if header not in include_h_set:
			include_h_set.add(header)
			include_h_list.append('#include "%s"' % header)

	# 需要在.cpp文件中include的文件列表
	include_cpp_set = set()
	include_cpp_list = []

	def _include_cpp(header):
		if header not in include_cpp_set:
			include_cpp_set.add(header)
			include_cpp_list.append('#include "%s"' % header)

	# 导出成员属性
	prop_getset_body_list = []
	prop_getset_list = []
	for prop_info in type_info['props']:
		prop_name = prop_info['name']
		prop_flags = prop_info['prop_flags']
		if prop_flags & context.curr_prop_filter_flags:
			dlog.debug('!skip prop by flags:', prop_name, UE4Flags.explain_prop_flags(prop_flags))
			continue
		if not prop_filter(type_name, prop_name):
			dlog.debug('!exclude prop by filter:', prop_name)
			continue

		prop_builder = PropBuilder(prop_info, cpp_type_name, context, is_struct)
		result = prop_builder.build_prop_getset(type_info)
		prop_getset_body_list.append(result['getset_body'])
		prop_getset_list.append(result['getset'])
		for child_header_name in result['include_list']:
			_include_cpp(child_header_name)

	# 导出成员方法
	func_names = set()
	func_list = []
	func_body_list = []
	number_func_list = []
	func_substitute_flags = FuncBuilderBase.get_default_substitute_flags()

	def export_func(func_info):
		func_name = func_info['pretty_name'] if ExportConfig.UsePrettyName else func_info['name']
		func_flags = func_info.get('func_flags', 0)
		# 同名函数去重
		if func_name in func_names:
			return
		if 'overloads' not in func_info and not func_flags & UE4Flags.FUNC_Native:
			# 'overloads' 的 func_info 不包含 func_flags，在这一步不处理
			dlog.debug('!skip not native func:', func_name, UE4Flags.explain_func_flags(func_flags))
			return
		if func_flags & context.curr_func_filter_flags:
			dlog.debug('!skip func by flags:', func_name, UE4Flags.explain_func_flags(func_flags))
			return
		if not func_filter(type_name, func_name):
			dlog.debug('!exclude func by filter:', func_name)
			return

		func_names.add(func_name)

		if 'overloads' not in func_info:
			func_builder = FuncBuilder(func_info, context, is_struct, type_info)
		else:
			func_builder = OverloadFuncBuilder(func_info, context, is_struct, type_info, context.curr_func_filter_flags)

		result = func_builder.build_member_func()
		func_body_list.append(result['func_body'])

		for child_header_name in result['include_list']:
			_include_cpp(child_header_name)

		if 'func' in result:
			func_list.append(result['func'])

		if 'number_func' in result:
			number_func_list.append(result['number_func'])

		if 'func_substitute_flags' in result:
			func_substitute_flags.update(result['func_substitute_flags'])

	for func_info in type_info.get('funcs', []):
		export_func(func_info)

	# 导出继承接口的方法
	def export_interface_func(interface_type_info):
		super_interface_type_info = context.current_gen.classes_by_name.get(interface_type_info['super'])
		if super_interface_type_info is not None:
			export_interface_func(super_interface_type_info)
		for interface_func_info in interface_type_info.get('funcs', []):
			export_func(interface_func_info)

	for interface_info in type_info.get('interfaces', []):
		interface_type_info = context.current_gen.classes_by_name.get(interface_info['name'])
		if not interface_type_info:
			continue
		export_interface_func(interface_type_info)

	# 注册Struct，用于动态反射获取和设置struct类型的属性
	has_static_struct = False
	if is_struct:
		if PropHelpers.has_export_struct_type(type_info):
			has_static_struct = True

	# 手写代码
	if 'manual_header_path' in type_info:
		_include_cpp(type_info['manual_header_path'])
		func_list.append('\tNePyManualExportFuncs')

	_include_h(type_info['header'])

	with_editor_begin, with_editor_end = get_with_editor_macros(context.curr_module_rule)

	substitute_dict = {
		# .h|.cpp
		'type_name': type_name,
		'cpp_type_name': cpp_type_name,
		'py_type_name': py_type_name,
		'init_func_name': init_func_name,
		'py_new_func_name': py_new_func_name,
		'py_dealloc_func_name': dealloc_func_name,
		'py_check_func_name': py_check_func_name,
		'py_get_type_func_name': py_get_type_func_name,
		'need_conversion_funcs': type_name != 'Object',
		'with_editor_begin': with_editor_begin,
		'with_editor_end': with_editor_end,
		'has_pydict': has_pydict,

		# .h
		'include_list_h': '\n'.join(include_h_list),
		'module_api_name': get_module_api_name(is_game_module),

		# .cpp
		'package_name': context.curr_package_name,
		'header_name': header_name,
		'include_list_cpp': '\n'.join(include_cpp_list),
		'has_props': len(prop_getset_list) > 0,
		'prop_getset_list': '\n'.join(prop_getset_list),
		'has_prop_bodies': len(prop_getset_body_list) > 0,
		'prop_getset_body_list': '\n\n'.join(prop_getset_body_list),
		'has_funcs': len(func_list) > 0,
		'func_list': '\n'.join(func_list) if len(func_list) > 0 else '__REMOVE_THIS_LINE__',
		'has_func_bodies': len(func_body_list) > 0,
		'func_body_list': '\n\n'.join(func_body_list) if len(func_body_list) > 0 else '__REMOVE_THIS_LINE__',
		'has_number_funcs': len(number_func_list) > 0,
		'number_func_list': '\n\t'.join(number_func_list),
		'has_static_struct': has_static_struct,
		'not_has_static_struct': not has_static_struct,
		'clear_pydict': clear_pydict,
		'is_object': is_object,
		'is_interface': is_interface,
	}
	substitute_dict.update(func_substitute_flags)

	if auto_export:
		TemplateHelper.build_template('templates/%s.h' % template_name, auto_gen_h_path, substitute_dict)
		TemplateHelper.build_template('templates/%s.cpp' % template_name, auto_gen_cpp_path, substitute_dict)

	dlog.unindent()

	return header_name, init_func_name


# 导出各种UProperty的UClass，方便Subclassing使用
def export_uproperty_classes():
	# type: () -> tuple[str, str]
	init_func_name = 'NePyInitUPropertyClasses'
	header_name = 'NePyUPropertyClasses.h'
	source_name = 'NePyUPropertyClasses.cpp'

	# need to be compatible with py2 and py3
	prop_class_names = list(PropHelpers.BUILTIN_TYPES.keys()) + list(PropHelpers.STRING_TYPES.keys())

	uproperty_codes = []
	for class_name in sorted(prop_class_names):
		line = 'PyModule_AddObject(PyOuterModule, "%s", NePyBase::ToPy(U%s::StaticClass()));' \
			% (class_name, class_name)
		uproperty_codes.append(line)

	substitute_dict = {
		'header_name': header_name,
		'init_func_name': init_func_name,
		'uproperty_codes': '\n\t'.join(uproperty_codes),
	}

	gen_path = ExportConfig.EngineBindingGeneratePath
	TemplateHelper.build_template('templates/uproperties.h', os.path.join(gen_path, header_name), substitute_dict)
	TemplateHelper.build_template('templates/uproperties.cpp', os.path.join(gen_path, source_name), substitute_dict)

	return header_name, init_func_name


# 导出内嵌执行的Python脚本
def export_builtin_script():
	# type: () -> str
	# need to be compatible with py2 and py3
	code_data = open_and_read_str('templates/builtin.py')
	code_data = TemplateHelper.substitute(code_data, {'root_module_name': ExportConfig.RootModuleName})
	code_data_lines = code_data.splitlines()
	code_data = '"' + '\\n"\n\t\t"'.join(code_data_lines) + '"'
	return code_data


# 执行UE项目的Commandlet生成蓝图信息
def dump_blueprint_infos(temp_dir):
	# type: (str) -> bool
	import subprocess
	import platform
	
	# 获取项目路径
	uproject_path = os.path.join(ExportConfig.GamePath, ExportConfig.GameProject)
	if not os.path.isfile(uproject_path):
		dlog.info('Error: Project file not found: %s' % uproject_path)
		return False
	
	# 获取引擎路径
	engine_path = ExportConfig.EnginePath
	if not engine_path or not os.path.isdir(engine_path):
		dlog.info('Error: Engine path not found: %s' % engine_path)
		return False
	
	# 根据平台和引擎版本构建UE编辑器路径
	ue_editor = None
	
	# 根据平台确定binaries目录和文件扩展名
	platform_info = {
		"Windows": ("Win64", ".exe"),
		"Darwin": ("Mac", ""),      # macOS无扩展名
		"Linux": ("Linux", "")
	}
	
	if platform.system() in platform_info:
		binaries_subdir, exe_ext = platform_info[platform.system()]
		binaries_dir = os.path.join(engine_path, 'Binaries', binaries_subdir)
		
		if os.path.isdir(binaries_dir):
			import glob
			# 查找所有UE编辑器可执行文件
			editor_patterns = [
				os.path.join(binaries_dir, 'UnrealEditor-*-Cmd' + exe_ext),
				os.path.join(binaries_dir, 'UnrealEditor-Cmd' + exe_ext),
				os.path.join(binaries_dir, 'UE4Editor-*-Cmd' + exe_ext),
				os.path.join(binaries_dir, 'UE4Editor-Cmd' + exe_ext),
			]
			
			candidates = []
			for pattern in editor_patterns:
				candidates.extend(glob.glob(pattern))
			
			# 按优先级排序：Release > Debug > 其他
			def get_priority(filename):
				name = os.path.basename(filename).lower()
				if 'release' in name or name in ['unrealeditor-cmd' + exe_ext.lower(), 'ue4editor-cmd' + exe_ext.lower()]:
					return 0  # 最高优先级
				elif 'debug' in name:
					return 1  # 次高优先级
				else:
					return 2  # 其他
			
			candidates.sort(key=lambda x: (get_priority(x), x))
			editor_paths = candidates
		else:
			editor_paths = []
	else:
		dlog.info('Error: Unsupported platform: %s' % platform.system())
		return False

	# 查找存在的编辑器可执行文件
	for editor_path in editor_paths:
		if os.path.isfile(editor_path):
			ue_editor = editor_path
			break

	if not ue_editor:
		dlog.info('Error: UnrealEditor-Cmd not found in any of these paths:')
		for path in editor_paths:
			dlog.info('  - %s' % path)
		return False

	dlog.debug('Found UE Editor: %s' % ue_editor)
	
	# 输出文件路径
	output_file = os.path.abspath(os.path.join(temp_dir, 'blueprint_infos_full.json'))
	
	# 构建命令
	# 添加 -compile 参数确保项目编译是最新的
	# 添加 -unattended 参数避免弹出对话框
	# 添加 -nopause 参数避免等待用户输入
	cmd = [
		ue_editor,
		uproject_path,
		'-Run=NePyDumpBlueprintInfosCommandlet',
		'-OutputFile=%s' % output_file,
		'-unattended',  # 无人值守模式，避免弹窗
		'-nopause',     # 不暂停等待输入
		'-buildmachine' # 构建机模式，确保编译
	]
	
	dlog.info('Running commandlet to dump blueprint infos...')
	dlog.debug('Command: %s' % ' '.join(cmd))
	
	try:
		result = subprocess.call(cmd)
		if os.path.isfile(output_file):
			if result != 0:
				dlog.info('Warning: Commandlet exited with code %d, but output file was generated.' % result)
			return True
		else:
			# 输出文件未生成，视为失败
			dlog.info('Error: Commandlet failed to generate output file. Exit code: %d' % result)
			return False
	except Exception as e:
		dlog.info('Error: Failed to run commandlet: %s' % str(e))
		return False

# 仅生成文档
def generate_docs_only(context, temp_dir, generate_normal_doc, generate_blueprint_doc):
	# type: (ExportContext, str, bool, bool) -> None
	if generate_normal_doc:
		dlog.info('Build python stubs ...')
		begin_time = time.time()
		DocGenerator().generate_docs(context)
		dlog.info('Build python stubs cost time: %.2f' % (time.time() - begin_time))

	if generate_blueprint_doc:
		blueprint_info_file_path = os.path.join(temp_dir, 'blueprint_infos_full.json')
		if os.path.isfile(blueprint_info_file_path):
			dlog.info('Build blueprint stubs ...')
			begin_time = time.time()
			BlueprintDocGenerator().generate_docs(context, blueprint_info_file_path)
			dlog.info('Build blueprint stubs cost time: %.2f' % (time.time() - begin_time))
		else:
			dlog.info('Blueprint info file not found, skipping blueprint stub generation: %s' % blueprint_info_file_path)


# 仅处理蓝图相关：导出蓝图信息 + 生成蓝图文档
def generate_blueprint_only(context, temp_dir):
	# type: (ExportContext, str) -> None
	"""
	只做两件事：
	1. 从UE项目导出蓝图信息
	2. 生成蓝图pyi文档
	"""
	# 1. 导出蓝图信息
	dlog.info('Dumping blueprint infos ...')
	begin_time = time.time()
	if not dump_blueprint_infos(temp_dir):
		dlog.info('Error: Failed to dump blueprint infos')
		return
	dlog.info('Dump blueprint infos cost time: %.2f' % (time.time() - begin_time))

	# 2. 生成蓝图文档
	blueprint_info_file_path = os.path.join(temp_dir, 'blueprint_infos_full.json')
	if os.path.isfile(blueprint_info_file_path):
		dlog.info('Build blueprint stubs ...')
		begin_time = time.time()
		BlueprintDocGenerator().generate_docs(context, blueprint_info_file_path)
		dlog.info('Build blueprint stubs cost time: %.2f' % (time.time() - begin_time))
	else:
		dlog.info('Error: Blueprint info file not found: %s' % blueprint_info_file_path)

def main(args):
	dlog.set_log_level(args.log_level)
	dlog.set_log_prefix(args.log_prefix)

	context = ExportContext()

	temp_dir = os.path.abspath('temp')
	if not os.path.isdir('temp'):
		os.makedirs(temp_dir)

	# 如果是只生成蓝图相关，直接调用专门的函数
	if args.blueprint_only:
		generate_blueprint_only(context, temp_dir)
		return

	if args.cleanup:
		cleanup_generation_paths()

	reflection_infos_file_path = os.path.join(temp_dir, 'reflection_infos.json')
	noexport_reflection_infos_file_path = os.path.join(temp_dir, 'reflection_infos_noexport.json')
	full_reflection_infos_file_path = os.path.join(temp_dir, 'reflection_infos_full.json')
	source_infos_file_path = os.path.join(temp_dir, 'source_infos.json')
	clang_infos_file_path = os.path.join(temp_dir, 'clang_infos.json')

	use_noexport_types_file = ExportConfig.UseNePyNoExportTypes and os.path.exists(ExportConfig.NePyNoExportTypesFilePath)
	dlog.info('ENGINE_MAJOR_VERSION:', ExportConfig.ENGINE_MAJOR_VERSION)
	dlog.info('ENGINE_MINOR_VERSION:', ExportConfig.ENGINE_MINOR_VERSION)
	dlog.info('bUseNoExportTypes:', use_noexport_types_file)
	if use_noexport_types_file:
		dlog.info('NoExportTypesFilePath:', ExportConfig.NePyNoExportTypesFilePath)

	if args.parse_source:
		import ReflectionDumper
		dlog.info('run ReflectionDumper ...')
		begin_time = time.time()
		if use_noexport_types_file:
			# 先将NoExportTypes.h拷贝到Source目录，以供UHT解析
			with ReflectionDumper.CopyNoExportTypesFileBlock():
				if not ReflectionDumper.main():
					return
		else:
			if not ReflectionDumper.main():
				return
		dlog.info('run ReflectionDumper cost time: %.2f' % (time.time() - begin_time))
		if not os.path.exists(reflection_infos_file_path) or not os.path.exists(full_reflection_infos_file_path):
			dlog.info('Error: reflection json output is missing. Do you forget to setup NePythonGenerator in your engine folder?')
			return

	if not os.path.exists(reflection_infos_file_path) or not os.path.exists(full_reflection_infos_file_path):
		dlog.info('Error: reflection json output is missing. Do you forget to generate reflection information')
		return

	load_reflection_infos(context.current_gen, reflection_infos_file_path, full_reflection_infos_file_path)

	if ExportConfig.EngineVersion < (5, 2):
		# UE 5.2 以后使用一定会使用C#版的NePythonGenerator
		# C#版的NePythonGenerator能够从UHT中获得足够的信息，不再需要SourceFinder了
		if args.parse_source or not os.path.isfile(source_infos_file_path):
			import SourceFinder
			dlog.info('run SourceFinder ...')
			begin_time = time.time()
			SourceFinder.main()
			dlog.info('run SourceFinder cost time: %.2f' % (time.time() - begin_time))
		load_source_infos(context.current_gen, source_infos_file_path)

	if use_noexport_types_file:
		if os.path.isfile(noexport_reflection_infos_file_path):
			load_noexport_type_infos(context.current_gen, noexport_reflection_infos_file_path)
		# 先保证向前兼容性
		else:
			load_noexport_type_infos(context.current_gen, full_reflection_infos_file_path)
	else:
		if args.parse_source or not os.path.isfile(clang_infos_file_path):
			import ClangDumper
			dlog.info('run ClangDumper ...')
			begin_time = time.time()
			ClangDumper.main()
			dlog.info('run ClangDumper cost time: %.2f' % (time.time() - begin_time))
		load_clang_infos(context.current_gen, clang_infos_file_path)

	# 先走一遍预处理
	for module_rule in ExportConfig.ExportModules:
		for package_rule in module_rule['packages']:
			preprocess_package_for_module(context, module_rule, package_rule)

	# 处理noexport类代码include路径
	fix_no_export_types_include_path(context.current_gen)

	# 为结构体添加Make/Break方法
	add_make_break_func_to_struct(context.current_gen)

	# 然后收集函数重载信息
	collect_overload_funcs(context.current_gen)

	# 做增量生成准备
	context.enable_increment_gen = args.increment_gen
	context.cache_gen_enable_debug = args.increment_gen_debug
	if args.increment_gen:
		dlog.info('Prepare for increment gen ...')
		begin_time = time.time()
		context.prepare_for_increment_gen(temp_dir)
		dlog.info('Prepare for increment gen cost time: %.2f' % (time.time() - begin_time))

	dlog.info('Export modules ...')
	begin_time = time.time()
	export_modules(context)
	dlog.info('Export modules cost time: %.2f' % (time.time() - begin_time))

	# 做增量生成收尾
	if args.increment_gen:
		dlog.info('Finish for increment gen ...')
		begin_time = time.time()
		context.finish_for_increment_gen()
		dlog.info('Finish for increment gen: %.2f' % (time.time() - begin_time))

	dlog.info('Build python stubs ...')
	begin_time = time.time()
	DocGenerator().generate_docs(context)
	dlog.info('Build python stubs cost time: %.2f' % (time.time() - begin_time))

	# 如果指定了-b参数，执行蓝图信息导出
	if args.dump_blueprint:
		dlog.info('Dumping blueprint infos ...')
		begin_time = time.time()
		if not dump_blueprint_infos(temp_dir):
			dlog.info('Warning: Failed to dump blueprint infos')
		else:
			dlog.info('Dump blueprint infos cost time: %.2f' % (time.time() - begin_time))

	# 生成蓝图文档
	blueprint_info_file_path = os.path.join(temp_dir, 'blueprint_infos_full.json')
	if os.path.isfile(blueprint_info_file_path):
		dlog.info('Build blueprint stubs ...')
		begin_time = time.time()
		BlueprintDocGenerator().generate_docs(context, blueprint_info_file_path)
		dlog.info('Build blueprint stubs cost time: %.2f' % (time.time() - begin_time))
	else:
		dlog.info('Blueprint info file not found, skipping blueprint stub generation: %s' % blueprint_info_file_path)

if __name__ == '__main__':
	if sys.maxsize <= 2 ** 32:
		print('BindingGenerator can only be run by 64-bit Python!')
		print(u'BindingGenerator只能使用64位Python运行！')
		exit(1)

	import time
	begin = time.time()

	parse = argparse.ArgumentParser()
	parse.add_argument('-c', '--cleanup', action='store_true',  help=u'Cleanup generation destination before code-generating. Default is False. (在生成代码前，清理生成目录。默认为False。)')
	parse.add_argument('-p', '--parse-source', action='store_true',  help=u'Parse source files before code-generating. Otherwise will use cache as source infos. Defualt is False. (在生成代码前，重新解析源码文件。否则，会尝试使用缓存的源码信息。默认为False。)')
	parse.add_argument('-i', '--increment-gen', action='store_true',  help=u'Whether to enable increment code gen. ( 是否使用增量代码生成。默认为False。)')
	parse.add_argument('-id', '--increment-gen-debug', action='store_true',  help=u'Whether to enable increment code gen. (是否使用增量代码生成调试输出。默认为False。)')
	parse.add_argument('-l', '--log-level', type=int, choices=[0, 1, 2], default=1, help=u'Log level. 0 means no logs at all, 1 means infos only, 2 means full debug logs. Defualt is 1. (日志等级。0是没有日志，1是仅输出普通信息，2是还包括调试信息。默认值为1.)')
	parse.add_argument('-lp', '--log-prefix', type=str, default="", help=u'Log prefix')
	parse.add_argument('-b', '--dump-blueprint', action='store_true',  help=u'Dump blueprint infos from UE project before code-generating. (在生成代码前，先从UE项目导出蓝图信息。默认为False。)')
	parse.add_argument('-bd', '--blueprint-only', action='store_true',  help=u'Only dump blueprint infos and generate blueprint type hints. (只导出蓝图信息并生成蓝图类型提示。默认为False。)')

	args = parse.parse_args()
	from clang.cindex import Config
	Config.set_library_path('clang/native/')
	main(args)

	dlog.info('cost time: %.2f' % (time.time() - begin))