using UnrealBuildTool;

/**
 * The half of GraphCode Play that needs an editor attached — which, today, is exactly one call.
 *
 * **WHY THIS MODULE EXISTS.** `GraphCodePlay` had to become `Runtime` for a packaged game to be
 * able to write a `.gcp` at all, and a `Runtime` module may not depend on an `Editor` one. The only
 * thing standing in the way was `FGraphCodeProducts::Register(EGcProduct::Play)`, which reaches into
 * Core's editor module. Trying to cut that dependency is what revealed it was load-bearing: without
 * it Core answers *Play is not installed* and `graphcode-capabilities.json` stops listing `play`.
 *
 * **And splitting rather than keeping it is the principled answer, not the convenient one.** The
 * product registry exists so the capability manifest and the MCP bridge can say what is installed.
 * A packaged game has no manifest and no `tools/list`, so it has nothing to answer — registration is
 * an editor concern by nature, and the runtime half should register nothing. `RUNTIME-DRIVE-DESIGN`
 * §17 P0 has the reasoning; what told us so was trying to cut the dependency and finding out what it
 * was for.
 */
public class GraphCodePlayEditor : ModuleRules
{
	public GraphCodePlayEditor(ReadOnlyTargetRules Target) : base(Target)
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
				// PUBLIC since `GraphCodePlayPie.h`, which is a public header carrying a session
				// spec: pressing Play from a tool can open the run in the same act (§13), so the
				// request struct names `FGcPlaySliceSpec` and a caller cannot compile without it.
				// Private would compile HERE and fail in whichever module included the header.
				"GraphCodePlay",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Core's editor module, for the product registry and nothing else. This is the
				// dependency `GraphCodePlay` used to carry, moved to the module that is allowed to
				// have it — Play -> Core is the permitted direction, and an Editor module may depend
				// on an Editor module.
				"GraphCode",
				// The capture spec loads a `.umap` and reads the editor world: `Editor.h` for
				// `GEditor`, `FileHelpers.h` for `FEditorFileUtils::LoadMap`. Both come through
				// UnrealEd, and both are the reason this spec is here rather than beside the writer
				// it exercises — R1 captures the EDITOR world, so its spec needs an editor.
				"UnrealEd",
				// `GraphCodeTestActors.h`, the one definition of `FindActor` that Core's specs and
				// this one share.
				"GraphCodeRuntime",
				// The execution-state gates BUILD a blackboard — a `UBlackboardData` with three
				// keys, initialised on a spawned probe — because the fixture map has no AI in it and
				// a spec that asserted against an empty world would pass without the feature.
				"AIModule",
				// **R5's gate BUILDS AND COMPILES A BLUEPRINT FUNCTION, for the same reason the R4
				// gate links the StateTree compiler: the subject reads something only the editor
				// can produce.** A native `UFUNCTION` has no bytecode and so no debug site, so the
				// call listener is invisible to every native fixture this suite owns however it is
				// labelled — it needs compiled Blueprint bytecode to listen to. `BlueprintGraph`
				// for `UK2Node_FunctionEntry` and `UEdGraphSchema_K2`, which is where a function's
				// parameters and its `Category` are authored; the compile itself is
				// `FKismetEditorUtilities`, already reachable through `UnrealEd`.
				//
				// **THE SUBJECT LINKS NEITHER**, which is the test of whether this belongs here: a
				// packaged game runs Blueprint functions it can never author.
				"BlueprintGraph",
				// The R7 gates BUILD a widget tree — a `UUserWidget` with a `UButton` in it, outered
				// to the world — because no fixture map has UI in it and a spec that asserted
				// against a world with no widgets would pass without the feature.
				"UMG",
			}
		);

		// **THE SAME GATE AS THE RUNTIME HALF'S, because a gate is per-MODULE.** The subject lives
		// in `GraphCodePlay` and its gates live here, so both have to ask — the define is not
		// inherited across the module boundary, and a spec file that included
		// `AbilitySystemComponent.h` on the strength of the other module's answer failed to find it.
		//
		// It resolves to the same value for the same reason: both ask the PROJECT DESCRIPTOR. So a
		// Core-only install compiles the subject out of one module and its gates out of the other,
		// and the suite is green rather than unbuildable.
		bool bAbilities = GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GameplayAbilities",
			"WITH_GRAPHCODE_ABILITIES", new string[] { "GameplayAbilities", "GameplayTags" });

		// **THE R4 GATE, AND IT LINKS ONE MODULE THE RUNTIME HALF DOES NOT: `StateTreeEditorModule`.**
		// The subject reads a COMPILED tree — `States` is baked, private, and written only by
		// `FStateTreeCompiler` — so a gate for it has to author one, and the only supported door to
		// the compiler is `UStateTreeEditingSubsystem::CompileStateTree`. That module is
		// `UncookedOnly`, which is exactly why the fixture belongs here and the subject does not:
		// a packaged game reads trees it can never compile.
		//
		// **`PropertyBindingUtils` IS THE FOURTH MODULE AND IT IS NOT OPTIONAL PADDING — the linker
		// asked for it.** `FStateTreeCompilerLog`'s message struct embeds an
		// `FPropertyBindingBindableStructDescriptor`, whose virtual destructor is exported from that
		// module, so merely DECLARING a compiler log leaves `DestructItems<FStateTreeCompilerLogMessage>`
		// unresolved. It is one of `StateTree.uplugin`'s own hard dependencies, so it is present
		// wherever `GameplayStateTree` is and the one gate still answers for all four.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GameplayStateTree",
			"WITH_GRAPHCODE_STATETREE",
			new string[] { "GameplayStateTreeModule", "StateTreeModule", "StateTreeEditorModule",
				"PropertyBindingUtils" });

		// **A `UCLASS` CANNOT BE GATED BY A PREPROCESSOR BLOCK, SO THE DIRECTORY IS THE GATE —
		// MEASURED IN UHT'S OWN SOURCE, NOT GUESSED.** `UhtHeaderFileParser` classifies every `#if`
		// it meets, and it knows exactly nine conditions: `CPP`, `0`, `1`, `WITH_EDITOR`,
		// `WITH_EDITORONLY_DATA`, `WITH_ENGINE`, `WITH_COREUOBJECT`, `WITH_HOT_RELOAD`, `WITH_TESTS`
		// and the two Verse ones. Anything else — `WITH_GRAPHCODE_ABILITIES` among them — becomes
		// `UhtCompilerDirective.Unrecognized`, and `IncludeCurrentCompilerDirective()` returns FALSE
		// for that: the block is skipped for code generation. A `UGameplayAbility` subclass written
		// behind our own gate would therefore get no generated code at all, and `GENERATED_BODY()`
		// would fail to compile in the very build the gate was ON for. The engine has no counter-
		// example: there is not one `UCLASS` inside a non-standard `#if` in the whole tree.
		//
		// `ConditionalAddModuleDirectory` is the supported way to say *these sources exist only in
		// some builds* — its own comment says "useful for NotForLicensees/NoRedist" — and the added
		// directory is a full module directory, so its `Private` folder joins the private include
		// paths and its headers reach UHT. The fixture ability therefore needs no `#if` of its own,
		// and a Core-only install never compiles it because it is never added.
		//
		// **THE TEST CONFIGURATION IS PART OF THE CONDITION AND NOT TIDINESS.** The fixture's tags
		// are `GraphCodePlayTestTags.h`, which is itself `WITH_GRAPHCODE_ABILITIES &&
		// WITH_DEV_AUTOMATION_TESTS` — so in a Shipping or Test build the tags are gone and the
		// fixture would not compile. It is excluded on exactly the condition that removes them.
		if (bAbilities
			&& Target.Configuration != UnrealTargetConfiguration.Shipping
			&& Target.Configuration != UnrealTargetConfiguration.Test)
		{
			// Built from `PluginDirectory` rather than by walking `..` out of `ModuleDirectory`, so
			// the path UBT stores as a module directory has no relative segments in it — several of
			// its own checks are `IsUnderDirectory` comparisons against exactly this value.
			ConditionalAddModuleDirectory(EpicGames.Core.DirectoryReference.Combine(
				new EpicGames.Core.DirectoryReference(PluginDirectory),
				"Optional", "GameplayAbilities"));
		}
	}
}



