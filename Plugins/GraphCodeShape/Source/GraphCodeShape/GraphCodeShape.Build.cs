using UnrealBuildTool;

public class GraphCodeShape : ModuleRules
{
	public GraphCodeShape(ReadOnlyTargetRules Target) : base(Target)
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
				// Core owns the product table, the `gcshape 1` header word and the seam this module
				// registers through. Nothing else of this product is Core's.
				"GraphCode",

				// THE FOUR GEOMETRY MODULES ARE NAMED AT M0 DELIBERATELY, though M0 calls none of
				// them (`GC-SHAPE-AUTHORING-DESIGN.md` §15). `GC-SHAPE-GRAMMAR-LEDGER.md` §11 prices
				// every instruction against a door in one of them, and a dependency discovered
				// missing halfway through M4 is a build break in the middle of the work it blocks
				// rather than at the start of it.
				//
				// GeometryAlgorithms is the one that would have been missed: it carries
				// `PolygonsUnion`/`PolygonsDifference`/`PolygonsIntersection` over
				// `FGeneralPolygon2d`, which is how group 5 works inside a `plane()` body — §7.1
				// case A, resolved as BUILD IT.
				"GeometryCore",
				"GeometryFramework",
				"GeometryAlgorithms",
				"GeometryScriptingCore",
				"PhysicsCore",

				// `DynamicMesh` carries `FDynamicMesh3`, the currency every one of those doors takes
				// and returns, and `GeometryScriptingEditor` carries the asset door itself --
				// `CreateNewStaticMeshAssetFromMesh`, editor-only because creating a UStaticMesh is.
				"DynamicMesh",
				"Eigen",
				"GeometryScriptingEditor",
				// Bake acceptance measures saved mesh UVs and physical triangle areas.
				"MeshDescription",
				"StaticMeshDescription",
				// Budget UV specs inspect the saved source mesh through the engine converter.
				"MeshConversion",
				"ImageCore",

				// `SplitTopLevel` and `ExtractBalanced`, which every GraphCode parser already reads
				// its brackets and brace bodies with. A second copy here would drift from them.
				"AuthoringCore",

				// The build gate writes a real asset and takes it away again: `ObjectTools` is
				// UnrealEd's and the registry is how a spec finds what a package holds.
				"UnrealEd",
				"AssetRegistry",

				// `IPluginManager`, for THIS plugin's own content root — `/GraphCodeShape/Presets/`
				// (`GC-SHAPE-GRAMMAR.md` §3.2). Asked of the plugin rather than assembled from
				// `FPaths::ProjectPluginsDir()`, because an installed copy of this product lives in
				// the ENGINE's plugin directory and a project-relative path would miss it.
				"Projects",
			}
		);
		// SVG Importer is linked only by the GraphCodeShapeSvg adapter (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): a
		// hard link here made Shape unloadable in any project without that plugin.

		// `subdivide()` drives the engine's own OpenSubdiv for its crease input, which neither
		// `FSubdividePoly` nor Geometry Script passes. The engine's condition, copied from
		// ModelingComponentsEditorOnly: the library ships for Windows, Mac and Linux.
		if (Target.IsInPlatformGroup(UnrealPlatformGroup.Windows) || Target.Platform == UnrealTargetPlatform.Mac
			|| Target.IsInPlatformGroup(UnrealPlatformGroup.Unix))
		{
			PrivateDependencyModuleNames.Add("OpenSubdiv");
		}
	}
}


