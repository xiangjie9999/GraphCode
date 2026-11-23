// Copyright OraStudio. All Rights Reserved.

// Resolving §20's directives against a world somebody else owns — `GC-SPATIAL-AUTHORING-DESIGN.md`
// §20.9, and the door Play needed.
//
// ---------------------------------------------------------------------------
// WHY THIS IS A NEW HEADER RATHER THAN A PROMOTED ONE
// ---------------------------------------------------------------------------
//
// `GraphCodeLevelPlacement.h` already answers the four questions the resolver cannot ask itself, and
// it is PRIVATE on purpose: it is the level writer's half of a conversation with itself. Moving it
// wholesale into `Public/` would export `PlacementFields`, `ParentToWorld` and the socket walk — a
// dozen entry points to serve one caller who wants one of them.
//
// **What a caller outside this module actually wants is the sentence, not the vocabulary**: *these
// directives, that world, where does it go*. That is one function, and it lives here so the
// orchestration stays beside the solver rather than being reassembled by whoever calls it. §18.5's
// *no second resolver* rule is about the arithmetic; this is the same rule one level up, about the
// ORDER the impure pieces are assembled in — targets before sockets before regions, contacts and
// navigation supplied, `ResolveAll` in dependency order.
//
// ---------------------------------------------------------------------------
// A WORLD IS A WORLD
// ---------------------------------------------------------------------------
//
// Everything under here takes a `UWorld*` and always has. So this serves the editor world and a PIE
// world with no branch and no second route, which is what lets a running game be asked *put the
// crate against that wall* in the same words the level document uses.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeSpatialPlacement.h"

class AActor;
class UClass;
class UWorld;

/** One thing to place, and what is known about its size. */
struct GRAPHCODE_API FGcLivePlacementRequest
{
	/**
	 * The entry's own name. It is also the SEED for `in=` / `walkable=` sampling, so two entries with
	 * different names scatter without either one stating a number.
	 */
	FString Name;

	FGcPlacementDirectives Directives;

	/**
	 * The subject as it stands, when it already does.
	 *
	 * EXACTLY ONE OF THESE TWO, and which one is not a style choice: an actor supplies its real
	 * component bounds, and a class supplies what a Blueprint's construction script WILL build. A
	 * request with neither has no size, so `@surface` would bury it in the floor.
	 */
	const AActor* Actor = nullptr;

	/** The subject before it is spawned — measured first, because a refusal after the spawn is a half-write. */
	const UClass* CreateAs = nullptr;
};

/** One request and what it resolved to, in the order the resolve ran. */
struct GRAPHCODE_API FGcLivePlacementOutcome
{
	FString Name;

	FGcPlacementResult Result;
};

class GRAPHCODE_API FGraphCodeLivePlacement
{
public:
	/**
	 * Every request resolved against `World`, in DEPENDENCY order rather than the order given.
	 *
	 * A payload may anchor one entry to another it also places, so resolving in list order would make
	 * the answer depend on where somebody typed a line. Cycles refuse. `OutRefusal` carries the faults
	 * that belong to no single entry; per-entry ones ride on each outcome's `Result.Refusal`.
	 *
	 * **Contacts and navigation are supplied whenever there is a world**, so `@surface` sweeps real
	 * collision rather than approximating the level as a box — stairs, ramps and a lip on a table all
	 * arrive free, and `walkable=` asks the navmesh that is actually built.
	 */
	static bool Resolve(UWorld* World, const TArray<FGcLivePlacementRequest>& Requests,
		TArray<FGcLivePlacementOutcome>& OutOutcomes, FString& OutRefusal);
};
