// The batch — `RUNTIME-DRIVE-DESIGN.md` §17 P4, [USER, 2026-08-16]: *"later on the commands should
// be able to be batched together, so something like resume, inject, load, invoke, wait, invoke
// should be a standard for real ai game playtest."*
//
// ---------------------------------------------------------------------------
// WHY A BATCH EXISTS AT ALL: A SEQUENCE OF SINGLE CALLS IS NOT A SEQUENCE
// ---------------------------------------------------------------------------
//
// Each round trip is tens of milliseconds of jitter against a 16 ms frame, so *invoke, then invoke*
// asks for an order the game does not receive. A batch is handed over once and executed INSIDE the
// game, which is the only place frame-scale order exists. Everything else here follows from that.
//
// **It is a mode's parameter and not a second name.** `gcp_run mode:"invoke"` with one act is the
// batch of one; `steps:` is the plural. Spending a verb on the plural of an existing verb is the
// shape [[a-misplaced-capability-costs-a-name]] warns about.
//
// ---------------------------------------------------------------------------
// `wait` IS NOT A STEP KIND, AND THAT IS THE RULING
// ---------------------------------------------------------------------------
//
// **Time is an attribute of every step, not a kind of step** — [USER]: *"or, the invoke itself could
// have an optional timer"*. A step is **when** plus **what**; `When` is the when, `Act` is the what,
// and **a step with a `when` and no act IS the wait**, which is why `EGcPlayStepAct::Wait` is the
// zero value rather than a verb. One rule instead of two, and no step kind whose only job is to do
// nothing.
//
// **What a `when` may NOT be: a predicate over world state.** `until: Health < 50` is an
// *assertion*, and an assertion belongs in the graph, where it can see everything, runs every frame
// and survives packaging (§14.2). Admitting one here would rebuild the step-list runner §13
// deliberately expired — a batch is the driver's PACING, never the test.
//
// ---------------------------------------------------------------------------
// THREE PROPERTIES IT OWES, EACH ALREADY PAID FOR ONCE IN THIS PRODUCT
// ---------------------------------------------------------------------------
//
// 1. **Tick-driven, never a sleep.** The executor advances on the session ticker and yields between
//    steps, for the reason `Await` is an `FTSTicker` action rather than a latent one: a wait that
//    blocked the game thread would freeze the thing that has to release it. The game keeps running
//    while a step waits, or the wait cannot end.
// 2. **Every step carries a deadline, and the batch inherits the run's.** `TimeoutSeconds` is the
//    per-step one; with none, the step waits until §3.1b's session limit ends the run underneath it.
//    A batch parked forever is the hang the whole of §14.5 exists to catch.
// 3. **Every step reports, in order, in the log.** A batch whose failure is visible only as the
//    absence of a later step is a batch nobody can debug ([[a-deferred-act-owes-an-outcome]]). A
//    step that refuses STOPS the batch and says which one it was — later steps were written on the
//    assumption that the earlier ones happened.
//
// ---------------------------------------------------------------------------
// A STEP IS NOT A SECOND SPELLING OF AN ACT
// ---------------------------------------------------------------------------
//
// The vocabulary is the channel's own — `continue`, `end`, `load`, an inject and an invoke — and
// each step reaches the SAME function the single mode calls: `FGraphCodePlayControl::Post` for a
// resume the parked `Await` must consume, `UGraphCodePlaySession::RequestEnd`,
// `FGraphCodePlayLoad::Resolve` + `StageEntry`, `FGraphCodePlayInject::Run`,
// `FGraphCodePlayCall::Execute`. Nothing here decides what a verb means.
//
// **NOT A `USTRUCT`, for `FGcPlaySliceSpec`'s reason**: §3.1a's rule is about what a GRAPH authors,
// and a graph never authors a batch. This is the driver's pacing, built from JSON by the tool and
// executed by the session; no pin ever shows it.

#pragma once

#include "CoreMinimal.h"

/** What a step does when its `when` comes round. */
enum class EGcPlayStepAct : uint8
{
	/** Nothing at all — the `when` IS the step. See the header: `wait` is not a kind. */
	Wait,

	/** Answer a parked `Await`. Posted, because the park is the only thing that may consume it. */
	Continue,

	/** Ask the run to stop, through the same door a driver's `end` uses. */
	End,

	/** Branch: a state and the entry that plays it. Settles when the entry has actually fired. */
	Load,

	/** Put a `.gcp` back without playing anything — `gcp_apply`'s act, inside the sequence. */
	Inject,

	/** Call something the game already knows how to do. */
	Invoke,

	/** Photograph the game — §17 P7. Queued rather than awaited; see `RunStep`. */
	Shot,

	/** Press a control in the game's own UI — R7. `WBP_HUD.StartButton`, `arguments` the event. */
	Activate,
};

/** When a step fires. Exactly one form, and `Now` is the absence of one. */
enum class EGcPlayStepWhen : uint8
{
	/** As soon as the step before it settled. */
	Now,

	/** After N frames. **The only unit that means anything at frame scale.** */
	Frames,

	/** After S seconds of wall clock — for anything a person would have counted. */
	Seconds,

	/**
	 * **Until the game says so** — the rendezvous with the graph's own narration, and the reason a
	 * batch does not have to understand the game: the content already announces its own beats, and a
	 * driver waiting on one is reading the log it was going to read anyway.
	 */
	Mark,
};

/** One thing to do, and when. */
struct FGcPlayStep
{
	EGcPlayStepAct Act = EGcPlayStepAct::Wait;

	EGcPlayStepWhen When = EGcPlayStepWhen::Now;

	/** `When == Frames`. */
	int32 Frames = 0;

	/** `When == Seconds`. Wall clock, because a parked world must still be able to time out. */
	double Seconds = 0.0;

	/** `When == Mark`: the `Mark` name to wait for. The NEXT one of that name, not any already written. */
	FString Mark;

	/**
	 * What the act is addressed to — the park's name for `continue`, the capture for `load` and
	 * `inject`, `Actor.Function` for `invoke`. Unused by `end` and `wait`.
	 */
	FString Target;

	/** The rest: `load`'s entry, `invoke`'s named arguments, `end`'s comment. */
	FString Payload;

	/**
	 * WHY this step is in the sequence — [USER, 2026-08-17], the same field `FGcPlayCommand` gained.
	 *
	 * A batch is the shape where this matters most: a single `invoke` arrives beside a reply the
	 * agent was writing anyway, while eight steps handed over at once are read a minute later by
	 * somebody who has only the log. `Step(act=invoke, target=…)` says what; this says why it was
	 * step six.
	 */
	FString Comment;

	/**
	 * This step's own deadline, covering its wait AND whatever it hands to another consumer. Zero
	 * inherits the run's, which is the session limit — see the header's property 2.
	 */
	double TimeoutSeconds = 0.0;
};

/** How a batch stopped. Written as `result=` on the `BatchEnded` record. */
enum class EGcPlayBatchResult : uint8
{
	/** Every step ran. */
	Completed,

	/** A step refused, or its deadline came round. The detail names which and why. */
	Stopped,

	/**
	 * Nobody is left to have asked for it — the lease lapsed, or the run ended under it.
	 *
	 * A SEPARATE TOKEN FROM `Stopped` for `Await`'s reason (§3.1d): spelling an abandoned batch as a
	 * refused one would make a dead driver read as a broken step.
	 */
	Abandoned,
};

/**
 * A batch mid-flight. One per session, because the channel is one ordered queue with one order.
 *
 * `Index` is the step being run and `INDEX_NONE` means no batch — the same *absence is the state*
 * idiom `bFiring` uses, rather than a second bool that can disagree with it.
 */
struct FGcPlayBatchState
{
	TArray<FGcPlayStep> Steps;

	int32 Index = INDEX_NONE;

	/** The act has been performed and the step is waiting for its consumer to settle. */
	bool bActRun = false;

	/** Wall clock and frame at the moment this step began waiting — both `when` forms need one. */
	double BeganAt = 0.0;
	uint64 BeganFrame = 0;

	/** How many marks of `Step.Mark` had been written when this step began. The next one releases it. */
	int32 MarkCountAtStart = 0;

	/** A `continue` this step posted, still on the queue. Zero when nothing is outstanding. */
	int64 PostedId = 0;

	/**
	 * The lease that posted the batch.
	 *
	 * CHECKED EVERY TICK, not on the way in — the same continuous presence rule `Await` follows, and
	 * for the same reason: a driver that hands over six steps and dies has not asked for the last
	 * five ([[gc-play-driver-presence]]).
	 */
	FGuid Lease;

	bool IsActive() const { return Index != INDEX_NONE; }
};
