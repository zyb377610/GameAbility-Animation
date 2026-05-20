# -*- encoding: utf-8 -*-
import sys

FOR_TYPE_HINT = False
if FOR_TYPE_HINT:
	import typing

# 是否启用自动reload功能
ENABLE_AUTO_RELOAD = True

def check_enable():
	if not ENABLE_AUTO_RELOAD:
		return False
	if sys.platform != 'win32':
		return False
	return True

if check_enable():
	import os
	import time
	import ue

	# https://docs.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-file_notify_information
	FILE_ACTION_ADDED = 1
	FILE_ACTION_REMOVED = 2
	FILE_ACTION_MODIFIED = 3
	FILE_ACTION_RENAMED_OLD_NAME = 4
	FILE_ACTION_RENAMED_NEW_NAME = 5

	g_reload_monitor = None

	class ReloadMonitor(object):
		def __init__(self):
			self._last_reload_time = 0 # type: float
			self._modified_files = set() # type: typing.Set[str]

		def start(self):
			script_root = self._get_script_root()
			if os.path.isdir(script_root):
				print(('[ReloadMonitor] monitor changes: %s' % script_root))
				ue.FmMonitorDirectoryChanges(script_root, self._on_script_changed)
			else:
				print(('[ReloadMonitor] diretory not exists: %s' % script_root))

			ue.AddTicker(self._tick)

		def _get_script_root(self):
			return os.path.dirname(os.path.abspath(__file__))

		def _on_script_changed(self, file_actions):
			# type: (typing.List[typing.Tuple[str, int]]) -> None
			for file, action in file_actions:
				ext = os.path.splitext(file)[-1]
				if ext != '.py':
					continue
				if action in (FILE_ACTION_ADDED, FILE_ACTION_MODIFIED):
					self._modified_files.add(file)

		def _tick(self, dt):
			now = time.time()
			if now - self._last_reload_time < 0.1:
				return
			self._last_reload_time = now

			if self._modified_files:
				self._do_reload()

		def _do_reload(self):
			module_names = self._get_modified_modules(self._modified_files)
			self._modified_files.clear()

			print('==> auto reload modules: %s' % module_names)
			import reloader
			reloader.reload(module_names)

		def _get_modified_modules(self, files):
			# type: (typing.Set[str]) -> typing.List[str]
			module_names = []
			for file_name in files:
				module_name = file_name.replace('\\', '/').replace('/', '.')[:-3]
				module_names.append(module_name)
			return module_names

	def start():
		global g_reload_monitor
		g_reload_monitor = ReloadMonitor()
		g_reload_monitor.start()
		print('ReloadMonitor started.')

else: # if not check_enable():
	def start():
		print('ReloadMonitor not running.')