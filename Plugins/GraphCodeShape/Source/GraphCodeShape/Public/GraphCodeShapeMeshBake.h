// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

struct FGcShapeDocument;
struct FGcShapeHeader;

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **`[maps=(normal, ao, curvature, height), from=$Detail, resolution=2048]` — O1, normal-map baking from one
 * recipe** (`GC-SHAPE-MESH-QUALITY-PROPOSAL.md` §8, "O1 design", PROPOSED spelling).
 *
 * A build line that names `maps=` bakes those maps for ITS OWN mesh — the light one, usually under a
 * `[triangles=]` budget — from a detailed source the same file builds: by default the line's own entry as
 * the program made it, before the budget; `from=$Entry` names another `mesh()` entry instead (a detail
 * variant). Both come from one recipe, so the bake needs no hand-made cage and repeats exactly.
 *
 * **IT IS A BUILD-LINE MODIFIER FOR `[bake=]`'S OWN REASON, INVERTED.** A material bake target is SHARED
 * (one atlas for many meshes), so its settings live on a `bake()` statement. A geometry bake is not: each
 * light mesh has its own UVs and its own maps, so its settings belong to its own line. `[bake=]` keeps its
 * meaning, and the two do not combine (a material bake re-lays UV0 after the mesh is written).
 */
struct GRAPHCODESHAPE_API FGcShapeMeshMaps
{
	enum EMap : uint8
	{
		Normal = 1 << 0,
		Occlusion = 1 << 1,
		Curvature = 1 << 2,
		Height = 1 << 3,
	};

	/** The maps asked for, as `EMap` bits. Zero means the line bakes nothing. */
	uint8 Maps = 0;

	/** `from=$Entry` without its `$`; empty means the line's own entry before its budget. */
	FString From;

	/** `resolution=`, square, in pixels; zero is unstated (1024, `bake()`'s own default). */
	int32 Resolution = 0;

	/**
	 * `supersample=N` (O1c, PROPOSED): the maps are baked at N times their size and averaged down N x N, so an edge that
	 * crosses a texel reads as a share of it instead of a step. Zero is unstated: **2 since O1d** (2026-11-13); 1 to 4,
	 * and `1` turns it off. The bake is at most 4096 square, so N falls to fit (a 4096 map is baked at 1). O1c left it off
	 * because the normal jump on a can where a smooth UV border meets a hard edge rose from 1.2 degrees at 1 to 9.8 at 2;
	 * the cause was samples on the crease reading the other face (`GraphCodeShapeMeshBake.cpp`, the detail sampler), and
	 * with that fixed the can reads 1.0 degree at 1, 2 and 3 alike.
	 */
	int32 Supersample = 0;

	/**
	 * `uv_channel=` (P-G15, agent-spelled, lead-approved 2026-11-18): the UV channel the maps' own layout lives in.
	 * Unstated (0, `bUvChannelAuto` false) is the bake as it always was: UV0, the bake's layout replacing it where it is
	 * not one a map can live in. `uv_channel=auto` keeps UV0 for the maps when it can hold them (already unique, or
	 * empty) and otherwise leaves the author's UV0 alone and lays the maps out in UV1; `uv_channel=1` always lays them
	 * out in UV1. The tangent frame is UV0's in every case. 2 to 7 are refused for now.
	 */
	int32 UvChannel = 0;
	bool bUvChannelAuto = false;

	bool IsSet() const { return Maps != 0; }
	bool UvChannelStated() const { return UvChannel != 0 || bUvChannelAuto; }
	bool Has(const EMap Map) const { return (Maps & Map) != 0; }
	int32 Size() const { return Resolution > 0 ? Resolution : 1024; }
	int32 Grid() const { return Supersample > 0 ? Supersample : 2; }

	/**
	 * One build-line modifier. `bOurs` is false (and nothing is written) for a key that is not `maps`,
	 * `from`, `resolution`, `supersample` or `uv_channel`; true with a false return is a refusal in `OutError`.
	 */
	static bool ReadModifier(const FGcShapeDocument& Document, const FString& Key, const FString& Value,
		FGcShapeMeshMaps& Out, bool& bOurs, FString& OutError);

	/** The cross-checks that need the whole bracket: `from=`/`resolution=` without `maps=`, and `[bake=]` with it. */
	static bool Validate(const FGcShapeMeshMaps& Maps, const FString& BakeTarget, FString& OutError);

	/** `normal`, `ao`, `curvature`, `height`. */
	static const TCHAR* Word(EMap Map);

	/** The texture asset beside the mesh: `<folder>/T_<Stem>_Normal`, Stem the mesh name without `SM_`. */
	static FString TexturePath(const FString& MeshPackagePath, EMap Map);
};

namespace GraphCodeShapeMeshBake
{
	/** One baked map, quantized exactly as it is written to the texture's source. */
	struct FImage
	{
		FGcShapeMeshMaps::EMap Map = FGcShapeMeshMaps::Normal;
		int32 Size = 0;
		/** 0: BGRA8 (normal), 1: G8 (ao, curvature), 2: G16 (height). */
		uint8 Format = 0;
		TArray64<uint8> Pixels;
	};

	/** What the build line and its note say. Every number is measured, never estimated. */
	struct FReport
	{
		int32 LightTriangles = 0;
		int32 DetailTriangles = 0;

		/** The two-sided distance between the light and the detailed surface, in cm: max, and mean over the detail by area. */
		double DeviationMax = 0.0;
		double DeviationMean = 0.0;
		UE::Math::TVector<double> DeviationAt = UE::Math::TVector<double>::ZeroVector;

		/** The ray distance the bake used: the measured maximum plus a margin. */
		double RayDistance = 0.0;

		/**
		 * Texels the light mesh's UV0 covers; of them, how many had no ray along the light normal reach the detail (they
		 * borrow the nearest detailed point, or a flat normal), and how many met a detailed face turned away.
		 */
		int64 Covered = 0;
		int64 Missed = 0;
		int64 Wrong = 0;
		/** Up to four places for each, as "N at (x, y, z)", worst first. */
		FString MissedWhere;
		FString WrongWhere;

		/** Texels per metre of surface: the area-weighted median, and the 5th and 95th percentiles. */
		double Density = 0.0;
		double DensityLow = 0.0;
		double DensityHigh = 0.0;
		/** Share of the square the UV0 islands cover. */
		double Coverage = 0.0;

		/** Samples per texel side the maps were baked with (`supersample=`). */
		int32 Supersample = 1;
		/** Where the bake laid out its own UV0: how many packs it tried before keeping the fullest that fit (O1c). */
		int32 PackTries = 0;
		/** Wall-clock seconds of the unwrap and of the bake proper (not the same twice; never part of the maps). */
		double UnwrapSeconds = 0.0;
		double BakeSeconds = 0.0;

		/** Across UV0 island borders where the light mesh shades smooth: the normal jump, degrees. */
		double SeamMax = 0.0;
		double SeamMean = 0.0;
		int32 SeamSamples = 0;
		/** Seam samples left out because the light surface there is buried inside another light shell (O1d): never seen. */
		int32 SeamBuried = 0;
		UE::Math::TVector<double> SeamAt = UE::Math::TVector<double>::ZeroVector;

		/** Non-empty where the bake laid out its own UV0, saying what it replaced, or its own UV1, saying UV0 was kept. */
		FString UvNote;

		/** The UV channel the maps were laid out in, and the one a material samples them with (P-G15). */
		int32 UvChannel = 0;
	};

	struct FBaked
	{
		FReport Report;
		TArray<FImage> Images;
	};

	/**
	 * WORKER THREAD. On the FINISHED light mesh (budget, pivot, normals done) and a detail mesh in the same
	 * frame: make UV0 a unique layout where it is not one (a deterministic unwrap after the budget), write
	 * MikkTSpace tangents into the light mesh so the asset renders in the frames the map is baked in, measure
	 * the deviation, then bake with the engine's `FMeshMapBaker`. A dry run bakes the normal map alone, which
	 * is every number the report owns; a real build bakes every map asked for.
	 */
	GRAPHCODESHAPE_API bool Bake(const FGcShapeMeshMaps& Spec, UE::Geometry::FDynamicMesh3& Light,
		UE::Geometry::FDynamicMesh3& Detail, bool bDryRun, TFunctionRef<bool()> Cancelled, FBaked& Out,
		FString& OutError);

	/**
	 * **THE TEXEL DENSITY A LIGHT MESH WILL GET, BEFORE IT IS BAKED** (O1c): the same UV0 decision `Bake` makes — the
	 * author's layout where it is one a map can live in, the bake's own unwrap otherwise — on a copy, and the
	 * area-weighted median texels per metre and the fill of the square that follow. `imprint()` reads it to keep as
	 * geometry what the maps could not draw. Deterministic, as the unwrap is.
	 */
	GRAPHCODESHAPE_API bool PlanDensity(const FGcShapeMeshMaps& Spec, const UE::Geometry::FDynamicMesh3& Light,
		double& OutDensity, double& OutFill, FString& OutError);

	/** The measure line's field: `maps normal+ao 2048 from $E: triangles 4998 of 87402, …`. */
	GRAPHCODESHAPE_API FString LineField(const FGcShapeMeshMaps& Spec, const FString& FromEntry, const FBaked& Baked);

	/** The note carrying the places and what each number means. */
	GRAPHCODESHAPE_API FString Note(const FString& EntryName, const FBaked& Baked);

	/**
	 * GAME THREAD. Each image as a `UTexture2D` beside the mesh, saved: a normal map with normal-map
	 * compression, the rest grayscale, all linear. A texture already there is replaced in place only when this
	 * program made it (its package carries the program's path); anything else at that path is refused.
	 */
	GRAPHCODESHAPE_API bool Write(const FString& MeshPackagePath, const FString& DocumentPath, const FBaked& Baked,
		TArray<FString>& OutWritten, FString& OutError);

	/**
	 * **A LIGHTMAP MAY NOT BE GENERATED INTO THE MAPS' CHANNEL (P-G15).** The engine's mesh build writes a generated
	 * lightmap into `DstLightmapIndex` (1 by default) over whatever is there, so maps laid out in UV1 on an asset whose
	 * `.gc` turns `bGenerateLightmapUVs` on would be sampled through the lightmap's layout. False, with the fix named,
	 * where `Existing` (the asset already at the path, if any) would do that to `Channel`; channel 0 is never checked.
	 * GAME THREAD.
	 */
	GRAPHCODESHAPE_API bool CheckLightmap(const class UStaticMesh* Existing, int32 Channel, FString& OutError);

	/**
	 * `CheckLightmap` against whatever static mesh is at `MeshPackagePath` — loaded in this editor (saved or not) or on
	 * disk; none is a pass. The build calls it straight after `Bake`, on BOTH paths, so a dry run refuses what the real
	 * build would. GAME THREAD.
	 */
	GRAPHCODESHAPE_API bool CheckLightmapAt(const FString& MeshPackagePath, int32 Channel, FString& OutError);
}
