// What changed INSIDE one value — `PLAY-SESSION-DESIGN.md` §4.5c.
//
// The differ compares a property's value as one string and writes the whole new one when it differs.
// For `Health=800.0` that is the whole story; for `BodyInstance=(…)` — one line, ~4,000 characters,
// forty top-level fields and a nested response table — it means a single changed flag rewrites all
// four thousand, and a reader cannot see which flag.
//
// ---------------------------------------------------------------------------
// THE TWO RULES, AND THEY DIFFER BECAUSE IDENTITY DOES
// ---------------------------------------------------------------------------
//
// **[USER, 2026-08-17]** *"for something like array, it should show something like (a=1->2, b=0,
// c=1000), so no -> if no changes … for key value pairs, don't even show the key value pairs"*, and
// the follow-up that settled which was which: *"I meant to show array, not key value pairs."*
//
//   A NAMED FIELD carries its own identity, so an unchanged one is dropped. `BodyInstance` reduces
//   to `(CollisionEnabled=QueryAndPhysics -> NoCollision)` and the other thirty-nine fields are
//   simply absent — the reader knows what is missing because every name is still spelled where it
//   matters. *"why sleep family=normal? did I change it? if not then why show"* [USER].
//
//   AN ARRAY ELEMENT is identified by its POSITION, so every element is kept. Dropping element 2
//   would silently promote element 3 into its place, and a reader comparing two such lists would be
//   comparing different things while everything looked fine. Unchanged elements render as
//   themselves; only the changed ones carry a marker.
//
// This is the format's own derivability rule applied one level down: state what a reader cannot
// work out, and drop what they can.
//
// ---------------------------------------------------------------------------
// TWO SPELLINGS, TWO PRODUCERS, AND DELIBERATELY NO PARAMETER
// ---------------------------------------------------------------------------
//
// `Arrow` is what `gc_play mode:"diff"` returns — a caller asked once and can afford both numbers,
// and *what was it* is exactly what today's delta throws away. `Signed` is what the rolling channel
// writes into every `ping_NN.gcp` — that file is produced on every wake and streamed into a context
// window, so it carries the movement for numbers and the landing value for everything else.
//
// **Nobody chooses between them**, which is why neither is a mode and neither is a parameter. They
// belong to two different producers with two different audiences; an earlier draft offered
// `values:"relative"` and that was one knob standing in for a distinction the callers already make.
//
// ---------------------------------------------------------------------------
// WHAT THIS IS NOT
// ---------------------------------------------------------------------------
//
// **Not a second opinion about the value grammar.** It never asks what a value MEANS — it does not
// know a struct from a vector and does not consult reflection. It splits on the punctuation the
// writer emits, exactly as `CountTopLevelFields` already does to enforce `found:`, and it hands back
// text. A value it cannot split is a scalar, which is the safe degradation: worst case it answers
// what the differ answers today.
//
// **Not applyable, and neither was what it replaces.** `Health=-400.0` is not a state — but neither
// is a document whose absent lines mean *unchanged*, and §18 already forbids a delta as input to any
// write. This makes an existing fact legible rather than introducing a new hazard.

#pragma once

#include "CoreMinimal.h"

/** How a changed value is spelled. One per producer; see the header. */
enum class EGcpValueDeltaStyle : uint8
{
	/** `1200.0 -> 800.0`. Both values, difference derivable. What the command returns. */
	Arrow,

	/**
	 * `-400.0` when both sides are numbers, the landing value otherwise.
	 *
	 * The asymmetry is not a compromise: a bool, an enum, a name and an object path have no
	 * arithmetic, so there is nothing to sign and the final value is the whole of what can be said.
	 */
	Signed,

	/**
	 * `+State.Attacking, -State.Guarding` — MEMBERSHIP, for a payload that is a SET.
	 *
	 * **A SET HAS NO KEYS AND NO POSITIONS, so neither of the two spellings above can state its
	 * change.** `Arrow` would print both whole sets and leave the reader to spot the one tag that
	 * moved; `Signed` has nothing to subtract. What changed about a set is exactly which members
	 * joined and which left, and that is what this writes.
	 *
	 * It is therefore NOT a third way of saying the same thing — it is the first way of saying a
	 * different thing, which is why it is a style rather than a flag on the caller
	 * ([[gc-delta-value-spelling]]: arrow versus signed is two PRODUCERS, not a parameter).
	 *
	 * Order is the sorted union rather than the order either side happened to arrive in, so two
	 * runs that changed the same tags produce the same line and a diff of two records is about the
	 * game rather than about the iteration order of a container.
	 */
	Set,
};

class FGraphCodeValueDelta
{
public:
	/**
	 * What changed between two rendered values, in the given spelling.
	 *
	 * Returns an EMPTY string when the two are equal — the caller drops the line entirely, which is
	 * how an unchanged named field disappears. Comparison is case-SENSITIVE throughout, for the
	 * reason the differ's own `SameText` is: `FString::operator==` folds case in UE, so `True` and
	 * `true` would compare equal and a real change would vanish from the channel built to report it.
	 */
	GRAPHCODEPLAY_API static FString Render(const FString& Before, const FString& Now,
		EGcpValueDeltaStyle Style);

	/**
	 * What changed inside a DIRECTIVE payload — `@blackboard[Alerted=false -> true]`.
	 *
	 * **A PAYLOAD IS A KEYED LIST WITHOUT ITS PARENTHESES**, so it is wrapped before the codec sees
	 * it and unwrapped after. Handing one over bare makes the codec read it as a scalar and answer
	 * with both whole payloads, which is exactly the behaviour a delta replaces — and it looks like
	 * it works. An answer that comes back unparenthesised came from the scalar path (two payloads of
	 * different shapes) and passes through whole.
	 *
	 * **HERE RATHER THAN IN EITHER CALLER, because there are two of them now**: the `.gcp` delta's
	 * `DiffDirectives` and the record's `Act` line. One wrap rule, so the two cannot learn different
	 * ones ([[a-read-answers-through-the-write-path]]). Empty means equal, and the caller drops the
	 * line.
	 */
	GRAPHCODEPLAY_API static FString RenderDirective(const FString& Before, const FString& Now,
		EGcpValueDeltaStyle Style);

	/**
	 * Top-level comma-separated segments of a payload, quotes and nesting respected.
	 *
	 * The splitting half of `CountTopLevelFields`, which counts these to enforce `found:`. Public
	 * because a directive payload is split by the same rule and the two must not learn different
	 * ones — `@placement[Location=(…), Rotation=(…)]` is this grammar without the brackets.
	 */
	GRAPHCODEPLAY_API static void SplitTopLevel(const FString& Payload, TArray<FString>& OutParts);
};
