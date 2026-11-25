// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeEngineCompat.h"

#include "Math/RandomStream.h"

struct FGcShapeDocument;

/**
 * A NUMBER THAT MAY VARY. `GC-SHAPE-GRAMMAR.md` §3; `GC-SHAPE-AUTHORING-DESIGN.md` §15 M2.
 *
 * **A PLAIN NUMBER IS A PROFILE WITH ONE ENTRY**, which is the whole reason this type exists rather
 * than a branch at every argument: `radius=52.0` and `radius=profile(0.0 = 52.0, 1.0 = 42.6)` are the
 * same argument at two lengths, so every numeric argument takes one of these and never asks which
 * form it was written in.
 *
 * **M2 IS NOT DEFERRABLE FOR EXACTLY THAT REASON.** Retrofitting a varying number after the
 * instructions ship touches every one of them.
 *
 * **WHAT `t` MEANS IS THE ARGUMENT'S, NOT THE PROFILE'S** (§3). `extrude(height=)` runs ACROSS the
 * outline and `extrude(to=)` runs ALONG the travel, and both appear on the same line — so this type
 * carries a curve and nothing about where it is read.
 */
struct GRAPHCODESHAPE_API FGcShapeProfile
{
	struct FStop
	{
		double T = 0.0;
		double Value = 0.0;

		/** Written as `82%`. What it is a fraction OF is the argument's business. */
		bool bPercent = false;

		/**
		 * **`t = v@sharp` — A CORNER** (`GC-SHAPE-CREASE-WARP-DESIGN.md` D2, §6.1; slice P1, 2026-11-15).
		 * Each side heads straight at its neighbour: the fill is handed only the stops between the
		 * nearest corners, so this stop is an END of each run and takes its one secant. On a first or
		 * last stop, or on a `[linear]` profile, the fill does not change: only the crease is declared.
		 */
		bool bSharp = false;

		/**
		 * **`t = v@(dx, dy)` — THE SLOPE dy/dx AT THIS STOP** (D3), in the profile's own value unit per
		 * unit of t, already ×0.01 on a percentage profile. One slope per SIDE, so a corner with its own
		 * slope on each side (slice PK, §16) needs no new field: a plain heading sets both sides to one
		 * slope. `bSlopeIn` shapes the segment that ENDS here, `bSlopeOut` the one that STARTS here.
		 */
		bool bSlopeIn = false;
		bool bSlopeOut = false;
		double SlopeIn = 0.0;
		double SlopeOut = 0.0;

		/** Any mark at all: `@sharp` or a heading on either side. */
		bool IsMarked() const { return bSharp || bSlopeIn || bSlopeOut; }
	};

	/** Sorted by `t`. Never empty in a profile that parsed. */
	TArray<FStop> Stops;

	/** `profile(…) [linear]` — straight segments instead of the smooth default. */
	bool bLinear = false;

	bool IsConstant() const { return Stops.Num() <= 1; }

	/**
	 * **ANY STOP CARRIES A MARK** (`@sharp` or a heading; P1). What a reader that cannot carry one asks
	 * before it refuses (a loft's `rails=`) or leaves a fast path that assumes the plain fill (the
	 * projected-texture refinement). False for every profile written before P1: `@` after a stop's
	 * value was refused until then.
	 */
	bool HasMarks() const;

	/**
	 * **ANY MARK THAT CHANGES THE FILL** (P1): a heading, or `@sharp` on an INTERIOR stop of a smooth
	 * profile. False for an unmarked profile, a `[linear]` one, and one whose only marks are `@sharp`
	 * on its ends (D2: those declare a crease and leave every value as it was). Where this is false,
	 * `Evaluate` makes exactly the single `AuthoringCurve::Evaluate` call it always made.
	 */
	bool ChangesFill() const;

	/** Any stop with a heading on either side. */
	bool HasHeadings() const;

	/** True when any stop was written as a percentage. Mixed forms are refused at parse. */
	bool IsPercent() const { return !Stops.IsEmpty() && Stops[0].bPercent; }

	/**
	 * The value at `t`, clamped to 0..1.
	 *
	 * **SMOOTH MEANS MONOTONE CUBIC (PCHIP), AND THE CHOICE IS NOT COSMETIC.** §3 says *smooth
	 * interpolation by default* and does not say which; a Catmull-Rom through the same stops
	 * OVERSHOOTS between them, and every profile in the grammar scales something physical — a
	 * radius, a height, a silhouette. An overshoot there is a radius that dips below zero and a shape
	 * that turns inside out between two stops an author wrote as a gentle curve. A monotone cubic
	 * never leaves the interval its neighbouring stops define, so entasis still swells 3% where it is
	 * asked to and nothing swells where it is not.
	 *
	 * **MARKS (P1) ARE A SHAPE-ONLY WRAPPER ROUND THAT ONE CALL** (`GC-SHAPE-CREASE-WARP-DESIGN.md`
	 * §6.1). With no mark that changes the fill (`ChangesFill`), it is the call it always was. A
	 * segment beside an interior `@sharp` is filled by `AuthoringCurve` over only the stops between
	 * the nearest corners; a segment beside a heading is a Hermite cubic with the written slope and,
	 * at its far stop, `AuthoringCurve`'s own tangent rule (copied here, held equal by a spec).
	 * Every other segment is untouched, bit for bit. A heading may overshoot its stops: the author
	 * has overridden the monotone guarantee, and `MinValue`/`MaxValue` say by how much.
	 */
	double Evaluate(double T) const;

	/**
	 * The largest value the curve reaches, never below zero. What a mesh has to be built big enough
	 * for. Without a heading that is the largest stop (see `MinValue`); beside a heading it also
	 * counts the segment's true peak, from the roots of its derivative.
	 */
	double MaxValue() const;

	/**
	 * The smallest value the curve reaches — what a refusal on the SIGN has to read.
	 *
	 * A monotone cubic never leaves the interval its neighbouring stops define, so the extremes of
	 * the curve are the extremes of the stops and this is exact rather than a sample. A corner's
	 * runs are monotone cubics too. **A heading is not** (P1): the segments beside it add their true
	 * extremes, found from the roots of the derivative (a quadratic), so every caller sees the
	 * overshoot.
	 */
	double MinValue() const;

	/**
	 * The stations a varying profile has to be sampled at to stay inside `Tolerance`.
	 *
	 * Always includes 0 and 1 and every declared stop. Between them it subdivides while the straight
	 * line between two samples misses the curve by more than the tolerance — the same tolerance
	 * `smooth:` spends on a circle, for the same reason: how finely a shape is cut is a property of
	 * how far it may be from the shape that was asked for, never of a count somebody typed.
	 *
	 * `Scale` converts a profile value into centimetres of error, because a profile that scales a
	 * 200 cm radius misses by more than one that scales a 5 cm radius for the same deviation.
	 */
	void Stations(double Tolerance, double Scale, TArray<double>& OutStations) const;

	/** A profile with one stop, which is what a plain number is. */
	static FGcShapeProfile Constant(double Value, bool bPercent = false);

	/**
	 * Read a value as written: `52.0`, `82%`, `profile(…)` or `$Name`.
	 *
	 * `$Name` reaches a NUMBER or a PROFILE entry and nothing else — a name that means a shape is
	 * M3's, where entries can reach each other and a cycle becomes possible. Refusing it here by name
	 * is what keeps M2 from being half of a DAG.
	 *
	 * **THIS RECURSES ON A NUMBER ENTRY AND CANNOT LOOP, AND MC4 §2.6 WIDENED WHAT ONE MAY HOLD
	 * WITHOUT CHANGING THAT.** A declaration may now compute — `Wide = $Tall * 2` — but it is
	 * SETTLED WHERE IT IS WRITTEN, reading only what is already above it, and stored back as a plain
	 * number. So every `$name` hop here still lands on a literal in ONE step, and the earlier note's
	 * warning (*anything that widens what a number entry may hold makes a cycle reachable through
	 * this function*) was answered by moving the evaluation rather than by adding a guard: a
	 * declaration cannot name something not yet declared, so `A = $B * 2` beside `B = $A * 2` cannot
	 * be written. **What still would widen it is a declaration resolved LAZILY** — that is the
	 * change this note is now warning about.
	 */
	static bool Parse(const FGcShapeDocument& Document, const FString& Text, FGcShapeProfile& Out,
		FString& OutError);
};

/**
 * THREE ANGLES PER STOP — `angular profile(0 0 0, 0 20 0, 0 40 20)`. Proposed by the user
 * 2026-09-14 (`GC-SHAPE-ANGULAR-PROFILES-DESIGN.md`); what the numbers mean was ruled the same day.
 *
 * **x AND y TILT THE DIRECTION A GROWTH RUNS IN, z TWISTS ITS SECTION ABOUT ITS OWN LENGTH** —
 * user-ruled, *tilt, then twist*, so the three effects are never conflated. The two tilts are one
 * rotation about the axis `(x, y, 0)`, so neither of them goes first.
 *
 * **A STOP IS AN ORIENTATION TO REACH**, measured from the base's own frame rather than added per
 * sample, so cutting more rings can never add rotation. Stops carry no positions and are spread
 * evenly. Each channel is an ordinary `FGcShapeProfile`, so the fill between stops is the one every
 * profile uses and `[linear]` means what it means there. Degrees are plain numbers: 0 to 720 is two
 * turns.
 */
struct GRAPHCODESHAPE_API FGcShapeAngularProfile
{
	FGcShapeProfile X;
	FGcShapeProfile Y;
	FGcShapeProfile Z;

	bool IsConstant() const { return X.IsConstant(); }

	/** The three angles at `t`, in degrees. */
	UE::Math::TVector<double> Evaluate(double T) const;

	/** No turn at all — what a growth with no `angle=` runs along. */
	static FGcShapeAngularProfile Straight();

	/** Read `angular profile(…)`, with its `[linear]`, or a `$Name` that is one. */
	static bool Parse(const FGcShapeDocument& Document, const FString& Text,
		FGcShapeAngularProfile& Out, FString& OutError);
};

/**
 * **A NUMBER THAT WAS DRAWN — `random(min = 10.0, max = 20.0)`.** MC4 §2.5;
 * `Docs/GC-SHAPE-MC-DONE.md`.
 *
 * **THE SEED'S PRESENCE IS THE WHOLE TYPE, and it splits the value into two kinds.**
 *
 * | written | seed | what it is |
 * |---|---|---|
 * | `random(seed = 30, min = 10.0, max = 20.0)` | its own | **ABSOLUTE** — a pure function of the text, so the same number everywhere the value is used |
 * | `random(min = 10.0, max = 20.0)` | the call's | **COMPOSING** — drawn against the `seed=` of the `add` it rides on, so two walls sharing one declared value differ |
 *
 * **BOTH HAVE A CASE IN ONE OBJECT AND THAT IS WHY THERE ARE TWO** (§2.5): two brick walls off one
 * `$BrickW` must look different, and two headlights must be IDENTICAL rather than independently
 * rolled. Without the absolute form nothing in the format can say *these two must match*, which a
 * symmetric object needs constantly.
 *
 * **AN ABSOLUTE DRAW IS A CONSTANT AND IS RESOLVED WHEREVER A NUMBER IS READ**;
 * a COMPOSING one has no reading away from a call, because there is nothing to compose with, and
 * `FGcShapeProfile::Parse` refuses it there by name rather than inventing a seed.
 *
 * **THE BOUNDS ARE LITERALS OR ONE `$name` HOP TO A NUMBER ENTRY, AND THAT RESTRICTION IS A CYCLE
 * GUARD RATHER THAN TIDINESS.** `FGcShapeProfile::Parse`'s own note says it: *anything that widens
 * what a number entry may hold makes a cycle reachable through this function*. A `random` entry
 * widens exactly that, so `Wear = random(min = $Wear, max = 20.0)` would recurse for ever —
 * refusing a non-number source for a bound is what keeps the hop count at one, the same argument
 * the parser already makes for `Wide = $Tall`.
 */
struct GRAPHCODESHAPE_API FGcShapeRandom
{
	double Min = 0.0;
	double Max = 0.0;

	/** `seed=` as written. Meaningless unless `bHasSeed`. */
	int32 Seed = 0;
	bool bHasSeed = false;

	/**
	 * One number in `[Min, Max]` from an ABSOLUTE draw — its own seed is the whole of its state.
	 *
	 * A pure function of the text, which is why it is the same number everywhere the value is used
	 * and why `FGcShapeProfile::Parse` can resolve it with nothing else in hand.
	 */
	double Draw() const;

	/**
	 * One number from a COMPOSING draw, taken off a stream the CALLER owns and advances.
	 *
	 * **THE STREAM IS THE ARRAY'S, RUN SEQUENTIALLY, AND THAT IS RULED RATHER THAN CONVENIENT**
	 * (MC4 §2.5, user-ruled 2026-08-29). Keyed per copy index instead, `repeat=(20,12)` → `(21,12)`
	 * inserts a whole COLUMN, so index 21 stops meaning what it meant and the mapping shifts however
	 * the stream is keyed — the stability a keyed stream buys does not exist in two dimensions.
	 * Determinism survives sequential: same file, same seed, same wall, every run.
	 *
	 * An absolute draw never comes through here — its own seed would be ignored, which is the one
	 * thing it exists to prevent.
	 */
	double DrawFrom(FRandomStream& Stream) const;

	/**
	 * Read `random(…)` — written inline, or reached through one `$name` hop to a `random` entry.
	 *
	 * `bOutIsRandom` false with a true return means *this text is simply not a random*, which is
	 * the answer every other value form needs and not an error. False means it IS one and is
	 * malformed.
	 */
	static bool Read(const FGcShapeDocument& Document, const FString& Text, bool& bOutIsRandom,
		FGcShapeRandom& Out, FString& OutError);
};

struct FGcShapeInstruction;

/**
 * READING ONE ARGUMENT OFF ONE INSTRUCTION, and there is one copy of each reading.
 *
 * **THESE MOVED OUT OF THE EVALUATOR AT M4 AND THE MOVE IS THE POINT.** Group 1 grew four
 * instructions and group 2 two, all in a second file, and every one of them takes a length, a count
 * or a flag. Two copies of *what a percentage means where a length is asked* is two answers, and the
 * one that drifts is the one nobody is looking at ([[two-carriers-of-one-fact-is-a-race]]).
 */
namespace GraphCodeShapeValue
{
	/**
	 * `$wear > 0.7` — the inside of an `if (…)`, answered true or false.
	 *
	 * **TWO NUMBERS AND ONE COMPARISON, AND BOTH SIDES ARE FULL NUMERIC VALUES.** Each side goes
	 * through `FGcShapeProfile::Parse`, so a side is a literal, a `$name`, an alias, arithmetic or
	 * a seeded draw — the same reader every argument in the format uses. Nothing else is a
	 * condition: there is no `and`, no `or` and no negation, because a shape file that needs them
	 * is a shape file with a decision in it that belongs to whoever wrote the numbers.
	 *
	 * **`==` AND `!=` COMPARE WITH A TOLERANCE AND THAT IS DELIBERATE.** Every number here is a
	 * double that may have been arrived at by arithmetic, so exact equality would be a coin toss
	 * on the last bit; refusing the operators outright would cost the flag-like case
	 * (`if ($cracked == 1.0)`) that authors reach for first.
	 */
	GRAPHCODESHAPE_API bool EvaluateCondition(const FGcShapeDocument& Document,
		const FString& Text, bool& bOut, FString& OutError);

	/**
	 * **WHAT A `$name` IS BOUND TO, MATCHED THE WAY THE GRAMMAR MATCHES NAMES — EXACTLY.**
	 *
	 * **A PAIR LIST RATHER THAN A `TMap<FString, …>`, AND THE MAP IS WHAT MADE IT NECESSARY.** An
	 * `FString` key compares with `operator==`, which is `Stricmp`, so a map filed `Rise` answered
	 * `Find("RISE")` with it. Every other reading of a name in this format is case-SENSITIVE and
	 * says so in code — `FGcShapeDocument::Resolve`, `FGcShapeEntry::FindParameter`, the
	 * declared-twice refusal and the parameter-shadows-an-entry refusal all pass
	 * `ESearchCase::CaseSensitive` — so the refusals that exist to keep bound names and document
	 * names disjoint were asking a finer question than the container could hold, and substitution
	 * quietly answered one name with another's value.
	 *
	 * **IT IS A STORE BUG AS WELL AS A LOOKUP BUG, WHICH IS WHY THE CONTAINER CHANGES RATHER THAN
	 * THE COMPARISON.** `mesh(wide = 40.0, WIDE = 90.0)` is two parameters by the duplicate-
	 * parameter refusal's own reckoning, and a map held ONE of them — the first spelling carrying
	 * the last value — with nothing printed. A case-sensitive `Find` over an aliasing map would
	 * have left that half standing.
	 *
	 * **ORDER IS THE DECLARED ORDER, WHICH THIS OWED ANYWAY.** `GcShapeBindingsFor` draws seedless
	 * `random()` defaults in sequence, so the order parameters are visited in IS which number each
	 * one gets; a pair list makes the order the code already depended on the order it stores
	 * ([[equal-ranks-sort-arbitrarily]]). Lookup is linear, and that costs nothing worth measuring:
	 * an entry's parameters and a file's counters are units and tens, against mesh work.
	 *
	 * **THE SAME SHAPE AS GC GROOM'S `FFeatureParameterBindings`** (`916f5f3e`, 2026-09-23), which
	 * is the same defect in the product next door and fixed the same way three hours earlier —
	 * [[find-the-precedent-before-inventing]].
	 */
	using FNameBindings = TArray<TPair<FString, FString>>;

	/**
	 * **ARE THESE ONE NAME? — THE ONE ANSWER EVERY STORE HOLDING A `.gcshape` NAME GIVES.** Exact:
	 * `Rise` is not `RISE`, because `FGcShapeDocument::Resolve`, `FindParameter` and the declared-
	 * twice refusal all say so, and a store that disagreed would be answering a different question
	 * from the one the parser asked.
	 *
	 * **ONE FUNCTION RATHER THAN A FLAG AT EVERY SITE**, so the pair list above and the keyed
	 * containers below cannot drift apart ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	UE_FORCEINLINE_HINT bool SameName(const FString& A, const FString& B)
	{
		return A.Equals(B, ESearchCase::CaseSensitive);
	}

	/**
	 * Key funcs for a `TMap` keyed by a `.gcshape` name. A plain `TMap<FString, V>` compares with
	 * `Stricmp`, so `$Cap` and `$cap` would be one entry — the defect the pair list above was built
	 * to remove, and one a map with ~40 call sites should not be rewritten into a list to escape.
	 *
	 * **THE HASH STILL FOLDS CASE, AND THAT IS CORRECT RATHER THAN AN OVERSIGHT.** `Rise` and `RISE`
	 * share a bucket and `Matches` separates them — an ordinary collision. The pairing that would
	 * LOSE lookups is the reverse, a case-sensitive hash under a folding compare, and keeping the
	 * stock `GetTypeHash` rules it out.
	 *
	 * **DECLARING `Matches` HIDES BOTH OF THE BASE'S**, the templated heterogeneous one included, so
	 * no lookup can fall back to `==` — it converts to `FString` or it does not compile.
	 */
	template <typename ValueType>
	struct TExactNameKeyFuncs : TDefaultMapKeyFuncs<FString, ValueType, false>
	{
		[[nodiscard]] static UE_FORCEINLINE_HINT bool Matches(const FString& A, const FString& B)
		{
			return SameName(A, B);
		}
	};

	/** The same, for a `TSet` of names. */
	struct FExactNameSetKeyFuncs : DefaultKeyFuncs<FString>
	{
		[[nodiscard]] static UE_FORCEINLINE_HINT bool Matches(const FString& A, const FString& B)
		{
			return SameName(A, B);
		}
	};

	/** A map from a `.gcshape` name, which keeps `$Cap` and `$cap` apart. */
	template <typename ValueType>
	using TNameMap = TMap<FString, ValueType, FDefaultSetAllocator, TExactNameKeyFuncs<ValueType>>;

	/** A set of `.gcshape` names, which keeps `Step` and `STEP` apart. */
	using FNameSet = TSet<FString, FExactNameSetKeyFuncs>;

	/** What this name is bound to, or null. Exact match — `Rise` is not `RISE`. */
	GRAPHCODESHAPE_API const FString* FindBinding(const FNameBindings& Bindings,
		const FString& Name);
	GRAPHCODESHAPE_API FString* FindBinding(FNameBindings& Bindings, const FString& Name);

	/** Is this name bound at all? The same exact match, asked rather than read. */
	GRAPHCODESHAPE_API bool HasBinding(const FNameBindings& Bindings, const FString& Name);

	/**
	 * Bind this name, replacing what it held. **CREATE-OR-SET RATHER THAN APPEND**, because a
	 * counter is written once per `set` and a parameter once per default and once per argument —
	 * an append would grow a list whose later copies the linear `FindBinding` would never reach.
	 * A new name lands at the end, so declared order survives.
	 */
	GRAPHCODESHAPE_API FString& SetBinding(FNameBindings& Bindings, const FString& Name,
		FString Value);

	/**
	 * Replace every `$name` these bindings carry and leave every other `$` exactly as written.
	 *
	 * **TOKEN-AWARE RATHER THAN A STRING REPLACE**, which is the whole correctness argument: the full
	 * name after the `$` is read and matched entire, so binding `w` does not touch `$width`. A plain
	 * `Replace(TEXT("$w"), …)` would corrupt the neighbouring name silently.
	 *
	 * **AND WHAT IT DOES NOT FIND, IT LEAVES.** An unbound name resolves the way it always did.
	 *
	 * **TWO CALLERS AND THAT IS WHY IT IS HERE.** MC4 §2b substitutes a CALL'S ARGUMENTS into an
	 * entry before evaluation; the parser points a REBOUND name at its current target. Same act,
	 * same correctness argument, and two copies of it would be two answers
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	GRAPHCODESHAPE_API FString SubstituteNames(const FString& In, const FNameBindings& Bindings);

	/**
	 * The same walk, over a store that is not a plain map — a body's values carry a KIND beside the
	 * text (§3.6), and the token walk is the part neither of them may have its own copy of.
	 *
	 * `Lookup` answers with the replacement text, or null to leave the name as written.
	 */
	GRAPHCODESHAPE_API FString SubstituteNames(const FString& In,
		TFunctionRef<const FString*(const FString&)> Lookup);

	/**
	 * Does this text name any of these? **The same token walk `SubstituteNames` does, asking rather
	 * than replacing** — so `$width` is not a mention of `w`, which a `Contains` would say it was.
	 *
	 * Its caller decides whether a duplicating line stamps one mesh or builds every copy, and a
	 * wrong answer there is silent either way: too eager costs builds nobody asked for, too shy
	 * hands every copy the first copy's number.
	 */
	GRAPHCODESHAPE_API bool MentionsAny(const FString& In, const FNameSet& Names);

	/** The most names one `warp(with=…)` holds: every point carries one slot per name. */
	constexpr int32 MostWarpWithNames = 16;

	/**
	 * **`warp(with=(Id: <expr>, Env: <expr>), …)`'S NAMED INTERMEDIATES, READ — agent-spelled 2026-11-18, approved
	 * after a Sonnet review (`Docs/GC-SHAPE-WOOD-DAMAGE.md`, "Grammar proposals").** A formula that needs one per-point
	 * value in several places (a strand's id from `point.x`, an envelope) wrote it out each time; the debris author
	 * generated a preset with a script for it. A named-key tuple, in the order written, each entry an expression of
	 * the point that later entries and the warp's own `to=` / `by=` read as `$Name`. One reader for the parser (the
	 * scope refusals) and the warp (the values), so the two cannot disagree on what the list says.
	 *
	 * Refused here: not one parenthesised list, an entry with no `:` or no expression, a name that is not a bare word
	 * (`$Id:` names nothing new — write `Id:` and read `$Id`), a name written twice, more than `MostWarpWithNames`, and
	 * an entry that reads its own name or a later one (an entry reads only the ones before it). `OutNames` are bare.
	 */
	GRAPHCODESHAPE_API bool ReadWarpWith(const FString& Value, TArray<FString>& OutNames,
		TArray<FString>& OutExpressions, FString& OutError);

	/**
	 * Is this text ARITHMETIC rather than one of the value forms that were always here? MC4 §2.6.
	 *
	 * **PUBLIC FOR ONE CALLER AND THE ALTERNATIVE WAS A SECOND COPY.** A position reader decides for
	 * itself whether a value is worth handing to `FGcShapeProfile::Parse` — it lets a `$name`
	 * through and stops at anything else — and a private answer to *is this an expression* would
	 * mean two of them, with the one nobody is looking at drifting
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * Every existing form answers false: a plain number, a negative literal, a percentage, a
	 * `profile(…)` and a `random(…)`.
	 */
	GRAPHCODESHAPE_API bool IsExpression(const FString& Text);

	/**
	 * `$Roof(t)` — the curve `Roof` read at `t`, which runs 0 to 1 (2026-10-30, the car's warp).
	 *
	 * **A CURVE IS HOW A DESIGNER STATES A LINE, AND THIS IS WHAT LETS ARITHMETIC USE ONE.** A roofline,
	 * a plan width or a beltline is a `profile(…)` of a few numbers; `warp(to=…)` reads it once per
	 * point at `point.t.x`. It reads at any value site too: `at=(0, 0, $Roof(.5))`. A `t` outside 0..1
	 * is refused rather than clamped, because the usual cause is centimetres where `t` was meant, and a
	 * curve written in percentages is refused because a percentage has nothing here to be a fraction of.
	 */
	GRAPHCODESHAPE_API bool CurveAt(const FGcShapeDocument& Document, const FString& Name, double T,
		double& Out, FString& OutError);

	/**
	 * **G28g — `random(seed=, min=, max=)` INSIDE ARITHMETIC** (2026-11-19, owner-approved). Rewrites every draw in
	 * `In` that is not the whole value into `((min) + $__span((max) - (min)) * $__draw(seed))`, the very operations
	 * `FGcShapeRandom::Draw` does, so `20 + 10 * random(seed=7, min=0, max=1)` reads the same number a set of the draw
	 * would. `$__draw(s)` is the absolute draw's fraction for seed `s` (G17's mix), and `$__span(d)` refuses ends the
	 * wrong way round; `CurveAt` answers both, so a warp's per-point seed draws per point. A draw needs `seed=` here
	 * (no line lends one), and a draw's own seed or end may not be a draw. Text with no `random(` is left alone.
	 */
	GRAPHCODESHAPE_API bool RewriteDraws(const FString& In, FString& Out, FString& OutError);

	/** G28g: is `Text` exactly one `random(…)` (the old whole-value form), rather than arithmetic holding one? */
	GRAPHCODESHAPE_API bool IsWholeDraw(const FString& Text);

	/**
	 * **SHAPE'S COPY OF THE FILL, MARKS IGNORED — FOR ITS SPEC ONLY** (P1; crease ledger R3).
	 *
	 * A heading's segment is a Hermite cubic whose far stop takes `AuthoringCurve`'s tangent rule,
	 * and that rule is COPIED into Shape rather than exposed, so AuthoringCore (shared with `.gcmove`)
	 * is not edited. This runs the whole open fill on that copy, exactly the code a heading's segment
	 * uses, so a spec can hold it bit-equal to `AuthoringCurve::Evaluate` on unmarked profiles: any
	 * change to the original fails the spec until the copy follows. Nothing in a build calls it.
	 */
	GRAPHCODESHAPE_API double EvaluateWithCopiedFillForTests(const FGcShapeProfile& Profile, double T);

	/**
	 * Does this text read one of `warp()`'s per-point words — `point.x`, `point.normal.z`, `point.t.y`?
	 * Bare, as `output.` is: a language word, never somebody's `$name`. Only a warp gives them a value,
	 * so every other line that holds one is refused by name.
	 */
	GRAPHCODESHAPE_API bool MentionsPointWord(const FString& Text);

	/** Every numeric argument takes a profile, so every numeric argument is read as one. */
	GRAPHCODESHAPE_API bool ReadProfile(const FGcShapeDocument& Document, const FString& Verb,
		const FString& Argument, const FString& Value, FGcShapeProfile& Out, FString& OutError);

	/**
	 * A plain length in centimetres — a profile with one stop, and a percentage refused.
	 *
	 * **A PERCENTAGE IS A FRACTION OF WHATEVER THE ARGUMENT IS MEASURED AGAINST** (§3), and a radius
	 * is measured against nothing: `rect(w=50%)` has no reading. M1 accepted it and quietly meant
	 * half a centimetre, which is the silent wrong answer this refusal replaces.
	 *
	 * **A VARYING profile is refused where §3's table gives the argument no domain — USER-RULED
	 * 2026-08-23: *"circle can't have varying radius."*** The rule is not *outline arguments never
	 * vary*: it is *`t` has to run ALONG something*, and a radius is measured along nothing. A path
	 * has a length, which is why `widen(by=)` takes a profile on the very next line.
	 */
	GRAPHCODESHAPE_API bool ReadLength(const FGcShapeDocument& Document, const FString& Verb,
		const FString& Argument, const FString& Value, double& Out, FString& OutError);

	/** An angle in degrees. A profile is refused for the same reason a radius refuses one. */
	GRAPHCODESHAPE_API bool ReadAngle(const FGcShapeDocument& Document, const FString& Verb,
		const FString& Argument, const FString& Value, double& Out, FString& OutError);

	/**
	 * A COUNT — a whole number, and **a profile is refused by name** (§7).
	 *
	 * Asked of the RESOLVED value rather than of the text, so `segments=$Steps` where
	 * `Steps = profile(…)` is refused too: a refusal that only reads the spelling is one a `$name`
	 * walks straight past.
	 */
	GRAPHCODESHAPE_API bool ReadCount(const FGcShapeDocument& Document, const FString& Verb,
		const FString& Argument, const FString& Value, int32 Least, int32& Out, FString& OutError);

	/** `true` or `false`, and nothing else is either. */
	GRAPHCODESHAPE_API bool ReadBool(const FString& Verb, const FString& Argument,
		const FString& Value, bool& Out, FString& OutError);

	/**
	 * A DIRECTION named by any argument — one of the six signed axes, or a vector, normalised.
	 *
	 * **THE ONE READER, AND IT EXISTS BECAUSE THERE WERE TWO ANSWERS.** `along=` took an axis OR a
	 * vector from M6.3 onward and `cut(plane=)` took an axis ALONE, so a fracture plane at any
	 * angle — the defining feature of broken stone, and every flat face on a real rock — had no
	 * spelling, while the waterfall's `extrude(along=(0.22, 0, -0.98))` two groups away did. **The
	 * restriction protected nothing**: `cut`'s `at=65%` is already measured along the plane's own
	 * normal (`GcShapeExtentAlong`), and `FMeshPlaneCut` takes an arbitrary one. Measured 2026-09-02,
	 * board row MR3.
	 *
	 * `ReadAlong` is this function with `along=`'s name and its `Z` default, so the two cannot drift
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 * With a Document, vector components may be constant scalar expressions in that scope.
	 * Without one this remains the literal-only reader used by existing consumers.
	 */
	GRAPHCODESHAPE_API bool ReadDirection(const FString& Verb, const FString& Argument,
		const FString& Value, UE::Math::TVector<double>& Out, FString& OutError,
		const FGcShapeDocument* Document = nullptr);

	/**
	 * One length in centimetres, or THREE — a per-axis feature size, `(200, 200, 25)`.
	 *
	 * **A SINGLE `scale=` CAN ONLY DESCRIBE ISOTROPIC NOISE, AND NO ROCK IS ISOTROPIC.** Bedding is
	 * noise stretched flat: large across X and Y, small up Z. Board row MR8, measured against two
	 * photogrammetry references 2026-09-02 — a cliff built with one `scale=` reads as lumps however
	 * the amplitude is tuned, and three numbers make it read as layered stone.
	 *
	 * A single value fills all three, so every file written before this reads identically. Each
	 * component is a positive length; a percentage is refused for `ReadLength`'s reason.
	 */
	GRAPHCODESHAPE_API bool ReadLengths3(const FGcShapeDocument& Document, const FString& Verb,
		const FString& Argument, const FString& Value, UE::Math::TVector<double>& Out,
		FString& OutError);

	/**
	 * `along=` — a DIRECTION: one of the six signed axes, or a vector, which is normalised.
	 *
	 * `Z` when the line does not say. **It is normalised before use and that is what makes a
	 * distance beside it real** — the waterfall writes `along=(0.22, 0, -0.98)`, so its `height=300`
	 * falls 300 rather than 300 times the vector's length.
	 *
	 * **THIS MOVED OUT OF THE EVALUATOR AT M6.3 BECAUSE GROUP 4 IS A SECOND READER.** `taper` and
	 * `roughen` take the same argument as `extrude` — where their profile's `t` runs — and a second
	 * copy of *what a direction is* is a second answer, which is the objection that put every other
	 * argument reading in this file ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * **`sweep(along=$Path)` IS A DIFFERENT ARGUMENT WEARING THE SAME WORD** — it FOLLOWS an entry
	 * rather than pointing somewhere — and it does not come through here.
	 */
	GRAPHCODESHAPE_API bool ReadAlong(const FGcShapeInstruction& Line,
		UE::Math::TVector<double>& Out, FString& OutError, const FGcShapeDocument* Document = nullptr);

	/**
	 * How many segments a circle of this radius needs to stay inside the file's chord tolerance.
	 *
	 * `smooth:` is a TOLERANCE and not a count, so this is where that ruling is actually spent: the
	 * sagitta of one segment is `r(1 - cos(pi/n))`, so `n = pi / acos(1 - tol/r)` is the smallest
	 * count that stays inside it. A kit authored at a fixed count has over-tessellated bolts and
	 * faceted archways; this gives each shape the count its own size earns.
	 */
	GRAPHCODESHAPE_API int32 SegmentsForArc(double Radius, double Tolerance, double SweepDegrees);

	/**
	 * **A ROUND MUST NOT CREASE ITSELF — S2, `Docs/GC-SHAPE-FIELD-REPORT-2026-10-13.md`.** The chord
	 * count above prices the SILHOUETTE; `shade:` is where this grammar says an edge turns hard, so a
	 * round whose facets each turn more than `shade:` draws a hard line down its own middle. MEASURED
	 * on the defaults: a 2 cm round at `smooth:0.5` was 2 segments meeting at 45 degrees, above
	 * `shade:40`. So the count is the chord count or `floor(sweep / shade) + 1`, whichever is larger.
	 * `shade:0` asks for facets everywhere and adds nothing.
	 */
	GRAPHCODESHAPE_API int32 SegmentsForRound(double Radius, double Tolerance, double ShadeDegrees,
		double SweepDegrees);

	/**
	 * **THE SAME PROMISE FOR A MESH ROUND, PRICED ON THE CURVE THE ENGINE ACTUALLY DRAWS.** `FMeshBevel`
	 * does not sample a circle at equal angles: `MakeArcSplineCurve` is a Hermite curve with tangents
	 * scaled by sqrt(2), sampled at equal PARAMETER steps, so its middle joints turn more than its
	 * ends. MEASURED by codex/shape-inbox 2026-10-13 (`Docs/Research/shape-field-s1-s3-review.md`):
	 * `floor(90/10)+1 = 10` segments still turn 10.85 degrees at `shade:10`. So this walks the
	 * engine's own planar curve for two faces whose normals turn `TurnDegrees` and adds segments from
	 * the chord count until no joint (face to facet, facet to facet) turns more than `shade:`. At the
	 * defaults a right-angle edge is unchanged (2 segments, 25.5 / 39 / 25.5 degrees). Capped at 512.
	 */
	GRAPHCODESHAPE_API int32 SegmentsForBevel(double Radius, double Tolerance, double ShadeDegrees,
		double TurnDegrees);
}
