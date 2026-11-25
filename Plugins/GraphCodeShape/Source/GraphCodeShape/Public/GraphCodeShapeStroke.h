// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GraphCodeShapeValue.h"
#include "Polygon2.h"

struct FGcShapeDocument;
struct FGcShapeInstruction;
struct FGcShapeHeader;
class IGcShapeVolumes;
namespace UE::Geometry { class FDynamicMesh3; }

/** A continuous projected footprint. Profiles read projected arc length, never stamp count. */
struct GRAPHCODESHAPE_API FGcShapeStroke
{
    TArray<FVector2d> Path;
    /** A directed three-dimensional ruler. Empty on the established projected-brush kind. */
    TArray<FVector3d> SpatialPath;
    TArray<double> Distances;

    /**
     * A saved field's current material coordinates, keyed by the vertex positions at the moment
     * the region is read. Empty for an inline ruler. Mesh weight layers are the durable carrier;
     * this is the evaluation snapshot profile-taking instructions consume.
     */
    TMap<FVector3d, double> ParametersAtPosition;
    FQuat4d Frame = FQuat4d::Identity;
    TArray<UE::Geometry::FPolygon2d> Patches;
    TArray<UE::Geometry::FAxisAlignedBox2d> PatchBounds;
    UE::Geometry::FAxisAlignedBox2d Bounds;
    double Feature = 0;

    bool IsSpatial() const { return !SpatialPath.IsEmpty(); }
    double Length() const { return Distances.IsEmpty() ? 0.0 : Distances.Last(); }

    /** Replace this value with a directed spatial path and rebuild its centimetre stations. */
    bool SetSpatialPath(const TArray<FVector3d>& Points, FString& Error);
    void RebuildSpatialDistances();

    FVector2d Project(const FVector3d& Point) const;
    double Parameter(const FVector3d& Point) const;
    double Distance(const FVector3d& Point) const;
    FVector2d Nearest(const FVector3d& Point) const;
    bool Contains(const FVector3d& Point) const;
    bool Refine(UE::Geometry::FDynamicMesh3& Mesh, FString& Error) const;

    /** Success with a null result means the unchanged fixed-region grammar. */
    static bool Read(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
        const FGcShapeHeader& Header, IGcShapeVolumes& Volumes,
        const UE::Geometry::FDynamicMesh3& Mesh, TSharedPtr<FGcShapeStroke>& Out, FString& Error,
        bool bInheritScope = true);
};
