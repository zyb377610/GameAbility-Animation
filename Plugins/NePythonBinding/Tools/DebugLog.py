# -*- encoding: utf-8 -*-

# 不输出任何日志
LOG_LEVEL_MUTE = 0
# 仅输出普通信息
LOG_LEVEL_INFO = 1
# 还输出调试信息
LOG_LEVEL_DEBUG = 2

# 设置日志等级
def set_log_level(log_level):
	# type: (int) -> None
	global g_log_level
	g_log_level = log_level

# 增加缩进
def indent():
	global g_indent
	g_indent += 1

# 减少缩进
def unindent():
	global g_indent
	g_indent -= 1

def set_log_prefix(prefix):
	global g_log_prefix
	g_log_prefix = prefix

# 输出普通日志
def info(*args):
	if g_log_level >= LOG_LEVEL_INFO:
		_log(*args)

# 输出调试日志
def debug(*args):
	if g_log_level >= LOG_LEVEL_DEBUG:
		_log(*args)

def _log(*args):
	print('    ' * g_indent + g_log_prefix + ' '.join(str(arg) for arg in args))

# 缩进值
g_indent = 0
# 日志等级
g_log_level = LOG_LEVEL_INFO
# Log前缀
g_log_prefix = ""