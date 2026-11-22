using UnrealBuildTool;

public class GraphCodeMvvm : ModuleRules
{
	public GraphCodeMvvm(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "GraphCode", "AuthoringCore", "WidgetIR", "UMG", "UMGEditor", "UnrealEd", "BlueprintGraph" });
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "ModelViewViewModel", "WITH_GRAPHCODE_MVVM",
			new string[] { "ModelViewViewModel", "ModelViewViewModelBlueprint" });
	}
}



