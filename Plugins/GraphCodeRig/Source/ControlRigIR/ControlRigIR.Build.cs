using UnrealBuildTool;

public class ControlRigIR : ModuleRules
{
	public ControlRigIR(ReadOnlyTargetRules Target) : base(Target)
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
				"AuthoringCore",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"BlueprintIR",

				// The hierarchy half: `URigHierarchy`, `FRigElementKey` and `ERigElementType`, plus
				// `UControlRigBlueprint` itself.
				"ControlRig",
				"ControlRigDeveloper",

				// `UControlRigBlueprintFactory`, and it is the SPECS that need it rather than the
				// reader: a spec that read back the one rig we happen to write would prove the
				// reader and that writer agree with each other and nothing about either being
				// right, so the specs author a rig of their own and this is the only door that
				// initialises a `UControlRigBlueprint` properly.
				"ControlRigEditor",

				// **THE MODEL, AND IT IS THE WHOLE REASON THIS MODULE EXISTS RATHER THAN A BRANCH IN
				// BlueprintIR.** `URigVMEdGraphSchema` derives from `UEdGraphSchema` and not from
				// `UEdGraphSchema_K2`, so BlueprintIR's admission list refuses a Control Rig graph —
				// correctly, because the EdGraph is only a VIEW. `URigVMGraph`/`URigVMNode`/
				// `URigVMPin` are the source of truth and a tidier target than the view ever was.
				"RigVM",
				"RigVMDeveloper",
			}
		);
	}
}


