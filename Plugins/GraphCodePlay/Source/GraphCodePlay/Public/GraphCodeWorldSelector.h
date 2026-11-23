// The selector language — `class:`, `name:`, `tag:`, `near:`, `*` — and the ONE implementation of
// what each of them means.
//
// Lifted out of `GraphCodeWorldSnapshot.cpp`'s anonymous namespace on 2026-08-14, when restore
// arrived and needed the same answer. `PLAY-SESSION-DESIGN.md` §6.3 requires a restore to **re-run
// the filter as a check**: the file's entries are what gets written, and re-running the filter is how
// you notice the world grew an actor the file does not mention. A second implementation of matching
// would make capture and restore disagree about what "in scope" means, silently, with nothing
// erroring — a read answering through anything but the write path.
//
// So the capture writer and the restore share this object, and `select:` in a `.gcp` envelope means
// exactly one thing no matter which of them is reading it.
//
// **`Matches` is deliberately non-const:** a selector counts its own hits, because a selector that
// matched nothing must be able to say so (§5.2's `select:` echo). The count is the reason a capture
// of a world with no doors reads as *looked, found none* rather than as a typo.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UClass;
class UWorld;

enum class EGraphCodeSelectorKind : uint8
{
	Class,
	Name,
	Tag,
	Near,

	/** `*` — every actor. Since §4.1 this is the SAVE STATE, not a debugging accident. */
	Everything,
};

/** One parsed selector, with the text it came from so a diagnostic can quote the caller. */
struct FGraphCodeWorldSelector
{
	EGraphCodeSelectorKind Kind = EGraphCodeSelectorKind::Class;
	FString Value;
	double Radius = 0.0;

	/** As the caller spelled it — what a "matched nothing" warning names. */
	FString Source;

	/** Set during the walk. Zero after a full walk means this selector matched nothing. */
	int32 Matches = 0;
};

/**
 * Whether a `.gcp` ENTRY is in some scope, answered from the file alone. **Three answers, and the
 * third is the whole point.**
 *
 * `Matches` needs a live `AActor` — its class, its tags, its position. A comparison of two files on
 * disk has none of those, only what each entry wrote down: a name, a class path, and whatever
 * directives came with it. For `*`, `name:` and `class:` that is enough. For `tag:` and `near:` it
 * is not, and the difference between *this entry is outside that scope* and *I cannot tell* must
 * survive into the answer, because they license opposite conclusions: the first says an absence is
 * meaningless, the second says nobody knows what the absence means.
 *
 * Collapsing `Undecidable` into `Outside` would silently drop entries; collapsing it into `Inside`
 * would report spawns and deaths that never happened. It is its own value so that neither happens.
 */
enum class EGraphCodeScopeAnswer : uint8
{
	Inside,
	Outside,

	/** A `tag:` or `near:` selector, or a class path naming a class that is not loaded. */
	Undecidable,
};

/**
 * A parsed selector set: the scope of a capture, and the same scope re-run as a restore's check.
 *
 * Selectors UNION. An actor matching any one of them is in scope, which is why a per-selector option
 * would have no principled tie-break for an actor matching two (see `bAllProperties`' comment in
 * `FGraphCodeSnapshotOptions`).
 */
class FGraphCodeWorldSelection
{
public:
	/** Parse every selector, or fail with the first one that is not a selector. */
	GRAPHCODEPLAY_API static bool Parse(const TArray<FString>& Texts, FGraphCodeWorldSelection& Out,
		FString& OutError);

	/**
	 * Resolve `near:` anchors BEFORE the walk.
	 *
	 * A missing anchor is one clear error rather than a selector that silently matches nothing —
	 * the two failures are indistinguishable from the file, and only one of them is the caller's
	 * fault.
	 */
	GRAPHCODEPLAY_API bool ResolveAnchors(UWorld* World, FString& OutError);

	/** Is this actor in scope? Counts the hit on every selector that claims it. */
	GRAPHCODEPLAY_API bool Matches(const AActor* Actor);

	/**
	 * Is this `.gcp` entry in scope, judged from what the entry wrote down?
	 *
	 * **`const`, AND IT COUNTS NOTHING.** `Matches` increments each selector's hit count because a
	 * capture's walk is what `select:`'s echo reports on (§5.2). This is not a walk — it asks about
	 * an entry in somebody else's file — so counting here would make `AppendMissWarnings` describe a
	 * capture that never happened.
	 *
	 * `Label` is `@label`'s payload where the entry carried one and empty otherwise, which is the
	 * file's spelling of the second half of `NameMatches`. Absent means *the label equals the name*,
	 * because the writer only emits `@label` when it differs — so an empty one costs no answer.
	 *
	 * **Class matching resolves rather than loads.** A leaf that equals the token, or the token with
	 * `_C`, is decided outright — which is what every `class:` in this corpus is. Anything needing
	 * the superclass chain asks for an already-loaded `UClass` and answers `Undecidable` when there
	 * is none: loading an asset to answer a read would be a read with side effects, and a stall on
	 * the game thread in the one path `Wake()` runs.
	 */
	GRAPHCODEPLAY_API EGraphCodeScopeAnswer CoversEntry(const FString& ActorName, const FString& Label,
		const FString& ClassPath) const;

	/** One warning per selector that matched nothing, after a complete walk. */
	GRAPHCODEPLAY_API void AppendMissWarnings(TArray<FString>& OutWarnings) const;

	/** Zero the hit counts, so one selection can be walked twice — capture, then restore's check. */
	GRAPHCODEPLAY_API void ResetCounts();

	/** Whether `*` is among them, which is what makes a capture the whole loadable state (§4.1). */
	GRAPHCODEPLAY_API bool SelectsEverything() const;

	bool IsEmpty() const { return Selectors.IsEmpty(); }
	const TArray<FGraphCodeWorldSelector>& Get() const { return Selectors; }

	/** Subclass-inclusive, and tolerates the `_C` a Blueprint class carries. */
	GRAPHCODEPLAY_API static bool ClassMatches(const UClass* Class, const FString& Token);

	/** Internal name first, then the editor label where one exists. */
	GRAPHCODEPLAY_API static bool NameMatches(const AActor* Actor, const FString& Token);

private:
	TArray<FGraphCodeWorldSelector> Selectors;
	TMap<FString, FVector> Anchors;
};
