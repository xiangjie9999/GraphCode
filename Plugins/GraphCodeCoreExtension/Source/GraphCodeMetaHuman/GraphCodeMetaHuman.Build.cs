using UnrealBuildTool;

public class GraphCodeMetaHuman : ModuleRules
{
	public GraphCodeMetaHuman(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Advanced renders the hair a MetaHuman is wearing when HairStrands is present; Groom's own
		// authoring code now lives in the separate GraphCodeGroom module.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "HairStrands", "WITH_GRAPHCODE_HAIRSTRANDS",
			new string[] { "HairStrandsCore", "MeshDescription", "Chaos", "ChaosCore", "AnimationCore" });
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GeometryScripting", "WITH_GRAPHCODE_GEOMETRY_SCRIPTING",
			new string[] { "GeometryScriptingCore", "GeometryScriptingEditor", "GeometryCore",
				"GeometryFramework", "DynamicMesh", "SkeletalMeshDescription" });
		// Integration specs exercise a Groom Binding on a second MetaHuman head. Advanced remains
		// independently installable; this module edge exists only when the Groom plugin is enabled.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "GraphCodeGroom",
			"WITH_GRAPHCODE_GROOM_PRODUCT", new string[] { "GraphCodeGroom" });


		// `kind:metahuman` and the `.gchuman` builder (`GC-HUMAN-GRAMMAR.md`). **USER-RULED
		// 2026-09-15 that this is ADVANCED's and not Move's**: MetaHumanCharacter is an optional
		// engine plugin, which is precisely what this product is for, and the gate below is the
		// section's own standing RULE -- *an optional plugin is never a blocker*.
		//
		// THREE MODULES, AND THE SPLIT BETWEEN THEM IS THE WHOLE DESIGN. `MetaHumanCharacter` is
		// RUNTIME and holds every settings struct `kind:metahuman` reflects, so the read half needs
		// only it. `MetaHumanCharacterEditor` holds `UMetaHumanCharacterEditorSubsystem` -- the
		// preset, blend, sculpt, measurement and variant doors a `.gchuman` build drives -- and
		// `MetaHumanCoreTechLib` holds `FMetaHumanCharacterIdentity::FState`, the type those doors
		// pass around. Both editor modules serve the AUTHORED half only; nothing in the projector
		// touches either, which is what keeps `saved` and `live` answering alike.
		//
		// **WHAT THIS DOES NOT BUY IS THE REGION NAMES.** `MetaHumanCreatorAPI::GetRegionNames()`
		// sits behind MetaHumanCoreTechLib's own `PrivateIncludePaths`, so linking the module does
		// not reach it -- `GC-HUMAN-GRAMMAR.md` SS0.1, and the one thing still open there.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "MetaHumanCharacter",
			"WITH_GRAPHCODE_METAHUMAN",
			// `MetaHumanCharacterPalette` is the collection, the instance and the wardrobe item —
			// what `outfit { }` reads and writes, and the only door to what a character is WEARING
			// rather than what it could wear (`GC-HUMAN-RIG-ASSEMBLY-DESIGN.md`, worn items).
			// `MetaHumanCharacterPaletteEditor` is the editor's preview actor, which `gc_render` draws
			// dressed. `MetaHumanDefaultPipeline` and `MetaHumanDefaultEditorPipeline` are the Skeletal
			// Clothing pipeline a garment's wardrobe item is made on, and its hidden face maps
			// (`GC-GARMENT-METAHUMAN.md` §4 step 3); `ImageCore`, the map as the engine reads it.
			// `ChaosClothAsset`, `ChaosClothAssetEngine` and `ChaosOutfitAssetEngine` are the Outfit Clothing
			// slot's garment — a cloth asset in an outfit (step 4). The MetaHuman Character plugin requires both
			// Chaos plugins, so this gate is theirs too; `GeometryFramework` is the garment handed over, and
			// `DataflowEngine` the refit graph Epic's pipeline runs an outfit through, which a spec runs too.
			new string[] { "MetaHumanCharacter", "MetaHumanCharacterEditor", "MetaHumanCoreTechLib",
				"MetaHumanCharacterPalette", "MetaHumanCharacterPaletteEditor", "MetaHumanFaceFittingSolver", "MetaHumanCoreTech",
				"MetaHumanIdentity", "CaptureDataCore", "GeometryCore", "DynamicMesh",
				"MetaHumanDefaultPipeline", "MetaHumanDefaultEditorPipeline", "ImageCore",
				"GeometryFramework", "Chaos", "ChaosClothAsset", "ChaosClothAssetEngine", "ChaosOutfitAssetEngine", "DataflowEngine" });

		// `.gcmove`'s clip(speech=) (GC-MOVE-LIP-SYNC.md L3): a face solved from a sound by Epic's offline audio-driven animation,
		// MetaHuman Animator. Its plugin key is `MetaHuman`; MetaHumanCharacter already enables it for the editor, and the project
		// names it too so this gate, which reads the .uproject, can see it.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "MetaHuman", "WITH_GRAPHCODE_METAHUMAN_ANIMATOR",
			// MetaHumanSpeech2Face holds EAudioDrivenAnimationOutputControls (full face / mouth only), which the Performance's
			// header uses but does not depend on publicly.
			// MetaHumanPipelineCore holds FPipelineData, whose exit status is the only completion report a Performance gives.
			new string[] { "MetaHumanPerformance", "MetaHumanSpeech2Face", "MetaHumanPipelineCore" });

		// `.gcmove`'s drive() of a face curve is checked against the curves the face's DNA reads (GC-MOVE-FACE-EXPRESSIONS-PLAN.md,
		// the curve-name check): RigLogic turns each raw control of the mesh's UDNAAsset into a curve name. Its plugin key is `RigLogic`;
		// MetaHumanCharacter and MetaHuman enable it for the editor, and the project names it too so this gate, which reads the
		// .uproject, can see it. RigLogicModule holds UDNAAsset and IDNAReader.
		GraphCodeBuildGates.AddOptionalPlugin(this, Target, "RigLogic", "WITH_GRAPHCODE_RIGLOGIC", new string[] { "RigLogicModule" });

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
				// Human references use Core's product-neutral mesh provenance seam. Neither Shape nor
				// Groom enters Advanced's hard dependency graph.
				"WidgetIR",
				"AuthoringCore",
				"UMG",
				"UMGEditor",
				"UnrealEd",
				"BlueprintGraph",
				// A MetaHuman GraphCode keeps open is let go when the Content Browser selects it with another:
				// opening those together skips the per-asset request `FGcMetaHumanOpen` otherwise answers.
				"ContentBrowser",

				// `kind:env_query` (`GC-AI-AUTHORING-DESIGN.md` §3). AIModule owns UEnvQuery and its
				// runtime option/generator/test instances and the FAIDataProviderValue provider
				// structs; AIGraph owns UAIGraphNode::SubNodes; EnvironmentQueryEditor owns the
				// authored EdGraph and its editor nodes. All three are enabled-by-default engine
				// modules, the same footing GraphCodeMove reads its BehaviorTreeEditor graph on —
				// which is why EQS is Core Extension: its editor half is an optional plugin, present
				// here but not guaranteed in every install.
				"AIModule",
				"AIGraph",
				"EnvironmentQueryEditor",

				// **`landmarks_config.json` — the ONE thing that makes a face region nameable.**
				// `GC-HUMAN-GRAMMAR.md` SS0.1: the engine has no public region-name accessor, and
				// the names are nevertheless shipped as plain JSON in the MetaHumanCharacter
				// plugin's own content. Projects mounts the plugin so its content directory can be
				// found; Json reads the file. Both are engine runtime modules, so neither is gated
				// even though the only caller is.
				"Json",
				"Projects",

				// A `.gchuman` assembly lists the folder it writes into so it can refuse to overwrite
				// packages it does not own, and records what it made as a manifest. Named rather than
				// leaned on transitively, like AssetTools below.
				"AssetRegistry",

				// `kind:env_query`'s greenfield factory goes through IAssetTools::CreateAsset, the
				// same door Core's kinds are created by. Named explicitly rather than leaned on
				// transitively, so the create keeps building if a module between here and it drops it.
				"AssetTools",

				// G00's GPU acceptance renders the isolated native Groom fixture and reads the
				// resulting pixels. These are editor-module test dependencies; the authored
				// builder still hands rendering and deformation to HairStrands at runtime.
				"Renderer",
				"RenderCore",
				"RHI",
			}
		);

		// The four framework plugins are optional. Their codecs belong in adapter modules or behind
		// GraphCodeBuildGates checks as they land; making any one a hard dependency here would prevent a
		// customer from installing the other three. This base module supplies the product identity.
	}
}



