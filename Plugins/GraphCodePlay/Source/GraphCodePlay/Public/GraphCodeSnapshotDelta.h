// What changed between two states — `PLAY-SESSION-DESIGN.md` §4.5.
//
// The rolling channel this feeds: Play keeps `previous.gcp` and `current.gcp`, both whole unfiltered
// captures, and at every wake computes the delta between them. The AI is handed the delta and never
// the state, which is what makes a whole-world capture per wake affordable at all — the file on disk
// is large and the thing that reaches a context window is small.
//
// ---------------------------------------------------------------------------
// THREE THINGS THIS FILE IS NOT
// ---------------------------------------------------------------------------
//
// **1. Not a patch.** A delta is evidence and §18 forbids it as input to any write. It is never
// applied to anything, and `previous.gcp`/`current.gcp` are each recomputed whole rather than being
// a base plus accumulated deltas — the standard incremental-state trap, where one dropped or
// misordered delta leaves a baseline confidently describing a world that never existed.
//
// **2. Not filtered.** The states it reads are whole-world by user ruling, so a delta is whole-world
// by construction. Its smallness comes from the world being mostly static, not from someone having
// guessed in advance which objects matter — which is the difference between a cheap report and a
// lucky one. A filter is an authored expectation, and this channel exists for the case where nobody
// has one yet.
//
// **3. Not an account of what the game DID.** It holds two sets of objects and compares them. It
// never saw a call, so it says `new` and `gone` rather than `spawned` and `destroyed`: a streaming
// level load, a deferred spawn finishing registration, or a capture-policy shift all produce an
// appearance with no `SpawnActor` anywhere. The session log's `Load(spawned=2, destroyed=1)` may use
// the other words because the restore acted; this only watched.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeSnapshotDocument.h"
#include "GraphCodeValueDelta.h"

/** How to label the result. The comparison itself takes no options — it is a diff, not a query. */
struct FGcpDeltaOptions
{
	/**
	 * What goes in `from:` — normally the previous state's filename.
	 *
	 * Required, because a delta whose absent lines mean *unchanged since* must name what since. The
	 * reader refuses one without it, so this is checked here rather than producing a document that
	 * cannot be read back.
	 */
	FString From;

	/**
	 * How a changed value is spelled — §4.5c.
	 *
	 * **NO DEFAULT, on purpose.** The two producers differ (`Arrow` for `gc_play mode:"diff"`,
	 * `Signed` for every `ping_NN.gcp`) and a default would let a caller inherit the other one's
	 * audience silently. There are three real call sites; each says which it is.
	 */
	EGcpValueDeltaStyle Style = EGcpValueDeltaStyle::Arrow;
};

/**
 * A property that went back to its archetype default between the two states.
 *
 * **[PROP] — the one part of §4.5 the user has not ruled on, and it is unrepresentable without a
 * decision.** Both states are `baseline: archetype`, so a deviating property is written and a
 * default one is absent. A property present in `previous` and absent in `current` therefore did
 * change — it returned to default — but the state file never wrote the value it returned to, so the
 * delta cannot quote it.
 *
 * Dropping it silently is the one option that is definitely wrong: a boss's Health falling back to
 * its class default would be invisible in the very channel built to report change. So it is written
 * as its name with this token as the value, and it is flagged rather than assumed settled.
 */
#define GCP_DELTA_DEFAULT_TOKEN TEXT("<default>")

/**
 * What the two captures' SCOPES cost the comparison — filled only when they differed.
 *
 * **[USER, 2026-08-17] ruled the differ filter-aware rather than shape-refusing:** *"consider all the
 * filters. like based on first filter, if second gcp has actor that fits the filter but not in first,
 * then it's a new. but if it didn't fit the filter of the first, it's omit."* So a scope difference
 * is no longer a reason to refuse a pair; it is a reason to decide each absence against the OTHER
 * capture's `select:` instead of against nothing.
 *
 * These counts exist because the tool that reports the delta must not re-derive them. It holds both
 * documents and could walk them again, but a second implementation of *what is in scope* is the
 * defect `FGraphCodeWorldSelection` was lifted out to prevent — capture and restore disagreeing
 * silently. The differ answers, and the reply quotes it.
 */
struct FGcpDeltaScopeReport
{
	/** Entries only one file could have held, dropped because the other never looked there. */
	int32 OutOfScope = 0;

	/**
	 * Entries dropped because the other capture's scope could not be judged from a file at all —
	 * a `tag:` or `near:` selector, or a class path naming a class that is not loaded.
	 *
	 * Counted apart from `OutOfScope` because they are a different claim: one says *that scope
	 * excludes this*, the other says *nobody can tell from here*. Both drop the entry, since
	 * asserting an appearance that might be a filter artifact is the failure this whole change
	 * exists to stop — but only one of them is a limit worth reporting as a limit.
	 */
	int32 Undecidable = 0;

	bool IsClean() const { return OutOfScope == 0 && Undecidable == 0; }
};

/** §4.5's differ. Reads two documents; writes nothing anywhere. */
class FGraphCodeSnapshotDelta
{
public:
	/**
	 * Compare Current against Previous and build the delta.
	 *
	 * Refuses rather than producing a misleading answer whenever the two are not comparable — see
	 * `AreComparable`. A delta computed across a precision change or an `omits:` change is not a
	 * smaller truth, it is a wrong one, and it looks exactly like a right one.
	 *
	 * **A differing `select:` is NOT one of those refusals** — it is handled, per `FGcpDeltaScopeReport`.
	 * `OutScope` is optional and stays untouched when the two scopes match, which is every call the
	 * rolling channel makes.
	 */
	GRAPHCODEPLAY_API static bool Compute(const FGcpDocument& Previous, const FGcpDocument& Current,
		const FGcpDeltaOptions& Options, FGcpDocument& OutDelta, FString& OutError,
		FGcpDeltaScopeReport* OutScope = nullptr);

	/**
	 * Whether two states can be subtracted at all, with the reason when they cannot.
	 *
	 * Exposed separately because the caller that rotates the state files wants to know *before*
	 * capturing that the answer will be usable.
	 */
	GRAPHCODEPLAY_API static bool AreComparable(const FGcpDocument& Previous,
		const FGcpDocument& Current, FString& OutError);

	/**
	 * Nothing changed — the assertion that a delta's empty body makes.
	 *
	 * **IT IS A QUERY AND NOT A DETECTOR, and that distinction is a ruling rather than a nicety**
	 * [USER, 2026-08-15]. An earlier draft of §4.5 made an empty delta the product's stall detector:
	 * a ping that found nothing changed would escalate into a park and write a `Stalled` record. The
	 * user deleted the whole apparatus — *"too long didnt reach next mark itself is a signal. let
	 * the ai judge"* — so **nothing in Play asks this question of a running game.** Overrun is the
	 * signal, the delta is the evidence, and the verdict belongs to whoever reads it.
	 *
	 * What that dissolved rather than answered: a world with an always-ticking reflected value — a
	 * clock actor, an accumulating score — against which no delta is ever empty. It was §4.5's one
	 * open question while a machine had to judge; a reader who can see the clock ticking never
	 * needed the definition.
	 *
	 * The live caller is the restore's acceptance test — *put a state back, re-capture, require the
	 * diff empty* — where both inputs are known and "empty" means exactly what it says.
	 */
	GRAPHCODEPLAY_API static bool IsEmpty(const FGcpDocument& Delta);
};
