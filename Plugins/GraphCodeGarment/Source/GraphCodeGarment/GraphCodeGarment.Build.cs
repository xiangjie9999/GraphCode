using UnrealBuildTool;

public class GraphCodeGarment : ModuleRules
{
	public GraphCodeGarment(ReadOnlyTargetRules Target) : base(Target)
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
				// Core owns the product table and the `gcgarment 1` header word. One way, always.
				"GraphCode",
				// `SplitTopLevel` and `ExtractBalanced`, the bracket readers every GraphCode parser uses.
				"AuthoringCore",

				// The body is read and the garment written as a dynamic mesh, as Shape and Rig do.
				// Not Control Rig and not the rig plugin: a garment makes no bones, and one built
				// `[as=skeletal]` takes the body's bones and weights through GeometryScript. Native cloth
				// is a separate optional dependency below.
				"GeometryCore",
				"GeometryFramework",
				"GeometryAlgorithms",
				"DynamicMesh",
				"GeometryScriptingCore",
				"GeometryScriptingEditor",
				// The skin-weight profile name `[as=skeletal]`'s weight transfer defaults to.
				"SkeletalMeshDescription",
				// Eigen. The engine's own copy, as GraphCode and GraphCodeShape use it: a block flattened onto the plane (GraphCodeGarmentPiece.cpp
				// FlattenBlock) relaxes with one sparse Cholesky factorization.
				"Eigen",
				"UnrealEd",
				"AssetRegistry",
				// GraphCodeGarment's own materials are made in the project from recipes the plugin ships (GC-GARMENT-STITCH.md §5).
				"Projects",
				// The research controls a clamp's row is checked against are JSON (GraphCodeGarmentClampTests.cpp).
				"Json",
			}
		);

		// A part a garment places (`add($Buckle.Buckle, at=...)`) is built by Shape from its own
		// `.gcshape`. Optional (user, 2026-09-26): without Shape every cloth verb still builds, and a
		// part is refused naming the plugin that is off.
		// Delay-loaded, and each call site asks first (GcGarmentShapeOn): a hard link made Garment unloadable in a
		// project without Shape (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md).
		if (GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GraphCodeShape", "WITH_GRAPHCODE_SHAPE",
			new string[] { "GraphCodeShape" }))
		{
			GraphCodeBuildGates.DelayLoad(this, Target, "GraphCodeShape");
		}

		// Every Chaos cloth asset a garment makes — `[as=outfit]`'s, `[as=cloth]`'s, a knot's settle — is optional; ordinary
		// construction needs no Chaos. `ClothingSystemRuntimeCommon` binds a drawn garment to its simulation (GC-GARMENT-CLOTH.md).
		// The plugin-owned Chaos modules are delay-loaded; GcGarmentChaosOn asks before every use, so a project with
		// Chaos Cloth Asset off still opens Garment and its cloth verbs refuse by naming the plugin.
		if (GraphCodeBuildGates.AddOptionalPlugin(this, Target, "ChaosClothAsset", "WITH_GRAPHCODE_GARMENT_CHAOS",
			new string[] { "ChaosClothAsset", "ChaosClothAssetEngine", "ChaosCloth", "Chaos", "ChaosCore", "ClothingSystemRuntimeInterface",
				"ClothingSystemRuntimeCommon", "ChaosCaching", "GeometryCache" }))
		{
			GraphCodeBuildGates.DelayLoad(this, Target, "ChaosClothAsset", "ChaosClothAssetEngine", "ChaosCloth", "ChaosCaching");
		}
	}
}


