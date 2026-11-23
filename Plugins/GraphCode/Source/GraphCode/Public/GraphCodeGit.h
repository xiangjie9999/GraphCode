#pragma once

#include "CoreMinimal.h"

/**
 * The editor surface's guarded `git` runner. GC-EDITOR-VCS-DESIGN.md §4.1.
 *
 * WHY THIS IS NOT `ISourceControlProvider`. The engine ships a Git provider
 * (`Engine/Plugins/Developer/GitSourceControl`) and it is the wrong shape for this: it models
 * check-out/check-in over a working copy, it has no three-way merge entry point, and it exposes
 * nothing to Blueprint. What the panel needs is four read-only queries and the three merge stages
 * out of the index, which is a smaller thing than a source-control provider, not a larger one.
 *
 * TWO RULES IT DOES NOT GET TO BREAK.
 *
 * **It never moves HEAD, the index or the working tree.** The verb allowlist is read-only and
 * closed: a panel that can commit is a panel that can lose work in a way the user cannot see, and
 * staging is a decision rather than a step. Resolving writes the one `.gc` the user is looking at,
 * through ordinary file IO, and stops there.
 *
 * **No shell, and every argument is quoted here.** `FPlatformProcess::ExecProcess` spawns the
 * executable directly — there is no `cmd /c` and no metacharacter interpretation — but Windows
 * still hands the child a single command line, so an argument holding a space has to be quoted by
 * us. An argument holding a quote or a control character is REFUSED rather than escaped: every
 * argument this surface passes is a ref, a path or a fixed flag, so a quote in one means the caller
 * built something we did not intend, and guessing what they meant is how an injection lands.
 */
struct GRAPHCODE_API FGraphCodeGitResult
{
	/** True when the process was launched at all. False means it was refused or git is missing. */
	bool bRan = false;

	int32 ExitCode = -1;
	FString StdOut;
	FString StdErr;

	/** Why it was refused, or why the launch failed. Empty when bRan is true. */
	FString Error;

	/** The exact argument list, for a panel that wants to show what it ran. */
	TArray<FString> Args;

	bool Ok() const { return bRan && ExitCode == 0; }
};

/**
 * WHAT THE TWO HALVES OF THE GIT MERGE DOOR SAY, AND WHETHER THEY AGREE. §5.0a, row S3.4.
 *
 * **THE DOOR HAS TWO HALVES AND ONLY ONE OF THEM CAN BE COMMITTED.** `.gitattributes` is shared;
 * the driver is registered in `.git/config`, which is per-clone. So a teammate can pull an
 * attribute naming a driver their clone has never heard of, and **git does not say a word about
 * it** — measured in a scratch repository 2026-08-21: an attribute naming an unregistered driver
 * falls back to git's DEFAULT LINE MERGE, interleaves both sides of a `.gc`, and reports
 * *"Auto-merging"* with exit 0 ([[an-unregistered-merge-driver-fails-open]]).
 *
 * That is precisely what `merge=binary` exists to prevent, and no git client reports the pair. This
 * is the thing that does.
 */
struct GRAPHCODE_API FGcMergeDoorState
{
	/** True when BOTH queries ran. False means the verdict below is not evidence of anything. */
	bool bChecked = false;

	/** The repo-relative path the attribute was asked about — attributes match paths, not files. */
	FString ProbePath;

	/** As git prints it: a driver name, or `unspecified` / `unset` / `set`. */
	FString Attribute;

	/** Whether `merge.<name>.driver` has a value in THIS clone. */
	bool bDriverRegistered = false;

	/** What it is registered as, for a panel that wants to show it. */
	FString DriverCommand;

	/** Why the check could not be made. Empty when `bChecked`. */
	FString Error;

	/** The attribute names a driver rather than a built-in strategy or nothing at all. */
	bool NamesADriver() const;

	/**
	 * **THE ONE THAT MATTERS**: the attribute sends `.gc` to a driver this clone cannot call.
	 *
	 * Deliberately NOT true when the attribute is `binary` and no driver is registered — that is
	 * this checkout's shipped state and it is safe, because `binary` conflicts whole rather than
	 * interleaving.
	 */
	bool IsDangerous() const;

	/** One sentence, in plain words, saying what the pair is and what it means. */
	FString Report() const;
};

class GRAPHCODE_API FGraphCodeGit
{
public:
	/**
	 * The repository root containing this project, with forward slashes and no trailing slash.
	 *
	 * Empty when the project is not inside a git work tree, which is a normal state and not an
	 * error — a project on a shared drive with no VCS still saves sidecars.
	 *
	 * Cached after the first successful answer. The root of a checkout does not move under a
	 * running editor, and re-asking would spawn a process per status row.
	 */
	static const FString& RepositoryRoot();

	/** Forget the cached root. For specs, and for a project that was `git init`ed mid-session. */
	static void ResetRepositoryRootCache();

	/** The closed, read-only verb list. Anything else is refused by name. */
	static bool IsAllowedVerb(const FString& Verb);

	/** Every allowed verb, so a spec asserts the list rather than sampling it. */
	static const TArray<FString>& AllowedVerbs();

	/**
	 * Whether this exact argument list may run: the verb list, PLUS one form-level exception.
	 *
	 * **`config` STAYS OFF THE VERB LIST AND THAT RULING IS NOT REOPENED HERE.** It reads like a
	 * query and writes with one more argument, which is why `EveryVerbThatCouldMoveHeadIsRefusedByName`
	 * names it beside `branch`. But §5.0a needs ONE reading of it — `git config --get
	 * merge.graphcode.driver` — and that exact form has no writing behaviour at all.
	 *
	 * So the exception is on the FORM and not on the verb: exactly three arguments, exactly
	 * `--get`, and a key that does not itself open with `-`. `config`, `config --get`,
	 * `config --unset x`, `config x y` and `config --get -x` are all still refused, and
	 * `IsAllowedVerb("config")` still answers false — which keeps the existing assertion honest
	 * rather than edited around.
	 */
	static bool IsAllowedInvocation(const TArray<FString>& Args, FString& OutReason);

	/**
	 * Build the door's state from the two commands' raw output. Pure — no process, no repository.
	 *
	 * Split out from `MergeDoorState` for the reason `FGcNotify` splits compose from raise: the
	 * interesting cases are a clone this suite cannot create, and a check whose only test is
	 * "run it here and see" asserts one configuration — this one — and calls it a rule.
	 */
	static FGcMergeDoorState ComposeMergeDoorState(
		const FString& ProbePath, const FString& CheckAttrStdOut, bool bCheckAttrRan,
		const FString& ConfigStdOut, bool bConfigFound);

	/**
	 * Ask this clone: `git check-attr merge -- <ProbePath>` and `git config --get merge.<n>.driver`.
	 *
	 * @param ProbePath  Repo-relative, and it need NOT exist — an attribute is matched against a
	 *                   PATH. Empty asks about a representative `.gc` under `Content/`.
	 */
	static FGcMergeDoorState MergeDoorState(const FString& ProbePath = FString());

	/**
	 * Reject an argument that cannot be passed safely. Out param carries the reason.
	 *
	 * Quotes, newlines and other control characters. See the class comment for why this refuses
	 * rather than escapes.
	 */
	static bool IsSafeArgument(const FString& Arg, FString& OutReason);

	/** Build the command line one argument at a time. Exposed so a spec can assert the quoting. */
	static FString BuildCommandLine(const TArray<FString>& Args);

	/** Run `git <Args>` with the repository root as the working directory. */
	static FGraphCodeGitResult Run(const TArray<FString>& Args);

	/**
	 * An absolute path as the repository spells it: relative to the root, forward slashes.
	 *
	 * Refuses a path outside the repository. Everything downstream passes a path to git, so this
	 * is the one place that has to be certain the panel is not addressing another checkout.
	 */
	static bool ToRepoRelative(const FString& AbsolutePath, FString& OutRelative, FString& OutError);

	/** `git show <Rev>:<RepoRelativePath>`. */
	static bool ShowBlob(
		const FString& Rev, const FString& RepoRelativePath, FString& OutText, FString& OutError);

	/**
	 * Read many blobs from one revision with one `git cat-file --batch` process.
	 *
	 * `ShowBlob` is the right primitive for one selected row. A project restore can resolve hundreds
	 * of changed sidecars at once, where starting one process per path costs far more than reading
	 * the files. Missing paths are a normal answer and are returned separately from process errors.
	 */
	static bool ShowBlobs(
		const FString& Rev, const TArray<FString>& RepoRelativePaths,
		TMap<FString, FString>& OutTexts, TSet<FString>& OutMissing, FString& OutError);

	/**
	 * One merge stage of a conflicted path: 1 base, 2 ours, 3 theirs — `git show :N:<path>`.
	 *
	 * This is what makes a merge preview cost nothing. During a conflicted merge git already holds
	 * all three sides in the index, so the driver's exact inputs are two process calls away and no
	 * temp file, no second editor and no driver invocation is involved.
	 */
	static bool ShowStage(
		int32 Stage, const FString& RepoRelativePath, FString& OutText, FString& OutError);

	/**
	 * The unmerged stages git holds for a path, ascending. Empty means no merge is in progress for
	 * it — which a preview reports rather than working around.
	 */
	static bool UnmergedStages(
		const FString& RepoRelativePath, TArray<int32>& OutStages, FString& OutError);

	/**
	 * `git status --porcelain` codes for paths under a directory, keyed by repo-relative path.
	 *
	 * One call for a whole panel refresh rather than one per row. A path absent from the map is
	 * unmodified, which is the common case and is why absence rather than a code means it.
	 */
	static bool StatusCodes(
		const FString& RepoRelativeDirectory, TMap<FString, FString>& OutCodes, FString& OutError);

	/** How many lines a path gained and lost against the last commit. */
	struct FLineChange
	{
		int32 Added = 0;
		int32 Removed = 0;
	};

	/**
	 * `git diff HEAD --numstat` for paths under a directory, keyed by repo-relative path.
	 *
	 * ONE CALL FOR A WHOLE PANEL REFRESH, like `StatusCodes` above and for the same reason: a number
	 * per row is only worth having if it costs nothing per row.
	 *
	 * `HEAD` rather than the index, because the question a person asks of a list of changed files is
	 * "how much of this is new since the last commit", and a diff against the index answers a
	 * different one for any file that is partly staged. A path git cannot count — an unmerged one,
	 * or a binary — prints `-` for both columns and is reported as zero rather than guessed at.
	 */
	/**
	 * `git diff <Rev> --numstat`, as a map from repo-relative path to (added, removed).
	 *
	 * @param Rev  Empty means `HEAD`, which is what every caller wanted until a comparison could be
	 *             against something else. Pass a branch and the counts describe THAT comparison —
	 *             without it every row in the panel was measured against the last commit no matter
	 *             which source the reader had chosen, so a branch comparison drew no counts at all.
	 */
	static bool LineChanges(
		const FString& RepoRelativeDirectory, TMap<FString, FLineChange>& OutChanges,
		FString& OutError, const FString& Rev = FString());
};
