# -*- encoding: utf-8 -*-


def str_decode(str_data, encoding):
	# type: (str, str) -> str
	return str_data.decode(encoding)


def open_and_read_str(file_path):
	# type: (str) -> str
	with open(file_path, 'rb') as f:
		return f.read()


def open_and_write_str(file_path, content):
	# type: (str, str) -> None
	with open(file_path, 'wb') as f:
		if not isinstance(content, str):
			try:
				content = content.encode('utf-8')
			except UnicodeEncodeError:
				content = content.encode('gbk')
		f.write(content)
