// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UObject;

/** Product-neutral fidelity of a rendered surface's link to authoring space. */
enum class EGcMeshSourceMappingQuality : uint8
{
	None,
	Exact,
	Derived,
	Approximate,
	Ambiguous,
};

/** One possible source instance for a rendered surface point. */
struct GRAPHCODE_API FGcMeshSourceCandidate
{
	int32 InstanceId = INDEX_NONE;
	FString InstancePath;
	bool bAddressable = false;
	bool bReflected = false;

	FString DefinitionLocator;
	FString DefinitionEntry;
	FString BuildKey;
	TMap<FString, FString> Parameters;
	TArray<FTransform> PlacementChain;

	/** The evaluated point with this candidate's outer placements removed in reverse order. */
	FVector CurrentPartPoint = FVector::ZeroVector;
};

/** Optional authoring answer for one barycentric point on one rendered LOD0 triangle. */
struct GRAPHCODE_API FGcMeshSourceResult
{
	EGcMeshSourceMappingQuality Quality = EGcMeshSourceMappingQuality::None;
	int32 ProducerLine = 0;
	double ErrorCm = 0.0;

	FVector CurrentMeshPoint = FVector::ZeroVector;
	bool bHasSourcePoint = false;
	FVector SourcePoint = FVector::ZeroVector;

	/** One entry for a unique mapping; several where coincident/generated geometry is ambiguous. */
	TArray<FGcMeshSourceCandidate> Candidates;
};

using FGcMeshSourceProvider = TDelegate<bool(
	const UObject* /*Asset*/,
	int32 /*RenderedLod0Triangle*/,
	const FVector& /*Barycentric*/,
	const FVector& /*CurrentMeshPoint*/,
	FGcMeshSourceResult& /*Out*/)>;

using FGcMeshSourceTokenProvider = TDelegate<FString(const UObject* /*Asset*/)>;

/**
 * Read-only product seam for optional mesh authoring metadata.
 *
 * Core owns the question because Render asks it, but it never includes a Shape type. With no
 * provider, or for a mesh with no product metadata, Query simply returns false and the ordinary
 * mesh-local hit remains complete. The provider token joins the mesh observation token so an old
 * capture cannot query changed source/instance data against unchanged triangles.
 */
class GRAPHCODE_API FGraphCodeMeshSource
{
public:
	static bool Register(FName Owner, FGcMeshSourceProvider Provider,
		FGcMeshSourceTokenProvider TokenProvider, FString& OutError);
	static void Unregister(FName Owner);

	static bool Query(const UObject* Asset, int32 RenderedLod0Triangle,
		const FVector& Barycentric, const FVector& CurrentMeshPoint, FGcMeshSourceResult& Out);
	static FString ObservationToken(const UObject* Asset);
	static bool IsAvailable();
};
