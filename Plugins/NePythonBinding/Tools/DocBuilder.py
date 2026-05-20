# -*- encoding: utf-8 -*-
import os
import re
import UE4Flags
import ExportConfig
import PropHelpers
from GeneratorContext import GeneratorContext # noqa
from CodeBuilder import CodeBuilder
from FuncBuilder import FuncBuilder
from OverloadFuncBuilder import OverloadFuncBuilder
from version_helper import open_and_read_str
import OperatorOverloadHelper

try:
	import typing # noqa
except:
	pass

class DocBuilder(object):
	def __init__(self, context):
		# type: (GeneratorContext) -> None
		self.context = context
		self.cb = CodeBuilder(ExportConfig.PythonDocIndentChars)
		self.cb.add_line('# -*- encoding: utf-8 -*-')
		self.cb.add_line('from __future__ import annotations')
		self.cb.add_line('import typing')
		self.cb.add_line('T = typing.TypeVar(\'T\')')
		self.cb.add_line()

		# 由begin_type设置
		self.type_info = None # type: dict

	def begin_type(self, type_info):
		# type: (dict) -> None
		self.type_info = type_info

		super_class = type_info.get('super')
		if super_class is None:
			if 'class_flags' in type_info:
				super_class = 'object'
			else:
				super_class = 'StructBase'
		interfaces = type_info.get('interfaces')
		if interfaces:
			for interface_info in interfaces:
				super_class += ', ' + interface_info['name']
		line = 'class %s(%s):' % (type_info['name'], super_class)
		if interfaces:
			line += ' # type: ignore' # 避免Pylance报告关于mro的错误
		self.cb.add_line(line)
		self.cb.inc_indent()

		doc = type_info.get('doc')
		if self.print_doc(doc, type_info['name']):
			self.cb.add_line()

	def end_type(self):
		# self.process_manual_code()

		type_name = self.type_info['name']
		if type_name in ('Object', 'StructBase'):
			# 派生Object和StructBase的对象能通过动态反射获取未导出的对象
			self.cb.add_line('def __getattr__(self, key: str) -> typing.Any:')
			self.cb.add_line('%spass' % self.cb.get_indent_chars())
			self.cb.add_line()
			self.cb.add_line('def __setattr__(self, key: str, value) -> None:')
			self.cb.add_line('%spass' % self.cb.get_indent_chars())
			self.cb.add_line()

		self.cb.add_line('pass')
		self.cb.dec_indent()
		self.cb.add_line()

		self.type_info = None

	def add_func(self, func_builder):
		# type: (FuncBuilder|OverloadFuncBuilder) -> None
		func_name = func_builder.func_name
		if func_name in self.type_info.get('manual_func_names', []):
			return
		if isinstance(func_builder, OverloadFuncBuilder):
			for func_info in func_builder.overload_func_info['overloads']:
				if func_info['func_flags'] & func_builder.no_export_flags:
					continue
				sub_func_builder = FuncBuilder(func_info, func_builder.context, func_builder.for_struct, func_builder.outer_type_info)
				sub_func_builder.is_overload_func = True
				sub_func_builder.func_name = func_name
				self._do_add_func(sub_func_builder)
		else:
			self._do_add_func(func_builder)

		# 一些函数在py3中的名字和py2不一样，这里需要特殊处理一下
		operator_info = OperatorOverloadHelper.OPERATOR_BY_PY_NAME.get(func_name)
		if operator_info:
			py3_func_name = operator_info.get('py3_name')
			if py3_func_name:
				func_builder.func_name = py3_func_name
				self.add_func(func_builder)
				func_builder.func_name = func_name

	def _do_add_func(self, func_builder):
		# type: (FuncBuilder) -> None
		input_params = []
		output_params = []
		generic_input_param_name, generic_output_param_name = self._parse_generic_param_names(func_builder)
		generic_bound_type = None

		if not func_builder.is_static:
			input_params.append('self')

		for index in func_builder.input_param_indices:
			param_info = func_builder.param_infos[index]
			param_name = str(param_info['name'])
			if param_info['type_info']:
				if param_name == generic_input_param_name:
					type_name = self._get_func_generic_type_name(param_info)
					generic_bound_type = self._get_func_generic_bound_type_name(param_info)
				else:
					type_name = self._get_func_type_name(param_info)
				if func_builder.is_native and (PropHelpers.is_allow_none_type(param_info) or param_info.get('default') == 'None'):
					# 在没有开启可None参数功能的情况下，静态生成的代码仍然支持对默认值为 nullptr 的参数传递 None
					type_name = '%s | None' % type_name
				param_name = '%s: %s' % (param_name, type_name)
			if 'default' in param_info:
				param_name += ' = %s' % PropHelpers.parse_py_default(param_info)
			input_params.append(param_name)

		if func_builder.return_param_index >= 0:
			param_info = func_builder.param_infos[func_builder.return_param_index]
			param_name = str(param_info['name'])
			if param_info['type_info']:
				if generic_input_param_name and generic_bound_type and param_name == generic_output_param_name:
					type_name = self._get_func_generic_type_name(param_info)
				else:
					type_name = self._get_func_type_name(param_info)
			else:
				type_name = 'typing.Any'
			output_params.append(type_name)

		for index in func_builder.output_param_indices:
			param_info = func_builder.param_infos[index]
			param_name = str(param_info['name'])
			if param_info['type_info']:
				if generic_input_param_name and generic_bound_type and param_name == generic_output_param_name:
					type_name = self._get_func_generic_type_name(param_info)
				else:
					type_name = self._get_func_type_name(param_info)
			else:
				type_name = 'typing.Any'
			output_params.append(type_name)

		if output_params:
			if len(output_params) > 1:
				output_params = ' -> tuple[%s]' % ', '.join(output_params)
			else:
				output_params = ' -> %s' % output_params[0]
		else:
			output_params = '-> None'

		if not func_builder.is_native:
			self.cb.add_line('@typing.type_check_only')

		if func_builder.is_static:
			self.cb.add_line('@staticmethod')

		if func_builder.is_overload_func:
			self.cb.add_line('@typing.overload')

		func_name = func_builder.func_name
		if func_builder.is_constructor:
			func_name = '__init__'

		if generic_input_param_name and generic_bound_type:
			line = 'def %s[T: %s](%s)%s:' % (func_name, generic_bound_type, ', '.join(input_params), output_params)
		else:
			line = 'def %s(%s)%s:' % (func_name, ', '.join(input_params), output_params)
		self.cb.add_line(line)
		self.cb.inc_indent()
		doc = func_builder.func_info.get('doc')
		self.print_doc(doc, func_name)
		self.cb.add_line('pass')
		self.cb.dec_indent()
		self.cb.add_line()

	def _parse_generic_param_names(self, func_builder):
		# type: (FuncBuilder) -> tuple[str, str]

		# 首先从meta_data里提取
		generic_input_param_name = func_builder.func_info.get('meta_data', {}).get('DeterminesOutputType', '')
		generic_output_param_name = func_builder.func_info.get('meta_data', {}).get('DynamicOutputParam', 'ReturnValue')
		if generic_input_param_name:
			return generic_input_param_name, generic_output_param_name

		# meta_data里没有，我们靠如下规则进行判定：
		# 输入参数中有且只有一个TSubclassOf[T]，且输出类型为T*

		subclass_of_param_infos = [] # type: list[tuple[str, str]]
		for index in func_builder.input_param_indices:
			param_info = func_builder.param_infos[index]
			if param_info['type'] == 'ClassProperty':
				if (param_info['prop_flags'] & UE4Flags.CPF_UObjectWrapper) and ('meta_class_name' in param_info):
					subclass_of_param_infos.append((param_info['name'], param_info['meta_class_name']))

		if len(subclass_of_param_infos) == 1:
			generic_input_param_name = subclass_of_param_infos[0][0]
			determines_output_type = subclass_of_param_infos[0][1]

			if func_builder.return_param_index >= 0:
				param_info = func_builder.param_infos[func_builder.return_param_index]
				if param_info['type'] == 'ObjectProperty' and param_info['class_name'] == determines_output_type:
					return generic_input_param_name, param_info['name']

			for index in func_builder.output_param_indices:
				param_info = func_builder.param_infos[index]
				if param_info['type'] == 'ObjectProperty' and param_info['class_name'] == determines_output_type:
					return generic_input_param_name, param_info['name']

		return '', ''

	def add_prop(self, prop_info):
		# type: (dict[str, typing.Any]) -> None
		prop_name = prop_info['name']
		if prop_info['name'] in ('None', 'True', 'False'):
			return

		if prop_info['type_info']:
			type_name = self._get_prop_type_name(prop_info)
			# 不给 Property 加上 Optional 标记，因为 property 可以作为返回值，Pylance 经常会报 reportOptionalMemberAccess 的 error.
			# if PropHelpers.is_allow_none_type(prop_info):
			# 	type_name = '%s | None' % type_name
		else:
			type_name = 'typing.Any'

		self.cb.add_line('%s: %s' % (prop_name, type_name))
		doc = prop_info.get('doc')
		self.print_doc(doc, prop_name)
		self.cb.add_line()

	def _get_func_type_name(self, prop_info):
		# type: (dict) -> str
		type_info = prop_info['type_info']
		type_name = type_info.get('py_name', type_info.get('name', ''))
		return self._do_get_type_name(type_name, prop_info, False)

	def _get_func_generic_type_name(self, prop_info):
		# type: (dict) -> str
		type_info = prop_info['type_info']
		type_name = type_info.get('py_name', type_info.get('name', ''))
		return self._get_generic_type_name(type_name, prop_info, False)

	def _get_func_generic_bound_type_name(self, prop_info):
		# type: (dict) -> str
		type_info = prop_info['type_info']
		type_name = type_info.get('py_name', type_info.get('name', ''))
		return self._get_generic_bound_type(type_name, prop_info, False)

	def _get_prop_type_name(self, prop_info):
		# type: (dict) -> str
		type_info = prop_info['type_info']
		type_name = type_info.get('py_name', type_info.get('name', ''))
		if prop_info['array_dim'] > 1 and type_name.startswith('list('):
			type_name = 'fixed_' + type_name
		type_name = self._do_get_type_name(type_name, prop_info, True)
		if type_name == 'bytes':
			# 对于属性而言，不会把TArray<uint8>映射为bytes
			type_name = self._do_get_type_name('list(int)', prop_info, True)
		return type_name

	def _do_get_type_name(self, type_name, prop_info, for_prop):
		# type: (str, dict, bool) -> str
		if type_name.startswith('list('):
			inner_type_name = self._do_get_type_name(type_name[5:-1], prop_info['inner_prop'], False)
			if for_prop:
				type_name = 'ArrayWrapper[%s]' % inner_type_name
			else:
				type_name = 'list[%s]' % inner_type_name
		elif type_name.startswith('set('):
			inner_type_name = self._do_get_type_name(type_name[4:-1], prop_info['element_prop'], False)
			if for_prop:
				type_name = 'SetWrapper[%s]' % inner_type_name
			else:
				type_name = 'set[%s]' % inner_type_name
		elif type_name.startswith('dict('):
			pairs = type_name[5:-1].split(', ')
			inner_key_name = self._do_get_type_name(pairs[0], prop_info['key_prop'], False)
			inner_val_name = self._do_get_type_name(pairs[1], prop_info['value_prop'], False)
			if for_prop:
				type_name = 'MapWrapper[%s, %s]' % (inner_key_name, inner_val_name)
			else:
				type_name = 'dict[%s, %s]' % (inner_key_name, inner_val_name)
		elif type_name.startswith('fixed_list('):
			inner_type_name = self._do_get_type_name(type_name[11:-1], prop_info['inner_prop'], False)
			if for_prop:
				type_name = 'FixedArrayWrapper[%s]' % inner_type_name
			else:
				type_name = 'list[%s]' % inner_type_name
		elif type_name == 'Callable':
			type_info = prop_info['type_info']
			if for_prop:
				py_wrapper = type_info['py_wrapper']
				type_name = self._get_callable_signature(type_info['func_info'], py_wrapper)
			else:
				type_name = self._get_callable_signature(type_info['func_info'])
		elif prop_info['type'] == 'ClassProperty':
			if (prop_info['prop_flags'] & UE4Flags.CPF_UObjectWrapper) and ('meta_class_name' in prop_info):
				inner_type_name = prop_info['meta_class_name']
				if (prop_info['prop_flags'] & UE4Flags.CPF_Parm) and not (prop_info['prop_flags'] & UE4Flags.CPF_OutParm):
					type_name = 'TSubclassOf[%s] | type[%s]' % (inner_type_name, inner_type_name)
				else:
					type_name = 'TSubclassOf[%s]' % inner_type_name
			else:
				type_name = prop_info['class_name']
		elif prop_info['type'] == 'SoftClassProperty':
			if (prop_info['prop_flags'] & UE4Flags.CPF_UObjectWrapper) and ('meta_class_name' in prop_info):
				inner_type_name = prop_info['meta_class_name']
				type_name = 'TSoftClassPtr[%s]' % inner_type_name
			else:
				type_name = prop_info['class_name']
		elif prop_info['type'] == 'SoftObjectProperty':
			type_name = 'TSoftObjectPtr[%s]' % prop_info['class_name']
		elif prop_info['type'] == 'WeakObjectProperty':
			type_name = 'TWeakObjectPtr[%s]' % prop_info['class_name']
		elif 'enum_name' in prop_info:
			return prop_info['enum_name']
		elif type_name.startswith('FNePy'):
			type_name = self._strip_fnepy(type_name)
		elif type_name == 'unsupported':
			type_name = 'typing.Any'

		return type_name

	def _get_generic_type_name(self, type_name, prop_info, for_prop=True):
		# type: (str, dict, bool) -> str
		if type_name.startswith('list('):
			if for_prop:
				type_name = 'ArrayWrapper[T]'
			else:
				type_name = 'list[T]'
		# elif type_name.startswith('dict('):
		# 	if for_prop:
		# 		type_name = 'MapWrapper[KT, VT]'
		# 	else:
		# 		type_name = 'dict[KT, VT]'
		elif type_name.startswith('set('):
			if for_prop:
				type_name = 'SetWrapper[T]'
			else:
				type_name = 'set[T]'
		elif type_name.startswith('fixed_list('):
			if for_prop:
				type_name = 'FixedArrayWrapper[T]'
			else:
				type_name = 'list[T]'
		elif prop_info['type'] == 'ClassProperty':
			if prop_info['prop_flags'] & UE4Flags.CPF_UObjectWrapper and 'meta_class_name' in prop_info:
				if (prop_info['prop_flags'] & UE4Flags.CPF_Parm) and not (prop_info['prop_flags'] & UE4Flags.CPF_OutParm):
					type_name = 'TSubclassOf[T] | type[T]'
				else:
					type_name = 'TSubclassOf[T]'
			else:
				type_name = 'T'
		elif prop_info['type'] == 'SoftClassProperty':
			if (prop_info['prop_flags'] & UE4Flags.CPF_UObjectWrapper) and ('meta_class_name' in prop_info):
				type_name = 'TSoftClassPtr[T]'
			else:
				type_name = 'T'
		elif prop_info['type'] == 'SoftObjectProperty':
			type_name = 'TSoftObjectPtr[T]'
		elif prop_info['type'] == 'WeakObjectProperty':
			type_name = 'TWeakObjectPtr[T]'
		else:
			type_name = 'T'

		return type_name

	def _get_generic_bound_type(self, type_name, prop_info, for_prop=True):
		# type: (str, dict, bool) -> str
		bound_name = ""
		if type_name.startswith('list('):
			bound_name = self._do_get_type_name(type_name[5:-1], prop_info['inner_prop'], False)
		elif type_name.startswith('set('):
			bound_name = self._do_get_type_name(type_name[4:-1], prop_info['element_prop'], False)
		# elif type_name.startswith('dict('):
		# 	pairs = type_name[5:-1].split(', ')
		# 	inner_key_name = self._do_get_type_name(pairs[0], prop_info['key_prop'], False)
		# 	inner_val_name = self._do_get_type_name(pairs[1], prop_info['value_prop'], False)
		# 	bound_name = inner_key_name, inner_val_name
		elif type_name.startswith('fixed_list('):
			bound_name = self._do_get_type_name(type_name[11:-1], prop_info['inner_prop'], False)
		elif prop_info['type'] in ('ClassProperty', 'SoftClassProperty'):
			if prop_info['prop_flags'] & UE4Flags.CPF_UObjectWrapper and 'meta_class_name' in prop_info:
				bound_name = prop_info['meta_class_name']
		elif prop_info['type'] == 'SoftObjectProperty':
			bound_name = prop_info['class_name']
		else:
			bound_name = self._do_get_type_name(type_name, prop_info, for_prop)
		return bound_name

	def _strip_fnepy(self, type_name):
		# type: (str) -> str
		index = type_name.find('_')
		if index >= 0:
			# strip FNePyStruct_ or FNePyObject_
			return type_name[index + 1:]
		# strip FNePy
		return type_name[5:]

	def _get_callable_signature(self, func_info, callable_type='typing.Callable'):
		# type: (dict, str) -> str
		outer_type_info = self.type_info
		for_struct = 'struct_flags' in outer_type_info
		func_builder = FuncBuilder(func_info, self.context, for_struct, outer_type_info)

		input_params = []
		output_params = []

		def process_params(indices, output):
			# type: (list[int], list) -> None
			for index in indices:
				if index == -1:
					continue
				param_info = func_builder.param_infos[index]
				if param_info['type_info']:
					type_name = self._get_func_type_name(param_info)
				else:
					type_name = 'unsupported'
				output.append(type_name)

		process_params(func_builder.input_param_indices, input_params)
		process_params([func_builder.return_param_index], output_params)
		process_params(func_builder.output_param_indices, output_params)

		input_params = ', '.join(input_params)
		if output_params:
			if len(output_params) > 1:
				output_params = 'tuple[%s]' % ', '.join(output_params)
			else:
				output_params = output_params[0]
		else:
			output_params = 'None'

		func_signature = '%s[[%s], %s]' % (callable_type, input_params, output_params)
		return func_signature

	def process_manual_code(self):
		manual_code_path = self.type_info.get('manual_code_path')
		self._do_process_manual_code(manual_code_path)

		type_name = self.type_info['name']
		for manual_code_path in ExportConfig.PyStubExtraParseFiles.get(type_name, []):
			self._do_process_manual_code(manual_code_path)

	def _do_process_manual_code(self, manual_code_path):
		# type: (str) -> None
		if not manual_code_path or not os.path.isfile(manual_code_path):
			return

		manual_func_names = set()

		# need to be compatible with py2 and py3
		data = open_and_read_str(manual_code_path)
		pat = re.compile(r'{\s*("\w+?",\s*?(?:NePyCFunctionCast|\(PyCFunction).*)}')
		for result in pat.findall(data):
			# ['"GetClass"', 'NePyCFunctionCast(&NePyObject_GetClass)', 'METH_NOARGS', '"() -> Class"']
			infos = result.split(',')
			if len(infos) < 4:
				continue
			if len(infos) > 4:
				infos = infos[:3] + [','.join(infos[3:]), ]
			infos = [i.strip() for i in infos]
			self._add_manual_func(infos[0], infos[2], infos[3])
			func_name = infos[0].strip('"').strip()
			manual_func_names.add(func_name)

		if self.type_info:
			self.type_info['manual_func_names'] = manual_func_names

	def _add_manual_func(self, func_name, func_flags, func_signature):
		# type: (str, str, str) -> None
		if 'METH_STATIC' in func_flags:
			self.cb.add_line('@staticmethod')
		elif 'METH_CLASS' in func_flags:
			self.cb.add_line('@classmethod')

		func_name = func_name.strip('"').strip()
		func_signature = func_signature.strip('"').strip()
		if func_signature == 'WithoutStub':
			# 加一个特殊标识，不生成任何代码提示，用于手写提示
			return
		func_signatures = func_signature.split(';')
		for func_sign in func_signatures:
			if len(func_signatures) > 1:
				self.cb.add_line('@typing.overload')
			if func_sign and self._is_valid_func_signature(func_sign):
				line = 'def %s%s:' % (func_name, func_sign)
			else:
				line = 'def %s(*args) -> typing.Any:' % (func_name)
			self.cb.add_line(line)
			self.cb.add_line('%spass' % self.cb.get_indent_chars())
			self.cb.add_line()

	def _is_valid_func_signature(self, func_signature):
		# type: (str) -> bool
		pat = re.compile(r'^\[?.*?\]?\s*\(.*\)?\s*->\s*.+$')
		return bool(pat.match(func_signature))

	def add_enum(self, enum_info):
		# type: (dict) -> None
		line = 'class %s(EnumBase):' % enum_info['name']
		self.cb.add_line(line)
		self.cb.inc_indent()

		doc = enum_info.get('doc')
		if self.print_doc(doc, enum_info['name']):
			self.cb.add_line()

		item_pairs = enum_info['pairs']
		item_docs = enum_info.get('item_docs')
		for item_idx, (item_name, item_val) in enumerate(item_pairs):
			item_name = PropHelpers.repl_enum_item_name(item_name)
			if item_name.endswith('_MAX'):
				continue
			line = '%s = %s' % (item_name, item_val)
			self.cb.add_line(line)
			if item_docs:
				doc_lines = item_docs[item_idx]
				self.print_doc(doc_lines, item_name)
				self.cb.add_line()
		self.cb.dec_indent()
		self.cb.add_line()

	def add_line(self, line=None):
		# type: (str|None) -> None
		self.cb.add_line(line)

	def build(self):
		return self.cb.build()

	def print_doc(self, doc, field_name=None):
		# type: (str, str | None) -> None
		if not doc:
			return False

		doc_lines = doc.splitlines()
		if len(doc_lines) > 1:
			self.cb.add_line('""" %s' % doc_lines[0])
			for doc_line in doc_lines[1:]:
				self.cb.add_line(doc_line)
			self.cb.add_line('"""  # noqa')
		else:
			doc = doc.strip()
			if not doc or doc == field_name:
				return False
			line = '""" %s """  # noqa' % doc
			self.cb.add_line(line)

		return True
