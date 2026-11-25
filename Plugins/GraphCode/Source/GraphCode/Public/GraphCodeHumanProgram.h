// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"
#include "UObject/StrongObjectPtr.h"

class UChaosClothAsset;
class UDynamicMesh;
class USkeletalMesh;
class USkeletalMeshLODSettings;
class UMaterialInterface;

// What Core knows about a `.gchuman` (`Docs/GC-HUMAN-GRAMMAR.md`): the header word and one seam, as
// `GraphCodeShapeProgram.h` does for Shape and `GraphCodeMoveProgram.h` for Move. Nothing here knows
// what a face is; it knows that a product may claim the word and that an install without that
// product must say so rather than fail obscurely.
//
// **THE PRODUCT IS CORE EXTENSION** -- `NEXT.md`'s Advanced section, *the fifth product, on optional
// engine plugins*. The two seams this one copies belong to Shape and Move because those products own
// a whole authored FORMAT each; this one is claimed by the product that owns the KIND the format
// builds, which is the arrangement `GC-HUMAN-GRAMMAR.md` §4 states.

/**
 * WHAT A CHARACTER A `.gchuman` BUILT REMEMBERS ABOUT THE PROGRAM THAT MADE IT —
 * `GC-HUMAN-GRAMMAR.md` §2.5, and `FGcMoveAssetProvenance` with the nouns swapped. The keys and the
 * reader are Core's for Shape's reason: the stamp is a fact about the asset, and reflecting facts
 * about an asset is what a `.gc` is for; only the stamping is the product's.
 *
 * **TWO FIELDS, NOT THREE.** A clip carries `Motion` as well, a digest of the bone tracks as built,
 * so an edit since the build is answerable without re-running the program. A character has no such
 * third thing: what a build produces is the face STATE, which lives in an `FEditorBulkData` blob
 * this document declares on `omits: identity` and never reads. A field that could only ever be
 * empty is not a field ([[an-omission-token-may-be-undone-not-blocked]] — say what is missing, do
 * not model what is absent).
 */
struct GRAPHCODE_API FGcHumanAssetProvenance
{
	/** The `.gchuman` this character was built from, as the caller addressed it. */
	FString Source;

	/** A digest of the program text that built it. */
	FString Program;

	/** A stamp names both a source and a program; either alone is not one. */
	bool IsSet() const { return !Source.IsEmpty() && !Program.IsEmpty(); }

	/** The two asset-metadata keys, in one place so a writer and a reader cannot disagree. */
	static const TCHAR* SourceKey();
	static const TCHAR* ProgramKey();

	/** Read the stamp off an asset. False, and `Out` untouched, where there is no stamp. */
	static bool Read(const UObject* Asset, FGcHumanAssetProvenance& Out);

	/** Write the stamp. It does not save: the build's own save carries it. */
	static void Stamp(UObject* Asset, const FGcHumanAssetProvenance& In);
};

/** Optional identity-stage selection and temporary, renderable evaluation. */
struct GRAPHCODE_API FGcHumanBuildOptions
{
	FString Target;
	FString Through;
	FString Preview;
	FString PreviewId;
};

/** What a human build did, or would do. Filled by GraphCode Core Extension; Core only carries it. */
struct GRAPHCODE_API FGcHumanBuildResult
{
	bool bRan = false;

	/** Every asset a `build { }` line put on disk, by package path, in declaration order. */
	TArray<FString> BuiltAssets;

	/** One line per create and per rebuild. */
	TArray<FString> Changes;

	TArray<FString> Notes;

	FString Message;
	FString PreviewId;
	FString PreviewAsset;
};

using FGcHumanProgramBuilder = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	FGcHumanBuildResult& /*OutResult*/,
	FString& /*OutError*/,
	const FGcHumanBuildOptions& /*Options*/)>;

/**
 * Does this program still hold the recipe for one character it built? TEXT ONLY, nothing evaluated —
 * `FGcMoveRecipeResolver` with the nouns swapped, asked by the corpus sweep once per `human:` line.
 * True means a `build { }` line produces `PackagePath` AND names an entry the file defines.
 */
using FGcHumanRecipeResolver = TDelegate<bool(
	const FString& /*ProgramText*/,
	const FString& /*PackagePath*/,
	FString& /*OutReason*/)>;

class GRAPHCODE_API FGraphCodeHumanProgram
{
public:
	/** `gchuman 1` on the first line that is not a comment. */
	static bool LooksLikeHumanProgram(const FString& Text);

	/** The whole seam or none of it, as Shape's and Move's: a half-bound one answers `IsAvailable()` and then refuses. */
	static bool Register(EGcProduct Product, FGcHumanProgramBuilder Builder,
		FGcHumanRecipeResolver Resolver, FString& OutError);

	static void Unregister(EGcProduct Product);

	/** Installed AND bound. */
	static bool IsAvailable();

	/** Build the program, or refuse naming GraphCode Core Extension as the fix. */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcHumanBuildResult& OutResult, FString& OutError,
		const FGcHumanBuildOptions& Options = FGcHumanBuildOptions());

	/**
	 * The sweep's question: is the recipe for this character still in the program that built it?
	 * False with `OutReason` is *the recipe is gone*; false with `OutError` is *no product here to
	 * ask*, which a Core-only install must not report as every built character being broken.
	 */
	static bool HoldsRecipeFor(const FString& ProgramText, const FString& PackagePath,
		FString& OutReason, FString& OutError);
};

/**
 * One of a body's detail levels: the share of its first level's triangles it keeps, the bones it poses there, and the
 * screen size it is drawn from — what a garment's own levels copy, so it drops what the body drops when the body does
 * (`GC-GARMENT-METAHUMAN.md` §7, garment levels).
 */
struct FGcHumanBodyLevel
{
	double TriangleShare = 1.0;
	TArray<FName> Bones;
	float ScreenSize = 0.f;
};

/**
 * A character's head and body as one skinned mesh at rest, on the body's skeleton — the mesh the engine
 * makes to refit an outfit (`GC-GARMENT-METAHUMAN.md` §3). A garment built `[as=skeletal]` on a character
 * copies its weights from it, and its levels from the body's. Null with the reason.
 */
using FGcHumanSkinnedBodyReader = TDelegate<USkeletalMesh*(UObject* /*Character*/, TArray<FGcHumanBodyLevel>& /*OutLevels*/,
	FString& /*OutReason*/)>;

/** The seam Core Extension fills, so a product that dresses a character never links MetaHuman. */
class GRAPHCODE_API FGcHumanSkinnedBody
{
public:
	static bool Register(EGcProduct Product, FGcHumanSkinnedBodyReader Reader, FString& OutError);
	static void Unregister(EGcProduct Product);

	/** The merged mesh, transient and held for the caller; or null with a refusal naming the fix. */
	static TStrongObjectPtr<USkeletalMesh> Read(UObject* Character, FString& OutError);

	/** The same, with the body's detail levels, first to last. */
	static TStrongObjectPtr<USkeletalMesh> Read(UObject* Character, TArray<FGcHumanBodyLevel>& OutLevels, FString& OutError);

	/**
	 * A skeletal mesh's own detail levels, as it is drawn, less the bones each is set to lose by `Settings` — the mesh's
	 * own LOD settings when null. Empty where it is not built for drawing yet.
	 */
	static TArray<FGcHumanBodyLevel> LevelsOf(const USkeletalMesh& Mesh, const USkeletalMeshLODSettings* Settings = nullptr);
};

/**
 * How much of a character's skin a garment leaves showing at a point on it at rest: 1 bare, 0 under the
 * cloth, and between at an opening's edge, where the skin is kept but drawn in under the cloth — the more
 * the nearer 0 (`GC-GARMENT-METAHUMAN.md` §4 step 3).
 */
using FGcHumanSkinShown = TFunction<float(const FVector& /*Point*/)>;

/** Draws a garment into a Chaos cloth asset on a body's bones and builds it: a line about it, or why not. */
using FGcHumanClothWriter = TFunction<bool(const USkeletalMesh& /*Body*/, UChaosClothAsset& /*Cloth*/,
	TConstArrayView<UMaterialInterface*> /*Kept*/, TArray<FString>& /*OutNotes*/, FString& /*OutError*/)>;

/** A garment made something a character wears: the wardrobe item it selects, and the skin it hides. */
struct FGcHumanWardrobeItem
{
	/** The character the garment was built on; its own head and body are what the hidden skin is read on. */
	UObject* Character = nullptr;

	/** Skeletal Clothing: the garment as built, a skeletal mesh on the body's skeleton. */
	USkeletalMesh* Garment = nullptr;

	/**
	 * Outfit Clothing instead, when set: the garment's own writer of its Chaos cloth asset (GC-GARMENT-CLOTH.md §B). The
	 * wardrobe writer saves the outfit's one size, the character's merged head and body, at `BodyPath`, calls this with it
	 * and the cloth asset at `ClothPath` — and with the slots of the outfit already at `OutfitPath`, whose looks an author
	 * assigned are kept — then saves the cloth and makes the outfit the item selects (`GC-GARMENT-METAHUMAN.md` §4 step 4).
	 */
	FGcHumanClothWriter WriteCloth;

	/**
	 * Set for an outfit that simulates (`[as=cloth]` on a character, GC-GARMENT-CLOTH.md): the writer makes the source body's
	 * Physics Asset here from the character's own body, as MetaHuman makes one, and gives it to that body before `WriteCloth`.
	 */
	FString PhysicsPath;
	FString OutfitPath;
	FString ClothPath;
	FString BodyPath;

	FString ItemPath;
	FString BodyMaskPath;

	/** Written only where the garment covers some of the head's skin. */
	FString HeadMaskPath;

	FGcHumanSkinShown Shown;

	/** How far skin is drawn in under the cloth at most, where it is kept nearest to being hidden, in cm. */
	float ShrinkCm = 0.f;
};

/** Writes and saves the item and its hidden face maps: what it wrote, a path each, and a line each about them. */
using FGcHumanWardrobeWriter = TDelegate<bool(const FGcHumanWardrobeItem& /*Item*/, TArray<FString>& /*OutWritten*/,
	TArray<FString>& /*OutNotes*/, FString& /*OutError*/)>;

/** The second seam Core Extension fills: a garment into a MetaHuman's wardrobe, without linking MetaHuman. */
class GRAPHCODE_API FGcHumanWardrobe
{
public:
	static bool Register(EGcProduct Product, FGcHumanWardrobeWriter Writer, FString& OutError);
	static void Unregister(EGcProduct Product);

	static bool Write(const FGcHumanWardrobeItem& Item, TArray<FString>& OutWritten, TArray<FString>& OutNotes,
		FString& OutError);
};
