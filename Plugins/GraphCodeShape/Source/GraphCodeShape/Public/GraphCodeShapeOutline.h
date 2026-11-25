// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

#include "Curve/GeneralPolygon2.h"
#include "Polygon2.h"

struct FGcShapeDocument;
struct FGcShapeHeader;
struct FGcShapeInstruction;

/**
 * **A KEY THAT SAYS SOMETHING: A NAME, A CORNER, OR BOTH** — `GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.1.
 * `points(GR: (45, 9)@sharp, …)`. `Point` is the index of the point the written key landed on in its
 * loop's points; a key with neither a name nor a mark is not stored, and a heading is spent by the
 * sampler and never kept.
 */
struct GRAPHCODESHAPE_API FGcShapeKey
{
	int32 Point = INDEX_NONE;
	FString Name;
	bool bSharp = false;

	/**
	 * B1 (§5.5.5): the mirrored twin of a named key of a `loop=mirror` half. Its `Name` is the key's name
	 * with a prime (`G'`), which no file can write, so a twin never collides with a written name.
	 */
	bool bTwin = false;

	/** B1 (§5.5.4 b): where this key sits in the outline's `Reference`, or `INDEX_NONE` without one. */
	int32 RefPoint = INDEX_NONE;
};

/** A key as WRITTEN, before sampling: its name, its mark, and a heading's direction and length. */
struct GRAPHCODESHAPE_API FGcShapeWrittenKey
{
	FString Name;
	bool bSharp = false;
	bool bHeading = false;
	UE::Math::TVector<double> Heading = UE::Math::TVector<double>::ZeroVector;
};

/**
 * **E1 — THE WRITTEN CURVE ITSELF, KEPT NEXT TO ITS REFERENCE** (`GC-SHAPE-LOFT-QUALITY-DIAGNOSIS.md` §4.1 step 1). A
 * closed `curve()` as `curve()` built it: key `i` at parameter `i`, and each key's FINAL tangents (auto, a `@sharp`
 * key's two chords, a heading's), so `Eval` is the curve `GcShapeSampleKeyedCurve` sampled, and a loft with `rails=`
 * can put its columns ON it rather than on a polyline that already spent `smooth:/8`. Empty on everything else.
 */
struct GRAPHCODESHAPE_API FGcShapeWrittenCurve
{
	TArray<UE::Math::TVector<double>> Keys;
	TArray<UE::Math::TVector<double>> Arrive;
	TArray<UE::Math::TVector<double>> Leave;

	bool IsEmpty() const { return Keys.Num() < 2 || Arrive.Num() != Keys.Num() || Leave.Num() != Keys.Num(); }

	/** The closed curve at `Param` (taken modulo the key count): the cubic Hermite `FInterpCurve::Eval` evaluates. */
	UE::Math::TVector<double> Eval(double Param) const;

	/** Move it the way a placement moved its outline: keys by `Map`, tangents by `Map(P + T) − Map(P)` (affine maps). */
	void MapBy(TFunctionRef<UE::Math::TVector<double>(const UE::Math::TVector<double>&)> Map);

	void Reset() { Keys.Reset(); Arrive.Reset(); Leave.Reset(); }
};

/**
 * WHAT GROUP 1 MAKES AND GROUP 2 CHANGES. `GC-SHAPE-GRAMMAR.md` §4 groups 1 and 2.
 *
 * **M1 AND M2 COULD USE `FPolygon2d` AND M4 CANNOT, because M4 is where OPEN happens.** `circle` and
 * `rect` are closed by construction, so a type whose every method assumes a closing edge was the
 * right one for two instructions; `arc`, `points` and `curve` are open unless a `loop=` says
 * otherwise, and the difference decides what may follow — `extrude` needs a closed outline
 * (§7.1 case B, user-ruled), `widen` turns an open one into a closed one, `revolve` reads either.
 *
 * **A 2D OUTLINE AND A 3D PATH ARE THE SAME TWO INSTRUCTIONS, TOLD APART BY THE TUPLE WIDTH**, so
 * `bPath` is set from what was WRITTEN and never inferred from the numbers: `points((0,0,0), (100,
 * 0,0))` is a path that happens to be level, and a reader that decided by looking at Z would call it
 * an outline and let `extrude` have it.
 *
 * **AN OUTLINE IS ONE OUTER BOUNDARY AND ANY NUMBER OF HOLES — MH.** `GC-SHAPE-GRAMMAR.md` §1 said
 * a `plane()` may carry them and until MH nothing could: `widen` around a closed path and a 2D
 * `subtract` both refused rather than dropping the inner loop, because a ring silently returned as
 * its outer boundary is a solid disc and nothing downstream could tell.
 *
 * **WHAT MH DID NOT BUY IS SEVERAL DISJOINT PIECES**, and that is a limit rather than an oversight.
 * `PolygonsUnion`/`Difference`/`Intersection` all return a `TArray<FGeneralPolygon2d>`, so a boolean
 * really can split a shape in two — but `to=` scales about *the outline's own centre*, `round` walks
 * *the* corner list and `t` runs along *the* arc length, so a second piece has no answer for any of
 * them. A boolean that splits is REFUSED naming the count, and the way out is two entries combined
 * in 3D after a depth instruction, which has worked since M3.
 *
 * **AND HOLES REACH DEPTH THROUGH `extrude` AND `fill` ONLY.** `revolve`, `sweep` and `loft` go
 * through engine generators that take a single `FPolygon2d` cross-section, so each refuses a holed
 * outline by name rather than turning the outer boundary alone — the same silent-solid trap one
 * layer down.
 */
struct GRAPHCODESHAPE_API FGcShapeOutline
{
	/** In order. Z is zero throughout a flat outline and carries the third component of a path. */
	TArray<UE::Math::TVector<double>> Points;

	/** The last point joins back to the first: `polygon`'s old meaning, now `loop=true`. */
	bool bClosed = false;

	/**
	 * **INNER LOOPS, EACH ALWAYS CLOSED AND ALWAYS FLAT** — MH. Empty on everything group 1 makes.
	 *
	 * Only two things produce one: a 2D `subtract`/`intersect` whose result encloses a void, and
	 * `widen` around a closed path, which is a ring. **A hole is never open and never a path**, so
	 * it carries no `bClosed`/`bPath` of its own — a flag that can only hold one value is a flag
	 * a reader has to check and a writer can get wrong.
	 *
	 * **`Points` STAYS THE OUTER BOUNDARY AND KEEPS EVERY MEANING IT HAD**, which is what makes MH
	 * additive: `Parameters()`, `Length()`, `round()`'s corner walk and `to=`'s centre all read the
	 * outer alone, exactly as they did, and an outline with no holes behaves byte-for-byte as before.
	 */
	TArray<TArray<UE::Math::TVector<double>>> Holes;

	/** True where a boolean or a `widen` left a void inside the boundary. */
	bool HasHoles() const { return !Holes.IsEmpty(); }

	/** Written with three-wide tuples. A PATH — what `sweep(along=)` follows. Never inferred. */
	bool bPath = false;

	/**
	 * **THE KEYS THE AUTHOR NAMED OR MARKED, ON THE OUTER BOUNDARY, ASCENDING `Point`** — §5.1. Empty
	 * on every outline written without a name or a mark, which is every file written before them, so
	 * every instruction below takes the path it always took (the ledger's §10).
	 */
	TArray<FGcShapeKey> Keys;

	/** The same for each hole, parallel to `Holes`: empty, or one list per hole. */
	TArray<TArray<FGcShapeKey>> HoleKeys;

	/** True where any loop carries a named or marked key. */
	bool HasKeys() const;

	/** The outer boundary's key named `Name` (case-sensitive), or null. */
	const FGcShapeKey* FindKey(const FString& Name) const;

	/** B1: true where the outer boundary NAMES a key, which is what makes a loft over it keyed (§5.5.1). */
	bool HasNames() const;

	/**
	 * **B1 — THE REFERENCE A KEYED LOFT FITS AGAINST** (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.5.4 b). A named
	 * closed `curve()` is ALSO sampled at `smooth:/8` and kept here, so a keyed loft's rings stay within
	 * `smooth:` of the WRITTEN curve rather than of a polyline that already spent it. Empty on every outline
	 * that names no key (the ledger's §10: made only for keyed outlines); `points()` keeps none, its points
	 * are its reference. Placements carry it (`MapExtras`); an instruction that rebuilds points drops it.
	 */
	TArray<UE::Math::TVector<double>> Reference;

	/**
	 * E1 (`GC-SHAPE-LOFT-QUALITY-DIAGNOSIS.md` §4.1): the named closed `curve()` the reference was sampled from, and each
	 * reference vertex's parameter on it. Kept, moved, reversed and dropped exactly with `Reference`; read only by a loft
	 * with `rails=`, so nothing else changes. Empty for `points()` (its points are its curve) and every unnamed outline.
	 */
	FGcShapeWrittenCurve Curve;
	TArray<double> RefParams;

	/** B1 (§5.5.5): a `loop=mirror` half, closed through its reflection across the line `MirrorA`–`MirrorB`. */
	bool bMirrorHalf = false;
	UE::Math::TVector<double> MirrorA = UE::Math::TVector<double>::ZeroVector;
	UE::Math::TVector<double> MirrorB = UE::Math::TVector<double>::ZeroVector;

	/** B1: move the reference and the mirror line the way a placement moved the points. */
	void MapExtras(TFunctionRef<UE::Math::TVector<double>(const UE::Math::TVector<double>&)> Map);

	/**
	 * B1: the reference a keyed loft reads, and where each key sits in it. The stored `Reference` where it
	 * is still the outline's own (every key's `RefPoint` valid and on its point, within 1e-6 cm); else the
	 * points themselves. A stale reference is never read: the points are the conservative answer.
	 */
	void KeyedReference(TArray<UE::Math::TVector<double>>& OutReference, TArray<FGcShapeKey>& OutKeys) const;

	/**
	 * E1: the curve behind the reference `KeyedReference` hands out, and each of its vertices' parameters; both empty
	 * where that reference is the points themselves (stale, `points()`, or a rebuilt outline).
	 */
	void KeyedCurve(FGcShapeWrittenCurve& OutCurve, TArray<double>& OutParams) const;

	bool IsEmpty() const { return Points.IsEmpty(); }

	/**
	 * The plane this outline lies in, as a world Z. Zero on an empty one.
	 *
	 * **ONLY MEANINGFUL WHERE `IsFlat()`** — on a path it is merely the first point's Z.
	 */
	double Height() const;

	/**
	 * True where every Z is THE SAME, which is what a depth instruction needs of an outline.
	 *
	 * **IT USED TO MEAN *every Z is zero*, and that conflated flat with at-the-origin** (row MZ).
	 * A depth instruction needs the outline to lie in ONE plane; which plane is `Height()`, and a
	 * verb either carries it or refuses it by name. Nothing may drop it silently.
	 */
	bool IsFlat() const;

	/** The closed, flat reading of the OUTER boundary. Only valid where `bClosed` and `IsFlat()`. */
	UE::Geometry::FPolygon2d ToPolygon() const;

	/**
	 * The outer boundary and every hole, which is the currency all three 2D booleans take.
	 *
	 * **THE HOLES ARE WOUND OPPOSITE THE OUTER**, because `TGeneralPolygon2::AddHole` checks
	 * orientation and containment and silently returns false rather than throwing — a hole added
	 * the wrong way round would simply not be there.
	 */
	UE::Geometry::FGeneralPolygon2d ToGeneralPolygon() const;

	/**
	 * The inverse: an outer loop plus its holes becomes this outline. Flat, closed, not a path.
	 *
	 * **KEYS GO BY POSITION** (§5.3): each moves to the new vertex within 1e-6 cm of the point it was on,
	 * on the outer boundary or a hole; a key with no such vertex is dropped and named in `OutDropped`.
	 */
	void FromGeneralPolygon(const UE::Geometry::FGeneralPolygon2d& Polygon, TArray<FString>* OutDropped = nullptr);

	/**
	 * **THE OUTER COUNTER-CLOCKWISE AND EVERY HOLE CLOCKWISE, WHICH IS WHICH WAY THE SOLID FACES.**
	 *
	 * MT1's second half. A depth instruction turns this outline into a surface, and the order the
	 * points are in is what decides whether that surface faces out of the shape or into it. `rect`
	 * emits counter-clockwise and says so in its own comment; `circle` and `arc` are generated the
	 * same way — but **`points` and `curve` are whatever the author typed**, so a closed loop
	 * written clockwise came back a solid that is invisible from outside and solid from within,
	 * with every number on the build line reading exactly as the correct shape's.
	 *
	 * **CALLED AT THE DEPTH DOOR AND NEVER AT THE OUTLINE ONE, and that is load-bearing**: `round`
	 * takes a tuple that indexes corners BY POINT ORDER (`round((0, 8, 8, 0))`), so reversing a
	 * loop before group 2 has run would silently move which corners a tuple names. By the time a
	 * depth instruction has the outline, every shaping verb has already run and the order is free.
	 *
	 * Does nothing to a path or an open outline, where a winding is not a fact about anything.
	 */
	void NormaliseWinding();

	/**
	 * **A STRAIGHT RUN IS BROKEN UP UNTIL NO SEGMENT IS MUCH LONGER THAN THE OUTLINE'S OWN SHORTEST
	 * FEATURE — board row MT13, reported from a WIREFRAME and MEASURED 2026-08-26.**
	 *
	 * `smooth:` prices CURVATURE, and a straight edge has none, so a flat face got the MINIMUM
	 * triangulation however big it was: `rect(w=900, h=120) extrude` was the same 12 triangles as
	 * `rect(w=100, h=100)` — `top 2 / bottom 2 / side 8` — which is a 900 x 120 cm cap described by
	 * two triangles with a 908 cm diagonal, while `circle(radius=450)` at the same span bought 296.
	 * **Correct as geometry and wrong as a surface**: two triangles describe a rectangle exactly, and
	 * then everything that varies ACROSS the face is interpolated in two halves — lit shading, an
	 * `auto` unwrap, and any later decimation. The user saw it as one corner-to-corner diagonal
	 * across a carriage door with the wood grain streaking along it.
	 *
	 * **THIS IS MS10'S OWN RULE APPLIED ONE DOOR EARLIER.** `GcShapeSquareTheWall` inserts rings
	 * along the TRAVEL, so it cannot reach a quad that is wide because the OUTLINE's segment is long
	 * — on a 900 x 120 x 20 slab it computes `Each = 1` and correctly declines, since more rings
	 * would make those quads MORE elongated rather than less. The free variable it cannot touch is
	 * the segment itself, and that is this.
	 *
	 * **THE NUMBER COMES OFF THE OUTLINE AND IS TYPED BY NOBODY**, exactly as nobody types a circle's
	 * segment count. `MostElongated` is the wall's own 2.0: an outline whose segments are ALL the
	 * same length is never touched, so every circle, arc and regular polygon in the corpus comes back
	 * bit-identical, and only a mix of long and short runs — which is what elongation IS — is
	 * divided. A 900 x 900 square keeps its two triangles and should: they are equilateral enough
	 * that nothing downstream suffers, and the defect was never size, it was aspect.
	 *
	 * **THE FLOOR IS WHAT KEEPS A FILLET FROM DECIDING THE WHOLE FACE.** `round(4.0)` on a metre-wide
	 * panel leaves corner chords a few millimetres long, and measuring the panel against those would
	 * buy hundreds of rings for a fillet nobody is looking at — the same argument `GcShapeSquareTheWall`
	 * makes for taking the MEAN segment rather than the shortest. So the feature is the shortest
	 * segment or `1/FilletFloor` of the longest, whichever is bigger.
	 *
	 * **CALLED AT THE DEPTH DOOR, FOR THE REASON `NormaliseWinding` IS**: `round((0, 8, 8, 0))`
	 * indexes corners BY POINT ORDER, so inserting points before group 2 has run would silently move
	 * which corners a tuple names.
	 *
	 * Holes are divided with the outer, against one feature size for the whole outline, so a void
	 * and the plate around it are triangulated at one density. Returns how many points were inserted;
	 * **whatever the ceiling costs is silent by design** — nothing is lost, the outline is simply
	 * divided less far than it would like, and there is no author decision behind it to report.
	 *
	 * **IT IS `ApplyDivision(DivisionPattern())`, SPLIT IN TWO FOR THE LOFT** — L0
	 * (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.8): a loft divides every ring by its start's own pattern,
	 * so the pattern has to exist apart from the points it was computed on.
	 */
	int32 DivideLongSegments();

	/**
	 * **WHAT `DivideLongSegments` WOULD DO, SAID WITHOUT DOING IT** — L0 (§5.8). The pieces each WRITTEN
	 * edge would be cut into, 1 for an edge left whole: the outer boundary's edges first, then each
	 * hole's (a hole under three points has none), the ceiling included. All ones where nothing would be
	 * cut; empty where it declines outright (a path, an open outline, under three points).
	 */
	TArray<int32> DivisionPattern() const;

	/** Insert what `DivisionPattern` said, at equal fractions of each edge. Returns the points added. */
	int32 ApplyDivision(const TArray<int32>& Pattern);

	/**
	 * Where each point sits along `t`, BY ARC LENGTH (§3).
	 *
	 * **ARC LENGTH RATHER THAN VERTEX INDEX, and the reason is `smooth:`.** A circle's vertex count
	 * is solved from the chord tolerance, so under an index-based `t` the same file at two
	 * tolerances would put a profile's stops in different PLACES on the same shape — tessellation
	 * would change geometry, which is the one thing a tolerance must never do.
	 *
	 * **ON A LOOP THE CLOSING EDGE IS COUNTED AND `t=1` IS THE FIRST POINT AGAIN**, which is the
	 * whole of the seam the `loop=` ruling refuses a disagreeing profile over: the domain really
	 * does wrap, so two values at one place is a contradiction rather than a rounding.
	 */
	void Parameters(TArray<double>& OutT) const;

	/** Total length, closing edge included on a loop. */
	double Length() const;
};

/**
 * GROUPS 1 AND 2 — the outline, and what shapes it while it is still flat.
 *
 * Split out of `GraphCodeShapeBuild.cpp` at M4 rather than for tidiness: group 1 gained four
 * instructions, a curve sampler and a corner-fillet solver in one milestone, and the evaluator is
 * where groups 3 and 5 live. Each half is now about one thing.
 */
namespace GraphCodeShapeOutline
{
	/**
	 * `circle(radius=, segments=, at=, rotate=)`.
	 *
	 * `segments=` is a COUNT and a profile there is refused by name; without one the count is solved
	 * from the header's chord tolerance, so a shape gets the tessellation its own size earns.
	 */
	GRAPHCODESHAPE_API bool RunCircle(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, FGcShapeOutline& Out, FString& OutError);

	/** `rect(w=, h=, at=, rotate=)`. Corners run counter-clockwise from `(-x,-y)`. */
	GRAPHCODESHAPE_API bool RunRect(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		FGcShapeOutline& Out, FString& OutError);

	/**
	 * `arc(radius=, from=, to=, segments=, at=)` — degrees, and an OPEN outline.
	 *
	 * Angle `0` is `+x` and the sweep runs counter-clockwise, so `from=-90, to=90` is the right-hand
	 * half of a circle: exactly the profile `revolve()` turns into a sphere (§2's `Ball`).
	 */
	GRAPHCODESHAPE_API bool RunArc(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, FGcShapeOutline& Out, FString& OutError);

	/**
	 * `points((x,y), …, loop=)` and `curve((x,y), …, loop=)` — and the 3D forms.
	 *
	 * **USER-RULED 2026-08-23: `loop=` REPLACES `polygon`.** Group 1 was three cells of four — open
	 * straight, open smooth, closed straight, and a closed SMOOTH outline could not be written at
	 * all, which is a kidney-shaped pond and a racetrack. `loop=` fills the hole and deletes a word
	 * in one move, and it adds no vocabulary because `sweep(loop=true)` already means *join the last
	 * point back to the first*.
	 */
	bool RunPointList(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, FGcShapeOutline& Out, FString& OutError);

	/**
	 * `curve()`'s own smooth spline through `Points`, sampled until no span sags from its chord by
	 * more than `Tolerance`. Shared so a path through saved places
	 * (`save($Lip) [through=(…)]`, `GC-CREATURE-MOUTH-DESIGN.md` §6) bends exactly as `curve()` does.
	 * A closed curve does not repeat its first point.
	 */
	void SampleCurve(const TArray<FVector3d>& Points, bool bClosed, double Tolerance,
		TArray<FVector3d>& Out);

	/**
	 * **THE SAME SPLINE WITH ITS WRITTEN KEYS' MARKS** — `GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.2. Built
	 * exactly as `SampleCurve` builds it, auto tangents first; then a `@sharp` key breaks to its two
	 * chords and a heading key takes `s·|h|·|T|·ĥ` both ways. No other key's tangent changes. `Keys` is
	 * one per written point; `OutKeyPoints[i]` is where written key `i` landed in `Out`.
	 */
	void SampleCurve(const TArray<FVector3d>& Points, bool bClosed, double Tolerance,
		TArrayView<const FGcShapeWrittenKey> Keys, TArray<FVector3d>& Out, TArray<int32>& OutKeyPoints);

	/**
	 * `round(r)` · `round((r, r, …))` on an outline's CORNERS — group 2, ours, an arc per corner. A
	 * corner without room for its radius takes the widest that fits and says so in `Notes`.
	 */
	bool RunRound(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, FGcShapeOutline& Outline, TArray<FString>* Notes,
		FString& OutError);

	/** `widen(by=, about=, offset=, ends=)` — an open path becomes a closed band. */
	bool RunWiden(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, FGcShapeOutline& Outline, FString& OutError);

	/**
	 * `add` · `subtract` · `intersect` OVER TWO OUTLINES — group 5 in 2D, row MH.
	 *
	 * **THE CURRENCY DECIDES WHICH GROUP 5 THIS IS, NOT THE ENTRY'S KIND.** `plane()` bodies are the
	 * obvious case, but the corpus writes the same three verbs in a `mesh()` body BEFORE the depth
	 * instruction — a plate, two windows subtracted, then one `extrude` that cuts the whole thing in
	 * one pass — and that is 2D too. A `mesh()` that has reached triangles keeps the 3D boolean.
	 *
	 * **A RESULT THAT IS SEVERAL DISJOINT PIECES IS REFUSED NAMING THE COUNT.** All three doors
	 * return an array, so it is a real outcome and not a theoretical one; an outline is one boundary
	 * and its holes, so there is no second piece for `to=`'s centre or `round`'s corner walk to mean
	 * anything about. The way out is two entries and a 3D `add`, which has worked since M3.
	 */
	bool RunCombine2D(const FString& Verb, const FString& OtherName, FGcShapeOutline& Subject,
		const FGcShapeOutline& Other, FString& OutError, TArray<FString>* Notes = nullptr, int32 LineNumber = 0);

	/** True for a group-1 verb, which is what may START a body. */
	bool IsOutlineVerb(const FString& Verb);

	/** Import one filled SVG element as the usual outline, then apply shared placement. */
	bool RunSVG(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		FGcShapeOutline& Out, FString& Error);

	/** True for a group-2 verb. `round` is in this list AND in group 4 — depth decides which. */
	bool IsShapingVerb(const FString& Verb);
}
