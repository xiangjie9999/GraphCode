// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

/**
 * THE SURFACE THE AUTHOR BUILT, NOT THE ONE A NANITE MESH KEEPS ON THE CPU. A Nanite static mesh's
 * LOD 0 render data is its fallback, a simplified stand-in: the 60 x 40 cm shell rug read as 230
 * triangles for a 4,802-triangle top (measured 2026-09-29), and a projected region could no longer be
 * traced across it. Anything that reads LOD 0 to place things ON the surface reads the stand-in.
 *
 * Two callers, which is why it is in Core: Render draws a Nanite mesh at full detail in a private
 * preview world, and Groom grows hair and shells on it.
 */
namespace GraphCode
{
	/**
	 * A transient, non-Nanite copy of Source built from its highest-detail editor source (the hi-res
	 * source when it has one, else LOD 0's mesh description), with Source's materials, sockets and
	 * build settings and no reduction. Built synchronously, so its render data is ready on return. nullptr
	 * when the source was stripped or the copy has no triangles. The caller keeps it alive.
	 */
	GRAPHCODE_API UStaticMesh* BuildFullDetailStaticMesh(UStaticMesh* Source);
}
