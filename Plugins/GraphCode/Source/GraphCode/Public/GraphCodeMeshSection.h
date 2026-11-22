// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Plane sections of a closed triangle surface, and the thin tube a diagnostic draws a line with —
// shared by GraphCode Rig and GraphCode Garment, so it is Core's. Plain arrays, so Core needs no
// geometry module; each product fills them from its own mesh type.

/** A triangle surface read once for cutting: positions, triangles, and one id per distinct position. */
struct FGcSectionSurface
{
	TArray<FVector> Positions;

	/**
	 * One id per distinct position. An imported mesh splits a vertex at every UV seam, and a loop
	 * chained through split vertices would break at each one.
	 */
	TArray<int32> Weld;

	TArray<uint32> Indices;

	/** Fill `Weld` from `Positions`. */
	GRAPHCODE_API void WeldPositions();
};

/** A plane and the 2D frame a section is written in. */
struct FGcSectionPlane
{
	FVector Origin = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;
	FVector E1 = FVector::ForwardVector;
	FVector E2 = FVector::RightVector;

	/** The plane through `Origin` square to `Normal`, framed by `Basis`. */
	GRAPHCODE_API static FGcSectionPlane Through(const FVector& Origin, const FVector& Normal);

	/** Two axes square to `Normal` and to each other, built from whichever world axis it lies along least. */
	GRAPHCODE_API static void Basis(const FVector& Normal, FVector& OutE1, FVector& OutE2);

	GRAPHCODE_API FVector2D ToPlane(const FVector& World) const;
	GRAPHCODE_API FVector ToWorld(const FVector2D& InPlane) const;
};

class GRAPHCODE_API FGraphCodeMeshSection
{
public:
	/**
	 * Every closed loop the plane cuts out of the surface, in the plane's frame about its origin. A
	 * walk that meets an open or non-manifold edge ends unclosed and is not returned. `Triangles`
	 * limits the cut; left empty it takes every triangle.
	 */
	static void CutLoops(const FGcSectionSurface& Surface, const FGcSectionPlane& Plane,
		TArray<TArray<FVector2D>>& OutLoops, const TArray<int32>& Triangles = TArray<int32>());

	/** Does the loop go round `Point`? A winding number, not a bounding box. */
	static bool Encloses(const TArray<FVector2D>& Loop, const FVector2D& Point);

	static double Perimeter(const TArray<FVector2D>& Loop);

	/**
	 * A thin three-sided tube round the segment A–B, the way a diagnostic draws a line: three corners
	 * round A, then three round B. False for a segment of no length.
	 */
	static bool TubeCorners(const FVector& A, const FVector& B, double Radius, FVector OutCorners[6]);

	/** The six triangles joining `TubeCorners`, as indices into them. */
	static TConstArrayView<FIntVector> TubeTriangles();
};
