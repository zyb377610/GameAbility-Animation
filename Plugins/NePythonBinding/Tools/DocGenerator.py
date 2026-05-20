# -*- encoding: utf-8 -*-
import os
import time
import shutil
import ExportConfig
import PropHelpers
from GeneratorContext import GeneratorContext, ExportContext
from DocBuilder import DocBuilder
from FuncBuilder import FuncBuilder
from OverloadFuncBuilder import OverloadFuncBuilder
from version_helper import open_and_write_str, open_and_read_str, str_decode

class DocGenerator(object):
	def __init__(self):
		# 当前模块的DocBuilder
		self.doc_builder = None # type: DocBuilder
		self.context = None # type: ExportContext
		self.sorted_enum_names = []
		self.sorted_struct_names = []
		self.sorted_class_names = []


	def generate_docs(self, context):
		# type: (ExportContext) -> None

		if not context.has_gen_difference():
			return

		self.context = context
		self.sorted_enum_names = sorted(context.current_gen.enums_by_name.keys())
		self._sort_types('StructBase', context.current_gen.structs_by_name, self.sorted_struct_names)
		self._sort_types('Object', context.current_gen.classes_by_name, self.sorted_class_names)

		self.begin_root_module(context)

		# enums
		self.doc_builder.add_line('# region enums')
		self.doc_builder.add_line()
		for enum_name in self.sorted_enum_names:
			enum_info = context.current_gen.enums_by_name[enum_name]
			self.add_enum(enum_info)
		self.doc_builder.add_line('# endregion enums')
		self.doc_builder.add_line()

		# structs
		self.doc_builder.add_line('# region structs')
		self.doc_builder.add_line()
		for struct_name in self.sorted_struct_names:
			struct_info = context.current_gen.structs_by_name.get(struct_name)
			if not struct_info:
				print('not struct info: %s' % struct_name)
				continue
			self.begin_type(struct_info)
			for prop_info in struct_info.get('props', []):
				prop_info = PropHelpers.parse_prop_info(prop_info, context)
				self.add_prop(prop_info)
			self.process_manual_code()
			for func_info in struct_info.get('funcs', []):
				if 'overloads' not in func_info:
					func_builder = FuncBuilder(func_info, context, True, struct_info)
				else:
					func_builder = OverloadFuncBuilder(func_info, context, True, struct_info)
				self.add_func(func_builder)
			self.end_type()
		self.doc_builder.add_line('# endregion structs')
		self.doc_builder.add_line()

		# classes
		self.doc_builder.add_line('# region classes')
		self.doc_builder.add_line()
		for class_name in self.sorted_class_names:
			class_info = context.current_gen.classes_by_name.get(class_name)
			if not class_info:
				print('not class info: %s' % class_name)
				continue
			self.begin_type(class_info)
			for prop_info in class_info.get('props', []):
				prop_info = PropHelpers.parse_prop_info(prop_info, context)
				self.add_prop(prop_info)
			self.process_manual_code()
			for func_info in class_info.get('funcs', []):
				if 'overloads' not in func_info:
					func_builder = FuncBuilder(func_info, context, False, class_info)
				else:
					func_builder = OverloadFuncBuilder(func_info, context, False, class_info)
				self.add_func(func_builder)
			self.end_type()
		self.doc_builder.add_line('# endregion classes')
		self.doc_builder.add_line()

		# self.add_uproperty_classes()

		self.end_root_module()

		self.add_builtin_doc()

		self.add_blueprint_doc()

		self.build()

	# 将类型按照基类/子类的顺序排序
	def _sort_types(self, root_type_name, type_info_dict, sorted_names):
		children_dict = {root_type_name: []}
		for type_name in type_info_dict:
			children_dict[type_name] = []
		for type_name, type_info in type_info_dict.items():
			if type_name == root_type_name:
				continue
			super_type_name = type_info.get('super', root_type_name)
			children_dict.setdefault(super_type_name, []).append(type_name)

		processed = set()
		def process(type_name):
			if type_name in processed:
				return
			processed.add(type_name)
			sorted_names.append(type_name)
			children_list = children_dict[type_name]
			children_list.sort()
			for child_type_name in children_list:
				process(child_type_name)

		process(root_type_name)
	

	def begin_root_module(self, context):
		# type: (GeneratorContext) -> None
		assert self.doc_builder is None
		self.doc_builder = DocBuilder(context)

	def end_root_module(self):
		assert self.doc_builder is not None
		self.doc_builder.add_line('# region top module')
		self.doc_builder.add_line()
		code_path = os.path.join(ExportConfig.EngineBindingGeneratePath, '..', '..', '..', 'Private', 'NePyTopModuleMethods.cpp')
		self.doc_builder._do_process_manual_code(code_path)
		# hard code 几个全局属性
		self.doc_builder.add_line(u'UE_SHIPPING: bool # 是否是Shipping包')
		self.doc_builder.add_line('')
		self.doc_builder.add_line(u'GIsEditor: bool # 是否正在运行编辑器')
		self.doc_builder.add_line(u'GIsClient: bool # 是否正在运行客户端')
		self.doc_builder.add_line(u'GIsServer: bool # 是否正在运行服务器')
		self.doc_builder.add_line('# endregion top module')
		self.doc_builder.add_line()

	def begin_type(self, type_info):
		# type: (dict) -> None
		self.doc_builder.begin_type(type_info)

	def process_manual_code(self):
		self.doc_builder.process_manual_code()

	def end_type(self):
		self.doc_builder.end_type()

	def add_func(self, func_builder):
		# type: (FuncBuilder) -> None
		self.doc_builder.add_func(func_builder)

	def add_prop(self, prop_info):
		# type: (dict) -> None
		self.doc_builder.add_prop(prop_info)

	def add_static_class(self):
		self.doc_builder.add_static_class()

	def add_enum(self, enum_info):
		# type: (dict) -> None
		self.doc_builder.add_enum(enum_info)

	def add_uproperty_classes(self):
		# type: () -> None
		self.doc_builder.add_line('# region uproperties')
		self.doc_builder.add_line()
		prop_class_names = list(PropHelpers.BUILTIN_TYPES.keys()) + list(PropHelpers.STRING_TYPES.keys())
		for class_name in sorted(prop_class_names):
			line = '%s: Class' % class_name
			self.add_line(line)
			self.add_line('')
		self.doc_builder.add_line('# endregion uproperties')
		self.doc_builder.add_line()

	def add_line(self, line):
		# type: (str) -> None
		self.doc_builder.add_line(line)

	def add_builtin_doc(self):
		# type: () -> None
		self.doc_builder.add_line('# region builtin')
		self.doc_builder.add_line('from .builtin_doc import * # noqa')
		self.doc_builder.add_line('# endregion builtin')
		self.doc_builder.add_line()
	
	def add_blueprint_doc(self):
		# type: () -> None
		self.doc_builder.add_line('# region blueprint')
		self.doc_builder.add_line('from .blueprint_doc import * # noqa')
		self.doc_builder.add_line('# endregion blueprint')
		self.doc_builder.add_line()

	def build(self):
		gen_path = ExportConfig.PythonStubDir

		output_dir = os.path.join(gen_path, ExportConfig.RootModuleName)
		if os.path.isdir(output_dir):
			shutil.rmtree(output_dir, ignore_errors=True)
			time.sleep(0.1)  # rmtree好像要稍微等一下，不然容易出错。。
		os.makedirs(output_dir)
		output_path = os.path.join(output_dir, '__init__.pyi')

		data = self.doc_builder.build()
		# need to be compatible with py2 and py3
		open_and_write_str(output_path, data)

		shutil.copy('templates/builtin_doc.pyi', os.path.join(output_dir, 'builtin_doc.pyi'))
		shutil.copy('templates/internal.pyi', os.path.join(output_dir, 'internal.pyi'))
