// Copyright GraphCode Sdn Bhd. All Rights Reserved.
//
// DID THIS ACTOR MOVE SINCE THE LAST SAVE? — `GC-WORLD-GENERATION-DESIGN.md` §13.6, the witness
// half of the `Reflect` hook.
//
// **§13.6 SAID SETTINGS HAD NO SECOND CARRIER, AND THAT WAS TRUE OF THE ACTOR ALONE.** The shape
// half (§13.4) works because the engine stores two independent things — the BUILDER holds what the
// last realise put there and the POLYGONS hold what the world is — so *did the intent change* and
// *was it hand-edited* are two questions with two carriers and nothing has to be written down. A
// setting has one carrier on the actor: its current value.
//
// **THE `.gc` ON DISK IS THE SECOND CARRIER, AND IT COSTS NOTHING BECAUSE IT IS ALREADY WRITTEN.**
// `FGraphCodeService::HandlePackageSaved` regenerates a sidecar for every real project save,
// `Package->ContainsMap()` included, so the level's `.gc` records what every actor WAS at the end
// of the last save. User-ruled 2026-08-21: *"the hook should be based on whether gc changed, or
// else it's double work. so it's hook to gc file, and see whether the volume changes?"* — and it is
// better than a saving of work, because it is the record the row was parked for wanting.
//
// **IT MUST BE READ PRE-SAVE AND THAT IS LOAD-BEARING.** `FEditorDelegates::PreSaveWorldWithContext`
// fires BEFORE the package write and `UPackage::PackageSavedWithContextEvent` after it, so during
// the hook the `.gc` on disk still predates this save. Move the caller post-save and the sidecar has
// already been rewritten with the new values, every comparison answers *nothing changed*, and the
// feature silently does nothing for ever ([[a-silent-no-op-is-worse-than-a-refusal]] — except this
// one could not even refuse).
//
// **THE ENTRY IS COMPARED WHOLE RATHER THAN FIELD BY FIELD, ON PURPOSE.** A per-field answer would
// need a map from every grammar key to the UE property path the `.gc` spells it with — `seed=` to
// `Seed`, `area=` to `AreaClass`, `materials=` to `Settings.WeightedBlendables.Array` — which is a
// second carrier of what `Apply` already knows ([[two-carriers-of-one-fact-is-a-race]]). Whole-entry
// is coarse: a volume somebody merely MOVED reads as changed. That is harmless, because the caller
// still writes a line only where a reflected value actually differs from what the line claims, so
// the worst an over-trigger costs is a comparison.
//
// **AND THE VALUE NEVER COMES FROM HERE.** This says *the world moved*; what it moved TO is read off
// the actor by the product's own hook, which is the exact inverse of its `Apply`. Scraping values out
// of this text would be the mapping above, arrived at by the back door.

#pragma once

#include "CoreMinimal.h"

class AActor;

namespace GraphCodeLevelReflect
{
	/**
	 * The level's `.gc` as it stands on disk — empty when the level has no sidecar yet.
	 *
	 * **LOADED ONCE PER SAVE BY THE CALLER, NOT ONCE PER ACTOR.** A hook walking twenty volume lines
	 * would otherwise read the same file twenty times inside a save the user is waiting on.
	 */
	GRAPHCODE_API FString LoadSavedSidecar(const FString& LevelPackage);

	/**
	 * The `.gc` on disk that records what THIS ACTOR was at the last save, when the actor lives in
	 * its own package — empty for an in-package actor, and empty for one nothing has saved yet.
	 *
	 * **THE LEVEL'S SIDECAR IS THE WRONG FILE ON A PARTITIONED MAP, AND THAT MADE THE WHOLE HOOK
	 * INERT THERE** (`GC-WORLD-PARTITION-DESIGN.md` §7 row 3, audit §2.1). Under OFPA the level
	 * deliberately does not state an external actor — its entry is in a sidecar beside its own
	 * package — so `FindActorEntry` looked in the level's text, found nothing, and `HasMoved`
	 * answered *cannot tell* for every actor of the map type most real games use. Nothing said so:
	 * both §13.6 halves were shipped and silently doing nothing ([[a-silent-no-op-is-worse-than-a-refusal]]).
	 *
	 * **THE HANDLE IS THE SAME ON BOTH SIDES and that is not luck.** `AppendActorsBlock` counts the
	 * `_UAID_` suffix off the LEVEL rather than off the list handed in (row L9), so an actor's own
	 * one-entry document names it exactly as the level's block would have ([[gc-actor-document-name]]).
	 *
	 * **THIS ONE IS PER ACTOR, where `LoadSavedSidecar` is deliberately per save.** There is no
	 * choice: N external actors are N files. It is a 2–5 KB read against a shape comparison that
	 * rebuilds a brush, so the cost sits well under what the caller already pays per line.
	 */
	GRAPHCODE_API FString LoadSavedActorSidecar(const AActor* Actor);

	/**
	 * This actor's `actors { }` entry as the projector would write it NOW, framing stripped.
	 *
	 * Runs the real projection rather than a summary of it, so the two sides of the comparison are
	 * produced by one writer and cannot drift ([[a-read-answers-through-the-write-path]]).
	 */
	GRAPHCODE_API FString ProjectActorEntry(AActor* Actor);

	/**
	 * The entry named `Handle` inside `SidecarText`'s `actors { }` block, framing stripped, or empty.
	 *
	 * **EMPTY IS *NOT FOUND*, WHICH IS NOT THE SAME CLAIM AS *UNCHANGED*.** An actor with no entry in
	 * the last save is one nobody has saved yet; `HasMoved` below treats that as *cannot tell* rather
	 * than as movement, because reflecting off a comparison against nothing would rewrite a line from
	 * a volume the document has never seen recorded.
	 */
	GRAPHCODE_API FString FindActorEntry(const FString& SidecarText, const FString& Handle);

	/**
	 * True when this actor differs from what the last save recorded for it.
	 *
	 * False when the two agree AND when the question cannot be answered at all — no handle, no
	 * sidecar, no entry. **A caller must read that as *do not reflect*, never as *nothing changed*:**
	 * *we did not look* must not become *we looked and it was fine* in the direction that overwrites
	 * somebody's file ([[refusal-reason-is-a-claim]]).
	 *
	 * **`SidecarText` IS THE LEVEL'S AND IS IGNORED FOR AN ACTOR THAT HAS ITS OWN.** The branch is
	 * here rather than at the call site on purpose: a caller that forgot it would get silence rather
	 * than a refusal, and the next caller written would forget it again
	 * ([[a-new-line-kind-must-reach-the-assembler]]). Callers keep loading the level's text once per
	 * save, because on a mixed map — which is what a conversion leaves — both kinds of actor are
	 * still asked through this one function.
	 */
	GRAPHCODE_API bool HasMoved(AActor* Actor, const FString& Handle, const FString& SidecarText);
}
