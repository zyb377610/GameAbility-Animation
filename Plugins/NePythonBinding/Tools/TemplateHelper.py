# -*- encoding: utf-8 -*-
import os
import string
from version_helper import open_and_read_str, open_and_write_str

try:
	import typing
except:
	pass

def build_template(template_path, output_file_path, substitute_dict):
	# type: (str, str, dict) -> None
	# need to be compatible with py2 and py3
	result = open_and_read_str(template_path)
	result = substitute(result, substitute_dict)
	result = strip_code(result)
	result = result.replace('\r\n', '\n').replace('\r', '\n')

	if os.path.isfile(output_file_path):
		old_result = open_and_read_str(output_file_path).replace('\r\n', '\n').replace('\r', '\n') # 兼容本地文件中换行符为其他形式（crlf, cr）的情况。
		if old_result == result:
			return
	# need to be compatible with py2 and py3
	open_and_write_str(output_file_path, result)

def substitute(code_data, substitute_dict):
	# type: (str, dict) -> str
	template = string.Template(code_data)
	return template.safe_substitute(substitute_dict)

def strip_code(code_data):
	# type: (str) -> str
	lines = code_data.splitlines() # 兼容支持'\n'和'\r\n'和'\r'三种换行符。
	line_count = len(lines)
	line_no = 0
	if_stack = []

	# 去除用 #if False ... #endif 宏包裹的代码
	# 去除 #if True 和 #endif 宏
	def process_if_region(line, line_no, line_count):
		# type: (str, int, int) -> tuple[bool, int]
		old_line_count = line_count
		if len(if_stack) > 0:
			in_true_region = if_stack[-1] is True
			in_false_region = if_stack[-1] is False
			if in_false_region:
				lines.pop(line_no)
				line_count -= 1
			if line.startswith('#endif'):
				if in_true_region or lines[line_no].strip() == '':
					# 如果#endif后面跟的是空行，则也把它移除了
					lines.pop(line_no)
					line_count -= 1
				if_stack.pop()
		return old_line_count != line_count, line_count

	# 检测是否进入 #if True 或 #if False 区域
	def enter_if_region(line, line_no, line_count):
		# type: (str, int, int) -> tuple[bool, int]
		if line.startswith('#if '):
			if line.startswith('#if True'):
				if_stack.append(True)
				lines.pop(line_no)
				line_count -= 1
				return True, line_count
			if line.startswith('#if False'):
				if_stack.append(False)
				lines.pop(line_no)
				line_count -= 1
				return True, line_count
			if len(if_stack) > 0:
				new_flag = None
				for flag in reversed(if_stack):
					if flag is True:
						break
					if flag is False:
						new_flag = False
						break
				if_stack.append(new_flag)
				return False, line_count
		return False, line_count

	# 检测关键字并删除指定的行
	# __REMOVE_THIS_LINE__ 删除本行
	# __REMOVE_THIS_AND_PREV_LINE__ 删除本行和上一行
	# __REMOVE_THIS_AND_NEXT_LINE__ 删除本行和下一行
	def process_remove_line(line, line_no, line_count):
		# type: (str, int, int) -> tuple[bool, int]
		if '__REMOVE_THIS_' not in line:
			return False, line_count
		if '__REMOVE_THIS_AND_PREV_LINE__' in line:
			lines.pop(line_no)
			if line_no > 0:
				lines.pop(line_no - 1)
			line_count -= 2
		elif '__REMOVE_THIS_AND_NEXT_LINE__' in line:
			lines.pop(line_no)
			if line_no < line_count - 1:
				lines.pop(line_no)
			line_count -= 2
		else: # '__REMOVE_THIS_LINE__' in line:
			lines.pop(line_no)
			line_count -= 1
		return True, line_count

	while line_no < line_count:
		line = lines[line_no]
		result, line_count = enter_if_region(line, line_no, line_count)
		if result:
			continue
		result, line_count = process_if_region(line, line_no, line_count)
		if result:
			continue
		result, line_count = process_remove_line(line, line_no, line_count)
		if result:
			continue
		line_no += 1

	result = '\n'.join(lines)
	return result
