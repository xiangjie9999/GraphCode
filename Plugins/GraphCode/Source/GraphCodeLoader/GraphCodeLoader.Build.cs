using UnrealBuildTool;

public class GraphCodeLoader : ModuleRules
{
    public GraphCodeLoader(ReadOnlyTargetRules Target) : base(Target)
    {
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
        BinariesSubFolder = "BuildOutputs";
        PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.Add("Core");
    }
}


