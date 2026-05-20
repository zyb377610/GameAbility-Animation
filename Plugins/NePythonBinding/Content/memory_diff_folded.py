# -*- coding: utf-8 -*-
#!/usr/bin/env python3
# memory_diff_folded.py

import sys
import os
import argparse

# chardet作为可选依赖
try:
	import chardet
	HAS_CHARDET = True
except ImportError:
	chardet = None
	HAS_CHARDET = False

class MemoryDiffFoldedConverter:
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

	def format_memory_size(self, bytes_count, signed=False):
		"""格式化内存大小"""
		if signed and bytes_count < 0:
			sign = "-"
			bytes_count = abs(bytes_count)
		elif signed and bytes_count > 0:
			sign = "+"
		else:
			sign = ""
			
		for unit in ['B', 'KB', 'MB', 'GB', 'TB']:
			if bytes_count < 1024.0:
				return "{}{:.1f}{}".format(sign, bytes_count, unit)
			bytes_count /= 1024.0
		return "{}{:.1f}PB".format(sign, bytes_count)

	def parse_folded_data(self, content):
		"""解析folded格式数据"""
		if not content or not content.strip():
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
		
		return data, total_memory, valid_lines

	def calculate_diff(self, data1, data2):
		"""计算两个folded数据的差值"""
		print("Calculating memory diff...")
		
		all_stacks = set(data1.keys()) | set(data2.keys())
		
		diff_data = {}
		total_diff = 0
		positive_count = 0
		negative_count = 0
		
		for stack in all_stacks:
			val1 = data1.get(stack, 0)
			val2 = data2.get(stack, 0)
			diff = val2 - val1
			
			if diff != 0:
				diff_data[stack] = diff
				total_diff += diff
				if diff > 0:
					positive_count += 1
				else:
					negative_count += 1
		
		print("Diff analysis:")
		print("  Increased stacks: {} items".format(positive_count))
		print("  Decreased stacks: {} items".format(negative_count))
		print("  Net memory change: {}".format(self.format_memory_size(total_diff, signed=True)))
		
		return diff_data, total_diff

	def save_diff_folded(self, diff_data, output_file):
		"""保存差值为folded格式文件 (只保存正增长)"""
		try:
			os.makedirs(os.path.dirname(output_file) or '.', exist_ok=True)
			
			positive_count = 0
			with open(output_file, 'w', encoding='utf-8') as f:
				for stack, diff in sorted(diff_data.items()):
					# 只保存正增长的数据
					if diff > 0:
						f.write("{} {}\n".format(stack, diff))
						positive_count += 1
			
			print("Diff folded file saved: {} (only {} positive changes)".format(output_file, positive_count))
			return True
			
		except Exception as e:
			print("ERROR: Failed to save diff folded file: {}".format(e))
			return False

	def convert_diff(self, file1, file2, output_file):
		"""差值转换流程"""
		for f in [file1, file2]:
			if not os.path.exists(f):
				print("ERROR: File not found: {}".format(f))
				return False
		
		print("Memory Diff: {} vs {}".format(file1, file2))
		
		content1 = self.read_file_smart(file1) or ""
		content2 = self.read_file_smart(file2) or ""
		
		data1, total1, valid1 = self.parse_folded_data(content1)
		data2, total2, valid2 = self.parse_folded_data(content2)
		
		print("File1: {} stacks, {}".format(valid1, self.format_memory_size(total1)))
		print("File2: {} stacks, {}".format(valid2, self.format_memory_size(total2)))
		
		diff_data, total_diff = self.calculate_diff(data1, data2)
		
		return self.save_diff_folded(diff_data, output_file)


def main():
	parser = argparse.ArgumentParser(description='Memory Diff Folded generator - 比较两个folded文件输出差值folded文件 (只显示增长)')
	parser.add_argument('file1', help='第一个folded文件 (基准)')
	parser.add_argument('file2', help='第二个folded文件 (对比)')
	parser.add_argument('-o', '--output', help='输出folded文件名')
	
	args = parser.parse_args()
	
	if not args.output:
		name1 = os.path.splitext(os.path.basename(args.file1))[0]
		name2 = os.path.splitext(os.path.basename(args.file2))[0]
		args.output = "diff_{}_vs_{}_positive.folded".format(name1, name2)
	
	print("Memory Diff Folded Generator (Positive Growth Only)")
	print("=" * 50)
	
	converter = MemoryDiffFoldedConverter()
	success = converter.convert_diff(args.file1, args.file2, args.output)
	
	print("=" * 50)
	if success:
		print("Diff folded file: {}".format(os.path.abspath(args.output)))
	else:
		print("Failed!")
		sys.exit(1)

if __name__ == "__main__":
	main()