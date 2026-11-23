#pragma once

#include "CoreMinimal.h"
#include "Internationalization/Text.h"

/**
 * S2.2 — **taking `.gc` files away when the setting that produced them stops applying.**
 * `GC-SETTINGS-DESIGN.md` §4.1b owns the rules; this is their one implementation.
 *
 * TWO SETTINGS REACH IT AND THEY ARE ONE BEHAVIOUR AT TWO SCOPES. Turning `bEnableSidecars` off
 * says *no `.gc` anywhere*; adding a folder to `ExcludedFolders` says *no `.gc` under here*. Both
 * were user-ruled to REMOVE what is already on disk rather than leave it (§4.1, §3.3), and leaving
 * it is the worst of the three options that were open: nothing regenerates a stale sidecar while
 * the switch that made it is off, so it sits in the panel and in `git status` describing an asset
 * it drifts further from every day, until `GcCheck -Prune` deletes it weeks later with nothing to
 * connect the two events.
 *
 * FOUR RULES, AND EACH ONE IS A DECISION SOMEBODY MADE:
 *
 *  - **It plans before it moves.** `Plan` touches nothing and `ComposeConfirmation` turns it into
 *    the sentence a person answers — with counts, because a checkbox is explicit intent only if
 *    the person could see what it costs. The split is also what lets a headless spec read the
 *    words, the same shape `FGcNotify` uses for toasts.
 *  - **It moves, it never unlinks.** Everything goes under `Saved/GraphCode/RemovedSidecars/<UTC
 *    stamp>/`, mirroring the long package name so the way back is obvious and two folders with the
 *    same leaf cannot collide. An uncommitted `.gc` is otherwise gone for good, and the whole
 *    reason somebody turns this off is that they are unsure.
 *  - **It never empties that store.** Same rule as `/Game/__MCPTrash`: GraphCode does not clean up
 *    its own recoverable deletions on a person's behalf.
 *  - **`.gcw` IS NEVER TOUCHED** (§4.1a). A `.gc` is projected and can be rebuilt from its asset by
 *    one sweep, which is what makes moving it recoverable. A `.gcw` is somebody's world program —
 *    the regions they named, the terrain and water hanging off them — and nothing can rebuild it,
 *    because there is no asset it is a projection of. The enumeration takes `.gc` and asserts the
 *    extension rather than trusting the wildcard, and a spec puts a `.gcw` beside a doomed `.gc`.
 */

/** Whole project, or one folder. The only difference is which files the plan holds. */
enum class EGcSidecarRemovalScope : uint8
{
	/** `bEnableSidecars` went off: every `.gc` under every root GraphCode sweeps. */
	Project,
	/** A folder joined `ExcludedFolders`: every `.gc` whose package sits under it. */
	Folder,
};

/** What a removal would do, decided and not yet performed. Nothing here has moved. */
struct GRAPHCODE_API FGcSidecarRemovalPlan
{
	EGcSidecarRemovalScope Scope = EGcSidecarRemovalScope::Project;

	/** The long package folder at folder scope — `/Game/Fab`. Empty at project scope. */
	FString Folder;

	/** Absolute `.gc` paths, sorted, GraphCode's own scratch already dropped. */
	TArray<FString> Files;

	/**
	 * The subset git has in its index, which is the half of the count that changes a person's mind.
	 *
	 * A tracked `.gc` reappears as a deletion in everybody's `git status` and is recoverable from
	 * the branch; an untracked one exists nowhere but this disk. Empty when the project is not in a
	 * work tree, which is a normal state — the removal still happens and the sentence says less.
	 */
	TSet<FString> Tracked;

	/** `Saved/GraphCode/RemovedSidecars/<stamp>/`, absolute, trailing slash. Fixed when planned. */
	FString DestinationRoot;

	int32 Num() const { return Files.Num(); }
	int32 NumTracked() const { return Tracked.Num(); }
	bool IsEmpty() const { return Files.IsEmpty(); }

	/** Where one planned file goes, absolute. Mirrors its long package name under the stamp. */
	FString DestinationFor(const FString& SidecarPath) const;
};

/** What actually happened. A failure is reported per file; the rest of the plan still runs. */
struct GRAPHCODE_API FGcSidecarRemovalResult
{
	TArray<FString> Moved;

	/** `<absolute path>: <reason>` for each file that could not be moved. */
	TArray<FString> Failed;

	bool IsClean() const { return Failed.IsEmpty(); }
};

class GRAPHCODE_API FGcSidecarRemoval
{
public:
	/** `Saved/GraphCode/RemovedSidecars/`, absolute, trailing slash. Created on the first move. */
	static FString StoreRoot();

	/**
	 * Every `.gc` the whole project would lose. Writes nothing, loads no asset.
	 *
	 * @param Now  the moment the destination folder is named after. Passed in rather than read so
	 *             a spec's two plans cannot collide in the same second — and so the stamp a person
	 *             confirmed is the stamp the move uses.
	 */
	static void PlanProject(const FDateTime& Now, FGcSidecarRemovalPlan& OutPlan);

	/**
	 * Every `.gc` under one long package folder. Writes nothing, loads no asset.
	 *
	 * The folder is matched the way `FGraphCodePolicy::IsUserExcludedFolder` matches it — equal, or
	 * under `entry + "/"` — because the two have to agree about which files an exclusion covers or
	 * this removes a file the setting would go on writing.
	 */
	static void PlanFolder(const FString& Folder, const FDateTime& Now, FGcSidecarRemovalPlan& OutPlan);

	/**
	 * The words, without Slate, so a spec can read them. Empty text for an empty plan.
	 *
	 * Says the count, the tracked half, and where the files go — in that order, because the first
	 * decides whether somebody reads the rest.
	 */
	static FText ComposeConfirmation(const FGcSidecarRemovalPlan& Plan);

	/** Move every planned file. Creates the destination; never deletes anything already in it. */
	static bool Execute(const FGcSidecarRemovalPlan& Plan, FGcSidecarRemovalResult& OutResult);
};
