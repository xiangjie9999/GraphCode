#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

class FProperty;

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
}
