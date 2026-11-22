// Blueprint IR - read-only detailed description of hypothetical node fragments.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintIRTypes.h"

class UBlueprint;
class UEdGraph;

struct BLUEPRINTIR_API FBpDescribeOptionHint
{
	FString Field;
	FString SelectionKind;
	FString ExpectedType;
	FString QueryTool;
	FString QueryKind;
	TArray<FString> Options;
	int32 TotalAvailable = INDEX_NONE;
	int32 Returned = 0;
	bool bTotalIsExact = false;
	bool bTruncated = false;
};

struct BLUEPRINTIR_API FBpDescribeSelectionRequest
{
	FString Field;
	FString Query;
	int32 Limit = 20;
	int32 Offset = 0;
};

struct BLUEPRINTIR_API FBpDescribeSelectionOption
{
	FString Label;
	FString Value;
	FString Canonical;
	FString Kind;
	FString Signature;
	bool bCompatible = true;
};

struct BLUEPRINTIR_API FBpDescribeSelectionResult
{
	FString Field;
	FString SelectionKind;
	FString Query;
	int32 TotalAvailable = INDEX_NONE;
	int32 TotalMatches = 0;
	int32 Returned = 0;
	int32 Offset = 0;
	int32 NextOffset = INDEX_NONE;
	bool bTotalIsExact = false;
	bool bTruncated = false;
	TArray<FBpDescribeSelectionOption> Options;
	TArray<FString> Notes;
};

struct BLUEPRINTIR_API FBpDescribePin
{
	FString Name;
	FString DisplayName;
	FString Direction;
	FString Role;
	FString Type;
	FString Category;
	FString SubCategoryObject;
	FString Container;

	bool bReference = false;
	bool bConst = false;
	bool bHidden = false;
	bool bAdvanced = false;
	bool bOrphaned = false;
	bool bDefaultIgnored = false;
	bool bNotConnectable = false;
	bool bAcceptsConnection = true;
	bool bDefaultValueVisible = false;
	bool bDefaultValueEditable = false;
	bool bAssetPicker = false;

	FString DefaultValue;
	FString DefaultObject;
	FString DefaultText;
	FString DefaultTextNamespace;
	FString DefaultTextKey;
};

struct BLUEPRINTIR_API FBpNodeDescription
{
	FString Alias;
	FString Op;
	FString NodeClass;
	FString Title;
	FString Tooltip;
	FString Category;
	FString Target;
	TArray<TPair<FString, FString>> Properties;
	TArray<FBpDescribePin> Pins;
	TArray<FBpDescribeOptionHint> OptionHints;
	TArray<FString> AddedPins;
	TArray<FString> RemovedPins;
	TArray<FString> Notes;
};

struct BLUEPRINTIR_API FBpFragmentDescription
{
	FString BlueprintPath;
	FString GraphName;
	FBpIrGraph::EKind GraphKind = FBpIrGraph::EKind::EventGraph;
	FString NormalizedIr;
	TArray<FString> Substitutions;
	TArray<FBpNodeDescription> Nodes;
	TArray<FBpDescribeSelectionResult> SelectionResults;
};

struct BLUEPRINTIR_API FBpDescribeOptions
{
	TArray<FString> DescribeAliases;
	TArray<FBpDescribeSelectionRequest> SelectionRequests;
	bool bIncludeOptionHints = true;
	int32 MaxStatements = 32;
	int32 MaxInlineOptions = 32;
	int32 MaxSelectionRequests = 16;
	int32 MaxSelectionResults = 100;
};

namespace BlueprintIR
{
	/**
	 * Construct, settle and connect a bounded IR fragment in a registered scratch graph,
	 * then report the real nodes Unreal produced. The Blueprint and context graph are never
	 * modified, compiled, dirtied, notified, laid out or saved.
	 */
	BLUEPRINTIR_API bool DescribeFragment(
		UBlueprint* Blueprint,
		UEdGraph* ContextGraph,
		const FBpIrGraph& Ir,
		const FBpDescribeOptions& Options,
		FBpFragmentDescription& OutDescription,
		TArray<FString>& OutErrors);
}
