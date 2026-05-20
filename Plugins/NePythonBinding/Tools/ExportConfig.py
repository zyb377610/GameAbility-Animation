# -*- encoding: utf-8 -*-
import os
import re
import io
import json
import platform
import DebugLog as dlog
import glob
import shutil

# 根模块名
RootModuleName = 'ue'

# 插件模块API名字
PluginModuleAPIName = 'NEPYTHONBINDING_API'

# 游戏工程源码目录
GamePath = '' # 可以不填 脚本目录一路向上搜索到的第一个.uproject文件所在目录即为游戏工程源码目录 (如果把Nepy插件放置在引擎目录则需要填写)

# 游戏工程名
GameProject = '' # 可以不填 会以游戏工程源码目录搜索出对应的.uproject文件

# 引擎源码目录
EnginePath = '' # 可以不填 会以.uproect文件的信息查找出引擎源码目录 源码目录会以/Engine/结尾

# 游戏工程配置
GameProjectConfiguration = 'Development' # 源码编译的UE可使用Debug，Development，Epic商城下载的UE可以用Development

# 静态导出是否使用PrettyName(ScriptName)
UsePrettyName = True

# 自动生成的Python文档缩进字符
PythonDocIndentChars = '\t'

def find_game_path():
	global GamePath
	if GamePath and os.path.exists(GamePath):
		return
	elif GamePath:
		dlog.info('[Error] Specified game source path does not exist: %s' % (GamePath,))
		return

	# 从当前文件目录开始
	search_path = os.path.dirname(os.path.abspath(__file__))

	while True:
		# 检查当前目录的.uproject文件
		if os.path.exists(search_path):
			for filename in os.listdir(search_path):
				if filename.endswith(".uproject"):
					GamePath = search_path
					dlog.info("Found game source: %s -> %s" % (filename, GamePath))
					return

		# 向上一级目录
		parent_path = os.path.dirname(search_path)
		if search_path == parent_path:  # 到达根目录
			break
		search_path = parent_path

	dlog.info("[Error] No game source path found, please specify GamePath in ExportConfig.py")

# 自动获取游戏工程源码目录
find_game_path()

def find_game_project():
	global GameProject
	# 检查指定的GameProject是否存在
	project_path = os.path.join(GamePath, GameProject)

	if GameProject and os.path.exists(project_path):
		dlog.info('Found game project file: %s' % (project_path,))
		return
	elif GameProject:
		dlog.info('[Error] Specified game project file does not exist: %s' % (GameProject,))
		return

	# 搜索目录中的所有.uproject文件
	search_pattern = os.path.join(GamePath, "*.uproject")
	uproject_files = glob.glob(search_pattern)

	if not uproject_files:
		dlog.info('[Error] No .uproject files found in directory: %s' % (GamePath,))
		return

	if len(uproject_files) == 1:
		# 只找到一个.uproject文件，直接使用
		new_project = os.path.basename(uproject_files[0])
		dlog.info('Auto-detected game project file: %s' % (new_project,))
		GameProject = new_project
	else:
		project_list = [os.path.basename(f) for f in uproject_files]
		dlog.info('[Error] Found multiple .uproject files: %s' % (project_list,))

# 自动获取游戏工程文件
find_game_project()

def makedirs_compat(path):
	if not os.path.exists(path):
		os.makedirs(path)
	elif not os.path.isdir(path):
		raise OSError("Path exists but not a directory: %s" % path)

# Game binding代码生成目录
GameAutoDir = 'Auto'
GameBindingGeneratePath = os.path.join(GamePath, "Source", GameProject.replace('.uproject', ''), "Public", "NePy", GameAutoDir)

makedirs_compat(GameBindingGeneratePath)

def get_unreal_engine_path():
	global GamePath, GameProject, EnginePath
	if EnginePath and os.path.exists(EnginePath):
		return
	elif EnginePath:
		dlog.info('[Error] Specified engine source path does not exist: %s' % (EnginePath,))
		return

	uproject_path = project_path = os.path.join(GamePath, GameProject)

	# Windows注册表导入
	if platform.system() == "Windows":
		try:
			import winreg
		except ImportError:
			winreg = None
	else:
		winreg = None

	try:
		# 检查项目文件是否存在
		if not os.path.exists(uproject_path):
			dlog.info('[Error] Project file does not exist: %s' % (uproject_path,))
			return

		# 从项目文件读取EngineAssociation
		try:
			with io.open(uproject_path, 'r', encoding='utf-8') as file:
				project_data = json.load(file)
			association = project_data.get('EngineAssociation', '')
		except Exception as e:
			dlog.info('[Error] Failed to parse project file: %s' % (e,))
			return

		if not association:
			dlog.info('[Error] No EngineAssociation found in project file')
			return

		engine_path = None

		# Windows系统处理
		if platform.system() == "Windows" and winreg:
			# 版本号格式查找 (如 "5.3", "5.4")
			if '.' in association:
				registry_paths = [
					(winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Epic Games\\Unreal Engine\\%s' % (association,)),
					(winreg.HKEY_CURRENT_USER, 'SOFTWARE\\Epic Games\\Unreal Engine\\%s' % (association,)),
					(winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\WOW6432Node\\Epic Games\\Unreal Engine\\%s' % (association,))
				]

				for hkey, reg_path in registry_paths:
					try:
						with winreg.OpenKey(hkey, reg_path) as key:
							install_dir, _ = winreg.QueryValueEx(key, "InstalledDirectory")
							if install_dir and os.path.exists(install_dir):
								engine_path = install_dir
								break
					except Exception:
						continue

			# 构建ID查找
			if not engine_path:
				build_registry_paths = [
					(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\Epic Games\\Unreal Engine\\Builds"),
					(winreg.HKEY_CURRENT_USER, "SOFTWARE\\Epic Games\\Unreal Engine\\Builds"),
					(winreg.HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Epic Games\\Unreal Engine\\Builds")
				]
				
				for hkey, reg_path in build_registry_paths:
					try:
						with winreg.OpenKey(hkey, reg_path) as key:
							build_path, _ = winreg.QueryValueEx(key, association)
							if build_path and os.path.exists(build_path):
								engine_path = build_path
								break
					except Exception:
						continue

		# Mac系统处理
		elif platform.system() == "Darwin":  # macOS
			# 首先尝试从Epic Launcher配置文件读取
			install_ini_path = os.path.expanduser("~/Library/Application Support/Epic/UnrealEngine/Install.ini")

			if os.path.exists(install_ini_path):
				try:
					with io.open(install_ini_path, 'r', encoding='utf-8') as f:
						lines = f.readlines()
					in_installations = False
					fallback_path = None

					for line in lines:
						line = line.strip()

						# 检查是否进入[Installations]节
						if line == '[Installations]':
							in_installations = True
							continue
						elif line.startswith('[') and line.endswith(']'):
							in_installations = False
							continue

						# 在[Installations]节中解析GUID=路径格式
						if in_installations and '=' in line:
							guid, install_location = line.split('=', 1)
							guid = guid.strip()
							# GUID补花括号与uproject里的EngineAssociation匹配
							guid = '{%s}' % (guid,)
							install_location = install_location.strip()
							if install_location and os.path.exists(install_location):
								# 保存第一个有效路径作为fallback
								if not fallback_path:
									fallback_path = install_location

								# 如果association是GUID格式，直接匹配GUID
								if guid == association:
									engine_path = install_location
									break

								# 如果association是版本号格式(如"5.5")，检查路径中的版本信息
								if '.' in association:
									# 检查路径名称是否包含版本号
									path_lower = install_location.lower()
									version_patterns = [
										'ue%s' % association,           # ue5.5
										'ue_%s' % association,          # ue_5.5  
										'unreal%s' % association,       # unreal5.5
										'unrealengine%s' % association, # unrealengine5.5
										association                     # 5.5
									]

									if any(pattern in path_lower for pattern in version_patterns):
										engine_path = install_location
										break

								# 通过Build.version文件验证版本
								version_file = os.path.join(install_location, "Engine", "Build", "Build.version")
								if os.path.exists(version_file):
									try:
										with io.open(version_file, 'r', encoding='utf-8') as vf:
											version_data = json.load(vf)
										major = version_data.get('MajorVersion', 0)
										minor = version_data.get('MinorVersion', 0)
										version_string = '%d.%d' % (major, minor)

										# 精确匹配版本号
										if version_string == association:
											engine_path = install_location
											break
									except Exception:
										continue

					# 如果没有精确匹配，使用fallback路径
					if not engine_path and fallback_path:
						engine_path = fallback_path
						dlog.info('[Warning] Using fallback engine path: %s' % engine_path)

				except Exception as e:
					dlog.info('[Warning] Failed to parse Install.ini: %s' % (e,))

			# 如果配置文件中没找到，使用默认路径查找
			if not engine_path:
				# Epic Games Launcher安装的标准路径
				epic_base_paths = [
					'/Users/Shared/Epic Games/UE_%s' % association,
					'/Applications/Epic Games/UE_%s' % association,
					os.path.expanduser('~/Epic Games/UE_%s' % association)
				]

				# 检查版本号格式的路径
				if '.' in association:
					for base_path in epic_base_paths:
						if os.path.exists(base_path):
							engine_path = base_path
							break

				# 如果没找到，尝试查找自定义构建路径
				if not engine_path:
					custom_paths = [
						'/opt/UnrealEngine_%s' % association,
						os.path.expanduser('~/UnrealEngine_%s' % association),
						'/usr/local/UnrealEngine_%s' % association
					]

					for custom_path in custom_paths:
						if os.path.exists(custom_path):
							engine_path = custom_path
							break

		else:
			dlog.info('[Error] Unsupported platform: %s' % platform.system())
			return

		# 验证引擎路径并设置EnginePath
		if engine_path:
			# 标准化路径
			engine_path = os.path.normpath(engine_path)

			# 验证是否为有效的UE安装
			engine_dir = os.path.join(engine_path, "Engine")

			# 根据平台检查编辑器可执行文件
			if platform.system() == "Windows":
				exe_path_ue4 = os.path.join(engine_path, "Engine", "Binaries", "Win64", "UE4Editor.exe")
				exe_path_ue5 = os.path.join(engine_path, "Engine", "Binaries", "Win64", "UnrealEditor.exe")
				valid_installation = os.path.exists(engine_dir) and (os.path.exists(exe_path_ue4) or os.path.exists(exe_path_ue5))
			elif platform.system() == "Darwin":
				editor_paths = [
					os.path.join(engine_path, "Engine", "Binaries", "Mac", "UnrealEditor.app"),
					os.path.join(engine_path, "Engine", "Binaries", "Mac", "UE4Editor.app")
				]
				valid_installation = os.path.exists(engine_dir) and any(os.path.exists(path) for path in editor_paths)

			if valid_installation:
				if platform.system() == "Windows":
					EnginePath = os.path.join(engine_path, "Engine").replace("\\", "/") + "/"
				else:
					EnginePath = os.path.join(engine_path, "Engine") + "/"
				dlog.info('Successfully found Unreal Engine %s at: %s' % (association, EnginePath))
			else:
				dlog.info('[Error] Invalid engine installation at: %s' % (engine_path,))
		else:
			dlog.info('[Error] Engine association not found: %s' % (association,))

	except Exception as e:
		import traceback
		traceback.print_exc()
		dlog.info('[Error] Exception during engine path search: %s' % (e,))

# 自动获取引擎源码目录
get_unreal_engine_path()

# 是否在游戏工程中使用Nepy插件
bUnderGamePlugin = False

def get_plugin_under_game_or_not():
	global bUnderGamePlugin, GamePath
	script_path = os.path.abspath(__file__)
	game_path = os.path.abspath(GamePath)

	def commonpath_compat(paths):
		paths = [os.path.abspath(p) for p in paths]
		split_paths = [p.split(os.sep) for p in paths]
		common = []
		for parts in zip(*split_paths):
			if len(set(parts)) == 1:
				common.append(parts[0])
			else:
				break
		return os.sep.join(common) if common else os.sep
	bUnderGamePlugin = commonpath_compat([script_path, game_path]) == game_path
	dlog.info('Plugin is under game: %s' % (bUnderGamePlugin))

# 获取插件是否放置在游戏源码目录里
get_plugin_under_game_or_not()

# 自动获取引擎版本号
build_version_file_path = os.path.join(EnginePath, 'Build/Build.version')
dlog.info('Build version file: %s' % (build_version_file_path,))
with io.open(build_version_file_path, 'r') as f:
	version_data = json.load(f)
	ENGINE_MAJOR_VERSION = version_data['MajorVersion']
	ENGINE_MINOR_VERSION = version_data['MinorVersion']
EngineVersion = (ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION)

# 引擎binding代码生成目录
EngineAutoDir = 'Auto'
EngineBindingGeneratePath = os.path.abspath('../Source/NePythonBinding/Public/NePy/%s' % EngineAutoDir)

makedirs_compat(EngineBindingGeneratePath)

# NePyNoExportTypes.h文件路径
# 若无匹配的引擎，则使用最大引擎版本
NePyNoExportTypesFilePath = os.path.abspath('templates/UE_%s%s' % (ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION))
if not os.path.isdir(NePyNoExportTypesFilePath):
	max_engine_version = ''
	template_path = os.path.abspath('templates')
	for cur_path, dir_list, file_list in os.walk(template_path):
		for dir_name in dir_list:
			if dir_name.startswith('UE_'):
				if dir_name > max_engine_version:
					max_engine_version = dir_name
	if max_engine_version:
		NePyNoExportTypesFilePath = os.path.abspath('templates/%s' % max_engine_version)
		dlog.info('fallback using: %s' % NePyNoExportTypesFilePath)

# python stub 生成目录
PythonStubDir = os.path.abspath('pystubs')

# 如果父类拥有PyDict的时候 是否创建子类的PyDict
# 如果设置为True 则会创建
# 如果设置为False 则会在该类Ready后 设置tp_dictoffset为0
AutoCreateSubClassPyDict = False

# 是否使用手动标注的NePyNoExportTypes.h来获取反射信息，这将禁用clang
UseNePyNoExportTypes = True

# 是否允许向 UObject* 类型的属性和函数参数传入None
AllowPropertySetNone = True

# 是否将Core/Math中的数学库模板使用double类型导出
# 如果设置为True，则使用double类型导出
# 如果设置为False，则使用float类型导出
# NOTE: UE5默认数学库为double类型，使用double类型导出能防止C++/Python接口层的精度丢失
UseDoubleFloatMath = ENGINE_MAJOR_VERSION >= 5
FloatType = 'double' if UseDoubleFloatMath else 'float'
NotUseFloatType = 'float' if UseDoubleFloatMath else 'double'

# 要导出的模块，必须按照模块依赖顺序排列（参考xxx.Build.cs）
ExportModules = [
	{
		# python模块名
		'module': 'core',
		# 对应的UE4包
		'packages': [
			{
				'name': '/Script/CoreUObject',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'CoreUObject'),
				'exclude': {
					'structs': {
						# 私有成员访问错误
						'DateRangeBound': True,
						'DoubleRangeBound': True,
						'FloatRangeBound': True,
						'Int8RangeBound': True,
						'Int16RangeBound': True,
						'Int32RangeBound': True,
						'Int64RangeBound': True,
						'FrameNumberRange': True,
						'FrameNumberRangeBound': True,
						'FrameTime': True,
						'PolyglotTextData': True,
						'TemplateString': True, # 5.1导出报错
						'Matrix44f': True, # 5.1导出报错
						'Transform3f': True, # 5.1属性不可访问
						'RemoteObjectId': True, # 5.6属性不可访问
						'RemoteServerId': True, # 5.6属性不可访问
						'RemoteObjectBytes': True, # 5.6属性不可访问
						'RemoteObjectData': True, # 5.6属性不可访问
						'RemoteObjectTables': True, # 5.6属性不可访问
						'RemoteObjectPathName': True, # 5.6属性不可访问
						'RemoteObjectReference': True, # 5.6属性不可访问
						'RemoteWorkPriority': True, # 5.6属性不可访问
						'RemoteTransactionId': True, # 5.6属性不可访问
						'PackedRemoteObjectPathName': True, # 5.6属性不可访问
						# 私有成员访问错误
						'RandomStream': {
							'props': True,
						},
						# 私有成员访问错误
						'PrimaryAssetType': {
							'props': True,
						},
						# 私有成员访问错误
						'SoftObjectPath': {
							'props': True,
						},
						# 私有成员访问错误
						'Transform': {
							'props': True,
						},
						# 蓝图里成员与C++里不一样
						'Matrix': {
							'props': True,
						},
						# 私有成员访问错误 5.5
						'Timespan': {
							'props': True,
						},
						# 私有成员访问错误 5.5
						'DateTime': {
							'props': True,
						},
						'Sphere': True,  # UE 5.2 模板编译错误
					}
				}
			},
		],
		# 代码生成目录
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'physicscore',
		'packages': [
			{
				'name': '/Script/PhysicsCore',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'PhysicsCore'),
				'exclude': {
					'structs': {
						'PhysicalMaterialStrength': True, # 5.3，no export
						'PhysicalMaterialDamageModifier': True, # 5.4，no export
					},
				},
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'input',
		'packages': [
			{
				'name': '/Script/InputCore',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'InputCore'),
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		# python模块名
		'module': 'engine',
		# 对应的UE4包
		'packages': [
			{
				'name': '/Script/Engine',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'Engine'),
				'exclude': {
					'structs': {
						'FloatRK4SpringInterpolator': {
							# 类成员均为protected，导出后会编译报错
							'props': True,
						},
						'VectorRK4SpringInterpolator': {
							# 类成员均为protected，导出后会编译报错
							'props': True,
						},
						'InputActionSpeechMapping': {
							'props': {
								# 私有成员访问错误
								'ActionName': True,
								'SpeechKeyword': True,
							},
						},
						# 私有成员访问错误
						'SoundConcurrencySettings': True,
						# 'TimerHandle': True, # 手动实现
						'AlphaBlend': True,
						'AnimLinkableElement': True,
						'AnimNode_AssetPlayerBase': True,
						'BodyInstance': True,
						# 依赖问题导致链接错误
						'InteriorSettings': True,
						'AtmospherePrecomputeParameters': True,
						'SoundClassProperties': True,
						'ActorDesc': True, # ue 5.1
						'AudioComponentParam': True, # ue 5.1
						'AudioBasedVibrationData': True, # ue 5.2
						'SpecularProfileStruct': True, # 5.3, no export
						'StreamingSourceShape': True, # 5.4
						'AttenuationSubmixSendSettings': True, # 5.4
						'UniqueNetIdRepl': True, # 5.4
						'ImportanceTexture': True, # 5.6
					},
					'classes': {
						# 无法 include "AsyncActionLoadPrimaryAsset.h"
						'AsyncActionLoadPrimaryAssetBase': True,
						'AsyncActionLoadPrimaryAsset': True,
						'AsyncActionLoadPrimaryAssetClass': True,
						'AsyncActionLoadPrimaryAssetList': True,
						'AsyncActionLoadPrimaryAssetClassList': True,
						'AsyncActionChangePrimaryAssetBundles': True,
						# 缺少DllExport导致链接错误
						'AtmosphericFogComponent': True,
						# 反射获取的属性名不对导致编译错误
						'AssetExportTask': {
							'props': {
								'Filename': True,
								'FileName': True,
							}
						},
						'AudioComponent': {
							'funcs': {
								'SetOverrideAttenuation': True, # 5.4
								'SetAttenuationSettings': True, # 5.4
								'SetAttenuationOverrides': True, # 5.4
							}
						},
						'BandwidthTestActor': True,
						'DialogueSoundWaveProxy': True,
						'GameplayStatics': {
							'funcs': {
								'BlueprintSuggestProjectileVelocity': True, # 5.4
							}
						},
						'InterpTrackColorScale': True,
						'InterpTrackFade': True,
						'InterpTrackFloatAnimBPParam': True,
						'InterpTrackFloatParticleParam': True,
						'InterpTrackSlomo': True,
						'KismetArrayLibrary': True,
						'PluginBlueprintLibrary': True, # no export api tag, ue 5.2
						'KismetMathLibrary': {
							'funcs': {
								'Convert1DTo2D': True, # 5.4
								'Convert1DTo3D': True, # 5.4
								'Convert2DTo1D': True, # 5.4
								'Convert3DTo1D': True, # 5.4
							}
						},
						'KismetSystemLibrary': {
							'funcs': {
								'StackTrace': True,
								'LoadAsset': True,  # NePy还没有处理类中类的情况 UKismetSystemLibrary::FOnAssetLoaded
								'LoadAssetClass': True,  # 同上 UKismetSystemLibrary::FOnAssetClassLoaded
								'IsValidInterface': True, # 5.4
								'IsObjectOfSoftClass': True, # 5.5
								'RaiseScriptError': True, # 5.6
							}
						},
						'DataTableFunctionLibrary': {
							'funcs': {
								'GetDataTableRowFromName': True,  # 这个函数手写导出了
							}
						},
						'LevelInstanceComponent': True, # ue 5.0
						'WorldPartitionBlueprintLibrary': True, # ue 5.1
						'LevelStreamingPersistent': True,
						re.compile(r'^MaterialExpression'): True,
						'MaterialInstanceConstant': {
							'funcs': {
								'K2_GetTextureParameterValue': True,
								'K2_GetScalarParameterValue': True,
								'K2_GetVectorParameterValue': True,
								'GetTextureParameterValue': True,
								'GetScalarParameterValue': True,
								'GetVectorParameterValue': True,
							}
						},
						'MaterialInstanceDynamic':{
							'funcs': {
								'K2_GetTextureParameterValue': True,
								'K2_GetScalarParameterValue': True,
								'K2_GetVectorParameterValue': True,
								'GetTextureParameterValue': True,
								'GetScalarParameterValue': True,
								'GetVectorParameterValue': True,
							}
						},
						'Texture': {
							'funcs': {
								'Blueprint_GetMemorySize': True,
								'Blueprint_GetTextureSourceDiskAndMemorySize': True, # ue 5.2
								'ComputeTextureSourceChannelMinMax': True, # ue 5.2
								'Blueprint_GetTextureSourceIdString': True, # ue 5.5
								'Blueprint_GetBuiltTextureSize': True, # ue 5.6
							},
						},
						'MaterialParameterCollection': {
							'funcs': {
								'GetScalarParameterNames': True, # ue 5.0
								'GetVectorParameterNames': True, # ue 5.0
								'GetScalarParameterDefaultValue': True, # ue 5.0
								'GetVectorParameterDefaultValue': True, # ue 5.0
							}
						},
						'MeshVertexPainterKismetLibrary': True,
						'ParticleSystem': {
							'funcs': {
								'ContainsEmitterType': True,
							},
						},
						'ParticleEventManager': True,
						'ParticleModuleLightBase': True,
						'PhysicsThruster': True,
						'PhysicsConstraintComponent': True, # 5.4
						'PlanarReflection': True,
						'RuntimeVirtualTextureStreamingProxy': True,
						'SceneCapture2D': True,
						'Texture2D': {
							'funcs': {
								'Blueprint_GetSizeX': True,
								'Blueprint_GetSizeY': True,
							},
						},
						'VirtualTexture2D': True,
						'TimelineComponent': {
							'funcs': {
								'OnRep_Timeline': True,
							},
						},
						'VisualLoggerKismetLibrary': True,
						'SceneComponent': {
							'funcs': {
								# 为了默认参数
								'SetRelativeLocation': True,
								'SetRelativeRotation': True,
							}
						},
						'SkeletalMeshComponent': {
							'props': {
								'AnimBlueprintGeneratedClass': True  # Deprecated 属性，并且导致编译报错
							}
						},
						'StaticMesh': {
							'funcs': {
								'SetMinLODForQualityLevels': True, # ue 5.2 关闭PCH以后链接错误
								'GetMinLODForQualityLevels': True # ue 5.2 关闭PCH以后链接错误
							}
						},
						'SkeletalMesh': {
							'funcs': {
								'SetMinLODForQualityLevels': True, # 5.4
								'GetMinLODForQualityLevels': True, # 5.4
							}
						},
						'SoundSubmix': {
							'funcs': {
								'SetOutputVolumeModulation': True, # 5.3, protected
								'SetWetVolumeModulation': True, # 5.3, protected
							}
						},
						'AnimMontage': {
							'funcs': {
								'IsValidAdditiveSlot': True, # 5.3, no export
							}
						},
						'MeshComponent': {
							'funcs': {
								'GetOverlayMaterialMaxDrawDistance': True, # 5.3, no export
							}
						},
						'FieldNotificationLibrary': {
							'funcs': {
								'SetPropertyValueAndBroadcast': True, # 5.3, no export
							}
						},
						'LevelStreamingLevelInstanceEditorPropertyOverride': True, # 5.5, no export
						'SceneCaptureComponent': {
							'funcs': {
								'GetShowFlagSettings': True, # 5.5, no export
							}
						},
						'SoundSubmix': {
							'funcs': {
								'SetOutputVolumeModulation': True, # 5.5, no export
								'SetWetVolumeModulation': True, # 5.5, no export
								'SetDryVolumeModulation': True, # 5.5, no export
							}
						},
						# 一些我看不顺眼的类
						re.compile(r'^VOIP'): True,
					},
				},
			},
		],
		# 代码生成目录
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'coreonline',
		'packages': [
			{
				'name': '/Script/CoreOnline',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'CoreOnline'),
				'exclude': {
					'structs': {
						'UniqueNetIdWrapper': True, # 5.4，no export
					},
				},
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'slate',
		'packages': [
			{
				'name': '/Script/SlateCore',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'SlateCore'),
				# 导出会报错
				'exclude': {
					'structs': {
						'Geometry': {
							'props': {
								'Size': True,
								'Scale': True,
								'Position': True,
								'AbsolutePosition': True,
							},
							'funcs': {
								'ToPaintGeometry': True,
								'ToOffsetPaintGeometry': True,
								'ToInflatedPaintGeometry': True,
								'GetClippingRect': True,
								'GetLayoutBoundingRect': True,
								'GetRenderBoundingRect': True,
								'GetAccumulatedLayoutTransform': True,
							}
						},
						'PointerEvent': {
							'funcs': {
								'IsMouseButtonDown': True,
								'GetEffectingButton': True,
								'GetGestureType': True,
							}
						},
						'DeprecateSlateVector2D': True, # ue 5.3
					},
				},
			},
			{
				'name': '/Script/Slate',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'Slate'),
			},
		],
		'gen_path': EngineBindingGeneratePath,
		'manual_structs': ['SlateApplication', ], # 手写的结构体，工具只用负责把它加入模块中即可
	},
	{
		'module': 'movie',
		'packages': [
			{
				'name': '/Script/MovieScene',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'MovieScene'),
				'exclude': {
					'classes': {
						re.compile(r'^TestMovieScene'): True,
					},
					'structs': {
						re.compile(r'^TestMovieScene'): True,
						'MovieSceneTimeWarpVariant': True, # ue 5.5
						'MovieSceneNumericVariant': True, # ue 5.5
					},
				},
			},
			{
				'name': '/Script/MovieSceneTracks',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'MovieSceneTracks'),
				'exclude': {
					'classes': {
						'MovieSceneSkeletalAnimationSection': {
							'props': {
								# TInstancedStruct 暂缺处理
								'MixedAnimationTarget': True,
							}
						},
					},
					'structs': {
						'MovieSceneSkeletalAnimationSectionTemplate': True,
						'MovieSceneSkeletalAnimationSectionTemplateParameters': True,
						'MovieSceneSkeletalAnimationParams': True,
					},
				},
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'umg',
		'packages': [
			{
				'name': '/Script/UMG',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'UMG'),
				'exclude': {
					'classes': {
						# 缺少DllExport导致链接错误
						'WidgetAnimation': {
							'funcs': {
								'UnbindAllFromAnimationStarted': True,
								'UnbindAllFromAnimationFinished': True,
								'UnbindFromAnimationFinished': True,
								'BindToAnimationFinished': True,
								'UnbindFromAnimationStarted': True,
								'BindToAnimationStarted': True,
							},
						},
						'WidgetAnimationPlayCallbackProxy': {
							'funcs': {
								'CreatePlayAnimationProxyObject': True,
								'CreatePlayAnimationTimeRangeProxyObject': True,
								'NewPlayAnimationProxyObject': True,
								'NewPlayAnimationTimeRangeProxyObject': True,
							},
						},
						'WidgetBlueprintLibrary': {
							'funcs': {
								# 'Create': True, # 此方法手写了
								'SetWindowTitleBarOnCloseClickedDelegate': True,
							}
						},
						'UserWidget':
						{
							'funcs': {
								'SetInputActionPriority': True, # ue 5.2 protected
								'SetInputActionBlocking': True, # ue 5.2 protected
							}
						}
					},
				},
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'sequence',
		'packages': [
			{
				'name': '/Script/LevelSequence',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'LevelSequence'),
				'exclude': {
					"classes": {
						"LevelSequenceMediaController": {
							"funcs": {
								"OnStopPlaying": True,
								"OnStartPlaying": True,
								"OnTick": True,
							},
						},
					},
				}
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'cinematic',
		'packages': [
			{
				'name': '/Script/CinematicCamera',
				'source_dir': os.path.join(EnginePath, 'Source', 'Runtime', 'CinematicCamera'),
				'exclude': {
					'classes': {
						'CineCameraSettings': True, # ue 5.1, UCineCameraSettings中全部的uproperty的setter函数都是private，
					}
				}
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'niagara',
		'packages': [
			{
				'name': '/Script/Niagara',
				'source_dir': os.path.join(EnginePath, 'Plugins', 'FX', 'Niagara', 'Source', 'Niagara'),
				'exclude': {
					'structs': {
						'NiagaraRendererReadbackParameters': True, # 5.6，属于private
					},
					'classes': {
						# UE4.26导出这个类型会编译错误，UE4.27以后就不会
						'NiagaraDataInterfaceGrid3DCollection': True,
						'NiagaraDataInterfaceSceneCapture2D': True, # 5.3，属于private
						'NiagaraDataInterfaceArrayMesh': True, # 5.6，属于private
						'NiagaraDataInterfaceStaticMesh': True, # 5.6，属于private
						'NiagaraStatelessEmitter': True, # 5.6，属于private
						'NiagaraComponent': {
							'funcs': {
								'SetOcclusionQueryMode': True, # 5.3， no export
							},
						},
					},
				},
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'gas',
		'packages': [
			{
				'name': '/Script/GameplayAbilities',
				'source_dir': os.path.join(EnginePath, 'Plugins', 'Runtime', 'GameplayAbilities', 'Source', 'GameplayAbilities'),
				'exclude': {
					'structs': {
						'GameplayTagChangedEventWrapperSpecHandle': True,
						re.compile('GameplayCueNotify.+'): True,
					},
					'classes': {
						'GameplayCueNotify_UnitTest': True,
						'MovieSceneGameplayCueTrack':
						{
							"funcs": {
								"SetSequencerTrackHandler": True,
							}
						}
					}
				}
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
	# 当需要跑NePythonTest的mvvm单元测试时候要在.uplugin与NePythonBinding.Build.cs配置好ModelViewViewModel插件依赖
	{
		'module': 'mvvm',
		'packages': [
			{
				'name': '/Script/ModelViewViewModel',
				'source_dir': os.path.join(EnginePath, 'Plugins', 'Runtime', 'ModelViewViewModel', 'Source', 'ModelViewViewModel'),
			}
		],
		'gen_path': EngineBindingGeneratePath,
	},
	{
		'module': 'nepy',
		'packages': [
			{
				'name': '/Script/NePythonBinding',
				'source_dir': os.path.join(GamePath, 'Plugins', 'NePythonBinding', 'Source', 'NePythonBinding') if not bUnderGamePlugin else os.path.abspath('../Source/NePythonBinding'),
			},
		],
		'gen_path': EngineBindingGeneratePath,
	},
]

# 要求强制导出的类
# 这些类一般会由于没有标记为BlueprintType，或被标记为Deprecated，而默认不导出
# 这些类的信息不会存在于reflection_infos.json，但存在于reflection_infos_full.json
ForceExportClasses = {
	"Level",
}

# 要求强制导出的结构体
# 这些类一般会由于没有标记为BlueprintType，或被标记为Deprecated，而默认不导出
# 这些类的信息不会存在于reflection_infos.json，但存在于reflection_infos_full.json
ForceExportStructs = {
}

# 要求强制导出的枚举
# 这些类一般会由于没有标记为BlueprintType，或被标记为Deprecated，而默认不导出
# 这些类的信息不会存在于reflection_infos.json，但存在于reflection_infos_full.json
ForceExportEnums = {
}

# 需要额外使用Clang解析源码的模块，一般用于解析无反射信息的结构体
# 注意：在此处定义的模块和文件仅用于提供生成信息，需要导出的模块仍需在ExportModules中定义！
ClangParsePackages = {
	# 模块名
	'/Script/CoreUObject': [
		{
			# 需导出的类/结构体所在的源文件路径，一般为.h文件
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Box2D.h'),
			# 需要导出的结构体列表
			'structs': {
				# 结构体名
				'FBox2D': {
					'funcs': {
						'exclude': [
							'FBox2D(int32)',
							'FBox2D(const FVector2D *, const int32)',
							'FBox2D(const UE::Math::FBox2D &)', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'BoxSphereBounds.h'),
			'structs': {
				'FBoxSphereBounds': {
					'funcs': {
						'exclude': [
							'FBoxSphereBounds(const FVector *, uint32)',
							'DiagnosticCheckNaN',
							'FBoxSphereBounds(const UE::Math::FBoxSphereBounds &)', # ue 5.0
							'GetSphere', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Color.h'),
			'structs': {
				'FLinearColor': {
					'funcs': {
						'exclude': [
							'FLinearColor(const FVector &)',
							'FLinearColor(const FVector4 &)',
							'FLinearColor(const FFloat16Color &)',
							'Serialize',
							'InitFromString',
						],
					},
				},
				'FColor': {
					'funcs': {
						'exclude': [
							'Serialize',
							'InitFromString',
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Misc', 'DateTime.h'),
			'structs': {
				'FDateTime': {
					'funcs': {
						'exclude': [
							'Serialize',
							'NetSerialize',
							'ExportTextItem',
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Misc', 'Guid.h'),
			'structs': {
				'FGuid': {
					'funcs': {
						'exclude': [
							'Serialize',
							'ExportTextItem',
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Matrix.h'),
			'structs': {
				'FMatrix': {
					'funcs': {
						'exclude': [
							'Serialize',
							'DebugPrint',
							'FMatrix(const UE::Math::FMatrix &)', # ue 5.0
							'DiagnosticCheckNaN', # ue 5.0
							'GetScaledAxis', # ue 5.0
							'GetUnitAxis', # ue 5.0
							'Mirror', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Plane.h'),
			'structs': {
				'FPlane': {
					'funcs': {
						'exclude': [
							'Serialize',
							'FPlane(const UE::Math::FPlane &)', # ue 5.0
							'DiagnosticCheckNaN', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'CoreUObject', 'Public', 'UObject', 'PrimaryAssetId.h'),
			'structs': {
				'FPrimaryAssetType': {
					'funcs': {
						'exclude': [
							'FPrimaryAssetType(EName)',
							'FPrimaryAssetType(const WIDECHAR *)',
							'FPrimaryAssetType(const ANSICHAR *)',
							'ExportTextItem',
						],
					},
				},
				'FPrimaryAssetId': {
					'funcs': {
						'exclude': [
							'ExportTextItem',
						]
					}
				}
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Quat.h'),
			'structs': {
				'FQuat': {
					'funcs': {
						'exclude': [
							'Serialize',
							'NetSerialize',
							'DiagnosticCheckNaN',
							'InitFromString',
							'FQuat(const UE::Math::FQuat &)', # ue 5.0
							'ToMatrix(FMatrix &)', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'RandomStream.h'),
			'structs': {
				'FRandomStream': {
					'funcs': {
						'exclude': [
							'ExportTextItem',
						]
					}
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Rotator.h'),
			'structs': {
				'FRotator': {
					'funcs': {
						'exclude': [
							'Serialize',
							'SerializeCompressed',
							'SerializeCompressedShort',
							'NetSerialize',
							'DiagnosticCheckNaN',
							'ToCompactString',
							'InitFromString',
							'FRotator(const UE::Math::FRotator &)', # ue 5.0
							'GetComponentForAxis', # ue 5.0
							'SetComponentForAxis', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'CoreUObject', 'Public', 'UObject', 'SoftObjectPath.h'),
			'structs': {
				'FSoftObjectPath': {
					'funcs': {
						'exclude': [
							'FSoftObjectPath(FSoftObjectPath &&)',
							'FSoftObjectPath(FWideStringView)',
							'FSoftObjectPath(FAnsiStringView)',
							'FSoftObjectPath(FName)',
							'FSoftObjectPath(const WIDECHAR *)',
							'FSoftObjectPath(const ANSICHAR *)',
							'FSoftObjectPath(TYPE_OF_NULLPTR)',
							'SetPath(FWideStringView)',
							'SetPath(FAnsiStringView)',
							'SetPath(FName)',
							'SetPath(const WIDECHAR*)',
							'SetPath(const ANSICHAR*)',
							'PreSavePath',
							'PostLoadPath',
							'FixupForPIE',
							'FixupCoreRedirects',
							'ExportTextItem',
							'TryLoad',
							'FSoftObjectPath(FObjectPtr)', # ue 5.0,
							'GetLongPackageFName', # ue 5.0
						],
					},
				},
				'FSoftClassPath': {
					'funcs': {
						'exclude': [
							'TryLoadClass',
						],
					},
				}
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'TransformNonVectorized.h'),
			'structs': {
				'FTransform': {
					'funcs': {
						'exclude': [
							'DiagnosticCheckNaN_Scale3D',
							'DiagnosticCheckNaN_Translate',
							'DiagnosticCheckNaN_Rotate',
							'DiagnosticCheckNaN_All',
							'DiagnosticCheck_IsValid',
							'DebugPrint',
							'DebugEqualMatrix',
							'InitFromString',
							'FTransform(const UE::Math::FTransform &)', # ue 5.0
							'operator*(int)', # ue 5.0
							'operator*=(int)', # ue 5.0
							'operator*(%s)' % FloatType, # ue 5.0
							'operator*=(%s)' % FloatType, # ue 5.0
							'GetScaledAxis', # ue 5.0
							'GetUnitAxis', # ue 5.0
							'Mirror', # ue 5.0
							'Accumulate(const FTransform &, %s)' % FloatType, # ue 5.0
							'Accumulate(const UE::Math::FTransform &, %s)' % FloatType, # ue 5.0
							'AccumulateWithShortestRotation', # ue 5.0
							'AccumulateWithAdditiveScale', # ue 5.0
							'LerpTranslationScale3D', # ue 5.0
							'BlendFromIdentityAndAccumulate', # ue 5.0
							'GetTranslationRegister', # ue 5.1
							'GetRotationRegister', # ue 5.1
							'SetRotationRegister', # ue 5.1
							'SetTranslationRegister', # ue 5.1
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Vector.h'),
			'structs': {
				'FVector': {
					'funcs': {
						'exclude': [
							'FVector(const FVector2D, %s)' % FloatType,
							'FVector(const FVector4&)',
							'FVector(const FLinearColor&)',
							'FVector(FIntVector)',
							'FVector(FIntPoint)',
							'Serialize',
							'NetSerialize',
							'DiagnosticCheckNaN',
							'ToText',
							'ToCompactString',
							'ToCompactText',
							'InitFromString',
							'FVector(const UE::Math::FVector &)', # ue 5.0
							'GetComponentForAxis', # ue 5.0
							'SetComponentForAxis', # ue 5.0
							'InitFromCompactString', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Vector2D.h'),
			'structs': {
				'FVector2D': {
					'funcs': {
						'exclude': [
							'FVector2D(FIntPoint)',
							'FVector2D(const FVector &)',
							'FVector2D(const FVector4 &)',
							'Serialize',
							'NetSerialize',
							'DiagnosticCheckNaN',
							'InitFromString',
							'FVector2D(const UE::Math::FVector2D)', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'Vector4.h'),
			'structs': {
				'FVector4': {
					'funcs': {
						'exclude': [
							'FVector4(const FVector&, %s)' % FloatType,
							'FVector4(const FLinearColor&)',
							'FVector4(FVector2D, FVector2D)',
							'FVector4(FIntVector4)',
							'Serialize',
							'DiagnosticCheckNaN',
							'InitFromString',
							'FVector4(const FVector &)', # ue 5.0
							'FVector4(const UE::Math::FVector4 &, %s)' % FloatType, # ue 5.0
							'FVector4(const FLinearColor&, %s)' % FloatType, # ue 5.0
							'FVector4(const FIntVector4 &)', # ue 5.0
							'FVector4(const UE::Math::FVector4 &)', # ue 5.0
						],
					},
				},
			},
		},
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'Core', 'Public', 'Math', 'UnrealMathUtility.h'),
			'structs': {
				'FMath': {
					'funcs': {
						'exclude': [
							'PolarToCartesian(const FVector2D,FVector2D &)',
							'CartesianToPolar(const FVector2D,FVector2D &)',
							'RandRange(%s, %s)' % (NotUseFloatType, NotUseFloatType), # ue 5.0
							'FRandRange(%s, %s)' % (NotUseFloatType, NotUseFloatType), # ue 5.0
							'IsPowerOfTwo', # ue 5.0
							'Log2(%s)' % NotUseFloatType, # ue 5.0
							'FastAsin(%s)' % NotUseFloatType, # ue 5.0
							'SphereAABBIntersection', # ue 5.0 # todo
							'LineSphereIntersection', # ue 5.0 # todo
						],
					},
				},
			},
		},
	],
	'/Script/InputCore': [
		{
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'InputCore', 'Classes', 'InputCoreTypes.h'),
			'structs': {
				'FKey': {
					'funcs': {
						'exclude': [
							'SerializeFromMismatchedTag',
							'ExportTextItem',
							'ImportTextItem',
							'PostSerialize',
							'PostScriptConstruct',
						]
					}
				}
			},
		}
	],
	'/Script/SlateCore': [
		{
			# 需导出的类/结构体所在的源文件路径，一般为.h文件
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'SlateCore', 'Public', 'Layout', 'Geometry.h'),
			# 需要导出的结构体列表
			'structs': {
				# 结构体名
				'FGeometry': {
					'funcs': {
						'exclude': [
							'FGeometry()',
							'operator=(const FGeometry& RHS)',
							'FGeometry(const FVector2D& OffsetFromParent, const FVector2D&, const FVector2D&, float)',
						],
					},
				},
			},
		},
		{
			# 需导出的类/结构体所在的源文件路径，一般为.h文件
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'SlateCore', 'Public', 'Input', 'Events.h'),
			# 需要导出的结构体列表
			'structs': {
				# 结构体名
				'FInputEvent': {
					'funcs': {
						'exclude': [
							'GetWindow()',
						]
					},
				},
				'FKeyEvent': {
				},
				'FAnalogInputEvent': {
				},
				'FCharacterEvent': {
				},
				'FPointerEvent': {
					'funcs': {
						'exclude': [
							'FPointerEvent(uint32, const FVector2D&, const FVector2D&, const TSet<FKey>&,FKey, float, const FModifierKeysState&)',
							'FPointerEvent(uint32,uint32,const FVector2D&,const FVector2D&,const TSet<FKey>&,FKey,float,const FModifierKeysState&)',
							'FPointerEvent()',
						],
					},
				},
				'FMotionEvent': {
				},
				'FNavigationEvent': {
				},
			},
		},
		{
			# 需导出的类/结构体所在的源文件路径，一般为.h文件
			'header': os.path.join(EnginePath, 'Source', 'Runtime', 'SlateCore', 'Public', 'Styling', 'SlateColor.h'),
			# 需要导出的结构体列表
			'structs': {
				# 结构体名
				'FSlateColor': {
					'funcs': {
						'exclude': [
							'SerializeFromMismatchedTag(const struct FPropertyTag& Tag, FStructuredArchive::FSlot Slot)',
						],
					},
				},
			},
		},
	],
}

# 带有NoExport的FLAG，但是依然可以register的一些struct
# NoExportTypes.h里的struct没有StaticStruct方法， 但以下列表中的struct在UObject/Class.h中通过模板类TBaseStructure实现了Get方法获取UScriptStruct*
NeedRegisterStruct = ['Rotator', 'Quat', 'Transform', 'Color', 'LinearColor', 'Plane', 'Vector', 'Vector2D', 'Vector4', 'RandomStream', 'Guid', 'Box2D',
'FallbackStruct', 'FloatRangeBound', 'FloatRange', 'Int32RangeBound', 'Int32Range', 'FloatInterval', 'Int32Interval', 'FrameNumber', 'SoftObjectPath',
'SoftClassPath', 'PrimaryAssetType', 'PrimaryAssetId', 'DateTime', 'PolyglotTextData']

# 一些特殊的UClass子类的头文件
SpecialClassHeaders = {
	'BlueprintGeneratedClass': 'Engine/BlueprintGeneratedClass.h',
	'AnimBlueprintGeneratedClass': 'Animation/AnimBlueprintGeneratedClass.h',
}

# 以下类型需要额外解析其它文件，来生成py stub
PyStubExtraParseFiles = {
	'Object': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', 'Manual', 'NePyObjectBase.inl')),
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyObject.cpp')),
	],
	'Class': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyClass.cpp')),
	],
	'ScriptStruct': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyScriptStruct.cpp')),
	],
	'StructBase': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyStructBase.cpp')),
	],
	'NePyObjectRef': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyObjectRef.cpp')),
	],
	'TickerHandle': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyTickerHandle.cpp')),
	],
	'TimerHandle': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyTimerHandle.cpp')),
	],
	'TimerManagerWrapper': [
		os.path.abspath(os.path.join(EngineBindingGeneratePath, '..', '..', 'NePyTimerManagerWrapper.cpp')),
	],
}

# 一些类是手写的，无需生成代码
# 但需要提供类型信息，并生成文档
ManualTypeInfos = {
	'Object': {
		'cpp_name': 'UObject',
		'doc': 'Base class for all UE objects',
		'package': '/Script/CoreUObject',
		'header': 'UObject/NoExportTypes.h',
		'extra_overrides': {
			'super': 'object',
			'py_name': 'FNePyObject',
			'py_new_func_name': 'NePyObjectNew',
			'py_check_func_name': 'NePyObjectCheck',
			'py_get_type_func_name': 'NePyObjectGetType',
			'py_header_name': 'NePyObject.h',
			'py_source_name': 'NePyObject.cpp',
			'py_header_path': 'NePyObject.h',
		}
	},
	'Class': {
		'cpp_name': 'UClass',
		'doc': 'An object class',
		'package': '/Script/CoreUObject',
		'header': 'UObject/Class.h',
		'extra_overrides': {
			'super': 'Object',
		}
	},
	'ScriptStruct': {
		'cpp_name': 'UScriptStruct',
		'doc': 'An ScriptStruct',
		'package': '/Script/CoreUObject',
		'header': 'UObject/Class.h',
		'extra_overrides': {
			'super': 'Object',
			'py_header_name': 'NePyScriptStruct.h',
			'py_source_name': 'NePyScriptStruct.cpp',
			'py_header_path': 'NePyScriptStruct.h',
		},
	},
	'Enum': {
		'cpp_name': 'UEnum',
		'doc': 'An Enum',
		'package': '/Script/CoreUObject',
		'header': 'UObject/Class.h',
		'extra_overrides': {
			'super': 'Object',
		},
	},
	'StructBase': {
		'cpp_name': '',
		'doc': 'Base class for unreal structs',
		'package': '/Script/CoreUObject',
		'header': '',
		'extra_overrides': {
			'super': 'object',
		}
	},
	'TableRowBase': {
		'cpp_name': 'FTableRowBase',
		'doc': 'Base class for all table row structs to inherit from.',
		'package': '/Script/Engine',
		'header': 'Engine/DataTable.h',
		'extra_overrides': {
			'super': 'StructBase',
			'py_name': 'FNePyTableRowBase',
			'py_new_func_name': 'NePyTableRowBaseNew',
			'py_check_func_name': 'NePyTableRowBaseCheck',
			'py_get_type_func_name': 'NePyTableRowBaseGetType',
			'py_header_name': 'NePyTableRowBase.h',
			'py_source_name': 'NePyTableRowBase.cpp',
			'py_header_path': 'NePyTableRowBase.h',
		}
	},
	'Interface': {
		'cpp_name': 'UInterface',
		'doc': 'Base class for all interfaces',
		'package': '/Script/CoreUObject',
		'header': 'UObject/Interface.h',
		'extra_overrides': {
			'super': 'Object',
			'py_get_type_func_name': 'NePyInterfaceGetType',
			'py_header_name': 'NePyInterface.h',
			'py_source_name': 'NePyInterface.cpp',
			'py_header_path': 'NePyInterface.h',
		}
	},
	'NePyObjectRef': {
		'cpp_name': 'FNePyObjectRef',
		'package': '/Script/NePythonBinding',
		'header': 'NePyObjectRef.h',
		'extra_overrides': {
			'is_boxing_type': True,
			'boxing_func_name': 'FNePyObjectRef::Boxing',
			'unboxing_func_name': 'FNePyObjectRef::Unboxing',
			'py_header_name': 'NePyObjectRef.h',
			'py_source_name': 'NePyObjectRef.cpp',
			'py_header_path': 'NePyObjectRef.h',
		},
	},
	'TickerHandle': {
		'cpp_name': 'FNePyTickerHandle',
		'package': '/Script/NePythonBinding',
		'header': 'NePyTickerHandle.h',
	},
	'TimerHandle': {
		'cpp_name': 'FTimerHandle',
		'package': '/Script/Engine',
		'header': 'Engine/EngineTypes.h',
		'extra_overrides': {
			'py_header_name': 'NePyTimerHandle.h',
			'py_source_name': 'NePyTimerHandle.cpp',
			'py_header_path': 'NePyTimerHandle.h',
		},
	},
	'TimerManagerWrapper': {
		'cpp_name': 'FTimerManager',
		'package': '/Script/Engine',
		'header': 'TimerManager.h',
	},
	'Math': {
		'cpp_name': 'FMath',
		'package': '/Script/CoreUObject',
		'header': 'Math/UnrealMathUtility.h',
		'need_build_template': True,
	}
}


def verify_configs():
	global ExportModules

	has_error = False

	for path_name in (
		'EnginePath',
		'GamePath',
		'EngineBindingGeneratePath',
		'GameBindingGeneratePath',
		'PythonStubDir',
		):
		path_val = globals().get(path_name)
		if not os.path.isdir(path_val):
			dlog.info('[Error] path "%s" not exist: %s' % (path_name, path_val))
			has_error = True

	for module_rule in ExportModules:
		module_name = module_rule['module']
		gen_path = module_rule['gen_path']
		if not os.path.isdir(gen_path):
			dlog.info('[Error] Code generation path for moudule "%s" dont exist: %s' \
				% (module_name, gen_path))
			has_error = True

		gen_path_arr = gen_path.replace('\\', '/').split('/')
		if not len(gen_path_arr) >= 2 or \
			not gen_path_arr[-1].startswith('Auto') or \
			gen_path_arr[-2] != 'NePy':
			dlog.info('[Error] Code generation path must end with ("NePy/Auto")! : %s' \
				% (gen_path, ))
			has_error = True

		for package_rule in module_rule['packages']:
			source_dir = package_rule.get('source_dir')
			if not source_dir:
				dlog.info('[Error] package "%s" did not specified source_dir!' \
					% (package_rule['name'], ))
				has_error = True
			elif not os.path.isdir(source_dir):
				dlog.info('[Error] source_dir for package "%s" dont exist: %s' \
					% (package_rule['name'], source_dir))
				has_error = True

	if not has_error:
		dlog.info('Check done, all OK!')


if __name__ == '__main__':
	verify_configs()
