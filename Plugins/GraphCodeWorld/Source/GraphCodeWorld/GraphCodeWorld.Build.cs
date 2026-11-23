using UnrealBuildTool;

public class GraphCodeWorld : ModuleRules
{
	public GraphCodeWorld(ReadOnlyTargetRules Target) : base(Target)
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
				"EditorSubsystem",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				// The dependency runs ONE WAY and must keep doing so. Core knows the NAME of every
				// kind in all five products so that it can refuse one by name; it must never need a
				// product's CODE to do that, or a Core-only build cannot be built at all.
				"GraphCode",
				// The shared actor-entry grammar, for `Indent` alone — L0a. A landscape is an ACTOR,
				// so what this module contributes is a directive line inside the entry Core already
				// writes, at the depth Core hands it. Reproducing "two spaces per level" here would be
				// a second carrier of the file's indentation ([[two-carriers-of-one-fact-is-a-race]]).
				"GraphCodeRuntime",
				"AuthoringCore",
				"NavigationSystem",
				// `GC-TERRAIN-DESIGN.md` L1/L3 — realizing a `terrain { }` program. Core owns the program
				// and evaluates it in centimetres, knowing nothing about landscapes; this module is where
				// those numbers meet an actor, because `ALandscape`, the edit-layer stack and
				// `FLandscapeEditDataInterface` all live here. Named rather than taken through UnrealEd:
				// `LANDSCAPE_ZSCALE` and `LandscapeDataAccess::MidValue` are the single carrier of what one
				// stored step is worth, and mirroring them into a constant of our own is exactly the race
				// [[two-carriers-of-one-fact-is-a-race]] is about.
				"Landscape",
				// Row G4 — painted foliage is PLACEMENT and placement is World's (user-ruled
				// 2026-08-20). Needed for two component CLASSES and nothing else:
				// UFoliageInstancedStaticMeshComponent and UGrassInstancedStaticMeshComponent,
				// which the facet names so a level stops writing every painted transform.
				// Unlike PCG directly below, this is a RUNTIME ENGINE MODULE that is always
				// present, so it raises none of the enabled-by-default question that one does.
				"Foliage",
				"UnrealEd",
				"BSPUtils",
				// `except=` cuts one region out of another, and the cut is a winding-number fill over a
				// constrained triangulation (`GraphCodeWorldRegionPrism.cpp`). `FGeomTools2D` was tried
				// first and MEASURED insufficient: its ReducePolygons only subtracts a ring that lies
				// wholly inside another, so a path crossing a forest's edge was rejected outright.
				"GeometryCore",
				// `kind:pcg_graph`'s greenfield factory announces the asset it just built. Named
				// rather than taken transitively through UnrealEd: a dependency this module's own
				// code calls is one this module's own file should state.
				"AssetRegistry",

			}
		);

	}
}



