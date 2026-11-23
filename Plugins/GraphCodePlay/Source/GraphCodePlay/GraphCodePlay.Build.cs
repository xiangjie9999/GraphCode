using UnrealBuildTool;

/**
 * GraphCode Play, the half that must exist in a packaged game.
 *
 * **RETYPED `Runtime` 2026-08-14, and the dependency on `GraphCode` is GONE — that was the thing
 * blocking it.** A `Runtime` module may not depend on an `Editor` one, so the retype was never a
 * `.uplugin` edit. `RUNTIME-DRIVE-DESIGN.md` §17 P0 guessed the dependency "simply goes away"; it
 * did not. It was carrying `FGraphCodeProducts::Register`, which is how Core answers *is Play
 * installed* and why `graphcode-capabilities.json` lists `play` at all. Cutting it here would have
 * unregistered the product.
 *
 * So registration moved to `GraphCodePlayEditor` instead, and that is principled rather than a
 * workaround: the registry exists so the capability manifest and the MCP bridge can say what is
 * installed, and **neither exists in a packaged game** — a shipped title has no `tools/list` to
 * answer. The runtime half registers nothing.
 *
 * What the capture writer actually needed from `GraphCode` was never the editor module: it was the
 * object grammar, which moved to `GraphCodeRuntime` on the same day for the same reason.
 */
public class GraphCodePlay : ModuleRules
{
	public GraphCodePlay(ReadOnlyTargetRules Target) : base(Target)
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
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// The dependency runs ONE WAY and must keep doing so. Core knows the NAME of every
				// kind in all five products so that it can refuse one by name; it must never need a
				// product's CODE to do that, or a Core-only build cannot be built at all.
				//
				// Both of these are Core's RUNTIME half, which is the only half Play is allowed to
				// want: `GraphCodeRuntime` is the `.gc` object grammar a capture and a projection
				// share, and `AuthoringCore` is the value codec under it. Neither pulls UnrealEd.
				"AuthoringCore",
				"GraphCodeRuntime",

				// Execution state — RUNTIME-DRIVE-DESIGN.md §20.4 item 3, ruled by building it.
				// A blackboard's values and a Behavior Tree's active branch are read through
				// AIModule, and this is the only module that may hold that dependency:
				// `GraphCodeRuntime` is shared with Core's EDITOR half and would spread AI onto the
				// level projection, describe, import and all three merge tiers; `GraphCodeMove` is
				// `Type: "Editor"` and a Runtime capture may not call it. AIModule is itself
				// `Runtime`, so this stays inside the one-way rule above. The animation third of the
				// subject needs nothing extra — state machines are in Engine.
				"AIModule",

				// Pressing the game's own UI — R7, `GraphCodePlayWidget.h`. All three are `Runtime`,
				// so this stays inside the one-way rule above; the editor's equivalent lives in
				// `WidgetIR`, which is `Type: "Editor"` and therefore unreachable from here.
				"UMG",
				"Slate",
				"SlateCore",
			}
		);

		// **ABILITIES ARE OPTIONAL AND GATED — R3, and the recipe is Water's rather than a new one.**
		// This module is `Type: "Runtime"` and ships INTO the customer's packaged game, so a hard
		// dependency here would say *to install GraphCode Play, enable GameplayAbilities*. That is
		// the hole `GC-WORLD-GENERATION-DESIGN.md` §8 recorded as OPEN when World hard-linked PCG,
		// and it is sharper here: World is editor-side and this one is in the shipped game.
		//
		// With the plugin off `WITH_GRAPHCODE_ABILITIES` is 0, the subject compiles out of
		// `FGraphCodeExecutionState::Collect`, and a record carries no `@tags` line — an absence
		// `omits:` already knows how to state, so nothing has to interpret it.
		//
		// The `.uplugin` half is paid the same way Water's is: `"GameplayAbilities",
		// "Enabled": false, "Optional": true` silences UBT's *"does not list plugin"* warning
		// without switching anything on downstream. `GameplayAbilities` is `EnabledByDefault: false`,
		// so no customer inherits it by accident.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GameplayAbilities",
			"WITH_GRAPHCODE_ABILITIES", new string[] { "GameplayAbilities", "GameplayTags" });

		// **STATE TREES ARE OPTIONAL AND GATED FOR THE SAME REASON — R4.** Same recipe, and the
		// same argument: this module ships into the packaged game, so a hard dependency would say
		// *to install GraphCode Play, enable GameplayStateTree*.
		//
		// **THE GATE ASKS ABOUT `GameplayStateTree` AND LINKS `StateTree` TOO, WHICH IS NOT A
		// SHORTCUT.** `GameplayStateTree.uplugin` lists `StateTree` as a hard `"Enabled": true`
		// dependency, so the two are enabled or absent together and one question answers for both —
		// and asking about `StateTree` instead would answer NO on this very project, because
		// `IsProjectPluginEnabled` reads the `.uproject` and a transitively-enabled plugin is not
		// in it. The subject needs both halves: `UStateTreeComponent` is the component to recognise
		// and `FStateTreeInstanceData` is where its active states live.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GameplayStateTree",
			"WITH_GRAPHCODE_STATETREE", new string[] { "GameplayStateTreeModule", "StateTreeModule" });
	}
}




