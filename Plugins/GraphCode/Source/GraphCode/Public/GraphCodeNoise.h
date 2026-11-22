// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * **THE FIELDS A SURFACE IS ROUGHENED BY, ONE WRITER EACH — moved into Core 2026-11-18.** GC Shape's
 * `roughen` displaces by them and a `warp` formula reads them (`noise()`, `cell()` in
 * `GraphCodeScalarExpression`), and the scalar evaluator is Core's, so the one implementation lives here
 * and Shape calls it. The Perlin field itself is the engine's `FMath::PerlinNoise3D`.
 */
namespace GraphCodeNoise
{
	/**
	 * `roughen(seed=)`'s draw: one `FRandomStream` fraction for `Key`, scaled by 10000, the engine's own
	 * arithmetic (`MeshDeformFunctions.cpp`). Every sample adds it to all three coordinates, so one key is one
	 * partition of the field. `roughen` passes the entry-keyed seed; `noise()` and `cell()` pass the seed alone.
	 */
	GRAPHCODE_API double SeedOffset(int32 Key);

	/**
	 * One value in -1..1 per Worley cell, constant inside the cell: `roughen(form=faceted)`'s field. `At` is in
	 * lattice units (one cell per unit), the seed's offset already added.
	 */
	GRAPHCODE_API double Cell(const FVector3d& At);
}
