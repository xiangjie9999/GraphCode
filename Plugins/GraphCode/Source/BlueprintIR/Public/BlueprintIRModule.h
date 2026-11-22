#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Blueprint IR core module.
 *
 * Owns the resolver's lifetime and its invalidation hooks. Knows nothing about
 * MCP, HTTP, or any bridge - tool handlers live in an adapter elsewhere and call
 * into the public headers here.
 */
class FBlueprintIRModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FBlueprintIRModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FBlueprintIRModule>(TEXT("BlueprintIR"));
	}

	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded(TEXT("BlueprintIR"));
	}
};
