using UnrealBuildTool;

public class GraphCodeCoreExtension : ModuleRules
{
	public GraphCodeCoreExtension(ReadOnlyTargetRules Target) : base(Target)
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
				"GraphCode",
				// Human references use Core's product-neutral mesh provenance seam. Neither Shape nor
				// Groom enters Advanced's hard dependency graph.
				"WidgetIR",
				"AuthoringCore",
				"UMG",
				"UMGEditor",
				"UnrealEd",
				"BlueprintGraph",
				// A MetaHuman GraphCode keeps open is let go when the Content Browser selects it with another:
				// opening those together skips the per-asset request `FGcMetaHumanOpen` otherwise answers.
				"ContentBrowser",

				// `kind:env_query` and its AIModule/AIGraph/EnvironmentQueryEditor links live in the
				// GraphCodeEnvQuery adapter (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): a project may switch
				// Environment Query Editor off, and that must not stop this module loading.

				// **`landmarks_config.json` — the ONE thing that makes a face region nameable.**
				// `GC-HUMAN-GRAMMAR.md` SS0.1: the engine has no public region-name accessor, and
				// the names are nevertheless shipped as plain JSON in the MetaHumanCharacter
				// plugin's own content. Projects mounts the plugin so its content directory can be
				// found; Json reads the file. Both are engine runtime modules, so neither is gated
				// even though the only caller is.
				"Json",
				"Projects",

				// A `.gchuman` assembly lists the folder it writes into so it can refuse to overwrite
				// packages it does not own, and records what it made as a manifest. Named rather than
				// leaned on transitively, like AssetTools below.
				"AssetRegistry",

				// G00's GPU acceptance renders the isolated native Groom fixture and reads the
				// resulting pixels. These are editor-module test dependencies; the authored
				// builder still hands rendering and deformation to HairStrands at runtime.
				"Renderer",
				"RenderCore",
				"RHI",
			}
		);

		// The four framework plugins are optional. Their codecs belong in adapter modules or behind
		// GraphCodeBuildGates checks as they land; making any one a hard dependency here would prevent a
		// customer from installing the other three. This base module supplies the product identity.
	}
}



