# -*- encoding: utf-8 -*-
import re
from clang.cindex import TranslationUnit
from clang.cindex import CursorKind

try:
	import typing
except:
	pass

# 判断符号是否需要被导出
class FilterRules(object):
	def __init__(self, exclude_rule):
		# type: (dict) -> None
		self.struct_exclude_names = exclude_rule.get('structs', {}) # type: dict[str, bool|dict]

		# need to be compatible with py2 and py3
		self.struct_exclude_pats = [k for k in self.struct_exclude_names.keys() if (not isinstance(k, str))] # type: list[re.Pattern]

		self.class_exclude_names = exclude_rule.get('classes', {}) # type: dict[str, bool|dict]

		# need to be compatible with py2 and py3
		self.class_exclude_pats = [k for k in self.class_exclude_names.keys() if (not isinstance(k, str))] # type: list[re.Pattern]

	# 是否需要导出结构体
	def should_export_struct(self, struct_name):
		# type: (str) -> bool
		if self.struct_exclude_names.get(struct_name) is True:
			return False
			
		for pat in self.struct_exclude_pats:
			if pat.match(struct_name):
				return False
		
		return True

	# 是否需要导出结构体中的属性
	def should_export_struct_prop(self, struct_name, prop_name):
		# type: (str, str) -> bool
		exclude_rule = self.struct_exclude_names.get(struct_name, {}).get('props')
		if exclude_rule is None:
			return True
		if exclude_rule is True:
			return False
		return exclude_rule.get(prop_name) is not True

	# 是否需要导出结构体中的方法
	def should_export_struct_func(self, struct_name, func_name):
		# type: (str, str) -> bool
		exclude_rule = self.struct_exclude_names.get(struct_name, {}).get('funcs')
		if exclude_rule is None:
			return True
		if exclude_rule is True:
			return False
		return exclude_rule.get(func_name) is not True

	# 是否需要导出类
	def should_export_class(self, class_name):
		# type: (str) -> bool
		if self.class_exclude_names.get(class_name) is True:
			return False
			
		for pat in self.class_exclude_pats:
			if pat.match(class_name):
				return False

		return True

	# 是否需要导出类中的属性
	def should_export_class_prop(self, class_name, prop_name):
		# type: (str, str) -> bool
		exclude_rule = self.class_exclude_names.get(class_name, {}).get('props')
		if exclude_rule is None:
			return True
		if exclude_rule is True:
			return False
		return exclude_rule.get(prop_name) is not True

	# 是否需要导出类中的方法
	def should_export_class_func(self, class_name, func_name):
		# type: (str, str) -> bool
		exclude_rule = self.class_exclude_names.get(class_name, {}).get('funcs')
		if exclude_rule is None:
			return True
		if exclude_rule is True:
			return False
		return exclude_rule.get(func_name) is not True

	# 是否需要为类实例带上__dict__
	def should_add_pydict_for_class(self, class_name):
		# type: (str) -> bool
		add_pydict = self.class_exclude_names.get(class_name, {}).get('has_pydict', False)
		return add_pydict


# 给Clang用的函数导出过滤器
class FunctionFilterRules(object):
	def __init__(self, include_rule, exclude_rule):
		# type: (list[str], list[str]) -> None
		self.include_rule = None
		self.exclude_rule = None

		if include_rule is not None:
			self.include_rule = set()
			for func_signature in include_rule:
				func_signature = self._normalize_signature(func_signature)
				self.include_rule.add(func_signature)

		if exclude_rule is not None:
			self.exclude_rule = set()
			for func_signature in exclude_rule:
				func_signature = self._normalize_signature(func_signature)
				self.exclude_rule.add(func_signature)

	def should_export(self, func_signature):
		# type: (str) -> None
		if self.include_rule is not None:
			return func_signature in self.include_rule
		if self.exclude_rule is not None:
			return func_signature not in self.exclude_rule
		return True

	CLANG_ARGS = [
		# c++标准
		'-std=c++17',
		# 忽略所有warning
		'-Wno-everything',
	]

	TYPE_NAME_PAT = re.compile(r'[a-zA-Z_][0-9a-zA-Z_]*')

	# 利用libclang，将函数签名规范化
	# 例如将 int FBox2D (const FVector2D&, const FVector2D & A)
	# 转化为 FBox2D(const FVector2D &, const FVector2D &)
	def _normalize_signature(self, func_signature):
		# type: (str) -> str
		lindex = func_signature.find('(')
		rindex = func_signature.rfind(')')
		if lindex == -1 or rindex == -1:
			# 解析失败，括号不匹配
			return func_signature

		params = func_signature[lindex + 1:rindex]
		if '<' in params or '>' in params:
			# 解析失败，无法处理模板
			return func_signature

		if '::' in params:
			# 解析失败，无法处理命名空间
			return func_signature

		type_name_set = set()
		for type_name in self.TYPE_NAME_PAT.findall(params):
			if type_name not in ('const', 'volatile'):
				type_name_set.add(type_name)

		source = []
		for type_name in type_name_set:
			source.append('typedef int %s;' % type_name)
		source.append('void %s;' % func_signature)

		tu = TranslationUnit.from_source('t.cpp', args=self.CLANG_ARGS, unsaved_files=[('t.cpp', '\n'.join(source))])
		for child_cursor in tu.cursor.get_children():
			if child_cursor.kind == CursorKind.FUNCTION_DECL:
				func_signature = child_cursor.displayname
				break
		return func_signature


def test():
	from clang.cindex import Config
	Config.set_library_path('clang/native/')

	include_rule = [
		'void FBox2D ()',
		'FBox2D (  EForceInit)',
		'FBox2D ( const TArray<FVector2D>&, int32 Count)',
		'int FBox2D (const FVector2D&, const FVector2D & A)',
		'void FBox2D (const FVector2D&, const FVector2D &)',
		'FVector(const UE::Math::FVector)',
		'FSoftObjectPath(FObjectPtr)',
	]
	filter_rule = FunctionFilterRules(include_rule, None)
	print('-------------------------------------')
	from pprint import pprint
	pprint(filter_rule.include_rule)
	print('-------------------------------------')


if __name__ == '__main__':
	test()
