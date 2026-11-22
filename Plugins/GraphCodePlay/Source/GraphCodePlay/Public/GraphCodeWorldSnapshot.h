// GraphCode World — a running (or resting) world projected to text.
//
// Docs/RUNTIME-DRIVE-DESIGN.md §5 is the spec. The short version of why this is a SEPARATE
// artifact from a `.gc` sidecar: a sidecar names the `.uasset` it projects and carries the
// signature `gc_apply` checks before writing back, and a world snapshot has no asset and no way
// back in. Sharing the extension would mean either faking an `asset:` or relaxing the check that
// makes the write door safe (§5.1).
//
// So: `.gcp`, its own header, and nothing in GraphCodeApply will look at one.
//
// What IS shared is the value grammar, deliberately — a location must render the same here as in
// a sidecar or the two cannot be read side by side. The one divergence is FText, which writes its
// displayed string rather than its key: a sidecar carries `key=` because it must round-trip
// without orphaning a translation, and a snapshot is answering "what does the score say".

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeExecutionState.h"

class UWorld;

/** What to capture, and how precisely. */
struct GRAPHCODEPLAY_API FGraphCodeSnapshotOptions
{
	/**
	 * At least one, or the capture is refused.
	 *
	 * `class:BP_Door` (subclass-inclusive), `name:PlayerCharacter` (name or label),
	 * `tag:Interactable`, `near:PlayerCharacter,500`, or `*`.
	 *
	 * There is no default. A snapshot with no selector is "everything in the world", which is
	 * both enormous and the one shape whose diff is guaranteed to be unreadable.
	 */
	TArray<FString> Selectors;

	/**
	 * Quantisation for every exported float. §5.8: default 0.01, powers of ten only.
	 *
	 * Ten to the minus six is the floor, and one is the ceiling. Both ends are arbitrary and
	 * both are wide enough that hitting one means the caller wanted something else.
	 */
	double Precision = 0.01;

	/**
	 * The RUN's start time, written into `run:` so the file says which playthrough produced it.
	 *
	 * Supplied by the caller rather than read from a global here, because a capture outside a session
	 * legitimately has no run and the writer must not invent one — an empty field means *not part of
	 * a run*, which is a different statement from *a run I could not identify*.
	 */
	FString Run;

	/**
	 * Turn on every reflected property instead of the §5.4 rule.
	 *
	 * CALL-LEVEL, not per-selector as §5.4 first had it. Selectors union, so an actor can match
	 * two of them, and a per-selector flag would give that actor two different answers with no
	 * principled tie-break. One switch for the call has no such case.
	 */
	bool bAllProperties = false;

	/**
	 * Properties to surface WHEN THEY DIFFER, past the category rules that would have hidden them.
	 *
	 * **The setting between "the visible set" and "everything" — user-directed 2026-08-14.** The
	 * real question is usually *show me Velocity*, and `Velocity` is transient, so before this the
	 * only spelling was `properties:all` — 2,915 properties to answer a question about one.
	 *
	 * Lifts transient, deprecated, editor-only and non-visible for the named property; the deviation
	 * rule still applies. A resting world writes nothing, which is what keeps a fifty-actor diff
	 * readable — absence here means *it was at rest*.
	 */
	TArray<FString> Show;

	/**
	 * Properties to write unconditionally, deviating or not.
	 *
	 * **A different question from `Show`, and the caller picks** — user-directed: *"I should be able
	 * to decide whether I should show ALL velocity, no matter default or not."*
	 *
	 *   `Show`        surfaces it when it changed — the reading for a DIFF.
	 *   `AlwaysShow`  surfaces it regardless      — the reading for a REPORT.
	 *
	 * Under `AlwaysShow` the line cannot go missing, so every actor is present to be compared and a
	 * table can be built from the file. Beats every rule, and beats `Show` when a name is in both.
	 *
	 * Both are bare names, matched against actors and components alike. **Whole name, never a
	 * substring** — `Velocity` does not pick up `LinearVelocity` — and **case-insensitive**, because
	 * these become `FName`s and FName comparison folds case. A name that matched nothing anywhere
	 * warns rather than passing silently — the same instinct that echoes `select:` into the
	 * envelope, since silence must not mean both "not there" and "you misspelled it".
	 */
	TArray<FString> AlwaysShow;

	/**
	 * What a running character DECIDED — its blackboard, its Behavior Tree's active branch, and its
	 * animation state machines (§20).
	 *
	 * **CATEGORIES IN THE `transient` FAMILY, DEFAULTING TO ON** — [USER, 2026-08-16]: *"they
	 * belongs to same family as transient, but by default will show."* That ruling is why there is
	 * no `execution:` line: `omits:` was already the echo, so a category absent from it is one the
	 * file carried, and the three names appear on `always:` when carried and on `omits:` when not.
	 *
	 * On by default because a capture exists to be compared, and an empty diff over a fight that
	 * reloaded into a different AI state is the false green this whole row exists to close.
	 */
	FGraphCodeExecutionStateOptions Execution;
};

/** The text, where it went, and enough counts to see a mis-typed selector at a glance. */
struct GRAPHCODEPLAY_API FGraphCodeSnapshotResult
{
	FString Text;

	/** Absolute path written, empty when the caller only asked for the text. */
	FString Path;

	/** The body, without the envelope — what two snapshots of an unchanged world must share. */
	FString BodyDigest;

	int32 ActorCount = 0;
	int32 PropertyCount = 0;

	/** The map identity, PIE prefix already removed (§5.6). */
	FString WorldPath;

	/** The PIE instance, empty for an editor-world snapshot. */
	FString PieInstance;

	/** Non-fatal notes: `*` was used, a selector matched nothing, a value had no spelling. */
	TArray<FString> Warnings;
};

/** §5's writer. Reads a world; never writes one. */
class GRAPHCODEPLAY_API FGraphCodeWorldSnapshot
{
public:
	/** Project World through Options. Fails only on a bad selector, bad precision, or no world. */
	static bool Capture(UWorld* World, const FGraphCodeSnapshotOptions& Options,
		FGraphCodeSnapshotResult& OutResult, FString& OutError);

	/**
	 * Capture and write under Saved/GraphCode/snapshots.
	 *
	 * Under `/Saved/` because a snapshot may hold anything the game holds - save data, a name, a
	 * credential somebody put in a variable - and `.gitignore` already excludes that whole tree.
	 */
	static bool CaptureToFile(UWorld* World, const FGraphCodeSnapshotOptions& Options,
		const FString& SnapshotName, FGraphCodeSnapshotResult& OutResult, FString& OutError);

	/**
	 * Powers of ten from 1 down to 0.000001.
	 *
	 * Anything else is refused with the nearest legal value named, rather than snapped silently:
	 * a caller who asked for 0.25 and got 0.1 would read every subsequent diff against a
	 * precision they did not choose.
	 */
	static bool ValidatePrecision(double Precision, FString& OutError);

	/** Decimal places a precision prints at. 0.01 -> 2. */
	static int32 DecimalsFor(double Precision);

	/** The directory CaptureToFile writes into. Created on demand. */
	static FString SnapshotDirectory();

	/**
	 * The map identity §5.6 writes into `world:` and `pie:` — the EDITOR path, prefix separated.
	 *
	 * Public because RESTORE asks the same question from the other side: a file names a world, and
	 * writing it into a different one destroys and spawns against a map the capture never saw. That
	 * check has to resolve through this function rather than through a second copy of the prefix
	 * rule, or the two answers drift and neither errors.
	 */
	static void WorldIdentity(const UWorld* World, FString& OutWorldPath, FString& OutPieInstance);
};
