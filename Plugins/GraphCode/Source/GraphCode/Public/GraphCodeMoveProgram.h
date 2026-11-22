// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"

// What Core knows about a `.gcmove` (`Docs/GC-MOVE-GRAMMAR.md`): the header word and one seam, as `GraphCodeShapeProgram.h` does for Shape.

/**
 * WHAT A CLIP A `.gcmove` BUILT REMEMBERS ABOUT THE PROGRAM THAT MADE IT — `GC-MOVE-GRAMMAR.md` §8
 * step 3, and `FGcShapeAssetProvenance` with the nouns swapped. The keys and the reader are Core's
 * for Shape's reason: the stamp is a fact about the asset, and reflecting facts about an asset is
 * what a `.gc` is for; only the stamping is the product's.
 */
struct GRAPHCODE_API FGcMoveAssetProvenance
{
	/** The `.gcmove` this clip was built from, as the caller addressed it. */
	FString Source;

	/** A digest of the program text that built it. */
	FString Program;

	/** Semantic digest as built: bone motion/curves for a clip, or settings/hierarchy/graph for a constraint rig. */
	FString Motion;

	/** A stamp names both a source and a program; either alone is not one. */
	bool IsSet() const { return !Source.IsEmpty() && !Program.IsEmpty(); }

	/** The three asset-metadata keys, in one place so a writer and a reader cannot disagree. */
	static const TCHAR* SourceKey();
	static const TCHAR* ProgramKey();
	static const TCHAR* MotionKey();

	/** Read the stamp off an asset. False, and `Out` untouched, where there is no stamp. */
	static bool Read(const UObject* Asset, FGcMoveAssetProvenance& Out);

	/** Write the stamp. It does not save: the build's own save carries it. */
	static void Stamp(UObject* Asset, const FGcMoveAssetProvenance& In);
};

/** What building a motion program did, or would do. Filled by GraphCode Move; Core only carries it. */
struct GRAPHCODE_API FGcMoveBuildResult
{
	bool bRan = false;

	/** Every asset a `build { }` line put on disk, by package path, in declaration order. */
	TArray<FString> BuiltAssets;

	/** One line per create and per rebuild. */
	TArray<FString> Changes;

	TArray<FString> Notes;

	FString Message;
};

class GRAPHCODE_API FGcMoveBuildJob
{
public:
	virtual ~FGcMoveBuildJob() = default;
	virtual bool IsReady() const = 0;
	virtual void Cancel() = 0;
	virtual FString Progress() const = 0;
	virtual bool Finish(FGcMoveBuildResult& OutResult, FString& OutError) = 0;
};
using FGcMoveProgramStarter = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	TSharedPtr<FGcMoveBuildJob>& /*OutJob*/,
	FGcMoveBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;
DECLARE_MULTICAST_DELEGATE(FGcMoveJobsReleasing);

using FGcMoveProgramBuilder = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	FGcMoveBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;

/**
 * Does this program still hold the recipe for one clip it built? TEXT ONLY, nothing evaluated —
 * `FGcShapeRecipeResolver` with the nouns swapped, asked by the corpus sweep once per `move:` line.
 * True means a `build { }` line produces `PackagePath` AND names a clip the file defines.
 */
using FGcMoveRecipeResolver = TDelegate<bool(
	const FString& /*ProgramText*/,
	const FString& /*PackagePath*/,
	FString& /*OutReason*/)>;

class GRAPHCODE_API FGraphCodeMoveProgram
{
public:
	/** `gcmove 1` on the first line that is not a comment. */
	static bool LooksLikeMoveProgram(const FString& Text);

	/** The whole seam or none of it, as Shape's: a half-bound one answers `IsAvailable()` and then refuses. */
	static bool Register(EGcProduct Product, FGcMoveProgramBuilder Builder, FGcMoveRecipeResolver Resolver,
		FString& OutError);
	static bool RegisterStarter(EGcProduct Product, FGcMoveProgramStarter Starter, FString& OutError);
	static bool BeginBuild(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		TSharedPtr<FGcMoveBuildJob>& OutJob, FGcMoveBuildResult& OutResult, FString& OutError);
	/** Holders finish and release Move-owned jobs before its module unloads. */
	static FGcMoveJobsReleasing& JobsReleasing();
	static void Unregister(EGcProduct Product);

	/** Installed AND bound. */
	static bool IsAvailable();

	/** Build the program, or refuse naming GraphCode Move as the fix. */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcMoveBuildResult& OutResult, FString& OutError);

	/**
	 * The sweep's question: is the recipe for this clip still in the program that built it? False with
	 * `OutReason` is *the recipe is gone*; false with `OutError` is *no product here to ask*, which a
	 * Core-only install must not report as every move-built clip being broken.
	 */
	static bool HoldsRecipeFor(const FString& ProgramText, const FString& PackagePath,
		FString& OutReason, FString& OutError);
};
