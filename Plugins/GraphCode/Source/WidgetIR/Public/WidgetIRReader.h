#pragma once

#include "CoreMinimal.h"
#include "WidgetIRTypes.h"

struct FDelegateEditorBinding;
class UWidgetBlueprint;

namespace WidgetIR
{
	/**
	 * The source member chain one engine binding names, guid-resolved, or empty if it names none.
	 *
	 * Public because the write path needs the SAME answer the read path gives: an apply compares a
	 * desired binding against the existing struct through this function and leaves the struct
	 * untouched when they agree. Two spellings of "what does this binding point at" would let a
	 * no-op apply silently rewrite a legacy binding into the modern property-path form.
	 */
	WIDGETIR_API TArray<FWidgetIRBindingMember> BindingSource(const FDelegateEditorBinding& Binding);

	/** Read one Widget Blueprint's owned Designer tree. Never descends into nested UserWidget trees. */
	WIDGETIR_API bool ReadWidgetBlueprint(const FString& AssetPath, const FWidgetIRReadOptions& Options,
		FWidgetIRDocument& OutDocument, FString& OutError);

	/**
	 * The same read against an already-resolved asset. A scratch duplicate lives in a transient
	 * package and has no asset path to load, so readback equivalence needs this overload.
	 */
	WIDGETIR_API bool ReadWidgetBlueprintObject(UWidgetBlueprint* Blueprint,
		const FWidgetIRReadOptions& Options, FWidgetIRDocument& OutDocument, FString& OutError);
}
