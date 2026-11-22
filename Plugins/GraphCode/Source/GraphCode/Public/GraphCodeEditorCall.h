// Calling one function on a live object in the EDITOR — the twin of `FGraphCodePlayCall`, which
// does the same thing inside a running game.
//
// ---------------------------------------------------------------------------
// WHY THERE ARE TWO, AND WHAT THEY SHARE
// ---------------------------------------------------------------------------
// Play's call is addressed `Actor.Function` inside a PIE world, and it REFUSES an editor world by
// name — measured, because `AActor::ProcessEvent` drops the call there and returns `void`, so a
// caller cannot tell a call that ran from one that did not. That refusal is right for Play, whose
// whole subject is the running game, and it left the editor with no way to make anything happen.
//
// This is the other half. It shares the marshalling — `AuthoringCore::FillParameters` and
// `ReadOutputs`, which moved out of Play for exactly this reason — and shares none of the
// addressing, because an editor target is not `Actor.Function`: it is the object a `.gc` document
// names, which is an instance of the class the document describes.
//
// ---------------------------------------------------------------------------
// THE EDITOR WORLD IS NOT A REFUSAL HERE, AND THIS IS THE MECHANISM
// ---------------------------------------------------------------------------
// `AActor::ProcessEvent` (`Actor.cpp:1468-1476`) runs the body when the world's actors are
// initialised, **or** the function carries `CallInEditor`, **or** `GAllowActorScriptExecutionInEditor`
// is true, **or** the object is a CDO. `FEditorScriptExecutionGuard` sets that global true for its
// own scope and restores it after (`ScriptCore.cpp:403`) — which is precisely what the details
// panel's own CallInEditor button wraps its call in
// (`PropertyCustomizationHelpers.cpp:424-443`). So the guard IS the editor's permission to run
// script, and this file takes it the same way the button does, for the same duration.
//
// A CONSEQUENCE WORTH SAYING OUT LOUD: the `[call_in_editor]` flag is not what makes a call
// possible. It is what makes a BUTTON possible — `CanCallFunctionBasedOnParams` gates the panel's
// list on that flag AND on `ParmsSize == 0`, because a button has nowhere to type an argument
// (`PropertyCustomizationHelpers.cpp:1367`). An agent is not a button. It can pass arguments, so the
// parameterless half of that rule does not bind it, and the flag half is reported rather than
// required: `bCallInEditor` on each callable says which ones a PERSON could also have pressed.
//
// What DOES bind: a function has to be reachable the way a graph node reaches it, and it has to
// have a body. Both are checked and both are named in the refusal, because
// `UObject::ProcessEvent`'s first early-out is a silent return for a function with no script
// ([[a-deferred-act-owes-an-outcome]] — an act that reports nothing is the one that gets believed).

#pragma once

#include "CoreMinimal.h"

class UClass;
class UFunction;
class UObject;

/** Which world a live instance is in — the difference between a real target and a preview one. */
enum class EGcEditorCallWorld : uint8
{
	/** No world at all: an editor utility object, a widget that is not in a level. */
	None,
	/** The level the user has open. The default target. */
	Editor,
	/** A Blueprint editor's preview scene. Real, addressable, and almost never what was meant. */
	Preview,
	/** Play In Editor. Live, but Play's — `gc_play` addresses those. */
	Play,
};

/** One live object the document's class could be invoked on. */
struct GRAPHCODE_API FGcEditorCallInstance
{
	/** `UObject::GetName()` — the handle, and the same spelling a level document writes. */
	FString Name;

	/** The World Outliner label, when an actor carries one that differs from its name. */
	FString Label;

	/**
	 * Full object path — the handle that is ALWAYS unique, and the second one `instance` accepts.
	 *
	 * A BARE NAME IS NOT AN ADDRESS. Measured 2026-08-27 driving a real app: thirteen live
	 * `BP_CabinetProperties` and eight `UW_ExtrudeProperties` shared one name each, so the name
	 * refused, this was printed, and passing it back refused too — visible and unaddressable.
	 */
	FString Path;

	/**
	 * True when this is a UMG widget Slate is currently showing in a window.
	 *
	 * The signal `gc_render source:"live"` already reports and this verb used to ignore. When
	 * exactly one instance of a class is on screen, that is the one a person asking about "the
	 * widget" means; the rest are constructed objects in no window.
	 */
	bool bOnScreen = false;

	/** The concrete class, which can be a subclass of the one the document describes. */
	FString ClassPath;

	EGcEditorCallWorld World = EGcEditorCallWorld::None;

	/** The level or window it is in, when there is one — what a person would use to recognise it. */
	FString Where;
};

/** One function that could be invoked on that class. */
struct GRAPHCODE_API FGcEditorCallable
{
	FString Name;

	/** `float Damage, bool Knockback (out)`, or `nothing`. */
	FString Signature;

	/** The `Category` metadata the details panel groups buttons by, when it has one. */
	FString Category;

	/** True when a person could also press this in the details panel: `[call_in_editor]`, no parameters. */
	bool bCallInEditor = false;

	/** True when this class declares it, rather than inheriting it. */
	bool bDeclaredHere = false;
};

/**
 * How much of the console log a call carries back.
 *
 * A THREE-WAY, NOT A BOOLEAN. The old `bCaptureLog` could only choose between paying for every
 * line a chatty function wrote and losing the error inside them, and a caller that wanted the
 * second without the first had no word for it.
 */
enum class EGcEditorCallLog : uint8
{
	/** Watch nothing. No output device is installed, so `LogErrors` is not a zero — it is unlooked. */
	None,
	/** Errors and warnings, in full. Quieter lines are COUNTED and left behind. */
	Loud,
	/** Every line at Log verbosity and louder, up to the cap, as `bCaptureLog:true` always did. */
	All,
};

struct GRAPHCODE_API FGcEditorCallOptions
{
	/** The function to call. Required. */
	FString FunctionName;

	/**
	 * Which live object: its name, an actor's outliner label, or its full object path.
	 *
	 * EMPTY IS A REQUEST TO CHOOSE, NEVER A REFUSAL. The strongest claim wins — on screen, then
	 * selected in the editor, then in the open level — and a tie is broken by path so the same
	 * object is chosen on every run. Whichever way it landed is said in `Warnings`, with the path
	 * to pass back to mean a different one. Only a preview scene or a running game is still
	 * refused, because neither is the place the caller was looking.
	 */
	FString InstanceName;

	/** `Damage=50,Knockback=true` — named, never positional, in the `.gc` value grammar. */
	FString Arguments;

	/** Report what would happen and call nothing. */
	bool bDryRun = false;

	/**
	 * How much of the console log the call brings back as evidence.
	 *
	 * SOMETHING by default, because a function that returns nothing leaves `it ran` as the whole
	 * answer otherwise, and the caller's next move would be to read the output log — a second call,
	 * for something this one was holding ([[a-write-finishes-itself]]).
	 *
	 * `Loud` rather than `All` by default because the two are not the same evidence. What a caller
	 * stepping a function twenty times needs from step nineteen is whether it complained; the
	 * twenty Print Strings it wrote on the way are the same twenty every step, and they are paid
	 * for every step. `All` is one word away for the step that needs them.
	 */
	EGcEditorCallLog Log = EGcEditorCallLog::Loud;
};

/** One DISTINCT line the call wrote to the log while it ran. */
struct GRAPHCODE_API FGcEditorCallLogLine
{
	FString Category;
	/** `Error`, `Warning`, `Display`, `Log`. */
	FString Verbosity;
	FString Message;

	/**
	 * How many times the call wrote this exact line — category, verbosity and text all equal.
	 *
	 * A LINE INSIDE A LOOP IS ONE FINDING, NOT FORTY. A function that prints the same complaint on
	 * every iteration is telling the caller one thing, and forty copies of it cost forty times what
	 * that thing is worth. The repeats are folded into the first occurrence and counted here, so
	 * what is lost is the INTERLEAVING with other lines, never a line and never a count.
	 *
	 * First-occurrence order is kept, so the sequence still reads as the story of the call. A caller
	 * who needs the true frame-by-frame interleaving of two alternating lines cannot get it from
	 * here — that is a trace, and a trace is `gc_play`'s.
	 */
	int32 Count = 1;
};

struct GRAPHCODE_API FGcEditorCallResult
{
	FString AssetPath;

	/** The class the document describes, resolved. */
	FString ClassPath;

	/** The object it was called on — empty when nothing was chosen. */
	FGcEditorCallInstance Instance;

	/**
	 * True when several instances had an EQUAL claim and the tie-break picked this one.
	 *
	 * The difference between "the only one on screen" and "the first of thirteen by path". Both are
	 * chosen rather than refused and both say so in `Warnings`; only this one leaves the caller a
	 * question, and it is what the tool layer attaches `Instances` to a SUCCESS for. A caller that
	 * had to make a second call to see the alternatives would be reading a later state of the
	 * editor than the one it just wrote to ([[a-write-finishes-itself]]).
	 */
	bool bInstanceAmbiguous = false;

	/**
	 * Every live instance, always, whether one was chosen or the choice was refused.
	 *
	 * ONE ENTRY CAN BE YOUNGER THAN THE CALL. An editor utility — an asset or actor action — has no
	 * live instance ever, so `Invoke` makes one the way the right-click menu does and lists it here
	 * with the rest. `Warnings` says so whenever that happened; nothing else distinguishes it, and
	 * nothing needs to, because its path addresses it for exactly as long as the call lasts.
	 *
	 * FILLED ALWAYS AND SERIALISED ONLY ON A REFUSAL. It is the answer to "which one did you mean",
	 * and it is 5–15k tokens of answer to a question a successful call did not ask
	 * ([[a-capped-list-must-be-ranked]] — a list nobody needs is not made cheaper by ranking it).
	 * The tool layer decides; this stays complete so the refusal it feeds is complete.
	 */
	TArray<FGcEditorCallInstance> Instances;

	/**
	 * What can be invoked here: every function this class declares, plus every one the details
	 * panel would offer as a button.
	 *
	 * DELIBERATELY NOT EVERY CALLABLE FUNCTION. A Blueprint actor inherits hundreds; a list of
	 * those is a truncation pretending to be an inventory. Any inherited BlueprintCallable function
	 * can still be invoked BY NAME, and a name that misses is answered with the closest matches out
	 * of the full set ([[a-capped-list-must-be-ranked]]).
	 */
	TArray<FGcEditorCallable> Callables;

	FString FunctionName;
	FString Signature;
	bool bCallInEditor = false;

	bool bInvoked = false;

	/** `Name=value` for every return and out parameter. Empty means it returned nothing. */
	FString Observed;

	TArray<FGcEditorCallLogLine> Log;
	int32 LogErrors = 0;
	int32 LogWarnings = 0;
	/**
	 * Lines below Warning the call wrote, counted under `Loud` as well as under `All`.
	 *
	 * COUNTED BEFORE THE MODE CAN DROP IT, so an empty `Log` under `Loud` reads as "it said nothing
	 * loud" and never as "it said nothing" ([[a-zero-from-a-producer-that-never-looked]]).
	 */
	int32 LogQuiet = 0;
	/** Lines the cap dropped. Said rather than silently truncated. */
	int32 LogDropped = 0;

	TArray<FString> Warnings;
};

class GRAPHCODE_API FGraphCodeEditorCall
{
public:
	/**
	 * Every live instance of the class `AssetPath` describes, in every world.
	 *
	 * Returns true when the path resolved to something that can HAVE instances, even when there are
	 * none — "nothing is open" is an answer, not a failure.
	 */
	static bool FindInstances(const FString& AssetPath, TArray<FGcEditorCallInstance>& OutInstances,
		FString& OutClassPath, TArray<FString>& OutErrors);

	/** What could be invoked on that class — the same list the result carries. */
	static bool FindCallables(const FString& AssetPath, TArray<FGcEditorCallable>& OutCallables,
		TArray<FString>& OutErrors);

	/**
	 * Resolve, gate, marshal, call, and report what came back.
	 *
	 * The gate, in order, each with its own sentence: the class resolves, an instance is chosen,
	 * the function exists on that instance's class, it is reachable the way a graph reaches it, it
	 * has a body, and every named argument imports. `bDryRun` stops after all of that.
	 *
	 * CHOSEN, OR MADE. Every ordinary class is chosen from what is live, and having nothing live is
	 * a refusal. An editor utility is the exception the engine itself carved: an asset action is
	 * never placed and never opened, so its object is built on the spot, used, and dropped — see
	 * `IsMadeOnDemand`. Only when no `InstanceName` was given, and only for that family.
	 */
	static bool Invoke(const FString& AssetPath, const FGcEditorCallOptions& Options,
		FGcEditorCallResult& OutResult, TArray<FString>& OutErrors);
};
