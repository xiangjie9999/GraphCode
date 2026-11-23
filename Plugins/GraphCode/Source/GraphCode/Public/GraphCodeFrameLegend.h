// Copyright OraStudio. All Rights Reserved.

// The frame legend — `GC-SPATIAL-AUTHORING-DESIGN.md` §20.22, task Z3.
//
// WHAT IT IS FOR. `gc_render` hands back a picture, and `screen_to_world` turns a pixel of that picture
// into a world point. Between them sits the step nothing answered: WHICH PIXEL. A caller looking at an
// image can see a brown rectangle and guess it is the door. The legend replaces the guess — every named
// actor in the frame, where its rectangle sits in the image, how far away it is, and whether anything
// stands in front of it.
//
// IT IS BUILT FROM `FGraphCodeRenderCameraSolver::ProjectNormalized`, which is §4b's ruling reaching one
// function further: the legend, the picture and the deprojection all pass through the same two matrices.
// A legend with its own projection would agree with the picture until one of them drifted, and would
// then name an actor at a pixel showing something else — worse than no legend, because a caller points
// at it and gets a confident deprojection of the wrong surface.
//
// OCCLUSION IS MEASURED AND NEVER INFERRED. An actor behind a wall is in frame and is not visible, and
// listing it without saying so sends a caller to point at the wall.
//
// The projection half is PURE and takes a box, so every rectangle rule is assertable with no world, no
// RHI and no map. Only the trace needs a level.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRender.h"

class AActor;
class UWorld;

/** One actor as it appears in one frame. */
struct GRAPHCODE_API FGcFrameLegendEntry
{
	FString ActorName;
	FString ActorLabel;

	/**
	 * The actor's bounds as an image rectangle, normalized with `(0,0)` at the TOP-LEFT — the same
	 * convention `screen_to_world` takes, so a coordinate read here can be handed straight back.
	 *
	 * DELIBERATELY NOT CLAMPED. An actor off to the left of frame has a real position in image space,
	 * and how far out it is decides whether a caller pans or gives up.
	 */
	double MinX = 0.0;
	double MinY = 0.0;
	double MaxX = 0.0;
	double MaxY = 0.0;

	/** Where to point at it — the projected CENTRE, which is inside the rectangle and need not be its
	 *  middle: a box's centre and the middle of its projected corners differ under perspective. */
	double PointX = 0.0;
	double PointY = 0.0;

	/** Public V7 coordinates: integer pixels, `(0,0)` at the image's top-left. */
	int32 MinPixelX = 0;
	int32 MinPixelY = 0;
	int32 MaxPixelX = 0;
	int32 MaxPixelY = 0;
	int32 PointPixelX = 0;
	int32 PointPixelY = 0;

	/** Exact legend-mode mapping. `#RRGGBB`, stable for this ordered frame. */
	FString Color;

	/** View-space depth of the centre, centimetres. POSITIVE IS IN FRONT — see `ProjectNormalized`. */
	double Depth = 0.0;

	/** The rectangle meets the frame and the actor is in front of the camera. */
	bool bInFrame = false;

	/**
	 * Something reached it. MEASURED with a trace, never inferred from the rectangle.
	 *
	 * `false` with `bInFrame` true is the case this field exists for, and it is not rare: a prop behind
	 * a wall occupies image space and shows nothing.
	 */
	bool bVisible = false;

	/**
	 * How many of the sample points were reached, out of `SamplesTraced`.
	 *
	 * A RATIO RATHER THAN A FLAG, because half-hidden is the common case and a boolean makes a caller
	 * treat a sliver behind a pillar as it treats a clear view. The samples are the bounds' centre and
	 * its eight corners, so this is coarse ON PURPOSE — it is a hint about where to point, not a
	 * coverage measurement, and calling it one would be the confident-wrong-number §5 refuses.
	 */
	int32 SamplesVisible = 0;
	int32 SamplesTraced = 0;

	/** What stood in the way of the first blocked sample, when one was blocked. */
	FString Occluder;
};

/**
 * R6 — an authored region as a CAMERA SUBJECT: the box to frame and the basis to orbit.
 *
 * IT TAKES A LEVEL PATH AND NOT A WORLD, which is what makes the claim assertable with no editor, no
 * RHI and no map — the same property §19.8 gives every other question about a `.gcw`. The render
 * service holds a `UWorld*` and passes its package name; nothing here opens anything.
 */
struct GRAPHCODE_API FGcRegionFraming
{
	/** The prism in `BasisToWorld`'s frame, centred on the origin. */
	FBox Box = FBox(ForceInit);

	/** Rotation and translation only — the extent is the box's, never a scaled basis. */
	FTransform BasisToWorld = FTransform::Identity;

	/** No `height:` was stated, so the prism is flat and the picture frames the plan (§21.5). */
	bool bIsColumn = false;
};

/** The region half of the render camera — one function, pure apart from reading the `.gcw`. */
class GRAPHCODE_API FGraphCodeRegionFraming
{
public:
	/**
	 * `$Crypt` in the level beside `LevelPath` into a box and a basis, or a refusal that says why.
	 *
	 * `bLocalBasis` false orbits WORLD axes and widens the box to the world-axis extent of the same
	 * prism, because a rotated room turned back into world axes really is a bigger box — the caller
	 * is framing more than the room and the number says so.
	 *
	 * The `$` mark is accepted and not required: a name read out of a `gcw_ask` answer pastes in.
	 */
	static bool Resolve(const FString& LevelPath, const FString& RegionName, bool bLocalBasis,
		FGcRegionFraming& OutFraming, FString& OutRefusal);
};

/** One named edge projected through the exact render camera. */
struct GRAPHCODE_API FGcFrameRegionEdge
{
	/** Its one name — the `as:` alias went with the pool that needed it (§21.1b). */
	FString Name;

	FString Color;
	TArray<FIntPoint> Pixels;
};

/** One authored `.gcw` region in a rendered level frame. */
struct GRAPHCODE_API FGcFrameRegionEntry
{
	FString Name;
	FString Color;
	FIntPoint CentrePixel = FIntPoint::ZeroValue;
	double Depth = 0.0;
	TMap<FString, FIntPoint> Points;
	TArray<FGcFrameRegionEdge> Edges;
};

class GRAPHCODE_API FGraphCodeFrameLegend
{
public:
	/**
	 * A world box to its image rectangle. Pure — no world, no trace, no visibility.
	 *
	 * ALL EIGHT CORNERS, and the reason is that a box's projection is not a projected box: under
	 * perspective the near face covers more image than the far one, so a rectangle built from two
	 * opposite corners is smaller than the thing it is meant to bound.
	 *
	 * `false` when the box is degenerate or every corner is BEHIND the camera. A box that straddles the
	 * camera plane answers `true` with a rectangle covering what is in front of it, and the caller sees
	 * the negative-depth corners through `OutEntry.Depth` on the centre.
	 */
	static bool ProjectBounds(const FGraphCodeRenderCamera& Camera, int32 Width, int32 Height,
		const FBox& WorldBounds, FGcFrameLegendEntry& OutEntry);

	/**
	 * Every projected actor of a loaded level, as it appears in one view.
	 *
	 * The actor set is `FGraphCodeLevelProjection::IsProjectedActor`'s, which is the same set a level's
	 * `.gc` names and the same set `@anchor[target=…]` can reach — §20.11's one-vocabulary rule, so a
	 * name read off a legend is a name that can be written.
	 *
	 * Sorted nearest first: the thing a caller means is usually the thing in front.
	 *
	 * `Ignored` is R8's render-side ignore list — the actors this view LEFT OUT, by internal name.
	 * They get no row and they occlude nothing, because a legend is an index of the IMAGE: naming
	 * something that drew no pixels sends a caller to point at whatever is behind it, and reporting
	 * it as an occluder describes a picture nobody took.
	 */
	static TArray<FGcFrameLegendEntry> Build(UWorld* World, const FGraphCodeRenderCamera& Camera,
		int32 Width, int32 Height, const TArray<FString>& Ignored = {});

	/** Authored regions from the `.gcw` beside this loaded level's `.gc`; empty when none exists. */
	static TArray<FGcFrameRegionEntry> BuildRegions(UWorld* World,
		const FGraphCodeRenderCamera& Camera, int32 Width, int32 Height, int32 ColorOffset,
		TArray<FString>* OutWarnings = nullptr);

	/** Stable collision-free colour for indices below 2^24. */
	static FString ColorForIndex(int32 Index);
};
