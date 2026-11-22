#pragma once

#include "CoreMinimal.h"

#include "AuthoringCoreTypes.h"
#include "WidgetIRTypes.h"

class UBlueprintExtension;
class UWidgetBlueprint;

/**
 * A framework-owned codec that contributes a nested zone to WidgetIR.
 *
 * WidgetIR owns the universal widget document and its stable text grammar. Optional framework
 * plugins own the engine objects behind individual zones. Registering through this seam keeps the
 * base module readable when a framework is absent and prevents it from linking framework modules.
 */
struct WIDGETIR_API FWidgetIRExtensionCodec
{
	/** Stable lower-case zone name, for example `mvvm`. */
	FName Name;

	/** Whether the backing framework is linked and writable in this build. */
	TFunction<bool()> IsAvailable;

	/** Whether an asset extension belongs to this codec. */
	TFunction<bool(const UBlueprintExtension*)> OwnsExtension;

	/** Project this codec's state into the neutral WidgetIR document. */
	TFunction<void(const UWidgetBlueprint*, FWidgetIRDocument&,
		TArray<FAuthoringDiagnostic>&)> Read;

	/** Apply this codec's part of the neutral document to the asset. */
	TFunction<bool(UWidgetBlueprint*, const FWidgetIRDocument&, TArray<FString>&)> Apply;
};

/** Process-wide registry populated by optional product modules. */
class WIDGETIR_API FWidgetIRExtensionCodecs
{
public:
	static void Register(FWidgetIRExtensionCodec Codec);
	static void Unregister(FName Name);

	static bool IsRegistered(FName Name);
	static bool IsAvailable(FName Name);
	static bool OwnsExtension(const UBlueprintExtension* Extension);
	static void ReadAll(const UWidgetBlueprint* Blueprint, FWidgetIRDocument& OutDocument,
		TArray<FAuthoringDiagnostic>& Diagnostics);
	static bool Apply(FName Name, UWidgetBlueprint* Blueprint, const FWidgetIRDocument& Desired,
		TArray<FString>& OutErrors);
};
