// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * The opaque context the caller of the call now running brought with it, for this thread: whatever the
 * tool layer received beside the call (the bridge's `X-MCP-Context`). Set around each call by the tool
 * layer and read wherever a call's own context matters. Empty when the call came from inside the editor,
 * a commandlet or a test. Nesting restores the previous value.
 */
class GRAPHCODE_API FGraphCodeCallerContext
{
public:
	explicit FGraphCodeCallerContext(const FString& Context);
	~FGraphCodeCallerContext();

	FGraphCodeCallerContext(const FGraphCodeCallerContext&) = delete;
	FGraphCodeCallerContext& operator=(const FGraphCodeCallerContext&) = delete;

	static const FString& Current();

private:
	FString Previous;
};
