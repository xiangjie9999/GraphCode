// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigLink.h"
#include "GraphCodeRigRegion.h"

class USkeletalMesh;

/**
 * **`keep_volume=` AS A NUMBER ON EVERY VERTEX — the half of it that is geometry's business.**
 *
 * The key says how much of its thickness a limb keeps as a joint closes, and the thing that honours
 * it is a skinning rule rather than a triangle: linear blend skinning averages two rigid transforms,
 * so a fold's cross-section flattens however many edges cross it. Ledger §0b.3 and §0b.4 measured
 * that from both directions — eight times the rings moved volume retention 0.8 of a point, and three
 * different ring placements at one budget gave 69.3 / 69.3 / 69.4 percent.
 *
 * **THIS FILE WRITES THE MASK AND STOPS THERE, AND THE LINE IS A PRODUCT BOUNDARY RATHER THAN A
 * TIDINESS ONE.** What consumes the mask is a Deformer Graph, which lives on an optional engine
 * plugin; linking that here would break the `omits: all@rig` stub a build WITHOUT the plugin must
 * still be able to write (ledger §12.10's reasoning, applied a second time). So GraphCode Rig states
	* the intent per vertex and GraphCode Advanced carries it. GraphCode Groom follows the same optional-
	* framework rule for HairStrands inside its own product module.
 *
 * **THE MASK IS COMPUTED FROM THE MESH DESCRIPTION'S OWN POSITIONS, NEVER BY INDEXING ACROSS TWO
 * MESHES.** A `UDynamicMesh` and the `FMeshDescription` a skeletal mesh is built from need not agree
 * about vertex order — a split for a UV seam or a normal makes one vertex into several — so a mask
 * carried by index would be a permutation nobody could see. Every value here is derived from the
 * position it is about, which cannot be shifted.
 */
class GRAPHCODERIG_API FGraphCodeRigKeepVolume
{
public:
	/**
	 * **THE NAME A DEFORMER GRAPH ASKS FOR, and it is fixed on purpose.** One graph serves every rig
	 * built by this product, because the authoring lives in the mask rather than in the graph — so
	 * the name is a contract between two modules and not something a file may rename.
	 */
	static FName AttributeName() { return FName(TEXT("GcKeepVolume")); }

	/**
	 * Write the per-vertex mask onto LOD 0 of a built skeletal mesh.
	 *
	 * **A MESH WHOSE RIG NEVER SAYS `keep_volume=` IS NOT TOUCHED AT ALL** — no attribute, no LOD
	 * entry, no rebuild. That is what makes the key safe to add to a language that already ships:
	 * unset has to mean *exactly as before*, not *the default value of a new thing*.
	 *
	 * Returns false only on a real failure; a rig that states nothing returns true having done
	 * nothing, and says so through `OutVertices` being zero.
	 */
	static bool Write(USkeletalMesh* Mesh, const FGcRigLinkedRig& Rig,
		const TArray<FGcRigRegionSpan>& Spans, int32& OutVertices, FString& OutError);

	/**
	 * How much of its section ONE point keeps, 1 for untouched and 0 for a fold left to pinch flat.
	 *
	 * **THE BAND IS DERIVED FROM THE REGION THE AUTHOR ALREADY PLACED** — full across it, fading over
	 * its outer quarter. `Falloff` alone is 1 only at a region's exact middle and near 0 across most
	 * of it, so using it raw leaves the crease a hairline on the centre line: measured, a whole sweep
	 * moved volume 98.4% to 98.1% and read as *the rule does not work* (ledger §13.12).
	 *
	 * Where regions overlap the most gathered one wins, because a fold is a fold however many
	 * declarations reach it.
	 */
	static double MaskAt(const FGcRigLinkedRig& Rig, const TArray<FGcRigRegionSpan>& Spans,
		const FVector& At);
};
