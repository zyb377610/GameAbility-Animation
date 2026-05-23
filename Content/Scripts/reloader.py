# -*- encoding: utf-8 -*-
import sys
import os
import gc
import inspect
import types
import importlib.util
import ue

FOR_TYPE_HINT = False
if FOR_TYPE_HINT:
	import typing


class ReloadRules(object):
	CLASS_NOT_RELOAD_ATTR_NAMES = {'__module__', '__dict__', '__weakref__'}

	# 判断module是否能被reload
	def check_module_can_reload(self, module_name, module):
		# type: (str, typing.Any) -> bool
		if not inspect.ismodule(module):
			return False
		if module_name in sys.builtin_module_names:
			return False
		if not getattr(module, '__file__', None):
			return False
		if not hasattr(module, '__dict__'):
			return False
		module_file = module.__file__.replace('\\', '/')
		if '/Lib/' in module_file or '/debuglib/' in module_file:
			return False
		return True

RULES = ReloadRules()


# 上一次reload时间，用于增量reload
g_last_reload_time = 0


# 初始化所有模块的上一次reload时间，用于增量reload
def init_last_reload_time():
	import time
	global g_last_reload_time
	g_last_reload_time = time.time()

# 对python模块进行reload
# module_names:
#   指定了需要reload的模块名称列表。
#   若未提供，则表示要reload所有模块。
# modified_only:
#   如果为True，则根据模块文件修改时间进行增量reload。
#   否则，就会全量reload所有模块。
def reload(module_names=None, modified_only=True):
	# type: (typing.List[str] | None, bool) -> None
	if module_names is None:
		module_names = list(sys.modules.keys())

	if modified_only:
		global g_last_reload_time
		last_reload_time = g_last_reload_time

	print('********** start reload script ***************')

	old_sys_meta_path = sys.meta_path
	sys.meta_path = [ReloadFinder()]

	gc_is_enabled = gc.isenabled()
	if gc_is_enabled:
		gc.disable()

	old_modules = {}
	old_module_dicts = {}
	for module_name, module in sys.modules.items():
		old_modules[module_name] = module
		if RULES.check_module_can_reload(module_name, module):
			old_module_dicts[module_name] = dict(module.__dict__)
	sys._reloader_old_modules = old_modules
	sys._reloader_old_module_dicts = old_module_dicts

	reload_module_names = []
	for module_name in module_names:
		module = sys.modules.get(module_name)
		if not RULES.check_module_can_reload(module_name, module):
			continue
		if modified_only:
			try:
				modify_time = os.stat(module.__file__).st_mtime
				py_file = os.path.splitext(module.__file__)[0] + '.py'
				if os.path.exists(py_file):
					modify_time = max(modify_time, os.stat(py_file).st_mtime)
				if modify_time < last_reload_time:
					continue
				if modify_time > g_last_reload_time:
					g_last_reload_time = modify_time
			except Exception as e:
				sys.stderr.write('failed to check modify time of "%s", because "%s"\n' % (module_name, e))
				continue
		sys.modules.pop(module_name, None)
		reload_module_names.append(module_name)

	try:
		for module_name in reload_module_names:
			print('reloading "%s" ...' % module_name)
			__import__(module_name)
	except:
		if modified_only:
			g_last_reload_time = last_reload_time
		sys.excepthook(*sys.exc_info())

	del sys._reloader_old_modules
	del sys._reloader_old_module_dicts

	#ue.FlushGeneratedTypeReinstancing()

	if gc_is_enabled:
		gc.enable()
		gc.collect()

	sys.meta_path = old_sys_meta_path

	print('********** reload script successed ***************')


class ReloadLoader(object):
	def __init__(self, module):
		# type: (types.ModuleType) -> None
		self._module = module

	# Python 3.12+ 新接口
	def create_module(self, spec):
		return self._module

	def exec_module(self, module):
		pass  # 模块已经加载好了，不需要重新执行


class ReloadFinder(object):
	# Python 3.12+ 新接口，替代已废弃的 find_module
	def find_spec(self, module_name, path, target=None):
		try:
			module = None
			for short_name in module_name.split('.'):
				module = self._get_reloaded_module(short_name, module)
			if module:
				loader = ReloadLoader(module)
				spec = importlib.util.spec_from_loader(module_name, loader)
				return spec
			return None
		except:
			old_module = sys._reloader_old_modules.get(module_name)
			if old_module:
				sys.modules[module_name] = old_module
			sys.excepthook(*sys.exc_info())
			raise

	def _get_reloaded_module(self, short_name, parent_module):
		# type: (str, types.ModuleType) -> types.ModuleType
		parent_name = getattr(parent_module, '__name__', None)
		parent_path = getattr(parent_module, '__path__', None)

		if parent_name:
			module_name = parent_name + '.' + short_name
		else:
			module_name = short_name

		not_reload_module = sys.modules.get(module_name)
		if not_reload_module:
			return not_reload_module

		return self._reload_or_create_module(short_name, module_name, parent_path)

	def _reload_or_create_module(self, short_name, module_name, parent_path):
		# type: (str, str, typing.List[str] | None) -> types.ModuleType
		old_module = sys._reloader_old_modules.get(module_name)
		if old_module:
			sys.modules[module_name] = old_module

		spec = None
		if parent_path:
			for search_path in parent_path:
				candidate = os.path.join(search_path, short_name + '.py')
				if os.path.exists(candidate):
					spec = importlib.util.spec_from_file_location(module_name, candidate)
					break
			if spec is None:
				for search_path in parent_path:
					candidate = os.path.join(search_path, short_name, '__init__.py')
					if os.path.exists(candidate):
						spec = importlib.util.spec_from_file_location(module_name, candidate)
						break
		if spec is None:
			for sp in sys.path:
				candidate = os.path.join(sp, short_name + '.py')
				if os.path.exists(candidate):
					spec = importlib.util.spec_from_file_location(module_name, candidate)
					break
				candidate = os.path.join(sp, short_name, '__init__.py')
				if os.path.exists(candidate):
					spec = importlib.util.spec_from_file_location(module_name, candidate)
					break
		if spec is None:
			raise ModuleNotFoundError("No module named '%s'" % module_name)
		new_module = importlib.util.module_from_spec(spec)
		sys.modules[module_name] = new_module
		spec.loader.exec_module(new_module)

		old_module_dict = sys._reloader_old_module_dicts.get(module_name)
		if not old_module_dict:
			return new_module

		for attr_name, new_attr in inspect.getmembers(new_module):
			old_attr = old_module_dict.get(attr_name)
			if isinstance(new_attr, type):
				if old_attr and isinstance(old_attr, type):
					if not self._update_class(old_attr, new_attr):
						setattr(new_module, attr_name, old_attr)
			elif inspect.isfunction(new_attr):
				if old_attr and inspect.isfunction(old_attr):
					if not self._update_func(old_attr, new_attr):
						setattr(new_module, attr_name, old_attr)

		new_module.__dict__.update(old_module_dict)
		return new_module

	# 使用新方法的func_code去替换旧方法的func_code
	# 返回是否替换成功
	@staticmethod
	def _update_func(old_func, new_func, update_depth=5):
		# type: (types.FunctionType, types.FunctionType, int) -> bool
		if inspect.isbuiltin(old_func) or inspect.isbuiltin(new_func):
			return False

		old_cell_num = 0
		if old_func.__closure__:
			old_cell_num = len(old_func.__closure__)
		new_cell_num = 0
		if new_func.__closure__:
			new_cell_num = len(new_func.__closure__)
		if old_cell_num != new_cell_num:
			return False

		old_func.__code__ = new_func.__code__
		old_func.__dict__ = new_func.__dict__

		defaults = ()
		if new_func.__defaults__:
			defaults = tuple([ReloadFinder._update_object(obj) for obj in new_func.__defaults__])
		old_func.__defaults__ = defaults

		if old_cell_num > 0 and update_depth > 0:
			for index in range(old_cell_num):
				old_cell = old_func.__closure__[index]
				new_cell = new_func.__closure__[index]
				if inspect.isfunction(old_cell.cell_contents) and inspect.isfunction(new_cell.cell_contents):
					ReloadFinder._update_func(old_cell.cell_contents, new_cell.cell_contents, update_depth - 1)

		return True

	# 使用新类中方法的func_code去替换旧类中方法的func_code
	# 返回是否替换成功
	@staticmethod
	def _update_class(old_class, new_class):
		# type: (type, type) -> bool
		if (old_class is type) or (new_class is type):
			return False
		if not (getattr(old_class, '__flags__', 0) & 0x200): # Py_TPFLAGS_HEAPTYPE
			return False
		if not (getattr(new_class, '__flags__', 0) & 0x200): # Py_TPFLAGS_HEAPTYPE
			return False

		# 遇到python生成类，禁用funcode reload，退化成原生reload
		if issubclass(old_class, ue.Object) or issubclass(new_class, ue.Object):
			return False
		if issubclass(old_class, ue.StructBase) or issubclass(new_class, ue.StructBase):
			return False
		if issubclass(old_class, ue.EnumBase) or issubclass(new_class, ue.EnumBase):
			return False

		for attr_name, new_attr in new_class.__dict__.items():
			if attr_name in RULES.CLASS_NOT_RELOAD_ATTR_NAMES:
				continue

			if attr_name not in old_class.__dict__:
				setattr(old_class, attr_name, new_attr)
				continue

			old_attr = old_class.__dict__[attr_name]
			if inspect.isfunction(old_attr) and inspect.isfunction(new_attr):
				if not ReloadFinder._update_func(old_attr, new_attr):
					setattr(old_class, attr_name, new_attr)
			elif isinstance(new_attr, staticmethod) or isinstance(new_attr, classmethod):
				if hasattr(old_attr, '__func__') and hasattr(new_attr, '__func__') \
					and not ReloadFinder._update_func(old_attr.__func__, new_attr.__func__):
					old_attr.__func__ = new_attr.__func__
			elif inspect.isclass(old_attr) and inspect.isclass(new_attr) and new_attr.__name__ is old_attr.__name__:
				ReloadFinder._update_class(old_attr, new_attr)

		return True

	@staticmethod
	def _update_object(default_val):
		new_class = getattr(default_val, '__class__', None) # type: type|None
		if new_class is None:
			return default_val
		if not (getattr(new_class, '__flags__', 0) & 0x200): # Py_TPFLAGS_HEAPTYPE
			return default_val

		old_module_dict = sys._reloader_old_module_dicts.get(new_class.__module__)
		if not old_module_dict:
			return default_val

		old_class = old_module_dict.get(new_class.__name__) # type: type|None
		if old_class:
			default_val.__class__ = old_class
		return default_val
