#pragma once

#include "CoreMinimal.h"
#include "GraphCodeLayout.h"
class UPCGGraph;

namespace GraphCodePcg
{
	/** Project native data pins into BPIR's organiser and copy only positions back. Never saves. */
	GRAPHCODEPCGAUTHORING_API bool Organise(UPCGGraph* Graph, bool bDryRun,
		FGraphCodeLayoutResult& Result, FString& Error);
}
