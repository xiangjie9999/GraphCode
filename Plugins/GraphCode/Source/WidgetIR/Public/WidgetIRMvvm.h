#pragma once

#include "CoreMinimal.h"

#include "AuthoringCoreTypes.h"
#include "WidgetIRTypes.h"

/**
 * The stable MVVM document model and text grammar — `UI-AUTHORING-DESIGN.md` §11.1a.
 *
 * WidgetIR owns these framework-neutral declarations because a committed `.gc` file must parse
 * whether or not the optional framework is installed. GraphCode Core Extension owns the engine
 * adapter and reaches this model through the extension-codec registry.
 */
namespace WidgetIR
{
	/**
	 * MATCH, DO NOT MINT - the answer to the question section 11.1b left open, and it is forced by a
	 * measurement rather than chosen. `ConversionFunctionHelper::CreateWrapperName` builds a wrapper
	 * graph's NAME out of `Binding.GetFName()`, which is `BindingId`, so a binding's guid is cited
	 * from OUTSIDE the struct by a `UEdGraph` sitting in `Blueprint->FunctionGraphs`. Re-minting on
	 * every write would leave that graph named after an identity nothing holds any more - and a
	 * conversion function is precisely what this document does not project, so it is exactly the
	 * state a write must not disturb. A viewmodel's `ViewModelContextId` is cited the same way by
	 * every binding's `ContextId` and by the events and conditions this document also omits.
	 *
	 * So an existing entity keeps its identity and only a genuinely new line gets a fresh one. A
	 * viewmodel is matched by NAME. A binding is matched by its two ENDS - `<destination, source>`,
	 * which is what the engine's own `GetDisplayNameString` prints - and never by mode, execution,
	 * enabled or compile: those four are exactly the fields an edit changes, and keying on them
	 * would lower "change a mode" to remove-plus-add, which is the re-mint this rule exists to stop.
	 * Two bindings may share both ends; the tie is broken by ORDINAL, the Nth line of a pair going to
	 * the Nth binding of that pair, which is why `MvvmBindingLess` had to become a total order.
	 *
	 * The Core Extension adapter applies this match rule after the widget tree exists.
	 */
	/** `<Destination> = <Source>` - the two ends, and the writer's match key. */
	WIDGETIR_API FString MvvmBindingPairKey(const FWidgetIRMvvmBinding& Entry);

	/** What admitting an `mvvm { }` payload decided, in the three lists a result reports it through. */
	struct WIDGETIR_API FMvvmWritePlan
	{
		/** Refusals. A non-empty list means the payload is not written at all. */
		TArray<FString> Errors;

		/** `viewmodel <Name>` / `bind <Dest> = <Source>` for what snapshot removes - `OmittedDeletions`. */
		TArray<FString> Removals;

		/** Removals that take unprojected state with them - `BrokenReferences`. */
		TArray<FString> Losses;
	};

	/**
	 * Decide whether a document's view may be written over the asset's, before anything is touched.
	 *
	 * SEPARATE FROM `ApplyMvvm` AND OUTSIDE THE AVAILABILITY GUARD, because it is a comparison of two
	 * documents and names no engine type - so it is the half a spec can run, and it runs the same in
	 * a build without the plugin, which is the build that has to refuse a payload carrying a view.
	 *
	 * `Renamed` is old widget name -> new, so a payload that renames a bound widget states its paths
	 * under the NEW name exactly as the binding and navigation families already allow.
	 */
	WIDGETIR_API void PlanMvvmWrite(const FWidgetIRDocument& Base, const FWidgetIRDocument& Desired,
		const TSet<FString>& DesiredWidgetNames, const TMap<FString, FString>& Renamed,
		bool bSnapshot, bool bAllowBreakingChanges, FMvvmWritePlan& OutPlan);

	/**
	 * Case-sensitive canonical order for `MvvmBindings`: destination, source, then every specifier.
	 *
	 * TOTAL, AND IT HAD TO BECOME SO FOR THE WRITER. Ordering on the pair alone left two bindings
	 * that share both ends tied, and `TArray::Sort` is an introsort - it does not preserve the
	 * engine's order among equal elements. That was invisible while the block was read-only and is
	 * load-bearing now: the writer maps the Nth line of a pair to the Nth binding of that pair, so a
	 * document order the content does not fully determine would map an edited mode onto the wrong
	 * binding. Two lines identical in every field remain tied, which is correct - the document
	 * cannot tell them apart either, and neither can any edit of it.
	 */
	WIDGETIR_API bool MvvmBindingLess(const FWidgetIRMvvmBinding& A, const FWidgetIRMvvmBinding& B);

	/**
	 * THE TEXT HALF, AND IT IS DELIBERATELY OUTSIDE THE AVAILABILITY GUARD.
	 *
	 * Everything below moves between `FWidgetIRMvvm*` structs and `FString` and never names an engine
	 * MVVM type, so it compiles and runs without ModelViewViewModel. That is not an
	 * accident of layering: a `.gc` committed from a project WITH the plugin has an `mvvm { }` block
	 * in it, and a build without the plugin still has to READ that file. Guarding the parser would
	 * make the same committed sidecar a syntax error in one build and a document in another, which is
	 * a worse failure than the absent codec it would be protecting.
	 *
	 * The emit and the parse ship together for the reason row 11k paid for: a block that renders and
	 * cannot be read back breaks the round trip for every widget sidecar, not only the ones with a
	 * view.
	 */

	/** `viewmodel <Name> = <ClassPath> [creation=…, …]`, no indent, no keyword-less form. */
	WIDGETIR_API FString RenderMvvmViewModel(const FWidgetIRMvvmViewModel& Entry);

	/** The inverse, from the text AFTER the `viewmodel` keyword. */
	WIDGETIR_API bool ParseMvvmViewModel(const FString& Text, FWidgetIRMvvmViewModel& Out,
		FString& OutError);

	/** `bind <Destination> = <Source> [mode=…, …]`, no indent. */
	WIDGETIR_API FString RenderMvvmBinding(const FWidgetIRMvvmBinding& Entry);

	/**
	 * The inverse, from the text AFTER the `bind` keyword.
	 *
	 * `ViewModels` is what makes a root readable: `Account.DisplayName` and `TitleText.Text` are the
	 * same shape, and only the block's own declarations say which is which. That is the
	 * names-resolve-inside-the-file rule doing real work rather than being restated — the engine
	 * stores the viewmodel end as a guid, and a file printing the guid would name something no
	 * reader could resolve.
	 */
	WIDGETIR_API bool ParseMvvmBinding(const FString& Text,
		const TArray<FWidgetIRMvvmViewModel>& ViewModels,
		FWidgetIRMvvmBinding& Out, FString& OutError);
}
