// The control channel — `PLAY-SESSION-DESIGN.md` §3.1, §3.5, `RUNTIME-DRIVE-DESIGN.md` §17 item 4.
//
// **THE AI TALKING BACK.** Everything before this was one-way: the game narrates, an agent reads. This
// is the return path — `continue` answers a parked `Await`, `end` asks the run to stop, `load` branches
// it. §5 ruled there is **one channel and not one per verb**: *"no second mechanism — the rule was
// always about the transport, not the number of verbs on it."*
//
// ---------------------------------------------------------------------------
// THREE RULES, AND THE FIRST IS THE ONE THAT MAKES THE GATE REAL
// ---------------------------------------------------------------------------
//
// **1. ONLY AN ATTACHED DRIVER MAY POST.** This is where §3.1d's third state stops being a label and
// starts being enforcement. *Driver-attached* is the only state that may pause or mutate a world, and
// every pause and every mutation arrives through here — so the lease check on `Post` IS that rule,
// rather than a separate thing that has to be remembered at each call site.
//
// **2. A COMMAND IS STAMPED WITH THE LEASE THAT POSTED IT, AND A COMMAND FROM A LAPSED LEASE IS
// DROPPED RATHER THAN RUN.** This is [USER]'s disconnect ruling (§3.1d) applied to instructions and
// not only to parking. A driver that says *continue* and then dies has not continued anything — its
// instruction died with it, and the alternative is a game that resumes minutes later on the word of
// something that is no longer there to see what happens next. It also disposes of the stale-command
// problem without inventing a TTL: the lease already has one.
//
// **3. TAKE IS ONE-SHOT AND ORDERED.** A command consumed twice is a second `continue` releasing a
// park nobody answered. FIFO, because `continue` then `end` must not arrive as `end` then `continue`.
//
// ---------------------------------------------------------------------------
// WHAT THIS DELIBERATELY DOES NOT DECIDE
// ---------------------------------------------------------------------------
//
// **Whether a `continue` matches the park it was meant for.** A command carries a `Target` and this
// layer never interprets it — matching belongs to `Await`, which is the only thing that knows what is
// parked and what it is called. Ruling it here would be inventing park identity before the node that
// owns it exists, and the danger it guards against (a stale `continue` releasing the *next* park) is
// already most of the way handled by rule 2.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodePlayBatch.h"
#include "GraphCodePlayDriver.h"

#include "GraphCodePlayControl.generated.h"

/** What a driver can say. Three, per §3.5 — the third round added `Load` to `continue` and `end`. */
UENUM(BlueprintType)
enum class EGcPlayCommand : uint8
{
	/** Answer a parked `Await` and let the graph run on. */
	Continue,

	/** Ask the run to stop. A REQUEST, not a kill: §5's one-writer rule means the game closes its own
	 *  log, so a driver that ended it directly would be a second writer on an append-only file. */
	End,

	/** Branch: end the current slice and enter another from a given state (§3.5). */
	Load,

	/**
	 * Make the game do one thing it already knows how to do: `Target` is `Actor.Function`, `Payload`
	 * the named arguments.
	 *
	 * **THE FOURTH VERB, AND IT IS ON THIS CHANNEL RATHER THAN BESIDE IT FOR ONE REASON — ORDER.**
	 * An invoke could have run where it was typed, since the tool handler is already on the game
	 * thread. But `continue`, `end` and `load` are consumed a tick later, so an invoke that ran
	 * immediately would happen BEFORE the resume it was written after — and the sequence this exists
	 * to serve is *resume, inject, load, invoke*. One channel, one order (§5).
	 *
	 * The lease rules above are the whole safety model for it, unchanged and not extended: an
	 * unattached poster is refused, a lapsed one is dropped. P4 asked for an allowlist of callable
	 * functions; that was written before the lease existed, and a second gate in front of the same
	 * door is not a safety measure.
	 */
	Invoke,

	/**
	 * Several of the above, in order, executed inside the game — `GraphCodePlayBatch.h`.
	 *
	 * **THE FIFTH VERB IS HERE FOR THE FOURTH'S REASON, ONE STEP FURTHER.** An invoke is on this
	 * channel so it keeps its place in *resume, inject, load, invoke*; a batch is on it because a
	 * sequence of single calls is not a sequence at all — each round trip is tens of milliseconds
	 * against a 16 ms frame. Handed over once, this is the only form in which frame-scale order
	 * survives the transport.
	 *
	 * `Steps` carries it, and `Target`/`Payload` are unused: a batch is addressed to nothing, because
	 * every step carries its own address.
	 */
	Batch,

	/**
	 * Photograph the game as it is right now — §17 P7, **[USER, ruled 2026-08-17]**: *"it take
	 * pictures at a mark, or from command from the ai."*
	 *
	 * **THE SIXTH VERB, AND IT EXISTS BECAUSE THE AUTHOR CANNOT PREDICT THE MOMENT.** A `Mark` with
	 * `Shot` covers every picture the graph knew to want; this covers the ones only a reader wants,
	 * and those are usually the interesting ones — an agent that has just read a delta and wants to
	 * see the thing it describes. Neither replaces the other.
	 *
	 * **IT IS ON THIS CHANNEL FOR `Invoke`'S REASON — ORDER.** *Resume, look, load* has to mean what
	 * it says, and a shot taken where it was typed would photograph the world before the resume it
	 * was written after. `Target` names the picture, so a run's images are as findable by name as its
	 * captures.
	 *
	 * **AND NOT A `gc_render` CALL.** That photographs an EDITOR viewport and answers to nobody's
	 * run: it writes no record, lands nowhere near the log, and is invisible to `collect`. A picture
	 * that is evidence has to be in the folder the evidence lives in.
	 */
	Shot,

	/**
	 * Put a captured state back with NO entry event — a `Load` that changes the world and tells
	 * nothing to play it.
	 *
	 * **THE SEVENTH VERB EXISTS BECAUSE ITS ABSENCE WAS AN ACCIDENT, NOT A RULING.** `EGcPlayStepAct`
	 * has carried `Inject` since the batch shipped, so a SEQUENCE could always inject and a single
	 * call never could — reachable only by writing a one-step batch. Nobody decided that; it fell out
	 * of the surface keeping two vocabularies for one idea, and it surfaced the moment those were
	 * folded into `execute act:` ([[one-concept-two-grammars]]).
	 *
	 * `Target` is the capture and `Payload` is unused: an inject IS the load without the event, and
	 * the missing event is the whole of the difference between them.
	 */
	Inject,

	/**
	 * Press a control in the game's own UI — R7, **[USER, 2026-08-19]**: *"i would want to invoke the
	 * umg buttons from command also."*
	 *
	 * **THE EIGHTH VERB, AND IT IS THE HALF OF LIVE UI A PICTURE COULD NOT DO.** A `Shot` already
	 * shows the player's HUD exactly as they see it — `bShowUI` is true and the engine composites the
	 * game's own Slate window — so an agent could always LOOK at a menu and never press one. A run
	 * that can open a door but not a main menu cannot test the first thing a player touches.
	 *
	 * **IT IS ON THIS CHANNEL FOR `Invoke`'S REASON — ORDER.** *Resume, press, look* has to mean what
	 * it says, and a press that ran where it was typed would happen before the resume it was written
	 * after.
	 *
	 * **AND NOT `Invoke`, though both make the game do something it already knows how to do.** An
	 * invoke calls a function by reflection and reaches any `UObject`; a press asks whether a PLAYER
	 * could have done it — realised, visible, hit-testable, enabled, big enough to hit — and refuses
	 * when they could not. Folding them would lose that question, which is the only thing separating
	 * *the menu works* from *the handler works* ([[an-impure-node-read-as-value]] read the other way:
	 * two verbs that differ in what they PROVE are two verbs).
	 *
	 * `Target` is `WBP_HUD.StartButton` (the host is optional), `Payload` the event name when it is
	 * not `OnClicked`.
	 */
	Activate,
};

/** One thing a driver said, and which driver said it. */
USTRUCT(BlueprintType)
struct FGcPlayCommand
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	EGcPlayCommand Verb = EGcPlayCommand::Continue;

	/**
	 * What it is addressed to — the parked `Await`'s name for `Continue`, the state to enter for
	 * `Load`, empty for `End`. **Never interpreted here**; see the header's last block.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FString Target;

	/** Anything else the verb carries. Opaque to this layer. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FString Payload;

	/**
	 * WHY the driver did this — **[USER, ruled 2026-08-17]**: *"any decisions made by ai, like
	 * continue, load, load + execute, inject etc. can be commented."*
	 *
	 * **A RUN RECORDS WHAT THE AGENT DID AND, WITHOUT THIS, NEVER WHY.** `Continued(name="checkpoint")`
	 * says a park was answered; it cannot say *because the door was already open, so the key is not
	 * the blocker*. Six months later the reasoning is the half nobody can reconstruct, and it is the
	 * half a person reading the run most needs. It goes on the act's OWN LINE for that reason: the
	 * log is what anybody reads, so a decision and its reason have to arrive together there rather
	 * than be assembled by something downstream.
	 *
	 * **PROSE, NEVER A FIELD ANYTHING BRANCHES ON.** Same split as `Mark`'s `Comment` against its
	 * `Values`: this is read once by a person, so nothing may parse it, and a driver that wants a
	 * comparable fact puts it in the game rather than in a sentence.
	 *
	 * **IT IS NOT `Payload`, though `end` already used that field for exactly this.** A comment on
	 * `end` was reachable only because `end` addresses nothing and needs no payload; `load` and
	 * `invoke` both carry theirs, so the reason had nowhere to go. One field for *what to do* and
	 * another for *why* is what makes the second one available on every verb rather than the ones
	 * with a spare slot ([[a-required-parameter-is-per-route]] read the other way round).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FString Comment;

	/** The lease that posted it. A command outlives its driver by exactly nothing. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FGuid PostedBy;

	/**
	 * Which post this was, counted from the start of the process. Zero is *never posted*.
	 *
	 * **IT EXISTS SO A POSTER CAN ASK WHETHER ITS COMMAND HAS BEEN CONSUMED**, which nothing needed
	 * until the batch: a batch's `continue` step is finished when the parked `Await` has TAKEN it,
	 * not when it was queued, and *resume, then invoke* means the invoke waits for the resume. The
	 * queue's length cannot answer that — another driver's command, or the batch's own next post,
	 * moves it for reasons that have nothing to do with this one ([[silence-is-not-sameness]]).
	 *
	 * Not a GUID: nothing outside this process ever names a command, and an ordinal is also the
	 * FIFO order it was accepted in.
	 */
	int64 Id = 0;

	/**
	 * `Batch` only: what to do, in order. Empty for every other verb.
	 *
	 * **NOT A `UPROPERTY`, and `FGcPlayStep` is not a `USTRUCT`** — see `GraphCodePlayBatch.h`. A
	 * graph never authors a batch, so reflecting it would buy a pin nobody can place and cost a
	 * second grammar for the value.
	 */
	TArray<FGcPlayStep> Steps;
};

/**
 * The one channel, in both directions of use: a driver posts, the game takes.
 *
 * Process-wide for the same reason the lease is — a driver drives a process, and a session outlives a
 * level load. Neither end may assume a world.
 */
class GRAPHCODEPLAY_API FGraphCodePlayControl
{
public:
	/**
	 * Say something. Refused, with a reason, when no driver holds the lease.
	 *
	 * The refusal is not a courtesy: an unattached poster is either a bug or the second agent on this
	 * machine, and both should be told rather than silently queued behind somebody else's run.
	 */
	static bool Post(EGcPlayCommand Verb, const FString& Target, const FString& Payload,
		const FString& Comment, FString& OutError, int64* OutId = nullptr);

	/**
	 * The same door with no reason given.
	 *
	 * **AN OVERLOAD RATHER THAN A DEFAULT ARGUMENT, because the defaultable slots are taken** —
	 * `OutError` and `OutId` are trailing, so a comment could only go before them. Two spellings of
	 * one call is a cost; the alternative was making every existing caller pass an empty string to
	 * say nothing, which is how a field that should be rare becomes noise at every site.
	 */
	static bool Post(EGcPlayCommand Verb, const FString& Target, const FString& Payload,
		FString& OutError, int64* OutId = nullptr);

	/**
	 * Post a whole sequence as ONE command, so the order in it is the order the game receives.
	 *
	 * Same lease rules as `Post` and no others: this is not a privileged door, it is the same door
	 * carrying more at once. An empty sequence is refused rather than accepted as a no-op — a caller
	 * that sent no steps meant something, and running nothing successfully is the reply least likely
	 * to be what they meant.
	 */
	static bool PostBatch(const TArray<FGcPlayStep>& Steps, FString& OutError, int64* OutId = nullptr);

	/**
	 * Is that command still waiting to be taken?
	 *
	 * False for a command that was consumed AND for one whose lease has lapsed, because both mean the
	 * same thing to the only caller there is: nobody is going to act on it now.
	 */
	static bool IsPending(int64 Id);

	/**
	 * Take the next command the CURRENT driver posted, oldest first.
	 *
	 * Commands stamped by a lapsed lease are discarded as they are reached — a dead driver's
	 * instructions are not executed. Returns false when nothing is waiting.
	 */
	static bool Take(FGcPlayCommand& OutCommand);

	/**
	 * Look at the next command without consuming it.
	 *
	 * IT EXISTS SO A CONSUMER CAN DECLINE, which rule 3 otherwise makes impossible: `Take` is the only
	 * way to see a command and it removes it, so a parked `Await` that took an `end` off the front
	 * would have swallowed a verb it cannot execute. Peek, decide, and take only what is yours — the
	 * order is never disturbed, so a command this build has no consumer for is still at the front for
	 * the one that arrives later.
	 *
	 * Discards lapsed-lease commands on the way, exactly as `Take` does: a peek must not report a
	 * command that a take would then refuse to hand over.
	 */
	static bool Peek(FGcPlayCommand& OutCommand);

	/** How many commands from the live driver are waiting. Lapsed ones do not count. */
	static int32 NumPending();

	/** Drop everything. Test teardown, and the same leak risk as the lease: process-wide state. */
	static void Reset();
};
