// The session layer — `PLAY-SESSION-DESIGN.md` §3.1 and §5, `RUNTIME-DRIVE-DESIGN.md` §17 item 3.
//
// THE GAME NARRATES ITSELF. A graph calls `Begin Play Session`, then `Mark` at the moments it
// decides matter, then `End Play Session`; what comes out is one append-only text file an agent can
// tail while the run is still going. That file IS the announcement channel and P2's transcript —
// §9 ruled them one artifact, because two accounts of one run can disagree.
//
// ---------------------------------------------------------------------------
// WHAT THIS LAYER IS NOT, so the next scope does not re-litigate it
// ---------------------------------------------------------------------------
//
// `Await`, the control channel and explicit slices are **item 4**, not this one. What is here is the
// half that needs nothing to answer back: a session opens, moments accumulate, the session closes.
// Nothing in this file blocks, so nothing in it can hang a run.
//
// ONE SLICE IS STILL WRITTEN, AND THAT IS DELIBERATE RATHER THAN A STUB. §3.6a ruled `Slice(…) { }`
// the log's ONE block kind and made `Start`/`End` session lines, on the argument that **every slice
// is a load and the first slice's `from:` is the map**. So a session with no explicit slices is not
// a session with no slice — it is one slice whose starting state is the level that was loaded. This
// writes that, which means item 4 adds `Begin Slice` by closing one brace and opening another rather
// than by changing the grammar of every file written before it.
//
// ---------------------------------------------------------------------------
// THE RULES THAT DECIDE THE CODE, all from §5
// ---------------------------------------------------------------------------
//
//   the newline is the commit   a line exists only when its terminating newline does, which is one
//                               rule for a torn line from a crash, a half-flushed append, and
//                               "has the session started yet". Every write here is a whole line,
//                               flushed.
//   one writer: the game        a forced end goes in a SIBLING file. Two writers on an append-only
//                               file is a torn-line generator, and the case you need it most is
//                               when the game is wedged.
//   no ordinal key              `frame`, `time` and usually `name` are real identity; file order is
//                               the order. An ordinal dies at branching anyway (§5.13).
//   the family's formatter      `AuthoringCore::Quote` and `QuoteIdentifier`, never a hand-rolled
//                               `name="…"`, or the log and the capture drift on escaping with
//                               nothing erroring.
//   the session is on the       or it dies at the first level load. Hence a GameInstance subsystem
//   GameInstance                rather than a world one, and hence the `Map` record.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Ticker.h"
#include "GraphCodePlayBatch.h"
#include "GraphCodePlayLoad.h"
// The record writer is held by value in a `TUniquePtr`, so the session's own destructor needs its
// complete type — a forward declaration compiles here and fails in the generated code.
#include "GraphCodePlayRecord.h"
#include "StructUtils/InstancedStruct.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "GraphCodePlaySession.generated.h"

struct FGcPlayCommand;

/**
 * Who this run is FOR — `for:` in the header, and the one fact two behaviours key on.
 *
 * **[USER, ruled 2026-08-17] and it is an audience, not a driver-presence state.** §3.1d's three
 * states answer *is anybody listening right now*; this answers *whose run is this*, which the lease
 * cannot: a game a person is playing has no driver either, and the two must not be treated alike.
 *
 * What it decides, and nothing else:
 *
 *   1. **A LAPSED LEASE ENDS AN `Ai` RUN AND NEVER A `Player` ONE.** *"if ai is not active at the
 *      other end, end this session if it's started by ai. that means the other end lost
 *      connection."* A test exists to answer the agent's questions, so an agent that has gone away
 *      leaves a run recording evidence for nobody. A person mid-game is not interruptible by the
 *      absence of a machine.
 *   2. **NOTHING MAY JUDGE A `Player` RUN.** *"if it's player started one, or one that's started by
 *      ai for the players themselves, it should not let ai to judge anything."* A reader of a player
 *      run reports what it did and draws no conclusions from it. The ruling stands on its own and
 *      binds whatever reads runs next — it was written for a `reel` that was built and then deleted
 *      the same evening, and it outlived it.
 *
 * **THE DEFAULT IS `Player`, AND THE DOOR DECIDES RATHER THAN THE STRUCT.** A graph calling
 * `Begin Play Session` is a game that plays; somebody pressed Play and the run is theirs. The tool's
 * `start` sets `Ai` on the way in, because a session opened through the agent's own door was opened
 * by an agent — and an agent setting a scenario up for a person to play says `for:"player"` there.
 * A default that claimed the AI would make every human playtest killable by a lease nobody took.
 */
UENUM(BlueprintType)
enum class EGcPlayAudience : uint8
{
	/** Somebody is playing. Never ended by a lost driver, and never judged. */
	Player,

	/** The agent's own test run: it may be ended when the agent goes away, and judged when it does not. */
	Ai,
};

/** How a session ended. Written as `result=` on the `End` line, and never inferred from silence. */
UENUM(BlueprintType)
enum class EGcPlaySessionResult : uint8
{
	/** The graph reached its own end and says the run is good. */
	Passed,

	/** The graph reached its own end and says it is not. A result, not an error. */
	Failed,

	/** Ended by something other than the graph's own verdict — a driver, a level change, PreExit. */
	Stopped,
};

// THERE IS A FOURTH ENDING AND IT IS NOT IN THIS ENUM, DELIBERATELY: `expired_unhandled`.
//
// This enum is what a GRAPH may say, and it reaches the log through `End Play Session`. *The run hit
// its own deadline and nothing handled it* is not a verdict a graph can state — by construction, since
// a graph that could state it would have been the thing that handled it. So the session writes that
// token itself, on a path no node can reach (`EndWithToken`), and the file carries an ending the
// authoring surface cannot forge.
//
// `stopped` would have been the lazy home for it and is a different fact: *somebody stopped this run*
// and *this run ignored its own limit* are the two answers a reader most needs to tell apart.

/** Why a run was asked to end — `reason=` on the record, and the graph's own branch (§3.1b). */
UENUM(BlueprintType)
enum class EGcPlayEndReason : uint8
{
	/** A driver asked, through `gcp_run mode:"end"`. Somebody is watching and wants it stopped. */
	Driver,

	/** §3.1b's session limit came round. Nobody asked; the run outlived the budget it declared. */
	Expired,

	/**
	 * The driver that was here is gone — its lease lapsed rather than being let go [USER, 2026-08-17].
	 *
	 * **THE THIRD REASON IS AN ABSENCE, WHICH IS WHY IT IS NOT `Driver`.** `Driver` means somebody
	 * asked; this means nobody is there to ask. Spelling them the same would make a run the agent
	 * deliberately stopped indistinguishable from one it abandoned, and those are the two endings a
	 * reader most needs to tell apart — the same argument that keeps `expired_unhandled` out of
	 * `EGcPlaySessionResult`.
	 *
	 * **A DELIBERATE `detach` IS NOT THIS.** Letting the lease go says *carry on without me*, which
	 * the card has always promised; only the clock running out says the other end went away.
	 * `EGcPlayAudience::Ai` is the other half of the condition — a player's run is nobody's to end.
	 */
	Disconnected,
};

/**
 * The run has been asked to stop — raised once per request, process-wide.
 *
 * STATIC AND NOT A MEMBER, for the reason the control channel and the driver lease already are: a
 * listener must be able to exist before the session does. A graph that places `On End Requested`
 * above `Begin Play Session` on the same exec chain is ordinary authoring, and a delegate living on
 * the subsystem would silently fail to bind for exactly that graph.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FGcPlayEndRequested, EGcPlayEndReason);

// A SLICE HAS NO RESULT, and the enum that used to be here is deleted rather than reduced.
//
// **[USER, ruled 2026-08-15]** *"you can't end a slice, you can only end a session. you end slice by
// placing another slice on top."* A slice is a SEGMENT of a run — where the run went — and a verdict
// is a statement about the run, which `End Play Session` already carries. `EGcPlaySliceResult` had
// `Passed`/`Failed` for an `End Slice` node that no longer exists, and `Reloaded` for a driver load
// that does not exist yet; what was left reachable was one value, and a field with one value is a
// field that says nothing.
//
// So a close record states only who closed it: `by=progression` when the next slice opened, and
// `by=session` when the run ended under it. If §3.6d's load later closes a slice a third way, it
// adds a `by=` token — which is the same mechanism, not a new one.

/**
 * What it takes to open a slice — its name and its ping cadence.
 *
 * A STRUCT AND NOT TWO PARAMETERS because two callers open the first slice (`Begin` and
 * `BeginReplacing`) and one opens every later one, and a slice that could be described differently
 * depending on which door it came through is a slice whose log records would differ for no reason.
 *
 * NOT a `USTRUCT`: the nodes take loose pins and build this on the way in. §3.1a's rule is about
 * what a graph AUTHORS, and a graph never sees this.
 */
struct FGcPlaySliceSpec
{
	FString Name;

	/**
	 * §3.1f's cadence, in seconds of wall clock. Zero means no clock and no pings.
	 *
	 * **PER SLICE AND NEVER PER SESSION** [USER, 2026-08-15]. Different stretches of a game deserve
	 * different resolution; what makes that choice safe rather than optional is that a session
	 * cannot start without a slice, so there is always somewhere for the cadence to live.
	 */
	double TimeLimitSeconds = 0.0;
};

/**
 * What it takes to open a SESSION — the prose, the caller's id, and §3.1b's deadline.
 *
 * The internal twin of `FGcPlaySessionOptions` below, and the same split `FGcPlaySliceSpec` already
 * has from the loose pins that build it: a graph authors pins, this layer takes a value.
 */
struct FGcPlaySessionSpec
{
	FString Comment;
	FString RequestId;

	/**
	 * §3.1b's SESSION limit, in seconds of wall clock — the run's budget, and the one limit that
	 * ENDS something.
	 *
	 * **NOT the slice clock, and the two are the most confusable pair in this product.** A slice's
	 * limit pings and ends nothing (§3.1f, and the asymmetry is deliberate: a slow slice and a dead
	 * slice look identical, so the clock hands the question over). This one raises
	 * `On End Requested(Expired)` and, if the graph does nothing about it, closes the run itself.
	 *
	 * Zero means no deadline, which is right for a human playtest and wrong for anything unattended.
	 */
	double LimitSeconds = 0.0;

	/** Who the run is for — `EGcPlayAudience`. The default is the safe one; see the enum. */
	EGcPlayAudience Audience = EGcPlayAudience::Player;

};

/**
 * Everything a session can be told at its opening, as one value — §3.1a's ruling applied where that
 * section said it would next apply.
 *
 * §3.1a left `Begin Play Session` as an `[OPEN]`: *"it carries Name / Comment / RequestId on
 * AdvancedDisplay today and still owes `TimeLimit`. Four is where Mark's argument starts to apply."*
 * §3.1b's session limit is the fifth, so it applies now. Same rules as `FGcPlayMarkOptions`: **flat,
 * because Blueprint splits a struct pin into its members and a nested struct is a pin the author has
 * to split twice.**
 *
 * `Name` and `Slice` stay as their own pins. They are required — a run has to be called something and
 * has to start somewhere — and folding a required input into an options struct is how a required
 * thing comes to look optional.
 */
USTRUCT(BlueprintType)
struct FGcPlaySessionOptions
{
	GENERATED_BODY()

	/** Prose for the `Start` line — why this run exists. */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	FString Comment;

	/** The caller's own id, written to the header as `request:`. Empty is what an unattended run looks like. */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	FString RequestId;

	/**
	 * §3.1f's ping cadence for the FIRST slice, in seconds. It ends nothing.
	 *
	 * Renamed from `TimeLimit` when `SessionLimit` arrived beside it: one node carrying two limits,
	 * where the generic name meant the narrower thing, is a pin that reads as the other one.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	float SliceLimit = 0.0f;

	/** §3.1b's run deadline, in seconds. **This is the one that ends the run.** Zero means none. */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	float SessionLimit = 0.0f;

	/**
	 * Who this run is for, and it defaults to the player BECAUSE A GRAPH IS AUTHORING IT.
	 *
	 * A shipped game whose graph opens a session is a game somebody is playing, so the safe reading
	 * of an unset field is *this run is theirs*: never ended by a lease nobody took, never judged.
	 * An agent harness that wants the other answer sets it here, and the agent's own tool door sets
	 * it for itself. See `EGcPlayAudience`.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	EGcPlayAudience Audience = EGcPlayAudience::Player;
};

/**
 * A live session, opaque and session-scoped (§17 P1).
 *
 * A GUID rather than an index, because the one thing a handle must not do is stay valid across
 * sessions: an index would silently address the next run.
 */
USTRUCT(BlueprintType)
struct FGcPlaySessionHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FGuid Id;

	bool IsValid() const { return Id.IsValid(); }
};

/** What a `Mark` carries. Every field is optional except the name (§3.1c). */
/**
 * WHICH KIND OF PICTURE A MOMENT ASKS FOR — `RUNTIME-DRIVE-DESIGN.md` §17 P7a-ii, [USER, 2026-08-19].
 *
 * **AN ENUM RATHER THAN AN INHERITANCE HIERARCHY, AND THAT IS THE USER'S RULING WITH A MEASURED
 * REASON BEHIND IT.** Two shot kinds want two types, and the two ways UE spells *a pin whose type
 * varies* both cost something this product cannot spend: a `UObject` pin allocates per `Mark`, and
 * `FInstancedStruct` lives in the **StructUtils plugin** before UE 5.5 — [USER]: *"instancedstruct
 * needs user to enable this plugin. my project has enabled it, not necessary others have enabled."*
 * Measured on 5.7: `InstancedStruct.h` is in CoreUObject and needs no plugin **there**, so the
 * concern is version-dated rather than wrong — and it is live, because [USER] *"might want to ship
 * in 5.0"*. An enum needs nothing on any version.
 *
 * **`None` IS THE DEFAULT, WHICH IS HOW *OMIT IT AND THERE IS NO PICTURE* SURVIVES A BY-VALUE
 * STRUCT.** A struct pin is always present, so the *absent* case has to be a value; making it the
 * zero value means an unwired pin and an untouched struct say the same thing.
 */
UENUM(BlueprintType)
enum class EGcPlayShot : uint8
{
	/** No picture. The default, and what an unwired `Shot` pin means. */
	None UMETA(DisplayName = "No Shot"),

	/** One picture, at this moment. The whole of what `bShot = true` used to mean. */
	Once UMETA(DisplayName = "Shot"),

	/**
	 * A picture every `Every` seconds until the next `Mark`, keeping the last `Keep` of them.
	 *
	 * **THE DASHCAM, AND IT IS WHY THE MEDIUM CAME BACK TO FRAMES.** §17 P7a-i had ruled VIDEO on the
	 * argument that stills force the density decision before anyone knows what they are looking for —
	 * true, and answered by a rolling window rather than by a container: record continuously, keep a
	 * bounded tail, and the frames that survive are the ones nearest whatever went wrong.
	 */
	Rolling UMETA(DisplayName = "Rolling Shots"),
};

/**
 * A moment's picture: which kind, how often, for how long, and how many to keep — §17 P7a-ii.
 *
 * **THE SPAN'S OTHER END IS THE NEXT `Mark`, WHICH IS A POINT THE GRAPH ALREADY HAS** [USER,
 * 2026-08-19]: *"it should end on next mark. unless next mark also connect to that shot."* That is
 * what makes a rolling cadence expressible without a node pair. §3.1a-i had ruled a pair on the
 * argument that `Mark` is PUNCTUAL and a recording is an INTERVAL — the argument was sound and its
 * conclusion did not follow, because an interval delimited by two points needs no node of its own
 * when the points are already there.
 *
 * **AND *CONNECTS TO THAT SHOT* IS VALUE EQUALITY.** Two `Mark`s fed the same rolling shot continue
 * one run rather than starting a second: the wire is the author's statement that it is the same
 * recording, and equal fields are what the wire amounts to at runtime.
 */
/**
 * A rolling or single picture, RESOLVED — §17 P7a-ii.
 *
 * **NOT A PIN AND NOT REFLECTED** [USER, 2026-08-21]: *"see redundant nodes? rolling shots, shot and
 * record all is within mark."* This was a `USTRUCT(BlueprintType)` nested inside
 * `FGcPlayMarkOptions`, which is exactly what that struct's own *keep it flat as it grows* rule
 * forbids — and the cost was three maker nodes (`Shot`, `Rolling Shots`, `Record`) plus the four
 * `Make`/`Break` nodes reflection generates for free, all of them existing only to fill a pin the
 * author could not discover how to split. The fields live on `Mark` as `Shot*` now. What is left
 * here is the value the SESSION works from, assembled by `FGcPlayMarkOptions::ShotValue()`, and its
 * `operator==` is the *connects to that shot* rule.
 */
struct FGcPlayShot
{
	/** Which kind. `None` — the default — means this moment takes no picture at all. */
	EGcPlayShot Mode = EGcPlayShot::None;

	/**
	 * Seconds between pictures. `Rolling` only, and it is the one field a rolling shot must set.
	 *
	 * **THE COST IS PER FRAME AND THE CADENCE IS WHAT BOUNDS IT.** A shot is a readback the render
	 * thread performs at the end of a frame, so a cadence shorter than the frame time asks for a
	 * picture that cannot be taken and the queue refuses rather than accumulating (`PumpShots`).
	 */
	float Every = 0.0f;

	/**
	 * Wall-clock seconds this rolling run may last. Zero means *until the next `Mark`*, which is the
	 * ordinary case.
	 *
	 * **IT IS THE BACKSTOP FOR THE MARK THAT NEVER COMES**, which is the failure the next-`Mark` rule
	 * creates: a graph that rolls and then crashes, or simply never marks again, would otherwise shoot
	 * for as long as the run lives. Wall clock, per §14.5, because the failure being guarded against
	 * is frames stopping.
	 */
	float Limit = 0.0f;

	/**
	 * How many pictures this rolling run keeps on disk. Zero means keep every one.
	 *
	 * **THE WINDOW ROLLS: the oldest is deleted when the newest lands**, so a long unattended run
	 * costs a bounded amount of disk rather than a growing one. [USER, 2026-08-19] — *"rolling one"*,
	 * *"and declare max shots"*.
	 *
	 * **IT DELETES ONLY PICTURES THIS RUN ITSELF WROTE**, tracked by absolute path as each lands, and
	 * never by pattern over the folder. A window that swept `*.png` would eat the single shots a
	 * `Mark` took beside it, and the run folder is the one place this product writes files somebody
	 * may already be reading.
	 */
	int32 Keep = 0;

	/** Same kind, same cadence, same bounds — §17 P7a-ii's *connects to that shot*. */
	bool operator==(const FGcPlayShot& Other) const
	{
		return Mode == Other.Mode && Every == Other.Every && Limit == Other.Limit
			&& Keep == Other.Keep;
	}
	bool operator!=(const FGcPlayShot& Other) const { return !(*this == Other); }
};

/**
 * Whether this moment opens a record — §3.1i.
 *
 * TWO VALUES, NOT THREE. `FGcPlayShot` needs `Once` because a single picture is a whole thing on
 * its own; a record of one instant is a capture, which already exists and is `bCapture`. So a
 * record is always a SPAN, and the only question here is whether this mark starts one.
 */
UENUM(BlueprintType)
enum class EGcPlayRecord : uint8
{
	/** No record. The default, and what an unwired pin means. */
	None UMETA(DisplayName = "No Record"),

	/**
	 * Follow the watched cast from here, until a `Mark` that does not carry this same record.
	 *
	 * **THE ROLLING SHOT'S SPAN RULE, AND IT IS THE SAME ONE** [USER, 2026-08-21]: *"see how
	 * periodic capture work."* `ApplyShot` ends a rolling run at the next `Mark` *"unless next mark
	 * also connect to that shot"*, which is value equality — so a mark carrying an equal record
	 * continues this span and any other mark ends it. The observations reach the log's closing
	 * `records { }` block and no file of their own.
	 */
	Open UMETA(DisplayName = "Open Record"),
};

/**
 * A span to watch: who to follow, how often to look, and when to stop waiting — §3.1i.
 *
 * FLAT, for `FGcPlayMarkOptions`' stated reason: Blueprint splits a struct pin into its members, so
 * a nested struct becomes a pin the author has to split twice and the second split is not
 * discoverable. Cluster by naming, never by nesting.
 */
/**
 * A span to watch, RESOLVED — §3.1i.
 *
 * **NOT A PIN AND NOT REFLECTED**, for `FGcPlayShot`'s reason and on the same ruling. Its fields
 * live on `Mark` as `Record*`; this is what the session compares a later mark against to decide
 * whether that mark CONTINUES the span or ends it, which is why `operator==` stays here.
 */
struct FGcPlayRecord
{
	/** Which kind. `None` — the default — means this moment opens no record at all. */
	EGcPlayRecord Mode = EGcPlayRecord::None;

	/**
	 * Who to follow: any mix of `class:` `name:` `tag:` `near:` `*`, the language `Filter` speaks.
	 *
	 * **IT BELONGS TO THE RECORD, WHICH IS WHERE IT STARTED** [USER, 2026-08-21]: *"watch should be
	 * inside record itself."* An earlier reading moved it to a session header; that was backwards
	 * for a thing whose whole job is to be pointed at somebody for a span, and there is no header at
	 * either scope now. **Where it is PRINTED is the one spelling §3.1i still has open** — the
	 * spec's own file shows it nowhere, so nothing prints it today.
	 *
	 * Empty is refused: a record of the whole world is every actor sampled for the length of a
	 * fight, so an unpointed one is refused rather than quietly enormous.
	 */
	TArray<FString> Watch;

	/**
	 * What NOT to track about them — [USER, 2026-08-21]: *"the exclude is excluding the trackings,
	 * like states, or behaviours etc."*
	 *
	 * **`Watch` AND THIS ARE DIFFERENT QUESTIONS AND NEITHER SUBSTITUTES FOR THE OTHER.** `Watch`
	 * says which actors; this says which of their tracked names are worth a line. The problem it
	 * exists for is one blackboard key that genuinely moves — a distance, a countdown — writing an
	 * `Act` on every sample until the record is about that key and nothing else. `Every` cannot help
	 * (the key changes on every interval however long it is) and `Precision` cannot either (the
	 * value is not wobbling, it is moving).
	 *
	 * Matched as a case-sensitive PREFIX against a tracked name, and against each step of a path —
	 * `FGraphCodeExecutionState::IsExcluded` owns the rule and the reasons.
	 *
	 * **PART OF THE SPAN'S IDENTITY**, below: two marks that exclude different things were not
	 * authored to mean one span, because the record they would produce is not one record.
	 */
	TArray<FString> Exclude;

	/**
	 * Which function calls to record — R5, [USER, 2026-08-21]: *"so you declare a name, our system
	 * check against name, category and keywords."*
	 *
	 * **ONE DECLARED TOKEN, THREE CARRIERS, AND THAT IS WHY R5 ADDS NO SPELLING TO THE LANGUAGE.**
	 * UE 5.7 has no native tag on a `UFunction` — the only thing one can hold is metadata — so
	 * rather than ruling one of `Category`/`Keywords` to BE the tag, a token here matches a function
	 * whose NAME equals it, or any segment of its `Category` (split on `|`), or any of its
	 * `Keywords` (split on whitespace). All three are already read and written by the `.gc` grammar,
	 * so a fixture is authorable today and no author has to move the tags they already have.
	 *
	 * **THIS IS NOT THE STRUCK `functions:` LINE.** That was a list of function NAMES and nothing
	 * else could ever match it. A name match here is one of three ways a token can hit, and the
	 * author who files `Fire` under `Category="Combat"` selects it with `Combat` without ever naming
	 * it — which is what *record tag for function* asked for.
	 *
	 * Empty means **record no calls**, which is the default. That is deliberately not *record all of
	 * them*: an unpointed function selector is `Watch`'s problem one field over, and every call in a
	 * fight is what a cap exists to survive rather than what a default should produce.
	 *
	 * **PART OF THE SPAN'S IDENTITY**, like `Watch` and `Exclude`: two marks selecting different
	 * functions were not authored to mean one span.
	 */
	TArray<FString> Calls;

	/**
	 * Seconds between looks. Zero means *every tick*, which is a legitimate ask for a short span.
	 *
	 * This is a SAMPLE RATE, not a guarantee: a change that begins and ends inside one interval is
	 * not seen at all. That is the same trade the rolling shot makes and it is why the interval is
	 * the author's — the run knows what it is watching for and this file does not.
	 *
	 * **IT DOES NOT GOVERN `Calls`.** A call is an EVENT, so the listener writes it at the moment it
	 * happens and no cadence can miss it — the same argument the ability listener makes one field
	 * over, and the reason both are listeners rather than samplers.
	 */
	float Every = 0.25f;

	/**
	 * Seconds before the record closes itself. Zero means wait for the mark, however long it takes.
	 *
	 * THE BACKSTOP FOR THE MARK THAT NEVER COMES, checked before the cadence so a span that is over
	 * does not take one more sample on its way out — `TickShotRun`'s rule, and its reason.
	 */
	float Limit = 0.0f;

	/**
	 * Most observations to write before this stops writing them and starts counting them.
	 *
	 * **A CAP, AND IT IS NOT `FGcPlayShot::Keep`.** That keeps the LAST N pictures by deleting older
	 * files — a dashcam, and the same behaviour here means rotating an append-only text file, which
	 * is a different mechanism. One word for two behaviours is how a reader ends up with two
	 * meanings for one silence, so this one is named for what it does.
	 *
	 * Zero means no cap. **What a cap drops is NOT counted anywhere** — [USER, 2026-08-21]: *"and we
	 * wont have dropped= and skipped isnt it?"* `dropped=` lived on the deleted `Records(…)` line and
	 * no home was invented for it on `record(…)`, so the counter went with the field. A `Max` set too
	 * low silently shortens a span; that consequence is stated rather than hidden.
	 */
	int32 Max = 0;

	/**
	 * Same cast, same cadence, same bounds — the rolling shot's *connects to that shot*, for a span.
	 *
	 * **THIS IS WHAT MAKES A MARK CONTINUE A RECORD RATHER THAN END IT.** Two `Mark`s whose
	 * `Record*` pins carry the same values were authored to mean the same span, and equal fields are
	 * the only thing a runtime can compare — there is no wire to ask.
	 */
	bool operator==(const FGcPlayRecord& Other) const
	{
		return Mode == Other.Mode && Watch == Other.Watch && Exclude == Other.Exclude
			&& Calls == Other.Calls && Every == Other.Every && Limit == Other.Limit
			&& Max == Other.Max;
	}
	bool operator!=(const FGcPlayRecord& Other) const { return !(*this == Other); }
};

/**
 * Everything a moment can carry, as one value — §3.1a, [USER, ruled 2026-08-15].
 *
 * **A STRUCT RATHER THAN PINS, AND THAT IS THE RULING RATHER THAN THE SHORTCUT.** §3.1a first
 * reached for `bAdvancedDisplay`, on the argument that a chevron lets a node carry rarely-used
 * inputs without cluttering the common case. It does not survive the count: **shot and capture each
 * bring their own cluster**, and a chevron hiding a dozen pins is a long list that is sometimes
 * collapsed, not a simple node. The ruling §3.1a actually made — *one node, not a separate `Shot`
 * node* — is untouched by this; only the delivery changed.
 *
 * **KEEP IT FLAT AS IT GROWS.** Blueprint splits a struct pin into its members, so a flat struct
 * authors as a flat pin list. A nested `FGcPlayShotOptions` in here would become a pin the author
 * has to split twice, and the second split is not discoverable. Cluster by NAMING — `Shot*`,
 * `Capture*` — never by nesting. The C++ tidiness of nesting is paid for at every call site.
 *
 * Still owed from §3.1's table: `Fidelity`, `Shot`, `Camera`, `Target`. **`Values` shipped and this
 * line went on listing it** — the stale-summary failure, in the comment whose own struct disproves it.
 *
 * **`Shot`/`Camera`/`Target` WERE WITHHELD FOR A REASON THAT NO LONGER HOLDS, MEASURED 2026-08-17.**
 * §3.1a's `[UNMEASURED]` note said there is no runtime screenshot path in this repo, which is true of
 * THIS repo's own pixel readers — all editor surfaces — and it then treated the engine's mechanism as
 * assumed. It is not: `FScreenshotRequest::RequestScreenshot` (`Runtime/Engine/Public/UnrealClient.h`,
 * `ENGINE_API`) is the player's own view, `USceneCaptureComponent2D` (`Runtime/Engine/Classes/
 * Components/`) is any angle, and `UKismetRenderingLibrary::ExportRenderTarget` writes the file with no
 * `WITH_EDITOR` guard in its body. All three are runtime and public, so a packaged build can shoot.
 * The one real limit is `-nullrhi`: no renderer, no pixels, which is P6's second headless mode already.
 */
USTRUCT(BlueprintType)
struct FGcPlayMarkOptions
{
	GENERATED_BODY()

	/** Prose for a person. Optional — the name is what a run-to-run comparison reads (§5.9). */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	FString Comment;

	/**
	 * The observation: named, typed fields this mark measured. `PLAY-SESSION-DESIGN.md` §3.1e.2.
	 *
	 * **`Comment` IS PROSE AND THIS IS EVIDENCE — two pins, two jobs, and neither replaces the
	 * other.** `comment="12 minions"` reads fine to a person and cannot be compared to 13 without
	 * parsing English; a struct field can. The split is *`Comment` for anything read once, `Values`
	 * for anything ever compared*.
	 *
	 * **THE TYPE IS THE AUTHOR'S AND THIS FORMAT MUST NOT KNOW WHAT IT MEANS.** An earlier draft
	 * proposed a fixed `stats=(mem, actors, ms)` schema, which is a hand-written list of what
	 * somebody thought to measure — the failure this project keeps meeting
	 * ([[gc-capability-registry]]). **[USER, third round]** *"remember we have data tables just for
	 * that? it's native to unreal."* So the type is a `USTRUCT` the author declares, the expectation
	 * is a Data Table row of that same struct beside the test, and the comparison is field-by-field
	 * and typed because **one type serves both sides** — neither can drift without a compile error.
	 *
	 * **`FInstancedStruct` RATHER THAN A WILDCARD PIN.** §3.1e.2 ruled out `CustomThunk` +
	 * `CustomStructureParam`, and this is how UE spells *a struct pin whose type the author chooses*
	 * without either. It also costs **zero new codec**: `AuthoringCore::ExportStructValue` already
	 * writes `instanced_struct(/Script/Module.Concrete,(Field=value,…))`, which carries the concrete
	 * type — and the type is not decoration here, it is what says which Data Table row this is
	 * comparable to.
	 *
	 * **WHAT BELONGS IN IT, and the rule is narrower than it looks** (§3.1e.1): *record a value only
	 * if the reader cannot get it from the capture.* `minions=12` earns its place when the filter
	 * excludes minions; if the capture already lists all twelve, the count is noise. And nothing
	 * DERIVED — no `dps=45.5` — because that spends game-thread time to save a reader arithmetic and
	 * bakes one interpretation into the record. Record observations; leave derivation to whoever
	 * reads.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	FInstancedStruct Values;

	/** Capture the world at this moment, and name the file on the line. */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	bool bCapture = false;

	/**
	 * What to capture — the same selector grammar `gcp_capture` takes.
	 *
	 * EMPTY MEANS EVERYTHING (`*`), which §4.1 ruled the loadable save state rather than a debugging
	 * toy. A filter narrows it, and the filter is also the WRITE SCOPE of any later restore.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	TArray<FString> Filter;

	/**
	 * `true` → a new numbered file per execution (`phase_01`, `phase_02`); `false` → one filename,
	 * overwritten.
	 *
	 * USER-RULED as a pin rather than a global policy. It matters more than it looks: **a capture is
	 * what a branch returns to**, so an overwritten file is a state that is gone by the time you go
	 * back for it. `false` is right for a heartbeat and wrong for anything you intend to reload.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	bool bUnique = true;

	/**
	 * Raise the wake signal — `notify=true` on the line.
	 *
	 * NARRATION AND ATTENTION ARE DIFFERENT, which is the hole every earlier design had: fifty marks
	 * in a run is fine, three notifies is not. Parking the graph until an answer comes back is a
	 * third thing again, and that is `Await`, in item 4.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	bool bNotify = false;

	/**
	 * Photograph this moment — §3.1a's `Shot`, the fourth medium, and the one a capture cannot carry.
	 *
	 * **THE PLAYER'S OWN VIEW, UI INCLUDED** (`FScreenshotRequest`). Any angle is `Camera`/`Target`
	 * beside this and is a scene capture rather than the viewport — designed, not built here.
	 *
	 * **IT DOES NOT MAKE `Mark` LATENT** (§3.1's structural rule). The request is queued on the way
	 * past and the pixels land a frame or two later — nothing in the graph ever reads them.
	 *
	 * **AND THE `Mark` LINE THEREFORE NEVER NAMES THE FILE.** It says `shot=pending`, and the picture
	 * gets its own `Shot(…)` record when it exists — the `Await`/`Continued` shape, for the same
	 * reason: a log line naming a file that is still being written is the one thing §3.1 designed the
	 * on-close stamp to prevent. A run that dies with a shot in flight resolves it as failed rather
	 * than leaving the pending line unanswered ([[a-deferred-act-owes-an-outcome]]).
	 *
	 * With no renderer — `-nullrhi`, a dedicated server, a commandlet — this refuses BY NAME on the
	 * record and costs the run nothing else.
	 *
	 * `Rolling Shots` keeps taking them until a `Mark` that does not ask for the same ones.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	EGcPlayShot ShotMode = EGcPlayShot::None;

	/**
	 * Seconds between pictures. `Rolling Shots` only, and it is the one field a rolling shot must set.
	 *
	 * **THE COST IS PER FRAME AND THE CADENCE IS WHAT BOUNDS IT.** A shot is a readback the render
	 * thread performs at the end of a frame, so a cadence shorter than the frame time asks for a
	 * picture that cannot be taken and the queue refuses rather than accumulating (`PumpShots`).
	 * Zero is REFUSED by name rather than read as *every frame*.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play", meta = (ClampMin = "0.0"))
	float ShotEvery = 0.0f;

	/**
	 * Wall-clock seconds a rolling run may last. Zero means *until the next `Mark`*, the ordinary case.
	 *
	 * **IT IS THE BACKSTOP FOR THE MARK THAT NEVER COMES**, which is the failure the next-`Mark` rule
	 * creates: a graph that rolls and then crashes, or simply never marks again, would otherwise shoot
	 * for as long as the run lives.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play", meta = (ClampMin = "0.0"))
	float ShotLimit = 0.0f;

	/**
	 * How many pictures a rolling run keeps on disk. Zero means keep every one.
	 *
	 * **THE WINDOW ROLLS: the oldest is deleted when the newest lands**, so a long unattended run
	 * costs a bounded amount of disk rather than a growing one. [USER, 2026-08-19] — *"rolling one"*,
	 * *"and declare max shots"*. **IT DELETES ONLY PICTURES THIS RUN ITSELF WROTE**, tracked by
	 * absolute path as each lands, never by pattern over the folder.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play", meta = (ClampMin = "0"))
	int32 ShotKeep = 0;

	/**
	 * Open a record here — §3.1i, [USER, ruled 2026-08-20].
	 *
	 * **A MARK CARRYING THE SAME RECORD CONTINUES THE SPAN; ANY OTHER MARK ENDS IT** — [USER,
	 * 2026-08-21] *"see how periodic capture work"*, which is `ShotMode`'s rule above and needs no
	 * word of its own. The log line carries `record=$NAME`, where NAME is this mark's own name plus
	 * the frame the span opened on.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	EGcPlayRecord RecordMode = EGcPlayRecord::None;

	/**
	 * Who the record follows: any mix of `class:` `name:` `tag:` `near:` `*`, the language `Filter`
	 * speaks.
	 *
	 * Empty is REFUSED on the mark's own line: a record of the whole world is every actor sampled for
	 * the length of a fight, so an unpointed one is refused rather than quietly enormous.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	TArray<FString> RecordWatch;

	/**
	 * What the record does NOT track about them — [USER, 2026-08-21]: *"the exclude is excluding the
	 * trackings, like states, or behaviours etc."*
	 *
	 * A list of names, matched case-sensitively as a PREFIX against a blackboard key, a gameplay
	 * tag, a behaviour-tree node or a state-tree state — so `State.` takes a whole tag family and
	 * `Guard.Chase` takes a branch. Empty tracks everything, which is the default.
	 *
	 * **REACH FOR IT WHEN ONE KEY IS DROWNING THE RECORD.** A value that genuinely moves every
	 * interval — a distance, a countdown, an ammo count — writes an `Act` on every sample, and
	 * neither `RecordEvery` nor `precision` can help: the first only changes how often you see it
	 * and the second only suppresses a value that is not really moving.
	 *
	 * It is part of the span's identity, so a mark that excludes different things opens a NEW span
	 * rather than continuing the one running — the record it would produce is not the same record.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	TArray<FString> RecordExclude;

	/**
	 * Which function CALLS the record writes a line for — R5, [USER, 2026-08-21]: *"so you declare
	 * a name, our system check against name, category and keywords."*
	 *
	 * Declare one token per entry. A Blueprint function is recorded when that token equals its name,
	 * or any segment of its Category (which nests on `|`, so `Combat|Offense` answers to both), or
	 * any of its Keywords. Whichever way the function is already labelled is the way it is selected;
	 * nothing has to be re-tagged.
	 *
	 * Empty — the default — records no calls at all. A native C++ function is invisible to this
	 * however it is labelled: it has no bytecode and so no debug site to listen at.
	 *
	 * Part of the span's identity, so a mark selecting different functions opens a NEW span.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play")
	TArray<FString> RecordCalls;

	/**
	 * Seconds between looks. Zero means *every tick*, which is a legitimate ask for a short span.
	 *
	 * This is a SAMPLE RATE, not a guarantee: a change that begins and ends inside one interval is not
	 * seen at all. The same trade the rolling shot makes, and why the interval is the author's.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play", meta = (ClampMin = "0.0"))
	float RecordEvery = 0.25f;

	/**
	 * The record's MAX TIME — seconds before it closes itself [USER, 2026-08-21]. Zero means wait for
	 * the mark, however long it takes.
	 *
	 * Checked BEFORE the cadence, so a span that is over does not take one more sample on its way out.
	 * A span this closes names no closing mark, which is how the block says a limit ended it.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play", meta = (ClampMin = "0.0"))
	float RecordLimit = 0.0f;

	/**
	 * Most observations to record before it stops recording them and starts counting them.
	 *
	 * **A CAP, AND IT IS NOT `ShotKeep`.** That keeps the LAST N pictures by deleting older files — a
	 * dashcam — and the same behaviour here means rotating an append-only list, a different mechanism.
	 * One word for two behaviours is how a reader ends up with two meanings for one silence.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|Play", meta = (ClampMin = "0"))
	int32 RecordMax = 0;

	/**
	 * The `Shot*` pins as the value the session works from.
	 *
	 * **THE RESOLVED VALUE IS NOT A SECOND CARRIER OF THE AUTHORED ONE**, the way `FGcPlayRecordSpan`
	 * is not a second carrier of `FGcPlayRecord`: this direction is one-way and computed, and it is
	 * the only place the flat pins become a struct.
	 */
	FGcPlayShot ShotValue() const
	{
		FGcPlayShot Shot;
		Shot.Mode = ShotMode;
		Shot.Every = ShotEvery;
		Shot.Limit = ShotLimit;
		Shot.Keep = ShotKeep;
		return Shot;
	}

	/** The `Record*` pins as the value the session works from, and compares the next mark against. */
	FGcPlayRecord RecordValue() const
	{
		FGcPlayRecord Record;
		Record.Mode = RecordMode;
		Record.Watch = RecordWatch;
		Record.Exclude = RecordExclude;
		Record.Calls = RecordCalls;
		Record.Every = RecordEvery;
		Record.Limit = RecordLimit;
		Record.Max = RecordMax;
		return Record;
	}
};

/** What one `Mark` wrote, so a graph can act on it without parsing its own log. */
USTRUCT(BlueprintType)
struct FGcPlayMarkResult
{
	GENERATED_BODY()

	/** How many marks of this name have now been written, 1-based. `phase_03` is sequence 3. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	int32 Sequence = 0;

	/** The `.gcp` this mark saved, or empty. Absolute. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FString SavedPath;

	/**
	 * The capture's body digest — §5.7's SKIP SIGNAL, not only an integrity check.
	 *
	 * Same digest as the previous mark means nothing being watched changed, so the file need not be
	 * opened at all. Over a ten-minute run that is most of the token cost.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FString Digest;
};

/**
 * The session, its log, and its storage — one per GameInstance, and never more than one live.
 *
 * A SUBSYSTEM RATHER THAN A SINGLETON because a session outlives a level and dies with the game
 * instance, which is exactly what `UGameInstanceSubsystem` already means. §5.12 states the
 * requirement and PIE is where it bites: a level load would take a world-scoped session with it.
 */
UCLASS()
class GRAPHCODEPLAY_API UGraphCodePlaySession : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * THE TEST-MODE GATE — §17 P1, and it answers two problems with one mechanism.
	 *
	 * It is how authored instrumentation knows to start, AND it is why a `Mark` left in a shipped
	 * graph is inert rather than dangerous: in Shipping this is false unless the build explicitly
	 * opts in with `GRAPHCODE_PLAY_ALLOW_SHIPPING=1`, so every entry point below returns without
	 * touching a disk. Everywhere else it is true, because an editor or a development build is where
	 * instrumentation is supposed to run.
	 *
	 * **The three-state question is OPEN (§8.18)** — off / instrumented-unattended / driver-attached,
	 * where only the third may pause or mutate. Nothing here pauses or mutates, so two states is the
	 * honest shape until `Await` arrives and needs the distinction.
	 */
	static bool IsTestModeEnabled();

	/** The live session's subsystem for a world, or null. Never creates one. */
	static UGraphCodePlaySession* Find(const UObject* WorldContextObject);

	/**
	 * Open a session and write its header, `Start` line and first `Slice(` — §3.6a.
	 *
	 * Refuses while one is live rather than stacking: a second session on one log would interleave
	 * two runs in one file. **That reason forbids two runs AT ONCE, which is all it forbids** — see
	 * `BeginReplacing` for the caller it never governed (§3.1g).
	 *
	 * Refuses an unnamed FIRST SLICE too, which is the newer rule and the load-bearing one: a run
	 * has to start somewhere, and until this was required the session invented `main` and no ping
	 * cadence could reach the opening stretch of any run.
	 */
	bool Begin(const FString& Name, const FGcPlaySliceSpec& First, const FGcPlaySessionSpec& Spec,
		FGcPlaySessionHandle& OutHandle, FString& OutError);

	/**
	 * The run has been asked to stop — §3.1b's ONE DOOR for every ending that is not the graph's own.
	 *
	 * A driver's `end` command and the session limit arrive here and are indistinguishable afterwards
	 * except by `reason=`, which is the point: the graph handles *stop* once, and what it does about it
	 * is the same work whoever asked.
	 *
	 * **THIS DOES NOT END ANYTHING.** It writes the request, raises the event, and starts the grace
	 * clock. The graph is supposed to end the run — usually `Mark(Capture=true)` then
	 * `End Play Session` — because a run that gets to write its own last state is strictly better
	 * evidence than one a watchdog killed. `EndWithToken` is what happens when it does not.
	 *
	 * A second request while one is outstanding is a no-op rather than a restarted grace clock: two
	 * `end`s should not buy the graph twice as long to ignore them.
	 */
	bool RequestEnd(EGcPlayEndReason Reason, const FString& Comment);

	/** Whoever wants to know the run has been asked to stop. Process-wide; see the declaration. */
	static FGcPlayEndRequested& OnEndRequested();

	/**
	 * Queue a resolved load — §3.6d-ii. The plan and the entry travel together and fire one tick later.
	 *
	 * **THE DEFERRAL IS A CORRECTNESS REQUIREMENT, not tidiness.** Dispatching the entry inside the
	 * caller would run the whole new slice — `Begin Slice`, the apply, everything wired after — nested
	 * in the `Load` node that asked for it. A graph that loads from within its own entry chain would
	 * then recurse inside a single frame and hang where it meant to loop. Deferred, that same graph
	 * advances one iteration per frame, which is the retry loop of §3.4.
	 *
	 * **ON THE SESSION TICKER, never a latent action**: `Await(Freeze=true)` pauses the world and a
	 * paused world processes no latent actions at all, so a latent trigger would never fire in exactly
	 * the state a driver most often leaves behind before branching.
	 *
	 * Queued in order. Two loads are two entry calls, not a contest for one slot — which is what
	 * dissolved §3.6d's precedence question rather than answering it.
	 */
	bool StageEntry(const FGcPlayEntryCall& Call, FString& OutError);

	/**
	 * The plan the running dispatch carries, if it has not been consumed yet.
	 *
	 * FOR `BeginSlice` AND FOR SPECS. It is deliberately not *the stage* — nothing holds a plan between
	 * dispatches, so there is no slot for a stale one to sit in (§3.6d-ii).
	 */
	const FGcPlayEntryCall* CurrentEntry() const;

	/**
	 * Take whatever the driver said and act on it — the control channel's consumer, run per tick.
	 *
	 * **`Await` PEEKS AND THIS TAKES**, which is the division the channel's `Peek` was added for: a
	 * parked graph consumes the `continue` addressed to it, and everything else belongs to the session.
	 * Without a consumer here, `end` and `load` were verbs the tool accepted and posted and nothing
	 * ever executed — a door with no room behind it.
	 *
	 * Public because a spec must be able to drive it without waiting for a tick, the same reason
	 * `Ping` and `UGcPlayAwait::PumpForTesting` are. True when it acted on something.
	 */
	bool PumpControl();

	/**
	 * How long the graph gets to handle `On End Requested` before Play closes the run itself.
	 *
	 * **A SETTABLE STATIC, and it exists for one caller: a spec that would otherwise sleep.** The
	 * alternative is a five-second wait in every test that asserts the backstop, which is how a suite
	 * becomes slow enough that people stop running it.
	 */
	static double& EndGraceSeconds();

	/**
	 * Make this the running session, ending any run already live — §3.1g.
	 *
	 * A DIFFERENT REQUEST FROM `Begin`, not a flag on it, because the two callers are asking for
	 * different things. A graph firing `Begin Play Session` twice means *open a session* and the
	 * second one is a bug: nothing about a graph's structure means *abandon the run in progress*, so
	 * killing it silently would hide the mistake. A driver saying *run test B* means exactly this.
	 *
	 * `Begin`'s refusal never governed this case. Two runs interleaved in one append-only file is
	 * not a log anybody can read — but the displaced run closes its own file first, and sequential
	 * files are not interleaved. The one it displaces ends `result=stopped`, which already means
	 * *ended by something other than the graph's own verdict*, so this costs no new vocabulary.
	 *
	 * `OutReplacedSessionId` is the run that was ended, or empty when none was — the caller has to
	 * be able to say what it displaced rather than reporting a clean start.
	 */
	bool BeginReplacing(const FString& Name, const FGcPlaySliceSpec& First,
		const FGcPlaySessionSpec& Spec, FGcPlaySessionHandle& OutHandle, FString& OutError,
		FString& OutReplacedSessionId);

	/** Append one moment. Synchronous, and it has no failure path a graph should branch on (§3.1). */
	bool Mark(const FString& Name, const FGcPlayMarkOptions& Options, FGcPlayMarkResult& OutResult);

	/** Close the slice, write `End`, and drop the session. A second call is a no-op, not a fault. */
	bool End(const FGcPlaySessionHandle& Handle, EGcPlaySessionResult Result, const FString& Comment);

	/**
	 * The graph stopped here and is waiting for the driver — §3.1d, written the moment it parks.
	 *
	 * TWO LINES AND NOT ONE, because a park has a duration and a log line does not: this one says the
	 * run reached the question, `AwaitResolved` says what came back and how long it took. A reader
	 * tailing the file sees the first one while the run is still stopped, which is the only way the
	 * file can answer *what is it waiting for* at the moment somebody asks.
	 */
	/**
	 * Wake the AI, stop the world, write the park — in that order, which is why it is one call.
	 *
	 * The delta is computed BEFORE the pause (a state captured through a pause is a state nobody
	 * played) and the pause is attempted before the line is written, so `froze=` can state what
	 * actually happened rather than what was asked for. `bOutFroze` is what the caller unpauses on.
	 * §3.1h: a handshake stops the world, so there is no `Freeze` parameter to disagree with.
	 */
	bool AwaitParked(const FString& Name, const FString& Comment, double TimeoutSeconds,
		bool& bOutFroze);

	/**
	 * Nobody was listening, so nothing was parked — and it is STILL recorded (§3.1d).
	 *
	 * That is the whole point: it keeps the shape of a supervised run and an unattended one
	 * comparable — same marks, same names, same order, one field saying nobody was watching. Omitting
	 * it silently would make the two incomparable, which is the one thing the shape reading is for.
	 */
	bool AwaitSkipped(const FString& Name, const FString& Comment, const TCHAR* Reason);

	/**
	 * What ended the park, and after how long.
	 *
	 * `Released` names the ending that is neither an answer nor a deadline — `driver_lost`, or a verb
	 * the park declined. It is a separate field from the deadline ON PURPOSE (§3.1d): spelling an
	 * abandoned run as a timeout would let a shape comparison read it as a slow one.
	 *
	 * `AfterSeconds` is WALL CLOCK, because `Freeze` stops the world's own clock and *how long were
	 * we stopped* must still have an answer when it does.
	 */
	bool AwaitResolved(const FString& Name, double AfterSeconds, bool bTimedOut,
		const TCHAR* Released, const FString& Answer = FString());

	/**
	 * Open a slice — §3.6a's one block kind — and in doing so CLOSE the one that was open (§3.6c).
	 *
	 * *A slice ends when the next one begins*, and nothing has to say so. The graph does not know it
	 * is being superseded; only the session sees both sides of the transition, so the session writes
	 * the close. That is why this is one call and not begin-after-end: a caller that had to end the
	 * previous slice itself could forget, and a forgotten close is spelled exactly like a crash.
	 *
	 * `TimeLimitSeconds` is §3.1f's slice clock: every that many seconds of WALL CLOCK the AI is
	 * pinged with what has changed. **It ends nothing** — that is the whole asymmetry with the
	 * session's limit, because a slow slice and a dead slice look identical from outside and only
	 * the AI can tell them apart by looking. Zero or less means no clock, which is the default: the
	 * graph decides the cadence and the runner never imposes one.
	 *
	 * **No `From?` yet**, and that absence is still on purpose: it restores an authored starting
	 * state, which is §3.6d's staged load.
	 */
	bool BeginSlice(const FGcPlaySliceSpec& Slice, FString& OutError);

	/**
	 * The slice clock fired — §3.1f's THIRD wake kind, and the only one nobody authored.
	 *
	 * **IT WRITES NO LOG LINE, and that is a rule rather than an economy** (§3.1f). §5.9's shape
	 * reading — kinds and names down the file — is the run-to-run regression signal, and it works
	 * only if the lines are about the game. A twenty-minute run pinging every thirty seconds would
	 * add forty records that say nothing about what the test did, and two runs on machines of
	 * different speeds would stop being line-comparable. **The clock must not be able to change a
	 * run's shape.** Everything a ping is worth is in the delta it writes.
	 *
	 * **IT DOES NOT JUDGE, and it does not pause** [USER, ruled 2026-08-15]. An earlier draft had an
	 * empty delta escalate into a park with a `Stalled` record; the user deleted the whole apparatus:
	 * *"too long didnt reach next mark itself is a signal. let the ai judge"*. Overrun is the finding,
	 * the delta is the evidence, and the verdict belongs to the reader. That also disposed of §4.5's
	 * two open questions rather than answering them — what counts as "empty" against a ticking world
	 * clock, and whether a repeated digest means a loop — because nothing here asks either question.
	 *
	 * Public because a spec must be able to fire it without sleeping for a real thirty seconds, the
	 * same reason `UGcPlayAwait::PumpForTesting` exists. False when there was nothing to write.
	 */
	bool Ping();

	/** The current slice, for `Get Current Slice`. Empty names mean no session. */
	void CurrentSlice(FString& OutName, FString& OutFrom) const;

	/** How many deltas this run has written. Zero means no slice ever carried a clock. */
	int32 PingsWritten() const { return PingCount; }

	/**
	 * How long since the heartbeat last beat, or a negative number when no clock is armed.
	 *
	 * FOR A DRIVER, and the mirror of `FGraphCodePlayDriver::SecondsRemaining` — one side's proof of
	 * life read by the other. **Negative is not zero**: *no clock* and *pinged just now* are opposite
	 * answers, and a caller that read them the same way would treat an uninstrumented run as a
	 * perfectly healthy one.
	 */
	double SecondsSinceLastPing() const;

	bool IsRunning() const { return Handle.IsValid(); }

	/** Where this session's log and captures live. Empty when nothing is running. */
	const FString& Directory() const { return SessionDirectory; }
	const FString& LogPath() const { return SessionLogPath; }

	/**
	 * The run's name — its start time, `YYYYMMDD-HHMMSS`, the same string the folder and every
	 * `run:` line carry. Empty when nothing is running.
	 *
	 * ASKED FOR RATHER THAN RECONSTRUCTED. The Blueprint node used to derive this from the handle's
	 * GUID with its own copy of the rule, which agreed with the session's copy only for as long as
	 * nobody changed either — and the day the rule became a timestamp, the two would have answered
	 * differently with nothing erroring ([[a-read-answers-through-the-write-path]]).
	 */
	const FString& RunName() const { return SessionId; }

	/**
	 * What goes in every `.gcp`'s `run:` — the run's NAME and its stamp, which is the folder's leaf.
	 *
	 * **[USER, 2026-08-17]: *"be more accurate about it, use name as well."*** The stamp alone was
	 * one second wide, and the `-2` de-duplicator does not help because it tests `<name>_<stamp>` —
	 * the FOLDER — so two runs called different things in the same second get two folders and one
	 * identical stamp. Measured before the fix: **80 `run:` values in this checkout were shared by
	 * two differently-named folders**, and for every one of those pairs `AreComparable`'s cross-run
	 * refusal did not fire at all, so two unrelated runs subtracted.
	 *
	 * The folder leaf is the string the guard already made unique, so carrying THAT is what makes the
	 * field unique — no wider clock, and [USER]'s ruling against milliseconds (*three digits of noise
	 * on every run*) stands untouched. `SessionId` is deliberately left alone: `follow`, `collect`
	 * and `kill` take it from callers and it is not this field's job.
	 */
	const FString& RunIdentity() const { return RunIdentityValue; }

	/** `USubsystem` — a session that is still open when the game instance goes is ENDED, not leaked. */
	virtual void Deinitialize() override;

private:
	/** One line, terminated and flushed, because the newline is the commit (§5.4). */
	void AppendLine(const FString& Line);

	/**
	 * `frame=…, time=…, fps=…, worst=…ms` — the envelope every record opens with, from the world the
	 * session opened in.
	 *
	 * **`fps=` AND `worst=` NAME THE INTERVAL SINCE THE PREVIOUS RECORD** [USER, ruled 2026-08-17],
	 * which is what makes them useful: consecutive records tile the run with no gaps and no overlaps,
	 * so a slow stretch is localised to the pair that brackets it, and adding marks inside that pair
	 * bisects it. `RUNTIME-DRIVE-DESIGN.md` §17 P6's frame-cost row. A whole-run average cannot do
	 * this — a 500ms hitch is 0.5% of a 90s run and 20% of a 2s interval, so the *window* is the
	 * instrument and the author sets it by placing marks.
	 *
	 * **THE TWO FIELDS ANSWER DIFFERENT QUESTIONS AND NEITHER SUBSTITUTES.** `fps=48.0` alone is
	 * equally consistent with one 500ms jolt and with every frame running at 20.8ms, which feel
	 * nothing alike; `worst=` is what separates them.
	 *
	 * **NOT `time=`'S CLOCK, AND THE ENGINE SOURCE IS THE REASON.** `LevelTick.cpp` accumulates
	 * `AudioTimeSeconds` from the delta *before* dilation and *before* `FixupDeltaSeconds` clamps it,
	 * and only when unpaused — so it is the one world clock that is both pause-aware and undistorted.
	 * `time=` is `GetTimeSeconds()`, which is dilated and clamped, so slomo would inflate the figure
	 * and a delta clamp could erase the very hitch being hunted. `worst=` is `DeltaRealTimeSeconds`
	 * for the same reason: it is saved before both adjustments.
	 *
	 * The consequence a reader must know: dividing the printed `frame=`/`time=` deltas reproduces
	 * `fps=` exactly in a game with no dilation and no clamp, and disagrees in one that has either.
	 * `fps=` is right in both. A second printed clock to make one field checkable is not worth a
	 * field on every record.
	 *
	 * **BOTH ARE ABSENT RATHER THAN ZERO, AND ONE GUARD COVERS THREE CASES**: the run's first record
	 * has no previous record; two records may share a frame (§5.9 allows it); and an interval spent
	 * wholly paused advances frames but not this clock, which would divide by zero. A zero here would
	 * read as *the game froze* — the same reason `timeout=` prints nothing rather than `0`.
	 *
	 * NOT CONST: stamping a record closes an interval and opens the next one.
	 */
	FString Envelope();

	/**
	 * Write the `Slice(…) {` opener, take the name and `from:` as the current ones, lay down the
	 * slice's rolling baseline and arm its clock.
	 */
	void OpenSlice(const FGcPlaySliceSpec& Slice, const FString& From);

	/**
	 * Write the slice's own `End(…)` record and then its `}`. A no-op when no slice is open.
	 *
	 * `By` is the only thing the record carries: `progression` when the next slice opened, `session`
	 * when the run ended under it. *A slice the game walked out of* and *a slice the run ended on*
	 * are different events, and a log that spelled them the same way would lose the distinction
	 * permanently (§3.6c).
	 */
	void CloseSlice(const TCHAR* By);

	// -------------------------------------------------------------------------------------------
	// §4.5 — THE ROLLING STATE, AND THE ONE THING THE AI IS PUSHED RATHER THAN PULLS
	// -------------------------------------------------------------------------------------------
	//
	// Everywhere else in this design the AI pulls: it reads the log, checks a digest, and decides
	// whether to open a capture. This half is pushed — **whenever the AI is woken it is told what
	// changed since the last time it was woken** — and it is stated on the WAKE rather than on any
	// one node, which is why it lives here and not in `Await`. Three wakes, no difference between
	// them in what arrives: `Mark(Notify=true)`, a park, and the slice clock.
	//
	// Three files, and only one of them is a new kind (§4.5's table):
	//
	//   pings/current.gcp    the whole world as of the last wake — the rolling baseline
	//   pings/previous.gcp   the whole world as of the wake before that
	//   pings/ping_NN.gcp    what changed between those two — the delta, and the only one that is
	//                        NOT loadable: it describes a change, not a state (§18)
	//
	// **Both states are whole and unfiltered [USER], and that is the load-bearing constraint rather
	// than a detail.** A filter is an authored expectation, and this channel exists precisely for
	// the case where nobody has one yet: filter the baseline and it silently inherits the blindness
	// the filter encodes while still reporting confidently. "Unfiltered" means WHICH OBJECTS, not
	// which properties — the states stay `baseline: archetype`, so every object is present and only
	// deviating properties are written, which is what keeps a whole-world capture per wake cheap
	// enough to take at all.
	//
	// **The write order IS the crash-safety property**: capture fresh → compute the delta against
	// `current.gcp` → write the delta → rotate `current` to `previous` → put the fresh capture at
	// `current`. The delta is durable before either state moves, and at no instant does less than
	// one complete state exist on disk.
	//
	// **Ground truth is recomputed, never accumulated** (§5.14a one level up). A delta is never
	// applied to anything; keeping a base file and folding deltas into it is the standard
	// incremental-state trap, where one dropped or misordered delta leaves a baseline confidently
	// describing a world that never existed.

	/** Where the rolling channel lives. Under the run folder, so a mark can never collide with it. */
	FString RollingDir() const;

	/**
	 * Capture, diff, rotate — §4.5. True when the world was captured and the states rotated.
	 *
	 * **THE RETURN AND THE OUT-PARAMETER ANSWER DIFFERENT QUESTIONS, and both are needed.** True is
	 * *the game is alive and its state is on disk* — the heartbeat. A non-empty leaf is *and here is
	 * what changed* — the report. A wake with nothing to compare against (the first after a seed
	 * that failed, or a baseline that is not comparable) is a real heartbeat with no delta, and
	 * folding the two would make a healthy run look like a dead one.
	 *
	 * A caller that names the leaf in a log line must also keep it: see `KeptDeltas`.
	 */
	bool Wake(FString& OutDeltaLeaf);

	/**
	 * Lay down the baseline a slice's first wake will be measured against — §4.5's boundary reset.
	 *
	 * **THE BASELINE RESETS AT EVERY SLICE BOUNDARY**, so a delta never spans a restore. Without it
	 * the first delta after a load reports *everything changed*, which is true and useless. The
	 * consequence worth stating: `from:` and the delta chain then agree about where a slice began —
	 * two accounts of one run that cannot disagree.
	 */
	void SeedRolling();

	/**
	 * The clean-exit sweep — the rolling files are a working set, not evidence [USER].
	 *
	 * A run folder holding forty superseded world states is not a record, it is litter. But the
	 * sweep cannot be indiscriminate, because a log line can name a delta and a log that names a
	 * file which is not there is worse than one that never named it. So retention falls out of a
	 * rule that already exists: **a delta survives exactly if a log line references it.**
	 *
	 * **A CRASHED RUN SWEEPS NOTHING, deliberately.** This runs on the way out through `End`, so a
	 * game that died keeps both states and every delta — precisely the forensic set somebody wants
	 * when §5.13's missing brace says the run ended here. A `finally` would have destroyed the
	 * evidence for the one class of run that needs it.
	 */
	void SweepRolling();

	/**
	 * Bind and drop `worst=`'s sample — see `WorstFrameSeconds` for why it is this delegate and not the
	 * ticker that was already here.
	 *
	 * A LAMBDA CAPTURING `this` MUST BE DROPPED EXPLICITLY, unlike the two tickers, which are weak: a
	 * world delegate has no weak form, so the unbind is the only thing standing between a torn-down
	 * subsystem and a call into it. Dropped by `End` and again by `Deinitialize`, for the same reason
	 * both drop the tickers.
	 */
	void ArmFrameSampler();
	void DisarmFrameSampler();

	/** Start (or replace) the slice clock. Zero or less disarms it. */
	void ArmSliceClock(double LimitSeconds);

	/**
	 * The one ticker a running session always has: §3.1b's deadline, the grace clock, and the control
	 * channel's consumer.
	 *
	 * **ONE TICKER FOR THREE JOBS RATHER THAN THREE**, because all three are *check a wall clock and
	 * look at a queue* and none of them is expensive. Three tickers on one subsystem is three lifetimes
	 * to get right, and the slice clock already proved that a ticker outliving its subject is the leak
	 * this file cannot afford twice.
	 *
	 * WALL CLOCK throughout, for §14.5's reason: what is being watched for is frames stopping.
	 */
	void ArmSessionTicker();
	bool SessionTick(float DeltaSeconds);

	/**
	 * Close the run with a token no node can produce (`expired_unhandled`) — §3.1b's backstop.
	 *
	 * `End` takes the graph's three-value verdict and forwards here; this takes the string, so the file
	 * can carry an ending the authoring surface cannot forge. See the note under
	 * `EGcPlaySessionResult`.
	 */
	bool EndWithToken(const TCHAR* ResultToken, const FString& Comment);

	/**
	 * Fire the oldest queued entry: write its `Load` record, make its plan current, dispatch, and
	 * report if nothing consumed it.
	 *
	 * ONE PER TICK, so a queue of loads is a sequence of slices rather than a stack of nested calls.
	 */
	void FireNextEntry();

	// -------------------------------------------------------------------------------------------
	// §17 P4 — THE BATCH: handed over once, because a sequence of single calls is not a sequence
	// -------------------------------------------------------------------------------------------
	//
	// **IT LIVES HERE AND NOT BESIDE THE CHANNEL** because every step it runs needs something only a
	// session has: the log, the mark counters a `mark:` rendezvous reads, the entry queue a `load`
	// settles against, and the ticker. A batch executor elsewhere would have needed all five made
	// public, which is a wider door than the feature.
	//
	// **IT RIDES THE SESSION TICKER, a fourth job on the one that already carries three** —
	// `ArmSessionTicker`'s argument applies unchanged: this is *check a wall clock and look at a
	// queue*, and a second ticker is a second lifetime to get right.
	//
	// The design is `GraphCodePlayBatch.h`. What is here is only the running of it.

	/** Take a posted batch and open its first step. False when there is nothing to run. */
	bool BeginBatch(const FGcPlayCommand& Command);

	/** One advance: the lease, the deadline, the `when`, the act, the settle. True when it did something. */
	bool TickBatch();

	/**
	 * Write the `Step(` record and start the current step's clocks.
	 *
	 * **WRITTEN WHEN THE STEP BEGINS, NOT WHEN IT FIRES**, which is `Await`'s two-line rule (§3.1d)
	 * applied to a batch: a park has a duration and a log line does not, so a step waiting on a mark
	 * that never comes has to be visible WHILE it waits. A record written on firing would leave a
	 * wedged batch as silence, and §14.5's outer expiry reads exactly this file to say what a run is
	 * parked on.
	 */
	void OpenStep();

	/** Has this step's `when` come round? */
	bool StepDue(const FGcPlayStep& Step) const;

	/** Perform the act, through the same function the single mode calls. False stops the batch. */
	bool RunStep(const FGcPlayStep& Step, FString& OutError);

	/**
	 * Has the act finished, including in whatever else had to act on it?
	 *
	 * **A STEP IS NOT DONE WHEN IT IS HANDED OVER.** A `continue` is done when the parked `Await`
	 * has taken it and a `load` when its entry has actually fired — *resume, then invoke* means the
	 * invoke waits for the resume, and a batch that advanced on the handover would deliver the
	 * sequence the transport was invented to prevent.
	 */
	bool StepSettled(const FGcPlayStep& Step) const;

	/** Write `BatchEnded` and drop the batch. A no-op when none is running. */
	void EndBatch(EGcPlayBatchResult Result, const FString& Detail);

	/**
	 * Call `Actor.Function` and write what it did — `Invoke(`, or `Finding(kind=invoke_refused`.
	 *
	 * SHARED BY THE SINGLE `invoke` AND THE BATCH STEP, so there is one record for one act rather
	 * than two spellings that can drift on what `observed=` means.
	 */
	bool RunInvoke(const FString& Target, const FString& Arguments, const FString& Comment,
		FString& OutError);

	/**
	 * Resolve a branch and queue it — `Finding(kind=load_refused` when it will not resolve.
	 *
	 * `Comment` becomes the load's `description=`: the field a `Load` node has always had and the
	 * driver's path could never fill. See the note at the call to `Resolve`.
	 */
	bool RunLoad(const FString& From, const FString& Entry, const FString& Comment,
		FString& OutError);

	/** Put a `.gcp` back and write `Inject(`, or `Finding(kind=inject_refused`. */
	bool RunInject(const FString& Capture, const FString& Comment, FString& OutError);

	/**
	 * Press a control in the game's own UI — `Activate(`, or `Finding(kind=activate_refused`.
	 *
	 * Shared by the single `activate` and the batch step, for `RunInvoke`'s reason. `Event` is the
	 * multicast delegate to broadcast and is empty for `OnClicked`.
	 */
	bool RunActivate(const FString& Target, const FString& Event, const FString& Comment,
		FString& OutError);

	FGcPlaySessionHandle Handle;
	FString SessionId;

	/** `<name>_<stamp>` — the folder leaf, and what `run:` carries. See `RunIdentity()`. */
	FString RunIdentityValue;
	FString SessionDirectory;
	FString SessionLogPath;
	FString SliceName;
	FString SliceFrom;

	/** Per-name counters, which is what makes `Mark("phase")` in a loop produce phase_01/02/03. */
	TMap<FString, int32> MarkCounts;

	/** Run-wide and never per slice: two slices producing two `ping_01` would collide in one folder. */
	int32 PingCount = 0;

	/** Deltas a log line names. Exactly these survive the sweep. */
	TArray<FString> KeptDeltas;

	/** §3.1f's slice clock. Wall-clock, because the thing being watched for is frames stopping. */
	FTSTicker::FDelegateHandle SliceClock;

	/**
	 * The cadence the live slice asked for, and when the heartbeat last actually beat.
	 *
	 * **WALL CLOCK FOR THE DEADLINE, GAME TIME FOR THE RECORD**, and the split is not fussiness: the
	 * deadline must keep moving when the world stops, and the record must line up with the `time=`
	 * on every other line in the file.
	 */
	double SliceLimit = 0.0;
	double LastPingAt = 0.0;
	double LastPingWorldTime = 0.0;

	/**
	 * What the previous record stamped, so the next one can state the interval between them — see
	 * `Envelope`. The clock is `GetAudioTimeSeconds()`: pause-aware, undilated, unclamped.
	 *
	 * `bStampedARecord` rather than testing the frame for zero, because `GFrameCounter` is the
	 * PROCESS's frame count and is already large when a run opens. There is no sentinel value here
	 * that means *no previous record*, so the flag has to carry it.
	 */
	uint64 LastRecordFrame = 0;
	double LastRecordRealTime = 0.0;
	bool bStampedARecord = false;

	/**
	 * The slowest single frame since the last record, in seconds — `worst=`. Reset by every stamp.
	 *
	 * **SAMPLED FROM `OnWorldPostActorTick`, AND THE FIRST ATTEMPT SAMPLED FROM `SessionTick` INSTEAD,
	 * WHICH WAS WRONG BY A WHOLE FRAME.** The reasoning was that a per-frame hook already existed for
	 * the deadline, so `worst=` could ride it and cost nothing. It cannot: `FTSTicker` is pumped
	 * EARLY in the frame and `UWorld::Tick` writes `DeltaRealTimeSeconds` at `LevelTick.cpp:1575`, so a
	 * core-ticker read gets the PREVIOUS frame's cost. `GraphCode.Play.FrameCostOnTheEnvelope` printed
	 * the proof while passing: a record stating `fps=5.8` — one frame across 172ms, the level-load
	 * hitch — carried `worst=8.3ms` beside it. The gate had no assertion that could see it, so the
	 * number was checked by reading the log it printed ([[silence-is-not-sameness]]).
	 *
	 * `OnWorldPostActorTick` broadcasts at `:1877`, after that write, and it fires once per WORLD tick
	 * rather than once per core-ticker pump — which also fixes the case the same record exposed, a
	 * world ticked more than once inside one engine frame.
	 *
	 * Still no new ticker, and the ping cadence is still untouched: the row's specification called for
	 * a sampler because capturing more often to see inside a frame would have made the captures their
	 * own measurement. A delegate the engine already broadcasts is not that.
	 */
	double WorstFrameSeconds = 0.0;

	/** `worst=`'s sample point. Bound for the length of a run — see `WorstFrameSeconds`. */
	FDelegateHandle FrameSampler;

	/** §3.1b's deadline and the control pump. Armed for the whole run, unlike the slice clock. */
	FTSTicker::FDelegateHandle SessionTicker;

	/**
	 * §3.1b's budget and how much of it has been spent, both wall clock. Zero limit means no deadline.
	 *
	 * ELAPSED IS ACCUMULATED, NOT DERIVED FROM A START TIME, because a paused world must not age (see
	 * `SessionTick`). `LastSessionTickAt` is the previous sample, and zero means *no sample yet* — the
	 * first tick must contribute nothing rather than the whole interval since the epoch.
	 */
	double SessionLimit = 0.0;
	double SessionElapsed = 0.0;
	double LastSessionTickAt = 0.0;

	/**
	 * When the end was requested, or zero. **This is also the "is one outstanding" flag**, so a second
	 * request cannot restart the grace clock the first one started.
	 */
	double EndRequestedAt = 0.0;

	/**
	 * Loads waiting to fire, oldest first, and the one currently firing — §3.6d-ii.
	 *
	 * **THERE IS NO "STAGE" MEMBER HERE AND THAT IS THE RULING**, not an omission. A slot that held a
	 * prepared state between dispatches is what forced §3.6d's precedence question (whose stage wins?)
	 * and what let a plan go stale. A plan lives exactly as long as the dispatch it was bound to.
	 */
	TArray<FGcPlayEntryCall> PendingEntries;
	FGcPlayEntryCall FiringEntry;
	bool bFiring = false;
	bool bFiringConsumed = false;

	/** The batch in flight, or none. One per session, because the channel has one order. */
	FGcPlayBatchState BatchState;

	// -------------------------------------------------------------------------------------------
	// §17 P7 — WHO THE RUN IS FOR, AND THE PICTURES IT TOOK
	// -------------------------------------------------------------------------------------------

	/** `for:` in the header. See `EGcPlayAudience`; it decides who may end this run and who may judge it. */
	EGcPlayAudience Audience = EGcPlayAudience::Player;

	/**
	 * Has a driver ever held the lease during this run?
	 *
	 * **WITHOUT THIS, THE DISCONNECT RULE WOULD END EVERY RUN AT ITS FIRST TICK.** `start` does not
	 * require a lease — an agent may open a run and attach afterwards, or never — so *no driver* at
	 * the beginning is an ordinary state and only the transition out of *attached* is a
	 * disconnection. An absence that was always there is not a loss.
	 */
	bool bDriverSeen = false;

	/** Per-name counters for pictures, exactly as `MarkCounts` numbers captures. */
	TMap<FString, int32> ShotCounts;

	/**
	 * Pictures asked for and not yet on disk, oldest first — the gap between *ask* and *it exists*.
	 *
	 * The engine holds ONE screenshot request, so this queue is what lets three marks in a row each
	 * get their own frame instead of overwriting one filename. Every entry leaves through
	 * `ResolveShot`, including at the end of a run: a pending line with no resolution is the shape
	 * [[a-deferred-act-owes-an-outcome]] is about.
	 */
	struct FPendingShot
	{
		FString Name;
		FString Leaf;
		FString Path;
		const TCHAR* By = nullptr;
		FString Comment;
		bool bIssued = false;

		/**
		 * The `.gcui` written beside the picture — R6, [USER, 2026-08-19].
		 *
		 * Filled when the screenshot is REQUESTED, not when it lands, because the engine reads the
		 * surface at the end of the frame it was asked in: the layout and the projection this
		 * records are the frame the picture shows. Carried on the pending shot rather than derived
		 * at resolve time so the record names the file that was actually written.
		 */
		FString UiLeaf;
		FString UiFailure;

		/** V2 item 4: screen messages and `showdebug` off for this frame (`gcp_run shot clean:true`). */
		bool bClean = false;
	};
	TArray<FPendingShot> PendingShots;

	/** Our subscription to `FGraphCodePlayShot::OnLanded`, dropped with the run. */
	FDelegateHandle ShotLanded;

	/**
	 * Ask for a picture — the one path an authored shot and a driver's shot both take.
	 *
	 * `By` is `mark` or `driver`, and it is the only difference between them: a shot is a shot, and
	 * two request paths would be two places for the queue's rules to be got wrong. False means
	 * nothing was queued and `OutFailure` says why, in a sentence meant for the log rather than for
	 * a programmer.
	 */
	bool QueueShot(const FString& Name, const TCHAR* By, const FString& Comment, FString& OutFailure);

	/** The same, for a clean still (V2 item 4): the request goes out with `bClean`. */
	bool QueueShot(const FString& Name, const TCHAR* By, const FString& Comment, bool bClean, FString& OutFailure);

	/** Issue the front of the queue if the engine is free. Called after a request and after each landing. */
	void PumpShots();

	/** Write one `Shot(…)` record — the picture landed, or it never will. */
	void ResolveShot(const FPendingShot& Shot, const FString& Failure);

	/** Every pending picture, answered as failed. The run is closing and nothing more will render. */
	void AbandonShots(const FString& Reason);

	/**
	 * The rolling shot this run is taking, and `Mode == None` when it is taking none — §17 P7a-ii.
	 *
	 * ONE AT A TIME, unlike `PendingShots` which is a queue: a second rolling run would be a second
	 * clock writing into one folder, and the window each was trimming would delete the other's
	 * frames. A `Mark` asking for a different rolling shot ends this one first.
	 */
	FGcPlayShot ShotRun;

	/** When the current rolling run started, for `Limit`. Wall clock, per §14.5. */
	double ShotRunStartedAt = 0.0;

	/** When the next picture in the rolling run comes due. Wall clock. */
	double ShotRunNextAt = 0.0;

	/** The name the rolling run's pictures are numbered under — the `Mark` that started it. */
	FString ShotRunName;

	/** How many pictures this rolling run has asked for, including ones the window has since dropped. */
	int32 ShotRunTaken = 0;

	/** How many the window has deleted, so the closing record can say what was thrown away. */
	int32 ShotRunDropped = 0;

	/**
	 * Absolute paths this rolling run wrote, oldest first — the window, and the ONLY delete list.
	 *
	 * Tracked rather than globbed for the reason `FGcPlayShot::Keep` gives: a pattern over the run
	 * folder would delete the single shots a `Mark` took beside the rolling ones, and a run folder is
	 * where this product's evidence lives.
	 */
	TArray<FString> ShotRunFiles;

	/**
	 * Apply a moment's `Shot` input: continue the rolling run, replace it, or take one picture.
	 *
	 * Appends to `Fields` rather than writing its own line, because the whole point of §3.1's one-node
	 * rule is that a moment's media are fields on the moment.
	 */
	void ApplyShot(const FString& Name, const FGcPlayShot& Shot, TArray<FString>& Fields);

	/** Due another picture? Called every tick from `SessionTick`, and cheap when nothing is rolling. */
	void TickShotRun(double Now);

	/** Close the rolling run and write its `Shots(…)` record. `By` is why it stopped. */
	void StopShotRun(const TCHAR* By);

	/** Trim the window to `Keep`, deleting the oldest pictures THIS rolling run wrote. */
	void TrimShotWindow();

	// ---------------------------------------------------------------------------
	// The record — §3.1f, and the rolling shot's approach applied to state
	// ---------------------------------------------------------------------------

	/** The open span's writer, or unset. One at a time: a mark closes whatever was open. */
	TUniquePtr<FGraphCodePlayRecordWriter> RecordRun;

	/** What the log's `record=$NAME` refers to — the opening mark and the frame it opened on. */
	FString RecordRunName;

	/**
	 * The pin value that opened the span, kept so the next `Mark` can be compared against it.
	 *
	 * **THIS IS THE WHOLE OF *CONTINUES OR ENDS*.** A mark carrying an equal record goes on watching
	 * the same span; anything else closes it. `ShotRun` gets this for free by BEING the pin value,
	 * which a record cannot do — its writer holds a resolved span with a name, a start frame and a
	 * parsed selection that the author never typed.
	 */
	FGcPlayRecord RecordRunSpec;

	/** Wall clock when the span opened, for `Limit`. The shot's `ShotRunStartedAt` twin. */
	double RecordRunStartedAt = 0.0;

	/** Wall clock of the next due sample. Advances to the next DUE moment, never by one interval. */
	double RecordRunNextAt = 0.0;

	/**
	 * The open span's cadence and backstop, in seconds.
	 *
	 * HELD HERE RATHER THAN READ BACK OFF THE WRITER, because the writer's copy is the SPAN'S — what
	 * the file's header claims — and this is the SESSION'S clock. Two carriers of one fact is a
	 * race ([[two-carriers-of-one-fact-is-a-race]]); these are two different facts that happen to
	 * start equal, and only this one is allowed to drive the tick.
	 */
	double RecordRunEverySeconds = 0.0;
	double RecordRunLimitSeconds = 0.0;

	/**
	 * Apply a moment's `Record` input: close whatever was open, then open a new one if asked.
	 *
	 * Appends to `Fields` rather than writing its own line, for `ApplyShot`'s reason — a moment's
	 * media are fields on the moment.
	 */
	void ApplyRecord(const FString& Name, const FGcPlayRecord& Record, TArray<FString>& Fields);

	/** Due another sample? Called every tick from `SessionTick`, and cheap when nothing is open. */
	void TickRecordRun(double Now);

	/** End the span. `ToMark` is the mark that closed it, empty when a limit or the run did. */
	void StopRecordRun(const FString& ToMark = FString());

	/**
	 * Every span this run finished, in the order they closed.
	 *
	 * Kept so the session can keep the log's closing `records { }` block CURRENT — the shape §3.1i
	 * designs. The two things the block adds, `cast { }` and an act's `ends=`, are still knowable
	 * only in arrears, but *in arrears* is not *at the end*: a span that has run for a minute knows
	 * both for everything except its own last act, and that one absence is information rather than
	 * a gap ([[a-log-writes-immediately-not-completely]]).
	 */
	TArray<FGcPlayRecordBuffer> ClosedRecords;

	TUniquePtr<IFileHandle> LogFile;

	/**
	 * Where in the log the header's `alive:` timestamp starts, in bytes.
	 *
	 * **THE ONE THING IN THIS FILE THAT IS OVERWRITTEN RATHER THAN APPENDED** [USER, 2026-08-21]:
	 * *"have a time in the header, to track the time… so a game crash we know when it crash."* A
	 * crashed run writes no `End(…)` and no `records { }` block, which says THAT it died and not
	 * WHEN. This does, and it can only be a fixed-width field at a known offset: an ISO-8601 stamp
	 * is always the same length, so restamping it never moves a byte of anything written after it.
	 */
	int64 AliveOffset = 0;

	/** Real time the stamp above was last written, so the periodic restamp is not a per-tick write. */
	double AliveStampedAt = 0.0;

	/** Restamp `alive:` if it is due, or unconditionally when a line has just been written. */
	void StampAlive(bool bForce);

	/**
	 * Where the FINAL part of the log ends, in bytes. Nothing below this is ever seeked over again.
	 *
	 * **THIS FILE HAS A REWRITABLE TAIL, AND IT IS BIGGER THAN THE BLOCK** — [USER, 2026-08-21]:
	 * *"all mark lines kept updating."* Every mark of an open span carries that span's running total,
	 * so the region that can still change starts at the mark which OPENED it and runs to the end of
	 * the file. Below `CommittedEnd` the bytes are settled: a closed span's marks hold the number
	 * they finished on and nothing will touch them.
	 */
	int64 CommittedEnd = 0;

	/**
	 * The lines between `CommittedEnd` and the block, held because the session is the only thing
	 * that can put them back.
	 *
	 * **BOUNDED BY THE SPAN, WHICH IS WHAT MAKES IT AFFORDABLE.** With no record open a line is
	 * written and committed in the same breath, so a run that never records anything keeps none of
	 * this — and a span holds only the handful of authored lines between its two marks.
	 */
	TArray<FString> LiveLines;

	/** Their byte length as last written, so committing them can advance `CommittedEnd` exactly. */
	int64 LiveBytes = 0;

	/**
	 * The `records { }` block as text. Computed by `BuildRecordBlock`, laid down by `WriteTail`.
	 *
	 * The two are separate because the block is no longer the only rewritable thing in the file: the
	 * marks above it are too, and both have to go down in ONE pass or the boundary between them is
	 * wrong for as long as it takes to do the second.
	 */
	FString RecordBlockText;

	/** Recompute `RecordBlockText` from the closed spans and the running one. Writes nothing. */
	void BuildRecordBlock();

	/**
	 * Lay the live lines and the block down from `CommittedEnd`, with every `acts=` set to `Acts`.
	 *
	 * **IT SHARES `alive:`'S CLOCK ON PURPOSE.** That stamp is the file's promise about how stale it
	 * is; a tail refreshing on a different cadence would make the promise true of the header and a
	 * lie about everything below it. One clock, one answer to *how far behind is this file*.
	 */
	void WriteTail(int32 Acts);

	/** Write the region one last time with the number it will keep, then stop owning those bytes. */
	void CommitLiveLines(int32 FinalActs);
};
