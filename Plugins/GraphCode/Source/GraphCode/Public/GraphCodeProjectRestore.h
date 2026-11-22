#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRecovery.h"
#include "GraphCodeRestore.h"

/**
 * E11.9 — TAKING THE WHOLE PROJECT BACK TO ONE MOMENT.
 *
 * `GC-LOCAL-HISTORY-DESIGN.md` §9. The panel's time machine already shows the project at an
 * instant; this is the verb that makes it so.
 *
 * **§9 IS A LIST OF GATES, NOT A LIST OF FEATURES, AND EVERY ONE OF THEM IS HERE:**
 *
 *  - the scope is GRAPHCODE-COVERED ASSETS, never "the whole project" — anything without a sidecar
 *    or a history lineage is not in the set and is said to be out of it;
 *  - ONE coherent cutoff resolves into an explicit per-asset action set, `Apply`/`Create`/`Remove`/
 *    `NoChange`, decided before anything moves;
 *  - EVERY target is preflighted before the FIRST mutation;
 *  - every live asset is checkpointed under ONE group id;
 *  - the whole run is one transaction, so it is one Ctrl+Z;
 *  - every read-back is verified, by the same per-asset pipeline that already did;
 *  - everything is left DIRTY AND UNSAVED with one group-level report.
 *
 * **AND THE ONE §9 DOES NOT NAME, WHICH THE USER DID:** a single backup folder holding the FILES,
 * so a revert that was saved can still be undone. `FGcRevertBackup` owns it. The rule that makes it
 * usable rather than annoying is that an EXPLORING revert reuses the backup it already has — see
 * `FGcRevertBackup::NeedsFresh`.
 *
 * **WHAT THIS IS NOT: a loop over `gc_apply`.** §9 says a sequential best-effort loop under an
 * atomic-sounding name is not a safety feature, and the difference is the two passes below: every
 * plan is built and preflighted, and only then does the first write happen.
 */

/**
 * WHAT THE WHOLE PROJECT IS BEING PUT BACK TO. E11.9.
 *
 * **USER-DIRECTED 2026-08-05:** *"revert can not only apply to timeline, but to git branches and
 * head etc."* — and the first cut of this was written against the timeline alone, which would have
 * been a project revert that could not do the commonest thing anybody wants from one.
 *
 * THE TWO TARGETS RESOLVE DIFFERENTLY AND EVERYTHING AFTER THAT IS SHARED. What differs is only how
 * one asset's old text is found and how *"did it exist then"* is answered:
 *
 * | | instant | ref |
 * |---|---|---|
 * | scope | every covered package | every `.gc` `git diff <ref>` names |
 * | old text | the archive at or after the moment | `git show <ref>:<path>` |
 * | existence then | `EGcHistoryKnowledge`, gated on the timeline stamp | the blob is there, or it is not |
 * | how far back | bounded by T and by pruning | bounded by nothing — git kept it all |
 *
 * The last row is why a ref target needs no timeline stamp: git has the whole history and does not
 * lose entries to a byte budget, so the two gates that exist for the store's honesty have nothing
 * to be honest about here.
 */
struct GRAPHCODE_API FGcProjectRestoreSide
{
	/** True for a local-history moment, false for a git revision. */
	bool bIsInstant = true;

	/** `bIsInstant`: the moment the project is resolved against. */
	FDateTime Instant;

	/** Otherwise: `HEAD`, or a local branch name. Never a remote-tracking ref — `ListBranches`. */
	FString Ref;

	/** `project at 5 Aug · 14:32`, `branch main`, `last commit`. Goes into every sentence. */
	FString Label;

	static FGcProjectRestoreSide AtInstant(const FDateTime& When, const FString& InLabel)
	{
		FGcProjectRestoreSide Side;
		Side.bIsInstant = true;
		Side.Instant = When;
		Side.Label = InLabel;
		return Side;
	}

	static FGcProjectRestoreSide AtRef(const FString& InRef, const FString& InLabel)
	{
		FGcProjectRestoreSide Side;
		Side.bIsInstant = false;
		Side.Ref = InRef;
		Side.Label = InLabel;
		return Side;
	}
};

struct GRAPHCODE_API FGcProjectRestoreTarget
{
	/**
	 * Where the project ends up: the only version, or — with a base — the EARLIER end of a window.
	 */
	FGcProjectRestoreSide To;

	/**
	 * **THE OTHER END OF A WINDOW, AND WITHOUT IT THIS BUTTON MEANT SOMETHING ELSE THAN THE ONE NEXT
	 * TO IT.** E11.12.
	 *
	 * E11.11 gave the per-file footer `Revert diffs` — *"undo this specific session only, eg, 1pm to
	 * 2pm coding"* — and left `Revert project…` reverting to one version. So the same two moments on
	 * screen meant *undo the window* on one control and *travel to its start* on the other, which is
	 * the kind of difference nobody discovers until it has removed a day's work.
	 *
	 * With a base, every asset is resolved at BOTH ends and merged exactly as `FGcRestore::PlanWindow`
	 * does for one: base is the later side, ours is the live asset, theirs is the earlier side. **The
	 * scope follows from it rather than being declared**: an asset the two ends agree about has
	 * nothing to undo and is never loaded, and an asset created AFTER the window is in neither end and
	 * is therefore left alone — which is the whole difference from reverting to one version.
	 */
	bool bHasBase = false;
	FGcProjectRestoreSide Base;

	/**
	 * `project at 5 Aug · 14:32`, or `undo project at 13:00 -> project at 14:32`.
	 *
	 * ONE SENTENCE-MAKER, because the label appears in the summary, in every skip message, in each
	 * per-asset plan and in the checkpoint's own record — and a window described as its earlier end
	 * alone would read as a revert to that moment in every one of them.
	 */
	FString Describe() const
	{
		return bHasBase
			? FString::Printf(TEXT("undo %s -> %s"), *To.Label, *Base.Label)
			: To.Label;
	}

	static FGcProjectRestoreTarget AtInstant(const FDateTime& When, const FString& InLabel)
	{
		FGcProjectRestoreTarget Target;
		Target.To = FGcProjectRestoreSide::AtInstant(When, InLabel);
		return Target;
	}

	static FGcProjectRestoreTarget AtRef(const FString& InRef, const FString& InLabel)
	{
		FGcProjectRestoreTarget Target;
		Target.To = FGcProjectRestoreSide::AtRef(InRef, InLabel);
		return Target;
	}

	/**
	 * A WINDOW. `To` is where it goes back to — the earlier end — and `Base` is what the work left
	 * behind.
	 *
	 * The two ends may be of different kinds: `last commit -> project at 14:32` is a perfectly good
	 * window, and the resolver treats each side on its own terms.
	 */
	static FGcProjectRestoreTarget Between(
		const FGcProjectRestoreSide& InBase, const FGcProjectRestoreSide& InTo)
	{
		FGcProjectRestoreTarget Target;
		Target.To = InTo;
		Target.bHasBase = true;
		Target.Base = InBase;
		return Target;
	}
};

/** Why an asset in scope was not acted on. Each of these draws differently. */
enum class EGcProjectSkipReason : uint8
{
	/** Not skipped. */
	None,

	/**
	 * Its `.gc` carries a blocking `errors { }` — it says itself that it cannot be built.
	 *
	 * **THE EXPECTED KIND OF SKIP** (user-directed): a module nobody has bought yet, a facet
	 * GraphCode cannot write yet. It is durable, it is in the file, and it is why this list has to
	 * be somewhere a reader can open rather than a log line.
	 */
	Annotated,

	/**
	 * The store cannot honestly say what this asset held at the cutoff — `EGcHistoryKnowledge::
	 * Unknown`.
	 *
	 * **NEVER TREATED AS "IT DID NOT EXIST".** That is the mistake the three-answer knowledge type
	 * exists to make unrepresentable, and at project scale it is the one that would delete somebody's
	 * work to reproduce a past that never happened.
	 */
	Unattested,

	/** The per-asset planner refused: a read-only block, a kind with no factory, a referencer. */
	Refused,
};

/**
 * **THE TWO VERSIONS THIS ASSET'S WRITE IS COMPUTED FROM, KEPT SO IT CAN BE COMPUTED AGAIN.** E11.20.
 *
 * User-directed 2026-08-06: *"it should not reuse the one during planned, because it might need to
 * merge against current file, not just the two timeline"*, and *"it's actually a three way merge,
 * compare the diff between the two files, then apply the diff to the current file."*
 *
 * A plan is made so a person can read it, and then they read it — which takes as long as it takes.
 * The live asset can move in that time: an autosave lands, another tool writes, they alt-tab and fix
 * something. `FGcRestorePlan::DesiredText` is *the live projection with blocks spliced in*, so it is
 * an answer computed against a version of the asset that may no longer exist, and writing it back
 * would silently discard whatever happened in between.
 *
 * So the plan carries its INPUTS rather than only its output, and `Execute` re-runs the merge with
 * `ours` re-projected at the moment of the write. Nothing here is the live side: that is deliberate,
 * because the whole point is that the live side is read late.
 */
struct GRAPHCODE_API FGcProjectRestoreSideText
{
	/** The version's text. Empty AND `!bIsLive` means the asset is absent at this end. */
	FString Text;

	/**
	 * This end IS the asset as it stands, so its text is whatever `ours` projects to when the write
	 * runs. `FResolved::bIsCurrentState` — an instant with nothing archived since equals now.
	 */
	bool bIsLive = false;

	bool IsPresent() const { return bIsLive || !Text.IsEmpty(); }
};

/** One asset's part of a project-wide revert. */
struct GRAPHCODE_API FGcProjectRestoreItem
{
	FString PackageName;

	EGcRestoreAction Action = EGcRestoreAction::NoChange;

	/** Filled for every action that would do something. Empty for `NoChange` and for a skip. */
	FGcRestorePlan Plan;

	/** Where the project goes back to — `theirs`. See `FGcProjectRestoreSideText`. */
	FGcProjectRestoreSideText To;

	/**
	 * The window's later end — `base`. `bHasBase` false means there is no window and the revert is a
	 * straight apply of `To` over whatever the asset holds.
	 */
	bool bHasBase = false;
	FGcProjectRestoreSideText Base;

	EGcProjectSkipReason SkipReason = EGcProjectSkipReason::None;

	/** The sentence a reader acts on. Non-empty exactly when `SkipReason != None`. */
	FString SkipMessage;

	/**
	 * **WHY A CONSIDERED ASSET IS NOT ACTED ON, IN WORDS.** E11.20.
	 *
	 * User-reported 2026-08-06: *"there's 2 file to revert, but why it says confirm revert 1 to
	 * create?"* — the second file was resolved, found to have existed at neither end, and dropped
	 * with no row, no count and no sentence. It was not skipped (nothing refused) and it was not an
	 * action, so every number the panel could show was silent about it.
	 *
	 * A plan that considered an asset now says what it concluded. Empty for the assets a reader would
	 * never ask about — one with no history and no change — and filled for every one the panel is
	 * capable of listing.
	 */
	FString NoChangeReason;

	/** After `Execute`: what happened to this one. */
	bool bPerformed = false;
	FString ResultMessage;

	bool IsSkipped() const { return SkipReason != EGcProjectSkipReason::None; }
	bool IsAction() const
	{
		return !IsSkipped() && Action != EGcRestoreAction::NoChange &&
			Action != EGcRestoreAction::Refused;
	}
};

/**
 * Which of the three passes a progress report is coming from. Each counts from 1 to its own total,
 * so a bar can be drawn per phase rather than over a number nobody can interpret.
 */
enum class EGcProjectRestorePhase : uint8
{
	/** Resolving every covered package against the target. Reading only. */
	Planning,

	/** §9's preflight: every target re-derived and dry-run BEFORE the first mutation. */
	Checking,

	/** The writes. */
	Writing,
};

/**
 * **HOW FAR ALONG, AND MAY IT CARRY ON.** E11.20, user-directed: *"pressing confirm reveals a
 * progress bar showing 1/30, then do it async."*
 *
 * A callback rather than an `FScopedSlowTask` in here, for one reason: this runs headless too, and a
 * backend that opened a dialog would be a backend the automation suite cannot call. The panel owns
 * the window; this owns the count.
 *
 * Returning FALSE stops the run at the next boundary. **During `Checking` nothing has been written
 * yet, so a stop there costs nothing.** During `Writing` everything already done is inside the one
 * transaction and comes back with the same single Ctrl+Z as a finished revert; the report says it
 * was stopped, and by how far.
 */
using FGcProjectRestoreProgress =
	TFunction<bool(EGcProjectRestorePhase Phase, int32 Done, int32 Total, const FString& What)>;

struct GRAPHCODE_API FGcProjectRestoreOptions
{
	/**
	 * Called as the plan walks the covered packages. **Planning is the expensive half** — it reads
	 * every sidecar and every lineage in the project — and it used to run behind a button that gave
	 * no sign it was working.
	 */
	FGcProjectRestoreProgress OnProgress;

	/**
	 * Include assets that exist now and did not at the cutoff. **The destructive half.**
	 *
	 * Default TRUE, because a revert that silently left new assets behind would produce a project
	 * that matches no moment in its own history — but it is a switch, because "put the old versions
	 * back" and "make it exactly as it was" are two different asks and only the reader knows which.
	 */
	bool bIncludeRemovals = true;

	/** Include assets that existed at the cutoff and do not now. */
	bool bIncludeCreations = true;

	/** A referencer outside the removal set stops refusing. §8's breaking-change override. */
	bool bAcceptBreakingReferences = false;
};

struct GRAPHCODE_API FGcProjectRestorePlan
{
	FGcProjectRestoreTarget Target;

	/** `project at 5 Aug · 14:32`, `branch main` — what every sentence calls the target. */
	FString Label;

	/** Ties every checkpoint, every archive and the backup to this one gesture. */
	FString GroupId;

	/** Every GraphCode-covered asset that was considered, including the ones with nothing to do. */
	TArray<FGcProjectRestoreItem> Items;

	/** The backup as it stands right now, and what this revert would do to it. */
	FGcBackupStatus Backup;
	bool bWouldReplaceBackup = false;

	/** Why a fresh backup is needed. Empty when the existing one already undoes this revert. */
	FString BackupReason;

	/**
	 * The sentence the reader must see before pressing. Empty when nothing would be destroyed.
	 *
	 * User-directed: *"warn them if we are going to delete the backup."*
	 */
	FString BackupWarning;

	/**
	 * Carried from the options the plan was made under, because `Execute` re-derives the removals
	 * and a referencer override the reader already granted may not evaporate between the two.
	 */
	bool bAcceptBreakingReferences = false;

	/** Why the whole revert is not on offer. Non-empty means nothing may be executed. */
	FString Refusal;

	bool IsRefused() const { return !Refusal.IsEmpty(); }

	int32 CountOf(EGcRestoreAction Action) const;
	int32 SkippedCount() const;
	TArray<FString> PackagesToBackUp() const;

	/** One line: `12 to restore, 2 to create, 1 to remove, 3 skipped`. */
	FString Summary() const;
};

struct GRAPHCODE_API FGcProjectRestoreExecuteOptions
{
	/** Validate everything and touch nothing — including the backup, which is not taken. */
	bool bDryRun = false;

	/** The reader saw `FGcProjectRestorePlan::BackupWarning`. Required when it is non-empty. */
	bool bAcceptReplacingBackup = false;

	/** The reader saw the removal list. Required when the plan holds any `Remove`. */
	bool bConfirmRemovals = false;

	/**
	 * The reader saw the skipped list. Required when the plan holds any skip.
	 *
	 * **A PARTIAL REVERT IS ALWAYS AVAILABLE — user-directed 2026-08-05:** *"they can always choose
	 * to revert back, matter whether we can apply it back or not... if we cant revert it back, let
	 * them know we cant do for these files, and they will remain untouched."* So this is a record
	 * that the list was SHOWN, not a veto: the panel's preview prints every skipped asset with its
	 * reason and the second press sets this. Nothing about a skip may stop the assets that CAN go
	 * back from going back — an asset GraphCode cannot rebuild is a reason to say so, not a reason
	 * to hold the other three hundred hostage to it.
	 *
	 * What it still buys is §9's real requirement: a revert that quietly did 340 of 343 and reported
	 * success would be telling somebody their project is at Tuesday when three of its assets are not.
	 */
	bool bAcceptPartialScope = false;

	/**
	 * Write the skip reason into each skipped asset's `.gc` as an `errors { }` block.
	 *
	 * **AN EXECUTE OPTION AND NOT A PLAN ONE, AND THE PLACEMENT IS THE RULE.** E10.12: reading must
	 * not write, and a plan is reading. What makes it worth doing at all is that the reason becomes
	 * durable and cheap — the next revert, the next `gc_apply` and the panel all learn it from the
	 * file instead of re-deriving it by loading an asset whose class does not resolve. `sig:` does
	 * not move, so an annotated sidecar is still current for every other gate.
	 */
	bool bAnnotateSkipped = true;

	/**
	 * **RE-DERIVE EACH WRITE AGAINST THE LIVE ASSET IMMEDIATELY BEFORE MAKING IT.** E11.20, and it
	 * defaults ON because the alternative is quietly writing a stale answer.
	 *
	 * `FGcRestorePlan::DesiredText` was computed from a projection taken when the plan was made,
	 * which is before the reader read the confirmation window. This re-runs the same three-way merge
	 * with `ours` re-projected NOW, from the two versions kept on `FGcProjectRestoreItem` — so an
	 * asset that moved while the window was open is merged with rather than overwritten.
	 *
	 * What a re-derive may legitimately change: an `Apply` can become a `NoChange` (somebody already
	 * put it back) or a refusal (it moved to a state the writer will not take). Both are reported per
	 * item; neither aborts the run.
	 *
	 * Turn it off only to execute a plan against a frozen state on purpose — a spec asserting the
	 * exact planned bytes, for instance.
	 */
	bool bRederiveAgainstLive = true;

	/** How far along, and may it carry on. See `FGcProjectRestoreProgress`. */
	FGcProjectRestoreProgress OnProgress;
};

struct GRAPHCODE_API FGcProjectRestoreReport
{
	bool bSuccess = false;

	FString GroupId;

	int32 Restored = 0;
	int32 Created = 0;
	int32 Removed = 0;
	int32 Failed = 0;
	int32 Skipped = 0;

	/**
	 * Assets whose re-derive against the live state came back with nothing to do — somebody had
	 * already put them where the revert was taking them. Not a failure and not a skip.
	 */
	int32 AlreadyThere = 0;

	/** The reader stopped it. What had been done is inside the transaction and undone by one Ctrl+Z. */
	bool bCancelled = false;

	/** Whether a new backup was taken, or the existing one was reused because nothing had changed. */
	bool bBackupTaken = false;
	bool bBackupReused = false;
	FGcBackupStatus Backup;

	/** Sidecars this run put an `errors { }` block on, so the reader can find them. */
	TArray<FString> Annotated;

	/** One line, already fit for a footer. */
	FString Message;
};

class GRAPHCODE_API FGcProjectRestore
{
public:
	/**
	 * Resolve one target into a per-asset action set. **Touches nothing and writes nothing.**
	 *
	 * Loads assets, because deciding whether an asset differs from its old text means projecting
	 * it. That is the cost §9 accepts in exchange for preflighting everything before the first
	 * mutation, and it is why the panel arms this behind a press rather than running it on a
	 * refresh.
	 */
	static bool Plan(
		const FGcProjectRestoreTarget& Target,
		const FGcProjectRestoreOptions& Options,
		FGcProjectRestorePlan& OutPlan,
		FString& OutError);

	/**
	 * Perform a plan under §9's gates: backup, checkpoints, one transaction, verified read-backs.
	 *
	 * ORDER IS CREATE, THEN APPLY, THEN REMOVE, and it is not arbitrary. An apply may reference an
	 * asset the same revert is creating, so the creations go first; a removal's referencer analysis
	 * is only true once every apply that might have dropped a reference has run, so they go last.
	 *
	 * Leaves everything DIRTY AND UNSAVED. `OutPlan` is updated in place with each item's outcome,
	 * so the caller can draw the same list it planned rather than a second one.
	 */
	static bool Execute(
		FGcProjectRestorePlan& Plan,
		const FGcProjectRestoreExecuteOptions& Options,
		FGcProjectRestoreReport& OutReport,
		FString& OutError);

	/**
	 * Every GraphCode-covered package: one with a sidecar in the working tree, or one the local
	 * history store holds a lineage for.
	 *
	 * THE UNION, because neither half is the scope on its own. A sidecar alone misses an asset that
	 * was deleted since the cutoff — the whole `Create` case. A history lineage alone misses every
	 * asset nobody has edited since GraphCode started watching, which is most of them.
	 */
	static TArray<FString> CoveredPackages();
};
