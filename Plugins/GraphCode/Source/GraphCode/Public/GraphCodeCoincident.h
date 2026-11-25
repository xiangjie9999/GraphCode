// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BoxTypes.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"

class UBlueprint;

/**
 * **TWO SEPARATE SURFACES IN THE SAME PLACE, FACING THE SAME WAY — THE OVERLAP THAT FLICKERS.**
 * User-reported 2026-09-28: overlapping parts *"ALWAYS cause things to keep flickering"*.
 * `GC-SHAPE-GRAMMAR.md` §6.2.
 *
 * **ONE RULE, TWO CALLERS, WHICH IS WHY IT IS IN CORE.** Shape asks it of the shells of one built mesh
 * (`coincident` on the build line); a Blueprint apply asks it of its static-mesh components
 * (user-ruled the same day: *on Blueprint apply*), because Shape never compares separate assets.
 * Each caller says who owns each triangle, and only triangles of DIFFERENT owners are compared.
 *
 * **THE RULE.** Two triangles whose normals agree within 5 degrees, whose overlap has area, and whose
 * whole overlap lies within 0.01 cm of both planes (or float noise on a very large shape), where
 * that place can be seen from outside. Same-facing only: two faces lying on each other and facing
 * OPPOSITE ways are a flush seat, sealed between two solids, measured harmless on 91 placements.
 *
 * **0.01 cm, ESTIMATED, NOT MEASURED.** Unreal's reversed-Z float depth would order two parallel
 * surfaces a millionth of the viewing distance apart; the rasteriser's sub-pixel snap loses the
 * order first, at roughly 3e-6 of the distance. So a 0.01 cm step holds to ~30 m and the 0.1 cm the
 * guides ask for to ~300 m, and anything inside 0.01 cm is an authored flush seat plus float noise.
 */
namespace GraphCodeCoincident
{
	/** One place two owners' surfaces lie on each other. */
	struct FPlace
	{
		int32 OwnerA = INDEX_NONE;
		int32 OwnerB = INDEX_NONE;

		/** The largest overlapping pair — one triangle of each owner, for naming. */
		int32 TriangleA = INDEX_NONE;
		int32 TriangleB = INDEX_NONE;

		FVector3d Normal = FVector3d::ZAxisVector;
		UE::Geometry::FAxisAlignedBox3d Box = UE::Geometry::FAxisAlignedBox3d::Empty();
		double Area = 0.0;

		/** The widest gap across the place, in cm — 0 up to float noise for a flush seat. */
		double Separation = 0.0;
	};

	/**
	 * Every visible place where triangles of different owners coincide, largest first.
	 * `OwnerOf` is indexed by triangle id; `INDEX_NONE` leaves a triangle out.
	 */
	GRAPHCODE_API void Find(const UE::Geometry::FDynamicMesh3& Mesh,
		UE::Geometry::FDynamicMeshAABBTree3& Tree, TConstArrayView<int32> OwnerOf,
		TArray<FPlace>& OutPlaces);

	/**
	 * **SEEN FROM OUTSIDE: ONE LINE OF SIGHT IS ENOUGH.** From `Standoff` in front of `Centre`, 24 rays
	 * spread over the half-space `Normal` faces; one that meets nothing is a line of sight. The rule
	 * `Find` counts a coincident place by, lifted out 2026-10-13 so GC Shape's findings (open borders,
	 * parts, fold clusters) say *seen* or *hidden* by the same test rather than a second copy of it.
	 */
	GRAPHCODE_API bool SeenFromOutside(UE::Geometry::FDynamicMeshAABBTree3& Tree, const FVector3d& Centre,
		const FVector3d& Normal, double Standoff);

	/** `+Z` for an axis-aligned facing, `(0.71, 0.00, 0.71)` otherwise. */
	GRAPHCODE_API FString Facing(const FVector3d& Normal);

	/** `flush`, or `0.004 cm apart`. */
	GRAPHCODE_API FString Gap(double Separation);

	/**
	 * **A BLUEPRINT'S STATIC-MESH COMPONENTS, IN THE BLUEPRINT'S OWN SPACE.** Each component's source
	 * mesh is placed by its relative transform composed up its construction-script parents, this
	 * Blueprint's and every Blueprint parent's. Returns how many places coincide and, when any do,
	 * `OutWhere` names them largest first: `1600 cm2 facing +Z at (0, 0, 20), flush, between Plate
	 * and Wall`.
	 *
	 * **WHAT IT DOES NOT SEE YET, SAID RATHER THAN GUESSED:** native (C++) components, inherited
	 * templates a child Blueprint overrides, and attachment to a named socket.
	 */
	GRAPHCODE_API int32 ForBlueprint(const UBlueprint* Blueprint, FString& OutWhere);
}
