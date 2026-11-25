// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeShapeFace.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **THE CAP A DEPTH INSTRUCTION CLOSED, TRIANGULATED AGAIN OVER ITS OWN RING — board row MT13b.**
 *
 * MT13 divided a long straight run so a flat face stops being described by two triangles with a
 * 908 cm diagonal, and then found the half nobody predicted: **the ear clipper cannot triangulate
 * what the division hands it.** `PolygonTriangulation::TriangulateSimplePolygon` names collinear
 * vertices as its own failure case, and a divided straight run is nothing else — MEASURED on the
 * 900 x 120 panel's ten points, **8 triangles, longest edge 685.6, two of them zero-area**, against
 * `FConstrainedDelaunay2d`'s **8 triangles, longest edge 255.0, none degenerate**. The count is the
 * same, so no number on the build line can tell the two apart.
 *
 * **MT13 SPENT THAT EXCHANGE AT ONE DOOR AND THERE ARE FIVE**, which is the whole of MT13b: `sweep`
 * shares `extrude`'s generator, `revolve` has three of its own, and `loft`'s cap is ours
 * (`GcShapeLoftDetail::CapRing`, ear clipping again). Four bespoke exchanges would need four
 * readings of four vertex layouts, and two of those layouts are engine internals nobody here has
 * measured.
 *
 * **SO IT IS DONE ONCE, HERE, OVER THE TAG RATHER THAN OVER A LAYOUT.** §5's tag layer already
 * knows which triangles are the cap — it is provenance, written when the face was made, and
 * `TagSweptTube`, `TagRevolved` and `loft`'s own `TagThese` all supply it. A pass keyed on the tag
 * needs to know nothing about which generator ran.
 *
 * **THIS IS NOT THE PASS MT13 DELETED, AND THE DIFFERENCE IS THAT NOTHING HERE IS MEASURED TO FIND
 * ITS SUBJECT.** That one flipped edges it had to identify by dihedral angle, and it cut the
 * corners off solids because a box's vertical edge and a cap's diagonal look alike to any threshold
 * ([[decoration-is-not-authored-state]] is the same shape of error). This one is told which
 * triangles to replace and replaces exactly those; the ring it re-fills is the boundary of what it
 * removed, so no edge outside the cap is touched and no vertex moves.
 *
 * **AND IT NEEDS NO WINDING CONVENTION, WHICH IS WHY IT REOPENS NOTHING.** The bespoke exchange at
 * `extrude` rests on *triangle index `i` IS ring column `i`*, which is true only because
 * `NormaliseWinding` runs at that door. Closed sweep sections are normalised there too; ribbons
 * keep their authored facing. Here the cap that
 * is already there IS the answer: its own area-weighted normal is measured before it is removed,
 * and the replacement is wound to agree with it. Faithful by construction — same ring, same plane,
 * same facing, better triangles.
 */
namespace GraphCodeShapeCap
{
	/**
	 * Replace one tagged cap with a constrained-Delaunay triangulation of the same ring.
	 *
	 * A cap of fewer than two triangles is left alone: there is only one triangulation of a triangle,
	 * and no triangles at all is an end that was never capped. Holes are carried — the region's
	 * boundary comes back as several loops, the widest is the outer and the rest are voids, which is
	 * what a holed outline's cap is.
	 *
	 * **THAT READING ASSUMES THE TAGGED REGION IS ONE CONNECTED FACE, AND EVERY CALLER'S IS.** A cap
	 * is a single face by construction at all three doors — `sweep` and `revolve` build from an
	 * outline with no boolean in between, and `loft`'s `top` after `Build` is the one ring it just
	 * closed, `TakeTopFace` having already refused a shape whose open top is more than one ring. A
	 * region that were genuinely two islands would be read here as a face with a hole, so a fourth
	 * caller owes that check before it asks.
	 *
	 * Returns false where the ring cannot be triangulated, and **every check that can refuse runs
	 * before a single triangle is removed** — the loops, the triangulation and the winding are all
	 * settled first, so an ordinary refusal leaves the mesh exactly as it was. The one exception is
	 * an `AppendTriangle` that the mesh itself rejects, which means the ring will not take a
	 * triangle it must; that also refuses, because a partly-filled cap is a hole and reads on the
	 * build line exactly like a full one.
	 */
	GRAPHCODESHAPE_API bool Retriangulate(UE::Geometry::FDynamicMesh3& Mesh, EGcShapeFace Face,
		const TCHAR* Verb, FString& OutError);
}
