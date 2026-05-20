# -*- coding: utf-8 -*-
#!/usr/bin/env python3
# memory_flamegraph.py

import sys
import os
import argparse
import subprocess
import shutil

# chardet作为可选依赖
try:
	import chardet
	HAS_CHARDET = True
except ImportError:
	chardet = None
	HAS_CHARDET = False

class MemoryFlameGraphConverter:
	def __init__(self):
		self.flamegraph_pl = None
		self.find_flamegraph_pl()
	
	def find_flamegraph_pl(self):
		"""查找flamegraph.pl"""
		candidates = [
			'flamegraph.pl',
			'./FlameGraph/flamegraph.pl',
			'./flamegraph.pl',
			'/usr/local/bin/flamegraph.pl',
			'/usr/bin/flamegraph.pl',
			os.path.expanduser('~/flamegraph.pl'),
			os.path.expanduser('~/FlameGraph/flamegraph.pl'),
		]
		
		if shutil.which('flamegraph.pl'):
			candidates.insert(0, 'flamegraph.pl')
		
		for path in candidates:
			if os.path.exists(path) and os.access(path, os.X_OK):
				self.flamegraph_pl = path
				print("Found flamegraph.pl: {}".format(path))
				return

		print("ERROR: flamegraph.pl not found")
		print("Please download: wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl")

	def read_file_smart(self, file_path):
		"""智能读取文件"""
		encodings = ['utf-8-sig', 'utf-8', 'gbk', 'gb2312', 'latin1']
		
		if HAS_CHARDET:
			try:
				with open(file_path, 'rb') as f:
					raw = f.read(10240)
				detected = chardet.detect(raw)
				if detected['encoding'] and detected['confidence'] > 0.7:
					encodings.insert(0, detected['encoding'])
			except:
				pass

		for encoding in encodings:
			try:
				with open(file_path, 'r', encoding=encoding) as f:
					return f.read()
			except:
				continue

		print("ERROR: Cannot read file: {}".format(file_path))
		return None

	def format_memory_size(self, bytes_count):
		"""格式化内存大小"""
		for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
			if bytes_count < 1024.0:
				return "{:.1f}{}".format(bytes_count, unit)
			bytes_count /= 1024.0
		return "{:.1f}PB".format(bytes_count)
	
	def ensure_output_dir(self, output_dir):
		"""确保输出目录存在"""
		if not output_dir:
			return True
			
		try:
			os.makedirs(output_dir, exist_ok=True)
			print("Output directory: {}".format(os.path.abspath(output_dir)))
			return True
		except Exception as e:
			print("ERROR: Cannot create output directory {}: {}".format(output_dir, e))
			return False
	
	def generate_output_filename(self, input_file=None, output_dir=None):
		"""智能生成输出文件名"""
		if input_file:
			base_name = os.path.splitext(os.path.basename(input_file))[0]
			filename = "flamegraph_{}.svg".format(base_name)
		else:
			# stdin模式
			filename = "flamegraph_stdin.svg"

		if output_dir:
			return os.path.join(output_dir, filename)
		else:
			return filename
	
	def resolve_output_path(self, explicit_output, input_file=None, output_dir=None):
		"""解析最终输出路径"""
		if explicit_output and explicit_output != 'memory_flamegraph.svg':
			# 用户明确指定了输出文件
			if output_dir and not os.path.isabs(explicit_output):
				output_path = os.path.join(output_dir, explicit_output)
			else:
				output_path = explicit_output
		elif output_dir:
			# 指定了输出目录，自动生成文件名
			output_path = self.generate_output_filename(input_file, output_dir)
		else:
			# 使用默认文件名或用户指定的文件名
			output_path = explicit_output or 'memory_flamegraph.svg'
		
		return os.path.abspath(output_path)
	
	def parse_folded_data(self, content):
		"""解析folded格式数据"""
		if not content or not content.strip():
			print("WARNING: Input content is empty")
			return {}, 0, 0
		
		if content.startswith('\ufeff'):
			content = content[1:]
		
		data = {}
		total_memory = 0
		valid_lines = 0
		
		for line in content.split('\n'):
			line = line.strip()
			if not line or line.startswith('#'):
				continue
			
			parts = line.rsplit(' ', 1)
			if len(parts) == 2:
				stack_str, count_str = parts
				try:
					memory_bytes = int(count_str.strip())
					stack_key = stack_str.strip()
					if stack_key:
						data[stack_key] = memory_bytes
						total_memory += memory_bytes
						valid_lines += 1
				except ValueError:
					pass
		
		if valid_lines == 0:
			print("WARNING: No valid stack traces found in input")
		
		return data, total_memory, valid_lines

	def analyze_folded_memory(self, content):
		"""分析folded内存数据"""
		if not content or not content.strip():
			print("WARNING: Empty input content")
			return False, 0
		
		data, total_memory, valid_lines = self.parse_folded_data(content)
		
		if valid_lines == 0:
			print("Memory analysis:")
			print("  Valid stacks: 0 items")
			print("  Total memory: 0 B")
			print("  Status: Empty or invalid folded format")
			return False, 0
		
		max_memory = max(data.values()) if data else 0
		
		print("Memory analysis:")
		print("  Valid stacks: {} items".format(valid_lines))
		print("  Total memory: {}".format(self.format_memory_size(total_memory)))
		print("  Max allocation: {}".format(self.format_memory_size(max_memory)))
		
		return True, total_memory
	
	def generate_empty_flamegraph(self, output_svg, title="Empty Memory Profile"):
		"""生成空的火焰图"""
		try:
			output_dir = os.path.dirname(output_svg)
			if output_dir and not self.ensure_output_dir(output_dir):
				return False
			
			print("Generating empty flamegraph: {}".format(output_svg))
			
			empty_content = "empty_profile 0\n"
			
			cmd = ['perl', self.flamegraph_pl]
			cmd.extend(['--title', title])
			cmd.extend(['--width', '1200'])
			cmd.extend(['--colors', 'mem'])
			cmd.extend(['--countname', 'bytes'])
			cmd.extend(['--fonttype', 'Verdana'])
			cmd.extend(['--fontsize', '12'])
			
			print("Executing: {} > {}".format(' '.join(cmd), output_svg))
			
			with open(output_svg, 'w', encoding='utf-8') as f:
				result = subprocess.run(
					cmd, 
					input=empty_content, 
					stdout=f, 
					stderr=subprocess.PIPE, 
					text=True, 
					timeout=120
				)
			
			if result.returncode == 0:
				size = os.path.getsize(output_svg)
				print("Empty flamegraph generated successfully ({} bytes)".format(size))
				return True
			else:
				print("ERROR: flamegraph.pl failed: {}".format(result.stderr))
				return False
				
		except Exception as e:
			print("ERROR: Failed to generate empty flamegraph: {}".format(e))
			return False
	
	def generate_memory_flamegraph(self, content, output_svg, title=None, width=1200, 
								 color_scheme='mem', reverse=False):
		"""生成内存火焰图"""
		if not self.flamegraph_pl:
			return False
		
		output_dir = os.path.dirname(output_svg)
		if output_dir and not self.ensure_output_dir(output_dir):
			return False
		
		try:
			print("Generating memory flamegraph: {}".format(output_svg))
			
			cmd = ['perl', self.flamegraph_pl]
			
			if title:
				cmd.extend(['--title', title])
			else:
				cmd.extend(['--title', 'Memory Flame Graph'])
			
			cmd.extend(['--width', str(width)])
			
			# 内存火焰图颜色方案
			if color_scheme == 'mem':
				cmd.extend(['--colors', 'mem'])
			elif color_scheme == 'blue':
				cmd.extend(['--colors', 'blue'])
			elif color_scheme == 'green':
				cmd.extend(['--colors', 'green'])
			elif color_scheme == 'red':
				cmd.extend(['--colors', 'red'])
			
			cmd.extend(['--countname', 'bytes'])
			
			if reverse:
				cmd.extend(['--reverse'])
			
			cmd.extend(['--fonttype', 'Verdana'])
			cmd.extend(['--fontsize', '12'])
			
			print("Executing: {} > {}".format(' '.join(cmd), output_svg))
			
			with open(output_svg, 'w', encoding='utf-8') as f:
				result = subprocess.run(
					cmd, 
					input=content, 
					stdout=f, 
					stderr=subprocess.PIPE, 
					text=True, 
					timeout=120
				)
			
			if result.returncode == 0:
				size = os.path.getsize(output_svg)
				print("Memory flamegraph generated successfully ({} bytes)".format(size))
				return True
			else:
				print("ERROR: flamegraph.pl failed: {}".format(result.stderr))
				return False
				
		except Exception as e:
			print("ERROR: Failed to generate flamegraph: {}".format(e))
			return False
	
	def convert(self, input_file, output_svg, title=None, width=1200, 
				color_scheme='mem', reverse=False):
		"""完整转换流程"""
		if not self.flamegraph_pl:
			return False
		
		# 读取输入
		if input_file:
			if not os.path.exists(input_file):
				print("ERROR: File not found: {}".format(input_file))
				return False
			
			if os.path.getsize(input_file) == 0:
				print("WARNING: Input file is empty: {}".format(input_file))
				return self.generate_empty_flamegraph(output_svg, title or "Empty Memory Profile")
			
			content = self.read_file_smart(input_file)
			if not title:
				title = "Memory Profile - {}".format(os.path.basename(input_file))
		else:
			print("Reading from stdin...")
			content = sys.stdin.read()
			if not title:
				title = "Memory Profile"
		
		if not content or not content.strip():
			print("WARNING: No data available")
			return self.generate_empty_flamegraph(output_svg, title or "Empty Memory Profile")
		
		# 分析内存数据
		is_valid, total_memory = self.analyze_folded_memory(content)
		if not is_valid:
			print("WARNING: No valid memory data found, generating empty flamegraph")
			return self.generate_empty_flamegraph(output_svg, title or "Empty Memory Profile")
		
		# 在标题中加入总内存信息
		if title and total_memory > 0:
			title = "{} (Total: {})".format(title, self.format_memory_size(total_memory))
		
		# 生成内存火焰图
		return self.generate_memory_flamegraph(content, output_svg, title, width, 
											 color_scheme, reverse)


def main():
	parser = argparse.ArgumentParser(description='Memory FlameGraph generator - 将folded格式转换为SVG火焰图')
	parser.add_argument('input', nargs='?', help='输入内存folded文件 (空=从stdin读取)')
	parser.add_argument('-o', '--output', default='memory_flamegraph.svg', help='输出SVG文件名')
	parser.add_argument('--output-dir', help='输出目录 (自动生成文件名)')
	parser.add_argument('-t', '--title', help='火焰图标题')
	parser.add_argument('-w', '--width', type=int, default=1200, help='火焰图宽度 (默认: 1200)')
	parser.add_argument('-c', '--color', choices=['mem', 'blue', 'green', 'red'], 
					   default='mem', help='颜色方案 (默认: mem)')
	parser.add_argument('-r', '--reverse', action='store_true', help='反转排序 (大内存在前)')
	
	args = parser.parse_args()
	
	print("Memory FlameGraph Converter")
	print("=" * 40)
	
	converter = MemoryFlameGraphConverter()
	
	# 解析输出路径
	output_path = converter.resolve_output_path(
		args.output, input_file=args.input, output_dir=args.output_dir)
	
	success = converter.convert(args.input, output_path, args.title, args.width, 
			   args.color, args.reverse)
	
	print("=" * 40)
	if success:
		print("Flamegraph: {}".format(output_path))
		if os.path.exists(output_path):
			file_size = os.path.getsize(output_path)
			print("Size: {} bytes".format(file_size))
	else:
		print("Failed!")
		sys.exit(1)

if __name__ == "__main__":
	main()

'''
python memory_flamegraph.py app_profile.folded
python memory_flamegraph.py app_profile.folded -o result.svg --output-dir ./reports
'''