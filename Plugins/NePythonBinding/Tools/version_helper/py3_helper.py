# -*- encoding: utf-8 -*-

NEWLINE = '\n'
# 根据本文件的内容决定 lf 或是 crlf
with open(__file__, 'r', encoding='utf-8', newline='') as f:
	contents = f.read()
	if contents.find('\r\n') != -1:
		NEWLINE = '\r\n'


def str_decode(str_data: str, encoding: str) -> str:
	return str_data


def open_and_read_str(file_path: str) -> str:
	with open(file_path, 'rb') as f:
		data = f.read()

	try:
		data = data.decode('utf-8')
	except UnicodeDecodeError:
		data = data.decode('gbk')

	return data


def open_and_write_str(file_path: str, content: str):
	with open(file_path, 'w', encoding='utf-8', newline=NEWLINE) as f:
		f.write(content)
