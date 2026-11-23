// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMeshDeformer;
class UPackage;
class USkeletalMesh;

/**
 * **THE FAR SIDE OF `keep_volume=` — a Deformer Graph that creases where a joint folds and stays
 * round away from it.** Ledger §13.11 through §13.15, and the user's own sentence for it:
 * *"the folding area expand the least, the further away, it expands more"*.
 *
 * **IT IS A PER-VERTEX BLEND BETWEEN TWO SKINNING RULES, NOT A SWITCH BETWEEN THEM.** Linear blend
 * skinning creases everywhere and holds volume nowhere — 69.3% of the section retained at a 145
 * degree fold, and eight times the triangles bought 0.8 of a point. Dual quaternion holds volume
 * everywhere and creases nowhere: 99.5%, a bent hose. A knee is neither, and the difference between
 * them is one number per vertex, which `FGraphCodeRigKeepVolume` has already written onto the mesh.
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────
 * **THIS MODULE, BEHIND A BUILD GATE — USER-RULED 2026-09-17, *"graphcode rig module"*.** The
 * ledger had this in Core Extension on the grounds that `DeformerGraph` is an optional engine
 * plugin. The line that actually holds is the one Control Rig already sits on: **a rig's own work
 * belongs to GraphCode Rig, and an optional plugin is answered by a GATE rather than by a second
 * product.** `GraphCodeBuildGates.AddOptionalPlugin` compiles this whole file out where the project
 * does not enable the plugin, which is the same mechanism Groom uses for HairStrands — so a build
 * without it still writes an honest `omits: all@rig` stub, which was the only thing the old ruling
 * was protecting.
 *
 * **EVERY PIECE OF THE GRAPH IS THE ENGINE'S OWN, and that is a product rule rather than a
 * preference.** `DG_Function_LinearBlendSkin`, `DG_Function_DualQuatSkin` and
 * `DG_Function_BlendPositions` ship with the plugin, and the last of them is literally
 * `lerp(PositionA, PositionB, ReadBlendWeight(Index))`. A custom compute kernel of ours would be one
 * node instead of five and was rejected for the reason a custom `FRigUnit` was rejected in §12.9:
 * what this product emits has to keep working with the product uninstalled, and it also has to stay
 * READABLE — somebody can open this graph and see which two skinnings are mixed, and by what.
 *
 * **WHICH WAY THE BLEND RUNS:** the mask is 1 away from a declared fold and falls toward the value
 * `keep_volume=` states across the band, so 1 is dual quaternion (round) and 0 is linear blend
 * (creases). `lerp(A, B, w)` gives A at 0, so **A is the linear branch and B is the dual quaternion
 * one**. Getting this backwards produces a graph that compiles, runs, and inverts the feature.
 *
 * **THE TANGENTS COME OFF THE LINEAR BRANCH, WHICH IS THE OPPOSITE OF WHAT LOOKS OBVIOUS.** Away
 * from a fold one bone dominates a vertex and the two rules give the same directions, so the choice
 * cannot be seen there. AT the fold they differ — and that is exactly where the author asked for a
 * crease, which shades as a crease only if its normals turn the way the surface does. Taking dual
 * quaternion's tangents would round the shading of a fold the mask had just sharpened. Recomputing
 * them from the blended positions is the fuller answer and is a row of its own: `DG_Function_
 * ComputeNormalsTangents` needs the mesh's half-edge buffer, which is a second asset-side demand.
 * ─────────────────────────────────────────────────────────────────────────────────────────────────
 */
class GRAPHCODERIG_API FGraphCodeRigDeformer
{
public:
	/**
	 * Is the engine's `DeformerGraph` plugin in this build at all?
	 *
	 * **A COMPILE-TIME ANSWER, and it has to be asked before anything is planned**, because a build
	 * that cannot write the graph must not take a name for it, must not report one, and must say by
	 * name that `keep_volume=` reached nothing.
	 */
	static bool IsAvailable();

	/**
	 * The sentence a build says where it is not available — **one place, because two copies of it
	 * would drift and this is the only thing an author ever sees about it.**
	 */
	static FString UnavailableReason();

	/** `DG_` — the engine's own prefix for a deformer graph, so a content folder reads as one. */
	static const TCHAR* NamePrefix();

	/**
	 * Build the graph into `Package` as `AssetName`, wired and COMPILED.
	 *
	 * **IT COMES BACK UNSAVED AND UNSTAMPED, which is the Control Rig writer's arrangement and for
	 * its reason:** the caller owns the provenance stamp and the save, so a graph saved before it
	 * was stamped would be an asset on disk that nothing claims — and the next build would refuse
	 * to replace its own work.
	 *
	 * Null with `OutError` set on any failure; nothing is left half-wired, because every piece the
	 * graph needs is asked for before the asset is created.
	 */
	static UMeshDeformer* Write(USkeletalMesh* Mesh, UPackage* Package, const FString& AssetName,
		int32& OutNodes, FString& OutError);
};
