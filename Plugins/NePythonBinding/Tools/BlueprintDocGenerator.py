# -*- encoding: utf-8 -*-
import os
import json
import codecs
import shutil
import time
import ExportConfig
from DocBuilder import DocBuilder
from FuncBuilder import FuncBuilder
from GeneratorContext import ExportContext
from version_helper import open_and_write_str
import PropHelpers

class BlueprintDocGenerator(object):
	"""
	从 blueprint_infos_full.json 读取蓝图信息，生成 blueprint_doc.pyi 文件
	"""
	def __init__(self):
		self.doc_builder = None # type: DocBuilder
		self.context = None # type: ExportContext
		self.blueprint_infos = None

	@staticmethod
	def _is_valid_python_identifier(name):
		# type: (str) -> bool
		"""
		检查名称是否是有效的 Python 标识符
		:param name: 要检查的名称
		:return: True 如果有效，False 如果无效
		"""
		if not name:
			return False
		
		# 检查是否以数字开头
		if name[0].isdigit():
			return False
		
		# 检查是否包含非ASCII字符（排除中文等非英文字符）
		try:
			name.encode('ascii')
		except UnicodeEncodeError:
			return False
		
		# 检查是否包含非法字符（Python 标识符只能包含字母、数字、下划线）
		import re
		if not re.match(r'^[a-zA-Z_][a-zA-Z0-9_]*$', name):
			return False
		
		# 检查是否是 Python 关键字
		import keyword
		if keyword.iskeyword(name):
			return False
		
		return True

	def generate_docs(self, context, blueprint_info_file_path):
		# type: (ExportContext, str) -> None
		"""
		生成蓝图文档
		:param context: 导出上下文
		:param blueprint_info_file_path: blueprint_infos_full.json 文件路径
		"""
		if not os.path.isfile(blueprint_info_file_path):
			print('Blueprint info file not found: %s' % blueprint_info_file_path)
			return

		self.context = context
		self._load_blueprint_infos(blueprint_info_file_path)

		if not self.blueprint_infos:
			print('No blueprint infos loaded')
			return

		# 创建 DocBuilder，它会自动添加基础头部
		self.doc_builder = DocBuilder(context)
		
		# 添加蓝图文档的特定信息
		self.doc_builder.add_line('"""')
		self.doc_builder.add_line('Blueprint Type Hints')
		self.doc_builder.add_line('Auto-generated from blueprint_infos_full.json')
		self.doc_builder.add_line('')
		# 添加生成时间
		import datetime
		gen_time = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
		self.doc_builder.add_line('Generated at: %s' % gen_time)
		self.doc_builder.add_line('"""')
		self.doc_builder.add_line('from . import *  # noqa')
		self.doc_builder.add_line()

		# 处理蓝图类
		blueprint_classes = self.blueprint_infos.get('classes', [])
		if blueprint_classes:
			self.doc_builder.add_line('# region Blueprint Classes')
			self.doc_builder.add_line()
			for bp_class_info in sorted(blueprint_classes, key=lambda x: x.get('name', '')):
				self._add_blueprint_class(bp_class_info)
			self.doc_builder.add_line('# endregion Blueprint Classes')
			self.doc_builder.add_line()

		# 处理蓝图结构体
		blueprint_structs = self.blueprint_infos.get('structs', [])
		if blueprint_structs:
			self.doc_builder.add_line('# region Blueprint Structs')
			self.doc_builder.add_line()
			for bp_struct_info in sorted(blueprint_structs, key=lambda x: x.get('name', '')):
				self._add_blueprint_struct(bp_struct_info)
			self.doc_builder.add_line('# endregion Blueprint Structs')
			self.doc_builder.add_line()

		# 处理蓝图枚举
		blueprint_enums = self.blueprint_infos.get('enums', [])
		if blueprint_enums:
			self.doc_builder.add_line('# region Blueprint Enums')
			self.doc_builder.add_line()
			for bp_enum_info in sorted(blueprint_enums, key=lambda x: x.get('name', '')):
				self._add_blueprint_enum(bp_enum_info)
			self.doc_builder.add_line('# endregion Blueprint Enums')
			self.doc_builder.add_line()

		self._build()

	def _load_blueprint_infos(self, blueprint_info_file_path):
		# type: (str) -> None
		"""
		加载 blueprint_infos_full.json 文件
		"""
		try:
			self.blueprint_infos = json.load(codecs.open(blueprint_info_file_path, 'r', 'utf-8-sig'))
			print('Loaded blueprint info from: %s' % blueprint_info_file_path)
		except Exception as e:
			try:
				self.blueprint_infos = json.load(codecs.open(blueprint_info_file_path, 'r', 'utf-8'))
				print('Loaded blueprint info from: %s' % blueprint_info_file_path)
			except Exception as e2:
				print('Failed to load blueprint info: %s' % str(e2))
				self.blueprint_infos = None

	def _add_blueprint_class(self, bp_class_info):
		# type: (dict) -> None
		"""
		添加蓝图类定义
		"""
		class_name = bp_class_info.get('name', 'UnknownClass')
		super_class = bp_class_info.get('super', 'Object')
		
		line = 'class %s(%s):' % (class_name, super_class)
		self.doc_builder.add_line(line)
		self.doc_builder.cb.inc_indent()

		doc = bp_class_info.get('doc', '')
		if doc:
			self.doc_builder.print_doc(doc, class_name)
			self.doc_builder.add_line()

		# 添加属性
		props = bp_class_info.get('props', [])
		if props:
			for prop_info in props:
				self._add_blueprint_property(prop_info)

		# 添加函数
		funcs = bp_class_info.get('funcs', [])
		if funcs:
			for func_info in funcs:
				self._add_blueprint_function(func_info)

		self.doc_builder.add_line('pass')
		self.doc_builder.cb.dec_indent()
		self.doc_builder.add_line()

	def _add_blueprint_struct(self, bp_struct_info):
		# type: (dict) -> None
		"""
		添加蓝图结构体定义
		"""
		struct_name = bp_struct_info.get('name', 'UnknownStruct')
		super_struct = bp_struct_info.get('super', 'StructBase')
		
		line = 'class %s(%s):' % (struct_name, super_struct)
		self.doc_builder.add_line(line)
		self.doc_builder.cb.inc_indent()

		doc = bp_struct_info.get('doc', '')
		if doc:
			self.doc_builder.print_doc(doc, struct_name)
			self.doc_builder.add_line()

		# 添加属性
		props = bp_struct_info.get('props', [])
		if props:
			for prop_info in props:
				self._add_blueprint_property(prop_info)

		# 添加函数
		funcs = bp_struct_info.get('funcs', [])
		if funcs:
			for func_info in funcs:
				self._add_blueprint_function(func_info)

		self.doc_builder.add_line('pass')
		self.doc_builder.cb.dec_indent()
		self.doc_builder.add_line()

	def _add_blueprint_enum(self, bp_enum_info):
		# type: (dict) -> None
		"""
		添加蓝图枚举定义
		"""
		enum_name = bp_enum_info.get('name', 'UnknownEnum')
		
		line = 'class %s(EnumBase):' % enum_name
		self.doc_builder.add_line(line)
		self.doc_builder.cb.inc_indent()

		doc = bp_enum_info.get('doc', '')
		if doc:
			self.doc_builder.print_doc(doc, enum_name)
			self.doc_builder.add_line()

		# 添加枚举项
		items = bp_enum_info.get('items', [])
		if items:
			for item_info in items:
				item_name = item_info.get('name', 'UNKNOWN')
				item_value = item_info.get('value', 0)
				item_doc = item_info.get('doc', '')
				
				# 检查枚举项名称是否有效
				if not self._is_valid_python_identifier(item_name):
					# 无效的枚举项，用注释形式展示
					self.doc_builder.add_line('# %s = %s  # [Unsupported] Invalid Python identifier, not supported in script' % (item_name, item_value))
					if item_doc:
						self.doc_builder.add_line('# """ %s """' % item_doc)
					self.doc_builder.add_line()
					continue
				
				line = '%s = %s' % (item_name, item_value)
				self.doc_builder.add_line(line)
				if item_doc:
					self.doc_builder.print_doc(item_doc, item_name)
					self.doc_builder.add_line()

		self.doc_builder.add_line('pass')
		self.doc_builder.cb.dec_indent()
		self.doc_builder.add_line()

	def _add_blueprint_property(self, prop_info):
		# type: (dict) -> None
		"""
		添加蓝图属性定义
		"""
		prop_name = prop_info.get('name', 'unknown_prop')
		prop_type = prop_info.get('type', 'typing.Any')
		prop_doc = prop_info.get('doc', '')

		# 检查属性名是否有效
		if not self._is_valid_python_identifier(prop_name):
			# 无效的属性名，用注释形式展示
			self.doc_builder.add_line('# %s: %s  # [Unsupported] Invalid Python identifier, not supported in script' % (prop_name, prop_type))
			if prop_doc:
				self.doc_builder.add_line('# """ %s """' % prop_doc)
			self.doc_builder.add_line()
			return

		line = '%s: %s' % (prop_name, prop_type)
		self.doc_builder.add_line(line)
		if prop_doc:
			self.doc_builder.print_doc(prop_doc, prop_name)
		self.doc_builder.add_line()

	def _add_blueprint_function(self, func_info):
		# type: (dict) -> None
		"""
		添加蓝图函数定义
		"""
		func_name = func_info.get('name', 'unknown_func')
		is_static = func_info.get('is_static', False)
		params = func_info.get('params', [])
		return_type = func_info.get('return_type', 'None')
		func_doc = func_info.get('doc', '')

		# 检查函数名是否有效
		if not self._is_valid_python_identifier(func_name):
			# 无效的函数名，用注释形式展示
			self.doc_builder.add_line('# def %s(...) -> %s:  # [Unsupported] Invalid Python identifier, not supported in script' % (func_name, return_type))
			if func_doc:
				self.doc_builder.add_line('#     """ %s """' % func_doc)
			self.doc_builder.add_line('#     pass')
			self.doc_builder.add_line()
			return

		# 构建参数列表
		param_list = []
		invalid_params = []
		if not is_static:
			param_list.append('self')

		has_invalid_param = False
		for param_info in params:
			param_name = param_info.get('name', 'param')
			param_type = param_info.get('type', 'typing.Any')
			param_default = param_info.get('default', None)
			
			# 检查参数名是否有效
			if not self._is_valid_python_identifier(param_name):
				has_invalid_param = True
				invalid_params.append(param_name)
				# 用占位符替换无效的参数名
				param_name = 'invalid_param_%d' % len(invalid_params)
			
			param_str = '%s: %s' % (param_name, param_type)
			if param_default is not None:
				param_str += ' = %s' % param_default
			param_list.append(param_str)

		# 如果有无效参数，整个函数用注释形式展示
		if has_invalid_param:
			self.doc_builder.add_line('# def %s(%s) -> %s:  # [Unsupported] Contains invalid parameter names, not supported in script' % (func_name, ', '.join([p.split(':')[0] if ':' in p else p for p in param_list]), return_type))
			if func_doc:
				self.doc_builder.add_line('#     """ %s """' % func_doc)
			self.doc_builder.add_line('#     # Invalid parameters: %s' % ', '.join(invalid_params))
			self.doc_builder.add_line('#     pass')
			self.doc_builder.add_line()
			return

		# 添加装饰器
		if is_static:
			self.doc_builder.add_line('@staticmethod')

		# 构建函数签名
		param_str = ', '.join(param_list)
		line = 'def %s(%s) -> %s:' % (func_name, param_str, return_type)
		self.doc_builder.add_line(line)
		self.doc_builder.cb.inc_indent()
		
		if func_doc:
			self.doc_builder.print_doc(func_doc, func_name)
		self.doc_builder.add_line('pass')
		self.doc_builder.cb.dec_indent()
		self.doc_builder.add_line()

	def _build(self):
		"""
		构建并保存 blueprint_doc.pyi 文件
		"""
		gen_path = ExportConfig.PythonStubDir
		output_dir = os.path.join(gen_path, ExportConfig.RootModuleName)
		
		# 确保输出目录存在
		if not os.path.isdir(output_dir):
			os.makedirs(output_dir)

		output_path = os.path.join(output_dir, 'blueprint_doc.pyi')
		
		data = self.doc_builder.build()
		open_and_write_str(output_path, data)
		
		print('Blueprint doc generated: %s' % output_path)
