// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

namespace GraphCodeOperation
{
	/** Scoped cancellation supplied by the caller. Checkpoints belong at safe algorithm boundaries. */
	class GRAPHCODE_API FScope
	{
	public:
		explicit FScope(TFunction<bool()> InPoll);
		~FScope();
		FScope(const FScope&) = delete;
		FScope& operator=(const FScope&) = delete;
		bool Poll();
	private:
		FScope* Previous = nullptr;
		TFunction<bool()> Callback;
		double NextPoll = 0;
		bool bCancelled = false;
	};
	GRAPHCODE_API bool IsCancelled();
	/** True while a checkpoint services control requests; nested editor mutations must not run. */
	GRAPHCODE_API bool IsPolling();
	GRAPHCODE_API bool Checkpoint(FString& Error);
}
