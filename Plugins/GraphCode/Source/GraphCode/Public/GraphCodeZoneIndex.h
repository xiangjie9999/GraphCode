// Copyright GraphCode Sdn Bhd. All Rights Reserved.

// The live zone index — `GC-SPATIAL-AUTHORING-DESIGN.md` §14, task Z1.
//
// The impure half of the pair. `GraphCodeSpatial.{h,cpp}` is arithmetic that has never heard of a
// world; this walks one and fills its structs. Keeping the seam explicit is what lets the maths be
// asserted with no map loaded, so nothing here may drift back across it.
//
// IT READS AND DOES NOT WRITE. No actor, component, package dirty flag, selection or file changes.
// `AActor::GetActorLabel` is deliberately absent: its default argument MINTS a label on an actor
// that has none, which is a write performed by a read, and the handle here is `GetFName()` anyway
// (`WORLD-GC-DESIGN.md` §13.1).

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeSpatial.h"

class UWorld;

/**
 * What one walk of a level found.
 *
 * The two report arrays exist because the alternative is silence. An author who tags a sphere
 * `Zone:Dungeon` and gets back an empty list has been told their map is wrong when the truth is that
 * this index does not carry their shape yet.
 */
struct GRAPHCODE_API FGcZoneIndexResult
{
	/** Every box-shaped volume found, askable by actor and component name. */
	TArray<FGcZone> Zones;

	/**
	 * Shapes that are real and are NOT indexed, one line each: `Actor.Component (SphereComponent)`.
	 *
	 * v1's volume is an oriented box and a sphere is not one. Approximating it with its bounding box
	 * would answer containment questions confidently and wrongly at every corner, so the shape is
	 * named as unsupported instead of quietly widened.
	 */
	TArray<FString> UnsupportedShapes;

	/**
	 * A `Zone:` path carried by an actor with more than one box, so the name resolves to several
	 * volumes.
	 *
	 * REPORTED HERE AND ARBITRATED ELSEWHERE. The index's job is to say what is in the level; it does
	 * not get to pick which box the author meant, and it does not strip the path either — that would
	 * silently un-name something somebody named. §10's "resolves to more than one" refusal belongs to
	 * the query that asked, which is the only layer that knows whether the ambiguity matters.
	 */
	TArray<FString> AmbiguousZonePaths;
};

/**
 * Builds the zone list for a loaded level.
 */
class GRAPHCODE_API FGraphCodeZoneIndex
{
public:
	/** The tag prefix that makes a volume LISTABLE — §14.1. Authored by the user, read by us. */
	static const TCHAR* ZoneTagPrefix;

	/**
	 * Walk `World`'s persistent level and collect every box volume.
	 *
	 * The PERSISTENT level's own actor array, not `TActorIterator`: that walks every loaded level,
	 * and a query about this map must not silently answer about a streamed neighbour. Same choice
	 * `GraphCodeLevelProjection` made and for the same reason.
	 */
	static bool Build(UWorld* World, FGcZoneIndexResult& OutResult, FString& OutError);

	/** The `Zone:<path>` value on an actor's tags, or empty. First match wins; extras are ignored. */
	static FString ZonePathFromTags(const TArray<FName>& Tags);
};
