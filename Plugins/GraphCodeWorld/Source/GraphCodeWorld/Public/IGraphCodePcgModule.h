// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
class UWorld;
class AActor;
struct FGcPopulateEntry;
struct FGcwGeneratedVolume;

/** Native PCG access without importing PCG into the rest of World. */
class IGraphCodePcgModule : public IModuleInterface
{
public:
	virtual bool ValidateGrid(UWorld* World, const FGcPopulateEntry& Entry, FString& Error) = 0;
	virtual bool ConfigureVolume(const FGcPopulateEntry& Entry, FGcwGeneratedVolume& Volume, FString& Error) = 0;
	virtual bool ReadVolume(const AActor& Actor, FGcPopulateEntry& State) = 0;
};
