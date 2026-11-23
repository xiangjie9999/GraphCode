#pragma once

#include "CoreMinimal.h"
#include "AuthoringPropertyCodec.h"
#include "WidgetIRSearch.h"
#include "WidgetIRTypes.h"

namespace WidgetIR
{
	/** One on-demand search into a leaf's value set, for the sets too large to list inline. */
	struct WIDGETIR_API FWidgetIRSelectionRequest
	{
		/** `alias.Path`, or `alias.@slot.Path` for slot state. */
		FString Field;
		FString Query;
		int32 Limit = 20;
		int32 Offset = 0;
	};

	struct WIDGETIR_API FWidgetIRSelectionOption
	{
		FString Value;
		FString Label;
		FString Kind;
	};

	struct WIDGETIR_API FWidgetIRSelectionResult
	{
		FString Field;
		FString SelectionKind;
		FString SelectionType;
		FString Query;
		int32 TotalMatches = 0;
		int32 Offset = 0;
		int32 NextOffset = INDEX_NONE;
		bool bTruncated = false;
		TArray<FWidgetIRSelectionOption> Options;
		TArray<FString> Notes;
	};

	/**
	 * One multicast delegate this widget class offers the Designer's Events section - the "+"
	 * beside OnClicked, OnCheckStateChanged, OnValueChanged.
	 *
	 * FACTS ONLY, AND DELIBERATELY NO BLUEPRINT GRAMMAR. Binding one of these means placing a
	 * `UK2Node_ComponentBoundEvent` in a graph, and §12.1 gives graphs to BlueprintIR; WidgetIR
	 * answers the half it owns - what this widget class exposes - and the adapter that already
	 * depends on both languages turns it into a statement. Nothing here spells a node.
	 *
	 * Single-cast `FDelegateProperty` members are NOT listed. The Designer shows those in the
	 * same panel, but they are property bindings (`UWidgetBlueprint::Bindings`), a different
	 * mechanism that generates a function rather than an event node - §11.1's Phase 8 codec,
	 * still unbuilt. Listing them here would offer a write that does not exist.
	 */
	struct WIDGETIR_API FWidgetIRBindableEvent
	{
		/** The delegate property's own name - `OnClicked`. This is what a binding names. */
		FString Delegate;
		/** The class that DECLARES it, which for an inherited delegate is not the widget's own. */
		FString OwnerClassPath;
		/** The signature UFunction - `OnButtonClickedEvent__DelegateSignature`. */
		FString Signature;
		FString DisplayName;
		FString Tooltip;
		/** `Name: type` per parameter, in signature order. Empty for a parameterless delegate. */
		TArray<FString> Parameters;
	};

	/** What the engine actually produced for one hypothetical statement. */
	struct WIDGETIR_API FWidgetIRWidgetDescription
	{
		/** The name the statement used, which is how a caller addresses it in a selection request.
		 *  `_` stays `_` here: it is what was written, not what the engine chose. */
		FString StatementName;
		FWidgetIRClassInfo Class;
		/** The name the engine gave the widget, which is not always the one that was asked for. */
		FString Name;
		/**
		 * `variable=true` - whether this widget gets a generated member on the Blueprint class.
		 *
		 * Read off the built widget rather than the statement, because the builder is what
		 * decides. It is the prerequisite for every entry in BindableEvents: a handler names its
		 * widget through that member, and the Designer hides the "+" until it exists.
		 */
		bool bVariable = false;

		TArray<AuthoringCore::FAuthoringPropertyDescription> Properties;
		/** Every delegate this widget class offers an event handler for. Canonical name order. */
		TArray<FWidgetIRBindableEvent> BindableEvents;

		/**
		 * The slot class the PARENT produced, never one the payload named. Empty when this statement
		 * has no parent in the fragment and no prospective parent was given.
		 */
		FString SlotClass;
		TArray<AuthoringCore::FAuthoringPropertyDescription> SlotProperties;
		/** How this widget is held: root, children, content or a named slot. */
		FString Attachment;
		FString NamedSlot;

		TArray<FString> Notes;
	};

	struct WIDGETIR_API FWidgetIRDescribeOptions
	{
		/** Optional target asset, for the same context filtering ui_search applies. */
		FString AssetPath;
		/** Optional prospective parent for the top-level statements, by class token or path. */
		FString ParentClass;
		/** The named slot of that parent to attach into, instead of its child list. */
		FString ParentNamedSlot;

		int32 MaxStatements = 16;
		int32 MaxInlineOptions = 64;
		int32 MaxSelectionRequests = 16;

		TArray<FWidgetIRSelectionRequest> SelectionRequests;
	};

	struct WIDGETIR_API FWidgetIRFragmentDescription
	{
		/** The fragment as canonical text - what to paste into a ui_apply document. */
		FString NormalisedIr;
		TArray<FWidgetIRWidgetDescription> Widgets;
		TArray<FWidgetIRSelectionResult> SelectionResults;
		TArray<FAuthoringDiagnostic> Diagnostics;
		FWidgetIRParentContext Parent;
	};

	/**
	 * Build a hypothetical fragment in a throwaway Widget Tree and report what Unreal produced.
	 *
	 * Nothing is loaded for writing, nothing is compiled, nothing is dirtied and no asset is saved.
	 * The construction path is the one ui_apply uses, so a fragment that describes cleanly is a
	 * fragment apply can build - which is the whole point of asking first.
	 */
	WIDGETIR_API bool DescribeFragment(const FString& FragmentIr,
		const FWidgetIRDescribeOptions& Options, FWidgetIRFragmentDescription& OutDescription,
		TArray<FString>& OutErrors);
}
