// Copyright Epic Games, Inc. All Rights Reserved.

// 使用NGTech Python，请到项目的.Target.cs和Editor.Target.cs里添加
// ProjectDefinitions.Add("NEPY_BUILD_WITH_NGTECH_PYTHON");
// 使用网易版UE，请到项目的.Target.cs和Editor.Target.cs里添加
// ProjectDefinitions.Add("NEPY_BUILD_WITH_NETEASE_PYTHON");
//   note: 打开后，会使用Netease_Python作为Python虚拟机。
// 使用Github版UE，请关闭此选项。
//   note: 关闭后，会使用内置的Python2.7虚拟机

using UnrealBuildTool;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;

namespace UnrealBuildTool.Rules
{
	public class NePythonBinding : ModuleRules
	{
		bool CheckContains(IReadOnlyList<string> List, string Target)
		{
			for (int Index = 0; Index < List.Count; Index++)
			{
				if (List[Index] == Target)
					return true;
			}
			return false;
		}

		public NePythonBinding(ReadOnlyTargetRules Target) : base(Target)
		{
#if !UE_5_0_OR_LATER
			// 针对UE4强制启用C++17支持
			CppStandard = CppStandardVersion.Cpp17;
#endif
			
			// Subclassing时，是否将Python类的'long'类型成员属性和函数参数映射为UE的'int64'类型
			// NOTE: 对于Python3而言，'int'就是'long'
			bool bSubclassingPyLongAsUEInt64 = false;

			// Subclassing时，是否将Python类的'float'类型成员属性和函数参数映射为UE的'double'类型
			bool bSubclassingPyFloatAsUEDouble = false;

			if (bSubclassingPyLongAsUEInt64)
			{
				PrivateDefinitions.Add("NEPY_SUBCLASSING_PY_LONG_AS_UE_INT64=1");
			}
			else
			{
				PrivateDefinitions.Add("NEPY_SUBCLASSING_PY_LONG_AS_UE_INT64=0");
			}

			if (bSubclassingPyFloatAsUEDouble)
			{
				PrivateDefinitions.Add("NEPY_SUBCLASSING_PY_FLOAT_AS_UE_DOUBLE=1");
			}
			else
			{
				PrivateDefinitions.Add("NEPY_SUBCLASSING_PY_FLOAT_AS_UE_DOUBLE=0");
			}

			// 是否启用结构体深层访问
			bool bEnableStructDeepAccess = true;
			if (bEnableStructDeepAccess)
			{
				// 注意是PublicDefinitions
				PublicDefinitions.Add("NEPY_ENABLE_STRUCT_DEEP_ACCESS=1");
			}
			else
			{
				PublicDefinitions.Add("NEPY_ENABLE_STRUCT_DEEP_ACCESS=0");
			}

			// 是否启用Python对象跟踪
			// 启用后，NePy会提供对象分配位置追踪、生命周期监控等调试功能
			bool bEnablePythonObjectTracking = false;
			if (bEnablePythonObjectTracking)
			{
				// 注意是PublicDefinitions
				PublicDefinitions.Add("NEPY_ENABLE_PYTHON_OBJECT_TRACKING=1");
			}
			else
			{
				PublicDefinitions.Add("NEPY_ENABLE_PYTHON_OBJECT_TRACKING=0");
			}

			// 是否启用NePyGeneratedType的世界分区支持
			// 在世界分区下，会保存Actor的生成类的Native父类，如果Python生成类本身就是Native父类
			// 会导致是世界分区的WorldPartitionActorDescUtils.cpp里的UClass::TryFindTypeSlow<UClass>找不到Python生成类
			// 解决方式：
			// 1.开启NEPY_GENERATED_TYPE_SUPPORT_WORLD_PARTATION宏
			// 或者：
			// 2.修改引擎源码把TryFindTypeSlow的参数从EFindFirstObjectOptions::ExactClass替换为EFindFirstObjectOptions::NativeFirst
			bool bEnableWorldPartationSupport = true;

			if (bEnableWorldPartationSupport)
			{
				PrivateDefinitions.Add("NEPY_GENERATED_TYPE_SUPPORT_WORLD_PARTATION=1");
			}
			else
			{
				PrivateDefinitions.Add("NEPY_GENERATED_TYPE_SUPPORT_WORLD_PARTATION=0");
			}

			bool UseNGTechPython = false;
			bool UseNeteasePython = false;
			if (CheckContains(Target.ProjectDefinitions, "NEPY_BUILD_WITH_NGTECH_PYTHON"))
			{
				UseNGTechPython = true;
				PrivateDefinitions.Add("USE_NGTECH_PYTHON=1");
			}
			else if (CheckContains(Target.ProjectDefinitions, "NEPY_BUILD_WITH_NETEASE_PYTHON"))
			{
				UseNeteasePython = true;
			}

			if (UseNGTechPython)
			{
				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"NGTech_Python"
					}
				);
				UseNeteasePython = true;
			}
			else if (UseNeteasePython)
			{
				PublicDependencyModuleNames.AddRange(
					new string[]
					{
						"NetEase_Python"
					}
				);
			}

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"SlateCore",
					"Slate",
					"UMG",
					"LevelSequence",
					"CinematicCamera",
					"MovieScene",
					"MovieSceneTracks",
					"PhysicsCore",
					"PakFile",
					"SandboxFile",
					"RenderCore",
					"ApplicationCore",
					"AppFramework",
					"RHI",
					"Json",
					"Niagara",
					"NetCore",
					"TraceLog",
					"DeveloperSettings",
					"GameplayAbilities",
					"GameplayTags"
				}
			);

#if UE_5_3_OR_LATER
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"FieldNotification",
					// "ModelViewViewModel"
				}
			);
#endif

			if (Target.bBuildEditor) {
				PrivateDependencyModuleNames.AddRange(
					new string[] {
					"UnrealEd",
					"BlueprintGraph",
					"Kismet",
					"PropertyEditor",
					"Projects"
					}
				);
			}

			string IncludePathBase = Path.Combine(ModuleDirectory, "Public", "NePy");
			foreach (string IncludePath in Directory.GetDirectories(IncludePathBase, "*", SearchOption.AllDirectories))
			{
				PublicIncludePaths.Add(IncludePath);
			}

			if (UseNeteasePython)
			{
				PublicDefinitions.Add("NEPY_BUILD_WITH_NETEASE_UE=1");
			}
			else
			{
				PublicDefinitions.Add("NEPY_BUILD_WITH_NETEASE_UE=0");
				string ThirdPartyPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "ThirdParty"));
				PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "openssl", "include"));
				PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, "Python312", "include"));

				if (Target.Platform == UnrealTargetPlatform.Win64)
				{
					//if (Target.LinkType == TargetLinkType.Modular)
					{
						PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Modular", "libssl.lib"));
						PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Modular", "libcrypto.lib"));
						PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "Python312.lib"));

						if (Target.bBuildEditor)
						{
							RuntimeDependencies.Add("$(BinaryOutputDir)/libcrypto-1_1-x64.dll", Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Modular", "libcrypto-1_1-x64.dll"));
							RuntimeDependencies.Add("$(BinaryOutputDir)/libssl-1_1-x64.dll", Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Modular", "libssl-1_1-x64.dll"));
							RuntimeDependencies.Add("$(BinaryOutputDir)/Python312.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "Python312.dll"));
							RuntimeDependencies.Add("$(BinaryOutputDir)/zlib.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "zlib.dll"));
							RuntimeDependencies.Add("$(BinaryOutputDir)/libbz2.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "libbz2.dll"));
							RuntimeDependencies.Add("$(BinaryOutputDir)/libexpat.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "libexpat.dll"));
						}
						else
						{
							RuntimeDependencies.Add("$(TargetOutputDir)/libcrypto-1_1-x64.dll", Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Modular", "libcrypto-1_1-x64.dll"));
							RuntimeDependencies.Add("$(TargetOutputDir)/libssl-1_1-x64.dll", Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Modular", "libssl-1_1-x64.dll"));
							RuntimeDependencies.Add("$(TargetOutputDir)/Python312.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "Python312.dll"));
							RuntimeDependencies.Add("$(TargetOutputDir)/zlib.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "zlib.dll"));
							RuntimeDependencies.Add("$(TargetOutputDir)/libbz2.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "libbz2.dll"));
							RuntimeDependencies.Add("$(TargetOutputDir)/libexpat.dll", Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Modular", "libexpat.dll"));
						}
					}
					//else
					//{
					//	PublicDefinitions.Add("Py_NO_ENABLE_SHARED");
					//	PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Monolithic", "libssl.lib"));
					//	PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "Win64Monolithic", "libcrypto.lib"));
					//	PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Monolithic", "Python312.lib"));
					//	PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Monolithic", "zlibstatic.lib"));
					//	PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Monolithic", "libbz2.lib"));
					//	PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "Win64Monolithic", "expat.lib"));
					//}
				}
				else if (Target.Platform == UnrealTargetPlatform.Android)
				{
					// armv8a
					Debug.Assert(Target.LinkType == TargetLinkType.Monolithic);
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "AndroidMonolithic", "libssl.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "AndroidMonolithic", "libcrypto.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "AndroidMonolithic", "libPython312.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "AndroidMonolithic", "libbz2.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "AndroidMonolithic", "libexpat.a"));
				}
				else if (Target.Platform == UnrealTargetPlatform.Mac)
				{
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "MacModular", "libssl.dylib"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "MacModular", "libcrypto.dylib"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "MacModular", "libPython312.dylib"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "MacModular", "libexpat.dylib"));
				}
				else if (Target.Platform == UnrealTargetPlatform.IOS)
				{
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "IOSMonolithic", "libssl.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "openssl", "Lib", "IOSMonolithic", "libcrypto.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "IOSMonolithic", "libPython312.a"));
					PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, "Python312", "Lib", "IOSMonolithic", "libexpat.a"));
					PublicSystemLibraries.Add("bz2");
				}
				else
				{
					throw new System.Exception("Platform not support yet: " + Target.Platform.ToString() + " " + Target.LinkType.ToString());
				}
			}
		}
	}
}