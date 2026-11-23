// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"

/**
 * WHAT CORE KNOWS ABOUT A `.gcshape` WITHOUT HOLDING ANY OF THE CODE THAT BUILDS ONE.
 * `GC-SHAPE-AUTHORING-DESIGN.md` §15 (M0); `GC-SHAPE-GRAMMAR.md` §1 is the header this recognises.
 *
 * The same ruling `GraphCodeProducts.h` opens with, applied to a whole DOCUMENT rather than to a
 * kind: **the vocabulary is Core's and the codecs are the products'.** A build with no GraphCode
 * Shape still recognises `gcshape 1` on line one, because the alternative is measured and bad — a
 * payload that falls through to the block machinery is applied as a write to an asset with the whole
 * program silently dropped, and a payload the router does not know at all answers *"unknown file"*,
 * which sends a person to check their spelling rather than their installation.
 *
 * **THIS IS THE WHOLE OF CORE'S SHAPE SURFACE AND IT IS MEANT TO STAY THAT WAY.** Everything the
 * grammar describes — the parser, the DAG, the evaluator, the asset door — is the product's and
 * lives in `Plugins/GraphCodeShape`. What is here is a header word, a refusal, and one seam.
 */

/**
 * **WHAT A BUILT ASSET REMEMBERS ABOUT THE FILE THAT MADE IT** — `GC-SHAPE-AUTHORING-DESIGN.md` §11,
 * row M8. It is the ONE thing that reads across the product boundary, and it reads one way: a
 * `.gcshape` never reads a `.gc`, and the asset records where it came from so a `.gc` can reflect
 * it, because reflecting facts about an asset is exactly what a `.gc` is for.
 *
 * **THE VOCABULARY IS CORE'S FOR THE REASON THE HEADER WORD IS.** `kind:static_mesh` must project
 * this line on an install with no GraphCode Shape at all — the asset is still on disk and still says
 * where it came from — so the key names and the reader live here, and only the STAMPING and the
 * divergence check live in the product.
 *
 * **THREE VALUES, AND §11 NAMED TWO.** The path and the program digest are §11's; `Shape` is the
 * third and it is not a decoration. Without it the only way to ask *has anybody edited this asset*
 * is to re-run the program that built it, and by the time anybody asks, the program on disk is the
 * NEW one — the author just edited it, which is why they are rebuilding. So the old program is gone
 * and the question would be unanswerable in exactly the case it is asked
 * ([[a-query-that-answers-as-close-as-it-got]]). `Shape` records what came out, so *nobody touched
 * it* is answerable without the program at all, and `IsSameAs` is spent only where the fast answer
 * says the mesh has moved.
 */
struct GRAPHCODE_API FGcShapeAssetProvenance
{
	/** The `.gcshape` this asset was built from, as the caller addressed it. */
	FString Source;

	/** A digest of the program text that built it. */
	FString Program;

	/**
	 * A digest of the mesh as it was built, taken from the asset AFTER the asset door had it, so
	 * comparing it against a later read of the same asset compares like with like.
	 */
	FString Shape;

	/** A stamp is present when it names both a source and a program; either alone is not one. */
	bool IsSet() const { return !Source.IsEmpty() && !Program.IsEmpty(); }

	/** The three asset-metadata keys, in one place so a writer and a reader cannot disagree. */
	static const TCHAR* SourceKey();
	static const TCHAR* ProgramKey();
	static const TCHAR* ShapeKey();

	/** Read the stamp off an asset. False, and `Out` untouched, where there is no stamp. */
	static bool Read(const UObject* Asset, FGcShapeAssetProvenance& Out);

	/**
	 * Write the stamp onto an asset. **It does not save** — the caller is mid-build and the package
	 * save that follows carries this with it, and a save here would write the asset twice.
	 */
	static void Stamp(UObject* Asset, const FGcShapeAssetProvenance& In);

	/** Take a stamp off an asset that no longer has one to make. Used where a build hands over. */
	static void Clear(UObject* Asset);
};

/** What building a shape program did, or would do. Filled by the product; Core only carries it. */
struct GRAPHCODE_API FGcShapeBuildResult
{
	bool bRan = false;

	/** Every asset a `build { }` line put on disk, by package path, in file declaration order. */
	TArray<FString> BuiltAssets;

	/** One line per create and per rebuild, in the order they happened. */
	TArray<FString> Changes;

	/** What the evaluator could not be given, said out loud rather than left to a silent default. */
	TArray<FString> Notes;

	/**
	 * **WHAT THE PROGRAM SPENT ON MESH BOOLEANS — HOW MANY, AND OVER HOW MANY TRIANGLES OF INPUT.**
	 * MV10. Zero for a file that ran none, which is most of them.
	 *
	 * **THE SECOND NUMBER IS THE ONE THAT SEPARATES A SLOW FILE FROM A FAST ONE**, and a count of
	 * calls cannot: two measured programs run 600 and 660 booleans and one of them takes the
	 * process, because the quantity is the size of the mesh under each call rather than the number
	 * of calls. Both operands are counted, since `FMeshBoolean` indexes each.
	 *
	 * **IT IS ON THE RESULT RATHER THAN IN A NOTE SO A SPEC CAN ASSERT IT.** A note appears in every
	 * caller's output and is read by the corpus sweep's own assertions, and a number with no ceiling
	 * sited against it yet has nothing to say to an author — but a measurement nothing can read back
	 * is not an instrument ([[an-artifact-needs-a-reader-with-a-measured-need]]).
	 *
	 * **CORE CARRIES THESE AND RULES NOTHING ABOUT THEM**, exactly as it carries the rest of this
	 * struct: the ceiling, when it is sited, is the product's.
	 */
	int32 Booleans = 0;
	int64 BooleanTriangles = 0;

	/** The sentence the caller relays. Empty means the changes speak for themselves. */
	FString Message;
};

/**
 * **ONE ENTRY OF ONE `.gcshape`, AS A LIBRARY INDEX SEES IT** — what `gc_search kind:"preset"`
 * ranks and what `gc_describe preset:` prints. `GC-SHAPE-PRESET-LIBRARY.md` §7.
 *
 * **THE VOCABULARY IS CORE'S FOR THE REASON THE HEADER WORD IS** — the same ruling that put
 * `FGcShapeBuildResult` here. The tools that ask this question live in `GraphCodeMCP`, which reaches
 * Shape only through this seam; a struct declared in the product would make the MCP module depend on
 * a plugin it is deliberately independent of.
 *
 * **EVERY FIELD IS A PROPERTY OF THE TEXT.** Nothing here needs an editor, an asset registry or an
 * evaluation, which is what lets the whole index cost a directory walk plus a parse.
 */
struct GRAPHCODE_API FGcShapeLibraryEntry
{
	/** The file's content path — `/GraphCodeShape/Presets/Surface`. */
	FString File;

	/** The file's last segment, which is how an import names it — `Surface`. */
	FString Family;

	/** The entry's own name — `Erode`. `$Surface.Erode` is `Family` and this. */
	FString Name;

	/** `mesh` · `plane` · `instruction` · `profile` · `material` · `random` · `number` · `module`. */
	FString Kind;

	/**
	 * **THE `#` BLOCK WRITTEN ABOVE THE ENTRY** — user-ruled 2026-09-11, *"the `#` is just equal to
	 * the description"*. Empty where the author wrote none, which is honest rather than invented:
	 * a search that fell back to the entry's own name would rank every undocumented entry as though
	 * it had been described.
	 */
	FString Description;

	/**
	 * Parameters as `name=default`, in order: the declared ones, then those a bare `$Entry` clause takes (K1). A call
	 * fills these by name.
	 */
	TArray<FString> Parameters;

	/**
	 * **K1 — THE BARE `$Entry` CLAUSES OF THE PARAMETER LIST**, as written (`$Dmg.Snap`); empty on nearly every entry.
	 * `Docs/GC-SHAPE-DAMAGE-KIT.md` §6. A file with one is linked to be indexed, since what it takes may sit in
	 * another file.
	 */
	TArray<FString> Inherits;

	/**
	 * K1: one per `Parameters` row, in the same order — empty where this entry declares it, else the clause it was
	 * TAKEN from, whose default it is. A change to that default changes what this entry builds.
	 */
	TArray<FString> ParameterFrom;

	/** K1: `name: $Dmg.Snap = 0` for each declared parameter a clause also offers — the default this list overrides. */
	TArray<FString> Overrides;

	/** True where a `build { }` line in this file names this entry — the family's own examples. */
	bool bIsBuildTarget = false;

	int32 Line = 0;
};

/**
 * Index the `.gcshape` files under some content roots. Roots or files that cannot be read are named
 * in `OutSkipped` rather than failing the call — see `FGraphCodeShapeLibrary::Index`.
 */
using FGcShapeLibraryIndexer = TDelegate<bool(
	const TArray<FString>& /*Roots*/,
	TArray<FGcShapeLibraryEntry>& /*Out*/,
	TArray<FString>& /*OutSkipped*/,
	FString& /*OutError*/)>;

/**
 * GraphCode Shape's implementation of the whole file. Core routes to it and owns nothing behind it.
 *
 * **`DocumentPath` IS THE CALLER'S `asset_path`, PASSED THROUGH UNREAD, AND M0 DELIBERATELY DOES NOT
 * RULE WHAT IT MEANS.** A `.gcw` is addressed by its level because it describes one; a `.gcshape`
 * describes no asset — its `build { }` block names the assets, and `into:` names where they land, so
 * the document's own address is a separate question that M1 answers when it puts the first
 * `UStaticMesh` on disk. Inventing an answer here would be a second carrier of a fact the header
 * already holds ([[two-carriers-of-one-fact-is-a-race]]).
 */
using FGcShapeProgramBuilder = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	FGcShapeBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;

/**
 * A shape build that evaluates off the game thread (GC-APPLY-ASYNC-PLAN.md E): `FGcMoveBuildJob` with the nouns
 * swapped. `IsReady`, `Cancel` and `Progress` from any thread; `Finish` once, on the game thread, after `IsReady`.
 */
class GRAPHCODE_API FGcShapeBuildJob
{
public:
	virtual ~FGcShapeBuildJob() = default;
	virtual bool IsReady() const = 0;
	virtual void Cancel() = 0;
	virtual FString Progress() const = 0;
	virtual bool Finish(FGcShapeBuildResult& OutResult, FString& OutError) = 0;
};
using FGcShapeProgramStarter = TDelegate<bool(
	const FString& /*DocumentPath*/,
	const FString& /*ProgramText*/,
	bool /*bDryRun*/,
	TSharedPtr<FGcShapeBuildJob>& /*OutJob*/,
	FGcShapeBuildResult& /*OutResult*/,
	FString& /*OutError*/)>;
DECLARE_MULTICAST_DELEGATE(FGcShapeJobsReleasing);

/**
 * Does this program still hold the recipe for one asset it is supposed to have built?
 *
 * **TEXT ONLY — NOTHING IS EVALUATED, and that is the whole reason it is not `Build(bDryRun)`.**
 * The question is asked once per shape-built `.gc` by a corpus sweep, so it has to cost a parse
 * rather than a mesh. A dry run answers the same question and builds every entry to do it.
 *
 * True means a `build { }` line in this program produces `PackagePath` AND names an entry the file
 * defines. False fills `OutReason` with which of those failed.
 */
using FGcShapeRecipeResolver = TDelegate<bool(
	const FString& /*ProgramText*/,
	const FString& /*PackagePath*/,
	FString& /*OutReason*/)>;

class GRAPHCODE_API FGraphCodeShapeProgram
{
public:
	/**
	 * `gcshape 1` on line one — the ONE string a Core-only build has to recognise.
	 *
	 * Deliberately the same shape as `FGraphCodeRegionWrite::LooksLikeRegionFile`, comment skipping
	 * included: a document is routed by the word it starts with, and two routers that disagree about
	 * what a leading `#` means would send the same file to two different places.
	 */
	static bool LooksLikeShapeProgram(const FString& Text);

	/**
	 * ONE CALL FOR THE WHOLE SEAM. A product registers everything it answers for or nothing:
	 * a half-registration would make `IsAvailable()` true while one question still refused as
	 * though the plugin were absent, which is the sentence that sends a reader to fix the one
	 * thing that is not broken.
	 */
	static bool Register(EGcProduct Product, FGcShapeProgramBuilder Builder,
		FGcShapeRecipeResolver Resolver, FGcShapeLibraryIndexer Indexer, FString& OutError);
	/** Optional, beside `Register`: with no starter bound, `BeginBuild` is `Build`. Move's `RegisterStarter`. */
	static bool RegisterStarter(EGcProduct Product, FGcShapeProgramStarter Starter, FString& OutError);
	/** Game thread. False = refused before anything left the game thread; true with `OutJob` = evaluating. */
	static bool BeginBuild(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		TSharedPtr<FGcShapeBuildJob>& OutJob, FGcShapeBuildResult& OutResult, FString& OutError);
	/** Holders finish and release Shape-owned jobs before its module unloads. */
	static FGcShapeJobsReleasing& JobsReleasing();
	static void Unregister(EGcProduct Product);

	/** Installed AND bound. A folder on disk whose module never started is not an installation. */
	static bool IsAvailable();

	/**
	 * Build the program, or refuse BY NAME.
	 *
	 * The refusal is `FGraphCodeProducts::UnavailableReason`, so the sentence is the one every other
	 * absent-product surface uses and there is one of it. It names GraphCode Shape and says
	 * installing is the fix — a refusal a model can relay without inventing the remedy.
	 */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcShapeBuildResult& OutResult, FString& OutError);

	/**
	 * H28's corpus question: is the recipe for this asset still in the program that built it?
	 *
	 * **THE FAILURE IT EXISTS FOR IS SILENT FOR AS LONG AS NOBODY BUILDS.** `SM_Trough.uasset` sat
	 * in the corpus with the level referencing it and its `.gc` naming the program by digest, while
	 * `HollowVale.gcshape` had no `Trough` entry at all — the asset had been built from a document
	 * pasted inline before a `.gcshape` could be applied from disk, and the entry never landed in
	 * the file. The evaluator's own `'$X' is not an entry in this file` was waiting the whole time
	 * and nothing asked it, because the asset already existed. The first whole-file apply, days
	 * later, refused at line 12 and the recipe was gone.
	 *
	 * **REFUSES BY NAME ON A CORE-ONLY INSTALL rather than answering false.** *No recipe* and *no
	 * product to ask* are different facts, and a sweep that folded them would report every
	 * shape-built asset in the corpus as broken on an install that simply does not have Shape.
	 */
	static bool HoldsRecipeFor(const FString& ProgramText, const FString& PackagePath,
		FString& OutReason, FString& OutError);

	/**
	 * **WHAT IS IN THESE FOLDERS OF `.gcshape` FILES** — the preset library's own question, and the
	 * third thing a product registers for. Refuses by name on a Core-only install, exactly as
	 * `Build` and `HoldsRecipeFor` do, because *no library* and *no product to ask* are different
	 * facts and folding them would report an empty shelf to somebody who simply has not installed
	 * GraphCode Shape.
	 */
	static bool IndexLibrary(const TArray<FString>& Roots, TArray<FGcShapeLibraryEntry>& OutEntries,
		TArray<FString>& OutSkipped, FString& OutError);

	/**
	 * **WHERE THE SHIPPED LIBRARY LIVES WHEN NOBODY SAYS** — a content path, which is vocabulary
	 * rather than code, so it is Core's for the same reason the header word is. One answer, so a
	 * search and a describe cannot disagree about what *the presets* means; `IndexLibrary` uses it
	 * when handed no roots at all.
	 */
	static const TArray<FString>& DefaultLibraryRoots();
};
