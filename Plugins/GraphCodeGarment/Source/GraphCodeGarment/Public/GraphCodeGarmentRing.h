// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeGarmentDocument.h"

class FGcGarmentBody;
struct FGcSectionPlane;

/** A ring built on the body (`GC-GARMENT-GRAMMAR.md` §2). */
struct GRAPHCODEGARMENT_API FGcGarmentBuiltRing
{
	FString Name;

	/**
	 * A closed polyline about a centimetre apart, starting at `@0` — the first typed point — and
	 * running the way the ring counts. The last point joins the first.
	 */
	TArray<FVector> Points;

	/** Where each typed point is in `Points`, in the order typed. `Pins[0]` is 0. */
	TArray<int32> Pins;

	/** From the ring toward the reference. The ring counts clockwise looking along it. */
	FVector Direction = FVector::ZeroVector;

	/** What `rotate` turns about: `midpoint=`, or the centre of the typed points. */
	FVector Midpoint = FVector::ZeroVector;

	/** A ring of one or two points is found in a plane, square to this. Zero for a drawn ring. */
	FVector PlaneNormal = FVector::ZeroVector;

	/** Its `expand` and `smooth=` as written — what a `join` holds the ring to again once it has pulled it. */
	double ExpandCm = 0.0;
	double SmoothPercent = 100.0;

	/**
	 * `ring($Source)`: copied from another ring and moved off the body as one piece. A copy's `ExpandCm`
	 * is its source's — the clearance its cloth was laid at — and its own `expand` only makes it bigger.
	 * Empty: found on the body.
	 */
	FString CopiedFrom;

	bool IsFloating() const { return !CopiedFrom.IsEmpty(); }

	/**
	 * Where in `Points` a `join` put the place its points met, once for each time the join named this
	 * ring: two for a skirt's lower ring pulled into an 8, one for a leg's top. Empty: not joined.
	 */
	TArray<int32> Joined;

	/**
	 * An edge `extend` grew rather than a ring built on the body (second slice §3): where each point
	 * sits round, as a fraction of the edge it grew from, and that edge's length. `$Top@u` is the point
	 * grown from `$Neck@u`, and `$Top@32cm` the one grown 32 cm round the neck. Empty: a ring counts
	 * round its own points.
	 */
	TArray<double> Places;
	double GrownFromLength = 0.0;

	double Length() const;

	/** Span in the ring's own side direction, perpendicular to its axis and centre-to-@0. */
	double SideWidthCm() const;
	/** Span toward @0 in the ring plane, used when matching another ring's opening size. */
	double DepthCm() const;

	/** The length a place in centimetres counts along: the ring's own, or the edge it grew from. */
	double CountingLength() const;

	/** A place as a fraction round from `@0`, in [0, 1). */
	double Fraction(const FGcGarmentPlace& Place) const;

	/** The fraction round of the place on the ring nearest `P`. */
	double PlaceOf(const FVector& P) const;

	/** A place on the ring — `@0.25`, `@32cm`, `@0.5+3cm`, `@0-5cm` — counted from `@0`. */
	FVector At(const FGcGarmentPlace& Place) const;

	/** The point `U` of the way round, a fraction counted as `At` counts. */
	FVector AtFraction(double U) const;

	/**
	 * Which way it counts, in the points its author wrote: `@0` and `@0.25` where they are, and, typed with three or more points, whether
	 * it runs in their order or the reverse - so which way `@0+cm` goes (the dry run's ring line, 2 Nov).
	 */
	FString CountingWay() const;
};

/**
 * A stretch of a ring that the build put somewhere else than the curve through its points, or the straight way across (G4, the check's
 * G4-1, `skirt-g4-slice1-17nov.md`). Its places are on the ring as built (`FGcGarmentBuiltRing::PlaceOf`, after `expand`); its depths are
 * the ring as found or drawn, before `expand`.
 */
struct GRAPHCODEGARMENT_API FGcGarmentRingRun
{
	/** Its two ends round the ring as built, fractions from `@0` (not ordered: the ring may count either way between them). */
	double FromU = 0.0;
	double ToU = 0.0;

	/** On a drawn ring, between its typed points `AfterPoint` and the next, counted from 0 in the order written; INDEX_NONE on a found ring. */
	int32 AfterPoint = INDEX_NONE;

	/**
	 * At its deepest, in centimetres: a pressed run, how far inside the body the curve through the points ran; a hollow, how far the skin
	 * (or a section's own loop) lies inside the curve or the straight way across.
	 */
	double DeepestCm = 0.0;

	/** How far the ring was moved there at that deepest place: a pressed run all of `DeepestCm`; a hollow `(100% - smooth)` of it. */
	double MovedCm = 0.0;

	/** The ring's point at the deepest place, as found or drawn. */
	FVector Deepest = FVector::ZeroVector;

	/** How many of the found or drawn ring's points it spans. */
	int32 Points = 0;
};

/** A typed point of a ring that was put on the skin: the ring passes through `Skin`, never through `Typed`. */
struct GRAPHCODEGARMENT_API FGcGarmentRingPointMoved
{
	/** Counted from 0 in the order written. */
	int32 Index = INDEX_NONE;
	FVector Typed = FVector::ZeroVector;
	FVector Skin = FVector::ZeroVector;
	double OffCm = 0.0;
};

/**
 * What `FGraphCodeGarmentRings::Build` did to a ring that its points do not show, measured again by `Report` from the same inputs - never
 * collected inside `Build`, whose rings and notes are untouched. Every move past the weld's resolution
 * (`FGcGarmentBody::SamePlaceCm`, the module's "one place") is here, however small: nothing is left out by a chosen size (rulings, 17 Nov).
 */
struct GRAPHCODEGARMENT_API FGcGarmentRingReport
{
	FString Name;
	int32 Line = 0;
	double SmoothPercent = 100.0;

	/** Drawn through three or more points; false for a ring found through one or two. */
	bool bDrawn = false;

	/** How many points were typed: a drawn ring's last span runs from the last back to the first. */
	int32 PointCount = 0;

	/** Each typed point that was not exactly on the skin, and so was put on it. */
	TArray<FGcGarmentRingPointMoved> Moved;

	/** A drawn ring only: each run where the curve through its points ran inside the body and was laid on the skin, so the ring there is further out. */
	TArray<FGcGarmentRingRun> Pressed;

	/**
	 * Each run where the skin lies inside the ring's way: on a drawn ring, the curve through its points standing off the skin; on a found
	 * ring, a hollow of the body's section under the straight way across it. At `smooth=100%` bridged (`MovedCm` 0); below it followed by
	 * `(100% - smooth)` of its depth.
	 */
	TArray<FGcGarmentRingRun> Hollows;
};

/** `LevelSection`: the body cut in one plane through a point, every loop the cut gives, and the one the point lies on pulled tight by `smooth=`. */
struct GRAPHCODEGARMENT_API FGcGarmentLevelSection
{
	/** How many closed loops the cut gave, and each one's length round, in the cut's order. */
	int32 LoopCount = 0;
	TArray<double> LoopLengths;

	/** The loop nearest the point - the one it lies on - and how far from it the point is in the plane. INDEX_NONE: the cut gave no loop. */
	int32 Chosen = INDEX_NONE;
	double PointOffCm = 0.0;

	/** That loop as a ring: its hull at `smooth=100%`, the loop itself at 0%, through the point. Empty when no ring could be made. */
	TArray<FVector> Points;

	/** Where the point is in `Points`. */
	int32 PointAt = INDEX_NONE;

	/** Each hollow of the chosen loop under the ring's straight way across, places as fractions round `Points` from the point. */
	TArray<FGcGarmentRingRun> Hollows;

	double Length() const;
};

/**
 * What a walk over the skin may not cross to get past a ring: a fan from the ring's middle to every
 * point, its rim a centimetre out of the skin along the skin's normal. It bends where the ring does,
 * which a flat disc through a V-neck does not; out from the middle would slide along the skin where a
 * V runs down the chest, and the skin there would be crossed outside the cap.
 */
class GRAPHCODEGARMENT_API FGcGarmentRingCap
{
public:
	FGcGarmentRingCap(const TArray<FVector>& Points, const FGcGarmentBody& Body);

	bool Crosses(const FVector& A, const FVector& B) const;

private:
	TArray<FVector> Rim;
	FVector Middle = FVector::ZeroVector;
	FBox Reach;
};

class GRAPHCODEGARMENT_API FGraphCodeGarmentRings
{
public:
	/**
	 * Each point's way across the ring along the skin — square to the ring, on the skin's tangent —
	 * turned to lean along `Axis`. A few centimetres along it from a point is the skin just past the
	 * ring on that side, which is how a walk over the skin says which side of a ring it reached.
	 */
	static TArray<FVector> AcrossSkin(const TArray<FVector>& Points, const FGcGarmentBody& Body, const FVector& Axis);

	/** How far a typed point may sit off the skin; nearer, it is moved onto it, and further is refused. */
	static constexpr double OnBodyCm = 1.0;

	/**
	 * How far the plane of a one-point ring may lean off square to the skin at its point. Past it a
	 * plane shaves a cap off a rounded surface, which is shorter and goes round nothing. The rig
	 * searched the same 30 degrees.
	 */
	static constexpr double MaxLeanDegrees = 30.0;

	static bool Build(const FGcGarmentRing& Declared, const FGcGarmentBody& Body, const FVector& Reference,
		FGcGarmentBuiltRing& Out, TArray<FString>& OutNotes, FString& OutRefusal);

	/**
	 * `ring($Source) { ... }` (second slice §6.1): `Source` as built, then each instruction in order on
	 * the whole ring — `move` and `rotate` in the mesh's axes, `shift` and `swing` in the ring's own:
	 * along is its direction, up runs from its middle toward its `@0`, side toward its `@0.25`, and
	 * each turn carries them. `expand` makes it bigger, out from its middle. Refused where any of it
	 * lies inside the body.
	 */
	static bool Copy(const FGcGarmentRing& Declared, const FGcGarmentBuiltRing& Source, const FGcGarmentBody& Body,
		FGcGarmentBuiltRing& Out, TArray<FString>& OutNotes, FString& OutRefusal);

	/**
	 * A flat ring pulled to a place off the skin (`join`, `GC-GARMENT-GRAMMAR.md` §10): in the ring's
	 * own plane, the body's loop nearest `Near` and `Through` with a string pulled tight round both —
	 * the ring's own rule, straight across what it spans — held off the skin at its `expand` again,
	 * less toward `Through`, whose own distance off the skin is where the strings from it start.
	 * `OutLobe` is closed, about a centimetre apart, starts exactly at `Through` and counts the way
	 * the ring does.
	 */
	static bool PullThrough(const FGcGarmentBuiltRing& Ring, const FGcGarmentBody& Body, const FVector& Near,
		const FVector& Through, TArray<FVector>& OutLobe, FString& OutRefusal);

	/**
	 * What `Build` did to `Built` that its points do not show (G4-1): typed points put on the skin, runs of a drawn ring's curve pressed
	 * onto the skin where the body bulges past it, and runs where the skin lies inside the ring's way (bridged at `smooth=100%`, followed
	 * below it). Measured again from `Declared` and `Body` with `Build`'s own steps, in a pass of its own: `Build` is not touched. Empty for a
	 * copied ring and for a band's edge (no typed points).
	 */
	static void Report(const FGcGarmentRing& Declared, const FGcGarmentBody& Body, const FGcGarmentBuiltRing& Built,
		FGcGarmentRingReport& OutReport);

	/** `Report` in words, one note per moved point and per run, every one (no floor): the texts the build prints (`skirt-g4-slice1-17nov.md`). */
	static TArray<FString> ReportNotes(const FGcGarmentRingReport& InReport);

	/**
	 * `Report` as ONE note for an outside author (G4 slice 2): every moved point and every run, by the points and places the author wrote, and
	 * what to write about it. Empty when the ring is exactly where its points put it. The build and the dry run print this line.
	 */
	static FString ReportLine(const FGcGarmentRingReport& InReport);

	/**
	 * The body cut in `Plane` (a level ring: through its point, square to the mesh's Z), every loop the cut gives counted, the loop
	 * `Point` lies on chosen (the nearest), and that loop pulled tight round and through `Point` by `smooth=` exactly as a ring found
	 * through one point is (`TightInPlane`). False, with the loops still counted, when the cut gives no loop or the nearest is further
	 * than `OnBodyCm` from the point (a found ring's own rule). Never a silent choice: the caller prints `LevelSectionNote`.
	 */
	static bool LevelSection(const FGcGarmentBody& Body, const FGcSectionPlane& Plane, const FVector& Point, double SmoothPercent,
		FGcGarmentLevelSection& OutSection);

	/** How many loops the cut gave and which one the ring is, in words (rulings G4: always printed). */
	static FString LevelSectionNote(const FString& RingName, int32 Line, double SmoothPercent, const FGcGarmentLevelSection& Section);
};
