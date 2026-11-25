// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **RESOLUTION — AND THE FILE NEVER MENTIONS IT** (board row MV, `GC-SHAPE-GRAMMAR.md` §4 group 4,
 * §5).
 *
 * `roughen(scale=14.0)` and `[fade=60]` are both only as fine as the mesh they are drawn on, and
 * before this neither bought the vertices it needed: a plain box is eight of them, so a 60 cm ramp
 * had nothing to ramp across and was refused, and a Perlin field of 14 cm features on a shape whose
 * vertices are 240 cm apart displaced every vertex it was given and reported success.
 *
 * **THE ROW ASKED WHICH OF TWO SPELLINGS TO ADD AND THE CORPUS REFUSED BOTH.**
 *
 *   - **A header default cannot say it.** `GC-SHAPE-EXAMPLE.md` §9 puts two parts in ONE file whose
 *     noise differs five-fold — the `Sheet`'s finest is `scale=14.0` and the `Lip`'s is `scale=70.0`
 *     — and the `Lip` is the bigger object of the two. One number would either starve the sheet or
 *     spend the sheet's density on a 980 x 260 slab, which is the same reason `into:` is a default a
 *     build line may override and the asset kind is not on the header at all.
 *
 *   - **An argument on the instruction was already deleted, with its reason.**
 *     `GC-SHAPE-AUTHORING-DESIGN.md` §5.0's table cut `tessellate(level=)` as *a preparation step;
 *     `displace` needs density, so it asks for its own*, under the rule that governs the whole
 *     vocabulary: *an operation belongs in a `.gcshape` only if a different choice would produce a
 *     different shape that a person could want; where there is one right answer, the EVALUATOR does
 *     it and the file never mentions it.*
 *
 * **AND THERE IS ONE RIGHT ANSWER, BECAUSE THE LINE ALREADY CARRIES THE LENGTH.** `scale=` IS the
 * feature size and `[fade=]` IS the ramp width. A field of wavelength S sampled at a spacing wider
 * than S/2 does not come back coarser, it comes back arbitrary — the bump that lands between two
 * vertices is not there at all — so nobody is choosing a look, they are choosing whether the number
 * they wrote means anything. **The author's control over what this costs is `scale=` itself and the
 * build line's `[triangles=]`**, which §5.0 already gave the job of being *a property of the OUTPUT,
 * not a step*.
 *
 * **REFINE-ONLY, WHICH IS WHAT MAKES A SECOND LINE SAFE.** An edge already short enough is left
 * exactly as it is, so the waterfall sheet's coarse `scale=52.0` line, arriving after its fine
 * `scale=14.0` one, finds every edge short enough and adds nothing — it can never coarsen away the
 * displacement the finer line put in.
 */
namespace GraphCodeShapeRefine
{
	/**
	 * **THREE SAMPLES PER FEATURE, AND THE ARITHMETIC IS WHY IT IS NOT TWO.** At a spacing of S/2 a
	 * bump can be sampled at both of its zero crossings and vanish entirely, so two is the count at
	 * which the number in the file stops meaning anything. At S/3 the worst-placed peak has a sample
	 * within S/6 of it, which for one lobe of the field is half its height or better — a bump that
	 * reads as a bump. Four would give 71% and cost 78% more triangles, since this number is squared
	 * on the way to a count.
	 */
	constexpr double SamplesPerFeature = 3.0;

	/**
	 * **THE NET, AND IT IS NOT A BUDGET.** `roughen(by=2.0, scale=0.5)` on a 900 cm block is a
	 * perfectly grammatical line asking for three hundred million triangles, and the ask has to be
	 * refused before it is cut rather than after. The corpus's own finest ask is §9's sheet at
	 * `scale=14.0` across an 840 cm crest, which PRICES near 80,000 — **not measured, because that
	 * file still stops at row MF and so has never been built whole**; the largest ask this code has
	 * actually run is the acceptance pillar's shaft. **A budget lives on the build line
	 * (`[triangles=]`) and is a CEILING the output is simplified to; this is the wall a runaway
	 * hits.**
	 */
	constexpr int32 MostTriangles = 1000000;

	/**
	 * **THE SECOND NET, ON WHAT WAS ACTUALLY CUT, AND IT IS THE ONE THAT HOLDS.**
	 *
	 * `MostTriangles` above is a PROJECTION, and 2026-08-24 measured how far a projection can be
	 * from the truth on the shapes this grammar makes. A disc of radius 232 under
	 * `roughen(scale=34.0)`: the estimate said about 2,600 triangles for the top face and the build
	 * delivered **123,472** — roughly fifty times under, and the whole disc came back 510,092
	 * against about 6,000. The same disc at `scale=8.0` projected about 107,000, sailed through the
	 * net, and took a 32 GB editor to 21 GB and climbing until it was killed from outside.
	 *
	 * **THE ESTIMATE IS NOT FIXABLE BY A BETTER FORMULA, WHICH IS WHY THIS EXISTS INSTEAD.**
	 * `GcShapeProject` is `Area / (0.5 * Target^2)` — the count a region of that area lands on when
	 * its triangles are WELL SHAPED. A `circle()` capped by `extrude()` is a triangle FAN: 56
	 * wedges each 232 cm long and tapering to a point, and splitting a sliver cascades in a way its
	 * area cannot predict, because the work is driven by the long edge while the area is set by the
	 * short one. This file's own comment already says the corpus is built this way — *every shape in
	 * this grammar that was extruded or swept is long and thin* — so the pathological case is the
	 * ordinary one, not the exception.
	 *
	 * **MV-Q TOOK THE CASCADE OUT AND LEFT THE QUANTIZATION, SO THIS STILL EXISTS AND FOR A SMALLER
	 * REASON.** Bisection cannot manufacture slivers, so the ~47x on that disc is gone; what no
	 * halving scheme can avoid is landing between half a target and a whole one, which is a fixed
	 * factor of about two and a half in count. A wall that cannot be wrong is still cheaper than a
	 * formula that is only usually right, and the running count stays.
	 *
	 * **A COUNT OF WHAT EXISTS CANNOT BE WRONG.** It costs one O(1) compare per split.
	 *
	 * **THE NUMBER IS MEASURED, NOT CHOSEN.** The largest thing this project has actually built is
	 * the spiral tower at **2,026,534** triangles, which must keep working; the runaway was in the
	 * tens of millions. Four million is a shade over twice the known-good ceiling and roughly a
	 * tenth of the failure, so it clears real work and stops a runaway while the machine still has
	 * its memory. **Raise it only against a measurement**, and note that `GcShapeMostPasses` is no
	 * backstop at all: twenty-four halvings of a 228-triangle disc is 3.8 billion.
	 */
	constexpr int32 MostTrianglesBuilt = 4000000;

	/** Edges no longer than this carry a feature of that size. Zero where nothing has to be read. */
	GRAPHCODESHAPE_API double TargetFor(double Feature);

	/**
	 * Refine `Where` until no edge of it is longer than a feature of `Feature` cm needs, and change
	 * nothing else.
	 *
	 * **`Where` EMPTY MEANS THE WHOLE SHAPE**, matching `FGcShapeSelection::bWhole`.
	 *
	 * **DOOR: `FDynamicMesh3::SplitEdge`, EACH TRIANGLE'S LONGEST EDGE, HALVED, UNTIL NONE IS TOO
	 * LONG.** MEASURED 2026-08-24 in `DynamicMesh3_Edits.cpp`: a split gives each new triangle the
	 * group of the one it came from and calls `Attributes()->OnSplitEdge`, so §5's tags — which ARE
	 * polygroups — and the UV, normal and material overlays all survive it. **That is what the row
	 * depends on**, since a refinement that renumbered a group would break every bracket downstream
	 * of it, quietly.
	 *
	 * **`LONGEST` IS ROW MV-Q'S ONE WORD, AND IT WAS MEASURED RATHER THAN PREFERRED.** The rule was
	 * *every edge longer than the target*, and on the friendliest input this code will ever see — a
	 * plain 200 cm box, six flat faces, no slivers — that delivered **7,816** triangles where the
	 * area asks for about 1,200. Splitting all three edges of a right-isoceles triangle leaves a
	 * MEDIAN of 224 where a proper subdivision leaves 141, so two of every four children need a whole
	 * extra level and a level costs 4x. Bisecting the longest edge alone leaves two triangles SIMILAR
	 * to the original at exactly half the area, and the count falls to ~3,000 — the quantization a
	 * halving scheme cannot avoid, and nothing else.
	 *
	 * **THE SHAPE OF THE RESULT MATTERS MORE THAN THE COUNT.** Rivara's theorem bounds every
	 * descendant's smallest angle at half the original's, so bisection can never manufacture a sliver
	 * out of a well-formed triangle; the old rule had no such bound and did. A noise field sampled on
	 * a mesh dense in one direction and sparse in the other does not read as lumps, it reads as
	 * streaks — which is what a roughened `circle()` cap, a triangle FAN, was doing.
	 *
	 * **AND IT SETTLES THE `FSelectiveTessellate` COMPARISON RATHER THAN RE-OPENING IT.** That door's
	 * concentric-rings pattern subdivides each triangle uniformly at its LONGEST edge's count, so the
	 * 14 x 520 cm sliver an `extrude` makes of a Doric shaft comes back as 60 x 60 sub-triangles
	 * where 2 x 60 is what the noise asked for — about 165,000 triangles against about 2,800.
	 * **That number was PRICED and not measured and still is**; what changed is the other side of the
	 * trade. Bisection is now priced by AREA times a fixed quantization, with a proved angle bound,
	 * so the shipped path no longer has the 6.5x defect that was the only evidence in the pattern's
	 * favour. Splitting costs the AREA, a pattern costs the longest edge squared, and every shape in
	 * this grammar that was extruded or swept is long and thin.
	 *
	 * **THE SELECTION MUST BE READ AGAIN AFTER THIS RETURNS.** Triangle IDs survive a split — the
	 * mesh is not compacted — but the triangles the split MADE are in no list a caller was holding,
	 * so a stale selection would displace half of a refined region and leave a seam through the
	 * middle of it. What chose the region is untouched: tags are groups the split inherits, and
	 * `above=`/`below=` are heights.
	 *
	 * `Subject` is the line as the author would recognise it — `roughen(scale=0.5)`,
	 * `taper() [fade=2.0]` — because the only thing that can go wrong here is that the number is
	 * finer than the shape can afford, and the refusal has to name which number.
	 *
	 * `TriangleCeiling` is the running wall, defaulted to `MostTrianglesBuilt` and taken as a
	 * parameter for ONE reason: a spec that had to reach four million triangles to watch the guard
	 * fire would allocate more than the suite should and take longer than anyone will keep running
	 * it. A test passes a small one and reads the same refusal the real ceiling produces. **No
	 * caller in the product passes it** — the default is the shipped number, so there is no second
	 * place for it to drift to.
	 */
	GRAPHCODESHAPE_API bool ForFeature(UE::Geometry::FDynamicMesh3& Mesh,
		const TArray<int32>& Where, double Feature, const FString& Subject, FString& OutError,
		int32 TriangleCeiling = MostTrianglesBuilt);

	/**
	 * **THE SAME REFINEMENT FOR A FEATURE STRETCHED PER AXIS — G8, 2026-11-18.** `roughen(scale=(0.6, 6, 0.6))`
	 * is grain ten times longer along y than across it, and the scalar door above priced it by its SHORTEST
	 * component in every direction: MEASURED by the debris prototyper, a smashed board's torn faces went to
	 * 54,778 triangles for detail whose long axis needed a tenth of that. Here an edge is measured in the field's
	 * own LATTICE units (each component over that axis's feature size) and split until none is longer than a
	 * third of one lattice unit, so the long axis buys a tenth of the edges. The same longest-edge bisection, in
	 * that space; the projection and the running wall count the same way.
	 *
	 * **A FEATURE EQUAL ON ALL THREE AXES IS THE SCALAR DOOR, CALLED AS IT IS**, so every one-number `scale=` builds
	 * byte for byte what it built before.
	 */
	GRAPHCODESHAPE_API bool ForFeature(UE::Geometry::FDynamicMesh3& Mesh,
		const TArray<int32>& Where, const FVector3d& Feature, const FString& Subject, FString& OutError,
		int32 TriangleCeiling = MostTrianglesBuilt);

	/**
	 * **`[fade=]`'S DENSITY, BOUGHT ONLY WHERE THE RAMP IS — G19, 2026-11-18.** A fade's weight is
	 * `min(1, depth / Fade)`, so it varies only within `Fade` of the selection's border and is 1 everywhere deeper;
	 * the line used to buy `Fade / 3` edges over its WHOLE selection whenever the fade was finer than its own
	 * feature, which on a 27 x 6 cm break cutter at `fade=0.35` was the 40x the wood-damage author measured. Here
	 * a triangle is split only while its longest edge is over a third of `Fade` and some corner lies within
	 * `Fade` plus that edge of the border (`DepthAt`, the distance `GraphCodeShapeSelect::Reach` measures); the
	 * rest keeps what the line's own feature bought. The same conforming longest-edge bisection as `ForError`.
	 */
	GRAPHCODESHAPE_API bool ForFade(UE::Geometry::FDynamicMesh3& Mesh, const TArray<int32>& Where, double Fade,
		TFunctionRef<double(const UE::Math::TVector<double>&)> DepthAt, const FString& Subject, FString& OutError,
		int32 TriangleCeiling = MostTrianglesBuilt);

	/** Refine a scalar field by interpolation error, with conforming longest-edge propagation. */
	GRAPHCODESHAPE_API bool ForError(UE::Geometry::FDynamicMesh3& Mesh,
		const TArray<int32>& Where, TFunctionRef<bool(int32)> NeedsDetail,
		const FString& Subject, FString& OutError, int32 TriangleCeiling = MostTrianglesBuilt);

	/**
	 * Cut a region's border into the surface: wherever `Volume`'s surface crosses `Mesh`'s, an edge
	 * is inserted, so a face selection by centre follows the region and not the triangles under it
	 * (user-ruled 2026-09-14). Exact, so running it again adds nothing. False where the engine could
	 * not insert a crossing; that stretch keeps whole triangles.
	 */
	GRAPHCODESHAPE_API bool ForBoundary(UE::Geometry::FDynamicMesh3& Mesh,
		const UE::Geometry::FDynamicMesh3& Volume);

	/**
	 * The fewest triangles a balance may add, however small its operands: a 76-triangle cutter gained up to
	 * 1,950 in the diagnosis's port (`C1000`), so a floor below that would stop the very case the pass is for.
	 */
	constexpr int32 LeastBalanceAllowance = 4000;

	/**
	 * **THE BALANCE'S ALLOWANCE IS CAPPED BY THE OPERANDS' OWN SIZE**: half of the triangles the two hold, and
	 * never under `LeastBalanceAllowance`. So the balanced operands are at most about one and a half times what
	 * the first cut ran over, and a re-cut's boolean costs at most about that much. Every fixture the port ran
	 * stayed well inside it (A400 gained 656 of 8,038 allowed; W800t 6,518 of 9,648).
	 */
	GRAPHCODESHAPE_API int32 BalanceAllowanceFor(int32 TrianglesA, int32 TrianglesB);

	/**
	 * What `BalanceForBoolean` is asked, and what it did. The asks are the measured defaults of
	 * `Docs/GC-SHAPE-BOOLEAN-ON-LOFTS-DIAGNOSIS.md` §5.1; a spec may tighten them, the product does not.
	 */
	struct FBooleanBalance
	{
		/** A triangle the other operand's edges cross more often than this is split. Clean at 8 and under, failing at 20 and over (§3.2). */
		int32 MostCrossings = 4;

		/**
		 * A triangle is never split once its longest edge is no longer than this many times the finer
		 * operand's shortest edge where the two cross: for each crossing, the shorter of the two triangles'
		 * shortest edges AS THE OPERANDS CAME, and the median of those over the crossings the triangle
		 * holds. Without a floor the two operands race each other down: measured at 1 crossing, both
		 * reached the absolute floor and the inside-out triangles went UP from 9–32 to 56–381 (§5.1).
		 */
		double FloorFactor = 1.5;

		/** The first operand, then the second, at most this many times over: the second may now take the first's new edges. */
		int32 Rounds = 3;

		/**
		 * How many triangles the two operands may gain between them. Zero is the default,
		 * `BalanceAllowanceFor`: half of what they hold, and at least `LeastBalanceAllowance`. The build's
		 * re-cut passes a smaller number where the file's boolean budget has less room left than that.
		 */
		int32 MostAdded = 0;

		/** How many of each operand's ORIGINAL triangles were split, the first operand's then the second's. */
		int32 Split[2] = { 0, 0 };

		/** How many triangles each operand gained. */
		int32 Added[2] = { 0, 0 };

		/** The most crossings one triangle of each operand held before anything was split. */
		int32 Worst[2] = { 0, 0 };

		/** True where it stopped at `MostAdded` (or `MostTrianglesBuilt`) with triangles still over `MostCrossings`. */
		bool bSpent = false;
	};

	/**
	 * **BEFORE A BOOLEAN, MAKE THE TWO OPERANDS ABOUT AS FINE AS EACH OTHER WHERE THEY CROSS, WITHOUT
	 * MOVING EITHER SURFACE** (`GC-SHAPE-BOOLEAN-ON-LOFTS-DIAGNOSIS.md` §5.1, stage 1).
	 *
	 * The engine's cut fails where one triangle takes a long run of the other operand's crossings: it
	 * pokes them in one at a time and leaves triangles of no height, some inside out, which the
	 * keep/delete pass cannot place (§4.2–4.4). This bisects the longest edge of every triangle that
	 * takes more than `MostCrossings`, following the longest-edge chain as `ForError` does, until none
	 * does or its edges are down to the other operand's.
	 *
	 * **WHAT IT GUARANTEES.** Every new vertex is the midpoint of an existing edge, so no vertex leaves
	 * the surface and volume, bounds and every face tag stay put: `SplitEdge` carries polygroups, UVs,
	 * normals, material ids and the attached layers, and a hard-edge mark stays on both halves
	 * (`FGcShapeHardEdgeAttribute::OnSplitEdge`). No edge is flipped. Only triangles the other operand
	 * crosses, and the neighbours their longest edges lead to, change. An edge of `topology=preserve`
	 * faces is never split.
	 *
	 * **WHAT STOPS IT.** The floor above; an absolute floor of 64 times the boolean's snap tolerance;
	 * `Rounds`; and `MostAdded`. Each pass splits a triangle at most once.
	 *
	 * `A` is the boolean's first operand and `B` its second, both in the space the boolean will read
	 * them in. True where anything was split.
	 */
	GRAPHCODESHAPE_API bool BalanceForBoolean(UE::Geometry::FDynamicMesh3& A, UE::Geometry::FDynamicMesh3& B,
		FBooleanBalance& Balance);
}
