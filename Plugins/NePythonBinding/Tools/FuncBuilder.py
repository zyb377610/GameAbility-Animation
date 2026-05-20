# -*- encoding: utf-8 -*-
import UE4Flags
import ExportConfig
import PropHelpers
import ConvertHelper
import DebugLog as dlog
from FuncBuilderBase import FuncBuilderBase
from DefaultParamBuilder import DefaultParamBuilder
from CodeBuilder import CodeBlock
from GeneratorContext import GeneratorContext, ExportContext

try:
	import typing
except:
	pass


# 用于生成函数绑定代码的辅助类
class FuncBuilder(FuncBuilderBase):
	def __init__(self, func_info, context, for_struct, outer_type_info):
		# type: (dict, ExportContext, bool, dict) -> None
		super(FuncBuilder, self).__init__(func_info, context, for_struct, outer_type_info)

		# 参数信息
		self.param_infos = None  # type: list[dict]
		# 是否为静态成员函数
		self.is_static = bool(self.func_info['func_flags'] & UE4Flags.FUNC_Static)
		# 是否为友元方法
		self.is_friend_func = func_info.get('is_friend_func', False)
		# 输入参数索引（索引至param_infos）
		self.input_param_indices = []  # type: list[int]
		# 输入参数索引（索引至param_infos）
		self.output_param_indices = []  # type: list[int]
		# 返回值索引（索引至param_infos）
		self.return_param_index = -1
		# 默认参数个数
		self.default_param_count = 0
		# 是否为函数重载
		self.is_overload_func = False
		# 1.配合函数重载使用: 若函数重载具有不同的输入参数，则强制使用METH_VARARGS
		# 2.配合默认参数使用：若函数具有默认参数，则强制使用METH_VARARGS
		self.force_var_args = False
		# 当函数本身具有重载时，有外层函数来设置Python异常信息
		# 内层函数不需要设置Python异常信息
		self.set_py_error = True

		self._parse_func_info()

	def get_fake_num_input_params(self):
		num_input_params = len(self.input_param_indices)
		if self.force_var_args:
			num_input_params = -1
		return num_input_params

	# 为类或结构体生成方法
	def build_member_func(self):
		dlog.debug('func:', self.func_name)
		dlog.indent()

		hosted_class_info = self.func_info.get('hosted_class')
		if hosted_class_info:
			dlog.debug('hosted class:', hosted_class_info['name'])
			dlog.debug('external func:', self.func_info['external_func']['name'])
			self.include(hosted_class_info['header'])

		func_flags = self.func_info['func_flags']
		dlog.debug('flags:', UE4Flags.explain_func_flags(func_flags))

		dlog.debug('params:')
		dlog.indent()
		for index, param_info in enumerate(self.param_infos):
			dlog.debug(param_info['name'])
			dlog.indent()
			prop_flags = param_info['prop_flags']
			dlog.debug('flags:', UE4Flags.explain_prop_flags(prop_flags))
			if 'default' in param_info:
				dlog.debug('default:', param_info['default'])
			dlog.unindent()
		dlog.unindent()

		self.cb.reset()
		self._build_signature()
		with CodeBlock(self.cb):
			self._build_self_check()
			self._build_params()
			self._build_native_func_call()
			self._build_return_values()
		dlog.unindent()

		num_input_params = self.get_fake_num_input_params()
		return self.get_result(num_input_params, self.is_static)

	# 解析反射信息
	def _parse_func_info(self):
		self.param_infos = []
		for param_info in self.func_info['params']:
			self.param_infos.append(PropHelpers.parse_prop_info(param_info, self.context))

		for index, param_info in enumerate(self.param_infos):
			prop_flags = param_info['prop_flags']
			if prop_flags & UE4Flags.CPF_ReturnParm:
				assert self.return_param_index == -1, 'Function already has return value!'
				self.return_param_index = index
			else:
				is_out_param = (prop_flags & UE4Flags.CPF_OutParm) and not (prop_flags & UE4Flags.CPF_ConstParm)
				is_reference_param = prop_flags & UE4Flags.CPF_ReferenceParm
				if is_out_param:
					self.output_param_indices.append(index)
				if not is_out_param or is_reference_param:
					self.input_param_indices.append(index)
					if 'default' in param_info:
						self.default_param_count += 1
						self.force_var_args = True

	# 函数签名
	def _build_signature(self):
		num_input_params = self.get_fake_num_input_params()
		self.build_signature(num_input_params)

	# 检查自身持有的UE对象是否合法
	def _build_self_check(self):
		if self.is_static:
			return
		if self.for_struct:
			return
		self.cb.add_line('if (!NePyBase::CheckValidAndSetPyErr(InSelf, "method \'%s\'"))' % self.func_name)
		with CodeBlock(self.cb):
			self.build_ret_failure()
		self.cb.add_line()

	# 解析输入输出参数
	def _build_params(self):
		num_input_params = len(self.input_param_indices)

		if self.is_binary_operator:
			assert num_input_params == 1
			if not self.is_overload_func:
				self.build_binary_operator_param_check()

		if num_input_params > 0:
			if num_input_params == 1 and not self.force_var_args:
				self.cb.add_line('PyObject* PyArgs[1] = {InArg};')
			else:
				self.cb.add_line('PyObject* PyArgs[%s] = {%s};' % (num_input_params, ', '.join(['nullptr'] * num_input_params)))

				if self.default_param_count > 0:
					format_str = ('O' * (num_input_params - self.default_param_count)) + '|' + ('O' * self.default_param_count)
				else:
					format_str = 'O' * num_input_params

				arg_list = []
				# need to be compatible with py2 and py3
				for i in range(num_input_params):
					arg_list.append('&PyArgs[%d]' % i)
				arg_list = ', '.join(arg_list)

				self.cb.add_line('if (!PyArg_ParseTuple(InArgs, "%s:%s", %s))' % (format_str, self.func_name, arg_list))
				with CodeBlock(self.cb):
					self.build_ret_failure()

			self.cb.add_line()

		input_param_index = 0
		for index, param_info in enumerate(self.param_infos):
			if index == self.return_param_index:
				continue

			if index in self.input_param_indices:
				py_param_name = 'PyArgs[%s]' % input_param_index
				self._build_input_param(input_param_index, py_param_name, param_info)
				input_param_index += 1
			else:
				self._build_output_param(param_info)

	# 解析单个输入参数
	def _build_input_param(self, param_index, py_param_name, param_info):
		# type: (int, str, dict) -> None
		prop_type = param_info['type']
		cpp_param_name = self.valid_param_name(param_info['name'])

		if PropHelpers.is_array_type(param_info):
			self._build_input_param_array(param_index, py_param_name, cpp_param_name, param_info)
			return
		if PropHelpers.is_builtin_type(param_info):
			self._build_input_param_builtin(param_index, py_param_name, cpp_param_name, param_info)
			return
		if PropHelpers.is_enum_type(param_info):
			self._build_input_param_enum(param_index, py_param_name, cpp_param_name, param_info)
			return
		if PropHelpers.is_string_type(param_info):
			self._build_input_param_string(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'FieldPathProperty':
			self._build_input_param_field_path(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'DelegateProperty':
			self._build_input_param_delegate(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'StructProperty':
			self._build_input_param_struct(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'ObjectProperty':
			self._build_input_param_object(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'InterfaceProperty':
			self._build_input_param_interface(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'ClassProperty':
			self._build_input_param_class(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'SetProperty':
			self._build_input_param_set(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == 'MapProperty':
			self._build_input_param_map(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == "SoftObjectProperty":
			self._build_input_param_soft_obj(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == "SoftClassProperty":
			self._build_input_param_soft_class(param_index, py_param_name, cpp_param_name, param_info)
			return
		if prop_type == "WeakObjectProperty":
			self._build_input_param_weak_obj(param_index, py_param_name, cpp_param_name, param_info)
			return
		reason = '[param "%s": unknown property type "%s"]' % (cpp_param_name, prop_type)
		self.mark_error('// unsupported %s %s %s %s;' % (py_param_name, prop_type, cpp_param_name, reason))

	# 生成数组类型输入参数的代码
	def _build_input_param_array(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		if not PropHelpers.check_prop_type_info(param_info):
			inner_type = param_info.get('inner_prop', {}).get('type', 'unknown')
			reason = '[param "%s": array element type "%s" not exported]' % (cpp_param_name, inner_type)
			self.mark_error('// unsupported %s ArrayProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		def handle_failure(*args):
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					type_info = param_info['type_info']
					if len(args) == 0:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, type_info['py_name']))
					else:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' %s");' % ((param_index + 1), cpp_param_name, args[0]))
				self.build_ret_failure()

		self.include_recursive(param_info)
		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.array_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成内置类型输入参数的代码
	def _build_input_param_builtin(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					builtin_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, builtin_info['py_name']))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.builtin_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure, need_temp_for_cast=False)

		self.cb.add_line()

	# 生成枚举类型输入参数的代码
	def _build_input_param_enum(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					builtin_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'(%s)");' % ((param_index + 1), cpp_param_name, builtin_info['py_name'], param_info['enum_name']))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.enum_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成字符串类型输入参数的代码
	def _build_input_param_string(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					string_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, string_info['py_name']))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.string_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成属性路径输入参数的代码
	def _build_input_param_field_path(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, 'FieldPath'))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.field_path_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成 TSoftObjectPtr 输入参数的代码
	def _build_input_param_soft_obj(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		type_info = param_info['type_info']
		self.include(type_info['py_header_path'])

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, 'SoftObjectPtr'))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.soft_obj_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成 TSoftClassPtr 输入参数的代码
	def _build_input_param_soft_class(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		type_info = param_info['type_info']
		self.include(type_info['py_header_path'])

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, 'SoftObjectPtr'))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.soft_class_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()
	
	# 生成 TWeakObjectPtr 输入参数的代码
	def _build_input_param_weak_obj(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		type_info = param_info['type_info']
		self.include(type_info['py_header_path'])

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, 'WeakObjectPtr'))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.weak_obj_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成Delegate类型输入参数的代码
	def _build_input_param_delegate(self, param_index, py_param_name, cpp_param_name, param_info):
		self.include('NePyDynamicDelegateWrapper.h')
		type_info = param_info['type_info']
		self.include(type_info['header'])

		# type: (int, str, str, dict) -> None
		def handle_failure(ufunction_not_exist=False):
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					if ufunction_not_exist:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' Cannot Find UFunction");' % ((param_index + 1), cpp_param_name))
					else:
						delegate_info = param_info['type_info']
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, delegate_info['py_name']))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.delegate_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure, self.func_name, self.is_static, self.outer_cpp_struct_name)

		self.cb.add_line()

	# 生成struct类型输入参数的代码
	def _build_input_param_struct(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		struct_info = param_info['type_info']
		if not PropHelpers.is_export_type(struct_info):
			struct_name = param_info.get('struct_name', 'unknown')
			reason = '[param "%s": struct type "%s" not exported]' % (cpp_param_name, struct_name)
			self.mark_error('// unsupported %s StructProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		self._build_input_param_custom_type(param_index, py_param_name, cpp_param_name, param_info, True, False)

	# 生成class类型输入参数的代码
	def _build_input_param_object(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		class_info = param_info['type_info']
		if not PropHelpers.is_export_type(class_info):
			class_name = class_info.get('name', 'unknown') if class_info else 'unknown'
			reason = '[param "%s": class type "%s" not exported]' % (cpp_param_name, class_name)
			self.mark_error('// unsupported %s ObjectProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		self._build_input_param_custom_type(param_index, py_param_name, cpp_param_name, param_info, False, False)

	# 生成interface类型输入参数的代码
	def _build_input_param_interface(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		class_info = param_info['type_info']
		if not PropHelpers.is_export_type(class_info):
			interface_name = class_info.get('name', 'unknown') if class_info else 'unknown'
			reason = '[param "%s": interface type "%s" not exported]' % (cpp_param_name, interface_name)
			self.mark_error('// unsupported %s InterfaceProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		self._build_input_param_custom_type(param_index, py_param_name, cpp_param_name, param_info, False, True)

	# 生成自定义类型(struct or class)输入参数的代码
	def _build_input_param_custom_type(self, param_index, py_param_name, cpp_param_name, param_info, is_struct_type, is_interface_type):
		# type: (int, str, str, dict, bool, bool) -> None
		type_info = param_info['type_info']
		self.include(type_info['py_header_path'])

		def handle_failure(*args):
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					if len(args) == 0:
						if is_interface_type:
							line = 'PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must implements interface \'%s\'");'
						else:
							line = 'PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");'
						self.cb.add_line(line % ((param_index + 1), cpp_param_name, type_info['name']))
					else:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' %s");' % ((param_index + 1), cpp_param_name, args[0]))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			if is_struct_type:
				out_is_pointer = not has_default_param
				if has_default_param:
					if param_info['default'] != 'nullptr':
						# 这是一个飞线，很丑但没办法
						# 当结构体存在默认值（且不为nullptr）时，变量为值类型，而不是默认的指针类型
						# 在native call时不需要解引用
						param_info['src_is_not_pointer'] = True
					else:
						out_is_pointer = True
				ConvertHelper.struct_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure, out_is_pointer)
			elif is_interface_type:
				ConvertHelper.interface_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)
			else:
				ConvertHelper.object_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成TSubclassOf<Class>类型输入参数的代码
	def _build_input_param_class(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		class_info = param_info['type_info']
		if not PropHelpers.is_export_type(class_info):
			class_name = param_info.get('class_name', 'unknown')
			reason = '[param "%s": TSubclassOf<%s> not exported]' % (cpp_param_name, class_name)
			self.mark_error('// unsupported %s ClassProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		self.include(class_info['header'])

		class_name = param_info['class_name']
		class_header = ExportConfig.SpecialClassHeaders.get(class_name)
		if class_header:
			self.include(class_header)

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'UClass<%s>\'");' % ((param_index + 1), cpp_param_name, class_info['name']))
				self.build_ret_failure()

		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.class_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成集合类型输入参数的代码
	def _build_input_param_set(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		if not PropHelpers.check_prop_type_info(param_info):
			element_type = param_info.get('element_prop', {}).get('type', 'unknown')
			reason = '[param "%s": set element type "%s" not exported]' % (cpp_param_name, element_type)
			self.mark_error('// unsupported %s SetProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		def handle_failure(*args):
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					type_info = param_info['type_info']
					if len(args) == 0:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, type_info['py_name']))
					else:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' %s");' % ((param_index + 1), cpp_param_name, args[0]))
				self.build_ret_failure()

		self.include_recursive(param_info)
		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.set_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成字典类型输入参数的代码
	def _build_input_param_map(self, param_index, py_param_name, cpp_param_name, param_info):
		# type: (int, str, str, dict) -> None
		if not PropHelpers.check_prop_type_info(param_info):
			key_type = param_info.get('key_prop', {}).get('type', 'unknown')
			value_type = param_info.get('value_prop', {}).get('type', 'unknown')
			reason = '[param "%s": map key type "%s" or value type "%s" not exported]' % (cpp_param_name, key_type, value_type)
			self.mark_error('// unsupported %s MapProperty %s %s;' % (py_param_name, cpp_param_name, reason))
			return

		def handle_failure(*args):
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					type_info = param_info['type_info']
					if len(args) == 0:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' must have type \'%s\'");' % ((param_index + 1), cpp_param_name, type_info['py_name']))
					else:
						self.cb.add_line('PyErr_SetString(PyExc_TypeError, "arg%d \'%s\' %s");' % ((param_index + 1), cpp_param_name, args[0]))
				self.build_ret_failure()

		self.include_recursive(param_info)
		with DefaultParamBuilder(self.cb, param_info, py_param_name, cpp_param_name) as has_default_param:
			ConvertHelper.map_prop_to_cpp(self.cb, param_info, py_param_name, cpp_param_name, not has_default_param, handle_failure)

		self.cb.add_line()

	# 生成输出参数声明
	def _build_output_param(self, param_info):
		# type: (dict) -> None
		prop_type = param_info['type']
		cpp_param_name = self.valid_param_name(param_info['name'])

		if not PropHelpers.check_prop_type_info(param_info):
			reason = '[output param "%s": type "%s" not supported]' % (cpp_param_name, prop_type)
			self.mark_error('// unsupported OutputParm %s %s %s;' % (prop_type, cpp_param_name, reason))
			return

		cpp_type = PropHelpers.extract_cpp_type(param_info)
		self.cb.add_line('%s %s;' % (cpp_type, cpp_param_name))
		self.cb.add_line()

	# 调用C函数
	def _build_native_func_call(self):
		arg_list = []

		if self.is_friend_func:
			# 目前只有UStruct才支持友元方法
			assert self.for_struct
			arg_list.append('*(%s*)InSelf->Value' % (self.outer_cpp_struct_name))

		for index, param_info in enumerate(self.param_infos):
			if index == self.return_param_index:
				continue
			prop_type = param_info['type']
			cpp_param_name = self.valid_param_name(param_info['name'])
			if index in self.input_param_indices and prop_type == 'StructProperty':
				# 根据输入和输出，选择是引用或解引用
				src_is_not_pointer = bool(param_info.get('src_is_not_pointer'))
				dst_is_pointer = bool(param_info.get('is_pointer'))
				if src_is_not_pointer:
					if dst_is_pointer:
						cpp_param_name = '&' + cpp_param_name
				else:
					if not dst_is_pointer:
						cpp_param_name = '*' + cpp_param_name
			arg_list.append(cpp_param_name)
		arg_list = ', '.join(arg_list)

		cpp_func_name = self.func_info['name']
		hosted_class_info = self.func_info.get('hosted_class')
		if hosted_class_info:
			external_func_info = self.func_info.get('external_func')
			cpp_func_name = '%s::%s' % (hosted_class_info['cpp_name'], external_func_info['name'])
		if self.is_blueprint_event and self.is_native:
			# 当导出 BlueprintNativeEvent 时，需要调用 _Implementation 的函数
			cpp_func_name = '%s_Implementation' % cpp_func_name

		# 构造函数
		if self.is_constructor:
			# 只有UStruct才能在Python中构造创建
			assert self.for_struct
			if hosted_class_info:
				# 5.3，部分struct（例如FQuat）增加了强转make func
				line = 'new (InSelf->Value) %s(%s(%s));' % (self.func_info['name'], cpp_func_name, arg_list)
			else:
				line = 'new (InSelf->Value) %s(%s);' % (cpp_func_name, arg_list)
		# 普通函数
		else:
			if self.return_param_index >= 0:
				ret_val = 'auto RetVal = '
			else:
				ret_val = ''
			if self.is_friend_func:
				caller = ''
			elif self.is_static:
				caller = self.outer_cpp_struct_name + '::'
			elif self.for_struct:
				caller = '((%s*)InSelf->Value)' % (self.outer_cpp_struct_name)
				if not hosted_class_info:
					caller += '->'
			elif self.for_interface:
				caller = 'Cast<I%s>(InSelf->Value)' % (self.outer_type_info['name'])
				if not hosted_class_info:
					caller += '->'
			else:
				caller = 'InSelf->GetValue()'
				if not hosted_class_info:
					caller += '->'
			if hosted_class_info:
				# 5.3，部分struct（例如FQuat）增加了强转break func
				struct_type = 'F%s' % external_func_info['params'][0]['struct_name']
				if struct_type != self.outer_cpp_struct_name:
					line = '%s%s((%s)*%s, %s);' % (ret_val, cpp_func_name, struct_type, caller, arg_list)
				else:
					line = '%s%s(*%s, %s);' % (ret_val, cpp_func_name, caller, arg_list)
			else:
				line = '%s%s%s(%s);' % (ret_val, caller, cpp_func_name, arg_list)

		self.cb.add_line(line)

	# 生成python返回值代码
	def _build_return_values(self):
		output_param_infos = []
		output_param_names = []
		if self.return_param_index >= 0:
			output_param_infos.append(self.param_infos[self.return_param_index])
			output_param_names.append('RetVal')
		for index in self.output_param_indices:
			output_param_infos.append(self.param_infos[index])
			output_param_names.append(self.param_infos[index]['name'])

		num_return_values = len(output_param_infos)

		# 无返回值
		if num_return_values == 0:
			self.build_ret_success()
			return

		self.cb.add_line()
		for index in range(num_return_values):
			param_info = output_param_infos[index]
			cpp_param_name = output_param_names[index]
			py_ret_name = 'PyRetVal%d' % index
			self._build_return_value(cpp_param_name, py_ret_name, param_info)

		# 单返回值
		if num_return_values == 1:
			self.cb.add_line('return PyRetVal0;')
			return

		# 多返回值
		self.cb.add_line()
		self.cb.add_line('PyObject* PyRetVals = PyTuple_New(%d);' % num_return_values)
		for index in range(num_return_values):
			self.cb.add_line('PyTuple_SetItem(PyRetVals, %d, PyRetVal%d);' % (index, index))
		self.cb.add_line('return PyRetVals;')

	# 生成返回值类型转换代码
	def _build_return_value(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		prop_type = param_info['type']

		if PropHelpers.is_array_type(param_info):
			self._build_return_value_array(cpp_param_name, py_ret_name, param_info)
			return
		if PropHelpers.is_builtin_type(param_info):
			self._build_return_value_builtin(cpp_param_name, py_ret_name, param_info)
			return
		if PropHelpers.is_enum_type(param_info):
			self._build_return_value_enum(cpp_param_name, py_ret_name, param_info)
			return
		if PropHelpers.is_string_type(param_info):
			self._build_return_value_string(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'FieldPathProperty':
			self._build_return_value_field_path(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'StructProperty':
			self._build_return_value_struct(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'ObjectProperty':
			self._build_return_value_object(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'InterfaceProperty':
			self._build_return_value_interface(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'ClassProperty':
			self._build_return_value_class(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'SetProperty':
			self._build_return_value_set(cpp_param_name, py_ret_name, param_info)
			return
		if prop_type == 'MapProperty':
			self._build_return_value_map(cpp_param_name, py_ret_name, param_info)
			return
		if PropHelpers.is_soft_type(param_info):
			self._build_return_value_soft(cpp_param_name, py_ret_name, param_info)
			return
		reason = '[return value: unknown property type "%s"]' % prop_type
		self.mark_error('// unsupported %s %s %s %s;' % (py_ret_name, prop_type, cpp_param_name, reason))

	# 数组类型
	def _build_return_value_array(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		if not PropHelpers.check_prop_type_info(param_info):
			inner_type = param_info.get('inner_prop', {}).get('type', 'unknown')
			reason = '[return value: array element type "%s" not exported]' % inner_type
			self.mark_error('// unsupported %s ArrayProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					type_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "%s \'%s\' with type \'%s\' convert to PyObject failed!");' % (py_ret_name, cpp_param_name, type_info['py_name']))
				self.build_ret_failure()

		self.include_recursive(param_info)
		ConvertHelper.array_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, handle_failure)

	# 内置类型
	def _build_return_value_builtin(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		ConvertHelper.builtin_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True)

	# 枚举类型
	def _build_return_value_enum(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		ConvertHelper.enum_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True)

	# 字符串类型
	def _build_return_value_string(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					string_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "%s \'%s\' with type \'%s\' convert to PyObject failed!");' % (py_ret_name, cpp_param_name, string_info['py_name']))
				self.build_ret_failure()

		ConvertHelper.string_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, handle_failure)

	# 属性路径类型
	def _build_return_value_field_path(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "%s \'%s\' with type \'%s\' convert to PyObject failed!");' % (py_ret_name, cpp_param_name, 'FieldPath'))
				self.build_ret_failure()

		ConvertHelper.field_path_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, handle_failure)

	def _build_return_value_soft(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "%s \'%s\' with type \'%s\' convert to PyObject failed!");' % (py_ret_name, cpp_param_name, 'SoftObjectPtr'))
				self.build_ret_failure()

		type_info = param_info['type_info']
		self.include(type_info['py_header_path'])
		ConvertHelper.soft_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, handle_failure)

	# UStruct实例
	def _build_return_value_struct(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		struct_info = param_info['type_info']
		if not PropHelpers.is_export_type(struct_info):
			struct_name = param_info.get('struct_name', 'unknown')
			reason = '[return value: struct type "%s" not exported]' % struct_name
			self.mark_error('// unsupported %s StructProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		prop_flags = param_info['prop_flags']
		src_is_pointer = (prop_flags & UE4Flags.CPF_ReferenceParm) and not (prop_flags & UE4Flags.CPF_ConstParm)

		self.include(struct_info['py_header_path'])
		ConvertHelper.struct_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, src_is_pointer)

	# UClass实例
	def _build_return_value_object(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		class_info = param_info['type_info']
		if not PropHelpers.is_export_type(class_info):
			class_name = class_info.get('name', 'unknown') if class_info else 'unknown'
			reason = '[return value: class type "%s" not exported]' % class_name
			self.mark_error('// unsupported %s ObjectProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		self.include(class_info['py_header_path'])
		ConvertHelper.object_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True)

	# UInterface实例
	def _build_return_value_interface(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		class_info = param_info['type_info']
		if not PropHelpers.is_export_type(class_info):
			interface_name = class_info.get('name', 'unknown') if class_info else 'unknown'
			reason = '[return value: interface type "%s" not exported]' % interface_name
			self.mark_error('// unsupported %s InterfaceProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		self.include(class_info['py_header_path'])
		ConvertHelper.interface_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True)

	# TSubclassOf<Class>类型
	def _build_return_value_class(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		class_info = param_info['type_info']
		if not PropHelpers.is_export_type(class_info):
			class_name = param_info.get('class_name', 'unknown')
			reason = '[return value: TSubclassOf<%s> not exported]' % class_name
			self.mark_error('// unsupported %s ClassProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		self.include(class_info['header'])
		ConvertHelper.class_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True)

	# 集合类型
	def _build_return_value_set(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		if not PropHelpers.check_prop_type_info(param_info):
			element_type = param_info.get('element_prop', {}).get('type', 'unknown')
			reason = '[return value: set element type "%s" not exported]' % element_type
			self.mark_error('// unsupported %s SetProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					type_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "%s \'%s\' with type \'%s\' convert to PyObject failed!");' % (py_ret_name, cpp_param_name, type_info['py_name']))
				self.build_ret_failure()

		self.include_recursive(param_info)
		ConvertHelper.set_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, handle_failure)

	# 字典类型
	def _build_return_value_map(self, cpp_param_name, py_ret_name, param_info):
		# type: (str, str, dict) -> None
		if not PropHelpers.check_prop_type_info(param_info):
			key_type = param_info.get('key_prop', {}).get('type', 'unknown')
			value_type = param_info.get('value_prop', {}).get('type', 'unknown')
			reason = '[return value: map key type "%s" or value type "%s" not exported]' % (key_type, value_type)
			self.mark_error('// unsupported %s MapProperty %s %s;' % (py_ret_name, cpp_param_name, reason))
			return

		def handle_failure():
			if self.is_equality_operator:
				self.build_equality_operator_failure()
			else:
				if self.set_py_error:
					type_info = param_info['type_info']
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "%s \'%s\' with type \'%s\' convert to PyObject failed!");' % (py_ret_name, cpp_param_name, type_info['py_name']))
				self.build_ret_failure()

		self.include_recursive(param_info)
		ConvertHelper.map_prop_to_py(self.cb, param_info, cpp_param_name, py_ret_name, True, handle_failure)

	def valid_param_name(self, cpp_param_name):
		# type: (str) -> str
		# 很不凑巧，函数原本的参数名与我们的冲突了
		if cpp_param_name in ('InArg', 'InArgs'):
			return cpp_param_name + '_'
		return cpp_param_name


# 单元测试
def test():
	func_info = {
		"name": "K2_SetActorLocationAndRotation",
		"pretty_name": "SetActorLocationAndRotation",
		"func_flags": 79823873,
		"params": [
			{
				"name": "NewLocation",
				"pretty_name": "NewLocation",
				"prop_flags": 6755469234274944,
				"type": "StructProperty",
				"struct_name": "Vector"
			},
			{
				"name": "NewRotation",
				"pretty_name": "NewRotation",
				"prop_flags": 4503669420589696,
				"type": "StructProperty",
				"struct_name": "Rotator"
			},
			{
				"name": "bSweep",
				"pretty_name": "bSweep",
				"prop_flags": 6755469234274944,
				"type": "BoolProperty"
			},
			{
				"name": "SweepHitResult",
				"pretty_name": "SweepHitResult",
				"prop_flags": 4504219176403328,
				"type": "StructProperty",
				"struct_name": "HitResult"
			},
			{
				"name": "bTeleport",
				"pretty_name": "bTeleport",
				"prop_flags": 6755469234274944,
				"type": "BoolProperty"
			},
			{
				"name": "ReturnValue",
				"pretty_name": "ReturnValue",
				"prop_flags": 6755469234276224,
				"type": "BoolProperty"
			}
		]
	}

	func_builder = FuncBuilder(func_info, None, False)
	result = func_builder.build()
	print('-------------------------------------')
	print(result)
	print('-------------------------------------')


if __name__ == '__main__':
	dlog.set_log_level(dlog.LOG_LEVEL_DEBUG)
	test()
