#pragma once

#include "CoreMinimal.h"

struct GRAPHCODE_API FGraphCodeGenerationReport
{
	int32 ScannedAssets = 0;
	int32 SupportedAssets = 0;
	int32 CurrentSidecars = 0;
	int32 MissingSidecars = 0;
	int32 StaleSidecars = 0;
	int32 WrittenSidecars = 0;
	int32 OrphanSidecars = 0;
	int32 PrunedSidecars = 0;

	/**
	 * Graphs in which one alias is used twice (GC-VCS-MERGE-DESIGN.md M0.4).
	 *
	 * Counted apart from Errors because it is a property of the FILE, not of the projection: the
	 * sidecar can be perfectly current and still carry this, since only an edit can produce it.
	 * It is still a clean-run failure — an alias is a stored identity, so two of them name one node
	 * twice and the apply resolves whichever it reaches first.
	 */
	int32 DuplicateAliases = 0;

	/**
	 * Sidecars still holding an unresolved `conflict { }` (GC-VCS-MERGE-DESIGN.md §4.12).
	 *
	 * Never zeroed by a regeneration, unlike the counters above. A regeneration OVERWRITES a
	 * conflicted file — the previous text is preserved under `Saved/GraphCode/Proposals/` — and
	 * "the conflict is gone because I destroyed the file that recorded it" is not a resolution.
	 * The count stands so the run still fails and somebody looks.
	 */
	int32 ConflictedSidecars = 0;

	/**
	 * Statements nothing points at, project-wide (GC-VCS-MERGE-DESIGN.md M4.5, §4.6 hole 5).
	 *
	 * **REPORTED AND DELIBERATELY NOT COUNTED BY `IsClean()`**, and the reason is a measurement
	 * rather than a preference: the committed corpus already holds **123 of these across 51 files**
	 * — a `VariableGet` nobody reads, a disabled `PrintString`, a `Knot()` with nothing attached.
	 * That is ordinary Blueprint detritus, not damage. A gate on it would have failed on the day it
	 * shipped and been switched off by the end of the week, which is how a checker stops being read
	 * at all.
	 *
	 * The hole it exists for is a developer who deletes a conflict annotation without reconnecting
	 * the wire: that passes every grep-based defence and leaves a floating node. What catches it is
	 * the number MOVING, which a reviewer can see and CI can baseline — not its absolute value.
	 */
	int32 OrphanNodes = 0;

	/**
	 * Shape-built assets whose recipe has left the program that built them — CP1 second walk, H28.
	 *
	 * **THE FIFTH CATEGORY, and it is a different question from the four above it.** Those ask
	 * whether the `.gc` still describes the ASSET. This asks whether the `.gcshape` the `.gc` names
	 * still describes how to REBUILD it — a fact no projection can carry, because the projection is
	 * of the finished mesh and the mesh is fine.
	 *
	 * `SM_Trough.uasset` sat in the corpus for days with the level referencing it, its `.gc` naming
	 * the program by digest, and no `Trough` entry in that program at all. Every counter above it
	 * read zero, honestly: the sidecar was current, the asset loaded, nothing was orphaned. The
	 * evaluator's own refusal was waiting the whole time and nothing asked it, because asking is
	 * what a BUILD does and the asset already existed. The first whole-file apply, days later,
	 * refused at line 12 and the recipe was unrecoverable — the shipped mesh had to be MEASURED to
	 * write a replacement.
	 *
	 * **IT GATES, and the measurement is why.** On 2026-08-25 the tracked corpus held 9 shape-built
	 * `.gc` files and all 9 resolved, so this ships as a regression guard rather than a backlog. The
	 * 10 that do not resolve are all in `Content/__MCPTests/ShapeLive/`, which is untracked session
	 * residue — a clean checkout has none of them. That is the difference between this and
	 * `OrphanNodes` above, which does NOT gate precisely because its corpus count was 123 on the day
	 * it shipped.
	 *
	 * **A CORE-ONLY INSTALL COUNTS NOTHING HERE rather than counting everything.** Only GraphCode
	 * Shape can read a `.gcshape`, and *we could not ask* is not *the recipe is gone*.
	 */
	int32 LostShapeRecipes = 0;

	/**
	 * THE SAME QUESTION OF A `.gcmove` — a clip whose `move:` line names a program that no longer
	 * builds it. Counted apart so a red run says which product's recipe went. Gated for Shape's
	 * reason, measured 2026-09-12: no tracked `.gc` carried a `move:` line, so nothing is grandfathered.
	 */
	int32 LostMoveRecipes = 0;

	/**
	 * AND OF A `.gchuman` — a character whose `human:` line names a program that no longer builds
	 * it. Counted apart for the same reason the two above are: a red run has to say which product's
	 * recipe went, and a Core-only install (or one without the MetaHuman engine plugin) counts
	 * nothing here rather than counting every character.
	 */
	int32 LostHumanRecipes = 0;

	int32 Errors = 0;
	TArray<FString> Messages;

	bool IsClean() const
	{
		return MissingSidecars == 0 && StaleSidecars == 0 &&
			OrphanSidecars == 0 && DuplicateAliases == 0 &&
			ConflictedSidecars == 0 && LostShapeRecipes == 0 && LostMoveRecipes == 0 &&
			LostHumanRecipes == 0 && Errors == 0;
	}
};

/** Whole-project regeneration/check implementation used by both commandlets. */
class GRAPHCODE_API FGraphCodeGenerator
{
public:
	/** True only for a canonical projection; pending edits and merge state are never disposable. */
	static bool CanPruneOrphanSidecar(const FString& SidecarPath, FString& OutReason);

	/** CheckOnly never writes. Prune only has an effect during regeneration. */
	static bool Run(bool bCheckOnly, bool bPrune, FGraphCodeGenerationReport& OutReport);

	/**
	 * The NATIVE class paths the sweep's Asset Registry query enumerates, before Blueprint-derived
	 * data-asset classes are appended and before `bRecursiveClasses` widens each one to its
	 * subclasses.
	 *
	 * **EXPOSED SO THE THIRD GATE CAN BE ASSERTED, 2026-08-23 — and it is exposed because it caught
	 * something.** A kind needs THREE registrations to produce a file: `TryGetAssetKind` names it,
	 * `IsSupportedAssetData` offers it, and this query has to enumerate it in the first place. The
	 * first two have specs; this one had none, and its own comments record it being forgotten six
	 * times. `kind:render_target` made it seven: both project render targets classified correctly,
	 * both passed `IsSupportedAssetData`, both were asserted green — and `GcRegenerate` wrote no
	 * sidecar for either, because `UTextureRenderTarget` does not derive from `UTexture2D` and
	 * nothing here reached it. The report said `errors=0`.
	 *
	 * That is the shape every comment in the query warns about: no compile error, no failing test,
	 * and `missing` stays 0 because the asset is never offered. Worse for a new kind than merely
	 * silent — the orphan pass calls any sidecar this list does not expect an orphan, so a
	 * forgotten line makes a kind delete its own output on the next `-Prune`.
	 *
	 * **PATHS AS WELL AS CLASSES, C13, 2026-08-23 — and that is what closed the other seventeen.**
	 * The seven forgettings above were each fixed one kind at a time while the list held 18 classes
	 * against 46 shipped kinds, so the accident was structural rather than seven accidents. An entry
	 * may now be spelled `X::StaticClass()->GetClassPathName()` OR written out as an
	 * `FTopLevelAssetPath` — the registry compares them without resolving either, so they do the
	 * same work, but only the second can name a class from a module Core does not compile against.
	 * `kind:data_registry` was un-sweepABLE under the old rule, not merely unswept.
	 *
	 * **THE GATE IS `GraphCodeSweepCoverageTests.cpp`, and it is not a list.** It walks every loaded
	 * `UClass`, asks `FGraphCodeProjector::IsSupportedAssetData` whether a sweep would offer an asset
	 * of that class, and fails when this query cannot reach one. Adding a kind and forgetting this
	 * function is a red test rather than a deleted file.
	 */
	static TArray<FTopLevelAssetPath> SweptNativeClassPaths();
};
