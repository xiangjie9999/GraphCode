// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigRegion.h"

class UDynamicMesh;

/**
 * THE RING CUT — the one pass in this product that changes the surface, and it DELETES NOTHING.
 *
 * **THAT IS THE WHOLE REASON IT SHIPS AND A RE-LAY DOES NOT** (ledger row 51, and the user's own two
 * questions behind it). A ring insert splits the edges a plane crosses and keeps every triangle that
 * was there; `FDynamicMeshAttributeSet::OnSplitEdge` carries UVs and normals through the split. A
 * remesh throws the surface away. So the geometry a FOLD needs is affordable on a mesh somebody
 * already authored, and the UV problem belongs to the re-lay alone.
 *
 * **`loops=` IS A TARGET, NOT A COMMAND** (grammar §11.9). A mesh that already has the rings is cut
 * nowhere — and that falls out of the method rather than needing a count: an edge lying ON the plane
 * does not CROSS it, so there is nothing to split. *Bind only* and *rings at joints* are the same
 * code path, which is what let the old A/B/C tiers collapse.
 */

/** What one region's cut did, or would do. */
struct GRAPHCODERIG_API FGcRigCutResult
{
	/** How many rings were asked for. */
	int32 Target = 0;

	/** How many planes actually split anything. A mesh that already met its target reports 0. */
	int32 Cuts = 0;

	/** How many of those were HALF-rings — `wedge=`, cut on the closing side only. */
	int32 HalfCuts = 0;

	/** The edges the cut ADDED — what a wireframe paints loud so a reader can see the new rings. */
	TSet<int32> AddedEdges;

	/** Worth saying: a target derived rather than stated, a region too thin to cut. */
	TArray<FString> Notes;
};

/**
 * What one region asks the cut for — the count, and the two keys that say where the rings GO.
 *
 * **`loops=` DECIDES HOW MANY AND THESE TWO DECIDE WHERE, which is why they ride the pass that already
 * exists rather than needing one of their own** (grammar section 12.10a). Neither adds a ring: a
 * crowded band and an even one cut the same number of planes.
 */
struct GRAPHCODERIG_API FGcRigCutRequest
{
	/** `loops=`, or what the angle and `folds=` derived. */
	int32 Target = 0;

	/**
	 * `crowd=0..1` — **the fraction of each ring's distance from the crease that is taken away**, which
	 * is the same shape `crease=` takes in the weighting and is what *bunch toward the crease* means as
	 * arithmetic. 0 is even. The ends do not move, because the crease is where the fold is and the
	 * region's edges are where it joins whatever is next door.
	 */
	double Crowd = 0.0;

	/** `wedge=n` — extra HALF-rings, cut on the closing side only. */
	int32 Wedge = 0;

	/**
	 * Which way the joint closes, in the mesh's own frame — `FGraphCodeRigRegion::ClosingSide`.
	 *
	 * Zero is legal and means nobody asked for half-rings; a `wedge=` with no side is refused rather
	 * than guessed, because the whole point of a half-ring is that it is on ONE side.
	 */
	FVector Closing = FVector::ZeroVector;
};

class GRAPHCODERIG_API FGraphCodeRigCut
{
public:
	/**
	 * **HOW MANY RINGS A JOINT NEEDS — `max(angle floor, 2*folds + 1)`** (grammar §5): a ring at the
	 * crease plus one either side per fold, and a floor from how far the joint actually travels.
	 *
	 * **THE ANGLE FLOOR'S DIVISOR IS DECLARABLE — `max_step=` per region, `max_step:` on the header,
	 * user-ruled 2026-09-17 (row 71)** — *"Max step should be declarable isn't it? And for default,
	 * just take a reasonable one"*. `MaxStep` unset takes `DefaultMaxStep()`.
	 *
	 * **AND THE DEFAULT IS MEASURED RATHER THAN CHOSEN.** Two readings by different routes put the
	 * knee of the curve at 21–24 degrees of fold per ring: shipped rigs in this tree spend 9 on a
	 * hand's fingers and 24 at worst (ledger §0b.1), and bending a leg gains 2.7 points from four
	 * rings to six and 0.4 from six to eight (§0b.3). **22 is the middle of that band.** It was 25,
	 * a ceiling used as a placeholder, so a bend now gets slightly more rings than it did.
	 */
	static int32 RingsFor(const FGcRigRange& Range, const TOptional<int32>& Folds,
		const TOptional<int32>& Loops, const TOptional<double>& MaxStep = TOptional<double>());

	/** Degrees of fold one ring may carry where nothing says otherwise. */
	static double DefaultMaxStep() { return 22.0; }

	/**
	 * Cut up to `Target` rings across the region, splitting edges and deleting nothing.
	 *
	 * **THE PLANES ARE EVENLY SPREAD AND NONE LANDS ON THE BOUNDARY** — `i/(Target+1)` — because a ring
	 * exactly on a region's edge is a ring shared with whatever is next door, and row 43's fade has
	 * nothing left to fade across.
	 *
	 * **AND THE CUT IS RESTRICTED TO THE REGION'S OWN TRIANGLES.** A plane is infinite: one through a
	 * knee also passes through the other leg, and through the belly. `SplitEdgesOnly` takes a triangle
	 * selection for exactly this reason.
	 */
	static bool CutRings(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span, int32 Target,
		FGcRigCutResult& Out, FString& OutRefusal);

	/**
	 * The same cut, told where the rings go.
	 *
	 * **THE HALF-RINGS GO IN THE GAPS NEAREST THE CREASE, and they are placed after the full rings
	 * rather than beside them.** `wedge=` is *density where it gathers, none wasted where it
	 * stretches* (grammar section 5), so the question it answers is which gap is closest to the fold —
	 * which cannot be asked until the full rings have been placed and crowded.
	 */
	static bool CutRings(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span,
		const FGcRigCutRequest& Request, FGcRigCutResult& Out, FString& OutRefusal);
};
