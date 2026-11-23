#pragma once

#include "CoreMinimal.h"
#include "GraphCodeSidecarWriter.h"

/**
 * What a sync did. Named for the three states a caller has to be able to tell apart, because
 * "the file is fine now" and "the file was fine already" are different answers to the question
 * an editing session is really asking, which is "did something move under me".
 */
enum class EGraphCodeSyncStatus : uint8
{
	/** The sidecar on disk is already this build's projection of this asset. Nothing written. */
	Current,
	/** No sidecar existed. One was written. */
	Created,
	/** A sidecar existed and disagreed with the asset. It was rewritten. */
	Regenerated,
	/**
	 * The projection was built and handed back, and **no file was written, because sidecars are
	 * off** (`UGraphCodeSettings::bEnableSidecars`). S2.1, D1 of `GC-SETTINGS-DESIGN.md`.
	 *
	 * **THIS EXISTS BECAUSE THE OTHER THREE WOULD ALL BE LIES HERE.** `Created` and `Regenerated`
	 * say in their own doc comments that a file was written; `Current` says the file on disk is
	 * this projection, and with the switch off there may be no file at all. A read that quietly
	 * reported `current` would be the switch's absence of effect made invisible — and the whole
	 * defect being fixed was an effect nobody could see.
	 */
	Suppressed,
};

struct GRAPHCODE_API FGraphCodeSyncOptions
{
	/** Skip the fast path and re-project unconditionally. The slow answer is never wrong. */
	bool bForce = false;

	/** Report what a sync would do and write nothing. */
	bool bDryRun = false;
};

struct GRAPHCODE_API FGraphCodeSyncResult
{
	EGraphCodeSyncStatus Status = EGraphCodeSyncStatus::Current;

	FString PackageName;
	FString SidecarPath;
	FString KindToken;

	/** The envelope of the file that is on disk when this returns - or would be, under dry run. */
	FString Signature;
	FString BinaryDigest;
	TArray<FString> Omits;
	TArray<FString> Blocks;

	/**
	 * True when the answer came from the header alone, with the asset never loaded.
	 *
	 * This is the whole point of `bin:` (§14.3) and it is reported rather than inferred, because
	 * a caller that wants to trust a "current" needs to know which of the two proofs produced it:
	 * a matching binary digest, or an actual re-projection.
	 */
	bool bAnsweredWithoutLoading = false;

	/** The package was already loaded and dirty, so no honest `bin:` could be written. */
	bool bAssetDirty = false;

	/**
	 * The sidecar that was here had a stale `sig:` - it was a proposal in flight, and this
	 * regeneration discarded it.
	 *
	 * Not a refusal. Every save regenerates the sidecar with no exception (§14.3), so a sync
	 * that protected pending text would make the file a source of truth. It is reported because
	 * a caller that just wrote a patch and then synced deserves to be told its patch is gone.
	 */
	bool bDiscardedProposal = false;

	/** The `sig:` the file stated before this call. Empty when there was no file. */
	FString PreviousSignature;

	/** Why the fast path did not apply, when a sidecar existed and it was tried. */
	FString FastPathDeclined;

	bool bWouldWrite = false;

	/**
	 * Why no `.gc` was written, when `Status` is `Suppressed`. Empty otherwise.
	 *
	 * Two causes reach it and a caller has to be able to tell them apart: sidecars are off for the
	 * whole project, or this one folder is excluded. The first is a switch somebody flips back; the
	 * second is a line in a list, and the sentence names which line.
	 */
	FString SuppressedReason;

	/**
	 * The projection the slow path built — the text that was written, or that a dry run declined
	 * to write.
	 *
	 * It is reported because "what does this asset project to right now" is a question a caller
	 * can otherwise only answer by re-projecting, which is the expensive half of this call, or by
	 * reading the file back, which is only the projection when this call was allowed to write it.
	 *
	 * **Empty when the fast path answered**, and that is not a gap. The fast path returns only
	 * once the guards in §14.3 have established that the file on disk IS this build's projection,
	 * so there `SidecarPath` is the answer and it is already on disk.
	 */
	FString ProjectedText;
};

/**
 * Make one asset's `.gc` exist and be current, as cheaply as is honest.
 *
 * The point is that nothing downstream ever has to handle a missing or stale sidecar: gc_read
 * and gc_apply can assume the file describes the asset, because this is what they are called
 * through. The `bin:` fast path is a guarded shortcut, never a substitute for GcCheck - see
 * GC-SIDECAR-DESIGN §14.3 for why a digest-only check is safe here and wrong there.
 */
class GRAPHCODE_API FGraphCodeSync
{
public:
	static bool SyncPackage(
		const FString& AssetPath,
		const FGraphCodeSyncOptions& Options,
		FGraphCodeSyncResult& OutResult,
		FString& OutError);

	/** `/Game/UI/W.W`, `/Game/UI/W.W:Sub` and `/Game/UI/W` all name one package. */
	static FString PackageNameFromAssetPath(const FString& AssetPath);

	/**
	 * The package that HOLDS the actor an object path names — the one address the line above
	 * cannot answer. World Partition row 5, `GC-WORLD-PARTITION-DESIGN.md` §5.
	 *
	 * **THE ONE CASE WHERE A PATH DOES NOT SPELL ITS OWN PACKAGE.** Under One File Per Actor an
	 * actor's outer is still `<map>.<map>:PersistentLevel`, so its object path reads like the
	 * level's — but the bytes live in `__ExternalActors__/<map>/3/SB/T9VSAWRCA6OHTHHSPM87TY`, whose
	 * name is a content hash and contains the actor's name nowhere. Trimming the string, which is
	 * what every caller did before this existed, therefore answered with the LEVEL's package:
	 * MEASURED 2026-08-21, `gc_read` and `gc_apply` both returned a DIFFERENT asset's document for
	 * `…:PersistentLevel.FixtureSun`, canonical and with a valid `sig`, and said nothing about the
	 * substitution.
	 *
	 * **IT ASKS THE ASSET REGISTRY, WHICH ALREADY KNEW.** An external actor package is indexed with
	 * the actor's name and its object path, so this is one hash lookup and NO LOAD — the registry
	 * answers for an actor in an unloaded region exactly as it does for a loaded one. That is also
	 * why the answer is trustworthy on a partitioned map, where `TActorIterator` is not
	 * ([[a-query-that-answers-as-close-as-it-got]]).
	 *
	 * False for everything else, including an ordinary level's actor: that actor is stated by its
	 * level's own `actors { }`, so the level's package is the right answer and the caller's
	 * existing string trim already gives it. A component below the actor — `…FixtureSun.Light` —
	 * resolves to FixtureSun, because the document stating that component is the actor's.
	 *
	 * The saved half of the same address lives in the bridge (`gc-sidecar.js`), which has no
	 * registry and finds the file by scanning the level's external-actor `.gc` documents instead.
	 * Two instruments, one rule, and they are the same saved/live split every other GraphCode
	 * answer already has.
	 */
	static bool TryGetExternalActorPackage(const FString& AssetPath, FString& OutPackageName);

	static const TCHAR* StatusToken(EGraphCodeSyncStatus Status);
};
