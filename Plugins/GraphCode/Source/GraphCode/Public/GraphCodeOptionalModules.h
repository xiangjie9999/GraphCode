// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Internal native adapters are loaded only when their engine plugins are enabled. */
class GRAPHCODE_API FGraphCodeOptionalModules
{
public:
	static bool Load(FName ModuleName, const TArray<FString>& RequiredPlugins);
	static FString UnavailableReason(FName ModuleName);

	/**
	 * For a module that delay-loads its framework instead of splitting into an adapter
	 * (Docs/GC-OPTIONAL-FRAMEWORKS-PLAN.md): true when every plugin is enabled; otherwise OutReason gets the
	 * same sentence an adapter's refusal gives. Call before the first use of a delay-loaded framework.
	 */
	static bool PluginsEnabled(const TArray<FString>& RequiredPlugins, FString& OutReason);
};
