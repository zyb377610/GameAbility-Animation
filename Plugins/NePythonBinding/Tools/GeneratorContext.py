# -*- encoding: utf-8 -*-
from FilterRules import FilterRules
import DebugLog as dlog
import ExportConfig
import os
import json
import pickle
import shutil

try:
	import typing
except:
	pass

class GeneratorContext(object):
	def __init__(self):
		# 由名称索引的类
		# key:class_name val:class_info
		self.classes_by_name = {} # type: dict[str, dict]

		# 由名称索引的结构
		# key:struct_name val:struct_info
		self.structs_by_name = {} # type: dict[str, dict]

		# 由名称索引的枚举
		# key:enum_name val:enum_info
		self.enums_by_name = {} # type: dict[str, dict]

		# 由包名索引的类集合
		# key:package_name val:[class_info, ...]
		self.classes_by_package = {} # type: dict[str, list]

		# 由包名索引的类集合
		# key:package_name val:[struct_info, ...]
		self.structs_by_package = {} # type: dict[str, list]

		# 由包名索引的类集合
		# key:package_name val:[enum_info, ...]
		self.enums_by_package = {} # type: dict[str, list]
	
	def has_content(self):
		for v in self.__dict__.values():
			if v not in (None, '', [], {}, ()):
				return True
		return False

class ExportContext(object):
	def __init__(self):
		# 本次的导出信息
		self.current_gen = GeneratorContext() # type: GeneratorContext

		# 当前正在处理的包
		self.curr_package_name = None # type: str

		# 当前包的导出过滤器
		self.curr_package_filter_rules = None # type: FilterRules

		# 当前正在导出的Python模块名
		self.curr_py_module_name = None # type: str

		# 当前正在导出的module的配置
		self.curr_module_rule = None # type: dict

		# 当前的属性导出过滤规则
		self.curr_prop_filter_flags = 0

		# 当前的函数导出过滤规则
		self.curr_func_filter_flags = 0

		# 当前的属性导出过滤方法
		self.curr_prop_filter_func = None # type: typing.Callable

		# 当前的函数导出过滤方法
		self.curr_func_filter_func = None # type: typing.Callable

		# 是否启用增量导出（增量导出相关）
		self.enable_increment_gen = False # type: bool

		# 上次的导出信息（增量导出相关）
		self.last_gen = GeneratorContext() # type: GeneratorContext

		# 本次导出信息增量（增量导出相关）
		self.inc_gen = GeneratorContext() # type: GeneratorContext

		# 本次导出信息减量（增量导出相关）
		self.dec_gen = GeneratorContext() # type: GeneratorContext

		# 上次代码生成的GUID（增量导出相关）
		self.last_gen_guid = "" # type: str

		# 当前代码生成的GUID（增量导出相关）
		self.current_gen_guid = "" # type: str

		# 代码生成缓存版本文件（增量导出相关）
		self.cache_gen_version_path = "" # type: str

		# 代码生成缓存内容文件（增量导出相关）
		self.cache_gen_content_path = "" # type: str

		# 代码生成缓存是否用debug (增量导出相关）
		self.cache_gen_enable_debug = False # type: bool

	def _make_gen_diff(self, gen_lhs, gen_rhs):
		def list_diff(l1, l2):
			diff = []
			for v in l1:
				if v not in l2:
					diff.append(v)
			return diff

		def dict_diff(d1, d2):
			diff = {}
			for k, v in d1.items():
				if k not in d2:
					diff[k] = v
				elif isinstance(v, dict) and isinstance(d2[k], dict):
					sub_diff = dict_diff(v, d2[k])
					if sub_diff:
						diff[k] = sub_diff
				elif isinstance(v, list) and isinstance(d2[k], list):
					sub_diff = list_diff(v, d2[k])
					if sub_diff:
						diff[k] = sub_diff
				elif v != d2[k]:
					diff[k] = v
			return diff

		gen = GeneratorContext()
		for attr in dir(gen_lhs):
			if attr.startswith('__'):
				continue
			v1 = getattr(gen_lhs, attr)
			v2 = getattr(gen_rhs, attr, None)
			if isinstance(v1, dict) and isinstance(v2, dict):
				diff = dict_diff(v1, v2)
				setattr(gen, attr, diff)
		return gen

	def _get_sorted_gen(self, gen):
		def sort_obj(obj):
			if isinstance(obj, dict):
				return {k: sort_obj(obj[k]) for k in sorted(obj)}
			elif isinstance(obj, list):
				return [sort_obj(v) for v in obj]
			else:
				return obj

		new_gen = GeneratorContext()

		for attr in dir(gen):
			if attr not in ['classes_by_name', 'structs_by_name', 'enums_by_name']:
				continue
			sorted_value = sort_obj(getattr(gen, attr))
			setattr(new_gen, attr, sorted_value)
			for k, v in sorted_value.items():
				package = v.get('package', None)
				if package:
					if attr == 'classes_by_name':
						new_gen.classes_by_package.setdefault(package, []).append(v)
					elif attr == 'structs_by_name':
						new_gen.structs_by_package.setdefault(package, []).append(v)
					elif attr == 'enums_by_name':
						new_gen.enums_by_package.setdefault(package, []).append(v)

		for attr in dir(new_gen):
			if attr not in ['classes_by_package', 'structs_by_package', 'enums_by_package']:
				continue
			value = getattr(new_gen, attr)
			setattr(new_gen, attr, sort_obj(value))

		return new_gen
	
	def _gen_to_json_file(self, dict, filename):
		try:
			if os.path.isfile(filename):
				shutil.copy2(filename, filename + ".old")
			with open(filename, 'w', encoding='utf-8') as f:
				json.dump(dict, f, ensure_ascii=False, indent=2)
		except Exception as e:
			dlog.info(f"Failed to export generator to {filename}: {e}")
	
	def _gen_to_bin_file(self, dict, filename):
		try:
			with open(filename, 'wb') as f:
				pickle.dump(dict, f)
		except Exception as e:
			dlog.info(f"Failed to export generator to {filename}: {e}")

	def _save_gen_to_file(self, gen, filename):
		def extract_dict_members(gen):
			result = {}
			for attr in dir(gen):
				if attr.startswith('__'):
					continue
				value = getattr(gen, attr)
				if isinstance(value, dict):
					result[attr] = value
			return result

		dict_members = extract_dict_members(gen)

		def remove_file(file):
			if os.path.isfile(file):
				os.remove(file)

		json_path = filename + ".json"
		bin_path = filename + ".bin"

		if self.cache_gen_enable_debug:
			remove_file(bin_path)
			self._gen_to_json_file(dict_members, json_path)
		else:
			remove_file(json_path)
			self._gen_to_bin_file(dict_members, bin_path)

	def _json_file_to_gen(self, gen, filename):
		try:
			with open(filename, 'r', encoding='utf-8') as f:
				dict_members = json.load(f)
			for k, v in dict_members.items():
				setattr(gen, k, v)
			return gen
		except Exception as e:
			dlog.info(f"Failed to load generator from {filename}: {e}")
			return gen

	def _bin_file_to_gen(self, gen, filename):
		try:
			with open(filename, 'rb') as f:
				dict_members = pickle.load(f)
			for k, v in dict_members.items():
				setattr(gen, k, v)
			return gen
		except Exception as e:
			dlog.info(f"Failed to load generator from {filename}: {e}")
			return gen

	def _restore_last_gen_content(self):
		json_path = self.cache_gen_content_path + ".json"
		bin_path = self.cache_gen_content_path + ".bin"
		if os.path.isfile(json_path):
			self._json_file_to_gen(self.last_gen, json_path)
			return True
		elif os.path.isfile(bin_path):
			self._bin_file_to_gen(self.last_gen, bin_path)
			return True
		else:
			dlog.info('export generator content file not found.')
			return False

	def _restore_last_gen_version(self):
		self.last_gen_guid = ""

		if not os.path.isfile(self.cache_gen_version_path):
			dlog.info('Last export generator version file not found.')
			return False

		auto_export_version_file_path = os.path.join(ExportConfig.EngineBindingGeneratePath, 'NePyAutoExportVersion.h')
		if not os.path.isfile(auto_export_version_file_path):
			dlog.info('NePyAutoExportVersion.h not found.')
			return False

		with open(self.cache_gen_version_path, 'r', encoding='utf-8') as f:
			guid_in_gen_content = f.readline().strip()

		with open(auto_export_version_file_path, 'r', encoding='utf-8') as f:
			import re
			content = f.read()
			match = re.search(r'#define\s+NEPY_EXPORT_GUID_VERSION\s+"([^"]+)"', content)
			guid_in_export_h = match.group(1) if match else None

		dlog.info('Last Export Guid in %s is: %s' % (os.path.basename(self.cache_gen_version_path),  guid_in_gen_content))
		dlog.info('Last Export Guid in %s is: %s' % (os.path.basename(auto_export_version_file_path), guid_in_export_h))

		if guid_in_gen_content == guid_in_export_h:
			dlog.info('Export guid match.')
			self.last_gen_guid = guid_in_gen_content
			return True
		else:
			dlog.info('Export guid mismatch. Last export generator json information can not be used.')
			return False

	def classes_dont_gen_this_time(self):
		if not self.enable_increment_gen:
			return []
		classes = []
		for class_name in self.dec_gen.classes_by_name.keys():
			if not self.current_gen.classes_by_name.get(class_name, None):
				classes.append(class_name)
		return classes

	def structs_dont_gen_this_time(self):
		if not self.enable_increment_gen:
			return []
		structs = []
		for struct_name in self.dec_gen.structs_by_name.keys():
			if not self.current_gen.structs_by_name.get(struct_name, None):
				structs.append(struct_name)
		return structs

	def enums_dont_gen_this_time(self):
		if not self.enable_increment_gen:
			return []
		enums = []
		for enum_name in self.dec_gen.enums_by_name.keys():
			if not self.current_gen.structs_by_name.get(enum_name, None):
				enums.append(enums)
		return enums

	def package_dont_need_to_gen_this_time(self, package_name):
		if not self.enable_increment_gen:
			return False

		if (package_name in self.current_gen.classes_by_package.keys() and 
			(package_name in self.inc_gen.classes_by_package.keys() or
			package_name in self.dec_gen.classes_by_package.keys())):
			return False

		if (package_name in self.current_gen.structs_by_package.keys() and 
			(package_name in self.inc_gen.structs_by_package.keys() or
			package_name in self.dec_gen.structs_by_package.keys())):
			return False

		if (package_name in self.current_gen.enums_by_package.keys() and 
			(package_name in self.inc_gen.enums_by_package.keys() or
			package_name in self.dec_gen.enums_by_package.keys())):
			return False

		return True

	def class_dont_need_to_gen_this_time(self, class_name):
		if not self.enable_increment_gen:
			return False
		if class_name not in self.current_gen.classes_by_name.keys():
			return True
		return not self.dec_gen.classes_by_name.get(class_name, None) and not self.inc_gen.classes_by_name.get(class_name, None)

	def struct_dont_need_to_gen_this_time(self, struct_name):
		if not self.enable_increment_gen:
			return False
		if struct_name not in self.current_gen.structs_by_name.keys():
			return True
		return not self.dec_gen.structs_by_name.get(struct_name, None) and not self.inc_gen.structs_by_name.get(struct_name, None)

	def enum_dont_need_to_gen_this_time(self, enum_name):
		if enum_name not in self.current_gen.enums_by_name.keys():
			return True
		return not self.dec_gen.enums_by_name.get(enum_name, None) and not self.inc_gen.enums_by_name.get(enum_name, None)

	def has_gen_difference(self):
		if not self.enable_increment_gen:
			return True
		return self.inc_gen.has_content() or self.dec_gen.has_content()

	def prepare_for_increment_gen(self, temp_dir):
		self.cache_gen_content_path = os.path.join(temp_dir, 'gen_content')
		self.cache_gen_version_path = os.path.join(temp_dir, 'gen_version.txt')

		self._restore_last_gen_version() and self._restore_last_gen_content()

		self.current_gen = self._get_sorted_gen(self.current_gen)

		# generator作差 判断哪些类需要重新导出
		self.inc_gen = self._make_gen_diff(self.current_gen, self.last_gen)
		self.dec_gen = self._make_gen_diff(self.last_gen, self.current_gen)

		if self.cache_gen_enable_debug:
			self._save_gen_to_file(self.inc_gen, os.path.join(temp_dir, 'inc_gen_content'))
			self._save_gen_to_file(self.dec_gen, os.path.join(temp_dir, 'dec_gen_content'))

		if self.has_gen_difference():
			dlog.info('Has gen difference.')
			self._save_gen_to_file(self.current_gen, self.cache_gen_content_path)
		else:
			dlog.info('No gen difference.')

		return True

	def finish_for_increment_gen(self):
		try:
			with open(self.cache_gen_version_path, 'w', encoding='utf-8') as f:
				f.write(self.current_gen_guid + '\n')
			return True
		except Exception as e:
			dlog.info(f"Failed to save generator version to {self.cache_gen_version_path}: {e}")
		return False