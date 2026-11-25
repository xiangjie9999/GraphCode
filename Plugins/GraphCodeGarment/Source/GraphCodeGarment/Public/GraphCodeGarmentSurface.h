// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "GraphCodeGarmentPiece.h"

class FGcGarmentBody;
struct FGcGarment;
struct FGcGarmentBuiltRing;
struct FGcGarmentDocument;
struct FGcGarmentFinishedSheet;
struct FGcGarmentHungCloth;
struct FGcGarmentRim;
struct FGcGarmentStep;
struct FGcGarmentAcross;

/**
 * How one edge of thick cloth is finished, as the build draws it (`construct(rim=...)`, GC-GARMENT-PLAN.md E1, E3): within
 * `TurnedCm` of the edge, along the cloth, `Layers` layers of it stand — the turned ones inside — and over `Cm` across the edge
 * the stack's thickness falls away as `KeptAt` keeps its half, from where the rounding starts to the edge. Below 0 (`Layers` 0):
 * unwritten, the garment's own — and for the garment, `Cm` half the stack, the round a turned edge has.
 */
struct GRAPHCODEGARMENT_API FGcGarmentRimShape
{
	double Cm = -1.0;

	/** The rim's profile, its shares 0 to 1; empty, a round. */
	TArray<TPair<double, double>> Keys;

	double TurnedCm = -1.0;
	int32 Layers = 0;

	/**
	 * A sewn-on piece's edge (set only by `GcGarmentPatch::Finish` on a patch's sewn sides and its hem: loops, labels, pockets, flaps),
	 * rounded INWARD: the round ends at the drawn outline at mid-thickness, its face and underside start their round inside it, so the piece
	 * is exactly as wide as written and meets the cloth under it with an undercut, never a wall flaring past the outline (the owner, 15 Nov,
	 * a belt loop's "diverging" side). False, a garment's own edge: rounded outward from the outline, as before (hems, folds, rims).
	 * Fits the struct's tail padding (sizeof 40 either way); carried in the staged record (`RimRecord`, ReadyVersion 43).
	 */
	bool bInset = false;

	/** The share of the half-thickness kept at `U` across the rim, 0 where the rounding starts, 1 at the edge. */
	double KeptAt(double U) const;

	/** This edge's own, with what it leaves unwritten taken from `Garment`'s. */
	FGcGarmentRimShape Over(const FGcGarmentRimShape& Garment) const;

	/** `Rim` as written, its profile looked up in `Document`. */
	static FGcGarmentRimShape From(const FGcGarmentRim& Rim, const FGcGarmentDocument& Document);
};

/** A `socket()` of a placed part, in the body's space: `$Buckle.Right` in a garment's `guide()`. */
struct GRAPHCODEGARMENT_API FGcGarmentPartPoint
{
	FString Name;
	FVector At = FVector::ZeroVector;

	/** The socket's +Z, placed: the way through an opening it marks. */
	FVector Facing = FVector::ZeroVector;

	/** Where it is and the way it faces in the part's own file: what a bent part carries it from. */
	FVector Made = FVector::ZeroVector;
	FVector MadeFacing = FVector::ZeroVector;
};

/**
 * One piece of a garment: lines of cloth side by side, each from one of its rings to another, every
 * line in the same number of stations. Neighbouring panels meet on a line both hold, point for point.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPanel
{
	/** What the panel is, for a build's notes only - nothing reads it to decide what to do: the garment's name, `front`, `shoulder at Armhole_L`. */
	FString Name;

	/** Made by `insert()`: its rows run across the opening it fills, not down the garment (read instead of its name, 16 Oct). */
	bool bInsert = false;

	/** Layers of its cloth, its free edge the fold where more than one: a knit `band()` grown as a piece of its own is doubled, 2. It weighs that many
	 *  times its cloth and bends the square of it in the hang, as the flat route's band does (step 17). */
	uint8 Layers = 1;

	TArray<TArray<FVector>> Lines;

	/** Resolved fabric, retained when this piece is attached, cut or extended. */
	FString Fabric;

	/** Immutable cut coordinates in cm for open-edge cloth; same indices as Lines, refined together. */
	TArray<TArray<FVector2f>> Pattern;

	/**
	 * Where each point of `Lines` lies on the flat cut of the piece `pattern()` took it into, in tiles (`TileCm`), V down the garment — what
	 * the cloth's UV0 is when the garment was designed in 2D, so a texture runs unbroken across each cut piece (GC-GARMENT-NOTCH-PLAN.md C2b).
	 * Same shape as `Lines` when set; empty when it was never taken off flat.
	 */
	TArray<TArray<FVector2f>> PatternFlat;

	/**
	 * The DRAWN flat (14 Nov, "each piece its own grain"): a one piece's UV0 laid BY LENGTHS - round each row, down the piece - and cut where a
	 * maker hides a tube's join (the wearer's left side), for the written mesh only; the hang reads `PatternFlat`. `DrawnFlat` its flat points
	 * (one to each of `Lines[0]`'s, then the copies along its cut); `DrawnFaces` each of `Faces`' corners' point in it. Empty: none, and the
	 * written UV0 is `PatternFlat`'s as before.
	 */
	TArray<FVector2f> DrawnFlat;
	TArray<FIntVector3> DrawnFaces;

	/**
	 * A patch's mouth station on each of its lines (14 Nov, ruled P1): the line's cloth runs from its foot to this station, its mouth, and past it
	 * only a hem turned to the inside (`patch(..., hem=(d, once|twice), ply)`), joined to the pocket by the fold edge at the mouth. Empty, or a
	 * line past its end: the mouth is the line's last station, as every panel was built before. Read through `GcGarmentMouthStation`.
	 */
	TArray<int32> MouthStation;

	/** The piece of `FGcGarmentBuiltSurface::Pieces` that `PatternFlat` is the cut of; INDEX_NONE without one. */
	int32 PatternPiece = INDEX_NONE;

	/** The author's chart this cloth is laid flat in (`uv()`, `FGcGarmentBuiltSurface::UVCharts`); INDEX_NONE: its own, 100 cm a tile. */
	int32 UVChart = INDEX_NONE;

	/** By line (as `Lines`): 1 where the line is cloth tucked under the face before it - a pleat's cloth folded under and coming back
	 *  (`match(..., pleat())`) - which rides on that face in the hang, as a fold()ed layer does (the user, 21 Oct: "Every fold rides"); 2 under
	 *  the face after it (a box or an inverted pleat's mirrored one, whose lines run the other way). Empty, or not as many as `Lines`: none. */
	TArray<uint8> Tucked;

	/**
	 * A piece drafted flat as a free mesh, not a grid (GC-GARMENT-DRAFT-PLAN.md S2a-2): its points are `Lines[0]`, all of them in its piece's
	 * vertex order, its cut `PatternFlat[0]`, and its triangles these — indices into `Lines[0]`, wound as its piece's. Empty: a grid.
	 */
	TArray<FIntVector3> Faces;

	/** Native midsurface-to-inner/outer directors, retained so thickening cannot change the solved volume. */
	TArray<TArray<FVector>> Directors;
	/** Held attachment directions from construction, independent of the subsequent guide pose. */
	TArray<FVector> RootDirectors;
	/** Openings already threaded by guide; later pulls must retain every material line through them. */
	TArray<TPair<FString, FGcGarmentPartPoint>> NativePasses;

	/** The lines go all the way round — a tube — so the last lies beside the first. */
	bool bClosed = false;

	/**
	 * How far along its garment the first station and the last are, 0 at the garment's first ring
	 * and 1 at its second: a shoulder runs from 0 to its armhole, a side from there to 1. What a
	 * `thickness=$Profile` is read against.
	 */
	double AlongFrom = 0.0;
	double AlongTo = 1.0;

	/**
	 * `thickness=` of the garment the panel belongs to, in centimetres, as `(along, cm)` stops — a
	 * sleeve's panels carry the sleeve's, or the shirt's when the sleeve has none. Empty: a single sheet.
	 */
	TArray<TPair<double, double>> Thickness;

	/**
	 * `match()`: how much cloth runs round each station, in centimetres, where more is gathered or pleated into it than its lines
	 * are laid round — at its seam, a gathered skirt's whole width held in the ring's length. What the piece is laid flat from
	 * (GC-GARMENT-STITCH.md §2). Empty: the cloth is as laid.
	 */
	TArray<double> ClothRound;

	/**
	 * `match()` on a stretch of its ring (a puff sleeve's cap): how far round the cloth each line is at each station, 0 to 1 - line J at
	 * station K is `[J * Stations + K]` - so the piece is laid flat with the gathered stretch cut wider and the rest of it as laid. Empty,
	 * or not one to every line and station: each line at one share of the cloth all the way down (`ClothRound`).
	 */
	TArray<double> ClothShare;

	/**
	 * How long it rests round each station - across its grain, round the lines - as a share of how it is laid there: a knit `band()` cut shorter
	 * than the edge it is sewn to by its fabric's stretch (`GcGarmentFlat::BandRatio`), sewn on stretched so it holds the edge round (the user,
	 * 16 Oct); a knit drawn onto a longer edge, `match($Cuff, 80%, stretch)`. Empty, or not as many as the stations: as laid.
	 */
	TArray<float> RestAcross;

	/**
	 * Which piece of cloth this is, in the order the pieces were laid: 0 the garment's own, then each garment added and each piece
	 * grown. What says which side of a ring is a seam's positive one — the piece laid first there (GC-GARMENT-STITCH.md §4).
	 */
	int32 Piece = 0;

	/**
	 * Cloth a cut part way down a line parted from another panel (GC-GARMENT-CUT-SEW.md §F item 2): that panel, whose last line is this one's
	 * first wherever the cut did not open it - its stations up to `PartedOpenFrom` (when above 0) and from `PartedOpenTo` (when set) - the
	 * cloth there one cloth still. Laid flat with it as one piece, slit where the cut opened, as a dart is sewn in one piece of cloth and a
	 * godet let into a slash of one. INDEX_NONE: a piece of its own.
	 */
	int32 PartedFrom = INDEX_NONE;
	int32 PartedOpenFrom = 0;
	int32 PartedOpenTo = INDEX_NONE;

	/**
	 * A `Faces` panel drafted flat (GC-GARMENT-SURFACE-STEPS.md step 1): each point of its one line's own thickness, centimetres, and the layers
	 * stacked on its front and back (`FGcGarmentPiece::FrontLayers`, `BackLayers`). Each one per point, or empty: `Thickness` read at `AlongFrom`
	 * and the rims' turned layers, as before.
	 */
	TArray<float> ThicknessAt;
	TArray<uint8> FrontLayers;
	TArray<uint8> BackLayers;

	/**
	 * The one piece with its holes (`GcGarmentOnePiece::Lay`): each point of its one line's place on the garment as it was laid - round its
	 * axis in radians (X) and down it (Y), 0 at the first ring and 1 at the second. Where a line of cloth runs, from a place on one ring to a
	 * place on the other, as the old way's string does: what `cut($Line)` cuts it along. Empty for every other panel.
	 */
	TArray<FVector2f> Placed;

	/**
	 * A cut across its lines (`FGcGarmentAcross`, Private/GraphCodeGarmentCutAcross.h, the third route of reports/cut-across-lattice-routes-15nov.md):
	 * `WeldSheet` splits the cells it crosses, the lattice itself unchanged. Null on every panel today: only the CutAcross test sets it (slice 1).
	 */
	TSharedPtr<const FGcGarmentAcross> Across;
};

/** The station of `Panel.Lines[Line]` that is its mouth (`FGcGarmentPanel::MouthStation`): stored, or the line's last station - as built before. */
inline int32 GcGarmentMouthStation(const FGcGarmentPanel& Panel, const int32 Line)
{
	const int32 Last = Panel.Lines.IsValidIndex(Line) ? Panel.Lines[Line].Num() - 1 : INDEX_NONE;
	return Panel.MouthStation.IsValidIndex(Line) && Panel.MouthStation[Line] >= 0 && Panel.MouthStation[Line] < Last ? Panel.MouthStation[Line] : Last;
}

/**
 * A pressed seam resolved on the finished cloth (GC-GARMENT-STITCH.md §4), in the sheet's space: the seam line, and at each of its
 * points the way across the cloth to its positive side. `ToMesh` cuts the step into the outside and stands it up.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPressLine
{
	TArray<FVector> Points;
	TArray<FVector> Across;
	bool bClosed = false;

	/** The allowance's width, its sign the side it lies on. */
	double WidthCm = 1.0;

	/** Pressed open: one layer more on both sides. */
	bool bOpen = false;

	/** Layers of cloth under the pressed side: two for a seam pressed to one side, one for a turned hem or each side of an open seam. */
	int32 Layers = 2;

	/**
	 * Where the seam ends in whole cloth - a slit sewn shut part way, as a dart is - at its first point or its last: the allowance there,
	 * the wedge folded under, narrows evenly to nothing, as a dart's does to its point.
	 */
	bool bEndsInClothFirst = false;
	bool bEndsInClothLast = false;

	int32 Line = 0;
	FString Target;

	/**
	 * A seam line drawn on a garment laid as one piece (13 Nov, slice B: the tee's shoulder): an open chain with the one piece on both sides.
	 * `ToMesh` presses it only as an open regular strip, after the band joins, and never by cut rows (which run past its ends).
	 */
	bool bDrawn = false;

	/**
	 * `press(..., follow=$Row, margin=)` (15 Nov): the allowance's width along the line, as (share of the line's length from its first point,
	 * width cm), every 0.1 cm: min(declared width, the row's distance from the line + the margin, as a true offset of the row). Empty: the
	 * declared width all along, as before.
	 */
	TArray<FVector2D> EdgeWidth;

	/**
	 * One seam made as two presses (17 Nov, seamfold2-17nov.md G; written only with `gc.Garment.SeamFold` 1, by `GcGarmentSeam::MarkContinuing` from
	 * the recipe): the `Line` of the press whose line's LAST place is this one's FIRST - the same declared point, on a ring and its half that the
	 * garment's `join()` names (the jeans: `$SideR` to `$Crotch@0`, `$OutR` from `$Top_R@0`). `GcGarmentSeam::Press` presses such presses as one
	 * line, so their fold runs through the ring. 0: none, as before.
	 */
	int32 ContinuesLine = 0;
};

/**
 * One line of stitching resolved on the finished cloth (GC-GARMENT-STITCH.md §5): the path it runs along, a point about every
 * millimetre, in the sheet's space — the cloth's inner face, which `ToMesh` grows the outside from.
 */
struct GRAPHCODEGARMENT_API FGcGarmentStitchPath
{
	TArray<FVector> Points;
	int32 Faces = 1;

	bool bClosed = false;

	/** From one needle hole to the next, adjusted on a closed path so a whole number of stitches goes round. */
	double EveryCm = 0.3;

	/** The thread's thickness. */
	double ThreadCm = 0.05;

	/** The thread's look, a material's path; empty: a plain off-white thread. */
	FString Material;

	/** Where it was written, for notes and refusals. */
	int32 Line = 0;
	FString Target;

	/**
	 * The cloth it is sewn through, as the row's bite reads it (17 Nov, stitch-bite-design-16nov.md; set by `StitchLine` only where the garment's
	 * fabric declares `pressed=`): one layer's declared thickness (`PlyCm`), its thickness under the thread as a share of that (`PressedShare`,
	 * 0: the fabric declares none - no bite), the pressure that reading holds at (`PressedKPa`) and one layer's bending rigidity, N m (`BendNm`:
	 * the geometric mean of the two ways its `stiffness=` and `weight=` give). Zero: nothing pressed, as before.
	 */
	double PlyCm = 0.0;
	double PressedShare = 0.0;
	double PressedKPa = 0.0;
	double BendNm = 0.0;
};

/**
 * A padded panel its stitches quilted (GC-GARMENT-STITCH.md §5a): the wall it is pinched to at a stitch - two of its fabric's layers - and each
 * of its points' wall before it was quilted, as `ThicknessAt`. What quilts the 1 cm cloth a garment ships at that cloth's own points (`Finish`).
 */
struct GRAPHCODEGARMENT_API FGcGarmentQuilt
{
	double PinchedCm = 0.1;
	TArray<float> WallAt;
};

/** An added fabric strip resolved in the cloth's intrinsic coordinates (GC-GARMENT-BAND.md). */
struct GRAPHCODEGARMENT_API FGcGarmentBand
{
	FString Fabric;
	FString Target;
	int32 Line = 0;
	int32 Sides = 3;
	bool bTwoEdges = false;
	FVector2d Smooth = FVector2d::Zero();
	/** At an original sheet point: intrinsic cross-cloth distance, front width, back width, added thickness. */
	TMap<FVector, FVector4d> Coordinates;
};

/**
 * Which triangles of a mesh `ToMesh` made are the cloth's outside, and where each vertex is on the sheet for stitching.
 * An outside face's original vertex is the sheet's vertex of the same number (GC-GARMENT-STITCH.md §5).
 */
struct GRAPHCODEGARMENT_API FGcGarmentOutside
{
	TArray<int32> Triangles;
	TArray<int32> InsideTriangles;
	TArray<FVector> Sheet;

	/** How many points the sheet has: vertex V of the mesh is the outside at sheet point V and `Count + V` the inside. */
	int32 Count = 0;

	/**
	 * Per sheet point, how far its cloth's inside is lifted off the sheet where it lies on a layer of the same garment laid before it (a fold
	 * turned back, laid paper-thin): that layer's thickness reaching past it, along the way out its thickness grows. Zero elsewhere.
	 */
	TArray<FVector> Lift;

	/** The garment's stitch paths (by index) whose bite `ToMesh` cut into the cloth (17 Nov): the sheet draws no pinch under them, the groove is the mesh's. */
	TArray<int32> Bitten;

	/** Where any vertex of the mesh is on the sheet — outside, inside, or one a pressed seam's cut added. */
	FVector Place(const int32 Vertex) const
	{
		const int32 On = Vertex >= Count && Vertex < 2 * Count ? Vertex - Count : Vertex;
		return Sheet.IsValidIndex(On) ? Sheet[On] : FVector::ZeroVector;
	}
};

/**
 * What a part that bends is carried by (GC-GARMENT-SHAPE-PARTS.md §6): the face it sits on, laid as lines side by
 * side along its own length — its X in its file — the same lines of cloth a `guide()` or `pull()` moves. The part is
 * made flat, as leather is cut, and whatever moves these lines bends it: each point of it keeps its place over them and
 * its height off them.
 */
struct GRAPHCODEGARMENT_API FGcGarmentPartRibbon
{
	/** Its lines as they lie now, in the body's space: from the part's -X end, its root, to its +X end. */
	FGcGarmentPanel Panel;

	/** The part in its own file's frame, cut across at every station so it can bend. */
	UE::Geometry::FDynamicMesh3 Made;

	/** The part's own box in that frame: the lines run along X from `Low.X` and lie side by side across Y, on `Low.Z`. */
	FVector Low = FVector::ZeroVector;
	FVector Size = FVector::ZeroVector;

	/** Each point of `Made`, placed on the lines as they lie now: `Out`, the part as the body wears it. */
	void Pose(UE::Geometry::FDynamicMesh3& Out, TArray<FGcGarmentPartPoint>& Points);

	/**
	 * A mesh's attributes point back at the mesh, and an array that grows moves its elements' bytes without telling them,
	 * so a part's mesh read after its array grew reads freed memory. Moving it out and back points them at it again — the
	 * engine re-points them on a move and nowhere else.
	 */
	static void Repoint(UE::Geometry::FDynamicMesh3& Mesh);

	/** A point in the part's frame, carried by the lines: `OutFacing` is the lines' outward normal there. */
	FVector Carry(const FVector& Made, FVector* OutFacing = nullptr) const;

	/** Its lines as it was made, flat in its own frame, `Stations` along each: the lengths it keeps however it is bent. */
	TArray<TArray<FVector>> Flat(int32 Stations) const;
};

/** A part from Shape, placed on a garment: in the body's space, where it sits, and how it moves with the body. */
struct GRAPHCODEGARMENT_API FGcGarmentPlacedPart
{
	/** `$Buckle.Buckle at $Waist@0`, for a build's notes; a row's first copy names the row and the rest are empty. */
	FString Name;

	/** `Buckle = add(...)`: the name its points are reached by, `$Buckle.Right`. Empty where the add is not named. */
	FString Handle;

	/** Its sockets, placed with it. */
	TArray<FGcGarmentPartPoint> Points;

	UE::Geometry::FDynamicMesh3 Mesh;

	/** The point on the surface it sits on, whose skin a stiff part moves with. */
	FVector Seat = FVector::ZeroVector;

	/** Out of the body at the seat: the part's own +Z. */
	FVector Out = FVector::ZeroVector;

	/** Up the body at the seat: the part's own +Y. A part placed at one of its points takes it. */
	FVector Up = FVector::ZeroVector;

	/** 1 moves as one piece, 0 bends with the body under it. */
	double Stiffness = 1.0;

	/** `gap=` as it was seated, never below the cloth: how far its underside stands off the cloth it holds. */
	double GapCm = 0.0;

	/** How far from where it is placed its farthest point lies as its file builds it, scaled: its own box's farthest corner (2 Nov). Fitted to the
	 *  cloth, a point of it lies at most root 2 times that from there wherever the cloth turns no more than a right angle under it. */
	double ReachCm = 0.0;

	/** It rests on the garment's cloth — not on a part's point, and not where no cloth lies under its place. */
	bool bOnCloth = false;

	/**
	 * How it holds the cloth under it (GC-GARMENT-SHAPE-PARTS.md §8): `HoldCm` the border's width, and `bPresses` where the cloth
	 * is still to be pressed onto its underside — seated and not yet lifted clear, since the cloth comes to it instead.
	 */
	double HoldCm = 0.0;
	bool bPresses = false;

	/**
	 * Where `hold=` was not written, the fabric's bending length: at rest the border widens past `HoldCm` until the cloth bends
	 * no tighter than this easing back from what the part pressed (§8, *the default width*). 0 where `hold=` was written.
	 */
	double RestBendCm = 0.0;

	/** The cloth it holds, by where each point is on the sheet, and how much of that point's motion it takes. */
	TArray<TPair<FVector, float>> Holds;

	bool Holding() const { return bOnCloth && HoldCm > 0.0 && Stiffness > 0.0; }

	/** A named part below 100% stiffness bends, carried by these lines; empty for a stiff one. */
	FGcGarmentPartRibbon Ribbon;

	bool Bends() const { return !Ribbon.Panel.Lines.IsEmpty(); }

	/** The `add()` that placed it. */
	const FGcGarmentStep* From = nullptr;

	/** A guide passes cloth through it: it is held where it is, never lifted by that cloth. */
	bool bThreaded = false;

	/**
	 * The flat route laid the cloth it holds wholly on its back and held it there through the hang - a patch (`FinishFlat`): the part stays where
	 * it is and the cloth beside it comes to it, never standing in front of its underside (`FGraphCodeGarmentParts::Hold`).
	 */
	bool bLaidOn = false;

	/** Moved as one piece, `By` further: its mesh, its seat and its points. */
	void Shift(const FVector& By);

	/** Turned as one piece by `By` about `About`: its mesh and normals, its seat, its frame, its points and its lines. */
	void Turn(const FQuat& By, const FVector& About);

	/** Its point `Name`, or null. */
	const FGcGarmentPartPoint* FindPoint(const FString& Name) const;
};

/**
 * Where a drawing (`cut('/Game/Art/Tears')`, GC-GARMENT-GRAMMAR.md §5) lies on the one piece, as its lay laid it (`GcGarmentDrawn::FFrame`):
 * at each of its places across, the round of the line it runs down at the first ring and the turn to the second, and - laid between two
 * rings - how far down the garment its top and bottom are there. Kept so the cut reads the very frame the cloth's edges were laid by.
 */
struct GRAPHCODEGARMENT_API FGcGarmentDrawnFrame
{
	TArray<double> From;
	TArray<double> Turn;
	/** Its top and bottom down the garment at each place across (two rings); empty: `Down` everywhere. */
	TArray<double> Top;
	TArray<double> Bottom;
	FVector2d Down = FVector2d(0.0, 1.0);
};

/**
 * One `uv()` (GC-GARMENT-STITCH.md §2a; the user, 21 Oct: "allow author to declare the UV, like a fixed UV scale", "can choose when to UV, just
 * like shape"): the cloth there was where it was written, each piece laid flat at the author's tile, or all of it packed into one (`fit`).
 */
struct GRAPHCODEGARMENT_API FGcGarmentUVChart
{
	/** One tile every so many centimetres of cloth; 0: `fit`, the whole chart in one tile, its proportions kept. */
	double TileCm = 0.0;

	/** Where it was written, for notes: the garment and the line. */
	FString Garment;
	int32 Line = 0;

	/** `seamless`: the tile nudged to a whole number of them round the chart's widest tube. */
	bool bSeamless = false;
};

/**
 * An edge of the cloth that is no ring (GC-GARMENT-SHAPE-PARTS.md §4): a side of a cut, or the far end of
 * cloth grown from one. Kept by where it is in the cloth, not by its points, which `relax` and `sew` move:
 * a whole line of a panel, or the same end of every line of one.
 */
struct GRAPHCODEGARMENT_API FGcGarmentOpenEdge
{
	FString Name;
	int32 Panel = INDEX_NONE;

	/** A cut: this line of the panel, from its first station. INDEX_NONE: the ends of the panel's lines. */
	int32 Line = INDEX_NONE;

	/** Which ends, where `Line` is none: the last station of every line, or the first. */
	bool bLast = true;

	/**
	 * A cut part way down a line of cloth (GC-GARMENT-CUT-SEW.md §F item 2): the stations it opens, `From` to `To` inclusive,
	 * INDEX_NONE the last. The station at its closed end is one point both sides share — the slit's top — and the cloth past it
	 * is one cloth still. 0 and INDEX_NONE: the whole line.
	 */
	int32 From = 0;
	int32 To = INDEX_NONE;

	/** A side of a cut through a free mesh (the one piece): its points in order along the cut, indices into the panel's one line. Set, it is
	 *  the edge, and `Line` is not read. */
	TArray<int32> Points;
};

/**
 * A garment (`GC-GARMENT-GRAMMAR.md` §4–§5): lines of cloth like strings between two hoops, one from
 * each place round the first ring to the same place round the second — one closed panel. A ring the
 * garment connects to splits it into panels round that ring; a garment it adds brings its own.
 */
struct GRAPHCODEGARMENT_API FGcGarmentBuiltSurface
{
	FString Name;

	/**
	 * With no ring connected, one closed panel whose `Lines[i]` runs from the first ring's `@i/N` to
	 * the second ring's: the first station exactly on the first ring, the last exactly on the second.
	 * Station k of every line together is a ring between the two.
	 */
	TArray<FGcGarmentPanel> Panels;

	/**
	 * The second ring counted round the other way, so both run the same way along the garment: a
	 * line then ends at the second ring's `@(1 - i/N)`. Opposite ways would send every line through
	 * the middle, which no author means.
	 */
	bool bSecondReversed = false;

	/** Built by the flat route (GC-GARMENT-SURFACE-STEPS.md) and hung: what it is worn over and under shaped it before it was cut (step 19), so
	 *  nothing moves its hung cloth after. Beside `bSecondReversed`, in its padding. */
	bool bHungFlat = false;

	/** What the build says about the cloth it laid: how far a `relax` settled it. */
	TArray<FString> Notes;

	/** The cloth's edges that are no ring, by name: a cut's two sides, and the far ends of cloth grown from them. */
	TArray<FGcGarmentOpenEdge> OpenEdges;

	/** Each drawing a `cut('/Game/Art/Tears')` names (GC-GARMENT-GRAMMAR.md §5, "Cutting with a drawn image"): its outlines on its own
	 *  canvas, (across, down) from 0 to 1 (`GcGarmentDrawn::Read`) - read once, laid by the one piece as its own edges, cut along at the step. */
	TMap<FString, TArray<TArray<FVector2d>>> Drawn;

	/** Each such drawing's lines (`GcGarmentDrawn::Read`'s `OutLines`): rips, laid as the cloth's own edges and cut along at the step. */
	TMap<FString, TArray<TArray<FVector2d>>> DrawnLines;

	/** Each drawn cut's frame as the one piece laid it, by the line the cut is written on. */
	TMap<int32, FGcGarmentDrawnFrame> DrawnFrames;

	/** Each `uv()` written, in order; a panel names the one it is laid in (`FGcGarmentPanel::UVChart`). */
	TArray<FGcGarmentUVChart> UVCharts;

	/**
	 * Each garment this one is worn over (`over=`) as the two hung together, one cloth (the user, 23 Oct: "hang a garment and the ones worn
	 * under it as one Chaos cloth ... no geometric press after"): its name, its panels as hung, and the same panels' lines as that garment
	 * was made (its own hang, before it was laid under this one) - line for line, point for point - so its own build takes its cloth from
	 * here (`GcGarmentLayer::Layer`). Kept in memory between builds; no record carries it.
	 */
	struct FWornUnder
	{
		FString Name;
		TArray<FGcGarmentPanel> Panels;
		TArray<TArray<TArray<FVector>>> MadeLines;
	};
	TArray<FWornUnder> WornUnder;

	/**
	 * The parts a `guide()` or `pull()` met (GC-GARMENT-SHAPE-PARTS.md §4): each written above one is seated on the
	 * cloth that stays as it finds it, takes part in it, and keeps the pose it leaves it in — the build places it there.
	 */
	TArray<FGcGarmentPlacedPart> Seated;

	/**
	 * Where the hang carried the cloth (23 Nov, <T>\parts-conform\parts-conform-why-23nov.md): each point of the settled sheet as laid, and where it
	 * hung. A part seated after the hang by a place in SPACE - a ring, `at=$LoopRow@0.125` - is moved with the cloth that lay there
	 * (`GcGarmentPartsCarry`), as a part sewn to that cloth is: the cargo trousers' band hung 0.9-1.1 cm below where it was laid and its loops,
	 * placed by a ring at the band's laid height, stood that far over its top edge. A later hang carries these on. Empty: never hung.
	 */
	TArray<TPair<FVector, FVector>> HangCarried;

	/** Every `.gcshape` those parts are built from, for the asset's digest. */
	FString SeatedFolded;

	/** The stitches `stitch()` resolved on the finished cloth, in the order written (GC-GARMENT-STITCH.md §5). */
	TArray<FGcGarmentStitchPath> Stitches;

	/** The seams `press()` resolved on the finished cloth, in the order written (GC-GARMENT-STITCH.md §4). */
	TArray<FGcGarmentPressLine> Presses;

	/** Points of one piece sewn to points of another as they are laid (30 Oct) - a collar's end sewn to its lapel's top along the gorge - each
	 *  side by panel, line and station. The hang holds each pair together as a seam (`FGcGarmentChaosInput::SewingPairs`). */
	TArray<TPair<FIntVector, FIntVector>> SewnPoints;

	/** Each stitch of a seam the hang sews (31 Oct, as Marvelous Designer sews): its two points as laid, one on each edge. `sew()` pairs them and
	 *  moves no cloth; the hang draws each pair shut and holds it (`FGcGarmentChaosInput::SewingPairs`), and what ships is one point there. */
	TArray<TPair<FVector, FVector>> SewnInHang;

	/** Each stitch of a cut's own two sides sewn back that stay TWO edges (15 Nov, the owner: a seam is two pieces of cloth, the line the real
	 *  gap between them; `gc.Garment.SewnSeamsApart`, opt-in): its two points as laid, one on each side. Never welded (`WeldSheet` reads only
	 *  `SewnInHang`); the hang holds each pair at `SewnApartRestCm` (parallel) with a native spring, so the edges move as one seam. */
	TArray<TPair<FVector, FVector>> SewnApartInHang;
	TArray<float> SewnApartRestCm;

	/** A gathered edge's thread (31 Oct, being built: `gc.Garment.GatherByThread`): each point of the edge as `match()` laid it, at its full
	 *  length round, and its own place on the ring the hang draws it onto - the folds the cloth's own (GC-GARMENT-MATCH.md top). */
	TArray<TPair<FVector, FVector>> GatheredOnto;
	TArray<FGcGarmentBand> Bands;

	/** How its edges round off (`rim=`), and, by each point of an edge `rim($Edge, ...)` named, that edge's own way. */
	FGcGarmentRimShape Rim;
	TMap<FVector, FGcGarmentRimShape> RimAt;

	/**
	 * The pieces cut flat (GC-GARMENT-PLAN.md P2D) — a collar — each drawn as the open edge's panel of its name (vertex `K·Rows + S` is
	 * that panel's `Lines[K][S]`), and where each is sewn: what the pattern-to-Chaos carrier hangs.
	 */
	TArray<FGcGarmentPiece> Pieces;
	TArray<FGcGarmentPieceSeam> PieceSeams;

	/** `EdgeName`'s points in order, and for each the point of cloth beside it across the edge. False where it is no open edge. */
	bool OpenEdge(const FString& EdgeName, TArray<FVector>& OutPoints, TArray<FVector>& OutInward) const;

	/** Where the second ring's end of line `Index` of the first panel sits on it, as a fraction round. */
	double PlaceOnSecond(int32 Index) const;

	/** How much cloth it is, one side, in square centimetres. */
	double Area() const;

	/**
	 * What is built: with a `thickness=`, the sheet grown outward by each panel's thickness, a rim
	 * closing every opening, so the inside keeps the rings' clearance; without one, a single sheet seen from both
	 * sides — every triangle twice, once facing out and once in. Points two panels share become one
	 * vertex, so the sheet is one piece wherever its panels meet.
	 */
	/**
	 * `bLaidFlat`: the cloth's UVs, each piece flattened (GC-GARMENT-STITCH.md §2) — what an asset is written with. A mesh
	 * the build only measures against does without them; flattening costs a solve per piece.
	 * `RoomBody` (15 Nov, a MEASUREMENT only): the body the cloth hangs on, alive for this call; when given, each press prints the room under
	 * it (`GcGarmentPressRoom`). The mesh made is the same with it or without.
	 */
	UE::Geometry::FDynamicMesh3 ToMesh(bool bLaidFlat = true, FGcGarmentOutside* OutOutside = nullptr, bool bSeparateFaces = false,
		const FGcGarmentBody* RoomBody = nullptr) const;

	/** Additional front/back material pairs after the root fabric: attached pieces, then bands. */
	TArray<FString> ExtraFabrics() const;

	/** The same sheet once, every triangle facing out: what the openings and the seams are read from. */
	UE::Geometry::FDynamicMesh3 ToSheet() const;

	/**
	 * The garment finished finer, once it hangs (the user, 27 Oct: "make finished garments about 4x finer so small gathers, folds and rolls
	 * show"; GC-GARMENT-DRAPE.md, c71): its welded sheet with every triangle cut in four, each new point where its edge's curve through its two
	 * ends puts it - kept off other cloth by the cloth's thickness, off the skin, and back on the hung cloth wherever the finer sheet would
	 * cross itself - and kept in `Finished`, which `ToMesh` and `ToSheet` make the garment from from then on. `OutNote` says what it made.
	 */
	void Finish(const FGcGarmentBody& Body, FString& OutNote);

	/** Set by `Finish`: the finer sheet and what each of its points and triangles carries. */
	TSharedPtr<const FGcGarmentFinishedSheet> Finished;

	/** Set by the hang: the 1 cm cloth it hung at, which `Finish` ships as the garment (Research/garment-3d-path-plan.md 12e). */
	TSharedPtr<const FGcGarmentHungCloth> HungCloth;

	/** By panel, each padded one its stitches quilted, sewn after the hang (GC-GARMENT-STITCH.md §5a). */
	TMap<int32, FGcGarmentQuilt> Quilts;
};

/**
 * Where a garment's lines land on a ring it shares with the garment that adds it: exactly these
 * points of that ring, in the ring's own order from its `@0` — the samples where that garment
 * connects to it, or a joined ring's own points — so the two pieces meet on the same points.
 */
namespace GcGarmentLayer { struct FClothesUnder; }

struct GRAPHCODEGARMENT_API FGcGarmentLanding
{
	FString Ring;
	TArray<FVector> Points;
	/** Worn over clothes (`over=` on the garment that adds it), the clothes and which of their pieces lies under this one - a coat's sleeve over
	 *  the shirt's sleeve - so it is laid bigger than that piece (`GcGarmentLayer::FClothesUnder`). Null / INDEX_NONE: none. */
	const GcGarmentLayer::FClothesUnder* Clothes = nullptr;
	int32 ClothesPiece = INDEX_NONE;
	/** A leg of trousers whose slack the garment adding it lays once over its seat and its legs together (26 Oct): laid tight here. */
	bool bSlackByWhole = false;
};

class GRAPHCODEGARMENT_API FGraphCodeGarmentSurface
{
public:
	/**
	 * The garment's cloth, laid last (`GC-GARMENT-GRAMMAR.md` §4): every `cut` and `join` read first,
	 * so the panels are known before any line is laid. Each line: at `loose=0%` the shortest path over
	 * the body, at `100%` the shortest that stays off it, blended between, held off the skin by the
	 * clearance of its two ends. `Rings` are every built ring, found by name. Every opening of what
	 * is built is a ring, or it is refused naming `join`.
	 */
	static bool Build(const FGcGarment& Declared, const FGcGarmentDocument& Document, const TArray<FGcGarmentBuiltRing>& Rings,
		const FGcGarmentBody& Body, FGcGarmentBuiltSurface& Out, FString& OutRefusal);

	/** The same, landing on `Landing`'s points — how an added garment meets the one that adds it. Null: none. */
	static bool Build(const FGcGarment& Declared, const FGcGarmentDocument& Document, const TArray<FGcGarmentBuiltRing>& Rings,
		const FGcGarmentBody& Body, const FGcGarmentLanding* Landing, FGcGarmentBuiltSurface& Out, FString& OutRefusal);

	/**
	 * While one lives, each garment of a document is built once on its rings and body and given again: garments worn together build each other
	 * (`over=`: the one under for the hang and the room, each one over to press under), and a file's build lines build each of them too. Every
	 * build opens one of its own; a file's build, or a spec building several garments of one document, opens one round them all. The documents,
	 * rings and bodies built from must outlive it.
	 */
	struct GRAPHCODEGARMENT_API FBuildOnce
	{
		FBuildOnce();
		~FBuildOnce();
		FBuildOnce(const FBuildOnce&) = delete;
		FBuildOnce& operator=(const FBuildOnce&) = delete;
	};

private:
	/**
	 * One section of a garment through middle rings, built as a garment of its own: its `loose=` is the
	 * whole garment's from `From` to `To` along it, unless the section has its own `loose@N=`.
	 */
	struct FSection
	{
		double From = 0.0;
		double To = 1.0;
	};

	static bool Build(const FGcGarment& Declared, const FGcGarmentDocument& Document, const TArray<FGcGarmentBuiltRing>& Rings,
		const FGcGarmentBody& Body, const FGcGarmentLanding* Landing, const FSection* OfWhole, FGcGarmentBuiltSurface& Out,
		FString& OutRefusal);

	/** The first `Build`'s own work - built, then layered over and under the garments it is worn with - which that one keeps once a build. */
	static bool BuildLayered(const FGcGarment& Declared, const FGcGarmentDocument& Document, const TArray<FGcGarmentBuiltRing>& Rings,
		const FGcGarmentBody& Body, FGcGarmentBuiltSurface& Out, FString& OutRefusal);
};
