// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UStaticMesh;

/**
 * **ONE MATERIAL FROM MANY — MB51, `GC-MATERIAL-BAKE-DESIGN.md`.** Core's half of `bake()`:
 * evaluate a set of assigned materials into pixels, pack them into one atlas, mint the textures
 * and the single generated material, and collapse the meshes onto it.
 *
 * **IT IS CORE'S AND NOT SHAPE'S BECAUSE IT HAS NO `.gcshape` KNOWLEDGE** (§4). Shape hands over
 * finished meshes and their material assignments; Cloth and any later mesh producer call the same
 * service. Conversely Shape must never grow a second material evaluator.
 *
 * **THE TARGET IS SHARED, WHICH IS THE WHOLE SHAPE OF THIS API** (§0). Several meshes bake into
 * ONE atlas and ONE material. This reduces material sections; separate mesh components do not
 * automatically become one draw call. That is why the
 * entry point takes an array of meshes rather than one, and why a single-mesh bake is simply the
 * degenerate case rather than a different path.
 */
namespace GraphCodeMaterialBake
{
	/**
	 * **MB52 — one mark laid over a baked surface.** `GC-SHAPE-DECAL-DESIGN.md`.
	 *
	 * **IT CARRIES A FIELD AND NOT A RULE.** Shape knows the geometry, so Shape computes the
	 * curvature and hands over a value per vertex; Core knows pixels, so Core rasterises and
	 * blends. Passing `mask=edges` down instead would put a geometry question in the module that
	 * has no geometry.
	 */
	struct GRAPHCODE_API FDecal
	{
		/** The material to lay over — a normal and a roughness, not just a colour. */
		UMaterialInterface* Source = nullptr;

		/** 0..1 per vertex of the member's mesh, already read for this decal's mask. */
		TArray<double> PerVertex;
		/** Optional per-texel coverage in published mesh space; multiplies a placed mark's facing mask. */
		TFunction<double(const FVector3d&, const FVector3d&, const FVector2d&)> Sample;

		/** `amount=`, how strongly it applies. */
		double Amount = 1.0;

		/**
		 * A PLACED mark's own coordinates per vertex, empty for a derived mask.
		 *
		 * **THE IMAGE HAS TO SAMPLE IN ITS OWN SPACE.** Baked through the surface's unwrap a crest
		 * would be smeared across whatever islands that unwrap happens to have; through these it
		 * lands square inside its footprint. The bake renders INTO the atlas and samples FROM
		 * these, which is the one place the two coordinate systems have to be told apart.
		 */
		TArray<UE::Math::TVector2<double>> SourceUV;

		/** A placed mark is shaped by its own alpha; a derived mask is shaped by the curvature. */
		bool bUseAlpha() const { return !SourceUV.IsEmpty(); }
	};

	/** One mesh contributing to a target, with the materials it wears. */
	struct GRAPHCODE_API FMember
	{
		UStaticMesh* Mesh = nullptr;

		/**
		 * Slot index -> the material to evaluate, `nullptr` for an empty slot.
		 *
		 * **THESE ARE THE RESOLVED MATERIALS AND NOT THE FILE'S DEFAULTS.** On a rebuild a slot
		 * wears whatever a person set in the editor (`GC-PRODUCT-SPLIT.md` §5.2), and that is what
		 * has to be baked — baking the document's first-build default would quietly revert a
		 * material somebody chose.
		 */
		TArray<UMaterialInterface*> Materials;

		/** Slot-indexed texels/metre overrides; zero inherits the target. */
		TArray<double> TexelDensities;

		/** Slot indices that keep their own slot instead of being baked — `except=` (§3a). */
		TSet<int32> Excluded;

		/**
		 * **MB52 — the marks laid over this mesh once its materials are baked.**
		 *
		 * The order is the order written in the file, which is paint order (user-ruled): a later
		 * decal covers an earlier one, and a reader can see why without leaving the file.
		 */
		TArray<FDecal> Decals;
	};

	struct GRAPHCODE_API FRequest
	{
		/** The target's name, for reports and refusals only. */
		FString Name;

		/**
		 * **THE FULL PACKAGE PATH OF THE MATERIAL TO MINT, AUTHORED RATHER THAN DERIVED** —
		 * `/Game/Props/M_Chest`, from the left-hand side of `M_Chest = bake(Chest, …)`.
		 *
		 * **MEASURED 2026-09-22, and it is why this is a path and not a folder.**
		 * `CreateProxyMaterialAndTextures` calls `CreatePackage` on what it is given and names the
		 * object inside it, so a folder made a package OF the folder. Deriving the name instead put
		 * the material somewhere no caller could asserted against. The author states it.
		 */
		FString MaterialPath;

		/** Square, in pixels. Zero selects from density, or 1024 without authored density. */
		int32 Resolution = 1024;

		/** Target default, texels/metre; zero uses the highest slot override if one exists. */
		double TexelDensity = 0.0;

		TArray<FMember> Members;
	};

	struct GRAPHCODE_API FResult
	{
		UMaterialInterface* Material = nullptr;

		/** Every asset minted, so a caller can save them and record provenance over them. */
		TArray<UObject*> Created;

		/** What a dry run or a report says, one line each. */
		TArray<FString> Notes;
	};

	/**
	 * Bake one target. Returns false with `OutError` naming the source slots and the incompatible
	 * property when the inputs cannot be one material (§3) — **atomically**, having written
	 * nothing and having left every source mesh and material untouched.
	 *
	 * **NOTHING IS PARTLY COLLAPSED.** A bake that half succeeded would leave a mesh wearing a
	 * generated material for some triangles and an original for others, which is a state no
	 * rebuild could reason about.
	 */
	GRAPHCODE_API bool Run(const FRequest& Request, FResult& OutResult, FString& OutError);

	/**
	 * Whether this process can bake at all, with `OutWhy` naming what is missing when it cannot.
	 *
	 * **A BAKE RENDERS, so a run with no RHI cannot do it** — and MEASURED 2026-09-22, the engine's
	 * baker CRASHES there rather than failing, which is why this is asked before the work and not
	 * discovered during it. Callers that are specs use it to skip and say so; a spec that quietly
	 * passed in the fast `-nullrhi` gate would be testing nothing.
	 */
	GRAPHCODE_API bool CanRun(FString& OutWhy);

	/**
	 * One material evaluated flat over a square, `Size` pixels a side, through the same baker `Run` uses: its base colour
	 * (sRGB) and its roughness (linear, in red). What a stitch reads its thread's look from (GC-GARMENT-STITCH.md §5) —
	 * Core's, so no product grows a second evaluator. A bake renders, so this needs a GPU (`CanRun`).
	 */
	GRAPHCODE_API bool Flatten(UMaterialInterface* Material, int32 Size, TArray<FColor>& OutBaseColour, TArray<FColor>& OutRoughness,
		FString& OutError);
}
