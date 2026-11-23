// Copyright OraStudio. All Rights Reserved.
//
// WHAT ONE WRITE CAN SEE ON A MAP WHOSE ACTORS ARE NOT IN MEMORY — World Partition row 8. The
// account is `GC-WORLD-PARTITION-DONE.md` §7a; the survey it corrects is
// `GC-WORLD-PARTITION-AUDIT.md` §2.3. **Every row on this subject is closed as of 2026-08-22**;
// `GC-WORLD-PARTITION-DESIGN.md` §7 carries the account of the last three.
//
// **THE DEFECT THIS EXISTS TO STOP, AND IT IS THE SILENT-DUPLICATE ONE.** Every realiser under a
// `.gcw` write asks the world what it already holds, and every one of them asks with
// `TActorIterator` — eleven sites across `GraphCodeWorldGeneratedVolume`, `…RegionNavigation`,
// `…Water`, `…Terrain` and `…FoliagePlant`, **plus a twelfth the audit's count missed**:
// `FGraphCodeRegionPromote` builds its `ByName` index the same way, in Core rather than World, and
// runs inside `ResolveRegionPayload` — BEFORE the realisers, which is what decided where a caller
// takes this scope. `TActorIterator` walks the actors in MEMORY. On a partitioned map the editor
// holds only the regions somebody loaded and only the data layers somebody left loaded, so:
//
// - a name that is taken out in an unloaded cell reads as FREE, and the realise spawns a SECOND
//   actor beside the first — nothing refuses, because nothing can see the first;
// - the sweep destroys what it cannot see, which is nothing, so deleting a line leaves a generated
//   volume standing out where nobody looked ([[a-gate-on-the-desired-state-cannot-see-a-removal]]);
// - a `from:` promotion resolves an author's `$handle` against a name index missing the actor it
//   names, so the one step that turns an intention into geometry reads a half-empty map.
//
// **IT IS NOT A HYPOTHESIS. MEASURED 2026-08-22 on this project's own `RuntimeFixture_WP`: a plain
// editor load leaves 4 of its 14 actors out of the walk** — 29% of the map invisible to every site
// above. `GraphCode.WorldPartition.LoadTheWalkSeesWhatThePinHolds` prints both counts on every run.
//
// **THE AUDIT POSED A FORK THAT THIS PROJECT HAD ALREADY ANSWERED ONE PLUGIN OVER.** §2.3 offered
// *refuse a partitioned apply unless the regions are loaded* or *answer identity from descriptors
// and load on demand*, and recommended the refusal as the cheaper first step. Neither was needed:
// `GraphCodeSliceApply` has held every actor loaded across a whole write since 2026-08-13, nine
// days before the row was picked up, for the same reason and with the same three lines. So this is
// that rule with its second caller, moved out of the call site before it could be copied a third
// time ([[two-carriers-of-one-fact-is-a-race]]).
//
// **THE PARAGRAPH BELOW IS SUPERSEDED — USER-RULED 2026-08-22, ROW 8d, and it is left standing
// because it is the reasoning that has to be walked back rather than quietly deleted.** The ruling:
// *"if possible, find the files using reflection based on the level itself, to find the actors, if
// really cannot, refer to gc."* Its objection — that a filter would have to carry the realisers'
// class vocabulary and go stale — **only holds for a filter written inside this pin.**
// `FActorDescContainerInstanceCollection::TIterator<AWaterZone> It(Partition)` takes the class AT
// THE CALL SITE, which is exactly where `TActorIterator<T>` already keeps it, so no list exists
// anywhere to go stale. What the ruling does NOT remove is this scope: a descriptor answers identity
// and cannot be written to, so the shape is roster from the record, load on demand for what is
// actually touched. **And the trap that decides how 8d is built: a descriptor is created in
// `UActorDescContainer::OnObjectPreSave`, so an actor spawned earlier in the same write has none** —
// the roster is descriptors UNION the loaded actors ([[a-descriptor-appears-at-save-not-at-spawn]]).
// See `GC-WORLD-PARTITION-DESIGN.md` §6.3.
//
// **WHY NOT THE NARROWER FILTER, WHICH IS MEASURABLY POSSIBLE.** `FWorldPartitionActorDesc` carries
// `GetActorName()`, `GetActorNativeClass()` AND `GetTags()` — `Tags = InActor->Tags` verbatim,
// serialised since `WorldPartitionActorDescTagsSerialization` — so a descriptor walk really can
// answer *is this one of ours* without loading, because ownership here IS a tag and `GraphCode.` is
// already the load-bearing prefix (`GcwPopulateTags::Match`). The reason to pin everything anyway is
// that a filter would have to ALSO name the classes the non-tagged realisers look up — `AWaterZone`,
// `AWaterBody`, `AWaterBodyIsland`, `AWaterBodyExclusionVolume`, `ALandscape`,
// `AInstancedFoliageActor` — and that list is a second carrier of the realisers' own vocabulary. It
// goes stale on the next verb somebody adds, silently and in the destructive direction
// ([[a-new-line-kind-must-reach-the-assembler]]). One blunt rule that cannot drift beats a precise
// one that can.
//
// **THE COST IS REAL AND IS THE REASON THE APPLY REPORTS IT.** On a large open world this loads
// every actor in the map for the duration of one write. That is an editor-time authoring cost paid
// by the operation that would otherwise corrupt the map, and it is stated in the result rather than
// spent in silence ([[a-deferred-act-owes-an-outcome]]).
//
// **AND THE `.gcw` WRITE DOOR NO LONGER TAKES IT — ROW 8d LANDED 2026-08-22.** `ApplyRegionFile`
// declares a `FGraphCodeScopedActorRoster` instead: all twelve enumeration sites under it now read
// the level's own record through `FGraphCodeActorLookup` and load only the actors they touch. What
// still takes this scope, and why, so the next reader does not assume it is dead code:
//
//   | caller | why it still loads the map |
//   |---|---|
//   | `.gcs` apply (`GraphCodeSliceApply`) | a slice NAMES its actors, so the same narrowing is available here and simply has not been taken. Not row 8d's twelve; a follow-up |
//
// **`gcw_ask` CAME OFF IT TOO — row 8c, 2026-08-22, and the table above used to list it.** Its
// `FillContents` was recorded here as the shape a record cannot narrow: every actor's ORIGIN tested
// against a prism. That was wrong about the record, not about the question —
// `FWorldPartitionActorDesc::ActorTransform` is *current actor-to-world transform, considering
// parent transform*, so `FGraphCodeActorRosterEntry::Transform` answers it exactly, for the whole
// map, opening nothing. §7 8c had designed a bounds PREFILTER with a stated loss direction; there is
// no prefilter and no loss.
//
// **THE RULE THAT FELL OUT OF ROW 8d SURVIVES, NARROWED BY THAT.** *A question about a NAMED thing
// is answerable from the record; a question about every actor's PROPERTIES is not* — and the record
// carries more properties than one assumes, so the second half is worth CHECKING per property rather
// than concluded. What genuinely needs the actor is what no descriptor stores: `MeshShape` in
// `GraphCodeWorldRegionNavigation` digests every actor's navigation-relevant COMPONENTS — bounds,
// collision profile, body setup — so it loads at the moment it needs to, rather than every write
// paying up front.
//
// **AND THE REFLECT NO LONGER NEEDS EITHER — row 8b, 2026-08-22.** The paragraph that stood here
// said §13.6's reflect runs inside `FCoreUObjectDelegates::OnObjectPreSave` and therefore may not
// load. **Measured and false**: `GraphCode.WorldPartition.WriteAPreSaveHookCanLoadAnUnloadedActor`
// loads an actor out of an unopened cell from inside that hook and the save then completes. Both
// reflects now read the whole level through a lookup ([[a-stated-constraint-is-a-claim-to-measure]]).

#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"
#include "Templates/UniquePtr.h"
#include "UObject/SoftObjectPath.h"
#include "WorldPartition/WorldPartitionHandle.h"

class AActor;
class UClass;
class UWorld;

/**
 * Every actor of a partitioned world, loaded for as long as this object lives.
 *
 * Construct it once at the top of a write and let it die at the end of that write, AFTER the save:
 * releasing the references unloads what they loaded, and an actor unloaded before its package is
 * committed is a write that reports a success it did not perform.
 *
 * **IT IS A NO-OP ON AN ORDINARY MAP**, which is what lets a caller take it unconditionally instead
 * of growing a partitioned/not branch at every door. `GetWorldPartition()` answers null there and
 * `Count()` stays zero.
 *
 * **IT PINS THROUGH AN UNLOADED DATA LAYER TOO, and that half is measured rather than assumed.**
 * `FWorldPartitionReferenceImpl::IncRefCount` calls `LoadAndRegisterActor` on the first hard
 * reference with no consultation of the data layer's editor state, so a layer somebody left unloaded
 * does not hide its actors from this. That is what closes row 7's stated limit — the foliage sweep
 * walking loaded foliage actors while `layers=` could file instances into an unloaded one.
 *
 * **A REFERENCE HELD ACROSS A DESTROY IS FINE AND THE ENGINE SAYS SO.** The sweep destroys actors
 * this object pinned; `FWorldPartitionReferenceImpl::DecRefCount` skips its unload when the
 * descriptor is already unregistering, which is exactly the state a deleted actor leaves behind.
 */
class GRAPHCODE_API FGraphCodeScopedPartitionLoad
{
public:
	/** Pins nothing. Useful where a caller must declare the scope before it knows the world. */
	FGraphCodeScopedPartitionLoad() = default;

	/** Pins every actor of `World`, or nothing at all when `World` is null or unpartitioned. */
	explicit FGraphCodeScopedPartitionLoad(UWorld* World);

	FGraphCodeScopedPartitionLoad(const FGraphCodeScopedPartitionLoad&) = delete;
	FGraphCodeScopedPartitionLoad& operator=(const FGraphCodeScopedPartitionLoad&) = delete;

	~FGraphCodeScopedPartitionLoad();

	/**
	 * Pin `World` now. Safe to call on an already-pinned scope: the previous pin is released first,
	 * so a caller that learns its world late cannot leak the earlier one.
	 */
	void Pin(UWorld* World);

	/** Release every reference this scope holds, unloading whatever only it was keeping in memory. */
	void Release();

	/**
	 * True when this scope is holding a partitioned world — NOT when it loaded something.
	 *
	 * The two answers differ on a partitioned map with no actor descriptors yet, and a caller
	 * reporting *this is a World Partition map* must not say *no* there.
	 */
	bool IsPartitioned() const { return bPartitioned; }

	/** How many actors this scope pinned. Zero on an ordinary map, and on an empty partitioned one. */
	int32 Count() const { return References.Num(); }

private:
	TArray<FWorldPartitionReference> References;
	bool bPartitioned = false;
};

// ---------------------------------------------------------------------------------------------
// **THE ROSTER — WHAT THE LEVEL SAYS IT HOLDS, WHICH IS NOT WHAT THE WALK SEES.** Row 8d,
// user-ruled 2026-08-22: *"if possible, find the files using reflection based on the level itself,
// to find the actors, if really cannot, refer to gc."* Record: `GC-WORLD-PARTITION-DESIGN.md` §6.3.
//
// The scope above answers *what is here* by LOADING the map. This answers it by ASKING the map, and
// loads only what a caller then says it needs. They are not rivals: identity comes from here, and a
// caller about to READ OR WRITE an actor's properties still needs it in memory, which is `Load()`.
//
// **NEITHER SOURCE IS THE ROSTER ON ITS OWN, and that is measured rather than assumed.** A
// descriptor is created in `UActorDescContainer::OnObjectPreSave` — there is no spawn-time hook:
//
//   | | in memory | in the level's record |
//   |---|---|---|
//   | an actor in an unloaded cell | no | YES |
//   | one spawned this write, unsaved | YES | no |
//   | one destroyed this write, unsaved | no | YES, still |
//
// So the roster is the UNION, deduped by actor GUID. Built from the record alone it would
// reintroduce row 8's duplicate ONE LINE LATER IN THE SAME WRITE — realise `SwampFog`, ask whether
// the name is taken, be told no — and that needs no unloaded cell to reproduce, which makes it
// worse than the defect row 8 closed ([[a-descriptor-appears-at-save-not-at-spawn]]).
//
// **THE CLASS FILTER IS THE CALLER'S WORD AND IS NEVER A LIST HELD HERE**, which is what answers
// row 8's objection: a filter that would go stale on the next verb is one written INSIDE a pin, and
// this one is passed at the call site, exactly where `TActorIterator<T>` already keeps it.
// ---------------------------------------------------------------------------------------------

/** One actor as the level records it, plus the actor itself when it happens to be in memory. */
struct GRAPHCODE_API FGraphCodeActorRosterEntry
{
	/** `GetFName()` loaded, `GetActorName()` from the record. The name a document would use. */
	FName Name;

	/** The editor label where there is one — `FindLandscape` keys on this, so it is not decoration. */
	FName Label;

	/** Nearest NATIVE class. Never the Blueprint one — see `Refusal()` for what that costs. */
	UClass* NativeClass = nullptr;

	/** `AActor::Tags` verbatim; the descriptor serialises them, so ownership is free to test. */
	TArray<FName> Tags;

	/** What the union dedupes on. Every actor has one and a descriptor is keyed by it. */
	FGuid Guid;

	FSoftObjectPath Path;

	/**
	 * Where the actor IS — actor-to-world, parent transform folded in.
	 *
	 * **THE RECORD CARRIES THIS, which is what turned row 8c from a trade-off into an answer.**
	 * `FWorldPartitionActorDesc::ActorTransform` is *current actor-to-world transform, considering
	 * parent transform*, and `FWorldPartitionActorDescInstance::GetActorTransform()` hands it over
	 * without loading anything. So *is this actor's ORIGIN inside that region* — §14's rule, the
	 * question `gcw_ask` exists to answer — is answerable for an actor nobody has opened, exactly,
	 * with no prefilter and nothing to lose.
	 *
	 * Identity for an entry that has neither: nothing in this system reads it without checking.
	 */
	FTransform Transform = FTransform::Identity;

	/** Valid when the actor is already in memory; null for one that lives only in the record. */
	TWeakObjectPtr<AActor> InMemory;

	/** True when the record supplied this entry. False for a spawn that has not been saved yet. */
	bool bFromRecord = false;

	bool IsLoaded() const { return InMemory.IsValid(); }
};

class GRAPHCODE_API FGraphCodeActorRoster
{
public:
	/**
	 * Read the roster. **LOADS NOTHING.**
	 *
	 * `OfClass` null means every actor. On an ordinary map this is the `TActorIterator` walk and
	 * every entry arrives loaded, which is what lets a caller take it without a partitioned/not
	 * branch.
	 */
	explicit FGraphCodeActorRoster(UWorld* World, const UClass* OfClass = nullptr);

	~FGraphCodeActorRoster();

	FGraphCodeActorRoster(const FGraphCodeActorRoster&) = delete;
	FGraphCodeActorRoster& operator=(const FGraphCodeActorRoster&) = delete;

	const TArray<FGraphCodeActorRosterEntry>& Entries() const { return Roster; }

	/**
	 * Re-read the LOADED half. The record half and everything `Load()` pinned stay.
	 *
	 * **A ROSTER HELD ACROSS A WRITE IS STALE THE MOMENT A REALISER SPAWNS ANYTHING**, and staleness
	 * here is row 8's defect wearing a new coat: ask *is this name taken* against a snapshot taken
	 * before the volume above it was realised and the answer is no, so a second one is spawned beside
	 * it. `TActorIterator` never had this problem because it re-walked memory every time it was
	 * asked — so this is what a site pays to keep that property, and it is a memory walk, not a load.
	 *
	 * **AN ACTOR THIS ROSTER WATCHED DIE NEVER COMES BACK**, which the record half alone would get
	 * wrong: a descriptor outlives its actor until the save, so the sweep destroying a volume and the
	 * realise then asking for its name would be told the name is still claimed — by a thing that is
	 * gone ([[a-gate-on-the-desired-state-cannot-see-a-removal]]).
	 */
	void Refresh();

	/**
	 * By the name a document would use. Unset when nothing claims it — row 8's wrong answer.
	 *
	 * **BY VALUE, AND THAT IS A LIFETIME RULE RATHER THAN A STYLE ONE.** `Refresh()` rebuilds the
	 * array, so a pointer into it is only good until the next question anybody asks — and the sites
	 * this exists for ask nested questions (`Realize` finds an actor, then resolves what it points
	 * at). A copy cannot dangle, and `Load()` needs only the GUID inside it.
	 */
	TOptional<FGraphCodeActorRosterEntry> Find(FName Name) const;

	/**
	 * Bring one entry into memory and hold it for this roster's lifetime.
	 *
	 * An already-loaded entry costs nothing. A caller that DESTROYS what it loaded is fine:
	 * `FWorldPartitionReferenceImpl::DecRefCount` skips its unload while the descriptor is already
	 * unregistering, which is the state a deleted actor leaves behind.
	 */
	AActor* Load(const FGraphCodeActorRosterEntry& Entry);

	/** True for a World Partition map, whether or not this roster loaded anything. */
	bool IsPartitioned() const { return bPartitioned; }

	/** How many entries `Load()` actually brought in — the number a result reports. */
	int32 LoadedByThisRoster() const { return Pinned.Num(); }

	/**
	 * Empty when the roster is whole; otherwise why it is not.
	 *
	 * **A CALLER THAT IGNORES THIS IS BACK TO THE SILENT SHORT ANSWER**, which is the defect this
	 * row exists to close. Today it has one cause: a Blueprint-generated `OfClass`. A descriptor
	 * carries `GetActorNativeClass()` and nothing else, so the record cannot say whether an unloaded
	 * actor is a `BP_Fog`, and a roster that quietly omitted the unloaded ones would read as
	 * *there is no fog here*.
	 */
	const FString& Refusal() const { return Unanswerable; }

private:
	/** The descriptor half, read once: it changes only at save, and re-reading it costs the map. */
	void CaptureRecord();

	/** The loaded half, re-walked, unioned onto the record half. `Refresh()` and the constructor. */
	void Rebuild();

	TArray<FGraphCodeActorRosterEntry> Roster;
	TArray<FGraphCodeActorRosterEntry> Record;
	TArray<FWorldPartitionReference> Pinned;

	/**
	 * Everything this roster has ever seen in memory, so a `Refresh()` can tell *destroyed* from
	 * *never loaded* — the two look identical in a snapshot and mean opposite things to a caller.
	 */
	TMap<FGuid, TWeakObjectPtr<AActor>> Watched;
	TSet<FGuid> Destroyed;

	TWeakObjectPtr<UWorld> WeakWorld;
	const UClass* Filter = nullptr;
	FString Unanswerable;
	bool bPartitioned = false;
};

/**
 * THE ROSTER ONE WRITE IS USING, declared once at the top of that write.
 *
 * **THIS IS WHAT THE PIN BECAME.** Row 8 held every actor of a partitioned map in memory for the
 * duration of a write, because the twelve enumeration sites underneath it each asked the world with
 * `TActorIterator` and would otherwise have read a half-empty map. Row 8d replaced the ANSWER — the
 * roster reads the level's own record and loads nothing — but a roster built inside a site would
 * release its loads at the end of that site, and an actor unloaded before the save that commits it
 * is a write reporting a success it did not perform. So the object lives here, at the write, and
 * `FGraphCodeActorLookup` below is what a site takes.
 *
 * **NOT A PARAMETER, AND THAT IS THE COST OF THIS SHAPE.** Threading a roster through every
 * realiser signature would say the same thing in the type system; this says it in a scope guard,
 * exactly as the pin it replaces did, and a site that runs outside a write still answers correctly
 * because the lookup falls back to a private roster. What it buys is that a site cannot be made
 * wrong by a caller that forgot a parameter — it can only be made SLOWER by a caller that forgot
 * the scope ([[a-silent-no-op-is-worse-than-a-refusal]] is why the failure mode has to be that way
 * round).
 */
class GRAPHCODE_API FGraphCodeScopedActorRoster
{
public:
	explicit FGraphCodeScopedActorRoster(UWorld* World);
	~FGraphCodeScopedActorRoster();

	FGraphCodeScopedActorRoster(const FGraphCodeScopedActorRoster&) = delete;
	FGraphCodeScopedActorRoster& operator=(const FGraphCodeScopedActorRoster&) = delete;

	FGraphCodeActorRoster& Get() { return Held; }

	/** The innermost live scope for `World`, or null when nothing declared one. */
	static FGraphCodeActorRoster* Active(const UWorld* World);

private:
	FGraphCodeActorRoster Held;
	FGraphCodeScopedActorRoster* Outer = nullptr;
	TWeakObjectPtr<UWorld> WeakWorld;
};

/**
 * WHAT A SITE ASKS. One question, one class, answered against the write's roster where there is one.
 *
 * It replaces `for (TActorIterator<T> It(World); It; ++It)` and answers the same question over the
 * whole map instead of over the loaded part of it. Two things it does that the iterator did not:
 *
 * - **it can be SHORT, and it says so.** `Refusal()` is non-empty when the record could not answer
 *   the filter — today a Blueprint class, which a descriptor cannot test. A caller that ignores it
 *   is back to reading *there are none of these here* off a half-empty map, which is the defect the
 *   row exists to close;
 * - **an entry may not be in memory.** `Load()` brings one in and it stays for the WRITE, not for
 *   this lookup, because the write's roster is what holds it.
 */
class GRAPHCODE_API FGraphCodeActorLookup
{
public:
	explicit FGraphCodeActorLookup(UWorld* World, const UClass* OfClass = nullptr);

	FGraphCodeActorLookup(const FGraphCodeActorLookup&) = delete;
	FGraphCodeActorLookup& operator=(const FGraphCodeActorLookup&) = delete;

	/** A snapshot, by value — see `FGraphCodeActorRoster::Find` for why nothing here is a pointer. */
	const TArray<FGraphCodeActorRosterEntry>& Entries() const { return Filtered; }

	/**
	 * Take the snapshot again. **A LOOP THAT SPAWNS MUST CALL THIS**, and that is the whole reason
	 * this object is separable from the question it answers: one lookup lives for the loop so what
	 * it loads stays loaded, and one refresh per turn keeps it seeing what the turn before it made.
	 * `TActorIterator` gave that away for free by being re-walked; a snapshot has to be asked.
	 */
	void Refresh();

	/** By the name a document would use. */
	TOptional<FGraphCodeActorRosterEntry> Find(FName Name) const;

	/**
	 * In memory, loading it if the record is where this entry came from. Null when it cannot be.
	 *
	 * **WHAT IT LOADS STAYS LOADED FOR AS LONG AS WHOEVER OWNS THE ROSTER**, which is the write when
	 * a `FGraphCodeScopedActorRoster` is live and THIS OBJECT otherwise. So a caller that hands the
	 * actor to something outliving this lookup must be under the write's scope — outside one, the
	 * actor is guaranteed only for as long as the lookup itself.
	 */
	AActor* Load(const FGraphCodeActorRosterEntry& Entry);

	/** Empty when this answer covers the whole map; otherwise why it does not. */
	const FString& Refusal() const;

	bool IsPartitioned() const;

private:
	void Apply();

	TUniquePtr<FGraphCodeActorRoster> Owned;
	FGraphCodeActorRoster* Using = nullptr;
	const UClass* OfClass = nullptr;
	TArray<FGraphCodeActorRosterEntry> Filtered;
	FString Unanswerable;
};

/**
 * The document-name claim counts for a level, taken over the LEVEL'S RECORD where there is one.
 *
 * **THIS EXISTS BECAUSE THE CHOP IS DECIDED BY A COUNT, AND THE COUNT WAS TAKEN OVER WHAT IS
 * LOADED** — row 8d's thirteenth site, which row 8's survey of eleven and its own twelfth both
 * missed. `FGraphCodeActorEntryWriter::ActorDocumentNameCounts(Level)` walks `Level->Actors`, so on
 * a partitioned map a base name with a second claimant in an unloaded cell counts as having ONE,
 * and `FixtureSun_UAID_…` shortens to `FixtureSun` — a handle two actors answer to. The chop is
 * right; the arithmetic under it was short.
 *
 * **IT NARROWS TO THE PARTITIONED PERSISTENT LEVEL AND NOTHING ELSE.** An ordinary map, and every
 * sublevel of one, is answered exactly as before, over that level's own actors — because the roster
 * is a WORLD's record and swapping it in everywhere would quietly change which actors count as
 * claimants on a multi-level map, which is a different behaviour wearing this fix's clothes.
 */
GRAPHCODE_API TMap<FString, int32> GraphCodeDocumentNameCounts(const ULevel* Level);
