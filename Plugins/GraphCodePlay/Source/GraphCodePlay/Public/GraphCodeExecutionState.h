// What a running character DECIDED, for a `.gcp` capture — RUNTIME-DRIVE-DESIGN.md §20.
//
// **THE ONE RULE THIS FILE IS BUILT ON: record what the game decided, never how far it has got**
// (§20.5). Which state, which node, which value are IDENTITY and change when the game decides
// something. Elapsed time, blend weight, node memory and asset-player position are PROGRESS and
// change every frame — writing any of them makes every capture of a motionless world differ from
// the one before it, which is not a hypothetical: `GraphCode.WorldSnapshot.IdleGate` already fails
// on exactly that. The gate for this feature was written before the feature.
//
// Four subjects, of which the last two are behind their plugin's gate:
//
//   @blackboard[…]     what it knows            UBlackboardComponent
//   @behaviorTree[…]   what it is doing         UBehaviorTreeComponent
//   @tags[…]           what it IS               UAbilitySystemComponent   WITH_GRAPHCODE_ABILITIES
//   @stateTree[…]      what it is doing         UStateTreeComponent       WITH_GRAPHCODE_STATETREE
//   ~~@stateMachine~~  what it LOOKED like      REMOVED — [USER]: *"if a field is derived, like
//                                               state machine, that don't affect final output, then
//                                               don't even show."* Both halves held: an animation
//                                               state is derived from the anim instance's variables
//                                               AND it is cosmetic. The behavior tree is derived
//                                               too and stays, because what an AI is doing is not.
//                                               A pose belongs to `gc_render source:"viewport"`,
//                                               which is the instrument for what a player sees.
//
// **WHY THIS IS IN PLAY AND NOT IN THE SHARED GRAMMAR** — §20.4 item 3, ruled by building it.
// `GraphCodeRuntime` is depended on by Core (`Editor`) and Play (`Runtime`) alike, so an `AIModule`
// dependency there would reach the level projection, `describe`, import and all three merge tiers,
// none of which has a running AI to ask. `GraphCodeMove` is `Type: "Editor"` and a `Runtime` capture
// may not call it. Play owns the capture and can take the dependency, so Play takes it, and what
// crosses into the shared grammar is `FGraphCodeActorEntryOptions::ComponentDirectives` — the SHAPE
// of a directive line and nothing about its contents.
//
// **VALUES GO THROUGH THE SAME SPELLING AS EVERY OTHER LINE IN THE FILE, which is why
// `DescribeKeyValue`/`DescribeActiveTasks` are not used** (§20.3). Those return debug prose: an
// object-typed key would print a display name that resolves to nothing, where §5.4's promise is that
// a reference names another entry in the same capture. The structured route — the key's own
// `UBlackboardKeyType` — is the only one that keeps that promise.

#pragma once

#include "CoreMinimal.h"

class UActorComponent;

/**
 * Which subjects a capture carries. **All three default to ON** — [USER, 2026-08-16] ruled them
 * members of the `transient` family that differ from it in exactly that (§20.8), which is why there
 * is no `execution:` line: `omits:` was already the echo, and a category absent from it is one the
 * file carried.
 */
struct GRAPHCODEPLAY_API FGraphCodeExecutionStateOptions
{
	bool bBlackboard = true;
	bool bBehaviorTree = true;

#if WITH_GRAPHCODE_ABILITIES
	/**
	 * Owned gameplay tags — R3, and the field is GATED rather than always present.
	 *
	 * **AN UNGATED BOOL HERE WOULD MAKE `Tokens` LIE.** With `GameplayAbilities` off nothing can
	 * produce a `tags` line, so advertising the category in `always:` would promise a reader a
	 * subject the binary cannot see — the opposite of what the envelope is for. Gated, a Core-only
	 * install never names it, and `omits:` never has to explain a silence that has no cause.
	 */
	bool bAbilities = true;
#endif

#if WITH_GRAPHCODE_STATETREE
	/**
	 * The active states of a `UStateTreeComponent` — R4, gated for the same reason `bAbilities` is.
	 *
	 * With `GameplayStateTree` off nothing can produce a `stateTree` line, so a bool that existed
	 * anyway would put the category in `always:` and promise a reader a subject the binary cannot
	 * see.
	 */
	bool bStateTree = true;
#endif

	/**
	 * Names NOT to track — [USER, 2026-08-21]: *"can you add exclude list, it takes a list of
	 * exclude strings, and match with tags/name… the exclude is excluding the trackings, like
	 * states, or behaviours etc."*
	 *
	 * **THE FILTER IS ON WHAT IS TRACKED, NOT ON WHO IS WATCHED.** `Watch` already answers *which
	 * actors*, and it is the wrong instrument for the problem this solves: one blackboard key that
	 * genuinely moves — a distance, a countdown, an ammo count — writes an `Act` on every sample, and
	 * the record fills with it. `Precision` blunts a float that only wobbles; it cannot help with a
	 * value that really is changing.
	 *
	 * **ONE LIST FOR EVERY SUBJECT, because a per-subject exclude would be four grammars for one
	 * idea** ([[one-concept-two-grammars]]). A blackboard key, a behaviour-tree node, a gameplay tag
	 * and a state-tree state are all *a thing being tracked, by name*, and the matching rule reads
	 * all four — see `FGraphCodeExecutionState::IsExcluded` for what a match is.
	 *
	 * **AND IT LIVES HERE RATHER THAN ON THE RECORD, so the capture and the record cannot disagree.**
	 * Both go through `Collect`; an exclude that only the record knew about would describe one
	 * blackboard two ways in one run, which is the drift only a reader comparing both files would
	 * ever find ([[a-read-answers-through-the-write-path]]).
	 */
	TArray<FString> Exclude;

	/** The capture's own, so a directive cannot round a number differently from the lines above it. */
	double Precision = 0.01;
	int32 Decimals = 2;

	bool Any() const
	{
#if WITH_GRAPHCODE_ABILITIES
		if (bAbilities)
		{
			return true;
		}
#endif
#if WITH_GRAPHCODE_STATETREE
		if (bStateTree)
		{
			return true;
		}
#endif
		return bBlackboard || bBehaviorTree;
	}
};

class GRAPHCODEPLAY_API FGraphCodeExecutionState
{
public:
	/**
	 * Every directive line this component owes, already indented to `Depth`.
	 *
	 * Writes nothing for a component that is none of the three kinds. Writes `[none]` for one that
	 * IS and was not running (§20.7) — the empty case is explicit so that absence never has to be
	 * interpreted, which is what lets `omits:` carry the on/off answer on its own.
	 *
	 * **RETURNS THE TOP-LEVEL FIELD COUNT, which the caller adds to `found:`.** Not bookkeeping: the
	 * reader ENFORCES `found:` as a checksum and counts directive FIELDS, so a writer that added
	 * lines without counting them produced a capture refused by its own reader — measured, and it
	 * is why `ComponentDirectives` returns an `int32`.
	 */
	static int32 Directives(const UActorComponent* Component, int32 Depth,
		const FGraphCodeExecutionStateOptions& Options, TArray<FString>& OutLines);

	/**
	 * `@name[a, b, c]` — ONE LINE, always.
	 *
	 * ~~One entry per line when there are several, because that is the diff-friendly shape.~~
	 * **MEASURED WRONG AND REVERTED 2026-08-16, by the round-trip gate this feature shipped without.**
	 * A wrapped directive does not parse: `ParseDirective` requires `@name[…]` on one trimmed line,
	 * so `@blackboard[` alone was *unreadable line 75* and every key after it followed. That is not a
	 * cosmetic fault — `gcp_apply` parses before it restores, the rolling channel parses
	 * `current.gcp` on every wake, and `gc_play mode:"diff"` parses both ends, so **any world with an
	 * AI carrying two or more keys took the whole loop down**. One key parsed, which is exactly why
	 * four green gates never saw it.
	 *
	 * **The deeper mistake was inventing a second spelling for a shape the format already had one
	 * of** ([[one-concept-two-grammars]]) — §20.3 rejected `Key=Value` lines on that very argument
	 * and this sprung the same trap from the other side. `@placement` has carried three fields on one
	 * line since R1, and `FGraphCodeSnapshotReader::Render` emits the single-line form for every
	 * directive, so the wrapped shape could not survive a round trip even if the parser took it: the
	 * reader would have had to learn WHICH directives wrap, which is the writer's business.
	 *
	 * The diff argument it was serving is answered elsewhere anyway — `DiffDirectives` compares a
	 * payload WHOLE and refuses to split it, deliberately, because the payload's internal shape
	 * belongs to the writer.
	 */
	static FString Render(int32 Depth, const struct FGcpDirective& One);

	/**
	 * The same directives, as name and payload rather than as lines — added 2026-08-21 for the
	 * record.
	 *
	 * **A CONSUMER THAT COMPARES DIRECTIVES NEEDS THE VALUE, NOT THE LINE.** The record's `Act`
	 * states the change — `@blackboard[Alerted=false -> true]` — which means diffing two payloads,
	 * and a caller that recovered them by parsing `  @blackboard[…]` back apart would become a
	 * SECOND READER of this file's own output. Two readers of one subject drift, and the drift is
	 * invisible until somebody compares a record against a capture from the same run
	 * ([[a-read-answers-through-the-write-path]]). `Directives` is this plus the rendering, so there
	 * is still exactly one place a directive becomes text.
	 *
	 * Returns the same top-level field count, for the same `found:` checksum.
	 */
	static int32 Collect(const UActorComponent* Component,
		const FGraphCodeExecutionStateOptions& Options, TArray<struct FGcpDirective>& Out);

	/**
	 * The category tokens, for the envelope. Sorted, and the same strings `omits:` uses when a
	 * caller turns one off — one vocabulary, spelled once.
	 */
	static void Tokens(const FGraphCodeExecutionStateOptions& Options, bool bCarried,
		TArray<FString>& OutTokens);

	/**
	 * Put a captured `@blackboard` back — §20.12, [USER-RULED 2026-08-16].
	 *
	 * **ONE SUBJECT IS RESTORABLE AND EVERY OTHER ONE IS ACCOUNTED FOR**, which is not a limitation
	 * being worked around:
	 *
	 *   `@blackboard`     WRITTEN. What an AI knows decides what it does, so a branch that reset it
	 *                     would be a different fight — the counterfactual grid's whole point is one
	 *                     variable moved, not two.
	 *   `@behaviorTree`   READ-ONLY. A tree's active branch is a CONSEQUENCE of the blackboard plus
	 *                     the tree. Forcing a node puts the tree somewhere its own decorators
	 *                     disagree with; restore what it knows and it re-derives what it is doing.
	 *   `@stateTree`      READ-ONLY, for the same reason and one more: entering a state RUNS its
	 *                     tasks, so writing one would start work rather than put a value back.
	 *   `@tags`           NOT RESTORED, and NOT ruled — an owned tag may come from a loose add or
	 *                     from an active effect and the line does not say which. It shipped with
	 *                     R3a reported nowhere at all; naming it is the honest answer until
	 *                     somebody rules on it.
	 *   `@stateMachine`   NOT CAPTURED AT ALL since 2026-08-16 — ruled out of the file rather than
	 *                     out of the restore. A `@stateMachine` therefore only ever arrives in a
	 *                     capture taken before that day, and this still names it, because a file
	 *                     somebody kept is a file somebody can put back.
	 *
	 * The read-only cases are reported through `OutNotRestored` rather than skipped, because §6.5
	 * says a fidelity limit is a value in the reply and not a paragraph in a design document.
	 *
	 * Returns the number of keys written.
	 *
	 * **NO DRY-RUN PARAMETER, and that is `gcp_apply`'s own honesty rather than an omission.** Its
	 * card already states that property paths are not pre-validated, because the codec is the only
	 * authority on what a path means; a blackboard key is the same case one level further out, and a
	 * plan that claimed to have checked every value would be making a promise the real call is the
	 * first thing to test. `FGraphCodeSnapshotRestore::Apply` returns before this is ever reached on
	 * a dry run, so the flag would have had one value forever.
	 */
	static int32 Restore(UActorComponent* Component, const TArray<struct FGcpDirective>& Directives,
		const FString& Label, TArray<FString>& OutNotRestored, TArray<FString>& OutErrors);

	/**
	 * Is this `always:`/`omits:` entry one of the three CATEGORIES rather than a property name?
	 *
	 * Exists because the envelope is read back as well as written: `FGraphCodeSnapshotRestore`
	 * reconstructs the capture's admission set from `shows:`/`always:` so a restore resets exactly
	 * what the capture considered, and it would otherwise take `blackboard` for a property nothing
	 * has. One predicate, so the writer and the reader cannot learn different vocabularies —
	 * [[a-read-answers-through-the-write-path]].
	 */
	static bool IsCategoryToken(FName Token);

	/**
	 * Is this entry one the caller asked not to track? — the `Exclude` rule, in one place.
	 *
	 * **A PREFIX, CASE-SENSITIVELY, ON A NAME RATHER THAN ON THE WHOLE ENTRY.** Every subject this
	 * file reads names something and then says what it is: `Alerted=true`, `State.Attacking`,
	 * `/Game/AI/BT_Guard > Root > Guard > Patrol`. So the value is cut off first — excluding
	 * `Alerted` must not depend on what `Alerted` currently is — and what remains is matched by
	 * prefix, because the names that need excluding are HIERARCHIES: `State.` should take the whole
	 * family, and so should `Guard.Chase`.
	 *
	 * **AND A PATH ENTRY IS MATCHED SEGMENT BY SEGMENT.** A behaviour tree's line is the whole route
	 * from the asset to the active node, so a prefix rule alone could only ever exclude by asset
	 * path — which is the one thing a caller writing *do not tell me about Patrol* does not have to
	 * hand. Any node on the route matching excludes the line.
	 *
	 * Case-sensitive, like every other name comparison in this family: `FString::operator==` folds
	 * case in UE, so a rule that did not say so would quietly exclude `alerted` too.
	 */
	static bool IsExcluded(const FString& Entry, const TArray<FString>& Exclude);
};
