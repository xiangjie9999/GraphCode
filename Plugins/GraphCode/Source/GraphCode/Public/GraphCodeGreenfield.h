#pragma once

#include "CoreMinimal.h"

#include "GraphCodeSidecarWriter.h"

/**
 * E11.7a — BUILDING AN ASSET THAT DOES NOT EXIST, FROM A `.gc` THAT DOES.
 *
 * `GC-LOCAL-HISTORY-DESIGN.md` §2.7's `Create`. Reached when a comparison finds the target has an
 * asset this project does not: a revert to before somebody deleted one, or to before a branch was
 * merged out.
 *
 * **THE CLAIM THIS FILE HAS TO EARN IS THAT A `.gc` IS A SUFFICIENT RECIPE**, and it is not one
 * claim, it is two:
 *
 *  1. **Per KIND** — is there a greenfield factory, and does the header carry the argument it takes?
 *     That is the capability registry's `Creation` field, decided once and written down.
 *  2. **Per DOCUMENT** — does *this* text, applied to a fresh asset of that kind, come back saying
 *     what it said? Nothing can decide that by reading; the only honest answer is to build one.
 *
 * So `Plan` BUILDS A SCRATCH ASSET and applies the target to it. That is expensive and it is the
 * point: the alternative is creating the real asset, applying, and discovering halfway that the
 * document was not a recipe — which leaves a half-built asset at a path that was empty when
 * somebody pressed the button. §6's rule is that everything is validated before the first mutation,
 * and for a create the validation IS a mutation, so it happens somewhere disposable.
 *
 * **WHAT IT IS NOT: a second writer.** Every block still goes through `FGraphCodeApply::ApplyDocument`
 * under `snapshot`, against the fresh asset's own live projection, with the ordinary staleness gate
 * passing for the ordinary reason. This file resolves a factory and rebases a document; it writes no
 * asset state itself.
 */

class UObject;
struct FGraphCodeParsedDocument;

/** A creation, decided and not yet performed. */
struct GRAPHCODE_API FGcGreenfieldPlan
{
	FString PackageName;

	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::Blueprint;
	FString KindToken;

	/** The resolved constructor argument — a parent class, a parent material, a row struct. */
	FString ConstructorArgument;

	/** Which of the target's blocks the create would write, in projection order. */
	TArray<FString> Blocks;

	/**
	 * What the created asset would project back to, when the caller asked for it. C21.3.
	 *
	 * **THE ONE THING A CREATE CANNOT GET ANY OTHER WAY: there is no previous version to diff
	 * against.** It carries the canonicalised values, the `omits:` line the new asset will have, and
	 * every field the factory filled in that the payload never mentioned — so a caller comparing it
	 * against what they sent sees the auto-generated half directly instead of being told about it.
	 *
	 * Empty unless `Plan` was asked for it, because producing it costs a scratch build.
	 */
	FString ProjectedText;

	/**
	 * Everything true about this create that the label cannot carry: a facet the target omits, a
	 * block the fresh asset has no slot for, a value the writer canonicalised on the scratch.
	 *
	 * ON A SUCCESSFUL PLAN AS WELL AS A REFUSED ONE, for the same reason `FGcRestorePlan` does it.
	 */
	TArray<FString> Caveats;

	/**
	 * A scratch asset of this kind was built, the target was applied to it, and it read back.
	 *
	 * FALSE ON A PLAN THAT IS OTHERWISE FINE means the proof has not been run yet — `Plan` sets it.
	 * `Create` refuses without it, because an unproven create is the thing this file exists to
	 * prevent.
	 */
	bool bProven = false;

	/** Why nothing is on offer. Non-empty means the plan refuses and nothing was touched. */
	FString Refusal;

	bool IsRefused() const { return !Refusal.IsEmpty(); }
};

class GRAPHCODE_API FGcGreenfield
{
public:
	/** `/Game/_GraphCode/Scratch` — excluded from projection, never saved, cleared on restart. */
	static const TCHAR* ScratchRoot();

	/**
	 * Is there already something at this path? The one predicate for it, C21.1.
	 *
	 * **ON DISK OR IN MEMORY, and the second half is the load-bearing one.** `Create` leaves the new
	 * package DIRTY AND UNSAVED on purpose, so `FPackageName::DoesPackageExist` — which asks the
	 * filesystem — answers `false` for an asset that was created ninety seconds ago and is sitting
	 * in front of you. A caller using only that half would offer to create it a second time.
	 *
	 * It is also what makes `ApplyDocument`'s create branch non-recursive without a flag: the scratch
	 * proof and the real create both build the package BEFORE applying to it, so by the time
	 * `ApplySnapshotOnto` re-enters `ApplyDocument` this answers `true` and the branch is not taken.
	 */
	static bool AssetExists(const FString& PackageName);

	/**
	 * Existing assets one small slip away from a path that holds nothing. C21.4.
	 *
	 * **THIS IS THE JOB THE OLD "not found" REFUSAL WAS SILENTLY DOING**, and C21 took it away.
	 * `asset:/Game/Blueprints/BP_Romo` used to be an error; under create-by-request it is an offer,
	 * and an offer accepted mints a second Blueprint beside `BP_Room` that nothing references. So
	 * the announcement has to carry the spell-check the refusal used to be.
	 *
	 * **A CASE-ONLY DIFFERENCE IS A NEAR MISS AND RANKS FIRST.** Every path lookup in this plugin is
	 * case-sensitive on purpose, and wherever a miss means *create* that strictness is a hazard on
	 * its own — `bp_room` against `BP_Room` is the exact shape of typo that ends with two assets.
	 *
	 * Same folder only. A right name in the wrong folder is a different mistake and finding it
	 * would mean scanning the project.
	 */
	static TArray<FString> NearMisses(const FString& PackageName, int32 MaxResults = 3);

	/**
	 * The two or three lines that WOULD have worked, for a document that has none. C21.4, §16.2.
	 *
	 * **A REFUSAL HERE TEACHES THE HEADER RATHER THAN DIAGNOSING IT.** "Expected `kind:` in header"
	 * is a parser talking to its author; a caller shown the recipe does not have to go and read a
	 * grammar. Where the kind is known, the template states THAT kind's `parent:` requirement out of
	 * the registry instead of a generic one — the floor is per kind.
	 *
	 * @param KindToken  empty when even the kind could not be read, which makes the template an
	 *                   example rather than a correction, and it says so.
	 */
	static FString HeaderTemplate(const FString& PackageName, const FString& KindToken);

	/**
	 * Decide whether `TargetText` can be built at `PackageName`, and PROVE it on a scratch asset.
	 *
	 * Refuses before building anything when: the kind's registry entry says creation is
	 * unsupported; the document is not a valid restore point (conflicted, an unabsorbed proposal,
	 * or carrying a blocking `errors { }`); the package path is occupied or holds a redirector; or
	 * the constructor argument the kind needs is missing or does not load.
	 *
	 * **THE SCRATCH PROOF IS MEMOISED PER (kind, constructor argument) FOR THE SESSION.** A
	 * project-wide revert that creates thirty user structs builds one scratch struct, not thirty:
	 * the question the proof answers is *"does a fresh asset of this shape take a document like
	 * this"*, and thirty documents of one kind ask it once. A NEW pair pays for its own proof.
	 *
	 * **WHAT THE MEMO HOLDS IS THE FRESH ASSET'S OWN PROJECTION, AND THAT IS C21.3'S FIX.** Keyed on
	 * a shape, it may only answer questions about a shape — and `Blocks`, `Caveats` and
	 * `ProjectedText` are all per DOCUMENT. Storing the baseline text lets the per-document half be
	 * re-decided from it without building anything, because deciding which blocks differ from the
	 * factory's defaults is pure text once you have the defaults. Before this the memoised branch
	 * simply skipped the assignment and reported `blocks: 0` for the second document of a shape.
	 *
	 * @param bWantProjectedText  fill `OutPlan.ProjectedText`, which FORCES the scratch build past
	 *                            the memo. The memo answers *does this shape take a document like
	 *                            this*; it cannot answer *what does THIS one come back as*.
	 */
	static bool Plan(
		const FString& PackageName,
		const FString& TargetText,
		bool bWantProjectedText,
		FGcGreenfieldPlan& OutPlan,
		FString& OutError);

	/**
	 * Build the real asset and apply the target to it. Requires `Plan` to have proven the shape.
	 *
	 * **SAVES THE NEW PACKAGE.** C24.0: a write finishes itself, and a create is a write. Until
	 * 2026-08-08 this left it dirty so that a revert's create step stayed undoable; that was put to
	 * the user when the change failed the spec carrying it, and the ruling is that creating a file
	 * does not really need undo. Both callers — `gc_apply create:true` and the restore path — save.
	 *
	 * @param bDryRun  validate and touch nothing. The plan already built a scratch, so a dry run
	 *                 here really is free: it re-checks the path is still empty and returns.
	 */
	static bool Create(
		const FGcGreenfieldPlan& Plan,
		const FString& TargetText,
		bool bDryRun,
		UObject*& OutObject,
		FString& OutError);

	/**
	 * Splice a target document's blocks onto a live asset's projection and apply them under
	 * `snapshot`. Shared by the scratch proof and the real create.
	 *
	 * **THE REBASE IS THE SAME ONE `FGcRestore` DOES** (§6): the desired document is the LIVE
	 * projection with the target's blocks replacing its own, so the header is the new asset's and
	 * the staleness gate passes because it is true, not because it was skipped.
	 */
	static bool ApplySnapshotOnto(
		const FString& PackageName,
		const FGraphCodeParsedDocument& Target,
		bool bDryRun,
		TArray<FString>& OutBlocksWritten,
		TArray<FString>& OutCaveats,
		FString& OutError);

	/** Forget every memoised scratch proof. For a spec that wants the build to actually happen. */
	static void ForgetProofs();
};
