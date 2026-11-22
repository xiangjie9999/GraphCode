// §7.3's T3, as a product facility rather than a probe — `PLAY-SESSION-DESIGN.md` §7.3b/§7.3c.
//
// Suppresses a Blueprint lifecycle EVENT for the duration of a scope, by removing the class's own
// entry from its function map. The BlueprintImplementableEvent thunk is an unconditional
// `ProcessEvent(FindFunctionChecked(...))`, so the lookup falls through to `AActor`'s own inert
// version — **the ordinary state of every actor with no such node.** Nothing is patched.
//
//   `UClass::RemoveFunctionFromFunctionMap`  public, lock-guarded  `Class.h:4139`
//   `UClass::AddFunctionToFunctionMap`       public                `Class.h:4118`
//
// **Why this exists at all, in one line:** for BeginPlay it is an improvement on T1, which already
// worked by writing values last. For EndPlay it is the ONLY mechanism there is — nothing can be
// injected after a destroy.
//
// ---------------------------------------------------------------------------
// THE THREE LIMITS, EVERY ONE OF THEM MEASURED RATHER THAN ASSUMED
// ---------------------------------------------------------------------------
//
// - **PER-CLASS, never per-instance.** Every live instance of the class is affected for the scope.
//   That is contained inside a synchronous restore, where nothing else can spawn, and is NOT
//   contained across a wait. It cost a red run to see: the gate that measured Tick suppressed its own
//   control actor, because both were the same Blueprint class.
// - **Blueprint events only.** A C++ `virtual void BeginPlay() override` is not dispatched through
//   the function map, and `AActor::OnEndPlay` is a native multicast delegate (`Actor.cpp:3245`).
//   Both fire regardless. A BlueprintNativeEvent degrades to running its C++ `_Implementation`.
// - **Actor classes only.** A component is a separate UClass with its own `ReceiveBeginPlay`
//   (`ActorComponent.h:929`) and needs its own unhook. `FGraphCodeSnapshotRestore` reports the ones
//   it could not cover rather than leaving the gap silent.
//
// RAII and non-copyable, because the entry is on the CLASS: a leak on an early return would disable
// that event for every later caller in the same process, which is far worse than a failed restore.

#pragma once

#include "CoreMinimal.h"

class UClass;
class UFunction;

/** Blueprint event names this suppresses. Spelled once, because a typo here fails silently. */
namespace GraphCodeLifecycleEvents
{
	GRAPHCODEPLAY_API extern const FName BeginPlay;
	GRAPHCODEPLAY_API extern const FName EndPlay;
	GRAPHCODEPLAY_API extern const FName Tick;
}

/**
 * Remove one Blueprint event from a set of classes for this scope, and put every entry back.
 *
 * Classes with no override of their own are skipped rather than treated as an error: that is the
 * ordinary case, and removing an INHERITED entry would remove `AActor`'s inert one — a no-op that
 * would nonetheless report a successful suppression of a class that never had an override.
 */
class FGraphCodeScopedEventSuppression
{
public:
	GRAPHCODEPLAY_API FGraphCodeScopedEventSuppression(TConstArrayView<UClass*> Classes, FName Event);
	GRAPHCODEPLAY_API ~FGraphCodeScopedEventSuppression();

	FGraphCodeScopedEventSuppression(const FGraphCodeScopedEventSuppression&) = delete;
	FGraphCodeScopedEventSuppression& operator=(const FGraphCodeScopedEventSuppression&) = delete;

	/** Class names actually unhooked — the ones that HAD their own override. Never a guess. */
	const TArray<FString>& SuppressedClassNames() const { return Names; }

	/** True when at least one class was carrying the event. */
	bool IsActive() const { return Entries.Num() > 0; }

	/** §7.4's detector, generalised past BeginPlay. Answers before anything runs. */
	GRAPHCODEPLAY_API static bool HasOwnBlueprintEvent(const UClass* Class, FName Event);

	/**
	 * Every component class an actor class will spawn with — §7.5, and it needs TWO sources.
	 *
	 * A component is a separate UClass with its own `ReceiveBeginPlay`, so suppressing the actor
	 * class alone leaves component events firing. That was measured before it was fixed: the gate saw
	 * a Blueprint component's BeginPlay run through an actor-level suppression.
	 *
	 * - **The CDO** carries components created in a C++ constructor, inherited ones included.
	 * - **The SCS** carries Blueprint-added components, and it is walked up the class chain because a
	 *   parent Blueprint's nodes are not on the child's script. These do NOT exist on the CDO, which
	 *   is why one source is not enough.
	 *
	 * Safe to suppress, and §7.5 measured why rather than assuming it: `UActorComponent::BeginPlay`
	 * sets `bHasBegunPlay` in its NATIVE body, after the Blueprint call, so `AActor::BeginPlay`'s
	 * `ensureMsgf(Component->HasBegunPlay())` (`Actor.cpp:4771`) still holds. The engine also skips
	 * the event entirely for native components already — a guarded call where `AActor`'s is
	 * unconditional — so this puts a Blueprint component in a state the engine itself produces.
	 */
	GRAPHCODEPLAY_API static void GatherComponentClasses(UClass* ActorClass, TArray<UClass*>& Out);

private:
	struct FEntry
	{
		UClass* Class = nullptr;
		UFunction* Function = nullptr;
	};

	TArray<FEntry> Entries;
	TArray<FString> Names;
	FName Event;
};
