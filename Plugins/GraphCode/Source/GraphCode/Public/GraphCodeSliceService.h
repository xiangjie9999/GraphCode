// Copyright OraStudio. All Rights Reserved.

// The live/file boundary for spatial tracking. The pure language remains in SliceGenerator.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"

class UWorld;

namespace GcsServiceToken
{
	inline constexpr const TCHAR* WorldMissing = TEXT("gcs_world_missing");
	inline constexpr const TCHAR* ProjectionFailed = TEXT("gcs_projection_failed");
	inline constexpr const TCHAR* EditedOutput = TEXT("gcs_output_edited");
	inline constexpr const TCHAR* EntryMissing = TEXT("gcs_entry_missing");
}

struct GRAPHCODE_API FGcsWriteResult
{
	FString Entry;
	FString Path;
	FString Text;
	bool bWritten = false;
};

/** The actor-level delta derived from old/new canonical level or external-actor projections. */
struct GRAPHCODE_API FGcsLevelChangeSet
{
	TSet<FString> Actors;
	/** New level sig when the trigger was the level; empty makes an actor-package trigger recompute it. */
	FString FromSignature;
	bool bActorSetChanged = false;
	bool bForceAll = false;
};

/** Collects one loaded level and maintains the local `.gcs` answers declared by its `.gcw`. */
class GRAPHCODE_API FGraphCodeSliceService
{
public:
	/** `Saved/GraphCode/Slices/Dungeon.enemies.gcs`. */
	static FString PathFor(const FString& LevelPackage, const FString& Entry);

	/** Generate one declaration from the loaded world without touching disk. */
	static bool Generate(UWorld* World, const FGcwDocument& Definition,
		const FGcwDeclaration& Declaration, FString& OutText, FString& OutRefusal);

	/** Compare an old level/external-actor `.gc` with its replacement before overwrite. */
	static bool CompareLevelProjections(const FString& Before, const FString& After,
		FGcsLevelChangeSet& OutChanges, FString& OutRefusal);

	/**
	 * Generate and atomically write declarations. When `bListenOnly`, omitted `listen:` means true.
	 * An existing file whose own `sig:` no longer matches is an authored proposal and is refused.
	 */
	static bool Write(UWorld* World, const FGcwDocument& Definition, bool bListenOnly,
		TArray<FGcsWriteResult>& OutResults, TArray<FString>& OutReports, FString& OutRefusal);

	/** Push only subscribed declarations affected by an already-computed level projection delta. */
	static bool WriteIncremental(UWorld* World, const FGcwDocument& Definition,
		const FGcsLevelChangeSet& Changes, TArray<FGcsWriteResult>& OutResults,
		TArray<FString>& OutReports, FString& OutRefusal);
};
