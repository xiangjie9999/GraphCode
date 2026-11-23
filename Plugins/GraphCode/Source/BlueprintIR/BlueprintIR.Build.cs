using UnrealBuildTool;

/// <summary>
/// Core Blueprint IR module.
///
/// ARCHITECTURE CONSTRAINT (non-negotiable): this module must have ZERO dependency on
/// UnrealClaude / any MCP bridge type. The MCP tool handlers are a thin adapter shell
/// that lives elsewhere and calls into here. When UE 5.8's official MCP Toolset Registry
/// lands, the adapter is deleted and this module is untouched.
///
/// If you ever find yourself adding "UnrealClaude" to this file, the design has failed.
/// </summary>
public class BlueprintIR : ModuleRules
{
	public BlueprintIR(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AuthoringCore",
				"Json",
				"JsonUtilities",

				// Editor + transaction support (FScopedTransaction, FKismetEditorUtilities)
				"UnrealEd",
				"EditorFramework",
				"EditorSubsystem",

				// Blueprint graph traversal and mutation
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"GraphEditor",

				// FBlueprintActionDatabase lives in BlueprintGraph; asset discovery for
				// UUserDefinedEnum / UUserDefinedStruct comes from the asset registry.
				"AssetRegistry",

				// Slate is needed only to refresh open Blueprint editor windows after a
				// write - stale widgets over mutated nodes crash.
				"Slate",
				"SlateCore",
			}
		);
	}
}



