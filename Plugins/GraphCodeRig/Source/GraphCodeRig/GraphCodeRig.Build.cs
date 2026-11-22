using UnrealBuildTool;

public class GraphCodeRig : ModuleRules
{
	public GraphCodeRig(ReadOnlyTargetRules Target) : base(Target)
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

		// **`keep_volume=`'S FAR SIDE — A DEFORMER GRAPH, AND THE ONE OPTIONAL PLUGIN THIS MODULE
		// GATES RATHER THAN LINKS.** Every other plugin above is either the project's own or
		// `EnabledByDefault` in the engine (ControlRig is), so naming it outright costs a customer
		// nothing. `DeformerGraph` is `EnabledByDefault: false`: a project may genuinely ship without
		// it, and a hard dependency would stop that project building at all.
		//
		// **THE GATE IS WHAT LETS THE FEATURE LIVE HERE INSTEAD OF IN A SECOND PRODUCT** (user-ruled
		// 2026-09-17, *"graphcode rig module"*). With the plugin off, `WITH_GRAPHCODE_DEFORMER=0`
		// compiles `GraphCodeRigDeformer.cpp` down to a refusal that names the plugin, the rig build
		// takes no name for a graph it cannot write, and the `omits: all@rig` stub is unaffected —
		// which was the only thing the old Core-Extension ruling protected. Groom's HairStrands work
		// sits behind exactly this call.
		//
		// `OptimusCore` owns `UOptimusDeformer`, its node graph, pins and links, and every data
		// interface the graph reads through. The editor module is NOT needed: nothing here opens a
		// graph, and `UOptimusDeformerFactory` is one line of `NewObject` plus a component binding.
		//
		// `ComputeFramework` is named because OptimusCore's own PUBLIC headers include it —
		// `OptimusDataType.h` reaches for `ComputeFramework/ShaderParamTypeDefinition.h` — so it is
		// a dependency of reading the API at all, not of anything this file calls. Found by a
		// missing-include error rather than by reading the module rules.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "DeformerGraph", "WITH_GRAPHCODE_DEFORMER",
			new string[] { "OptimusCore", "ComputeFramework" });

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// Core owns the product table and the `gcrig 1` header word. One way, always.
				"GraphCode",

				// `SplitTopLevel` and `ExtractBalanced` — the bracket and brace readers every other
				// GraphCode parser already uses. A second copy here would drift from them, and this
				// format's whole correction on 2026-09-15 was about not inventing a second grammar.
				"AuthoringCore",
				"Json",

				// THE DOORS ARE NAMED NOW THOUGH THE PARSER CALLS NONE OF THEM, for the reason Shape
				// named its four geometry modules at M0: a dependency discovered missing halfway
				// through the builder is a break in the middle of the work it blocks.
				//
				// `MeshBoneWeightFunctions.h` carries the whole rig half — `CopyBonesFromSkeleton`,
				// `GetAllBonesInfo` (which is how an existing rig reads BACK, ledger §7),
				// `SetVertexBoneWeights` and `ComputeSmoothBoneWeights`.
				"GeometryCore",
				"GeometryFramework",
				"DynamicMesh",
				"GeometryScriptingCore",

				// `CreateNewSkeletalMeshAssetFromMesh` and `…FromMeshLODs` are editor-only, because
				// creating a USkeletalMesh is. So is `USkeletonFactory`.
				"GeometryScriptingEditor",
				"UnrealEd",
				"AssetRegistry",
				// The diagnostic render gate reads gc_render's PNG back from disk. The picture,
				// rather than only its vertex buffer, is the contract this product ships.
				"ImageWrapper",

				// `FGeometryScriptBoneWeightProfile`'s default is `FSkeletalMeshAttributesShared::
				// DefaultSkinWeightProfileName`, which lives here — so the DEFAULT-CONSTRUCTED
				// argument to `ComputeSmoothBoneWeights` is what pulls this module in, not any call
				// of ours. Found by a linker error on a parameter nobody wrote.
				"SkeletalMeshDescription",

				// A second hinge added to an imported mesh re-points skin weights in each LOD's own
				// mesh description (`GraphCodeRigKeptSkinEdit.cpp`); `FBoneWeights` is AnimationCore's.
				"MeshDescription",
				// A morph's normals are written into the built mesh's description, beside its vertex-instance normals,
				// whose attribute name (`MeshAttribute::VertexInstance::Normal`) is exported here (GraphCodeRigMorphs.cpp).
				"StaticMeshDescription",
				"AnimationCore",
				"RenderCore",

				// `FReferenceSkeletonModifier` is ENGINE_API in `ReferenceSkeleton.h`, so the bone
				// hierarchy itself needs nothing beyond Engine. Named here is the IK half:
				// `kind:ik_rig`'s chains and `kind:ik_retargeter` already create, and this product
				// fills them from `joints { }`.
				"IKRig",

				// `UIKRigController` — the only way to fill an IK rig's retarget chains — lives in
				// the EDITOR half, and `AddRetargetChain(name, start bone, end bone, goal)` takes
				// exactly what a `chains { }` line already is.
				"IKRigEditor",

				// The engine's own full-body solver, run by one spec as the witness for which way a
				// written limit opens (`GraphCodeRigSolverLimitTests.cpp`).
				"PBIK",

				// `FPhysicsAssetUtils::CreateFromSkeletalMesh`, which makes a body per bone from the
				// mesh's own vertices. Its `bShowProgress` defaults TRUE and must not, here.
				"PhysicsUtilities",

				// `FMeshUtilitiesEngine::CalcBoneVertInfos`, which the engine's one-bone body door
				// (`CreateCollisionFromBone`) takes beside the bone: a hull body that stands proud of the
				// skin is rebuilt in pieces through it (`GraphCodeRigHullFit.cpp`).
				"MeshUtilitiesCommon",
				"MeshUtilitiesEngine",

				// THE VOLUME DRIVER'S DOORS, named here for the same reason the geometry ones above
				// were named at M0 (ledger §9, user-ruled IN 2026-09-17). `bulge=` builds the helper
				// bone and nothing moves it; a post-process Animation Blueprint is where a mover
				// lives, because `USkeletalMesh::PostProcessAnimBlueprint` is a
				// `TSubclassOf<UAnimInstance>` and a Control Rig is not one — so BOTH routes on the
				// table need a generated anim graph and these two modules with it.
				//
				// `AnimGraphRuntime` carries `FAnimNode_BoneDrivenController`, the node that maps a
				// driver bone's fold onto a driven bone's translation; `AnimGraph` carries its editor
				// node, the AnimGraph schema and the input-pose node a post-process graph reads
				// through.
				"AnimGraph",
				"AnimGraphRuntime",

				// **THE OTHER ROUTE'S DOORS, NAMED FOR THE PROBE THAT DECIDES BETWEEN THEM.** A bone
				// driven controller reads one Euler component and an `FRotator`'s Pitch turns back
				// at ninety degrees, so two of four ordinary limbs cannot be driven to a knee's 145
				// (ledger §12.6). Control Rig works in quaternion space and has no such ceiling —
				// and it need not be hand-authored: `UControlRigBlueprintFactory` and
				// `URigVMController::AddUnitNode` are both `BlueprintCallable`, which is how Epic's
				// own Python builds rigs.
				"ControlRig",
				"ControlRigDeveloper",
				"ControlRigEditor",
				"RigVM",
				"RigVMDeveloper",

				// **`kind:control_rig`'S READER, AND THE REASON IT IS NAMED HERE RATHER THAN IN
				// CORE.** Core hard-links `MaterialIR`, which is safe because MaterialIR needs only
				// always-present engine modules; `ControlRigIR` needs the OPTIONAL ControlRig plugin
				// above, and Core linking that would break the one thing the product model exists
				// for — a build without the plugin still has to classify a `UControlRigBlueprint`
				// and write an honest `omits: all@rig` stub for it. This module already depends on
				// all five ControlRig/RigVM modules, so the dependency costs nothing new here.
				"ControlRigIR",
			}
		);
	}
}



