using UnrealBuildTool;

public class GraphCodeIntegrate : ModuleRules
{
    public GraphCodeIntegrate(ReadOnlyTargetRules Target) : base(Target)
    {
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
        // Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
        BinariesSubFolder = "BuildOutputs";
        PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "GraphCode" });
        RuntimeDependencies.Add("$(PluginDir)/Resources/...", StagedFileType.NonUFS);
    }
}


