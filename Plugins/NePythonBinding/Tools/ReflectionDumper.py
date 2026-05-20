# -*- encoding: utf-8 -*-
import os
import time
import shutil
import ExportConfig
import DebugLog as dlog
import stat
import platform

TARGET_NAME = ExportConfig.GameProject.replace('.uproject', '') + 'Editor'
TARGET_CONF = ExportConfig.GameProjectConfiguration

# 通过UHT获取反射信息时，临时将NePyNoExportTypes.h复制到Source目录中。
# 反射信息获取结束后，自动将NePyNoExportTypes.h删除，因为它会导致编译不通过
class CopyNoExportTypesFileBlock(object):
	def __init__(self):
		self.target_path = os.path.abspath('../Source/NePythonBinding/Public/NePy/Manual')  # type: str
		self.source_path = ExportConfig.NePyNoExportTypesFilePath  # type: str

	def __enter__(self):
		if not os.path.exists(self.source_path):
			return
		target_path = os.path.dirname(self.target_path)
		if not os.path.exists(target_path):
			os.makedirs(target_path)
		for path, dirs, file_list in os.walk(self.source_path):
			for file_name in file_list:
				source_file_path = os.path.join(path, file_name)
				target_file_path = os.path.join(self.target_path, file_name)
				shutil.copy(source_file_path, target_file_path)

	def __exit__(self, exc_type, exc_val, exc_tb):
		if not os.path.exists(self.source_path):
			return
		for path, _, file_list in os.walk(self.source_path):
			for file_name in file_list:
				target_file_path = os.path.join(self.target_path, file_name)
				os.chmod(target_file_path, stat.S_IWRITE)
				os.remove(target_file_path)


class RunNePythonGenerator(object):
	def __init__(self):
		self.target_path = os.path.abspath('../Source/NePythonBinding/Public/NePy/Manual/RunNePythonGenerator.h') # type: str
		self.source_path = os.path.abspath('templates/RunNePythonGenerator.h') # type: str

	def __enter__(self):
		if os.path.exists(self.source_path):
			target_path = os.path.dirname(self.target_path)
			if not os.path.exists(target_path):
				os.makedirs(target_path)
			shutil.copy(self.source_path, self.target_path)

	def __exit__(self, exc_type, exc_val, exc_tb):
		if os.path.exists(self.target_path):
			os.remove(self.target_path)


def dump_reflection_infos_by_ubt():
	# UBT.exe文件路径
	if platform.system() == "Windows":
		ubt_paths = [
			# ue5
			os.path.join(ExportConfig.EnginePath, 'Binaries', 'DotNET', 'UnrealBuildTool', 'UnrealBuildTool.exe'),
			# ue4
			os.path.join(ExportConfig.EnginePath, 'Binaries', 'DotNET', 'UnrealBuildTool.exe'),
		]
	elif platform.system() == "Darwin":  # macOS
		ubt_paths = [
			# ue5
			os.path.join(ExportConfig.EnginePath, 'Binaries', 'DotNET', 'UnrealBuildTool', 'UnrealBuildTool'),
			# ue4
			os.path.join(ExportConfig.EnginePath, 'Binaries', 'DotNET', 'UnrealBuildTool'),
		]
	elif platform.system() == "Linux":
		ubt_paths = [
			# ue5
			os.path.join(ExportConfig.EnginePath, 'Binaries', 'Linux', 'UnrealBuildTool'),
			# ue4
			os.path.join(ExportConfig.EnginePath, 'Binaries', 'Linux', 'UnrealBuildTool'),
		]

	for ubt_path in ubt_paths:
		if os.path.isfile(ubt_path):
			# found!
			break
	else:
		dlog.info('Error: UnrealBuildTool.exe is missing (%s)' % ubt_paths)
		return False

	dlog.debug('ubt_path:', ubt_path)
	# 游戏工程文件路径
	project_file_path = os.path.join(ExportConfig.GamePath, ExportConfig.GameProject)
	if not os.path.isfile(project_file_path):
		dlog.info('Error: project file is missing (%s)' % project_file_path)
		return False
	dlog.debug('project_file_path:', project_file_path)
	# UBT编译目标参数
	target_param_str = ""
	if platform.system() == "Windows":
		target_param_str = '-Target="%s Win64 %s -Project=\\"%s\\""' % (TARGET_NAME, TARGET_CONF, project_file_path)
	elif platform.system() == "Darwin":  # macOS
		target_param_str = '-Target="%s Mac %s -Project=\\"%s\\""' % (TARGET_NAME, TARGET_CONF, project_file_path)
	else:
		raise Exception('Error: platform is not supported (%s)' % platform.system())
	
	cmd = [
		os.path.abspath(ubt_path),
		target_param_str,
		'-FromMsBuild',
		# '-WaitMutex', # 加锁
		'-ForceHeaderGeneration', # 强制调用UHT
		'-SkipBuild', # 不进行编译
		'-Mode="Build"', # 执行Build模式
	]
	cmd = ' '.join(cmd)
	dlog.debug('cmd:', cmd)

	# 正确的命令行示例：
	#UnrealBuildTool.exe -Target="ClientEditor Win64 Debug -Project=\"F:\yy46_svn\code\trunk\client\game\Client.uproject\"" -Mode="Build" -FromMsBuild -ForceHeaderGeneration -SkipBuild
	cwd = os.getcwd()
	os.chdir(os.path.dirname(ubt_path))
	ret = os.system(cmd)
	os.chdir(cwd)
	if ret != 0:
		dlog.info('Error: UnrealBuildTool.exe return error code (%s)' % ret)
		return False
	return True


def main():
	with RunNePythonGenerator():
		if not dump_reflection_infos_by_ubt(): # 最优方案
			return False
	return True


if __name__ == '__main__':
	dlog.set_log_level(dlog.LOG_LEVEL_DEBUG)
	begin = time.time()
	main()
	print('cost time: %.2f' % (time.time() - begin))
