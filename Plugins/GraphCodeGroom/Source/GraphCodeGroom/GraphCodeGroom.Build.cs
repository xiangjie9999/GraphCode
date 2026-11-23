using UnrealBuildTool;

public class GraphCodeGroom : ModuleRules
{
	public GraphCodeGroom(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Groom remains installable when either optional engine framework is absent. The program
		// seam still registers and names the missing framework when native publication is requested.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "HairStrands", "WITH_GRAPHCODE_GROOM",
			new string[] { "HairStrandsCore", "MeshDescription", "StaticMeshDescription", "Chaos", "ChaosCore",
				"AnimationCore", "Niagara" });
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GeometryScripting",
			"WITH_GRAPHCODE_GROOM_GEOMETRY",
			new string[] { "GeometryScriptingCore", "GeometryScriptingEditor", "GeometryCore",
				"GeometryFramework", "DynamicMesh", "SkeletalMeshDescription" });

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
				"GraphCode",
				"GraphCodeRuntime",
				"AuthoringCore",
				"GeometryCore",
				"UnrealEd",
				"AssetRegistry",
				"AssetTools",
				"ImageWrapper",
				// CF5 painted tiles read the author's PNGs through FImage.
				"ImageCore",
				// The shell material is made in the project from the recipe in this plugin's Resources.
				"Projects",
				"Renderer",
				"RenderCore",
				"RHI",
			}
		);
	}
}


