// Two-step import — `Docs/GC-IMPORT-DESIGN.md`.
//
// Step 1 (Plan) builds the pipeline objects Interchange WOULD use for a source file, projects them
// as a `kind:import` document, and throws them away. It creates nothing and touches no asset.
// Step 2 (Apply) takes that document back — edited or not — rebuilds the stack, imports the edits
// onto it, and runs the import.
//
// THE ONE THING TO UNDERSTAND BEFORE READING THE CODE: the settings are not on the pipeline. A
// `UInterchangeGenericAssetsPipeline` is nine scalars and five `UPROPERTY(Instanced)` pointers to
// sub-pipelines, and the 55 properties a person actually edits live on those
// (`InterchangeGenericAssetsPipeline.h:88-108`). Before row 11n, AuthoringCore stopped at an object
// property and exported one reference — the historical `instanced_subobjects` omission — so a flat
// projection showed a handful of scalars and paths while every real setting stayed invisible. The
// shared codec can carry the graph now. This import surface deliberately keeps its rule-7 CHILDREN:
// named pipeline stages are the stable, readable unit a caller edits and the import planner applies.
//
// Nothing here is a table of settings. The defaults come from the project's own configured pipeline
// stack and from each pipeline's CDO; a hand-maintained mirror of an engine settings object stops
// matching it silently, which is the failure the node property channel was built to avoid.

#pragma once

#include "CoreMinimal.h"

struct GRAPHCODE_API FGraphCodeImportResult
{
	bool bSuccess = false;

	/** The `kind:import` document — the plan, or the plan that was applied. */
	FString Text;

	/** Package paths of everything the import created or reimported. Empty on a plan or a dry run. */
	TArray<FString> ImportedAssets;

	TArray<FString> Changes;
	TArray<FString> Errors;

	/**
	 * The members an archive source imported, and the ones it could not — §7.
	 *
	 * REPORTED SEPARATELY FROM `Errors`, because a batch of 192 meshes where three fail is not a
	 * failed call. `Errors` is what makes `Apply` return false; these two are the roll-call of a run
	 * that happened. Both are empty for a plain single-file source, which has no members to count.
	 */
	TArray<FString> ImportedMembers;
	TArray<FString> FailedMembers;

	/**
	 * Every created asset was saved. C24.1, §11.1's invariant reaching this verb.
	 *
	 * **ALL OF THEM, NOT SOME** — an import builds a set, and "mostly saved" is the state that
	 * leaves a caller believing their mesh is on disk when its material is not. A partial save
	 * reports false and names what is missing in `SaveSkipped`.
	 */
	bool bSaved = false;

	/** Why `bSaved` is false, in the caller's words. Empty exactly when `bSaved` is true. */
	FString SaveSkipped;

	/** Which packages a save actually ran for, so a partial one is answerable rather than implied. */
	TArray<FString> SavedAssets;
};

class GRAPHCODE_API FGraphCodeImport
{
public:
	/**
	 * What a `kind:import` document does not carry, and the `ir:` it speaks — THE ONE COPY.
	 *
	 * Exposed 2026-09-17 because the capability registry was restating neither: it derives every
	 * kind's envelope from `FGraphCodeProjector::ExpectedEnvelope`, which had no `Import` case, so
	 * the registry published `omits: none` for a document that declares three tokens and
	 * `GraphCode.Capabilities.TheRegistryMatchesWhatTheProjectorActuallyWrote` read a committed
	 * import plan as a contradiction. Answered by MOVING THE READER to the one definition rather
	 * than by copying the list into the registry, which is the rule that file states about its own
	 * `Entry.Omits`: derived, never restated.
	 */
	static TArray<FString> Omissions();
	static const TCHAR* IrVersion();

	/**
	 * Step 1. The settings Interchange would use for this file, as text. Creates nothing.
	 *
	 * Destination is a folder (`/Game/Props`), existing or new. To choose a name, supply an
	 * explicit object path (`/Game/Props/SM_Chair.SM_Chair`). Folder existence never changes meaning.
	 */
	static bool Plan(const FString& SourceFile, const FString& Destination,
		FGraphCodeImportResult& OutResult);

	/**
	 * Step 1 for an ARCHIVE — `Docs/GC-IMPORT-DESIGN.md` §7.
	 *
	 * `SourceFile` may be a `.zip` or `.7z`, which is unpacked under `Intermediate/` and projected as
	 * ONE document listing every importable member in a `members { }` block. `MemberPatterns` are
	 * wildcards (`*.fbx`, `3D Files/FBX/*`) matched against each member's path inside the archive;
	 * an empty list means every member the engine can read.
	 *
	 * ONE DOCUMENT FOR THE WHOLE BATCH, WHICH IS THE POINT rather than a convenience. The settings a
	 * caller edits are the pipeline stack's, and a pack's 192 meshes share one stack — so a document
	 * per member would be 192 copies of one decision, and the middle step the two-step import exists
	 * for would become the thing nobody does. `destination` is a FOLDER for an archive, and the
	 * archive's own directory tree is mirrored under it.
	 *
	 * The patterns are honoured at PLAN time and the plan states what survived them, so the document
	 * remains the whole truth about what step 3 will import: deleting a line from `members { }` is
	 * how you narrow it further, and nothing that is not in the block can be imported by it.
	 */
	static bool Plan(const FString& SourceFile, const FString& Destination,
		const TArray<FString>& MemberPatterns, FGraphCodeImportResult& OutResult);

	/**
	 * Step 2. Run the import the document describes.
	 *
	 * A dry run resolves the document — every pipeline class, every property path — and reports
	 * what it would do without importing. As with the level writer, that proves structure and not
	 * values; a value is only proven by importing it.
	 *
	 * `bSave` IS NOT DEFAULTED, on purpose. C24.1 changes what this call does, and a default that
	 * flips silently rewrites behaviour nobody reviewed — so every call site states its choice and
	 * a reader can see which ones mean it.
	 */
	static bool Apply(
		const FString& DocumentText, bool bDryRun, bool bSave, FGraphCodeImportResult& OutResult);

	/**
	 * The `import { }` block for an asset that ALREADY EXISTS — GC-IMPORT-DESIGN.md §1, build order 3.
	 *
	 * The prize the two-step import was built for: import settings enter git, so a source file
	 * moving shows up in `git blame` with a commit message instead of living in `UAssetImportData`
	 * inside the `.uasset` where nothing can read it.
	 *
	 * FOUND BY REFLECTION, not by a per-class property name. Every importable asset holds its record
	 * in a `UAssetImportData` sub-object, and each class calls the property something slightly
	 * different; walking for the first object property whose class derives from `UAssetImportData`
	 * is the same rule the node property channel follows, and it means a kind that gains a `.gc`
	 * later gets this block with no change here.
	 *
	 * ALWAYS RETURNS A BLOCK, empty when nothing imported the asset. `omits:` has to be answerable
	 * from the KIND alone for gc_sync's registry fast path, so "no import" cannot be spelled as a
	 * per-asset omission — and it does not need to be. An empty block says "this asset was not
	 * imported", which is the coverage statement rule 4 already gives every other facet.
	 *
	 * WHAT IT WRITES IS THE STORED RELATIVE PATH, never a resolved absolute one.
	 * `UAssetImportData` deliberately stores a path relative to the package so it survives a
	 * different machine; resolving it here would put `D:/someone-elses-drive/…` in a file whose
	 * whole purpose is to diff cleanly between checkouts.
	 */
	static FString AssetImportBlock(const UObject* Asset);

	/**
	 * The stored Interchange pipeline settings for an asset that already exists.
	 *
	 * Interchange serializes the exact pipeline objects used for an import inside
	 * `UInterchangeAssetImportData`. Projecting those objects, rather than rebuilding today's
	 * project-default stack, is what makes a six-month-old import reproducible after defaults move.
	 * The block is always returned. A legacy import with no stored Interchange pipeline gets an
	 * explicit `# UNSUPPORTED:` diagnostic inside it rather than silently looking complete.
	 */
	static FString AssetPipelineBlock(const UObject* Asset);

	/**
	 * Reimport one existing Interchange asset from its `import { }` and `pipelines { }` blocks.
	 *
	 * The proposed source hash must match the file now on disk. The source path itself may not be
	 * repointed here: Unreal's reimport identity includes the stored source slot, so changing it is
	 * a separate operation rather than a settings edit. A dry run parses every pipeline and property
	 * but invokes no Interchange work.
	 */
	static bool Reimport(UObject* Asset, const FString& ImportBlockText,
		const FString& PipelineBlockText, bool bDryRun, bool bSave,
		FGraphCodeImportResult& OutResult);
};
