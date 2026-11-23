// Calling one thing the running game already knows how to do — `RUNTIME-DRIVE-DESIGN.md` §17 P4.
//
// **THE AI's HANDS.** Everything before this either read the world (`gcp_capture`), wrote its state
// (`gcp_apply`) or talked to the graph (`continue`/`end`/`load`). None of them can make the game DO
// something on its own terms: fire an ability, open a door, save, spawn the boss. This is that verb,
// and [USER, 2026-08-16] ruled its shape — *"invoke should straight away invoke the function
// themselves"*, and *"it should be able to control everything"*.
//
// ---------------------------------------------------------------------------
// WHY THIS FILE EXISTS AT ALL, AND IT IS NOT THE INVOKE
// ---------------------------------------------------------------------------
//
// `Actor.Function` was already resolved once, inside `FGraphCodePlayLoad::Resolve`, where a load
// finds the entry event that opens its new slice. An invoke needs the identical four steps — split
// on the last dot, find the actor, find the function, say something useful when one of them is not
// there — and **two implementations of *which function did you mean* would drift**, which is the
// same argument that lifted the selector language out of `GraphCodeWorldSnapshot.cpp`
// ([[a-read-answers-through-the-write-path]]).
//
// So the resolution moved HERE and both callers use it. What did **not** move is the wording: this
// returns a FAULT, and each caller renders its own sentence from it, because a load's refusal talks
// about entries and slices and an invoke's talks about arguments. **Branch on the fault, never on
// who is asking** ([[branch-on-data-not-on-caller-identity]]).
//
// ---------------------------------------------------------------------------
// WHAT AN INVOKE IS ALLOWED TO BE
// ---------------------------------------------------------------------------
//
// **It is POSTED on the control channel and executed in order, not run where it was typed.** The
// tool call could reach straight into the world — the handler is already on the game thread — and
// that was the first design. It is wrong, and the reason is the sequence this exists to serve:
// *resume, inject, load, invoke*. `continue`, `end` and `load` are queued and consumed a tick later,
// so an invoke that ran immediately would happen BEFORE the resume it was written after. One
// channel, one order (§5) — and the price, paid deliberately, is that the reply says what was ASKED
// FOR and the result arrives in the log ([[a-deferred-act-owes-an-outcome]]).
//
// **The lease is the whole safety model and it comes for free.** Only an attached driver may post,
// a command from a lapsed lease is dropped rather than run, and a packaged game has no registrar to
// attach to. P4's original wording asked for an allowlist of callable functions; that was written
// before the lease existed and it would now be a second gate in front of the same door.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UFunction;
class UWorld;

/** Why a call could not be resolved. One sentence per caller, rendered from this rather than here. */
enum class EGcPlayCallFault : uint8
{
	None,

	/** Not `Actor.Function` — no dot, or nothing on one side of it. */
	Unqualified,

	/** No actor of that name is in this level. */
	NoSuchActor,

	/** The actor is there and carries no function of that name. */
	NoSuchFunction,
};

/** One resolved call: who, what, and the spelling it arrived as. */
struct FGcPlayCallTarget
{
	/**
	 * WEAK, because a posted command waits at least a tick and the world does not hold still. A
	 * target that died in the gap is a finding rather than a crash — the same reason
	 * `FGcPlayEntryCall::Target` is weak.
	 */
	TWeakObjectPtr<AActor> Actor;

	UFunction* Function = nullptr;

	FString ActorName;
	FString FunctionName;

	/** `Actor.Function` as written, kept for the log and for every refusal that has to name it. */
	FString Spec;

	bool IsValid() const { return Actor.IsValid() && Function != nullptr; }
};

/** What a call did, as evidence. Empty `Observed` means the function returned nothing to observe. */
struct FGcPlayCallResult
{
	/** `Name=value` for every return and out parameter, in the `.gc` value grammar. */
	FString Observed;
};

class GRAPHCODEPLAY_API FGraphCodePlayCall
{
public:
	/**
	 * Find the actor and the function `Spec` names. Fills `Out` only on `None`.
	 *
	 * Case-sensitive on the actor's name because that is how the level addresses it, and by `FName`
	 * on the function because that is how the engine does.
	 */
	static EGcPlayCallFault Resolve(UWorld* World, const FString& Spec, FGcPlayCallTarget& Out);

	/**
	 * Actors in this world carrying a function of that name — the *did you mean* list.
	 *
	 * Sorted, and it exists because the commonest way to get a call wrong is to name the function
	 * correctly and the actor not at all.
	 */
	static TArray<FString> ActorsCarrying(const UWorld* World, const FString& FunctionName);

	/**
	 * Resolve and typecheck everything WITHOUT calling anything.
	 *
	 * The tool runs this before posting so a caller's refusal arrives in the reply to the call it
	 * made rather than as a finding in the log a tick later. It allocates the parameter block,
	 * imports every argument into it and throws it away — so a value that cannot be parsed is caught
	 * here, by the code that will parse it for real.
	 */
	static bool Plan(UWorld* World, const FString& Spec, const FString& Arguments,
		TArray<FString>& OutErrors);

	/**
	 * Resolve, marshal the arguments, call it, and read back what came out.
	 *
	 * **Arguments are NAMED, never positional** — `(Damage=50,Knockback=true)`, in the same value
	 * grammar a `.gc` and a `.gcp` use, because a game's function signature is not something a
	 * caller should have to know the order of. An omitted parameter is left zero-initialised, which
	 * is what `ProcessEvent` would have given it anyway; an argument naming a parameter the function
	 * does not have is a refusal, since silently dropping it is how a caller comes to believe they
	 * set something.
	 */
	static bool Execute(UWorld* World, const FString& Spec, const FString& Arguments,
		FGcPlayCallResult& OutResult, TArray<FString>& OutErrors);
};
