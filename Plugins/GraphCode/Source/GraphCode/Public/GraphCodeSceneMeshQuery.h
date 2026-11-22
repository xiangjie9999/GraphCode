// Copyright OraStudio. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class UWorld;
class UStaticMeshComponent;
class UStaticMesh;
struct FGcScreenToWorldResult;

/** An explicit scene object. An omitted component/instance must resolve uniquely. */
struct GRAPHCODE_API FGcSceneMeshSelector
{
	FString Actor;
	FString Component;
	int32 Instance = INDEX_NONE;
};

/** One synchronous ask batch; never retained across editor changes. */
struct FGcSceneMeshQueryContext
{
	struct FGeometry
	{
		TArray<FVector> Vertices;
		TArray<uint32> Indices;
		FString Kind, Error;
		bool bValid = false;
	};
	TMap<UStaticMesh*, FGeometry> Meshes;
	UStaticMesh* RetainMesh = nullptr;
	FString Observation;
};

namespace GraphCodeSceneMeshQuery
{
	/** Geometry and exact placement identity, including collision-disabled and instanced meshes. */
	GRAPHCODE_API FString ObservationToken(UWorld* World, FGcSceneMeshQueryContext* Context = nullptr);
	GRAPHCODE_API bool Resolve(UWorld* World, const FGcSceneMeshSelector& Selector,
		UStaticMeshComponent*& OutComponent, FTransform& OutTransform, int32& OutInstance, FString& Error);
	/** Explicit mesh query; other scene objects do not occlude the selected mesh. */
	GRAPHCODE_API bool Query(UWorld* World, const FGcSceneMeshSelector& Selector,
		const FGcSceneMeshSelector& RelativeTo, const FString& Snap,
		const FVector& Origin, const FVector& Direction, double MaxDistance,
		FGcScreenToWorldResult& Out, FString& Error, FGcSceneMeshQueryContext* Context = nullptr);
}
