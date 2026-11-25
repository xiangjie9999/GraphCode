// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeNavigation.h"
#include "GraphCodeProducts.h"
#include "GraphCodeRegionFile.h"

class UWorld;

/** What World's region-navigation compiler changed or would change. */
struct GRAPHCODE_API FGcRegionNavigationCompileResult
{
	bool bRan = false;
	bool bBuildRequested = false;
	bool bNeedsBuild = false;
	int32 CreatedActors = 0;
	int32 UpdatedActors = 0;
	int32 RemovedActors = 0;
	/** Qualified `Region.MeshId` identities, because stale is per logical mesh within a region. */
	TArray<FString> StaleRegions;
	TArray<FString> RegionChanged;
	TArray<FString> NavMeshChanged;
	TArray<FString> Changes;

	/**
	 * Bodies this compile ADDED to the project's Supported Agents, as `Mouse (radius=8, height=12)`.
	 *
	 * Said out loud because it is the one effect of a region write that reaches outside the level:
	 * the project settings now declare a mesh they did not before, and every level in the project
	 * generates it from here on. A dry run fills this with what it WOULD declare and writes nothing.
	 */
	TArray<FString> DeclaredBodies;

	/**
	 * Area-class assets this compile MINTED for per-mesh blockers, as `/Game/GraphCode/NavAreas/…`.
	 *
	 * Said out loud for `DeclaredBodies`' reason: it is content created outside the level, by a build
	 * nobody asked to create an asset. One asset per distinct agent set, shared by every region that
	 * blocks that set, and a dry run fills this with what it WOULD mint and writes nothing.
	 */
	TArray<FString> MintedAreas;

	/**
	 * Every boundary the build MEASURED, as `Hall.east <-> Store`, `Hall.east -> Store` (out only),
	 * `Hall.east <- Store` (in only) or `Hall.east -|- Store` (they meet and nothing gets through) —
	 * the same four facts the wall keys carry, in the order they were written.
	 */
	TArray<FString> Crossings;

	/**
	 * Why a boundary the geometry found is NOT stated in the file. A wall meeting a region that
	 * declares no navigation, a region that is not on the level's navmesh at all, and a stretch too
	 * thin to stand a probe in each land here rather than becoming a key nobody can trust.
	 */
	TArray<FString> ConnectivityNotes;

	FGcNavBuildResult Build;
};

/**
 * World's implementation of N2. Core owns the `.gcw` types and calls this seam; World owns every
 * actor and Recast decision behind them.
 */
using FGcRegionNavigationCompiler = TDelegate<bool(
	UWorld* /*World*/,
	const FGcwDocument* /*Existing*/,
	FGcwDocument& /*InOutDesired*/,
	bool /*bDryRun*/,
	bool /*bForceBuild*/,
	FGcRegionNavigationCompileResult& /*OutResult*/,
	FString& /*OutError*/)>;

class GRAPHCODE_API FGraphCodeRegionNavigation
{
public:
	static bool Register(EGcProduct Product, FGcRegionNavigationCompiler Compiler, FString& OutError);
	static void Unregister(EGcProduct Product);
	static bool IsAvailable();

	static bool Compile(UWorld* World, const FGcwDocument* Existing, FGcwDocument& InOutDesired,
		bool bDryRun, bool bForceBuild, FGcRegionNavigationCompileResult& OutResult,
		FString& OutError);
};
