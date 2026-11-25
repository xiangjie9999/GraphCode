#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "GraphCodeProducts.h"
#include "GraphCodeSidecarWriter.h"

struct FAssetData;

/**
 * The header fields this build would write for one asset kind.
 *
 * Everything here is decided by the KIND alone, which is what makes it answerable from the
 * Asset Registry without loading the package - and that is the whole of gc_sync's fast path.
 * BuildDocument reads the same table, so a projector change that adds a block or an `omits:`
 * token moves this too, and a sidecar written before that change stops matching. Two copies
 * of the list would instead let the fast path certify a projection that is now wrong, which
 * is the silent drift GC-SIDECAR-DESIGN §10 exists to catch.
 */
struct GRAPHCODE_API FGraphCodeEnvelopeExpectation
{
	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::Blueprint;
	TArray<FString> Omits;
	TArray<FString> IrVersions;
};

/**
 * How to read an asset when projecting it.
 *
 * THE DEFAULTS ARE THE CANONICAL SET, and that is the only reason this struct exists. A `.gc`
 * has exactly one legal rendering per asset — that is what makes `sig:` a digest, what lets
 * gc_sync decide "current" by comparing text, and what makes a git diff mean something. Every
 * field below was previously a literal inside BuildDocument; they are named here so that a
 * caller who deviates can be TOLD it deviated, rather than receiving text that looks like a
 * `.gc`, carries a `sig:`, and would be refused by gc_apply for reasons resembling a race.
 *
 * A non-default read is a legitimate thing to want — it is what bp_read's `include_inherited`
 * and ui_read's `verify_values` are for. It is simply not a `.gc`. Callers must ask
 * IsCanonical() and suppress the signature when it answers false.
 */
struct GRAPHCODE_API FGraphCodeProjectionOptions
{
	/** Blueprint: inherited variables. Hundreds of entries on a Character subclass. */
	bool bIncludeInherited = false;

	/** Widget: reflected properties equal to class/slot defaults. */
	bool bIncludeWidgetDefaults = false;

	/** Widget: prove each value survives export-import-export before emitting it. Slow. */
	bool bVerifyWidgetValues = false;

	/** Blueprint: restrict graph/functions to one graph by exact name. Empty means all. */
	FString GraphName;

	/**
	 * Refuse a package GraphCode may not write a sidecar for. TRUE for every write path.
	 *
	 * The eligibility rule (`/Game` only, minus the settings denylist) exists to stop the save
	 * hook writing `.gc` files where they are unwanted. A READ has no such consequence: nothing
	 * is created, and refusing one is refusing to show the user their own asset because of a
	 * setting about file generation.
	 *
	 * This is not a loosening of the write rule. gc_apply still syncs, and the sync still
	 * refuses — so an ineligible asset remains unwritable through GraphCode, and merely becomes
	 * readable. The read reports `sidecar:"none"` so a caller knows the staleness check that
	 * normally guards an edit is unavailable here.
	 */
	bool bRequireEligiblePackage = true;

	bool IsCanonical() const
	{
		return !bIncludeInherited && !bIncludeWidgetDefaults && !bVerifyWidgetValues
			&& GraphName.IsEmpty();
	}

	/** Which fields took it off canonical, for reporting. Empty exactly when IsCanonical(). */
	TArray<FString> NonCanonicalFields() const
	{
		TArray<FString> Fields;
		if (bIncludeInherited) Fields.Add(TEXT("include_inherited"));
		if (bIncludeWidgetDefaults) Fields.Add(TEXT("include_defaults"));
		if (bVerifyWidgetValues) Fields.Add(TEXT("verify_values"));
		if (!GraphName.IsEmpty()) Fields.Add(TEXT("graph_name"));
		return Fields;
	}
};

/** Turns one supported Unreal asset into its complete deterministic GraphCode document. */
class GRAPHCODE_API FGraphCodeProjector
{
public:
	static bool IsSupportedAssetData(const FAssetData& AssetData);

	/**
	 * In the allowlist by class, and deliberately not projected by this build anyway.
	 *
	 * Exactly one case today: a World Partition level (WORLD-GC-DESIGN.md §6). It exists because a
	 * refusal and a failure must not be spelled the same in a sweep — GcCheck returning non-zero
	 * because a project contains a WP level would be a CI failure for a decision, and a decision
	 * that fails CI gets "fixed" by whoever is unblocking the build.
	 *
	 * Answering requires loading the world, so this is only asked of a kind that can refuse.
	 */
	static bool IsRefusedByDesign(const FAssetData& AssetData, FString& OutReason);

	/** Which kind this asset projects as, decided from registry class data alone. */
	static bool TryGetAssetKind(const FAssetData& AssetData, EGraphCodeAssetKind& OutKind);

	/**
	 * Which kind a LOADED Blueprint projects as — the same question, asked of the object.
	 *
	 * **TWO ADAPTERS, ONE DECISION, AND THE SPLIT IS FORCED.** The registry path must answer without
	 * loading, so it reads the `BlueprintType` tag (a string, because the property is
	 * `UPROPERTY(AssetRegistrySearchable)`); BuildDocument already holds the object and reads the
	 * enum. Neither can do the other's job.
	 *
	 * What was NOT forced, and was the defect §11.4b would have shipped: BuildDocument deciding for
	 * itself with an inline `IsA<UAnimBlueprint>()` ternary, under a comment claiming it asked
	 * `TryGetAssetKind` *"so this file has ONE definition of what an asset's kind is"*. It had two,
	 * and they agreed only because the question was binary. Add three sub-types to one and not the
	 * other and the sweep certifies `blueprint_interface` while the projection stamps `blueprint` —
	 * a `.gc` whose fast path and slow path disagree about its own header.
	 *
	 * `GraphCode.Projection.BothKindDoorsAgree` drives BOTH against the six real sub-type assets in
	 * this project, which is what makes the agreement measured rather than asserted.
	 */
	static EGraphCodeAssetKind BlueprintKindOf(const UBlueprint* Blueprint);

	/**
	 * The same mapping read backwards — which `EBlueprintType` a kind's factory has to be given.
	 *
	 * **HERE RATHER THAN IN GraphCodeGreenfield, so both directions of one mapping live in one
	 * file.** The factory is the only caller, and putting it beside the factory is what would let
	 * the two halves drift: a kind added to the read direction and forgotten in the write one
	 * projects correctly and then cannot be created, which is precisely the state §11.4b existed to
	 * end. `GraphCode.Projection.BlueprintKindRoundTripsThroughItsType` walks the enum and asserts
	 * the two are inverses.
	 *
	 * Returns `BPTYPE_Normal` for every kind that is not one of the three sub-types, including the
	 * kinds that are not Blueprints at all — their factories never read it.
	 */
	static EBlueprintType BlueprintTypeFor(EGraphCodeAssetKind Kind);

	/**
	 * Which revision of this project's projection code wrote a `.gc`. Stamped as `projector:N` on
	 * the header's `source:` line, and compared by gc_sync's fast path.
	 *
	 * **BUMP THIS IN THE SAME COMMIT AS ANY CHANGE THAT ALTERS CANONICAL PROJECTED TEXT** — here, in
	 * a kind's writer, or in BlueprintIR / WidgetIR / AuthoringCore, whose readers decide most of
	 * what a Blueprint document says. The number is what makes `bin:` insufficient on its own: a
	 * matching binary proves the ASSET is unchanged, and only this proves the CODE that read it is
	 * the code in front of you. Without it a projector change leaves every sidecar in every checkout
	 * certifying itself as current (GC-SIDECAR-DESIGN §14.7).
	 *
	 * NOT an `ir:` token, and the difference is a refusal. An `ir:` version is a compatibility claim
	 * — a reader refuses a grammar it was not written for. This is provenance: a document stamped by
	 * a newer projector is perfectly readable, it is merely no longer what this build would write.
	 * So nothing refuses on it; the fast path declines and re-projects, which is the whole remedy.
	 *
	 * A bump rewrites one header line in every sidecar whose asset is present, and moves no `sig:`
	 * (the body digest excludes the header). Cheap, reviewable, and the honest size of the claim.
	 *
	 *   1  2026-08-11  The stamp itself. Baseline: the projector as of row 11o's regeneration,
	 *                  pin-type-hint change included.
	 */
	static int32 ProjectionRevision();

	/** The `omits:` and `ir:` this build writes for that kind. */
	static FGraphCodeEnvelopeExpectation ExpectedEnvelope(EGraphCodeAssetKind Kind);

	/** Asset-aware expectation; distinguishes Actor Blueprint facets without loading the asset. */
	static FGraphCodeEnvelopeExpectation ExpectedEnvelope(const FAssetData& AssetData);

	/** The canonical projection — the one the save hook writes and gc_sync certifies. */
	static bool BuildDocument(UObject* Asset, FGraphCodeDocument& OutDocument, FString& OutError);

	/** A projection under explicit options. Non-canonical output must not be stamped with a sig:. */
	static bool BuildDocument(UObject* Asset, const FGraphCodeProjectionOptions& Options,
		FGraphCodeDocument& OutDocument, FString& OutError);

	/**
	 * A STUB: the header, the binary digest, and a blocking `MissingModule` entry saying which
	 * GraphCode product would have projected this asset. `GC-PRODUCT-PACKAGING-DESIGN.md` §2.
	 *
	 * The KIND-scope answer, and it is written rather than skipped for one reason: an asset with no
	 * sidecar is silently invisible to version control, which is the state the block was
	 * commissioned to end. Nothing may be read out of it — `omits: all@<product>` says so, `sig:` is
	 * absent because a stub is not a projection, and `HasBlockingError()` refuses it a second way.
	 *
	 * Takes `FAssetData` rather than a loaded object ON PURPOSE. The whole premise is that the code
	 * which understands this asset is not in the build, so the object may not resolve — and a stub
	 * that could only be written for assets that load would be absent exactly where it is needed.
	 * Everything it states comes from the registry entry and the file on disk.
	 *
	 * @param Product the product that carries the kind and is not installed.
	 */
	static bool BuildStubDocument(const FAssetData& AssetData, EGcProduct Product,
		FGraphCodeDocument& OutDocument, FString& OutError);

	/**
	 * The advisory `errors { }` entries a projection of this kind carries right now, one per absent
	 * product. Empty on a complete install, which is every development checkout.
	 */
	static TArray<FGcSidecarError> ProductAdvisories(EGraphCodeAssetKind Kind);

	static bool BuildText(const FAssetData& AssetData, FString& OutPath, FString& OutText, FString& OutError);

	/**
	 * The canonical LIVE text for one package — what `gc_read source:"live"` answers.
	 *
	 * The registry lookup, the "which object in this package does the projector accept" question and
	 * the build, in one place. It was written twice before this existed (the pre-revert checkpoint
	 * and the restore planner), and the two copies would have had to agree about the second of those:
	 * a package can hold several objects, and taking the first and hoping is how a restore ends up
	 * describing an asset nobody asked about.
	 *
	 * Includes UNSAVED edits, by construction — it projects the loaded object. That is the whole
	 * reason a checkpoint uses it rather than reading the `.gc`.
	 */
	static bool BuildTextForPackage(const FString& PackageName, FString& OutText, FString& OutError);

	/**
	 * Every block a Blueprint's own document carries, in §13.9's order — W6.1.
	 *
	 * **PUBLIC BECAUSE A LEVEL NESTS ONE.** A `ULevelScriptBlueprint` IS a `UBlueprint`, so
	 * `kind:level`'s `level_blueprint { }` has no grammar of its own: its body is this, indented one
	 * level. Extracted from `BuildProjection`'s Blueprint branch rather than copied, because the
	 * failure mode of a copy is precise and silent — the day a block is added to one, a Level
	 * Blueprint quietly stops carrying it and the sidecar still says `omits: none`.
	 *
	 * Takes the IR the caller already read rather than reading again: a Blueprint read is the
	 * expensive half of a projection, and two of them could not be proved identical anyway.
	 *
	 * @param Kind decides only which blocks another product owns; pass the kind the CALLER stamps.
	 */
	static bool BuildBlueprintBlocks(UBlueprint* Blueprint, const struct FBpIrBlueprint& Ir,
		const FGraphCodeProjectionOptions& Options, EGraphCodeAssetKind Kind,
		TArray<FString>& OutBlocks, FString& OutError);

	/**
	 * Take the blocks an absent product would have written OUT of a finished projection, and say so
	 * in `omits:` and in `errors { }`. The FACET-scope answer; a no-op on a complete install.
	 *
	 * After the projection rather than before it, deliberately. Core's writers do not know which
	 * blocks are somebody else's, and teaching them would put the ownership table in seventeen
	 * places. Removing a block that was already built costs one projection nobody reads and keeps
	 * the rule in one function.
	 *
	 * **PUBLIC SO A SPEC CAN HAND IT A DOCUMENT**, which is the only way this is testable at all
	 * here: the one Animation Blueprint in this project is untracked, so a spec that projected a
	 * real one would pass on this machine and fail on a fresh checkout. A synthetic document is the
	 * honest subject anyway — what this function promises is a transformation of text, not a
	 * reading of an asset.
	 */
	static void ApplyFacetGate(FGraphCodeDocument& Document);

private:
	/**
	 * The projection itself, with no product gate on it — every per-class branch lives here.
	 *
	 * Split out on 2026-08-07 so that `BuildDocument` is the ONE place a missing product is
	 * answered. Putting the gate inside the branches would have meant seventeen copies of one
	 * ruling, and the branch somebody added next would have had none.
	 */
	static bool BuildProjection(UObject* Asset, const FGraphCodeProjectionOptions& Options,
		FGraphCodeDocument& OutDocument, FString& OutError);
};
