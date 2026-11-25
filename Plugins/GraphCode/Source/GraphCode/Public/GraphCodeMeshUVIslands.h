// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"

namespace UE::Geometry { class FDynamicMesh3; }

/** UV connectivity shared by inspection and Shape. IDs are scoped to this mesh revision. */
namespace GraphCodeMeshUVIslands
{
struct FTriangle
{
    int32 Id = INDEX_NONE;
    int32 Island = INDEX_NONE;
    FVector2D UV[3];
    FBox2D Bounds = FBox2D(ForceInit);
    bool bPickable = false;
};
struct FIsland
{
    TArray<int32> Triangles;
    FBox2D Bounds = FBox2D(ForceInit);
    FVector2D Seed = FVector2D::ZeroVector;
    double Area = 0;
};
struct FAnalysis
{
	struct FNode
	{
		FBox2D Bounds = FBox2D(ForceInit);
		int32 Start = 0, Count = 0, Left = INDEX_NONE, Right = INDEX_NONE;
	};
    int32 Channel = 0;
    int32 Unassigned = 0;
    int32 Degenerate = 0;
    FString Signature;
    FBox2D Bounds = FBox2D(ForceInit);
    TArray<FTriangle> Triangles;
    TArray<int32> TriangleToIsland;
    TArray<FIsland> Islands;
	TArray<int32> PickOrder;
	TArray<FNode> PickTree;
};

GRAPHCODE_API bool Analyze(const UE::Geometry::FDynamicMesh3& Mesh, int32 Channel,
    FAnalysis& Out, FString& Error);
/** All islands under this UV point. Scope disambiguates stacked charts without welding them. */
GRAPHCODE_API TArray<int32> Find(const FAnalysis& Analysis, const FVector2D& UV,
    const TSet<int32>* Scope = nullptr);
GRAPHCODE_API bool Barycentric(const FVector2D& P, const FVector2D& A,
    const FVector2D& B, const FVector2D& C, FVector& Out);
GRAPHCODE_API FString Selector(const FVector2D& UV, int32 Channel);
}
