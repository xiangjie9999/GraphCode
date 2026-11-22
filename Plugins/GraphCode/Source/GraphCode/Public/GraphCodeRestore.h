#pragma once

#include "CoreMinimal.h"

#include "GraphCodeCapabilities.h"
#include "GraphCodeGreenfield.h"
#include "GraphCodeRecovery.h"

/**
 * E11.6/E11.7 — PUTTING AN OLDER VERSION BACK INTO THE ASSET.
 *
 * `GC-LOCAL-HISTORY-DESIGN.md` §6 owns the pipeline and §2.5/§8 own what a restore is allowed to
 * call itself. Read those before changing anything here; several shapes below look like ceremony
 * until you know which ruling produced them.
 *
 * WHAT IT IS. One backend for every restore the panel offers, whatever the target is — a local
 * history entry, a git ref, or HEAD. The target only ever arrives here as canonical GraphCode
 * TEXT, so the same planner serves all three and none of them can grow its own semantics.
 *
 * WHAT IT IS NOT: a second mutation engine. Every write goes through `FGraphCodeApply::ApplyDocument`
 * and therefore through the same per-block writers, the same staleness gates, the same
 * reference-impact and compile checks. This layer decides WHAT to write and refuses when the
 * capability registry says the answer would be partial; it never writes a byte itself.
 *
 * THE THREE RULINGS WORTH KNOWING BEFORE READING THE CODE:
 *
 *  1. **An old header is provenance, not an apply base** (§6). The desired document is the LIVE
 *     projection with the target's blocks spliced into it, so its `sig:` is the asset's current one
 *     and the ordinary staleness gate passes for the ordinary reason. Feeding a history entry
 *     straight to `gc_apply` is refused as stale, correctly, and always should be.
 *
 *  2. **The label may never overclaim** (§2.5) — and what that forbids GOT NARROWER, user-directed
 *     2026-08-05: *"yes node position cant be restored, but who cares if we have auto organise."*
 *     E11.7 refused to emit the words `Revert Entire Asset` at all, on §2.2a's measurement that a
 *     `.gc` carries no node positions. The measurement stands and the conclusion drawn from it was
 *     too strong: `BlueprintGraphLayout` can lay a graph out, so positions are REGENERABLE rather
 *     than lost, and the honest consequence is "the layout will be re-organised", not "this cannot
 *     be called a full revert".
 *
 *     What still decides the label is `FGcRestoreBlock::bCompletelyDetermined`: a plan may say
 *     `Revert Entire Asset to <target>` only when every differing block is writable AND its
 *     omission model lets a complete payload settle it. `timelines` (omission preserves) and
 *     `curves` (channel set closed by the class) do not, so a plan touching either says
 *     `Restore N of M changed facet(s)` — which is about what survives the write, not about layout.
 *
 *  3. **A restore with no undo is not a restore** (§2.4). `Execute` checkpoints the LIVE state —
 *     unsaved edits included, because those are exactly what is about to be destroyed and nothing
 *     else anywhere holds a copy of them — and a checkpoint that cannot be written aborts the whole
 *     plan rather than proceeding without one.
 */

/**
 * Which lifecycle action one exact package needs. §2.7.
 *
 * **EXISTENCE IS PART OF THE COMPARISON, so this is not three spellings of `gc_apply`.** `gc_apply`
 * mutates a resolved existing asset; it cannot prove that GraphCode can construct every kind from
 * nothing, and it certainly cannot prove one can be destroyed safely. So the registry advertises
 * `create`, `apply` and `recoverable_remove` independently (`FGcKindCapability::Creation` and
 * `::Removal`), and each action has its own machinery: `FGcGreenfield` for the first,
 * `FGcRecoveryTrash` for the last.
 *
 * **BOTH ARE REAL AS OF E11.7a.** They were named-and-refused for one milestone, and that is worth
 * remembering rather than deleting: a planner that returned "no action" for a target-only asset
 * would have been indistinguishable from one that had considered it and found nothing to do.
 */
enum class EGcRestoreAction : uint8
{
	/** The two sides already agree about everything this format represents. */
	NoChange,

	/** Both sides have the asset and represented state differs. */
	Apply,

	/** The target has it and the project does not. Built by `FGcGreenfield`, proven on a scratch. */
	Create,

	/** The project has it and the target does not. Never a file delete — `FGcRecoveryTrash`. */
	Remove,

	/** Nothing can be offered, and `FGcRestorePlan::Refusal` says why in one sentence. */
	Refused,
};

/**
 * One line of the confirmation window a restore or a revert shows before it writes. E11.19.
 *
 * `Detail` is the sentence the planner already produced — a refusal, a caveat, the `errors { }`
 * summary a skipped document wrote about itself — carried verbatim rather than rewritten, for the
 * same reason `FGcGreenfield`'s refusals are carried verbatim: whoever wrote the diagnosis knew
 * more than the layer that displays it.
 */
struct GRAPHCODE_API FGcConfirmItem
{
	FString Title;
	FString Detail;
};

/** One block the restore looked at, and what the capability registry said about writing it. */
struct GRAPHCODE_API FGcRestoreBlock
{
	FString Name;

	/** The block's body differs between the live asset and the target. */
	bool bDiffers = false;

	/** This block is in the desired document, so the apply will write it. */
	bool bIncluded = false;

	/**
	 * A complete payload for this block fully determines the result — nothing live survives by
	 * omission.
	 *
	 * TRUE for `Deletes`, `Refuses` (under snapshot), `Truncates` and `ResetsToDefault`; FALSE for
	 * `Preserves` and `Fixed`, and that pair is the whole reason this field exists. A timeline the
	 * target never had is left in place by `timelines`' own identity model, and a curve channel the
	 * target lacks cannot be dropped because a curve's channel set is closed by its class. Both are
	 * correct writer behaviour and both make the restore PARTIAL, which the label has to say.
	 */
	bool bCompletelyDetermined = false;

	/** Empty unless the block was left out. Names the reason in the words a reader can act on. */
	FString Refusal;
};

/**
 * A restore, decided and not yet performed.
 *
 * PLANNING AND PERFORMING ARE SEPARATE CALLS because the plan is what the panel DRAWS. A button
 * whose label is computed by one function and whose effect is computed by another is the defect
 * E10.12 removed from this panel once already; here the label, the caveats and the desired document
 * are one object, and `Execute` takes it rather than recomputing anything.
 */
struct GRAPHCODE_API FGcRestorePlan
{
	EGcRestoreAction Action = EGcRestoreAction::Refused;

	FString PackageName;

	/** What the target is called on screen — `10:42:07`, `branch main`. Goes into every sentence. */
	FString TargetLabel;

	/**
	 * Empty for a whole-asset restore, and empty for a plan over SEVERAL blocks; otherwise the one
	 * block this plan is scoped to. It names the transaction, which wants a name and not a list.
	 */
	FString BlockName;

	/** The plan's actual scope: empty for every represented facet, otherwise the blocks asked for. */
	TArray<FString> BlockNames;

	/** Every block in the live projection, with what was decided about it. */
	TArray<FGcRestoreBlock> Blocks;

	/**
	 * The live projection with the included blocks replaced by the target's. `sig:` is the LIVE
	 * one — see ruling 1 in this file's header.
	 */
	FString DesiredText;

	/** The live text the plan was computed against, so `Execute` can detect the asset moving under it. */
	FString BaseSignature;

	/**
	 * Ties this restore's checkpoint to its siblings. Empty means `Execute` mints one.
	 *
	 * SET BY A PROJECT-WIDE REVERT (§9) so that eleven checkpoints written in one gesture read back
	 * as eleven parts of one thing rather than eleven unrelated reverts somebody happened to do in
	 * the same minute.
	 */
	FString GroupId;

	/**
	 * The exact words for the control that performs this. Never `Revert Entire File` — ruling 2.
	 */
	FString Label;

	/**
	 * Everything true about this restore that the label cannot carry: facets the target never held,
	 * blocks left behind, entries that will survive by omission.
	 *
	 * ON A SUCCESSFUL PLAN AS WELL AS A REFUSED ONE. A caveat only shown when something fails is a
	 * caveat nobody reads at the moment it matters.
	 */
	TArray<FString> Caveats;

	/** Why nothing is on offer. Non-empty exactly when `Action == Refused`. */
	FString Refusal;

	/**
	 * `Create` only: the proven greenfield plan. Empty `PackageName` when the action is not Create.
	 *
	 * **CARRIED ON THE PLAN RATHER THAN RECOMPUTED IN `Execute`**, because it holds the scratch
	 * PROOF. Recomputing it would mean building another scratch asset between the preview somebody
	 * read and the button they pressed — and a proof taken after the decision is not the proof the
	 * decision was made on.
	 */
	FGcGreenfieldPlan Creation;

	/** `Remove` only: the resolved trash destination, referencers and refusal. */
	FGcTrashPlan Removal;

	/**
	 * `Remove` only, and it is the difference between a warning and a decision.
	 *
	 * §6 step 4 requires explicit confirmation for a removal. `Plan` never sets this; a caller sets
	 * it after showing the reader `Removal.Referencers` and the recovery destination, and `Execute`
	 * refuses without it. A flag `Plan` could set would be consent the planner gave on the reader's
	 * behalf.
	 */
	bool bRemovalConfirmed = false;

	/**
	 * **HOW MANY LINES OF THE `.gc` THIS WOULD MOVE — the same two numbers the file list draws.**
	 * E11.20, user-directed: *"pressing it open a popup window showing all potential file and line
	 * changes"*.
	 *
	 * Measured at plan time, between the LIVE projection and `DesiredText`, because that is the one
	 * moment both are in hand — recomputing it for a dialog would mean re-projecting every asset in
	 * the plan. A `Create` is all additions and a `Remove` is all removals; neither has two sides to
	 * diff, so each counts the one text it has.
	 *
	 * **NOT A PROMISE ABOUT THE ASSET.** These count text, and text is what the reader is being shown;
	 * what a writer does with a line is `Blocks` and `Caveats`.
	 */
	int32 LinesAdded = 0;
	int32 LinesRemoved = 0;

	/** `+12 −3`, or `nothing`. One spelling, so the dialog and the footer cannot disagree. */
	FString LineSummary() const;

	/** How many blocks the apply would write. */
	int32 IncludedCount() const
	{
		int32 Count = 0;
		for (const FGcRestoreBlock& Block : Blocks) { Count += Block.bIncluded ? 1 : 0; }
		return Count;
	}

	/** How many blocks differ, whether or not they can be written. */
	int32 DifferingCount() const
	{
		int32 Count = 0;
		for (const FGcRestoreBlock& Block : Blocks) { Count += Block.bDiffers ? 1 : 0; }
		return Count;
	}

	/** Every differing block is included AND completely determined by its payload. */
	bool CoversEveryDifference() const;
};

/** What actually happened. */
struct GRAPHCODE_API FGcRestoreResult
{
	bool bSuccess = false;

	/** The pre-revert checkpoint's immutable id, so a person can be told what to undo to. */
	FString CheckpointEntryId;

	/** Ties the checkpoint to this restore, and to its siblings when a group restore lands. */
	FString GroupId;

	int32 BlocksWritten = 0;

	/** Named by the writers, rolled up: what the payload removed by omission. */
	TArray<FString> Deleted;

	/** Blocks that could not honour snapshot mode, in the writers' own words. */
	TArray<FString> ModeNotices;

	/** One line, already fit to put in a footer. */
	FString Message;

	/** The asset is still dirty after this restore, which as of C24.1 means a save was refused. */
	bool bAssetDirty = false;

	/**
	 * A save ran. C24.1 — §11.1's invariant, REPORTED here rather than newly performed.
	 *
	 * This route has saved since C24.0 without anybody noticing: it writes through `ApplyDocument`,
	 * whose `bSave` defaults true, and creates through `FGcGreenfield::Create`, which saves too. So
	 * the behaviour was already right and the SURFACE could not say so — which is its own defect,
	 * because "the asset is dirty and unsaved" is what this result's own field name still implied.
	 */
	bool bSaved = false;

	/** Why `bSaved` is false, in the caller's words. Empty exactly when `bSaved` is true. */
	FString SaveSkipped;

	/** What actually happened, so a caller does not have to infer it from the plan it passed in. */
	EGcRestoreAction Action = EGcRestoreAction::NoChange;

	/** `Remove` only: where the asset and its sidecar went. */
	FGcTrashRecord Recovery;
};

class GRAPHCODE_API FGcRestore
{
public:
	/**
	 * Decide what restoring `TargetText` into `PackageName` would mean. Touches nothing.
	 *
	 * @param BlockName  one block, or empty for every represented facet.
	 *
	 * Refuses before it plans when the target is not a valid restore point: unparsable, conflicted,
	 * an unabsorbed merge proposal, a projection of a DIFFERENT asset, or a different kind. Each of
	 * those is a document that describes a version that never existed as this asset, and offering
	 * one as "the state it was in" is the one lie a recovery feature may not tell.
	 */
	static bool Plan(
		const FString& PackageName,
		const FString& TargetText,
		const FString& TargetLabel,
		const FString& BlockName,
		FGcRestorePlan& OutPlan,
		FString& OutError);

	/**
	 * The same, scoped to a SET of blocks. Empty means every represented facet, exactly as an empty
	 * `BlockName` does — the two spellings of "all" are one code path.
	 *
	 * **E11.11's `Revert diffs` IS THIS, AND IT IS WHY A LOOP WOULD HAVE BEEN WRONG.** *"it should
	 * only apply all before blocks that changed … undo the specific session, not up until now"*: the
	 * scope is the blocks the two chosen versions disagree about, which is a set with no single name.
	 * Planning them one at a time would project the live asset once per block — the same per-item cost
	 * E11.7b measured and removed — and would produce N plans where the reader is consenting to one.
	 */
	static bool PlanBlocks(
		const FString& PackageName,
		const FString& TargetText,
		const FString& TargetLabel,
		const TArray<FString>& BlockNames,
		FGcRestorePlan& OutPlan,
		FString& OutError);

	/**
	 * **UNDO ONE WINDOW OF WORK ON ONE ASSET.** E11.12, extracted from `BuildSessionRevertPlan`.
	 *
	 * *"i want to undo this specific session only, eg, 1pm to 2pm coding … like apply back (including
	 * deleting lines), the previous version, like a 2 times 3 way merge."*
	 *
	 * **IT IS `git revert`, AND THE ASSIGNMENT OF THE THREE SIDES IS THE WHOLE THING:** the base is
	 * the state at the END of the window, theirs is the state at its BEGINNING, ours is the asset as
	 * it is now. What the window did is `base -> theirs` read backwards, and applying that to whatever
	 * the asset has become keeps everything done since. Writing the earlier side's blocks instead —
	 * which is what this replaced — rolls a block changed at 1pm and again at 4pm all the way back to
	 * 1pm, so undoing the afternoon's first hour also undoes its last.
	 *
	 * It gets DELETION right for free, which no block copy can: an entity the window ADDED is in the
	 * base and not in theirs, so the merge removes it and `Execute` applies in snapshot mode where an
	 * omitted entry is a removal.
	 *
	 * **A CONFLICTED ENTITY IS LEFT AT OURS AND NAMED, NOT A REFUSAL OF THE WHOLE UNDO.** A conflict
	 * means one thing changed inside the window AND again after it, so no version of it both undoes
	 * the window and keeps the later work. That is a fact about ONE entity; refusing the other forty
	 * because of it would be the coarse answer. `Pick(Ours)` removes the construct and touches nothing
	 * else, so the later work is kept by construction, and every name goes into `OutPlan.Caveats` and
	 * into `OutLeftAlone`.
	 *
	 * **ONE IMPLEMENTATION, TWO CALLERS**, which is why it lives here rather than in the panel: the
	 * footer's `Revert diffs` does this to the selected asset and `Revert project diffs` does it to
	 * every asset the window touched. Two copies of the base/theirs assignment is exactly the pair
	 * this codebase keeps finding disagreeing.
	 *
	 * @param OutLeftAlone  entity (or `entity.property`) names the merge could not settle.
	 */
	static bool PlanWindow(
		const FString& PackageName,
		const FString& BaseText,
		const FString& OursText,
		const FString& TheirsText,
		const FString& WindowLabel,
		FGcRestorePlan& OutPlan,
		TArray<FString>& OutLeftAlone,
		FString& OutError);

	/**
	 * The `Remove` half: the target does not have this asset and the project does.
	 *
	 * A SEPARATE ENTRY POINT because there is no target TEXT to pass — absence is the target, and
	 * threading an empty string through `Plan` would make "the target is empty" and "the target
	 * does not exist" the same argument. They are opposite: the first is a corrupt entry and the
	 * second is a deletion.
	 *
	 * @param AlsoLeaving  every other package this same revert is removing. Excluded from the
	 *                     referencer analysis, because three assets that reference each other and
	 *                     leave together have no surviving referencer between them.
	 */
	static bool PlanRemoval(
		const FString& PackageName,
		const FString& TargetLabel,
		const TSet<FString>& AlsoLeaving,
		bool bAcceptBreakingReferences,
		FGcRestorePlan& OutPlan,
		FString& OutError);

	/**
	 * Perform a plan. **Checkpoints first, and refuses if the checkpoint cannot be written.**
	 *
	 * The order is the whole safety argument (§6 steps 5 and 6): project the live state, archive it,
	 * verify it landed, and only then open the transaction. A dry run skips the checkpoint because
	 * it changes nothing — and it is the same preflight the real run does first, through
	 * `ApplyDocument`, so a plan that cannot apply says so before anything is archived.
	 *
	 * Leaves the asset DIRTY AND UNSAVED. That is not laziness: an unsaved apply is one Ctrl+Z from
	 * never having happened, and saving on the user's behalf would spend the undo they might want.
	 */
	static bool Execute(
		const FGcRestorePlan& Plan,
		bool bDryRun,
		FGcRestoreResult& OutResult,
		FString& OutError);


	/**
	 * Which top-level block a line of the target document belongs to, or empty for the envelope.
	 *
	 * The panel's surgical restore needs it: a person clicks a row of a diff and means *this*, and
	 * the smallest thing GraphCode can safely put back is the block that owns it (§7 — an entity
	 * that cannot be isolated without replacing its owner gets the owner). Brace depth over the
	 * canonical text, which is the same walk `FGraphCodeSidecarReader` does; a line inside a nested
	 * `function Foo { }` answers `functions`, because that is the block a writer takes.
	 */
	static FString BlockOwningLine(const FString& Text, int32 OneBasedLine);

	/**
	 * The same answer for EVERY line at once: index `n` holds the block owning one-based line `n+1`.
	 *
	 * **ONE PARSE FOR A WHOLE DOCUMENT, WHICH IS WHY IT EXISTS.** `BlockOwningLine` parses the file to
	 * answer about one line — right for a click, ruinous for a redraw, and E11.11 needs the answer on
	 * every row of a comparison so the two columns can be coloured by whether each side's block is
	 * what the asset holds. Asking the single-line form four hundred times would be four hundred
	 * parses of the same text.
	 *
	 * Same spans, same rule: `FGraphCodeBlock::FirstLine`/`LastLine` out of the reader, so a line
	 * inside a nested `function Foo { }` answers `functions` here exactly as it does there.
	 *
	 * @return false when the text does not parse, leaving `OutBlocks` empty — the ordinary state of a
	 *         side a branch has never held.
	 */
	static bool BlockPerLine(const FString& Text, TArray<FString>& OutBlocks);

	/**
	 * The same question one level down: which ENTITY each line belongs to, as `Frame/RootStack/Root`.
	 *
	 * **A GROUP IS A DECLARING LINE PLUS A BRACED BODY**, so an entity and its subtree is a writable
	 * unit exactly the way a top-level block is — and it is a much narrower one. Empty for a line
	 * outside any tree block: the envelope, `graph { }`, and the blocks whose entries are not a tree.
	 * Which blocks those are is `FGcDocumentMerge::TierOf`, so there is one table rather than two.
	 */
	static bool EntityPerLine(const FString& Text, TArray<FString>& OutPaths);

	/**
	 * The live document with ONE entity taken from `SideText` — replaced, inserted, or removed when
	 * that side does not have it.
	 *
	 * The result is a whole document whose named block differs from the asset in that entity alone,
	 * which is what lets `PlanEntity` write it through `PlanBlocks` with no new writer and no new
	 * deletion rule.
	 */
	static bool SpliceEntity(
		const FString& LiveText,
		const FString& SideText,
		const FString& BlockName,
		const FString& EntityPath,
		FString& OutText,
		FString& OutError);

	/** `PlanBlocks` aimed at one entity: splice, then plan the block that now holds it. */
	static bool PlanEntity(
		const FString& PackageName,
		const FString& TargetText,
		const FString& TargetLabel,
		const FString& BlockName,
		const FString& EntityPath,
		FGcRestorePlan& OutPlan,
		FString& OutError);

private:
	/**
	 * `Create`'s half of `Execute`. Records the creation transition BEFORE building (§6 step 5).
	 *
	 * A creation has no predecessor text, so what is archived is a `Created` tombstone — the
	 * statement that the package did not exist before this. Without it a later cutoff planner
	 * cannot tell "unchanged since then" from "did not exist then", which is the distinction
	 * `EGcHistoryKnowledge` exists to keep.
	 */
	static bool ExecuteCreate(
		const FGcRestorePlan& Plan, bool bDryRun, FGcRestoreResult& OutResult, FString& OutError);

	/** `Remove`'s half. Checkpoints the LIVE text first, because the move is what destroys it. */
	static bool ExecuteRemove(
		const FGcRestorePlan& Plan, bool bDryRun, FGcRestoreResult& OutResult, FString& OutError);
};
