// Copyright OraStudio. All Rights Reserved.
//
// The seam between a `terrain { }` program and a landscape — `GC-TERRAIN-DESIGN.md` §5, L1 and L3.
//
// **THE SAME SHAPE AS `FGraphCodeRegionGeneration`, AND FOR THE SAME REASON.** Core owns the `.gcw`
// types, parses the block and evaluates it in centimetres; World owns `ALandscape`, the edit-layer
// stack and every decision behind them. The dependency runs one way, so Core names the capability
// and refuses it by name when World is not installed, without ever needing World's code.
//
// A SEAM OF ITS OWN rather than a value on generation's: filling a region with PCG and shaping the
// ground are different questions, and a Core-only install has to be able to refuse each by its own
// name ([[a-refusal-s-reason-is-a-claim]]).

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"
#include "GraphCodeRegionFile.h"

class UWorld;

/** What realizing a program did, so a caller reports it rather than re-deriving it. */
struct GRAPHCODE_API FGcTerrainRealizeResult
{
	/** The landscape it landed on, by actor label. */
	FString Landscape;

	/** The edit layer it was written to — never the base heightmap (§5). */
	FString Layer;

	/** True when this run had to bring the landscape into being first (L3's door). */
	bool bCreated = false;

	/**
	 * `stream:tiles(components=N)` — what the header asked for, and what it comes to in pieces.
	 * Both zero when the program said nothing, which is the landscape as a single actor.
	 */
	int32 StreamComponents = 0;
	int32 StreamTiles = 0;

	/**
	 * True only when this run MOVED components into streaming proxies.
	 *
	 * False on a dry run and false on a program whose landscape is already cut that way — which is
	 * the ordinary state of a settled document, and is a different answer from *it was never asked*.
	 * `StreamComponents` above carries the ask; this carries the act.
	 */
	bool bStreamed = false;

	int32 SizeX = 0;
	int32 SizeY = 0;

	double MinHeight = 0.0;
	double MaxHeight = 0.0;

	/**
	 * WHAT THE LANDSCAPE ENDED UP HOLDING, against `MinHeight`/`MaxHeight`, which is what the
	 * PROGRAM computed — board row W1a, `GC-TERRAIN-DESIGN.md` §12a.6d.
	 *
	 * **TWO NUMBERS, BECAUSE ONE CANNOT TELL THE TWO FAILURES APART.** *The program computed
	 * nothing* and *the heights never landed* are different defects with different owners, and a
	 * single "it is flat" sends the next reader to whichever one they guessed
	 * ([[an-inert-feature-can-have-two-causes]]).
	 *
	 * **AND THE SECOND FAILURE IS REAL AND SILENT.** A program writes an EDIT LAYER, which is a CPU
	 * write and always lands; the layer is blended into the component heightmap ON THE GPU, on a
	 * tick. So on a host with no rendering device the write reports complete success and the ground
	 * stays a plane — measured 2026-08-22, program 4304 cm, landscape 0 — while every spec on this
	 * subject passes, because they read the layer through `FLandscapeEditDataInterface` rather than
	 * the ground. Reported rather than refused: the layer data is CORRECT and an editor blends it on
	 * load, so the only thing wrong is the derived heightmap, and the caller is owed the number
	 * rather than a refusal of a write that worked ([[a-silent-no-op-is-worse-than-a-refusal]]).
	 *
	 * `-1.0` means NOT MEASURED — a dry run, or no landscape to ask.
	 */
	double LandscapeRelief = -1.0;

	/** One line per instruction, straight from the evaluation. */
	TArray<FString> Applied;

	/**
	 * One line per `sculpt(…)` that ran on ground other than the one it was cut from — §6 #3.
	 *
	 * CARRIED UP RATHER THAN LEFT IN `Applied`, because it is the one thing here a caller must act
	 * on: everything else in the report says what happened, and this says what may be wrong. A
	 * deferred act owes an outcome, and this is that outcome ([[a-deferred-act-owes-an-outcome]]).
	 */
	TArray<FString> Rebased;

	/** Layer info assets the paint pass wrote, in the program's rule order. */
	TArray<FString> Painted;

	/**
	 * True when a `hole(…)` opened the ground — reported because what it does next is the caller's
	 * to finish, not ours.
	 *
	 * A mouth is a DOOR and the room behind it is a model somebody places: a heightfield holds one
	 * height per (x, y) and therefore cannot state a roof. So an opened landscape with nothing
	 * behind it is a pit, and saying so here is what stops that reading a success.
	 */
	bool bOpenedGround = false;
};

/**
 * What a capture took off a landscape — `GC-TERRAIN-DESIGN.md` §5, §6, rows L3 and L4.
 *
 * **THE INVERSE OF REALIZE, AND IT SITS BESIDE IT** ([[gc-placement-inverse]]). Realize turns a
 * program into ground; this turns ground into a file and the one line that reads it back.
 */
struct GRAPHCODE_API FGcTerrainSculptResult
{
	FString Landscape;

	/** Where the blob went, in full, and the same path as the line spells it. */
	FString File;
	FString Relative;

	int32 SizeX = 0;
	int32 SizeY = 0;

	double RangeMin = 0.0;
	double RangeMax = 0.0;

	/** The lowest and highest world Z the captured ground actually reaches. */
	double MinHeight = 0.0;
	double MaxHeight = 0.0;

	/** The digest of the ground this was cut from — for a first line, the program's flat base. */
	FString Base;

	/** The `sculpt(…)` line, rendered exactly as the block writes it. */
	FString Line;

	/**
	 * The same line as an instruction, so a caller that owns the document inserts THIS rather than
	 * rebuilding it from the fields above.
	 *
	 * **ONE BUILDER OR THEY DRIFT.** `Line` is for a person to read and paste; the save hook needs
	 * the structure, and a second assembly of the same fields is the shape that goes wrong the first
	 * time one is added ([[a-read-answers-through-the-write-path]]).
	 */
	FGcTerrainInstruction Instruction;

	/**
	 * True when the landscape's own heights were flattened, so the ground is now stated once — in
	 * the document (user-ruled 2026-08-19).
	 */
	bool bMoved = false;

	/** The edit layers whose heights were taken and then zeroed, by name. */
	TArray<FString> Cleared;

	/**
	 * Layers this build could not add up, left where they are and named — a hidden one, or one that
	 * blends rather than adds (the splines layer is the one every real landscape has).
	 *
	 * **NAMED RATHER THAN SKIPPED IN SILENCE.** Taking part of the ground into the document and
	 * leaving the rest on the landscape is the exact state this act exists to prevent, so a caller
	 * who has one of these has to be told which.
	 */
	TArray<FString> Skipped;

	/** True when the program's own generated layer held content that was excluded from the record. */
	bool bExcludedGenerated = false;

	/**
	 * True when a person had sculpted INSIDE the program's own `gc_` layer and that edit was taken
	 * with the rest.
	 *
	 * **THE DEFAULT LAYER A PERSON SCULPTS INTO IS WHICHEVER ONE IS SELECTED**, and after a realize
	 * that may well be ours. Excluding `gc_` wholesale — which is right for what the program itself
	 * wrote — would then throw a person's work away on the next realize, silently. What is ours is
	 * exactly what the program EVALUATES to, so anything else in that layer is a person's.
	 */
	bool bTookEditFromGeneratedLayer = false;

	/**
	 * The `sculpt` line this record was folded INTO, when the program already ended with one.
	 *
	 * User-ruled 2026-08-19: *"there will only be no two consequent sculpt, two deltas will be
	 * merged together."* Empty means the line is new and goes at the end.
	 */
	FString MergedInto;

	/** True when there was nothing on the landscape that was not already the program's. */
	bool bNothingToRecord = false;
};

/**
 * World's implementation. `bCreate` is the caller ASKING for a landscape that does not exist yet.
 *
 * **CREATION IS NEVER IMPLIED**, which is why it is a parameter rather than a fallback: a realize
 * that spawned an actor because it could not find one would turn a typo in `terrain <Name>` into a
 * second landscape nobody wanted, sitting on top of the first.
 */
using FGcTerrainRealizer = TDelegate<bool(
	UWorld* /*World*/,
	const FGcwDocument& /*Document*/,
	const FGcTerrainProgram& /*Program*/,
	bool /*bCreate*/,
	bool /*bDryRun*/,
	FGcTerrainRealizeResult& /*OutResult*/,
	FString& /*OutError*/)>;

/**
 * The other direction — a landscape's ground into a delta file and the line that reads it back.
 *
 * **BOUND IN THE SAME REGISTRATION AS THE REALIZER, WHICH IS THE EXCEPTION AND NOT THE RULE HERE.**
 * The three seams before this one are deliberately separate because each is a capability a caller
 * asks for by name. These two are one capability's two directions: a product that could shape ground
 * and never take it back would leave the human-sculpt migration (§5) permanently half-built, and the
 * refusal for either still names the ACT rather than the module, so a caller who asked to capture is
 * told about capturing. It is the shape `FGraphCodeActorFacets` already uses for the same reason.
 */
using FGcTerrainSculptRecorder = TDelegate<bool(
	UWorld* /*World*/,
	const FGcwDocument& /*Document*/,
	const FGcTerrainProgram& /*Program*/,
	const FString& /*DeltaName*/,
	bool /*bDryRun*/,
	FGcTerrainSculptResult& /*OutResult*/,
	FString& /*OutError*/)>;

class GRAPHCODE_API FGraphCodeTerrainRealize
{
public:
	static bool Register(EGcProduct Product, FGcTerrainRealizer Realizer, FGcTerrainSculptRecorder Recorder,
		FString& OutError);
	static void Unregister(EGcProduct Product);
	static bool IsAvailable();

	/**
	 * Record what a person changed on a landscape as a delta and the `sculpt(…)` line that reads it.
	 *
	 * **THE ONLY CALLER IS THE LEVEL SAVE HOOK, AND THAT IS THE WHOLE SURFACE** — user-ruled
	 * 2026-08-20: *"isn't sculpt equals to delta? auto update due to user change on the terrain isn't
	 * it? … why is it in GC apply."* There is one act, it happens when a person changes the ground
	 * and saves, and a tool parameter beside it was a second door onto the same thing
	 * ([[a-misplaced-capability-costs-a-name]] for the second time on this row). A zero delta records
	 * nothing, which is what makes an automatic hook cheap enough to always be on.
	 *
	 * Refused by the ACT when World is not installed, so *this install cannot record a sculpt* is
	 * what a caller reads rather than *GraphCodeWorld is missing* ([[a-refusal-s-reason-is-a-claim]]).
	 */
	static bool RecordSculpt(UWorld* World, const FGcwDocument& Document,
		const FGcTerrainProgram& Program, const FString& DeltaName, bool bDryRun,
		FGcTerrainSculptResult& OutResult, FString& OutError);

	/**
	 * True when the document carries a program at all — the test that decides whether the realizer is
	 * needed, so a `.gcw` with no `terrain { }` never asks for World.
	 */
	static bool Declares(const FGcwDocument& Document);

	static bool Realize(UWorld* World, const FGcwDocument& Document, const FGcTerrainProgram& Program,
		bool bCreate, bool bDryRun, FGcTerrainRealizeResult& OutResult, FString& OutError);
};
