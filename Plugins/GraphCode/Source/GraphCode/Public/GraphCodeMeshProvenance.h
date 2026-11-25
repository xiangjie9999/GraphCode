// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * Product-neutral static-mesh reading and deterministic identity.
 *
 * Core owns this seam because mesh-backed products besides Shape need to read and identify an
 * authored source mesh. Nothing here knows about a .gcshape program, Shape provenance stamps, or
 * Shape's overwrite policy.
 */
namespace GraphCodeMeshProvenance
{
	/**
	 * A stable digest of mesh positions and topology.
	 *
	 * Positions are quantised at the same 0.01 cm tolerance used by Shape's hand-edit guard. The
	 * signature namespace intentionally stays GraphCodeShape.Mesh:1 so moving this implementation
	 * does not invalidate assets already stamped by Shape.
	 */
	GRAPHCODE_API FString MeshDigest(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * Read one authored static-mesh source model, without applying its build settings.
	 *
	 * Source data is used instead of render data so this works without an RHI and measures the mesh
	 * an author edits rather than a derived render representation.
	 */
	GRAPHCODE_API bool ReadAssetMesh(UStaticMesh* Asset, UE::Geometry::FDynamicMesh3& Out,
		FString& OutError, int32 SourceModelLod = 0);

	/**
	 * A dependency digest including topology, UVs, material IDs and static-mesh material slots.
	 * `Mesh` must be the result of `ReadAssetMesh(Asset, ..., SourceModelLod)`.
	 */
	GRAPHCODE_API FString AssetMeshDigest(UStaticMesh* Asset,
		const UE::Geometry::FDynamicMesh3& Mesh, int32 SourceModelLod);
}
