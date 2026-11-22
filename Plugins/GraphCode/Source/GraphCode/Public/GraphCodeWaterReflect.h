// Copyright OraStudio. All Rights Reserved.
//
// The seam between real water and the `water { }` block — `GC-TERRAIN-DESIGN.md` §12i, row L8's
// engine half, first direction.
//
// **REFLECT COMES BEFORE REALISE, AND THAT ORDER IS THE DESIGN'S** (§12i.0, restated in
// `GC-TERRAIN-DONE.md` §12j): reflection is what water is FOR — a body is a sparse parametric object
// whose intent is already symbolic, so it comes back as readable text with nothing lost, where a
// quarter-million heightmap samples never can. A realiser built first would be a writer with no
// corpus to check itself against.
//
// **THE SAME SHAPE AS `FGraphCodeTerrainRealize`, AND FOR THE SAME REASON.** Core owns the `.gcw`
// types, the grammar and the renderer; World owns `AWaterBody` and every decision behind it. The
// dependency runs one way, so Core names the capability and refuses it by name when World is absent
// — or, and this is new here, when World is present and built against a project with the `Water`
// plugin switched off. Both arrive as *this install cannot read water bodies*, which is the sentence
// a caller can act on ([[a-refusal-s-reason-is-a-claim]]).

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"
#include "GraphCodeWater.h"

class UWorld;
struct FGcwDocument;

/** What reading a level's water found, so a caller reports it rather than re-deriving it. */
struct GRAPHCODE_API FGcWaterReflectResult
{
	/**
	 * Every water entity in the level, as the block that would be written for it.
	 *
	 * A WHOLE BLOCK AND NOT A LIST OF ENTRIES, because a caller merging this into a document needs
	 * the block's own fields — and because `GcwWater::RenderWater` takes exactly this, so the text a
	 * caller shows is the text the reader would read back ([[a-read-answers-through-the-write-path]]).
	 */
	FGcWaterBlock Block;

	/**
	 * Body document name → the `.gc` `splines { }` handle for that body's own curve —
	 * `Bend` → `Bend.WaterSpline`. §13.4a, added 2026-08-21.
	 *
	 * **THE ONE FACT ONLY THIS PASS CAN SUPPLY, which is why it is a field and not a lookup.** The
	 * save hook in `GraphCodeWaterSave.cpp` is CORE's, and Core does not link the Water plugin — that
	 * is what `IsAvailable()` exists for — so it cannot walk `AWaterBody` actors and cannot reach the
	 * `USplineComponent` a handle is spelled from. Matching a reflected entry back to an actor by
	 * NAME inside that hook would be a second answer to *which actor is this body*, agreeing with
	 * this pass most of the time ([[two-carriers-of-one-fact-is-a-race]]). **The pass that walked the
	 * actors is the one that can name them** ([[a-read-answers-through-the-write-path]]).
	 *
	 * **KEYED BY THE BODY'S DOCUMENT NAME because that is the key the consumer already has**: the
	 * save hook finds its live counterpart with `EntryNamed(Live.Block, Entry.Name)`, so a second
	 * key would be a second way to fail to match. A body whose name this grammar cannot spell has no
	 * entry here and needs none — nothing can reference it either.
	 *
	 * **EMPTY IS NORMAL AND IS NOT A FAILURE.** A body the engine has not built a spline for, or one
	 * whose owner has no document name, contributes nothing; the handle is absent and the save hook
	 * leaves that body alone rather than writing a reference to an entry the `.gc` does not declare.
	 */
	TMap<FString, FString> ShapeHandles;

	/** How many of each thing were read, for a report that says what it covered. */
	int32 Bodies = 0;
	int32 Zones = 0;
	int32 Islands = 0;
	int32 Exclusions = 0;

	/**
	 * Anything the reflection could not put in the document, named.
	 *
	 * **NAMED RATHER THAN DROPPED IN SILENCE.** A mirror that quietly leaves something out is worse
	 * than one that refuses: the file looks complete, and the missing property is discovered when a
	 * realise puts the water back without it. Every entry here is one property or one actor and says
	 * which.
	 */
	TArray<FString> Skipped;

};

/**
 * World's implementation — read every water actor in a level into the block that describes them.
 *
 * There is no `bDryRun`: reflection is a READ, and a read that could write would be the thing
 * [[reading-must-not-write]] is about.
 */
using FGcWaterReflector = TDelegate<bool(
	UWorld* /*World*/,
	FGcWaterReflectResult& /*OutResult*/,
	FString& /*OutError*/)>;

/** What realising a `water { }` block did, so a caller reports it rather than re-deriving it. */
struct GRAPHCODE_API FGcWaterRealizeResult
{
	/** How many of each thing the run spawned or updated, so a report says what it covered. */
	int32 Bodies = 0;
	int32 Zones = 0;
	int32 Islands = 0;
	int32 Exclusions = 0;

	/**
	 * How many actors this run had to bring into being, of everything above.
	 *
	 * SEPARATE FROM THE COUNTS, because *updated three, spawned none* and *spawned three* are
	 * different sentences to a reader deciding whether a realise did what they meant — an idempotent
	 * second run reports the same bodies and zero spawns, which is the honest answer.
	 */
	int32 Spawned = 0;

	/**
	 * True when the run minted an `AWaterZone` a level did not have — the one creation `bCreate`
	 * gates (§12i.1). A body spawns freely because a document that names one plainly wants it; a
	 * zone is the level's own singular machinery, so making one is opt-in exactly as a landscape is.
	 */
	bool bCreatedZone = false;

	/** One line per entry realised, straight from the run. */
	TArray<FString> Applied;

	/**
	 * The bodies whose `bAffectsLandscape` this run SET TRUE, by name — the carve authority handed to
	 * Water's own brush (§12i.5).
	 *
	 * **CARRIED UP RATHER THAN LEFT IN `Applied`, because it is the one outcome here a caller must
	 * act on**: a carving body writes the shared `Water` edit layer, so a terrain `carve()` on the
	 * same course would double-cut. Naming which bodies carve is what lets the refusal in §12i.5 be
	 * checked against a real list ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	TArray<FString> Carved;

	/**
	 * Anything the realise could not put into the level, named.
	 *
	 * **NAMED RATHER THAN DROPPED IN SILENCE**, the same contract the reflector keeps in the other
	 * direction: a body whose `along=$Course` names a region this document does not declare, an
	 * `exclude` listing a body no entry named, a tail key that would not import. A realise that
	 * quietly left one out would produce a level that no longer round-trips to the document it came
	 * from.
	 */
	TArray<FString> Skipped;
};

/**
 * World's implementation — put a `water { }` block into a level as real actors.
 *
 * **`bCreate` GATES THE ZONE AND NOTHING ELSE** (§12i.1). A body is spawned whenever the document
 * names one that is not there, because a `water` block that lists a river plainly wants that river;
 * the `AWaterZone` is the level's singular derived-mesh machinery, and minting one silently would be
 * the terrain-creation trap one subject over — a realise that made a level's zone because it could
 * not find one.
 *
 * The `FGcwDocument` rides along because a body's geometry may be `along=$Course`, which is a region
 * the document declares and terrain's own resolver reads — a water line and a terrain line say a
 * place with the same characters because they run the same function ([[one-concept-two-grammars]]).
 */
using FGcWaterRealizer = TDelegate<bool(
	UWorld* /*World*/,
	const FGcwDocument& /*Document*/,
	const FGcWaterBlock& /*Block*/,
	bool /*bCreate*/,
	bool /*bDryRun*/,
	FGcWaterRealizeResult& /*OutResult*/,
	FString& /*OutError*/)>;

class GRAPHCODE_API FGraphCodeWaterReflect
{
public:
	/**
	 * Bind the reflector and the realiser together. World is the only product that may.
	 *
	 * **TWO DIRECTIONS NOW, WHERE THERE WAS ONE — AND THE SAME REASON TERRAIN BINDS TWO.** This call
	 * shipped with only the reflector because the realiser did not exist, and a second delegate then
	 * would have been a parameter every caller passed `nullptr` for — a promise with nothing behind
	 * it. The realiser exists (§12i.1, row L8), so it joins the call it was always going to: a product
	 * that could read a level's water and never write it back would leave authoring permanently
	 * half-built, and a refusal for either still names the ACT rather than the module. Both must be
	 * bound or the pair is refused, exactly as `FGraphCodeTerrainRealize::Register` refuses a half pair.
	 */
	static bool Register(EGcProduct Product, FGcWaterReflector Reflector, FGcWaterRealizer Realizer,
		FString& OutError);
	static void Unregister(EGcProduct Product);

	/**
	 * True when something is bound — which on this seam means BOTH that World is installed AND that
	 * it was built against a project with the `Water` plugin enabled.
	 *
	 * The two are one answer on purpose. A caller does not want to know which of them failed; it
	 * wants to know whether this install can read water, and `Reason()` says the rest.
	 */
	static bool IsAvailable();

	/**
	 * Why not, in a sentence naming the act. Empty when it is available.
	 *
	 * ITS OWN FUNCTION BECAUSE THE TWO CAUSES READ DIFFERENTLY: World absent is the ordinary
	 * missing-product sentence every seam already writes, and World present with `Water` off is a
	 * build switch — the same product, the same version, and a capability that is genuinely not
	 * there. Telling somebody to install World when they already have it is the failure this avoids.
	 */
	static FString Reason();

	/** Read a level's water. Refuses by name rather than returning an empty block. */
	static bool Reflect(UWorld* World, FGcWaterReflectResult& OutResult, FString& OutError);

	/**
	 * True when the document carries a `water { }` block at all — the test that decides whether the
	 * realiser is needed, so a `.gcw` with no water never asks for World.
	 *
	 * The mirror of `FGraphCodeTerrainRealize::Declares`, and the reason `build:"water"` can refuse
	 * *this file describes no water* before it ever reaches for a product.
	 */
	static bool Declares(const FGcwDocument& Document);

	/**
	 * Put a level's water there — spawn or update the actors a `water { }` block describes.
	 *
	 * Refused by the ACT when this install cannot write water, so a caller reads *this install cannot
	 * realise water bodies* rather than *GraphCodeWorld is missing* ([[a-refusal-s-reason-is-a-claim]]).
	 */
	static bool Realize(UWorld* World, const FGcwDocument& Document, const FGcWaterBlock& Block,
		bool bCreate, bool bDryRun, FGcWaterRealizeResult& OutResult, FString& OutError);
};
