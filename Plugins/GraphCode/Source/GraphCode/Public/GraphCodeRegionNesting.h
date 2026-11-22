// Copyright OraStudio. All Rights Reserved.

// A REGION INSIDE A REGION — `GC-SPATIAL-AUTHORING-DESIGN.md` §21.17, task R11.
//
// *"allows anyone seeing the region knows, what other region is inside this region"* — a READ want,
// and the `.gcw` already holds every polygon, so this is DERIVED and nothing is declared. §21.17's
// four reasons in one line each: a `contains:` line would put back the cross-reference §21.1b spent a
// format break removing, a declaration can be wrong where a measurement cannot, a declaration cannot
// notice that a region is a PRISM, and deriving it costs an author nothing.
//
// **NO NEW GRAMMAR, NO NEW VERB, NO NEW TOKEN** — §21.16's test for anything that wants one. This
// file adds a pure answer; `gcw_ask $Dungeon` renders it.
//
// **THE PRISM IS THE WHOLE POINT, and it is what a typed line cannot do.** A cellar whose FOOTPRINT
// sits inside the house is not inside the house, and a mezzanine at 300..600 inside a house of 0..600
// is. So the plan answer and the Z answer are computed separately and BOTH are reported: an entry
// reading *plan inside, overall disjoint* is a caller being told which of the two decided it, rather
// than being handed a verdict it cannot argue with.
//
// **A CLASSIFICATION AND NEVER A BOOLEAN** — §21.14a's ruling one surface over, and §20.22a's before
// it. A room poking 2 cm into its neighbour and a room built halfway inside it are the same bit and
// very different buildings, so the overlapping case carries how much, the way a crack carries its gap
// and occlusion carries its ratio.
//
// **`Contains` IS A FOURTH WORD AND IT WAS NOT RULED — taken here as a default, with the reason, per
// `GC-PRODUCT-SPLIT`'s own habit of saying which was taken.** §21.17 names three
// (`inside`/`overlaps`/`disjoint`) because it was written from the downward question. The measurement
// is symmetric — asking whether the Crypt is inside the Dungeon computes exactly what asking the
// reverse does — so a container answering `overlaps` would report a partial share where the truth is
// total containment the other way. Suppressing the direction costs a fact and buys one fewer word.
//
// **THE OVERLAP IS MEASURED OVER THE TESSELLATION, and both percentages come from that same
// tessellation** so they agree with each other. `FGcResolvedRegion::Area` is exact against arcs and is
// what `gcw_ask` reports as the region's area; on a bowed room the two differ by
// `TessellatedOutline`'s conservatism, which is under half a centimetre on the ordinary gently bowed
// wall (`FGraphCodeRegionSolver::ArcSegments`). Stated rather than left to be discovered by somebody
// dividing one by the other.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegion.h"

/**
 * How one region stands to another, in the direction the question was asked.
 *
 * `Inside` means the OTHER region is inside the one being asked about. `Contains` is the same
 * measurement read the other way. Both require the plan AND the Z range to be contained — a tower
 * whose footprint is inside the house but which pokes through its roof is `Overlaps`, and that is the
 * answer somebody wants rather than a rounding of it.
 */
enum class EGcRegionRelation : uint8
{
	/** They share no space at all. A shared WALL is this: two rooms that meet are not nested. */
	Disjoint,
	/** The other region lies wholly within this one. */
	Inside,
	/** This region lies wholly within the other one. */
	Contains,
	/** They share space and neither contains the other. `PlanOverlapCm2` says how much. */
	Overlaps,
};

GRAPHCODE_API const TCHAR* LexToString(EGcRegionRelation Relation);

/** One other region, measured against the one that was asked about. */
struct GRAPHCODE_API FGcRegionNesting
{
	/** The OTHER region's declared name. */
	FString Region;

	/** Plan and Z together — what a caller branches on. */
	EGcRegionRelation Relation = EGcRegionRelation::Disjoint;

	/**
	 * The footprints alone, with Z ignored entirely.
	 *
	 * REPORTED BESIDE `Relation` BECAUSE THE DISAGREEMENT IS THE INFORMATION: a cellar reads
	 * `Plan == Inside` and `Relation == Disjoint`, which says *directly below the house* in two fields
	 * and would say nothing at all in one.
	 */
	EGcRegionRelation Plan = EGcRegionRelation::Disjoint;

	/** Square centimetres the two footprints share, over the tessellation. Zero when they do not. */
	double PlanOverlapCm2 = 0.0;

	/** That overlap as a percentage of the OTHER region's plan area, and of this one's. */
	double PercentOfOther = 0.0;
	double PercentOfThis = 0.0;

	/** Do the two Z ranges meet at all? An unstated side reaches forever, so it meets everything. */
	bool bZOverlaps = false;

	/**
	 * How tall the shared slab is — **only when both sides of it were stated**.
	 *
	 * A shared range open at either end has no height, and writing one would tell a reader a number
	 * nobody authored (§21.18c's rule, one answer over). `bZOverlapBounded` false with `bZOverlaps`
	 * true is *they meet, and the shared part is unbounded*.
	 */
	bool bZOverlapBounded = false;
	double ZOverlapCm = 0.0;
};

/**
 * The region-in-region answer. Pure arithmetic over authored geometry — no world, no editor, no level.
 */
class GRAPHCODE_API FGraphCodeRegionNesting
{
public:
	/**
	 * One pair. `Subject` is the region asked about; `Other` is the one being classified against it.
	 *
	 * Refuses only what `Triangulate` refuses — a self-intersecting outline, which is the one shape
	 * every containment answer in this file is already undefined for.
	 */
	static bool Classify(const FGcRegionSpace& Space, const FGcResolvedRegion& Subject,
		const FGcResolvedRegion& Other, FGcRegionNesting& OutNesting, FString& OutRefusal);

	/**
	 * Every other region in the space, classified against this one and sorted for a stable answer.
	 *
	 * ORDER IS `Inside`, `Contains`, `Overlaps`, `Disjoint`, then by name — deterministic, and it puts
	 * the tree-walk answer (§21.17a's *what is in this dungeon*) at the top where a reader looks.
	 *
	 * **A REGION THAT DOES NOT RESOLVE BECOMES A NOTE AND NEVER A SILENT ABSENCE.** An open loop is
	 * exactly the file somebody is asking about, and dropping it would report *nothing is inside you*
	 * about a room the file does describe.
	 */
	static void ClassifyAll(const FGcRegionSpace& Space, const FGcResolvedRegion& Subject,
		TArray<FGcRegionNesting>& OutNesting, TArray<FString>& OutNotes);
};
