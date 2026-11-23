// Copyright Natali Caggiano. All Rights Reserved.
//
// DERIVED FROM UnrealClaude (https://github.com/Natfii/UnrealClaude), MIT, (c) 2025 Natali
// Caggiano. See THIRD-PARTY-NOTICES.md at this plugin's root. The module was renamed from
// `UnrealClaude` to `GraphCodeMCP` on 2026-08-27 when it moved into this plugin and its in-editor
// chat UI was dropped; `ToolMenus` and `WorkspaceMenuStructure` left with that UI, since a module
// with no menu entry and no nomad tab needs neither.
using UnrealBuildTool;

public class GraphCodeMCP : ModuleRules
{
	public GraphCodeMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// GraphCode Play carries the world capture (`export_world_snapshot`), and it is the first
		// GraphCode PRODUCT this module links rather than merely names. Optional for the reason
		// GC-PRODUCT-PACKAGING-DESIGN gives: a Core-only customer has no GraphCodePlay to link, and
		// a hard dependency would turn "that product is not installed" from a refusal this tool can
		// word into a build that does not complete.
		// `GraphCodePlayEditor` came with `gcp_run mode:"start"` pressing Play: `RequestPlaySession`
		// is `GEditor`, and Play's runtime half may not name it. Both modules ride the ONE gate,
		// because they arrive and leave together — a build that had the runtime half and not the
		// editor half would compile every refusal and none of the acts.
		// Delay-loaded (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): the gate above decides from the BUILDING project,
		// which has Play, so a hard link made GraphCode Core unloadable in a Core-only project. Every Play call
		// sits behind `FGraphCodeProducts::IsInstalled(EGcProduct::Play)`, which is false when Play is off.
		if (GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GraphCodePlay", "WITH_GRAPHCODE_PLAY",
			new string[] { "GraphCodePlay", "GraphCodePlayEditor" }))
		{
			GraphCodeBuildGates.DelayLoad(this, Target, "GraphCodePlay", "GraphCodePlayEditor");
		}
		
		PublicIncludePaths.AddRange(
			new string[] {
			}
		);
				
		PrivateIncludePaths.AddRange(
			new string[] {
			}
		);
			
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Slate",
				"SlateCore",
				"EditorStyle",
				"UnrealEd",
				"Projects",
				"EditorFramework"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Project Settings -> Plugins -> Unreal Claude (UUnrealClaudeSettings)
				"DeveloperSettings",
				"Json",
				"JsonUtilities",
				"AudioMixer",
				"AuthoringCore",
				"RHI",
				"HTTP",
				"HTTPServer",
				"Sockets",
				"Networking",
				"ImageWrapper",
				// Blueprint manipulation
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"GraphEditor",
				"UMG",
				"UMGEditor",
				// `gc_open kind:"widget"` runs an Editor Utility Widget, and the two types that
				// takes — UEditorUtilityWidgetBlueprint and UEditorUtilitySubsystem — live here.
				// An engine EDITOR module, not a plugin, so this is a hard dependency like UMGEditor
				// rather than one of GraphCodeBuildGates' optional ones.
				"Blutility",
				"MaterialEditor",
				"AssetRegistry",
				"AssetTools",
				// Animation Blueprint manipulation
				"AnimGraph",
				"AnimGraphRuntime",
				// Asset saving
				"EditorScriptingUtilities",
				// Enhanced Input
				"EnhancedInput",

				// Blueprint IR core. Dependency is one-directional: BlueprintIR knows
				// nothing about UnrealClaude, so it survives a bridge migration intact.
				"BlueprintIR",

				// Widget Blueprint Designer-state IR. Same one-way adapter boundary.
				"WidgetIR",

				// The .gc text sidecar. Same direction again: GraphCode's save path calls no
				// HTTP and knows no MCP type, so the sidecars keep working with this off.
				"GraphCode",

				// Material IR, for the parameters{} grammar. The adapter needs the block PARSER,
				// which lives beside the renderer so the two cannot drift; taking the dependency
				// is what stops a second parser being written here against the same text.
				"MaterialIR",

				// `gcw_ask mesh:` names WHICH navmesh answered, and the name is on the nav data's
				// own config. Everything else navigation-shaped here goes through GraphCode's
				// FGraphCodeNavigation; this is the one engine type read directly.
				"NavigationSystem"
			}
		);

		// Clipboard support (FPlatformApplicationMisc) on all platforms
		PrivateDependencyModuleNames.Add("ApplicationCore");

		// Windows only
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// LiveCoding is only available in editor builds on Windows
			if (Target.bBuildEditor)
			{
				PrivateDependencyModuleNames.Add("LiveCoding");
			}
		}
	}
}



