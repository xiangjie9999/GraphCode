// Copyright GraphCode Sdn Bhd. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IndexTypes.h"

namespace UE::Geometry { class FDynamicMesh3; class FMeshConstraints; }

/** Connectivity permissions, independent of detail or an output triangle ceiling. */
enum class EGcShapeTopology : int8 { Inherit = -1, Adaptive = 0, Preserve = 1, Refine = 2 };

namespace GraphCodeShapeTopology
{
GRAPHCODESHAPE_API bool Read(const FString& Value, EGcShapeTopology& Out, FString& Error);
GRAPHCODESHAPE_API EGcShapeTopology Get(const UE::Geometry::FDynamicMesh3& Mesh, int32 Triangle);
GRAPHCODESHAPE_API void Set(UE::Geometry::FDynamicMesh3& Mesh, EGcShapeTopology Mode,
    const TArray<int32>& Triangles = {});
GRAPHCODESHAPE_API bool HasRestricted(const UE::Geometry::FDynamicMesh3& Mesh);
GRAPHCODESHAPE_API void PrepareAppend(UE::Geometry::FDynamicMesh3& Into,
    const UE::Geometry::FDynamicMesh3& From);
GRAPHCODESHAPE_API bool CanSplit(const UE::Geometry::FDynamicMesh3& Mesh, int32 Edge);
GRAPHCODESHAPE_API void ConstrainReduction(const UE::Geometry::FDynamicMesh3& Mesh,
    UE::Geometry::FMeshConstraints& Constraints);

/**
 * Planar, attribute-preserving reconnection. Positions and triangle count do not change. `Held`
 * edges are never flipped — the lines a cutter put in, which the reconnection must work around.
 */
GRAPHCODESHAPE_API int32 Improve(UE::Geometry::FDynamicMesh3& Mesh,
    const TSet<int32>* Region = nullptr, const TSet<int32>* Held = nullptr);

/**
 * **THE SAME RECONNECTION, GUARDED BY A TURN AND VISITED IN AN ORDER TAKEN FROM POSITIONS** — the
 * declared-crease pass's sliver step (`GC-SHAPE-CREASE-WARP-DESIGN.md` §6.6). A flip is made only where
 * the two triangles' normals are within `MostTurnDegrees` of each other (the plane test above is
 * replaced by that guard), every other test of `Improve` is kept, and each pass visits `Region`'s edges
 * sorted by their end and opposite positions, so the result does not depend on vertex or edge ids. The
 * overload above and its callers are untouched. Module-internal: not exported.
 */
int32 ImproveInPositionOrder(UE::Geometry::FDynamicMesh3& Mesh, const TSet<int32>& Region,
    const TSet<int32>& Held, double MostTurnDegrees);

/** An override lasts one operation; inherited policy follows any subdivided triangles. */
class GRAPHCODESHAPE_API FScope
{
public:
    explicit FScope(UE::Geometry::FDynamicMesh3& InMesh);
    ~FScope();
    void Override(EGcShapeTopology Mode, const TArray<int32>& Triangles = {});
    void Capture();
    bool Validate(const FString& Subject, FString& Error) const;
    FScope(const FScope&) = delete;
    FScope& operator=(const FScope&) = delete;
private:
    UE::Geometry::FDynamicMesh3& Mesh;
    FName Restore;
    EGcShapeTopology Fallback = EGcShapeTopology::Adaptive;
    TMap<int32, UE::Geometry::FIndex3i> Faces;
    TMap<int32, int32> VertexEdges;
};
}
