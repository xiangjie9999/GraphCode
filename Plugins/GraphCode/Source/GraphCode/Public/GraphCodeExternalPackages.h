// Copyright OraStudio. All Rights Reserved.
//
// WHAT ONE WRITE MUST SAVE ON A MAP WHOSE ACTORS ARE NOT IN IT — `GC-WORLD-PARTITION-DESIGN.md` §7
// rows 1a/1b. The measurements are `GC-WORLD-PARTITION-AUDIT.md` §2.2 and §2.2a.
//
// **THE DEFECT THIS EXISTS TO STOP, MEASURED 2026-08-21.** Every realiser under a `.gcw` write
// spawns or edits an actor. On an ordinary level that actor lives in the `.umap`, so saving the
// world commits it; under OFPA it lives in its OWN package and saving the world commits nothing
// about it. One `postprocess(...)` line applied to `RuntimeFixture_WP` created the volume, wrote its
// `.gc` with NO `bin:` — the writer knew there was no package to digest — and never wrote the
// `.uasset`. The actor existed only in memory while a document on disk described it.
//
// **THE KEY IS WHAT THIS WRITE DIRTIED, not which actors carry a `GraphCode.*` tag.** A tag
// vocabulary grows one verb at a time and the next verb added would silently not be saved
// ([[a-new-line-kind-must-reach-the-assembler]]). Snapshotting the already-dirty set first keeps the
// save narrow in the other direction too: an actor somebody ELSE left dirty is not this write's to
// commit.
//
// **ASK THE PACKAGE FOR ITS EXTERNAL PACKAGES, NOT THE LEVEL FOR ITS ACTORS, and that is the whole
// of why the sweep works.** The first cut walked `PersistentLevel->Actors` — which can only ever see
// an actor that still EXISTS, so a DESTROYED actor's now-empty package was never collected and its
// file survived the sweep. `UPackage::GetExternalPackages()` is the engine's own set, the one
// `FileHelpers.cpp`'s `SaveExternalPackages` uses, and it holds a destroyed actor's package for as
// long as it stays loaded. So ONE collection serves BOTH directions: a created actor's package is
// saved, and a destroyed actor's empty one goes to the same save, which is where the engine removes
// the file. The sidecar follows on its own — `FGraphCodeService::HandleAssetRemoved` already listens
// on `OnAssetRemoved` and deletes it.
//
// **IT LIVES HERE RATHER THAN AT THE CALL SITE BECAUSE IT HAS MORE THAN ONE CALLER.** A `.gcw`
// apply is the one that found the defect; anything else that realises actors into a world and then
// saves it needs the same two lines, and a rule copied is a rule that drifts
// ([[two-carriers-of-one-fact-is-a-race]]).

#pragma once

#include "CoreMinimal.h"

class UPackage;
class UWorld;

namespace GraphCodeExternalPackages
{
	/**
	 * Every external package of this world that is dirty RIGHT NOW, by name.
	 *
	 * **NAMES AND NOT POINTERS, deliberately.** A realiser between the two calls may collect
	 * garbage, and a raw `UPackage*` held across that would be comparing against a corpse.
	 *
	 * On an unpartitioned, non-OFPA map this is always empty, which is what makes the pair below
	 * a no-op there rather than a special case anybody has to remember.
	 */
	GRAPHCODE_API void CollectDirty(const UWorld* World, TSet<FName>& OutNames);

	/**
	 * The packages one write must save: the world's own, PLUS every external one this write dirtied
	 * and found clean on the way in.
	 *
	 * `DirtyBefore` is what `CollectDirty` answered before the write ran. The world's package is
	 * unconditional — `bOnlyDirty=true` at the save decides whether it is actually written — so on
	 * an unpartitioned map this returns exactly the one-package array it replaced.
	 */
	GRAPHCODE_API TArray<UPackage*> ToSaveAfterWrite(UWorld* World, const TSet<FName>& DirtyBefore);

	/**
	 * The same set WITHOUT the world's own package, by name: exactly what this write dirtied.
	 *
	 * **IT EXISTS BECAUSE ONE CALLER'S SAVE IS NOT AT ITS WRITE.** `gc_apply` routes a block to a
	 * writer and a single wrapper saves afterwards (`GraphCodeApply.cpp`, C24.0), so the seat that
	 * knows the world is not the seat that calls the save — §7 row 4a. That caller records this
	 * answer on its result and the wrapper hands it to `SavePackageAfterWrite`, which already
	 * carries the world's own package by name.
	 *
	 * Names for `ToSaveAfterWrite`'s reason and one more: this answer is *carried across the rest of
	 * the apply*, not consumed where it was taken, so a `UPackage*` in it would be a pointer held
	 * across a sidecar re-projection as well as a write.
	 */
	GRAPHCODE_API TArray<FName> DirtiedByWrite(const UWorld* World, const TSet<FName>& DirtyBefore);

	/**
	 * The editor world this SAVING OBJECT belongs to, when it lives in one of that world's external
	 * actor packages — null otherwise. The second half of `GC-WORLD-PARTITION-DESIGN.md` §7 row 3b.
	 *
	 * **PAIR IT WITH `FCoreUObjectDelegates::OnObjectPreSave`, WHICH IS THE ONE THAT FIRES.**
	 * `UPackage::PreSavePackageWithContextEvent` is the obvious hook, it is public, it is not
	 * deprecated — and MEASURED 2026-08-21, **nothing in UE 5.7 broadcasts it**: the symbol is
	 * defined in `Package.cpp:76` and referenced nowhere else in the engine. Hooking it compiles,
	 * registers, and never runs, which is this row's own defect wearing the fix's clothes
	 * ([[engine-has-the-field-vs-engine-runs-it]]). `OnObjectPreSave` is broadcast from
	 * `UObject::PreSave` (`Obj.cpp:1531`, under `WITH_EDITOR`), before the package's bytes are
	 * written — which is the side the witness needs.
	 *
	 * **WHY ANY OF THIS EXISTS: A SAVE THAT MOVES ONLY AN ACTOR NEVER SAVES THE WORLD.** Every
	 * pre-save reflect in this project hangs off `FEditorDelegates::PreSaveWorldWithContext`, which
	 * `UEditorEngine::OnPreSaveWorld` broadcasts only while the WORLD object is being saved
	 * (`EditorEngine.cpp:4622`) — and `FEditorFileUtils::SaveCurrentLevel` puts the level's package
	 * in the save set only when it is dirty (`FileHelpers.cpp:4438`), gathering external packages
	 * separately beside it. Measured: moving an external actor dirties its OWN package and leaves
	 * the world's clean. So on a partitioned map a person drags a generated volume, saves, and the
	 * hook never reaches its first line ([[a-silent-no-op-is-worse-than-a-refusal]]).
	 *
	 * **IT ASKS THE ACTOR, NOT THE PATH.** Reverse-mapping the `__ExternalActors__` path to a level
	 * package name would answer about a level that may not be loaded, and comparing against the
	 * world's whole external set is quadratic across a save batch. The actor knows its level and its
	 * world, and that is one hop.
	 *
	 * **AND IT IS CHEAP IN THE COMMON CASE, WHICH MATTERS BECAUSE IT RUNS ON EVERY OBJECT SAVED
	 * ANYWHERE.** The path test is first and rejects every ordinary asset with a substring scan; the
	 * object walk below it happens only inside a package that already matched, and only when the
	 * saving object is not itself the actor (a component's `PreSave` fires too).
	 *
	 * **THE CALLER STILL OWES THE ONCE-PER-SAVE GUARD**, and it is not optional. A batch saves N
	 * packages and this answers for every one of them, so a caller that reflected per call would
	 * rewrite its document N times — and worse, only the FIRST call is correct: by the second
	 * package's pre-save the first has already been written and its sidecar regenerated, so the
	 * witness would compare against a baseline this same save produced.
	 */
	GRAPHCODE_API UWorld* SavingExternalActorWorld(UObject* SavingObject);
}
