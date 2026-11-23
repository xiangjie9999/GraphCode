#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GraphCodeHistory.h"
#include "GraphCodeProjectRestore.h"
#include "GraphCodeRestore.h"
#include "GraphCodeVcsLibrary.h"
#include "GraphCodeVcsStyle.h"
#include "Styling/SlateBrush.h"

#include "GraphCodeVcsWidget.generated.h"

class UBorder;
class UButton;
class UComboBoxString;
class UHorizontalBox;
class UListView;
class UScrollBox;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UGraphCodeVcsWidget;

/**
 * WHAT THE PANEL IS SHOWING — and it is TWO ZONES now, not one list of four.
 *
 * E10.10, user-directed: *"have conflict above the comparing block, because it's its own zone"*.
 * Deciding a merge and comparing two versions were entries in one picker, and they are not the same
 * kind of thing. A decision is work the repository is BLOCKED on, it exists or it does not, and it
 * is answered by taking a side. A comparison is a question anybody can ask at any time about any
 * pair of versions, and it is answered by reading. Putting them in one control meant the entry that
 * matters — "two files are stuck" — was one of four lines in a dropdown that had to be opened to be
 * read, and it read as a view you might choose rather than as work you have.
 *
 * So the decisions live in their own zone above, present only when there ARE any, and the compare
 * zone below is a pair of source pickers. `Head` and `Live` are gone as modes because they were
 * never modes: they were two of the source pairs this now spells out, `branch main ↔ local` and
 * `local ↔ editor`.
 */
UENUM(BlueprintType)
enum class EGcCompareMode : uint8
{
	/** The unresolved decisions in the working file: each side's value, side by side. */
	Conflicts,

	/** What the merge driver would produce, against what is in the working tree now. */
	Merge,

	/** Any two sources, named by `FGcCompareSource`. The whole compare zone is this one mode. */
	Compare,
};

/**
 * What the panel's one action button does for the selected file. E10.12.
 *
 * ONE FUNCTION DECIDES THIS AND TWO CALLERS ASK IT — the label in `RefreshActions`, the action in
 * `OnPrimaryClicked`. They were a hand-kept mirror ("same order as RefreshActions chose the label
 * in, because the button does what it says"), and a mirror is a comment asking two switch statements
 * to stay in step; the failure it invites is a button that does something other than what it says,
 * which is the worst thing a control can do and the hardest to notice in review.
 *
 * There is deliberately no `Preflight`. A dry run whose only purpose is to arm a second press of the
 * same button is a confirm step under another word — see `ApplyProposalForSelection`.
 */
enum class EGcPrimaryAction : uint8
{
	/** Nothing to offer: no selection, the decisions are already on screen, or the file is settled. */
	None,

	/**
	 * The file has undecided conflicts and the reader is not looking at them. E10.15.
	 *
	 * **THE STATE THAT HAD NO WAY OUT.** Reported 2026-08-05 with a screenshot: *"looks like there's
	 * conflict but i cant even see it"*. A conflicted file selected from the COMPARE list draws a
	 * comparison, and a comparison against a mid-merge file shows the raw `conflict { }` construct
	 * as added lines — `kind:`/`entity:`/`base:`/`ours:`/`theirs:` grammar, which this panel already
	 * ruled *"nine lines nobody can act on"* when it stopped drawing it in the merge view. There is
	 * no take button on a comparison, so the decisions were visible as text and unreachable as
	 * decisions, and nothing on screen said where to go.
	 *
	 * `None` was correct for the resolve view — every row carries its own pair of take buttons, so a
	 * footer action would be a third copy — and it was wrong everywhere else, because "the panel has
	 * nothing to offer" and "you are standing in the wrong room" are not the same answer.
	 *
	 * **TWO DESTINATIONS SINCE 2026-08-05, AND THAT IS WHAT KILLED THE SECOND MERGE BUTTON.** The
	 * same sentence is true of a file git is mid-merge on: its decisions live on the MERGE screen,
	 * and a reader who picked it out of the compare list is in the wrong room in exactly the way this
	 * action exists to fix. Which room is `ModesFor`'s answer — `Merge` if it offers one, the resolve
	 * view otherwise — so there is still one place that decides and no new enumerator to keep in step.
	 */
	GoToDecisions,

	/** A resolution is held in memory and has not reached the file. Unreachable from the UI. */
	SaveResolution,

	/**
	 * Git holds both sides and the merge combines them WITHOUT A SINGLE DISAGREEMENT.
	 *
	 * **IT WAS OFFERED WITH DECISIONS ON SCREEN TOO, AND THAT WAS THE SECOND MERGE BUTTON** —
	 * *"why is there another merge button"*, user-reported 2026-08-05 over a merge screen already
	 * showing two amber decisions, a pair of take buttons on each of them, and `Take all 2 from the
	 * left` / `Take all 2 from the right` under the columns. The footer said `Start merging — 2 to
	 * decide` beneath all of it.
	 *
	 * The button was describing a distinction only the panel has a use for. Under `merge=binary` the
	 * working file is ours verbatim, so the decisions on screen come from the driver's output and are
	 * not in the file yet — true, and irrelevant to the reader, because **a take from that screen
	 * already writes the whole merge**: `TakeSide` folds the write in and says so out loud ("The merge
	 * is in the file now, with that side taken"). So the footer was a second door into a room the
	 * reader was standing in, named after the panel's bookkeeping.
	 *
	 * With nothing to decide there are no take buttons and no other door, so the action survives
	 * exactly there — and only there.
	 */
	WriteMerge,

	/**
	 * The `.gc` holds text its asset has not taken in, and no gesture in this panel put it there.
	 *
	 * **WHY IT IS ASKED AT ALL** — *"i thought all assets are auto updated when apply? why do we even
	 * need to ask"*, user-reported 2026-08-05. It IS automatic wherever a gesture exists: taking the
	 * last side of a merge applies the result itself (E10.7 — "deciding the last conflict IS asking
	 * for the merge"), and a clean merge applies itself the moment it is written.
	 *
	 * This is the case with no gesture attached to it — a `.gc` that arrived ahead of its asset from
	 * `gc_resolve`, from the merge driver in a shell, or from a hand edit. The only moment the panel
	 * could apply one automatically is when somebody LOOKS at it, and E10.12 is the standing ruling
	 * that looking at a file may not change it: *"suddenly, navigating the items changed the
	 * conflict"*. So the press is not a permission check, it is the missing gesture.
	 */
	ApplyProposal,

	/**
	 * The comparison is against a version this panel can put back — a local history instant or a
	 * git ref — and the two sides differ. E11.6/E11.7.
	 *
	 * **DECIDED FROM CHEAP FACTS ONLY, AND THAT IS LOAD-BEARING.** Whether a restore is POSSIBLE is
	 * a question about the compare source and the selection; whether it would be COMPLETE needs the
	 * live projection, which loads the asset. This enum is read on every redraw, so planning here
	 * would make hovering a file list load assets. The plan happens on the first press, which is
	 * also where its caveats can be read — see `FGcRestorePlan`.
	 */
	Restore,
};

/**
 * One end of a comparison. E10.10.
 *
 * THREE KINDS, AND THE THIRD IS THE ONE NO GIT CLIENT HAS. A branch and the working file are what a
 * version-control panel is expected to hold; the asset **in this editor, unsaved**, is a version
 * that exists nowhere on disk and is invisible to every other tool — and between a save and the next
 * one it is the only copy of some of the work.
 */
UENUM(BlueprintType)
/**
 * Whether a row's asset exists at each end of the comparison on screen.
 *
 * **A FACT ABOUT THE WINDOW, NOT ABOUT NOW.** `Deleted` means the two versions bracket its removal;
 * whether it is on disk at this moment is `EGcSidecarState::NotInProject`, which strikes the name
 * through. A file deleted inside the window is both, and that is the point: two marks, two
 * questions, and neither one answers the other.
 */
enum class EGcRowExistence : uint8
{
	Unchanged,
	Created,
	Deleted,
};

enum class EGcSourceKind : uint8
{
	/** The `.gc` in the working tree: what a take writes and what `gc_apply` reads. */
	Working,

	/** The asset as this editor holds it, unsaved edits included. Projected on demand, never a file. */
	Editor,

	/** A git ref — a local branch. Read with `git show <ref>:<path>`. */
	Ref,

	/**
	 * One instant in this asset's LOCAL HISTORY — the fourth kind, and the one git structurally
	 * cannot have. E11.4.
	 *
	 * A `.uasset` is binary and a `.gc` is a projection of it, so the moment an asset is saved the
	 * previous text is gone from everywhere except a commit that may never have been made. Git
	 * covers what you committed; this covers the saves in between, and `FGraphCodeHistory` is the
	 * store.
	 *
	 * **THE SOURCE IS AN INSTANT, NOT AN ENTRY — E11.8 changed this and it is the whole time
	 * machine.** `Instant` carries the moment; each asset's entry is DERIVED from it, per file, by
	 * `HistoryEntryAt`. E11.4 put the entry id in `Ref` because history is per asset, which made the
	 * source itself per asset: the comparison meant something different for every row, and the panel
	 * could not answer *"what did the project look like on Tuesday"* at all. One instant means the
	 * same thing for every asset, and §3.2 says exactly which entry each of them resolves to.
	 */
	History,
};

/**
 * WHICH KIND OF THING THE COMPARISON IS AGAINST — the first of the two compare pickers. E11.8.
 *
 * **USER-DIRECTED 2026-08-05, AND IT REVERSES A CALL I GOT WRONG THE SAME DAY.** E11.5 shipped one
 * flat picker holding every source at once, arguing that a category level puts a branch two clicks
 * away. The reply: *"the picker decision should be remained, since they are totally different
 * things, and the branches are too many."* Both halves are right and the second is a measurement —
 * **this repository has 24 local branches**, so the flat list was a scroll rather than a picker, and
 * my own probe log printed `24 source(s) offered` while I argued from taste. `GC-LOCAL-HISTORY-
 * DESIGN.md` §5.1 is the ruling.
 *
 * **A CATEGORY IS PANEL STATE, NOT A FIFTH `EGcSourceKind`.** Every entry here resolves to a source
 * that already existed: `Head` is `Ref` with `Ref = "HEAD"`, which `git show HEAD:<path>` has always
 * answered. Adding a kind would mean a second spelling of the same comparison, and the two would
 * eventually disagree about which one a restore can put back.
 *
 * IT IS ALSO WHERE THE REFRESH GOT CHEAPER, which is why this is more than taste. `git for-each-ref`
 * is ~100 ms and used to run on EVERY refresh inside `RefreshSources()`, to fill a list nobody was
 * looking at. Branches are needed only under `Branch` now, so the default refresh does not spawn it
 * at all — and under `Head` the compare scope needs no `git diff` either, because the scan has
 * already asked git that exact question for the row counts. See `RefreshCompareScope`.
 */
UENUM(BlueprintType)
enum class EGcCompareCategory : uint8
{
	/**
	 * Git: **the last commit and every local branch**, in one category with two kinds of target.
	 *
	 * **E11.11 FOLDED `Head` IN HERE, USER-DIRECTED** — *"the options are git branches and timeline
	 * and current, with current commit be at git branch"*. `Head` was never a different KIND of
	 * thing: it has always been `Ref` with `Ref = "HEAD"`, which is why it needed no case of its own
	 * in `ProjectRevertTarget`. Three categories with two sides each is six controls; five would
	 * have been seven, and one of them would have been a category with exactly one member.
	 *
	 * **THE CHEAP PATH SURVIVES AND IS NO LONGER KEYED ON THE CATEGORY.** `last commit` against the
	 * working file is the one comparison the scan has already asked git about (`git diff HEAD
	 * --numstat` fills every row's `+N −N`), so `ShowsInView` may read that answer instead of
	 * spawning a second process. That test is now `IsWorkingAgainstHead()`, which asks about the
	 * two SOURCES — because with a free left and right, `Branch` no longer implies `HEAD` and
	 * `HEAD` no longer implies the other side is the working tree.
	 */
	Branch,

	/** Local history. The target is the time machine — a date and time, not a list. */
	History,

	/**
	 * What there is **now**: the saved `.gc` on disk, and the asset as this editor holds it.
	 *
	 * TWO TARGETS, AND THEY ARE DIFFERENT VERSIONS RATHER THAN TWO NAMES FOR ONE. `local` is the
	 * text beside the asset — what a merge writes and what `gc_apply` reads. `editor (unsaved)` is
	 * the object in this process, which between a save and the next one is the only copy of some of
	 * the work and is invisible to every other tool. The pair is the one comparison no git client
	 * can make, and E11.11's picker is the first shape in which both are reachable on either side.
	 */
	Current,
};

/**
 * A source, and exactly what the picker entry and the column caption call it.
 *
 * ONE LABEL FIELD RATHER THAN A FORMATTER. The picker entry, the column caption and the empty-state
 * sentence all name this source, and the moment two of them compute the name separately they can
 * disagree — which on a comparison screen means the reader is told they are looking at something
 * they are not.
 */
struct FGcCompareSource
{
	EGcSourceKind Kind = EGcSourceKind::Working;

	/** The branch name, or `HEAD`, for `Ref`. Empty otherwise. */
	FString Ref;

	/**
	 * WHICH MOMENT, for `History` — and it is the whole asset-independence of the time machine.
	 *
	 * E11.8 replaced the per-asset instant dropdown with one project-wide instant, user-directed:
	 * *"global time machine should be a time picker"*. So a history source no longer names an ENTRY,
	 * which belonged to one package and meant nothing for the next row; it names a POINT IN TIME,
	 * which means the same thing for every asset in the project. The entry each asset resolves to is
	 * derived — its earliest surviving entry at or after this instant, which is the state it held
	 * then (§3.2) — and derived per file, at read time, by `HistoryEntryAt`.
	 *
	 * `Ref` IS UNUSED FOR `History` NOW. Two ways to name one source is how the panel would end up
	 * comparing against an entry the picker is not pointing at.
	 */
	FDateTime Instant;

	/** `local`, `editor`, `branch main (current)`, `last commit`, `project at 5 Aug 14:32`. */
	FString Label;

	/**
	 * A `Ref` that names ONE COMMIT rather than a moving branch. E11.26, display only.
	 *
	 * It changes two things and neither is identity: which end of the label is elided (a branch is
	 * recognised by its tail, a commit by its head) and whether the branch box may offer it. The
	 * alternative was testing whether `Ref` looks like a sha, which is a lookup that is correct only
	 * because of a fact about the data — the same rule that keeps both pickers reading by index.
	 *
	 * **A BRANCH'S TIP IS NOT ONE.** Selecting the newest entry in the commit row keeps the branch's
	 * own ref, so the comparison is byte-identical to picking the branch and `IsWorkingAgainstHead`
	 * still finds `HEAD` where it expects it.
	 */
	bool bCommit = false;

	bool operator==(const FGcCompareSource& Other) const
	{
		if (Kind != Other.Kind)
		{
			return false;
		}
		return Kind == EGcSourceKind::History
			? Instant == Other.Instant
			: Ref.Equals(Other.Ref, ESearchCase::CaseSensitive);
	}
};

/**
 * One row's data. `UListView` takes UObjects, so a row is one of these rather than a struct.
 *
 * It carries the whole status rather than the three strings the row draws, because the details strip
 * reads the SAME object when the selection changes — two paths to one row's facts is how a list and a
 * detail pane start disagreeing.
 */
UCLASS()
class GRAPHCODE_API UGraphCodeVcsRowItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	FGcSidecarStatus Status;

	/** A group heading rather than a file. The list holds both so headings scroll with their rows. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	bool bIsHeading = false;

	/**
	 * WHICH LIST THIS ROW IS IN, because the two lists are about different things.
	 *
	 * E10.11, user-directed: *"compare shouldnt show conflict info and conflict shouldnt care about
	 * +13 or -12"*. The same file can legitimately be in both lists at once, and drawing every fact
	 * about it in both places is how a row ends up carrying three numbers of which two are noise for
	 * wherever you are reading it. A conflict row shows the decision count and no line counts; a
	 * compare row shows `+N`/`−N` and no decision count.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	bool bDecisionRow = false;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	FString HeadingText;

	/** Who to tell when the row is double-clicked. Weak: the panel owns every item it makes. */
	TWeakObjectPtr<UGraphCodeVcsWidget> Owner;
};

/**
 * The entry widget for one file row. Its layout is `WBP_GraphCodeVcsRow`.
 *
 * THREE COLUMNS AND A COLOURED EDGE, AND THE MIDDLE COLUMN IS A WORD. The brief's rule is icon + word
 * + colour, never colour alone: a red edge tells a colour-blind reader nothing, and a status column
 * that reads "conflicted" is legible in a screenshot, a log and a bug report.
 */
UCLASS(Blueprintable)
class GRAPHCODE_API UGraphCodeVcsRow : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** The word a sidecar state gets, everywhere. Its colour lives in `FGcVcsStyle::StateColor`. */
	static FString StateWord(EGcSidecarState State);

	/**
	 * Fill this row in. **Public on this class where the engine keeps it protected**, and the reason
	 * is the same one the panel's own header gives for being constructible bare: a spec has to be
	 * able to ask what a row DRAWS.
	 *
	 * There is no public way in from outside — `IUserObjectListEntry::SetListItemObject` is private
	 * with `SObjectTableRow` as its only friend — so the alternatives were a test-only method beside
	 * this one, which is a second path into the same code, or standing up a real `UListView` to
	 * assert a colour. Selection has a public engine entry point (`IUserListEntry::
	 * UpdateItemSelection`) and is left where it is.
	 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

protected:

	/**
	 * Selection drawn by the LIST rather than by a flag on the item.
	 *
	 * The list already knows what is selected and already tells its rows; a `bSelected` on the item
	 * would be a second copy of that answer, kept in step by hand, wrong exactly when a refresh
	 * rebuilds the items and the list's own selection survives.
	 */
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;

	/** The whole row's background: hover, selection, and nothing else. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> RowFill;

	/**
	 * A two-pixel bar in the state's colour.
	 *
	 * THE ONLY PER-ROW STATE MARK NOW. There used to be a word beside it, and against a list that is
	 * already GROUPED BY STATE that word is the group heading repeated on every row under it — three
	 * rows reading "current" below a heading that already said so, spending the width the path needed.
	 * The brief forbids colour ALONE; the heading one scroll-locked line above IS the word, so the
	 * pair still satisfies it.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> StateEdge;

	/**
	 * The asset's name, and its COLOUR is the git fact.
	 *
	 * Green means the file is new — git has never committed it — and red means it is gone. Anything
	 * else reads in the ordinary ink. That is the brief's §1 rule (the git layer stays visible and
	 * separate from GraphCode's) drawn rather than spelled: it used to be a column of words reading
	 * `both changed it` / `changed` / `new` beside every row, and a word that appears on every row is
	 * a column of noise the path needed the width for.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> PathText;

	/**
	 * `+12` and `−3`, in the diff's own two colours.
	 *
	 * HOW MUCH, NOT JUST WHETHER. A list that only says which files moved makes a person open each
	 * one to find out whether it was a typo or a rewrite. Both are hidden at zero rather than drawn
	 * as `+0`, because a zero is a number the eye has to read to discard.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> AddedText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RemovedText;

	/** How many decisions this file is waiting on, in the conflict colour. Hidden at zero. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> DecisionText;

private:
	void PaintFill();

	/**
	 * **THE SOLE OWNER OF THE TITLE'S INK AND ITS STRIKE.** Both used to be written twice — once when
	 * the row was filled in, from the comparison window, and again here from git — and this one ran
	 * second, on every hover and every selection change. So the red the E11.19 rule computed was
	 * overwritten before it was ever drawn. One property, one painter.
	 */
	void PaintTitle();

	/** A brush that draws nothing: how a `UTextBlock` is told to stop striking. */
	static FSlateBrush NoStrike();

	/** A one-pixel strike tinted to match the name it crosses out, so the pair reads as one mark. */
	static FSlateBrush StrikeIn(const FLinearColor& Ink);

	TWeakObjectPtr<UGraphCodeVcsRowItem> Item;
	bool bRowSelected = false;
	bool bRowHovered = false;
	bool bRowIsHeading = false;
};

/**
 * One line of the side-by-side comparison, or one heading inside it.
 *
 * WHY BOTH SIDES ARE ON ONE ITEM. Two lists scrolling beside each other drift the moment either side
 * has a line the other does not, and every line in a diff is exactly that. Pairing happens once, in
 * `UGraphCodeVcsWidget::PairDiff`, and after it a row IS a pair — so the left and right halves cannot
 * come apart no matter how the list virtualizes.
 *
 * No `UPROPERTY` on any of it: the list holds a hard reference to the item, nothing here is a UObject
 * pointer that needs tracing, and reflecting fields the Designer will never see is cost with no
 * reader.
 */
UCLASS()
class GRAPHCODE_API UGcDiffRowItem : public UObject
{
	GENERATED_BODY()

public:
	/** Non-empty means a heading spanning both columns, and the cells below are not drawn. */
	FString Heading;

	FString LeftText;
	FString RightText;

	/** 1-based line number on each side, or 0 where the line does not exist on that side. */
	int32 LeftLine = 0;
	int32 RightLine = 0;

	EGcDiffTone LeftTone = EGcDiffTone::Context;
	EGcDiffTone RightTone = EGcDiffTone::Context;

	/** Non-zero on a row that offers a choice. It is what the hover buttons resolve. */
	int32 ConflictId = 0;

	// `RestoreBlock` IS GONE, AND ITS ABSENCE IS THE FIX.
	//
	// It held the block a click on this row would restore, computed for EVERY row at render time —
	// which meant parsing both documents on every redraw of every comparison, to answer a question
	// only a click ever asks. `UGraphCodeVcsWidget::BlockOwningRow` answers it lazily now, from the
	// two texts the panel keeps for the comparison on screen.
	//
	// **THE SMALLEST THING GRAPHCODE CAN SAFELY RESTORE IS A BLOCK, NOT A LINE** — §7's ruling and
	// not a shortcut. A line of a diff is text; what a writer takes is a complete block through its
	// own identity model, because "replace this one statement" is a sentence only that block's
	// grammar can interpret, and interpreting it here would be the second mutation engine §7 forbids.

	/**
	 * Everything the merger said about this decision, for the one surface where a sentence is free.
	 *
	 * THE APPARATUS AROUND A DECISION USED TO BE FOUR ROWS — a heading naming the entity and the
	 * base, a `yours`/`theirs` caption, the values, and the merger's own detail line, fenced with a
	 * rule top and bottom. All true, and unreadable stacked: five lines of scaffolding around two
	 * values, per decision. The row is amber, the columns are captioned, and the rest is here.
	 */
	FString Tooltip;

	/**
	 * `>= 0` on the "N unchanged line(s)" row, naming the run it stands for so a click can open it.
	 *
	 * THE ELISION USED TO BE A DEAD END. It said how many lines it had swallowed and gave no way to
	 * see them, so the one question a diff cannot answer without its context — "what is this change
	 * sitting in?" — had to be answered by opening the file in something else. Runs are numbered in
	 * document order by whichever builder emitted them, which is what lets an expansion survive the
	 * redraw that a hover or a pan causes.
	 */
	int32 RunIndex = INDEX_NONE;

	/**
	 * Part of something a person has to read: a changed line, or any row of a decision.
	 *
	 * SET BY THE BUILDER RATHER THAN INFERRED FROM THE TONE, because a decision's heading and its
	 * `was:` line are context-toned and still belong to the change — inferring would break the block
	 * into three "changes" and make the next-change button walk inside one decision.
	 */
	bool bIsChange = false;

	/** First row of a run of changed rows: what the next/previous-change buttons jump between. */
	bool bStartsChange = false;

	/**
	 * WHICH BLOCK THIS ROW'S LINE BELONGS TO, ON EACH SIDE — and it is filled by the builder now,
	 * which reverses the ruling `RestoreBlock`'s removal made. E11.11.
	 *
	 * **THE OLD RULING WAS ABOUT COST, AND THE COST IS GONE.** `RestoreBlock` was computed per row by
	 * calling `BlockOwningLine`, which PARSES THE WHOLE DOCUMENT — once per row, on every redraw. What
	 * fills these is one parse of each side for the whole comparison, then an array lookup per row:
	 * `FGraphCodeBlock` already carries `FirstLine`/`LastLine`, so a line's owner is a walk over a few
	 * spans rather than a re-read of the file. Same answer, two parses instead of four hundred.
	 *
	 * It has to be per row now because the SIDES ARE COLOURED by it — *"change the colour of the text
	 * … to represent whether is this current"* — and a colour is a property of the redraw. A hover
	 * cannot paint a file.
	 */
	FString LeftBlock;
	FString RightBlock;

	/**
	 * And which ENTITY inside that block, as `Frame/RootStack/Root`. Empty outside a tree block.
	 *
	 * **THE UNIT A PRESS TAKES, WHEN THERE IS ONE.** A group is a declaring line plus a braced body,
	 * so an entity is writable the way a block is and is very much smaller: `widgets{}` is the whole
	 * layout, and the row under the pointer is one widget of it. Filled by the same pass that fills
	 * the block, from one parse per side.
	 */
	FString LeftEntity;
	FString RightEntity;

	/**
	 * Is **this line** what the live asset holds right now?
	 *
	 * **THE THIRD TEXT, AND IT IS WHY THIS IS NOT `LeftText == RightText`.** With one side always the
	 * working file, "the same as current" was a question about the two texts already on screen. With
	 * a free left and right — `main` against `last Tuesday`, neither of them now — neither column is
	 * the asset, so the answer needs the asset: `UGraphCodeVcsWidget::CurrentTextForSelection`.
	 *
	 * **PER LINE, AND IT WAS PER BLOCK UNTIL E11.12 — A REPORTED BUG RATHER THAN A REFINEMENT.**
	 * *"why all left hand side rows are dull? it should be, any row same with current row MUST be
	 * bright."* The old predicate asked whether this side's WHOLE block equalled the asset's, so one
	 * changed line inside `graph { }` marked every line of it — hundreds — as not-current on both
	 * sides. A selected file nearly always differs somewhere inside its largest block, so the mark
	 * spent most of its life saying *nothing here is what the asset holds* over a screen that was
	 * mostly exactly that. `MarkBlockCurrency` now diffs each side against the live asset and keeps
	 * the surviving context lines.
	 *
	 * **AN EMPTY CELL IS CURRENT WHEN THERE IS NOTHING THERE TO REMOVE.** A side with no line on this
	 * row means the version does not have it, and applying that side deletes what the asset holds — so
	 * it is already current exactly when the asset does not hold the other column's line either.
	 *
	 * `false` on both sides is the ordinary reading of a changed line nobody has applied; `true` on
	 * one side is *this is what you are looking at in the editor*; `true` on both happens on any line
	 * the two versions agree about and the asset kept, which is most of a file.
	 */
	bool bLeftIsCurrent = false;
	bool bRightIsCurrent = false;

	/** Who to tell when a side is taken. Weak because the panel outlives no item and owns them all. */
	TWeakObjectPtr<UGraphCodeVcsWidget> Owner;
};

/**
 * How much of the file the comparison draws.
 *
 * A STRUCT RATHER THAN THREE PARAMETERS because the three are one decision — how much context is on
 * screen — and a call site passing `false, true, Set` says nothing about which is which.
 */
struct FGcDiffViewOptions
{
	/** Drop unchanged lines entirely. The old `bChangedLinesOnly`. */
	bool bChangedLinesOnly = false;

	/** Nothing is elided: the whole file, both columns, however long it is. */
	bool bShowAllLines = false;

	/** Runs the reader has opened one at a time, by `UGcDiffRowItem::RunIndex`. */
	TSet<int32> ExpandedRuns;
};

/**
 * A block whose every line changed, as the span of the diff it stands for. E10.9.
 *
 * WHY ONE ROW FOR FORTY. A `widgets { }` tree rebuilt or a `graph { }` relaid comes out of a line
 * diff as forty red lines beside forty green ones — technically the truth, and practically
 * unreadable: the reader compares forty pairs to learn one fact, which is *this block is different
 * now*. The pairs are still there and one click opens them; what changes is that the default answer
 * is the fact rather than the evidence for it.
 *
 * THE DIFF ENGINE IS NOT WHERE THIS GOES, and that is a ruling rather than a preference.
 * `UGraphCodeVcsLibrary::DiffText` is a plain deterministic text diff — that IS the demonstration
 * the whole format exists to make, and `FGcDocumentMerge` depends on its exactness. This is a layer
 * above it that reads the same lines and decides what to draw.
 */
struct FGcBlockRun
{
	/** First and last diff-line index the collapsed row stands for, inclusive. */
	int32 First = 0;
	int32 Last = 0;

	/** `widgets`, `function IsEqual`, `RowFill` — what the row calls the block. */
	FString Name;

	/** How many lines the block's body held on each side. */
	int32 LeftLines = 0;
	int32 RightLines = 0;
};

/**
 * The entry widget for one comparison line. Its layout is `WBP_GraphCodeVcsDiffRow`.
 *
 * HOVER IS WHERE THE RESOLVE LIVES NOW. It used to be a spin box in the footer: list the conflicts,
 * read an id off the screen, type it into a box, press Take ours. Every step of that is a chance to
 * resolve a conflict you were not looking at, and the code that fed it carried a comment about
 * exactly that risk. A button that appears ON the value it takes cannot name the wrong one.
 */
UCLASS(Blueprintable)
class GRAPHCODE_API UGraphCodeVcsDiffRow : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/**
	 * Scroll this row's two cells to `Pixels`, so a line wider than its column can still be read.
	 *
	 * EACH CELL IS ITS OWN HORIZONTAL `UScrollBox`, and the panel keeps every one of them at the same
	 * offset. One scroll box cannot serve both halves — a scroll box is one viewport, and the whole
	 * point of a side-by-side is two viewports showing the same offset of different text — so the
	 * synchronising is the panel's job. What UMG gives for free in exchange is the clipping, the
	 * scrollable extent, and an offset that survives the row being recycled.
	 *
	 * The GUTTER sits outside the scroll box and does not move: a line number that scrolls away is one
	 * you have to scroll back for to know where you are.
	 */
	void SetPan(float Pixels);

	/** How far this row could scroll, in pixels. Zero when its longest cell already fits. */
	float GetPanRange() const;

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;

	/** Which half the pointer is in — the row spans both, so entering says nothing about the side. */
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;

	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> HeadingBand;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> HeadingText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UHorizontalBox> SplitRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> LeftFill;

	/** The left cell's viewport. Clips the line and carries the pan. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UScrollBox> LeftScroll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> LeftNumber;

	/**
	 * `-` or `+`, and it is not decoration.
	 *
	 * The fill says which side a line is on and the sign says it again, because the fill alone is a
	 * colour and this project's rule for every other status is icon + word + colour. It is also what
	 * survives a screenshot pasted into a bug report, and what a person copying a line out expects to
	 * see in front of it.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> LeftSign;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> LeftBody;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> RightFill;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UScrollBox> RightScroll;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RightNumber;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RightSign;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RightBody;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UButton> TakeLeftButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UButton> TakeRightButton;

	/**
	 * `Apply properties` / `Applied`, on the row the pointer is over — **one per side**. E10.17,
	 * two-sided in E11.11.
	 *
	 * **THE SIDE YOU HOVER IS THE SIDE THAT GETS WRITTEN** — user-directed 2026-08-05: *"to apply a
	 * block, i supposed to choose left or right to apply, and so the apply would be at left if hover
	 * around left"*. With one fixed side there was one answer and the button could live anywhere; with
	 * a free left and right, "apply this block" is two different writes and the only unambiguous place
	 * to say which is ON the column it takes from. Same argument that put the take buttons on their
	 * own values in E10.4, and the same one that keeps `RestoreButton` under the left column.
	 *
	 * **`BindWidgetOptional`, and that is not laziness.** Every other control here is required, so a
	 * layout that drops one fails to compile — right for a panel whose layout ships beside its class.
	 * These arrived after `WBP_GraphCodeVcsDiffRow` was already in somebody's checkout, and a required
	 * binding would turn a stale layout into a class that cannot be constructed at all, which is a
	 * worse failure than a missing button.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> LeftBlockApplyButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> LeftBlockApplyLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> BlockApplyButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> BlockApplyLabel;

private:
	void ShowTakeButtons(bool bShow);

	/**
	 * Draw the per-block control for the row under the pointer, or take it away. Once per side.
	 *
	 * **STILL ASKED ON HOVER, AND WHAT IT ASKS IS CHEAPER NOW.** It used to parse both comparison
	 * texts to find the row's block and to compare the two copies of it; both answers are computed
	 * once per comparison and carried on the item (`UGcDiffRowItem::LeftBlock`,
	 * `bLeftIsCurrent`), so a hover is now a string copy and two branches. E11.7b's rule — per-row
	 * work happens on the gesture, never on the redraw — is kept by making the work not happen at
	 * all rather than by moving where it does.
	 */
	void ShowBlockApply(bool bShow);

	/** One side of it. `bRightSide` picks both the button and the half of the item it reads. */
	void ShowBlockApplySide(bool bShow, bool bRightSide);

	/** Work out which column the pointer is in, from the row's own geometry. */
	void NoteHoverSide(const FGeometry& Geometry, const FPointerEvent& Event);

	UFUNCTION() void OnTakeLeftClicked();
	UFUNCTION() void OnTakeRightClicked();
	UFUNCTION() void OnLeftBlockApplyClicked();
	UFUNCTION() void OnBlockApplyClicked();

	TWeakObjectPtr<UGcDiffRowItem> Item;

	/** The block each apply button is currently offering, resolved on the hover that drew it. */
	FString HoveredLeftBlock;
	FString HoveredBlock;

	/** Which column the pointer is in. Only that side's apply button is drawn. */
	bool bHoveringRight = false;
};

/**
 * One decision, with the line of the document it actually belongs to.
 *
 * WHY THIS EXISTS. A `conflict { }` construct is written as ONE block at the END of the document —
 * `FGcDocumentMerge::RenderConflicts` says why: the reader enforces that a top-level name appears
 * once, so one block per conflict would make the file unparsable. That is right for the format and
 * wrong for a screen: it puts the decision hundreds of lines away from the statement it decides, so
 * the two things a person needs at once are never on screen together.
 *
 * The site is findable, and it is findable BY THE SAME PREDICATE THE WRITER USES. `FGcResolve::Pick`
 * locates the line it will edit by requiring that `ours` occurs exactly once outside the construct;
 * this locates it the same way. Copying the predicate rather than inventing a second one is what
 * makes the line the panel draws the line a take will actually change — a lookup of my own would be
 * a second opinion, and the first time the two disagreed the panel would be pointing at the wrong
 * statement while the write went somewhere else.
 */
struct FGcInlineDecision
{
	FGcConflictView Conflict;

	/** The whole line `ours` was found on, or empty when the site could not be identified. */
	FString SiteLine;

	/** 1-based line number of the site, or of the construct when there is no site. */
	int32 SourceLine = 0;
};

/**
 * The GraphCode version-control panel. GC-EDITOR-VCS-DESIGN.md §7 and the information architecture in
 * `GC-EDITOR-VCS-VISUAL-BRIEF.md` §4-§6 at the scope this milestone ships.
 *
 * WHAT IS HERE AND WHAT IS NOT. The brief describes five views — Changes, Compare, Merge, History,
 * Checks — over a repository snapshot E8 has not built yet. This panel ships the three that need no
 * snapshot: Changes (the list), Compare (a side-by-side of any legal source pair) and Merge (preview,
 * conflicts, resolve, write, apply). **History is absent rather than disabled**, because the brief's
 * §3 scope guard says an absent mutation must never look shipped, and a greyed tab with no owner is
 * exactly that. Checks folds into the footer's result line for now.
 *
 * WHAT E10 CHANGED, AND WHY EACH THING MOVED:
 *
 *   - **Eleven always-visible controls became three plus one.** The header carries the repository, a
 *     refresh and a sync; everything else project-wide is behind the overflow button. The footer
 *     carries at most one primary action, chosen by the state, so a button that cannot do anything is
 *     not on screen saying so.
 *   - **Four view buttons became one picker** whose entries name BOTH sources in plain words. Four
 *     buttons is a radio group drawn as actions, and "HEAD" is a word this panel's reader is not
 *     required to know.
 *   - **One text blob became a list of paired lines.** `FormatDiff` produced `-`/`+` text in a single
 *     `TextBlock`; a `+` and a `-` cannot be coloured differently inside one, so the diff was grey on
 *     grey. Rows can be, and rows can also be hovered.
 *   - **The conflict spin box is gone.** Resolving is a button on the value it takes.
 *   - **The inspector column became a strip under the header**, because the middle and right of the
 *     panel are now the two sides of the comparison. Its facts did not move out of the reading path:
 *     `omits:` gets a permanent chip in the header whenever it is non-empty, which is the brief's §6
 *     requirement that uncompared facets stay visible even when the merge is clean.
 *
 * THE LAYOUT IS `WBP_GraphCodeVcs` AND THE LOGIC IS HERE. `BindWidget` is the contract: a layout that
 * drops a control fails to compile rather than silently doing nothing. Every use is still
 * null-checked, because a spec must be able to construct this class bare.
 */
UCLASS(Blueprintable, meta = (DisplayName = "GraphCode Version Control Panel"))
class GRAPHCODE_API UGraphCodeVcsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- what the panel says, as pure functions over data -------------------------------------

	/** The heading over the list: how many need attention, and of what kind. */
	static FString SummarizeStatus(const TArray<FGcSidecarStatus>& Rows);

	/**
	 * The group a row belongs to, and the word that names it — ONE vocabulary for both kinds of stuck.
	 *
	 * E10.3 SPLIT A FACT AND E10.6 STOPPED SPELLING THE SPLIT AT THE READER. Git's stuckness and the
	 * file's stuckness are genuinely two things, and the panel still keeps them apart internally — but
	 * a person reading the list only wants to know which files need a decision, and it read
	 * `MID-MERGE (3)` over one group and `CONFLICTED (1)` over another as though those were different
	 * jobs. They are the same job. Both land in `conflict` now; the row's tooltip and the details strip
	 * still say which of the two it is, because that decides which screen can answer it.
	 */
	static FString AttentionWord(const FGcSidecarStatus& Row);

	/**
	 * Is somebody's choice the only thing that can move this file forward?
	 *
	 * ONE PREDICATE, AND IT IS `ModesFor`'s — the conflict list, the header count, the group word and
	 * the Next button all ask it, and they were four spellings of it.
	 *
	 * **THE BUG THIS FIXES: a resolved conflict never went away.** The old spelling was "git says
	 * `UU`, or the text holds a construct", and taking every side clears the second but NOT the
	 * first: git keeps all three index stages until the merge is committed, and this panel cannot
	 * `git add` — the allowlist has no writing verb and never will. So a file whose every decision
	 * had been taken, whose merge had been written and applied, sat in the conflict group at `0`
	 * forever. `ModesFor` already knew better: it stops offering the merge once the file carries
	 * `merged-from:`, because a preview of a merge already in the file can only repeat itself. This
	 * asks that same question instead of a second one that disagrees with it.
	 */
	static bool NeedsDecision(const FGcSidecarStatus& Row);

	/**
	 * The groups, worst first — the single ordering the list, the count line and Next all walk.
	 *
	 * There used to be three copies of this order in this file and one of them had a sixth entry.
	 */
	static const TArray<FString>& GroupOrder();

	/**
	 * `git status --porcelain`'s two letters, in words.
	 *
	 * "WHAT IS UU?" IS A FAIR QUESTION AND THE PANEL USED TO HAVE NO ANSWER. The brief's §1 rule is
	 * that git's layer stays visible and is never folded into GraphCode's — it is not a rule that the
	 * layer has to be shown in git's private two-character shorthand. The code stays, in the tooltip
	 * and the details strip, next to the sentence that says what it means.
	 */
	static FString GitWord(const FString& PorcelainCode);

	/** The long form: what this code means and what it implies for the file. Tooltip material. */
	static FString GitExplanation(const FString& PorcelainCode);

	/**
	 * How many facets this document really declares uncovered.
	 *
	 * `omits: none` IS A DECLARATION THAT NOTHING IS OMITTED, and counting its one token made the
	 * header say "1 parts of this asset are not covered" about a file whose own header says the
	 * opposite. `FGcDocumentMerge` has had the right predicate all along; this is the second reader.
	 */
	static int32 CountOmissions(const TArray<FString>& Omits);

	/** The line a Sync All or Check Only run leaves behind. */
	static FString SummarizeSync(const FGcSyncReport& Report);

	/**
	 * What an apply that WROTE something adds to its own summary, given what that summary already says.
	 *
	 * **BECAUSE THE PANEL SAID IT TWICE.** The apply layer's summary already ends with *"The asset is
	 * dirty and unsaved."* for most kinds, and the panel appended its own copy in different capitals,
	 * so a press produced *"The asset is dirty and unsaved.   The asset is dirty and UNSAVED. Save it
	 * to keep this."* — one fact, two sentences, read by a person as two things having happened.
	 *
	 * A static over the summary rather than a branch inside `RunApply` for one reason: it is the half
	 * of that function a spec can hold still. Everything else there needs a selection, a live asset
	 * and a write.
	 */
	static FString SaveReminderFor(const FString& Summary);

	/** The conflict list as text — the accessible unified reading the brief's §8 also requires. */
	static FString SummarizeConflicts(const TArray<FGcConflictView>& Conflicts);

	/** What a merge preview says, including when it declines — which is most of the time. */
	static FString SummarizePreview(const FGcMergePreview& Preview);

	/** The footer's one-line description of what is selected and what can be done to it. */
	static FString SummarizeSelection(const FGcSidecarStatus& Row, int32 UnresolvedConflicts);

	/** The picker entry for a mode: both sources, in the order they are drawn. */
	FString ModeLabel(EGcCompareMode Mode) const;

	/**
	 * Which blocks of this diff are wholly changed, in document order and non-overlapping. E10.9.
	 *
	 * OUTERMOST WINS AND THE REST IS RECURSION. `functions { }` holding one rewritten function is
	 * not a wholly-changed block — the other functions survived — but the rewritten function inside
	 * it is, so a block that does not qualify is descended into rather than dropped.
	 *
	 * WHAT "WHOLLY CHANGED" MEANS, EXACTLY, because a looser rule would collapse edits people need
	 * to see: no line inside the block survives on both sides. A blank line and a line holding only
	 * braces are not survivors — those are punctuation the matcher aligned, not a statement anybody
	 * wrote — and every other matched line is, so one surviving property keeps the whole block on
	 * the line-by-line reading.
	 *
	 * THREE THINGS IT REFUSES TO COLLAPSE, each of them a way this could hide something:
	 *   - a span that also covers a line belonging to neither side's block, so a change next door
	 *     can never be swallowed by its neighbour's summary;
	 *   - a span holding a conflict marker — a decision that is not drawn is one nobody can take;
	 *   - a block small enough that the summary costs a click and saves nothing.
	 */
	static void PlanBlockCollapse(const TArray<FGcDiffLine>& Lines, TArray<FGcBlockRun>& OutRuns);

	/**
	 * `  ·  5 Aug 14:32` — the stamp a column caption carries. Empty for an instant nobody has.
	 *
	 * **USER-DIRECTED 2026-08-05:** *"can you make the time appear beside the the title of current
	 * and before? so we know that when is the file last changed."* Two versions side by side and no
	 * dates on either is the one thing a comparison screen cannot afford to leave out: `local` and
	 * `branch main` say WHICH versions, and nothing said HOW OLD — so a column of identical-looking
	 * code gave no clue whether the difference was made this morning or last spring.
	 *
	 * The date is always there, not only the time. `14:32` on its own is the same six characters
	 * whether it was today or in March, and against a branch it is very often not today.
	 */
	static FString WhenStamp(const FDateTime& Utc);

	/**
	 * When the file behind one side of the comparison last changed. Epoch when nothing can say.
	 *
	 * WHERE EACH ANSWER COMES FROM, because they are four different questions:
	 *
	 * | side | the honest "last changed" |
	 * |---|---|
	 * | `Working` / `Editor` | the newest local-history instant for the package |
	 * | `History` | the resolved entry's own instant |
	 * | `Ref` | `git log -1` on that ref for that path |
	 *
	 * **NOT THE SIDECAR'S mtime, EVER** — §2.2a's measurement: a `.gc` gets rewritten by
	 * regeneration, by a merge and by an apply, so its mtime measures the tool rather than the work.
	 * The store's instants are stamped when GraphCode OBSERVED a real body change, which is the
	 * thing a reader means by "when did this last change".
	 */
	FDateTime LastChangedFor(const FGcCompareSource& Source) const;

	/**
	 * The two column captions, left then right. Same words the picker used, split at the arrow.
	 *
	 * ONE WORD EACH. They were sentences — `Your file now` over one column and `After the merge`
	 * over the other — and a caption is read once and then sat above four hundred lines of code for
	 * the rest of the session. What a diff's two columns mean is `Before` and `Now`; what a
	 * conflict's mean is `Yours` and `Theirs`, and that pair is fixed wherever a decision is drawn.
	 * The picker in the left pane still joins both words, so nothing is lost by shortening them —
	 * which is the only reason it is safe to.
	 *
	 * NOT STATIC ANY MORE. `Compare`'s two sides are whatever the two source pickers hold, so the
	 * captions are a fact about this panel rather than about the enum.
	 */
	void ModeSides(EGcCompareMode Mode, FString& OutLeft, FString& OutRight) const;

	/**
	 * The sources the compare zone can resolve to: `local`, `editor`, the timeline, `HEAD`, and —
	 * only when somebody is actually looking at them — one per local branch.
	 *
	 * **THE BRANCHES ARE LAZY NOW, AND THAT IS A QUARTER OF WHAT A REFRESH COST.** `git for-each-ref`
	 * is ~100 ms, it used to run unconditionally here, and the panel opens on `HEAD` — so every
	 * refresh spawned a process to enumerate 24 branches that no control on screen was showing. They
	 * are fetched when the category becomes `Branch`, and re-fetched on each refresh that runs while
	 * it still is, which is exactly the staleness the unconditional version had.
	 */
	void RefreshSources();

	/**
	 * Read the local branches if this panel has not already, and add them to `SourceOptions`.
	 *
	 * Public because a spec that wants to assert what the branch picker offers has to be able to ask
	 * for the branches without pretending to be a click.
	 */
	void EnsureBranchesLoaded();

	/**
	 * Learn every ref's TIP TIME, once per session. E11.12 — the epoch half of `OrderOf`.
	 *
	 * **THE PROCESSES ARE SPENT ON A GESTURE AND NOWHERE ELSE**, which is E11.8's rule and the reason
	 * this is a separate call rather than something `OrderOf` does when it needs an answer. Two doors
	 * lead here: pulling a branch box down, and putting a side on the timeline. Until one of them
	 * happens a ref has no time and is `Unordered` against a moment — which is exactly what E11.11
	 * offered, so nothing is worse before the reader asks and everything is right after.
	 *
	 * The branches cost NOTHING beyond the `for-each-ref` the list already needs
	 * (`ListBranchesWithTips`); `HEAD` costs one `git log -1`, because it is not a branch and
	 * `for-each-ref` does not report it.
	 *
	 * Public so a spec can establish the same state a gesture would, rather than asserting over a
	 * panel whose ordering has not been given its facts yet.
	 */
	void EnsureRefTimesLoaded();

	/** Which two sources the comparison is between. Every caption reads these. */
	const FGcCompareSource& GetLeftSource() const { return LeftSource; }
	const FGcCompareSource& GetRightSource() const { return RightSource; }

	/**
	 * Is this kind one of the two things the panel means by **now** — the file on disk, or the asset?
	 *
	 * The predicate behind `ComparedAgainst`, `SourceToRestoreFrom`'s label rule and the `Current`
	 * category, spelled once so those three cannot come to disagree about what "current" is.
	 */
	static bool IsCurrentKind(const EGcSourceKind Kind)
	{
		return Kind == EGcSourceKind::Working || Kind == EGcSourceKind::Editor;
	}

	/**
	 * The side that is NOT the current one — what a caption, a heading or a file list is ABOUT.
	 *
	 * **THIS IS HALF OF WHAT `ComparedAgainst` USED TO BE, AND THE SPLIT IS E11.11's FIRST RULING.**
	 * Its 32 callers wanted one of two different things and could not be told apart: *"the version
	 * this panel could put back"* — which is always the LEFT column — and *"the one that is not
	 * now"*, which is this. While one side was always the working file the two coincided; with a free
	 * left and right they do not, and a caller that wanted the first and got the second restores from
	 * the wrong version.
	 *
	 * THE PAIR FLIPS FOR THE EDITOR, AND THAT IS THE DIFF CONVENTION RATHER THAN A SPECIAL CASE.
	 * The left column is the older, settled side and the right is the one in front of you, so a `+`
	 * always means "this is new". Against a branch that puts the branch on the left and `local` on
	 * the right; against the editor the settled side is the FILE, so `local` goes left and the
	 * unsaved object goes right — and an unsaved edit reads as `+` in both, which is the point.
	 *
	 * **WITH NEITHER SIDE CURRENT IT ANSWERS `LeftSource`**, and that is not a fallback: the left is
	 * the settled, older side by the same convention, so a heading reading *"N files differ from
	 * branch main"* over a `main ↔ last Tuesday` comparison names the side the reader is measuring
	 * FROM. `SourceToRestoreFrom` returns the same thing for the same reason, which is why the two
	 * agree exactly where it matters and diverge only against the editor.
	 */
	const FGcCompareSource& ComparedAgainst() const
	{
		return IsCurrentKind(LeftSource.Kind) ? RightSource : LeftSource;
	}

	/**
	 * The version a restore, a revert or a whole-file put-back takes FROM: **the left column,
	 * unconditionally.** E11.11's first ruling, and it needs no rule of its own.
	 *
	 * The diff convention already puts the older, settled side on the left — so with two versions
	 * neither of which is now, the one you could go back TO is the one a `+` is measured against.
	 * Every caller that asks "what would pressing restore write" asks this; every caller that asks
	 * "what is this comparison about" asks `ComparedAgainst`.
	 */
	const FGcCompareSource& SourceToRestoreFrom() const { return LeftSource; }

	/** Put `Source` opposite the working file, on whichever side the convention above wants. */
	void CompareAgainst(const FGcCompareSource& Source);

	// ---- the categories, which are the first picker on each side --------------------------------
	//
	// **TWO SIDES, THREE CATEGORIES, AND ONE TIME MACHINE.** E11.11, user-directed: *"for the
	// compare, make it left and right selection, with 2 rows when selected"*. Each side has a
	// category box and, when its category has more than one target, a second box under it — so a
	// side is at most two rows and the pair is at most four.
	//
	// **THE TIME MACHINE IS THE SECOND ROW OF A SIDE ON `local history`** (E11.16, user-directed:
	// *"below local history is a text field for time, below also, if it's also time based"*). It was
	// one shared control with a driven end, two pressable halves and a readout above them — four
	// pieces of vocabulary to express *which end moves*, over a question that answers itself once
	// each side has its own box.

	/** Which kind of thing one side is pointed at. `bRightSide` picks the side. */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	EGcCompareCategory GetCategoryFor(bool bRightSide) const
	{
		return bRightSide ? RightCategory : LeftCategory;
	}

	/**
	 * The category of the side that is not the current one — the question every pre-E11.11 caller
	 * was really asking, kept so a spec written against one picker still means something.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	EGcCompareCategory GetCompareCategory() const
	{
		return IsCurrentKind(LeftSource.Kind) ? RightCategory : LeftCategory;
	}

	/**
	 * Point one side at a KIND of comparison, and land it on that kind's default target.
	 *
	 * A CATEGORY WITHOUT A TARGET IS A CONTROL THAT DOES NOTHING UNTIL YOU FIND THE OTHER ONE. So
	 * picking `Branch` lands on the last commit, and picking `History` lands on the newest instant
	 * the store holds — the same rule E11.4 already applied when `local history` was one entry in a
	 * flat list, for the same reason.
	 *
	 * **IT REFUSES TO PUT THE SAME SOURCE ON BOTH SIDES**, by moving the OTHER side to that side's
	 * own default rather than by declining the press. A picker that can express `local ↔ local` is a
	 * picker that can be set to say nothing, and the reader's own action is never the thing to undo.
	 *
	 * **RE-PICKING `local history` ON A SIDE IS NOT A NO-OP — IT AIMS THE TIME MACHINE AT THAT SIDE.**
	 * See `DriveTimeMachineEnd`. Every other category keeps the don't-punish-a-stray-click rule.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SetCategoryFor(bool bRightSide, EGcCompareCategory Category);

	/**
	 * Point the one time machine at one END of the comparison. E11.12.
	 *
	 * **THIS IS THE HALF OF E11.11's SECOND RULING THAT WAS NEVER BUILT, AND THE USER FOUND IT:**
	 * *"i see that two local history, cant select two time myself??"* — with both sides on the
	 * timeline, `bTimeMachineOnRight` was set only when a side's CATEGORY changed, so the driven end
	 * was whichever was chosen last and nothing on screen could switch it. The readout already marked
	 * the driven end with `[ ]`; there was no gesture that moved the brackets, which made the second
	 * moment unreachable rather than merely awkward.
	 *
	 * **WHAT E11.16 LEFT OF IT.** Each side owns a step pair and a typed box now, so every gesture
	 * says which end it means by which row it is in — and this is what those gestures call to record
	 * it. It stayed for two reasons rather than three: re-picking `local history` in a side's
	 * category box still aims it, and `GetComparedInstant` still needs to know which end a caller
	 * with no side in hand means.
	 *
	 * Refuses, in words, when that side is not a moment. Silently doing nothing is how a control
	 * teaches somebody that it is broken.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void DriveTimeMachineEnd(bool bRightSide);

	/** Which end the time machine drives — the one the readout brackets. */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsTimeMachineOnRight() const { return bTimeMachineOnRight; }

	/** Move the side that is not the current one. The one-picker spelling, kept for callers. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SetCompareCategory(EGcCompareCategory Category);

	/** Put `Source` on one named side, leaving the other alone. The two-sided picker's one verb. */
	void SetSourceFor(bool bRightSide, const FGcCompareSource& Source);

	/** Every source this category can be pointed at, in picker order. */
	TArray<FGcCompareSource> TargetsFor(EGcCompareCategory Category);

	/**
	 * Which of two versions came first — and **`Unordered` is a real answer**, not a failure.
	 *
	 * **THE LEFT IS ALWAYS BEFORE.** User-directed 2026-08-05: *"selecting a before will filter out
	 * selections for after. same as selecting after will filter out options for before. and lhs is
	 * always before."* The picker enforces it by dropping the candidates it can PROVE are on the
	 * wrong side, and `SetSourceFor` enforces it for everyone else by swapping.
	 *
	 * **WHAT CAN BE PROVED IS LESS THAN A CLOCK WOULD SUGGEST, AND PRETENDING OTHERWISE IS THE TRAP.**
	 *
	 * **E11.12 REPLACED THE ANSWER WITH ONE RULE: EVERY SOURCE GETS AN EPOCH, AND THE SORT IS THE
	 * ORDER.** User-directed 2026-08-05: *"why dont, just give each one an epoch or something like
	 * that, then sort it? then show anything before/after"*. `SortKeyFor` is that epoch and this is
	 * that sort, so there is one place where "which came first" is decided for all four kinds.
	 *
	 * - Two MOMENTS order by instant. This is the case the ruling was asked for — *"1pm to 2pm
	 *   coding"* — and it is exact.
	 * - `local` and `editor (unsaved)` come after every archived or committed version, because both
	 *   are states of NOW; and `local` comes before `editor`, because the unsaved object is the file
	 *   plus whatever has not been written yet. **They are a TIER rather than a clock reading**: the
	 *   working file's mtime is a fact about the last write and not about what it holds, so the two
	 *   things that are now are ranked rather than dated.
	 * - **A REF ORDERS BY ITS TIP TIME — against a moment and against another ref.** This reverses
	 *   E11.11, which answered `Unordered` on the argument that a commit time is a fact about a clock
	 *   rather than about ancestry. **That argument is still true and it is about ANCESTRY**; this
	 *   control's axis is TIME — the timeline defines it — and on a time axis a branch does have a
	 *   place: when it last moved. Refusing to put it there left the one category with no time in a
	 *   picker whose whole subject is when.
	 * - **`Unordered` now means exactly one thing: a source with no time.** A ref whose tip time has
	 *   not been asked for yet (`EnsureRefTimesLoaded`) and one git cannot resolve both land there, so
	 *   the permissive answer is what a missing fact produces rather than what a whole kind produces.
	 *
	 * **WHAT ORDERING REFS AGAINST EACH OTHER COSTS, SAID OUT LOUD:** two branches whose tips differ
	 * by a minute are now strictly ordered, so exactly one of `main -> dev` and `dev -> main` is
	 * offered — the older tip on the left. That is the consequence E11.11 refused to accept silently;
	 * it is accepted deliberately here, because a picker whose two halves are labelled *before* and
	 * *after* has already promised an order, and offering both directions was the panel declining to
	 * keep that promise for one category out of four.
	 */
	enum class EGcOrder : uint8
	{
		Before,
		After,
		Unordered,
	};

	/**
	 * **NOT STATIC ANY MORE, AND THE REASON IS THE CACHE.** A ref's epoch is a git fact, and the one
	 * rule this panel keeps about git is that drawing may not spend a process. So `RefTipTimes` is
	 * filled by a gesture and read here; a static function would have had to either take the times as
	 * an argument at every call site or fetch them on a redraw.
	 */
	EGcOrder OrderOf(const FGcCompareSource& A, const FGcCompareSource& B) const;

	/**
	 * One source's place on the axis: a TIER, and a moment inside it.
	 *
	 * Tier 0 is a version that already happened; tier 1 is the file on disk and tier 2 the unsaved
	 * object. The moment is only consulted inside tier 0, and `bHasTime` false there is what makes
	 * `OrderOf` answer `Unordered`.
	 */
	struct FGcSortKey
	{
		int32 Tier = 0;
		FDateTime When;
		bool bHasTime = false;
	};

	FGcSortKey SortKeyFor(const FGcCompareSource& Source) const;

	/** The targets of `Category` that may sit on this side, given what the other side holds. */
	TArray<FGcCompareSource> OrderedTargetsFor(EGcCompareCategory Category, bool bRightSide);

	/** What the first picker calls a category. One table, so the entry and the lookup agree. */
	static FString CategoryLabel(EGcCompareCategory Category);

	/** Which category a resolved source belongs to. The inverse, used to keep the two boxes in step. */
	static EGcCompareCategory CategoryOf(const FGcCompareSource& Source);

	// ---- the commit row, E11.26 -----------------------------------------------------------------

	/**
	 * One entry in the commit box: what it draws, and what it means.
	 *
	 * TWO FIELDS BECAUSE THE TIP IS SPELLED TWO WAYS. Its display is the commit — the sha and the
	 * subject, like every other entry — while its SOURCE is the branch's own ref, so that picking it
	 * is byte-identical to picking the branch and `IsWorkingAgainstHead` still finds `HEAD` where it
	 * expects it. Everywhere else the source carries its own label and this would be one field.
	 */
	struct FGcCommitOption
	{
		FString Display;
		FGcCompareSource Source;
	};

	/** Which branch's commits this side's row is listing, or empty when it has none to list. */
	FString BranchContextFor(bool bRightSide) const;

	/** That branch's commits as picker entries, newest first. Never spawns a process. */
	TArray<FGcCommitOption> CommitOptionsFor(bool bRightSide);

	/** One `git log` for a ref, cached until somebody opens the box again. Seeds `RefTipTimes`. */
	void EnsureCommitsLoaded(const FString& Ref);

	/** `a15272f  <subject>` — the sha first, because that is the end two commits differ at. */
	static FString CommitLabel(const FGcCommitEntry& Commit);

	// ---- the timeline -------------------------------------------------------------------------

	/**
	 * The selected asset's history, newest first. Empty is the ordinary state for a new asset.
	 *
	 * **STILL PER ASSET, AND NO LONGER WHAT THE CONTROL PICKS FROM.** E11.4 put these in a combo
	 * beside the source picker, because an instant belonged to one asset and meant nothing for the
	 * next row. E11.8 turned that around instead of working around it: the panel picks a moment
	 * project-wide and each asset resolves its OWN entry from it (`HistoryEntryAt`), so the list
	 * below is what the store can speak for at that moment rather than one file's saves. This array
	 * survives as the selected file's resolved timeline — the footer names an entry out of it, and a
	 * restore still restores exactly one asset (§9 keeps project-wide mutation in V2).
	 */
	const TArray<FGcHistoryEntry>& GetHistoryEntries() const { return HistoryEntries; }

	/**
	 * The time machine's stops, newest first: one per SECOND the store holds an archive in, each
	 * kept to the millisecond it really happened at.
	 *
	 * **BOTH HALVES OF THAT ARE THE POINT** — user-directed 2026-08-06: *"we can have millisecond
	 * precision, but group by second together"*. A save writes a burst of archives a few hundred
	 * milliseconds wide, and one stop per FILE made most presses redraw an identical screen; the
	 * instant itself stays exact, because it is what a package resolves its own entry from.
	 *
	 * **ONE GLOB AND NOTHING IS OPENED** — `FGraphCodeHistory::ScanArchives` reads the date out of
	 * each archive's FILENAME, which is why the name carries one. Cached per refresh, so dragging
	 * the slider costs no file system at all: a control somebody is holding may not do IO per frame.
	 */
	const TArray<FDateTime>& GetTimelineInstants() const { return TimelineInstants; }

	/** Show the project as it was at `Utc`. The time machine's one verb. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void CompareAgainstInstant(const FDateTime& Utc);

	/**
	 * One moment older or newer, on the side that asked. All four `<`/`>` buttons are this, so they
	 * cannot drift apart.
	 *
	 * **THE SIDE IS AN ARGUMENT NOW** (E11.16). Each side owns its own steps and its own typed box,
	 * so a press says which end it means instead of aiming a shared control first.
	 *
	 * **AND A MOMENT IS A SECOND** — `RefreshTimelineInstants` groups a save's burst of archives into
	 * one stop, so this steps one thing that happened rather than one file that was written.
	 *
	 * Public so a spec can press a step without a mouse — and because "did that press land on a
	 * moment the store actually holds" is the assertion the snapping exists for.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void StepTimeline(bool bRightSide, bool bOlder);

	/**
	 * Out of this block of work and into the one before or after it — the `prev`/`next` pair.
	 *
	 * **A BLOCK IS A RUN OF SAVES WITH NO QUIET MINUTE IN IT** — user-directed 2026-08-06: *"next
	 * block is group by anything that happened within a minute. so a 60 minute record continuously
	 * is one block"*. So this crosses a SILENCE rather than a fixed span, and an afternoon of steady
	 * work is one press wide instead of several hundred.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void StepBlock(bool bRightSide, bool bOlder);

	/** Which moment is on screen. Epoch when the comparison is not against the timeline. */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	FDateTime GetComparedInstant() const;

	/**
	 * `5 Aug 2026  ·  14:32:07` — a UTC instant in the reader's own clock.
	 *
	 * NO `before` HERE, unlike `HistoryLabel`, and the difference is exactly the one E11.8 turned
	 * around. An ENTRY is named for the save that replaced it, so it must say which side of that
	 * moment it is. An INSTANT is just a moment: the project as it stood at 14:32:07, and every
	 * asset's entry is resolved from it. Putting `before` on this one would be claiming the moment
	 * itself has a side.
	 */
	static FString InstantLabel(const FDateTime& Utc);

	/**
	 * The entry that holds what `Package` looked like at `Utc` — its EARLIEST SURVIVING ENTRY AT OR
	 * AFTER it. §3.2, and the direction is the part everybody gets backwards.
	 *
	 * An entry archives the state that was true BEFORE the save that replaced it. So the version an
	 * asset held at 14:00 is not the newest entry before 14:00 — that one was already replaced — it
	 * is the earliest one written after it, whose payload is precisely what the save overwrote.
	 *
	 * @return false when the store has nothing at or after `Utc` for this package, which is the
	 *         ordinary "it has not changed since then" and not an error.
	 */
	static bool HistoryEntryAt(const FString& Package, const FDateTime& Utc, FGcHistoryEntry& Out);

	/**
	 * Rebuild the selected file's timeline.
	 *
	 * **THE CHOSEN MOMENT NO LONGER HAS TO SURVIVE A CHANGE OF FILE, AND THAT IS THE POINT.** §4's
	 * old rule — keep the chosen entry only if it belongs to the new file — existed because an entry
	 * id is unique inside one package's directory and names nothing outside it, so carrying one
	 * across a selection would point at a moment in another asset's life. An INSTANT has no such
	 * problem: 14:32 on Tuesday is the same 14:32 for every asset in the project, so walking the
	 * file list holds the moment still and moves only which entry each file resolves to. That is
	 * what a time machine is, and the old shape could not express it.
	 */
	void RefreshHistoryOptions();

	/**
	 * Compare against the moment one entry belongs to, by immutable entry id.
	 *
	 * KEPT AS THE ENTRY-SHAPED DOOR into an instant-shaped model: callers that already hold an entry
	 * — a spec, a restore that has just planned one — should not have to know that the panel stores
	 * a time. Resolution is exact in both directions: `HistoryEntryAt` takes the earliest entry at
	 * or after the instant, so asking for an entry's own instant hands that entry back.
	 */
	void CompareAgainstHistoryEntry(const FString& EntryId);

	/**
	 * What one instant is CALLED, and the word `before` in it is the correctness point.
	 *
	 * An entry archives the state that was true BEFORE the save that replaced it (`ArchivePredecessor`
	 * is named for that), so a label reading `14:32:07` invites "the asset at 14:32", which is the
	 * one thing this entry is not: at 14:32 the asset had already become the next version. `before
	 * 14:32:07` is the same six characters longer and cannot be misread.
	 *
	 * Local time, because a person reads their own clock; the store is UTC, because a store that
	 * moves when the clocks do cannot be ordered.
	 */
	static FString HistoryLabel(const FGcHistoryEntry& Entry);

	/**
	 * The same entry as a COLUMN CAPTION: `HistoryLabel` plus whichever half of the moment it left
	 * out. Never both halves twice.
	 *
	 * **IT EXISTS BECAUSE `Label + WhenStamp` COULD NOT BE RIGHT FOR BOTH DAYS.** `HistoryLabel`
	 * spells the clock always and the date only for an entry from another day, so a stamp appended
	 * unconditionally produced `before 14:36:24  ·  05 Aug 14:36` for today — one instant printed
	 * twice, in two different spellings, which reads as two.
	 *
	 * A caption is the one place the reader confirms WHICH version is on screen, so saying it once
	 * and completely is the whole job.
	 */
	static FString HistoryCaption(const FGcHistoryEntry& Entry);

	/**
	 * Can this source be written back into the asset at all?
	 *
	 * True for a git ref, a local-history instant and the working file; false for the unsaved editor
	 * object, which is not a version — it is the asset itself, and writing a thing onto itself is not
	 * an operation. Cheap by construction: it reads the source kind and nothing else, because
	 * `PrimaryActionForSelection` and every hover ask it.
	 */
	static bool CanRestoreFromSource(const FGcCompareSource& Source);

	/**
	 * Can the LEFT column be put back into the asset? What the footer's restore is about.
	 *
	 * `SourceToRestoreFrom` rather than `ComparedAgainst`, which is E11.11's first ruling applied to
	 * the one caller where getting it wrong restores from the wrong version.
	 */
	bool CanRestoreFromComparedSource() const;

	/**
	 * What the LIVE ASSET holds right now, as `.gc` text, for the selected file. Empty when it
	 * cannot be projected.
	 *
	 * **THE THIRD VERSION ON A TWO-COLUMN SCREEN, and E11.11 is the first pass that needs it.** With
	 * one side always the working file, "is this block what the asset has" was answered by the two
	 * texts on screen. Comparing `main` against last Tuesday puts the asset in neither column, and
	 * the question is still the one the reader is asking — *"which one is currently applied"*.
	 *
	 * **COMPUTED ONCE PER COMPARISON, NOT PER ROW AND NOT PER HOVER.** A projection loads the asset;
	 * `RenderCompare` asks for it once and every row reads the answer off its item. It re-uses a
	 * column when a column already IS the live projection (`editor (unsaved)`), so the common pairs
	 * cost nothing extra.
	 */
	const FString& CurrentTextForSelection();

	/**
	 * Plan a restore of `BlockName` (empty for every represented facet) from the LEFT column.
	 * **Reads the live asset; writes nothing.**
	 */
	bool BuildRestorePlan(const FString& BlockName, FGcRestorePlan& OutPlan, FString& OutError);

	/**
	 * The same, scoped to a SET of blocks — `Revert diffs`' plan. Empty means every facet.
	 *
	 * **A SET RATHER THAN A LOOP, AND IT WAS MEASURED BEFORE IT WAS CHOSEN.** E11.11 left this open:
	 * `BuildRestorePlan` took one block name, and re-planning per block would project the live asset
	 * once per block — for a `widget_blueprint` that is the whole widget tree read six times to write
	 * six blocks, which is E11.7b's per-item cost with a bigger item. `FGcRestore::Plan` was already
	 * doing a name test per block; taking a list there costs one `Contains` and one projection total.
	 */
	bool BuildRestorePlanFor(const TArray<FString>& BlockNames, FGcRestorePlan& OutPlan,
		FString& OutError);

	/**
	 * Arm a restore: plan it, dry-run it, and put its exact label and caveats on the footer.
	 *
	 * **TWO PRESSES, AND THE SECOND ONE IS THE CONSENT.** This is §6 step 4 and the same shape the
	 * apply already uses: the first press produces a preview naming every block that would be
	 * written, everything the target could not represent, and every entity the write would remove;
	 * the second press performs it. Anything that moves — a new selection, a different instant, a
	 * refresh — disarms it, because a confirmation is only safe while the thing that was previewed
	 * is still the thing that would happen.
	 *
	 * @param BlockName  empty for the whole asset; a block name for §7's surgical restore.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ArmRestore(const FString& BlockName);

	/** The same, over a set of blocks. */
	void ArmRestoreOf(const TArray<FString>& BlockNames);

	/**
	 * Arm `Revert diffs`: undo what changed between the two chosen versions and nothing else.
	 *
	 * **NOT A BLOCK COPY — see `BuildSessionRevertPlan`.** The scope is the window and the operation
	 * is the window's delta applied backwards over whatever the asset is NOW, so work done after the
	 * window survives and an entity the window ADDED is removed.
	 */
	void ArmSessionRevert();

	/** Dry-run a plan, print its caveats and hold it for the second press. The two arms share it. */
	void ArmPlan(FGcRestorePlan&& Plan);

	/**
	 * Plan the undo of one window: `merge(base = the newer side, ours = the live asset, theirs = the
	 * older side)`, then plan that document against the asset. **Reads; writes nothing.**
	 *
	 * An entity that changed inside the window AND again after it has no answer that does both, so
	 * it is left exactly as it is and named in the plan's caveats.
	 */
	bool BuildSessionRevertPlan(FGcRestorePlan& OutPlan, FString& OutError);

	/**
	 * Would the footer's control be `Revert diffs` rather than `Restore this file`?
	 *
	 * True exactly when NEITHER side is the current version, which is when a whole-file restore would
	 * undo more than the window on screen. It decides the label and the scope together, in one place,
	 * for the same reason `PrimaryActionForSelection` decides what the button does: two functions
	 * answering it separately is a button that says one thing and does another.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsRevertingDiffs() const;

	/**
	 * Did this asset come into existence, or go out of it, BETWEEN the two versions on screen?
	 *
	 * The colour of a row's name — green created, red deleted — and it is a fact about the WINDOW
	 * rather than about the file: whether it is on disk right now is the strike-through's question,
	 * and the two are drawn together because a file deleted inside the window is usually both.
	 */
	EGcRowExistence ExistenceAcrossComparison(const FGcSidecarStatus& Row) const;

	/**
	 * What a plan cannot restore, and what it will leave alone — the popup's two lists.
	 *
	 * **NOTHING IS RE-DERIVED HERE.** Every sentence comes off the plan: a refusal, a caveat, a
	 * block that is not included, a skip whose `errors { }` said why. The popup exists because those
	 * sentences were being spent on a footer that truncates, not because they were missing.
	 *
	 * Static and public so a spec can ask what the dialog WOULD say without a Slate window — the
	 * dialog itself answers "go ahead" unattended, which is the only way a headless suite can drive
	 * a modal at all.
	 */
	static void CollectConfirmItems(
		const FGcRestorePlan& Plan,
		TArray<FGcConfirmItem>& OutWillChange,
		TArray<FGcConfirmItem>& OutCannotRestore,
		TArray<FGcConfirmItem>& OutWillNotTouch);

	/**
	 * The same three lists for a project-wide revert.
	 *
	 * `OutWillChange` carries one row per asset the run would write, with its `.gc` line counts —
	 * user-directed 2026-08-06, *"showing all potential file and line changes"*. `OutLeftAlone` is
	 * the fourth answer and the one whose absence was a defect: an asset the plan considered and
	 * concluded nothing about used to appear in no list and no count at all.
	 */
	static void CollectConfirmItems(
		const FGcProjectRestorePlan& Plan,
		TArray<FGcConfirmItem>& OutWillChange,
		TArray<FGcConfirmItem>& OutCannotRestore,
		TArray<FGcConfirmItem>& OutWillNotTouch,
		TArray<FGcConfirmItem>* OutLeftAlone = nullptr);

	/** Perform the armed plan. Checkpoints live state first and refuses if it cannot. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void PerformArmedRestore();

	/**
	 * Aim the footer's restore at one block, or clear it. **Costs nothing** — see
	 * `PendingRestoreBlock`. Clicking the same block again lets go of it.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ChooseRestoreBlock(const FString& BlockName);

	/** Which block the footer would restore, or empty for every represented facet. */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	const FString& GetPendingRestoreBlock() const { return PendingRestoreBlock; }

	/** Which block of the comparison on screen this row's line belongs to. One parse, on demand. */
	FString BlockOwningRow(const UGcDiffRowItem& Row) const;

	// `BlockMatchesCurrent` IS GONE, AND ITS ABSENCE IS THE FIX (E11.13).
	//
	// "Does this differ from the asset" was answered in three places: per line on the row (the mark),
	// per block here, and per block again inside `FGcRestore::PlanBlocks` as `bDiffers`. E11.12 made
	// the mark per line and the three stopped agreeing — two specs failed on the seam. The planner
	// already projects the asset and decides what is worth writing, so this copy went and the press
	// lets the plan answer.

	/**
	 * Write exactly one block of **one side** into the asset, leaving every other block alone.
	 * E10.17, two-sided in E11.11.
	 *
	 * **THIS IS A MERGE, AND THAT IS NOT A METAPHOR** — *"reason through whether is it possible to
	 * merge the block with current block. I assume yes?"*, user-directed 2026-08-05, and yes.
	 * `gc_apply` has always written a document block by block against the LIVE asset: each block is
	 * routed to its own writer and a block nobody names is not touched. So applying the `properties`
	 * block of last week's version to an asset whose `graph` block moved this morning keeps both —
	 * the result is the merge of the two, performed by the writers rather than by a text merge.
	 *
	 * **THE SIDE IS AN ARGUMENT NOW, AND THE BUTTON THAT CALLS IT SITS ON THAT SIDE'S COLUMN.** With
	 * two versions on screen, "apply this block" is two different writes; the gesture names which by
	 * where it is, which is the same reason the take buttons live on their own values.
	 *
	 * **ONE PRESS, unlike the footer's restore, and the difference is what the reader has seen.**
	 * E11.7's two presses exist because a whole-file restore is offered from a DIFF, and the two
	 * questions a diff cannot answer are how much can be put back and what gets deleted. Here the
	 * gesture is on the block itself, in the row that is drawing its difference. What the press
	 * cannot show in advance it says afterwards: the result line names every entity the write
	 * removed.
	 *
	 * **`EntityPath` AIMS IT NARROWER, AND IT IS THE SAME WRITE.** A group is a declaring line plus a
	 * braced body, so one entity of a tree block is a writable unit — the payload becomes the live
	 * block with that entity taken from the chosen side, and everything else in the block is the
	 * asset's own text. Empty means the whole block, which is what a row outside a tree block gets.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ApplyBlockFromComparison(const FString& BlockName, bool bRightSide,
		const FString& EntityPath);

	/**
	 * Which blocks the two sides disagree about, in document order. The scope of `Revert diffs`.
	 *
	 * **IT IS THE WHOLE OF *"undo the specific session, not up until now"*.** A restore from a moment
	 * writes every block that moment can represent, which against a version from LAST week undoes
	 * this week as well. The blocks that differ between the two CHOSEN versions are exactly what
	 * changed in the window the reader picked out, and everything else in the asset is untouched.
	 */
	TArray<FString> BlocksDifferingBetweenSides();

	// ---- the project-wide revert (E11.9) --------------------------------------------------------
	//
	// **THE TIME MACHINE'S VERB.** E11.8 made the timeline show the project at a moment; this is what
	// puts it back. The footer's other restore acts on the SELECTED ASSET and this one acts on every
	// covered asset at once, which is why it is a second control rather than a mode of the first:
	// two gestures with the same button and wildly different blast radius is the worst shape a
	// control can have.

	/**
	 * Plan and preview a project-wide revert to the instant on screen. **Writes nothing.**
	 *
	 * Expensive by construction — it projects every covered asset — so it runs on a press and never
	 * on a refresh. The preview it leaves behind is §9's: what would be restored, created and
	 * removed, what is skipped and why, and whether the file backup is about to be replaced.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ArmProjectRevert();

	/**
	 * Perform the armed project revert. Three acceptances, and each one is a different question.
	 *
	 * The second press carries all three — removals, a partial scope, and replacing the backup —
	 * because the preview put all three in front of the reader. They are separate FLAGS on the
	 * backend so a caller that shows them separately can answer them separately.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void PerformArmedProjectRevert();

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsProjectRevertArmed() const { return bProjectRevertArmed; }

	/**
	 * What the compare zone's current source means as a project-wide revert target, if anything.
	 *
	 * **THE LAST COMMIT, A BRANCH, OR A MOMENT** — user-directed: *"revert can not only apply to
	 * timeline, but to git branches and head etc."* False only for `editor (unsaved)`, which is not
	 * a version: restoring the project onto its own current state is not an operation.
	 *
	 * Public because the button's visibility, its label and its effect all ask it, and a control
	 * whose visibility is decided somewhere other than its effect is the E10.12 defect again.
	 */
	bool ProjectRevertTarget(FGcProjectRestoreTarget& OutTarget) const;

	/** The armed plan, for a spec that wants to assert what the button is about to do. */
	const FGcProjectRestorePlan& GetArmedProjectRevert() const { return ArmedProjectRevert; }

	/**
	 * The collapsible section: what would be created, what removed, and what is skipped with why.
	 *
	 * **A SECTION RATHER THAN A SENTENCE, USER-DIRECTED:** *"have a collapsible section that shows
	 * the creation/deletion of the asset, but show them the error message, that they couldnt be
	 * recovered"*. Creations and removals are the two actions a person cannot undo by looking, and
	 * a skipped asset's reason is the one thing a summary count destroys — `3 skipped` is not
	 * information, and *"DA_Foo: the VRExpansion plugin is not installed"* is.
	 *
	 * Static and pure over the plan, so the same lines can be asserted without a widget.
	 */
	static TArray<FString> ProjectRevertDetailLines(const FGcProjectRestorePlan& Plan);

	/** Open or close the section. It starts CLOSED, and a plan with skips opens it once. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ToggleProjectRevertDetails();

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool AreProjectRevertDetailsOpen() const { return bProjectRevertDetailsOpen; }

	/** Is a restore previewed and waiting for its second press? */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsRestoreArmed() const { return bRestoreArmed; }

	/**
	 * Which of the two action controls is on screen — the footer's `PrimaryButton` and the diff
	 * column's `RestoreButton`.
	 *
	 * **THEY EXIST SO THE SPLIT CAN BE ASSERTED AGAINST THE SCREEN RATHER THAN THE ENUM.** The
	 * restore moved out of the footer on 2026-08-05, so `PrimaryActionForSelection`'s ordering
	 * ruling — a discretionary restore is never offered beside a required merge — now depends on
	 * `RefreshActions` drawing exactly one of two buttons. Reading the enum cannot see the case
	 * where it forgets to hide the other.
	 *
	 * NULL IS NOT SHOWN, which is the honest answer for a bare panel: the bindings are optional, and
	 * a control that does not exist is not one somebody can press.
	 */
	bool IsPrimaryButtonShown() const;
	bool IsRestoreButtonShown() const;

	/**
	 * Is this panel holding a decision that exists nowhere but in it? E11.10.
	 *
	 * An edited resolution and an un-applied merge are both destroyed by a refresh, which is right
	 * when a person asked for the refresh and is silent data loss when a background save asks for
	 * it. `HandleExternalChange` defers on this; nothing else may, because every other kind of stale
	 * state — an armed restore, a passed preflight — is supposed to be cleared by anything that
	 * moves.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsHoldingUnwrittenWork() const { return bResolutionHeld || !PendingMergeText.IsEmpty(); }

	/** The armed plan, for a spec that wants to assert what the button is about to do. */
	const FGcRestorePlan& GetArmedRestore() const { return ArmedRestore; }

	/** The sources on offer, in picker order. */
	const TArray<FGcCompareSource>& GetSourceOptions() const { return SourceOptions; }

	/**
	 * Choose the pair explicitly, refilter the list and redraw.
	 *
	 * The shipped panel goes through `CompareAgainst`, which puts one picked source opposite the
	 * working file. This is the two-sided form underneath it, kept because the renderer is genuinely
	 * generic over both sides and a spec should be able to say so.
	 */
	void SetCompareSources(const FGcCompareSource& Left, const FGcCompareSource& Right);

	/**
	 * Does the COMPARISON have anything to say about this file? The compare list is this, exactly.
	 *
	 * **IT DOES NOT LOOK AT `CompareMode`, AND THAT IS THE FIX FOR "IT HIDES THE OTHER UNRELATED
	 * FILE".** It used to switch on the mode, which was right when the panel had one list and the
	 * mode chose what that list was OF. E10.11 gave the decisions a list of their own and left the
	 * switch standing, so selecting a conflicted file — which moves the mode to `Conflicts` —
	 * refilled the COMPARE list with the conflict list's contents: files that really differed from
	 * the branch vanished, files that did not appeared, and the heading counted the wrong set.
	 *
	 * The conflict list asks `NeedsDecision`. This asks about the comparison. Neither asks the other
	 * question, and no mode changes either answer.
	 *
	 * PUBLIC because the heading over the list is a count of what this keeps, and the spec that
	 * checks the heading has to be able to compute the same subtraction independently.
	 */
	bool ShowsInView(const FGcSidecarStatus& Row) const;

	/** The revision each row's `+N −N` is measured against, or empty when git cannot measure it. */
	FString CountedAgainstRev() const;

	/** Read one side of the comparison. `OutCaption` is what that column ends up called. */
	bool TextForSource(const FGcCompareSource& Source, FString& OutText, FString& OutCaption,
		FString& OutError) const;

	/** The scan behind both lists. Public so a spec can check the lists against what was scanned. */
	const TArray<FGcSidecarStatus>& GetRows() const { return Rows; }

	/**
	 * Each decision, paired with the line of `Text` it decides. See `FGcInlineDecision`.
	 *
	 * Order and count are the input's: a decision whose site cannot be identified — a whole-block
	 * conflict, or one whose value no longer occurs exactly once — comes back with an empty
	 * `SiteLine` rather than being dropped. A decision that is not drawn is one nobody can take.
	 */
	static void LocateDecisions(const FString& Text, const TArray<FGcConflictView>& Decisions,
		TArray<FGcInlineDecision>& OutLocated);

	/**
	 * Is git holding BOTH sides of this file — is a merge in progress for it right now?
	 *
	 * Read off the porcelain code the row already carries, so it costs no process. It is the only
	 * condition under which `PreviewMerge` can answer, because the preview's three inputs are index
	 * stages and nothing else produces them.
	 */
	static bool IsMidMerge(const FGcSidecarStatus& Row);

	/**
	 * Which comparisons this file can actually answer, worst-first.
	 *
	 * A file with no conflicts has no ours-and-theirs and a file git has never seen has no last
	 * commit. Returning the legal set rather than disabling entries is what keeps the picker from
	 * being a list of ways to be told no.
	 *
	 * TWO DIFFERENT FACTS LIVE HERE AND CONFLATING THEM MADE THE RESOLVE VIEW UNREACHABLE.
	 * `conflict { }` constructs are in the FILE, and something has to have written a merge proposal
	 * into it for them to exist. Git's three stages are in the INDEX, and `.gitattributes` says
	 * `*.gc … merge=binary`, so a real `git merge` writes no proposal at all: it leaves the working
	 * file at one side verbatim and marks the path unmerged. A stuck file therefore reads as clean
	 * text, and offering it the view that parses its text was offering the one screen that could not
	 * answer.
	 */
	static TArray<EGcCompareMode> ModesFor(const FGcSidecarStatus& Row);

	/**
	 * A unified diff, paired into left/right cells.
	 *
	 * A run of removals followed by a run of additions is one edit shown twice, so the two runs are
	 * ZIPPED: removal *i* sits opposite addition *i*, and whichever run is shorter leaves empty cells
	 * at the end. Emitting them sequentially instead — every removal, then every addition — is what a
	 * unified diff already does, and reading it side by side would then mean reading it twice.
	 */
	static void PairDiff(const TArray<FGcDiffLine>& Lines, const FGcDiffViewOptions& Options,
		TArray<UGcDiffRowItem*>& OutRows, UObject* Outer);

	/** The old shape, kept because most callers only ever wanted the default context policy. */
	static void PairDiff(const TArray<FGcDiffLine>& Lines, bool bChangedLinesOnly,
		TArray<UGcDiffRowItem*>& OutRows, UObject* Outer);

	/**
	 * Mark which rows a next/previous-change button should jump between, in place.
	 *
	 * A CHANGE IS A RUN, NOT A LINE. Twelve consecutive `+` lines are one thing a person reads, and a
	 * button that stopped at each of them twelve times would be a scroll wheel with extra steps.
	 * Returns the anchors, in order.
	 */
	static void MarkChangeRuns(const TArray<UGcDiffRowItem*>& Rows, TArray<int32>& OutAnchors);

	/**
	 * Drop the `gc 1 …` header pair when the only thing that moved on it is `sig:`/`bin:`.
	 *
	 * Those two are digests OF the body, so they change on every edit without exception. The first
	 * line of every comparison was therefore always a difference, always the longest line on screen,
	 * and never information — it said "the body changed", which is what the rest of the diff is for.
	 * Anything else on that line is authored, and a change to it still shows.
	 */
	static TArray<FGcDiffLine> WithoutDerivedNoise(const TArray<FGcDiffLine>& Lines);

	// ---- the operations ------------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void Refresh();

	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SyncAll(bool bCheckOnly);

	/** Select by absolute sidecar path, or a repository-relative path for existing callers. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SelectFile(const FString& RepoRelativePath);

	/** Choose what is being compared, and redraw. Refuses a mode this file cannot answer. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SetCompareMode(EGcCompareMode Mode);

	/**
	 * Rebuild the comparison from the current file, mode and in-memory text.
	 *
	 * CALLED AFTER EVERY CHANGE, which is the point. Selecting a file, changing mode and taking a side
	 * all end here, so what is on screen is never one action behind — the old panel redrew only when a
	 * view button was pressed, so a resolve left the diff showing the state before it.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void RenderCurrentView();

	/**
	 * Put the merge preview's result into the `.gc` on disk. The asset is not touched.
	 *
	 * THE STEP THAT CONNECTS THE TWO HALVES OF THIS PANEL. The preview computes a merge from git's
	 * index stages and holds it in memory; the resolve view reads `conflict { }` constructs out of
	 * the working FILE. Until something wrote one into the other, a real merge could be previewed and
	 * never resolved — the resolve view had nothing on disk to read and said so.
	 *
	 * It is a decision rather than a step, and that is why the preview does not write itself: the
	 * merged text replaces what is in the working tree, and §6's rule that this panel never writes
	 * quietly applies to a `.gc` exactly as it applies to an asset.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void WriteMergeProposal();

	/**
	 * Take one side of one conflict, and PUT IT IN THE FILE.
	 *
	 * E10.6 — AND IT SUPERSEDES §4.9 AND §6's "the panel never writes quietly" FOR THIS ONE WRITE.
	 * The old shape was: take a side (memory), take another (memory), press Save resolution, press
	 * Check it applies, press Apply. Five gestures for one decision, and the first three of them left
	 * nothing anywhere — every writer in the project, `gc_resolve` included, reads the FILE, so until
	 * the save the decision had happened to nobody and no other tool could see it.
	 *
	 * What makes the write safe to fold in is a fact about where the panel is standing: this only ever
	 * runs on a file git is mid-merge on or one holding an unresolved construct, and **git is still
	 * holding all three original sides in its index**. Taking a side rewrites a projection whose
	 * inputs are all still recoverable, and `git checkout --merge <path>` puts it back. That is not
	 * true of the ASSET, which is why the apply below is still two deliberate steps.
	 *
	 * On the merge screen the first take also writes the merge itself, because the side is taken ON
	 * the merged text — so accepting a merge and deciding its first conflict are one gesture rather
	 * than two, which is what "shouldn't it just merge when I choose a side" was asking for.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void TakeSide(int32 ConflictId, bool bTakeTheirs);

	/**
	 * Take one side on EVERY remaining decision in this file, in one write.
	 *
	 * **USER-DIRECTED 2026-08-05:** *"add a button on each side to take all of theirs or all of
	 * ours."* Taking every decision the same way is the commonest ending of a real merge by a wide
	 * margin — and doing it a row at a time meant N clicks, N file writes and N redraws to express
	 * one decision somebody had already made before they opened the panel.
	 *
	 * **NOT A LOOP OVER `TakeSide`, AND THAT IS THE POINT.** That would write the file once per
	 * decision and re-scan between them; each of those intermediate files is a state no other tool
	 * in the project would understand, and a failure halfway through would leave one. This resolves
	 * against a running text and writes ONCE, through the same `ResolveConflict` primitive — so
	 * there is no second resolve engine, and the result is byte-identical to having clicked every
	 * row.
	 *
	 * It still ends where `TakeSide` ends: `RecordMergeSettled`, and `bAutoResolve`'s apply on the
	 * last decision. Taking every side IS deciding the merge, which is E10.7's ruling unchanged.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void TakeEverySide(bool bTakeTheirs);

	/** Show every unchanged line, or elide the long runs again. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SetShowAllLines(bool bShowAll);

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsShowingAllLines() const { return bShowAllLines; }

	/** Open one elided run, by the index its row carries. Nothing else on screen moves. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ExpandRun(int32 RunIndex);

	/**
	 * Scroll to the next (`+1`) or previous (`-1`) change, wrapping at either end.
	 *
	 * "I DON'T KNOW THE CONTEXT" IS WHAT THIS AND THE ELISION ANSWER TOGETHER. A diff that elides its
	 * unchanged runs is readable only if you can move between what it kept; a diff that shows every
	 * line is readable only if you can skip what has not changed. The pair is the whole control.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void GoToChange(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	int32 GetChangeCount() const { return ChangeAnchors.Num(); }

	/** Write the resolved text back to the `.gc` on disk. Still not the asset. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void WriteFile();

	/** Apply the `.gc` to its asset. A dry run is a check; a real apply is a separate decision. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void ApplyToAsset(bool bDryRun);

	/**
	 * What the one action button offers for the file selected right now. E10.12.
	 *
	 * The single source of truth for both the label and the action — see `EGcPrimaryAction`. Public
	 * because it is also the only way a spec can assert that a control the panel is DRAWING matches
	 * the thing pressing it would do, without a real mouse and a real window.
	 */
	EGcPrimaryAction PrimaryActionForSelection() const;

	/**
	 * Press it. What a click on `PrimaryButton` runs, exactly.
	 *
	 * Deliberately NOT a `UFUNCTION`: the button is bound in C++ and nothing in Blueprint needs to
	 * fire it, and every reflection change costs a full editor build to verify.
	 */
	void PressPrimaryAction();

	/**
	 * Put the panel's tab across the bottom of the editor, beside the Output Log.
	 *
	 * A CONTROL RATHER THAN A HOPE. With no berth in a saved layout a nomad tab opens as a floating
	 * window — which goes behind the editor and is then simply gone — and no layout extension can
	 * fix an editor whose layout already exists. `FGcVcsTab::Open` does this automatically the first
	 * time; this is the button for every time after, because a person who has dragged the panel
	 * somewhere and wants it back has no other way to say so.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void DockToBottom();

	/**
	 * Does taking the LAST side of a merge apply it to the asset? It does; this is the spec switch.
	 *
	 * **NAVIGATION IS NOT IN THIS FLAG ANY MORE — E10.12, and read the narrowing before widening it
	 * back.** It used to mean "does navigating write", because selecting a mid-merge file wrote the
	 * merge and selecting an unabsorbed proposal applied it. Both were deliberate (E10.7: *"deciding
	 * the last conflict IS asking for the merge"*) and both were wrong in the same way, reported by
	 * the one person using it: *"suddenly, navigating the items changed the conflict. can the whole
	 * operation be so fragile?"*. Looking at a thing may not change it — not because the writes were
	 * unsafe (git holds all three sides), but because a panel whose LIST CONTENTS move when you read
	 * a row cannot be reasoned about, and every surprise after that gets charged to it.
	 *
	 * So selection reads, and the two writes are `PrimaryButton` — one press, named for what it does.
	 * What remains here is the one write that follows an unambiguous gesture: answering the last
	 * decision of a merge. That is not navigation, and E10.7's ruling stands for it.
	 *
	 * A spec that means to prove that path leaves this on AND restores the bytes it touched.
	 * Nothing in the shipped panel ever turns it off.
	 */
	UPROPERTY(BlueprintReadWrite, Category = "GraphCode|VCS")
	bool bAutoResolve = true;

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	const FString& GetSelectedPath() const { return SelectedPath; }

	/**
	 * Is this the selected file? THE ROW ASKS RATHER THAN REMEMBERING.
	 *
	 * A `UListView` recycles its entry widgets, so a row that keeps a `bSelected` of its own carries
	 * the previous occupant's answer into the next one — the highlight lands on whichever row
	 * happens to reuse the widget, or on none. `NativeOnItemSelectionChanged` fixes it only for rows
	 * that are on screen when the click happens; a row scrolled into view afterwards was never told.
	 * The panel holds the one true answer, so the row reads it at the moment it is filled in.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	bool IsSelectedSidecar(const FString& SidecarPath) const
	{
		return !SidecarPath.IsEmpty() && SidecarPath == SelectedPath;
	}

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	EGcCompareMode GetCompareMode() const { return CompareMode; }

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	int32 GetUnresolvedCount() const { return UnresolvedCount; }

	/**
	 * Open the selected row's asset in its own editor.
	 *
	 * The panel names assets on every line and, until now, gave no way to reach one — a version-control
	 * view of a thing you cannot open is a report rather than a tool.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void OpenSelectedAsset();

	/**
	 * Go to the next thing that needs a decision, worst-first.
	 *
	 * Inside the selected file first — the next unresolved conflict, scrolled to — and when that file
	 * is done, on to the next file that needs attention at all. Wraps rather than stopping, so holding
	 * the button walks the whole repository's worth of decisions.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void GoToNextAttention();

	/** Slide both comparison columns. Clamped to the widest line the current view holds. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	void SetDiffPan(float Pixels);

	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	float GetDiffPan() const { return DiffPan; }

	/**
	 * A realized row reporting how far it could scroll.
	 *
	 * Only a laid-out `UScrollBox` knows its own scrollable extent, so the panel is told rather than
	 * measuring text in a font it would have to be given anyway. The widest row wins.
	 */
	void NotePanRange(float Range);

protected:
	virtual void NativeConstruct() override;

	/** Drops the change subscription. A panel in a closed tab must not still be rescanning. */
	virtual void NativeDestruct() override;

	/**
	 * Something outside this panel changed and the burst has gone quiet. E11.10, `FGcChangeNotice`.
	 *
	 * **THIS IS THE HOLE E10.13 LEFT.** The panel re-read when its tab was brought forward, which
	 * covered everything except the case a person actually hits: sitting with the panel in front of
	 * them while a save, an agent, or a deletion changed the thing they are looking at. Nothing
	 * announced it, so the view stayed wrong until they clicked away and back — reported directly,
	 * 2026-08-05.
	 *
	 * IT REFUSES WHEN THE PANEL IS HOLDING WORK NOBODY HAS WRITTEN. A refresh clears `SelectedText`
	 * and `PendingMergeText` by design — that is right when a person asked for it and is silent data
	 * loss when a background save asks for it, because the takes somebody made on a merge exist
	 * nowhere else. So a held resolution or an un-applied merge defers the rescan; the serial and
	 * the fingerprint remember that it is owed, and the next refresh that is safe pays it.
	 *
	 * **AN ARMED PLAN DEFERS TOO, AND THIS COMMENT SAID THE OPPOSITE UNTIL 2026-08-05.** The old
	 * ruling — a preview is a claim about a world that has just moved, so let it disarm — is right
	 * for a refresh somebody asked for and wrong for one nobody did. A save landing between the two
	 * presses disarmed the plan, the second press re-armed instead of performing, and the reported
	 * symptom was *"the revert/restore doesnt work"*. An arm is a gesture in progress; it lives for
	 * seconds, and the refresh it defers is still owed and still runs.
	 */
	void HandleExternalChange(const FString& Reason, uint64 Serial);

	/**
	 * Dragging the split handle.
	 *
	 * The panel does this rather than the handle, because a drag has to keep receiving moves after the
	 * pointer has left the two-pixel strip it started on — which is what mouse capture is for, and
	 * capture belongs to the widget that will still be under the pointer.
	 */
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonUp(
		const FGeometry& Geometry, const FPointerEvent& Event) override;

	UFUNCTION() FEventReply OnSplitHandlePressed(FGeometry Geometry, const FPointerEvent& Event);

	/** The footer's result line — what just happened, or why a button would refuse. */
	void SayResult(const FString& Line);

	/**
	 * The same line, said as the ANSWER TO A PRESS rather than as ambient commentary.
	 *
	 * **A SUCCESSFUL PRESS TAKES ITS OWN BUTTON OFF THE SCREEN, AND THAT IS WHAT A READER SEES.**
	 * Reported 2026-08-05: *"start merging — 2 to decide, when i press it it dissapeared"*. It had
	 * worked — the merge went into the file and the panel moved to the decisions — but every button
	 * here is named for a step, so finishing the step is precisely what removes it. The panel said so
	 * on `ResultLine`, in the dimmest ink it owns at the smallest size in the layout, at the bottom
	 * edge of the tab.
	 *
	 * So an outcome is drawn like one: a tick, the brightest ink, one size up, immediately under the
	 * button that has just gone. Ambient text — a preview's caveats, a refusal, the running
	 * commentary a redraw writes — goes back through `SayResult` and stays quiet, which is what keeps
	 * the loud version meaning something.
	 */
	void SayDone(const FString& Line);

	/** Ink and size for the result line, in one place so the two callers cannot disagree. */
	void StyleResultLine(bool bOutcome);

	/** Draw a set of prepared rows, with the caption over each column. */
	void ShowRows(const FString& LeftSide, const FString& RightSide,
		const TArray<UGcDiffRowItem*>& NewRows, const FString& EmptyMessage);

	/** Rebuild the details strip, the header chip and the footer from the current selection. */
	void RefreshSelectionUi();

	/** Group `Rows` worst-first and hand them to the list. The only writer of the file list. */
	void FillFileList();

	/** The conflict list: every file `NeedsDecision` keeps, ungrouped, marked as decision rows. */
	void FillConflictList();

	/**
	 * The line over the compare list: how many files it holds, and what they are being read against.
	 *
	 * ITS OWN FUNCTION BECAUSE THE TIME MACHINE MOVES THE LIST WITHOUT RESCANNING. It used to be
	 * spelled inline at the end of `Refresh`, which was fine while every change to the list came
	 * from a scan; moving the moment changes which rows are listed out of data already in memory,
	 * and a heading left behind by that would be a count of a list that is no longer on screen.
	 */
	void RefreshChangesHeading();

	/**
	 * THE LEDGER OF MERGES THIS PANEL HAS FINISHED — the one fact neither the file nor git can hold.
	 *
	 * Lines of `<repo-relative-path>\t<base>:<ours>:<theirs>` under `Saved/GraphCode/`, which is
	 * gitignored, because this is a note about the checkout rather than about the project. Keyed on
	 * the stage triple so it expires by itself: a different merge on the same path has different
	 * stages and the record simply stops matching. Nothing prunes it and nothing needs to.
	 */
	void LoadMergeLedger();

	/**
	 * Ask git for the stage triples — but only when the scan found a path it is stuck on.
	 *
	 * `git ls-files -u` is 60 ms and answers "nothing" on every checkout nobody is merging in, which
	 * is nearly all of them. `git status --porcelain` has already said which paths are unmerged, so
	 * the expensive question is only asked when its answer can be non-empty.
	 */
	void RefreshUnmergedStages();

	/** Set `bMergeSettled` on every row the ledger still vouches for. After the scan, before the lists. */
	void MarkSettledMerges();

	/** The whole file, every time — it holds one line per merge in flight. Both writers use it. */
	void WriteMergeLedger() const;

	/** Remember that this path's merge is decided. A no-op when git is not mid-merge on it. */
	void RecordMergeSettled(const FString& RepoRelativePath);

public:
	/**
	 * Forget that this path's merge was decided — the inverse, and it exists because a teardown
	 * that restores BYTES does not restore this.
	 *
	 * The ledger is keyed on the stage triple so it expires when a DIFFERENT merge lands on the
	 * path. Putting the same merge back is not a different merge: the write gate settles a fixture
	 * and then restores the file it settled, and the record went on matching — so the next run met
	 * a file that looked untouched and a panel that correctly said there was nothing left to
	 * decide. There is no fact on disk that could tell those apart (take `ours` throughout and the
	 * result IS ours), which is the whole reason the ledger exists, so anything that rewinds a
	 * merge has to rewind this too. `Docs/tools/merge-fixture.sh` does the same for a re-plant.
	 */
	void ForgetMergeSettled(const FString& RepoRelativePath);

protected:


	/** Decide which single action the current state is asking for, and label it. */
	void RefreshActions();

	/**
	 * Draw the project-revert button and its collapsible section. Called by `RefreshActions`.
	 *
	 * SEPARATE FROM `RefreshActions`' SWITCH because it is not part of it: that switch chooses ONE
	 * action for the selected file and returns from each case, and folding a second, independent
	 * control into it would mean the project button's visibility depended on which case the file
	 * happened to be in.
	 */
	void RefreshProjectRevertControls();

	/**
	 * The header's four numbers, from the rows in hand.
	 *
	 * NUMBERS AND COLOURS RATHER THAN A SENTENCE. The header used to carry one button whose label was
	 * a phrase — `3 decisions left`, then `2 files need you` — and a phrase that changes shape is
	 * something a person has to re-read every time. A count in the colour of what it counts is read
	 * without being read: amber is a decision, green is new, red is gone.
	 */
	void RefreshCounts();

	/**
	 * Repopulate the picker with the modes this file can answer, keeping the choice if it survives.
	 *
	 * @param bLandOnWorst  a NEW selection, so a file that is stuck may override the kept choice.
	 *                      A rebuild of the same file passes false: being yanked out of the
	 *                      comparison you were reading, by a refresh you asked for, is its own bug.
	 */
	void RefreshModes(bool bLandOnWorst);

	/** Fill both category pickers, and put each on the category its own side's source belongs to. */
	void RefreshCategoryBox();

	/**
	 * Fill each side's target picker with its category's targets, or hide it when the category has
	 * one target or none to pick from (`History`, whose target is the time machine).
	 */
	void RefreshTargetBox();

	/**
	 * Fill each side's commit picker from its branch, or hide it away from `git branch`. E11.26.
	 *
	 * A CLOSED BOX HOLDS ONLY WHAT IT CAN ANSWER FOR NOTHING, exactly as the target box does: before
	 * anybody opens it the row shows the one entry the side is standing on, and the list arrives on
	 * the opening gesture.
	 */
	void RefreshCommitBox();

	/** One side of the pair above. */
	void RefreshCategoryBoxSide(bool bRightSide);
	void RefreshTargetBoxSide(bool bRightSide);
	void RefreshCommitBoxSide(bool bRightSide);

	/** Draw both sides' time rows from `TimelineInstants`, and the note under them. */
	void RefreshTimeMachine();

	/** One side of it: the steps' reachability and the box's contents, or the row hidden. */
	void RefreshTimeRowSide(bool bRightSide);

	/**
	 * Where one side's step would land, or null when it has nowhere legal to go.
	 *
	 * ONE ANSWER FOR TWO QUESTIONS — whether to draw the button, and what pressing it does. Two
	 * copies of "is there a stop that way" is how a control ends up on screen doing nothing.
	 */
	const FDateTime* NextStopFor(bool bRightSide, bool bOlder) const;

	/**
	 * The same question for the block pair: the near edge of the block across the next silence.
	 *
	 * Separate from `NextStopFor` because the two disagree about what a step IS, not about how far
	 * it goes — this one walks stop by stop looking for a GAP, and stops at the other end rather
	 * than searching past it.
	 */
	const FDateTime* NextBlockFor(bool bRightSide, bool bOlder) const;

	/** What both public verbs are. `bWholeBlock` picks the finder; everything else is shared. */
	void StepTo(bool bRightSide, bool bOlder, bool bWholeBlock);

	/**
	 * Rebuild `TimelineInstants` and `HistoryLatest` from ONE glob over the archive names.
	 *
	 * Called where the branch scope is computed, and only for a history comparison, for the same
	 * reason: both answer "which files does this comparison list", and two places deciding that is
	 * how the list and its heading came to disagree once already (E10.11a).
	 *
	 * **A STOP IS A MOMENT THE PANEL CAN ANSWER AT, AND THAT IS TWO CONDITIONS** — user-directed
	 * 2026-08-05: *"if a moment cannot say anything, there shouldn't be the moment at all"*.
	 * Archives alone made 88 stops of which about 25 listed nothing and 60 refused outright, so
	 * stepping the timeline walked mostly through moments with nothing to show:
	 *
	 *  - **at or after `EarliestRestorableInstant`**, because `KnowledgeAt` answers `Unknown` for
	 *    EVERY asset before the tracking stamp — a stop there is a stop that refuses;
	 *  - **held by a package that still exists**, because a moment whose only archives belong to
	 *    deleted automation fixtures lists nothing and reads as *"nothing has changed since"*.
	 *
	 * **AND ONE STOP PER SECOND, NAMED BY THAT SECOND'S EARLIEST ARCHIVE** (E11.24). A burst is one
	 * thing that happened; naming it by its earliest keeps the whole burst inside the moment, since
	 * §3.2 resolves a package to its first archive at or after the stop.
	 */
	void RefreshTimelineInstants();

	/**
	 * Add a row for every package the STORE has history for whose sidecar is no longer on disk.
	 *
	 * The scan walks files, so a deleted asset has no row — which makes the one thing a time machine
	 * is for unreachable: its text is in the store and the panel offers no way to select it. Only
	 * for a history comparison, because only the timeline can answer for a file that is not there.
	 */
	void AppendHistoryOnlyRows(TArray<FGcSidecarStatus>& InOutRows) const;

	/** Read the working file into `SelectedText` unless a resolution is already held in memory. */
	bool EnsureSelectedText(FString& OutError);

	/**
	 * Apply the selected `.gc`, and hand back what to say about it. Touches no UI.
	 *
	 * Split out because the apply now has two callers with different sentences to write: the footer
	 * button, which says what happened, and the automatic apply at the end of a merge, which has to
	 * say what happened AFTER saying which side was taken. Two copies of the refusal-formatting is
	 * how one of them ends up not mentioning the refusals.
	 */
	bool RunApply(bool bDryRun, FString& OutBody);

	/**
	 * Re-project the selected `.gc` when it is still calling itself a proposal it has already
	 * delivered. Safe exactly here — see the body — and a no-op otherwise.
	 *
	 * @return true when it rewrote the file, so the caller knows to rescan.
	 */
	bool SettleAppliedProposal();

	/**
	 * Bring git's merge into the `.gc`. **The button's, not selection's — E10.12.**
	 *
	 * Until this runs, the merge exists in this process and nowhere else: `gc_resolve`, every writer
	 * and the panel's own resolve view are all looking at a file that says nothing about it. Under
	 * `merge=binary` that file is ours verbatim, so there is nothing on disk to read the decisions
	 * out of — which is why this step exists at all and why it cannot be skipped.
	 *
	 * It ran on SELECTION for one day (E10.7/E10.8) and that is what *"navigating the items changed
	 * the conflict"* was. The write itself is safe — git holds all three original versions in its
	 * index until the merge is committed, so `git checkout --merge <path>` puts it back — and safe
	 * was never the complaint. Reading a row moved the row, and moved which rows the OTHER list held
	 * (the E10.12 report), and a panel that does that cannot be predicted by the person using it.
	 *
	 * Computes the merge if nothing is pending, and never recomputes over a side already taken.
	 */
	void WriteMergeForSelection();

	/**
	 * Put a `.gc` its asset has not absorbed into the asset. **The button's, not selection's.**
	 *
	 * Same text arriving by any door — this panel's own merge, `gc_resolve`, the merge driver in a
	 * shell, a hand edit. One press: it runs in an `FScopedTransaction`, leaves the asset dirty and
	 * UNSAVED and undoable, and says what it did. A confirm step on top would be the panel asking
	 * permission for the thing the reader just asked for, which is the objection E10.7 was right
	 * about; what E10.7 got wrong was answering it by removing the button rather than the confirm.
	 *
	 * A refusal is reported and not retried — a `data_asset`'s `values` block has no writer, and
	 * that boundary must be readable rather than re-run on every redraw.
	 */
	void ApplyProposalForSelection();

	// ---- header --------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> HeaderBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RepoLine;

	/**
	 * `Refresh` IS GONE — user-directed, 2026-08-05: *"there shouldnt even be a refresh button"*.
	 *
	 * A control whose whole content is "show me what is actually there" is the panel conceding that
	 * what it is showing might not be, and it puts the job of noticing that on the reader. The panel
	 * re-reads when its tab is brought forward (`FGcVcsTab::MakePanelTab`) and at the end of every
	 * action it takes. `Refresh()` is still the API and still what both of those call.
	 *
	 * Not a contradiction of E10.12: that ruling is about writes. A refresh reads.
	 */

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UButton> SyncAllButton;

	/**
	 * `Check only` IS GONE, AND IT WAS NOT A LAYOUT DECISION.
	 *
	 * It ran `FGraphCodeGenerator::Run(bCheckOnly)`, which LOADS every asset in the project to
	 * re-project it, on the game thread. Pressing it froze the editor for as long as that takes —
	 * "pressing check only lags the whole thing" — and what it bought was `Sync all` with a flag,
	 * beside `Sync all`. A control that stops the editor to tell you what another control would have
	 * done anyway is not worth the row it sits in. `SyncAll(bCheckOnly)` is still the API.
	 */

	/**
	 * `Dock bottom` IS GONE TOO — user-directed, same message: *"remove dock button also, because it
	 * didnt do anything"*.
	 *
	 * And it did nothing for a reason worth keeping: `DockToBottom` only moves a tab that is
	 * FLOATING, and E10.7 already made `FGcVcsTab::Open` catch that case and dock it automatically.
	 * So by the time anybody could press this, the one state it could fix had been fixed on the way
	 * in — leaving a button that is inert in every situation a person can actually reach it from.
	 * `DockToBottom()` stays, because the automatic catch is its real caller.
	 */

	/**
	 * THE COUNT IS THE BUTTON.
	 *
	 * This was a control labelled with a changing phrase — `3 decisions left`, `2 files need you` —
	 * sitting beside three other labelled buttons, and the row read as four things to choose between.
	 * It is one amber number now: how many decisions are waiting, and pressing it goes to the next
	 * one. Absent at zero, like every other control on this panel that would have nothing to do.
	 *
	 * Worst-first, and it crosses files: the next unresolved conflict in the selected file, then the
	 * next file that needs attention at all.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UButton> NextButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> NextLabel;

	/**
	 * How many files are changed, new and deleted, in the colour each of those means.
	 *
	 * Not buttons. There is nowhere useful for "5 changed" to go — the list below already is that
	 * navigation — and a number that looks pressable and is not is the defect E10.2 removed from
	 * this same header.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> StatChanged;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> StatNew;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> StatDeleted;

	// ---- the details strip is gone ---------------------------------------------------------------
	//
	// `KIND — SIDECAR unapplied merge — git stopped mid-merge — GIT both changed it (UU) — NOT
	// COMPARED 11 parts: …`, permanently, in the row above the work. Four labelled fields of
	// vocabulary, and the reader's verdict on all of it was "too verbose, but nothing lands".
	//
	// Both halves of that are fair. Every field restated something the screen already showed — the
	// kind is in the row's tooltip, the sidecar state is the group heading the file is filed under,
	// the git state is now the title's COLOUR and the `+N`/`−N` beside it — and the one fact that was
	// genuinely only here, `omits:`, was a caveat nobody could act on written in the format's own
	// tokens. §6 requires uncompared facets to stay in the reading path; that is now one short clause
	// on the selection line, which is on screen in every view, and the full list is its tooltip.

	// ---- left: changes -------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> LeftPane;

	/**
	 * The left pane's width, and the thing the drag handle drives.
	 *
	 * A `USizeBox` rather than a fill proportion because UMG SHIPS NO SPLITTER — there is no
	 * `USplitter` in `Components/`, so a resizable pane is a width somebody has to own. Slate has
	 * `SSplitter` and this panel cannot reach it without becoming Slate.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<USizeBox> LeftPaneSize;

	/** The grab strip between the panes. Two pixels wide, and the only reason the panel needs mouse capture. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> SplitHandle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> ChangesHeading;

	/**
	 * THE CONFLICT ZONE — its own caption and ITS OWN LIST, above the comparison, absent when empty.
	 *
	 * E10.10 made it a zone; **E10.11 made it a list**, user-directed, and the reason is that the
	 * two zones are not two views of one list. *"compare and conflict might have the same file"* —
	 * they can and they do, and a button that SWITCHED between them meant reading either one cost a
	 * click and hid the other. Both are on screen now, each showing only what it is about.
	 *
	 * So there is no `Decide` button any more and no zone-is-active bullet: nothing is being
	 * switched between. Selecting in this list puts the panel on the decision screen; selecting in
	 * the compare list below puts it on the comparison. The control IS the list.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UVerticalBox> ConflictZone;

	/** `CONFLICT (3)` — the count is in the caption, because the list under it is the detail. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> ConflictCaption;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UListView> ConflictList;

	/**
	 * TWO SIDES, EACH A CATEGORY AND A TARGET. E11.11, user-directed: *"make it left and right
	 * selection, with 2 rows when selected"*.
	 *
	 * **THIS IS NOT E10.11 UNDONE, AND THE DIFFERENCE IS WHAT THE OFF-DIAGONAL IS FOR.** E10.11 cut
	 * a free left and right down to one picker, user-directed, on the argument that *branch against
	 * branch answers a question about two commits neither of which is in front of you*. That was
	 * right about GIT and wrong about the timeline, which E11.4 had not shipped yet: *"they can view
	 * two timelines to see what has changed in that time and undo the specific session, not up until
	 * now"*. Two versions neither of which is now IS the question — it is the only way to ask what
	 * one afternoon changed — and `Revert diffs` is the verb that makes the answer actionable.
	 *
	 * **WHAT E10.11 ACTUALLY COST IS PAID FOR HERE.** Its complaint was a permanent *"which box is
	 * which side"* on a 300-pixel column. The boxes carry a caption each now (`from` / `to`), the
	 * second row of a side appears only when that side's category has more than one target, and the
	 * `Head` category is gone — folded into `Branch` as a target — so the common state is two boxes
	 * and two captions rather than four boxes.
	 *
	 * **ALL FOUR `BindWidgetOptional`**, and the reason is the ordering hazard the handoff's
	 * "Verification recipe" documents: a required binding means the C++ and the layout asset have to
	 * land in one step, and the step in between is a panel whose Blueprint will not compile. Every
	 * use of every control here is null-checked anyway — a spec constructs this class bare — so what
	 * a required binding buys is a compile error instead of a missing control, and it costs a window
	 * in which the shipped panel is broken.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UComboBoxString> CompareLeftCategoryBox;

	/**
	 * WHICH ONE, within the left side's category.
	 *
	 * **HIDDEN UNLESS IT HAS SOMETHING TO CHOOSE BETWEEN**, which is the same rule every other
	 * control on this panel follows. `History`'s target is a moment rather than a list, so the time
	 * machine takes this row instead; `Branch` and `Current` both hold two or more, so both get one.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UComboBoxString> CompareLeftTargetBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UComboBoxString> CompareRightCategoryBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UComboBoxString> CompareRightTargetBox;

	/**
	 * WHICH COMMIT on that side's branch — the third row, E11.26.
	 *
	 * `GC-LOCAL-HISTORY-DESIGN.md` §5.3. Before this a side pointed at git could reach exactly two
	 * commits, `HEAD` and a branch's tip; every commit behind them was unreachable from the panel.
	 * It is a ROW under the branch box rather than a fourth category because the first thing a commit
	 * list needs is *commits of which branch*, and a category would have replaced the control that
	 * answers that.
	 *
	 * SHOWN ONLY UNDER `git branch`, like every other control here that has nothing to say elsewhere.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UComboBoxString> CompareLeftCommitBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UComboBoxString> CompareRightCommitBox;

	/**
	 * `Compare from:` and `to`, over their own side's boxes.
	 *
	 * THE ONE THING E10.11 SAID A TWO-SIDED PICKER COULD NOT HAVE ON THIS COLUMN. Four unlabelled
	 * boxes really are unreadable; two words are eight pixels of height and remove the whole
	 * question. They also say which side a `+` is measured against, which no other text on the left
	 * pane does.
	 *
	 * **THE SECTION CAPTION IS THE LEFT SIDE'S CAPTION NOW** — user-directed 2026-08-05: *"make
	 * COMPARE and from together, becoming Compare from:"*. `COMPARE` on its own line said which
	 * section this was and then `from` said it again one line lower; one line says both.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> CompareLeftCaption;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> CompareRightCaption;

	// ---- the time machine, one row per side -------------------------------------------------------
	//
	// **A DATE AND A TIME, NOT A DROPDOWN OF INSTANTS** — user-directed 2026-08-05: *"global time
	// machine should be a time picker"*. E11.4's combo listed the SELECTED ASSET's saves, so the
	// question it could answer was "what did this file look like before its third-from-last save".
	// The question somebody actually has is *"what did the project look like on Tuesday afternoon"*,
	// and no per-asset list can be asked that: the instants differ per file, and the answer for the
	// file you are not looking at is the whole point.
	//
	// **BUILT FROM PRIMITIVES BECAUSE THERE IS NOTHING TO USE.** `ui_search` over this editor found
	// no native UMG date, time or calendar control — measured, recorded in §5, and not worth
	// re-running. So a side's moment is a box you type into with a step either side of it, and the
	// steps land on the adjacent instant the store actually holds.
	//
	// **ONE ROW PER SIDE, WHICH DELETED THE DRIVEN END** (E11.16, user-directed: *"instead of so
	// complicated, having buttons and text field and confusing as such, make it such that, below
	// local history is a text field for time, below also, if it's also time based"*). The shared
	// control had to name the end it was pointed at, so it grew a readout, brackets, two pressable
	// halves and a sentence under them explaining the arrangement. A box in the side's own column is
	// the same capability with none of that: the row IS which end it moves.

	/** The left side's steps and typed moment. Present only while that side is on `local history`. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UHorizontalBox> LeftTimeRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UHorizontalBox> RightTimeRow;

	/**
	 * One stop further back, and one stop forward, for that side.
	 *
	 * THEY LAND ON REAL INSTANTS RATHER THAN STEPPING A FIXED AMOUNT. Between two adjacent archives
	 * every moment produces the same project, so a "minus one hour" button would spend most of its
	 * presses redrawing the identical screen — and the one press that mattered would be
	 * indistinguishable from the ones that did not.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> LeftTimeBackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> LeftTimeForwardButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> RightTimeBackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> RightTimeForwardButton;

	/**
	 * The block pair, on their own row under the typed box — user-directed 2026-08-06: *"so four
	 * buttons, maybe next and prev will be in next row"*.
	 *
	 * **A SECOND SIZE OF STEP, BECAUSE ONE SIZE CANNOT BE BOTH.** `<` and `>` are the right grain
	 * for reading through a sitting and the wrong grain for leaving one: a store records hundreds of
	 * moments inside an afternoon's work and one press each is not a way out of it. These cross the
	 * silence between sittings — see `NextBlockFor` for what counts as one.
	 *
	 * Their own row rather than the ends of the existing one: five controls on a 300-pixel column
	 * leaves the typed box too narrow to hold the date it must accept.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UHorizontalBox> LeftTimeBlockRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UHorizontalBox> RightTimeBlockRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> LeftTimeBackBlockButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> LeftTimeForwardBlockButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> RightTimeBackBlockButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> RightTimeForwardBlockButton;

	/**
	 * Type a moment, and read the one this side is showing.
	 *
	 * IT ACCEPTS WHAT THE PANEL PRINTS, first and foremost — `05 Aug 2026 14:32`, or `today · 14:32`
	 * — because the first thing anybody types into a box like this is the thing written above it.
	 * ISO-8601 and `YYYY-MM-DD HH:MM` are accepted too; a bare `14:32` means today.
	 *
	 * **IT IS ALSO THE READOUT, WHICH IS WHY THERE ISN'T ONE.** User-directed: *"remove the today
	 * 23:24, it's redundancy and could write inside the selection menu itself"*. The box already
	 * holds the chosen moment in the form it accepts — printing that same moment on a line above it
	 * was the panel saying one fact twice and spending a row on the second copy.
	 *
	 * **AN UNREACHABLE MOMENT IS ANSWERED WITH THE RANGE, NEVER WITH A REFUSAL.** Same user
	 * direction: *"if not available, tell them it's only valid from xxx time"*. A store that cannot
	 * go back that far is not a typing mistake, and "invalid" would send somebody to check their
	 * date format when the answer is that the record does not reach that week.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<class UEditableTextBox> LeftTimeEntry;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<class UEditableTextBox> RightTimeEntry;

	/**
	 * How far the timeline reaches and why it stops there — the sentence `EGcHistoryKnowledge`
	 * exists to make sayable, at the level of the whole project.
	 *
	 * IT NAMES `EarliestRestorableInstant`, WHICH IS THE LATER OF THE STAMP AND THE CUTOFF. Naming
	 * the oldest surviving archive instead advertised reach the store then refused at, which is what
	 * *"most timeline said this"* was about: 60 of 88 stops were before the tracking stamp and every
	 * asset answered `Unknown` at all of them.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> TimeMachineNote;

	/**
	 * The whole left column, scrolling as one.
	 *
	 * **USER-DIRECTED 2026-08-05: *"the scroller is bad, consider a much smaller scroller, and
	 * should scroll across the whole left panel"*.** The only thing that scrolled was the file list,
	 * so the compare zone and the revert controls above it took whatever height they wanted and the
	 * list got the remainder — on a short pane that is a list two rows tall under three quarters of
	 * a screen of controls. The bar is 4px and belongs to the pane rather than to one control in it.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UScrollBox> LeftPaneScroll;

	/**
	 * What gives the file list a height inside that scroll box.
	 *
	 * A `UListView` in an unbounded slot has no extent to arrange into, so it draws a row or two and
	 * hides the rest behind a scrollbar of its own — two bars over one list. The box is sized to the
	 * rows the list actually holds (`FillFileList`), which leaves the pane's bar as the only one.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<USizeBox> FileListSize;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UListView> FileList;

	// ---- centre and right: the comparison ------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> DiffPane;

	/** Always names the concrete source, never "current" — brief §4's comparison-source rule. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> LeftSideName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RightSideName;

	/**
	 * THE COMPARISON'S OWN TOOLBAR IS GONE, AND THAT IS FOUR CONTROLS SUBTRACTED.
	 *
	 * It carried `Show every line`, `Prev`, `Next` and a counter reading `1 change`, above a
	 * comparison that is at most a few hundred rows and has a scroll wheel. Each of them answered a
	 * question the list already answers: an elided run says how many lines it swallowed and opens
	 * when it is clicked, which is `Show every line` exactly where it is wanted rather than as a
	 * global mode; and a decision is drawn in amber where it occurs, which is what `Next` was for.
	 * `SetShowAllLines`, `GoToChange` and `MarkChangeRuns` all survive as API — the Next button in
	 * the header still uses them to walk decisions — they simply have no chrome of their own.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UListView> DiffList;

	/** What the empty list means. A blank pane and "no differences" are not the same answer. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> EmptyMessage;

	/**
	 * Horizontal pan, driving BOTH diff columns at once.
	 *
	 * A `.gc` header line runs to about two hundred characters and a column holds forty. Without this
	 * the choice is between text that overflows into the other side — which is what shipped, and it is
	 * why the pane was unreadable — and text cut off with no way to reach the rest.
	 *
	 * A `USlider` rather than a `UScrollBar`, because UMG's scroll bar is an INDICATOR: it has
	 * `SetState` and no input delegate at all, so it can report a scroll position and never originate
	 * one. It is built to be driven by a `UScrollBox`, and a scroll box cannot drive two halves of a
	 * virtualized list in step.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<USlider> DiffPanBar;

	/**
	 * TAKE ALL OF OURS / TAKE ALL OF THEIRS — one per column, **below the diff, not above it.**
	 *
	 * User-directed 2026-08-05: *"add a button on each side to take all of theirs or all of ours"*,
	 * then *"maybe not header, but below"*. Below is right and it is not only taste: the caption row
	 * is the panel's answer to *which two versions am I looking at*, and hanging the most
	 * consequential control in the view off it would make reading the answer and pressing the button
	 * the same gesture's neighbours. Under the diff, the buttons sit after the evidence — which is
	 * the order somebody actually decides in.
	 *
	 * Each is half-width and lines up with the column it takes, so the button under `Theirs` takes
	 * theirs. Present only while there is something to decide; a take-all with nothing to take is a
	 * control whose whole content is a refusal.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UHorizontalBox> TakeAllRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> TakeAllLeftButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> TakeAllLeftLabel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> TakeAllRightButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> TakeAllRightLabel;

	// ---- footer --------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UBorder> FooterBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> SelectionLine;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> ResultLine;

	/**
	 * The action the current state is asking for, or collapsed when it is asking for none.
	 *
	 * IT IS ALSO THE CONFIRMATION, AND THAT IS WHY THERE IS ONLY ONE BUTTON NOW. The apply used to
	 * be two controls in the footer — `Check it applies`, which ran a dry run, and `Apply to asset`,
	 * which appeared beside it afterwards. Two buttons, one of which is absent until the other has
	 * been pressed, is a sequence drawn as a choice: the second one materialises somewhere the eye
	 * was not, and the first one is named after an internal step rather than after what a person
	 * wants. One button that says `Apply to asset`, runs the preflight, and then says
	 * `Confirm — apply for real` is the same two deliberate steps in the place the hand already is.
	 *
	 * The two-step is NOT folded away. §6's rule that this panel never quietly writes an asset is
	 * exactly what the confirm state is: nothing reaches the asset until the same button is pressed
	 * a second time, and any change at all — a take, a refresh, a different comparison — disarms it.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UButton> PrimaryButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> PrimaryLabel;

	/**
	 * Restore this file from the version on the LEFT — and it sits under that column for that reason.
	 *
	 * **IT IS A PLACE, NOT A SIXTH STATE OF `PrimaryButton`.** User-directed 2026-08-05: the
	 * revert-file control belongs under the `before` column. A restore is the one action here whose
	 * argument is a COLUMN — it takes the left side and puts it into the asset — so a footer button
	 * far from that column names its source in words (`Restore this file from project at today ·
	 * 14:36:24...`) where the layout could have said it by adjacency. The label is short now because
	 * the position carries the rest.
	 *
	 * **AND THE ORDERING RULING SURVIVES INTACT**, which is the thing a naive split would have
	 * broken. `PrimaryActionForSelection` puts `Restore` LAST on purpose: a restore is discretionary
	 * and everything above it is work the file is blocked on, so offering "put an older version back"
	 * beside "start merging" would put a chosen write in front of a required one. That still cannot
	 * happen — one function still decides, and it now chooses WHICH control is drawn rather than what
	 * the single control says. This button is shown exactly when that function answers `Restore`, so
	 * it is never on screen beside a required action.
	 *
	 * `BindWidgetOptional`, the same trade the project-revert controls document: a required binding
	 * would mean the C++ and the layout asset have to land in one step.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> RestoreButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> RestoreLabel;

	/**
	 * Open the selected asset in its own editor.
	 *
	 * A BUTTON RATHER THAN A CLICK. Selecting a row and opening its asset were briefly the same
	 * gesture, and it was wrong: clicking a file to read its diff is the commonest thing anybody does
	 * here, and it should not tear open an asset editor every time. Opening is a decision, so it gets
	 * a control.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "GraphCode|VCS")
	TObjectPtr<UButton> OpenButton;

	/**
	 * Save the selected asset — **on screen only while there is something to save.** E11.11,
	 * user-directed: *"maybe beside open asset will be save asset, when it can save"*.
	 *
	 * **IT IS THE OTHER HALF OF A SENTENCE THE PANEL HAS BEEN SAYING SINCE E10.7.** Every write here
	 * ends *"the asset is dirty and UNSAVED — Ctrl+Z undoes it"*, which is the right guarantee and
	 * leaves the reader with a keystroke to find and a window to go and find it in. A block apply
	 * makes that worse rather than better: it is a small, cheap, repeatable gesture, so a session is
	 * now several of them against one asset, and the thing you do at the end of them has no control
	 * on this panel at all.
	 *
	 * **PRESENT ONLY WHEN THE PACKAGE IS DIRTY**, which is the same rule as every other control here
	 * and is what keeps it from reading as a second Apply. A save button that is always there invites
	 * pressing it to find out whether anything needed saving; one that appears when the asset moves
	 * is a statement that it has.
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> SaveButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> SaveLabel;

	// ---- the project-wide revert (E11.9) --------------------------------------------------------
	//
	// **PRESENT ONLY WHILE THE COMPARISON IS THE TIMELINE.** A project revert is against a MOMENT,
	// and the other three categories do not have one: `last commit` and a branch are git's job and
	// `editor (unsaved)` is not a version at all. A button that was on screen for all four would be
	// a refusal three quarters of the time.
	//
	// All `BindWidgetOptional`, the same trade `CompareTargetBox` documents: a required binding
	// means the C++ and the layout asset have to land in one step and the step between them is a
	// panel whose Blueprint will not compile.

	/** Arms the preview, then performs it. One button, two presses — the restore's own shape. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> ProjectRevertButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> ProjectRevertLabel;

	/** Opens and closes the created/removed/skipped list. Collapsed when there is nothing in it. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UButton> ProjectRevertDetailsToggle;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> ProjectRevertDetailsToggleLabel;

	/** The section itself. Collapsed unless it has been opened AND has something to show. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UVerticalBox> ProjectRevertDetails;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "GraphCode|VCS")
	TObjectPtr<UTextBlock> ProjectRevertDetailsText;

	// ---- state ---------------------------------------------------------------------------------

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	TArray<FGcSidecarStatus> Rows;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	FString SelectedPath;

	/** The selected file's text, held between a take and a write. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|VCS")
	FString SelectedText;

private:
	const FGcSidecarStatus* FindRow(const FString& RepoRelativePath) const;
	FGcSidecarStatus SelectedRow() const;

	/**
	 * One decision, as rows in the two columns: a heading naming it, the common base when there is
	 * one, and the pair of values with the take buttons on them.
	 *
	 * ONE BUILDER FOR BOTH VIEWS. The merge and the resolve screen show the same decisions and used
	 * to render them differently — the merge as a slab of text under the comparison, the resolve view
	 * as rows. Two renderings of one thing is two places to fix a wording and one of them gets missed.
	 */
	void AppendConflictRows(const FGcInlineDecision& Decision, TArray<UGcDiffRowItem*>& OutRows);

	/**
	 * The whole file, with every decision drawn where it actually is.
	 *
	 * THE RESOLVE SCREEN USED TO BE A LIST OF DECISIONS AND NOTHING ELSE — two values and a pair of
	 * buttons, with no file around them, so "which line is this?" and "what else is in here?" had no
	 * answer on the screen that was asking for the answer. The merge screen had already been fixed
	 * this way in E10.5; this is the same treatment for the screen a person reaches next, out of the
	 * same builder, so a decision reads identically wherever it is met.
	 */
	void BuildFileWithDecisions(const FString& Text, const TArray<FGcInlineDecision>& Decisions,
		TArray<UGcDiffRowItem*>& OutRows);

	/** Turn each conflict marker in a paired diff back into its decision rows, in place. */
	void ExpandMarkerRows(const TArray<UGcDiffRowItem*>& Paired,
		const TArray<FGcInlineDecision>& Decisions, TArray<UGcDiffRowItem*>& OutRows);

	void RenderConflicts();
	void RenderMerge();

	/**
	 * Any two sources, side by side. E10.10 — this is `RenderAgainstHead` and `RenderAgainstLive`
	 * folded into one, because those were one function with the sources written into it.
	 */
	void RenderCompare();

	/**
	 * Which files the current pair of sources disagree about — one `git diff --name-only`, cached.
	 *
	 * ASKED OF GIT, ONCE. The alternative is reading every sidecar at both revisions to find out
	 * whether it belongs in the list, which is two `git show` processes per file for a question git
	 * answers for the whole repository in one.
	 */
	void RefreshCompareScope();

	/**
	 * Fill in each row's owning block on each side, and whether that side's copy is what the asset
	 * holds. One parse per document for the whole comparison — see `UGcDiffRowItem::LeftBlock`.
	 */
	void MarkBlockCurrency(const TArray<UGcDiffRowItem*>& InRows);

	UGcDiffRowItem* MakeRow();

	/** The current view's options, so every builder elides by the same rule. */
	FGcDiffViewOptions ViewOptions() const;

	/**
	 * The merge preview for the selected file, computed at most once per selection.
	 *
	 * THREE `git show :N:<path>` PROCESSES PER CALL, and the merge itself on top. It used to be
	 * recomputed on every redraw — which includes every take, every pan and every mode change — and
	 * that is most of why a conflicted file felt slow. Its three inputs are index stages, and nothing
	 * this panel does moves those, so the answer is stable for as long as the selection is.
	 */
	const FGcMergePreview& PreviewForSelection();
	void InvalidatePreview();

	/**
	 * The selected file's text changed on disk; update the row, the counts and the screen.
	 *
	 * NOT `Refresh()`, WHICH RESCANS EVERY SIDECAR IN THE PROJECT. One take used to cost a full scan
	 * plus a fresh merge preview, and a merge with six decisions cost six of them.
	 */
	void NoteSelectedTextChanged(const FString& NewText);

	/** Put the list's own selection on the selected path, without that reading back as a click. */
	void SyncListSelection();

	UFUNCTION() void OnSyncAllClicked();
	UFUNCTION() void OnNextClicked();
	UFUNCTION() void OnPrimaryClicked();

	/**
	 * The restore control under the left column. Routes into `PressPrimaryAction` rather than into
	 * `ArmRestore` directly, and the indirection is the point: this button is only ever shown when
	 * `PrimaryActionForSelection` answers `Restore`, so sending it through the one dispatcher keeps
	 * a single answer to "what does pressing this do" — the defect §59 of this header describes is a
	 * second switch that has to be kept in step by hand.
	 */
	UFUNCTION() void OnRestoreClicked();

	UFUNCTION() void OnRowSelected(UObject* Item);

	/**
	 * A row picked in the CONFLICT list.
	 *
	 * Separate from `OnRowSelected` because the two lists mean two different things by a click:
	 * this one asks for the decision screen, the other for the comparison. One handler taking a
	 * flag would be the same thing with the difference hidden inside it.
	 */
	UFUNCTION() void OnConflictRowSelected(UObject* Item);

	UFUNCTION() void OnOpenClicked();

	/** Save the selected asset. Shown only while its package is dirty — see `SaveButton`. */
	UFUNCTION() void OnSaveClicked();

	/**
	 * A category picked in one side's first box. Lands that side on the category's default target.
	 *
	 * FOUR HANDLERS RATHER THAN ONE WITH A FLAG, because `UComboBoxString::OnSelectionChanged` is a
	 * dynamic multicast that carries no sender: the only thing that can say which box was used is
	 * which function got called. A shared handler would have to guess from the label, which is the
	 * lookup-by-words this panel already refuses everywhere else.
	 */
	UFUNCTION() void OnLeftCategoryChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnRightCategoryChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	/**
	 * A target picked in one side's second box — a branch, or `local` / `editor (unsaved)`.
	 *
	 * BY INDEX RATHER THAN BY LABEL, and it is the rule for every box here rather than a
	 * branch-shaped detail: a label is words a UI chose and nothing guarantees two of them differ,
	 * while the position in the list is what `AddOption` was called in. E11.4 met the strong form of
	 * this — two saves inside one second produce two entries with identical labels — and the weak
	 * form is still real for branches, so the lookup is the same in all four.
	 */
	UFUNCTION() void OnLeftTargetChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnRightTargetChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	/**
	 * A target box being pulled down — the one gesture that means *show me the branches*.
	 *
	 * E11.8's rule is that `git for-each-ref` runs only while somebody is looking at the list it
	 * fills, and E11.11 broke that by accident: folding `last commit` into the branch category put
	 * that box on screen in the panel's default state, so filling it on every draw put ~100 ms back
	 * on every refresh. Opening the box is what "looking at it" actually means.
	 */
	UFUNCTION() void OnLeftTargetsOpening();
	UFUNCTION() void OnRightTargetsOpening();

	/** A commit picked, and the box opened. E11.26 — same two gestures, one row lower. */
	UFUNCTION() void OnLeftCommitChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnRightCommitChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnLeftCommitsOpening();
	UFUNCTION() void OnRightCommitsOpening();

	/** What all six of the above do, once. The side is the only thing they disagree about. */
	void CategoryPicked(bool bRightSide);
	void TargetPicked(bool bRightSide);
	void TargetsOpening(bool bRightSide);
	void CommitPicked(bool bRightSide);
	void CommitsOpening(bool bRightSide);

	// ---- the time machine's gestures, four per side ----------------------------------------------
	//
	// TEN THUNKS AND THREE BODIES, which is the same shape the category and target boxes already use:
	// a `UFUNCTION` delegate cannot carry the side, so the side is in the name and nowhere else — and
	// since E11.24 the SIZE of the step is there too.

	UFUNCTION() void OnLeftTimeBackClicked();
	UFUNCTION() void OnLeftTimeForwardClicked();
	UFUNCTION() void OnRightTimeBackClicked();
	UFUNCTION() void OnRightTimeForwardClicked();

	UFUNCTION() void OnLeftTimeBackBlockClicked();
	UFUNCTION() void OnLeftTimeForwardBlockClicked();
	UFUNCTION() void OnRightTimeBackBlockClicked();
	UFUNCTION() void OnRightTimeForwardBlockClicked();

	/** A typed moment, on Enter or on focus leaving the box. */
	UFUNCTION() void OnLeftTimeCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UFUNCTION() void OnRightTimeCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** What both typed boxes do, once. */
	void TimeCommitted(bool bRightSide, const FText& Text, ETextCommit::Type CommitMethod);

public:
	/**
	 * Read a typed moment as UTC. Pure, static and public so a spec can read it without a panel.
	 *
	 * FIVE FORMS, AND THE FIRST IS THE ONE THE PANEL ITSELF PRINTS: `05 Aug 2026 14:32`, optionally
	 * with the `·` separator and seconds. Then `today 14:32`, ISO-8601, `YYYY-MM-DD HH:MM`, and a
	 * bare `14:32` meaning today. Anything else is refused with `OutError` naming the forms.
	 *
	 * LOCAL IN, UTC OUT. Every instant this panel holds is UTC and every instant it shows is local,
	 * so a typed value is read as local — the one the reader can see on their own clock.
	 */
	static bool ParseTypedInstant(const FString& Typed, FDateTime& OutUtc, FString& OutError);

	/**
	 * A name that fits the left pane. Tail-preserving — see the definition for why.
	 *
	 * `Reserved` is how many characters the caller is going to add after it. **THE BUDGET BELONGS TO
	 * THE LABEL, NOT TO THE NAME**: E11.12 began appending a tip stamp, and a name elided to the full
	 * budget plus `  ·  5 Aug 14:32` overflows the box this function exists to fit.
	 *
	 * **NAMED FOR THE RULE AND NOT FOR THE FIRST CALLER**, because there are two now: a branch picker's
	 * options and a file row's folder. Both are strings whose head is what every candidate shares and
	 * whose tail is what tells one from another, and both were being cut at the identifying end.
	 *
	 * Public and static for the same reason `ParseTypedInstant` is: it is a pure string rule, and a
	 * spec that had to build a panel and a git repository to check an ellipsis would not be checking
	 * the ellipsis.
	 */
	static FString ElideKeepingTail(const FString& Label, int32 Reserved = 0);

	/**
	 * The same budget, cut from the other end. E11.26, and it is not a preference.
	 *
	 * `ElideKeepingTail` exists because branch names share their head (`claude/…`) and differ at the
	 * tail. **A COMMIT IS THE EXACT OPPOSITE**: its sha and the start of its subject are what
	 * identify it, and two commits on one branch look most alike at the end of their message. Cutting
	 * a commit line at the tail-preserving end would leave a picker of entries beginning `…`.
	 *
	 * Static and public for the reason its twin is: a rule about a string, checkable without a panel.
	 */
	static FString ElideKeepingHead(const FString& Label, int32 Reserved = 0);

	/** How many commits the picker offers. A dropdown is not a log; see `CommitsCapped`. */
	static constexpr int32 GcCommitPickerMax = 50;

	/**
	 * What one target reads as INSIDE A PICKER: the elided label, and the source's own time. E11.12.
	 *
	 * **THE STAMP GOES HERE AND NOT ON `FGcCompareSource::Label`, AND THE ORDER OF THE TWO OPERATIONS
	 * IS THE WHOLE REASON.** `ElideKeepingTail` keeps the TAIL of a long branch name — that is its
	 * design, because real names here share a `claude/`-shaped prefix — so a label carrying its stamp
	 * at the end would be elided down to the timestamp and lose the branch. The name is shortened
	 * first and the time appended after, which is also why this is one function used by both the fill
	 * loop and `SetSelectedOption`: a box whose selected string is built differently from its options
	 * silently selects nothing.
	 *
	 * The other half is that a caption is not a picker entry. `TextForSource` appends the time this
	 * ref last touched THE SELECTED FILE, which is a different and better answer over a diff; putting
	 * a tip time into `Label` would have printed both, a foot apart, disagreeing.
	 */
	FString PickerOptionLabel(const FGcCompareSource& Source) const;

	/**
	 * The moment this store can actually answer at or after `Wanted`, and why not when it cannot.
	 *
	 * Split from the handler so the sentence a person reads is built by something a headless spec
	 * can call — the same compose/raise split `FGcNotify` uses, for the same reason.
	 */
	bool ResolveTypedInstant(const FDateTime& WantedUtc, FDateTime& OutUtc, FString& OutMessage) const;

protected:

	/** One button, two presses: arm the preview, then perform it. E11.9. */
	UFUNCTION() void OnProjectRevertClicked();
	UFUNCTION() void OnProjectRevertDetailsClicked();

	/** Take every decision on one side. The column each sits under is the side it takes. */
	UFUNCTION() void OnTakeAllLeftClicked();
	UFUNCTION() void OnTakeAllRightClicked();

	/**
	 * Point the comparison at a moment and disarm anything previewed. **Draws nothing.**
	 *
	 * The half of `CompareAgainstInstant` a caller wants when a full `Refresh` is coming anyway:
	 * redrawing first would build the list out of the previous category's rows and then throw it
	 * away.
	 *
	 * It moves the end the time machine is driving — see `bTimeMachineOnRight`.
	 */
	void SetHistoryInstant(const FDateTime& Utc);

	/** The same, on a named side. What a category change and the pickers use. */
	void SetHistoryInstantFor(bool bRightSide, const FDateTime& Utc);

	/**
	 * Take both sides at once, derive their categories, and disarm anything previewed. Draws nothing.
	 *
	 * **THE ONE PLACE A COMPARISON IS ASSIGNED**, and the reason it is private and separate from
	 * `SetCompareSources` is that the public setter starts a `Refresh` — and half the callers here
	 * are already inside one.
	 */
	void AdoptSources(const FGcCompareSource& Left, const FGcCompareSource& Right);

	/** `last commit` and `local`: what the panel opens on, and what a side falls back to. */
	static FGcCompareSource DefaultLeftSource();
	static FGcCompareSource DefaultRightSource();

public:
	/**
	 * The instant the end the time machine is NOT driving holds, or epoch when it is not a moment.
	 *
	 * Public because "did stepping move ONE end" is the assertion the one-control ruling exists for,
	 * and a spec cannot make it without being able to read the other end.
	 */
	FDateTime GetOtherInstant() const;

	/**
	 * Exactly `local ↔ last commit`, in either order — the one comparison the scan has already asked
	 * git about.
	 *
	 * **IT REPLACES `CompareCategory == Head`, AND THE REPLACEMENT IS THE POINT.** `ScanSidecars`
	 * fills every row's `+N −N` from one `git diff HEAD --numstat`, so this comparison needs no
	 * second process and must not run one: `git diff --name-only` counts a digest-only line that the
	 * scan deliberately drops, and the two answers disagreeing is what drew `BP_WindowProperties.gc`
	 * as `+1 −1` for a fortnight. With `Head` folded into the `Branch` category the category can no
	 * longer say this — `git branch` now covers `main` against `dev` as well — so the question is
	 * asked of the two SOURCES, which is what it was always really about.
	 */
	bool IsWorkingAgainstHead() const;

	/**
	 * Did `Package` hold different text at `From` than at `To`? §3.2's rule, over `HistoryInstants`.
	 *
	 * The compare list for a two-moment comparison, one array walk per row and no file system —
	 * which is what lets a drag on the time machine redraw the list while it moves.
	 */
	bool ChangedBetween(const FString& Package, const FDateTime& From, const FDateTime& To) const;

protected:
	UFUNCTION() void OnDiffPanned(float Offset);

	/**
	 * A click on a comparison row.
	 *
	 * `UListView::OnItemClicked` FOR THE SAME REASON THE FILE LIST USES IT: a `UListView` wraps every
	 * entry in an `SObjectTableRow` that handles the click itself, so an override on the entry widget
	 * never fires. Measured in E10.2, and it cost a build to find.
	 */
	UFUNCTION() void OnDiffRowClicked(UObject* Item);

	/** Push the current pan into every realized row and show or hide the bar. */
	void RefreshPan();

	float DiffPan = 0.0f;
	float DiffPanRange = 0.0f;

	/** Guards the mirror pass, so pushing an offset into a row cannot read back as a user scroll. */
	bool bSyncingPan = false;

	/** Narrower than the 26% that shipped, which took a quarter of a very wide dock tab. */
	float LeftWidth = 300.0f;
	bool bDraggingSplit = false;

	EGcCompareMode CompareMode = EGcCompareMode::Compare;

	/**
	 * Somebody used a picker, so their choice outranks worst-first from here on.
	 *
	 * Set ONLY by a picker's own delegate. A mode the panel selected — landing a stuck file on the
	 * screen that can unstick it — is not somebody choosing something, and counting it as one would
	 * pin the view to whatever the first conflicted file happened to need.
	 */
	bool bModePinned = false;

	/**
	 * Which kind of thing each side is pointed at. Panel state — see `EGcCompareCategory`.
	 *
	 * **DERIVED IN ONE PLACE**, `SetSourceFor`, for exactly the reason the single `CompareCategory`
	 * was: every path into a comparison passes through it — both pickers, the time machine,
	 * `SetCompareSources`, a refresh repairing a deleted branch, a spec choosing a source directly —
	 * and a category kept in step by each of those separately is wrong on whichever one gets added
	 * next. Its symptom is a picker naming a comparison the panel is not showing.
	 */
	EGcCompareCategory LeftCategory = EGcCompareCategory::Branch;
	EGcCompareCategory RightCategory = EGcCompareCategory::Current;

	/** The sources on offer. The fixed four always; the branches only once somebody asks for them. */
	TArray<FGcCompareSource> SourceOptions;

	/**
	 * `SourceOptions` holds this checkout's branches, as of the last refresh that ran while they
	 * were being shown.
	 *
	 * Cleared at the top of `Refresh`, so a branch created behind the panel appears exactly as soon
	 * as it used to — the unconditional `for-each-ref` also only ever ran inside a refresh.
	 */
	bool bBranchesValid = false;

	/**
	 * Somebody has pulled a branch box down at least once this session, so the list is worth having.
	 *
	 * SEPARATE FROM `bBranchesValid`, WHICH IS ABOUT STALENESS. That one says "the list in hand is
	 * from this refresh"; this one says "there should be a list at all". A refresh clears the first
	 * and leaves the second, so a reader who has opened the picker keeps getting fresh branches and
	 * one who never has keeps paying nothing.
	 */
	bool bBranchesWanted = false;

	FGcCompareSource LeftSource;
	FGcCompareSource RightSource;

	/**
	 * Repository-relative paths the two sources disagree about, when either of them is a branch.
	 *
	 * EMPTY AND VALID IS A REAL ANSWER — the two sides agree about everything — and it is why this
	 * carries its own flag rather than being read as "not computed yet".
	 */
	TSet<FString> CompareScope;
	bool bCompareScopeValid = false;

	/**
	 * PACKAGE → the newest instant the store holds an archive for it at. The compare list of a
	 * history comparison, and the reason the time machine can redraw while a slider is moving.
	 *
	 * **THE LIST IS "HAS THIS ASSET CHANGED SINCE THE CHOSEN MOMENT", WHICH IS ONE COMPARISON PER
	 * ROW AGAINST THIS MAP AND NO FILE SYSTEM AT ALL.** An asset with an archive at or after the
	 * moment was saved-with-a-change after it, so what it holds now is not what it held then — a
	 * difference, and a row. An asset with none has not changed since, so a comparison would draw
	 * nothing; E11.4 listed those anyway, because its list was "every asset with any history".
	 *
	 * BY PACKAGE, because that is what the store is keyed on (§2.3: the basename is not identity)
	 * and it is what every row already carries. Built once per refresh from one glob and the
	 * package walk that was already happening.
	 */
	TMap<FString, FDateTime> HistoryLatest;

	/**
	 * PACKAGE → every instant the store holds an archive for it at, newest first.
	 *
	 * **`HistoryLatest` CANNOT ANSWER A WINDOW, WHICH IS WHY THIS EXISTS.** One side on the timeline
	 * asks *"has this changed since T"*, and the newest instant answers it. Two sides ask *"did this
	 * change BETWEEN A and B"* — and an asset saved yesterday and again this morning has a newest
	 * inside every window that ends now, whether or not anything happened inside the window.
	 *
	 * Built in the same pass over the same glob, so it costs one array per package and no extra IO.
	 */
	TMap<FString, TArray<FDateTime>> HistoryInstants;

	/** Every distinct instant in the whole store, newest first. The time machine's stops. */
	TArray<FDateTime> TimelineInstants;

	/**
	 * Which END the one time machine is currently driving. E11.11's second ruling.
	 *
	 * **ONE CONTROL, NOT TWO, AND "TWO TIMELINES" IS ABOUT THE READING.** The time machine is five
	 * controls plus the snapping rule that every landing lands on an instant the store actually holds
	 * (§5), and a second copy is a second place for that rule to drift. What *"they can view two
	 * timelines"* asks for is a comparison between two moments, which is a property of two INSTANTS —
	 * and the panel already has somewhere to keep the second one: the other side's source.
	 *
	 * So the control points at whichever side is on `local history`; when both are, this says which
	 * end the slider and the step buttons move, and the readout names both.
	 */
	bool bTimeMachineOnRight = false;

	/**
	 * `<ref>|<repo-relative path>` → when that ref last changed that path. `git log -1`, cached.
	 *
	 * MUTABLE BECAUSE THE CAPTION IS COMPUTED IN A CONST PATH, and cached because a caption is
	 * rebuilt on every redraw — a hover, a pan, an expansion — while its answer changes only when
	 * the ref or the selection does. Spawning a git process per redraw is the mistake E11.7b
	 * measured at four processes per scan; one per (ref, path) for the whole session is the same
	 * fix applied before it can happen.
	 *
	 * An empty path means the ref's own tip commit, which is what the merge captions want.
	 */
	mutable TMap<FString, FDateTime> RefFileTimes;

	/**
	 * `<ref>` → when that ref last moved. The epoch `OrderOf` sorts by, and `SortKeyFor` reads.
	 *
	 * **DELIBERATELY NOT KEYED ON THE SELECTED PATH**, which is the whole difference between this and
	 * `RefFileTimes` above. A picker entry's time has to be path-independent or the branch list would
	 * re-order itself as the reader walks the file list — and *"when did this branch last move"* is
	 * the question a person reading a branch name is asking, not *"when did it last touch the file I
	 * happen to have clicked"*.
	 *
	 * FILLED ONLY BY `EnsureRefTimesLoaded`. An absent entry is *"nobody has asked yet"*, and every
	 * reader treats it as no-time-known rather than as a date.
	 */
	TMap<FString, FDateTime> RefTipTimes;

	/** Whether the gesture that pays for `RefTipTimes` has happened in this session. */
	bool bRefTimesValid = false;

	/**
	 * `<ref>` → its newest commits, newest first. E11.26, and the whole cost of the commit row.
	 *
	 * ONE `git log` PER OPENING GESTURE, and nothing else spends one — the same rule `for-each-ref`
	 * has followed since E11.8. The entry is dropped and re-read when the box is opened again, so
	 * the list is as fresh as the gesture that asked for it and a redraw costs nothing.
	 */
	TMap<FString, TArray<FGcCommitEntry>> CommitsByRef;

	/**
	 * `<package>` → the kind its archives say it was, for assets that exist only in the store.
	 *
	 * **A DELETED ASSET'S KIND CANNOT CHANGE**, so this is answered once per session rather than once
	 * per refresh. `FGraphCodeHistory::List` globs a package directory and `AppendHistoryOnlyRows`
	 * runs on every refresh against the timeline, which made walking N stops cost N globs per
	 * history-only lineage — measured on this project at 264 ms a stop across 689 stops. An empty
	 * value is a cached *"this lineage names no kind"*, which is the answer that would otherwise be
	 * re-asked forever.
	 */
	mutable TMap<FString, FString> HistoryOnlyKinds;

	/** The store's packages as `RefreshTimelineInstants` last read them, and which refresh that was. `AppendHistoryOnlyRows`
	 * reuses them within the same refresh instead of walking the 17k-file store again (2026-10-29). */
	TArray<FString> StorePackages;
	int32 StorePackagesRefresh = INDEX_NONE;
	/** Counts refreshes that ran past the fingerprint's early return. */
	int32 RefreshSerial = 0;

	/** The refs whose list git had more of than `GcCommitPickerMax`. The box says so. */
	TSet<FString> CommitsCapped;

	/**
	 * Which branch each side's commit row is listing, once its source is a commit and can no longer
	 * say. Empty until a commit is picked; `HEAD` is the fallback, never a guess at a branch name.
	 */
	FString LeftCommitBranch;
	FString RightCommitBranch;

	/** The selected asset's instants, newest first. Rebuilt whenever the selection moves. */
	TArray<FGcHistoryEntry> HistoryEntries;

	/**
	 * A restore that has been planned and previewed and is waiting for its second press.
	 *
	 * THE PLAN IS THE BUTTON. Its `Label` is what the control says and its `DesiredText` is what
	 * pressing it writes, so the label and the effect cannot come apart — the same defect E10.12
	 * removed from `RefreshActions`/`OnPrimaryClicked` by making one function answer both.
	 */
	FGcRestorePlan ArmedRestore;
	bool bRestoreArmed = false;

	/**
	 * The project-wide revert that has been planned and previewed, waiting for its second press.
	 *
	 * SEPARATE FROM `ArmedRestore` AND NEVER ARMED AT THE SAME TIME. They are two buttons with two
	 * blast radii, and one armed state shared between them is a press that does whichever was armed
	 * last — which the reader has no way to see.
	 */
	FGcProjectRestorePlan ArmedProjectRevert;
	bool bProjectRevertArmed = false;

	/** Whether the created/removed/skipped section is open. Closed until somebody opens it. */
	bool bProjectRevertDetailsOpen = false;

	/**
	 * The block a click on a changed diff row aimed the footer at, or empty for the whole asset.
	 *
	 * A NAME, AND NOTHING ELSE — which is the point. The first version of this ran the whole plan
	 * from the row's own hover button: it projected the live asset and dry-ran an apply, so a stray
	 * click on a diff cost seconds and the button doing it was labelled `Keep yours`. Selecting is
	 * free and the footer says what it would do; the expensive part belongs to the control that
	 * says it is about to be expensive.
	 */
	FString PendingRestoreBlock;

	/**
	 * The two texts the comparison on screen is OF, kept so a click can ask which block a line
	 * belongs to without the redraw having worked it out for every row in advance.
	 */
	FString ComparisonLeftText;
	FString ComparisonRightText;

	/**
	 * The LIVE asset's own text for the selected file — the third version, and the one `Applied` is
	 * a claim about. Memoized per selection; see `CurrentTextForSelection`.
	 *
	 * **INVALIDATED WHEREVER THE SCAN MEMO IS**, and for the same reason: a block apply changes what
	 * the asset holds without moving anything a `stat` can see, so a cache keyed on the filesystem
	 * would keep saying `Applied` about the version you just replaced.
	 */
	FString ComparisonCurrentText;
	bool bCurrentTextValid = false;

	/** Which selection `ComparisonCurrentText` was projected for. Empty until one has been. */
	FString CurrentTextForPath;

	/** Path → the stage triple its merge was decided against. See `LoadMergeLedger`. */
	TMap<FString, FString> SettledMerges;

	/** Path → the stage triple git is holding right now. One `git ls-files -u` per refresh. */
	TMap<FString, FString> UnmergedStages;

	/** How many `conflict { }` constructs the in-memory text still carries. -1 means not counted. */
	int32 UnresolvedCount = INDEX_NONE;

	/** `SelectedText` differs from what is on disk: a resolution nobody has written yet. */
	bool bResolutionHeld = false;

	/**
	 * The merge preview's output, held for the one button that writes it. Empty means there is none.
	 *
	 * CLEARED AT THE TOP OF EVERY REDRAW, so it can only ever be armed by the view a person is
	 * actually looking at. A merge proposal held across a mode change would be a primary action
	 * offering to overwrite a file with the result of a comparison that is no longer on screen.
	 */
	FString PendingMergeText;

	/**
	 * A side has been taken on the pending merge, so it must not be recomputed.
	 *
	 * Taking a side redraws, and the redraw calls `PreviewMerge` again — which would hand back the
	 * merge driver's untouched output and silently undo the decision that triggered it. This is the
	 * flag that makes the merge view's decisions stick until they are written or abandoned.
	 */
	bool bPendingMergeEdited = false;

	/** How many decisions the pending merge still carries, so the write button can say so. */
	int32 UnresolvedMergeDecisions = 0;

	/**
	 * A preflight passed for the text currently on disk, so the button is asking for confirmation.
	 *
	 * CLEARED BY EVERY CHANGE, and that is the whole reason it is a flag rather than a memory of the
	 * last result. A confirm state is safe only while the thing that was preflighted is still the
	 * thing that would be applied; anything that moves the file drops the panel back to `Apply`, and
	 * a second press then runs a fresh preflight rather than an unchecked write.
	 */
	bool bPreflightPassed = false;

	/** Guards the picker while it is being repopulated, so rebuilding it does not look like a choice. */
	bool bSettingModes = false;

	/** Guards the list while the panel puts its own selection on it, so it does not read as a click. */
	bool bSyncingSelection = false;

	/**
	 * A refresh is running, so a nested one is a no-op.
	 *
	 * `Refresh` parses every `.gc` under every content root and runs four git processes — the
	 * panel's most expensive call by an order of magnitude. Selecting a conflicted file now writes a
	 * merge and applies a proposal, and both of those end in a refresh, so one click was paying for
	 * two full rescans on top of the merge preview's three `git show` calls. The outermost one is
	 * the one that runs, after everything it is describing has actually happened.
	 */
	bool bRefreshing = false;
	bool bRefreshWanted = false;

	/** The panel's subscription to `FGcChangeNotice::OnSettled`, released in `NativeDestruct`. */
	FDelegateHandle ChangeNoticeHandle;

	/**
	 * What the working tree and the chosen source looked like at the last scan that actually ran.
	 *
	 * The panel re-reads when its tab is brought forward, at the end of every action it takes, and
	 * since E11.10 whenever anything else announces a change — which is right, and most of those
	 * find nothing has changed. Comparing this first turns those into ten milliseconds of `stat`
	 * instead of 350 ms of git and 352 parses. See `UGraphCodeVcsLibrary::WorkingTreeFingerprint`.
	 *
	 * THE THIRD SOURCE IS WHY THE SERIAL IS IN THERE. An announcement can be about a change no file
	 * records, and a fingerprint that could not see one would make every such refresh a no-op.
	 */
	FString LastScanFingerprint;

	/** Has a scan ever run? The first one must, whatever the fingerprint says. */
	bool bScanned = false;

	/**
	 * The rows the last scan produced, and the working tree plus the arguments it produced them from.
	 *
	 * **SEPARATE FROM `LastScanFingerprint` BECAUSE THE TWO SIDES ARE NOT INPUTS TO THE SCAN.** That
	 * one asks *has anything at all changed*, and picking a different moment changes the answer to it
	 * — so a refresh that moves the timeline runs, as it must: the list, the diff and every per-row
	 * question have to be redone. What none of them need is the project re-read, and re-reading it was
	 * the whole cost of a press. `Refresh` has the measurement.
	 *
	 * Empty means no scan is remembered, which is what `InvalidateScanMemo` leaves behind.
	 */
	FString LastRowScanMark;
	TArray<FGcSidecarStatus> ScannedRows;

	/**
	 * Forget the memo above, so the next `Refresh` really rescans. Called AT EVERY WRITE THIS PANEL
	 * PERFORMS, not at the refreshes that follow them.
	 *
	 * **A WRITE THE PANEL MADE IS NOT A POLL, AND THE FINGERPRINT CANNOT ALWAYS TELL.** It is built
	 * from `stat` — every `.gc`'s size and modified time, `.git/index`, `.git/HEAD`, and the
	 * editor-state serial — which is exactly right for "has anything happened while I was away" and
	 * has two blind spots for "did what I just did land": an apply marks a package dirty and the
	 * serial is advanced only by the MCP dispatch and the save hook, so the panel's own apply moves
	 * nothing it can see; and a re-projection that produces the same bytes at the same size relies
	 * on a timestamp to be noticed at all.
	 *
	 * The cost of being wrong in one direction is a stale panel after the button somebody pressed —
	 * *"when apply, it shows save, but the panel didnt update"*, 2026-08-05 — and in the other, one
	 * rescan per deliberate gesture, which is what `Refresh`'s own header says it is for.
	 */
	void InvalidateScanMemo()
	{
		bScanned = false;
		LastScanFingerprint.Reset();
		// THE ROWS GO WITH IT, and this is the line that makes the row memo safe to keep at all: the two
		// blind spots named above are both cases where the tree fingerprint the rows are keyed on has
		// not moved, so nothing else would make the next refresh re-read the project.
		LastRowScanMark.Reset();
		// THE ASSET'S OWN TEXT IS THE OTHER THING A WRITE MOVES WITHOUT MOVING A FILE, and it is the
		// one `Applied` is computed from. Left stale it would report the version you have just
		// replaced as the version the asset holds.
		bCurrentTextValid = false;
	}

	// ---- how much of the file is on screen ------------------------------------------------------

	bool bShowAllLines = false;

	/** Runs opened one at a time. Cleared whenever the text under them moves. */
	TSet<int32> ExpandedRuns;

	/** Row indices the change buttons jump between, in document order. */
	TArray<int32> ChangeAnchors;

	/** Which anchor was last jumped to. `INDEX_NONE` before the first jump. */
	int32 CurrentChange = INDEX_NONE;

	// ---- the cached merge -----------------------------------------------------------------------

	FGcMergePreview CachedPreview;
	FString CachedPreviewPath;
	bool bPreviewCached = false;
};
