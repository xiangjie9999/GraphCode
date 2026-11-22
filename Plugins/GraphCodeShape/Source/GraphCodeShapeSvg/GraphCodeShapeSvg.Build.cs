using UnrealBuildTool;

// GraphCode Shape's SVG Importer adapter (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): the one module that links Epic's
// SVG types. LoadingPhase None; GraphCode Shape loads it only when SVG Importer is enabled.
public class GraphCodeShapeSvg : ModuleRules
{
	public GraphCodeShapeSvg(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GraphCode", "GraphCodeShape" });
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "SVGImporter", "WITH_GRAPHCODE_SVG", new string[] { "SVGImporter" });
	}
}


