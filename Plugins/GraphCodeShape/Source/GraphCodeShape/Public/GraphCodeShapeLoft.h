// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "DynamicMesh/DynamicMesh3.h"

#include "GraphCodeShapeOutline.h"
#include "GraphCodeShapeValue.h"

/**
 * `loft` — THE SKIN BETWEEN TWO OUTLINES. `GC-SHAPE-GRAMMAR.md` §4 group 3; `Docs/NEXT.md` M5.
 *
 * **THE ONLY INSTRUCTION IN GROUP 3 THAT IS OURS, and the reason is a measured absence**: there is no
 * loft anywhere in GeometryScript or in `Generators/` — `AppendSweepPolygon` carries
 * `StartScale`/`EndScale`, so the engine can taper ONE cross-section and cannot morph section A into
 * section B. Its own step in the milestone table because all four hard parts are here rather than
 * behind a door: resample to a common count, rotate the seam, ease with `profile=`, and continue from
 * the shape's current top face.
 *
 * **THE AUTHOR NEVER MATCHES VERTEX COUNTS, AND THAT IS THE WHOLE OF §5.0 APPLIED TO THE OPERATION
 * THAT MOST TEMPTS OTHERWISE.** A 25-point circle skinned to a 4-point square is legal and means what
 * it looks like. Resampling KEEPS every point it was given and adds the difference down the longest
 * edges, so a corner never moves and a rounded rectangle never loses its fillets to a sampler.
 *
 * **THE SEAM IS THE ARTEFACT NOBODY EXPECTS AND EVERYBODY HITS.** Even at equal counts, pairing index
 * 0 to index 0 twists the surface whenever the two rings start at different angles; the whole strip
 * then spirals and reads as a mistake in the file rather than in the evaluator. So the
 * correspondence is ROTATED to the offset that minimises total edge length, and `twist=` is how an
 * author asks for the spiral back.
 */
namespace GraphCodeShapeLoft
{
	using FGcLoftVector = UE::Math::TVector<double>;

	/**
	 * Take the shape's TOP FACE off and hand back the ring it bounded, in loop order.
	 *
	 * **THE TOP FACE IS THE TRIANGLES LYING IN THE SHAPE'S TOPMOST HORIZONTAL PLANE, and stating it
	 * that plainly is what lets the refusal be exact.** Up is the one privileged direction in this
	 * grammar (§4.1: up exists and front does not), so *top* needs no argument; a shape whose top is
	 * not flat — an extrude under a varying `height=`, a dome — has no such face and is REFUSED
	 * naming what was found, rather than skinned from a ring that is half a lid.
	 *
	 * A cap adds no vertices of its own (`SweepGenerator.cpp:130`), so the lid is exactly the
	 * triangles all three of whose vertices sit in that plane, and removing them leaves the ring
	 * already in the mesh — which is why a chained loft is watertight without a weld.
	 */
	GRAPHCODESHAPE_API bool TakeTopFace(UE::Geometry::FDynamicMesh3& Mesh, double Tolerance,
		TArray<int32>& OutRing, FString& OutError);

	/**
	 * Every point kept, the difference added down the longest edges.
	 *
	 * **NOT AN ARC-LENGTH RESAMPLE, and the difference is a corner.** Sampling a closed outline at
	 * `i/K` of its perimeter lands wherever it lands, so a rounded rectangle comes back with its
	 * corners cut. Adding points to existing edges keeps every vertex the author's instructions
	 * produced and puts the new ones where the strip is coarsest, which is the only place they buy
	 * anything.
	 *
	 * `OutKept[j]` is where input point `j` ended up, which is what lets the SAME split be made in a
	 * mesh that already holds the ring.
	 */
	GRAPHCODESHAPE_API void Resample(const TArray<FGcLoftVector>& In, int32 Count,
		TArray<FGcLoftVector>& Out, TArray<int32>& OutKept);

	/**
	 * Turn `To` until the strip between the two rings is shortest, reversing it if it winds the other
	 * way.
	 *
	 * Both are already at the same count. The reversal is checked FIRST because a ring that winds
	 * backwards has no good rotation: every offset is a Möbius strip, and the minimum among them is
	 * simply the least bad one.
	 */
	GRAPHCODESHAPE_API void Correspond(const TArray<FGcLoftVector>& From, TArray<FGcLoftVector>& To);

	/**
	 * `Correspond`'s search without the move: whether `To` winds against `From` (and so is read
	 * backwards), then the rotation of the (reversed) `To` with the least centroid-relative squared
	 * distance. `Correspond` is this and then the move, with an identical result. Point `i` of the
	 * paired ring is `To[(i + Offset) % N]`, or `To[N - 1 - (i + Offset) % N]` when reversed.
	 */
	GRAPHCODESHAPE_API void CorrespondOffset(const TArray<FGcLoftVector>& From, const TArray<FGcLoftVector>& To,
		int32& OutOffset, bool& bOutReversed);

	/**
	 * **L0 — A DIVISION NEVER BREAKS CORRESPONDENCE** (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.8).
	 *
	 * Each ring of `Rings` is a station as WRITTEN. It is put into its partner's order (`Offsets[k]`,
	 * `Reversals[k]`, from `CorrespondOffset` against the ring below it), and then its paired edge `j` is
	 * cut into `StartPattern[j]` pieces at equal fractions: the start's own division, so point `i` is one
	 * feature on every ring. Nothing is resampled after it.
	 */
	GRAPHCODESHAPE_API void DivideTogether(TArray<TArray<FGcLoftVector>>& Rings, const TArray<int32>& StartPattern,
		const TArray<int32>& Offsets, const TArray<bool>& Reversals);

	/**
	 * The rings between the two ends, as fractions of the way across.
	 *
	 * Always `0` and `1`. A straight loft needs nothing between them — the strip IS the surface — so
	 * the stations only grow where `profile=` bends the transition or `twist=` turns it, and each
	 * gets the count its own deviation earns out of the header's chord tolerance, the same rule a
	 * circle's segment count comes from.
	 *
	 * **THE TWO ARGUMENTS ARE DIFFERENT DISTANCES AND SHARING ONE WOULD BE WRONG BOTH WAYS.** An
	 * easing bends a point along its own travel, so its error scales with `Reach` — how far that
	 * point moves. A twist carries the ring round its axis, so its error scales with `Radius` — how
	 * far the ring stands out from that axis. A tall narrow loft has a large reach and a small
	 * radius, and one number for both would over-tessellate the turn or under-tessellate it.
	 */
	GRAPHCODESHAPE_API void Stations(const FGcShapeProfile& Ease, double Twist, double Tolerance,
		double Reach, double Radius, TArray<double>& OutStations);

	/**
	 * Put an outline into an empty mesh as a ring, with its cap if the loft is to be closed.
	 *
	 * The case where a loft is the entry's FIRST depth instruction — `circle()` then
	 * `loft(to=$Square)`, which is the whole of *circle to rounded square in one instruction*. It
	 * exists so that the two cases meet at a ring of vertex ids and everything after them is one
	 * code path: a loft that continues from a top face must not build a different surface from one
	 * that starts an entry.
	 */
	GRAPHCODESHAPE_API bool Seed(UE::Geometry::FDynamicMesh3& Mesh, const TArray<FGcLoftVector>& Outline,
		const FGcLoftVector& Travel, bool bCapped, TArray<int32>& OutRing, FString& OutError);

	/** What a loft was asked for, once every argument has been read and resolved. */
	struct GRAPHCODESHAPE_API FSkin
	{
		/** Where the far end goes, already placed by `at=`. Ordered as the file wrote it. */
		TArray<FGcLoftVector> To;

		/** `profile=`, normalised to 0..1. NO STOPS AT ALL means a straight transition. */
		const FGcShapeProfile* Ease = nullptr;

		/** `twist=` in degrees, turned about the travel and reaching all of it at the far end. */
		double Twist = 0.0;

		/** The header's chord tolerance — what decides how many rings a curved transition earns. */
		double Tolerance = 0.5;

		/** `capped=` — the FAR end's lid. The near end is whatever the shape below already had. */
		bool bCapped = true;

		/** Case B: the ring came off a mesh, so its winding is the solid's and cannot be flipped. */
		bool bFromShape = false;

		/**
		 * L0 (§5.8): which points of the near ring were WRITTEN (true) and which a division inserted,
		 * one per point of the ring `Build` is given, in its order. Empty: every point is written.
		 */
		TArray<bool> FromWritten;

		/** Out, when set: the far ring's vertex ids in ring order and which of them were written. */
		TArray<int32>* OutTopRing = nullptr;
		TArray<bool>* OutTopWritten = nullptr;
	};

	/**
	 * Skin from a ring already in `Mesh` out to `Skin.To`, and cap the far end if asked.
	 *
	 * `Ring` is resampled IN THE MESH where the far end has more points — the edges are split, which
	 * keeps the shape below watertight without a weld and leaves its own vertices exactly where they
	 * were.
	 *
	 * **L0 (§5.8):** where the near ring was divided (`FromWritten`) and the far outline is written
	 * with the near one's written count, the two are paired on their written points and the far one is
	 * divided on its paired edges by the near one's own pattern — no `Resample`, no seam search after.
	 * Every other loft is built exactly as before.
	 */
	GRAPHCODESHAPE_API bool Build(UE::Geometry::FDynamicMesh3& Mesh, TArray<int32>& Ring,
		const FSkin& Skin, FString& OutError);

	/**
	 * The second derivatives of the NATURAL cubic spline through `(Knots[i], Values[i])` — second
	 * derivative zero at both ends, which is what "natural" means and the only end condition that
	 * asks the author for nothing more than the stations.
	 *
	 * **ONE TRIDIAGONAL SYSTEM PER COORDINATE, SOLVED BY ELIMINATION WITHOUT PIVOTING**: the matrix is
	 * strictly diagonally dominant for any rising knots (`2(h0 + h1)` against `h0` and `h1`), so the
	 * elimination cannot divide by something small. Two knots are a straight line and come back all
	 * zero; `Knots` must rise strictly and a caller that cannot promise it should not call.
	 */
	GRAPHCODESHAPE_API void NaturalSplineSecondDerivatives(const TArray<double>& Knots,
		const TArray<double>& Values, TArray<double>& OutSecond);

	/** The spline's value at `At`, inside the span that runs from `Knots[Span]` to `Knots[Span + 1]`. */
	GRAPHCODESHAPE_API double EvaluateNaturalSpline(const TArray<double>& Knots,
		const TArray<double>& Values, const TArray<double>& Second, int32 Span, double At);

	/** What a FAIRED loft was asked for, once every argument has been read and resolved. */
	struct GRAPHCODESHAPE_API FThrough
	{
		/**
		 * One placed outline per station, bottom first, each already at its height: every point of
		 * station `k` carries the same Z, and that Z is the knot the spline passes through. The
		 * start ring is not listed — it is the ring already in the mesh.
		 */
		TArray<TArray<FGcLoftVector>> Stations;

		/** Where the start ring stands: the top face's height for a chained loft, `0` for a fresh one. */
		double StartZ = 0.0;

		/** The header's chord tolerance — what decides how many rings each span earns. */
		double Tolerance = 0.5;

		/** `capped=` — the FAR end's lid. The near end is whatever the shape below already had. */
		bool bCapped = true;

		/**
		 * L0 (§5.8): which points of the start ring were WRITTEN (true) and which a division inserted,
		 * one per point of the ring `BuildThrough` is given, in its order. Empty: every point is written.
		 */
		TArray<bool> StartWritten;

		/** Out, when set: the last station's ring, vertex ids in ring order, and which were written. */
		TArray<int32>* OutTopRing = nullptr;
		TArray<bool>* OutTopWritten = nullptr;
	};

	/**
	 * `loft(through=…, heights=…)` — ONE skin from the ring already in `Mesh` through every station,
	 * **faired**: x and y are natural cubic splines of z through the stations' knots, so the side line
	 * has no slope or curvature break at any station, where a chain of `Build` calls has a break at
	 * each one. Rings are level, because z is the spline's own variable — the skin cannot fold back.
	 *
	 * This is `Build` over N rings, and everything `Build` does is done the same way: the ring is
	 * resampled IN THE MESH, the winding is read off the triangle already on it, each ring is
	 * corresponded to the one below, the strips are cut along their shorter diagonals, and the skin
	 * is shaded in ONE pass so no station shows a seam.
	 *
	 * **L0 (§5.8):** where the start ring was divided (`StartWritten`) and every station is written with
	 * the start's written count, the rings are paired on their written points (`CorrespondOffset`, ring
	 * to ring) and every station is divided by the start's own pattern (`DivideTogether`) — no
	 * `Resample`. Unequal written counts keep the old path exactly: there is no point identity to keep.
	 */
	GRAPHCODESHAPE_API bool BuildThrough(UE::Geometry::FDynamicMesh3& Mesh, TArray<int32>& Ring,
		const FThrough& Through, FString& OutError);

	// ---- B1: THE KEYED LOFT (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.5) --------------------------------------

	/**
	 * One ring of a keyed loft, as the runner hands it over: its reference polyline, already placed (a closed
	 * loop, no repeated first point), and its keys, whose `Point` indexes `Reference`. A ring with no named key
	 * is `R0` taken from the mesh (§5.5.1: an unnamed `R0` is resampled to `R1`'s count and turned to it).
	 */
	struct GRAPHCODESHAPE_API FKeyedRing
	{
		TArray<FGcLoftVector> Reference;
		TArray<FGcShapeKey> Keys;

		/** Where the ring stands along the loft: its knot (`through=`), or its plane's height (`to=`). */
		double Height = 0.0;

		/** How notes and refusals name it: `` `$S03` `` or `the start outline`. */
		FString Label;

		/** §5.5.5: a `loop=mirror` half, and the line it was closed across. */
		bool bMirror = false;
		FGcLoftVector MirrorA = FGcLoftVector::ZeroVector;
		FGcLoftVector MirrorB = FGcLoftVector::ZeroVector;

		/**
		 * E1 (`GC-SHAPE-LOFT-QUALITY-DIAGNOSIS.md` §4.1): the written `curve()` behind `Reference`, placed as it is, and each
		 * reference vertex's parameter on it. Empty for a `points()` ring (its polyline is its curve). Read only with `rails=`.
		 */
		FGcShapeWrittenCurve Curve;
		TArray<double> RefParams;
	};

	// ---- C: LINES ALONG THE LOFT (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.6, §6 "Slice C") -----------------------

	/**
	 * §5.6.4 "One fill, behind one function": the fills along the loft. `Monotone` is today's profile fill
	 * (`AuthoringCurve::Evaluate`: monotone cubic, C1, never past a stop) and every loft's unless it selects another.
	 * `Fair` (E4, `GC-SHAPE-LOFT-QUALITY-DIAGNOSIS.md` §4.3) is the natural cubic spline through the values, C2: a run of
	 * two or more equal values is held exactly and the spline is split at its ends with zero-slope ends (C1 there), a
	 * step (two values at one height) ends the curve on each side as `Monotone`'s does, and a swing past the
	 * neighbouring values is noted (N12), never clamped. No file can select it yet (§13 S8); `FKeyed::Fill` and the
	 * test switch `SetFillForTests` do.
	 */
	enum class EFill : uint8
	{
		Monotone,
		Fair
	};

	/**
	 * Values over height made ready for `FillAlong` once (review fix 7: the stops are built once per series, not on
	 * every call): the heights' own range and the stops of a profile over it. `Set` takes rising heights.
	 */
	struct GRAPHCODESHAPE_API FAlongCurve
	{
		TArray<FGcShapeProfile::FStop> Stops;
		double Low = 0.0;
		double Width = 0.0;

		/**
		 * E4 (`EFill::Fair`), made by `Set` with the stops: per stop, the fair spline's second derivative over the stops'
		 * own `t`; per stop `i`, 1 where the stretch from it to stop `i + 1` is a flat run, held exactly. Read only by `Fair`.
		 */
		TArray<double> Second;
		TArray<uint8> Held;

		void Set(TArrayView<const double> Heights, TArrayView<const double> Values);
		bool IsEmpty() const { return Stops.IsEmpty(); }
	};

	/**
	 * **EVERY INTERPOLATION OVER HEIGHT IN A LOFT WITH LINES GOES THROUGH THIS ONE FUNCTION** (§5.6.4): a line's
	 * profile, an unlined key coordinate, a span's shape sample, a key's direction. The value at `At`, held past either
	 * end; one height: that value; two equal heights are a step, as two stops at one `t` are.
	 */
	GRAPHCODESHAPE_API double FillAlong(const FAlongCurve& Curve, double At, EFill Kind = EFill::Monotone);

	/** The same over raw rising `Heights` and `Values`: builds the curve and reads it through the one above. */
	GRAPHCODESHAPE_API double FillAlong(TArrayView<const double> Heights, TArrayView<const double> Values, double At,
		EFill Kind = EFill::Monotone);

	/** Test-only: how many times `FillAlong` has run in this process (`EveryFillGoesThroughFillAlong`). */
	GRAPHCODESHAPE_API uint64 FillAlongCallsForTests();

	/**
	 * Test-only: a MARKED fill, `Mark` added to every value `FillAlong` returns (0 restores it). A spec swaps it in and
	 * checks that every line, unlined key, sample and direction moves: a fill that bypassed `FillAlong` would not.
	 */
	GRAPHCODESHAPE_API void SetFillMarkForTests(double Mark);

	/** Test-only: switch §5.6.4's end correction off (false) to measure what it corrects; true restores it. */
	GRAPHCODESHAPE_API void SetEndCorrectionForTests(bool bEnabled);

	/**
	 * Test-only, E4 (`GC-SHAPE-LOFT-QUALITY-DIAGNOSIS.md` §4.3): `bForce` true makes every loft with lines fill with `Kind`
	 * (`Fair` on, `Monotone` forced off); false restores each loft's own `FKeyed::Fill` (`Monotone` unless set: no file
	 * spelling selects `Fair` yet, §13 S8). A loft without `rails=` calls no fill, so it is never touched.
	 */
	GRAPHCODESHAPE_API void SetFillForTests(bool bForce, EFill Kind = EFill::Monotone);

	/**
	 * Test-only, loft quality (`GC-SHAPE-LOFT-QUALITY-DIAGNOSIS.md` §4.1, §4.2). Both run only in a loft with `rails=`.
	 * `bCurveColumns` is E1: the columns ride the written curve and a facet turns at most 8° (the count bounded by turning
	 * as well as distance). `bSharpCorners` is E2: a `@sharp` key's corner is not span turning. Both true by default; a spec
	 * switches one off to build what today's loft builds, and true restores it.
	 */
	GRAPHCODESHAPE_API void SetLoftQualityForTests(bool bCurveColumns, bool bSharpCorners);

	/** Test-only: E1's per-loft triangle cap (0 restores the built-in one); `LoftTriangleCap` reads the one in force. */
	GRAPHCODESHAPE_API void SetLoftTriangleCapForTests(int32 Cap);
	GRAPHCODESHAPE_API int32 LoftTriangleCap();

	/**
	 * §5.6.1 — ONE LINE OF `rails=`, as the runner read it: two profiles (`G: ($U, $V)`), one coordinate (`P: (X: $U)`)
	 * or a 3D path (`K: $Path`). A profile is read at `t = h / H`; a path is `(u, v, height above the loft's start)`,
	 * rising. `U` is the station outline's first coordinate as the loft places it, `V` its second.
	 */
	struct GRAPHCODESHAPE_API FGcLoftLine
	{
		FString Name;
		bool bHasU = false;
		bool bHasV = false;
		FGcShapeProfile U;
		FGcShapeProfile V;
		TArray<FGcLoftVector> Path;

		/** How the file wrote the line's value (`$GEdge`, `($Gz, $Gy)`), for refusals. */
		FString Text;
	};

	/**
	 * Test-only: what a keyed loft emitted, ring by ring, so a spec can follow one named column through the mesh. Filled
	 * when `FKeyed::Probe` (or the process-wide probe) is set; never read by the build.
	 */
	struct GRAPHCODESHAPE_API FKeyedProbe
	{
		TArray<FString> Order;                    // the names in the loft's cyclic order
		TArray<int32> KeyColumns;                 // per name: its column
		TArray<int32> Counts;                     // per span (name i to name i + 1): its interior count
		TArray<double> Heights;                   // per emitted ring: its height above the start
		TArray<int32> Stations;                   // per emitted ring: the ring `Rj` it is, or INDEX_NONE
		TArray<TArray<int32>> Ids;                // per emitted ring: the mesh vertex of each column
		TArray<TArray<FGcLoftVector>> Rings;      // per emitted ring: each column after welds and merges
		TArray<double> Knots;                     // C (§5.6.7): the knot heights above the start; empty without lines

		/**
		 * E4, IN: heights above the start where a spec wants the SURFACE, not the mesh: a loft with lines fills `DenseRings`
		 * with its point function (§5.6.3–§5.6.4) there, every column in the section plane, before welds and merges. At
		 * most 200,000 heights; none without lines. Never read by the build.
		 */
		TArray<double> DenseHeights;
		TArray<TArray<FVector2d>> DenseRings;
	};

	/** Test-only: a probe every keyed loft fills while it is set (null clears it), for specs that build whole programs. */
	GRAPHCODESHAPE_API void SetKeyedProbeForTests(FKeyedProbe* Probe);

	/** A keyed loft once every argument has been read: both forms, `through=` and `to=`. */
	struct GRAPHCODESHAPE_API FKeyed
	{
		/** `R0 … Rn`. `Rings[0]` with no names is the ring already in the mesh. */
		TArray<FKeyedRing> Rings;

		/** `through=` (faired, natural splines per column) or `to=` (`Build`'s easing and twist). */
		bool bThrough = true;
		const FGcShapeProfile* Ease = nullptr;
		double Twist = 0.0;

		double Tolerance = 0.5;

		/** `capped=` at the far end; and at the near end when the loft seeds its own named start. */
		bool bCapped = true;

		/** A fresh loft whose start outline is named: the builder clears the mesh and lays `R0` itself. */
		bool bSeedStart = false;

		/** Notes go here when set, each prefixed by `Where` (`line 9: 'loft(through=…)'`). */
		TArray<FString>* Notes = nullptr;
		FString Where;

		/** Out, when set: the far ring's DISTINCT vertex ids in ring order (welded ids once). */
		TArray<int32>* OutTopRing = nullptr;

		/**
		 * C (§5.6): the lines `rails=` gives. **EMPTY ON EVERY LOFT WITHOUT `rails=`, AND THEN NOTHING OF §5.6 RUNS**: the loft
		 * builds exactly as B1 leaves it (the ledger's §10). Non-empty only on a fresh `through=` loft with a named start.
		 */
		TArray<FGcLoftLine> Lines;

		/**
		 * C (§5.6.6): `loft(heights=(H), rails=…)` with no `through=`. `Rings[1]` is then the start outline again at `H`, a
		 * copy that is not an authored section: the start is the one shape sample, carried the whole length.
		 */
		bool bOneSample = false;

		/** Test-only: filled with what the loft emitted when set. */
		FKeyedProbe* Probe = nullptr;

		/**
		 * E4 (§5.6.4): the fill every interpolation over height uses in this loft, read only with `rails=`. `Monotone` unless
		 * set; no file spelling sets it yet (§13 S8), so today only `SetFillForTests` makes a loft `Fair`.
		 */
		EFill Fill = EFill::Monotone;
	};

	/**
	 * §5.5.2.1 — ONE CYCLIC ORDER FOR THE WHOLE LOFT. `RingNames[r]` is ring `r`'s names in ring order, in the
	 * reading order `R1 … Rn, R0`; empty lists are skipped. `RingGroups` (optional, one id per name) marks names
	 * that stand at one point on their ring: they are TIED, so their order among themselves is read from the order
	 * already built, and the first ring with no ties gives the order. Refuses L2 (another order, or a new name whose place
	 * is ambiguous) naming `Labels[r]`.
	 */
	GRAPHCODESHAPE_API bool OrderNames(const TArray<TArray<FString>>& RingNames, const TArray<FString>& Labels,
		TArray<FString>& OutOrder, FString& OutError, const TArray<TArray<int32>>* RingGroups = nullptr);

	/**
	 * §5.5.4 c — `Count` points spaced along the polyline `Span` (key to key) by the measure
	 * `μ = (1 − β)·arc/ℓ + β·turn/Θ`, `β = Θ/(Θ + 15°)`, each vertex's turning split half to each edge beside it
	 * and spread uniformly along it. `TurnIn`/`TurnOut` are the two keys' own turning angles (radians); half of
	 * each counts in this span.
	 */
	GRAPHCODESHAPE_API void SpaceSpan(const TArray<FGcLoftVector>& Span, double TurnIn, double TurnOut, int32 Count,
		TArray<FGcLoftVector>& Out);

	/**
	 * §5.5.4 d — THE TWO-WAY FIT TEST: every vertex of `Reference` within `Budget` of `Ring`, and the midpoint and
	 * both quarter points of every edge of `Ring` within `Budget` of `Reference`. `OutMiss` is the worst distance.
	 */
	GRAPHCODESHAPE_API bool SpanFits(const TArray<FGcLoftVector>& Reference, const TArray<FGcLoftVector>& Ring,
		double Budget, double* OutMiss = nullptr);

	/**
	 * THE KEYED LOFT (§5.5): names are columns, no rotation search between named rings, one count per span for
	 * the whole loft, carried names, keys that meet, the merge floor, welded caps, the L8 guard and the hard
	 * columns of §5.5.6. Entered only when a ring is named; an unnamed loft never reaches it.
	 */
	GRAPHCODESHAPE_API bool BuildKeyed(UE::Geometry::FDynamicMesh3& Mesh, TArray<int32>& Ring, const FKeyed& Keyed,
		FString& OutError);

	/**
	 * L8's crossing rule on one ring (§5.5.3 "What L8 counts as a crossing"): `Ids` logical vertex ids (welded or
	 * merged points share one), `Points` the ring in plan. True where the ring touches itself after consecutive
	 * repeats are reduced, or two edges meet that share no id; zero-length edges are skipped.
	 */
	GRAPHCODESHAPE_API bool KeyedRingCrosses(const TArray<int32>& Ids, const TArray<FVector2d>& Points, int32& OutA,
		int32& OutB);
}
