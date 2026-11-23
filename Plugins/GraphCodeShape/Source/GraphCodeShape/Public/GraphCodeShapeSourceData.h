// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetUserData.h"

#include "GraphCodeShapeSourceData.generated.h"

/** Persistence quality for one rendered triangle's link back to an evaluated source surface. */
UENUM(BlueprintType)
enum class EGraphCodeShapeSourceMappingQuality : uint8
{
	None,
	Exact,
	Derived,
	Approximate,
	Ambiguous,
};

/** The recipe output an evaluated surface ultimately came from. */
USTRUCT(BlueprintType)
struct GRAPHCODESHAPE_API FGraphCodeShapeSourceDefinitionData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FString Locator;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FString Entry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FString BuildKey;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	TMap<FString, FString> Parameters;
};

/** One recipe use in an assembly. InstanceId is private to this asset build. */
USTRUCT(BlueprintType)
struct GRAPHCODESHAPE_API FGraphCodeShapeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	int32 InstanceId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FString RelativePath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	bool bAddressable = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FGraphCodeShapeSourceDefinitionData Source;

	/** Ordered rather than flattened: rotated non-uniform scales may not compose without shear. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	TArray<FTransform> PlacementChain;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	bool bReflected = false;
};

/** Source correspondence for one triangle in the asset's rendered LOD0 index order. */
USTRUCT(BlueprintType)
struct GRAPHCODESHAPE_API FGraphCodeShapeTriangleSourceData
{
	GENERATED_BODY()

	/** Unique owner when exact/derived/approximate; all candidates when geometry is coincident. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	TArray<int32> CandidateInstanceIds;

	/** Source corners aligned to the rendered triangle's three index-buffer corners. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FVector SourceA = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FVector SourceB = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FVector SourceC = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	EGraphCodeShapeSourceMappingQuality Quality = EGraphCodeShapeSourceMappingQuality::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	int32 ProducerLine = 0;

	/** Largest rendered-to-evaluated corner residual, in centimetres. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	double ErrorCm = 0.0;
};

/**
 * Shape-owned editor metadata saved on generated static meshes.
 *
 * Triangles are keyed by rendered LOD0 order, never evaluator IDs. The data is editor-only: cheap
 * provenance digests remain on the asset in cooked builds, while authoring correspondence does not.
 */
UCLASS(BlueprintType)
class GRAPHCODESHAPE_API UGraphCodeShapeSourceData : public UAssetUserData
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentFormatVersion = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	int32 FormatVersion = CurrentFormatVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	FString GeometryDigest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	int32 RootInstanceId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GraphCode|Shape")
	TArray<FGraphCodeShapeInstanceData> Instances;

	// Bulk correspondence is queried through the source provider. Exposing every
	// triangle as Details widgets can allocate gigabytes when a mesh editor opens.
	UPROPERTY(BlueprintReadOnly, Category="GraphCode|Shape")
	TArray<FGraphCodeShapeTriangleSourceData> Triangles;

	const FGraphCodeShapeInstanceData* FindInstance(int32 InstanceId) const;
	virtual bool IsEditorOnly() const override { return true; }
};
