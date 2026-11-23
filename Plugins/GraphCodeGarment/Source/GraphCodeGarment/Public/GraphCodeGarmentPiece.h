// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * A piece of a garment cut flat (GC-GARMENT-PLAN.md P2D, the user 2026-10-07: "i thought everything is designed in 2d, then
 * thickness and triangles later?"). Its flat shape is the design and the cloth's rest; `Placed` is only where it starts on the body
 * before Chaos sews and hangs it; thickness and the final triangles come after. Drafted by the build from the author's words — never
 * sewn by hand (user ruling 2026-09-19: panels and seams are build output).
 *
 * Shared between the drafting (GraphCodeGarmentPiece.cpp, claude/main) and the pattern-to-Chaos carrier
 * (GraphCodeGarmentPatternCarrier, codex): one vertex = one flat point and one start point; triangles index both.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPiece
{
	/** What it is, for notes: `Collar`, `LapelR`. */
	FString Name;

	/** The cut, in centimetres, one point per vertex. */
	TArray<FVector2f> Flat;

	/** Where each vertex starts on the body, before the cloth is hung; the same count as `Flat`. */
	TArray<FVector> Placed;

	/** Indices into `Flat` and `Placed`, wound so the cloth's outside faces away from the body. */
	TArray<FIntVector3> Triangles;

	/** Named runs of its boundary, each in order along the edge: `Neck`, `Outer`, `End0`, `End1`. */
	TMap<FString, TArray<int32>> Edges;

	/**
	 * A run of `Edges` sewn gathered (GC-GARMENT-DRAFT-PLAN.md D5b): the places along that run, in order, its first and last among them,
	 * where a gathered seam welds it — a skirt's top row at its folds' valleys; the points between are the gathers' crests, sewn to nothing.
	 * Listed per piece, so a lattice and the cloth cut finer from it each say their own. A run not listed is welded at every point.
	 */
	TMap<FString, TArray<int32>> Gathered;

	/**
	 * A line the cloth turns on: a collar's roll line, a lapel's, a hem's. `Hinges` are its vertex pairs, in order along it;
	 * `AngleDeg` how far it is turned (0 flat, 180 right back over itself). Pressed (`press($X)` after `fold($X)`), the iron set
	 * that turn: it is the hinges' rest and held hard. Unpressed, the cloth rests flat there — its cut and its fabric shape the
	 * roll — and the angle only says how `Placed` lays it to start (agreed with codex, 2026-10-07).
	 */
	struct FFold
	{
		TArray<FIntPoint> Hinges;
		float AngleDeg = 0.f;
		bool bPressed = false;
	};
	TArray<FFold> Folds;

	/** The warp's direction in `Flat`. */
	FVector2f Grain = FVector2f(0.f, 1.f);

	/** The cloth's thickness, centimetres, for the shell grown last. */
	float ThicknessCm = 0.f;

	/**
	 * E4 (GC-GARMENT-DRAFT-PLAN.md): how many times its cloth's bending each point bends with — a turned hem's layers, an interfaced band —
	 * one per vertex, or empty for its cloth's own everywhere. The user, 14 Oct: "usually the collar, basically all the ends of the garment is
	 * stiffer, that restraint the foldings".
	 */
	TArray<float> BendScale;

	/** The fabric's name, as on the garment's panels. */
	FString Fabric;

	/**
	 * Where a point goes on the body (GC-GARMENT-SURFACE-STEPS.md step 1): the span of the garment's measure it lies in, its depth along that
	 * span's axis in centimetres, and its angle round it in radians (-π to π). Two numbers in one span's chart, so 2D; the last step lays the
	 * point there.
	 */
	struct FPlace
	{
		int32 Span = INDEX_NONE;
		float T = 0.f;
		float Theta = 0.f;
	};

	/** One per point of `Flat`, or empty: a piece drafted before places were kept. */
	TArray<FPlace> Places;

	/** One layer's thickness at each point, centimetres; empty, `ThicknessCm` everywhere. */
	TArray<float> ThicknessAt;

	/**
	 * Layers stacked on the cloth's own layer at each point, on its front (outside) and its back (inside) - a hem turned in is a back layer
	 * within its turned depth. Each one per point, or empty for none. Numbers until the last step; nothing is grown from them before.
	 */
	TArray<uint8> FrontLayers;
	TArray<uint8> BackLayers;
};

/**
 * Where a piece is sewn: one of its edges to another piece's edge, or to the garment's own 3D sheet (while the rest of the garment is
 * still laid in 3D). `SheetPoints` are where that seam is held, in order, one per point of the piece's edge: the MIDDLE of the cloth
 * it is sewn to (its sheet half its thickness out, as a settle simulates), the very points the piece's edge was cut to the length of —
 * one canonical attachment (codex, 2026-10-07). On the built surface a piece's `Placed` is its panel's sheet; handed to the carrier
 * (`FGcGarmentCloth::HangPiece`) it is the middle, and its sewn edge starts exactly at `SheetPoints`.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPieceSeam
{
	FString Piece;
	FString Edge;

	/** The other side: a piece's name, or empty for the garment's sheet. */
	FString OtherPiece;
	FString OtherEdge;
	bool bReversed = false;

	/**
	 * `sew($A, $B, match=ease)` below `pattern()`: the two edges are cut to different lengths on purpose — a back shoulder eased onto
	 * the front — and are sewn point for point, each keeping its own cut; the hang takes the ease up. False: the two lie flat as long
	 * as each other walked end to end, within 1 %. Either way both cuts are kept; how the carrier accepts each step is its own contract
	 * (GC-GARMENT-NOTCH-PLAN.md C0, open 13:20Z).
	 */
	bool bEased = false;

	/**
	 * `sew()` below `pattern()`: `SewPieces` accepted it end to end (walked within 1 %, or `match=ease`) and said its worst step; the carrier
	 * takes each step as cut (GC-GARMENT-NOTCH-PLAN.md C2a). False — a collar's seams — each step within 1 % or 0.001 cm, or refused.
	 */
	bool bPatterned = false;
	TArray<FVector> SheetPoints;

	/**
	 * D5b: a gathered seam when above 0 — each side welded at its run's `Gathered` places (every point where its piece lists none), point
	 * for point, and each welded step's cut on this side, its edges summed, `GatherRatio` times the other's (1 %): a skirt cut 250 % fuller
	 * than its waistband, 2.5. 0: a plain seam, every point welded, its steps as long as each other's.
	 */
	double GatherRatio = 0.0;
};

namespace GcGarmentPiece
{
	/**
	 * The places a seam welds, as indices into each side's run (`Edges`), in pairs along it, the other side's read the way the seam runs
	 * (`bReversed`): every point of both for a plain seam; for a gathered one (`GatherRatio` > 0) the places each piece's `Gathered` lists
	 * for that run, else every point. Refused when a run is missing, a list is not in order from the run's first point to its last, or the two
	 * sides weld different counts. A seam to the sheet welds its run's every point to `SheetPoints`; the other side's places are then theirs.
	 */
	GRAPHCODEGARMENT_API bool SeamWelds(const FGcGarmentPiece& One, const FGcGarmentPiece* Other, const FGcGarmentPieceSeam& Seam,
		TArray<FIntPoint>& OutPlaces, FString& OutRefusal);

	/** B2: measurements of the lapel drawn turned, then reflected onto the original flat front. No implicit gorge angle. */
	struct FLapelDraft
	{
		FString Neck = TEXT("Neck"), Shoulder = TEXT("Shoulder"), Armhole = TEXT("Armhole");
		FString Side = TEXT("Side"), Hem = TEXT("Hem"), Front = TEXT("FrontEdge");
		double RollOutCm = 0., BreakUpCm = 0., GorgeDownCm = 0., LapelWidthCm = 0., NotchCm = 0.;
		/** From the chest-facing perpendicular to the roll line toward its neckward direction, in the turned drawing. */
		double GorgeAngleDeg = TNumericLimits<double>::Max();
	};

	/** Construction points from the production draft, for its flat/turned drawing and seam checks. */
	struct FLapelConstruction
	{
		FVector2d NP = FVector2d::ZeroVector, RP = FVector2d::ZeroVector, BP = FVector2d::ZeroVector;
		FVector2d GCut = FVector2d::ZeroVector, GDraw = FVector2d::ZeroVector;
		FVector2d NCut = FVector2d::ZeroVector, NDraw = FVector2d::ZeroVector;
		FVector2d LPCut = FVector2d::ZeroVector, LPDraw = FVector2d::ZeroVector;
		FVector2d Neckward = FVector2d::ZeroVector, Chestward = FVector2d::ZeroVector;
	};

	/**
	 * Extend the six named boundaries of a simple front with a deliberately straight paper lapel. The width is measured
	 * perpendicular to the roll line at LP. Preserve the retained outline and material, remesh flat with edges <= 0.5 cm,
	 * and leave Placed empty for arrangement. The unpressed 180-degree roll is a chain of interior mesh edges.
	 * A refusal leaves Out empty and never changes Front; Construction retains any successfully constructed points.
	 */
	GRAPHCODEGARMENT_API bool DraftLapel(const FGcGarmentPiece& Front, const FLapelDraft& Draft, FGcGarmentPiece& Out,
		FLapelConstruction& Construction, FString& OutRefusal);

	/** One named run of a closed outline, in centimetres, its points in order along it. */
	struct FOutlineRun
	{
		FString Name;
		TArray<FVector2d> Points;
	};

	/**
	 * A flat piece meshed from its outline (GC-GARMENT-DRAFT-PLAN.md S2a-3), the lapel's meshing without its roll. The runs go in order round
	 * one closed outline: each run's last point is the next run's first, and the last run's last is the first run's first. The caller samples
	 * them — so a seam's two sides can share one count — and they are NOT resampled. Inside, a triangular lattice `SpacingCm` apart, kept at
	 * least 0.45 times `SpacingCm` off the outline; constrained Delaunay to the boundary, the odd-winding fill; edges longer than 1.5 times
	 * `SpacingCm` are split until none are (at most 12 passes, as the lapel's). `Out.Flat`, `Out.Triangles` (wound so a triangle's flat signed
	 * area has the sign of `Winding`, positive counter-clockwise) and `Out.Edges[Name]`, that run's vertex ids in order, both ends included;
	 * `Placed` is left empty. Refused, `Out` left empty, when the outline is not simple, a run has fewer than 2 points, consecutive runs do not
	 * share their end point, the mesh does not cover exactly the outline's area, or a boundary edge is lost or duplicated; also when two runs share
	 * a name, or a step along a run is longer than 1.5 times `SpacingCm` (the refinement would cut it, and a boundary edge cannot be cut).
	 */
	GRAPHCODEGARMENT_API bool MeshOutline(const TArray<FOutlineRun>& Runs, double SpacingCm, int32 Winding, FGcGarmentPiece& Out,
		FString& OutRefusal);

	/**
	 * How a collar is drafted (GC-GARMENT-PLAN.md P2D slice 1), centimetres: one column per point of the neckline it is sewn to,
	 * each running across the collar from the neck edge to the outer edge, every column the same rows — the stand, a half round
	 * where it rolls, the fall.
	 */
	struct FCollarDraft
	{
		/** The neckline, in order from one end to the other, as the sheet holds it. */
		TArray<FVector> Neck;

		/** How wide the collar is at each neckline point, stand and fall: the author's profile read along it. */
		TArray<double> Width;

		/**
		 * How far it stands up the neck before it turns, to its ends, as a shirt collar's band does; a column too narrow to stand that far,
		 * turn and fall a step stands what it can, and its fall may end on the band - a leaf's raked front edge.
		 */
		double StandCm = 3.0;

		/** Or how far each column stands (`stand=$Profile`, read round the ring it is sewn to): taller at the centre back, as collars are drafted. */
		TArray<double> StandAt;

		/** The roll's half round: half the gap between the stand and the fall lying on it. */
		double RollRadiusCm = 0.0;

		/** Steps no longer than this along the stand and the fall. */
		double SpacingCm = 1.0;

		/**
		 * The collar as it is laid on where it is to lie — its middle surface, vertex `K·Rows + S` column K's row S, row 0 the neckline
		 * (`Neck`): its roll row and its last row are what the stand's top and the fall's outer edge are cut as long as.
		 */
		TArray<FVector> Middle;
	};

	/** What a column's rows are: its distance from the neck edge at each row, and the row it rolls on. */
	struct FCollarRows
	{
		/** `Along[K][S]`: column K's row S, centimetres from the neck edge along the column. */
		TArray<TArray<double>> Along;
		/** `Stand[K]`: how far column K stands. */
		TArray<double> Stand;
		int32 Roll = 0;
		int32 RoundSteps = 6;

		int32 Rows() const { return Along.IsEmpty() ? 0 : Along[0].Num(); }
	};

	/** The rows of `Draft`'s columns. False with `OutRefusal` when its stand is no stand or leaves no fall over it. */
	GRAPHCODEGARMENT_API bool CollarRows(const FCollarDraft& Draft, FCollarRows& Out, FString& OutRefusal);

	/**
	 * The collar cut as a collar with a separate stand (GC-GARMENT-PLAN.md P2D slice 1, 4th draft): a stand that hugs the neck and a fall
	 * that spreads over the shoulders cannot be one flat piece, so two, sewn along the roll line, each a ring's sector — exactly flat.
	 * `OutStand` (`<Name>.Stand`, rows 0 to `Rows.Roll`): its neck edge the neckline point for point, its top (`Roll`) as long as the roll
	 * row of `Middle`. `OutFall` (`<Name>.Fall`, rows `Rows.Roll` to the last): its inner edge (`Roll`) the stand's top segment for segment,
	 * its outer edge (`Outer`) as long as the last row of `Middle`. Each has `End0` and `End1`; vertex `K·Rows + S` is column K's row S of
	 * that piece. `Placed` is left for the caller. False with `OutRefusal` when a piece cannot be cut that long.
	 */
	GRAPHCODEGARMENT_API bool DraftCollar(const FCollarDraft& Draft, const FCollarRows& Rows, const FString& Name, FGcGarmentPiece& OutStand,
		FGcGarmentPiece& OutFall, FString& OutRefusal);
	/**
	 * How well a block lies flat (GC-GARMENT-NOTCH-PLAN.md B1). Sigmas are the principal stretches of every triangle, from the 3D
	 * triangle to its flat one, both of each triangle pooled: 1 is cloth neither stretched nor squeezed, 1.02 a fiftieth longer.
	 */
	struct FBlockFit
	{
		float MinSigma = 0.f;
		float MaxSigma = 0.f;
		/** The 5th and 95th share of the pooled stretches, so a few bad triangles at an edge do not hide the rest. */
		float P5Sigma = 0.f;
		float P95Sigma = 0.f;
		/** Triangles whose flat area is not above 1e-9 of their 3D area: turned over, or collapsed. */
		int32 Flipped = 0;
		/** No two boundary segments that do not share a point touch or cross. */
		bool bBoundaryInjective = true;
		/** Global solves the relaxation took (0: the conformal start was kept). */
		int32 Iterations = 0;
	};

	/**
	 * A fitted block opened onto the plane ONCE, in centimetres (GC-GARMENT-NOTCH-PLAN.md B1): a least-squares conformal start, then
	 * as-rigid-as-possible relaxation that never turns a triangle over. `Triangles` index `Points`, wound so the surface faces outward;
	 * `OutFlat` is one point per point, seen from the side the triangles face (counter-clockwise), turned so flat Y is as near the
	 * world's up as the block's mean plane allows (toward +X for a block that faces straight up or down), vertex 0 where the start put it.
	 * A block of one piece, with a boundary, every point in a triangle. False with `OutRefusal` when it cannot be opened (a closed
	 * surface, a start that folds over itself, a flat that turned a triangle over or crossed its own boundary); `OutFlat` and `OutFit`
	 * still hold the layout it reached, so a refused block can be looked at.
	 */
	GRAPHCODEGARMENT_API bool FlattenBlock(const TArray<FVector>& Points, const TArray<FIntVector3>& Triangles, TArray<FVector2f>& OutFlat,
		FBlockFit& OutFit, FString& OutRefusal);

	/**
	 * `FlattenBlock`'s own boundary test, for any flat: no two of `Segments` (index pairs into `Flat`) that share no point cross, touch or
	 * overlap - a proper crossing, or a point exactly on the other segment (collinear overlaps included); no tolerance of its own.
	 */
	GRAPHCODEGARMENT_API bool SegmentsInjective(const TArray<FIntPoint>& Segments, const TArray<FVector2d>& Flat);

	/** Wind `Piece`'s triangles so their face is `Outside` at `Vertex`'s triangle (its `Placed`): the whole piece one way. True where it turned them. */
	GRAPHCODEGARMENT_API bool FaceOut(FGcGarmentPiece& Piece, int32 Vertex, const FVector& Outside);

	/** Every triangle of `Piece` wound the other way. */
	GRAPHCODEGARMENT_API void Turn(FGcGarmentPiece& Piece);

	/**
	 * Where a flat piece starts, wrapped on a cylinder (GC-GARMENT-NOTCH-PLAN.md C1): the cloth bent round an axis, not stretched. A flat
	 * point (x, y), counted from `FlatAnchor`, goes to the angle `AngleDeg + x / RadiusCm` round the axis and the height `HeightCm + y`
	 * along it, `RadiusCm` out from it — so the cloth is exactly as long along the axis and round it as it was cut, and only a straight
	 * edge between two points of a curve is shorter, by its chord.
	 */
	struct FArrangement
	{
		/** A point on the axis: the height 0 is where it is. */
		FVector Origin = FVector::ZeroVector;

		/** The way the axis runs; any length but none. Height is counted along it. */
		FVector Axis = FVector::ZAxisVector;

		/** The way angle 0 points: whatever of it lies along the axis is taken off, so it need not be square to it. Angles count the right-handed way about `Axis`. */
		FVector Zero = FVector::XAxisVector;

		/** How far the cloth lies from the axis, more than 0. */
		double RadiusCm = 0.0;

		/** The flat point that lands at `AngleDeg` and `HeightCm`. */
		FVector2f FlatAnchor = FVector2f::ZeroVector;
		double AngleDeg = 0.0;
		double HeightCm = 0.0;
	};

	/**
	 * `Piece`'s `Placed`, laid by `At`; nothing else of it changes. Its `Placed` may be empty (it is sized to `Flat`) but, if it is not, it
	 * is as long as `Flat`. The piece is wrapped the way round that keeps its triangles facing away from the axis — the way they are wound
	 * as stored, so a piece turned over (`Turn`) is wrapped the other way round and faces out all the same. False with `OutRefusal`, and
	 * `Placed` as it was, when `At` is no cylinder or the piece has nothing to wrap.
	 */
	GRAPHCODEGARMENT_API bool Arrange(FGcGarmentPiece& Piece, const FArrangement& At, FString& OutRefusal);
}
