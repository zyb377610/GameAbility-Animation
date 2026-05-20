# -*- encoding: utf-8 -*-
import os
import re
import json
import SourceHelpers
import DebugLog as dlog
from version_helper import open_and_read_str

try:
	import typing
except:
	pass

# 解析引擎源码文件，找到所有UEnum和UStruct和UClass
class SourceFinder(object):
	def __init__(self, source_dir):
		# type: (str) -> None
		# 需要解析的源码目录
		self.source_dir = source_dir
		# 对应的UE4包名
		self.package_name = source_dir.replace('\\', '/').rpartition('/')[-1]
		# 当前正在解析的头文件路径
		self.curr_header_path = None

		# 解析结果
		# key:符号名 val:相对文件路径
		self.enum_to_file_dict = {} # type: dict[str, dict[str, str]]
		self.struct_to_file_dict = {} # type: dict[str, dict[str, str]]
		self.class_to_file_dict = {} # type: dict[str, dict[str, str]]

	def run(self):
		for header_path in self.iter_cpp_headers():
			dlog.debug('parsing:', header_path)
			dlog.indent()
			self.parse_one_header(header_path)
			dlog.unindent()

		return {
			'enums': self.enum_to_file_dict,
			'structs': self.struct_to_file_dict,
			'classes': self.class_to_file_dict,
		}

	UENUM_PAT = re.compile(r'^\s*UENUM\(')
	USTRUCT_PAT = re.compile(r'^\s*USTRUCT\(')
	UCLASS_PAT = re.compile(r'^\s*UCLASS\(')
	UINTERFACE_PAT = re.compile(r'^\s*UINTERFACE\(')

	def parse_one_header(self, header_path):
		# type: (str) -> None
		# need to be compatible with py2 and py3
		file_data = open_and_read_str(header_path)
		if 'UENUM(' not in file_data \
			and 'USTRUCT(' not in file_data \
			and 'UCLASS(' not in file_data \
			and 'UINTERFACE(' not in file_data:
			return

		self.curr_header_path = header_path

		# 解析出UE4中include用的相对路径
		include_header_path = SourceHelpers.get_relative_header_path(header_path, self.source_dir)

		lines = file_data.splitlines()

		line_count = len(lines)
		line_no = 0
		while line_no < line_count:
			line = lines[line_no].strip()

			if self.UENUM_PAT.match(line):
				line_no, enum_name = self.parse_enum(lines, line_no)
				self.enum_to_file_dict[enum_name] = {
					'header': include_header_path
				}
			elif self.USTRUCT_PAT.match(line):
				line_no, cpp_struct_name = self.parse_struct(lines, line_no)
				struct_name = cpp_struct_name[1:] # strip F
				self.struct_to_file_dict[struct_name] = {
					'cpp_name': cpp_struct_name,
					'header': include_header_path
				}
			elif self.UCLASS_PAT.match(line):
				line_no, cpp_class_name, api_name = self.parse_class(lines, line_no)
				class_name = cpp_class_name[1:] # strip U or A
				self.class_to_file_dict[class_name] = {
					'cpp_name': cpp_class_name,
					'header': include_header_path
				}
			elif self.UINTERFACE_PAT.match(line):
				line_no, cpp_class_name, api_name = self.parse_interface(lines, line_no)
				class_name = cpp_class_name[1:] # strip U or A
				self.class_to_file_dict[class_name] = {
					'cpp_name': cpp_class_name,
					'header': include_header_path
				}
			else:
				line_no += 1

	# 跳过UENUM/USTRUCT/UCLASS/UINTERFACE宏的小括号部分
	# NOTE: Engine\Classes\Materials\MaterialExpressionGetLocal.h这个沙雕文件居然把小括号打在下一行
	#       我们简单地统计一下左括号和右括号的数量是否匹配就好了
	def skip_parenthesis_lines(self, lines, line_no):
		# type: (list[str], int) -> int
		num_left = 0
		num_right = 0
		while line_no < len(lines):
			line = lines[line_no]
			# 简单排除一下注释（例子：FSlateBrush）
			comment_index = line.find('//')
			if comment_index >= 0:
				line = line[:comment_index]
			num_left += line.count('(')
			num_right += line.count(')')
			if num_left == num_right:
				break
			line_no += 1
		return line_no + 1 if num_left > 0 else line_no

	# 跳过注释行
	def skip_comment_lines(self, lines, line_no):
		# type: (list[str], int) -> int
		in_comment_block = False
		while line_no < len(lines):
			line = lines[line_no].strip()
			if line.startswith('//'):
				line_no += 1
				continue
			if line.startswith('/*'):
				if not line.endswith('*/'):
					in_comment_block = True
				line_no += 1
				continue
			if in_comment_block:
				if line.endswith('*/'):
					in_comment_block = False
				line_no += 1
				continue
			break
		return line_no

	ENUM_REGULAR_PAT = re.compile(r'^\s*enum\s+([_A-Za-z][_A-Za-z0-9]*)')
	ENUM_NAMESPACED_PAT = re.compile(r'^\s*namespace\s+([_A-Za-z][_A-Za-z0-9]*)')
	ENUM_CLASS_PAT = re.compile(r'^\s*enum\s+class\s+([_A-Za-z][_A-Za-z0-9]*)')

	# 参见 UHT HeaderParser.cpp FHeaderParser::CompileEnum()
	def parse_enum(self, lines, line_no):
		# type: (list[str], int) -> tuple[int, str]
		enum_name = ''
		line_no = self.skip_parenthesis_lines(lines, line_no)
		line_no = self.skip_comment_lines(lines, line_no)
		while line_no < len(lines):
			line = lines[line_no]

			result = self.ENUM_NAMESPACED_PAT.match(line)
			if result:
				enum_name = result.group(1)
				dlog.debug('UEnum', enum_name, '[ENUM_NAMESPACED]')
				break

			result = self.ENUM_CLASS_PAT.match(line)
			if result:
				enum_name = result.group(1)
				dlog.debug('UEnum', enum_name, '[ENUM_CLASS]')
				break

			result = self.ENUM_REGULAR_PAT.match(line)
			if result:
				enum_name = result.group(1)
				dlog.debug('UEnum', enum_name, '[ENUM_REGULAR]')
				break

			assert False, 'parse_enum() failed! file:%s line_no:%s line:%s' % (self.curr_header_path, line_no, line)
			break

		return line_no, enum_name

	STRUCT_WITH_API_PAT = re.compile(r'^\s*struct\s+[_A-Za-z][_A-Za-z0-9]*\s+([_A-Za-z][_A-Za-z0-9]*)')
	STRUCT_WITHOUT_API_PAT = re.compile(r'^\s*struct\s+([_A-Za-z][_A-Za-z0-9]*)')

	# 参见 UHT HeaderParser.cpp FHeaderParser::CompileStructDeclaration()
	def parse_struct(self, lines, line_no):
		# type: (list[str], int) -> tuple[int, str]
		struct_name = ''
		line_no = self.skip_parenthesis_lines(lines, line_no)
		line_no = self.skip_comment_lines(lines, line_no)
		while line_no < len(lines):
			line = lines[line_no]

			result = self.STRUCT_WITH_API_PAT.match(line)
			if result:
				struct_name = result.group(1)
				dlog.debug('UStruct', struct_name, '[STRUCT_WITH_API]')
				break

			result = self.STRUCT_WITHOUT_API_PAT.match(line)
			if result:
				struct_name = result.group(1)
				dlog.debug('UStruct', struct_name, '[STRUCT_WITHOUT_API]')
				break

			assert False, 'parse_struct() failed! file:%s line_no:%s line:%s' % (self.curr_header_path, line_no, line)
			break

		return line_no, struct_name

	CLASS_WITH_API_PAT = re.compile(r'^\s*class\s+([_A-Za-z][_A-Za-z0-9]*)\s+([_A-Za-z][_A-Za-z0-9]*)')
	CLASS_WITHOUT_API_PAT = re.compile(r'^\s*class\s+([_A-Za-z][_A-Za-z0-9]*)')

	# 参见 UHT HeaderParser.cpp FHeaderParser::CompileClassDeclaration()
	def parse_class(self, lines, line_no):
		# type: (list[str], int) -> tuple[int, str, str]
		class_name = ''
		line_no = self.skip_parenthesis_lines(lines, line_no)
		line_no = self.skip_comment_lines(lines, line_no)
		while line_no < len(lines):
			line = lines[line_no]

			result = self.CLASS_WITH_API_PAT.match(line)
			if result:
				class_name = result.group(2)
				api_name = result.group(1)
				dlog.debug('UClass', class_name, '[CLASS_WITH_API]', api_name)
				break

			result = self.CLASS_WITHOUT_API_PAT.match(line)
			if result:
				class_name = result.group(1)
				api_name = ''
				dlog.debug('UClass', class_name, '[CLASS_WITHOUT_API]')
				break

			assert False, 'parse_class() failed! file:%s line_no:%s line:%s' % (self.curr_header_path, line_no, line)
			break

		return line_no, class_name, api_name

	# 参见 UHT HeaderParser.cpp FHeaderParser::CompileInterfaceDeclaration()
	def parse_interface(self, lines, line_no):
		# type: (list[str], int) -> tuple[int, str, str]
		class_name = ''
		line_no = self.skip_parenthesis_lines(lines, line_no)
		line_no = self.skip_comment_lines(lines, line_no)
		while line_no < len(lines):
			line = lines[line_no]

			result = self.CLASS_WITH_API_PAT.match(line)
			if result:
				class_name = result.group(2)
				api_name = result.group(1)
				dlog.debug('UInterface', class_name, '[INTERFACE_WITH_API]', api_name)
				break

			result = self.CLASS_WITHOUT_API_PAT.match(line)
			if result:
				class_name = result.group(1)
				api_name = ''
				dlog.debug('UInterface', class_name, '[INTERFACE_WITHOUT_API]')
				break

			assert False, 'parse_interface() failed! file:%s line_no:%s line:%s' % (self.curr_header_path, line_no, line)
			break

		return line_no, class_name, api_name

	def iter_cpp_headers(self):
		for root, dirs, files in os.walk(self.source_dir):
			for fn in files:
				if not fn.endswith('.h'):
					continue
				yield os.path.join(root, fn)


def main():
	package_name_to_source_dir_dict = SourceHelpers.get_package_name_to_source_dir_dict()

	source_infos = {}
	# need to be compatible with py2 and py3
	for package_name, source_dir in package_name_to_source_dir_dict.items():
		source_infos[package_name] = SourceFinder(source_dir).run()

	temp_dir = os.path.abspath('temp')
	if not os.path.isdir('temp'):
		os.path.mkdirs(temp_dir)

	source_infos_file_path = os.path.join(temp_dir, 'source_infos.json')
	with open(source_infos_file_path, 'w') as f:
		json.dump(source_infos, f, indent=4, sort_keys=True)


if __name__ == '__main__':
	dlog.set_log_level(dlog.LOG_LEVEL_DEBUG)
	import time
	begin = time.time()
	main()
	dlog.info('cost time: %.2f' % (time.time() - begin))
