// Copyright OraStudio. All Rights Reserved.
//
// A person drags a river's control point and saves; the document stops claiming it drew that shape
// — `GC-TERRAIN-DESIGN.md` §12i.4, row L8's last open item.
//
// **THIS IS THE SAME ACT AS TERRAIN'S SCULPT HOOK, ON THE ONE SUBJECT WHERE IT IS CHEAP.** A
// person's change to the ground costs a quarter-million samples and a blob file beside the
// document; a person's change to a water body costs a few dozen control points, which are readable
// text. So where terrain COMPOSES — a delta stacked on the program that made it — water REPLACES:
// a spline is an IDENTITY, not an accumulation, and there is no such thing as half a river
// ([[gc-terrain-delta]] is the contrast, and it is deliberate).
//
// **WHAT IS WRITTEN IS `[override=$Bend.WaterSpline]`, AND THE INTENT LINE IS KEPT WHOLE** — §13.4a,
// user-ruled 2026-08-20, built 2026-08-21. `along=$Valley.spine` says *I drew this from that
// course*; the moment the shape stops matching the course that sentence is false, and a document
// that went on saying it would re-straighten the person's river on the next realise. So the line
// stops CLAIMING the shape and starts POINTING at it: the override names the `.gc` `splines { }`
// entry that now owns the curve, and `FGraphCodeWorldWater::Realise` reads the mark and applies
// everything except the course.
//
// > **SUPERSEDED, and recorded because the shape of the old answer explains this one.** Until
// > 2026-08-21 this hook COPIED the control points into the `.gcw` and pushed the intent into a `#`
// > comment above the entry. That lost the machine-readable intent — provenance nobody can act on —
// > and left the same curve written in two files, which is what the user's *"why is there two
// > splines? it should only be one and it should be in .gc"* struck out.
//
// **AND DELETING THE BRACKET IS HOW AN AUTHOR SAYS *REBUILD THIS***. With no override the realise
// draws the course again, the shape matches the intent, and this hook has nothing to record — so
// the mark is the switch as well as the record.
//
// **PRE-SAVE, ADVISORY, AND NEVER ABLE TO FAIL A SAVE**, exactly as `FGraphCodeTerrainSave` is. A
// person pressing save is not asking our permission, and a refusal here means the shape stays in
// the `.umap` for now — which is where it was a moment ago.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeWater.h"

class UWorld;
struct FGcwDocument;

/** What one save does to one water entry. */
enum class EGcWaterEntryUpdate : uint8
{
	/** Nothing moved, or nothing here can be said about it. The entry is not touched. */
	None,

	/**
	 * The entry already CARRIED its points, so the new ones simply replace them.
	 *
	 * No provenance is lost and none is recorded: the line this rewrites never said where its shape
	 * came from, so there is no intent to keep and nothing to point at.
	 */
	Restate,

	/**
	 * The entry stated an INTENT, a hand moved the shape away from it, and the line now points at the
	 * `.gc` entry that owns the curve — §13.4a. The intent is kept exactly as written.
	 */
	Reference,
};

/**
 * What one save decided about one entry — the whole rule, with no world and no file in it.
 *
 * **SPLIT OUT SO THE RULE IS GATEABLE.** `RecordForWorld` needs a loaded level, the Water plugin and
 * a `.gcw` on disk beside a real `/Game/` package, which is why the hook shipped with no spec over
 * its decision at all. The decision itself is arithmetic over two documents and a name, so it is
 * taken here and APPLIED there. This is the shape volumes already use for the same question
 * (`FGcwVolumeVerdict`/`Classify`), which §13.4a is making one language across all three subjects.
 */
struct GRAPHCODE_API FGcWaterEntryVerdict
{
	EGcWaterEntryUpdate Update = EGcWaterEntryUpdate::None;

	/** `Reference` — the `.gc` handle to put in `[override=$…]`, bare, with no `$`. */
	FString Override;

	/** `Restate` — the points to write in place of the ones the entry carried. */
	TArray<FGcWaterPoint> Spline;
};

class GRAPHCODE_API FGraphCodeWaterSave
{
public:
	/**
	 * What this save should do to one entry — see `FGcWaterEntryVerdict`.
	 *
	 * `Handle` is the `.gc` `splines { }` name for this body's own curve, from
	 * `FGcWaterReflectResult::ShapeHandles`; empty when the level cannot spell one, which is
	 * deliberately NOT the same as *nothing moved* and is why it is a parameter rather than a lookup.
	 */
	static FGcWaterEntryVerdict Classify(const FGcwDocument& Document, const FGcWaterBody& Entry,
		const FGcWaterBody& Reflected, const FString& Handle);

	/** Bind the pre-save hook. Idempotent. */
	static void Install();
	static void Uninstall();

	/**
	 * Do the whole act for one world, or say why not — the body of the hook, exposed so it is
	 * testable without saving anything.
	 *
	 * Returns false ONLY on a refusal a caller should hear about. *Nothing had moved* and *this level
	 * declares no water* are successes with `OutRecorded` false: they are the ordinary answers on
	 * almost every save of almost every level, and a hook that warned on each would be noise nobody
	 * reads.
	 */
	static bool RecordForWorld(UWorld* World, bool& OutRecorded, FString& OutSummary,
		FString& OutError);
};
