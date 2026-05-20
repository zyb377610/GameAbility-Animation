# -*- encoding: utf-8 -*-

# 用于生成代码的辅助类
class CodeBuilder(object):
	def __init__(self, indent_chars='\t'):
		self.reset()
		self._indent_chars = indent_chars

	def reset(self):
		# 代码生成缓冲区
		self._lines = []
		# 当前缩进
		self._indent = ''

	def build(self):
		return '\n'.join(self._lines)

	def build_comment_out(self):
		result = '\n// '.join(self._lines)
		result = '// ' + result
		return result

	def add_line(self, line=None):
		# type: (str|None) -> None
		if line:
			self._lines.append(self._indent + line)
		else:
			self._lines.append('')

	def inc_indent(self):
		self._indent += self._indent_chars

	def dec_indent(self):
		self._indent = self._indent[:-len(self._indent_chars)]

	def begin_block(self):
		self.add_line('{')
		self.inc_indent()

	def end_block(self):
		self.dec_indent()
		self.add_line('}')

	def get_indent_chars(self):
		return self._indent_chars

# 用大括号{}框柱的代码段
class CodeBlock(object):
	def __init__(self, cb):
		# cb为CodeBuilder
		self.cb = cb # type: CodeBuilder

	def __enter__(self):
		self.cb.begin_block()

	def __exit__(self, exc_type, exc_val, exc_tb):
		self.cb.end_block()
