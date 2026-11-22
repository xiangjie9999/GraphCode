// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **MB52 — where a shape STICKS OUT and where it COLLECTS.** `GC-SHAPE-DECAL-DESIGN.md` §1.
 *
 * This is what makes `decal($Bare) [mask=edges]` mean something: the author places nothing, and the
 * form decides where the paint is gone. Real wear is a function of exposure — convex edges get
 * knocked, concave recesses hold dirt — so the mask is curvature and not noise.
 *
 * **PER VERTEX AND SIGNED, which is the whole trick.** One field answers both masks: positive is
 * convex and negative is concave, so `edges` and `crevices` are two readings of one pass rather
 * than two passes.
 *
 * **COMPUTED HERE RATHER THAN BORROWED.** UE ships no discrete-curvature helper in GeometryCore —
 * measured 2026-09-22 — so this is the standard dihedral formulation: each interior edge bends by
 * the angle between its two face normals, signed by which way it folds, accumulated onto both of
 * its endpoints and weighted by edge length so a long crease counts for more than a short one.
 */
namespace GraphCodeShapeCurvature
{
	/**
	 * Signed discrete mean curvature per vertex, indexed by vertex ID.
	 *
	 * **NORMALISED TO -1..1 ACROSS THE MESH**, because a mask is a proportion and an author writing
	 * `amount=70%` is talking about this shape rather than about radians. A flat mesh comes back
	 * all zeroes rather than dividing by nothing.
	 */
	GRAPHCODESHAPE_API void Compute(const UE::Geometry::FDynamicMesh3& Mesh,
		TArray<double>& OutPerVertex);

	/**
	 * One vertex's mask value in 0..1 for a given reading.
	 *
	 * `bConvex` selects `edges`; otherwise `crevices`. The opposite sign reads as zero rather than
	 * as a negative, so a mask never subtracts.
	 */
	GRAPHCODESHAPE_API double Read(const TArray<double>& PerVertex, int32 VertexID, bool bConvex);
}

/**
 * **MB52 — stamping a PLACED mark onto a surface, and MB15's cast done once.**
 *
 * A decal at a point, cast along a direction, covering a footprint. Two things make this more than
 * a plane projection, and both are MB15's measured complaints about MR19's brush:
 *
 * **IT DOES NOT STAMP THE FAR SIDE.** A cast through a box hits the front face and the back one;
 * only faces turned TOWARD the projector take the mark. Without that a crest on a chest's front
 * appears mirrored on its back, which is the defect MB15 opens with.
 *
 * **IT IS HONEST ABOUT THE OTHER HALF.** The footprint is measured in the projection PLANE, so a
 * mark cast onto a steep slope covers more surface than its `size=` states — MB15's second
 * complaint. That is a property of planar projection and is what shipped engines do; measuring on
 * the surface is a geodesic problem and belongs to MB15 proper, not here.
 */
namespace GraphCodeShapeProject
{
	/** Shared decal/texture-brush frame, prepared once in authored mesh coordinates. */
	struct GRAPHCODESHAPE_API FFrame
	{
		FVector3d Centre = FVector3d::ZeroVector;
		FVector3d Forward = FVector3d::ZeroVector;
		FVector3d U = FVector3d::ZeroVector, V = FVector3d::ZeroVector;
		bool Set(const FVector3d& At, const FVector3d& Direction, const FVector2d& Size, double Turn);
		FVector2d UV(const FVector3d& Position) const
		{
			const FVector3d Offset = Position - Centre;
			return FVector2d(Offset.Dot(U) + .5, Offset.Dot(V) + .5);
		}
	};

	/** What a cast produced: where it landed, and how strongly, per vertex. */
	struct GRAPHCODESHAPE_API FCast
	{
		/** 0..1 per vertex — one inside the footprint on a facing triangle, zero elsewhere. */
		TArray<double> PerVertex;

		/** The mark's own coordinates per vertex, so its image samples in the right place. */
		TArray<UE::Math::TVector2<double>> PerVertexUV;

		/** False when the cast reached no facing surface at all. */
		bool bHit = false;
	};

	/**
	 * Cast a footprint onto a mesh.
	 *
	 * `At` is the centre in the mesh's own space, `Direction` points INTO the surface, `Size` is the
	 * footprint in centimetres and `Turn` rotates it about the cast axis in degrees.
	 */
	GRAPHCODESHAPE_API void Cast(const UE::Geometry::FDynamicMesh3& Mesh,
		const UE::Math::TVector<double>& At, const UE::Math::TVector<double>& Direction,
		const UE::Math::TVector2<double>& Size, double Turn, FCast& Out);
}
