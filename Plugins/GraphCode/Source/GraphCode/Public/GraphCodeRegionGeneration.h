// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"
#include "GraphCodeRegionFile.h"

class UWorld;

/** What World's region-generation compiler changed, or would change. */
struct GRAPHCODE_API FGcRegionGenerationCompileResult
{
	bool bRan = false;
	int32 CreatedActors = 0;
	int32 UpdatedActors = 0;
	int32 RemovedActors = 0;

	/** One line per create, update and remove, in the order they happened. */
	TArray<FString> Changes;

	/**
	 * What a generator could not be given, said out loud rather than left to a silent default.
	 *
	 * A graph parameter an override names but the graph does not declare is the case this exists for:
	 * the write refuses on it, and a note is what carries the near-misses that did not refuse.
	 */
	TArray<FString> Notes;
};

/**
 * World's implementation of G1. Core owns the `.gcw` types and calls this seam; World owns
 * `APCGVolume`, `UPCGComponent` and every PCG decision behind them.
 *
 * A SECOND SEAM RATHER THAN A SECOND VALUE ON NAVIGATION'S, because the two answer different
 * questions and a Core-only install must be able to refuse each by its own name.
 */
using FGcRegionGenerationCompiler = TDelegate<bool(
	UWorld* /*World*/,
	const FGcwDocument& /*Desired*/,
	bool /*bDryRun*/,
	FGcRegionGenerationCompileResult& /*OutResult*/,
	FString& /*OutError*/)>;

class GRAPHCODE_API FGraphCodeRegionGeneration
{
public:
	static bool Register(EGcProduct Product, FGcRegionGenerationCompiler Compiler, FString& OutError);
	static void Unregister(EGcProduct Product);
	static bool IsAvailable();

	/** True when the document's `populate { }` block holds anything. A property of ONE document —
	 *  the transition is `Reconciles` below, and the gate is that one. */
	static bool Declares(const FGcwDocument& Document);

	/**
	 * True when this WRITE has to run the reconciler — **the document it lands OR the one it
	 * replaces.**
	 *
	 * **THE SECOND HALF IS THE WHOLE POINT, and asking only about the desired document was a live
	 * defect** (row G1b, found 2026-08-20). This pass is a reconcile: create the missing, update the
	 * changed, **remove the orphaned**. Gated on the desired document alone, deleting one of two
	 * lines works — the survivor's pass sweeps the other — and deleting the ONLY line runs nothing,
	 * so the volume or the forest it made stays in the level with no file anywhere recording it.
	 * A removal is a change, and a gate that cannot see removals is a gate on the wrong question.
	 *
	 * **NAVIGATION HAS ANSWERED IT THIS WAY SINCE IT SHIPPED** — `bNeedsNavCompiler` ORs in the
	 * existing document beside the desired one — so this is that ruling applied rather than a new
	 * one. It is here in the SEAM rather than inline at the call site precisely because it was got
	 * wrong once by being written twice.
	 *
	 * **AND IT KEEPS BOTH PROPERTIES THE OLD GATE WAS PROTECTING**: a file that has never declared a
	 * generator still never asks for World, so a Core-only install reads and writes regions
	 * untouched, and a `.gcw` write on a level that is not open still succeeds when generation is
	 * not in play either side of the write.
	 *
	 * `Existing` is null for a file that does not exist yet, which declares nothing by definition.
	 */
	static bool Reconciles(const FGcwDocument& Desired, const FGcwDocument* Existing);

	static bool Compile(UWorld* World, const FGcwDocument& Desired, bool bDryRun,
		FGcRegionGenerationCompileResult& OutResult, FString& OutError);
};
