# -*- encoding: utf-8 -*-
import UE4Flags
from FuncBuilder import FuncBuilder
import DebugLog as dlog
from FuncBuilderBase import FuncBuilderBase
from CodeBuilder import CodeBlock
from GeneratorContext import GeneratorContext

try:
	import typing
except:
	pass

# 用于处理函数重载的辅助类
class OverloadFuncBuilder(FuncBuilderBase):
	def __init__(self, overload_func_info, context, for_struct, outer_type_info, no_export_flags=0):
		# type: (dict, GeneratorContext, bool, dict, int) -> None
		super(OverloadFuncBuilder, self).__init__(overload_func_info, context, for_struct, outer_type_info)

		#
		self.overload_func_info = overload_func_info
		# 哪些函数不应该被导出
		self.no_export_flags = no_export_flags

		# 是否为静态成员函数
		self.is_static = overload_func_info['is_static'] # type: bool

	# 为类或结构体生成方法
	def build_member_func(self):
		# 确保所有重载函数具有相同数量的输入
		# key: input_count
		# val: func_builder
		builder_with_same_input_count = {} # type: dict[int, list[FuncBuilder]]

		# 收集并解析所有重载函数信息
		has_default_value = False
		for func_info in self.overload_func_info['overloads']:
			if func_info['func_flags'] & self.no_export_flags:
				dlog.debug('!skip overload func by flags:', self.func_name, UE4Flags.explain_func_flags(func_info['func_flags']))
				continue

			func_builder = FuncBuilder(func_info, self.context, self.for_struct, self.outer_type_info)
			func_builder.is_overload_func = True
			func_builder.set_py_error = False

			sub_num_input_params = len(func_builder.input_param_indices)
			if sub_num_input_params not in builder_with_same_input_count:
				builder_with_same_input_count[sub_num_input_params] = [func_builder]
			else:
				builder_with_same_input_count[sub_num_input_params].append(func_builder)

			if func_builder.default_param_count > 0:
				has_default_value = True

		if (not has_default_value) and len(builder_with_same_input_count) == 1:
			# 说明所有重载具有相同的输入
			# need to be compatible with py2 and py3
			num_input_params = list(builder_with_same_input_count.keys())[0]
		else:
			num_input_params = -1

		# 生成所有重载函数
		# need to be compatible with py2 and py3
		for input_count in sorted(builder_with_same_input_count.keys()):
			sub_builders = builder_with_same_input_count[input_count]
			for func_builder in sub_builders:
				func_builder.force_var_args = num_input_params == -1
				result = func_builder.build_member_func()
				self.cb.add_line(result['func_body'])
				self.cb.add_line()

				if not func_builder.has_errors:
					for header_name in result['include_list']:
						self.include(header_name)

		dlog.debug('func:', self.func_name)
		dlog.indent()

		# 函数签名
		self.build_signature(num_input_params)

		# 函数本体
		with CodeBlock(self.cb):
			if self.is_constructor:
				self._build_constructor(num_input_params, builder_with_same_input_count)
			else:
				self._build_normal_func(num_input_params, builder_with_same_input_count)

		dlog.unindent()

		return self.get_result(num_input_params, self.is_static)

	# 生成构造函数重载
	def _build_constructor(self, num_input_params, builder_with_same_input_count):
		# type: (int, dict[int, list[FuncBuilder]]) -> None

		self.cb.add_line('if (%s::Init(InSelf, nullptr, nullptr) < 0)' % self.outer_struct_name)		
		self.cb.begin_block()
		self.cb.add_line('PyErr_SetString(PyExc_RuntimeError, "can\'t construct memory for struct \'%s\'");' % (self.func_name, ))
		self.cb.add_line('return -1;')
		self.cb.end_block()
		self.cb.add_line()

		if num_input_params == -1:
			self.cb.add_line('int32 ArgCount = (int32)PyTuple_Size(InArgs);')

		self.cb.add_line()

		# need to be compatible with py2 and py3
		for sub_num_input_params in sorted(builder_with_same_input_count.keys()):
			sub_builders = builder_with_same_input_count[sub_num_input_params]
			if num_input_params == -1:
				self.cb.add_line('if (ArgCount == %d)' % sub_num_input_params)
				self.cb.begin_block()

			for index, func_builder in enumerate(sub_builders):
				if index > 0:
					self.cb.add_line()

				overload_func_name = '%s_%s' % (self.outer_struct_name, func_builder.func_name)
				if func_builder.has_errors:
					error_reasons = '; '.join(func_builder.unsupported_reasons) if func_builder.unsupported_reasons else '[unknown error]'
					self.cb.add_line('// unsupported overload: %s, %s' % (overload_func_name, error_reasons))
				else:
					self.cb.add_line('if (%s(InSelf, InArgs, InKwds) == 0)' % overload_func_name)
					with CodeBlock(self.cb):
						self.cb.add_line('return 0;')
					self.cb.add_line('PyErr_Clear();')

			if num_input_params == -1:
				self.cb.end_block()
				self.cb.add_line()

		if num_input_params != -1:
			self.cb.add_line()

		if self.is_equality_operator:
			self.build_equality_operator_failure()
		else:
			self.cb.add_line('PyErr_SetString(PyExc_RuntimeError, "can\'t resolve overload constructor \'%s\'");' % (self.func_name, ))
			self.build_ret_failure()

	# 生成普通函数重载
	def _build_normal_func(self, num_input_params, builder_with_same_input_count):
		# type: (int, dict[int, list[FuncBuilder]]) -> None
		if num_input_params == -1:
			self.cb.add_line('int32 ArgCount = (int32)PyTuple_Size(InArgs);')

		self.cb.add_line('PyObject* PyRetVal = nullptr;')
		self.cb.add_line()

		if self.is_binary_operator:
			self.build_binary_operator_param_check()

		# need to be compatible with py2 and py3
		for sub_num_input_params in sorted(builder_with_same_input_count.keys()):
			sub_builders = builder_with_same_input_count[sub_num_input_params]
			if num_input_params == -1:
				self.cb.add_line('if (ArgCount == %d)' % sub_num_input_params)
				self.cb.begin_block()

			for index, func_builder in enumerate(sub_builders):
				if index > 0:
					self.cb.add_line()

				overload_func_name = '%s_%s' % (self.outer_struct_name, func_builder.func_name)
				if func_builder.has_errors:
					error_reasons = '; '.join(func_builder.unsupported_reasons) if func_builder.unsupported_reasons else '[unknown error]'
					self.cb.add_line('// unsupported overload: %s, %s' % (overload_func_name, error_reasons))
				else:
					if num_input_params == 0:
						self.cb.add_line('PyRetVal = %s(InSelf);' % overload_func_name)
					elif num_input_params == 1:
						self.cb.add_line('PyRetVal = %s(InSelf, InArg);' % overload_func_name)
					else:
						self.cb.add_line('PyRetVal = %s(InSelf, InArgs);' % overload_func_name)
					self.cb.add_line('if (PyRetVal)')
					with CodeBlock(self.cb):
						self.cb.add_line('return PyRetVal;')
					self.cb.add_line('PyErr_Clear();')

			if num_input_params == -1:
				self.cb.end_block()
				self.cb.add_line()

		if num_input_params != -1:
			self.cb.add_line()

		if self.is_binary_operator:
			self.build_binary_operator_error()
		else:
			self.cb.add_line('PyErr_SetString(PyExc_RuntimeError, "can\'t resolve overload function \'%s\' on \'%s\'");' % (self.func_name, self.outer_cpp_struct_name))

		self.build_ret_failure()
