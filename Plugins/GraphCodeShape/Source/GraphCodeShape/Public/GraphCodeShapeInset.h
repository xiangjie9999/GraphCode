// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **THE BORDER MOVE UNDER `raise(inset=)` — board row MB14, `Docs/GC-SHAPE-INSET-CORNER.md`.**
 *
 * The engine's `FInsetMeshRegion` insets each border vertex on its own. A corner goes to the mitre,
 * where the offset lines of its two edges meet; a vertex part-way along a straight edge is pushed
 * square to that edge and KEEPS ITS PLACE ALONG IT. So a border vertex nearer a convex corner than
 * the inset lands past the mitre and the ring doubles back on itself — in the surface, so it reads
 * as creases, until a lift moves the reversed triangles along their reversed normals into tears.
 * A coarse surface has no vertex between its corners, which is why it never tore.
 *
 * **THIS IS THAT OPERATION WITH ONE STEP REPLACED.** A run of border vertices between two corners
 * whose inset positions lost their order is spread along the inset run by the fraction of the run
 * each one sat at — which on a straight edge is the one placement a mitred offset allows. A corner
 * still goes to the mitre, and a run that kept its order is left exactly where the engine put it,
 * so every shape that never folded builds as it did. The rest — the disconnect, the stitched strip,
 * the interior settle and the strip's UVs — is the engine's, with reprojection off.
 */
namespace GraphCodeShapeInset
{
	/**
	 * Move the border of each connected piece of `Triangles` `Distance` inward within the surface,
	 * stitch a strip between the old border and the new one, and settle the interior against it.
	 * The region keeps its triangle ids; the strip's are appended to `OutStitched`. False when a
	 * piece could not be separated from the surface around it.
	 */
	GRAPHCODESHAPE_API bool Apply(UE::Geometry::FDynamicMesh3& Mesh, const TArray<int32>& Triangles,
		double Distance, TArray<int32>& OutStitched);
}
