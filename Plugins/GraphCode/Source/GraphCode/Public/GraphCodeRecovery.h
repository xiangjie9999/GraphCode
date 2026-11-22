#pragma once

#include "CoreMinimal.h"

/**
 * E11.7a/E11.9 — THE TWO PLACES A REVERT PUTS THINGS SO THEY CAN COME BACK.
 *
 * `GC-LOCAL-HISTORY-DESIGN.md` §2.7 owns the removal rules and §9 owns the project-wide gates.
 * Read those first; several shapes here look like ceremony until you know which one produced them.
 *
 * TWO STORES, AND THEY ARE NOT THE SAME STORE:
 *
 *  - **`FGcRecoveryTrash`** is where an asset goes when a restore decides the target did not have
 *    it. Never a file delete — the `.uasset` moves to an excluded content path and the sidecar
 *    moves under `Saved/`, with a manifest naming both. A `.gcc` carries only what the format
 *    carries, so for a REMOVAL the moved package is the recovery, not the text.
 *  - **`FGcRevertBackup`** is the undo for a whole project-wide revert: one folder holding the
 *    on-disk bytes of every file the revert is about to touch. `FGraphCodeHistory` already
 *    checkpoints each asset's canonical TEXT; this holds the FILES, which is what puts a project
 *    back when the text was never the whole story.
 *
 * NEITHER IS EVER EMPTIED AUTOMATICALLY, and that is not the same rule twice. The trash is
 * unbounded on purpose (§2.7). The backup is exactly one folder and IS replaced — but only with a
 * warning first, because replacing it is the one moment somebody's undo disappears.
 */

class UObject;

// -----------------------------------------------------------------------------------------------
// Recovery trash
// -----------------------------------------------------------------------------------------------

/** What a removal would do, decided and not yet performed. */
struct GRAPHCODE_API FGcTrashPlan
{
	FString PackageName;

	/** `/Game/_GraphCode/Trash/<Name>`, uniquified against what is already in there. */
	FString TrashPackageName;

	/** Absolute. Where the `.gc` will be moved to under `Saved/GraphCode/RecoveryTrash/`. */
	FString SidecarRecoveryPath;

	/** Absolute path of the sidecar as it stands, or empty when the asset has none. */
	FString SidecarPath;

	/**
	 * Packages that still point at this one, EXCLUDING the removal set the caller passed.
	 *
	 * The exclusion is what makes a project-wide revert possible at all: three assets that
	 * reference each other and are all leaving together have no surviving referencer between them,
	 * and analysing them one at a time would refuse all three.
	 */
	TArray<FString> Referencers;

	/** Non-empty means the plan refuses. Nothing has been touched. */
	FString Refusal;

	bool IsRefused() const { return !Refusal.IsEmpty(); }
};

/** One asset that left the project, and everything needed to bring it back. */
struct GRAPHCODE_API FGcTrashRecord
{
	FString PackageName;
	FString TrashPackageName;
	FString SidecarRecoveryPath;

	/** Ties every removal of one revert together, so a reader sees one gesture rather than eleven. */
	FString GroupId;

	FDateTime TimeUtc;

	/** Who still pointed at it when it left. Recorded because after the move it cannot be asked. */
	TArray<FString> Referencers;

	/**
	 * The source `.uasset` the move could not remove, or empty — which is the ordinary answer.
	 *
	 * A move that leaves the file at the original path is not a move: the asset registry finds it
	 * again on the next editor start while the manifest says it left. Normally the file goes and
	 * this stays empty; when the filesystem refuses, the removal still succeeded — the bytes are in
	 * the trash package — so this reports rather than fails, and the caller says so out loud.
	 */
	FString SourceFileRemains;
};

class GRAPHCODE_API FGcRecoveryTrash
{
public:
	/** `/Game/_GraphCode/Trash` — excluded from projection, so nothing in it grows a sidecar. */
	static const TCHAR* ContentRoot();

	/** `Saved/GraphCode/RecoveryTrash/`, absolute, trailing slash. */
	static FString SidecarRoot();

	/** `Saved/GraphCode/RecoveryTrash/manifest.jsonl` — one line per removal, append only. */
	static FString ManifestPath();

	/**
	 * Who still references `PackageName`, minus `AlsoLeaving` and minus anything already in trash.
	 *
	 * EXACT, from the asset registry's hard dependency graph, not a name search. A referencer this
	 * misses is an asset that comes back broken.
	 */
	static TArray<FString> Referencers(
		const FString& PackageName, const TSet<FString>& AlsoLeaving);

	/**
	 * Decide a removal. Touches nothing, loads nothing that is not already loaded.
	 *
	 * Refuses when: the kind's registry entry says removal is unsupported; the package is already
	 * inside the trash; the package is a level that is currently open in this editor; or something
	 * outside `AlsoLeaving` still references it and `bAcceptBreakingReferences` is false.
	 */
	static bool Plan(
		const FString& PackageName,
		const TSet<FString>& AlsoLeaving,
		bool bAcceptBreakingReferences,
		FGcTrashPlan& OutPlan,
		FString& OutError);

	/**
	 * Perform a plan: move the package, move the sidecar, append the manifest line.
	 *
	 * **ORDER IS THE SAFETY ARGUMENT.** The manifest line is written LAST, because a manifest
	 * naming a move that did not happen is worse than a move no manifest names: the first sends a
	 * recovery at a path that holds nothing, the second is still discoverable by looking in the
	 * trash. If the sidecar move fails the package move is put back.
	 *
	 * The moved package is left DIRTY AND UNSAVED like every other GraphCode mutation, so the whole
	 * revert is one Ctrl+Z. Its file on disk is untouched until somebody saves, which is why the
	 * revert backup holds the bytes as well.
	 */
	static bool Move(
		const FGcTrashPlan& Plan,
		const FString& GroupId,
		bool bDryRun,
		FGcTrashRecord& OutRecord,
		FString& OutError);

	/** Every removal ever recorded, newest last. Empty is the ordinary state. */
	static TArray<FGcTrashRecord> Manifest();
};

/**
 * **A NAME BEING REUSED RATHER THAN FOLLOWED — and it has to be said out loud, because the editor
 * assumes the opposite for the rest of the session.**
 *
 * When an asset is renamed or moved to trash, the editor records an in-memory redirection from its
 * old path to its new one in `GRedirectCollector`, so soft references keep resolving. That is right
 * for a reference and wrong for a NAME A BUILD IS ABOUT TO TAKE BACK: the old path is not going to
 * the trash, it is about to hold a new asset.
 *
 * **LEAVING IT STANDING SILENTLY CORRUPTS WHAT IS SAVED AT THAT PATH, which is how this was found
 * (`GC-MOVE-ANIMATION-DONE.md` §16).** `FSoftObjectPath::SerializePath` applies the redirection to EVERY soft path a
 * saving archive touches — and a package's metadata map is keyed BY soft path and serialized in
 * place, so saving the rebuilt asset rewrote its own provenance key from `/Game/X/A_Lift.A_Lift` to
 * the trashed `/Game/X/Trash_A_Lift.Trash_A_Lift`. The stamp was never deleted; it was relabelled
 * onto the asset that had been carried away, and the next run read the clip as something nothing
 * built. Nothing logs, because to the engine this is an ordinary reference fixup.
 *
 * It is Core's because all three build products take names back the same way.
 */
struct GRAPHCODE_API FGcAssetPathReuse
{
	/**
	 * Forget any redirection standing for the asset this package path names, so a save writes the
	 * path as itself. True where one was there — which is worth reporting, since it means something
	 * in this session had carried that name away.
	 *
	 * **Call it BEFORE the asset is created**, not before the save: creation is where the claim
	 * "this name is mine again" is made, and anything the engine does in between would otherwise
	 * still be following the old redirection.
	 */
	static bool Forget(const FString& PackagePath);
};

// -----------------------------------------------------------------------------------------------
// The one revert backup
// -----------------------------------------------------------------------------------------------

/** One file the backup holds, as it was when the backup was taken. */
struct GRAPHCODE_API FGcBackupFile
{
	/** Absolute path in the project the file came from and goes back to. */
	FString OriginalPath;

	/** Absolute path of the copy under the backup root. Empty when `bWasAbsent`. */
	FString BackupPath;

	int64 Size = 0;
	FDateTime ModifiedUtc;

	/** MD5 of the bytes when the backup was taken. What "has this really changed" is asked against. */
	FString Digest;

	/**
	 * The file did NOT exist when the backup was taken.
	 *
	 * Recorded rather than skipped, because undoing a revert that CREATED an asset means putting
	 * the absence back. A backup that only knows about files that existed cannot undo a create.
	 */
	bool bWasAbsent = false;
};

struct GRAPHCODE_API FGcBackupStatus
{
	bool bExists = false;
	FDateTime TakenUtc;
	FString GroupId;

	/** What the backup was of, in the words the reader saw: `project at 5 Aug · 14:32`. */
	FString Label;

	int32 FileCount = 0;
	int64 Bytes = 0;

	/** Package names the backup covers. */
	TArray<FString> Packages;

	/**
	 * A file the backup holds has changed on disk since it was taken — somebody SAVED.
	 *
	 * **THIS IS THE WHOLE "ARE THEY STILL EXPLORING" TEST.** A revert leaves every asset dirty and
	 * UNSAVED, so it changes no file: revert to Tuesday, look, revert to Monday, look again, and the
	 * backup taken before the first one is still the undo for all of it. The moment a real file
	 * changes, it is not, and the next revert has to take a new one — which destroys this.
	 */
	bool bStale = false;

	/** Which files moved, so the warning can name one rather than assert staleness. */
	TArray<FString> ChangedSince;
};

class GRAPHCODE_API FGcRevertBackup
{
public:
	/** `Saved/GraphCode/RevertBackup/`, absolute, trailing slash. ONE folder, by design. */
	static FString Root();

	/** What is in there now, including whether anything it holds has been saved over since. */
	static FGcBackupStatus Status();

	/**
	 * Does this revert need a NEW backup, or does the one on disk already undo it?
	 *
	 * **USER-DIRECTED 2026-08-05:** *"for continuous revert before any real file change, dont backup
	 * the folder, because they are just exploring."* Three conditions, all required, and the middle
	 * one is the one that is easy to leave out:
	 *
	 *  1. a backup exists;
	 *  2. it covers every package this revert would touch — a revert reaching one asset further
	 *     than the last one has an asset the backup cannot put back;
	 *  3. nothing it holds has been written since it was taken.
	 *
	 * @param OutWhy  the sentence to show when the answer is yes. Empty when it is no.
	 */
	static bool NeedsFresh(
		const TArray<FString>& PackageNames, const FGcBackupStatus& Current, FString& OutWhy);

	/**
	 * The sentence a reader must see BEFORE an existing backup is replaced.
	 *
	 * **USER-DIRECTED: *"warn them if we are going to delete the backup."*** Empty when there is
	 * nothing to warn about — no backup, or one this revert is going to reuse.
	 */
	static FString ReplacementWarning(const FGcBackupStatus& Current);

	/**
	 * Copy every named package's `.uasset`/`.umap` and `.gc` into the backup root.
	 *
	 * **REPLACES WHATEVER IS THERE, and refuses to do so silently.** `bAcceptReplacingExisting`
	 * must be true when `Status().bExists`; without it this returns false and says what would have
	 * been lost. That flag is the record of somebody having read `ReplacementWarning`.
	 *
	 * A package with no file on disk is recorded as `bWasAbsent` rather than skipped — see
	 * `FGcBackupFile::bWasAbsent`.
	 */
	static bool Take(
		const TArray<FString>& PackageNames,
		const FString& Label,
		const FString& GroupId,
		bool bAcceptReplacingExisting,
		FGcBackupStatus& OutStatus,
		FString& OutError);

	/**
	 * Put every backed-up file back, and delete the ones that were absent when it was taken.
	 *
	 * **FILES ONLY. IT DOES NOT TOUCH THE EDITOR.** Packages already loaded in memory still hold
	 * the reverted state, so this is the undo for a revert that was SAVED, and the caller has to
	 * reload or restart. Saying that plainly is better than pretending otherwise: a silent
	 * half-undo where the disk says one thing and the editor says another is the worst outcome
	 * available here.
	 */
	static bool RestoreAll(int32& OutRestored, TArray<FString>& OutReloadNeeded, FString& OutError);

	/** Delete the backup folder. Only ever from an explicit gesture. */
	static bool Discard(FString& OutError);

	/** Absolute path of the `.uasset`/`.umap` a package name resolves to, or empty. */
	static FString PackageFilePath(const FString& PackageName);
};
