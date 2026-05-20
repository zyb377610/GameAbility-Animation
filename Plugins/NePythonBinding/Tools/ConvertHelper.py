# -*- encoding: utf-8 -*-
# 提供各种方法，供Python对象和C++对象进行互转换
#
# 参数说明
# cb: CodeBuilder
# prop_info: 经过PropHelpers.parse_prop_info处理的数据
#            索引至reflection_infos.json里的prop_info项
# in/out_cpp_name: C++变量名称
# in/out_py_name: Python变量名称
# declare_out: 是否需要声明返回值（若为False，则只赋值，不声明）
# handle_failure: 当转换失败后的处理函数
#
# 注意
# 所有的to_py接口，返回的都是 New reference
# 所有的to_cpp接口，均不会改变 in_py_name 变量的引用计数

import re
import UE4Flags
import PropHelpers
from CodeBuilder import CodeBlock
from CodeBuilder import CodeBuilder


CPP_OBJECT_TMPL = "TSoftObjectPtr"

CPP_CLASS_TMPL = "TSoftClassPtr"

try:
	import typing
except:
	pass

def prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	prop_type = prop_info['type']
	if PropHelpers.is_array_type(prop_info):
		array_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure)
	elif prop_type == 'SetProperty':
		set_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure)
	elif prop_type == 'MapProperty':
		map_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure)
	elif PropHelpers.is_builtin_type(prop_info):
		builtin_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif PropHelpers.is_enum_type(prop_info):
		enum_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif PropHelpers.is_string_type(prop_info):
		string_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure)
	elif prop_type == 'FieldPathProperty':
		field_path_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure)
	elif prop_type == 'StructProperty':
		struct_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif prop_type == 'ObjectProperty':
		object_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif prop_type == 'WeakObjectProperty':
		weak_object_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif prop_type == 'InterfaceProperty':
		interface_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif prop_type == 'ClassProperty':
		class_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif prop_type == 'ClassPtrProperty':
		# ClassPtrProperty 是 TObjectPtr<UClass>，和 ObjectProperty 相同处理
		object_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out)
	elif PropHelpers.is_soft_type(prop_info):
		soft_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure)

def prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	prop_type = prop_info['type']
	if PropHelpers.is_array_type(prop_info):
		array_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'SetProperty':
		set_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'MapProperty':
		map_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif PropHelpers.is_builtin_type(prop_info):
		builtin_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif PropHelpers.is_enum_type(prop_info):
		enum_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif PropHelpers.is_string_type(prop_info):
		string_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'FieldPathProperty':
		field_path_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'StructProperty':
		struct_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'ObjectProperty':
		object_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'WeakObjectProperty':
		weak_obj_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'InterfaceProperty':
		interface_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'ClassProperty':
		class_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	# elif prop_type == 'SoftClassProperty':
	# 	class_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == 'ClassPtrProperty':
		object_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == "SoftObjectProperty":
		soft_obj_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)
	elif prop_type == "SoftClassProperty":
		soft_class_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure)


def builtin_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out):
	# type: (CodeBuilder, dict, str, str, bool) -> None
	builtin_info = prop_info['type_info']
	to_py_expr = builtin_info['to_py_expr'] % in_cpp_name

	line = '%s = %s;' % (out_py_name, to_py_expr)
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

# need_temp_for_cast: 是否需要生成临时变量以处理类型转换
def builtin_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, need_temp_for_cast=True):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable, bool) -> None
	prop_type = prop_info['type']
	builtin_info = prop_info['type_info']
	cpp_type = builtin_info['cpp_name']

	# UE4会使用C++位域来表示一些特定类型属性，需要特殊处理
	if need_temp_for_cast and prop_type in ('BoolProperty', ):
		temp_cpp_name = prop_info['name'] + 'Temp'
		cb.add_line('%s %s;' % (cpp_type, temp_cpp_name))
	else:
		temp_cpp_name = None

	if declare_out:
		cb.add_line('%s %s;' % (cpp_type, out_cpp_name))

	cb.add_line('if (!NePyBase::ToCpp(%s, %s))' % (in_py_name, (temp_cpp_name if temp_cpp_name else out_cpp_name)))
	with CodeBlock(cb):
		handle_failure()

	if temp_cpp_name:
		cb.add_line('%s = %s;' % (out_cpp_name, temp_cpp_name))

def enum_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out):
	# type: (CodeBuilder, dict, str, str, bool) -> None
	builtin_info = prop_info['type_info']
	cpp_type = builtin_info['cpp_name']
	in_cpp_name = '(%s)%s' % (cpp_type, in_cpp_name)
	to_py_expr = builtin_info['to_py_expr'] % in_cpp_name

	line = '%s = %s;' % (out_py_name, to_py_expr)
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

def enum_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	prop_type = prop_info['type']
	if prop_type == 'EnumProperty':
		underlying_type = prop_info['enum_underlying_type']
		builtin_info = PropHelpers.BUILTIN_TYPES[underlying_type]
	else: # ByteProperty
		builtin_info = PropHelpers.BUILTIN_TYPES[prop_type]

	cpp_type = builtin_info['cpp_name']
	enum_cpp_name = prop_info['enum_cpp_name']

	temp_cpp_name = prop_info['name'] + 'Temp'
	cb.add_line('%s %s;' % (cpp_type, temp_cpp_name))
	cb.add_line('if (!NePyBase::ToCpp(%s, %s))' % (in_py_name, temp_cpp_name))
	with CodeBlock(cb):
		handle_failure()

	line = '%s = (%s)%s;' % (out_cpp_name, enum_cpp_name, temp_cpp_name)
	if declare_out:
		line = ('%s ' % enum_cpp_name) + line
	cb.add_line(line)

def string_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	if declare_out:
		cb.add_line('PyObject* %s;' % (out_py_name))

	cb.add_line('if (!NePyBase::ToPy(%s, %s))' % (in_cpp_name, out_py_name))
	with CodeBlock(cb):
		handle_failure()

def string_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	prop_type = prop_info['type']
	string_info = PropHelpers.STRING_TYPES[prop_type]
	cpp_type = string_info['cpp_name']

	if declare_out:
		cb.add_line('%s %s;' % (cpp_type, out_cpp_name))

	cb.add_line('if (!NePyBase::ToCpp(%s, %s))' % (in_py_name, out_cpp_name))
	with CodeBlock(cb):
		handle_failure()

def field_path_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	if declare_out:
		cb.add_line('PyObject* %s;' % (out_py_name))

	cb.add_line('if (!NePyBase::ToPy(%s, %s))' % (in_cpp_name, out_py_name))
	with CodeBlock(cb):
		handle_failure()

def field_path_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	if declare_out:
		cb.add_line('TFieldPath<F%s> %s;' % (prop_info['class_name'], out_cpp_name))

	cb.add_line('if (!NePyBase::ToCpp(%s, %s))' % (in_py_name, out_cpp_name))
	with CodeBlock(cb):
		handle_failure()


def soft_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	if declare_out:
		cb.add_line('PyObject* %s;' % (out_py_name))
	pure_name = out_py_name.split('.')[-1]
	temp_value_name = "Temp%s" % pure_name
	cb.add_line("FSoftObjectPtr* %s = (FSoftObjectPtr*)&%s;" % (temp_value_name, in_cpp_name))
	cb.add_line('if (!NePyBase::ToPy(%s, %s))' % (temp_value_name, out_py_name))
	with CodeBlock(cb):
		handle_failure()


def soft_obj_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	soft_info = prop_info['type_info']
	cpp_type = soft_info['cpp_name']
	if declare_out:
		cb.add_line('TSoftObjectPtr<%s> %s;' % (cpp_type, out_cpp_name))
	pure_name = out_cpp_name.split('.')[-1].split('->')[-1]
	temp_value_name = "SoftPtr%s" % pure_name
	cb.add_line("FSoftObjectPtr* %s = (FSoftObjectPtr*)&%s;" % (temp_value_name, out_cpp_name))
	cb.add_line('if (!NePyBase::ToCpp(%s, *%s))' % (in_py_name, temp_value_name))
	with CodeBlock(cb):
		inner_name = 'SoftPtrObjectTemp'
		inner_out_cpp_name = '%sValue' % inner_name
		fake_inner_prop = {
			"name": inner_name,
			"pretty_name": "inner_name",
			"prop_flags": prop_info["prop_flags"],
			"array_dim": 1,
			"doc": "",
			"type": "ObjectProperty",
			"class_name": soft_info["name"],
			"meta_data": {},
			"type_info": soft_info,
		}
		prop_to_cpp(cb, fake_inner_prop, in_py_name, inner_out_cpp_name, True, lambda *args: handle_failure())
		cb.add_line('*SoftPtr%s = %s;' % (pure_name, inner_out_cpp_name))

def soft_class_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	soft_info = prop_info['type_info']
	cpp_type = soft_info['cpp_name']
	if declare_out:
		cb.add_line('TSoftClassPtr<%s> %s;' % (cpp_type, out_cpp_name))
	pure_name = out_cpp_name.split('.')[-1].split('->')[-1]
	temp_value_name = "SoftPtr%s" % pure_name
	cb.add_line("FSoftObjectPtr* %s = (FSoftObjectPtr*)&%s;" % (temp_value_name, out_cpp_name))
	cb.add_line('if (!NePyBase::ToCpp(%s, *%s))' % (in_py_name, temp_value_name))
	with CodeBlock(cb):
		inner_name = 'SoftPtrClassTemp'
		inner_out_cpp_name = '%sValue' % inner_name
		fake_inner_prop = {
			"name": inner_name,
			"pretty_name": "inner_name",
			"prop_flags": prop_info["prop_flags"],
			"array_dim": 1,
			"doc": "",
			"type": "ClassProperty",
			"class_name": soft_info["name"],
			"meta_data": {},
			"type_info": soft_info,
		}
		prop_to_cpp(cb, fake_inner_prop, in_py_name, inner_out_cpp_name, True, lambda *args: handle_failure())
		cb.add_line('*SoftPtr%s = %s;' % (pure_name, inner_out_cpp_name))

def weak_obj_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	if declare_out:
		cb.add_line('PyObject* %s;' % (out_py_name))
	pure_name = out_py_name.split('.')[-1]
	temp_value_name = "Temp%s" % pure_name
	cb.add_line("FWeakObjectPtr* %s = (FWeakObjectPtr*)&%s;" % (temp_value_name, in_cpp_name))
	cb.add_line('if (!NePyBase::ToPy(%s, %s))' % (temp_value_name, out_py_name))
	with CodeBlock(cb):
		handle_failure()

def weak_obj_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	soft_info = prop_info['type_info']
	cpp_type = soft_info['cpp_name']
	if declare_out:
		cb.add_line('TWeakObjectPtr<%s> %s;' % (cpp_type, out_cpp_name))
	pure_name = out_cpp_name.split('.')[-1].split('->')[-1]
	temp_value_name = "WeakPtr%s" % pure_name
	cb.add_line("FWeakObjectPtr* %s = (FWeakObjectPtr*)&%s;" % (temp_value_name, out_cpp_name))
	cb.add_line('if (!NePyBase::ToCpp(%s, *%s))' % (in_py_name, temp_value_name))
	with CodeBlock(cb):
		inner_name = 'WeakPtrObjectTemp'
		inner_out_cpp_name = '%sValue' % inner_name
		fake_inner_prop = {
			"name": inner_name,
			"pretty_name": "inner_name",
			"prop_flags": prop_info["prop_flags"],
			"array_dim": 1,
			"doc": "",
			"type": "ObjectProperty",
			"class_name": soft_info["name"],
			"meta_data": {},
			"type_info": soft_info,
		}
		prop_to_cpp(cb, fake_inner_prop, in_py_name, inner_out_cpp_name, True, lambda *args: handle_failure())
		cb.add_line('*WeakPtr%s = %s;' % (pure_name, inner_out_cpp_name))

def delegate_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, func_name, is_static=False, static_type_name=''):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable, str, bool, str) -> None
	delegate_info = prop_info['type_info']
	cpp_type = delegate_info['cpp_name']
	if is_static:
		owner_object = '%s::StaticClass()->GetDefaultObject()' % (static_type_name, )
	else:
		owner_object = 'InSelf->GetValue()'

	if declare_out:
		cb.add_line('%s %s;' % (cpp_type, out_cpp_name))
	cb.add_line('if (PyCallable_Check(%s))' % (in_py_name))
	with CodeBlock(cb):
		if is_static:
			cb.add_line('NePyStealReference(FNePyHouseKeeper::Get().NewNePyObject(%s));' % (owner_object, ))
		cb.add_line('UNePyDelegate* PyDelegate = FNePyDynamicDelegateParam::FindOrAddNePyDelegate(%s, "%s", "%s", %s);'
			% (owner_object, func_name, out_cpp_name, in_py_name))
		cb.add_line('if (!PyDelegate)')
		with CodeBlock(cb):
			handle_failure(True)
		cb.add_line('%s.BindUFunction(PyDelegate, UNePyDelegate::FakeFuncName);' % out_cpp_name)
	cb.add_line('else')
	with CodeBlock(cb):
		handle_failure()

# out_is_pointer:
# 	输出变量是否为指针类型，这是一项优化。
# 	若输出变量类型为StructProperty，且为指针类型，可节省一次值拷贝。
def wrapper_type_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, is_object_prop, is_interface_prop, out_is_pointer=False):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable, bool, bool, bool) -> None
	type_info = prop_info['type_info']
	py_type = type_info['py_name']
	cpp_type = type_info['cpp_name']
	temp_py_name = 'Py' + prop_info['name']

	if declare_out:
		if is_interface_prop:
			cb.add_line('TScriptInterface<I%s> %s;' % (type_info['name'], out_cpp_name))
		elif is_object_prop or out_is_pointer:
			cb.add_line('%s* %s;' % (cpp_type, out_cpp_name))
		else:
			cb.add_line('%s %s;' % (cpp_type, out_cpp_name))

	is_allow_nullptr = PropHelpers.is_allow_none_type(prop_info)
	if is_allow_nullptr:
		cb.add_line('if (%s == Py_None)' % in_py_name)
		with CodeBlock(cb):
			cb.add_line('%s = nullptr;' % out_cpp_name)
	if_seg = 'else if' if is_allow_nullptr else 'if'
	if is_interface_prop:
		cb.add_line('%s (FNePyObjectBase* %s = NePyObjectBaseCheck(%s))' % (if_seg, temp_py_name, in_py_name))
	else:
		cb.add_line('%s (%s* %s = %s(%s))' % (if_seg, py_type, temp_py_name, type_info['py_check_func_name'], in_py_name))
	with CodeBlock(cb):
		if is_interface_prop:
			cb.add_line('if (FNePyHouseKeeper::Get().IsValid(%s))' % temp_py_name)
			with CodeBlock(cb):
				cb.add_line('if (%s->Value->Implements<%s>())' % (temp_py_name, cpp_type))
				with CodeBlock(cb):
					cb.add_line('%s = (%s*)%s->Value;' % (out_cpp_name, cpp_type, temp_py_name))
				cb.add_line('else')
				with CodeBlock(cb):
					handle_failure()
			cb.add_line('else')
			with CodeBlock(cb):
				handle_failure('underlying UObject is invalid')
		elif is_object_prop:
			cb.add_line('if (FNePyHouseKeeper::Get().IsValid(%s))' % temp_py_name)
			with CodeBlock(cb):
				cb.add_line('%s = %s->GetValue();' % (out_cpp_name, temp_py_name))
			cb.add_line('else')
			with CodeBlock(cb):
				handle_failure('underlying UObject is invalid')
		elif out_is_pointer:
			cb.add_line('%s = (%s*)%s->Value;' % (out_cpp_name, cpp_type, temp_py_name))
		else:
			cb.add_line('%s = *((%s*)%s->Value);' % (out_cpp_name, cpp_type, temp_py_name))
	cb.add_line('else')
	with CodeBlock(cb):
		handle_failure()

def struct_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, src_is_pointer=False):
	# type: (CodeBuilder, dict, str, str, bool, bool) -> None
	struct_info = prop_info['type_info']
	if struct_info.get('is_boxing_type'):
		cpp_type = struct_info['cpp_name']
		line = '%s = %s(const_cast<%s&>(%s));' % (out_py_name, struct_info['unboxing_func_name'], cpp_type, in_cpp_name)
	elif src_is_pointer:
		line = '%s = %s(*%s);' % (out_py_name, struct_info['py_new_func_name'], in_cpp_name)
	else:
		line = '%s = %s(%s);' % (out_py_name, struct_info['py_new_func_name'], in_cpp_name)
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

def struct_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, out_is_pointer=False):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable, bool) -> None
	type_info = prop_info['type_info']
	if not type_info.get('is_boxing_type'):
		wrapper_type_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, False, False, out_is_pointer)
		return

	# 避免函数参数传递时多余的解引用操作
	# 这是一个飞线，丑陋，但有效
	prop_info['src_is_not_pointer'] = True

	py_type = type_info['py_name']
	cpp_type = type_info['cpp_name']
	temp_py_name = 'Py' + prop_info['name']

	if declare_out:
		cb.add_line('%s %s;' % (cpp_type, out_cpp_name))

	cb.add_line('if (%s* %s = %s(%s))' % (py_type, temp_py_name, type_info['py_check_func_name'], in_py_name))
	with CodeBlock(cb):
		cb.add_line('%s = (%s*)%s->Value;' % (out_cpp_name, cpp_type, temp_py_name))
	cb.add_line('else if (!%s(%s, %s))' % (type_info['boxing_func_name'], in_py_name, out_cpp_name))
	with CodeBlock(cb):
		handle_failure()

def object_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out):
	# type: (CodeBuilder, dict, str, str, bool) -> None
	line = '%s = NePyBase::ToPy(%s);' % (out_py_name, in_cpp_name)
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

def object_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	wrapper_type_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, True, False)

def interface_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out):
	# type: (CodeBuilder, dict, str, str, bool) -> None
	line = '%s = NePyBase::ToPy(%s.GetObject());' % (out_py_name, in_cpp_name)
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

def interface_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	wrapper_type_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure, False, True)

def class_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out):
	# type: (CodeBuilder, dict, str, str, bool) -> None
	line = '%s = NePyBase::ToPy(%s);' % (out_py_name, in_cpp_name)
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

def class_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	prop_flags = prop_info['prop_flags']
	# ClassProperty很特殊，type_info中的cpp_type指的是它持有的类型
	# 这里需要的是ClassProperty自身的类型，例如UClass，UBlueprintGeneratedClass
	prop_cpp_type = 'U' + prop_info['class_name']
	class_info = prop_info['type_info']
	cpp_type = class_info['cpp_name']

	if declare_out:
		cb.add_line('UClass* %s;' % out_cpp_name)

	if not declare_out and prop_flags & UE4Flags.CPF_UObjectWrapper:
		# 在判空之前先把 Temp 名称定义了
		temp_cpp_name = prop_info['name'] + 'Temp'
		cb.add_line('UClass* %s;' % temp_cpp_name)
	
	is_allow_nullptr = PropHelpers.is_allow_none_type(prop_info)
	if is_allow_nullptr:
		cb.add_line('if (%s == Py_None)' % in_py_name)
		with CodeBlock(cb):
			cb.add_line('%s = nullptr;' % out_cpp_name)
	if_seg = 'else if' if is_allow_nullptr else 'if'

	if not declare_out and prop_flags & UE4Flags.CPF_UObjectWrapper:
		cb.add_line('%s (NePyBase::ToCpp(%s, %s, %s::StaticClass()))' % (if_seg, in_py_name, temp_cpp_name, cpp_type))
		with CodeBlock(cb):
			if prop_flags & UE4Flags.CPF_UObjectWrapper:
				cb.add_line('%s = %s;' % (out_cpp_name, temp_cpp_name))
			else:
				cb.add_line('%s = Cast<%s>(%s);' % (out_cpp_name, prop_cpp_type, temp_cpp_name))
		cb.add_line('else')
		with CodeBlock(cb):
			handle_failure()
	else:
		cb.add_line('%s (!NePyBase::ToCpp(%s, %s, %s::StaticClass()))' % (if_seg, in_py_name, out_cpp_name, cpp_type))
		with CodeBlock(cb):
			handle_failure()

def array_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	if PropHelpers.is_bytes_type(prop_info):
		# 特殊处理bytes类型
		if declare_out:
			line = 'PyObject* %s;' % out_py_name
			cb.add_line(line)

		cb.add_line('if (!NePyBase::ToPy(%s, %s))' % (in_cpp_name, out_py_name))
		with CodeBlock(cb):
			handle_failure()
	else:
		inner_prop_info = prop_info['inner_prop']
		inner_cpp_type = PropHelpers.extract_cpp_type(inner_prop_info)

		inner_num_name = valid_name(in_cpp_name) + 'Num'
		if prop_info['array_dim'] > 1:
			cb.add_line('int32 %s = %d;' % (inner_num_name, prop_info['array_dim']))
		else:
			cb.add_line('int32 %s = (int32)%s.Num();' % (inner_num_name, in_cpp_name))

		line = '%s = PyList_New(%s);' % (out_py_name, inner_num_name)
		if declare_out:
			line = 'PyObject* ' + line
		cb.add_line(line)

		inner_in_cpp_name = valid_name(in_cpp_name) + 'Inner'
		inner_out_py_name = valid_name(out_py_name) + 'Inner'
		cb.add_line('for (int32 Index = 0; Index < %s; ++Index)' % inner_num_name)
		with CodeBlock(cb):
			if PropHelpers.is_pointer_type(inner_prop_info) or PropHelpers.is_builtin_type(inner_prop_info):
				cb.add_line('%s %s = %s[Index];' % (inner_cpp_type, inner_in_cpp_name, in_cpp_name))
			else:
				cb.add_line('const %s& %s = %s[Index];' % (inner_cpp_type, inner_in_cpp_name, in_cpp_name))
			prop_to_py(cb, inner_prop_info, inner_in_cpp_name, inner_out_py_name, True, handle_failure)
			cb.add_line('PyList_SetItem(%s, Index, %s);' % (out_py_name, inner_out_py_name))

def array_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	type_info = prop_info['type_info']

	if declare_out:
		cb.add_line('%s %s;' % (type_info['cpp_name'], out_cpp_name))

	if PropHelpers.is_bytes_type(prop_info):
		# 特殊处理bytes类型
		cb.add_line('if (!NePyBase::ToCpp(%s, %s))' % (in_py_name, out_cpp_name))
		with CodeBlock(cb):
			handle_failure()
	else:
		inner_prop_info = prop_info['inner_prop']

		cb.add_line('if (PySequence_Check(%s))' % in_py_name)
		with CodeBlock(cb):
			inner_num_name = valid_name(in_py_name) + 'Num'
			if prop_info['array_dim'] > 1:
				cb.add_line('int32 %s = %d;' % (inner_num_name, prop_info['array_dim']))
			else:
				cb.add_line('int32 %s = (int32)PySequence_Size(%s);' % (inner_num_name, in_py_name))

			if declare_out:
				cb.add_line('%s.Reserve(%s);' % (out_cpp_name, inner_num_name))

			inner_in_py_name = valid_name(in_py_name) + 'Inner'
			inner_out_cpp_name = valid_name(out_cpp_name) + 'Inner'
			cb.add_line('for (int32 Index = 0; Index < %s; ++Index)' % inner_num_name)
			with CodeBlock(cb):
				cb.add_line('PyObject* %s = PySequence_GetItem(%s, Index);' % (inner_in_py_name, in_py_name))
				prop_to_cpp(cb, inner_prop_info, inner_in_py_name, inner_out_cpp_name, True, handle_failure)
				cb.add_line('%s.Add(%s);' % (out_cpp_name, inner_out_cpp_name))

		cb.add_line('else')
		with CodeBlock(cb):
			handle_failure()

def set_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	element_prop_info = prop_info['element_prop']

	line = '%s = PySet_New(nullptr);' % out_py_name
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

	element_in_cpp_name = valid_name(in_cpp_name) + 'Elem'
	element_out_py_name = valid_name(out_py_name) + 'Elem'
	cb.add_line('for (auto& %s : %s)' % (element_in_cpp_name, in_cpp_name))
	with CodeBlock(cb):
		prop_to_py(cb, element_prop_info, element_in_cpp_name, element_out_py_name, True, handle_failure)
		cb.add_line('PySet_Add(%s, %s);' % (out_py_name, element_out_py_name))
		cb.add_line('Py_DECREF(%s);' % element_out_py_name)

def set_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	type_info = prop_info['type_info']
	element_prop_info = prop_info['element_prop']

	if declare_out:
		cb.add_line('%s %s;' % (type_info['cpp_name'], out_cpp_name))

	cb.add_line('if (PyAnySet_Check(%s))' % in_py_name)
	with CodeBlock(cb):
		element_in_py_name = valid_name(in_py_name) + 'Elem'
		element_out_cpp_name = valid_name(out_cpp_name) + 'Elem'
		iter_py_name = valid_name(in_py_name) + 'Iter'
		cb.add_line('FNePyObjectPtr %s = FNePyObjectPtr::StealReference(PyObject_GetIter(%s));' % (iter_py_name, in_py_name))
		cb.add_line('while (PyObject* %s = PyIter_Next(%s))' % (element_in_py_name, iter_py_name))
		with CodeBlock(cb):
			prop_to_cpp(cb, element_prop_info, element_in_py_name, element_out_cpp_name, True, handle_failure)
			cb.add_line('%s.Add(%s);' % (out_cpp_name, element_out_cpp_name))
			cb.add_line('Py_DECREF(%s);' % element_in_py_name)

	cb.add_line('else')
	with CodeBlock(cb):
		handle_failure()

def map_prop_to_py(cb, prop_info, in_cpp_name, out_py_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	key_prop_info = prop_info['key_prop']
	value_prop_info = prop_info['value_prop']

	line = '%s = PyDict_New();' % out_py_name
	if declare_out:
		line = 'PyObject* ' + line
	cb.add_line(line)

	key_out_py_name = valid_name(out_py_name) + 'Key'
	value_out_py_name = valid_name(out_py_name) + 'Val'
	temp_key_out_py_name = key_out_py_name + 'Temp'
	temp_value_out_py_name = value_out_py_name + 'Temp'

	cb.add_line('for (auto& Pair : %s)' % in_cpp_name)
	with CodeBlock(cb):
		prop_to_py(cb, key_prop_info, 'Pair.Key', temp_key_out_py_name, True, handle_failure)
		cb.add_line('FNePyObjectPtr %s = FNePyObjectPtr::StealReference(%s);' % (key_out_py_name, temp_key_out_py_name))
		prop_to_py(cb, value_prop_info, 'Pair.Value', temp_value_out_py_name, True, handle_failure)
		cb.add_line('FNePyObjectPtr %s = FNePyObjectPtr::StealReference(%s);' % (value_out_py_name, temp_value_out_py_name))
		cb.add_line('PyDict_SetItem(%s, %s, %s);' % (out_py_name, key_out_py_name, value_out_py_name))

def map_prop_to_cpp(cb, prop_info, in_py_name, out_cpp_name, declare_out, handle_failure):
	# type: (CodeBuilder, dict, str, str, bool, typing.Callable) -> None
	type_info = prop_info['type_info']
	key_prop_info = prop_info['key_prop']
	value_prop_info = prop_info['value_prop']

	if declare_out:
		cb.add_line('%s %s;' % (type_info['cpp_name'], out_cpp_name))

	cb.add_line('if (PyDict_Check(%s))' % in_py_name)
	with CodeBlock(cb):
		key_out_cpp_name = valid_name(out_cpp_name) + 'Key'
		value_out_cpp_name = valid_name(out_cpp_name) + 'Val'
		temp_key_out_cpp_name = key_out_cpp_name + 'Temp'
		temp_value_out_cpp_name = value_out_cpp_name + 'Temp'
		key_in_py_name = valid_name(in_py_name) + 'Key'
		value_in_py_name = valid_name(in_py_name) + 'Val'
		index_py_name = valid_name(in_py_name) + 'Index'

		cb.add_line('PyObject* %s = nullptr;' % key_in_py_name)
		cb.add_line('PyObject* %s = nullptr;' % value_in_py_name)
		cb.add_line('Py_ssize_t %s = 0;' % index_py_name)
		cb.add_line('while (PyDict_Next(%s, &%s, &%s, &%s))' % (in_py_name, index_py_name, key_in_py_name, value_in_py_name))
		with CodeBlock(cb):
			prop_to_cpp(cb, key_prop_info, key_in_py_name, temp_key_out_cpp_name, True, handle_failure)
			prop_to_cpp(cb, value_prop_info, value_in_py_name, temp_value_out_cpp_name, True, handle_failure)
			cb.add_line('%s.Add(%s, %s);' % (out_cpp_name, temp_key_out_cpp_name, temp_value_out_cpp_name))

	cb.add_line('else')
	with CodeBlock(cb):
		handle_failure()


VALID_NAME_CHAR_PAT = re.compile(r'[^0-9a-zA-Z_]')

# 将名称中特殊符号都去除
def valid_name(origin_name):
	# type: (str) -> str
	return VALID_NAME_CHAR_PAT.sub('', origin_name)
