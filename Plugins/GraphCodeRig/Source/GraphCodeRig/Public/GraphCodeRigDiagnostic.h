// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigPose.h"

class UDynamicMesh;

/**
 * SEEING A RIG — the colours that turn a bind into something a person can judge.
 *
 * **THE BUILDER SHIPS WEIGHTS THAT NOBODY HAS LOOKED AT.** `ComputeSmoothBoneWeights` runs, a
 * skeletal mesh lands on disk, and every check on it so far is arithmetic: bone counts, parentage,
 * positions. Whether the weights are any GOOD is a question only an eye answers, and until this file
 * there was no way to ask it short of opening the asset and turning a bone by hand.
 *
 * **THE RAMP IS EVERY OTHER RIGGING TOOL'S, ON PURPOSE (user-ruled 2026-09-16).** Blue for none
 * through cyan, green and yellow to red for full — what Blender's weight paint draws, and near enough
 * what everything else does. A rigger reading this picture should not have to learn a second
 * language for it, and a novel palette would be exactly that.
 *
 * **WHAT LIVES WHERE.** This file makes the coloured diagnostic geometry. The renderer's `overlay:`
 * option puts that geometry over the real mesh with depth disabled; depth is a property of the
 * transient preview material, not of a vertex colour or of the diagnostic asset. A joint that names
 * `turns_in=` uses that region's colour here, and its generated helper bones inherit it.
 */
class GRAPHCODERIG_API FGcRigDiagnostic
{
public:
	/**
	 * A weight as a colour: **0 blue, 0.25 cyan, 0.5 green, 0.75 yellow, 1 red**, interpolated
	 * between, and clamped outside.
	 *
	 * **FIVE STOPS RATHER THAN THREE BECAUSE THE MIDDLE IS WHERE A BIND GOES WRONG.** A blue-green-red
	 * ramp gives one flat green band across the whole middle, and the thing worth seeing at a joint is
	 * precisely how fast the hand-over happens across it.
	 */
	static FLinearColor WeightColour(double Weight);

	/**
	 * A stable, distinct colour per bone index — for the view that paints each vertex by whichever
	 * bone pulls it hardest.
	 *
	 * **STABLE MATTERS MORE THAN PRETTY.** Two renders of one rig must give a bone the same colour, or
	 * comparing a before and an after is worthless; so it is a function of the index and never of a
	 * counter, a random seed, or the order anything was visited.
	 */
	static FLinearColor BoneColour(int32 BoneIndex);

	/**
	 * A stable, distinct colour per REGION — and the same one the region's edges are drawn in, so a
	 * wireframe and a painted surface say the same thing about the same patch (user-ruled 2026-09-16).
	 *
	 * **OFFSET HALF A STEP FROM THE BONE COLOURS ON PURPOSE.** A picture may carry both — a skeleton
	 * drawn over a region-painted skin — and a region that happened to share a bone's colour would be
	 * unreadable exactly where the two matter most.
	 */
	static FLinearColor RegionColour(int32 RegionIndex);

	/**
	 * The colour overrides for bones that turn in a named skin region. The joint and the patch it
	 * controls deliberately share `RegionColour`; rigid bones keep their ordinary `BoneColour`.
	 * Twist helpers inherit the region of the authored bone whose turn they carry.
	 */
	static TMap<int32, FLinearColor> JointSkinColours(const FGcRigLinkedRig& Rig,
		const struct FGcRigSkeletonPlan& Plan);

	/**
	 * Paint every vertex by how hard ONE bone pulls it. The usual view, and the one that answers
	 * *does this bone own what it should*.
	 *
	 * A vertex the bone does not touch is weight 0 — blue — rather than uncoloured, because absent
	 * and zero look identical in a picture and only one of them is a bug.
	 */
	static bool PaintBoneWeight(UDynamicMesh* Mesh, int32 BoneIndex, FString& OutRefusal);

	/**
	 * Paint every vertex by whichever bone pulls it hardest — the other standard view, and the one
	 * that shows a SEAM: where ownership changes, and whether it changes where a joint is.
	 */
	static bool PaintDominantBone(UDynamicMesh* Mesh, FString& OutRefusal);

	/**
	 * **THE WIREFRAME, AS GEOMETRY RATHER THAN AS A VIEW MODE.** Every edge of `Source` becomes a thin
	 * three-sided tube in `Out`, vertex-coloured, so an ordinary lit render shows the edges.
	 *
	 * **THE ENGINE'S OWN WIREFRAME IS NOT AVAILABLE HERE AND THAT IS MEASURED, NOT ASSUMED**
	 * (`GraphCodeRender.h`, the withdrawals list): `VMI_Lit_Wireframe` ASSERTS in any `-unattended`
	 * process, because the mesh-edge pre-pass hangs off a scene view extension registered only for an
	 * attended editor — so it is a hard crash in the automation gate and every commandlet.
	 * `VMI_Wireframe` renders a solid white plate, because wireframe needs the editor-primitive
	 * composition pass and an offscreen game-flag family does not run it.
	 *
	 * **AND DRAWING THEM IS BETTER THAN BORROWING THEM ANYWAY, which is the part worth keeping.** The
	 * engine's wireframe is one uniform colour, so it cannot answer either question a retopo actually
	 * raises. `EdgeColours` answers both, because it is a colour PER EDGE rather than one flag:
	 *
	 * - *Which edges did the build ADD?* Map the inserted rings to `AddedEdgeColour`.
	 * - *Which vertices belong to which region?* **USER-RULED 2026-09-16 — map each region's edges to
	 *   that region's own colour, THE SAME ONE ITS SKIN IS PAINTED.** Then the wireframe and the
	 *   surface agree, and a reader can see at a glance which loops a region actually owns rather
	 *   than guessing from a silhouette.
	 *
	 * Hand it an empty map and it is an ordinary wireframe, which is what a `.gcshape` wants when the
	 * question is just *what does this topology look like*.
	 *
	 * **`MaxEdges` IS A CEILING AND EXCEEDING IT REFUSES**, because a tube per edge on a dense mesh is
	 * millions of triangles and the failure would otherwise be a hang rather than a sentence.
	 *
	 * **AND IT DOES NOT DRAW EVERY EDGE BY DEFAULT, which is `gc_render`'s rule rather than a new
	 * one.** `FGraphCodeHighlightMeshStyle::bKeepInteriorEdges` already draws this line: a quad
	 * triangulates into two triangles and the shared diagonal is not an edge of the shape, it is an
	 * artefact of how the shape was cut up — drawn, a box gets an X across every face and reads as a
	 * thicket. So a coplanar edge is skipped unless `bKeepInteriorEdges` asks for it. **A RETOPO VIEW
	 * IS THE CASE THAT ASKS**: the rings a cut inserted lie flat along a curved limb and are exactly
	 * the data, the same way a navmesh's own triangles are (user-ruled 2026-08-18).
	 */
	static bool BuildEdgeMesh(UDynamicMesh* Source, UDynamicMesh* Out, double Radius,
		const TMap<int32, FLinearColor>& EdgeColours, bool bKeepInteriorEdges, int32 MaxEdges,
		FString& OutRefusal, bool bCarryColours = true);

	/** An edge no pass touched. Deliberately quiet — the emphasis is what a reader should see. */
	static FLinearColor PlainEdgeColour();

	/** An edge a pass ADDED. Loud, and never a colour the weight ramp uses. */
	static FLinearColor AddedEdgeColour();

	/**
	 * **THE SKELETON AS GEOMETRY — the shape every rigging tool draws**, an octahedron per bone:
	 * a point at the joint, a square a little way down, and a point at the child.
	 *
	 * **THE SHAPE IS NOT DECORATION.** It says three things at once that a line cannot: where the bone
	 * starts, which way it points, and how long it is. A rigger reads a wrong-facing bone off the
	 * silhouette immediately, and off a line never.
	 *
	 * A joint that names `turns_in=` takes that region's `RegionColour`, so the joint and the skin it
	 * turns in agree. Its generated helpers inherit that colour. A rigid bone with no region keeps its
	 * stable `BoneColour`, which also matches the dominant-bone view.
	 *
	 * **A LEAF HAS NO LENGTH**, so it is drawn as a small stub in its inherited direction rather than
	 * skipped: a missing bone in a picture reads as a missing bone in the rig.
	 */
	/**
	 * Copy a mesh's per-vertex colours into the attribute-set OVERLAY, which is the only one an
	 * asset write reads.
	 *
	 * **PUBLIC BECAUSE AN APPEND DROPS THEM AGAIN.** `AppendMesh` brings the per-vertex array across
	 * and not the overlay, so a mesh built out of two coloured ones arrives at the asset writer
	 * uncoloured — which is how the x-ray shipped grey while the wireframe it is half made of came
	 * out right. Anything that combines two of these meshes calls this afterwards.
	 */
	static void CarryVertexColoursToOverlay(UDynamicMesh* Out);

	/**
	 * Add `From` to `Into` keeping both meshes' colours — what `AppendMesh` does not do, and the
	 * reason the x-ray shipped grey twice.
	 */
	static void AppendColoured(UDynamicMesh* Into, UDynamicMesh* From);

	/**
	 * `bCarryColours` false leaves the colours in the mesh's PER-VERTEX array and writes no overlay.
	 *
	 * **IT EXISTS BECAUSE THE CARRY HAS TO HAPPEN ONCE AND LAST.** A mesh that is going to be
	 * combined with another must not have an overlay yet: appending vertices afterwards leaves the
	 * overlay describing fewer vertices than the mesh has, and an asset written from that reads a
	 * colour set that does not cover it. So a picture written on its own carries here, and a picture
	 * built out of two carries after the join.
	 */
	static bool BuildBoneMesh(const struct FGcRigSkeletonPlan& Plan, UDynamicMesh* Out,
		double LeafStubLength, FString& OutRefusal, bool bCarryColours = true,
		const TMap<int32, FLinearColor>* ColourOverrides = nullptr);

	/**
	 * **EVERY REGION'S OWN EDGES, IN ITS OWN COLOUR — feed straight to `BuildEdgeMesh`.** A region is
	 * shown by its LINES rather than as a painted blob (user-ruled 2026-09-16), because a blob hides
	 * the surface it is describing, and because `gc_render` already draws what you cannot otherwise
	 * see as coloured wireframe.
	 *
	 * **WHERE TWO REGIONS OVERLAP, THE SMALLER WINS** — ledger row 43, the user's own ruling about
	 * which of two overlapping regions governs. Honoured here rather than left to whichever was
	 * declared first, so the picture agrees with what the build will actually do.
	 */
	static bool RegionEdgeColours(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		UDynamicMesh* Mesh, TMap<int32, FLinearColor>& Out, FString& OutRefusal);
};
