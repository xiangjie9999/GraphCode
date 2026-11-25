// Copyright GraphCode Sdn Bhd. All Rights Reserved.

// The spatial kernel — `GC-SPATIAL-AUTHORING-DESIGN.md` §14, task Z0.
//
// Every function here is pure arithmetic over the structs below: no world, no RHI, no asset, no map.
// That is the same contract `FGraphCodeRenderCameraSolver` holds and it exists for the same reason —
// the maths is proved before anything can write with it (§11, S0's exit).
//
// WHERE THE WORLD IS. Nothing here reads a level. A caller walks the world, fills `FGcZone` values,
// and asks. That seam is what makes the whole file assertable under `-nullrhi` with no map loaded.

#pragma once

#include "CoreMinimal.h"

/**
 * An oriented box in world space — v1's only zone shape.
 *
 * SCALE IS FOLDED INTO `HalfExtent` AND `Frame` CARRIES NONE. A component gives you both a scaled
 * transform and an unscaled extent, and keeping the scale on the frame double-counts it: the inverse
 * transform divides the point by the scale and the comparison then meets an extent that was already
 * multiplied by it. `FGcZone::FromComponent`'s callers do the folding once, here, rather than each
 * query rediscovering the rule.
 */
struct GRAPHCODE_API FGcZoneVolume
{
	/** Rotation and translation only. A scaled frame here is a bug, not a variation. */
	FTransform Frame = FTransform::Identity;

	/** Centimetres, already scaled. Unreal units throughout; the kernel accepts no other. */
	FVector HalfExtent = FVector::ZeroVector;
};

/**
 * HOW A RUN BENDS AT ONE OF ITS POINTS — the type word and the tangent pair, §21.19f.
 *
 * **THE VOCABULARY WAS CHOSEN BEFORE THIS STRUCT EXISTED AND IT IS WATER'S** (§21.19d, user-asked
 * 2026-08-24): a control point has spelled its interpolation as a bare word in the trailing bracket
 * — `linear`, `clamped`, `custom` — with `tangent=(in=…, out=…)` beside it, since the water grammar
 * shipped. A region course asking the same question in different words would be a second spelling of
 * one concept, which is what got the deleted `splines { }` block its `modes(curveauto)`
 * ([[one-concept-two-grammars]]). So the WORDS are checked by water's own `GcWater::IsLegalPointType`
 * — one function, so the two lists cannot drift — and only this carrier is new.
 *
 * **IT IS HERE, IN THE LOWEST HEADER BOTH SIDES ALREADY INCLUDE**, because a region point states it
 * and a resolved course carries it: `FGcRegionPoint::Curve` is the authored end and
 * `FGcPlacementTarget::CourseCurve` is the resolved one, and a struct owned by either would make the
 * other include it.
 *
 * **THE ENGINE'S ENUM IS DELIBERATELY NOT HERE.** `ESplinePointType` is the engine's and this is
 * Core's; the realiser maps the word, exactly as `FGcWaterPoint::Type` already does, so a typo
 * refuses at the parse rather than becoming a straight fence three steps later.
 */
struct GRAPHCODE_API FGcCourseCurve
{
	/** `linear` / `clamped` / `custom`. EMPTY is the ordinary curved point and is never written. */
	FString Type;

	bool bHasArrive = false;
	FVector Arrive = FVector::ZeroVector;

	bool bHasLeave = false;
	FVector Leave = FVector::ZeroVector;

	/** Nothing stated. The whole corpus, and what keeps a document with no curve words unchanged. */
	bool IsPlain() const { return Type.IsEmpty() && !bHasArrive && !bHasLeave; }
};

/**
 * One askable volume: an actor's shape component, plus whatever the author declared about it.
 *
 * `Path` is the value of a `Zone:<path>` tag and is EMPTY for the merely askable — §14.1's split.
 * Any actor with a shape component can be asked about by name; only a tagged one is listed.
 */
struct GRAPHCODE_API FGcZone
{
	/** The handle, and it is the actor's internal name — `WORLD-GC-DESIGN.md` §13.1. */
	FString Actor;

	/** Which shape component on that actor. An actor may carry more than one. */
	FString Component;

	/** From `Zone:<path>`, e.g. `Dungeon/BossRoom`. Empty means askable-but-not-listed. */
	FString Path;

	FGcZoneVolume Volume;
};

/** Whose forward. `offset=(forward=200)` is meaningless until this is answered — §3. */
enum class EGcFrame : uint8
{
	/** The anchor's own rotation. The default, because "in front of the door" means the door's front. */
	Actor,

	/** World axes, for a caller who means north rather than ahead. */
	World
};

/** An offset stated in the frame's terms rather than in X/Y/Z, which is why the frame must be named. */
struct GRAPHCODE_API FGcOffset
{
	double Forward = 0.0;
	double Right = 0.0;
	double Up = 0.0;
};

/**
 * A tag path claiming a nesting the geometry does not have — §14.3.
 *
 * Reported rather than refused: the author's tag is real authored state and this kernel does not get
 * to overrule it. What it does get to do is say the two disagree, so nobody trusts the string.
 */
struct GRAPHCODE_API FGcZoneNestingConflict
{
	/** Index into the zone list whose `Path` claims a parent. */
	int32 ChildIndex = INDEX_NONE;

	/** Index of the zone its path names. `INDEX_NONE` when the named parent does not exist. */
	int32 ParentIndex = INDEX_NONE;

	FString ClaimedParentPath;
};

/**
 * The zone half of the spatial surface. Pure, static, and asserted with no editor.
 */
class GRAPHCODE_API FGraphCodeSpatialSolver
{
public:
	/** Is the point inside the box? Boundary counts as inside — a wall is in the room it bounds. */
	static bool ContainsPoint(const FGcZoneVolume& Volume, const FVector& Point);

	/**
	 * Is `Inner` wholly inside `Outer`?
	 *
	 * Exact rather than approximate, and cheaply so: both shapes are convex, so testing `Inner`'s
	 * eight corners against `Outer` decides it. No sampling, no tolerance parameter to tune.
	 */
	static bool ContainsVolume(const FGcZoneVolume& Outer, const FGcZoneVolume& Inner);

	/** Cubic centimetres. The ranking key, so it is a function and not an inlined expression. */
	static double VolumeCm3(const FGcZoneVolume& Volume);

	/**
	 * Every zone containing the point, MOST SPECIFIC FIRST — §14.3's ruling.
	 *
	 * Specificity is the measured volume, never the tag path's depth, because a tag is a string
	 * somebody typed and the box is where the box actually is. Ties break on actor then component
	 * name so the order is total: a tie that falls back on iteration order lets the level's actor
	 * array decide the answer, and it changes when somebody adds an unrelated actor.
	 */
	static TArray<int32> RankContaining(const TArray<FGcZone>& Zones, const FVector& Point);

	/** Splits `Dungeon/BossRoom` into its segments. Empty for an untagged zone. */
	static TArray<FString> PathSegments(const FString& Path);

	/** The path a nested one claims as its parent — `Dungeon/BossRoom` → `Dungeon`. Empty at root. */
	static FString ClaimedParentPath(const FString& Path);

	/**
	 * Every zone whose tag path claims a parent that does not geometrically contain it.
	 *
	 * A missing parent is reported too, with `ParentIndex = INDEX_NONE`: a path naming a zone nobody
	 * authored is the same defect one step earlier.
	 */
	static TArray<FGcZoneNestingConflict> FindNestingConflicts(const TArray<FGcZone>& Zones);

	/** World location for an offset stated in the anchor's frame. Step 2 of §3's composition order. */
	static FVector ResolveOffset(const FTransform& Anchor, EGcFrame Frame, const FGcOffset& Offset);

	/**
	 * A deterministic point inside the volume — §8's hard requirement.
	 *
	 * A pure function of `(Seed, Index, Volume)` and of nothing else: no time, no actor count, no
	 * iteration order. Same document twice must produce the same transforms, or the second apply
	 * writes a diff and C18's second-pass-zero rule fails.
	 */
	static FVector SamplePoint(const FGcZoneVolume& Volume, int32 Seed, int32 Index);

	/**
	 * The per-element seed `SamplePoint` derives, exposed so a caller can reproduce one element.
	 *
	 * Deliberately NOT `HashCombine`. Determinism here is a format-level promise that has to survive
	 * an engine upgrade, and an engine helper can change its mixing without anything failing loudly —
	 * every stored transform would simply drift. This mix is written out so it cannot.
	 */
	static int32 ElementSeed(int32 Seed, int32 Index);
};
