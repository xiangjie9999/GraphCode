// Resolving a load — `PLAY-SESSION-DESIGN.md` §3.4, §3.6d, §3.6d-ii.
//
// A LOAD IS A STATE AND AN ENTRY, AND THEY TRAVEL TOGETHER. §3.6d-ii replaced §3.6d's single global
// stage slot with a plan bound to the dispatch it was made for, and this file is that binding: a
// resolved document plus the exact `UFunction` on the exact actor that will consume it.
//
// ---------------------------------------------------------------------------
// EVERYTHING IS RESOLVED BEFORE ANYTHING MOVES
// ---------------------------------------------------------------------------
//
// §3.5's plan-then-apply, raised from properties to the slice transition. The file is found, parsed
// and PLANNED against the live world; the entry is resolved to an actor and a function; and only then
// is anything queued. A caller that got a `false` from here knows the world is exactly as it was.
//
// **The one refusal that is not tidiness: a restore that would destroy the actor holding the entry.**
// §3.5 measured the symptom — `FLatentActionManager` keys on `TWeakObjectPtr` and silently deletes
// every action belonging to a collected object, with no exec pulse and no log line — and it is
// nondeterministic on top, since it depends on whether a GC ran. A self-destroying load is refused
// here, where refusing costs nothing.
//
// ---------------------------------------------------------------------------
// WHY THE PATH RULE LIVES HERE AND NOT IN THE TOOL
// ---------------------------------------------------------------------------
//
// `gcp_apply` had it first, in an editor-side anonymous namespace. The `Load` NODE needs the same
// rule and cannot reach that module, so it moved down rather than being copied: two answers to
// *which file does this name mean* would drift with nothing erroring, and a caller would get a
// different file depending on whether an AI or a graph asked for it
// ([[a-read-answers-through-the-write-path]]).

#pragma once

#include "CoreMinimal.h"

#include "UObject/WeakObjectPtr.h"

struct FGcpDocument;
class AActor;
class UWorld;

/**
 * One resolved load: the state to put back, and the entry that will consume it.
 *
 * **`Document` is SHARED rather than owned** because the same resolved load is handed from the
 * resolver to the queue to the dispatch, and copying a parsed world document at each hop is the kind
 * of cost that only shows up on a populated level.
 */
struct FGcPlayEntryCall
{
	TSharedPtr<FGcpDocument> Document;

	/** What the slice's `from:` will name. A PATH, never a blob, so the run stays replayable (§3.6d). */
	FString FromPath;

	/** The branch's prose — `description=` on the `Load` record. */
	FString Description;

	/**
	 * The actor whose event opens the new slice.
	 *
	 * WEAK, because a load sits in a queue for at least a tick and the world does not hold still for
	 * it. A target that died in that gap is a finding rather than a crash.
	 */
	TWeakObjectPtr<AActor> Target;

	FName Function;

	/** `Actor.Event`, kept as written for the log and for every refusal that has to name it. */
	FString Entry;

	bool IsValid() const { return Document.IsValid() && Target.IsValid() && !Function.IsNone(); }
};

/** §3.4's load, resolved. Reads a file and a live world; writes neither. */
class GRAPHCODEPLAY_API FGraphCodePlayLoad
{
public:
	/**
	 * `capture:` — a bare name as passed to `gcp_capture name:`, or a path. Empty when unresolvable.
	 *
	 * Three places, in the order a caller means them: an existing path wins, because somebody holding
	 * the `path` this family's own capture returned should never have to take it apart; then the
	 * snapshot directory, which is where `gcp_capture` puts things; then the RUNNING session's own
	 * folder, which is where a `Mark(Capture=true)` puts them and is the one a branch most often wants.
	 *
	 * `WorldContext` is only needed to reach that third place, so a caller with no world still gets
	 * the first two rather than a refusal.
	 */
	static FString ResolveCapturePath(const FString& Capture, const UObject* WorldContext = nullptr);

	/**
	 * Find the file, parse it, plan it against this world, and resolve `Actor.Event` — refusing at the
	 * first thing that does not hold, with nothing touched.
	 *
	 * `Entry` is `Actor.Event` and the qualification is required [USER, 2026-08-15]. An unqualified
	 * name is refused with the actors that carry an event of that name, which is §3.6a-i's *a menu, not
	 * a lock*: the caller is choosing, so it may be shown the choices.
	 */
	static bool Resolve(UWorld* World, const FString& From, const FString& Entry,
		const FString& Description, FGcPlayEntryCall& OutCall, TArray<FString>& OutErrors);
};
