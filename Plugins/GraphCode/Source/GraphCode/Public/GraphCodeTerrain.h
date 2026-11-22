// Copyright OraStudio. All Rights Reserved.
//
// The `terrain { }` instruction program — `GC-TERRAIN-DESIGN.md` §3–§6, §9.
//
// **A LINE IS AN INSTRUCTION, NOT A LAYER** (user-ruled). Every one is the same shape,
// `Name = operation(args) [filter]`, they run top to bottom over an accumulating heightfield, and
// edit layers are PACKAGING applied at realisation rather than the authoring unit. The author writes
// coarse rules with a wide domain at the top and fine ones with a tight domain below, and the order
// executes: `erode` carves whatever the lines above it built.
//
// TWO KINDS OF OPERATION EXIST AND THE AUTHOR NEVER HAS TO KNOW: a CONTRIBUTION adds to what is
// there, a TRANSFORM reads the whole accumulated field and rewrites it. The compiler knows which is
// which; both are just legal lines.
//
// **DETERMINISM IS THE CONTRACT.** Every generator takes an explicit `seed=`, refused and never
// defaulted, and `gen:` on the block header pins OUR implementations so improving an algorithm is a
// recorded version bump rather than a silent change to every terrain ever authored. Same document
// twice, byte-identical field.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeTerrainDelta.h"

struct FGcCourseCurve;

/**
 * One node of a `[all(slope=(Min=45), not(near=River))]` filter.
 *
 * A FLAT ARRAY WITH INDEX CHILDREN rather than a nested struct: the tree is small, it has to be
 * copied by value with the document, and a self-referencing `TArray` member is the one shape that
 * makes that fragile for no gain.
 *
 * **THE COMPOSITES ARE `all`/`any`/`not` BECAUSE THIS PROJECT ALREADY SHIPS THEM** — the Behavior
 * Tree writer spells exactly these — so inventing `&&`/`||` beside them would be
 * [[one-concept-two-grammars]]. Prefix composites also delete operator precedence: `any(a, all(b, c))`
 * is unambiguous and nests forever.
 */
struct GRAPHCODE_API FGcTerrainFilterNode
{
	enum class EKind : uint8
	{
		All,
		Any,
		Not,
		/** `height=(Min=2000, Max=8000)` — the accumulated field's own Z, in world units. */
		Height,
		/** `slope=(Min=45)` — degrees from horizontal, measured on the accumulated field. */
		Slope,
		/** `inside=$Valley` — a region of this same `.gcw`. */
		Inside,
		/** `near=$River  within=800` — within a distance of a region's outline. */
		Near,
	};

	EKind Kind = EKind::All;

	/** Composite children, as indices into the owning filter's node array. */
	TArray<int32> Children;

	bool bHasMin = false;
	double Min = 0.0;
	bool bHasMax = false;
	double Max = 0.0;

	/**
	 * `inside=`/`near=` — the region's name, held BARE and written `$Marked`.
	 *
	 * The mark is grammar, not identity: §18.3c's rule is that an unmarked name is an ACTOR as far as
	 * a `.gcw` is concerned, and `ResolveName` answers a bare one with not-declared rather than a
	 * refusal — so the reader requires the mark and the renderer puts it back, while everything
	 * between them compares plain names.
	 */
	FString Region;

	/** `near=`'s distance in centimetres. Required on `near=`, because *near* is not a measurement. */
	double Within = 0.0;
};

/** A whole filter. Empty means the instruction applies everywhere its domain reaches. */
struct GRAPHCODE_API FGcTerrainFilter
{
	/** `Nodes[0]` is the root. */
	TArray<FGcTerrainFilterNode> Nodes;

	bool IsEmpty() const { return Nodes.IsEmpty(); }
};

/**
 * WHERE an instruction applies — the spatial language, reused rather than reinvented.
 *
 * `GC-TERRAIN-DESIGN.md` §3.5's lesson is that *"in the valley"* is almost never the whole valley:
 * it is a point offset from a corner, or a fraction along an edge, authored one at a time exactly as
 * an actor is placed. So the three forms below are ordered by how often they are the right one, and
 * the whole-region form is the one to use sparingly.
 */
/**
 * WHERE A LINE WAS AUTHORED, when that is not where it runs — §14.11's provenance, and the whole
 * reason the feature costs no consumer a single edit.
 *
 * **A REGION-SCOPED BLOCK IS FLATTENED INTO THE MAIN STREAM AT PARSE TIME.** `$Forest.Greens` in
 * `populate { }` splices that block's lines in at exactly that position, so by the time any consumer
 * sees the document there is ONE ordered list and it is the effective one. Measured before choosing:
 * **eight non-test files read `Populate.Entries` or `Terrain` directly**, and a resolver they each
 * had to remember to call is the shape that ships working in seven places and silently inert in the
 * eighth ([[a-shared-reader-with-private-writers]], written the same day one floor down).
 *
 * **SO THIS MARK IS WHAT THE WRITER PUTS THE FILE BACK WITH, and it is the ONE carrier.** The region
 * body does not keep a second copy of its own lines — it renders them by walking the main stream for
 * the ones marked as its own, and the main block renders `$Forest.Greens` where that run begins and
 * skips the rest ([[two-carriers-of-one-fact-is-a-race]]).
 */
struct GRAPHCODE_API FGcwScopeMark
{
	/** The region whose body declared this line. Empty for one written in the main block. */
	FString Region;

	/** The named block inside it — `Greens` in `Greens = populate { … }`. */
	FString Block;

	/**
	 * The domain came from the SCOPE and was never written on the line.
	 *
	 * **USER-RULED 2026-08-24:** *"inside region populate, you declare everything you need but
	 * doesn't need to refer to the own region anymore as it's auto referenced."* So a scoped line
	 * stating no place means `in=$<its region>`, and this bit is what stops the writer from putting
	 * an `in=` back that the author never typed — which would be the file growing a key every time
	 * it was read ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 */
	bool bDomainImplicit = false;

	/**
	 * THIS LINE IS THE `$Forest.Greens` REFERENCE ITSELF, NOT A LINE SPLICED FROM ONE.
	 *
	 * **TRUE ONLY BETWEEN THE BLOCK PARSER AND `GcwScoped::Expand`, WHICH RUNS IN THE SAME CALL.**
	 * A reference has to hold its POSITION in the stream while the file is still being read — the
	 * region that supplies it may be declared below the block that names it — and an entry is the
	 * only thing this stream holds positions of. `Expand` replaces every one of them before any
	 * consumer sees the document, so a placeholder reaching a reader is a defect and not a state.
	 *
	 * **AND IT MUST NOT SURVIVE, for a measured reason rather than a tidy one**: `FGcPopulateEntry`
	 * defaults its verb to `Pcg`, so a placeholder left in the stream does not read as *nothing* to
	 * the eight files that walk it — it reads as a PCG generator somebody asked for.
	 */
	bool bPlaceholder = false;
	bool IsScoped() const { return !Region.IsEmpty(); }

	/** Same region AND same block — what makes one contiguous run render as one `{ }`. */
	bool SameBlockAs(const FGcwScopeMark& Other) const
	{
		return Region == Other.Region && Block == Other.Block;
	}

	/** `Forest.Greens`, for a refusal to name. Never a spelling the file parses without the `$`. */
	FString Qualified() const { return Region + TEXT(".") + Block; }
};

struct GRAPHCODE_API FGcTerrainDomain
{
	enum class EKind : uint8
	{
		/** No domain stated — the whole world. Correct for exactly one line, the coarse ground. */
		World,
		/** `in=$Valley` — everywhere inside a region's outline. */
		Region,
		/** `at=(8200, 5400)` or `at=$Valley.p3 right=400 forward=300`, with `radius=`. */
		Point,
		/** `along=((a) -> (b) -> (c))` or `along=$StreamPath`, with `width=` — a course. */
		Path,
	};

	EKind Kind = EKind::World;

	/**
	 * The region named by `in=` — and by `along=$StreamPath`, which is a COURSE that is named rather
	 * than written out (§3.5). So `Kind == Path` with this set is a run along a region, and `Kind ==
	 * Path` with `Path` filled is the same run written as literal points.
	 */
	FString Region;

	/** `at=(x, y)` when `Anchor` is empty; otherwise the resolved anchor lands here. */
	FVector2D Point = FVector2D::ZeroVector;

	/**
	 * `at=$Valley.p3` or `at=$Valley.east@t=40%` — a corner, a landmark, or a fraction along an edge.
	 *
	 * HELD WHOLE AND STILL MARKED, unlike `Region` above: this is a reference EXPRESSION handed to the
	 * solver verbatim, not a name to look up, and taking it apart here would duplicate the one piece
	 * of parsing `@anchor[target=]` already owns.
	 *
	 * Resolved through the same solver `@anchor[target=]` already uses, so a terrain feature and an
	 * actor find the same spot from the same words ([[a-read-answers-through-the-write-path]]).
	 */
	FString Anchor;

	/**
	 * `right=` / `forward=` / `up=` — HOW FAR OFF THE PLACE THIS ONE SITS, in centimetres.
	 *
	 * **THE PLACE AND THE OFFSET ARE ONE STATEMENT, which is why they live on the domain and not on
	 * the operation.** `at=$Valley.p3 right=400` and `along=$Track.spine right=400` are the same
	 * sentence about two different kinds of place, so one reader takes both and one writer puts both
	 * back — a second copy per block is how `.gcw` and `.gc` start spelling a distance two ways
	 * ([[one-concept-two-grammars]]).
	 *
	 * **THE FRAME IS THE PLACE'S OWN AND NEVER THE WORLD'S, and that rule predates the course by
	 * months** (§20). An anchor rotates them through its own transform, so a pad off a wall walks
	 * into the room whichever way that wall faces; a course rotates them through its CHORD — see
	 * `OffsetCourse`. Adding these to a world location directly would put a fence north of a track
	 * whatever way the track runs, and the file would read correct while the ground was wrong.
	 *
	 * **`up=` IS THE ONE THAT IS NEW (2026-08-24), AND THE OTHER TWO ARE OLDER THAN THEIR MEANING.**
	 * `right=`/`forward=` have parsed on every operation since §20 and were applied by the ANCHOR
	 * path alone: a course consumer read `Target.Course` and never looked at them, so
	 * `along=$Track.spine right=400` parsed, round-tripped and did nothing, in silence. That is the
	 * defect this trio closes, not a feature it adds beside one
	 * ([[a-silent-no-op-is-worse-than-a-refusal]]) — and where an offset still cannot mean anything,
	 * `ValidateOffsets` now refuses it by name instead of dropping it.
	 */
	double Right = 0.0;
	double Forward = 0.0;
	double Up = 0.0;

	/** `offset=` follows the evaluated course in XY: positive right, negative left (cm). */
	double CurveOffset = 0.0;
	bool bHasCurveOffset = false;

	/**
	 * `along=$Valley.east` — A WALL AS THE COURSE, held whole and still marked, exactly as `Anchor` is.
	 *
	 * The third way to say a course, and the one §9's own worked example wanted: a ridge that follows
	 * the curve of a valley wall rather than a straight line somebody typed near it. It costs no
	 * grammar — `$X.y` is a reference form the file already has — and no geometry, because
	 * `FGraphCodeRegionTargets::ResolveOne` now answers a wall with its own run, ARC INCLUDED and in
	 * the same tessellation containment uses.
	 *
	 * **AN EDGE HAS NO INTERIOR, so this form REQUIRES `width=`**, where `along=$Region` refuses one.
	 * A region says how wide it runs by being a shape; a line is a line.
	 */
	FString Course;

	/** `along=` — world-space points in order. */
	TArray<FVector2D> Path;

	bool bHasRadius = false;
	double Radius = 0.0;

	bool bHasWidth = false;
	double Width = 0.0;

	/** True when a translation or curve offset changes the geometry. */
	bool HasOffset() const { return Right != 0.0 || Forward != 0.0 || Up != 0.0 || CurveOffset != 0.0; }

	/**
	 * THE OFFSET IN A RESOLVED FRAME'S OWN AXES — what `at=$Valley.p3 right=400 forward=300` means.
	 *
	 * §20's rule, lifted out of the two call sites that had copied it (terrain's `AddAnchor` and
	 * populate's `ResolveScatterShape`). They agreed, which is exactly why this could be lifted
	 * without deciding anything — and why leaving them apart was a bet that they would go on agreeing
	 * the day one of them learned a third axis. `up=` is that day.
	 */
	FVector OffsetInFrame(const FTransform& Frame) const
	{
		return Frame.GetRotation().RotateVector(FVector(Forward, Right, Up));
	}

	/**
	 * MOVE A WHOLE COURSE OFF ITSELF — `along=$Track.spine right=400` is the fence beside the track.
	 *
	 * **THE FRAME IS THE CHORD: one straight line from the course's first point to its last, and the
	 * offset is a RIGID TRANSLATION of every point by the same vector** (user-ruled 2026-08-24 —
	 * *"it measures a straight line from start to finish of the spline, then we could apply
	 * front/back or left/right(relative) or up/down of it"*). Two fences 400 either side of a track
	 * are `right=400` and `right=-400` on the same course, and they are exactly parallel to each
	 * other and to the run because all three are the same points plus a constant.
	 *
	 * **THE CHORD IS MEASURED IN PLAN, NOT IN 3D, and that is a choice with a consequence.** Forward
	 * is the chord flattened to XY and `up=` is world +Z, so `right=` is always horizontal and `up=`
	 * is always vertical however steeply the run climbs. A 3D chord would tilt the lateral axis on a
	 * climbing course and a fence beside a hill road would lean — a fence does not lean because the
	 * road goes up.
	 *
	 * **AND HERE IS WHAT IT IS NOT: this is not a parallel curve.** On a course that bends, a rigid
	 * translation keeps the offset constant in WORLD space, so the sideways distance from the run
	 * varies through the bend and a hard enough turn puts the offset course across the original. That
	 * is the stated cost of the frame the user asked for, and the alternative — offsetting each point
	 * along its own local normal — is `offset=` (user-ruled 2026-10-07). It follows the evaluated
	 * curve, including closed runs, and refuses folds/collapse or self-intersections. Native curve
	 * metadata, when supplied, is replaced with linear samples of that derived curve. `right=` and
	 * `forward=` still translate the whole result in the ORIGINAL chord frame; `up=` is world Z.
	 *
	 * `Subject` names the line for the refusal. False leaves the run untouched.
	 *
	 * **THE RUN IS NAMED `InOutRun` AND NOT `Course` BECAUSE THIS STRUCT HAS A `Course` MEMBER**
	 * — that one is the NAME of the place, this is the resolved geometry. `/W4` calls the clash
	 * shadowing and refuses the build, which is the compiler catching a real reader's ambiguity
	 * rather than enforcing a style rule.
	 */
	bool OffsetCourse(TArray<FVector>& InOutRun, const FString& Subject, FString& OutRefusal,
		TArray<FGcCourseCurve>* InOutCurves = nullptr) const;

	/**
	 * IS AN OFFSET SAYABLE ABOUT THIS PLACE AT ALL? Called once per line, after every key is read.
	 *
	 * **AFTER, BECAUSE THE KIND IS NOT KNOWN UNTIL THEN.** Keys arrive in the author's order, so
	 * `right=400 in=$Grove` reaches `right` before it knows the place is a region — a reader that
	 * judged as it went would refuse a line whose only crime was word order.
	 *
	 * A POINT has a frame and a COURSE has a chord; a REGION and the whole WORLD have neither, and an
	 * offset written there used to be dropped without a word. Refused by name now: moving a region is
	 * a different act from placing something beside one, and nothing here can honestly guess which
	 * was meant.
	 */
	bool ValidateOffsets(const FString& Subject, FString& OutRefusal) const;

	/**
	 * A PLACE NAME PLUS THIS OFFSET — the key a cache of resolved places must use.
	 *
	 * **AND IT IS A DEFECT FIX, NOT A NEW REQUIREMENT.** `FGcTerrainEval` has cached resolved anchors
	 * and courses under the bare name since §20 and its offsets were already part of the stored
	 * value, so `ridge(at=$Valley.p3, right=400)` and `raise(at=$Valley.p3, right=-400)` in one
	 * program shared the FIRST line's point and the second silently drew in the wrong place. Nothing
	 * caught it because nothing in the corpus had ever written two offsets off one corner.
	 *
	 * Byte-identical to `Place` when no offset is stated, which is every committed `.gcw` — so the
	 * key only splits where sharing was wrong.
	 */
	FString PlaceKey(const FString& Place) const;
};

/**
 * `min=`/`max=`/`follows=` — THE TWO Z BOUNDS A SOLID NEEDS, `GC-TERRAIN-DESIGN.md` §12a.6.
 *
 * **A SURFACE TAKES ONE HEIGHT, A SOLID TAKES TWO** (user-ruled 2026-08-20), and that is the whole
 * reason this struct exists beside `FGcTerrainDomain` rather than inside it. The place words above
 * were sized for a consumer that needs one Z per XY — a `ridge`, a `carve`, a `raise` all produce a
 * heightfield, so `at=$Peak radius=1800` is complete for them. `populate { }` borrowed the vocabulary
 * whole and its consumer builds a VOLUME, which needs two. The words were one number short by
 * inheritance, from the day they were reused; a region merely happens to have been given a second
 * number for its own reasons. **Terrain never fills one of these**, which is why it rides beside the
 * domain instead of joining it.
 *
 * **`min=` AND `max=` ARE ALWAYS OFFSETS FROM A DATUM, and `follows=` names the datum.** `absolute`
 * is the datum that is zero everywhere, `region` is a step function, `ground` is the terrain surface
 * — so a fourth datum is a new VALUE and not new grammar ([[a-new-token-not-a-new-line]]).
 *
 * **THE CHANGE IS ADDITIVE.** Today's `in=$Valley` — take the region's own band — is exactly
 * `follows=region, min=0, max=0`, so the new keys CONTAIN the shipped behaviour instead of replacing
 * it: no corpus migrates and no spelling is refused by name. An entry that states none of the three
 * keys is what every committed `.gcw` already holds.
 */
struct GRAPHCODE_API FGcVolumeBand
{
	/** What zero means. `absolute` is the default and is never written. */
	enum class EDatum : uint8
	{
		/** World Z. The datum that is zero everywhere. */
		Absolute,
		/** The band of the region this line names — a step function across a course that crosses rooms. */
		Region,
		/** The terrain surface under each of the shape's own points. */
		Ground,
	};

	/**
	 * The floor and the ceiling, each as STOPS KEYED BY ARC-LENGTH FRACTION down a course.
	 *
	 * **A SCALAR IS THE DEGENERATE PROFILE**, which is L7's own rule (*any scalar, not a blessed
	 * list*), so `min=0` is one stop and `min=(0=1150, 0.5=880, 1=840)` is three. One stop is a
	 * constant whatever fraction it sits at, so the renderer writes it back as a scalar — §20.6's one
	 * spelling per shape, the same canonicalisation `trigger=load` already gets.
	 */
	TArray<TPair<double, double>> Min;
	TArray<TPair<double, double>> Max;

	bool bHasMin = false;
	bool bHasMax = false;

	EDatum MinDatum = EDatum::Absolute;
	EDatum MaxDatum = EDatum::Absolute;

	/** `follows=` was stated at all — so a stated `absolute` can be canonicalised away knowingly. */
	bool bHasFollows = false;

	/**
	 * `follows=(min=ground, max=absolute)` stated the CEILING'S OWN datum.
	 *
	 * **AN ABSENT CEILING DATUM INHERITS THE FLOOR'S**, user-ruled in one sentence, and the direction
	 * is floor → ceiling and never the reverse: a solid rests on the ground and a ceiling is measured
	 * from wherever the author put the floor. So `follows=(min=ground)` puts BOTH on the ground and
	 * `follows=ground` is its shorthand.
	 */
	bool bMaxDatumStated = false;

	bool IsEmpty() const { return !bHasMin && !bHasMax && !bHasFollows; }

	/** True where the bound varies down a course — which only a course can carry. */
	bool MinIsProfile() const { return Min.Num() > 1; }
	bool MaxIsProfile() const { return Max.Num() > 1; }

	/**
	 * The two world-Z numbers a band comes to over a REGION, which is the domain that realises today.
	 *
	 * **A BAND-LESS ENTRY ANSWERS THE REGION'S OWN `min:`/`max:` UNCHANGED**, so every document
	 * written before this row resolves to exactly the prism it always did.
	 *
	 * **`GroundZ` IS THE CALLER'S MEASUREMENT AND THE ARITHMETIC STAYS HERE.** Sampling terrain needs
	 * a landscape and a trace, which are World's; what a datum MEANS is Core's. Pass the world Z of
	 * the ground under the point being resolved and `follows=ground` becomes an offset from it;
	 * pass nothing and it refuses exactly as it always did, which is what every Core caller with no
	 * world still gets ([[a-read-answers-through-the-write-path]] — one reader, not two).
	 */
	bool ResolveOverRegion(double RegionMin, double RegionMax, const FString& What, double& OutMin,
		double& OutMax, FString& OutError, const double* GroundZ = nullptr) const;

	/**
	 * The two world-Z numbers this band comes to at ONE FRACTION along a course — `ResolveOverRegion`
	 * for the domain that has no region, called once per COLUMN of a strip's solid.
	 *
	 * **ONE READER, CALLED TWICE, IS THE WHOLE OF A PROFILE'S GEOMETRY.** `min=(0=100, 0.5=900,
	 * 1=100)` down a course becomes a lid that slopes because this is asked at each ring point's own
	 * fraction; a scalar is the degenerate profile and answers the same number every time, which is
	 * how a flat band and a shaped one stay one code path.
	 *
	 * **`region` IS REFUSED HERE RATHER THAN APPROXIMATED**, and it is not the same refusal
	 * `ResolveOverRegion` makes: a disc at a landmark and a run written out as points name no region
	 * at all, so *this region's own top* has nothing to measure from. `ground` is refused only when
	 * the caller supplies no `GroundZ` — see `ResolveOverRegion` for why that number is the caller's
	 * and the addition is ours.
	 *
	 * The floor-under-ceiling check the grammar already made at every merged stop is not repeated
	 * where both bounds share a datum: it is exact under linear interpolation, so a column between
	 * two stops cannot invert where neither stop does. **It IS made when the two datums differ**,
	 * because the reader compared OFFSETS and two datums are two different zeroes — a floor on the
	 * ground and a ceiling in world Z can cross where neither offset pair looked wrong.
	 */
	bool ResolveAlongCourse(double T, const FString& What, double& OutMin, double& OutMax,
		FString& OutError, const double* GroundZ = nullptr) const;
};

/**
 * One line of the program: `Name = operation(args) [filter]`.
 *
 * `Args` keeps every scalar the operation reads, BY NAME AND IN AUTHORED ORDER, so the renderer puts
 * back exactly what it read. The domain and the filter are lifted out because they are shared by
 * every operation and are the two things the evaluator asks about before it asks what to do.
 */
struct GRAPHCODE_API FGcTerrainInstruction
{
	FString Name;

	/**
	 * `perlin`, `ridged`, `billow`, `raise`, `ridge`, `carve`, `flatten`, `hole`, `roughen`,
	 * `erode`, `smooth`, `terrace`, `sculpt`, `stamp`.
	 *
	 * **NEVER `cone` OR `dome` — THE PARSER SPENDS THOSE BEFORE ANYTHING SEES THEM** (board row W19).
	 * Both still read and both become `raise`, a `cone` carrying the `slope=(0.0=0.0, 1.0=1.0)` that
	 * keeps its flank straight, so a consumer never has to know the pair existed.
	 */
	FString Operation;

	/**
	 * External height samples for `sculpt` and `stamp`. Sculpts declare exact grid dimensions
	 * and may carry a base digest; stamps read dimensions from the PNG. Empty Path means no file.
	 */
	FGcTerrainDeltaSource Source;

	/** True for a recorded sculpt or a placed texture stamp. */
	bool HasSource() const { return !Source.Path.IsEmpty(); }

	/** Add/subtract use signed offsets; the other modes use absolute world heights. */
	enum class EStampBlend : uint8 { Add, Subtract, Replace, Min, Max, Average };
	EStampBlend StampBlend = EStampBlend::Add;
	const TCHAR* StampBlendName() const;

	/** Scalars, in authored order — `amplitude`, `height`, `depth`, `seed`, `iterations`, … */
	TArray<TPair<FString, double>> Args;

	/**
	 * `depth=(0=0, 0.2=100, 0.6=-250)` — a scalar that VARIES ALONG THE COURSE instead of being one
	 * number for the whole run. The key is a fraction of the way along, 0 at the head and 1 at the
	 * mouth; between two stops the value runs straight, and past the last it holds.
	 *
	 * **ANY SCALAR, NOT A LIST OF BLESSED ONES.** A river deepens downstream (`depth`), spreads out
	 * (`width`) and trades a gorge for a flood plain (`falloff`, which IS the bank's slope once the
	 * depth is fixed) — and there is no principled line between those three and the rest, so the
	 * rule is that a number can always be a profile. The operation table still says which keys the
	 * operation HAS; this only says how one of them is spelled.
	 *
	 * **STOPS ARE ABSOLUTE, NEVER DELTAS.** A delta needs a base, and the base would be a second
	 * number for the same quantity sitting on the same line — two carriers of one fact
	 * ([[two-carriers-of-one-fact-is-a-race]]). `depth=(0=0, …)` says the channel starts at nothing,
	 * which is what a stream rising out of flat ground actually does.
	 *
	 * **WHAT `t` MEANS IS THE ARGUMENT'S, NOT THIS CONTAINER'S — `GC-TERRAIN-DESIGN.md` §14.2.**
	 * Every key above runs ALONG the course, head to mouth. `slope=` does not: it runs ACROSS the
	 * domain, 0 at the border and 1 at the crest, and it is the FLANK — what fraction of the line's
	 * height a point that far in receives. They are two arguments and not one because neither can do
	 * the other's job: `height=(0=900, 0.5=5600, 1=1200)` is a ridge that rises to a summit and
	 * drops to a saddle, and `slope=(0=0, 0.35=0.22, 1=1)` is the concave talus every point of it
	 * keeps. Together they are a mountain; the same rule `.gcshape` settled at its §3.
	 */
	TArray<TPair<FString, TArray<TPair<double, double>>>> Profiles;

	/** `noise=(seed=, amplitude=, frequency=)` — the per-feature roughening a stamp may carry. */
	bool bHasNoise = false;
	int32 NoiseSeed = 0;
	double NoiseAmplitude = 0.0;
	double NoiseFrequency = 0.0;

	/**
	 * `warp=(seed=, amplitude=, frequency=)` — the sample is MOVED before this line measures it.
	 *
	 * **THE KEY THAT STOPS EVERYTHING BEING A CIRCLE.** A point domain weighs a sample by its
	 * distance from the anchor, so every stamp in this language has a perfectly round footprint no
	 * matter how much `noise=` roughens its surface: the flanks get rough and the OUTLINE stays a
	 * disc. Displacing the position first makes the outline wander instead, which is the difference
	 * between a cone with texture on it and a hill. (The SHAPE, not the retired operation name.)
	 *
	 * `amplitude` is CENTIMETRES — how far the ground is pulled sideways — because every other
	 * amplitude in this language is a length and one key meaning two units is the trap this file
	 * keeps paying for ([[one-concept-two-grammars]]).
	 *
	 * ONE KEY AT BOTH SCALES, which is what makes it worth having: on the coarse `perlin` line it
	 * warps the whole world's noise into something that does not read as a grid, and on a single
	 * `raise` it warps that one mountain's footprint. The author chooses per line, so a town pad that
	 * must stay a clean disc simply does not state one.
	 *
	 * It moves the DOMAIN and the NOISE, never the field lookups a filter reads: `slope=` and
	 * `height=` still describe the sample they are asked about, or a filter would answer for
	 * somewhere else.
	 */
	bool bHasWarp = false;
	int32 WarpSeed = 0;
	double WarpAmplitude = 0.0;
	double WarpFrequency = 0.0;

	FGcTerrainDomain Domain;
	FGcTerrainFilter Filter;


	/**
	 * The author's own grouping — full lines immediately above this one, `#` comments and BLANKS,
	 * in order and trimmed. `Trailing` is the `# …` after it, hash and leading space removed.
	 *
	 * **ON THE INSTRUCTION AND NOT IN THE DOCUMENT'S TRIVIA MAP**, which is where the rest of the
	 * `.gcw` keeps this. The map is keyed by name, and that is right for a point or a wall, which
	 * are looked up. A terrain program is an ORDERED STREAM THE AI REWRITES: it moves lines, and it
	 * renames them. Keyed by name, a rename silently orphans the heading that explains the line;
	 * held here, the comment travels with the line through both.
	 *
	 * The blank line is grouping, not whitespace — it is what separates one pass from the next
	 * (§9's passes), so it is kept as an empty entry rather than tidied away.
	 */
	TArray<FString> Leading;
	FString Trailing;

	/**
	 * WHICH REGION'S BODY DECLARED THIS INSTRUCTION, or empty for one written in the program itself.
	 *
	 * **THE MAIN PROGRAM IS THE MACRO PASS AND A REGION BLOCK IS THE FINE TUNING** (user, 2026-08-24:
	 * *"you describe macro in main populate or terrain first, fine tune in regions"*), and the
	 * reference's POSITION is where the fine tuning runs. That is what let terrain into this at all:
	 * §9.8 kept terrain out of region bodies because its coarse-to-fine order IS its semantics, and an
	 * instruction scattered into a region body lost that order. Spliced at the reference, the order is
	 * still one visible sequence in one place — the objection was to losing the order, not to where
	 * the text sits.
	 */
	FGcwScopeMark Scope;

	/** By key, or `Fallback`. */
	double Arg(const TCHAR* Key, double Fallback = 0.0) const
	{
		for (const TPair<FString, double>& Pair : Args)
		{
			if (Pair.Key == Key)
			{
				return Pair.Value;
			}
		}
		return Fallback;
	}

	bool HasArg(const TCHAR* Key) const
	{
		return Args.ContainsByPredicate(
			[Key](const TPair<FString, double>& Pair) { return Pair.Key == Key; });
	}

	bool HasProfile(const TCHAR* Key) const
	{
		return Profiles.ContainsByPredicate(
			[Key](const TPair<FString, TArray<TPair<double, double>>>& Pair)
			{ return Pair.Key == Key; });
	}

	/**
	 * The value of `Key` a fraction `Along` down the course — the profile if it has one, else the
	 * plain scalar, so every reader asks the same question and neither has to know which was written.
	 *
	 * Held flat and CLAMPED at both ends rather than extrapolated: a course is measured from its own
	 * first and last point, so there is nothing outside it to extend into, and a run-off would make
	 * the head of a river deepen upward out of the ground.
	 */
	double ValueAt(const TCHAR* Key, const double Along, const double Fallback = 0.0) const
	{
		for (const TPair<FString, TArray<TPair<double, double>>>& Profile : Profiles)
		{
			if (Profile.Key != Key || Profile.Value.IsEmpty())
			{
				continue;
			}
			const TArray<TPair<double, double>>& Stops = Profile.Value;
			if (Along <= Stops[0].Key)
			{
				return Stops[0].Value;
			}
			for (int32 Index = 0; Index + 1 < Stops.Num(); ++Index)
			{
				if (Along <= Stops[Index + 1].Key)
				{
					const double Span = Stops[Index + 1].Key - Stops[Index].Key;
					const double T = Span > 0.0 ? (Along - Stops[Index].Key) / Span : 0.0;
					return FMath::Lerp(Stops[Index].Value, Stops[Index + 1].Value, T);
				}
			}
			return Stops.Last().Value;
		}
		return Arg(Key, Fallback);
	}

	/** True when this operation reads the whole accumulated field and rewrites it. */
	bool IsTransform() const
	{
		return Operation == TEXT("erode") || Operation == TEXT("smooth")
			|| Operation == TEXT("terrace");
	}
};

/** `Snow = layer(info=obj'…') [all(height=(Min=4800))]` — one rule of the paint pass. */
struct GRAPHCODE_API FGcTerrainPaintRule
{
	FString Name;

	/** `info=` — a `ULandscapeLayerInfoObject` asset path. */
	FString Info;

	/** Empty means unconditioned, which exactly one rule may be, and it must be last. */
	FGcTerrainFilter Filter;

	/** The author's own grouping, exactly as an instruction carries it. */
	TArray<FString> Leading;
	FString Trailing;
};

/**
 * `Overworld = terrain(guid=…, base=flat(z=0)) { … }` — the whole program, one entry of
 * `terrains { }`.
 *
 * A BLOCK RATHER THAN A PER-REGION TOKEN, unlike `nav(…)` and `pcg(…)`: terrain spans scales, so the
 * coarse-to-fine stream wants to be ONE ordered sequence rather than scattered across region bodies
 * where its order would be lost. A region is a PLACE an instruction references, not the owner of the
 * instruction (§9.8).
 */
struct GRAPHCODE_API FGcTerrainProgram
{
	/** The landscape actor's label — which terrain in the level this program is for. */
	FString Name;

	/**
	 * `guid=` — `ALandscape::LandscapeGuid`, eight hex digits.
	 *
	 * Carried because under World Partition one landscape is many proxy actors in their own packages
	 * and the GUID is the only thing saying they are one landscape. Optional: a level with a single
	 * landscape needs no disambiguation, and refusing one that has never been written would make
	 * every hand-authored program start with a lookup.
	 */
	FString Guid;

	// **`gen:` MOVED TO THE FILE HEADER, 2026-08-21** — `FGcwDocument::Generation`, user-directed:
	// a version belongs at the top of the file and not on a block head in the middle of it. The old
	// `terrain Name  gen:1 {` spelling still READS and is lifted onto the document; nothing writes
	// it here again, and the field is gone rather than left as a second carrier of one fact
	// ([[two-carriers-of-one-fact-is-a-race]]).

	/** `base=flat(z=0)` — the starting field. Empty means flat at zero. */
	FString Base;
	double BaseZ = 0.0;

	/**
	 * `stream:tiles(components=4)` — how many landscape components go into ONE streamed piece.
	 * Zero is the ordinary state and means the landscape is one actor.
	 *
	 * **THE UNIT IS COMPONENTS AND THE SPELLING HAS TO SAY SO** (§12 L6). `populate { }`'s `grid=`
	 * counts CENTIMETRES, and two keys spelled `grid=` in one file meaning different units is
	 * [[one-concept-two-grammars]] with both halves real — so this is neither called `grid=` nor
	 * left as a bare number. `tiles(components=4)` reuses `base:flat(z=0)`'s `key:shape(name=value)`
	 * form, names the thing it makes and names what it counts, in one field.
	 *
	 * **IT DOES NOT TOUCH THE PROGRAM.** One logical landscape, N streamed pieces, one authored
	 * document: every instruction, domain and filter is written against the whole ground exactly as
	 * before, which is what the user asked for — *"just treat it like a sub graph for the main
	 * terrain{}"*.
	 */
	int32 StreamComponents = 0;

	TArray<FGcTerrainInstruction> Instructions;

	TArray<FGcTerrainPaintRule> Paint;

	/** Whatever sits above `paint {` — the blank line that separates it, and any heading. */
	TArray<FString> PaintLeading;

	/**
	 * Comments left at the BOTTOM of the block, under every line and inside the closing brace.
	 *
	 * Its own field because it belongs to no instruction: attaching it to the last one would move it
	 * when that line moved, and a note about the program as a whole would follow a peak up the file.
	 */
	TArray<FString> Trailing;
};

/**
 * A heightfield being built — the accumulator the instructions run over.
 *
 * IN WORLD UNITS AND NOT IN THE ENGINE'S `uint16`, deliberately. Every operation here is arithmetic
 * on centimetres, so a filter can say `height=(Min=4800)` and mean 48 metres; the conversion to the
 * landscape's own quantised representation happens once, at realisation, where the actor's scale is
 * known. A program is a pure function of its text and nothing in it depends on how a landscape was
 * created.
 */
struct GRAPHCODE_API FGcTerrainField
{
	int32 SizeX = 0;
	int32 SizeY = 0;

	/** World position of sample (0,0), and the world distance between samples. */
	FVector2D Origin = FVector2D::ZeroVector;
	double QuadSize = 100.0;

	/** `SizeX * SizeY` heights in world Z, row-major. */
	TArray<double> Height;

	/** One weight per paint rule per sample, in the program's rule order. `Paint[Rule][Sample]`. */
	TArray<TArray<double>> Paint;

	/**
	 * `hole(…)` — where the ground is ABSENT, one weight per sample, 0 solid and 1 open.
	 *
	 * ITS OWN CHANNEL RATHER THAN A HEIGHT, because a heightfield holds one height per (x, y) and
	 * therefore cannot state a roof: `carve` deep enough is a gorge, never a cave. The engine's
	 * answer, and every shipped UE game's, is a HOLE in the ground with a mesh behind it —
	 * `ALandscapeProxy::VisibilityLayer` and `LandscapeHoleMaterial` exist for nothing else. So this
	 * is the door, and the cave itself is a model somebody placed.
	 *
	 * NOT A PAINT RULE, though it realises as a weightmap like one. A paint rule carries a filter
	 * and no domain; `hole` is where the AI says WHERE, so it needs `at=`/`in=`/`along=` and belongs
	 * in the instruction stream with the rest of the position vocabulary.
	 */
	TArray<double> Visibility;

	FVector2D WorldOf(const int32 X, const int32 Y) const
	{
		return Origin + FVector2D(X * QuadSize, Y * QuadSize);
	}

	double At(const int32 X, const int32 Y) const
	{
		const int32 Cx = FMath::Clamp(X, 0, SizeX - 1);
		const int32 Cy = FMath::Clamp(Y, 0, SizeY - 1);
		return Height.IsValidIndex(Cy * SizeX + Cx) ? Height[Cy * SizeX + Cx] : 0.0;
	}

	bool IsValid() const { return SizeX > 1 && SizeY > 1 && Height.Num() == SizeX * SizeY; }
};

struct FGcwDocument;

/** What running a program measured, so a caller can report it without re-deriving it. */
struct GRAPHCODE_API FGcTerrainEvaluation
{
	FGcTerrainField Field;

	/** One line per instruction: what it was, and how many samples it actually touched. */
	TArray<FString> Applied;

	/**
	 * One line per `sculpt(…)` whose stated `base=` is not the ground it actually landed on — §6's
	 * load-bearing safety rule, and the price of the user's *freely reorderable* ruling.
	 *
	 * **REPORTED AND NOT REFUSED, and it has to be that way round.** Deltas were ruled cherry-pickable
	 * commits rather than adjustment layers, so moving one is the FEATURE; a refusal would make the
	 * ruling unusable. What the author is owed instead is being told, precisely and by name, when a
	 * patch has come to rest on ground that changed underneath it — and never having it silently
	 * repaired ([[a-silent-no-op-is-worse-than-a-refusal]] pointing the other way: the act happens,
	 * and the notice is what stops it being silent).
	 */
	TArray<FString> Rebased;

	/** The lowest and highest Z the program produced. */
	double MinHeight = 0.0;
	double MaxHeight = 0.0;
};

class GRAPHCODE_API FGraphCodeTerrain
{
public:
	/**
	 * Run a program over a field, top to bottom.
	 *
	 * `Document` resolves every region an instruction or a filter names. `OutEvaluation.Field` must
	 * arrive sized and placed; its heights are overwritten from `base:`.
	 *
	 * `SourceDir` is the directory the `.gcw` lives in, which is what a `sculpt(source=file'…')` is
	 * named relative to. **DEFAULTED TO NOTHING AND REFUSING RATHER THAN GUESSING**: a caller who did
	 * not say where the document is has not made deltas unavailable by accident — the one line that
	 * needs it says so by name, and every program without one runs exactly as before.
	 *
	 * **PURE.** Same program, same field extent, same document → identical output, every time and on
	 * every machine. That is the property `gen:` exists to protect and the one the specs assert. A
	 * `sculpt(…)` does not weaken it: the file's bytes are an input like the text is, and the same
	 * bytes give the same ground.
	 */
	static bool Evaluate(const FGcTerrainProgram& Program, const FGcwDocument& Document,
		FGcTerrainEvaluation& OutEvaluation, FString& OutError,
		const FString& SourceDir = FString());

	/** The generator version this build implements. A program authored above it is reported. */
	static int32 CurrentGeneration();

	/**
	 * One instruction, rendered exactly as the block writes it — indent included.
	 *
	 * **PUBLIC SO THAT A CAPTURE'S LINE COMES OUT OF THE RENDERER AND NOT OUT OF A `Printf`.** The
	 * capture half of L3 hands its caller a `sculpt(…)` to paste into the program, and a second
	 * spelling of that grammar living in World would drift from this one the first time a field was
	 * added ([[a-read-answers-through-the-write-path]]). It is the same function the file uses, so
	 * what a capture offers is by construction what a parse will take back.
	 */
	static FString RenderLine(const FGcTerrainInstruction& Instruction);
};
