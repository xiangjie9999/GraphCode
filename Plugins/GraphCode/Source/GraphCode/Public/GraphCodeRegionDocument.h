// Copyright OraStudio. All Rights Reserved.

// WHERE A LEVEL'S `.gcw` IS — `GC-SPATIAL-AUTHORING-DESIGN.md` §21, tasks R2 and R4.
//
// The impure inch of the region surface, and it is deliberately its own file. `GraphCodeRegionFile.h`
// turns text into a document and promises no world, no asset and no editor; `GraphCodeRegion.h` is
// pure arithmetic. Something still has to know that a level's regions live beside its `.gc`, and
// putting that in either of them would falsify a card.
//
// ONE DOOR, AND THAT IS THE WHOLE REASON THIS IS SHARED. The level writer resolves `$Crypt` while
// applying a payload (`FGraphCodeLevelPlacement::AddRegionTargets`) and `gcw_ask` answers questions
// about the same name with no editor open. If those two found their file by two rules, a payload
// could place an actor against one polygon while the question that checked the work read another —
// and nothing would error. They call this.
//
// NO EDITOR AND NO LOADED WORLD, which is §21.10's F1: a level PACKAGE NAME is enough, so *the
// project becomes askable without opening it* (§19.8) is finally true of something.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"

/** Refusal tokens for the lookup, so a caller branches on the reason and not on the sentence. */
namespace GcwDocumentToken
{
	inline constexpr const TCHAR* PathUnavailable = TEXT("gcw_path_unavailable");
	inline constexpr const TCHAR* FileMissing = TEXT("gcw_file_missing");
	inline constexpr const TCHAR* FileUnreadable = TEXT("gcw_file_unreadable");
	inline constexpr const TCHAR* ParseFailed = TEXT("gcw_parse_failed");
	inline constexpr const TCHAR* LevelMismatch = TEXT("gcw_level_mismatch");
}

/** A level's region document, found and parsed. Reads a file; touches no world and no asset. */
class GRAPHCODE_API FGraphCodeRegionDocument
{
public:
	/** `/Game/Maps/Dungeon` to `…/Dungeon.gcw`, beside the level's own sidecar. */
	static FString PathForLevel(const FString& LevelPackage, FString& OutRefusal);

	/**
	 * The folder that file lives in — what a `file'…'` a line names is read relative to. Empty where the
	 * level has no place on disk. ONE door for a sculpt and a painted forest alike.
	 */
	static FString FolderForLevel(const FString& LevelPackage);

	/** True when that file exists. Asking is not an error: most levels declare no regions. */
	static bool ExistsForLevel(const FString& LevelPackage);

	/**
	 * Load and parse it, refusing by token with the path in the sentence.
	 *
	 * A `level:` HEADER THAT NAMES A DIFFERENT LEVEL IS REFUSED rather than trusted or ignored. The
	 * file's position beside the level is what makes it this level's, so a header disagreeing with its
	 * own location means one of the two moved — and picking either would author against a room the
	 * caller cannot see.
	 */
	static bool LoadForLevel(const FString& LevelPackage, FGcwDocument& OutDocument, FString& OutPath,
		FString& OutRefusal);

	/**
	 * **THE ONLY WAY A `.gcw` IS REPLACED, AND IT KEEPS A COPY OF WHAT WAS THERE FIRST.**
	 *
	 * A region file is AUTHORED. Nothing projects one, nothing can re-derive one, and the version
	 * being overwritten exists in exactly one place — so the write is the instant it stops existing.
	 * That was true of a `.gc` too until E11.0 gave it local history; this stream had none at all
	 * until 2026-09-12, which is what `FGraphCodeHistory::ArchiveAuthoredFile` closes.
	 *
	 * **AND IT IS ONE DOOR FOR THE SAME REASON THE LOOKUP ABOVE IS.** Four different things rewrite
	 * this file — a payload apply, and the sculpt, water and volume recorders that run on a level
	 * save — so a backup bolted onto each of them is four places for the fifth writer to forget.
	 * The header's own argument, applied to the write instead of the read.
	 *
	 * **A FAILED COPY IS REPORTED, NEVER A REFUSAL.** Every caller reaches here after the EDITOR
	 * has already moved: a recorder has flattened the ground it is recording, and the payload route
	 * saves the level before it commits. Refusing would leave the file describing a world that is no
	 * longer in the map, which is the state this feature exists to prevent.
	 *
	 * **AND IT SETTLES AN ENCODING THE FOUR WRITERS DID NOT AGREE ON.** The payload route always
	 * passed `ForceUTF8WithoutBOM`; the three recorders took `SaveStringToFile`'s default, which is
	 * `AutoDetect` — **UTF-16 with a BOM the moment the text holds one non-ASCII character.** So a
	 * level whose `.gcw` carried an accented comment or one of this project's Cyrillic asset names
	 * changed encoding depending on which writer touched it last, and a UTF-16 file is the one
	 * `grep` reports as binary and finds nothing in
	 * ([[defaultengine-ini-is-utf16-and-grep-reads-it-as-binary]]). One door, one encoding, UTF-8.
	 *
	 * @param OutError  set only when the WRITE failed, which is the caller's own refusal to word.
	 */
	static bool SaveDocument(const FString& DocumentPath, const FString& Text, FString& OutError);
};
