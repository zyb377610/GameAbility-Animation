# -*- encoding: utf-8 -*-
import re
import ExportConfig

try:
	import typing
except:
	pass

# 返回包名到源码目录的字典
def get_package_name_to_source_dir_dict():
	# type: () -> dict[str, str]
	package_name_to_source_dir_dict = {}
	for module_rule in ExportConfig.ExportModules:
		for package_rule in module_rule['packages']:
			package_name = package_rule['name']
			source_dir = package_rule['source_dir']
			old_source_dir = package_name_to_source_dir_dict.get(package_name)
			assert old_source_dir is None or old_source_dir == source_dir, 'package "%s" has different source directories: "%s" "%s"' \
				% (package_name, old_source_dir, source_dir)
			package_name_to_source_dir_dict[package_name] = source_dir
	return package_name_to_source_dir_dict


# 解析出UE4中include用的相对路径
def get_relative_header_path(header_path, source_dir):
	# type: (str, str) -> str
	include_header_path = header_path[len(source_dir) + 1:].replace('\\', '/')
	if include_header_path.startswith('Public/'):
		include_header_path = include_header_path[len('Public/'):]
	elif include_header_path.startswith('Private/'):
		include_header_path = include_header_path[len('Private/'):]
	elif include_header_path.startswith('Classes/'):
		include_header_path = include_header_path[len('Classes/'):]
	return include_header_path

FLOAT_TYPE = ExportConfig.FloatType

MATH_TMPL_REPL = [
	# 将模板类替换回普通类型
	(re.compile(r'\bTBox<.*?>'), 'FBox'),
	(re.compile(r'\bTBox\b'), 'FBox'),
	(re.compile(r'\bTBox2<.*?>'), 'FBox2D'),
	(re.compile(r'\bTBox2\b'), 'FBox2D'),
	(re.compile(r'\bTBoxSphereBounds<.*?>'), 'FBoxSphereBounds'),
	(re.compile(r'\bTBoxSphereBounds\b'), 'FBoxSphereBounds'),
	(re.compile(r'\bTMatrix<.*?>'), 'FMatrix'),
	(re.compile(r'\bTMatrix\b'), 'FMatrix'),
	(re.compile(r'\bTPlane<.*?>'), 'FPlane'),
	(re.compile(r'\bTPlane\b'), 'FPlane'),
	(re.compile(r'\bTQuat<.*?>'), 'FQuat'),
	(re.compile(r'\bTQuat\b'), 'FQuat'),
	(re.compile(r'\bTRotator<.*?>'), 'FRotator'),
	(re.compile(r'\bTRotator\b'), 'FRotator'),
	(re.compile(r'\bTSphere<.*?>'), 'FSphere'),
	(re.compile(r'\bTSphere\b'), 'FSphere'),
	(re.compile(r'\bTTransform<.*?>'), 'FTransform'),
	(re.compile(r'\bTTransform\b'), 'FTransform'),
	(re.compile(r'\bTVector<.*?>'), 'FVector'),
	(re.compile(r'\bTVector\b'), 'FVector'),
	(re.compile(r'\bTVector2<.*?>'), 'FVector2D'),
	(re.compile(r'\bTVector2\b'), 'FVector2D'),
	(re.compile(r'\bTVector4<.*?>'), 'FVector4'),
	(re.compile(r'\bTVector4\b'), 'FVector4'),

	# 位于每个文件底部的模板特化会导致clang傻逼掉
	# 我们去掉对这些文件的include，改用前向声明（参见MATH_FWD_DECL）
	# 一个例外是UnrealMathUtility.h，这里面有很多常量定义是后续要用到的
	(re.compile(r'Math/(?!UnrealMathUtility).*\.h"'), ''),

	# 干掉UE5新增的命名空间以及模板定义
	(re.compile(r'\bUE::Math::\b'), ''),
	(re.compile(r'\s*template\s*<\s*(typename|class).*>'), ''),
	(re.compile(r'typename TEnableIf<TIsFloatingPoint<T>::Value, T>::Type'), FLOAT_TYPE),

	# 干掉函数返回值自动类型推导
	(re.compile(r'\bauto\b'), FLOAT_TYPE),
	(re.compile(r'\s*->\s*decltype\(.*\)'), ''),

	# 将模板参数替换回float/double
	(re.compile(r'\bT\b'), FLOAT_TYPE),
	(re.compile(r'\bU\b'), FLOAT_TYPE),
	(re.compile(r'\bT\d\b'), FLOAT_TYPE), # T1 T2 T3 T4 ...
	(re.compile(r'\bTExtent\b'), FLOAT_TYPE),
	(re.compile(r'\bFArg\b'), FLOAT_TYPE),
	(re.compile(r'\b\w+::FReal\b'), FLOAT_TYPE),
	(re.compile(r'\bFReal\b'), FLOAT_TYPE),

	(re.compile(r'    '), '\t'),
]

# 由于干掉了数学库的Include，补上类型前向声明
MATH_FWD_DECL = [
	'struct FBox;',
	'struct FBox2D;',
	'struct FBoxSphereBounds;',
	'struct FIntPoint;',
	'struct FIntVector;',
	'struct FIntVector4;',
	'struct FMatrix;',
	'struct FPlane;',
	'struct FQuat;',
	'struct FRotator;',
	# 'struct FSphere;', # 不支持导出
	'struct FTransform;',
	'struct FVector;',
	'struct FVector2D;',
	'struct FVector4;',
]

# UE5把数学库全部换成了模板，以支持float/double切换
# 把它们重新还原回非模板的UE4命名，以支持自动binding生成
def replace_math_templates(file_path, source_code):
	# type: (str, str) -> str
	normalized_file_path = file_path.replace('\\', '/')
	if '/Core/Public/Math/' not in normalized_file_path:
		return source_code

	for src, dst in MATH_TMPL_REPL:
		source_code = src.sub(dst, source_code)

	# 一些函数使用到了数学库的静态成员变量作为默认参数
	# 仅补充前向声明是不够的，还需要具体类型定义
	if not file_path.endswith('IntPoint.h'):
		source_code = '''
		struct FIntPoint {
			static FIntPoint ZeroValue;
			static FIntPoint NoneValue;
		};
		''' + source_code
	if not file_path.endswith('IntVector.h'):
		source_code = '''
		struct FIntVector {
			static FIntVector ZeroValue;
			static FIntVector NoneValue;
		};
		''' + source_code
	if not file_path.endswith('Matrix.h'):
		source_code = '''
			struct FMatrix {
			static FMatrix Identity;
		};
		''' + source_code
	if not file_path.endswith('Quat.h'):
		source_code = '''
		struct FQuat {
			static FQuat Identity;
		};
		''' + source_code
	if not file_path.endswith('Rotator.h'):
		source_code = '''
		struct FRotator {
			static FRotator ZeroRotator;
		};
		''' + source_code
	if not file_path.endswith('Vector.h'):
		source_code = '''
		struct FVector {
			static FVector ZeroVector;
			static FVector OneVector;
			static FVector UpVector;
			static FVector DownVector;
			static FVector ForwardVector;
			static FVector BackwardVector;
			static FVector RightVector;
			static FVector LeftVector;
			static FVector XAxisVector;
			static FVector YAxisVector;
			static FVector ZAxisVector;
		};
		''' + source_code
	if not file_path.endswith('Vector2D.h'):
		source_code = '''
		struct FVector2D {
			static FVector2D ZeroVector;
			static FVector2D UnitVector;
			static FVector2D Unit45Deg;
		};
		''' + source_code

	# 补上类型前向声明
	source_code = '\n'.join(MATH_FWD_DECL) + '\n' + source_code

	return source_code


# 预处理代码，将杂七杂八的宏都干掉
def preprocess_source_for_clang(source_code):
	# type: (str) -> str
	lines = source_code.splitlines()
	out_lines = []

	# 需要被干掉的宏
	need_strip_macros = [
		# GENERATED_BODY_LEGACY(...)
		# GENERATED_BODY(...)
		# GENERATED_USTRUCT_BODY(...)
		# GENERATED_UCLASS_BODY(...)
		# GENERATED_UINTERFACE_BODY(...)
		# GENERATED_IINTERFACE_BODY(...)
		'GENERATED_',
	]

	for line in lines:
		line = line.strip()
		if not line:
			continue

		skip = False
		for macro in need_strip_macros:
			if line.startswith(macro):
				skip = True
				break
		
		if skip:
			continue

		out_lines.append(line)

	return '\n'.join(out_lines)


def test():
	src_file = r'G:\UE_5.0\Engine\Source\Runtime\Core\Public\Math\RandomStream.h'
	dst_file = r'G:\yy46-git\nepy\RandomStream_ue5.h'

	with open(src_file, 'rb') as f:
		source_code = f.read()

	source_code = replace_math_templates(source_code)

	with open(dst_file, 'wb') as f:
		f.write(source_code)


if __name__ == '__main__':
	test()
