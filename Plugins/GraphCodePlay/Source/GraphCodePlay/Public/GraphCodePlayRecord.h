// What a chosen cast DID between two marks — the record, `PLAY-SESSION-DESIGN.md` §3.1i.
//
// **ONE FILE. THE RECORD LIVES IN THE SESSION LOG AND NOWHERE ELSE** [USER, 2026-08-21, §3.1i-A —
// the spec is the file the user wrote, and where it and anything else disagree, it wins]. There is
// no `.gcpr` sidecar, no header of its own, no `Records(…)` summary line and no `watch:`,
// `functions:` or `precision:` line at either scope. The span's observations are held as lines and
// the session keeps the `records { }` block at the end of the log built from them, refreshed as the
// run goes rather than assembled once at the end ([USER, 2026-08-21]: *"record is keep updating"*).
//
// **WHAT SURVIVES OF *USE THE SAME APPROACH AS THE PERIODIC SCREENSHOT* (2026-08-20) IS THE
// MECHANISM, NEVER THE VOCABULARY.** That sentence rules WHEN a thing is observed, and it was read
// as ruling what it is CALLED — so the shot's whole log surface got cloned onto a record, one file
// and one summary line at a time ([[an-analogy-is-not-a-licence-to-invent]]). Two properties of it
// are real and both are kept:
//
//   1. **The cadence advances to the next DUE moment, never by one interval from now**, so a hitch
//      does not drag the whole sample clock behind it and a very late frame does not queue a burst
//      of catch-up samples. `TickShotRun`'s rule, minus its counting — `skipped=` does not exist.
//   2. **A `Limit` backstop for the mark that never comes** — the record's MAX TIME [USER,
//      2026-08-21] — checked before the cadence so a span that is over does not take one more
//      sample on its way out.
//
// And the third thing the shot settles is how a span ENDS: *"it should end on next mark. unless
// next mark also connect to that shot"* [USER, 2026-08-19], which `ApplyShot` spells as value
// equality. A `Mark` carrying the same `FGcPlayRecord` continues the span; any other mark ends it.
// **The record was ruled onto that same rule 2026-08-21** — *"see how periodic capture work"* —
// which is what a mark ending a record means, and it needs no word the grammar does not have.
//
// ---------------------------------------------------------------------------
// WHY THE OBSERVATIONS ARE HELD AS LINES AND THE BLOCK IS BUILT FROM THEM
// ---------------------------------------------------------------------------
//
// `cast { }` and `ends=` are the two things the spec's file has that no forward pass can produce:
//
//   `cast { }`   The block is complete only once the span is over — who appeared is not known
//                until nobody else can. **THAT IS NOT THE SAME AS *NOT UNTIL THE RUN ENDS***: the
//                block is on disk and current from the moment a span opens, and what is missing
//                from it while the fight runs is the report rather than a gap. **NAMES AND CLASSES ONLY, `=` aligned in a column**; the opening
//                state does NOT ride under each entry (it did until 2026-08-21, and §3.1i-A item 6
//                deleted it: `TargetActor=unset -> Hero` already carries the opening value ON the
//                arrow, so printing it twice is two carriers of one fact).
//   `ends=`      An act's end is not known when the act begins. It is the next act naming that
//                same actor, or the span's own end — so every act except each actor's latest has
//                one while the span is still running.
//
// **AND A RUN THAT DIED HAS NO BLOCK AND NO `End(…)`, WHICH IS HOW ANYBODY KNOWS IT DIED** [USER,
// 2026-08-21]: *"if it drops, or crashed, we will not see the end= parameter."* That absence is the
// crash report, so nothing here carries a `by=` to explain itself. WHEN it died is the session
// header's `alive:`, restamped as the run writes.
//
// ---------------------------------------------------------------------------
// WHY THE SAMPLER DIFFS TEXT AND OWNS NO READER OF ITS OWN
// ---------------------------------------------------------------------------
//
// `FGraphCodeExecutionState::Collect` already spells what a running character DECIDED, and its
// own first rule is *record what the game decided, never how far it has got* — identity, not
// progress. That is exactly the property a change detector needs: what it returns is stable frame
// to frame while nothing is decided, and `GraphCode.WorldSnapshot.IdleGate` is the existing test
// that holds it to that. So the sampler compares this tick's directives against last tick's and
// writes an `Act(…)` when they differ.
//
// **AND THE ACT STATES THE CHANGE, NOT THE STATE — `@blackboard[Alerted=false -> true]`** (R1,
// 2026-08-21). The arrow is `FGraphCodeValueDelta`'s, which is the same codec `gc_play mode:"diff"`
// answers with, so *what changed* is spelled one way across the product. That is also why `Collect`
// exists: the delta needs the PAYLOAD, and recovering it by parsing this file's own `@…[…]` lines
// back apart would make the record a second reader of the writer it shares
// ([[a-read-answers-through-the-write-path]]).
//
// **A SECOND READER OF THE SAME SUBJECT WOULD DRIFT FROM THE CAPTURE'S**
// ([[a-read-answers-through-the-write-path]]), and the drift would be invisible: a record and a
// capture taken in the same run would describe one blackboard two ways, and only the reader
// comparing them would ever find out.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeSnapshotDocument.h"
#include "GraphCodeWorldSelector.h"
#include "UObject/ObjectKey.h"

class AActor;
class UFunction;
class UWorld;
struct FBlueprintExceptionInfo;
struct FFrame;

#if WITH_GRAPHCODE_ABILITIES
class UAbilitySystemComponent;
class UGameplayAbility;
struct FAbilityEndedData;
struct FGameplayTagContainer;
#endif

/**
 * The function-selector rule — R5, [USER, 2026-08-21]: *"so you declare a name, our system check
 * against name, category and keywords."*
 *
 * **PUBLIC AND FREE, BECAUSE IT IS A RULE RATHER THAN A STEP.** The record's own use of it is
 * cached and buried on the VM's hottest path, so a gate reaching it through the listener would have
 * to run a Blueprint to ask a question that involves no Blueprint at all. Named here, the three
 * carriers and their near-misses are provable against any `UFunction` — including a native one,
 * which the LISTENER can never see and the RULE has no reason to refuse.
 */
namespace GraphCodePlayCalls
{
	/**
	 * Whether Token selects Function: its name, any segment of its `Category` (split on `|`), or
	 * any of its `Keywords` (split on whitespace). Case-insensitive on all three.
	 */
	GRAPHCODEPLAY_API bool TokenSelects(const UFunction& Function, const FString& Token);
}

/**
 * What one component was last seen deciding — the unit a change is measured against.
 *
 * **KEYED BY THE COMPONENT, NOT FLATTENED INTO ONE LIST PER ACTOR.** An actor can own two things
 * that both spell `@blackboard`, and a flat list would match this tick's first against last tick's
 * first — which is right until the day one of them is added or removed, and then every line after
 * it is a change that did not happen. Matching the component first costs one map lookup and cannot
 * misalign.
 */
struct FGcPlayRecordedComponent
{
	FObjectKey Component;
	TArray<FGcpDirective> Directives;
};

/**
 * How a `Mark` opened a record: what to follow, how often to look, and when to give up waiting for
 * the mark that closes it.
 *
 * Deliberately NOT a `USTRUCT` — this is the resolved span the writer works from, assembled from
 * the Blueprint-facing `FGcPlayRecord` pin. Keeping the reflected pin and the working struct
 * separate is what let the pin stay small while this grew a start frame, a precision and a parsed
 * selection.
 */
struct FGcPlayRecordSpan
{
	/**
	 * What the log's `record=$NAME` refers to and what names the entry in `records { }`.
	 *
	 * **THE MARK'S NAME AND THE FRAME IT OPENED ON** [USER, 2026-08-21]: *"actually use framenumber,
	 * so BOSSFIGHT_11084."* A record is not simply named for its mark, because a run can mark one
	 * name twice and two entries of one name in one block is a reference that resolves to the wrong
	 * document rather than to none ([[a-digest-is-not-a-document]]). The frame it started on is
	 * already unique, already in the file beside it, and needs no uniquer counting anything.
	 */
	FString Name;

	/** The mark that opened it, quoted onto the block's `from=` so the record says where it began. */
	FString FromMark;

	/** Any mix of `class:` `name:` `tag:` `near:` `*` — the same language `select:` speaks. */
	TArray<FString> Watch;

	/**
	 * Which function calls earn a `Call(…)` line — R5, [USER, 2026-08-21].
	 *
	 * One declared token per entry, matched against a Blueprint function's NAME, any segment of its
	 * `Category` and any of its `Keywords`. Empty listens for nothing, which is the default.
	 */
	TArray<FString> Calls;

	/**
	 * Tracked names this span does not want — matched by `FGraphCodeExecutionState::IsExcluded`.
	 *
	 * It is carried on the span rather than only inside the execution-state options because the
	 * span has to STATE it: `exclude=` goes on the `record(…)` line so a reader can tell a quiet
	 * fight from a heavily filtered one, and two records taken under different rules apart.
	 */
	TArray<FString> Exclude;

	/** Seconds between samples. Zero means every tick, which is a legitimate ask for a short span. */
	float Every = 0.25f;

	/** Seconds before this closes itself. Zero means *wait for the mark*, however long that takes. */
	float Limit = 0.0f;

	/**
	 * Most observations to write before this stops writing them and starts counting them.
	 *
	 * **A CAP, NOT THE ROLLING SHOT'S WINDOW, and the different word is the point.** `Shot`'s
	 * `Keep` keeps the LAST N pictures by deleting older files — a dashcam. The same behaviour here
	 * means rotating an append-only text file, which is a different mechanism and not one this
	 * increment builds. Calling a cap `keep=` would put one word on two behaviours, which is how a
	 * reader ends up with two meanings for one silence ([[one-concept-two-grammars]]).
	 *
	 * Zero means no cap, which is the default. **WHAT A CAP DROPS IS NOT REPORTED** [USER,
	 * 2026-08-21] — `dropped=` and `skipped=` do not exist. Their only home was the deleted
	 * `Records(…)` line, and the record's own line states what the span WAS, never what watching it
	 * cost. So a cap set too low silently shortens the span, and the number is the author's to pick.
	 */
	int32 Max = 0;

	/** The run's own, so a directive here rounds exactly as it would in a capture from the same run. */
	double Precision = 0.01;
	int32 Decimals = 2;

	/** Where the span began, for the sidecar's header. */
	int64 StartFrame = 0;
	double StartTime = 0.0;
};

/**
 * A closed span, as the lines it produced — what one entry of `records { }` is assembled from.
 *
 * **THE SPAN'S OWN FACTS TRAVEL BESIDE ITS LINES RATHER THAN INSIDE THEM.** They used to be a
 * `gcp_record 1 …` header at the top of the sidecar, which the fold then parsed back apart; with
 * the file gone, writing a header purely to read it again would make the assembler a second reader
 * of its own writer ([[a-read-answers-through-the-write-path]]).
 */
struct FGcPlayRecordBuffer
{
	/** `records { }`'s key, and what the opening `Mark`'s `record=$NAME` points at. */
	FString Name;

	/** `(925, 0.01)` — the frame and time the span opened on, as `start=` wants it. */
	FString Start;

	/** The mark that opened it, already quoted, for `from=`. */
	FString FromMark;

	/**
	 * What the span was pointed at and what it was told to ignore, already quoted, for `watch=` and
	 * `exclude=` on the `record(…)` line.
	 *
	 * **THE RECORD STATES ITS OWN RULES — [USER, 2026-08-21], answering the one spelling §3.1i left
	 * open**: *"in the record."* Two records that look alike may have been taken under different
	 * rules, and without these a reader cannot tell a quiet fight from a heavily filtered one — nor
	 * compare two runs and know they were comparable. `watch:` had been printed NOWHERE since the
	 * header was struck, which is the same hole seen from the other side.
	 *
	 * Empty means *nothing excluded*; the field is omitted rather than written empty, because an
	 * absent rule and a rule that excludes nothing are the same fact and the file states facts once.
	 */
	FString Watch;
	FString Exclude;

	/**
	 * The declared function selectors, already quoted, for `calls=` — R5.
	 *
	 * Carried for `Watch`'s reason and not for symmetry: a span that listened for calls and heard
	 * none is indistinguishable from one that never listened unless the line says which it was.
	 */
	FString Calls;

	/** `cast …`, `Act(…)` and `  @…` in the order they were seen, and the span's own `End(…)`. */
	TArray<FString> Lines;
};

/**
 * The watcher: samples the cast on a cadence and holds what changed, as lines, until the log's
 * `records { }` block is assembled.
 *
 * One instance per open record, owned by the session. Not static — unlike the shot request, which
 * is process state because the engine holds exactly one, a record is entirely this session's.
 */
class GRAPHCODEPLAY_API FGraphCodePlayRecordWriter
{
public:
	FGraphCodePlayRecordWriter() = default;

	/**
	 * **DECLARED RATHER THAN DEFAULTED, BECAUSE AN ABANDONED SPAN STILL HOLDS DELEGATE HANDLES.**
	 * `Close` releases them, and a run that died never reaches `Close` — so this is the second door,
	 * and it is the one that matters: the handles capture a raw `this` into ability systems that may
	 * outlive the writer, and a call into a freed one is not a wrong line but a crash.
	 */
	~FGraphCodePlayRecordWriter();

	FGraphCodePlayRecordWriter(const FGraphCodePlayRecordWriter&) = delete;
	FGraphCodePlayRecordWriter& operator=(const FGraphCodePlayRecordWriter&) = delete;

	/**
	 * Resolve the watched cast and open the span.
	 *
	 * Fails when the selectors do not parse — and it fails LOUDLY rather than following nothing,
	 * because a record that quietly watched an empty cast produces a valid, empty, entirely
	 * misleading entry ([[a-gate-on-the-desired-state-cannot-see-a-removal]]).
	 */
	bool Open(const FGcPlayRecordSpan& Span, UWorld* World, FString& OutError);

	/**
	 * Look at the cast once and append whatever changed since the last look.
	 *
	 * Called on the cadence, never every tick unless `Every` is zero. The frame and time come from
	 * the session so every line in the record shares the log's own envelope.
	 */
	void Sample(UWorld* World, int64 Frame, double Time);

	/**
	 * End the span: this is what closes the last act of every actor, and what `ends=` reads.
	 *
	 * `ToMark` is the mark that closed it, or empty when a limit or the run's own end did.
	 *
	 * **THERE IS NO `by=`, AND `to=` IS WHY.** The reason a span ended used to be a word on the
	 * deleted `Records(…)` line — `mark`, `limit`, `session` — and every one of those is already
	 * legible here: a span closed by a mark names it, one that was not has no `to=`, and one that
	 * never ended at all has no `ends=` because the run did not survive to write it. Keeping the
	 * word as well would be two carriers of one fact ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	void Close(int64 Frame, double Time, const FString& ToMark = FString());

	/**
	 * Assemble closed spans into the `records { }` block the log ends with — §3.1i.
	 *
	 * **THIS IS THE ONE DEFERRED STEP IN THE WHOLE FORMAT, AND IT IS WHAT `cast { }` AND `ends=`
	 * COST.** The log goes on appending its own lines for the length of a span — a `Shot` taken
	 * mid-record would land inside the block — so the block cannot be open while the record is.
	 * Both of the things it adds are knowable only once nobody else can appear and no act can still
	 * be running, which is the same moment.
	 *
	 * A run that died reaches neither this nor its own `End(…)`, and that pair of absences is the
	 * crash report [USER, 2026-08-21]. Nothing is written to say so.
	 */
	static void Fold(const TArray<FGcPlayRecordBuffer>& Records, TArray<FString>& OutLines);

	bool IsOpen() const { return bOpen; }

	/** The span's lines so far — what proves an observation lands at the moment it is seen. */
	const TArray<FString>& Seen() const { return Lines; }

	/** The closed span, moved out for the session to keep. */
	FGcPlayRecordBuffer Take();

	/**
	 * The span AS IT STANDS, copied — what a still-running record contributes to the block on disk.
	 *
	 * **THE BLOCK IS NOT ASSEMBLED AT THE END AND NEVER SHOULD HAVE BEEN** [USER, 2026-08-21]:
	 * *"since when i said record is assembled at the end? record is keep updating."* The deferred
	 * version had a defect its own design rule forbids: `Fold` ran once, after the run's `End(…)`,
	 * so a run that CRASHED lost every record entirely. *No `ends=` means it crashed* can only be
	 * read off a record that is on disk to be missing one.
	 *
	 * Nothing in `Fold` had to change to take one of these. A record still running carries no
	 * `End(…)` line of its own, so it gets no `ends=` on its `record(…)` line and its last act per
	 * actor gets none either — which is exactly the half-written state a reader is owed.
	 */
	FGcPlayRecordBuffer Snapshot() const;

	/** Observations kept. What the cap counts against, and what a gate reads to prove a span ran. */
	int32 Written() const { return WrittenCount; }

	/** How many distinct actors ever appeared — the size of `cast { }`. */
	int32 CastCount() const { return DocumentNames.Num(); }

private:
	/** Everything a buffer carries except the lines — shared by `Take` and `Snapshot`. */
	FGcPlayRecordBuffer Describe() const;

	/** One observation, one line, in the order it was seen. */
	void Append(const FString& Line);

	/** Every component of this actor that owes execution-state directives, as values not lines. */
	void ReadState(const AActor* Actor, TArray<FGcPlayRecordedComponent>& Out) const;

	/**
	 * The `@…` lines an `Act` carries: what CHANGED between two looks, in the arrow spelling.
	 *
	 * Empty means nothing was decided, and the caller writes no act at all. A directive that is new
	 * since the last look stands as its whole payload — there is nothing to arrow FROM — and one
	 * that stopped being reported is spelled with an empty payload, which is what the `.gcp` delta
	 * has spelled a gone directive since it shipped.
	 */
	void DeltaLines(const TArray<FGcPlayRecordedComponent>& Before,
		const TArray<FGcPlayRecordedComponent>& Now, TArray<FString>& OutLines) const;

	/** The name this actor is called, resolved once and remembered — a record must not rename mid-span. */
	const FString& NameFor(const AActor* Actor, const UWorld* World);

	/**
	 * The same name, and the `cast` line the first time this actor shows anything at all.
	 *
	 * **`DocumentNames` IS BOTH THE NAME CACHE AND THE HAS-BEEN-INTRODUCED SET — `CastCount()`
	 * READS IT — SO THE TWO CANNOT BE WRITTEN SEPARATELY.** A caller that resolved a name without
	 * appending the line left that actor named in its own statements, absent from `cast { }`, and
	 * counted in `CastCount()`; and because the key was now present, the NEXT thing it did was
	 * treated as a repeat appearance and never introduced it either. The ability listener did
	 * exactly that until this existed — one actor, three ways of being in the record
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * An actor is introduced by the first thing it actually SHOWS, which is why this is called from
	 * the act path and the ability path and from nowhere else: being watchable is not an appearance.
	 */
	const FString& IntroduceCast(AActor* Actor, const UWorld* World);

	/**
	 * Listen for Blueprint function calls — R5. Bound by `Open`, and only when `Calls` is non-empty.
	 *
	 * **A CALL IS AN EVENT, WHICH IS WHY THIS IS A LISTENER AND NOT A SAMPLE** — the ability
	 * listener's argument one field over. A call that happens between two samples leaves no state
	 * behind, so a sampler would report that nothing happened.
	 *
	 * **NOT GATED ON AN OPTIONAL PLUGIN.** `OnScriptException` is CoreUObject's and always exists.
	 * What is conditional is narrower and lives inside `SelectsFunction`: two of the three carriers
	 * are metadata, and metadata is `WITH_EDITORONLY_DATA`.
	 */
	void WatchCalls();

	/** Released by `Close`, and by the destructor when a span was abandoned. */
	void UnwatchCalls();

	/**
	 * Every executed Blueprint node in the running world arrives here, so the order of its tests is
	 * the design rather than an implementation detail — cheapest and most-rejecting first.
	 */
	void OnScriptEvent(const UObject* Context, const FFrame& Stack,
		const FBlueprintExceptionInfo& Info);

	/**
	 * Whether a declared token selects this function — [USER, 2026-08-21]: *"so you declare a name,
	 * our system check against name, category and keywords."*
	 *
	 * Cached per `UFunction` because the answer cannot change while a span is open and the question
	 * is asked on the hottest path this product has.
	 */
	bool SelectsFunction(const UFunction& Function) const;

	FDelegateHandle ScriptHandle;

	/**
	 * Function to the lowest script offset a tracepoint has been seen at in it — the entry site.
	 *
	 * **THE ENTRY IS AN OFFSET, NOT A FRAME, AND THE FRAME VERSION WAS BUILT AND FOUND WRONG.**
	 * Tracking `FFrame` addresses models the call stack only while every return is observable, and
	 * the outermost return never is: the caller of the first Blueprint call is native C++, which has
	 * no tracepoint to pop it with. The frame stayed on the stack, the next call reused the address,
	 * and two calls produced one line — the failure mode the row named, reached by the fix for it.
	 *
	 * An offset is a property of the bytecode, so nothing has to unwind and no return has to be
	 * seen. `ConstructFunction`'s linear execution list starts at the entry node, so the entry's
	 * debug site is the function's lowest, and a second invocation returns to it.
	 */
	TMap<FObjectKey, int32> FirstCallSiteOffset;

	/** Function to whether it is selected. Resolved once; a span cannot change its own selectors. */
	mutable TMap<FObjectKey, bool> SelectedFunctions;

	/**
	 * The world the span is watching, for the clock a delegate does not carry.
	 *
	 * **NOT UNDER THE ABILITIES GATE, and it was until R5.** Both listeners need a clock and only
	 * one of them is optional, so a build without `GameplayAbilities` compiled the field away and
	 * took the call listener's clock with it.
	 */
	TWeakObjectPtr<const UWorld> ClockWorld;

#if WITH_GRAPHCODE_ABILITIES
	/**
	 * Bind one actor's ability system, once — R3b.
	 *
	 * **AN ACTIVATION IS AN EVENT, WHICH IS WHY THIS IS A LISTENER AND `@tags` IS NOT.** A tag set is
	 * state, so diffing two `Collect` results finds every change that outlived a sample; an ability
	 * that activated and ended between two samples leaves no state behind at all, and a sampler would
	 * report that nothing happened. That is the same argument R5 makes for the script VM, and it is
	 * why these two halves of one row shipped separately.
	 *
	 * Bound during `Sample` rather than in `Open`, because an actor the selector matches may not
	 * exist yet when the span opens — the record already re-resolves its cast on every look, and a
	 * one-shot bind would silently miss everything spawned mid-span.
	 */
	void WatchAbilities(AActor* Actor);

	/** Every handle released. Called from `Close`, and from the destructor if a span was abandoned. */
	void UnwatchAbilities();

	/**
	 * The ACTOR is passed in, not searched for — the delegates do not carry one.
	 *
	 * **NEITHER CALLBACK NAMES ITS ABILITY SYSTEM**, so a handler that recovered the actor by walking
	 * the watch list answered with whichever entry came first and was right only while exactly one
	 * actor was watched. A record whose whole purpose is *who did what* would then attribute every
	 * refusal to one actor, consistently — and consistently wrong is what survives a gate. Binding a
	 * lambda per ability system closes it: the actor is captured where it is still known.
	 */
	void OnAbilityActivated(UGameplayAbility* Ability, AActor* Actor);
	void OnAbilityEnded(const FAbilityEndedData& Data);
	void OnAbilityFailed(const UGameplayAbility* Ability, const FGameplayTagContainer& Reason,
		AActor* Actor);

	/**
	 * The three handles one ability system costs, and the actor they belong to.
	 *
	 * **THE ACTOR, NOT ITS DOCUMENT NAME.** `IntroduceCast` names an actor and puts it in `cast { }`
	 * in one step, so naming one at bind time would list an actor that went on to do nothing — and
	 * the cast block's rule is that an actor is introduced by the first thing it actually shows. An
	 * activation is such a thing; being bindable is not.
	 */
	struct FWatchedAbilitySystem
	{
		TWeakObjectPtr<UAbilitySystemComponent> Component;
		TWeakObjectPtr<AActor> Actor;
		FDelegateHandle Activated;
		FDelegateHandle Ended;
		FDelegateHandle Failed;
	};
	TArray<FWatchedAbilitySystem> WatchedAbilities;

	/**
	 * A running ability, to the index of its own unfinished line in `Lines`.
	 *
	 * **THE LINE IS APPENDED WHEN THE ABILITY STARTS AND COMPLETED WHEN IT ENDS**, so the file keeps
	 * activation ORDER while still carrying `ends=` and `result=`, neither of which is knowable at
	 * the moment it begins. `Act(…)` solves the same problem in `Fold`, by the rule *an act runs
	 * until the next line naming that actor* — a rule that is WRONG here, because an ability ends
	 * when it ends and not when its owner next does something else.
	 */
	struct FOpenAbility
	{
		int32 LineIndex = INDEX_NONE;
		/** `frame=…, time=…` as it was when the ability STARTED. Not restamped at the end. */
		FString Envelope;
		FString ActorName;
		FString Tag;
		FString From;
	};
	TMap<FObjectKey, FOpenAbility> OpenAbilityLines;
#endif

	/** The span's own lines, in the order they were seen. The file layer this replaced is deleted. */
	TArray<FString> Lines;
	bool bOpen = false;

	FGcPlayRecordSpan Span;
	FGraphCodeWorldSelection Selection;

	/** Actor to the name it was given. Also the cast list, and the *have I introduced this one* set. */
	TMap<FObjectKey, FString> DocumentNames;

	/** Actor to what it last showed, which is what a change is measured against. */
	TMap<FObjectKey, TArray<FGcPlayRecordedComponent>> LastState;

	/** Resolved once per span. Rebuilding it per sample would let an actor be renamed mid-record. */
	TMap<FString, int32> NameCounts;
	bool bNameCountsBuilt = false;

	int32 WrittenCount = 0;
};
