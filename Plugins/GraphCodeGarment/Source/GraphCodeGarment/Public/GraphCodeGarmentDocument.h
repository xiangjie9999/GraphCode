// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeGarmentClothWords.h"

// A `.gcgarment` read into declarations — `Docs/GC-GARMENT-GRAMMAR.md` §1, "the whole grammar,
// construct by construct". The parser checks spelling and references; anything that needs the body
// (a point off the skin, a stitch not on the garment) is the build's to refuse.

/** `(x, y, z)`, or `(x, y, z)@sharp` — a corner. Centimetres, in the body mesh's own space. */
struct GRAPHCODEGARMENT_API FGcGarmentPoint
{
	FVector Position = FVector::ZeroVector;
	bool bSharp = false;
};

/**
 * What follows `@`: a fraction of the way round (`@0.25`) or a distance round (`@32cm`), then an
 * optional distance on or back (`@0.5+3cm`, `@0-5cm`). Never a percent.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPlace
{
	double Base = 0.0;
	bool bBaseIsDistance = false;
	double AdjustCm = 0.0;
};

/**
 * `$Ring@u` or `$Line@u`, and for a place on a named line an optional `offset=` across — centimetres
 * or a fraction of the way round.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPosition
{
	FString Target;
	FGcGarmentPlace Place;
	bool bHasOffset = false;
	double Offset = 0.0;
	bool bOffsetIsDistance = false;
};

enum class EGcGarmentRingOp : uint8
{
	Expand,
	Width,
	Match,
	Rotate,

	/** `move((x, y, z))`, `shift((along, up, side))`, `swing((along, up, side))` — a copy only (second slice §6.1). */
	Move,
	Shift,
	Swing,

	/**
	 * `circular()` (user, 2026-09-26): the ring made a circle about its middle, in its own plane, reaching its
	 * farthest point — so it still goes round everything it went round — `@0` and the way it counts kept.
	 */
	Circular,
};

/** One instruction in a ring's body, in order. */
struct GRAPHCODEGARMENT_API FGcGarmentRingStep
{
	EGcGarmentRingOp Op = EGcGarmentRingOp::Expand;
	double ExpandCm = 0.0;

	/** Absolute side-to-side size; a ring/measurement reference is resolved before geometry builds. */
	double WidthCm = 0.0;
	TOptional<double> DepthCm;
	FString SizeReference;

	/**
	 * `expand((up, side))` (user, 2026-09-26): the ring grows by `ExpandCm` toward its `@0` and across from it, and by
	 * this square to that — the ring's own up and side, as `shift` has them — blended between. Unset: `expand(cm)`,
	 * the same all round.
	 */
	TOptional<double> ExpandSideCm;

	/**
	 * Degrees as written: `rotate` about X, Y and Z, read the way Shape's `rotate=` reads them;
	 * `swing` about the ring's own along, up and side.
	 */
	FVector RotateDegrees = FVector::ZeroVector;

	/** Centimetres as written: `move` along X, Y and Z; `shift` along the ring's own along, up and side. */
	FVector MoveCm = FVector::ZeroVector;

	/**
	 * `shift`'s parts written as a percent: along, of the way from the ring to where its part of the
	 * body ends or forks, measured the way the sign points; up and side, of the ring's own width.
	 */
	FVector MovePercent = FVector::ZeroVector;

	int32 Line = 0;
};

/**
 * `Name = ring(point, point@sharp, ..., smooth=100%, midpoint=point) { expand(cm) rotate((rx, ry, rz)) circular() }`,
 * or `Name = ring($Other) { ... }` — a copy of another ring, off the body.
 */
struct GRAPHCODEGARMENT_API FGcGarmentRing
{
	FString Name;

	/**
	 * `ring($Cuff)`: the ring this one is copied from (user, 2026-09-25). The copy keeps its shape, its
	 * `@0` and the way it counts, and floats — it is never found on the body again, and `move`, `shift`
	 * and `swing` move it as one piece. Empty: made from its points, on the body.
	 */
	FString Source;

	TArray<FGcGarmentPoint> Points;
	double SmoothPercent = 100.0;
	TOptional<FVector> Midpoint;
	TArray<FGcGarmentRingStep> Steps;
	int32 Line = 0;

	/**
	 * The far edge of a `band($Neck, 1.5)` (user, 2026-09-26), which the parser writes, never the author:
	 * `BandFrom` slid `BandCm` over the skin, away from `BandAwayFrom` — the next ring along the garment
	 * the band is on. On the body, like a ring from points. Empty: not a band's edge.
	 */
	FString BandFrom;
	FString BandAwayFrom;
	double BandCm = 0.0;

	/**
	 * `ring((x, y, z), level)` (G4 slice 2, 17 Nov; spelling awaiting the owner's confirmation): one point, and the body cut flat at its height,
	 * square to the mesh's Z, in place of the shortest plane through it (`FGraphCodeGarmentRings::LevelSection`). False: as before.
	 */
	bool bLevel = false;

	bool IsBandEdge() const { return !BandFrom.IsEmpty(); }
};

/**
 * A new edge's length (GC-GARMENT-CUT-SEW.md §B): `line($A, $B, 30cm)` that much cloth, `+5cm` / `-5cm` that much more or less
 * than corner to corner.
 */
struct GRAPHCODEGARMENT_API FGcGarmentEdgeLength
{
	double Cm = 0.0;
	bool bOnCorners = false;
};

/** `Name = line($Neck@0, $Hem@0)` — a named path for places off the rings. */
struct GRAPHCODEGARMENT_API FGcGarmentLine
{
	FString Name;
	TArray<FGcGarmentPosition> Positions;

	/** `line($A, $B, 30cm)`: a free edge of a piece `insert` makes, and how long it is. Unset: corner to corner. */
	TOptional<FGcGarmentEdgeLength> Length;

	/** `line(..., drop=3cm)`: how far the free edge's middle hangs below level - an `insert()`ed piece's free edge (`FGcGarmentCloth::Insert`). */
	TOptional<double> DropCm;

	int32 Line = 0;
};

/** `Name = profile(0=40%, 0.5=100%, 1=80%)`. All percent (an amount) or all bare (centimetres). */
struct GRAPHCODEGARMENT_API FGcGarmentProfile
{
	FString Name;
	TArray<TPair<double, double>> Keys;
	bool bPercent = false;
	int32 Line = 0;
};

/** A number, or `$Profile` to vary it from the garment's first ring (0) to its second (1). */
struct GRAPHCODEGARMENT_API FGcGarmentAmount
{
	double Value = 0.0;
	FString Profile;

	bool IsProfile() const { return !Profile.IsEmpty(); }
};

/**
 * What an open edge IS, as its maker finishes it (the owner, 16 Nov: "The type of edge should be able to be declared or override isn't it";
 * GC-GARMENT-EDGE-PLAN.md section 4, Research/garment-edge-reads-as-cloth.md). One kind per open edge: the garment's default
 * `construct(edge=turned(2.54cm))`, one edge's own `rim($Hem, turned(2cm, twice, first=0.6cm))`, the override always winning. Appended
 * values only: compared and may be stored.
 */
enum class EGcGarmentEdgeKind : uint8
{
	/** `raw`: the cloth's own cut edge, nothing turned (the old `turned=0cm`). */
	Raw,
	/** `overlocked` / `overlocked(0.4cm)`: a cut edge wrapped in overlock thread, that wide when written. */
	Overlocked,
	/** `turned(d)`, `turned(d, once)`, `turned(d, twice)`, `turned(d, twice, first=d0)`: turned in to the inside and stitched (the old `turned=`). */
	Turned,
	/** `faced(d, $Fabric, roll=, grade=)`: seamed to a facing of depth d at the edge and turned, the seam rolled under. */
	Faced,
	/** `bound(w, $Fabric)`: a binding strip of that fabric folded over the edge, w showing on each face. */
	Bound,
	/** DERIVED, never written: a `band()` is joined to the edge. Writing a kind on a banded edge is refused. */
	Banded,
};

/** The sourced defaults of a faced edge (Research/garment-edge-reads-as-cloth.md section 1; GC-GARMENT-EDGE-PLAN.md section 6). */
namespace GcGarmentEdgeDefault
{
	/** The turn of cloth: the seam rolled 1/16 in under the visible edge (turn-of-cloth trade pages). */
	inline constexpr double FacedRollCm = 0.16;
	/** The enclosed allowances graded: the under one to 3 mm, the upper one to 6 mm (Tilly and the Buttons, collars). */
	inline constexpr double FacedGradeUnderCm = 0.3;
	inline constexpr double FacedGradeUpperCm = 0.6;
}

/**
 * One declared edge kind, as written (`rim($Edge, <kind>)` or `construct(edge=<kind>)`), or as `GcGarmentEdge::Resolve` maps an old
 * `turned=` / `layers=` record onto it. Every distance in centimetres.
 */
struct GRAPHCODEGARMENT_API FGcGarmentEdgeKind
{
	EGcGarmentEdgeKind Kind = EGcGarmentEdgeKind::Raw;

	/** Turned and faced: the depth d; bound: the width w showing on each face; overlocked: its thread's width (0 unwritten); banded: the band's
	 *  front width when it is a number; raw: 0. */
	double DepthCm = 0.0;

	/** Turned: 1 once, 2 twice. 0 only for an old `turned=d` that wrote no count (meshed with the garment's layers, as before). */
	int32 Turns = 0;

	/** `turned(d, twice, first=d0)`: the first (inner) turn's depth, 0 < d0 < d. Unset: equal turns. */
	TOptional<double> FirstCm;

	/** Faced and bound: the facing's or binding's fabric, without its `$`; banded: the band's fabric. */
	FString Fabric;

	/** Faced: the seam rolled this far under on the facing side, and the enclosed allowances graded to these (under, upper). */
	double RollCm = GcGarmentEdgeDefault::FacedRollCm;
	double GradeUnderCm = GcGarmentEdgeDefault::FacedGradeUnderCm;
	double GradeUpperCm = GcGarmentEdgeDefault::FacedGradeUpperCm;
	bool bRollWritten = false;
	bool bGradeWritten = false;

	/** `layers=n` written beside a turned kind: an explicit ply count over the kind's own. Refused with every other kind. */
	TOptional<int32> Layers;

	/** Where it was written (the rim() or construct() line); for a derived banded edge, the band()'s. */
	int32 Line = 0;

	/** Plies over the edge's depth, the cloth counted: raw and overlocked 1, turned once 2 and twice 3 (an old count-less turn 2), faced 2
	 *  (cloth and facing; the graded allowances are zones inside it), bound 3 (the binding's two faces and the cloth), banded 2. */
	int32 Plies() const;

	/** As an author reads it: "turned 2.54 cm twice, first 0.6 cm", "faced 6 cm in $Lining, roll 0.16 cm, grade 0.3/0.6 cm". */
	FString Describe() const;

	/** What today's mesher builds in its place, when it is not this kind itself: "raw (a cut edge, 1 ply)", "turned(6 cm, once), 2 plies of
	 *  the garment's own cloth (no facing fabric, roll or grade)". Empty: built as written (raw, turned without first=, banded). */
	FString MeshedAs() const;
};

/**
 * How a thick cloth's edge is finished — `construct(rim=(turned=4cm, layers=2, falloff=0.1cm))` for every edge of a garment,
 * `rim($Edge, ...)` for one (GC-GARMENT-PLAN.md E1, E3; the user 2026-10-07: *"ensure the parameter can distinguish coat edge vs
 * hoodie one vs shirt one"*). Three measurements, each optional: `TurnedCm` how deep the cloth is turned in at the edge (0 a cut
 * edge), `Layers` how many layers stand there with the cloth counted (2 one turn, 3 a narrow hem turned twice), and `Cm` how far
 * across the edge the stack's thickness falls away — keeping at each place the share of its half-thickness `Profile` says, a percent
 * profile from where the rounding starts (0) to the very edge (1); empty, a round; 0 a square edge, its corners crisp.
 */
struct GRAPHCODEGARMENT_API FGcGarmentRim
{
	TOptional<double> Cm;
	FString Profile;
	TOptional<double> TurnedCm;
	TOptional<int32> Layers;

	/**
	 * `turned=(2.54cm, once)` / `turned=(d, twice)` on the per-edge `rim($Edge, ...)` (15 Nov, the patch's `hem=(d, once|twice)`
	 * spelling): how many times the edge is turned; 0 when the plain `turned=d` is written (every file before 15 Nov), which builds as
	 * before. Carried beside the measurements above: `FGcGarmentRimShape` (the staged record) does not read it.
	 */
	int32 HemTurns = 0;

	/**
	 * `rim($Edge, turned=(d, once), ply)`: the turn is real cloth, stitched by the file's own rows, as `patch(..., ply)`'s hem is.
	 * Per edge only (`construct(rim=...)` refuses it), `once` only, and its garment's fabric must declare `crease=`. Parse only until
	 * the build slices land.
	 */
	bool bPly = false;

	/**
	 * The edge's declared kind (16 Nov): `rim($Edge, faced(6cm, $Lining))` for one edge, `construct(edge=turned(2.54cm, twice))` for the
	 * garment. Unset: none written here (an old `turned=` record, or nothing). When set, the parser has ALSO lowered it onto `TurnedCm`,
	 * `Layers` and `HemTurns` as the nearest behaviour today's mesher builds (`FGcGarmentEdgeKind::MeshedAs`), so `FGcGarmentRimShape`
	 * reads it unchanged; the edge mesher reads the kind through `GcGarmentEdge::Resolve`.
	 */
	TOptional<FGcGarmentEdgeKind> Edge;
};

/** `Gold = material('/Game/Fabric/M_ThreadGold')` — a look and nothing else, Shape's spelling: an Unreal material, no drape. */
struct GRAPHCODEGARMENT_API FGcGarmentMaterial
{
	FString Name;
	FString Path;
	int32 Line = 0;
};

/**
 * `Poplin = fabric(weight=90, thickness=0.31mm, stretch=(1.5%, 7.6%), shear=36.7, stiffness=(1.87, 1.38), friction=0.5, material=$Look)` —
 * what a garment is made of (GC-GARMENT-STITCH.md §3). Every setting is a measured quantity in the unit a fabric test reports it in
 * (user, 2026-09-27: "numbers that have meaning"; 31 Oct: "more professional parameters instead of 0..1"), SiroFAST's where FAST
 * measures it, and every one is optional - a hang refuses a fabric that leaves out one it reads.
 */
struct GRAPHCODEGARMENT_API FGcGarmentFabric
{
	FString Name;

	/** `weight=`: grams a square metre. */
	TOptional<double> WeightGsm;

	/** `thickness=`: one layer, in centimetres — the garment's wall where the garment states none. */
	TOptional<double> ThicknessCm;

	/**
	 * `pressed=(0.497mm, 256kPa)` (17 Nov, stitch-bite-design-16nov.md): one layer's thickness under a sewn row's thread, and the pressure that
	 * reading holds at - the row's bite (`ToMesh`): the plies under a stitch row pressed to it, half from each face, the groove's width from the
	 * layer's bending against the plies under it at that pressure. Unset: no row presses the cloth (as before). The builder assumes no test method:
	 * the author states the reading and its pressure, as the source printed them.
	 */
	TOptional<double> PressedCm;
	TOptional<double> PressedKPa;

	/** `stretch=`: FAST's E100 - how far a strip lengthens under 100 gf/cm (0.98 N/cm), in percent - along the grain, and across it. */
	TOptional<FVector2D> StretchPercent;

	/** `shear=`: FAST's shear rigidity G, N/m - 123 / EB5, its bias extension in percent under 5 gf/cm. */
	TOptional<double> ShearNm;

	/** `stiffness=`: bending length from the overhang test, in centimetres — along the grain, and across it. Larger is stiffer. */
	TOptional<FVector2D> StiffnessCm;

	/** `friction=`: the kinetic coefficient, fabric on fabric. */
	TOptional<double> Friction;

	/**
	 * `crease=`: degrees a pressed crease reopens to - the angle between the two limbs of a strip folded flat and set by a steam press, as the
	 * FAST / IWTO TM 42-02 press test prints it (0 stays folded, 180 fully reopened; Research/fabric-crease-recovery.md). `press($Line)` on a
	 * line drawn on the cloth, and `press($Lapel)` after `fold($Lapel)` (10 Nov), rest their hinges folded 180 less this from flat; either is
	 * refused in a fabric without it. A cold crease recovery angle (AATCC 66, ISO 2313) is not it.
	 */
	TOptional<double> CreaseDeg;

	/** `material=$Look`: a `material()` this file declares, without its `$`. Empty: none. */
	FString Material;
	/** Second reference in `material=($Outside, $Inside)`. Empty: both faces use Material. */
	FString InsideMaterial;

	/**
	 * The cloth's face, as its spec names its construction (13 Nov, the fabric's look): `knit=jersey` (columns of V-shaped loops, purl
	 * loops on the back), `knit=rib1x1` (alternate wales raised and sunk, the same both sides), `weave=plain` (over one, under one).
	 * None: the builder draws no structure, and the garment wears what it wore before these words.
	 */
	enum class EFace : uint8 { None, Jersey, Rib1x1, Plain, Woven };
	EFace Face = EFace::None;

	/**
	 * A woven's crossing rule (16 Nov, GC-GARMENT-FABRIC-LOOK-PLAN.md section 3.1), for `weave=plain` and `Woven`: end i (warp, along V)
	 * is up over pick j (weft, along U) when ((j/Group + Direction x Move x T(i/Group)) mod (Over + Under)) < Over, T(k) = k, or a point
	 * zig-zag of `Reverse` ends for a herringbone. `weave=plain` is 1/1/1; `twill(3/1, z)` 3/1/1, Direction +1 (z: the line rises
	 * toward +U and -V on the face); `satin(5, 2)` 4/1/2; `basket(2/2)` plain in groups of 2; `herringbone(2/2, 8, z)` a 2/2 twill
	 * reversing every 8 ends. `Spelling` is the word as written, for the build's notes.
	 */
	struct FWeave
	{
		int32 Over = 1;
		int32 Under = 1;
		int32 Move = 1;
		int32 Direction = 1;
		int32 Group = 1;
		int32 Reverse = 0;
		FString Spelling = TEXT("plain");
	};
	FWeave Weave;

	/** True for `weave=` (plain or any other woven): the face is drawn by the crossing rule, from ends and picks. */
	bool IsWoven() const { return Face == EFace::Plain || Face == EFace::Woven; }

	/**
	 * `gauge=(15, 19.4)`: loops a centimetre as a knit's spec lists them, wales then courses (a woven's ends then picks), counted on the
	 * cloth laid flat - the texture's real scale. Wales run down the grain (UV0's V), so wales are counted across it.
	 */
	TOptional<FVector2D> GaugePerCm;

	/** `colour=#8C8C8C`: the cloth's colour as a swatch's sRGB, kept here linear. Unset: the plain fabric's own grey. */
	TOptional<FLinearColor> Colour;

	/**
	 * `colour=(#warp, #weft)` (16 Nov, wovens only): yarn-dyed cloth, the weft's colour here and the warp's in `Colour`; the woven's
	 * mask says which shows. Unset: one colour (piece-dyed), the weft the same as the warp.
	 */
	TOptional<FLinearColor> WeftColour;

	/** `yarn=(100tex, 80tex)`: the warp's and the weft's linear density, tex (g/km); `Ne 6` is read as 590.5 / 6 tex. */
	TOptional<FVector2D> YarnTex;

	/** `crimp=(10%, 6%)`: how much longer the warp and the weft are than the cloth they lie in, as fractions (0.10). */
	TOptional<FVector2D> Crimp;

	/** `fibre=cotton`: the fibre, and its density in g/cm3 (the plan's table, C until the page is read). Empty: unset. */
	FString Fibre;
	TOptional<double> FibreDensity;

	/** True when the line says anything about how the cloth looks (`knit=`, `weave=`, `gauge=`, `colour=`). */
	bool HasLook() const { return Face != EFace::None || Colour.IsSet(); }

	int32 Line = 0;
};

/**
 * `Buckle = shape('/Game/Kits/Buckle', Width = 6)` — a `.gcshape` the garment places parts from, Shape's
 * own import line. Each argument replaces a top-level value of that file, as it does between two
 * `.gcshape` files; the values are written out, since a garment has no `$names` Shape could read.
 */
struct GRAPHCODEGARMENT_API FGcGarmentShape
{
	FString Name;
	FString Path;
	TArray<TPair<FString, FString>> Arguments;
	int32 Line = 0;
};

/**
 * `add($Buckle.Buckle, at=$Waist@0, gap=0.2, rotate=(0, 0, 90), stiffness=100%)`: a part made by Shape,
 * seated on the finished cloth at a place on a ring. The part is modelled at its origin with its mounting
 * face at z=0 (Shape's rule for a part), and arrives with +Z out of the body and +Y up it.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPart
{
	/** The `shape()` import's name, and the entry in that file. */
	FString Shape;
	FString Entry;

	FGcGarmentPosition At;

	/**
	 * `at=$Strap.Buckle` in place of a ring place (GC-GARMENT-SHAPE-PARTS.md §6): a point a part written above declares,
	 * the part seated there facing the way the point faces and up the way that part is. Empty: `At`.
	 */
	FString AtPart;
	FString AtPoint;

	/**
	 * `along=$Ring, repeat=N` in place of `at=` (Shape's array words): N copies evenly round the ring from its
	 * `@0`, each placed as `at=` would place one there. Empty: `at=`'s place, or over the cloth where `scatter=` or
	 * `onto=` is among `Arguments` (GC-GARMENT-SHAPE-PARTS.md §7).
	 */
	FString Along;
	int32 Repeat = 1;

	/** Centimetres off the surface it lands on; below zero sinks into it. */
	double GapCm = 0.0;

	/** Shape's `rotate=`, about the part's own axes before it is placed. */
	FVector RotateDegrees = FVector::ZeroVector;

	/** How far it resists bending with the body: 100% moves as one piece, 0% bends with the skin under it. */
	double StiffnessPercent = 100.0;

	/**
	 * `hold=`: how wide, in centimetres, the border is over which the cloth under it eases back from taking its shape and
	 * moving with it (GC-GARMENT-SHAPE-PARTS.md §8). Every part holds the cloth under it as stiffly as it is; 0 holds none.
	 */
	double HoldCm = 3.0;

	/** `hold=` was written: it is then the border at rest too, never widened to the fabric's bend (§8, *the default width*). */
	bool bHoldWritten = false;

	/**
	 * Every other argument, as written, handed to Shape: the words that make copies (`repeat=`, `scatter=`, `onto=`, `step=`,
	 * `jitter=`, …) it keeps and reads, and the rest go to the entry as a call's arguments.
	 */
	TArray<TPair<FString, FString>> Arguments;
};

/**
 * One place a `guide()` passes, in order (GC-GARMENT-SHAPE-PARTS.md §5): a place on a ring or an edge
 * (`$BeltLine@0.2`), a point a named part declared (`$Buckle.Right`, passed through the way it faces), or a point in
 * the body's space (`(0, 12, 104)`).
 */
struct GRAPHCODEGARMENT_API FGcGarmentGuidePoint
{
	/** `$Ring@u`: set where `Position.Target` is. */
	FGcGarmentPosition Position;

	/** `$Buckle.Right`: the named part, and its point. */
	FString Part;
	FString Point;

	/** `(x, y, z)`. */
	TOptional<FVector> At;

	/**
	 * `(x, y, z)@over`: a place two guides both pass — where their cloth crosses — with this one on top (GC-GARMENT-KNOT.md).
	 * The mark is the point's kind, as a ring's `@sharp` is.
	 */
	bool bOver = false;

	/**
	 * `$Knot`: a `constraint()` the garment names, passed; `$Knot@wrap`, this end goes once round the other there
	 * (GC-GARMENT-KNOT.md §8.3).
	 */
	FString Constraint;
	bool bWrap = false;
};

enum class EGcGarmentOp : uint8
{
	Cut,
	Add,
	Stitch,
	Relax,
	Join,
	Construct,
	Extend,
	Sew,
	Line,

	/** `add($Shape.Entry, at=...)` — spelled `add`, but a part from Shape rather than a garment. */
	Part,

	/**
	 * `guide($Edge, place, ..., gap=cm)` (user, 2026-09-27) — the edge's cloth laid along the places in order, its end
	 * taken to the last, its cloth keeping its length. Moving cloth to a place is a guide; one place is enough.
	 */
	Guide,

	/** `pull($Edge, ...)` (user, 2026-09-27) — each edge drawn on the way its cloth lies until the cloth is snug. */
	Pull,

	/**
	 * `fold($Edge, gap=cm)` (GC-GARMENT-SHAPE-PARTS.md §6) — the cloth `extend()` grew out to `$Edge`, turned back at the edge
	 * it grew from and laid over the cloth there: a lapel, a turned-down collar, a turned-up cuff.
	 */
	Fold,

	/**
	 * `match($Waist, 180%, gather)` (GC-GARMENT-MATCH.md, user 2026-09-27) — more cloth along one of the garment's rings than the
	 * ring is long, and how the extra shows: the ring stays put, and what the silhouette has no room for is folded away.
	 */
	Match,

	/**
	 * `press($Seam, 1cm)` (GC-GARMENT-STITCH.md §4) — a seam's allowance folded to one side and pressed there, so that side
	 * stands two thicknesses of the cloth higher; `open` presses it both ways.
	 */
	Press,
	Band,

	/**
	 * `settle(time=1)` (GC-GARMENT-KNOT.md §6) — Chaos lets the cloth hang and press against itself and the body for that many
	 * seconds. Groom's word and its `time=`. Built for a knot's tails.
	 */
	Settle,

	/**
	 * `Knot = constraint(at=(x, y, z), facing=(x, y, z), up=(x, y, z))` (GC-GARMENT-KNOT.md §8.1–§8.3, Groom's word) — a named
	 * place where cloth is held, and its frame. Two guides passing it, one or both `@wrap`, tie a declared knot there.
	 */
	Constraint,

	/**
	 * `insert($Slit, $Flare, $Back, fabric=$Ivory)` (GC-GARMENT-CUT-SEW.md §B) — a piece filling an opening: sewn to the cut
	 * edges in the list, its other sides free edges `line(..., length)` as long as written.
	 */
	Insert,

	/**
	 * `hang(from=$Waist)` (GC-GARMENT-CLOTH.md, user-ruled 2026-10-02): built `[as=cloth]`, the cloth is held on the first ring's
	 * side of that ring and hangs free on the other. Moves no cloth; the cloth writer reads it.
	 */
	Hang,

	/** `rim($Edge, 0.2cm)` / `rim($Edge, (0.2cm, $Round))` (GC-GARMENT-PLAN.md E1): this edge rounds off its own way, not the garment's. */
	Rim,

	/**
	 * `pattern()` (GC-GARMENT-NOTCH-PLAN.md B0): below `construct()` and the garment's cuts, every piece of the cloth between its cuts and
	 * rings is taken off flat once, each edge named; from then on the garment is those flat pieces, and `sew()` joins them edge to edge.
	 */
	Pattern,

	/**
	 * `uv(20cm)`, `uv(fit)`, `uv(20cm, seamless)` (GC-GARMENT-STITCH.md §2a; the user, 21 Oct: "can choose when to UV, just like shape"): the
	 * cloth there is where it is written laid flat at the author's tile, or packed into one tile.
	 */
	UV,

	/**
	 * `Pocket = patch($Top, $Side, $Foot, $Side2, open=$Top, turn=1cm, hem=2cm, fabric=$Leather)` (pockets-13nov, the owner: "A new piece of
	 * garment/cloth sewn on it"): a piece cut to a closed run of drawn lines, laid ON the cloth and sewn to it along every side but `open=`.
	 * Written last so no earlier value moves. Its step reuses existing fields (no new member, so this header grows by one value only):
	 * `Around` its sides in order, `Pulled` its open sides, `Amount.Value` its turn (cm), `Stand.Value` its hem (cm), `PieceFabric` its fabric,
	 * `GapCm` its gap. Built by `GcGarmentPatch` (GraphCodeGarmentPatch.h).
	 */
	Patch,

	/** A bar tack (14 Nov): parsed later; no builder yet. Appended at the end: values are compared and may be stored. */
	Tack,

	/** A buttonhole (14 Nov): parsed later; no builder yet. Appended at the end: values are compared and may be stored. */
	Buttonhole,

	/**
	 * `length($Knee, $Ankle, 4cm)` (23 Nov; Docs/Research/standing-garment-fold-causes.md cause (a), garment-length-ease-stacking.md): the
	 * garment's cloth between two of its rings cut that much LONGER along its lines than the lay puts it - a trouser's break, a jogger's stack
	 * above its cuff, a sleeve longer than the arm. Nothing moves at the lay; the hang rests the cloth at its cut, so held above and landing on
	 * what is below, the extra length folds where it lands. Fields reused (no new member): `Target` the first ring, `Second` the second,
	 * `Amount.Value` the extra length (cm). Appended at the end: values are compared and may be stored.
	 */
	Length,
};

/** How `match()`'s extra cloth shows (GC-GARMENT-MATCH.md §2): one job, the look a parameter. */
enum class EGcGarmentMatchStyle : uint8
{
	/** Many small soft folds at the ring, merging lower down into fewer, a little uneven. */
	Gather,

	/** Even, crisp folds: `pleat(12)`, or `pleat(every=4cm)`. */
	Pleat,

	/** No folds: the cloth stands a little rounder. */
	Ease,

	/** Below 100%: a knit drawn onto a longer edge, resting that much shorter round where it is sewn - a knit cuff, hem or neck holding in. */
	Stretch,
};

/** How far above the cloth under it folded cloth lies when `fold()` says no `gap=`: where the hang's contact meets two layers - the thin cloth's
 *  contact either side (`GcGarmentDraft::ThinContactCm`, 0.05) - and a hair, so they start touching, never inside each other - where they lie
 *  parallel; across a turn sampled by a station or two, or where three layers meet, they start closer, and each point there keeps the contact
 *  its start allows (`GcGarmentDrape::ContactItsStartAllows`, 30 Oct). Laid 0.08 apart,
 *  inside it, a fold's two layers were a pair Chaos never meets for the whole hang, and once the folds hung with the cloth (30 Oct) a lapel
 *  passed through the front it lies on (sim2: 323 crossings; sim3: 8). Marvelous Designer lays folded layers its collision thickness apart. */
inline constexpr double FoldGapCm = 2.0 * 0.05 + 0.001;

/** One instruction in a garment's body, in order. */
struct GRAPHCODEGARMENT_API FGcGarmentStep
{
	EGcGarmentOp Op = EGcGarmentOp::Cut;

	/**
	 * After `construct()`: the step acts on the cloth rather than building on the body (second slice
	 * §3). A cut here cuts the cloth along its ring or line instead of connecting to a ring.
	 */
	bool bOnCloth = false;

	/** The name on the left: `Top = extend(...)` names the edge it grows, `Back_L = line(...)` a line. */
	FString Name;

	/** `cut($Ring)`, `add($Garment)`, `stitch($Line)`, `extend($Edge, ...)`, `sew($Line, ...)` — the first name it takes. */
	FString Target;

	/** `sew($LineA, $LineB)`, and `cut($A, $B)` on the cloth: the second. */
	FString Second;

	/**
	 * `cut('/Game/Art/Tears')` (GC-GARMENT-GRAMMAR.md §5, "Cutting with a drawn image"): the drawing - the author's image converted to SVG
	 * and imported as SVG Data - whose filled shapes are cut out of the cloth. Empty for a cut at a ring or along a line.
	 */
	FString Drawing;

	/** `cut('/Game/Art/Rag', keep)`: the drawn shapes are the cloth that stays, and the rest of the region is cut away. */
	bool bKeep = false;

	/** `uv(20cm)`: one tile every so many centimetres of cloth, both ways; 0 for `uv(fit)`, the whole chart in one tile. */
	double UVTileCm = 0.0;

	/** `uv(20cm, seamless)`: the tile nudged to a whole number of them round each tube the chart covers. */
	bool bUVSeamless = false;

	/** `cut('/Game/Art/Tears', 0.5, 1)`: the drawing laid from one place down the garment to another, 0 its first ring and 1 its second.
	 *  Unset: from its first ring to its second, or between the two edges named in `Target` and `Second`. */
	TOptional<FVector2D> DrawnDown;

	/**
	 * `guide($Front@0.45, place)` (GC-GARMENT-DRAPE.md §3): the one place along a side of a cut the guide takes there, 0 at the
	 * garment's first ring and 1 at its last; the rest of the edge follows. Unset: the whole edge.
	 */
	TOptional<double> TakenAt;

	/** Written inside `settle(time=) { ... }` (GC-GARMENT-DRAPE.md §3): that settle's line; 0 anywhere else. */
	int32 Within = 0;

	/**
	 * `settle() { guide($Hem, $HemUp) }` (19 Nov, reports/hem-guide-slice-19nov.md; the owner: "Pull it up, and hold it there"): a whole ring
	 * of the garment taken to a ring of the file while it hangs, and held there. Read off what is written - a settle's guide with no place
	 * along its edge and one bare name after it - so the step carries no field of its own. The bare name is the place ring (`HoldPlace`).
	 */
	bool IsRingHold() const
	{
		return Op == EGcGarmentOp::Guide && Within > 0 && !TakenAt.IsSet() && Via.Num() == 1 && !Via[0].Constraint.IsEmpty() && !Via[0].bWrap;
	}
	const FString& HoldPlace() const { return Via[0].Constraint; }

	/** `relax(cm)`; `extend($Edge, cm)` or `extend($Edge, $Profile)`. */
	FGcGarmentAmount Amount;

	/** `extend($Edge, cm, 150%)` (GC-GARMENT-CUT-SEW.md, "extend() grows fuller"): the grown cloth's far edge this much of the edge it grows
	 *  from, flaring evenly to it. 0 unwritten: 100%. */
	double FullPercent = 0.0;

	/** `press($Lapel)`, no width: the turn of an earlier `fold($Lapel)` steam-set - its hinges rest at 180 less its fabric's `crease=` (the
	 *  physical fold, GC-GARMENT-FOLD-PHYSICAL-PLAN.md stage 3; ~~as tight as its layers allow~~, drawn, to 10 Nov). */
	bool bPressFold = false;

	/** `fold(..., inside)`: the cloth folded to the inside (ruled, 14 Nov). Not built yet: nothing reads it. */
	bool bFoldInside = false;

	/**
	 * `fold($Collar, stand=3cm)` or `stand=$CollarStand`: a collar, cut flat, standing this far up the neck before it turns down over
	 * itself — centimetres at its widest, or a profile read round the ring it is sewn to, taller at the centre back as collars are
	 * drafted. Unset: no stand.
	 */
	FGcGarmentAmount Stand;
	bool HasStand() const { return Stand.IsProfile() || Stand.Value > 0.0; }

	/** `rim($Edge, ...)`: how that edge is finished; what it leaves unwritten is the garment's. */
	FGcGarmentRim Rim;

	/** `sew(..., smooth=)`: 100% a rounded seam, 0% a sharp ridge. */
	double SmoothPercent = 100.0;

	/**
	 * `sew($A, $B, match=ease)`: the two edges whole, end to end, the longer eased onto the shorter (GC-GARMENT-CUT-SEW.md §F).
	 * Unwritten, a seam joins start to start and gathers nothing.
	 */
	bool bSewEase = false;

	/**
	 * `sew($A, $B, match=gather)`: paired end to end as eased, the longer gathered onto the shorter - as many of its points onto each of the
	 * shorter's as their shares put there, and every row of its cloth the seam drew in laid folded back to its cut length, in folds about as far
	 * apart as match()'s small gathers (GC-GARMENT-MATCH.md §1: "the seam's form"). The hang settles the folds.
	 */
	bool bSewGather = false;

	/**
	 * `sew($Band, $Edge, match=stretch)`: the shorter edge - a knit band - drawn along the longer end to end, the longer staying where it lies;
	 * the band's cloth stretched in the seam's draw as far as the two differ, then resting at its cut length round there (`RestAcross`), so the
	 * hang draws it in as its fabric stretches (GC-GARMENT-MATCH.md §5 item 5).
	 */
	bool bSewStretch = false;

	/**
	 * `zip($A, $B, closed=60%)` (fly-zip-13nov.md, the owner 13 Nov: "cut it first, add zips, then pull it together"): a closure, read as a
	 * `Sew` step on the two sides of one cut. The share of the edges, from their far end (the cut's foot, or its last ring) toward their start,
	 * that the slider has shut: those pairs are joined as `sew()` joins them; the pairs above the slider are left as the cut left them. Unset: a
	 * plain `sew()`, the whole edge. The parts it places (`chain=`, `slider=`) are read into `Part` steps written after it.
	 */
	TOptional<double> ClosedPercent;

	/** `join($Ring@u, ...)`; a line's positions. */
	TArray<FGcGarmentPosition> Positions;

	/** `line($A, $B, 30cm, drop=3cm)`: a free edge's length and drop, which `insert` reads. */
	TOptional<FGcGarmentEdgeLength> EdgeLength;
	TOptional<double> DropCm;

	/** `insert($Edge, $Edge, ..., fabric=$Fabric)` (GC-GARMENT-CUT-SEW.md §B): every edge of the piece, in order round it. */
	TArray<FString> Around;

	/** `insert(..., fabric=$Ivory)`: the piece's fabric, without its `$`. Empty: the garment's. */
	FString PieceFabric;

	/** `guide($Edge, ...)`: the places it passes, in order; the end goes to the last. */
	TArray<FGcGarmentGuidePoint> Via;

	/** `pull($A, $B)`: every edge it draws snug, together. */
	TArray<FString> Pulled;

	/** `patch(..., ply)`: the patch a declared layer that stitch rows tie (ruled 14 Nov 11:15): read by `GcGarmentPatch::IsPly`, which gates
	 *  the ply's own panel, its lay along the cloth's normal and the needle holes' ties. False: the patch rides its cloth as before. */
	bool bPly = false;
	/** `patch(..., hem=(1.27cm, once))` / `hem=(1.27cm, twice)`: how many times the open side is turned, the garment's declared number (ruled
	 *  14 Nov). The plain `hem=1.27cm` and an unwritten hem: turned twice, as built before. */
	int32 HemTurns = 2;
	/** `patch(..., turn=0, layers=3, ply)` (15 Nov, patch-raw-plies-15nov.md): how many plies of its fabric the piece is, stacked and sewn as one
	 *  (a belt loop folded in three), the word rim()'s `layers=` uses. Its stack is this many times its fabric's thickness. 1 unwritten. */
	int32 PatchLayers = 1;
	/** A button's place along a band's lap, cm (14 Nov, for the waistband slice). Below 0: none. Not built yet: nothing reads it. */
	double ButtonCm = -1.0;
	/** `patch(..., turn=0, ply, ease=0.3cm)` (15 Nov, patch-ease-15nov.md): the piece cut this much longer than the span between its sewn ends,
	 *  along its long way, cm; laid as the arc of that length (`GcGarmentPatchEase::Arch`). Unset: laid flat, as before. */
	TOptional<double> EaseCm;
	/** `patch(..., turn=0, ply, under, gap=0.1cm, window=($W1, ..., $Wn))` (16 Nov, the owner: "the front pocket separate layer"): the piece
	 *  lies UNDER the cloth it is drawn on (on the body side), and the cloth above it is opened along `WindowLines`, a closed outline of drawn
	 *  lines in order round it (the last ends where the first starts), independent of the piece's own outline (pocket-under-layer-16nov.md). */
	bool bUnder = false;
	TArray<FString> WindowLines;
	/** `sew` / `zip` / `band` `over=`, `lap=`: the proper home of a closing's over-side and lap (today carried in `Pulled` / `DropCm`, which
	 *  stay the carriers until a later step moves them). Not built yet: nothing reads these. */
	FString ClosingOver;
	TOptional<double> ClosingLapCm;
	/** `zip(..., tape=, stop=)`: the zip tape's fabric and its stop's part, without their `$`. Not built yet: nothing reads these. */
	FString TapeFabric;
	FString StopPart;
	/** `tack($Row@u, length=0.95cm)`: the bar tack's length, cm (`EGcGarmentOp::Tack`); built at settle() as needle ties (Drape.cpp, `GcGarmentNeedleThrough::Tack`). */
	TOptional<double> TackLengthCm;
	/** `press($FlyL, -3.195cm, follow=$J, margin=0.68cm)` (15 Nov, press-follow-15nov.md): the stitch row the pressed allowance's free edge
	 *  follows, without its `$`, and how far outside that row the edge lies, cm; the declared width stays the cap. Empty / unset: a strip of the
	 *  declared width, as before. One row or several (`follow=($J1, $J2)`, press-follow-rows-15nov.md): each an ordinary two-place line, their
	 *  samples pooled into the one offset curve, in any order. */
	TArray<FString> FollowRows;
	TOptional<double> FollowMarginCm;

	/** `guide(..., gap=)`: centimetres between the guided cloth and what it is laid over. */
	double GapCm = 0.0;

	/**
	 * `guide(..., at=$Knot)`: the constraint whose frame its `(x, y, z)` places are written in, without its `$`. Groom's
	 * `spline(at=$Tie)` (GC-GARMENT-KNOT-PLAN.md §17): `(0, 0, 0)` is the constraint's place, Z runs along its `facing=`, X
	 * along its `up=` (the world's, unwritten) turned square to the facing, Y = Z × X. Empty: the body's space.
	 */
	FString GuideFrame;

	/** `guide(..., clamp=30%)`: how hard the guided end is gathered, 0% as wide as it is to 100% as tight as it can be (Groom's `spline(clamp=)`). */
	TOptional<double> ClampPercent;

	/** `settle(time=)`: seconds. */
	double SettleSeconds = 1.0;

	/** `constraint(at=, facing=, up=)`: where, which way is out of the body, and which way is up — unset, the world's up. */
	FVector HeldAt = FVector::ZeroVector;
	FVector Facing = FVector::ZeroVector;
	TOptional<FVector> Up;

	/** `add($Shape.Entry, ...)`. */
	FGcGarmentPart Part;

	/** `match($Edge, 180%, pleat(12))`: the style, and a pleat's count or its spacing — one of the two, the other 0. The percentage is `Amount`.
	 *  On a `sew()`, `match=pleat(3)`: the count alone, the longer edge folded into that many knife pleats onto the shorter (GC-GARMENT-MATCH.md
	 *  §5 item 1); 0, not pleated. */
	EGcGarmentMatchStyle MatchStyle = EGcGarmentMatchStyle::Gather;
	int32 PleatCount = 0;
	double PleatEveryCm = 0.0;

	/** `pleat(16, stitched=15cm)`: how far down the pleats are stitched flat before they open. 0: to where the cloth leaves the body. */
	double PleatStitchedCm = 0.0;

	/** `pleat(12, box)`, `pleat(12, inverted)`: each pleat two knife pleats folded toward each other behind a face - a box - or facing each other
	 *  over the cloth folded under them - inverted, the box pleat seen from its back. 0 knife (the default, all one way), 1 box, 2 inverted. */
	uint8 PleatKind = 0;

	/** `pleat(2, reverse)`: knife pleats folded the other way - each fold edge facing against the way the ring counts, or on trousers toward
	 *  the pockets, where they open toward the fly by default (GC-GARMENT-MATCH.md §5 item 2). Knife pleats only. */
	bool bPleatReverse = false;

	/** `gather(every=8cm)`: the large folds' spacing round the ring. 0: the default. */
	double GatherEveryCm = 0.0;

	/** `gather(jitter=60%)`: how uneven the folds are, 0% even; unset, today's 35%. */
	TOptional<double> GatherJitterPercent;

	/** `gather(seed=3)`: the pattern of the folds, by the seed alone; unset, seeded from the garment's and the ring's names. */
	TOptional<int32> GatherSeed;

	/**
	 * `match(..., falloff=100%)` or `falloff=$Fade`: how much of the fullness has gone by each height, 0 the ring and 1 the
	 * garment's other end. A percentage is the far end's, faded evenly from 0% at the ring. Unset: none goes.
	 */
	TOptional<FGcGarmentAmount> Falloff;

	/**
	 * `stitch(..., offset=0.6cm)` — how far across the cloth from its line the stitch runs; `press($Seam, 1cm)` — the allowance's
	 * width. Both signed as `offset=` is: on a ring, positive into the cloth laid first there and negative into the cloth added or
	 * grown there; on a line of cloth, positive the way the rings count (GC-GARMENT-STITCH.md §4–§5).
	 */
	double OffsetCm = 0.0;

	/**
	 * `stitch($Hem, at=turn, needles=2, gauge=4.8mm)` (22 Nov): the rows a machine sews from what the edge declares - `at=turn`, the first
	 * needle at the edge's own turned depth (its raw edge); `needles=n` rows, each `gauge=` nearer the edge than the last. Unwritten: one
	 * row at `offset=`, as before.
	 */
	bool bStitchAtTurn = false;
	int32 StitchNeedles = 1;
	double StitchGaugeCm = 0.0;
	/** `inset=` with `at=turn`: the first needle this far inside the turn's inner edge (an edge-stitched hem's 2 mm); 0, at it (a coverstitch). */
	double StitchInsetCm = 0.0;

	/** `stitch(..., every=3mm)`: from one needle hole to the next. */
	double EveryCm = 0.3;

	/** `stitch(..., material=$Gold)`: the thread's look, a `material()` without its `$`. Empty: a plain off-white thread. */
	FString Material;

	/** `press($Seam, 1cm, open)`: pressed open, one layer more on each side. */
	bool bOpen = false;

	/** Added fabric coverage; widths are front/back centimetres along the cloth, not total cut width. */
	FString BandFabric;
	FGcGarmentAmount BandWidths[2];
	TOptional<FGcGarmentAmount> BandThickness;
	FVector2d BandSmooth = FVector2d::Zero();
	/** `band(..., cut=95%)`: the band cut that share of its edge's length, as a pattern states it (FreeSewing Hugo's ribbing, 95 %). Unset:
	 *  `GcGarmentFlat::BandRatio`'s default from the band fabric's stretch. */
	TOptional<double> BandCutPercent;
	/** `band(..., ends=)`: how an open band's ends are finished, as written (14 Nov). Not built yet: nothing reads it. */
	FString BandEnds;
	/** Shared operation suffix: [front]=1, [back]=2, [both]=3. Band defaults to both; other surface operations to front. */
	int32 Faces = 1;
	bool bFacesWritten = false;

	int32 Line = 0;

	/** `Placket = extend($Seam, 3.184, fabric=$F, turn=0.509cm)` (17 Nov, gc.Garment.OneFold 6, reports/one-fold-s6a-strip-17nov.md): on a strip
	 *  `extend` lays ON the cloth (`ExtendOnCloth`), how far each of its two LONG edges (its foot and its far edge) is turned under, cm, written
	 *  >= 0 - `patch()`'s word for the same thing (its turn lives in `Amount`, which `extend` uses for its length, so it has its own field here).
	 *  Its plies, `layers=`, go in `PatchLayers` (as `sew()` reuses it). Unset: not written, and every existing file is unset - laid as before.
	 *  Read at gc.Garment.OneFold 6 only; on an `extend` that is not laid on the cloth it is refused at the lay by name. Appended last: no brace
	 *  initialiser of this struct exists. */
	TOptional<double> OnClothTurnCm;

	/** `hang(from=$Hips, damping=0.05, local_damping=0.3, iterations=12, substeps=4, ccd, self=0.1cm, repair)` (18 Nov, G1 slice 2, the lead's
	 *  rulings of 17 Nov): the game cloth's runtime words, read by `[as=cloth]` only (`GcGarmentClothAsset::ApplySolverWords`). Unset: not
	 *  written, and every existing file is unset - the asset as before. Appended last: no brace initialiser of this struct exists, and
	 *  `LayerStage.cpp` stages no step (no ReadyVersion). */
	FGcGarmentClothSolverWords ClothSolver;

	/** `Over = extend($Wrap, $Overlap, over)` (18 Nov, G2 of the skirt programme, reports/wrap-over-18nov.md): the bare word `over` - cloth grown on
	 *  from a side of a cut and laid round OVER the garment's own cloth past that cut (a wrap's over-front), welded at its root, hanging as cloth.
	 *  With it `gap=` (in `GapCm`, the patch's meaning: clear space added on top of the plies' rest) and `layers=` (in `PatchLayers`) are read.
	 *  A new word no shipped file writes: read at every level. False: not written, and every existing file is false. Appended last: no brace
	 *  initialiser of this struct exists, and `LayerStage.cpp` stages no step (no ReadyVersion). */
	bool bOnClothOver = false;
};

/** `Name = garment($RingA, $RingB, loose=70%, thickness=0.3, clearance=$Puff) { ... }` */
struct GRAPHCODEGARMENT_API FGcGarment
{
	FString Name;
	FString RingA;
	FString RingB;

	/**
	 * The rings the cloth passes through between its two ends, in the order written —
	 * `garment($Neck, $Crown, $Face, ...)` (user, 2026-09-25). Section N runs from the Nth ring written
	 * to the next. Empty: one section, end to end.
	 */
	TArray<FString> Middle;

	/** Percent, or a percent profile, from the first ring (0) to the last (1). Above 100 is slack. */
	FGcGarmentAmount Loose;

	/**
	 * `loose@N=` and `clearance@N=`: one section's own, by its number from 1, its 0 to 1 running across
	 * that section alone. Each replaces the whole garment's `loose=` or `clearance=` there.
	 */
	TMap<int32, FGcGarmentAmount> SectionLoose;
	TMap<int32, FString> SectionClearance;

	int32 Sections() const { return Middle.Num() + 1; }

	/**
	 * Centimetres, or a centimetre profile. Unset: a single sheet — or, on a garment added to another,
	 * the thickness of the garment it is added to.
	 */
	TOptional<FGcGarmentAmount> Thickness;

	/** `construct(rim=...)`: how every edge of its cloth is finished, unless `rim($Edge, ...)` says otherwise for one (GC-GARMENT-PLAN.md E3). */
	FGcGarmentRim Rim;

	/**
	 * `fabric=$Denim`: what it is made of (GC-GARMENT-STITCH.md §3), without its `$`. Empty: the fabric of the garment that adds
	 * it, if any.
	 */
	FString Fabric;

	/**
	 * `over=$Shirt` or `over=($Shirt, $Vest)` (GC-GARMENT-COVERAGE.md gap 1): the garments this one is worn over, without their `$`.
	 * Where they cover the skin their outside is this garment's floor — it is made, relaxed and settled on them.
	 */
	TArray<FString> Over;

	/**
	 * The guides written inside a settle's braces (GC-GARMENT-DRAPE.md §3): where a hand draws the cloth while it hangs, in Chaos,
	 * each `Within` its settle's line — `settle(time=2) { guide($Front@0.7, $Hips@0.3) }`.
	 */
	TArray<FGcGarmentStep> SettleGuides;

	/** Explicit native-cloth contact set. Empty/unset retains existing construction semantics. */
	bool bNativeCloth = false;
	bool bSelfCollision = false;
	TArray<FString> CollisionParts;

	/**
	 * `clearance=$Profile`: a centimetre profile of how much further off the body the garment stands
	 * between its rings, on top of the clearance blended from their `expand`. 0 at both ends, which are
	 * the rings' own, and at every middle ring the cloth passes through. Empty: none.
	 */
	FString Clearance;

	/**
	 * `Belt = accessory() { ... }` (GC-GARMENT-SHAPE-PARTS.md §6): parts with no cloth — no rings, nothing to construct —
	 * placed on the body, and guided and pulled as cloth is. Built like a garment; its steps are parts, `guide()` and
	 * `pull()`.
	 */
	bool bAccessory = false;

	TArray<FGcGarmentStep> Steps;
	int32 Line = 0;
};

/** What a build line makes — its `[as=]`, Shape's slot (`GC-GARMENT-METAHUMAN.md` §2). */
enum class EGcGarmentAssetKind : uint8
{
	/** No `[as=]`: a static mesh. */
	Static,

	/** `[as=skeletal]`: a skeletal mesh on the body's own skeleton, its weights copied from the skin. */
	Skeletal,

	/**
	 * `[as=outfit]`: a Chaos outfit Epic refits to other MetaHumans, on a character only — its cloth asset, the body
	 * it was made on saved as its one size, and the wardrobe item, all written by Core Extension (step 4).
	 */
	Outfit,

	/**
	 * `[as=cloth]` (GC-GARMENT-CLOTH.md, user-ruled 2026-10-02): a Chaos Cloth Asset on the body's skeleton that simulates in the
	 * game — the garment as drawn, bound to a simulation mesh of its cloth's middle, held where `hang()` says, colliding with
	 * the body's Physics Asset.
	 */
	Cloth,
};

/** `SM_TShirt = $TShirt`, or `SK_TShirt = $TShirt [as=skeletal]` — an asset made from a garment. */
struct GRAPHCODEGARMENT_API FGcGarmentBuildLine
{
	/** As written: a bare name or a full package path. */
	FString Asset;

	/** Resolved against `into:` where the name was bare. */
	FString PackagePath;

	FString Garment;
	EGcGarmentAssetKind As = EGcGarmentAssetKind::Static;
	int32 Line = 0;
};

/**
 * `T_Stitches = bake(decal, resolution=2048)` in `build { }` — the file's stitch sheet (GC-GARMENT-STITCH.md §5), Shape's
 * decal sheet spelled as Shape spells it. A stitch with no sheet to be drawn into is refused, Shape's rule.
 */
struct GRAPHCODEGARMENT_API FGcGarmentSheet
{
	/** As written, and resolved against `into:` where it is bare. */
	FString Asset;
	FString PackagePath;

	/** Square, in pixels. */
	int32 Resolution = 1024;

	int32 Line = 0;
};

struct GRAPHCODEGARMENT_API FGcGarmentHeader
{
	int32 Version = 0;
	FString Mesh;
	FString Into;

	/** Where every ring's counting direction is set from. Absent: the body mesh's origin. */
	TOptional<FVector> Reference;
};

/**
 * `Tee = worn('/Game/Characters/Clothing/SKM_Tee')` (G4 slice 2, 17 Nov; spelling awaiting the owner's confirmation): a skeletal or static mesh the
 * body already has on. Read by the build, measured against and reported by place (`GcGarmentWorn`); never changed, never laid against.
 */
struct GRAPHCODEGARMENT_API FGcGarmentWornMesh
{
	FString Name;
	FString Path;
	int32 Line = 0;
};

// CLOTH-CURTAIN1 slice A1: a curtain's track and panels (Docs/GC-CLOTH-CURTAIN-DESIGN.md section 9), defined here because the document
// below holds them and a panel's body reuses FGcGarmentStep above (GraphCodeGarmentCurtain.h says how its guard works).
#define GRAPHCODEGARMENT_DOCUMENT_CURTAIN_POINT 1
#include "GraphCodeGarmentCurtain.h"
// end CLOTH-CURTAIN1

struct GRAPHCODEGARMENT_API FGcGarmentDocument
{
	FGcGarmentHeader Header;
	TArray<FGcGarmentRing> Rings;
	TArray<FGcGarmentLine> Lines;
	TArray<FGcGarmentProfile> Profiles;
	TArray<FGcGarment> Garments;
	TArray<FGcGarmentShape> Shapes;
	TArray<FGcGarmentMaterial> Materials;
	TArray<FGcGarmentFabric> Fabrics;
	TArray<FGcGarmentBuildLine> Build;
	TOptional<FGcGarmentSheet> Sheet;

	/**
	 * One line per declared edge kind today's mesher builds as something nearer (16 Nov, `FGcGarmentEdgeKind::MeshedAs`): "garment 'Coat'
	 * edge $LapelR: faced 6 cm in $Lining, ... (rim() override, line 190) declared, meshed as ... until the edge mesher takes it". Filled by
	 * the parser (and logged once each, Display); a build adds them to its notes.
	 */
	TArray<FString> EdgeNotes;

	/** Each `worn('...')` declared, in the file's order. Empty: none, as every file before 17 Nov. */
	TArray<FGcGarmentWornMesh> Worn;

	const FGcGarmentRing* FindRing(const FString& Name) const;
	const FGcGarmentLine* FindLine(const FString& Name) const;
	const FGcGarmentProfile* FindProfile(const FString& Name) const;
	const FGcGarment* FindGarment(const FString& Name) const;
	const FGcGarmentShape* FindShape(const FString& Name) const;
	const FGcGarmentMaterial* FindMaterial(const FString& Name) const;
	const FGcGarmentFabric* FindFabric(const FString& Name) const;
	const FGcGarmentWornMesh* FindWorn(const FString& Name) const;

	// CLOTH-CURTAIN1 slice A1
	/** Each `track()` and `panel()` declared (GraphCodeGarmentCurtain.h), in the file's order. Empty in every garment file. */
	TArray<FGcGarmentTrack> Tracks;
	TArray<FGcCurtainPanel> CurtainPanels;
	const FGcGarmentTrack* FindTrack(const FString& Name) const;
	const FGcCurtainPanel* FindCurtainPanel(const FString& Name) const;
	// end CLOTH-CURTAIN1

	/** Its own `fabric=`, else the fabric of the garment that adds it, and so on up. Null: none. */
	const FGcGarmentFabric* FabricOf(const FGcGarment& Garment) const;

	/** Its own `thickness=`, else its own fabric's. Unset: a single sheet, or the thickness of the garment that adds it. */
	TOptional<FGcGarmentAmount> ThicknessOf(const FGcGarment& Garment) const;
};

/**
 * One open edge's effective kind (16 Nov), what the edge mesher reads: the edge's own `rim()` if it writes a kind (or an old `turned=`),
 * else the garment's default, else raw; and banded, derived, where a `band()` is joined to it. Says where it came from, so a build line
 * can print "edge $Hem: turned 2.54 cm twice (garment default, line 185)".
 */
struct GRAPHCODEGARMENT_API FGcGarmentEdgeResolved
{
	enum class ESource : uint8
	{
		/** Nothing written: raw. */
		Unwritten,
		/** The garment's `construct(edge=)` or `construct(rim=(turned=))`. */
		Default,
		/** The edge's own `rim($Edge, ...)`. */
		Override,
		/** Banded: a `band()` joined to it. */
		Derived,
	};

	FString Edge;
	FGcGarmentEdgeKind Kind;
	ESource Source = ESource::Unwritten;

	/** The line it came from: the rim(), the construct(), the band(); 0 unwritten. */
	int32 Line = 0;

	/** True when read from an old `turned=` / `layers=` record (no kind word): today's rules exactly, `Kind.Turns` 0 where no count was written. */
	bool bLegacy = false;

	/** An explicit ply count in effect (`layers=` on the edge, else the garment's written one): unset, `Kind.Plies()`. */
	TOptional<int32> Layers;

	/** `falloff=` in effect, the edge's own else the garment's; `FalloffProfile` its profile's name, empty a round. Unset: half the stack. */
	TOptional<double> FalloffCm;
	FString FalloffProfile;

	int32 Plies() const { return Layers.IsSet() ? *Layers : Kind.Plies(); }

	/** "edge $Hem: turned 2.54 cm twice (garment default, line 185)". */
	FString Describe() const;
};

namespace GcGarmentEdge
{
	/** `Edge`'s effective kind on `Garment` (an edge name without its `$`: a ring the garment ends at, an extend()'s or cut()'s or patch()'s
	 *  name). Pure: reads the parsed document only. */
	GRAPHCODEGARMENT_API FGcGarmentEdgeResolved Resolve(const FGcGarment& Garment, const FString& Edge);

	/** The kind one level writes: its `Edge` when a kind word is written, else its old `turned=` mapped (0cm raw, d turned with `HemTurns`
	 *  as its count). False when that level says nothing about the kind (no kind word, no `turned=`). */
	GRAPHCODEGARMENT_API bool WrittenKind(const FGcGarmentRim& Rim, FGcGarmentEdgeKind& OutKind);
}

class GRAPHCODEGARMENT_API FGraphCodeGarmentParser
{
public:
	/** False with `OutRefusal` — `line N: ...` — on the first thing the file cannot say. */
	static bool Parse(const FString& Text, FGcGarmentDocument& OutDocument, FString& OutRefusal);
};
