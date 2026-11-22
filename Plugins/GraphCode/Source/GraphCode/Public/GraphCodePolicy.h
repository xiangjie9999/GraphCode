#pragma once

#include "CoreMinimal.h"

/** Pure path/save policy shared by live hooks, commandlets, and automation tests. */
class GRAPHCODE_API FGraphCodePolicy
{
public:
	/**
	 * The mounted content root a package belongs to, if GraphCode projects that root at all.
	 * `/Game/` or GraphCode's own plugin mount; empty for everything else. E9.1.
	 *
	 * TWO ROOTS, AND THE SECOND IS NOT "PLUGINS". A plugin's content is somebody else's asset: it
	 * arrives with the plugin, it is not in this repository, and projecting it would put a `.gc`
	 * beside every file of every third-party plugin the project happens to have — in the save hook,
	 * in the committed corpus and in CI. The exception is GraphCode's OWN content, because from
	 * E9 on the merge panel lives there, and a panel this project cannot read as text is a panel it
	 * cannot author. So the rule is "the project, plus the one plugin whose panel this is".
	 *
	 * Derived from `IPluginManager` rather than spelled `/GraphCode/`: the mount follows the
	 * plugin's name, and a hard-coded copy would keep working while pointing at nothing.
	 */
	static const FString& OwnPluginMountPoint();

	/** `/Game/`, `OwnPluginMountPoint()`, or empty when the package is under neither. */
	static FString EligibleMountPoint(const FString& PackageName);

	/**
	 * The same roots as long package paths, for an `FARFilter` — `/Game`, then the plugin's own.
	 *
	 * **THE THIRD HALF OF E9.1, AND IT WAS MISSING FOR A DAY WITH TEETH.** Widening what may be
	 * written and what is swept, without widening what is ENUMERATED, is worse than either omission
	 * on its own: `FGraphCodeGenerator::Run` finds every `.gc` under the swept directories and calls
	 * anything its asset scan did not expect an ORPHAN — and `Sync all` prunes orphans. The panel's
	 * own three sidecars were reported as orphans by `GcCheck` on 2026-08-05, which means the next
	 * clean sync would have deleted the tracked text of the panel this project reads as text. The
	 * class axis of this same trap is recorded three times in that function (UMaterial, UWorld,
	 * UDataTable); this is the path axis of it.
	 *
	 * So there is now ONE declaration of the roots and both sides derive from it. Trailing slashes
	 * are trimmed, because `FARFilter::PackagePaths` wants `/Game` and `GetMountedAssetPath` gives
	 * `/GraphCode/`.
	 */
	static TArray<FName> PackageRootsToScan();

	/**
	 * Every content directory a `*.gc` sweep has to walk — absolute, normalized, project first.
	 *
	 * THE OTHER HALF OF E9.1, and the half that would have rotted silently. Widening what may be
	 * WRITTEN without widening what is SCANNED leaves the panel unable to list its own sidecar, the
	 * launch scan unable to warn about it, and `GcCheck` unable to fail on it — a file that is
	 * projected, committed, and checked by nothing. Five sweeps existed and all five said
	 * `FPaths::ProjectContentDir()`; they say this instead.
	 *
	 * A root that is not mounted is simply absent from the list, so a build where the plugin
	 * declares no content sweeps exactly what it used to.
	 */
	static TArray<FString> ContentDirectoriesToSweep();

	/**
	 * GraphCode's own disposable workspace: `/Game/_GraphCode/Scratch`, one spelling for everybody.
	 *
	 * `FGcGreenfield::ScratchRoot` returns this rather than repeating it, because the day the two
	 * disagree the greenfield proof builds its probe somewhere the policy has never heard of.
	 */
	static const TCHAR* OwnScratchRoot();

	static bool IsOwnScratch(const FString& PackageName);

	/**
	 * A content root that a TOOL owns rather than a person: the two MCP roots and the recovery
	 * trash. Never eligible, and — the point of the function — **not a setting**.
	 *
	 * **THESE USED TO SIT IN `ExcludedFolders`, WHERE SOMEBODY COULD DELETE THEM** (S1.0, D3 of
	 * `GC-SETTINGS-DESIGN.md`). Deleting the recovery-trash entry makes every revert grow a `.gc`
	 * beside every recovered asset and makes the panel report each removal as a NEW file, which
	 * reads as the revert having created things. That is not a preference somebody can hold; it is
	 * a consequence of where the trash is. `/Game/_GraphCode/Scratch` was moved out for exactly
	 * this reason on 2026-08-06 and the rest did not follow until now.
	 *
	 * The recovery root is asked of `FGcRecoveryTrash::ContentRoot()` rather than respelled, so the
	 * day it moves again this follows it. `ContentRoot`'s own comment used to say "the two have to
	 * be spelled the same"; there is only one spelling now.
	 *
	 * **THE GATE IS `IsPackageEligible`, NOT `MaintainsSidecar`, AND THAT IS DELIBERATE.** It is
	 * where the settings list checked them before, so this move changes WHERE the rule lives and
	 * nothing about what it does — a trashed asset is as unreadable today as it was yesterday.
	 * Whether it should be readable is D2's question and is not answered here.
	 */
	static bool IsToolOwnedRoot(const FString& PackageName);

	/**
	 * Output that exists to be PROVED and then forgotten: GraphCode's own scratch, the roots a tool
	 * owns, and the automation fixture roots. SM1(b).
	 *
	 * **THIS IS NOT AN ELIGIBILITY QUESTION AND IT MUST NOT BECOME ONE.** Every root named here is
	 * readable, writable and projectable exactly as it was — `IsPackageEligible` and
	 * `MaintainsSidecar` are unchanged and do not call this. What it decides is a DEFAULT ON A
	 * BUILT ASSET: a mesh cooked here should not register ray-tracing geometry, because a BLAS is
	 * built when the mesh LOADS and its coarsest LOD is exempt from eviction, so a probe nothing
	 * ever placed costs the always-resident pool exactly what a visible mesh costs, for as long as
	 * the editor runs. 498.96 MB against a 400 MB pool, measured 2026-09-11.
	 *
	 * **THE FIXTURE ROOTS ARE SPELLED RATHER THAN DERIVED, AND THAT IS THE HONEST PART.** There is
	 * no flag on a package saying *a spec wrote this*; `/Game/__MCPTests` and its neighbours are
	 * conventions this repository holds, and a convention has to be written down somewhere to be
	 * asked about. A name that stops being used costs a stale entry and nothing else — which is why
	 * `/Game/_GENERATED` was removed from `IsToolOwnedRoot` when it turned out to name nothing.
	 */
	static bool IsDisposableOutput(const FString& PackageName);

	/**
	 * Is this package under a folder the PROJECT chose to exclude — `UGraphCodeSettings::ExcludedFolders`?
	 *
	 * **THIS IS NOT AN ELIGIBILITY QUESTION AND IT USED TO BE ONE.** User-ruled 2026-08-24:
	 * *"an excluded folder should still be readable by the AI."* Excluding a folder says **do not
	 * keep a `.gc` beside these** — it does not say *pretend these assets do not exist*. Until this
	 * ruling the list sat inside `IsPackageEligible`, which gates `gc_read`, `gc_search` and
	 * `gc_apply` as well as the file, so naming a vendored pack made every asset in it unanswerable.
	 * D2 of `GC-SETTINGS-DESIGN.md`, whose §8 recommended keeping one meaning until somebody asked.
	 * Somebody asked.
	 *
	 * So it is asked by the two places that decide whether a FILE is kept — `MaintainsSidecar`, and
	 * the sync's write — and by nothing that reads. The matcher is unchanged and always was a folder
	 * test: equal to the entry, or under `entry + "/"`.
	 */
	static bool IsUserExcludedFolder(const FString& PackageName, FString* OutReason = nullptr);

	/**
	 * Is a `.gc` forbidden for this package **by a setting** — the project switch, or an excluded
	 * folder? The reason says which, in words a person can act on.
	 *
	 * **ONE PREDICATE BECAUSE THE TWO ARE ONE BEHAVIOUR AT TWO SCOPES**, and because a fact with
	 * two carriers is a race ([[two-carriers-of-one-fact-is-a-race]]). `SyncPackage` spelled the
	 * pair inline for its `Suppressed` exit, and the day a second writer needed the same test it
	 * asked neither: `gc_apply` re-projects through `RewriteSidecarNow`, which writes the file
	 * directly rather than through the sync, so an apply to an excluded folder went on leaving text
	 * beside the asset. That was found by a spec asserting the ruling, not by a reader of the code.
	 *
	 * **IT IS NOT `MaintainsSidecar`, AND THE DIFFERENCE IS WHOSE RULE IT IS.** `MaintainsSidecar`
	 * also drops GraphCode's own scratch, which is a fact about the tool — a greenfield probe grows
	 * a sidecar for the length of its proof and must go on doing so. This asks only about the two
	 * settings a person set, so a writer that must honour a person's choice without inheriting the
	 * tool's bookkeeping has exactly one thing to call.
	 */
	static bool SidecarsSuppressedFor(const FString& PackageName, FString* OutReason = nullptr);

	/**
	 * May GraphCode operate on this package at all — read it, sync it, apply to it?
	 *
	 * The roots of E9.1, a valid long package name, and the tool-owned roots. **NOT the project's
	 * `ExcludedFolders`**, which is about the file and is asked elsewhere — see above.
	 *
	 * **GRAPHCODE'S OWN SCRATCH IS ELIGIBLE AND IS NOT SWEPT, AND THOSE ARE TWO QUESTIONS.** They
	 * used to be one, and the cost was exact: `/Game/_GraphCode/Scratch` was on the exclusion list so
	 * the panel would not list a probe as a new file — and that ALSO made the probe unreadable and
	 * unwritable by the pipeline, so `FGcGreenfield`'s scratch-first proof (§2.7, and the design's
	 * own requirement) could never pass. Measured 2026-08-06: **every one of 77 deleted packages
	 * refused with `package matches an excluded GraphCode content prefix`** — the create path had
	 * never worked for any kind, and the sentence that said so was truncated by the panel's footer.
	 *
	 * So "is this mine to write files beside" is `MaintainsSidecar` now, and it is what every sweep,
	 * the save hook and the panel's scan ask.
	 */
	static bool IsPackageEligible(const FString& PackageName, FString* OutReason = nullptr);

	/**
	 * Does GraphCode keep a `.gc` on disk beside this package, sweep it, and list it?
	 *
	 * Eligible, minus GraphCode's own scratch. Everything that GENERATES asks this; everything that
	 * READS or WRITES an asset asks `IsPackageEligible`. A probe still grows a sidecar for the
	 * duration of the proof, because the apply pipeline's first act is a sync — `DiscardScratch`
	 * takes that file away again with the probe.
	 */
	static bool MaintainsSidecar(const FString& PackageName, FString* OutReason = nullptr);

	static bool IsRealProjectAssetSave(
		const FString& PackageName,
		const FString& TargetFilename,
		bool bSaveSucceeded,
		bool bIsCooking,
		bool bIsProcedural,
		bool bIsAutoSave,
		bool bContainsMap,
		FString* OutReason = nullptr);
};
