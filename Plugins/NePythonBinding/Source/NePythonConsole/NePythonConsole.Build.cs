using UnrealBuildTool;
using System.Collections.Generic;

public class NePythonConsole : ModuleRules
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

	public NePythonConsole(ReadOnlyTargetRules Target) : base(Target)
	{
		bool UseNGTechPython = false;
		bool UseNeteasePython = false;
		if (CheckContains(Target.ProjectDefinitions, "NEPY_BUILD_WITH_NGTECH_PYTHON"))
		{
			UseNGTechPython = true;
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
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"UnrealEd",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"TargetPlatform",
				"NePythonBinding",
				"OutputLog"
			}
		);
	}
}
