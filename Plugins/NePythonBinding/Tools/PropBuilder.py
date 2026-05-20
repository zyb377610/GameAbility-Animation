# -*- encoding: utf-8 -*-
from functools import partial
import UE4Flags
import ExportConfig
import PropHelpers
import ConvertHelper
import DebugLog as dlog
from CodeBuilder import CodeBuilder, CodeBlock
from GeneratorContext import GeneratorContext

try:
	import typing
except:
	pass


class GetterRegion():
	def __init__(self, builder):
		# type: (PropBuilder) -> None
		self.builder = builder

	def __enter__(self):
		if self.builder.for_struct:
			func_name = '%s_Get%s' % (self.builder.outer_struct_name, self.builder.prop_name)
		else:
			# 防止与函数名冲突
			func_name = '%s_Prop_Get%s' % (self.builder.outer_struct_name, self.builder.prop_name)
		self.builder.cb.add_line('PyObject* %s(%s* InSelf, void* InClosure)' % (func_name, self.builder.outer_struct_name))
		self.builder.cb.begin_block()
		if not self.builder.for_struct:
			self.builder.cb.add_line('if (!NePyBase::CheckValidAndSetPyErr(InSelf, "attribute \'%s\'"))' % self.builder.prop_name)
			self.builder.cb.begin_block()
			self.builder.cb.add_line('return nullptr;')
			self.builder.cb.end_block()
			self.builder.cb.add_line()
		return func_name

	def __exit__(self, exc_type, exc_val, exc_tb):
		self.builder.cb.add_line('return RetValue;')
		self.builder.cb.end_block()


class SetterRegion():
	def __init__(self, builder):
		# type: (PropBuilder) -> None
		self.builder = builder
		# 一个标志位，防止递归进入Region
		self.entered_region = False

	def __enter__(self):
		if self.builder.in_setter_region:
			return
		self.builder.in_setter_region = True
		self.entered_region = True

		if self.builder.for_struct:
			func_name = '%s_Set%s' % (self.builder.outer_struct_name, self.builder.prop_name)
		else:
			# 防止与函数名冲突
			func_name = '%s_Prop_Set%s' % (self.builder.outer_struct_name, self.builder.prop_name)
		self.builder.cb.add_line('int %s(%s* InSelf, PyObject* InValue, void* InClosure)' % (func_name, self.builder.outer_struct_name))
		self.builder.cb.begin_block()
		if not self.builder.for_struct:
			self.builder.cb.add_line('if (!NePyBase::CheckValidAndSetPyErr(InSelf, "attribute \'%s\'"))' % self.builder.prop_name)
			self.builder.cb.begin_block()
			self.builder.cb.add_line('return -1;')
			self.builder.cb.end_block()
			self.builder.cb.add_line()
		return func_name

	def __exit__(self, exc_type, exc_val, exc_tb):
		if not self.entered_region:
			return
		self.builder.in_setter_region = False
		self.builder.cb.add_line('return 0;')
		self.builder.cb.end_block()


class PropBuilder(object):
	def __init__(self, prop_info, cpp_type_name, context, for_struct):
		# type: (dict, str, GeneratorContext, bool) -> None
		self.prop_info = PropHelpers.parse_prop_info(prop_info, context)
		# GeneratorContext
		self.context = context
		# 宿主是UStruct还是UClass
		self.for_struct = for_struct

		# 属性名
		self.prop_name = prop_info['name']  # type: str
		# 属性类型
		self.prop_type = prop_info['type']  # type: str
		# 是否为默认类型
		self.is_builtin_type = False  # type: bool
		# 是否为枚举类型
		self.is_enum_type = False  # type: bool
		# 是否为字符串类型
		self.is_string_type = False  # type: bool
		# 是否为数组类型
		self.is_array_type = False  # type: bool
		# 是否为只读类型
		self.is_readonly_type = False  # type: bool
		# 属性所属的类或结构体的信息
		self.outer_type_info = None  # type: dict
		# 属性所属的类或结构体
		self.outer_struct_name = None  # type: str
		# 属性所属的类或结构体的c++类名
		self.outer_cpp_struct_name = None  # type: str
		# 需要include的头文件
		self._need_header_list = []  # type: list[str]
		self._need_header_set = set()  # type: set[str]
		#
		self.cb = CodeBuilder()

		# 供代码生成用的一些内部变量
		if for_struct:
			self.self_name = '((%s*)InSelf->Value)->'  % (cpp_type_name,) # type: str
		else:
			self.self_name = 'InSelf->GetValue()->'  # type: str
		self.cpp_name = self.self_name + self.prop_name  # type: str
		self.in_py_name = 'InValue'  # type: str
		self.out_py_name = 'RetValue'  # type: str

		# 无法生成代码的原因
		self.unsupported_reason = '[unknown type]'  # type: str
		# 是否正在生成Setter代码
		self.in_setter_region = False  # type: bool

		self._parse_prop_info()

	# 为类或结构体生成属性的getter/setter
	def build_prop_getset(self, outer_type_info):
		# type: (dict) -> dict
		self.outer_type_info = outer_type_info
		self.outer_struct_name = outer_type_info['py_name']
		self.outer_cpp_struct_name = outer_type_info['cpp_name']

		dlog.debug(self.prop_name, '[%s]' % self.prop_type)
		dlog.indent()
		dlog.debug('flags:', UE4Flags.explain_prop_flags(self.prop_info['prop_flags']))

		result = None
		if not self.prop_info['array_dim'] > 1:
			if self.is_builtin_type:
				result = self._do_build_prop_getset(self._build_prop_getter_builtin, self._build_prop_setter_builtin)
			elif self.is_enum_type:
				result = self._do_build_prop_getset(self._build_prop_getter_enum, self._build_prop_setter_enum)
			elif self.is_string_type:
				result = self._do_build_prop_getset(self._build_prop_getter_string, self._build_prop_setter_string)
			elif self.prop_type == 'FieldPathProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_field_path, self._build_prop_setter_field_path)
			elif self.prop_type == 'StructProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_struct, self._build_prop_setter_struct)
			elif self.prop_type == 'ObjectProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_object, self._build_prop_setter_object)
			elif self.prop_type == 'WeakObjectProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_weak, self._build_prop_setter_weak_object)
			elif self.prop_type == 'ClassProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_class, self._build_prop_setter_class)
			elif self.prop_type == 'ClassPtrProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_class, self._build_prop_setter_class)
			elif self.prop_type == 'ArrayProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_array, self._build_prop_setter_array)
			elif self.prop_type == 'MapProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_map, self._build_prop_setter_map)
			elif self.prop_type == 'SetProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_set, self._build_prop_setter_set)
			elif self.prop_type == 'DelegateProperty':
				result = self._do_build_prop_getset(self._build_prop_getter_dynamic_delegate, None)
			elif self.prop_type in ('MulticastInlineDelegateProperty', 'MulticastSparseDelegateProperty'):
				result = self._do_build_prop_getset(self._build_prop_getter_dynamic_multicast_delegate, None)
			elif self.prop_type == "SoftObjectProperty":
				result = self._do_build_prop_getset(self._build_prop_getter_soft, self._build_prop_setter_soft_object)
			elif self.prop_type == "SoftClassProperty":
				result = self._do_build_prop_getset(self._build_prop_getter_soft, self._build_prop_setter_soft_class)
		else:  # self.prop_info['array_dim'] > 1
			result = self._do_build_prop_getset(self._build_prop_getter_fixed_array, self._build_prop_setter_fixed_array)

		dlog.unindent()

		if not result:
			result = {
				'getset': '\t// unsupported %s %s %s,' % (self.prop_type, self.prop_name, self.unsupported_reason),
				'getset_body': '// unsupported %s %s %s' % (self.prop_type, self.prop_name, self.unsupported_reason),
			}

		is_editor_only = self.prop_info['prop_flags'] & UE4Flags.CPF_EditorOnly
		if is_editor_only:
			with_editor_wrapper = '#if WITH_EDITOR\n%s\n#endif // WITH_EDITOR'
			result['getset'] = with_editor_wrapper % result['getset']
			result['getset_body'] = with_editor_wrapper % result['getset_body']

		result['include_list'] = self._need_header_list
		return result

	# 生成内置类型属性的getter/setter
	def _do_build_prop_getset(self, build_getter_func, build_setter_func):
		# type: (typing.Callable, typing.Callable) -> dict
		self.cb.reset()

		type_info = self.prop_info['type_info']
		if not PropHelpers.is_export_type(type_info):
			self.unsupported_reason = '[type not exported: %s]' % type_info.get('name', 'unknown')
			return None

		if 'getter' in self.prop_info:
			if not self._is_func_exposed(self.prop_info['getter']):
				dlog.debug('native getter func not exposed: %s' % self.prop_info['getter'])
				self.unsupported_reason = '[native getter func "%s" not exposed]' % self.prop_info['getter']
				return None

			def build_getter_func(inner_build_getter_func=build_getter_func): return self._build_prop_getter_func(inner_build_getter_func)

		if build_setter_func and 'setter' in self.prop_info:
			if not self._is_func_exposed(self.prop_info['setter']):
				dlog.debug('native setter func not exposed: %s' % self.prop_info['setter'])
				build_setter_func = None
			else:
				def build_setter_func(inner_build_setter_func=build_setter_func): return self._build_prop_setter_func(inner_build_setter_func)

		getter_name = build_getter_func()
		if not getter_name:
			if not self.unsupported_reason or self.unsupported_reason == '[unknown type]':
				self.unsupported_reason = '[failed to build getter]'
			return None
		getter_name = '(getter)' + getter_name

		if not self.is_readonly_type and build_setter_func:
			self.cb.add_line()
			setter_name = '(setter)' + build_setter_func()
		else:
			setter_name = 'nullptr'

		return {
			'getset': '\t{ (char*)"%s", %s, %s, nullptr, nullptr },' % (self.prop_name, getter_name, setter_name),
			'getset_body': self.cb.build(),
		}

	# 看看UE提供的getter/setter方法是否允许被导出
	def _is_func_exposed(self, func_name):
		# type: (str) -> bool
		return self.context.curr_func_filter_func(self.outer_type_info['name'], func_name)

	# 使用UE提供的getter方法
	def _build_prop_getter_func(self, inner_build_getter_func):
		# type: (typing.Callable) -> str|None
		cpp_type = PropHelpers.extract_cpp_type(self.prop_info)
		if 'unsupported' in cpp_type:
			self.unsupported_reason = '[unsupported C++ type: %s]' % cpp_type
			return

		getter_name = self.prop_info['getter']
		old_cpp_name = self.cpp_name
		self.cpp_name = '%s%s()' % (self.self_name, getter_name)
		func_name = inner_build_getter_func()
		self.cpp_name = old_cpp_name
		return func_name

	# 使用引擎提供的setter方法
	def _build_prop_setter_func(self, inner_build_setter_func):
		# type: (typing.Callable) -> str
		temp_cpp_name = 'TempValueForSetter'
		setter_name = self.prop_info['setter']

		with SetterRegion(self) as func_name:
			self.cb.add_line('%s %s;' % (PropHelpers.extract_cpp_type(self.prop_info), temp_cpp_name))
			old_cpp_name = self.cpp_name
			self.cpp_name = temp_cpp_name
			inner_build_setter_func()
			self.cpp_name = old_cpp_name
			self.cb.add_line('%s%s(%s);' % (self.self_name, setter_name, temp_cpp_name))

		return func_name

	# 生成内置类型getter方法
	def _build_prop_getter_builtin(self):
		with GetterRegion(self) as func_name:
			ConvertHelper.builtin_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True)

		return func_name

	# 生成内置类型setter方法
	def _build_prop_setter_builtin(self):
		with SetterRegion(self) as func_name:
			def handle_failure():
				builtin_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % builtin_info['py_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.builtin_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成枚举类型getter方法
	def _build_prop_getter_enum(self):
		with GetterRegion(self) as func_name:
			ConvertHelper.enum_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True)

		return func_name

	# 生成枚举类型setter方法
	def _build_prop_setter_enum(self):
		with SetterRegion(self) as func_name:
			def handle_failure():
				builtin_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'(%s)");' % (builtin_info['py_name'], self.prop_info['enum_name']))
				self.cb.add_line('return -1;')

			ConvertHelper.enum_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成字符串类型getter方法
	def _build_prop_getter_string(self):
		with GetterRegion(self) as func_name:
			def handle_failure():
				string_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "convert to str failed, src type is \'%s\'");' % string_info['py_name'])
				self.cb.add_line('return nullptr;')

			ConvertHelper.string_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True, handle_failure)

		return func_name

	# 生成字符串类型setter方法
	def _build_prop_setter_string(self):
		with SetterRegion(self) as func_name:
			def handle_failure():
				string_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % string_info['py_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.string_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成属性路径类型getter方法
	def _build_prop_getter_field_path(self):
		with GetterRegion(self) as func_name:
			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "convert to FieldPath failed, src type is \'%s\'");' % self.prop_info['type'])
				self.cb.add_line('return nullptr;')

			ConvertHelper.field_path_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True, handle_failure)

		return func_name

	# 生成属性路径类型setter方法
	def _build_prop_setter_field_path(self):
		with SetterRegion(self) as func_name:
			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'FieldPath\' or \'str\'");')
				self.cb.add_line('return -1;')

			ConvertHelper.field_path_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成SoftObjectPtr类型getter方法
	def _build_prop_getter_soft(self):
		with GetterRegion(self) as func_name:
			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "convert to SoftObjectPtr failed, src type is \'%s\'");' % self.prop_info['type'])
				self.cb.add_line('return nullptr;')

			ConvertHelper.soft_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True, handle_failure)

		return func_name

	# 生成TSoftObjectPtr类型setter方法
	def _build_prop_setter_soft_object(self):
		with SetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'SoftObjectPtr\' or \'%s\'");' % self.prop_info['class_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.soft_obj_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成TWeakObjectPtr类型getter方法
	def _build_prop_getter_weak(self):
		with GetterRegion(self) as func_name:
			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "convert to WeakObjectPtr failed, src type is \'%s\'");' % self.prop_info['type'])
				self.cb.add_line('return nullptr;')

			ConvertHelper.weak_obj_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True, handle_failure)

		return func_name

	# 生成TWeakObjectPtr类型setter方法
	def _build_prop_setter_weak_object(self):
		with SetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'WeakObjectPtr\' or \'%s\'");' % self.prop_info['class_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.weak_obj_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成TSoftClassPtr类型setter方法
	def _build_prop_setter_soft_class(self):
		with SetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'SoftObjectPtr\' or \'%s\'");' % self.prop_info['class_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.soft_class_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成结构体类型getter方法
	def _build_prop_getter_struct(self):
		with GetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			ConvertHelper.struct_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True)

		return func_name

	# 生成结构体类型setter方法
	def _build_prop_setter_struct(self):
		with SetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % self.prop_info['struct_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.struct_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成类对象类型getter方法
	def _build_prop_getter_object(self):
		with GetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			ConvertHelper.object_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True)

		return func_name

	# 生成类对象类型setter方法
	def _build_prop_setter_object(self):
		with SetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['py_header_path'])

			def handle_failure(*args):
				if len(args) == 0:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % self.prop_info['class_name'])
				else:
					self.cb.add_line('PyErr_SetString(PyExc_TypeError, "Value %s");' % args[0])
				self.cb.add_line('return -1;')

			ConvertHelper.object_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成类类型getter方法
	def _build_prop_getter_class(self):
		with GetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['header'])

			class_name = self.prop_info['class_name']
			class_header = ExportConfig.SpecialClassHeaders.get(class_name)
			if class_header:
				self._include(class_header)

			ConvertHelper.class_prop_to_py(self.cb, self.prop_info, self.cpp_name, self.out_py_name, True)

		return func_name

	# 生成类类型setter方法
	def _build_prop_setter_class(self):
		with SetterRegion(self) as func_name:
			type_info = self.prop_info['type_info']
			self._include(type_info['header'])

			class_name = self.prop_info['class_name']
			class_header = ExportConfig.SpecialClassHeaders.get(class_name)
			if class_header:
				self._include(class_header)

			def handle_failure():
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'UClass<%s>\'");' % self.prop_info['meta_class_name'])
				self.cb.add_line('return -1;')

			ConvertHelper.class_prop_to_cpp(self.cb, self.prop_info, self.in_py_name, self.cpp_name, False, handle_failure)

		return func_name

	# 生成数组类型getter方法
	def _build_prop_getter_array(self):
		if self.for_struct:
			if not PropHelpers.has_export_struct_type(self.outer_type_info):
				return

		with GetterRegion(self) as func_name:
			if self.for_struct:
				self.cb.add_line('PyObject* %s = FNePyStructArrayWrapper::New(InSelf, TBaseStructure<%s>::Get(), InSelf->Value, &%s, "%s");' % (self.out_py_name, self.outer_type_info['cpp_name'], self.cpp_name, self.prop_name))
			else:
				self.cb.add_line('PyObject* %s = FNePyArrayWrapper::New(InSelf->GetValue(), &%s, "%s");' % (self.out_py_name, self.cpp_name, self.prop_name))

		self._include('NePyArrayWrapper.h')

		return func_name

	# 生成数组类型setter方法
	def _build_prop_setter_array(self):
		with SetterRegion(self) as func_name:
			if self.for_struct:
				cpp_class = 'TBaseStructure<%s>::Get()' % self.outer_type_info['cpp_name']
				cpp_instance = 'InSelf->Value'
			else:
				cpp_class = 'InSelf->GetValue()->GetClass()'
				cpp_instance = 'InSelf->GetValue()'

			self.cb.add_line('if (!FNePyArrayWrapper::Assign(InValue, %s, %s, &%s, "%s"))' % (cpp_class, cpp_instance, self.cpp_name, self.prop_name))
			with CodeBlock(self.cb):
				type_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % type_info['py_name'])
				self.cb.add_line('return -1;')

		self._include('NePyArrayWrapper.h')

		return func_name

	# 生成字典类型getter方法
	def _build_prop_getter_map(self):
		if self.for_struct:
			if not PropHelpers.has_export_struct_type(self.outer_type_info):
				return

		with GetterRegion(self) as func_name:
			if self.for_struct:
				self.cb.add_line('PyObject* %s = FNePyStructMapWrapper::New(InSelf, TBaseStructure<%s>::Get(), InSelf->Value, &%s, "%s");' % (self.out_py_name, self.outer_type_info['cpp_name'], self.cpp_name, self.prop_name))
			else:
				self.cb.add_line('PyObject* %s = FNePyMapWrapper::New(InSelf->GetValue(), &%s, "%s");' % (self.out_py_name, self.cpp_name, self.prop_name))

		self._include('NePyMapWrapper.h')

		return func_name

	# 生成字典类型setter方法
	def _build_prop_setter_map(self):
		with SetterRegion(self) as func_name:
			if self.for_struct:
				cpp_class = 'TBaseStructure<%s>::Get()' % self.outer_type_info['cpp_name']
				cpp_instance = 'InSelf->Value'
			else:
				cpp_class = 'InSelf->GetValue()->GetClass()'
				cpp_instance = 'InSelf->GetValue()'

			self.cb.add_line('if (!FNePyMapWrapper::Assign(InValue, %s, %s, &%s, "%s"))' % (cpp_class, cpp_instance, self.cpp_name, self.prop_name))
			with CodeBlock(self.cb):
				type_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % type_info['py_name'])
				self.cb.add_line('return -1;')

		self._include('NePyMapWrapper.h')

		return func_name

	# 生成集合类型getter方法
	def _build_prop_getter_set(self):
		if self.for_struct:
			if not PropHelpers.has_export_struct_type(self.outer_type_info):
				return

		with GetterRegion(self) as func_name:
			if self.for_struct:
				self.cb.add_line('PyObject* %s = FNePyStructSetWrapper::New(InSelf, TBaseStructure<%s>::Get(), InSelf->Value, &%s, "%s");' % (self.out_py_name, self.outer_type_info['cpp_name'], self.cpp_name, self.prop_name))
			else:
				self.cb.add_line('PyObject* %s = FNePySetWrapper::New(InSelf->GetValue(), &%s, "%s");' % (self.out_py_name, self.cpp_name, self.prop_name))

		self._include('NePySetWrapper.h')

		return func_name

	# 生成集合类型setter方法
	def _build_prop_setter_set(self):
		with SetterRegion(self) as func_name:
			if self.for_struct:
				cpp_class = 'TBaseStructure<%s>::Get()' % self.outer_type_info['cpp_name']
				cpp_instance = 'InSelf->Value'
			else:
				cpp_class = 'InSelf->GetValue()->GetClass()'
				cpp_instance = 'InSelf->GetValue()'

			self.cb.add_line('if (!FNePySetWrapper::Assign(InValue, %s, %s, &%s, "%s"))' % (cpp_class, cpp_instance, self.cpp_name, self.prop_name))
			with CodeBlock(self.cb):
				type_info = self.prop_info['type_info']
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % type_info['py_name'])
				self.cb.add_line('return -1;')

		self._include('NePySetWrapper.h')

		return func_name

	# 生成静态数组类型getter方法
	def _build_prop_getter_fixed_array(self):
		if self.for_struct:
			if not PropHelpers.has_export_struct_type(self.outer_type_info):
				return

		with GetterRegion(self) as func_name:
			if self.for_struct:
				self.cb.add_line('PyObject* %s = FNePyStructFixedArrayWrapper::New(InSelf, TBaseStructure<%s>::Get(), InSelf->Value, &%s, "%s");' % (self.out_py_name, self.outer_type_info['cpp_name'], self.cpp_name, self.prop_name))
			else:
				self.cb.add_line('PyObject* %s = FNePyFixedArrayWrapper::New(InSelf->GetValue(), &%s, "%s");' % (self.out_py_name, self.cpp_name, self.prop_name))

		self._include('NePyFixedArrayWrapper.h')

		return func_name

	# 生成静态数组类型setter方法
	def _build_prop_setter_fixed_array(self):
		with SetterRegion(self) as func_name:
			if self.for_struct:
				cpp_class = 'TBaseStructure<%s>::Get()' % self.outer_type_info['cpp_name']
				cpp_instance = 'InSelf->Value'
			else:
				cpp_class = 'InSelf->GetValue()->GetClass()'
				cpp_instance = 'InSelf->GetValue()'

			self.cb.add_line('if (!FNePyFixedArrayWrapper::Assign(InValue, %s, %s, &%s, "%s"))' % (cpp_class, cpp_instance, self.cpp_name, self.prop_name))
			with CodeBlock(self.cb):
				type_info = self.prop_info['type_info']
				type_name = type_info['py_name']
				type_name = type_name[5:-1]  # strip list()
				type_name = '%s[%d]' % (type_name, self.prop_info['array_dim'])
				self.cb.add_line('PyErr_SetString(PyExc_TypeError, "argument type must be \'%s\'");' % type_name)
				self.cb.add_line('return -1;')

		self._include('NePyFixedArrayWrapper.h')

		return func_name

	# 生成DynamicDelegate类型getter方法
	def _build_prop_getter_dynamic_delegate(self):
		if self.for_struct:
			# struct真的会有delegate吗？
			return

		with GetterRegion(self) as func_name:
			self.cb.add_line('PyObject* %s = FNePyDynamicDelegateWrapper::New(InSelf->GetValue(), &%s, "%s");' % (self.out_py_name, self.cpp_name, self.prop_name))

		self._include('NePyDynamicDelegateWrapper.h')

		return func_name

	# 生成DynamicMulticastDelegate类型getter方法
	def _build_prop_getter_dynamic_multicast_delegate(self):
		if self.for_struct:
			# struct真的会有delegate吗？
			return

		with GetterRegion(self) as func_name:
			self.cb.add_line('PyObject* %s = FNePyDynamicMulticastDelegateWrapper::New(InSelf->GetValue(), &%s, "%s");' % (self.out_py_name, self.cpp_name, self.prop_name))

		self._include('NePyDynamicMulticastDelegateWrapper.h')

		return func_name

	def _parse_prop_info(self):
		self.is_builtin_type = PropHelpers.is_builtin_type(self.prop_info)
		self.is_enum_type = PropHelpers.is_enum_type(self.prop_info)
		self.is_string_type = PropHelpers.is_string_type(self.prop_info)
		self.is_array_type = PropHelpers.is_array_type(self.prop_info)
		prop_flags = self.prop_info['prop_flags']
		self.is_readonly_type = False  # 去掉 ReadOnly 访问限制，但先保留这个功能。

	def _include(self, header):
		# type: (str) -> None
		if header not in self._need_header_set:
			self._need_header_set.add(header)
			self._need_header_list.append(header)


def test():
	struct_name = 'NePyStructType_Guid'
	prop_info = {
		"name": "A",
		"pretty_name": "A",
		"prop_flags": 6755469251052033,
		"type": "IntProperty"
	}

	prop_builder = PropBuilder(prop_info)
	result = prop_builder.build_prop_getset(struct_name, None)
	print('-------------------------------------')
	print(result['getset_body'])
	print('-------------------------------------')


if __name__ == '__main__':
	dlog.set_log_level(dlog.LOG_LEVEL_DEBUG)
	test()
