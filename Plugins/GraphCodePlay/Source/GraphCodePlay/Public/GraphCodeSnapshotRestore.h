// Putting a captured world back — `PLAY-SESSION-DESIGN.md` §6, and the FIRST thing GC Play builds
// after the reader, ahead of the whole session layer.
//
// The order was ruled by the user on 2026-08-14 (`RUNTIME-DRIVE-DESIGN.md` §17's head): this is the
// only box in Play that can turn out to be impossible, everything else is a node appending a line to
// a file, and slices, branching and `<slice>.start.gcp` are all premised on it working.
//
// ---------------------------------------------------------------------------
// WHAT THIS IS NOT: a second implementation of the level writer
// ---------------------------------------------------------------------------
//
// `GraphCodeLevelWriter` already reconciles actors from a `.gc` — spawn by presence, properties
// through `ImportDeviations`, `@placement` through `DenormalisePlacementNames`. It lives in the
// EDITOR module and walks a different document, so its code cannot be called from here; what would
// be fatal is duplicating its RULES. So the rules are taken from the same place it takes them:
//
//   the property set     `FGraphCodeActorEntryWriter::PlacedActorPolicy` / `ComponentPolicy`
//   what "default" means  `FGraphCodeActorEntryWriter::BaselineFor`
//   what a value means    `AuthoringCore::ImportDeviations`, the codec's own inverse of the export
//   placement's names     `FGraphCodeActorEntryWriter::DenormalisePlacementNames`
//
// A restore that resolved any of those for itself would drift from the capture that produced the
// file, silently, with nothing erroring — the failure `GraphCodeWorldSelector` was extracted to
// prevent, one layer up.
//
// ---------------------------------------------------------------------------
// `baseline:` IS THE IMPORT MODE — §6.2's three-level contract, already implemented
// ---------------------------------------------------------------------------
//
// §6.2 draws a table of what an absent line means and warns it is "the row that will be got wrong".
// It maps exactly onto a switch the codec has had all along:
//
//   `baseline: archetype`  absence means AT ITS DEFAULT      -> snapshot: reset every managed field
//                                                               to `BaselineFor`, then import
//   `baseline: none`       absence means DROPPED, unknown    -> patch: reset nothing
//   `baseline: previous`   absence means UNCHANGED           -> REFUSED. A delta is evidence and
//                                                               never loadable (§4.5), and it is
//                                                               syntactically un-applyable anyway
//                                                               because `gone` is not `delete`.
//
// The middle row is why this is not merely a nicety: restoring a deviation-only capture WITHOUT the
// reset leaves anything that has drifted since the capture still drifted, and the re-capture gate
// would not catch it, because a diff compares what was written.
//
// ---------------------------------------------------------------------------
// T1, AND WHAT IT COSTS — §7.3
// ---------------------------------------------------------------------------
//
// Spawn, then inject. Not T2 (defer, inject, dispatch) and not T3 (unhook `ReceiveBeginPlay`),
// because the user's own ruling is that T1 is the starting point and because T1 DELETES the
// double-apply problem outright: the whole BeginPlay-stomping question was an artefact of applying
// values *before* BeginPlay, which only the deferred plan does. Values written last win.
//
// What T1 does not fix, stated rather than discovered: BeginPlay ran against defaults and a partial
// world, so anything it COMPUTED is stale — a bar sized from default MaxHealth, a partner pointer
// cached before the partner spawned. **A T1 restore can therefore re-capture clean and still be
// wrong**, because the derived thing usually is not in the file.
//
// `BeginPlayClasses` is §7.4's detector and it is what makes that honest: it asks each class in the
// restore set whether it has its OWN `ReceiveBeginPlay` before anything runs, so a restore whose
// list is empty is PROVABLY in the safe case. Escalate to T2/T3 per class, when a class on that list
// causes a problem someone can point at — never pre-emptively.

#pragma once

#include "CoreMinimal.h"

struct FGcpDocument;
class AActor;
class UWorld;

/** What to put back, and how much of the world to consider. */
struct FGcpRestoreOptions
{
	/**
	 * The scope. EMPTY means the document's own `select:`, which is the ordinary call.
	 *
	 * §4.1, user-ruled: the filter is in the file because it is the injection's WRITE SCOPE. A
	 * caller passing its own narrows (or widens) that, and the plan reports what it used.
	 */
	TArray<FString> Selectors;

	/**
	 * Destroy in-scope actors the file does not mention — §6.1's existence reconciliation.
	 *
	 * **An entry in the file is an existence claim**, so a restore that only writes properties onto
	 * whatever survived is reading half the file. The filter is what makes destroy-what-is-missing
	 * well defined: two bosses live, one in the file, scope `class:BP_Boss` — the extra goes, and the
	 * player is not in scope so it is never considered.
	 */
	bool bReconcileExistence = true;

	/**
	 * Suppress Blueprint `BeginPlay` and `EndPlay` for the classes this restore touches — §7.3b/c.
	 *
	 * **OPT-IN, and the default is off on purpose.** Suppression is not deferral: the state flag
	 * still ends up correct, so a skipped `BeginPlay` is skipped for that actor's whole life and can
	 * never be run later. A class whose `BeginPlay` does real SETUP — binding a delegate, caching a
	 * partner, starting a timer — is left without it, and the restore only carries reflected
	 * properties. Off by default means "the world behaves as the game built it" unless a caller
	 * decides otherwise.
	 *
	 * What it buys, on each end:
	 *
	 *   SPAWN    the spawn sound, the "wave started" broadcast, the analytics event, an actor that
	 *            destroys itself from BeginPlay. T1 already protects the VALUES by writing last, so
	 *            this is purely about what a restore does to the WORLD.
	 *   DESTROY  loot drops, score, autosave. **There is no T1 equivalent here — nothing can be
	 *            injected after a destroy — so for this end suppression is the only mechanism.**
	 *
	 * Held only across the destroy pass and the spawn pass, never across the inject pass and never
	 * across a wait: the unhook is per-CLASS, so a scope that outlived this synchronous call would
	 * silence every live instance of that class, including ones no restore is touching.
	 */
	bool bSuppressLifecycleEvents = false;

	/** Plan only. Nothing is spawned, destroyed or written. */
	bool bDryRun = false;

	/**
	 * Where a RELATIVE placement's transform comes from — `GC-SPATIAL-AUTHORING-DESIGN.md` §20.9.
	 *
	 * Null is the ordinary call and the ordinary answer: an entry carrying `@anchor` / `@surface` /
	 * `@facing` REFUSES BY NAME rather than being ignored, because a placement directive that does
	 * nothing puts an actor at the origin and reads like a successful restore.
	 *
	 * **AN INTERFACE FOR THE REASON `IGcPlacementContacts` IS ONE, and it is the same seam a third
	 * time.** The placement resolver lives in Core's editor module; this one is Runtime, and a
	 * packaged game has no resolver at all. Rather than dragging §20's arithmetic across a module
	 * boundary it cannot cross, the caller that HAS one hands it in — so the capability is honestly
	 * editor-bound, and the refusal above says so out loud instead of a shipped build silently
	 * placing things at the origin.
	 *
	 * A CAPTURE NEVER WRITES THESE. `@placement` is what a capture records, because a captured world
	 * is absolute by definition; relative directives only ever arrive in a document somebody
	 * AUTHORED, which is exactly the `document:` half of `gcp_apply`.
	 */
	const class IGcPlayPlacementResolver* PlacementResolver = nullptr;
};

/**
 * The four relative directives, and what one resolved to — §20's result, restated with no dependency
 * on §20's types so this header stays Runtime.
 *
 * IT MIRRORS `FGcPlacementResult` FIELD FOR FIELD ON PURPOSE. `bStatesLocation` and friends are not
 * conveniences: a `@scale` on its own states neither a position nor a rotation, and answering
 * identity for the ones it does not state would spin an actor to yaw zero because somebody asked to
 * move it sideways.
 */
struct FGcpRelativePlacement
{
	bool bStatesLocation = false;
	FVector Location = FVector::ZeroVector;

	bool bStatesRotation = false;
	FQuat Rotation = FQuat::Identity;

	bool bStatesScale = false;
	FVector Scale = FVector::OneVector;

	/** `@surface -> contact 12.5 up, patch normal (0,0,1)` — what a caller reads when it lands oddly. */
	TArray<FString> Evidence;
};

/**
 * Resolves `@anchor` / `@surface` / `@facing` on a live world. Implemented editor-side.
 *
 * IT ANSWERS IN WORLD SPACE, which is what the restore wants: `ApplyWorldPlacement` sets an actor's
 * WORLD transform, because a capture is absolute and an attached actor is still somewhere in the
 * world. A resolve answers in world space too, for the same reason — the targets are.
 */
class GRAPHCODEPLAY_API IGcPlayPlacementResolver
{
public:
	virtual ~IGcPlayPlacementResolver() = default;

	/**
	 * `Entry`'s directives against `World`, or a named refusal.
	 *
	 * `Actor` is the thing being placed when it already stands there, and null for one this restore is
	 * about to spawn — the resolver needs its SIZE either way, and a class default is where that comes
	 * from before a spawn. Measured before the spawn on purpose: a placement that can only refuse
	 * after the actor exists refuses halfway through a write.
	 */
	virtual bool Resolve(UWorld* World, const struct FGcpActor& Entry, const AActor* Actor,
		FGcpRelativePlacement& OutPlacement, FString& OutRefusal) const = 0;

	/** Whether this entry carries anything relative at all — asked before a resolver is required. */
	static bool EntryStatesRelativePlacement(const struct FGcpActor& Entry);
};

/** What a restore will do, or did. Also the diagnosis when it refuses. */
struct FGcpRestorePlan
{
	/** Live, in the file: properties written onto an actor that already existed. */
	TArray<FString> Update;

	/** In the file, not live: spawned at the name the entry gives. */
	TArray<FString> Spawn;

	/** Live and in scope, not in the file: destroyed. Empty when `bReconcileExistence` is off. */
	TArray<FString> Destroy;

	/**
	 * §7.4 — classes in the restore set with their OWN Blueprint `ReceiveBeginPlay`.
	 *
	 * Reported whether or not it matters, because *empty* is the interesting value: it is a proof
	 * that no Blueprint BeginPlay logic ran during this restore, available before anything is
	 * touched. A non-empty list is not a fault; it is the list to read once if something looks wrong.
	 */
	TArray<FString> BeginPlayClasses;

	/**
	 * Classes whose Blueprint `BeginPlay` / `EndPlay` this restore actually unhooked.
	 *
	 * Empty when `bSuppressLifecycleEvents` is off, and **also empty when it is on and no class in
	 * the set carried the event** — which is the ordinary case and not a failure. Reported separately
	 * per end because they are different sets: `BeginPlay` covers what was spawned, `EndPlay` what
	 * was destroyed, and a restore very often does one and not the other.
	 */
	TArray<FString> SuppressedOnSpawn;
	TArray<FString> SuppressedOnDestroy;

	/** Refusals and faults. Non-empty means nothing was applied, unless `Written` says otherwise. */
	TArray<FString> Errors;

	TArray<FString> Warnings;

	/**
	 * Actors already written when a fault stopped the apply.
	 *
	 * The level writer's precedent and its reasoning: a UWorld is not duplicated to prove an edit, so
	 * a failure mid-apply cannot claim the world is untouched. Naming what was written beats both
	 * lying about it and rolling back something we cannot prove we can roll back.
	 */
	TArray<FString> Written;

	/**
	 * Lines the file carries that this restore could not put back, each with the codec's reason.
	 *
	 * **The honest half of the result, and NOT an error.** §6.5 measured that a `.gcp` restore is
	 * exact for reflected state and a save-point primitive for everything else; this is where that
	 * stops being a paragraph in a design document and becomes a value the caller can read. A
	 * capture's value grammar is wider than the codec's importer — `BodyInstance=[]` is observable
	 * and not parseable — so some lines are evidence rather than instructions.
	 *
	 * Empty means the file was restored in full. It must never be quietly empty: the re-capture gate
	 * CANNOT catch a silent drop here, because the next capture writes the same unimportable
	 * spelling and the diff comes back clean.
	 */
	TArray<FString> NotRestored;

	int32 PropertiesWritten = 0;

	/** True only after a non-dry `Apply` that reached the end. */
	bool bApplied = false;
};

/** §6's restore. Reads a document, writes a live world — never an asset (§18). */
class FGraphCodeSnapshotRestore
{
public:
	/**
	 * Resolve and validate everything that can be checked without writing.
	 *
	 * Classes resolve, names match, no live actor answers to a name at a DIFFERENT class, nothing in
	 * scope is replicated, and the baseline is one a restore may act on. Property paths are not
	 * pre-validated: `ImportDeviations` is the only authority on what a path means and it collects
	 * every independent failure itself, so duplicating that here would be a second opinion that can
	 * disagree. Those faults surface in `Apply`, alongside `Written`.
	 */
	GRAPHCODEPLAY_API static bool Plan(UWorld* World, const FGcpDocument& Document,
		const FGcpRestoreOptions& Options, FGcpRestorePlan& OutPlan);

	/**
	 * Plan, then do it: destroy, spawn ALL, then inject ALL.
	 *
	 * **Two passes, and it is the one ordering that actually bites.** Injecting as each actor spawns
	 * leaves actor 1's `LinkedActor` pointing at actor 5 unresolvable, because actor 5 does not exist
	 * yet. Spawning everything first costs nothing and removes the case entirely.
	 */
	GRAPHCODEPLAY_API static bool Apply(UWorld* World, const FGcpDocument& Document,
		const FGcpRestoreOptions& Options, FGcpRestorePlan& OutPlan);

	/** §7.4's detector, exposed because "does this class have Blueprint BeginPlay" has other callers. */
	GRAPHCODEPLAY_API static bool HasOwnBlueprintBeginPlay(const UClass* Class);

	/**
	 * ONE property of ONE live actor, as a designer could set it on an instance (gap report 13 Nov, V2
	 * item 6; card P201) — `gcp_run set`, the write beside `query what:actor property:`.
	 *
	 * **NO NEW PARSER, AND NO NEW RULE FOR WHAT MAY BE WRITTEN.** The value goes through
	 * `AuthoringCore::ImportDeviations` in PATCH under `FGraphCodeActorEntryWriter::PlacedActorPolicy`
	 * against `BaselineFor` — the level writer's own import of a placed actor — so the value grammar is
	 * a `.gc`'s and a property the level document cannot carry is refused here too. It must also be
	 * `CPF_Edit` without `CPF_DisableEditOnInstance` or `CPF_EditConst`: what the Details panel lets a
	 * person type on an instance. `OutBefore`/`OutAfter` are the engine's export text, the same text
	 * `query` returns, so a `query` after a `set` reads back exactly `OutAfter`.
	 *
	 * Nothing reruns construction scripts or calls `PostEditChange`: this is a running game, and a
	 * construction script rerun would reset the very state a test set up.
	 */
	GRAPHCODEPLAY_API static bool SetInstanceProperty(AActor* Actor, const FString& Property, const FString& Value,
		FString& OutBefore, FString& OutAfter, FString& OutError);
};
