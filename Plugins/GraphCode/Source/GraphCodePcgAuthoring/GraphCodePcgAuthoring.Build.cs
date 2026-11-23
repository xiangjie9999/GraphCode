using UnrealBuildTool;

// Shared native graph codec. Product modules own registrations, not this module.
public class GraphCodePcgAuthoring : ModuleRules
{
    public GraphCodePcgAuthoring(ReadOnlyTargetRules Target) : base(Target)
    {
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
        BinariesSubFolder = "BuildOutputs";
        PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GraphCode", "AuthoringCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "AssetRegistry", "BlueprintIR" });
        bool bPcg = GraphCodeBuildGates.IsProjectPluginEnabled(Target, "PCG")
            || GraphCodeBuildGates.IsProjectPluginEnabled(Target, "ProceduralVegetationEditor");
        PublicDefinitions.Add("WITH_GRAPHCODE_PCG_AUTHORING=" + (bPcg ? "1" : "0"));
        if (bPcg) PublicDependencyModuleNames.Add("PCG");
    }
}


