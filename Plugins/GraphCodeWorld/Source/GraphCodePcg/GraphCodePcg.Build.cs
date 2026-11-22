using UnrealBuildTool;
public class GraphCodePcg : ModuleRules
{
	public GraphCodePcg(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "GraphCode", "GraphCodeRuntime", "AuthoringCore", "GraphCodePcgAuthoring",
			"GraphCodeWorld", "UnrealEd", "AssetRegistry", "NavigationSystem", "Landscape", "Foliage", "BSPUtils", "GeometryCore"
		});
		PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "../GraphCodeWorld/Private"));
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "PCG", "WITH_GRAPHCODE_PCG", new string[] { "PCG" });
	}
}



