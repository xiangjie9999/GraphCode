using UnrealBuildTool;

// Core Extension's `kind:env_query` adapter (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): the one module that links Epic's
// Environment Query Editor. LoadingPhase None; Core Extension loads it only when that plugin is enabled, and without
// it `kind:env_query` refuses by naming the plugin.
public class GraphCodeEnvQuery : ModuleRules
{
	public GraphCodeEnvQuery(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "GraphCode", "AuthoringCore", "UnrealEd",
			// `kind:env_query`'s greenfield factory goes through IAssetTools::CreateAsset, the same door Core's kinds
			// are created by.
			"AssetTools",
			// AIModule owns UEnvQuery and its option/generator/test instances and the FAIDataProviderValue provider
			// structs; AIGraph owns UAIGraphNode::SubNodes; EnvironmentQueryEditor owns the authored EdGraph and its
			// editor nodes (GC-AI-AUTHORING-DESIGN.md §3).
			"AIModule",
			"AIGraph",
			// Linked directly, not through GraphCodeBuildGates.AddOptionalPlugin: the plugin is EnabledByDefault, so a
			// project that does not list it still has it, and the gate reads an unlisted plugin as absent. It is always
			// on the engine's disk, so this adapter always builds; whether it LOADS is the consuming project's call.
			"EnvironmentQueryEditor",
		});
	}
}


