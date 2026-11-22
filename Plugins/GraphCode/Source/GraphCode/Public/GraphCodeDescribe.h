#pragma once

#include "CoreMinimal.h"

/** One reflected leaf in a GraphCode describe response. */
struct GRAPHCODE_API FGraphCodePropertyDescription
{
	FString Path;
	FString Type;
	FString Value;
	FString Default;
	FString Category;
	FString Description;
	FString SelectionKind;
	FString SelectionType;
	TArray<FString> Options;
	bool bAuthored = false;
	bool bEditable = true;
	bool bAdvanced = false;
};

/** One authored object described through a GraphCode facet. */
struct GRAPHCODE_API FGraphCodeObjectDescription
{
	FString Handle;
	FString ClassPath;
	FString BaselinePath;
	FString ParentHandle;
	TArray<FGraphCodePropertyDescription> Properties;
	TArray<FGraphCodePropertyDescription> RelationshipProperties;
	TArray<FString> Notes;
};

struct GRAPHCODE_API FGraphCodeDescribeResult
{
	FString AssetPath;
	FString Kind;
	FString Block;
	/** "covered" or "omitted" in the current sidecar contract. */
	FString Coverage;
	/** "writable", "projection_only", or "not_projected". */
	FString WriteStatus;
	TArray<FGraphCodeObjectDescription> Objects;
	TArray<FString> Diagnostics;
};

/**
 * Reflection-backed discovery for GraphCode facets that do not yet have their own IR describe.
 *
 * This is intentionally read-only. In particular, describing an SCS component does not imply
 * that the current gc 1 document projects or writes a components block: Coverage and WriteStatus
 * say that boundary explicitly so a default-valued leaf can never be confused with an omitted
 * facet.
 */
class GRAPHCODE_API FGraphCodeDescribe
{
public:
	static bool DescribeComponentTemplates(const FString& AssetPath, const FString& Handle,
		int32 MaxInlineOptions, FGraphCodeDescribeResult& OutResult, FString& OutError);

	static bool DescribeClassDefaults(const FString& AssetPath, int32 MaxInlineOptions,
		FGraphCodeDescribeResult& OutResult, FString& OutError);
};
