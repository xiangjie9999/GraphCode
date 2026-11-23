#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class FCompilerResultsLog;

/**
 * §11.1c. ONE COMPILE PER ASSET FOR A BATCH, instead of one per block write.
 *
 * User-directed 2026-08-08, alongside §11.1b: a batch of applies should not pay a compile each.
 *
 * **THE COST IS MEASURED, NOT ESTIMATED.** A graph write compiles inside `BlueprintIRApply` per
 * write, and five separate GraphCode block writers each mark the Blueprint structurally modified —
 * so a five-block document against one Blueprint compiles up to five times, and a batch of
 * documents pays that again per document.
 *
 * **"ONE COMPILE" CAN ONLY MEAN ONE PER ASSET.** A compile regenerates one class; a batch spanning
 * four Blueprints is four compiles and no design makes it one. Saying so plainly is cheaper than a
 * row that claims something it cannot do.
 *
 * ---
 *
 * **THE HAZARD THE ROW WAS OPENED FOR TURNS OUT TO BE SPLIT ALREADY, and that is the finding.**
 * §11.1c named a dependency nobody had measured: `FBlueprintEditorUtils::AddMemberVariable` appends
 * to `NewVariables`, and the name resolves only once the skeleton class carries it — so a document
 * where block A adds a variable and block B's graph reads it would refuse if the compile between
 * them disappeared. The row guessed the rule would be *defer the full compile, keep the skeleton
 * refresh eager*, and guessed right for a reason it did not state: **those are already two different
 * calls.** `MarkBlueprintAsStructurallyModified` IS the skeleton regeneration and
 * `FKismetEditorUtilities::CompileBlueprint` is the full compile. Deferring only the second leaves
 * every dependent writer with exactly what it needed.
 *
 * So this class touches the compile and never the skeleton mark. A writer that added a variable has
 * still regenerated the skeleton by the time the next block runs, whether or not a scope is open —
 * and `BlueprintIRDeferredCompileTests` drives that document to prove it rather than assert it from
 * this paragraph.
 *
 * ---
 *
 * **NOT PURE RAII, deliberately.** The obvious shape hands each writer's `FCompilerResultsLog*` to
 * the scope and fans results back on destruction — which stores pointers to logs that may not
 * outlive the scope. Instead the scope OWNS the compile it defers and the caller reads the outcome
 * from `Flush`, so there is nothing to dangle. The destructor still flushes, discarding the
 * messages, so an early return cannot leave a Blueprint marked structurally modified and never
 * compiled.
 *
 * **WIDGETIR IS NOT COVERED AND DOES NOT NEED TO BE.** `WidgetIRApply` compiles once per apply
 * rather than per block — a Designer document is one whole-tree write — so the five-compiles
 * problem is Blueprint-specific. It also could not share this without a dependency it does not
 * have: WidgetIR and BlueprintIR are siblings, and their only common module, `AuthoringCore`, is
 * deliberately editor-free (`Core`/`CoreUObject`/`Engine`/`Json`) and must stay that way for the
 * eventual Runtime retype.
 */
class BLUEPRINTIR_API FBpDeferredCompileScope
{
public:
	FBpDeferredCompileScope();
	~FBpDeferredCompileScope();

	FBpDeferredCompileScope(const FBpDeferredCompileScope&) = delete;
	FBpDeferredCompileScope& operator=(const FBpDeferredCompileScope&) = delete;

	/** True while any scope is open, so a writer can say it deferred rather than say nothing. */
	static bool IsDeferring();

	/**
	 * Compile this Blueprint, or record that it needs one.
	 *
	 * With no scope open this compiles immediately into `OutLog` — today's behaviour exactly, which
	 * is what keeps every existing caller and every existing spec unchanged. With a scope open it
	 * records the Blueprint, leaves `OutLog` untouched and returns false, and the compile happens
	 * once at `Flush`.
	 */
	static bool RequestCompile(UBlueprint* Blueprint, FCompilerResultsLog& OutLog);

	/**
	 * Compile each unique Blueprint that asked, once, and hand back what the compiler said.
	 *
	 * Idempotent: a second call with nothing pending does nothing. Returns how many compiles it
	 * actually ran, which is what a spec asserts against — a five-block document returning 1 is the
	 * row's whole claim.
	 */
	int32 Flush(TArray<FString>& OutErrors, TArray<FString>& OutWarnings);

	/**
	 * How many compiles this process has run through here, ever.
	 *
	 * The row's claim — *a five-block document compiles once* — is otherwise unmeasurable from
	 * outside: `ApplyDocument` owns its scope, so a spec cannot read that scope's return value. A
	 * before/after delta around a real apply is the only honest way to assert it, and a claim about
	 * a compile count that nothing counts is the kind this arc keeps finding written down and never
	 * run.
	 */
	static int32 TotalCompilesPerformed();

	/**
	 * K4: whether a compile asked for now would run inside another one. `RequestCompile` and
	 * `Flush` refuse such a compile with a sentence instead of handing it to the engine; any other
	 * caller of `FKismetEditorUtilities::CompileBlueprint` should ask this first.
	 */
	static bool IsBeingCompiled(const UBlueprint* Blueprint);

private:
	/** Only the outermost scope defers and flushes; an inner one is a no-op it can nest inside. */
	bool bOutermost = false;
};
