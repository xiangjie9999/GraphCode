// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGcShapeDocument;
struct FGcShapeInstruction;
namespace UE::Geometry { class FDynamicMesh3; }

/**
 * GROUP 1 — `polyhedron(size=, faces=, evenness=, seed=, mode=)`.
 *
 * A first-class closed 3D start, user-ruled 2026-09-25. Unlike the retired convenience
 * primitives, this is not a second spelling for an outline plus depth: the requested active plane
 * count and its regular-to-irregular arrangement are the geometry.
 */
namespace GraphCodeShapePolyhedron
{
	/** One outward supporting plane of a convex solid: points with Normal.Dot(P) <= Distance live. */
	struct GRAPHCODESHAPE_API FConvexPlane
	{
		UE::Math::TVector<double> Normal = UE::Math::TVector<double>::Zero();
		double Distance = 0.0;
		int32 Group = 0;
	};

	enum class EMode : uint8
	{
		/** A complete convex solid is formed first, then its largest face becomes the base. */
		Free,

		/** Parallel flat top and bottom with a regular-to-irregular footprint. */
		Prism,
	};

	struct GRAPHCODESHAPE_API FOptions
	{
		/** Full requested bounding-box dimensions, in centimetres. */
		UE::Math::TVector<double> Size = UE::Math::TVector<double>(100.0, 100.0, 100.0);

		/** Geometric polygonal faces, not carrying triangles. Four is the 3D minimum. */
		int32 Faces = 4;

		/** 1 is regular/balanced, 0 is maximally irregular within the quality guard. */
		double Evenness = 1.0;

		/** Stable variation. It has no geometric effect at Evenness == 1. */
		int32 Seed = 0;

		/** Free is the rock-oriented default; Prism deliberately retains a flat top. */
		EMode Mode = EMode::Free;
	};

	/** Build one convex, closed mesh. Its horizontal `[bottom]` face lies exactly on z=0. */
	GRAPHCODESHAPE_API bool Generate(const FOptions& Options,
		UE::Geometry::FDynamicMesh3& OutMesh, FString& OutError);

	/**
	 * Recover the minimal supporting planes of an already-built convex closed mesh. Coplanar
	 * carrier triangles become one plane; a concave or open input is refused rather than silently
	 * replaced by its convex hull.
	 */
	GRAPHCODESHAPE_API bool ExtractConvexPlanes(const UE::Geometry::FDynamicMesh3& Mesh,
		TArray<FConvexPlane>& OutPlanes, FString& OutError);

	/**
	 * Intersect a family of outward half-spaces and triangulate every active polygonal face once.
	 * Redundant planes are legal and omitted. OutActivePlanes receives their input indices.
	 */
	GRAPHCODESHAPE_API bool BuildConvexPlanes(const TArray<FConvexPlane>& Planes,
		UE::Geometry::FDynamicMesh3& OutMesh, TArray<int32>* OutActivePlanes, FString& OutError);

	/** Read the public instruction, generate it, then apply its ordinary group-1 placement. */
	GRAPHCODESHAPE_API bool Run(const FGcShapeDocument& Document,
		const FGcShapeInstruction& Line, UE::Geometry::FDynamicMesh3& OutMesh, FString& OutError);
}
