using UnrealBuildTool;

/**
 * The `.gc` object grammar, with no editor under it.
 *
 * **WHY THIS MODULE EXISTS, and why the answer is not "move these files into Play".**
 * `RUNTIME-DRIVE-DESIGN.md` §17 P0 asked for `FGraphCodeActorEntryWriter` to move out of Core's
 * editor module and into Play's runtime one. Measured 2026-08-14 before doing it: that header is
 * included by SEVENTEEN files in Core — the level writer, the input writer, the slice service, all
 * three merge tiers, import, describe, capabilities — and `GraphCodeActorProjection.h` by sixteen.
 * Moving either into Play makes **Core depend on Play**, which inverts the one rule
 * `GraphCodePlay.Build.cs` exists to state: Core must never need a product's code, or a Core-only
 * build cannot be built at all.
 *
 * So the files move DOWN rather than sideways. Everything here is what a capture and a projection
 * genuinely share — the framing, the property rule, the sort order, the placement rule, the layout
 * rule — and none of it has ever touched `GEditor`, `GetActorLabel`, `GetActorGuid` or `UnrealEd`
 * (§19's measurement, confirmed by this module compiling as `Runtime`). `GraphCodeActorEntryWriter.h`
 * predicted the split in its own comments: the label and folder are PASSED IN precisely because
 * "this file is the one three products share — a runtime capture in GraphCode Play compiles it too."
 *
 * The dependency still runs one way. Core (`GraphCode`, Editor) depends on this; Play
 * (`GraphCodePlay`, Runtime) depends on this; neither depends on the other.
 *
 * **It registers nothing.** `FGraphCodeProducts::Register` is how the capability manifest and the
 * MCP bridge answer *what is installed*, and neither exists in a packaged game — so product
 * registration is an editor concern and stays in an editor module (§17's ruling for Play, applied
 * here too). This module is code and no lifecycle, which is why it uses `FDefaultModuleImpl`.
 */
public class GraphCodeRuntime : ModuleRules
{
	public GraphCodeRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        // The ordinary build produces the runtime SDK used by the installed folder.
        // Keep its PCH object inside this plugin so a recipient can link without this host.
        if (!Target.bBuildEditor)
        {
            bPrecompile = true;
            PrecompileForTargets = PrecompileTargetsType.Any;
            PrivatePCHHeaderFile = "Private/GraphCodeRuntimePrivatePCH.h";
        }

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				// Every public header here takes or returns an AuthoringCore type —
				// `FPropertyExportPolicy`, `FAuthoringProperty`, `FAuthoringDiagnostic` — so the
				// include path is part of this module's interface, not an implementation detail.
				// AuthoringCore is `Runtime` as of 2026-08-14, which is what makes this compile.
				"AuthoringCore",
			}
		);
	}
}




