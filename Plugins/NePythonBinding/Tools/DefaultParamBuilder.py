# -*- encoding: utf-8 -*-
import re
import UE4Flags
import PropHelpers
from CodeBuilder import CodeBuilder

# 处理参数默认值
#
# 参数说明
# cb: CodeBuilder
# prop_info: 经过PropHelpers.parse_prop_info处理的数据
#            索引至reflection_infos.json里的prop_info项
# in_cpp_name: C++变量名称
# in_py_name: Python变量名称
#
# 注意
# 关于UE4是如何把C++的默认参数变成这种扭曲的字符串形式，
# 可以参见FHeaderParser::DefaultValueStringCppFormatToInnerFormat。
class DefaultParamBuilder():
	def __init__(self, cb, prop_info, in_py_name, in_cpp_name):
		# type: (CodeBuilder, dict, str, str) -> None
		self.cb = cb
		self.prop_info = prop_info
		self.in_py_name = in_py_name
		self.in_cpp_name = in_cpp_name

		self.type_builder = self._build_cpp_type_default
		self.value_builder = None
		self.has_default_value = 'default' in self.prop_info
		self.is_struct_type = False
		if self.has_default_value:
			prop_type = self.prop_info['type']
			if PropHelpers.is_array_type(prop_info):
				self.value_builder = self._build_default_value_array
			elif prop_type == 'SetProperty':
				self.value_builder = self._build_default_value_set
			elif prop_type == 'MapProperty':
				self.value_builder = self._build_default_value_map
			elif PropHelpers.is_builtin_type(prop_info):
				self.value_builder = self._build_default_value_builtin
			elif PropHelpers.is_enum_type(prop_info):
				self.type_builder = self._build_cpp_type_enum
				self.value_builder = self._build_default_value_enum
			elif PropHelpers.is_string_type(prop_info):
				self.value_builder = self._build_default_value_string
			elif prop_type == 'StructProperty':
				self.value_builder = self._build_default_value_struct
				self.is_struct_type = True
			elif prop_type == 'ObjectProperty':
				self.value_builder = self._build_default_value_object
			elif prop_type == 'ClassProperty':
				self.type_builder = self._build_cpp_type_class
				self.value_builder = self._build_default_value_class

		self.default_value = None
		if self.value_builder:
			self.default_value = self.value_builder()
			prop_info['default'] = self.default_value # 处理后的参数默认值

	def __enter__(self):
		if self.has_default_value:
			if self.default_value is not None:
				cpp_type = self.type_builder()
				self.cb.add_line('%s %s = %s;' % (cpp_type, self.in_cpp_name, self.default_value))
				if self.default_value == 'nullptr':
					self.cb.add_line('if (%s && %s != Py_None)' % (self.in_py_name, self.in_py_name))
				else:
					self.cb.add_line('if (%s)' % self.in_py_name)
				self.cb.begin_block()
			else:
				self.cb.add_line('// unsupported default value %s %s [unknown type]' % (self.in_cpp_name, self.prop_info['type']))

		return self.default_value is not None

	def __exit__(self, exc_type, exc_val, exc_tb):
		if self.default_value is not None:
			self.cb.end_block()

	def _build_cpp_type_default(self):
		cpp_type = self.prop_info['type_info']['cpp_name']
		if PropHelpers.is_pointer_type(self.prop_info):
			cpp_type += '*'
		elif self.is_struct_type and self.default_value == 'nullptr':
			cpp_type += '*'
		return cpp_type

	def _build_default_value_array(self):
		# unsupported
		return None

	def _build_default_value_set(self):
		# unsupported
		return None

	def _build_default_value_map(self):
		# unsupported
		return None

	def _build_default_value_builtin(self):
		prop_type = self.prop_info['type']
		default_value = self.prop_info['default']

		if prop_type == 'FloatProperty':
			# libclang解析的结果后已经带了f，或者可能是个宏
			if PropHelpers.is_number(default_value):
				default_value += 'f'

		return default_value

	def _build_cpp_type_enum(self):
		return self.prop_info['enum_cpp_name']

	def _build_default_value_enum(self):
		default_value = self.prop_info['default']
		enum_cpp_name = self.prop_info['enum_cpp_name']
		enum_cpp_form = self.prop_info['enum_cpp_form']
		if enum_cpp_form == UE4Flags.ECF_Regular:
			pass
		elif enum_cpp_form == UE4Flags.ECF_Namespaced:
			index = enum_cpp_name.find(':')
			default_value = '%s::%s' % (enum_cpp_name[:index], default_value)
		else: # enum_cpp_form == UE4Flags.ECF_EnumClass:
			default_value = '%s::%s' % (enum_cpp_name, default_value)

		return default_value

	def _build_default_value_string(self):
		default_value = self.prop_info['default']

		cpp_name_pat = re.compile(r'(FString|FName|FText)\(')
		result = cpp_name_pat.match(default_value)
		if result:
			return default_value

		if default_value.startswith('"'):
			default_value = 'TEXT(%s)' % default_value
		else:
			prop_type = self.prop_info['type']
			if prop_type == 'NameProperty' and default_value == 'None':
				# FName特殊处理，这玩意是个名为EName的Enum，定义于UnrealNames.h
				default_value = 'NAME_None'
			elif prop_type == 'TextProperty':
				if default_value == '':
					default_value = 'FText::GetEmpty()'
				else:
					pass
			else:
				default_value = 'TEXT("%s")' % default_value
		return default_value

	def _build_default_value_struct(self):
		# UE4对以下几种结构体做了特殊处理（可能是为了性能吧）
		default_value = self.prop_info['default']
		struct_name = self.prop_info['struct_name']

		# 处理libclang获取的默认参数值
		if default_value == 'NULL':
			return 'nullptr'
		elif '::' in default_value:
			return default_value

		if struct_name == 'Vector':
			if not default_value:
				return 'FVector::ZeroVector'
			if default_value.endswith('Vector'):
				if not default_value.startswith('FVector::'):
					default_value = 'FVector::' + default_value
				return default_value
			if default_value.startswith('('):
				# (X=1,Y=1,z=1) => (1,1,1)
				default_value = default_value[1:-1]
				default_value = [v[2:] for v in default_value.split(',')]
			else:
				default_value = default_value.split(',')
			value_tuple = tuple(float(v) for v in default_value)
			if value_tuple == (1, 1, 1):
				return 'FVector::OneVector'
			elif value_tuple == (0, 0, 1):
				return 'FVector::UpVector'
			elif value_tuple == (0, 0, -1):
				return 'FVector::DownVector'
			elif value_tuple == (1, 0, 0):
				return 'FVector::ForwardVector'
			elif value_tuple == (-1, 0, 0):
				return 'FVector::BackwardVector'
			elif value_tuple == (0, 1, 0):
				return 'FVector::RightVector'
			elif value_tuple == (0, -1, 0):
				return 'FVector::LeftVector'
			default_value = 'f, '.join(default_value) + 'f'
			return 'FVector(%s)' % default_value
		elif struct_name == 'Vector2D':
			if not default_value:
				return 'FVector2D::ZeroVector'
			if default_value.endswith('Vector') or default_value == 'Unit45Deg':
				if not default_value.startswith('FVector2D::'):
					default_value = 'FVector2D::' + default_value
				return default_value
			if default_value.startswith('('):
				# (X=1,Y=1) => (1,1)
				default_value = default_value[1:-1]
				default_value = [v[2:] for v in default_value.split(',')]
			else:
				default_value = default_value.split(',')
			value_tuple = tuple(float(v) for v in default_value)
			if value_tuple == (1, 1):
				return 'FVector2D::UnitVector'
			default_value = 'f, '.join(default_value) + 'f'
			return 'FVector2D(%s)' % default_value
		elif struct_name == 'Rotator':
			if not default_value:
				return 'FRotator::ZeroRotator'
			if default_value.endswith('Rotator'):
				if not default_value.startswith('FRotator::'):
					default_value = 'FRotator::' + default_value
				return default_value
			default_value = default_value.split(',')
			default_value = 'f, '.join(default_value) + 'f'
			return 'FRotator(%s)' % default_value
		elif struct_name == 'LinearColor':
			if not default_value:
				return 'FLinearColor(EForceInit)'
			# (R=%f,G=%f,B=%f,A=%f) => %f,%f,%f,%f
			default_value = default_value[1:-1]
			default_value = [v[2:] for v in default_value.split(',')]
			default_value = 'f, '.join(default_value) + 'f'
			return 'FLinearColor(%s)' % default_value
		elif struct_name == 'Color':
			if not default_value:
				return 'FColor(EForceInit)'
			# (R=%f,G=%f,B=%f,A=%f) => %f,%f,%f,%f
			default_value = default_value[1:-1]
			default_value = [v[2:] for v in default_value.split(',')]
			default_value = ', '.join(default_value)
			return 'FColor(%s)' % default_value

	def _build_default_value_object(self):
		default_value = self.prop_info['default']
		if default_value == 'None':
			# 目前只能处理这种
			return 'nullptr'

	def _build_cpp_type_class(self):
		return 'UClass*'

	def _build_default_value_class(self):
		default_value = self.prop_info['default']
		if default_value == 'None':
			# 目前只能处理这种
			return 'nullptr'
		
