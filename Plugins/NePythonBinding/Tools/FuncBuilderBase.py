# -*- encoding: utf-8 -*-
import UE4Flags
import PropHelpers
from CodeBuilder import CodeBuilder, CodeBlock
import OperatorOverloadHelper
from GeneratorContext import GeneratorContext

try:
	import typing
except:
	pass

# 函数代码生成基类，提供一些公共的方法
class FuncBuilderBase(object):
	def __init__(self, func_info, context, for_struct, outer_type_info):
		# type: (dict, GeneratorContext, bool, dict) -> None
		#
		self.func_info = func_info
		#
		self.outer_type_info = outer_type_info
		# GeneratorContext
		self.context = context
		# 宿主是UStruct还是UClass
		self.for_struct = for_struct
		# 宿主是否为UInterface
		self.for_interface = not for_struct and (outer_type_info['class_flags'] & UE4Flags.CLASS_Interface) # type: bool
		# 方法所属的类或结构体
		self.outer_struct_name = outer_type_info.get('py_name', 'unsupported') # type: str
		# 方法所属的类或结构体的c++类名
		self.outer_cpp_struct_name = outer_type_info.get('cpp_name', 'unsupported') # type: str

		# 函数名
		self.func_name = func_info['pretty_name'] # type: str
		# 是否为构造函数
		self.is_constructor = func_info['name'] == self.outer_cpp_struct_name # type: bool
		# 是否为二元运算符
		self.is_binary_operator = func_info['name'] in OperatorOverloadHelper.BINARY_OPERATORS # type: bool
		# 是否为相等性比较运算符
		self.is_equality_operator = func_info['name'] in OperatorOverloadHelper.EQUALITY_OPERATORS # type: bool
		# 是否为仅编辑器可用
		self.is_editor_only = func_info.get('func_flags', 0) & UE4Flags.FUNC_EditorOnly # type: bool
		# 是否为 Native 实现的函数，BlueprintImplementableEvent 不具有这个标志
		self.is_native = func_info.get('func_flags', 0) & UE4Flags.FUNC_Native  # type: bool
		# 是否为 BlueprintImplementableEvent 或  BlueprintNativeEvent
		self.is_blueprint_event = func_info.get('func_flags', 0) & UE4Flags.FUNC_BlueprintEvent  # type: bool
		# 需要include的头文件
		self.need_header_list = [] # type: list[str]
		self.need_header_set = set() # type: set[str]
		#
		self.cb = CodeBuilder()
		# 是否有悲剧发生
		self.has_errors = False # type: bool
		# 不支持导出的原因（可能有多个）
		self.unsupported_reasons = [] # type: list[str]

	# 添加需要include的头文件
	def include(self, header):
		# type: (str) -> None
		if header not in self.need_header_set:
			self.need_header_set.add(header)
			self.need_header_list.append(header)

	# 添加需要include的头文件
	# 会递归处理容器类型
	def include_recursive(self, prop_info):
		# type: (dict) -> None
		prop_type = prop_info['type']
		if PropHelpers.is_array_type(prop_info):
			self.include_recursive(prop_info['inner_prop'])
		elif prop_type == 'SetProperty':
			self.include_recursive(prop_info['element_prop'])
		elif prop_type == 'MapProperty':
			self.include_recursive(prop_info['key_prop'])
			self.include_recursive(prop_info['value_prop'])

		type_info = prop_info.get('type_info')
		if PropHelpers.is_export_type(type_info):
			header = type_info.get('py_header_path')
			if header:
				self.include(header)

	# 添加一行代码，并标记错误
	def mark_error(self, error_line):
		# type: (str) -> None
		self.cb.add_line(error_line)
		self.cb.add_line()
		self.has_errors = True

		# 从错误行中提取原因（从方括号中提取）
		# 只提取以特定关键字开头的错误原因，避免提取代码中的数组索引等
		extracted_reason = None
		if '[' in error_line and ']' in error_line:
			start = error_line.find('[')
			end = error_line.find(']', start)
			if start != -1 and end != -1:
				temp_reason = error_line[start:end+1]
				# 检查是否是有效的错误原因（包含关键字）
				if any(keyword in temp_reason for keyword in ['param', 'return', 'output', 'type', 'not exported', 'not supported', 'unknown']):
					extracted_reason = temp_reason
		
		# 如果没有成功提取到方括号原因，尝试从错误行中提取描述性文本
		if not extracted_reason:
			# 尝试提取 "unsupported" 后面的内容作为原因
			if 'unsupported' in error_line.lower():
				# 提取 "unsupported" 之后的内容，去掉开头的空格和注释符号
				parts = error_line.split('unsupported', 1)
				if len(parts) > 1:
					desc = parts[1].strip()
					# 清理前缀的空格和分号
					desc = desc.strip(' ;')
					if desc:
						extracted_reason = '[' + desc + ']'
		
		# 添加到原因列表（避免重复）
		if extracted_reason and extracted_reason not in self.unsupported_reasons:
			self.unsupported_reasons.append(extracted_reason)

	# 生成函数签名
	def build_signature(self, num_input_params):
		# type: (int) -> None
		if self.is_constructor:
			template = 'int %s_%s(%s* InSelf, PyObject* InArgs, PyObject* InKwds)'
		elif num_input_params == 0:
			template = 'PyObject* %s_%s(%s* InSelf)'
		elif num_input_params == 1:
			template = 'PyObject* %s_%s(%s* InSelf, PyObject* InArg)'
		else:
			template = 'PyObject* %s_%s(%s* InSelf, PyObject* InArgs)'
		self.cb.add_line(template % (self.outer_struct_name, self.func_name, self.outer_struct_name))

	# python对于二元运算符重载，有可能会调换输入参数的顺序，需要特殊检查
	# 参见Python源码：abstract.c -> binary_op1
	def build_binary_operator_param_check(self):
		self.cb.add_line('if (!%s(InSelf))' % (self.outer_type_info['py_check_func_name']))
		with CodeBlock(self.cb):
			self.build_binary_operator_error()
			self.build_ret_failure()
		self.cb.add_line()

	# 获取运算符
	def get_op_name(self):
		# type: () -> str
		return self.func_info['name'][len('operator'):].strip()
	
	# 生成二元运算符调用错误
	def build_binary_operator_error(self):
		op_name = self.get_op_name()
		self.cb.add_line('NePyBase::SetBinopTypeError(InSelf, InArg, "%s");' % (op_name, ))

	# 生成相等性比较失败
	def build_equality_operator_failure(self):
		op_name = self.get_op_name()
		if op_name == '==':
			self.cb.add_line('Py_RETURN_FALSE;')
		else: # op_name == '!='
			self.cb.add_line('Py_RETURN_TRUE;')

	# 生成成功返回值
	def build_ret_success(self):
		if self.is_constructor:
			self.cb.add_line('return 0;')
		else:
			self.cb.add_line('Py_RETURN_NONE;')

	# 生成失败返回值
	def build_ret_failure(self):
		if self.is_constructor:
			self.cb.add_line('return -1;')
		else:
			self.cb.add_line('return nullptr;')

	# 获取python函数标志位
	def get_python_meth_flags(self, num_input_params, is_static):
		# type: (int, bool) -> str
		if num_input_params == 0:
			meth_flag = 'METH_NOARGS'
		elif num_input_params == 1:
			meth_flag = 'METH_O'
		else:
			meth_flag = 'METH_VARARGS'
		if is_static:
			meth_flag += ' | METH_STATIC'
		return meth_flag

	# 获取函数代码生成结果
	def get_result(self, num_input_params, is_static):
		# type: (int, bool) -> dict
		result = {}
		result['include_list'] = self.need_header_list

		if self.has_errors:
			func_body = self.cb.build_comment_out()
		else:
			func_body = self.cb.build()
		if self.is_editor_only:
			func_body = '#if WITH_EDITOR\n%s\n#endif // WITH_EDITOR' % func_body
		result['func_body'] = func_body

		is_special_func = False
		full_func_name = '%s_%s' % (self.outer_struct_name, self.func_name)

		if not is_special_func:
			if self.is_constructor:
				result['func_substitute_flags'] = {
					'has_constructor': True
				}
				is_special_func = True

		if not is_special_func:
			operator_info = OperatorOverloadHelper.OPERATOR_BY_PY_NAME.get(self.func_name)
			if operator_info:
				if 'number_method_name' in operator_info:
					number_method_name = operator_info['number_method_name']
					number_method_type = operator_info['number_method_type']
					py3_number_method_name = operator_info.get('py3_number_method_name')
					if py3_number_method_name:
						result['number_func'] = '#if PY_MAJOR_VERSION < 3\n\tPyNumber.%s = (%s)&%s;\n\t#else\n\tPyNumber.%s = (%s)&%s;\n\t#endif' % (number_method_name, number_method_type, full_func_name, py3_number_method_name, number_method_type, full_func_name)
					else:
						result['number_func'] = 'PyNumber.%s = (%s)&%s;' % (number_method_name, number_method_type, full_func_name)
					is_special_func = True
				elif 'cmp_method_flag' in operator_info:
					cmp_method_flag = operator_info['cmp_method_flag']
					result['func_substitute_flags'] = {
						cmp_method_flag: True,
						'has_cmp_funcs': True,
					}
					is_special_func = True

		if not is_special_func:
			meth_flag = self.get_python_meth_flags(num_input_params, is_static)
			func = '{"%s", NePyCFunctionCast(&%s), %s, ""},' % (self.func_name, full_func_name, meth_flag)
			if self.has_errors:
				# 如果有多个错误原因，用分号连接，放在函数定义后面
				if len(self.unsupported_reasons) > 0:
					reason_text = ' ' + '; '.join(self.unsupported_reasons)
				else:
					reason_text = ''
				func = '\t// unsupported %s%s' % (func, reason_text)
			else:
				func = '\t' + func
			if self.is_editor_only:
				func = '#if WITH_EDITOR\n%s\n#endif // WITH_EDITOR' % func
			result['func'] = func

		return result

	@classmethod
	def get_default_substitute_flags(cls):
		return {
			'has_constructor': False,
			'has_cmp_lt': False,
			'has_cmp_le': False,
			'has_cmp_eq': False,
			'has_cmp_ne': False,
			'has_cmp_gt': False,
			'has_cmp_ge': False,
			'has_cmp_funcs': False,
		}
