using UnrealBuildTool;

public class GraphCode : ModuleRules
{
	public GraphCode(ReadOnlyTargetRules Target) : base(Target)
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
				"DeveloperSettings",
				// GraphCodeSchemaWriter.h exposes AuthoringCore::FAuthoringRename in the
				// public write request, so every consumer of GraphCode needs this include path.
				"AuthoringCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// The object grammar with no editor under it — the actor entry writer and the two
				// projections, which seventeen files in this module include and which GraphCode Play
				// needs from a Runtime module. PRIVATE because every one of those includers is under
				// Private/: nothing in this module's public interface names those types, so a
				// consumer of GraphCode does not inherit the include path.
				"GraphCodeRuntime",
				"AssetRegistry",
				// E11.7a. IAssetTools::RenameAssets is how a recoverable removal MOVES a package
				// rather than deleting it, and AssetTools.CreateAsset is the greenfield factory
				// runner. Both come through UnrealEd's headers and neither symbol does, which is
				// the confusing half of this dependency and the reason it is named here.
				"AssetTools",
				"Json",
				// U3's update check (Docs/GC-UPDATES-U3-PLAN.md): two POSTs to the release service.
				"HTTP",
				"InputCore",
				"Projects",
				"UnrealEd",
				// N0's brush rebuild (`GC-NAVIGATION-DESIGN.md` §7). `FBSPOps::csgPrepMovingBrush` is
				// what `UActorFactory::CreateBrushForVolumeActor` calls after building a volume's
				// model, and it lives in the BSPUtils DEVELOPER module rather than in UnrealEd —
				// which is the surprising half of this dependency and the reason it is named here.
				"BSPUtils",
				// MB51's material bake (`GC-MATERIAL-BAKE-DESIGN.md`). Both are DEVELOPER modules
				// and editor-only, which is why this service is Core's and never a runtime one.
				// `MaterialBaking` renders a material's properties to pixels — and the atlas comes
				// through custom destination UVs packed globally on the CPU; the unit
				// `FMeshData::TextureCoordinateBox` preserves that layout. `MaterialUtilities`
				// then mints the textures and the single proxy material under names WE choose,
				// which is what the deterministic `M_<target>_Baked` family needs and what the
				// merge path could not give (§4.1).
				"MaterialBaking",
				"MaterialUtilities",
				// The bake reads and rewrites LOD0's UVs, so it needs the description types
				// themselves — `FMeshDescription` and `FStaticMeshAttributes` live in these two and
				// not in the baking modules that consume them.
				"MeshDescription",
				"StaticMeshDescription",
				// Product-neutral source-mesh reading and hashing. MeshConversion converts the authored
				// MeshDescription directly, so Core does not acquire the GeometryScripting plugin merely
				// because Shape and Advanced both need the same mesh identity seam.
				"GeometryCore",
				"DynamicMesh",
				"MeshConversion",
				// The body-collision bench's dense test hands (GC-COLLISION-SPEED.md, step 0): the engine's own
				// skeletal LOD removal. Removing a source model alone leaves its LOD info and imported model, and
				// the next build asserts on the gap. A DEVELOPER module, editor-only, as this module is.
				"SkeletalMeshUtilitiesCommon",
				// GC Vision (`GC-VISION-DESIGN.md`). The scene render needs the render-target and
				// view types; ImageWrapper encodes the PNG, the same pair ui_render uses. RHI is
				// for `GMaxRHIFeatureLevel`/`GMaxRHIShaderPlatform` — the capture has to name the
				// platform it is asking a material whether it has finished compiling for.
				"RenderCore",
				"RHI",
				"ImageWrapper",
				// A6 (2026-10-29): `GraphCodeRender::BesideReference` composes a render and a reference picture as FImages.
				"ImageCore",
				"BlueprintGraph",
				// §11.1b. FCompilerResultsLog::GetBlueprintMessageLog is UNREALED_API and returns
				// a TSharedRef<IMessageLogListing> — the header for that interface is this module
				// and nothing else brings it, so the call compiles nowhere without it.
				"MessageLog",
				"UMGEditor",
				// UMG itself, for C6a's probe: UMGEditor brings the HEADERS transitively, so
				// UUserWidget/UButton/UTextBlock compile without it and fail at the LINK - which is
				// the confusing half of this dependency and the reason it is named rather than left
				// to whoever hits it next. Runtime module, so it does not affect the eventual
				// Runtime retype of this plugin's lower half (GC-PRODUCT-SPLIT.md).
				"UMG",
				// E6's notifications. Slate/SlateCore raise the toast; Blutility is only for the
				// "Open GraphCode" button, which spawns an Editor Utility Widget tab. Both are
				// editor-only and neither is reachable from the projection or merge paths, so the
				// eventual Runtime retype of this plugin's lower half is unaffected
				// (GC-PRODUCT-SPLIT.md).
				"Slate",
				"SlateCore",
				"Blutility",
				// E6.1's Tools-menu entry. See GraphCodeMenu.h for why that entry is native rather
				// than the Blueprint UToolMenuEntryScript the design first called for.
				"ToolMenus",
				// E9.5's nomad tab. WorkspaceMenuStructure is what puts it in the Window menu's
				// Tools category; without it the tab exists and nothing lists it. LevelEditor is
				// for the layout extension that gives the tab a default berth bottom-right —
				// without one a nomad tab opens in a floating window over the editor.
				"WorkspaceMenuStructure",
				"LevelEditor",
				// E10.4's button in every asset editor's status bar. FToolkitManager is how the
				// status bar name is reached from the asset without downcasting an
				// `IAssetEditorInstance*`, and it is EDITORFRAMEWORK_API — so the header comes
				// through UnrealEd and the SYMBOL does not.
				"EditorFramework",
				// The bottom drawer. StatusBar carries UStatusBarSubsystem; ToolWidgets carries
				// FWidgetDrawerConfig, which StatusBar's own headers only forward-declare.
				"StatusBar",
				"ToolWidgets",
				"BlueprintIR",
				"MaterialIR",
				"WidgetIR",
				// The two-step import (GC-IMPORT-DESIGN.md). InterchangeCore carries
				// UInterchangePipelineBase; InterchangeEngine the manager, the project settings and
				// the pipeline stack. Both are Runtime modules, so neither blocks the eventual
				// Runtime retype of this plugin's lower half (GC-PRODUCT-SPLIT.md).
				"InterchangeCore",
				"InterchangeEngine",
				// Z4's `walkable=` (GC-SPATIAL-AUTHORING-DESIGN.md §20.19) and N1's build and
				// reachability queries (GC-NAVIGATION-DESIGN.md §7). Both go through
				// `FGraphCodeNavigation` — the sampling stays in the pure resolver, and `walkable=`
				// reads the answer through the same code the build writes it with. A SECOND
				// includer arrived 2026-08-17: `GraphCodeRenderHighlight.cpp` gathers the navmesh
				// SURFACE through `ARecastNavMesh::GetDebugGeometryForTile` for §4.3b's overlay,
				// which is a picture rather than a query and so does not belong behind the resolver.
				// Runtime module, so the eventual Runtime retype is unaffected.
				"NavigationSystem",
				// R5b's `from:` promotion (GC-SPATIAL-AUTHORING-DESIGN.md §21.8a). The collision
				// ELEMENTS come through Engine, and `UBodySetupCore::GetCollisionTraceFlag` does
				// not - it is PHYSICSCORE_API, so the header compiles without this and the link
				// fails, which is the confusing half of this dependency and why it is named here.
				// The accessor rather than the field because `CTF_UseDefault` means *whatever the
				// project setting says*, and a footprint traced from an aggregate the physics world
				// ignores is a wrong answer nothing would report. Runtime module, so the eventual
				// Runtime retype of this plugin's lower half is unaffected.
				"PhysicsCore",
				"Chaos", // Native body overlap queries shared by Rig and Move authoring.
				"ChaosCore", // AABB and capsule query symbols used by the Chaos templates.
				// `GraphCodeEditorCall`'s answer to *what could a person press here*.
				// `PropertyCustomizationHelpers::GetCallInEditorFunctionsForClass` is the SAME call the
				// details panel makes to build its CallInEditor buttons, so asking it is the only way
				// that list cannot drift from what the editor actually shows — a hand-written filter
				// would be a second opinion about `CanCallFunctionBasedOnParams`. Editor module, and
				// this module is already Editor-only.
				"PropertyEditor",
				// Tight-Inclusion's certified swept contact (`GraphCodeContact`, Docs/GC-CONTACT.md) is written over
				// Eigen. The engine's own copy, as GraphCodeShape uses it; nothing of Eigen is vendored.
				"Eigen",
				// N15 (GC-MOVE-NIAGARA-FIELD-1116-PLAN.md): a Niagara Fluids system is rendered in the SAME scene the Niagara
				// editor viewport builds (`SNiagaraSystemViewport.cpp:848-854`): this engine class, so its lit sky, lights
				// and post-process come from the user's asset viewer profile rather than from constants copied here.
				// Editor module, as this module is.
				"AdvancedPreviewScene",
			}
		);

		// Tight-Inclusion includes itself as <tight_inclusion/...>; its source is vendored under Private/ThirdParty
		// (README there: the pinned commit, what is unchanged, and GraphCode's three stand-in headers).
		PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Private", "ThirdParty", "TightInclusion"));

		// §11.6's MP4 (`GraphCodeRenderMovie.cpp`). Windows Media Foundation already contains an
		// H.264 encoder and an MP4 muxer, so `video:"mp4"` costs THREE SYSTEM DLLS and nothing else:
		// no `ffmpeg`, no `ThirdParty/` binary, no licence to carry in a plugin that ships on Fab.
		// This is the same list the engine's own `MovieRenderPipelineMP4Encoder` declares.
		//
		// DELAY-LOADED on purpose. A Windows "N" edition ships without the Media Feature Pack, and a
		// hard import would make the whole editor fail to start there rather than making one optional
		// output format unavailable. `MFStartup` returning a failure is the path that machine takes,
		// and the capture falls back to an animated PNG on it.
		if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
		{
			PublicDelayLoadDLLs.Add("mfplat.dll");
			PublicDelayLoadDLLs.Add("mfreadwrite.dll");
			PublicDelayLoadDLLs.Add("mfuuid.dll");

			PublicSystemLibraries.Add("mfplat.lib");
			PublicSystemLibraries.Add("mfreadwrite.lib");
			PublicSystemLibraries.Add("mfuuid.lib");

			// Signed work (Docs/GC-LICENSING-SEALS-PLAN.md S3-S4): SHA-256 and P-256 checks through
			// BCrypt, this PC's key through NCrypt (TPM or software provider). Both ship with Windows.
			PublicSystemLibraries.Add("bcrypt.lib");
			PublicSystemLibraries.Add("ncrypt.lib");
		}
	}
}



