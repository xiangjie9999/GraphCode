using UnrealBuildTool;

public class GraphCodePve : ModuleRules
{
    public GraphCodePve(ReadOnlyTargetRules Target) : base(Target)
    {
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
        BinariesSubFolder = "BuildOutputs";
        PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject", "Engine", "GraphCode", "AuthoringCore", "UnrealEd", "GraphCodePcgAuthoring", "BlueprintIR",
            "AssetTools", "AssetRegistry", "Projects"
        });
        GraphCodeBuildGates.AddOptionalPlugin(this, Target, "ProceduralVegetationEditor",
            "WITH_GRAPHCODE_PVE", new[] { "ProceduralVegetation", "ProceduralVegetationEditor", "PCG", "Chaos", "AdvancedPreviewScene" });
    }
}


