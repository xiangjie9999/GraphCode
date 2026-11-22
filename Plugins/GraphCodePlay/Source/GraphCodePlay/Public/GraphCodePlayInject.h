// Putting a `.gcp` back — the ACT under `gcp_apply`, lifted so a batch step can reach it.
//
// ---------------------------------------------------------------------------
// WHY THIS FILE EXISTS, AND IT IS THE THIRD TIME THIS PROJECT HAS DONE IT
// ---------------------------------------------------------------------------
//
// `gcp_apply` had the whole sequence in an editor-side translation unit: find the file, parse it,
// check that the capture and the running world are the same map, restore. A batch's `inject` step
// needs the identical four, and it lives in the RUNTIME module where none of that is reachable —
// so it moved down rather than being copied. The same move that produced
// `FGraphCodePlayLoad::ResolveCapturePath` (the path rule), `FGraphCodePlayCall::Resolve` (the
// `Actor.Function` rule) and `FGraphCodeWorldSnapshot::WorldIdentity` (the PIE-prefix rule), for the
// same reason each time: **two answers to one question drift with nothing erroring**
// ([[a-read-answers-through-the-write-path]]).
//
// The map check is the one that would have hurt. A capture of one level applied to another destroys
// what is in scope and spawns a world that was never loaded, and **the result reads exactly like a
// successful restore** — so a second copy of that rule is not a duplicated sentence, it is a second
// chance to lose a level.
//
// ---------------------------------------------------------------------------
// WHAT DID NOT MOVE: THE WORDING
// ---------------------------------------------------------------------------
//
// This returns a FAULT and each caller renders its own sentence, because `gcp_apply` is answering a
// person who typed a name and the batch is writing a log record inside a running game. **Branch on
// the fault, never on who is asking** ([[branch-on-data-not-on-caller-identity]]).
//
// **The PIE-only refusal stays with the tool**, deliberately: `GEditor->PlayWorld` is editor-only,
// and the rule *the editor world is the level package and is written by `gc_apply kind:"level"`*
// is about which TOOL a caller wants. Inside a running game there is no other world to confuse it
// with.
//
// **And the parse still answers before the world does.** A caller whose document is malformed AND
// whose game is not running should learn both in the order they can act on, and a headless spec has
// to be able to reach the parse refusal with no PIE to get past. That ordering was `gcp_apply`'s;
// it now lives here, where both callers inherit it rather than one of them re-deciding it.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeSnapshotRestore.h"

class UWorld;

/** Why an inject did not happen. `None` means the restore ran — check `Plan.bApplied` for what it did. */
enum class EGcPlayInjectFault : uint8
{
	None,

	/** Neither a capture name nor a document, or both. */
	NoSource,

	/** A name that resolves to no file. */
	NotFound,

	/** It is not a readable `.gcp`; `Errors` holds every fault, not the first. */
	Unreadable,

	/** Nothing is playing. */
	NoWorld,

	/** The capture is of another map. The one refusal here that cannot be read back out of a result. */
	WrongMap,

	/** The restore itself refused; `Plan.Errors` says why. */
	Refused,
};

/** What to put back, and how much of the world it is allowed to touch. */
struct FGcPlayInjectRequest
{
	/** A name as passed to `gcp_capture name:`, or a path. Give this or `Document`, never both. */
	FString Capture;

	/** A whole `.gcp` as text, for a state that was never captured. */
	FString Document;

	/** The WRITE SCOPE — what may be destroyed for not being in the file. Empty takes the file's own. */
	TArray<FString> Selectors;

	bool bReconcileExistence = true;
	bool bSuppressLifecycle = false;
	bool bDryRun = false;

	/**
	 * Editor-side resolver for `@anchor` / `@surface` / `@facing` — §20.9, and null is the ordinary call.
	 *
	 * Passed straight through to `FGcpRestoreOptions`, which is where the whole contract is documented.
	 * It rides on the REQUEST rather than being found by the act, because the act is Runtime and the
	 * resolver is not: this is the caller saying *I am in an editor and I can answer this*, which is
	 * exactly the same shape as the `PlayWorld` the tool resolves one field above it.
	 */
	const class IGcPlayPlacementResolver* PlacementResolver = nullptr;
};

/** What happened, in the shape both callers report from. */
struct FGcPlayInjectOutcome
{
	EGcPlayInjectFault Fault = EGcPlayInjectFault::None;

	/** The path, or *the document passed inline* — whatever a message about this call should name. */
	FString SourceLabel;

	/** The capture's map and the live one. Both filled on `WrongMap`, which is the whole of that refusal. */
	FString CaptureWorld;
	FString LiveWorld;
	FString LivePieInstance;

	/** Parse faults on `Unreadable`. The restore's own live in `Plan.Errors`. */
	TArray<FString> Errors;

	/** The file's `select:`, so a caller can echo the scope a destroy list actually came from. */
	TArray<FString> FileSelectors;

	FGcpRestorePlan Plan;
};

class GRAPHCODEPLAY_API FGraphCodePlayInject
{
public:
	/**
	 * Find it, read it, check the map, restore it — refusing at the first thing that does not hold,
	 * with nothing touched.
	 *
	 * `World` may be null: everything up to the map check still answers, which is what keeps a parse
	 * refusal reachable with no game running.
	 */
	static bool Run(UWorld* World, const FGcPlayInjectRequest& Request, FGcPlayInjectOutcome& Out);
};
