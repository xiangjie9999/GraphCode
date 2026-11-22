// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BoxTypes.h"
#include "TransformTypes.h"

struct FGcShapeDocument;
struct FGcShapeArgument;
struct FGcShapeInstruction;

/**
 * WHERE AN INSTRUCTION'S CONTRIBUTION SITS. `GC-SHAPE-GRAMMAR.md` §4.1;
 * `GC-SHAPE-PLACEMENT-DESIGN.md` §2 is the reasoning and §4 is the whole surface.
 *
 * **THIS IS THE ONE READER, AND THAT IS THE POINT.** Every instruction that takes `at=` goes through
 * `Read` and nothing parses a position for itself — the same discipline `FGcShapeProfile::Parse`
 * imposes on every numeric argument. It is what makes the relational half of `at=` a change to ONE
 * function rather than to every instruction that takes a position, which is the objection the board
 * raised against ordering M3 before the placement design was ruled.
 *
 * **`at=` HAS THREE READINGS AND THE TEXT DECIDES WHICH**, which is not an overload:
 *
 * | written | reading |
 * |---|---|
 * | `at=(0, 0, -750.0)` · `at=(-215.0, 0)` | a POINT — a plain translation, and a 2-tuple is an outline's |
 * | `at=65%` | a FRACTION of the shape's own span along the instruction's axis — `cut(plane=Z, at=65%)` |
 * | `at=against($T, +Z, gap=46.0)` · `at=on()` · `at=center()` | a RELATION — measured, §2 |
 *
 * The three are one idea under §4.1's own rule — *a percentage in a spatial position is a fraction of
 * a measured extent* — so a fraction is what an anchor already is, and a point is the case where
 * nothing is measured. **A point is NOT superseded by a relation**: `circle(radius=…, at=(-215, 0))`
 * places an outline in a plane where there is nothing to measure against, and the shipped corpus
 * writes both forms in one file (`GC-SHAPE-EXAMPLE.md`).
 */

/**
 * THE THREE TEXT READINGS EVERY PLACEMENT ARGUMENT IS BUILT OUT OF, in one place.
 *
 * **THEY LIVE HERE BECAUSE THE DUPLICATION HALF IS A SECOND READER.** `spread=(80%, 90%)`,
 * `step=((60,0,0), (0,0,80))` and `jitter=(move=…, turn=…)` are the same tuple-of-tuples and the
 * same *bare number or a percentage* as `align=` and `gap=` — and a second copy of *what `80%`
 * means* is a second answer, which is the objection that put every argument reading in one file
 * already ([[two-carriers-of-one-fact-is-a-race]]).
 */
namespace GraphCodeShapeText
{
	/** `(a, b, c)` → its parts, or false where the text is not parenthesised as ONE balanced group. */
	GRAPHCODESHAPE_API bool Unwrap(const FString& Text, FString& OutInside);

	/** Top-level commas, each part trimmed. False only where the brackets do not balance. */
	GRAPHCODESHAPE_API bool Split(const FString& Text, TArray<FString>& OutParts);

	/**
	 * **ONE `Label: value` ENTRY OF A LIST, SPLIT AT ITS FIRST COLON**, both sides trimmed. False where the text has no
	 * colon. Shared by `displace(slope=(X: …))` and `loft(rails=(G: …))` (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.6.1:
	 * "lifted into a shared helper, not copied"); the value may hold colons of its own, `P: (X: $Top)`.
	 */
	GRAPHCODESHAPE_API bool Labelled(const FString& Part, FString& OutLabel, FString& OutValue);

	/**
	 * **IS THIS TEXT ONE NUMBER LITERAL, EXPONENT FORM INCLUDED** (fault 6, 2026-11-20): what `FString::IsNumeric` took, OR
	 * core's `GraphCodeScalarExpression::IsNumericLiteral` (`1e-14`, `2.5E+3`). G28e writes a set's values with `%.17g`, so
	 * a tiny computed value comes back as `1.4210854715202004e-14`, and `IsNumeric` refused it. Only text refused before
	 * is newly read; every text read before reads as it did.
	 */
	GRAPHCODESHAPE_API bool IsNumber(const FString& Text);

	/** A bare number, or `82%` as 0.82. Resolves nothing: use the overload below where a name may go. */
	GRAPHCODESHAPE_API bool Number(const FString& Text, double& Out, bool& bOutPercent);

	/**
	 * The same, but through `FGcShapeProfile::Parse` — so a `$name` and an EXPRESSION both work.
	 *
	 * **THIS WAS PRIVATE TO ONE FILE AND THE MEASUREMENT THAT FOUND IT WAS A LIVE RUN.** MC4 §2.6's
	 * census listed six clusters by grepping for `Atod`/`Atoi`, and the duplication arguments do not
	 * grep that way — they go through the bare `Number` above, which resolves nothing. So
	 * `step=(($Bay, 0, 0))` was refused as *not a distance in centimetres* while `at=($Bay, 0, 0)`
	 * on the same line worked, and no spec could see it because the corpus writes both as literals.
	 * **A census that greps for a spelling misses every caller that reached the spelling through a
	 * helper** ([[a-shared-reader-with-private-writers]]).
	 */
	GRAPHCODESHAPE_API bool Number(const FGcShapeDocument& Document, const FString& Text,
		double& Out, bool& bOutPercent, FString& OutError);
}

/** One of the six signed axes, and there are no others (§4.1: up exists and front does not). */
struct GRAPHCODESHAPE_API FGcShapeAxis
{
	/** 0, 1, 2 for X, Y, Z. */
	int32 Index = 2;

	/** +1 or -1. */
	double Sign = 1.0;

	/** `X` `+X` `-X` `Y` `-Y` `Z` `-Z` — and a plain name means `+`. */
	static bool Parse(const FString& Text, FGcShapeAxis& Out);

	FString ToText() const;
};

/**
 * One axis of a relation: a fraction of the target's span, a fraction of the subject's, an offset.
 *
 * **§2's whole mechanism, and there is nothing else to it.** The subject moves until the two planes
 * coincide:
 *
 *     P_target = near(T, A) + Target × span(T, A)
 *     P_self   = near(S, A) + Self   × span(S, A)
 *     move     = P_target − P_self + Offset
 */
struct GRAPHCODESHAPE_API FGcShapeAnchor
{
	double Target = 0.5;
	double Self = 0.5;

	/** Centimetres, or a fraction of the SUBJECT's span when `bOffsetIsFraction` (§3). */
	double Offset = 0.0;
	bool bOffsetIsFraction = false;

	/** True where the file said something about this axis, rather than it falling to the centres. */
	bool bStated = false;
};

enum class EGcShapeAtKind : uint8
{
	/** No `at=` at all — the origin, and no measurement. */
	None,

	/** `at=(x, y)` or `at=(x, y, z)`. */
	Point,

	/** `at=65%` — resolved by the instruction, which is the only thing that knows its axis. */
	Fraction,

	/** `against` · `center` · `on` · `under` — and `at=(92%, 0%, 50%)`, which is `center` with numbers. */
	Relation,

	/**
	 * `at=$Pin` — a place named by something the BODY holds, MB6.
	 *
	 * **CARRIED HERE AND SPENT BY THE INSTRUCTION, which is exactly what `Fraction` already does.**
	 * A captured region's centre and a handle's placed bounds are both places, and neither is
	 * anything this reader can look up — only the evaluator knows what a body captured. So the
	 * reading belongs to the one placement reader and the ANSWER belongs to the verb, and `Resolve`
	 * refuses it for every instruction that has no way to ask ([[ask-the-interface-not-the-struct]]).
	 */
	Named,
};

/**
 * `at=`, `align=`, `rotate=` and `scale=`, read once and resolved once.
 *
 * **THE ORDER WITHIN ONE LINE IS SCALE, THEN ROTATE, THEN PLACE, AND IT IS FORCED** rather than
 * chosen: a relation measures where the subject will actually be, so it must measure the subject
 * after its own `scale=` and `rotate=` have moved its vertices. `add($Ring, rotate=(90,0,0),
 * at=on())` seats the ring that was turned on its side — read the other way round it would seat the
 * ring's untouched height and then tip it through the shape it was seated on.
 */
struct GRAPHCODESHAPE_API FGcShapePlacement
{
	EGcShapeAtKind Kind = EGcShapeAtKind::None;

	/** `EGcShapeAtKind::Point`. Z is zero for a 2-tuple, which `bPointIsFlat` records. */
	UE::Math::TVector<double> Point = UE::Math::TVector<double>::Zero();
	bool bPointIsFlat = false;

	/** `EGcShapeAtKind::Fraction` — `at=65%` as 0..1. */
	double Fraction = 0.0;

	/** `EGcShapeAtKind::Relation`: the entry named, or empty for the accumulator (§4.1). */
	FString Target;

	/** `EGcShapeAtKind::Named` — the name without its `$`. */
	FString Name;

	/** The word as written, for a refusal that quotes the line back. */
	FString Word;

	/**
	 * **THE `at=` AS WRITTEN IS ALREADY WHOLE, with no parentheses left to fill** — `(92%, 0%, 50%)`
	 * and `$Pin`, against `center` and `against`, which are words a refusal completes with `(…)`.
	 *
	 * Recorded rather than guessed at from the text, for `bAskedACondition`'s reason: a refusal
	 * quoting back `at=(92%, 0%, 50%)(…)` is quoting a line nobody wrote.
	 */
	bool bAtIsWhole = false;

	/** The `at=` as the file wrote it. Every refusal that quotes one back goes through this. */
	FString AtText() const
	{
		return bAtIsWhole ? Word : Word + TEXT("(…)");
	}

	/**
	 * THE AXES THE RELATION FIXED — `align=` may not restate one and an array's spread may not
	 * consume one. `against` `on` `under` fix ONE, `edge` TWO and `corner` THREE; `center` fixes
	 * NONE, because it states the default every unmentioned axis already has and `align=` may
	 * still override an axis of it.
	 *
	 * **THIS WAS ONE AXIS UNTIL MB4** and the resolver never was: `Resolve` has always run §2's
	 * four lines per axis over all three anchors, so a word fixing two or three of them needed no
	 * new arithmetic — only somewhere to record which. Keeping it as a single index would have
	 * left `corner()` fixing three axes while one door believed it fixed one
	 * ([[a-switch-honoured-at-one-door-only]]).
	 */
	bool bAxisFixed[3] = { false, false, false };

	/**
	 * **`at=on(snap=true)` — SEAT AGAINST THE SURFACE UNDER THE PART, NOT AGAINST THE TARGET'S
	 * BOUNDING BOX.** Row MB18, user-raised 2026-09-07.
	 *
	 * **THIS IS NOT `contact=touch|bounds`, WHICH `GC-SHAPE-PLACEMENT-DESIGN.md` §10 DELETED, AND
	 * THE DIFFERENCE IS WHY IT COMES BACK.** That ruling is correct as written: for an axis
	 * direction the support function and the AABB extent ARE the same number, so there is nothing
	 * to choose about where the target's own extreme is. **What it did not name is that the extreme
	 * and the surface UNDER THIS PART are different places on everything that is not flat.** §10's
	 * dome is the case where they coincide — a subject at the crown touches — and moving that same
	 * subject off-centre is the case nobody wrote down: the crown is still the box top, the flank
	 * below the subject is not, and the subject hangs in the air by the difference.
	 *
	 * **MEASURED RATHER THAN ARGUED, by MB17 on its first run over the tree: 102 placements that
	 * never reach what they join** — `BarPull`'s post 11.420 cm clear of its bar, `Balkenkreuz` at
	 * 6 cm on three copies — every one of them a box seat that landed on an extreme somewhere else.
	 *
	 * **SO THE AXIS OF CHOICE IS LOCAL AGAINST GLOBAL, NOT TOUCH AGAINST BOUNDS**, and `snap` is
	 * the word for it: rest on the target's highest point, or descend until this part actually
	 * lands. Off by default, because turning it on would move geometry in every shipped file.
	 */
	bool bSnap = false;

	/**
	 * **WHICH SURFACE TO LAND ON — `at=on(snap=$Roof)`. USER-RULED 2026-09-07.**
	 *
	 * Empty with `snap=true` means *the first thing under the part, whatever it is*, and that is the
	 * roof gun's whole failure: the tallest thing beneath a turret-mounted gun was the hatch HANDLE,
	 * so a snap seats it perfectly on the handle — **worse than the float it replaced, because it
	 * now looks deliberate.** A snap can correct a seat and can never infer which object was meant,
	 * so the file has to be able to say.
	 *
	 * **AND IT ONLY BECAME CHEAP WHEN MB21 LANDED.** `State.Placed` keeps a placed part's BOUNDS and
	 * no surface — which is why a named target was refused when `snap` first shipped — but MB21 kept
	 * every added part's MESH so a later part could be measured against it. Two rows built the same
	 * day, and the second one paid for the first one's refusal.
	 *
	 * **ONE KEY, AND THE VALUE SAYS WHICH**, exactly as §3.6's `$Shell = mesh` decides its kind from
	 * its right-hand side: `snap=true` is any surface and `snap=$Name` is that one.
	 */
	FString SnapTo;

	/**
	 * The ONE axis the seat fixed, and which side of the target the part was seated on — so the
	 * snap knows which way to travel. `-Sign` along `SnapAxis`, because `against(+A)` puts the
	 * part's low side on the target's high side and the part therefore descends.
	 *
	 * `INDEX_NONE` whenever `bSnap` is false. A relation fixing two or three axes cannot fill these
	 * and is refused by name in `Read`.
	 */
	int32 SnapAxis = INDEX_NONE;
	double SnapSign = 1.0;

	FGcShapeAnchor Anchors[3];

	/** Degrees, applied about the subject's own origin. */
	UE::Math::TVector<double> Rotate = UE::Math::TVector<double>::Zero();

	/** `scale=` — uniform or per axis, `100%` being 1.0. */
	UE::Math::TVector<double> Scale = UE::Math::TVector<double>::One();

	bool bMoves() const
	{
		return Kind != EGcShapeAtKind::None || !Rotate.IsZero() || Scale != UE::Math::TVector<double>::One();
	}

	/**
	 * Read `at=`, `align=`, `rotate=` and `scale=` off one instruction.
	 *
	 * **THE DOCUMENT IS READ BECAUSE A POSITION'S NUMBERS ARE NUMBERS** (§3): `gap=-$Flute_Depth` and
	 * `at=(0, 0, $Rise)` reach a number entry exactly as `extrude(height=$Tall)` does. A varying
	 * profile is refused by name, for `circle(radius=)`'s reason — `t` has to run along something,
	 * and a place is measured along nothing.
	 *
	 * Every refusal that is a property of the TEXT is raised here — a direction that is not one of
	 * the six, `against()` with none at all, an anchor outside its span, `align=` on the axis the
	 * relation already fixed, `align=` where `at=` is a plain point. The ones that need a shape (a
	 * target nothing placed, a target that is not a shape) belong to `Resolve`.
	 */
	static bool Read(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		FGcShapePlacement& Out, FString& OutError);

	/**
	 * Read only `at=`'s value through the canonical placement grammar.
	 *
	 * UV projection frames need the same point / bounds-fraction / named-place / `center()`
	 * language without also inheriting placement's `align=`, `rotate=` and shaping `scale=`. Keeping
	 * the one `at=` reader public prevents the two coordinate systems from drifting while allowing
	 * each consumer to decide which readings it can actually resolve.
	 */
	static bool ReadAt(const FGcShapeDocument& Document, const FString& Text,
		FGcShapePlacement& Out, FString& OutError);

	/**
	 * The same reading, from a bare argument list rather than an instruction — MC3's
	 * `[in=$Tool, at=…]`, where a placement lives inside a BRACKET.
	 *
	 * **ONE READER, SO EVERY REFUSAL IT MAKES HOLDS IN BOTH PLACES.** A second placement reader for
	 * brackets is one concept with two grammars, which is the defect MT13b measured
	 * ([[one-concept-two-grammars]]).
	 */
	static bool ReadFrom(const FGcShapeDocument& Document,
		const TArray<FGcShapeArgument>& Arguments, const FString& Verb, FGcShapePlacement& Out,
		FString& OutError);

	/**
	 * The scale-and-rotate half, as a transform about the subject's own origin.
	 *
	 * Split from `Resolve` because it has to run FIRST — the subject is transformed, and only then
	 * is it measured for the relation.
	 */
	UE::Geometry::FTransformSRT3d Shaping() const;

	/**
	 * The translation, given the subject as it will be placed and whatever it is measured against.
	 *
	 * `Subject` and `Against` are both bounds because for an AXIS direction the support function and
	 * the AABB extent are the same number (`GC-SHAPE-PLACEMENT-DESIGN.md` §10) — which is why
	 * `contact=touch|bounds` was deleted before it was proposed.
	 */
	bool Resolve(const UE::Geometry::FAxisAlignedBox3d& Subject,
		const UE::Geometry::FAxisAlignedBox3d& Against, UE::Math::TVector<double>& OutMove,
		FString& OutError) const;
};
