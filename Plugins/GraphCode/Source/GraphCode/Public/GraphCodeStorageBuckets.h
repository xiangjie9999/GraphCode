#pragma once

#include "CoreMinimal.h"

/**
 * Which sweep policy an item under `Saved/GraphCode` is subject to.
 * `GC-TOOL-SURFACE-DESIGN.md` §14.1 has the table and the argument for four rather than three.
 */
enum class EGcStorageBucket : uint8
{
	/** Deleted at write time or next editor start. No report, no prompt, nothing to ask. */
	Transient,
	/** Age + size cap, oldest first, automatic. Regenerable — a render can be retaken. */
	Cache,
	/** Age + count cap, automatic, WITH A FLOOR: never below N per package, never the last version. */
	History,
	/** NEVER automatic. Reported; removed on explicit intent only. */
	Recovery,
};

/**
 * C24.10's second half. The first emptied `renders/.transient`; this is what lets anything else be
 * swept safely, and what §12's storage report reads.
 *
 * THE BUCKET IS STAMPED BY THE PRODUCER, NEVER INFERRED FROM THE FOLDER — §14.1, and `Proposals` is
 * the proof: 663 of its 1,378 files are spec fixtures and a handful name real project assets, one
 * directory holding two buckets with no path pattern or date range between them. The writer always
 * knows which it made; a sweeper has to guess, and the guess is where the loss happens.
 *
 * THE STAMP SITS BESIDE WHAT IT DESCRIBES, one file, rather than in a shared ledger — and that is
 * `GraphCodeShadow.h`'s ruling reused rather than re-derived. `GcMerge` is a separate process git
 * launches, possibly while an editor is open and touching the same stores, and two processes
 * appending to one file interleave with the damage landing mid-line, in the record whose only job
 * is to be trustworthy later. A stamp beside its item has no shared writer, and deleting a capture
 * directory takes its stamp with it, so the record cannot outlive its subject.
 *
 * A BUCKET SUBFOLDER WOULD HAVE BEEN CHEAPER AND IS NOT AVAILABLE. Putting the token in the path
 * (`History/history/…`) needs no second file and cannot go stale — but `History/`, `Shadow/` and
 * `RecoveryTrash/` all have readers, including the VCS panel, and moving them is a migration this
 * row does not own. The stamp is additive for that reason.
 *
 * UNSTAMPED IS `unknown`, NOT A GUESS. `BucketFor` returns nothing rather than reading the folder
 * NAME — which is exactly what the sweeper is being denied. A report says how many items it could
 * not name, and that count falling is what tells you the producers are wired; a report that quietly
 * bucketed them by folder would look finished on day one.
 *
 * **WHICH IS NOT THE SAME AS "EVERYTHING OLDER THAN THIS ROW IS UNKNOWN", and the first run of the
 * spec next door corrected that claim.** Declaring a store labels everything already inside it, so
 * the captures and versions on disk today ARE named the moment `DeclareKnownStores` runs — that is
 * the point of labelling the store rather than the item. What stays unknown is anything in a store
 * GraphCode has NOT declared: `Proposals`, deliberately, because it is mixed and only a per-item
 * stamp can speak for it; and `PreStampBackup-2026-08-03`, which nothing in this build produces.
 * So the honest reading of an unknown count is *"nobody has claimed this"*, not *"this is old"*.
 */
class GRAPHCODE_API FGcStorageBuckets
{
public:
	/** `transient` / `cache` / `history` / `recovery` — the spelling on disk and in a report. */
	static const TCHAR* Token(EGcStorageBucket Bucket);
	static TOptional<EGcStorageBucket> FromToken(const FString& InToken);

	/**
	 * Record what this item is, right after writing it.
	 *
	 * `ItemPath` may be a file or a directory: the stamp lands INSIDE a directory and BESIDE a
	 * file, so a whole-directory delete is still self-cleaning. Returns false if the stamp could
	 * not be written, which a producer may log but must never fail its own write over — the item
	 * is already on disk and an unstamped item is merely unknown.
	 */
	static bool Stamp(const FString& ItemPath, EGcStorageBucket Bucket);

	/**
	 * §14.1's table, as code: each store root labelled with the one bucket it holds.
	 *
	 * MOST STORES ARE NOT MIXED, and pretending otherwise would mean a stamp beside all 2,812
	 * render captures to say the same word 2,812 times. `Proposals` is the ONLY store measured to
	 * hold two buckets, so it is the only one that stamps per item; everywhere else the producer's
	 * statement is "this whole store is cache" and it is made once, here.
	 *
	 * Labels only what already exists — declaring a store must never be what creates it, or the
	 * first call would conjure empty directories for stores this project has never written to.
	 * Idempotent, so module startup can call it every time.
	 */
	static void DeclareKnownStores();

	/**
	 * What the producer said, or nothing at all. Never infers, and never writes.
	 *
	 * Looks for the item's own stamp first and then for the nearest ancestor carrying a store
	 * stamp, stopping at `Saved/GraphCode`. Walking up is not the folder-inference §14.1 forbids —
	 * a store stamp is a producer's statement too. What is forbidden is reading the folder's NAME.
	 */
	static TOptional<EGcStorageBucket> BucketFor(const FString& ItemPath);

	/** Where `Stamp` puts it — exposed so a cleanup pass cannot guess it wrong. */
	static FString StampPathFor(const FString& ItemPath);

	/** `Saved/GraphCode`, the ceiling `BucketFor`'s walk stops at. */
	static FString StoresRoot();

	/** True for the stamp files themselves, so a walk can count items without counting stamps. */
	static bool IsStampFile(const FString& Path);
};
