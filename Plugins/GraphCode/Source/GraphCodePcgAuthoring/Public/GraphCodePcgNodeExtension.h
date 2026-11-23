#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

class FProperty;
class UScriptStruct;

// Optional native node families supply only the state ordinary reflection cannot carry.
struct GRAPHCODEPCGAUTHORING_API FPcgNodeAuthoringExtension
{
	UClass* SettingsBase = nullptr;
	TSet<FString> BodyNames;
	TFunction<bool(const UObject*, const FProperty*)> IsAuthoredField;
	TFunction<bool(const FProperty*)> IsBodyField;
	TFunction<void(const UObject*, const UObject*, int32, TArray<FString>&, TArray<FAuthoringDiagnostic>&)> AppendBodies;
	TFunction<bool(UObject*, const FString&, const TArray<FString>&, TArray<FString>&)> ApplyBody;
	TFunction<void(UObject*)> AfterWrite;
};

namespace GraphCodeWorld
{
	GRAPHCODEPCGAUTHORING_API void RegisterPcgNodeExtension(FName Name, FPcgNodeAuthoringExtension Extension);
	GRAPHCODEPCGAUTHORING_API void UnregisterPcgNodeExtension(FName Name);
	GRAPHCODEPCGAUTHORING_API const FPcgNodeAuthoringExtension* PcgNodeExtension(const UObject* Owner);
	GRAPHCODEPCGAUTHORING_API bool PcgIsAuthoredField(const UObject* Owner, const FProperty* Property);
	GRAPHCODEPCGAUTHORING_API bool PcgIsCustomBodyField(const FProperty* Property);
	/**
	 * A struct with fields but NONE an author edits field by field (FPCGAttributePropertySelector: its
	 * fields are UPROPERTY but not EditAnywhere, a details customisation edits them). A series entry of
	 * one travels as the struct's OWN text form, read and written whole (23 Nov: an
	 * InstanceDataPackerByAttribute's AttributeSelectors read back as `()` and refused every write).
	 */
	GRAPHCODEPCGAUTHORING_API bool PcgIsOpaqueStruct(const UScriptStruct* Struct, const UObject* Owner);
	/** `(PCGBegin(Dry)PCGEnd)` -> `Dry`: true when `Native` is a PCG attribute selector's own export. */
	GRAPHCODEPCGAUTHORING_API bool PcgSelectorTextOf(const FString& Native, FString& OutSelector);
	/** `("Dry")` -> `(PCGBegin(Dry)PCGEnd)`; any other entry is returned unchanged. */
	GRAPHCODEPCGAUTHORING_API FString PcgSelectorNativeText(const FString& Entry);
}
