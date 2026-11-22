// Copyright OraStudio. All Rights Reserved.

// WRITING A `.gcw` — `GC-SPATIAL-AUTHORING-DESIGN.md` §21, task R5.
//
// §18 designed the region file as authored-by-hand and read-only to tooling, and §21.8 corrected
// that: *a tool-driven write is not the auto-write that rule forbids — it is `gc_apply`'s
// relationship to a `.gc`*. So this is not a new mechanism. It is the same one, one document over.
//
// **THE DOCUMENT SAYS WHAT IT IS, so nothing new is spelled at the tool.** A payload whose header is
// `gcw 1` is a region file and `gc_apply` routes it here; `level:` names the level it belongs beside.
// That is exactly how a `.gc` payload already tells `gc_apply` its kind and its asset, and it is why
// R5 costs no parameter, no `kind:` value on the write side and no second verb.
//
// FOUR RULINGS THIS FILE MAKES, none of which §21 states:
//
//   1. **A `.gcw` PAYLOAD IS COMPLETE: omitting a declaration REMOVES it.** Every other choice was
//      worse. A level deletes actors only through explicit `delete` lines because a level document is
//      a PROJECTION of hundreds of things nobody typed — omission there is usually ignorance. A
//      `.gcw` is the opposite: it is small, hand-authorable, and every name in it is authored, with
//      no upstream to diff against. Requiring `delete e7` would mean a file could not be written by
//      simply stating what it should contain, which is the whole point of a text document.
//
//   2. **AND EVERY REMOVAL IS PRICED BY NAME.** The complete-payload reading is only safe because a
//      dry run reports what would go, one line per declaration, before anything is written. That is
//      this project's standing rule for deletions, not a courtesy.
//
//   3. **A NAME THAT WAS MINTED AND IS NOW ABSENT MAY NOT COME BACK.** `GraphCodeRegionFile.h`'s
//      note 2 exists because delete-then-mint hands out a name that still RESOLVES: drop `e9`, mint
//      an edge, and every `@anchor[target=$Crypt.e9]` in the project now points at a different wall
//      with nothing erroring anywhere. The parser records the high-water mark so the tool cannot do
//      that; this refuses a hand-written payload that does it, which is the same defect arriving
//      through the door the parser cannot see.
//
//   4. **WHAT IS WRITTEN IS THE CANONICAL RENDER, NEVER THE PAYLOAD'S BYTES.** Same as a `.gc`:
//      block order, spacing and the spelling of every number belong to the writer. So applying the
//      same document twice is a no-op, and reading back what you wrote gives you the file rather
//      than your own typing. An author keeps their COMMENTS, which R1's trivia already carries.
//
// AND A REGION WRITE CANNOT LOSE A SUBSCRIPTION. `minions.gcs { … }` and every other block this
// parser does not own are kept verbatim as `ForeignBlocks` and put back by the same `Render`, so
// "complete" means complete in the DECLARATIONS this file owns — not in the file's other tenants.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"

class UWorld;

/** Refusal tokens for the write, so a caller branches on the reason and not on the sentence. */
namespace GcwWriteToken
{
	inline constexpr const TCHAR* NotARegionFile = TEXT("gcw_not_a_region_file");
	inline constexpr const TCHAR* LevelMismatch = TEXT("gcw_level_mismatch");
	inline constexpr const TCHAR* MintReused = TEXT("gcw_mint_reused");
	inline constexpr const TCHAR* MintRewound = TEXT("gcw_mint_rewound");
	inline constexpr const TCHAR* WriteFailed = TEXT("gcw_write_failed");
}

/** One declaration that the payload does not carry and the file on disk does. */
struct GRAPHCODE_API FGcwRemoval
{
	/** `point`, `edge` or `region` — what kind of declaration is going. */
	FString Kind;

	FString Name;

	/**
	 * What else in the PAYLOAD still refers to it, if anything.
	 *
	 * A removal nothing references is a tidy-up; a removal something still names is the one a caller
	 * has to look at, and the difference is worth more than the count.
	 */
	TArray<FString> StillReferenced;
};

/** What a write would do, before it does any of it. */
struct GRAPHCODE_API FGcwWritePlan
{
	/** The level this file belongs beside, from the payload's header or the caller's target. */
	FString LevelPath;

	/** Where it would be written. */
	FString Path;

	/** True when no `.gcw` exists yet: this write creates the level's first regions. */
	bool bCreates = false;

	/** Canonical text — what would actually land on disk, not the payload's bytes. */
	FString Text;

	/** Byte-identical to what is already there, so the write would change nothing. */
	bool bUnchanged = false;

	/**
	 * The canonical text of the document this plan was DIFFED AGAINST — empty on a create.
	 *
	 * **IT IS THE ONLY THING THAT CAN TELL `Commit` THE GROUND MOVED UNDER IT, and the writer that
	 * moves it is usually OURS.** Saving a level fires three recorders that render this same
	 * document and save it — a terrain sculpt, a water `[override=]` and the volume reflect — and a
	 * `.gcw` write saves the level BEFORE it commits, so the file at commit time is not always the
	 * file the plan read. A `.gcw` carries no `sig:`/`bin:` to compare (it is authored, not
	 * projected), so without this there is nothing to notice with ([[the-gc-on-disk-is-the-second-carrier]]).
	 */
	FString BaseText;

	/** Priced by name, never counted — ruling 2 above. */
	TArray<FGcwRemoval> Removals;

	/** Declarations the payload adds. Reported for symmetry so a plan reads as a diff. */
	TArray<FGcwRemoval> Additions;

	int32 RegionCount = 0;
	int32 EdgeCount = 0;
	int32 PointCount = 0;

	/**
	 * WHAT PARSED AND IS INERT — one whole sentence each, reported to the caller.
	 *
	 * **A WARNING WITH NOWHERE TO ARRIVE IS NOT A WARNING** ([[a-deferred-act-owes-an-outcome]]), and
	 * this field is the only reason §14.11's ruling is shipped rather than merely coded: the user
	 * ruled that an unreferenced region block WARNS instead of refusing, and a warning the tool never
	 * prints leaves an author with a block that quietly fills nothing and no way to find out.
	 */
	TArray<FString> Warnings;
};

/** The `.gcw` write surface. `Plan` is pure; `Commit` is the only thing here that touches disk. */
class GRAPHCODE_API FGraphCodeRegionWrite
{
public:
	/** `gcw` as the payload's first token — how `gc_apply` recognises a region file at all. */
	static bool LooksLikeRegionFile(const FString& Text);

	/**
	 * Parse the payload, check it against the file already there, and say what would change.
	 *
	 * `Existing` is the document on disk, or `nullptr` when there is none — a create. Everything
	 * here is arithmetic and string work over two documents: no file is read and none is written.
	 */
	static bool Plan(const FString& PayloadText, const FGcwDocument* Existing,
		const FString& TargetLevel, const FString& TargetPath, FGcwWritePlan& OutPlan,
		FString& OutRefusal);

	/**
	 * Write `Plan.Text` to `Plan.Path`, creating the directory if the level has no sidecars yet.
	 *
	 * **REFUSES IF THE FILE MOVED SINCE THE PLAN WAS MADE — W10.** The plan is a diff against a
	 * document read earlier in the call, and a `.gcw` write saves the LEVEL first, which fires the
	 * save-time recorders that write this very file. Overwriting what they left deleted the one
	 * line naming a hand sculpt and orphaned its blob; the ground had already been moved out of the
	 * landscape by then, so the blob was its only carrier ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	static bool Commit(const FGcwWritePlan& WritePlan, FString& OutRefusal);
};

/**
 * Marks the synchronous level save performed by a `.gcw` apply. Save-time recorders must not
 * compare the newly realised world with the OLD `.gcw` on disk and reflect that difference as a
 * person's edit while the new program is waiting to commit. Ordinary user saves still record
 * edits, and Commit's changed-file guard still protects genuine concurrent writes.
 */
class GRAPHCODE_API FScopedGcwApplySave
{
public:
	explicit FScopedGcwApplySave(const UWorld* World);
	~FScopedGcwApplySave();
	static bool IsActiveFor(const UWorld* World);

private:
	const UWorld* Previous = nullptr;
};
