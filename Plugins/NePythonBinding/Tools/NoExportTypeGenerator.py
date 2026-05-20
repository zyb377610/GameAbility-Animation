# -*- encoding: utf-8 -*-
# 此工具通过读取clang_infos.json，生成NePyNoExportTypes.h，以供UHT解析

import os
import json
from GeneratorContext import GeneratorContext
from CodeBuilder import CodeBuilder
import BindingGenerator
import PropHelpers
import UE4Flags
from version_helper import open_and_write_str
import ExportConfig

# 过滤掉的函数
IGNORE_FUNCS = {
	# ↓↓  函数参数中含有结构体指针，UHT解析报错
	'Vector': ('Triple', ),
	'Matrix': ('SetAxes', ),
	'Quat': ('Identical', ),
	'Transform': ('Identical', 'Multiply'),
	# ↓↓ 函数参数中含有double类型，不支持
	'Math': ('IsNearlyEqual_Overload2', 'IsNearlyZero_Overload2', 'IsNearlyEqualByULP_Overload2', 'MakePulsatingValue', 'TruncateToHalfIfClose_Overload2'),
	# ↓↓ 函数形参和结构体属性同名了，UHT报错
	'Color': ('MakeRequantizeFrom1010102', ),
}

def get_prop_cpp_type(context, prop_info):
	# type: (GeneratorContext, dict) -> str
	prop_type = 'unknow'
	if prop_info.get('enum_cpp_name'):
		if prop_info['enum_cpp_name'] in context.enums_by_name:
			prop_type = prop_info['enum_cpp_name']
			if prop_info.get('enum_cpp_form') == 1:
				prop_type = 'TEnumAsByte<%s>' % prop_type
	elif prop_info['type_info']:
		prop_type = prop_info['type_info']['cpp_name']
	
	if prop_type == 'unknow':
		print('Warning: Ignore unkonw param type:', prop_info)
		print('\n')
	return prop_type

def get_param_cpp_type_str(context, param_info):
	# type: (GeneratorContext, dict) -> str
	prop_type = get_prop_cpp_type(context, param_info)
	prop_flags = param_info['prop_flags']
	if prop_flags & UE4Flags.CPF_OutParm and not (prop_flags & (UE4Flags.CPF_ReferenceParm | UE4Flags.CPF_ReturnParm)):
		return '%s&' % prop_type
	elif prop_flags & UE4Flags.CPF_ReferenceParm:
		return 'const %s&' % prop_type
	elif param_info.get('is_pointer') or param_info.get('type') in ('ObjectProperty', 'ClassProperty'):
		if prop_flags & UE4Flags.CPF_ConstParm:
			return 'const %s*' % prop_type
		else:
			return '%s*' % prop_type
	else:
		return '%s' % prop_type

def get_param_cpp_str(context, param_info):
	# type: (GeneratorContext, dict) -> str
	param_str = '%s %s' % (get_param_cpp_type_str(context, param_info), param_info.get('name'))
	return param_str

def get_func_cpp_name(func_info):
	# type: (dict,) -> str
	return func_info['func_cpp_name'] if 'func_cpp_name' in func_info else func_info['pretty_name']

def check_func_need_generate(context, struct_info, func_info):
	# type: (GeneratorContext, dict, dict) -> bool
	is_override = False
	func_name = get_func_cpp_name(func_info)
	if func_name in IGNORE_FUNCS.get(struct_info['name'], []):
		return False, is_override
	super_struct_name = struct_info.get('super')
	if not super_struct_name:
		return True, is_override
	super_struct_info = context.structs_by_name.get(super_struct_name)
	if not super_struct_info:
		return True, is_override
	for super_func_info in super_struct_info['funcs']:
		if func_info['pretty_name'] == super_func_info['pretty_name']:
			is_override = True
			return True, is_override
	return True, is_override

def generate_one_doc(code_builder, doc_str):
	# type: (CodeBuilder, str) -> None
	doc_lines = doc_str.splitlines()
	if len(doc_lines) == 1:
		code_builder.add_line('// %s' % doc_str)
	else:
		code_builder.add_line('/**')
		for doc_line in doc_lines:
			code_builder.add_line('* %s' % doc_line)
		code_builder.add_line('*/')

def generate_one_prop(code_builder, context, prop_info):
	# type: (CodeBuilder, GeneratorContext, dict) -> None
	prop_flags = prop_info['prop_flags']
	if prop_flags & BindingGenerator.NO_EXPORT_PROP_FLAGS:
		return
	prop_info = PropHelpers.parse_prop_info(prop_info, context)
	prop_type = get_prop_cpp_type(context, prop_info)
	if 'unknow' in prop_type:
		return

	if 'doc' in prop_info:
		generate_one_doc(code_builder, prop_info['doc'])
	code_builder.add_line('UPROPERTY(%s)' % UE4Flags.explain_prop_flags(prop_flags, True))
	code_builder.add_line('%s %s;' % (prop_type, prop_info.get('name')))
	code_builder.add_line('')

def generate_one_func(code_builder, context, struct_info, func_info, is_override):
	# type: (CodeBuilder, GeneratorContext, dict, dict, bool) -> None
	func_flags = func_info['func_flags']
	if func_flags & BindingGenerator.NO_EXPORT_FUNC_FLAGS:
		print('Ignore func:', func_info['name'])
		return
	param_infos = []
	for param_info in func_info['params']:
		param_infos.append(PropHelpers.parse_prop_info(param_info, context))

	is_static = bool(func_flags & UE4Flags.FUNC_Static)
	is_friend_func = func_info.get('is_friend_func', False)
	params_str_list = []
	return_param_index = -1
	default_values = []
	if is_friend_func:
		params_str_list.append('const F%s& InSelf' % struct_info['name'])
	for index, param_info in enumerate(param_infos):
		prop_flags = param_info['prop_flags']
		if prop_flags & UE4Flags.CPF_ReturnParm:
			return_param_index = index
			continue
		param_str = get_param_cpp_str(context, param_info)
		if 'unknow' in param_str:
			return
		if 'default' in param_info:
			default_values.append('CPP_Default_%s = "%s"' % (param_info.get('name'), param_info['default']))
		params_str_list.append(param_str)
	# print(param_infos)
	# print(return_param_index)
	
	return_type_str = 'void'
	if return_param_index >= 0:
		return_type_str = get_param_cpp_type_str(context, param_infos[return_param_index])
	if 'unknow' in return_type_str:
		return

	cpp_func_name = get_func_cpp_name(func_info)
	# 由于重写等原因，函数名和父类重名了，修改之
	if is_override:
		cpp_func_name = '%s_%s' % (struct_info['name'], cpp_func_name)

	uht_str_list = []
	func_flags_str = UE4Flags.explain_func_flags(func_flags, True)
	if func_flags_str:
		uht_str_list.append(func_flags_str)
	meta_str_list = []
	meta_str_list.append('bFriendFunction = %s' % ('true' if is_friend_func else 'false'))
	if 'func_cpp_name' in func_info or cpp_func_name != func_info['pretty_name']:
		meta_str_list.append('ScriptName = "%s"' % func_info['pretty_name'])
	if cpp_func_name != func_info['name']:
		meta_str_list.append('OriginalName = "%s"' % func_info['name'])
	if default_values:
		meta_str_list.append(', '.join(default_values))
	meta_str = 'meta=(%s)' % (', '.join(meta_str_list))
	uht_str_list.append(meta_str)

	if 'doc' in func_info:
		generate_one_doc(code_builder, func_info['doc'])
	code_builder.add_line('UFUNCTION(%s)' % (', '.join(uht_str_list)))
	code_builder.add_line('%s%s %s(%s);' % ('static ' if is_static else '', return_type_str, cpp_func_name, ', '.join(params_str_list)))
	code_builder.add_line('')

def get_struct_super(struct_info):
	if not struct_info.get('super'):
		return 'UObject'
	return 'UNePyNoExportType_%s' % (struct_info.get('super'))

def generate_one_struct(code_builder, context, struct_info, structs_generated):
	# type: (CodeBuilder, GeneratorContext, dict, list) -> None
	if struct_info['name'] in structs_generated:
		return
	if struct_info.get('super') and struct_info.get('super') not in structs_generated:
		generate_one_struct(code_builder, context, context.structs_by_name[struct_info.get('super')], structs_generated)
	structs_generated.append(struct_info['name'])
	if 'doc' in struct_info:
		generate_one_doc(code_builder, struct_info['doc'])
	code_builder.add_line('UCLASS(meta=(Package = "%s"))' % struct_info.get('package'))
	code_builder.add_line('class UNePyNoExportType_%s : public %s' % (struct_info.get('name'), get_struct_super(struct_info)))
	code_builder.begin_block()
	code_builder.add_line('GENERATED_BODY()')
	code_builder.add_line('')
	code_builder.add_line('public:')
	for prop_info in struct_info.get('props', []):
		generate_one_prop(code_builder, context, prop_info)
	# 预处理函数名，如果有重名的，修改之。因为UFunction不支持
	func_name_cnt = {}
	for func_info in struct_info.get('funcs', []):
		func_name = func_info['pretty_name']
		if func_name in func_name_cnt:
			func_name_cnt[func_name] += 1
		else:
			func_name_cnt[func_name] = 1
	func_cpp_name_index = {}
	for func_info in struct_info.get('funcs', []):
		func_name = func_info['pretty_name']
		if func_name_cnt[func_name] > 1:
			cur_func_index = func_cpp_name_index.get(func_name, 1)
			func_info['func_cpp_name'] = '%s_Overload%s' % (func_name, cur_func_index)
			func_cpp_name_index[func_name] = cur_func_index + 1
	for func_info in struct_info.get('funcs', []):
		need_generate, is_override = check_func_need_generate(context, struct_info, func_info)
		if need_generate:
			generate_one_func(code_builder, context, struct_info, func_info, is_override)
	code_builder.dec_indent()
	code_builder.add_line('};')
	code_builder.add_line('')

def main():
	# 加载信息
	context = GeneratorContext()
	temp_dir = os.path.abspath('temp')
	reflection_infos_file_path = os.path.join(temp_dir, 'reflection_infos.json')
	reflection_infos_full_file_path = os.path.join(temp_dir, 'reflection_infos_full.json')
	source_infos_file_path = os.path.join(temp_dir, 'source_infos.json')
	clang_infos_file_path = os.path.join(temp_dir, 'clang_infos.json')
	BindingGenerator.load_reflection_infos(context, reflection_infos_file_path, reflection_infos_full_file_path)
	# BindingGenerator.load_reflection_infos(context, reflection_infos_full_file_path, True)
	BindingGenerator.load_source_infos(context, source_infos_file_path)
	BindingGenerator.load_clang_infos(context, clang_infos_file_path)
	# 先走一遍预处理
	for module_rule in ExportConfig.ExportModules:
		for package_rule in module_rule['packages']:
			BindingGenerator.preprocess_package_for_module(context, module_rule, package_rule)
	# 然后清理无用信息
	# BindingGenerator.cleanup_non_export_types(context)

	# 文件头
	code_builder = CodeBuilder()
	code_builder.add_line('#pragma once')
	code_builder.add_line('#include "CoreMinimal.h"')
	code_builder.add_line('#include "NePyNoExportTypes.generated.h"')
	code_builder.add_line()
	code_builder.add_line(u'// 此文件用于手动添加标注，给UHT提供元信息，以导出属性、函数信息')
	code_builder.add_line(u'// 编译时，请删除此文件')
	code_builder.add_line()
	code_builder.add_line('#if !CPP')
	code_builder.add_line()
	# 生成伪造的结构体类代码
	if not os.path.isfile(clang_infos_file_path):
		return
	with open(clang_infos_file_path, 'r') as f:
		clang_infos = json.load(f)
	structs_generated = []
	for clang_struct_info in sorted(clang_infos.get('structs', []), key=lambda info: info.get('name')):
		struct_name = clang_struct_info['name']
		if struct_name in context.structs_by_name:
			struct_info = context.structs_by_name[struct_name]
			generate_one_struct(code_builder, context, struct_info, structs_generated)
	# 文件尾
	code_builder.add_line('#endif')
	code_builder.add_line()
	# 写入文件
	code_str = code_builder.build()
	# print(code_str)
	target_dir = ExportConfig.NePyNoExportTypesFilePath
	if not os.path.exists(target_dir):
		os.makedirs(target_dir)
	open_and_write_str(os.path.join(ExportConfig.NePyNoExportTypesFilePath, "NePyNoExportTypes.h"), code_str)

if __name__ == '__main__':
	main()