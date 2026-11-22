using UnrealBuildTool;

public class GraphCodeMetaSound : ModuleRules
{
	public GraphCodeMetaSound(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GraphCode", "AuthoringCore", "UnrealEd", "AssetTools" });
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "Metasound", "WITH_GRAPHCODE_METASOUND",
			new string[] { "MetasoundEngine", "MetasoundFrontend", "MetasoundGraphCore", "MetasoundEditor" });
	}
}



