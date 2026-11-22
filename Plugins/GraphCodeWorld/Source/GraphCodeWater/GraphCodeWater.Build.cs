using UnrealBuildTool;

public class GraphCodeWater : ModuleRules
{
	public GraphCodeWater(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GraphCode", "GraphCodeRuntime", "AuthoringCore", "GraphCodeWorld", "Landscape", "UnrealEd" });
		PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../GraphCodeWorld/Private"));
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "Water", "WITH_GRAPHCODE_WATER", new string[] { "Water" });
	}
}



