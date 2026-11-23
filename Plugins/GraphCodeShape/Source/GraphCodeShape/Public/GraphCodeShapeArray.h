// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BoxTypes.h"
#include "GraphCodeShapePlacement.h"
#include "TransformTypes.h"

struct FGcShapeDocument;
struct FGcShapeInstruction;
struct FGcShapeOutline;

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * THE FRAME WHOSE `+Z` IS `Direction`, with `+X` world up projected into it — §4.2's rule.
 *
 * **ONE CONVENTION FOR EVERY ORIENTED THING THIS FORMAT MAKES, and MB6 is what took it out of one
 * file.** `onto=` with `orient=normal` has turned copies this way since 2026-08-24, so a socket
 * declaring a direction inherits it rather than choosing again — two answers to *which way is up
 * once you have said which way is forward* would put a hinge pin and the copies around it in
 * different frames on the same shape ([[two-carriers-of-one-fact-is-a-race]]).
 *
 * `Direction` is expected normalised; a socket's comes through `ReadDirection`, which normalises.
 */
GRAPHCODESHAPE_API UE::Math::TQuat<double> GraphCodeShapeArrayFrame(
	const UE::Math::TVector<double>& Direction);

/**
 * **COPIES.** `GC-SHAPE-GRAMMAR.md` §4.2; `GC-SHAPE-PLACEMENT-DESIGN.md` §5–§6 is the reasoning.
 *
 * **THE OTHER HALF OF `at=`, AND IT IS ONE READER FOR THE SAME REASON.** `FGcShapePlacement` decides
 * where ONE contribution sits; this decides how many there are and where each of the rest goes.
 * Splitting them is not tidiness — a relation is resolved ONCE, against the pre-union accumulator
 * (order rule 3), and every copy then rides off that single answer. An implementation that
 * re-measured per copy would make the array climb, which the design names as the mistake this gets
 * wrong by accident.
 *
 * **THE THREE FAMILIES ARE A GRID, A RING AND A PROJECTION**, and they compose:
 *
 * | written | copies go |
 * |---|---|
 * | `repeat=(10, 4), step=((60,0,0), (0,0,80))` | on a lattice, explicit spacing |
 * | `repeat=(10, 4), spread=(80%, 90%)` | on a lattice that FILLS an extent |
 * | `repeat=24, turn=15.0, around=Z` | round the entry's own axis |
 * | `repeat=24, around=Z, spread=360` | the same ring, with the division done for you |
 * | `repeat=(10,4), spread=(80%,90%), onto=-Y [side]` | wherever a ray cast at those faces lands |
 * | `repeat=12, along=$Rope` | by ARC LENGTH down a path |
 *
 * **`spread=` AND `step=` ARE TWO INTENTS AND BOTH SURVIVE** — widen the target and `step` keeps the
 * spacing while the array stops covering it; `spread` keeps covering and lets the spacing change.
 * Writing both on one line is refused rather than ranked.
 */

/** What a copy's own frame is turned to. `GC-SHAPE-GRAMMAR.md` §4.2. */
enum class EGcShapeOrient : uint8
{
	/** As placed. The only reading for a plain grid or ring, and the DEFAULT there. */
	Keep,

	/** The hit normal. **The default under `onto=`**, because it is the reason to project at all. */
	Normal,

	/** The path direction. **The default under `along=`.** */
	Tangent,
};

/**
 * `collide=` — what a scattered copy does when its SHAPE meets a copy already placed. MB61,
 * user-raised 2026-09-28: *"can our add interact with collision and either avoid collision, or add
 * on top of each other?"*
 */
enum class EGcShapeCollide : uint8
{
	/** Left out: copies may pass through each other, as they always could. */
	None,

	/** Drawn again, from the copy's own stream, until it lands clear of every earlier copy. */
	Avoid,

	/** Lifted along the normal it landed on until it rests on the earlier copy it met. */
	Stack,
};

/** One dimension of a lattice: how many, and how far apart. */
struct GRAPHCODESHAPE_API FGcShapeArrayDimension
{
	/** `repeat=`'s count for this dimension. Never below 1. */
	int32 Count = 1;

	/** `step=`'s displacement, in the ENTRY's frame. Only meaningful where `bHasStep`. */
	UE::Math::TVector<double> Step = UE::Math::TVector<double>::Zero();
	bool bHasStep = false;

	/**
	 * `spread=`'s extent — centimetres, or a fraction of the TARGET's span when `bSpreadIsFraction`.
	 *
	 * **UNDER `around=` THIS DIMENSION'S SPREAD IS AN ANGLE IN DEGREES**, and a percentage there is
	 * refused: a fraction of a span has no reading once the axis is an orbit.
	 */
	double Spread = 0.0;
	bool bSpreadIsFraction = false;
	bool bHasSpread = false;

	/**
	 * `stagger=` — how far each successive index of THIS dimension shifts the array along dimension
	 * one, as a fraction of dimension one's own spacing. Zero on dimension one itself.
	 */
	double Stagger = 0.0;
};

/**
 * `repeat=` `step=` `spread=` `turn=` `around=` `stagger=` `along=` `onto=` `orient=` `jitter=`
 * `seed=`, read once off one instruction.
 *
 * **EVERY REFUSAL THAT IS A PROPERTY OF THE TEXT IS RAISED IN `Read`**, exactly as
 * `FGcShapePlacement` splits its own; the ones that need a shape — a path entry that is not a path,
 * a projection where no ray hit anything — belong to `Resolve`, which is the only thing that can see
 * them.
 */
struct GRAPHCODESHAPE_API FGcShapeArray
{
	/** One to three. Empty where the line makes no copies at all. */
	TArray<FGcShapeArrayDimension> Dimensions;

	/** `turn=` in degrees, applied cumulatively to dimension one. */
	double Turn = 0.0;
	bool bHasTurn = false;

	/**
	 * `around=Z` — the axis the ring orbits, THROUGH THE ENTRY'S OWN ORIGIN.
	 *
	 * **THE ENTRY'S ORIGIN AND NOT THE ACCUMULATOR'S CENTRE, and the choice is stated rather than
	 * incidental.** §4.2's sibling rule already pins the frame — *`step=` is applied in the ENTRY's
	 * frame, never in the accumulated one* — and an orbit read off the accumulator's bounds would
	 * move the flutes when a plinth was added below the shaft.
	 */
	FGcShapeAxis Around;
	bool bHasAround = false;

	/**
	 * `around=self` — each copy spins about its OWN centre instead of orbiting.
	 *
	 * **THE SPIN AXIS IS THE DIRECTION THE ARRAY ADVANCES**, dimension one's own step normalised,
	 * and `Z` where the array has no direction to advance along. A chain rising up Z has its links
	 * turning about Z; the same chain laid along X turns about X, and neither needed a second
	 * argument to say so. Flutes ORBIT a shaft and chain links spin in place — §4.2's two objects.
	 */
	bool bAroundSelf = false;

	/** `along=$Path` — the entry whose 3D path the copies are spaced down, by ARC LENGTH. */
	FString Along;

	/** `onto=` — the direction the grid is cast in. Normalised. */
	UE::Math::TVector<double> Onto = UE::Math::TVector<double>::Zero();
	bool bHasOnto = false;

	/**
	 * `onto=(-Y, gap=3.0)` — how far off the surface each copy sits, in centimetres.
	 *
	 * **SPELLED THE WAY `against($T, +Z, gap=…)` ALREADY SPELLS IT**, because it is the same idea:
	 * `against` is contact with a target and `onto` is contact with a surface. Positive lifts,
	 * negative sinks.
	 */
	double OntoGap = 0.0;

	/**
	 * `scatter=<count>` — N places drawn over the faces the bracket chose, AREA-WEIGHTED.
	 *
	 * **A JITTERED LATTICE IS NOT A SCATTER, AND THAT IS WHY THIS EXISTS.** `repeat=`
	 * builds a grid and `jitter=` nudges each copy INSIDE ITS OWN CELL, so the lattice
	 * always survives: two copies can never meet, no patch is ever bare, and the count is
	 * locked to a product of per-dimension counts. Turned up, it reads as a messy grid
	 * rather than as scatter. Rivets want the grid; gravel, pitting, pips and moss do not.
	 *
	 * **AREA-WEIGHTED, WHICH IS THE WHOLE CORRECTNESS CLAIM.** Drawing triangles uniformly
	 * puts more points on a densely-tessellated patch than its area deserves — and in THIS
	 * format every mesh is unevenly tessellated, because `roughen(scale=)` and `[fade=]`
	 * buy their own triangles locally. A naive draw would clump wherever an earlier line
	 * happened to refine, which is a bug no author could see the cause of.
	 *
	 * **IT NEEDS NO `project=` AND CASTS NO RAY.** A scatter samples the chosen faces
	 * directly — a triangle by area, then a point on it — so it cannot miss, it follows
	 * curvature for free, and it has no direction to be given. `onto=`'s drop-the-missers
	 * rule is therefore NOT inherited here: there is nothing to miss.
	 */
	int32 Scatter = 0;
	bool bHasScatter = false;
	
	/**
	 * `apart=<cm>` — the least distance between two scattered places. Zero means no rule.
	 *
	 * **UNIFORM RANDOM CLUMPS, AND AN AUTHOR FIGHTS IT BY RAISING THE COUNT, WHICH MAKES
	 * IT WORSE.** Pairs land touching and whole patches stay bare, because that is what
	 * uniform sampling looks like. A minimum separation is what makes a scatter read as
	 * deliberate, and it is the one number that cannot be recovered afterwards.
	 */
	double Apart = 0.0;

	/**
	 * `collide=avoid|stack` — the placed SHAPES tested against each other, earlier copies winning.
	 * `apart=` measures between places, and a 6 mm sprinkle reaches 3 mm past its own.
	 */
	EGcShapeCollide Collide = EGcShapeCollide::None;
	
	EGcShapeOrient Orient = EGcShapeOrient::Keep;
	bool bOrientStated = false;

	/** `jitter=(move=, turn=, scale=)`. `turn` in degrees, `scale` as a fraction of 1. */
	UE::Math::TVector<double> JitterMove = UE::Math::TVector<double>::Zero();
	/**
	 * **PER AXIS, IN `rotate=`'S ORDER** — `turn=(0, 0, 180)` spins a copy about its own Z alone,
	 * which under `onto=` or `scatter=` is the surface normal, and a sprinkle lying on a glaze wants
	 * nothing else. One number fills all three, which is what `turn=8.0` always meant.
	 */
	UE::Math::TVector<double> JitterTurn = UE::Math::TVector<double>::Zero();
	double JitterScale = 0.0;
	bool bHasJitter = false;

	/** `seed=`, keyed by the entry's OWN name and by each copy's index — see `Resolve`. */
	int32 Seed = 0;

	/** True where the line asked for anything this type reads. */
	bool bStated() const
	{
		return !Dimensions.IsEmpty() || bHasTurn || bHasAround || !Along.IsEmpty() || bHasOnto
			|| bHasJitter || bOrientStated || bHasScatter;
	}

	/**
	 * True where the copies' positions come from a SURFACE or a PATH rather than from `at=`.
	 *
	 * **THE TWO PATHS THROUGH THE EVALUATOR DIFFER AND THIS IS THE TEST.** A grid or a ring rides
	 * off the one placement `at=` resolved; a projection has no `at=` at all — the surface or the
	 * path decides — so the subject is scaled but never placed, and each copy is seated at its own
	 * hit.
	 */
	// **A SCATTER PROJECTS FOR THE SAME REASON `onto=` DOES**: the positions come from a
	// SURFACE rather than from `at=`, so the subject is scaled and never placed, and each
	// copy is seated where its own sample landed.
	bool bProjects() const { return bHasOnto || bHasScatter || !Along.IsEmpty(); }

	/** How many copies the line asks for. One where no `repeat=` was written. */
	int32 CopyCount() const;

	/**
	 * Read the duplication arguments off one instruction, given the placement already read from it.
	 *
	 * `Placement` is read rather than re-derived because three refusals are about the PAIR: `at=`
	 * with `along=` or `onto=` (the path and the surface decide, so a relation would be a second
	 * answer), and `align=(… target=…)` under `along=` (each copy's target is a POINT and a point
	 * has no span to take a fraction of).
	 */
	static bool Read(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FString& Verb, const FGcShapePlacement& Placement, FGcShapeArray& Out,
		FString& OutError);
};

/** What `Resolve` is allowed to measure. Every field is optional, and a missing one is a refusal. */
struct GRAPHCODESHAPE_API FGcShapeArrayContext
{
	/** The subject as it will be placed — its bounds after `scale=`, `rotate=` and `at=`. */
	UE::Geometry::FAxisAlignedBox3d Subject;

	/** What the relation measured against. `spread=`'s percentage is a fraction of THIS. */
	UE::Geometry::FAxisAlignedBox3d Target;
	bool bHasTarget = false;

	/**
	 * The axes `at=`'s relation fixed, which the spread dimensions may not consume. `against` `on`
	 * `under` fix one, `edge` two and `corner` all three — MB4, and it was a single index until
	 * then, which would have let a spread run straight along an axis `corner()` had already spent.
	 */
	bool AxisFixed[3] = { false, false, false };

	/** The accumulator, for `onto=` to cast at. */
	const UE::Geometry::FDynamicMesh3* Surface = nullptr;

	/** The triangles the `[ ]` bracket chose. Empty and non-null means the whole shape. */
	const TArray<int32>* SurfaceTriangles = nullptr;

	/** `along=$Path`'s resolved outline. */
	const FGcShapeOutline* Path = nullptr;

	/**
	 * `along=` naming a `[border=]` PATH SET — every run, longest first, `Path` the first of them.
	 * Empty for one path. Passes spread over the runs by their total length
	 * (`GC-SHAPE-BLOCKS-AND-EFFECTS.md` §8.4).
	 */
	TArray<const FGcShapeOutline*> Paths;

	/** The copy's own shape, before it is placed — what `collide=` tests one copy against another with. */
	const UE::Geometry::FDynamicMesh3* SubjectMesh = nullptr;

	/**
	 * **EACH COPY'S OWN SHAPE, WHERE THE LINE DRAWS ONE PER COPY — G14 (wood damage, 2026-11-18).** A line
	 * like `add($Piece, length=random(…), scatter=12, collide=stack)` builds twelve different pieces, and
	 * `collide=` used to test every one of them with `SubjectMesh`, the FIRST copy's shape: a 5 cm piece
	 * cleared the others while the 20 cm piece actually placed there ran through them. Set, `collide=`
	 * asks it for copy `i`'s shape (unplaced, as `SubjectMesh` is) in ascending order, the same copy
	 * possibly several times; null means copy `i` built nothing and is placed without a body to test.
	 * Unset, every copy is `SubjectMesh`, as before.
	 */
	TFunction<const UE::Geometry::FDynamicMesh3*(int32 /*Copy*/)> CopyMesh;

	/** **THE ENTRY'S NAME KEYS THE JITTER**, so two entries with `seed=5` are two scatters. */
	FString EntryName;
};

struct FGcShapeProfile;

namespace GraphCodeShapeArray
{
	/**
	 * Where every copy goes, as a transform applied to the subject in the ENTRY's frame.
	 *
	 * For a grid or a ring the subject is already placed, so copy zero's transform is the identity
	 * and the rest are offsets from it. For a projection the subject is only SCALED, and each
	 * transform carries the copy's whole frame — `orient=`'s rotation, then `rotate=` inside it,
	 * then the seat at the hit or the path point.
	 *
	 * **A RAY THAT HITS NOTHING PLACES NOTHING**, so a projected array may return fewer transforms
	 * than `CopyCount()` asks for; `OutLanded` is how many, and the caller reports it. A window in a
	 * door means a gap in the studs, free and correct. **NONE landing is refused** — the whole
	 * instruction did nothing, and a silent no-op is what §7 exists to prevent.
	 *
	 * **THE RANDOM STREAM IS KEYED BY EACH COPY'S OWN INDEX, NOT DRAWN IN SEQUENCE**
	 * ([[a-sequential-random-stream-couples-unrelated-items]]): `repeat=24` → `repeat=25` re-rolls
	 * the twenty-fifth flute and leaves the first twenty-four exactly where they were.
	 *
	 * **`OutDirections` IS OPTIONAL AND IS THE WAY EACH COPY'S FRAME FACES** — added 2026-09-23 for
	 * `loop (…) { … }`, which asks this function the same question every other caller does and then
	 * has no copy to apply the answer to. A loop body reads `$loop.normal` and wants the hit normal
	 * or the path tangent ITSELF, not a rotation that happens to encode it — and under
	 * `orient=keep` the rotation does not encode it at all.
	 *
	 * **IT IS HANDED BACK RATHER THAN RE-DERIVED, FOR `CastGrid`'S OWN REASON**: a second answer to
	 * *which way does this place face* could differ from the first exactly where a copy sits on a
	 * crease ([[a-second-writer-of-one-rule-inherits-none-of-its-fixes]]). Filled in step with
	 * `OutCopies`, so index `i` of one names index `i` of the other. A lattice has no surface and
	 * no path to read, so its direction is the copy's own rotation applied to +Z.
	 */
	GRAPHCODESHAPE_API bool Resolve(const FGcShapeArray& Array, const FGcShapePlacement& Placement,
		const FString& Verb, const FGcShapeArrayContext& Context,
		TArray<UE::Geometry::FTransformSRT3d>& OutCopies, FString& OutError,
		TArray<UE::Math::TVector<double>>* OutDirections = nullptr);

	/**
	 * **THE PROJECTED GRID'S RAYS, AND WHERE THEY LANDED** — the block `Resolve` used to hold
	 * inline, lifted so a second caller can have the ANSWER without the ACTION.
	 *
	 * `Resolve` wants transforms to place copies by; `save($Set) [project=, repeat=]` wants the
	 * places themselves. **Copying the loop would have given the cast a private copy of a rule that
	 * has been fixed three times** ([[a-second-writer-of-one-rule-inherits-none-of-its-fixes]]), so
	 * the grid frame, the span of the chosen faces, `spread=`/`step=`/`stagger=`/`jitter=` and the
	 * miss rule all live here and both callers read them.
	 *
	 * **A RAY THAT HITS NOTHING PLACES NOTHING**, so `OutPoints` may be shorter than
	 * `Array.CopyCount()` — and **NONE landing is refused**, because a line that did nothing while
	 * reporting success is what §7 exists to prevent. Both are the rules `onto=` already shipped;
	 * the cast inherits them rather than restating them.
	 *
	 * Directions are the OUTWARD normal at each hit — flipped where the face pointed back at the
	 * caster, which is what makes a copy sit on the outside of the surface.
	 *
	 * **`CastWord` IS THE WORD THE AUTHOR WROTE**, `onto` or `project`, and it exists only so the
	 * all-miss refusal names a line they would recognise. The direction is the same field either
	 * way; what differs is the spelling that put it there.
	 *
	 * **`OutTriangles` IS OPTIONAL AND IS THE FACE EACH RAY LANDED ON** — added 2026-09-18 for
	 * `save($Name) [project=, repeat=, follows]`, so a grid of places can be anchored to the
	 * surface exactly as a single one is. Every caller that does not ask passes nullptr and nothing
	 * about the grid changes; the array is filled in step with `OutPoints`, so index `i` of one
	 * names index `i` of the other. **A cast has to hand this back rather than have its caller
	 * re-find it**: a second search would be a second answer to *which face*, and the two could
	 * differ at exactly the edges where it matters.
	 */
	GRAPHCODESHAPE_API bool CastGrid(const FGcShapeArray& Array, const FString& Verb,
		const TCHAR* CastWord, const FGcShapeArrayContext& Context,
		const UE::Math::TVector<double>* Centre,
		TArray<UE::Math::TVector<double>>& OutPoints,
		TArray<UE::Math::TVector<double>>& OutDirections, FString& OutError,
		TArray<int32>* OutTriangles = nullptr);

	/**
	 * **N PLACES DRAWN INSIDE A SOLID, 2026-09-14 — `save($Cores) [inside, scatter=24]`.**
	 *
	 * The third way this format makes places, and the first that is not ON a surface. `scatter=`
	 * over a triangle set samples the SKIN, area-weighted; a cast lands where a ray hits. Neither
	 * can put a point in the MIDDLE of a solid, and that is what breaking one into pieces needs —
	 * a fracture is built around points inside the thing being broken.
	 *
	 * **IT LIVES HERE RATHER THAN BESIDE ITS CALLER SO IT SHARES THE STREAM KEY.** Every scatter in
	 * this format draws one stream per copy keyed by that copy's own index, so `scatter=24` ->
	 * `scatter=25` adds a place and leaves the first twenty-four exactly where they were. A second
	 * copy of that key in another file would be a second writer of one rule and would inherit none
	 * of its fixes; `apart=` rejection is here for the same reason.
	 *
	 * **REJECTION SAMPLING AGAINST THE WINDING NUMBER**, which is the only honest way: a solid has
	 * no closed form to draw from. Points are drawn in the bounding box and kept when they are
	 * inside. **That makes the hit rate the shape's own business** — a box is nearly free, a thin
	 * shell or a sparse branching form is mostly empty space — so the attempt budget is finite and
	 * `OutPoints` may come back SHORTER than `Wanted`. That is the cast's own shipped rule (*a ray
	 * that hits nothing places nothing*) and the caller reports the shortfall; **none landing is
	 * refused here**, because a line that did nothing while reporting success is what §7 prevents.
	 *
	 * **NO DIRECTIONS COME BACK, and that is a fact about the geometry rather than a shortcut.** A
	 * point on a surface has a normal; a point in the middle of a solid faces nothing. A caller that
	 * needs a facing must refuse rather than invent one.
	 */
	GRAPHCODESHAPE_API bool ScatterInVolume(const UE::Geometry::FDynamicMesh3& Mesh,
		const int32 Wanted, const double Apart, const int32 Seed, const FString& Verb,
		const FString& EntryName, TArray<UE::Math::TVector<double>>& OutPoints, FString& OutError);

	/** What a graded draw reports beside its places: the distance range and the realised near/far split. */
	struct FGcGradedDraw
	{
		double Nearest = 0.0;   // the solid's least distance from `to=` (0 when `to=` is inside it)
		double Farthest = 0.0;  // its greatest (a vertex: the farthest point of a mesh is one)
		int32 Near = 0;         // places in the nearer half of that range
		int32 Far = 0;          // and in the farther half
	};

	/**
	 * **G7 — PLACES DENSER NEAR AN IMPACT** (`Docs/GC-SHAPE-DAMAGE-KIT.md` §6, Sonnet-reviewed and lead-ruled
	 * 2026-11-19: the graded draw lives HERE, in the shared point-set draw, so `fracture` and `save [inside,
	 * scatter=]` both have it). `Apart` is a profile of the least spacing in cm over `t`, where `t` is 1 at `Target`
	 * and 0 at the farthest point of the solid from it, with no rescale (so a target off the solid never reaches
	 * the profile's `t = 1` end). A place at distance `d` is drawn with density proportional to
	 * `1 / apart(t(d))^3` — shell sampling about `Target` from a CDF tabulated once from the solid and the profile
	 * alone, never from `Wanted`, so one more place moves none — then kept only if it is inside and no earlier
	 * place is nearer than the SMALLER of the two places' own spacing (symmetric). Each place draws from its own
	 * stream, as the uniform draw does. A shortfall drops and is the caller's to report; none landing is refused.
	 * The uniform draw above is untouched: a constant `apart=` never reaches this function.
	 */
	GRAPHCODESHAPE_API bool ScatterGradedInVolume(const UE::Geometry::FDynamicMesh3& Mesh,
		const int32 Wanted, const UE::Math::TVector<double>& Target, const FGcShapeProfile& Apart,
		const int32 Seed, const FString& Verb, const FString& EntryName,
		TArray<UE::Math::TVector<double>>& OutPoints, FGcGradedDraw& OutDraw, FString& OutError);

	/**
	 * The smallest spacing a graded `apart=` may ask for: `max(0.001 x the shape's largest extent, 0.01 cm)`. A
	 * smaller one asks for a density no 500-piece budget or per-place draw could honour (12 to 1.5 is already a
	 * 512-fold ratio), so it is refused with this number rather than drawn as a thinner field.
	 */
	GRAPHCODESHAPE_API double GradedApartFloor(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * **THE OTHER ARRANGEMENT: A LATTICE INSIDE THE SOLID** — `[inside, repeat=(4, 3, 2)]`, and
	 * `fracture(repeat=)`'s own route.
	 *
	 * **WHICH ARRANGEMENT THE POINTS ARE IN IS THE WHOLE DIFFERENCE BETWEEN ROCK AND BLOCKS**, which
	 * is the finding this exists for: a Voronoi cell around a RANDOM point is an irregular chunk,
	 * and a cell around a point on a CUBIC LATTICE is exactly a cube. So `ScatterInVolume` above is
	 * rubble and this is masonry, out of one verb and no new vocabulary.
	 *
	 * **IT WALKS THE BOUNDING BOX AND KEEPS WHAT IS INSIDE**, which is the same winding test the
	 * draw makes and the same refusals — an open shell has no inside, and none landing refuses.
	 * `OutCells` is how many it walked, so the caller can report what it dropped.
	 *
	 * **`Repeat` IS x, y AND z OF THE BOX, WHICH IS NOT HOW AN ARRAY READS THE SAME WORD.** An
	 * array's `repeat=(4, 3)` is two dimensions whose DIRECTIONS come from `step=`; a box already
	 * has its axes, so there is nothing left for a direction to decide.
	 *
	 * **`Jitter` IS A FRACTION OF THE CELL AND NEVER A LENGTH.** How big a cell is, is derived from
	 * `Repeat` and the shape, so centimetres would be a number the author cannot know; a fraction is
	 * the only one they can choose. It is tested AFTER the nudge, because a nudge near a face is
	 * exactly what carries a point out of the solid. ONE FRACTION PER AXIS (2026-11-20): `jitter=30%` is the
	 * same fraction on all three; `jitter=(30%, 0, 10%)` scales each axis's nudge by its own.
	 */
	GRAPHCODESHAPE_API bool LatticeInVolume(const UE::Geometry::FDynamicMesh3& Mesh,
		const FIntVector& Repeat, const UE::Math::TVector<double>& Jitter, const int32 Seed, const FString& Verb,
		const FString& EntryName, TArray<UE::Math::TVector<double>>& OutPoints, int32& OutCells,
		FString& OutError);
}
