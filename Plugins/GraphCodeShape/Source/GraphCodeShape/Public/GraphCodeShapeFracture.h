// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **BREAK ONE SOLID INTO N REAL SEPARATE PIECES** — `Docs/GC-SHAPE-FRACTURE.md`.
 *
 * **THE CELLS ARE VORONOI AND THE CUTTER IS THE SHIPPED ONE.** A Voronoi cell around a point is the
 * intersection of the half-spaces bisecting it against every other point, and `FMeshPlaneCut` —
 * what `cut(plane=)` already runs, against ANY direction since board row MR3 — is exactly one of
 * those half-spaces. So a piece is the solid cut N-1 times and capped each time, and nothing new
 * had to learn how to split a mesh.
 *
 * **A PIECE IS A CONNECTED COMPONENT, NOT A SEPARATE ASSET.** The caller welds these back into one
 * mesh, which is what `parts` has always counted and what `GcShapeWhereParts` already reports
 * largest-first. This namespace hands back the pieces so a per-piece body can run between the two.
 */
namespace GraphCodeShapeFracture
{
	/** What a break came out as, for the note the caller prints. */
	struct GRAPHCODESHAPE_API FReport
	{
		/** Break points that enclosed no solid at all — two points on one spot, or a `gap=` past the cell. */
		int32 Empty = 0;
	};

	/**
	 * The pieces of `Solid` broken around `Cores`, in the order `Cores` gave them.
	 *
	 * `Pattern` STRETCHES the break: only its ratio is read, because how big a cell is, is decided
	 * by how many there are. `(1, 1, 0.25)` gives slabs, `(1, 0.1, 0.1)` splinters.
	 *
	 * `Gap` holds the pieces apart, each cell losing half of it on every face — Blender's margin,
	 * and this format's own `conform(gap=)` word.
	 *
	 * **AN EMPTY CELL IS DROPPED AND COUNTED, NEVER REFUSED.** Two break points landing on one spot
	 * is an `apart=` the author can fix, and a whole fracture that came out empty is the caller's
	 * refusal to make, where it knows which argument to name.
	 */
	GRAPHCODESHAPE_API bool Break(const UE::Geometry::FDynamicMesh3& Solid,
		const TArray<UE::Math::TVector<double>>& Cores,
		const UE::Math::TVector<double>& Pattern, double Gap,
		TArray<UE::Geometry::FDynamicMesh3>& OutPieces, FReport& OutReport, FString& OutError);
}
