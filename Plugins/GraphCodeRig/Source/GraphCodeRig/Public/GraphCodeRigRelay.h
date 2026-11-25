// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigRegion.h"

class UDynamicMesh;

/**
 * THE RE-LAY — `[relay]`, the one pass that throws a surface away and builds another.
 *
 * **IT IS THE OPPOSITE END OF ONE AXIS FROM `keep`** (ruling 57): how much of the author's mesh the
 * build may spend. The ring cut splits edges and deletes nothing, which is why it ships for everybody;
 * this replaces a patch outright, which is why a region has to ask for it by name.
 *
 * **THE ENGINE'S OWN REMESH TAKES NO SELECTION AND RE-LAYS THE WHOLE MESH** — `ApplyUniformRemesh`,
 * and that is what made this look hard. The route that works is a rung lower: `FRemesher` from
 * GeometryCore takes an `FMeshConstraints`, so the restriction is expressed as *every vertex outside
 * this region is pinned* rather than as a selection the operator has to understand. Nothing outside
 * the band can move, split or collapse, so the patch is replaced and its seam stays welded.
 *
 * **AND THE SHAPE IS KEPT BY PROJECTING BACK ONTO THE MESH AS IT ARRIVED.** Remeshing smooths, and
 * smoothing on its own shrinks a limb toward its own axis a little more with every pass. The original
 * surface is copied before the first pass and every new vertex is pulled back onto it, so the re-lay
 * changes how the surface is BUILT and not where it is.
 *
 * **WHAT IT CANNOT DO, and this is a limit of the engine rather than of the format.** A uniform
 * remesh has no direction: it makes well-shaped triangles of an even size and has no opinion about
 * which way they run. That is why `flow=` and `pole=` are GONE rather than pending (ledger row 78):
 * both asked for direction, both wanted a field-guided quad remesher this engine does not contain,
 * and the parser now refuses them by name. What a re-lay does take is `density=`, its edge length.
 */

/** What one region's re-lay did, or would have done. */
struct GRAPHCODERIG_API FGcRigRelayResult
{
	int32 TrianglesBefore = 0;
	int32 TrianglesAfter = 0;

	/** Vertices fully owned by the region, including fixed seams that permit subdivision. */
	int32 Interior = 0;

	/** Original vertices fixed to preserve source boundaries and everything outside the region. */
	int32 Pinned = 0;

	/** Rebuilt triangles whose primary UVs were transferred from the immutable source surface.
	 * Every existing UV channel is transferred; seams and material boundaries keep their positions
	 * and can only subdivide, interpolating their original coordinates.
	 * Transfer preserves surface references, with interpolation error dependent on mesh density.
	 */
	int32 Transferred = 0;

	TArray<FString> Notes;
};

class GRAPHCODERIG_API FGraphCodeRigRelay
{
public:
	/**
	 * Rebuild one region's surface at `TargetEdgeLength` centimetres, and touch nothing else.
	 *
	 * **A REGION WITH NO INTERIOR IS REPORTED, NOT REMESHED.** A band two triangles across is all
	 * seam: every vertex it owns is shared with what is next door, so there is nothing free to move
	 * and a re-lay would be a no-op wearing a cost. Saying so is the difference between a surface that
	 * could not be improved and one that nobody tried to improve.
	 */
	static bool Relay(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span, double TargetEdgeLength,
		FGcRigRelayResult& Out, FString& OutRefusal);

	/**
	 * **HOW MANY PASSES — 20, and it is the engine's own number rather than one chosen here.**
	 * `FGeometryScriptRemeshOptions::RemeshIterations` defaults to 20 for `ApplyUniformRemesh`, which
	 * is the same operator over the whole mesh; a region is smaller than a mesh, so the same count
	 * cannot be too few.
	 */
	static int32 Passes();
};
