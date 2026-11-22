// Copyright OraStudio. All Rights Reserved.

// The two read routes — `GC-SPATIAL-AUTHORING-DESIGN.md` §14.2 and §14.6, task Z2.
//
//   `read` answers about the document, `describe` about a THING, `search` about a SET,
//   `render` about the frame.
//
// This file is the engine under the middle two. Both are thin over `FGraphCodeSpatialSolver` (the
// arithmetic) and `FGraphCodeZoneIndex` (the walk), so containment is computed once and exposed
// twice — `gc_describe Dungeon` and `gc_search in:Dungeon` returning the same set is fine; building
// that set two different ways would not be.
//
// THE POSITION OF AN ACTOR IS ITS ORIGIN, and that is measured rather than preferred.
// `GraphCodeActorEntryWriter.cpp:743` writes `@placement` from `AActor::GetActorLocation()`, so the
// write surface's idea of where a thing is has to be the read surface's too. Test containment on the
// bounds instead and `@place[in=Dungeon]` could place an actor that `in:Dungeon` then fails to find.
// Bounds are REPORTED so a caller can see the difference; they do not decide membership.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeSpatial.h"
#include "GraphCodeZoneIndex.h"

class UWorld;

/**
 * What a caller pointed at. ONE grammar for every predicate here and every directive in §3 — a
 * second resolution rule would be a second set of refusals to keep in step.
 */
struct GRAPHCODE_API FGcAnchor
{
	enum class EKind : uint8
	{
		Unresolved,
		/** A placed actor, by its internal name — the handle `WORLD-GC-DESIGN.md` §13.1 rules. */
		Actor,
		/** A `Zone:` path, or an actor carrying a box asked for by name. */
		Zone,
		/** A literal `(x, y, z)`. */
		Point
	};

	EKind Kind = EKind::Unresolved;

	/** As the caller typed it, so a refusal can quote them back to themselves. */
	FString Text;

	/** The resolved position, whatever the kind. For a zone, its volume's centre. */
	FVector Location = FVector::ZeroVector;

	/** Set for `Zone`. `in:` needs one; `near:` does not. */
	FGcZoneVolume Volume;
	bool bHasVolume = false;

	/** Which index entry answered, for a zone. */
	int32 ZoneIndex = INDEX_NONE;
};

/** One containing zone, in the ranked answer. */
struct GRAPHCODE_API FGcZoneMembership
{
	FString Actor;
	FString Component;
	/** Empty when the volume is askable but untagged — §14.1. */
	FString Path;
	double VolumeCm3 = 0.0;
};

/** `gc_describe <anchor>` — the unary facts about one named thing. */
struct GRAPHCODE_API FGcSpatialDescription
{
	FString Anchor;
	/** `actor`, `zone` or `point`. */
	FString Kind;

	FVector Location = FVector::ZeroVector;

	/** The actor's world bounds. Reported, never used to decide membership — see the file header. */
	bool bHasBounds = false;
	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;

	/**
	 * The half-extent of the anchor's own box, when it has one.
	 *
	 * Separate from `BoundsExtent` because they answer different questions and can disagree by a lot:
	 * bounds are everything the actor renders, this is the volume it defines. A trigger twice the
	 * size of its mesh is normal and neither number is wrong.
	 */
	bool bHasVolume = false;
	FVector VolumeExtent = FVector::ZeroVector;

	/** Which way it faces, so an offset can be stated in its frame without a second query. */
	bool bHasOrientation = false;
	FVector Forward = FVector::ForwardVector;
	FVector Right = FVector::RightVector;
	FVector Up = FVector::UpVector;

	/** Smallest first — §14.3. Volume decides, never the tag path's depth. */
	TArray<FGcZoneMembership> ContainingZones;

	/** Actor handles inside, when the anchor is a zone. The zone's own actor is not its own content. */
	TArray<FString> Contents;

	/**
	 * Every socket this actor DECLARES, spelled as `Actor.Socket` — the writeable token, not the bare
	 * name — §20.18 and task P3.
	 *
	 * IT IS PART OF THE WRITE SURFACE RATHER THAN A NICETY. §20.11 makes the read and the write
	 * vocabularies one vocabulary, so a name `@anchor[target=…]` accepts has to be reachable without an
	 * unstable search. Declared sockets only: a skinned mesh answers `DoesSocketExist` for any BONE, and
	 * a rigger names `hand_r` without anybody choosing which way its forward points.
	 */
	TArray<FString> Sockets;

	/** Nesting conflicts, ambiguity and unsupported shapes in reach. Never silent. */
	TArray<FString> Notes;
};

/**
 * `gcw_ask <anchor> against=<reference>` — the RELATIVE read, §20.9.
 *
 * IT IS REQUIRED RATHER THAN A CONVENIENCE, and §20.9 says why: without it an author describes the
 * room, describes the door, and SUBTRACTS TWO POSITIONS — coordinate arithmetic, which is the exact
 * habit the placement grammar exists to remove. The query surface would be handing back the one
 * thing nobody is supposed to think in.
 *
 * THE ANSWER IS IN PLACEMENT GRAMMAR, so it pastes into `@anchor` with the numbers adjusted. That is
 * §20.11's measurability rule cashed: one vocabulary for asking and for telling, with no translation
 * step for a mistake to live in.
 *
 * **EVERY NUMBER HERE COMES FROM `FGraphCodePlacementSolver::Measure`** — the resolver run backwards,
 * which already exists for `.gcs` generation. §18.5's *no second resolver* rule is why this struct
 * holds values instead of computing them: a convention changed in the writer and not here is a
 * document that reads back as a different world with nothing erroring.
 */
struct GRAPHCODE_API FGcRelativeDescription
{
	/** As the caller spelled them. */
	FString Subject;
	FString Reference;

	/**
	 * BOTH answerable spellings, because §20.9 wants the author to choose which edit stays true and
	 * they cannot choose without seeing both. Rows and grids read in `forward`/`right`; rings read in
	 * `bearing`/`offset`. One `Measure` call each, same inputs.
	 */
	double Forward = 0.0;
	double Right = 0.0;
	double Bearing = 0.0;
	double Offset = 0.0;

	/** The anchor's own up, which is why it is not part of the polar pair. Stated even at zero. */
	double Up = 0.0;

	/**
	 * `Percent` IS NOT ANSWERED, and that is the resolver's ruling rather than a gap here.
	 * `FGcMeasuredPlacement` refuses it because a percentage of the reference's half-extent measures
	 * the same standing actor differently the day somebody resizes the box. Answering one here would
	 * mean computing it outside `Measure`, which is the second arithmetic §18.5 forbids. The extent
	 * below is reported instead, so a caller who wants a percentage can see what it would be OF.
	 */
	bool bHasExtent = false;
	FVector Extent = FVector::ZeroVector;

	/**
	 * The departure from *aimed at the reference* — §20.7c's baseline on all three axes, so zero
	 * everywhere is pointed straight at it and level.
	 */
	bool bHasFacing = false;
	double Yaw = 0.0;
	double Pitch = 0.0;
	double Roll = 0.0;

	/**
	 * The subject stands exactly where its reference does, so there is no direction between them.
	 *
	 * REPORTED AND NEVER INVENTED, matching the write side's `facing_undefined`: the position is
	 * still answered, and only the facing is withheld.
	 */
	bool bFacingUndefined = false;

	/**
	 * AGAINST A WHOLE COURSE, WHERE ALONG IT — §22.6's third shape, owed since §22.17 gave a `.gcs` entry
	 * a station. In the words that entry writes: `t=` in centimetres, then `forward`/`right`/`up` from
	 * the frame at that station, and the facing departure from `toward=`. Absent for anything that is
	 * not a course, which keeps it the same door the generator uses rather than a reading of its own.
	 */
	bool bHasStation = false;
	FString Station;
	double StationForward = 0.0;
	double StationRight = 0.0;
	double StationUp = 0.0;
	bool bHasStationFacing = false;
	double StationYaw = 0.0;
	double StationPitch = 0.0;
	double StationRoll = 0.0;

	/** Anything the resolution had to say — an unrotatable reference, a missing box. Never silent. */
	TArray<FString> Notes;
};

/** `gc_search` — the set half. Both filters are optional; together they intersect. */
struct GRAPHCODE_API FGcSpatialSearchRequest
{
	/** A zone path or an actor carrying a box. Empty is no filter. */
	FString In;

	/** An anchor to measure from. Empty is no filter. */
	FString Near;

	/**
	 * REQUIRED whenever `Near` is set, and that is not pedantry: a bare `near:Door_01` either means
	 * nothing or means an invisible default, and §5 already ruled that a number nobody stated must
	 * not decide geometry.
	 */
	TOptional<double> Within;
};

struct GRAPHCODE_API FGcSpatialSearchResult
{
	/** Matching actor handles, sorted, so the same level answers the same way twice. */
	TArray<FString> Actors;

	/** What was actually applied, echoed back — a filter that silently did nothing reads as a match. */
	FString AppliedFilters;

	TArray<FString> Notes;
};

/**
 * Both routes. Read-only throughout: nothing here dirties an actor, a package or a selection.
 */
class GRAPHCODE_API FGraphCodeSpatialQuery
{
public:
	/** `(1500, 500, 0)` or `1500,500,0`. Pure — no world, so it is asserted without a map. */
	static bool ParsePoint(const FString& Text, FVector& OutPoint);

	/**
	 * Resolve one anchor against a built index.
	 *
	 * Order is literal point, then zone path, then actor — and a name answering as BOTH a zone path
	 * and an actor refuses as ambiguous rather than picking. §10 owes that refusal; guessing here
	 * would spend it silently.
	 */
	static bool ResolveAnchor(UWorld* World, const FGcZoneIndexResult& Index, const FString& Text,
		FGcAnchor& OutAnchor, FString& OutError);

	static bool Describe(UWorld* World, const FString& Anchor,
		FGcSpatialDescription& OutDescription, FString& OutError);

	/**
	 * `Anchor` measured in `Reference`'s frame — §20.9, and the read half of `@anchor`.
	 *
	 * IT TAKES A `UWorld*` LIKE EVERYTHING ELSE HERE, which is what makes it answerable about a
	 * RUNNING game: a PIE world is a world, so the same call serves `gcw_ask` in the editor and
	 * during a play session without a second route.
	 *
	 * Both names resolve through `ResolveAnchor`, so an actor, a zone path and a literal point all
	 * work on either side and refuse with the words the rest of this surface already uses.
	 */
	static bool MeasureAgainst(UWorld* World, const FString& Anchor, const FString& Reference,
		FGcRelativeDescription& OutMeasured, FString& OutError);

	static bool Search(UWorld* World, const FGcSpatialSearchRequest& Request,
		FGcSpatialSearchResult& OutResult, FString& OutError);
};
