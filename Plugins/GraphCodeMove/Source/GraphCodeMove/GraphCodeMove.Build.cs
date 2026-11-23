using UnrealBuildTool;

public class GraphCodeMove : ModuleRules
{
	public GraphCodeMove(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		// Native units are discovered by reflection; UE 5.7's physics unit header includes a private header.
		DynamicallyLoadedModuleNames.Add("ControlRigPhysics");

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
				"GraphCode",
				"ACLPlugin",
				"AssetRegistry",
				"PhysicsUtilities",
				"MeshUtilitiesEngine",
				"MeshUtilitiesCommon",

				// A research request's input file (`seat_override`, GraphCodeMoveProgramBuild.cpp) is JSON, read as Core reads
				// the request itself (GraphCodeResearchCapture.cpp); Core already links it, so the product gains no module.
				"Json",

				// The graph reader and its serializer, which are GENERIC — `FGraphReader::Read`
				// walks the engine's own `GetSubGraphs()` virtual and has no anim-specific line in
				// it, which is why an AnimGraph's state machines already project three levels deep.
				// What is animation-specific and stays in Core is `IsAnimationGraphSchema`: it is
				// also what keeps an AnimGraph OUT of `functions { }`, so moving it here would ship
				// `functions { }`'s removal-by-omission back to every Core-only customer as silent
				// data loss (truth-audit §6 Q1).
				"BlueprintIR",

				// `animations { }` on a Widget Blueprint. UMG holds UWidgetAnimation and its
				// bindings, UMGEditor holds UWidgetBlueprint itself, and the two MovieScene modules
				// hold what a widget animation IS: MovieScene for the sequence, its bindings and the
				// channel proxy this codec is built on, MovieSceneTracks for the concrete property
				// tracks a UMG animation is made of.
				//
				// NOT WidgetIR. The codec reads the ENGINE's objects, not Core's projection of them,
				// so the one-way dependency above is unchanged and a widget animation block needs no
				// widget document to exist.
				"UMG",
				"UMGEditor",
				"MovieScene",
				"MovieSceneTracks",
				"MovieSceneTools",

				// `kind:level_sequence`'s host. A ULevelSequence is a UMovieSceneSequence like the
				// widget animation above, so the three modules already listed carry its interior;
				// what this adds is the ASSET - its possessables, and the director Blueprint its
				// event endpoints live on. LevelSequence is an engine RUNTIME module and not an
				// optional plugin, so this needs no build gate.
				"LevelSequence",

				// A skeletal animation clip's `swapRootBone=` - `ESwapRootBone` is declared HERE, and
				// MovieSceneTracks only forward-declares it, so naming its entries means stating this.
				"AnimGraphRuntime",

				// A binding reference's `FUniversalObjectLocator` - the Level Sequence specs resolve one
				// against a spawnable's template, which is what proves a component binding really binds.
				"UniversalObjectLocator",

				// A spawnable's TEMPLATE is an actor, and `kind:level` already writes actors: the
				// binding declaration reads one through `FGraphCodeActorEntryWriter` and imports it
				// under that writer's own baselines and policies, so the two spellings of an actor in
				// this language cannot drift. GraphCodeRuntime is in the GraphCode plugin already
				// listed, so the one-way rule above is unchanged.
				"GraphCodeRuntime",

				// The value grammar and the shared line collector. GraphCode re-exports its types
				// through GraphCodeSchemaWriter.h, which is why this compiled without it and did not
				// LINK: a header is visible through a transitive include, a symbol is not.
				// The event channel's endpoint. `FMovieSceneEvent::WeakEndpoint` is a UObject pointer
				// and the NAME a person gave it is on the node type - `CustomFunctionName` on a custom
				// event, the graph's name on a function entry - so naming an endpoint means naming
				// those two classes, which live here.
				"BlueprintGraph",

				"AuthoringCore",

				// `kind:behavior_tree`. AIModule owns the runtime node instances, AIGraph owns
				// SubNodes, and BehaviorTreeEditor owns the authored BTGraph and its editor nodes.
				"AIModule",
				"AIGraph",
				"BehaviorTreeEditor",

				// `kind:ik_rig` and `kind:ik_retargeter` (`GC-MOVE-ANIMATION-DESIGN.md` §4.0b).
				// **THE PRODUCT NAMES THESE TYPES AND CORE DOES NOT** — Core classifies both by
				// literal class path (`GcIkRigClassPath`) precisely so that this dependency stops
				// here, at the plugin that reads them, instead of reaching a Core-only build.
				//
				// The IKRig plugin is `EnabledByDefault: true`, so it is present without a
				// `.uproject` entry; it is declared in `GraphCodeMove.uplugin` anyway, because a
				// dependency that happens to be satisfied is not a dependency that is stated.
				"IKRig",
				"FullBodyIK", // Joint-space contact solve includes pad position and facing together.

				// **ADDED 2026-08-16, AND IT REVERSES A DECISION THIS PLUGIN MADE ON PURPOSE.**
				// `GraphCodeMoveIkRig.h` records that the retargeter's blocks are read and written
				// through the asset's own accessors "instead of taking an IKRigEditor dependency for
				// its controller" — right for reading and writing FIELDS, which is all that half does.
				//
				// It is wrong for two acts that are not field writes. `UIKRetargetFactory` is what
				// builds a retargeter the way the editor builds one (six default ops, not zero), and
				// `UIKRetargeterController::SetIKRig` is what makes an assigned rig REACH the op stack
				// — it rebuilds each op's FRetargetChainMapping, which no reflective write to
				// `SourceIKRigAsset` can do. Both live here and neither has a runtime equivalent.
				"IKRigEditor",

				// constraints() emits stock Control Rig units; no GraphCode runtime dependency.
				"ControlRig",
				"PhysicsControl",
				"ControlRigDeveloper",
				"ControlRigEditor",
				"RigVM",
				"RigVMDeveloper",
				// GraphCode Rig's reader, for a constraint rig's digest. Delay-loaded below and asked for first
				// (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): Move must load in a project without GraphCode Rig.
				"ControlRigIR",

				// `kind:niagara_system` (`GC-MOVE-NIAGARA-DESIGN.md`). Niagara owns the runtime
				// UNiagaraSystem/Emitter/Script objects and their properties; NiagaraEditor owns the
				// authored stack — UNiagaraScriptSource, the UNiagaraGraph, and the
				// UNiagaraNodeFunctionCall modules the projection reads. Core stays free of both: it
				// classifies the asset by literal class path, the same arrangement the IK kinds use.
				"Niagara",
				"NiagaraCore",
				"NiagaraEditor",

				// `kind:state_tree` (`GC-STATE-TREE-DESIGN.md`). StateTreeModule owns the runtime
				// UStateTree and its node structs; StateTreeEditorModule owns the authored
				// UStateTreeEditorData, UStateTreeState and the FStateTreeEditorNode this projection
				// reads. Core classifies a UStateTree by literal class path (`GcStateTreeClassPath`),
				// the same arrangement the IK and Niagara kinds use, so the dependency stops here.
				"StateTreeModule",
				"StateTreeEditorModule",

				// **THE BINDING PATH IS NOT StateTree's — it moved out from under it.** A
				// `FStateTreePropertyPathBinding` holds two `FPropertyBindingPath`s, and both the
				// path's `ToString`/`FromString` and the collection's `AddBinding` live in this
				// plugin. Reading and writing `bind D = S` lines links against it, so it is stated
				// rather than left to arrive transitively through StateTreeModule's own include.
				"PropertyBindingUtils",

				// `FBlueprintEditorUtils` and `FKismetEditorUtilities`. The fixture commandlet
				// (`Private/Tests/…WidgetAnimFixtureCommandlet.cpp`) needs them because a fixture has
				// to be COMPILED before it is saved or the generated class does not carry the
				// animations the asset holds — and as of the animation LIFECYCLE (2026-08-12) the
				// codec needs them too: an animation is a variable on the generated class, so
				// creating, renaming or deleting one has to move the graphs that name it
				// (`ReplaceVariableReferences`) and tell the class its variable set changed.
				"UnrealEd",
				"PhysicsUtilities",
				"MeshDescription",
				"StaticMeshDescription",

				// A plant's sole support (GraphCodeMovePlantSupport.cpp) is PolygonsUnion/PolygonsDifference over FGeneralPolygon2d; the GeometryProcessing plugin carries both.
				"GeometryCore",
				"GeometryAlgorithms",

				// OBJ1 S6: a released prop's centre of mass (GcMoveCollisionProgram::PropCentre) asks its body setup for its collision complexity through
				// the engine's own UBodySetupCore::GetCollisionTraceFlag (PHYSICSCORE_API), which maps "use default" to the project's setting. It compiled
				// without this through a transitive header and did not LINK (OBJ1m, 2026-10-11); the core module lists it for the same reason.
				"PhysicsCore",

				// MOVE-PARAM1 SLICE 0 (b), the GraphCode Pose tab (Docs/GC-MOVE-POSE-PANEL-INTEGRATION.md; the owner, 17 Nov: "we should be able to have
				// the dedicated viewport for it"): its own viewport (SEditorViewport, UnrealEd) on its own preview world (FAdvancedPreviewScene), drawn
				// with Slate. In this module and not a new one because the window's build, gate and publish route this module only.
				"Slate",
				"SlateCore",
				"InputCore",
				"AdvancedPreviewScene",
				// The panel's Apply and Save go through the agents' build queue: the editor's own MCP task endpoint, over local HTTP.
				"HTTP",
				// The pose panel's Copy and Paste use the system clipboard (FPlatformApplicationMisc::ClipboardCopy/Paste; the owner, 17 Nov: "right click
				// to copy").
				"ApplicationCore",
				// RULING 106 SLICE 5 (Saved/Research/MOVE-PARAM1/CODER-NOTES-13.md): the Pose tab's Upload pictures and the Train window's Python pick files with the
				// editor's own file dialog, and the Train window shows the held-out pictures the route drew (FImageUtils::LoadImage into an FImage).
				"DesktopPlatform",
				"ImageCore",
			}
		);

		// The GraphCode Pose tab sits in the Window menu's Level Editor section (one entry; an ungrouped tab is listed twice, the owner, 17 Nov:
		// "there's two GraphCode pose buttons"). WorkspaceMenu::GetMenuStructure() is inline over the module's interface: its header only, no link.
		PrivateIncludePathModuleNames.Add("WorkspaceMenuStructure");

		// **THE METAHUMAN GATE LEFT THIS FILE 2026-09-15, USER-RULED.** It landed here because the
		// SPIKE landed here, and that is provenance rather than ownership: `MetaHumanCharacter` is
		// an optional engine plugin, which is what the ADVANCED product is for, so the gate, the
		// spike and `kind:metahuman`'s codec now live in `GraphCodeCoreExtension` beside MVVM and
		// MetaSound. Nothing in Move references a MetaHuman type, so this module drops three module
		// dependencies and a define with it.

		GraphCodeBuildGates.DelayLoad(this, Target, "ControlRigIR");
	}
}


