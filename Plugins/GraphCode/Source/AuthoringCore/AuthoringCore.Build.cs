using System;
using UnrealBuildTool;

/**
 * Build-time availability gates shared by every module in this project — GraphCode's, and named
 * for it [USER, 2026-08-21]: *"what OraBuildGate? who gave this name? this is called graphcode."*
 * The defines it sets have always been `WITH_GRAPHCODE_*`; the class was the odd one out.
 *
 * **IT LIVES IN A `.Build.cs` BECAUSE UBT COMPILES NOTHING ELSE.** `Rules.FindAllRulesFiles`
 * gathers `*.Build.cs` and `*.Target.cs` by suffix, so a tidy `GraphCodeBuildGates.cs` beside them is
 * never compiled and every caller fails with `CS0103: The name 'GraphCodeBuildGates' does not exist`.
 * That was measured, not assumed. All project plugins compile into ONE rules assembly, so a
 * `public static` declared here is visible from any `*.Build.cs` under `Plugins/` with no
 * dependency declared between the modules — and AuthoringCore is the plugin they all already
 * depend on, so nothing about the placement is new coupling.
 *
 * One copy, for the reason `GraphCodeCapabilities.h` gives about runtime capability data and
 * `GC-PRODUCT-PACKAGING-DESIGN.md` §6 gives about ownership: nobody keeps N hand-written copies of
 * a rule in step by reading them. The first copy of this lived in WidgetIR.Build.cs; the second
 * one would have been where the two answers started to differ.
 */
public static class GraphCodeBuildGates
{
	/**
	 * Is a plugin the CONSUMING PROJECT enables available to link against in this target?
	 *
	 * **Asked of the project descriptor rather than of the disk, and the difference is the whole
	 * point: an engine that HAS a plugin is not a target that may LINK it.** Adding a module from a
	 * present-but-disabled plugin is the exact UBT error this exists to prevent — *"module is part
	 * of plugin X which is not enabled"* — and a `Directory.Exists` check answers the wrong one of
	 * those two questions for every project that ships a stock engine and enables nothing.
	 *
	 * **IT FAILS SAFE IN ONE DIRECTION ONLY, and that is deliberate.** A plugin enabled
	 * TRANSITIVELY — by another plugin's dependency rather than by the `.uproject` — reads as
	 * absent here, so the feature compiles out and the capability registry says so at runtime. The
	 * cost of that miss is a facet a project could have had; the cost of the opposite error is a
	 * build that does not complete in somebody else's project. Only one of those is recoverable by
	 * reading a message.
	 *
	 * The caller still owes the OTHER half: the referencing `.uplugin` must list the plugin as
	 * `"Enabled": false, "Optional": true`. That suppresses UBT's *"does not list plugin"* warning
	 * — `UEBuildTarget.ValidatePlugin` checks `bOptional` ALONE and never reads `bEnabled` — while
	 * `AddPlugin`'s *"Ignore disabled references"* means it neither enables nor disables anything.
	 * `"Enabled": true, "Optional": true` also silences the warning but switches the plugin ON in
	 * every downstream project, which is a change made to the user's project on their behalf to
	 * save one branch here.
	 */
	public static bool IsProjectPluginEnabled(ReadOnlyTargetRules Target, string PluginName)
	{
		if (Target.ProjectFile == null)
		{
			// Engine-target builds have no .uproject to ask. Nothing here is built that way, and
			// answering "unavailable" keeps the no-project case on the safe branch.
			return false;
		}

		ProjectDescriptor Project = ProjectDescriptor.FromFile(Target.ProjectFile);
		if (Project.Plugins == null)
		{
			return false;
		}

		foreach (PluginReferenceDescriptor Reference in Project.Plugins)
		{
			if (!String.Equals(Reference.Name, PluginName, StringComparison.Ordinal))
			{
				continue;
			}
			// All three, because a reference can be enabled in general and excluded from THIS
			// build: TargetAllowList and platform lists are how a project says "editor only" or
			// "not on Android", and honouring bEnabled alone would re-create the same link error
			// one platform at a time.
			return Reference.bEnabled
				&& Reference.IsEnabledForPlatform(Target.Platform)
				&& Reference.IsEnabledForTarget(Target.Type);
		}
		return false;
	}

	/**
	 * The whole gate for one optional plugin: take the dependency when it is there, define
	 * `<DefineName>=0|1` either way, and SAY which branch was taken.
	 *
	 * The define is unconditional on purpose. A define that exists only on the available branch
	 * makes every guard `#ifdef` rather than `#if`, and an `#ifdef` cannot tell "compiled out" from
	 * "misspelled" — which is the failure that silently removes a feature nobody notices is gone.
	 *
	 * PUBLIC, so a dependent module cannot compile against a different answer than the one the
	 * defining module was built with. Every module computes it from the same project file, so they
	 * agree by construction rather than by convention.
	 */
	public static bool AddOptionalPlugin(
		ModuleRules Module,
		ReadOnlyTargetRules Target,
		string PluginName,
		string DefineName,
		string[] ModuleNames)
	{
		bool bAvailable = IsProjectPluginEnabled(Target, PluginName);
		if (bAvailable)
		{
			Module.PrivateDependencyModuleNames.AddRange(ModuleNames);
		}
		Module.PublicDefinitions.Add(DefineName + "=" + (bAvailable ? "1" : "0"));
		// SAID OUT LOUD, once per build. A gate that decides what is in the binary and reports
		// nothing is the debugging session somebody loses to a switch they did not set — the same
		// reason SimulateMissingProducts logs on every query. It is also the only way the OFF
		// branch is provable without reading the linker.
		Console.WriteLine("{0}: {1}={2} ({3} {4} in the project)",
			Module.GetType().Name, DefineName, bAvailable ? 1 : 0, PluginName,
			bAvailable ? "enabled" : "not enabled");
		return bAvailable;
	}

	/**
	 * The gate above decides at BUILD time from the building project, and our development project enables every framework,
	 * so a shipped DLL linked them hard and could not load in a project without them (user-ruled 2026-10-20:
	 * optional, never forced; Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md). Delay-loading those module DLLs lets the
	 * product load anywhere; its code calls FGraphCodeOptionalModules::PluginsEnabled before the first use and
	 * refuses by naming the plugin. Only plugin-owned modules belong here, never engine core ones.
	 */
	public static void DelayLoad(ModuleRules Module, ReadOnlyTargetRules Target, params string[] ModuleNames)
	{
		string Prefix = Target.Type == TargetType.Editor ? "UnrealEditor" : Target.Name;
		foreach (string Name in ModuleNames)
		{
			Module.PublicDelayLoadDLLs.Add(Prefix + "-" + Name + ".dll");
		}
	}

	/**
	 * Is the engine building this target at least Major.Minor? The build-rules half of
	 * Docs/GC-ENGINE-VERSIONS-PLAN.md: one source tree builds for UE 5.6, 5.7 and 5.8, and a module
	 * that exists only from some engine on is referenced only there. C++ asks the same question only
	 * inside GraphCodeEngineCompat.h.
	 */
	public static bool EngineAtLeast(ReadOnlyTargetRules Target, int Major, int Minor)
	{
		return Target.Version.MajorVersion > Major
			|| (Target.Version.MajorVersion == Major && Target.Version.MinorVersion >= Minor);
	}
}

public class AuthoringCore : ModuleRules
{
	public AuthoringCore(ReadOnlyTargetRules Target) : base(Target)
	{
		bUsePrecompiled = true;
		// Keep compiler DLLs outside the plugin's ordinary Windows DLL search directory.
		BinariesSubFolder = "BuildOutputs";
		PCHUsage = Target.bBuildEditor ? ModuleRules.PCHUsageMode.UseSharedPCHs : ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        // The ordinary build produces the runtime SDK used by the installed folder.
        // Keep its PCH object inside this plugin so a recipient can link without this host.
        if (!Target.bBuildEditor)
        {
            bPrecompile = true;
            PrecompileForTargets = PrecompileTargetsType.Any;
            PrivatePCHHeaderFile = "Private/GraphCodeRuntimePrivatePCH.h";
        }

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
				"Json",
			}
		);
	}
}




