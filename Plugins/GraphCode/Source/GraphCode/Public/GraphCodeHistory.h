#pragma once

#include "CoreMinimal.h"

/**
 * E11.0/E11.1 — LOCAL HISTORY: the previous canonical body of a `.gc`, kept where git is not.
 *
 * `GC-LOCAL-HISTORY-DESIGN.md` owns the behaviour and the safety rulings; this file is the contract
 * and the store. Read that document before changing anything here — several of the shapes below look
 * like over-engineering until you know which measurement produced them.
 *
 * WHAT THIS IS FOR, IN ONE SENTENCE: a `.uasset` is binary and a `.gc` is a projection of it, so the
 * moment an asset is saved the previous text is gone from everywhere except a commit that may never
 * have been made. Git covers what you committed. This covers the last ten saves.
 *
 * WHAT IT IS NOT: a replacement for git, and never a claim about state GraphCode does not carry. A
 * `.gcc` is ordinary canonical GraphCode text and therefore inherits that text's `omits:` exactly —
 * so restoring one restores what the format represents and NOTHING ELSE, which is why §2.5 forbids
 * ever labelling an action "Revert Entire File" (see `EGcRestoreAction`).
 */

/** Why an entry was written. Normal saves and pre-revert checkpoints share the store, not the reason. */
enum class EGcHistoryReason : uint8
{
	/** A save replaced the canonical text with a logically different one. The ordinary entry. */
	Save,

	/**
	 * The live state, captured immediately BEFORE a revert mutates the asset (§2.4).
	 *
	 * Projected from the loaded asset rather than copied off disk, because the disk `.gc` trails
	 * unsaved editor work — and the whole value of this entry is that it holds what was about to be
	 * overwritten, including the part that had never been saved.
	 */
	PreRevert,

	/** Written by hand or by a tool through the service's own API. */
	Manual,

	/**
	 * A PROJECTION REFRESH replaced the text with a logically different rendering of the SAME asset
	 * — a corpus sweep, or the re-projection after an apply's save.
	 *
	 * **IT IS AN ENTRY, AND IT USED NOT TO BE.** The refresh scope suppressed the archive outright
	 * on the reasoning that a sweep supersedes machine-written text with no authored version
	 * underneath it. That is true of the ASSET and false of the FILE: when `projector:` moves, the
	 * body genuinely changes, the old rendering is the only copy of itself, and discarding it left
	 * assets nobody had edited holding exactly one entry forever — measured 2026-08-27 as 137 of
	 * 147 real packages, against a 5.4 MB store with a 256 MB budget. The cost the suppression was
	 * avoiding was never being paid.
	 *
	 * It is a SEPARATE reason rather than a `Save` so the panel can say which of the two happened,
	 * and so a future retention rule can prefer authored entries without having to guess.
	 * `ArchivePredecessor` still refuses an identical or envelope-only body, so a sweep over an
	 * unchanged corpus costs nothing at all.
	 */
	Refresh,
};

/**
 * What happened to the ASSET at this transition, which is not the same question as what happened to
 * its text (§2.7).
 *
 * A future cutoff planner has to be able to distinguish "unchanged since then" from "did not exist
 * then", and no amount of predecessor text can say the second. That is what the tombstone kinds are
 * for, and recording them is the whole of what V1 owes V2.
 */
enum class EGcHistoryTransition : uint8
{
	/** The asset began existing here. No predecessor text — this entry IS the statement of absence. */
	Created,

	/** The ordinary case: it existed before and its represented state changed. */
	Updated,

	/** The package moved. Carries `PreviousPackageName` so the lineage stays walkable. */
	Renamed,

	/** The asset stopped existing. Carries the recovery-trash path — never a permanent delete. */
	Removed,
};

/**
 * One immutable history record.
 *
 * `EntryId` is the identity everything else refers to, and it is the archive's own basename: a
 * UTC timestamp to the millisecond plus a short body digest, with a numeric suffix when even that
 * collides. §2.3 — two saves in one clock tick may not overwrite one another, and a name that can
 * be recomputed from its contents is a name that cannot drift from them.
 */
struct FGcHistoryEntry
{
	/** Immutable, unique within the package's history directory. The `.gcc` basename. */
	FString EntryId;

	/** Full long package path — `/Game/UI/WBP_Menu`. NEVER the basename; see §2.3. */
	FString PackageName;

	/** The `kind:` token the archived document declared, for reading the list without opening it. */
	FString Kind;

	/** `sig:` of the archived body. What "logically different" is decided against (§2.2). */
	FString BodySignature;

	FDateTime TimeUtc;

	EGcHistoryReason Reason = EGcHistoryReason::Save;

	EGcHistoryTransition Transition = EGcHistoryTransition::Updated;

	/** Ties several entries to one restore, so a multi-asset revert can be reasoned about later. */
	FString GroupId;

	/** `Renamed` only: where this lineage came from. */
	FString PreviousPackageName;

	/** `Removed` only: where the `.uasset` and sidecar were moved to. Not a deletion. */
	FString RecoveryPath;

	/** Absolute path to the `.gcc`, or empty for a tombstone that carries no payload. */
	FString ArchivePath;

	/** What this entry costs on disk. Zero for a tombstone, which is one index line and no file. */
	int64 SizeBytes = 0;

	/** A `Created` transition has no predecessor, so there is nothing to read back. */
	bool IsTombstone() const { return ArchivePath.IsEmpty(); }
};

/**
 * What the store can HONESTLY say about an asset at some past instant.
 *
 * **THREE ANSWERS, BECAUSE TWO IS THE BUG** — user-directed 2026-08-05: *"if we delete based on
 * oldest, it might appear to be that the file havent existed yet"*. Exactly so. An empty result was
 * being asked to mean both "this did not exist" and "we no longer hold this", and those are opposite
 * facts: the first says a restore should DELETE the asset, the second says a restore must refuse.
 * A caller cannot get that wrong by accident if the type will not let it.
 */
enum class EGcHistoryKnowledge : uint8
{
	/** A creation tombstone proves the asset began existing later than the instant asked about. */
	DidNotExist,

	/** Every entry that covered this instant still survives, so the state is exactly recoverable. */
	Known,

	/**
	 * The store held this once and does not any more, or never reached back this far.
	 *
	 * **NEVER SILENTLY TREATED AS `DidNotExist`.** That mistake turns "I pruned your oldest
	 * versions" into "this asset was not in the project then", which is how a project-wide restore
	 * would delete somebody's work to reproduce a past that never happened.
	 */
	Unknown,
};

/**
 * One archive, as its FILENAME describes it — read without opening anything.
 *
 * **THE TIME MACHINE'S WHOLE DATA SOURCE.** User-directed 2026-08-05: put the history in the file
 * name, `BP_Player_2026_08_08_20_12_34_007_a1b2c3d4.gcc`. Which assets have a state at which
 * instants is then one directory glob — no index read, no file read, no package load — which is
 * what lets a date control redraw a project-wide list while somebody drags it.
 *
 * `AssetName` IS THE SHORT NAME AND IS NOT IDENTITY (§2.3): two assets can share it. The directory
 * still carries the full package path, and anything resolving an entry to an ASSET must use that.
 * Only the list may read the name.
 */
struct FGcArchiveName
{
	/** The `.gcc` basename, which is also the entry's immutable id. */
	FString EntryId;

	/** `BP_Player` — the package's short name, as the file is called. Not identity. */
	FString AssetName;

	FDateTime TimeUtc;

	FString ArchivePath;
};

/**
 * The store. Every write is atomic and containment-checked; nothing here loads an asset.
 *
 * CONTAINMENT IS CHECKED, NOT ASSUMED (§3). A package name arrives from an asset path and becomes
 * directories, so a `..` that survived sanitisation would let this write anywhere on the disk. The
 * `.gitignore` entry is a statement of intent and is not a filesystem sandbox; `WithinRoot` is.
 */
class GRAPHCODE_API FGraphCodeHistory
{
public:
	/** `Saved/GraphCode/History/`, absolute, with a trailing slash. */
	static FString Root();

	/**
	 * `05 Aug 2026 14:32` — one UTC instant in the reader's own clock. Empty for epoch.
	 *
	 * **IT EXISTS BECAUSE `FDateTime::ToString` HAS NO `%b` AND DOES NOT SAY SO.** Found 2026-08-05
	 * by a spec that asked for a month name and got the letter `b`: FOUR files were formatting
	 * `%d %b %Y`, so every instant any of them drew outside today read `05 b 2026`. An unsupported
	 * specifier is emitted as its literal character rather than refused, which is precisely the
	 * failure a format string cannot warn you about — and it survived because the store is young
	 * enough that almost everything the panel drew took a same-day branch that never formatted a
	 * month.
	 *
	 * One spelling, in the module that owns instants, so the next caller cannot reintroduce it.
	 *
	 * @param bWithYear  false for a compact stamp beside a caption, where the year is noise until
	 *                   the project is a year old and the reader can get it from the one that has it.
	 */
	static FString LocalLabel(const FDateTime& Utc, bool bWithYear = true);

	/**
	 * `05 Aug 2026` — the same instant, date only, for a caller that spells the time its own way.
	 *
	 * @param bWithYear  false for `05 Aug`, which is what a caption wants when it is completing a
	 *                   label that already carries the clock. Same reason as `LocalLabel`'s.
	 */
	static FString LocalDateLabel(const FDateTime& Utc, bool bWithYear = true);

	/** Local time from a UTC instant. The store is UTC so it can be ordered; a person reads a clock. */
	static FDateTime ToLocalTime(const FDateTime& Utc);

	/**
	 * The clock words a person types, as a UTC instant. The inverse of `LocalLabel`, loosely.
	 *
	 *   1am, 1:30pm, 13:00, 13:00:05     the MOST RECENT time that reads that way — today if it has
	 *                                    already passed, yesterday if it has not
	 *   today, yesterday                 that local day's midnight
	 *   today 3pm, yesterday 13:00       that local day at that clock
	 *   2026-08-28, 2026-08-28 09:00     an explicit local date, with or without a clock
	 *   2h ago, 30m ago, 3 days ago      counted back from this instant (s/m/h/d/w)
	 *
	 * ISO-8601 is NOT handled here: `FDateTime::ParseIso8601` already does it, it carries its own
	 * zone, and a second parser for it would be a second answer to one question. Callers try that
	 * first and this second — which is also the order that keeps an ISO string with an offset from
	 * being read against the local clock.
	 *
	 * **LOCAL, ON PURPOSE AND THROUGHOUT.** `LocalLabel` exists because a person reads a clock, and
	 * somebody typing `1am` means the one on their own wall. A UTC reading would be silently wrong
	 * by the offset — the shape of the defect a spec hit typing a rendering of a time and pinning
	 * itself to the timezone it was written in.
	 *
	 * **EVERY FORM IT ACCEPTS IS A RESERVED WORD WHEREVER VERSIONS ARE ALSO GIT REFS**, so the set
	 * was kept small deliberately: `today` and `yesterday` are the only bare words, and every other
	 * form carries a `:`, a space or an `am`/`pm` — the first two being characters a git ref may not
	 * contain at all. A bare number is never a clock, because `13` is also a short sha.
	 *
	 * Reads the wall clock and nothing else: no file, no store, no editor.
	 *
	 * @param OutHow  one clause naming the instant it became, for a caller that has to show its
	 *                arithmetic. `1am` at half past midnight is LAST night's, and nothing else in an
	 *                answer would say so.
	 */
	static bool ParseMoment(const FString& Spec, FDateTime& OutUtc, FString& OutHow);

	/** Where one package's entries live. Empty when the package name cannot be made safe. */
	static FString DirectoryFor(const FString& PackageName);

	/**
	 * **SPECS ONLY: move the WHOLE STORE to a scratch directory.** Empty puts it back.
	 *
	 * **THIS EXISTS BECAUSE THE PROBE-PACKAGE CONVENTION DOES NOT CONFINE A PROJECT-WIDE VERB.**
	 * Every spec in `GraphCodeHistoryTests.cpp` works on a package name no asset owns, and that is
	 * enough for `ArchiveText`, `List` and `RecordTransition`, which only ever touch the package
	 * they are handed. It is NOT enough for `Prune`, whose whole contract is that it walks the store
	 * — so a spec that set the budget to 1 MB and called it was pruning the developer's own history,
	 * down to the per-package floor of one entry, every time the gate ran. Measured 2026-08-28: 141
	 * of this project's 402 packages stripped to a single version, with `timeline-cutoff.txt` absent
	 * because the same spec's teardown deleted the record it had just written. The user's report was
	 * an asset worked on for weeks whose history began that morning.
	 *
	 * The lesson generalises past `Prune`: **a verb whose scope is the store needs a store of its
	 * own to be tested in**, and a naming convention is not one. Anything destructive added here
	 * later gets the same treatment.
	 *
	 * Not `#if WITH_DEV_AUTOMATION_TESTS` — this module ships that macro off in some configurations
	 * and a header that changes shape between them is how a link error appears in one target only.
	 * It is one static string and nothing in production ever sets it.
	 */
	static void SetRootForTests(const FString& AbsoluteDirectory);

	/** Settings, read through here so a caller never has to know where they live. */
	static bool IsEnabled();

	/**
	 * The WHOLE STORE's disk budget in bytes — **a figure to WARN above, not one anything enforces**
	 * (2026-08-28; see `Prune`). Zero stops new entries and deletes nothing.
	 */
	static int64 BudgetBytes();

	/** What the whole store currently occupies on disk, in bytes. One directory walk. */
	static int64 BytesUsed();

	/**
	 * How far back ONE asset can be taken. Epoch when it has nothing retained.
	 *
	 * Pass an empty package for the oldest surviving entry anywhere — but read `TimelineCutoff`
	 * before using that for anything, because it is **not** the date the project can be rebuilt to.
	 */
	static FDateTime OldestRetained(const FString& PackageName = FString());

	/**
	 * **THE DATE BEFORE WHICH THIS STORE CANNOT HONESTLY REBUILD THE PROJECT.** Epoch when nothing
	 * has ever been pruned, meaning the store is coherent all the way back to its oldest entry.
	 *
	 * User-directed 2026-08-05: *"it ensures that we can at least have a timeline where it can be a
	 * true fact"*. The reasoning, because it is not obvious and the first implementation got it
	 * wrong:
	 *
	 * An entry archives the state that was true BEFORE the save that replaced it, so reconstructing
	 * asset A at time T means finding A's earliest surviving entry after T — that payload is what A
	 * held at T. Delete A's entry at time *d* and every T < *d* stops being answerable for A: the
	 * next surviving entry describes a state that only began after *d*.
	 *
	 * So the project is truthfully reconstructible at T exactly when NO asset lost an entry after T,
	 * which makes the honest cutoff **the newest entry ever deleted, anywhere**. That figure cannot
	 * be recomputed from the survivors — deletion is what destroys the evidence of it — so it is
	 * persisted, and it only ever moves forward.
	 *
	 * **THIS IS WHY `OldestRetained()` MUST NOT BE SHOWN AS THE PROJECT'S CUTOFF.** The floor keeps
	 * every asset's last copy, so one asset edited once a year holds the oldest-surviving date at
	 * that date forever — while the assets around it have been trimmed and cannot go back anywhere
	 * near it. The store would be advertising a year of history it cannot deliver.
	 *
	 * And it is why `Prune` takes the globally OLDEST first: that is the order that moves this date
	 * the least. Freeing the same bytes from whichever asset has the most copies would delete a
	 * RECENT entry and jerk the cutoff forward to it.
	 */
	static FDateTime TimelineCutoff();

	/**
	 * **T — THE GLOBAL TIMELINE STAMP: the moment every covered asset was known to be in sync.**
	 * Epoch when the project has never been stamped.
	 *
	 * User-directed 2026-08-05, in two messages that are one ruling: *"we need to have a time where
	 * the plugin started to track all files, and also a date where everything before that can be
	 * treated as non existent (the T time), dictates how much you can revert"*, then *"there should
	 * be a global timeline stamp. so do it, like a metadata? stamp it once all assets are correctly
	 * synced"*.
	 *
	 * **WHY "ALL ASSETS SYNCED" AND NOT "THE FIRST ENTRY WAS WRITTEN".** The claim T has to support
	 * is *at this instant, the text GraphCode holds described every asset exactly*. A first-archive
	 * date supports nothing of the sort: it says one asset was observed once. A clean whole-project
	 * sync is the only event that makes the strong claim true, and `FGraphCodeGenerationReport::
	 * IsClean` is already exactly that event — every eligible asset has a current sidecar, nothing
	 * is stale, missing, conflicted or unreadable.
	 *
	 * **WHAT IT BUYS, AND IT IS THE DIFFERENCE BETWEEN A WORKING PROJECT REVERT AND A USELESS ONE:**
	 *
	 *  - **BEFORE T, NOTHING IS ANSWERABLE.** There is no evidence about any asset, so a
	 *    project-wide revert to an instant before T refuses outright rather than reconstructing a
	 *    project out of whatever happens to have survived.
	 *  - **AT OR AFTER T, SILENCE IS EVIDENCE.** An asset with NO entries used to answer `Unknown`,
	 *    because the store had never heard of it — so a project revert skipped every asset nobody
	 *    had edited, which is most of them, and reported the project reverted. With T it answers
	 *    `Known`: everything was correct at T, we have been watching since, and nothing was
	 *    recorded — so it has not changed, and what it looks like now is what it looked like then.
	 *
	 * Written ONCE and never moved. It is not `OldestRetained()`, which walks forward as entries are
	 * pruned: the moment the project was known-good does not un-happen.
	 */
	static FDateTime TrackingSince();

	/** How many covered assets the sync that stamped T accounted for. Zero when unstamped. */
	static int32 TrackedAssetsAtStamp();

	/**
	 * Was this package one of the assets that existed and was correct at T?
	 *
	 * **THE STAMP CARRIES THE PACKAGE LIST, NOT JUST THE DATE, AND THAT IS WHAT MAKES A REMOVAL
	 * ANSWERABLE.** User-directed 2026-08-05: *"so an asset not existed between the selected
	 * timeline (needs to be later than the time T), and the tracked since, can be deleted"*.
	 *
	 * A date alone can say *"we were watching"*. It cannot say *"this asset was not here"*, and
	 * without that the only assets a project revert could ever remove are the ones GraphCode itself
	 * watched being born. With the list, an asset absent from it was born after T — so at any
	 * instant before its first evidence it genuinely did not exist, and putting the project back to
	 * that instant means removing it.
	 *
	 * False for every package when the project is unstamped, which is why `KnowledgeAt` falls back
	 * to its per-asset rules there rather than reading absence as a statement.
	 */
	static bool WasCoveredAtStamp(const FString& PackageName);

	/**
	 * **THE EARLIEST INSTANT THE PROJECT CAN BE PUT BACK TO TODAY.** Epoch when unstamped.
	 *
	 * **TWO TIMES, AND THEY ARE NOT THE SAME TIME — user-directed 2026-08-05:** *"two times, one is
	 * earliest possible return of time, and one is tracked since"*. Keeping them apart is the whole
	 * of this pair, because they answer different questions and move differently:
	 *
	 * | | `TrackingSince` (T) | this |
	 * |---|---|---|
	 * | what it means | when the project was first known correct | how far back you can go **now** |
	 * | does it move | never | forward, every time the budget sweep takes an entry |
	 * | what makes it move | nothing | `TimelineCutoff` — *"delete the repeated file … until the storage is met"* |
	 *
	 * They are equal until the first prune. After that the GAP BETWEEN THEM IS WHAT RETENTION HAS
	 * COST, and a panel that showed only one of them would be hiding exactly that: show T alone and
	 * it advertises history the store has already deleted; show this alone and a reader cannot tell
	 * a young project from one that has been trimmed hard.
	 */
	static FDateTime EarliestRestorableInstant();

	/**
	 * Stamp T, if it has not been stamped before. Called by a CLEAN whole-project sync and nowhere
	 * else.
	 *
	 * **IDEMPOTENT AND ONE-WAY.** A second clean sync does not move the stamp: T is the earliest
	 * moment the project was known correct, and moving it forward would throw away every instant
	 * between the two syncs — the exact period the store has the most evidence for.
	 *
	 * @param CoveredPackages  every package that had a current `.gc` at that instant. Recorded in
	 *                         full — see `WasCoveredAtStamp` for why the list rather than the count
	 *                         is what makes a removal answerable.
	 * @return true when this call wrote the stamp; false when one was already there or the write
	 *         failed (`OutError` distinguishes them — it is empty for "already stamped").
	 */
	static bool StampTrackingStart(const TArray<FString>& CoveredPackages, FString& OutError);

	/**
	 * The entry holding what `PackageName` looked like at `When` — its EARLIEST SURVIVING ENTRY AT
	 * OR AFTER it. §3.2, and the direction is the part everybody gets backwards.
	 *
	 * An entry archives the state that was true BEFORE the save that replaced it, so the version an
	 * asset held at 14:00 is not the newest entry before 14:00 — that one had already been replaced
	 * — it is the earliest one written after it, whose payload is precisely what that save
	 * overwrote.
	 *
	 * **IT LIVES IN THE STORE BECAUSE THE RULE IS THE STORE'S.** It was the panel's for one
	 * milestone, and a second caller (the project-wide planner) would have made it two copies of
	 * §3.2 — the kind of pair this codebase keeps finding disagreeing.
	 *
	 * @return false when the store holds nothing at or after `When` for this package, which is the
	 *         ordinary "it has not changed since then" and not an error. Ask `KnowledgeAt` first:
	 *         false means "unchanged" only when the answer there was `Known`.
	 */
	static bool EntryAt(const FString& PackageName, const FDateTime& When, FGcHistoryEntry& Out);

	/**
	 * The earliest instant this package's history has EVER covered. Never moves forward.
	 *
	 * Written when the package's first entry is, and deliberately not touched by pruning: it is the
	 * difference between *"this asset did not exist then"* and *"it did, and we no longer hold it"*.
	 */
	static FDateTime KnownSince(const FString& PackageName);

	/** The newest entry ever deleted FOR THIS PACKAGE. Epoch when it has never been pruned. */
	static FDateTime PackageCutoff(const FString& PackageName);

	/**
	 * **THE QUESTION A RESTORE PLANNER MUST ASK BEFORE IT DECIDES ANYTHING.**
	 *
	 * `DidNotExist` when a creation tombstone puts the asset's birth after `When`; `Unknown` when the
	 * store never reached that far back, or reached it and was pruned; `Known` when every entry that
	 * covered `When` still survives, which is the only case where the state can be produced exactly.
	 *
	 * A planner that inferred existence from an empty `List` would read `Unknown` as `DidNotExist`
	 * and plan a deletion — see `EGcHistoryKnowledge`.
	 */
	static EGcHistoryKnowledge KnowledgeAt(const FString& PackageName, const FDateTime& When);

	/**
	 * The same question asked by something that only wants to LOOK, which is a different question.
	 *
	 * **THE STAMP GATE IS A RULE ABOUT DELETING, AND IT WAS BEING APPLIED TO READING.**
	 * `KnowledgeAt` answers `Unknown` for every asset before `TrackingSince()` — deliberately, and
	 * it must keep doing so for `FGcProjectRestore`, which DELETES assets it decides did not exist.
	 * Being wrong there destroys work. Being wrong here shows a diff that is slightly stale.
	 *
	 * User-reported 2026-08-05, with the numbers that make the case: this project holds archives for
	 * seven structs — `S_Hole`, `S_MaterialSlot`, `S_Triangle`, `S_ColorOpions`, `S_UWOption`,
	 * `S_BlockCut`, `S_Range` — every one of them written BEFORE the stamp at 19:06. The store was
	 * holding all seven and refusing to show any: *"the store cannot say what 'S_BlockCut' held"*
	 * about text sitting on disk. *"stop checkpoint stopping"*.
	 *
	 * **WHAT IT STILL REFUSES, because these are facts rather than policy:**
	 *  - a creation tombstone proves the asset was born later — still `DidNotExist`;
	 *  - an instant before this package's OWN trim cutoff is genuinely gone — still `Unknown`;
	 *  - a package with no entries at all cannot be spoken for — still `Unknown`.
	 *
	 * **WHAT IT NO LONGER REFUSES:** an instant before the project-wide stamp, when this package's
	 * own entries cover it. The caveat that made the stamp exist is real — before T nobody promises
	 * GraphCode was watching, so an edit may have gone unarchived — but the honest response to that
	 * is a caption, not a refusal to open the file.
	 */
	static EGcHistoryKnowledge KnowledgeForRead(const FString& PackageName, const FDateTime& When);

	/**
	 * Archive `OldText` as the predecessor of `NewText`, but ONLY if they differ logically.
	 *
	 * §2.2 — `bin:` and `source:` churn is not authored history, and a store that filled up with it
	 * would push the entries somebody actually wants off the end of a ten-deep list. Both sides must
	 * parse and both must be canonical; an unreadable, hand-edited, conflicted or unabsorbed-merge
	 * `.gc` is NOT a valid restore point and belongs to the existing proposal-preservation path
	 * instead (`Saved/GraphCode/Proposals/`).
	 *
	 * @return true when an entry was written. `OutWritten` is false with no error when the two were
	 *         logically identical, which is the common path and not a failure.
	 */
	static bool ArchivePredecessor(
		const FString& PackageName,
		const FString& OldText,
		const FString& NewText,
		EGcHistoryReason Reason,
		const FString& GroupId,
		bool& OutWritten,
		FGcHistoryEntry& OutEntry,
		FString& OutError);

	/**
	 * Archive this exact text unconditionally. The pre-revert checkpoint's route (§2.4).
	 *
	 * Deduplicated against the NEWEST entry only: a checkpoint whose body already sits at the top of
	 * the list adds nothing, and §3 forbids a revert creating a duplicate for that reason. An older
	 * identical body is left alone, because the list is a sequence and not a set.
	 */
	static bool ArchiveText(
		const FString& PackageName,
		const FString& Text,
		EGcHistoryReason Reason,
		EGcHistoryTransition Transition,
		const FString& GroupId,
		bool& OutWritten,
		FGcHistoryEntry& OutEntry,
		FString& OutError);

	/**
	 * Copy an AUTHORED document — `.gcshape`, `.gcw`, `.gcmove` — into the store as it stands on
	 * disk right now. Call it immediately BEFORE anything replaces that file.
	 *
	 * **THE SECOND STREAM, AND IT IS NOT A SIDECAR.** Everything above this line archives a `.gc`,
	 * which is a PROJECTION: written from an asset, re-derivable from it, and gated on `sig:`
	 * matching a body the projector produced. A shape program, a region file and a move program are
	 * the opposite — a person or an agent types them, nothing can re-derive one, and the only copy
	 * of yesterday's version is the bytes that were on disk before today's build wrote over them.
	 * They had no history at all until now, which is what this closes.
	 *
	 * **SO NONE OF THE CANONICAL GATES APPLY AND NONE ARE ASKED FOR.** There is no `sig:` to match,
	 * no `kind:` to read, no merge proposal to refuse: the contract here is *these exact bytes were
	 * on disk at this instant*, and a document that does not parse is MORE worth keeping than one
	 * that does, not less. Deduplication is against the newest copy of the same document by content
	 * digest, which is the whole of what "logically different" can honestly mean without a projector.
	 *
	 * **NAMING — the same store, the same per-document directory, the file's OWN extension.**
	 * `Saved/GraphCode/History/Game/Steampunk/Recipes/BevelGears/BevelGears_<stamp>_<digest>.gcshape`.
	 * One place to look for a backup, and the extension is what separates the two streams where they
	 * meet: a LEVEL has both an `L_Town.gc` and an `L_Town.gcw`, so they share a directory and must
	 * never share a lineage. Keeping the real extension also means a copy opens in whatever already
	 * reads that format — a `.gcc` rename would have made a shape program unreadable to the thing
	 * that wrote it.
	 *
	 * **INVISIBLE TO THE PANEL AND THE TIME MACHINE, DELIBERATELY (user-directed 2026-09-12:
	 * *"dont even need to track in vcs panel for now"*).** Nothing here writes `index.tsv`, and every
	 * reader above — `List`, `Find`, `ScanArchives`, `Prune` — is keyed to the index or globs `.gcc`.
	 * So these copies are inert to the asset history: they cannot appear as a version of a level,
	 * cannot be restored onto one by accident, and cannot be pruned out from under their author.
	 * `BytesUsed` counts them, because a store's size is its size.
	 *
	 * @param DocumentFilePath  the absolute or project-relative path of the file ABOUT to be
	 *                          replaced — not the package path, because a document is a file first.
	 * @return true when nothing went wrong. `OutWritten` is false with no error for the two ordinary
	 *         non-events: the file does not exist yet (a first write has no predecessor) and its
	 *         bytes already sit at the top of the list.
	 */
	static bool ArchiveAuthoredFile(
		const FString& DocumentFilePath,
		bool& OutWritten,
		FString& OutArchivePath,
		FString& OutError);

	/** The extensions `ArchiveAuthoredFile` accepts, lower-case and dotted. One place, so another
	 *  authored format is added here and is then backed up everywhere at once. */
	static const TArray<FString>& AuthoredExtensions();

	/**
	 * **THE PRE-REVERT CHECKPOINT (§2.4). Call this before ANY restore mutates ANY asset.**
	 *
	 * Projects the asset with the same canonical live projector `gc_read source:"live"` uses, and
	 * archives that. **Live, not the disk `.gc`** — copying the sidecar is the obvious thing and it is
	 * wrong, because the sidecar trails unsaved editor work, and unsaved work is exactly what a
	 * revert is about to destroy and what nothing else anywhere is holding a copy of.
	 *
	 * **IT REFUSES RATHER THAN PROCEEDING.** If the projection or the archive fails, the caller must
	 * touch nothing: a restore whose undo could not be written is a restore that trades one version
	 * of somebody's work for another with no way back. `false` here means abort the whole plan, not
	 * "carry on without a checkpoint".
	 *
	 * `GroupId` ties every checkpoint of one multi-asset restore together, so a later reader can see
	 * that eleven entries were one gesture rather than eleven.
	 *
	 * Loads nothing: the asset must already be in memory, which it is — a restore resolved it first.
	 */
	static bool CheckpointLiveState(
		const FString& PackageName,
		const FString& GroupId,
		FGcHistoryEntry& OutEntry,
		FString& OutError);

	/**
	 * Record a transition that has no predecessor text — `Created`, or a `Removed` whose recovery
	 * path is what matters rather than the body.
	 */
	static bool RecordTransition(
		const FString& PackageName,
		EGcHistoryTransition Transition,
		const FString& GroupId,
		const FString& PreviousPackageName,
		const FString& RecoveryPath,
		FGcHistoryEntry& OutEntry,
		FString& OutError);

	/** Newest first. Empty for a package with no history, which is not an error. */
	static TArray<FGcHistoryEntry> List(const FString& PackageName);

	/**
	 * Every package this store holds anything for. **The file list of a history comparison.**
	 *
	 * E11.4 needs it for the same reason `ChangedPathsBetween` exists for a branch: a comparison is a
	 * list of what its two sources disagree about, and against local history that list is *the assets
	 * this store can say anything about*. Asking it per row would be one index read per sidecar —
	 * three hundred file reads for a question one directory walk answers.
	 *
	 * Package NAMES, not paths, because the name is read out of the index rather than reconstructed
	 * from the directory: `PackageToRelativeDirectory` strips the mount's leading slash and guessing
	 * it back would have to know which mount points exist.
	 */
	static TArray<FString> PackagesWithHistory();

	/**
	 * Every archive in the store, read from its FILENAME alone. Newest first.
	 *
	 * One glob and no file is opened — see `FGcArchiveName`. This is what a date control asks, and
	 * asking it must stay cheap enough to answer while somebody is dragging one.
	 *
	 * **BOTH SPELLINGS THE STORE HAS EVER MINTED ARE READ.** An entry whose name carries NO date is
	 * skipped rather than guessed at — it is still a valid archive and still restorable through the
	 * index, and a list that invented an instant for it would be a timeline of made-up moments. An
	 * entry whose name carries a date in the older spelling is READ, because reading a date that is
	 * written down is not guessing; treating those two cases as one is what left the time machine
	 * with 2 stops out of 53 (see `TryParseLegacyEntryName`).
	 */
	static TArray<FGcArchiveName> ScanArchives();

	/**
	 * `PackagesWithHistory` and `ScanArchives` from ONE walk of the store, for a caller that needs both.
	 *
	 * **THE STORE IS 17,089 FILES NOW (2026-10-29), AND EACH OF THOSE TWO WALKED IT.** Measured on this project: a walk
	 * ~300 ms, reading the 2,513 indexes 200–370 ms. A panel refresh against the timeline asked for the packages twice
	 * and the archives once — three walks and two index passes, ~2 s, for one answer.
	 */
	static void ScanStore(TArray<FString>& OutPackages, TArray<FGcArchiveName>& OutArchives);

	/**
	 * Split `BP_Player_2026_08_08_20_12_34_007_a1b2c3d4` back into its asset name and its instant.
	 *
	 * **PARSED FROM THE RIGHT, because an asset name may contain underscores** — most in this
	 * project do. The tail is fixed-width (seven numeric fields plus a digest, with an optional
	 * `-N` collision run after it) and the head is whatever remains, which is the only split that
	 * cannot be fooled by the name itself.
	 */
	static bool TryParseEntryName(
		const FString& Basename, FString& OutAssetName, FDateTime& OutTimeUtc);

	/**
	 * The instant out of the OLDER name this store minted — `20260804-194952-996-3c9c44ef`.
	 *
	 * `<8 date><6 time><3 millis><digest>`, hyphen-separated, checked by field width rather than by
	 * position alone: a digest is eight characters and can be all-numeric, so the lengths are what
	 * tell the fields apart.
	 *
	 * No asset name comes back, because there is none in there — §2.3 already says the basename is
	 * not identity, and the directory is what resolves an archive to an asset.
	 */
	static bool TryParseLegacyEntryName(const FString& Basename, FDateTime& OutTimeUtc);

	/** The archived body. False for a tombstone, which has none by definition. */
	static bool Read(const FGcHistoryEntry& Entry, FString& OutText, FString& OutError);

	/** Find one entry by its immutable id, so a UI can hold an id rather than an index. */
	static bool Find(const FString& PackageName, const FString& EntryId, FGcHistoryEntry& OutEntry);

	/**
	 * Move a lineage to follow a renamed asset, and record the rename (§2.3).
	 *
	 * The old directory is MOVED rather than copied, so a lineage is never in two places and a
	 * `List` on the new name is the whole story.
	 */
	static bool NoteRename(const FString& OldPackageName, const FString& NewPackageName,
		FString& OutError);

	/**
	 * Trim the WHOLE STORE to `BudgetBytes`, oldest first.
	 *
	 * **NEVER AUTOMATIC. NOTHING CALLS THIS ON A SAVE, AND NOTHING MAY** — user-directed 2026-08-28:
	 * *"don't even do auto pruning, since people lose their work without notice"*. Until there is a
	 * gesture that asks for it, the only callers are specs. `BudgetBytes` is what the startup check
	 * warns above, not a figure anything enforces behind the user's back.
	 *
	 * **GLOBAL, AND THAT IS THE POINT.** Pruning per asset could not honour a project-wide figure —
	 * every asset would be individually within its share and the total would be whatever the number
	 * of edited assets made it. So the sweep sorts every payload entry in the store by age and drops
	 * from the oldest end until the total fits, which is exactly "the cutoff moves closer to now".
	 *
	 * **TWO THINGS IT WILL NOT TAKE, AND ONE FLOOR IT CANNOT GO UNDER:**
	 *
	 * - A **tombstone**, which is the only record that an asset once did not exist and costs one
	 *   index line and no file. A budget in megabytes has no reason to want it.
	 * - **The newest payload entry of each package.** Losing depth on one asset is a smaller harm
	 *   than losing an asset's last recoverable point entirely, and a global sweep without this
	 *   floor would happily spend a big asset's only version on a small asset's tenth.
	 * - Consequently the store can sit ABOVE budget when the floor alone exceeds it. That is
	 *   reported rather than enforced: deleting somebody's last copy to hit a number is the one
	 *   thing a recovery store may not do.
	 *
	 * A **pre-revert checkpoint** is prunable here, and that supersedes E11.1's first rule that it
	 * never was. That rule made checkpoints immortal, so a hundred reverts was a hundred permanent
	 * entries and the budget was unbounded from the one direction nothing else covered. It is still
	 * protected by the per-package floor above, so the most recent thing you could undo to survives.
	 *
	 * @return how many entries were removed.
	 */
	static int32 Prune(FString& OutError);

	/** Is this resolved path inside the history root? The sandbox, and it is checked on every write. */
	static bool WithinRoot(const FString& AbsolutePath);
};
