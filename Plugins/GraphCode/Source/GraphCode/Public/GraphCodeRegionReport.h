// Copyright OraStudio. All Rights Reserved.

// What a `.gcw` says ABOUT ITSELF — `GC-SPATIAL-AUTHORING-DESIGN.md` §21, task R4.
//
// Two answers live here, and they share a file because the second runs on top of the first:
//
//   1. **HOW CURVED IS THIS WALL, in units a person reads** (§21.6a). `bulge:-0.3` is the right thing
//      to STORE — it is scale-free and no value of it is geometrically impossible — and the wrong
//      thing to be the only thing anybody can see. R0 already computes every readable form of it:
//      `ArcOf` returns the centre, the radius and a signed sweep, `EdgeLength` returns the arc length.
//      This names them, and adds the one identity §21.6a states — a bulge IS a bow depth over the
//      half-chord, so `bulge:0.4` on a 1200 cm wall bows out 240 cm at its middle.
//
//   2. **THE CRACK LIST** (§21.14a): two walls lying on top of each other, and HOW FAR APART they are.
//
// **WHAT THIS LIST MEANS CHANGED WITH §21.1b, and it changed for the better.** It was written when a
// region could share a wall, so two coincident walls meant *you drew two where you should have named
// one* — a defect. Nothing is shared now (user-ruled 2026-08-12: *"a region is its own"*), so two
// rooms that meet ALWAYS declare two walls in the same place, and the coincidence is the normal way a
// building is authored. What the reader wants to know is therefore not *are these two* but **how far
// apart are they**: zero is a clean join, and four centimetres is a crack somebody will fall through.
// The list already answered exactly that, which is why the ruling cost it nothing.
//
// A FINDING REPORTS THE GAP AS A DISTANCE AND NEVER AS A FLAG — §20.22a's ruling one surface over.
// Occlusion reports a ratio because a boolean makes a sliver behind a pillar look like a clear view; a
// crack reports HOW FAR APART because a boolean makes a 0.1 mm rounding error look like a metre-wide
// hole. The tolerance below is then only a coarse *worth mentioning at all* cutoff, and the real
// number travels with the finding where a reader can weigh it.
//
// NOTHING HERE RESOLVES A RING, deliberately. Membership is AUTHORED — a region's edges are its own —
// so both answers hold for a file whose region does not close, which is exactly the file most likely
// to have a crack in it. `FGraphCodeRegionSolver::Resolve` would refuse it and answer nothing.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegion.h"

// `EGcEdgeUse` — Unused / Free / Shared — STOOD HERE AND IS GONE (§21.1b). It counted the loops that
// named an edge, and under ownership that count is 1 for every edge in the file: a wall is declared
// inside exactly one region and the order of the declarations is that region's ring, so there is
// nothing to tally. A three-valued answer that can only ever be one value is not information, and the
// specs asserting on it could not have failed ([[specs-that-cannot-fail]]). What a reader actually
// wanted from it — *which room is this wall in* — is now one field on the report below.

/** One edge's curvature in units a person reads — §21.6a. Every field is R0's, renamed for a reader. */
struct GRAPHCODE_API FGcEdgeCurvature
{
	/** False leaves the circle fields zero: a straight wall has no centre and no radius to report. */
	bool bIsArc = false;

	/** As stored. Kept beside the readable forms so a caller can write the file back. */
	double Bulge = 0.0;

	/**
	 * The SAGITTA in centimetres, signed the way the bulge is: positive bows LEFT of the edge's own
	 * declared direction, whichever loop happens to name it.
	 *
	 * `Bulge * Chord / 2` exactly, by the half-angle identity §21.6a writes out — no trigonometry, and
	 * no dependence on the arc having been built.
	 */
	double BowCm = 0.0;

	/** Straight-line distance between the two declared points. */
	double ChordCm = 0.0;

	/** Along the curve — the chord when straight. What a wall actually measures. */
	double LengthCm = 0.0;

	/** Zero when straight. A semicircle's radius is half its chord; nothing smaller is expressible. */
	double RadiusCm = 0.0;

	/**
	 * Signed like the bow, in DEGREES because that is the unit the question was asked in.
	 *
	 * This is `FGcRegionArc::Sweep` NEGATED, and the difference is not an accident: the kernel signs a
	 * sweep by the direction the traversal turns — a left-bowing arc turns clockwise — while a reader
	 * asking how curved a wall is needs one sign for one shape.
	 */
	double SweepDegrees = 0.0;

	/** Plan centre of the circle, world centimetres. Meaningless when straight. */
	FVector2D Centre = FVector2D::ZeroVector;
};

/** One edge, as the file describes it: who names it, where it runs, and how it bends. */
struct GRAPHCODE_API FGcEdgeReport
{
	int32 EdgeIndex = INDEX_NONE;

	/** The one name it has, since `as:` went — the word on the left of its declaration. */
	FString Name;

	/** The region that declares it, which under §21.1b is the only one that can. */
	FString Region;

	FVector2D Start = FVector2D::ZeroVector;
	FVector2D End = FVector2D::ZeroVector;

	/** Every point it runs through, so a jogged wall reports its bends and not only its ends. */
	TArray<FVector2D> Through;

	FGcEdgeCurvature Curvature;
};

/**
 * Two walls lying on top of each other — one finding, with the gap on it.
 *
 * `A` IS THE SHORTER EDGE, always, and that is what makes `OverlapCm` readable: the common case is a
 * 400-wide corridor meeting the MIDDLE of a 1200-wide hall wall (§21.14a's point 4), where *400 cm of
 * this edge is covered* is the sentence a reader needs and *400 of 1200* is the one they do not.
 */
struct GRAPHCODE_API FGcRegionCrack
{
	int32 EdgeA = INDEX_NONE;
	int32 EdgeB = INDEX_NONE;

	FString EdgeAName;
	FString EdgeBName;

	/** The region declaring each. Under §21.1b every wall has exactly one, and often they differ. */
	FString RegionA;
	FString RegionB;

	/**
	 * How far apart they actually are, in centimetres, at the WORST point of the overlap.
	 *
	 * THE WHOLE FINDING, and never a flag. Zero is two rooms meeting exactly, which since §21.1b is
	 * how neighbours are authored; five is a gap somebody will fall through; a wall's thickness is a
	 * wall's thickness and the reader is the one who knows which.
	 */
	double GapCm = 0.0;

	/** How much of the SHORTER edge the other one covers. Below `MinimumOverlapCm` is not a finding. */
	double OverlapCm = 0.0;

	/** Both ends coincide within the cutoff: the two rooms line up wall for wall, not part way. */
	bool bDuplicate = false;

	/** The two walls belong to ONE region — the same room drawn over itself, which is never right. */
	bool bSameRegion = false;
};

/** One document's self-description: every edge, and every crack between two free ones. */
struct GRAPHCODE_API FGcRegionFileReport
{
	/** Parallel to nothing — `EdgeIndex` carries the identity, in declaration order. */
	TArray<FGcEdgeReport> Edges;

	/**
	 * Ordered by gap, closest first, then by edge index — so the duplicate walls a reader must fix
	 * arrive before the 20 cm coincidences they must judge. [[a-capped-list-must-be-ranked]].
	 */
	TArray<FGcRegionCrack> Cracks;
};

/** The `.gcw`'s report surface. Pure arithmetic over a space: no file, no world, no editor. */
class GRAPHCODE_API FGraphCodeRegionReport
{
public:
	/**
	 * How far apart two free edges may be and still be worth mentioning — a quarter of a metre.
	 *
	 * A COARSE CUTOFF AND NOT A JUDGEMENT (§21.14a). It is set above a typed-by-hand gap and below an
	 * ordinary wall thickness, so the common false positive — two rooms whose walls have substance —
	 * mostly falls out while a 5 cm mistake is still reported. Anything it does report carries its own
	 * distance, so a reader who disagrees with this number can see straight past it.
	 */
	static constexpr double ReportableGapCm = 25.0;

	/**
	 * Two edges meeting at a corner overlap by nothing; below this they are touching, not coinciding.
	 *
	 * One centimetre rather than zero, because two walls that share a corner have an overlap of exactly
	 * zero only in exact arithmetic.
	 */
	static constexpr double MinimumOverlapCm = 1.0;

	/**
	 * A bow this shallow is a straight wall for coincidence purposes — R0's boundary tolerance.
	 *
	 * `FGcRegionEdge::IsArc` asks whether the bulge is exactly zero, which is the right question for
	 * the kernel and the wrong one here: an edge bowing by a ten-thousandth of a centimetre coincides
	 * with a straight one, and calling it an arc would send it down the same-circle path with a radius
	 * of several kilometres.
	 */
	static constexpr double StraightBowCm = FGraphCodeRegionSolver::BoundaryToleranceCm;

	/** One edge's curvature in readable units, against the edge's OWN declared direction. */
	static FGcEdgeCurvature CurvatureOf(const FGcRegionSpace& Space, int32 EdgeIndex);

	/**
	 * Two walls that coincide, ranked closest-gap first.
	 *
	 * EVERY WALL IS ASKED. It used to be free edges only, because a shared edge could not be a crack
	 * with itself; §21.1b leaves nothing shared, so the filter had become *all of them* and saying so
	 * is cheaper than a tally that always answers the same. Two walls of the SAME room coinciding is
	 * still a different and rarer finding, and it is flagged rather than dropped.
	 *
	 * O(n²) over walls, which is nothing at a file's scale.
	 */
	static void FindCracks(const FGcRegionSpace& Space, TArray<FGcRegionCrack>& OutCracks);

	/** The whole document: every wall with its room and its curvature, then the cracks. */
	static void Build(const FGcRegionSpace& Space, FGcRegionFileReport& OutReport);
};
