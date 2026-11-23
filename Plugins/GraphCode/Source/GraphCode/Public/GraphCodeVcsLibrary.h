#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GraphCodeVcsLibrary.generated.h"

/**
 * GraphCode's Git surface, as Blueprint nodes. GC-EDITOR-VCS-DESIGN.md §4.
 *
 * WHY THIS FILE EXISTS AT ALL. Everything GraphCode does about Git today happens in a shell —
 * `GcMerge`, `GcResolve`, `GcCheck`, `GcApply` are commandlets — which is right for a merge driver
 * git invokes, and wrong for the one human who has to DECIDE a conflict while the asset it
 * describes sits unopened in another window. The blocker was never the UI: UMG can draw all of it.
 * It was that the plugin exposed **zero** `BlueprintCallable`, and that Blueprint cannot spawn a
 * process. This closes both, and nothing more.
 *
 * IT IS A WRAPPER, NEVER A SECOND IMPLEMENTATION. Every function here forwards to the call the
 * commandlets already use — `FGraphCodeGenerator::Run`, `FGraphCodeSync::SyncPackage`,
 * `FGcDocumentMerge::Merge`, `FGcResolve::List`/`Pick`, `FGraphCodeApply::ApplyDocument`. A panel
 * and a shell that disagree about a merge would be worse than no panel.
 *
 * WHAT IT DELIBERATELY DOES NOT OFFER, and §4.1 has the argument: staging, committing, merging,
 * checking out, resetting. The verb list in `FGraphCodeGit` is read-only and closed. The only thing
 * that writes is a resolve, and it writes the one `.gc` the user is looking at.
 */

/**
 * What a `.gc` on disk is, answered from the FILE alone with no asset loaded.
 *
 * Read the names carefully, because the cheap answer and the expensive one are different questions
 * and conflating them is how a panel lies. Nothing here claims the sidecar matches its asset — that
 * needs a projection, which needs the asset loaded, which is `SyncAllSidecars(bCheckOnly)`. What
 * these states describe is the file's own internal consistency, which is exactly the axis a merge
 * moves along.
 */
UENUM(BlueprintType)
enum class EGcSidecarState : uint8
{
	/** Did not parse. Corrupt, truncated, or not a `.gc` at all. */
	Unreadable,

	/** `sig:` matches the body's digest. The text is a projection nobody has edited since. */
	SelfConsistent,

	/**
	 * `sig:` disagrees with the body. Somebody edited the text: a proposal in flight, not damage.
	 * Applying it makes it real; regenerating discards it.
	 */
	EditedProposal,

	/** Clean merge output — carries `merged-from:`, and the asset has not absorbed it yet. */
	MergeProposal,

	/** Carries an unresolved `conflict { }` construct. Every writer refuses it by name. */
	Conflicted,

	/**
	 * There is no file. The store holds versions of this package and the project does not have it.
	 *
	 * **NOT `Unreadable`, WHICH IS WHAT IT USED TO BORROW.** A deleted asset's row is built from the
	 * archive rather than from a sidecar, so it inherited the zero value and every screen said
	 * *unreadable* — the one word that makes a recoverable file look damaged. The distinction is the
	 * whole reason `AppendHistoryOnlyRows` exists: nothing is wrong with this package, it is simply
	 * not here, and the panel's answer is `Restore this file…` rather than a repair.
	 *
	 * Last, so the existing values keep their numbers in anything already serialized.
	 */
	NotInProject,

	/**
	 * A recipe: it names `kind:` and `asset:` and makes no projection claim, so `sig:`, `omits:`
	 * and `source:` are excused — `FGraphCodeSidecarReader::Parse`'s `bRecipe`, C21.5 §16.2.
	 *
	 * **NOT `Unreadable`, WHICH IS WHAT IT USED TO BORROW.** `gc_describe kind:` documents create as
	 * *write the `.gc` at the sidecar path, then `gc_apply create:true`* — so between those two acts
	 * a legal `.gc` sits under `Content/` and every screen called it corrupt. Nothing is wrong with
	 * this file; it has simply never been projected, which is a third answer rather than a stricter
	 * one. Whether the asset exists is a different question, and this scan deliberately does not
	 * load one to ask it.
	 *
	 * After `NotInProject` for the same reason that one is last.
	 */
	Recipe,
};

/** One row of the panel's status screen. */
USTRUCT(BlueprintType)
struct FGcSidecarStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString PackageName;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString SidecarPath;

	/** Repository-relative, forward slashes. Empty when the project is not in a work tree. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString RepoRelativePath;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Kind;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	EGcSidecarState State = EGcSidecarState::Unreadable;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 ConflictCount = 0;

	/** `git status --porcelain`'s two characters. Empty means unmodified, which is the common case. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString GitStatus;

	/**
	 * Lines gained and lost against the last commit.
	 *
	 * A NUMBER IS THE SHORTEST HONEST SUMMARY OF A CHANGE, and a list of files that only says which
	 * ones moved makes a person open each of them to find out whether it was a typo or a rewrite.
	 * Both are zero for a file git cannot count — see `FGraphCodeGit::LineChanges` — and for an
	 * untracked file `LinesAdded` is its whole length, because every line of it is new.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 LinesAdded = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 LinesRemoved = 0;

	/**
	 * This merge has been decided, and NOTHING ON DISK COULD HAVE TOLD YOU THAT.
	 *
	 * Take `ours` on every conflict — the commonest way a merge ends — and the result is ours, which
	 * is byte-identical to the file a merge nobody has started leaves behind. Git cannot help
	 * either: its index stays unmerged until the commit, and this panel has no verb that could
	 * change that. So the panel REMEMBERS, keyed on the stage triple so the memory expires the
	 * moment a different merge lands on the same path — see `UnmergedStageDigests`.
	 *
	 * Filled by the panel after the scan, not by the scan: the ledger is the panel's, and a status
	 * query has no business reading one.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bMergeSettled = false;

	/** The facets this document declares it does not carry. A no-diff means nothing about these. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> Omits;

	/** Why it is unreadable, when it is. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Error;
};

/** `FGraphCodeGenerationReport`, as Blueprint can see it. */
USTRUCT(BlueprintType)
struct FGcSyncReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 ScannedAssets = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 SupportedAssets = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 CurrentSidecars = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 MissingSidecars = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 StaleSidecars = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 WrittenSidecars = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 OrphanSidecars = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 DuplicateAliases = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 ConflictedSidecars = 0;

	/** Reported, never gated — the committed corpus already holds 123 of these (M4.5). */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 OrphanNodes = 0;

	/**
	 * Shape-built assets whose `.gcshape` no longer holds their recipe (H28). Gated, unlike
	 * `OrphanNodes`: the tracked corpus was at zero when this shipped.
	 *
	 * **MIRRORED HERE AND NOT ONLY IN THE COMMANDLET.** `bClean` below already counts it, so a
	 * panel without this field would go red with no number saying which category moved — two doors
	 * onto one report, one of them unable to say what it just refused for.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 LostShapeRecipes = 0;

	/** The same for a `.gcmove`-built clip, mirrored for the same reason: `bClean` counts it. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 LostMoveRecipes = 0;

	/** And for a `.gchuman`-built character. Third of three, same reason again. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 LostHumanRecipes = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 Errors = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> Messages;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bClean = false;
};

UENUM(BlueprintType)
enum class EGcDiffLineKind : uint8
{
	Context,
	Removed,
	Added,
};

USTRUCT(BlueprintType)
struct FGcDiffLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	EGcDiffLineKind Kind = EGcDiffLineKind::Context;

	/** 1-based line number on each side, or 0 where the line does not exist on that side. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 LeftLine = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 RightLine = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Text;
};

/** `FGcConflictRecord`, as Blueprint can see it. */
USTRUCT(BlueprintType)
struct FGcConflictView
{
	GENERATED_BODY()

	/** 1-based, positional. Stable only while the file is unmodified — which is what `Pick` checks. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 Id = 0;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Kind;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Entity;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Property;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Detail;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Base;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Ours;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Theirs;

	/** False when the merge had no base for this entity — §3.6, and it changes what a choice means. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bHasBase = false;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 Line = 0;
};

/**
 * What the driver WOULD do to a conflicted file, computed without touching the working tree.
 *
 * This is also shadow mode's missing producer (§1). `GcMerge -Shadow` only runs when somebody runs
 * a merge from a shell and remembers the flag; this runs every time a person opens a conflicted
 * file to look at it, which is the same computation over the same three inputs.
 */
USTRUCT(BlueprintType)
struct FGcMergePreview
{
	GENERATED_BODY()

	/** False when there is no merge in progress for this path, or the merge could not be attempted. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bAvailable = false;

	/** Why not, when not. A preview declines rather than inventing a base. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Reason;

	/** Stage 1 was absent: the two sides added this file independently (§3.6). */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bMissingBase = false;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString MergedText;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FGcConflictView> Conflicts;

	/**
	 * The `omits:` tokens no text merge can compare — §4.6 hole 1.
	 *
	 * A preview that showed a clean merge without this line would claim more than the format can
	 * prove, so the panel shows it on a CLEAN merge too. It is not an error list.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> UncomparedFacets;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> Notes;
};

/**
 * What folding the editor's unsaved state into a merge result would produce. M1c / §4.13.
 *
 * The row nothing else in this file can give you, and the one that decides whether a merge is safe
 * to apply: between a save and the next one, the asset in memory is the only copy of some of the
 * work, and git has never seen it.
 */
USTRUCT(BlueprintType)
struct FGcLiveMergeView
{
	GENERATED_BODY()

	/** False when the fold was refused OR when there was nothing unsaved to fold. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bFolded = false;

	/** Why not, when not — including the sound-base refusal, which is the important one. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Reason;

	/** The merge result plus every unsaved edit that did not collide with it. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Text;

	/**
	 * M1c.4 — every unsaved edit the merge overruled, with the value that was discarded.
	 *
	 * §4.13: dropping something a developer typed thirty seconds ago is the worst failure available
	 * here, "worse than a conflict, because nobody learns of it". This list is how they learn.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> DroppedLiveEdits;

	/** M1c.5 — `EventGraph n6 -> n9`, one per node whose provisional number had to move. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> ReboundAliases;

	/** Everything else the fold decided and did not have to overrule. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> Notes;

	/** The facets no text merge can compare, exactly as `FGcMergePreview` reports them. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	TArray<FString> UncomparedFacets;

	/**
	 * M1c.6 — the snapshot is a photograph and the asset is reload-pending from the moment it was
	 * taken. Edits made after it are a SECOND fold against a moved base, not a refinement of this
	 * one, and a panel that does not say so is inviting somebody to keep typing.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bReloadPending = false;
};

UENUM(BlueprintType)
enum class EGcResolveChoice : uint8
{
	/** Already what the merge left in the text, so this only removes the construct. */
	Ours,
	Theirs,
	Custom,
};

/** A `git` invocation's outcome, for a panel that wants to show exactly what it ran. */
USTRUCT(BlueprintType)
struct FGcGitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	bool bOk = false;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	int32 ExitCode = -1;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString StdOut;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString StdErr;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Error;
};

/**
 * One commit on a branch, as the picker needs it. E11.26.
 *
 * The full sha is what git is asked with and the short one is what a person reads; both are stored
 * because deriving one from the other at draw time is a rule about sha length, which git changes.
 */
USTRUCT(BlueprintType)
struct FGcCommitEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Sha;

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString ShortSha;

	/** Committer date, UTC. Absent-as-epoch is read by `OrderOf` as "no time known". */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FDateTime When;

	/** The first line of the message, verbatim. */
	UPROPERTY(BlueprintReadOnly, Category = "GraphCode")
	FString Subject;
};

/** One `.gc` as a directory enumeration sees it. Not reflected: C++ plumbing between a fingerprint and a scan. */
struct FGcSidecarFileStat
{
	FString Path;
	int64 Size = 0;
	FDateTime Modified;
};

UCLASS()
class GRAPHCODE_API UGraphCodeVcsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * The repository root containing this project, or empty when there is no work tree.
	 *
	 * PURE, and the three pure functions here are pure for a reason worth stating once. An impure
	 * `BlueprintCallable` whose exec input is unconnected **does not run and yields its return
	 * type's default** — the Blueprint compiler does not complain, so a graph that reads such a
	 * node as a value silently gets an empty string. That produced a path of `/Content/…` in the
	 * first build of the panel: the root was empty, the concatenation still succeeded, and the
	 * error surfaced two calls later as a file that could not be read. Anything here that is a
	 * side-effect-free query is therefore pure, so it evaluates where it is used.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	static FString GetRepositoryRoot();

	/**
	 * Every `.gc` under the project's Content directory, with its file state and git state.
	 *
	 * NO ASSET IS LOADED. That is what makes a panel refresh cheap enough to be automatic, and it
	 * is also why the states are the file's own (see `EGcSidecarState`): "does this text match its
	 * asset" is `SyncAllSidecars(bCheckOnly)`, a different and much more expensive question.
	 *
	 * @param bOnlyNeedsAttention  drop rows that are self-consistent and unmodified in git.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static TArray<FGcSidecarStatus> GetSidecarStatus(bool bOnlyNeedsAttention = false);

	/**
	 * The same scan, with a set of paths the attention filter may never drop.
	 *
	 * IT EXISTS BECAUSE "NEEDS ATTENTION" IS A QUESTION ABOUT **HEAD**, and a panel that can compare
	 * against any branch asks a different one. A sidecar that is self-consistent and unmodified — or
	 * whose only change since the last commit is its own digest — is correctly absent from a list of
	 * work, and is exactly the file somebody means when they pick another branch to compare against.
	 * Without this the caller's list can only ever be a SUBSET of the attention scan, so a file the
	 * scan drops is invisible to every comparison no matter what the comparison says about it:
	 * reported as "it hides the other unrelated file", 2026-08-04.
	 *
	 * NOT REFLECTED, and not because it is private: UHT cannot parse an array default, and making
	 * every existing caller pass an empty one to keep a Blueprint surface nothing calls would be
	 * paying for reflection twice.
	 */
	/**
	 * @param ComparedRev  The revision the `+N −N` on each row should be measured against. Empty
	 *                     means `HEAD`. A panel comparing against a branch and printing counts
	 *                     against the last commit is printing an answer to a question nobody asked.
	 */
	static TArray<FGcSidecarStatus> ScanSidecars(
		bool bOnlyNeedsAttention, const TArray<FString>& AlsoKeep,
		const FString& ComparedRev = FString());

	/**
	 * A cheap answer to "could anything the panel shows have changed?" — stats only, no process.
	 *
	 * **THIS EXISTS SO THE PANEL CAN STOP REFRESHING CONSTANTLY**, which is a different fix from
	 * making each refresh faster and the one that was actually needed. A refresh re-reads every
	 * sidecar and asks git several questions; it used to run whenever the tab was brought forward,
	 * whether or not a byte had moved. Comparing this string against the last one turns that into
	 * about ten milliseconds of `stat`.
	 *
	 * Folds every `.gc`'s path, size and modified time together with `.git/index`'s modified time
	 * and `.git/HEAD`'s contents — the last two because a commit, a merge or a branch switch changes
	 * what git says about files whose bytes never moved. Equal strings mean nothing this panel reads
	 * has changed; different strings mean something might have.
	 *
	 * **AND `FGcChangeNotice`'s EDITOR-STATE SERIAL, WHICH IS THE ONE INPUT THE FILESYSTEM CANNOT
	 * SUPPLY** (E11.10). An agent's `bp_apply` rewrites a Blueprint and marks its package dirty; no
	 * `.gc` is written until somebody saves, so every `stat` above answers "nothing happened" while
	 * the panel's `editor (unsaved)` comparison — the one no git client can even ask — has gone
	 * stale. The serial advances once per such change and never otherwise, which is what keeps this
	 * an ANSWER rather than a cache-buster: a session that only reads still gets equal strings.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static FString WorkingTreeFingerprint();

	/**
	 * `WorkingTreeFingerprint` that also hands back the walk it made, so a refresh that goes on to scan reads the tree
	 * once. **ONE ENUMERATION, ITS OWN STATS (VCS panel, 2026-10-29).** The tree grew from 352 sidecars to 10,420, and
	 * the fingerprint and the scan each walked it and then asked size and time of every file again: ~40,000 `stat`s a
	 * refresh. A directory enumeration already carries both, and measured here it is 130 ms against 330 ms per walk.
	 */
	static FString WorkingTreeFingerprintAndWalk(TArray<FGcSidecarFileStat>& OutWalk);

	/** `ScanSidecars` over a walk the caller already made — `WorkingTreeFingerprintAndWalk`'s, in the panel's refresh. */
	static TArray<FGcSidecarStatus> ScanSidecarsFromWalk(const TArray<FGcSidecarFileStat>& Walk,
		bool bOnlyNeedsAttention, const TArray<FString>& AlsoKeep, const FString& ComparedRev = FString());

	/** Every `.gc` under every swept content root, with the size and modified time its directory entry carries. */
	static TArray<FGcSidecarFileStat> WalkSidecars();

	/**
	 * Project-wide regeneration, or check. `FGraphCodeGenerator::Run` — the same call
	 * `GcRegenerate` and `GcCheck` make, so the panel and CI cannot drift.
	 *
	 * Loads assets. Slow by nature; this is the button, not the refresh.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static FGcSyncReport SyncAllSidecars(bool bCheckOnly);

	/** One package. `/Game/UI/W.W`, `/Game/UI/W` and `/Game/UI/W.W:Sub` all name the same one. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool SyncAsset(const FString& AssetPath, bool bDryRun, FString& OutStatus, FString& OutError);

	/**
	 * Run a read-only git command. Refuses any verb that could move HEAD, the index or the tree.
	 *
	 * Exposed because a panel always needs one query nobody anticipated. The allowlist is what
	 * keeps that from becoming a git client (§4.1).
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static FGcGitResult RunGit(const TArray<FString>& Args);

	/** `git show <Rev>:<path>`, with the path given as an absolute file path. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool GetFileAtRevision(
		const FString& Rev, const FString& AbsolutePath, FString& OutText, FString& OutError);

	/**
	 * Every local branch, and which one is checked out.
	 *
	 * WHY THE PANEL NEEDS THEM AT ALL. A comparison used to be four fixed pairs, all of which had
	 * `HEAD` or the working file on one side — so the question a person asks before every merge,
	 * "what does this asset look like on the branch I am about to merge", had no answer here and
	 * had to be asked of a git client that cannot read a `.uasset`.
	 *
	 * Local branches only. A remote-tracking ref is a cached copy of somebody else's work whose
	 * freshness depends on a `fetch` this panel is not allowed to run, and offering one that is
	 * three days stale under the same word as a branch that is current would be the panel lying
	 * about which side it read.
	 *
	 * @param OutCurrent  the checked-out branch, or empty on a detached HEAD or an empty repository.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static void ListBranches(TArray<FString>& OutBranches, FString& OutCurrent);

	/**
	 * The same list, and each branch's TIP TIME, **out of the same one process**. E11.12.
	 *
	 * **A BRANCH HAS A PLACE ON A TIME AXIS AND THE PANEL'S AXIS IS ONE** — user-directed 2026-08-05:
	 * *"the git branch should also filter by time determined by local history time and show time"*,
	 * then *"why dont, just give each one an epoch or something like that, then sort it?"*. That is
	 * what this exists for: it is the epoch, and `UGraphCodeVcsWidget::OrderOf` is the sort.
	 *
	 * **THE COST IS THE REASON IT IS HERE RATHER THAN IN A LOOP.** `git log -1 --format=%cI` per
	 * branch is 28 processes in this checkout, on a control somebody just pulled down; `for-each-ref`
	 * already walks every ref and will print any field asked of it, so the tip times cost NOTHING
	 * beyond the list itself. A second field is free; a second process per branch is the ~100 ms E11.8
	 * spent a whole pass removing.
	 *
	 * `%09` is git's own escape for a tab, and a tab cannot appear in a refname — so the split is
	 * exact without a rule about which characters a branch is allowed to contain.
	 *
	 * @param OutTipTimes  branch name → when that branch last moved, in UTC. A ref whose date does
	 *                     not parse is absent rather than epoch: `OrderOf` reads a missing entry as
	 *                     *"no time known"*, which is the honest answer and not "1 Jan 0001".
	 */
	static void ListBranchesWithTips(
		TArray<FString>& OutBranches, TMap<FString, FDateTime>& OutTipTimes, FString& OutCurrent);

	/**
	 * The newest commits reachable from a ref, newest first. E11.26, `GC-LOCAL-HISTORY-DESIGN.md` §5.3.
	 *
	 * ONE PROCESS FOR THE WHOLE LIST, like `ListBranchesWithTips` above and for the same reason: a
	 * `git log -1` per entry would be `Max` processes on a control somebody just pulled down.
	 *
	 * **THE SUBJECT IS LAST ON THE LINE ON PURPOSE.** The fields are tab-separated and a commit
	 * subject may contain a tab; sha and date cannot, so splitting twice from the left is exact
	 * without a rule about what a person may type in a commit message.
	 *
	 * @param Ref        a branch, `HEAD`, or a sha. Empty is refused rather than defaulted, because
	 *                   "the commits of nothing" is a caller bug and `HEAD` is a different answer.
	 * @param Max        how many to return. The caller draws a picker, not a log.
	 * @param OutbMore   true when git had more than `Max` — asked for by requesting one extra and
	 *                   dropping it, which costs nothing and is the only honest way to know.
	 */
	static bool ListCommitsOn(
		const FString& Ref, int32 Max, TArray<FGcCommitEntry>& OutCommits, bool& OutbMore);

	/**
	 * Every path git is mid-merge on, with the three blob hashes it is holding, in one process.
	 *
	 * WHAT IT IS FOR: identifying a MERGE, not a file. Nothing on disk can say "this merge has been
	 * dealt with" — a resolved sidecar that took ours is byte-identical to one nobody has touched,
	 * and git's index stays unmerged until the commit — so the panel has to remember, and a memory
	 * needs a key that changes when the thing it describes does. The stage triple is that key: a
	 * different merge on the same path is a different triple, so a stale record can never claim a
	 * new merge is finished.
	 *
	 * Keyed by repository-relative path; the value is `base:ours:theirs`, with an empty field for a
	 * stage git does not hold (a merge with no common ancestor has no base).
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static void UnmergedStageDigests(TMap<FString, FString>& OutDigests);

	/**
	 * Repository-relative paths that differ between two revisions — `git diff --name-only`.
	 *
	 * WHAT A COMPARISON IS A LIST OF. The picker names two sources and the file list is the files
	 * those two sources disagree about; deciding that per row would be one `git show` per sidecar,
	 * which is hundreds of processes for a question git answers in one.
	 *
	 * @param RightRev  empty means the working tree, which is what `git diff <rev>` already means.
	 *                  Untracked files are absent by nature — git cannot diff what it has never
	 *                  seen — so a caller that shows a working tree must add those itself.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static TArray<FString> ChangedPathsBetween(const FString& LeftRev, const FString& RightRev);

	/**
	 * Do these two `.gc` documents differ anywhere a PERSON wrote, rather than only in their digests?
	 *
	 * **THE ONE SPELLING OF "IS THIS A CHANGE", and it is public because having two of them is a bug
	 * this project has now shipped twice.** `sig:` and `bin:` are digests OF the body: `sig:` moves on
	 * every edit, and `bin:` moves whenever the `.uasset` is re-saved — which happens for reasons that
	 * have nothing to do with content, so a `.gc` can differ from a branch in `bin:` ALONE while every
	 * line anybody authored is identical.
	 *
	 * The scan learned that on 2026-08-04. The comparison did not, because E10.11a then handed the
	 * scan `AlsoKeep` — the comparison's own scope — as a set it may never drop, and that scope came
	 * from `git diff`, which counts the line. `BP_WindowProperties.gc` was drawn `+1 −1` against
	 * `branch main (current)` for a fortnight of digests. A spec asserting the list with its own byte
	 * comparison would have been the third spelling and the next place this reappears.
	 *
	 * Both sides are expected LF-normalised; trailing whitespace per line is ignored.
	 */
	static bool DocumentsDifferBeyondDigests(const FString& A, const FString& B);

	/**
	 * A `gc 1 …` header line with `sig:` and `bin:` taken out and the whitespace collapsed.
	 *
	 * **PUBLIC BECAUSE IT WAS WRITTEN TWICE**, byte for byte, in `GraphCodeVcsLibrary.cpp` and
	 * `GraphCodeVcsWidget.cpp` — two anonymous-namespace copies of the one predicate this project
	 * has already shipped two answers to (see `DocumentsDifferBeyondDigests`). They coexisted
	 * silently because internal linkage kept them apart, and a unity build that happened to put both
	 * files in one translation unit turned it into a redefinition error. That is a lucky failure:
	 * the real defect was two copies of a rule about what counts as a change, either of which could
	 * be narrowed without the other.
	 */
	static FString HeaderWithoutDigests(const FString& Line);

	/**
	 * The asset a `.gc` describes, projected as it is in memory right now.
	 *
	 * The first half of `DiffSidecarAgainstLive`, split out because the editor is a comparison
	 * SOURCE now rather than one fixed comparison: it can be put opposite a branch, and that pair
	 * has no sidecar-versus-live diff to borrow.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool ProjectLiveForSidecar(
		const FString& AbsoluteSidecarPath, FString& OutText, FString& OutError);

	/**
	 * Line diff of two texts.
	 *
	 * A `.gc` is deterministic LF text, so this is an ordinary text diff with no engine in it —
	 * which is the entire point of the format and the screen that demonstrates it.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	static TArray<FGcDiffLine> DiffText(const FString& Left, const FString& Right);

	/**
	 * What the merge driver would produce for a conflicted `.gc`, without touching anything.
	 *
	 * Reads the three sides straight out of git's index (`:1:`, `:2:`, `:3:`) and runs the same
	 * `FGcDocumentMerge::Merge` the driver runs. No temp file, no second editor process.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static FGcMergePreview PreviewMerge(const FString& AbsoluteSidecarPath);

	/** Every unresolved conflict in a `.gc`'s text, as fields rather than as text. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static TArray<FGcConflictView> ListConflicts(const FString& Text, FString& OutError);

	/**
	 * Resolve one conflict and return the new text. Writes nothing.
	 *
	 * Two steps on purpose, and §4.9 is the reason: resolving PROPOSES and applying COMMITS. The
	 * exact-or-refuse rule inside `FGcResolve::Pick` stands — if the file moved under the
	 * construct, this reports that rather than guessing which line was meant.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool ResolveConflict(
		const FString& Text,
		int32 ConflictId,
		EGcResolveChoice Choice,
		const FString& CustomValue,
		FString& OutText,
		FString& OutError);

	/** Read a `.gc` off disk, CRLF folded to LF so it compares against everything else here. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool ReadSidecarText(
		const FString& AbsoluteSidecarPath, FString& OutText, FString& OutError);

	/** Write resolved text back over a `.gc`. LF is preserved; nothing else is touched. */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool WriteSidecarText(
		const FString& AbsoluteSidecarPath, const FString& Text, FString& OutError);

	/**
	 * Project the asset as it is IN MEMORY RIGHT NOW — `gc_read source:"live"`.
	 *
	 * The asset is loaded if it is not already. If it IS already loaded and dirty, this is the
	 * unsaved state, which is the entire point: it is the only view of an edit that has not reached
	 * the disk yet.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool ProjectLiveDocument(const FString& AssetPath, FString& OutText, FString& OutError);

	/**
	 * The row git cannot give you: the `.gc` on disk versus what the asset projects to right now.
	 *
	 * Left is the file, right is the live asset, so a `+` line is an editor edit nobody has saved.
	 * git can only ever compare two things that reached the disk; between a save and the next one
	 * this is the only diff there is, and it is the one an author actually wants mid-edit.
	 *
	 * One node rather than eight, because the panel would otherwise carry the read, the parse, the
	 * projection and the diff as separate statements in a Blueprint graph, and a graph is a bad
	 * place to keep a sequence whose only reason to exist is that it is a sequence.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static TArray<FGcDiffLine> DiffSidecarAgainstLive(
		const FString& AbsoluteSidecarPath, FString& OutError);

	/**
	 * Fold the editor's UNSAVED state into the merge result sitting in the working tree. M1c.
	 *
	 * The three inputs, and where each comes from, because the choice of source is most of what
	 * makes this sound:
	 *
	 *   ours   `git show HEAD:<path>` — during a merge HEAD is still our side, whether or not git
	 *          has marked the path resolved, so this works for both a driver merge that came out
	 *          clean and one that conflicted and was then resolved.
	 *   M      the `.gc` on disk. Whatever produced it — the driver, `gc_resolve`, a person — that
	 *          file is what the merge currently says, and reproducing the merge here instead would
	 *          be a second opinion nobody asked for.
	 *   live   `ProjectLiveDocument`, the loaded object INCLUDING unsaved edits.
	 *
	 * IT WRITES NOTHING, and in particular it never reconciles into the in-memory object. §4.13:
	 * the object is snapshotted and discarded. The output is a proposal for `gc_apply`, exactly
	 * like every other merge result, and the asset is reload-pending until it lands.
	 *
	 * REFUSES RATHER THAN GUESSES when it cannot establish that the memory is derived from ours —
	 * see `FGcLiveBaseline`. An asset held open across an earlier pull has an unknown base and the
	 * whole two-three-way decomposition stops being sound; a refusal that says so is cheap and a
	 * merge that quietly assumed it is not.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static FGcLiveMergeView PreviewLiveMerge(const FString& AbsoluteSidecarPath);

	/**
	 * Open the merge panel named by `UGraphCodeSettings::MergePanelWidget`. E6.1.
	 *
	 * HERE RATHER THAN IN THE TOOLBAR BLUEPRINT, and it is the same rule as everything else in this
	 * file: E6's toast already has to open the panel, so a Blueprint that reached for
	 * `SpawnAndRegisterTab` itself would be a second implementation of one action — and the two
	 * would then disagree about which asset the panel is the moment somebody changes the setting.
	 * The toolbar entry is still "no C++" in §3's sense: it needs no new extension point and no new
	 * module, only the wrapper this file exists to be.
	 *
	 * @return false when the setting names nothing loadable, so a caller can say so rather than
	 *         appearing to have done something.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool OpenMergePanel();

	/**
	 * How many sidecars need attention right now, read from the FILES with no asset loaded.
	 *
	 * E2.3 asked for this for E6's badge and pointed at `FGraphCodeGenerator::Run(bCheckOnly)`.
	 * **That is the wrong source for a badge and the difference is seconds versus minutes**: the
	 * generator LOADS every asset to re-project it, which is the button, not the refresh. The badge
	 * asks the cheap question — is any committed `.gc` conflicted or an unabsorbed proposal — and
	 * that is a text property `GetSidecarStatus` already computes without touching a package.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static void CountSidecarsNeedingAttention(int32& OutConflicted, int32& OutProposals);

	/**
	 * Diff lines as one displayable block, `-`/`+`/space prefixed.
	 *
	 * Here rather than in the panel because building a string in a Blueprint loop is a page of
	 * nodes that says nothing, and every client would write the same page.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	static FString FormatDiff(const TArray<FGcDiffLine>& Lines, bool bChangedLinesOnly = true);

	/**
	 * The same diff as TWO DOCUMENTS rather than one interleaved stream — the earlier version above,
	 * the later one below, each line carrying its own mark.
	 *
	 * **IT IS THE SAME DIFF LAID OUT TWICE, NOT A SECOND COMPARISON.** Both panes are rendered from
	 * the one `FGcDiffLine` array `DiffText` produced, so a caller's `+`/`−` counts and the marks on
	 * the page cannot disagree. A layout that recomputed the comparison would be two answers to one
	 * question, which is the whole reason this sits beside `FormatDiff` instead of in a caller.
	 *
	 * With `bChangedLinesOnly` FALSE each pane is that version's WHOLE document, every line marked —
	 * a space where the two agree — so the panes stay aligned by construction and need no line
	 * numbers to be read against each other. With it TRUE the top holds only what went and the
	 * bottom only what came: the changed-only answer in the same layout rather than a different one.
	 *
	 * @param FromLabel  the earlier version, in the caller's own words. Empty prints a bare `---`.
	 * @param ToLabel    the later one.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|VCS")
	static FString FormatDiffSplit(
		const TArray<FGcDiffLine>& Lines,
		const FString& FromLabel,
		const FString& ToLabel,
		bool bChangedLinesOnly = true);

	/**
	 * Apply a `.gc` file on disk to its asset — `FGraphCodeApply::ApplyDocument`.
	 *
	 * Refuses a conflicted file by name, already. This is the second of the two buttons; it is
	 * never folded into the first.
	 *
	 * @param OutBlocksWritten  HOW MANY BLOCKS IT ACTUALLY WROTE, and zero-with-success is a real
	 *                          answer rather than a contradiction. A proposal whose every block
	 *                          already matches the asset applies successfully and writes nothing —
	 *                          correct, and indistinguishable from a proposal that wrote six unless
	 *                          the caller is told. The panel needs the difference twice: it must not
	 *                          claim the asset is dirty when it is not, and a file with nothing left
	 *                          to deliver has to stop calling itself an unapplied proposal instead
	 *                          of sitting in the list saying "unapplied" with nothing to apply.
	 */
	/**
	 * Write T, the global timeline stamp, if this project has none. **Only ever call this after a
	 * CLEAN whole-project sync**, which is the one event that supports the claim it makes: *at this
	 * instant the text GraphCode holds described every covered asset exactly*.
	 *
	 * **PUBLIC BECAUSE THE HEADLESS DOOR NEEDED IT AND WAS QUIETLY WITHOUT IT.** `SyncAllSidecars`
	 * stamped inline, so pressing `Rewrite all .gc` in the panel stamped and `-run=GcRegenerate` —
	 * the same operation, from a build script or an agent — did not. A project synced only from the
	 * commandlet therefore refused every timeline revert by name and said it was unstamped, with
	 * nothing anywhere explaining which of the two doors writes it.
	 *
	 * @return true when there is something to say about the attempt; false is the ordinary case of a
	 *         project that already carries a stamp, which never moves once written.
	 */
	static bool StampTimelineAfterCleanSync(FString& OutMessage);

	UFUNCTION(BlueprintCallable, Category = "GraphCode|VCS")
	static bool ApplySidecarFile(
		const FString& AbsoluteSidecarPath,
		bool bDryRun,
		FString& OutSummary,
		TArray<FString>& OutRefusals,
		FString& OutError,
		int32& OutBlocksWritten);
};
