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

		/**
		 * **G32: ONE JOINT SET** (`joints=((dip=, azimuth=, spacing=), ...)`, 2026-11-20, `Docs/GC-SHAPE-ROCK-GRAMMAR.md`
		 * §3 item 1): a PAIR of parallel planes across the `size=` box. `Dip` is the planes' angle from level in degrees
		 * (0 level, 90 upright); `Azimuth` the way they lean down, degrees from +X toward +Y; `Spacing` the gap between
		 * the pair as a share of the box's width across them (1 = the pair touches the box's far corners).
		 */
		struct FJointSet
		{
			double Dip = 0.0;
			double Azimuth = 0.0;
			double Spacing = 1.0;

			/**
			 * **G33: `save=$Fresh`** (2026-11-20, `Docs/GC-SHAPE-ROCK-GRAMMAR.md` §4): the name, without its `$`, of the
			 * ordinary region the faces of BOTH planes of this set become. Empty: the set's faces stay unnamed. Two sets
			 * with one name make one region, their union.
			 */
			FString Save;
		};

		/** G32: with sets, `Faces` counts EXTRA random breaks (0 allowed) and every plane goes into one rebuild. */
		TArray<FJointSet> Joints;

		/** G32: the most, in degrees, a joint plane leans off its set's direction (seeded per plane). */
		double Tilt = 0.0;
	};

	/** The most joint sets one polyhedron takes, and the most extra breaks beside them. */
	constexpr int32 MostJointSets = 8;
	constexpr int32 MostJointBreaks = 64;
	/** The most a joint plane may lean off its set, degrees: past it the set no longer reads as one family. */
	constexpr double MostJointTilt = 45.0;

	/**
	 * G33: one named region a polyhedron made (`joints=((..., save=$Fresh))`): its name without the `$`, and the
	 * triangles of the output mesh it holds, in order. The evaluator writes it into the body's ONE NAMESPACE.
	 */
	struct FSavedRegion
	{
		FString Name;
		TArray<int32> Triangles;
	};

	/** Build one convex, closed mesh. Its horizontal `[bottom]` face lies exactly on z=0. */
	GRAPHCODESHAPE_API bool Generate(const FOptions& Options,
		UE::Geometry::FDynamicMesh3& OutMesh, FString& OutError, TArray<FString>* OutNotes = nullptr,
		TArray<FSavedRegion>* OutRegions = nullptr);

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
		const FGcShapeInstruction& Line, UE::Geometry::FDynamicMesh3& OutMesh, FString& OutError,
		TArray<FString>* OutNotes = nullptr, TArray<FSavedRegion>* OutRegions = nullptr);
}
