// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeGarmentProgram.h"
#include "UObject/StrongObjectPtr.h"

class UDynamicMesh;
class UMaterialInstanceConstant;
class UMaterialInterface;
class USkeletalMesh;
class UTexture2D;
struct FGcHumanBodyLevel;
struct FGcGarmentBuiltRing;
struct FGcGarmentChaosFabric;
struct FGcGarmentClothSolverWords;

/**
 * The vertices of one part placed on a garment, and how they move with the body: at `Stiffness` 1 every
 * one takes the weights where the part sits (`Seat`), so it moves as one piece; at 0 each keeps the
 * weights of the skin under it; between, a blend. `Holds` are the cloth's vertices it holds and how much of
 * each one's motion it takes (GC-GARMENT-SHAPE-PARTS.md §8): that share of its weights, the rest their own.
 */
struct FGcGarmentStiffPart
{
	TArray<int32> Vertices;
	FVector Seat = FVector::ZeroVector;
	double Stiffness = 1.0;
	TArray<TPair<int32, float>> Holds;
};

/**
 * What a `fabric()` line says of its look (13 Nov), set on the material instance a slot wears: `M_GcFabric`'s `FabricColour`, and its
 * `WeaveNormal` drawn at the fabric's gauge with one tile `WeaveCm` across. Unset parts leave the parent's default.
 */
struct FGcGarmentFabricLook
{
	TOptional<FLinearColor> Colour;
	UTexture2D* WeaveNormal = nullptr;
	/** The cloth a tile of `WeaveNormal` covers, in cm (UV0 is the cloth laid flat, 100 cm a tile). 0 with no `WeaveNormal`. */
	double WeaveCm = 0.0;
	/** A woven's weft colour (16 Nov, `WeftColour`): the second of `colour=(#warp, #weft)`, or the one colour again. */
	TOptional<FLinearColor> WeftColour;
	/** A woven's mask (`WeaveMask`, same tile as `WeaveNormal`): R the warp's share of a texel, G one less the slope variance its mip lost. */
	UTexture2D* WeaveMask = nullptr;
};

/** How a build's mesh becomes an asset: one writer for the garments and the ring overlay. */
class GRAPHCODEGARMENT_API FGcGarmentAsset
{
public:
	/**
	 * Write the mesh as an ordinary static mesh, rebuilding one already there rather than beside it
	 * (Shape's rule), stamped with its provenance, and saved. `Material` goes into the slot only where the slot is empty
	 * — a new asset, or one nobody dressed — so a rebuild keeps whatever somebody chose (Shape's rule); null keeps the
	 * engine's default. Every writer here takes `bSave`: false leaves what it wrote dirty, `gc_apply save:false`.
	 */
	static bool Write(UDynamicMesh* Mesh, const FString& PackagePath, const FGcGarmentAssetProvenance& Stamp,
		UMaterialInterface* Material, FString& OutError, bool bSave = true, UMaterialInterface* InsideMaterial = nullptr,
		const TArray<UMaterialInterface*>& Palette = {});

	/**
	 * `[as=skeletal]`: the mesh skinned to `Body` — each vertex weighted as the nearest skin is, on the
	 * body's own skeleton — and written as a skeletal mesh with a detail level for each of `BodyLevels`
	 * (`Levels`), each drawn from the body's own screen size, rebuilt in place like `Write`, and dressed as `Write` dresses.
	 */
	static bool WriteSkinned(UDynamicMesh* Mesh, USkeletalMesh* Body, const TArray<FGcHumanBodyLevel>& BodyLevels,
		const FString& PackagePath, const FGcGarmentAssetProvenance& Stamp, FString& OutError,
		const TArray<FGcGarmentStiffPart>& Parts = {}, UMaterialInterface* Material = nullptr, bool bSave = true, UMaterialInterface* InsideMaterial = nullptr,
		const TArray<UMaterialInterface*>& Palette = {});

	/**
	 * A weighted garment's detail levels, one for each of the body's (`GC-GARMENT-METAHUMAN.md` §7, garment levels):
	 * the first `Mesh` itself, each after it simplified to the body's own share of triangles there; and at every level
	 * a bone the body does not pose there hands its weight to its nearest parent that it does, as the engine folds a
	 * removed bone into its parent. No levels: `Mesh` alone.
	 */
	static TArray<TStrongObjectPtr<UDynamicMesh>> Levels(UDynamicMesh* Mesh, const TArray<FGcHumanBodyLevel>& BodyLevels);

	/**
	 * The mesh weighted as the skin it lies on is, with the body's bones — what `WriteSkinned` writes, and what
	 * an `[as=outfit]` line hands the wardrobe writer. `PackagePath` names the asset in a refusal.
	 */
	static bool Weight(UDynamicMesh* Mesh, USkeletalMesh* Body, const FString& PackagePath, FString& OutError,
		const TArray<FGcGarmentStiffPart>& Parts = {});

	/**
	 * What the weights came to, read at the garment's rings, for the build's notes (field report GC-02, 2026-10-23: the author
	 * measured a jacket's weights with per-vertex editor Python, which crashed the editor three times). For each ring: the bones
	 * that carry `Mesh` within 2 cm of it and their shares, and how its points split by lead bone. For each ring in `Seams` (one
	 * another garment of the document also meets, GC-04): the bone of `Body`'s chain the ring sits on (the upper arm, for an
	 * armhole; twist and corrective bones are not the chain), and the ring's distance from the nearer of that bone's joint and the
	 * next one down; 3 cm or more is said, because each panel then bends inside itself. `Line` prefixes each note. Reads only.
	 */
	static void WeightNotes(const UDynamicMesh* Mesh, USkeletalMesh* Body, const TArray<const FGcGarmentBuiltRing*>& Rings,
		const TSet<FString>& Seams, int32 Line, TArray<FString>& OutNotes);

	/**
	 * `[as=cloth]` (GC-GARMENT-CLOTH.md §B): a Chaos Cloth Asset on `Body`'s skeleton that simulates in the game — `Mesh` drawn at
	 * each of `BodyLevels` as `[as=outfit]`'s cloth is, every level over `Middle`, the cloth's middle, both weighted from the skin;
	 * each point of `Middle` held to the body or free by `MaxDistanceCm` (0 held), colliding with `Body`'s Physics Asset.
	 * `DensityKgPerM2` 0 and `Friction` below 0 keep the engine's. Rebuilt in place, stamped and saved as `Write` does.
	 */
	/**
	 * `[as=cloth]`'s middle weighted from the skin as `Weight` weights, and held: its `MaxDistance` weight layer, each point's
	 * `MaxDistanceCm` (0 held). What `WriteCloth` simulates, and what an outfit built `[as=cloth]` hands its cloth writer.
	 */
	static bool HoldCloth(UDynamicMesh* Middle, const TArray<float>& MaxDistanceCm, USkeletalMesh* Body, const FString& PackagePath,
		FString& OutError);

	/** `Fabric`, `StretchStiffness` (17 Nov, G1): the fabric's bend and stretch as the outfit route hands them (`FGcGarmentClothSimulation`);
	 *  read only at `gc.Garment.ClothAsLaid` 1 - at 0, and when null, the asset keeps the engine node's, as before.
	 *  `Solver` (18 Nov, G1 slice 2): `hang()`'s runtime words; null or empty, the asset is written as before. */
	static bool WriteCloth(UDynamicMesh* Mesh, UDynamicMesh* Middle, const TArray<float>& MaxDistanceCm, USkeletalMesh* Body,
		const TArray<FGcHumanBodyLevel>& BodyLevels, const FString& PackagePath, const FGcGarmentAssetProvenance& Stamp,
		float DensityKgPerM2, float Friction, float ContactCm, float BodyContactCm, const TArray<UMaterialInterface*>& Palette, TArray<FString>& OutNotes,
		FString& OutError, bool bSave = true, const TArray<FGcGarmentStiffPart>& Parts = {},
		const FGcGarmentChaosFabric* Fabric = nullptr, const float* StretchStiffness = nullptr, const FGcGarmentClothSolverWords* Solver = nullptr);

	/** What a texture of the stitch sheet holds, which decides how the engine compresses and reads it. */
	enum class ETexture : uint8
	{
		Colour,
		Normal,
		Mask,
	};

	/**
	 * One texture of the stitch sheet (GC-GARMENT-STITCH.md §5 item 6), `Size` pixels a side, written at `PackagePath` — rebuilt in
	 * place where one is there already, so what reads it keeps reading it — and saved.
	 */
	static UTexture2D* WriteTexture(const FString& PackagePath, int32 Size, const TArray<FColor>& Pixels, ETexture Kind, FString& OutError,
		bool bSave = true);

	/**
	 * `<asset>_Material` (§5 item 7): an instance of `Parent` with the sheet's three textures in its `StitchColour`, `StitchNormal` and
	 * `StitchMask`, rebuilt in place, and saved.
	 */
	static UMaterialInterface* WriteStitched(const FString& PackagePath, UMaterialInterface* Parent, UTexture2D* Colour, UTexture2D* Normal,
		UTexture2D* Mask, FString& OutError, bool bSave = true, const FGcGarmentFabricLook* Look = nullptr);

	/**
	 * `<asset>_Look_<Fabric>` (13 Nov): an unstitched slot's instance of `Parent` carrying only the fabric's look, rebuilt in place, and saved.
	 * The stitched slots carry the same look on their `<asset>_Material` (`WriteStitched`'s `Look`).
	 */
	static UMaterialInterface* WriteFabricLook(const FString& PackagePath, UMaterialInterface* Parent, const FGcGarmentFabricLook& Look,
		FString& OutError, bool bSave = true);

	/**
	 * The fabric-look parameters of an instance (`FabricColour`, `WeaveNormal`, `WeaveCm`) set from `Look`, or - `Look` null - removed, so an
	 * instance rebuilt from a line that no longer says them is what that line alone makes. Public for the look's spec.
	 */
	static void SetFabricLook(UMaterialInstanceConstant& Instance, const FGcGarmentFabricLook* Look);

	/** A slot the builder may re-dress on a rebuild: its plain fabric, or a `_Look_` instance it wrote for this asset (`PackagePath`). */
	static bool BuilderDressed(const UMaterialInterface* Material, const FString& PackagePath);
};
