// Marshalling one call: named arguments in, out parameters back.
//
// ---------------------------------------------------------------------------
// WHY THIS IS IN AUTHORINGCORE AND NOT BESIDE EITHER CALLER
// ---------------------------------------------------------------------------
// Every line of this was `GraphCodePlayCall.cpp`'s, where it served ONE caller: an invoke posted on
// Play's control channel, into a running game. The editor half — calling the same kind of function
// on a live object with no PIE session at all — needs the identical four steps, and the identical
// value grammar, because the arguments are spelled the way a `.gc` spells a value.
//
// Two copies of *what does `Damage=50` mean* would drift on exactly the values that are hard to
// spell, which is the argument that moved the selector language out of `GraphCodeWorldSnapshot.cpp`
// and the resolution into `FGraphCodePlayCall` ([[a-read-answers-through-the-write-path]]). It lands
// HERE rather than in either plugin because Core must not depend on Play — Play is a product a
// customer may not have bought — and because the subject is values, which is this module's.
//
// WHAT STAYED BEHIND is the wording of every refusal that names a target: a Play call talks about
// actors in a level and an editor call talks about instances of a class. This produces the sentences
// that are about the ARGUMENTS, and nothing else.

#pragma once

#include "CoreMinimal.h"

class UFunction;
class UObject;

namespace AuthoringCore
{
	/**
	 * Split `(A=1,B=(X=2,Y=3),C="a,b")` into named arguments, at depth zero only.
	 *
	 * **A comma inside a nested struct or a quoted string is not a separator**, which is the whole
	 * reason this is not `ParseIntoArray`. The value grammar this feeds nests without limit — a
	 * vector inside a struct inside an array — and one wrong split produces a value that fails to
	 * import with an error about the wrong half of it.
	 *
	 * The outer parentheses are optional: `(A=1)` and `A=1` are the same argument list, because a
	 * caller writing one argument should not have to know it is a struct.
	 *
	 * POSITIONAL IS REFUSED RATHER THAN GUESSED. A signature's order is not something a caller
	 * reading a document can see, so a bare `50` comes back as an error naming what is missing.
	 */
	AUTHORINGCORE_API bool SplitNamedArguments(const FString& Arguments,
		TArray<TPair<FString, FString>>& Out, FString& OutError);

	/** Every parameter of a function, by name, in declaration order. */
	AUTHORINGCORE_API void GatherParameters(const UFunction* Function, TArray<FProperty*>& Out);

	/**
	 * Is this parameter something the call ANSWERS with, rather than something it takes?
	 *
	 * **`CPF_OutParm` ALONE IS NOT THE QUESTION, AND READING IT AS ONE MAKES EVERY BY-REFERENCE
	 * INPUT LOOK LIKE AN OUTPUT.** A Blueprint passes an array, a map or a struct input by
	 * reference, so the compiler sets `CPF_OutParm | CPF_ReferenceParm` on it — measured 2026-09-05
	 * on `BP_ProceduralBlock::GetPolyGroupIDFromSurface`, whose `TArray<FVector2D> Points` is
	 * declared under `input { }` in the sidecar and was reported by this module as `(out)`. A caller
	 * reading that does not pass it, and the function they most want to probe is the one they
	 * believe they cannot call.
	 *
	 * THE RULE IS THE PROJECT'S OWN, not a new one: `BlueprintIRResolver::RenderFunctionSignature`
	 * and `gc_describe`'s pin direction both ask exactly this, and both were right while this
	 * module's copy was wrong. Three spellings of one rule is how the third one drifts, so they
	 * share this.
	 */
	AUTHORINGCORE_API bool IsOutputParameter(const FProperty* Parameter);

	/**
	 * `float Damage, bool Knockback (out)` — or the word `nothing`.
	 *
	 * Every refusal about an argument has to say what the function actually takes, or the caller's
	 * next move is to go and read the graph.
	 */
	AUTHORINGCORE_API FString DescribeParameters(const UFunction* Function);

	/**
	 * The parameter block, constructed and destroyed by the same owner.
	 *
	 * `ProcessEvent` reads uninitialised memory when handed a block that was merely zeroed — a
	 * `FString` parameter is a live object with a heap pointer, and the engine's own
	 * `InitializeValue_InContainer` is the only thing that knows that. The destructor half matters
	 * as much: every out parameter that came back as a string or an array owns memory now.
	 */
	struct AUTHORINGCORE_API FFunctionParameterBlock
	{
		explicit FFunctionParameterBlock(const UFunction* InFunction);
		~FFunctionParameterBlock();

		FFunctionParameterBlock(const FFunctionParameterBlock&) = delete;
		FFunctionParameterBlock& operator=(const FFunctionParameterBlock&) = delete;

		const UFunction* Function = nullptr;
		uint8* Memory = nullptr;
	};

	/**
	 * Import every named argument into a freshly built block. Every fault, not the first.
	 *
	 * An argument naming a parameter the function does not have is a REFUSAL, never a skip: a
	 * dropped argument is how a caller comes to believe they set something, and the next thing they
	 * measure is a world that did not change for a reason they cannot see. An omitted parameter is
	 * left at its initialised value, which is what `ProcessEvent` would have given it anyway.
	 */
	AUTHORINGCORE_API bool FillParameters(const UFunction* Function, uint8* Memory, UObject* Owner,
		const FString& Arguments, TArray<FString>& OutErrors);

	/**
	 * `Name=value` for every return and out parameter, in the canonical value grammar.
	 *
	 * WHAT CAME BACK IS EVIDENCE, so it is read rather than discarded: a call whose answer is only
	 * visible as a side effect gives the caller nothing to assert on. A value this grammar cannot
	 * spell is said so — `<unspellable: reason>` — because an out parameter that vanished from the
	 * record reads as a function that returned nothing.
	 */
	AUTHORINGCORE_API FString ReadOutputs(const UFunction* Function, const uint8* Memory,
		const UObject* Owner);
}
