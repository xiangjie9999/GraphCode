// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeShapeProgram.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **DOES THE PROGRAM STILL DESCRIBE THE ASSET?** — `GC-SHAPE-AUTHORING-DESIGN.md` §11, row M8.
 *
 * Somebody opens a built mesh in Modeling Mode, drags a vertex and saves. The program now describes
 * the old shape, and the next build overwrites the edit **silently, reporting success**. That is not
 * a new problem: it is verbatim `GraphCodeVolumeShape.h`, written for brushes at row G7 — *"
 * `FGeomObject::SendToSource` writes dragged vertices straight into `Polys->Element` and never
 * touches `BrushBuilder`"* — and §11's table transfers the shipped answer with the nouns swapped.
 *
 * **THE STATES ARE G7's FOUR AND THEY MEAN THE SAME THINGS**, including the one that exists only so
 * that a caller cannot confuse two claims: *we could not tell* is not *it differs*.
 *
 * **THE CHECK IS ON DEMAND, NEVER ON SAVE** (§11). It is spent when a build is asked to write over
 * an asset that is already there; a per-save rebuild of every generated mesh would make the editor
 * unusable.
 *
 * **THE FAST ANSWER IS A DIGEST AND THE GATE IS STILL `IsSameAs`.** The recorded `Shape` digest is
 * taken from the asset AFTER the asset door had it, so comparing it against a later read of the same
 * asset compares like with like and *nobody has touched this* is exact, cheap and needs no rebuild
 * at all. Only when that says the mesh HAS moved is the tolerant comparison spent — and only there
 * can it be spent, because it needs the mesh the recorded program produces, which is in hand exactly
 * when the program being applied is the one that built the asset.
 */
namespace GraphCodeShapeProvenance
{
	/** Which of §11's four states this asset is in. */
	enum class EState : uint8
	{
		/** No stamp: nothing claims this asset. An imported or hand-made mesh — **never ours to rebuild**. */
		NoProgram,

		/** The program rebuilds to the same mesh. Safe: a rebuild takes nothing away. */
		ProgramDescribes,

		/** A stamp is present and the asset has MOVED AWAY from it — the hand-edited case. **Refuse.** */
		ProgramDiverges,

		/**
		 * The question has no answer.
		 *
		 * Deliberately NOT folded into `ProgramDiverges`, which is G7's ruling and its reason: a
		 * caller that silently rebuilt on a failed check would turn *we could not tell* into a
		 * permanent overwrite ([[refusal-reason-is-a-claim]]). It is reached when the asset's mesh
		 * cannot be read at all, and when the mesh has moved while the program in hand is not the
		 * one that built it — there is no old program left to ask.
		 */
		Indeterminate,
	};

	/** What the comparison found, so a caller reports a number rather than an impression. */
	struct GRAPHCODESHAPE_API FComparison
	{
		EState State = EState::NoProgram;

		/** The stamp read off the asset, empty where there is none. */
		FGcShapeAssetProvenance Recorded;

		/** True where the program being applied is the one that built the asset. */
		bool bSameProgram = false;

		/** Triangles in the asset as it stands, and in the mesh the program produced. */
		int32 AssetTriangles = 0;
		int32 ProgramTriangles = 0;

		/** What differed, or why the question had no answer. */
		FString Detail;

		bool DescribesItsAsset() const { return State == EState::ProgramDescribes; }
	};

	/** The digest that goes on the asset for the program text that built it. */
	GRAPHCODESHAPE_API FString ProgramDigest(const FString& ProgramText);

	/**
	 * Ask the asset at `PackagePath` whether the program still describes it.
	 *
	 * `Evaluated` is the mesh THIS build produced and `IncomingDigest` is the digest of the program
	 * that produced it; together they answer the divergence question exactly when the incoming
	 * program is the recorded one. Nothing here loads the recorded `.gcshape` from disk: by the time
	 * anybody rebuilds, the file on disk is the NEW program, so reading it would answer a question
	 * about the wrong text ([[a-digest-is-not-a-document]]).
	 */
	GRAPHCODESHAPE_API FComparison Compare(const FString& PackagePath,
		const UE::Geometry::FDynamicMesh3& Evaluated, const FString& IncomingDigest);

	/** The sentence a refusal relays, naming the state and what to do about it. */
	GRAPHCODESHAPE_API FString Refusal(const FComparison& Comparison, const FString& PackagePath);

	/** What `KeepProgramOnDisk` did, so the caller says the true one of four sentences. */
	enum class EKept : uint8
	{
		/** The file already held this exact text. Nothing was written and nothing needed to be. */
		AlreadyThere,

		/** The file did not exist and now does. */
		Written,

		/** A file was there holding OTHER text, and this program replaced it. */
		Replaced,

		/** Nothing could be written, and `OutDetail` says why. */
		Failed,
	};

	/**
	 * **PUT THE PROGRAM WHERE THE ASSET SAYS IT IS.** — the other half of H28.
	 *
	 * Every built mesh is stamped `Source = DocumentPath`, and `GcCheckShapeRecipe` in the corpus
	 * sweep goes to `<DocumentPath>.gcshape` to ask whether that program still builds it. For a
	 * program pasted INLINE there was never a file there at all, so the stamp named a document that
	 * did not exist and the recipe was gone the moment the conversation ended. That is not a rare
	 * shape of the failure, it is the commonest one: `SM_Trough` shipped in the corpus with its
	 * `.gc` naming a program by digest and the program nowhere, and it had to be re-authored by
	 * measuring the mesh.
	 *
	 * **SO A BUILD LEAVES ITS RECIPE, ALWAYS, AND THAT IS CHEAPER THAN ANY CHECK FOR IT.** The
	 * sweep's fifth counter stays — it still catches a file somebody deleted or edited past its own
	 * build lines — but the state it was written for cannot be reached by building any more.
	 *
	 * **THE ADDRESS IS THE PACKAGE PATH AND NOT THE SIDECAR SETTING.** A `.gcshape` is an AUTHORED
	 * file, not a projection this code may place where it likes, and both readers that look for one
	 * — `GcCheckShapeRecipe` and the tool's `GcFindAuthoredFileBeside` — resolve it exactly this way
	 * ([[excluded-folder-is-readable]]). Three readers, one rule, or the file lands where nobody
	 * looks.
	 *
	 * **A DIFFERENT TEXT AT THAT ADDRESS IS REPLACED AND SAID OUT LOUD.** The assets on disk were
	 * just built from the text in hand; leaving the old file would leave the one document that
	 * claims to build them describing something else, which is the H28 state arrived at from the
	 * other side. The caller reports `Replaced` rather than performing it silently
	 * ([[a-write-finishes-itself]]).
	 *
	 * **NEVER FROM A DRY RUN** — the caller holds that gate, because a dry run writes nothing at
	 * all ([[dry-run-must-not-write-anything]]).
	 */
	GRAPHCODESHAPE_API EKept KeepProgramOnDisk(const FString& DocumentPath,
		const FString& ProgramText, FString& OutFile, FString& OutDetail);

	/**
	 * **THE RECIPE'S HOME AS A PACKAGE PATH, HOWEVER `asset_path` SPELLED IT** — G2, LaFerrari field
	 * report 2026-11-13. Every reader of the address (the recipe left on disk, a `reference()` picture
	 * written relative to the recipe, an import's own path) asks `FPackageName` for a LONG PACKAGE NAME,
	 * and an inline apply addressed as `/Game/A/Recipe.Recipe`, `/Game/A/Recipe.gcshape` or the file
	 * `…/Content/A/Recipe.gcshape` is not one: the picture `Refs/side.png` was read against the process
	 * working directory and refused as *could not be read as a picture*. Those spellings come back as
	 * `/Game/A/Recipe`; a package path, an empty path and anything that names no mounted package come
	 * back unchanged, so every caller that already passed a package path sees nothing new.
	 */
	GRAPHCODESHAPE_API FString DocumentPackage(const FString& DocumentPath);
}
