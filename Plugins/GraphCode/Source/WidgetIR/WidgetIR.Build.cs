using UnrealBuildTool;

public class WidgetIR : ModuleRules
{
	public WidgetIR(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		bPrecompile = true;
		PrecompileForTargets = PrecompileTargetsType.Any;
		PrivatePCHHeaderFile = "Private/GraphCodeReleasePrivatePCH.h";
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Optional framework adapters do not belong here. WidgetIR owns the neutral document and
		// grammar; GraphCode Core Extension conditionally links ModelViewViewModel and registers its
		// adapter through FWidgetIRExtensionCodecs.
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"AuthoringCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Json",
				"JsonUtilities",
				"UMG",
				"UMGEditor",
				"UnrealEd",
				"AssetRegistry",
				"Projects",
				// Two questions, both asked of the engine rather than reimplemented:
				// UEdGraphSchema_K2::CanUserKismetAccessVariable, the gate for which delegates the
				// Designer offers an event handler for, and UK2Node::ReferencesVariable, which is
				// how a widget name's graph consumers are found before it is renamed or deleted.
				// This is the K2 SCHEMA and the node base class, not BlueprintIR: the one-way
				// dependency in UI-AUTHORING-DESIGN.md section 1 is between the two authoring
				// languages, and it holds - WidgetIR asks a node one boolean and never reads a
				// graph's meaning.
				"BlueprintGraph",
				// A widget animation binds its target by FName and its MovieScene possessable
				// carries that name too, so a rename has to move both or the animation is stranded.
				"MovieScene",
				// ui_render. FWidgetRenderer lives in UMG (already above); these are what turn its
				// render target into a file: Slate/SlateCore for the application and geometry,
				// RenderCore for FlushRenderingCommands, ImageWrapper for the PNG encoder.
				"Slate",
				"SlateCore",
				"RenderCore",
				"ImageWrapper",
			}
		);
	}
}



