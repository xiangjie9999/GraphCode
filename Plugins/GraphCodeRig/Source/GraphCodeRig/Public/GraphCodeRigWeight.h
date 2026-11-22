// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigRegion.h"

class UDynamicMesh;
class USkeleton;
struct FGcRigSkeletonPlan;
struct FGcRigRestPose;
struct FGeometryScriptSmoothBoneWeightsOptions;

/**
 * THE REGION WEIGHTING — what a region SAYS, reaching the weights the engine computed.
 *
 * **IT MODULATES; IT DOES NOT COMPUTE.** `ComputeSmoothBoneWeights` already lays a smooth hand-over
 * over the whole surface, and it is good at that. What it cannot know is that a knee's back gathers
 * where its front stretches, or that a kneecap stays with the thigh while the shin turns under it —
 * which is exactly what `skin { }` was written to say. So this pass runs AFTER the bind and edits
 * what it produced, rather than standing in for it.
 *
 * **IT MOVES WEIGHT AMONG A BONE, ITS PARENT AND THE HELPER THE REGION ITSELF DERIVED, AND NOWHERE
 * ELSE — and that is what makes it safe to run over a finished bind.** A vertex's pair holds whatever
 * share of it the engine gave them; this pass changes how that share splits and how much of it the
 * bulge helper takes off the top, never the total. Nothing renormalises, no bone outside the region's
 * own three is touched, and a vertex the pair barely reaches barely moves — which is the correct
 * behaviour at a region's far edge and falls out of the method rather than needing a guard.
 *
 * **THE PAIR IS THE TURNING BONE AND ITS PARENT, because a fold is owned by the child** (grammar
 * §11.3aa): `knee_B_L` is the shin and the knee's crease is at its head. A region names its bone
 * through `turns_in=`, so the pair is already written down.
 *
 * **THE FADE ACROSS A REGION'S EDGE IS `FGraphCodeRigRegion::Falloff`, NOT A SECOND ANSWER**
 * (ledger row 43). A hard edge would put a kink exactly where the skin bends, and a weighting that
 * faded differently from the picture would let a render show a cure working.
 *
 * **`bulge=` IS A BONE BEFORE IT IS A WEIGHT**, so the skeleton plan derives the helper and this pass
 * hands it skin: `bulge x fade` of the pair's total, taken off the top, leaving the hand-over the
 * other three keys shaped exactly as they shaped it. The helper hangs off the turning bone, so a
 * vertex given to it still follows the limb — what it adds on top is the roll, and driving that is a
 * Control Rig's job, the same division every shipped corrective uses.
 *
 * **WHAT IS NOT HERE.** `density=` is the re-lay's, and only a `[relay]` region may state one;
 * `crowd=` and `wedge=` place rings and belong to the cut (grammar §12.10a).
 */

/** One region's share of the pass, or what it would have done. */
struct GRAPHCODERIG_API FGcRigWeightResult
{
	/** Regions whose keys actually moved weight. */
	int32 Regions = 0;

	/** Vertices whose split changed by more than the reporting floor. */
	int32 Vertices = 0;

	/**
	 * Vertices a region took back from a bone outside its own family, and how much skin that was
	 * in total — the ear flap the chest had a fifth of (below).
	 */
	int32 Reclaimed = 0;
	double ReclaimedWeight = 0.0;

	/**
	 * Worth saying out loud: a key asked for on a region with no hand-over to shape, a profile that
	 * names nothing. **Reported BY NAME rather than silently passed over** — a knob that looks set and
	 * does nothing is this product's recurring defect (`density=`, grammar §12.10a).
	 */
	TArray<FString> Notes;
};

class GRAPHCODERIG_API FGraphCodeRigWeight
{
public:
	/** Distribute each carrier's existing skin along its twist stations after region shaping.
	 * Other bones and total weights are preserved. Where a vertex would then hold more bones than the cap,
	 * its smallest weights are pruned and the rest renormalised, as the cap promises; a prune that would
	 * drop a weight above 5% is refused before writing, naming the cap that would keep it (2026-10-03).
	 * What was pruned is said in OutNotes. */
	static bool SpreadTwists(UDynamicMesh* Mesh, const FGcRigRestPose& Pose,
		const FGcRigSkeletonPlan& Plan, int32 Influences, int32& OutVertices, FString& OutError,
		TArray<FString>* OutNotes = nullptr);

	/**
	 * **THE ROOT IS A HANDLE, NOT A BONE, SO IT IS TAKEN BACK OFF THE SKIN.**
	 *
	 * `ComputeSmoothBoneWeights` weights every bone in the hierarchy and has no notion of one that is
	 * there only to be moved. A root sits on the floor by universal convention, so its fan is a long
	 * segment from the origin up to the pelvis that runs **through the middle of the subject** — and
	 * under any distance metric that is the nearest thing to a great deal of the ventral surface.
	 * Measured on the elephant (ledger §19.12): 51 units from the belly where the nearest anatomical
	 * bone was 212.8, taking **76.3%** of it, and the largest single influence on the flank too.
	 *
	 * Each vertex's root share is removed and the remainder renormalised, so nothing else's *relative*
	 * split changes. `OutStranded` counts vertices that held the root and nothing else — those keep
	 * it, because a vertex with no weight at all follows nothing and collapses to the origin.
	 */
	static bool ReleaseRootFromSkin(UDynamicMesh* Mesh, int32& OutReleased,
		int32& OutStranded, FString& OutError);

	/**
	 * A socket's own bone (`bone=`, EGcRigHelperKind::Attach) wears no skin: every weight the bind gave one goes to the
	 * bone it hangs on, so the hand bends exactly as it would without it and a clip moving it moves only what is held.
	 */
	static bool ReleaseAttachBonesFromSkin(UDynamicMesh* Mesh, const FGcRigSkeletonPlan& Plan, int32& OutVertices,
		FString& OutError);

	/** Bind without bulge/jiggle branches extending the anatomical bone fans, or second hinges; restore the full bone map by name. */
	static bool BindWithoutRegionHelpers(UDynamicMesh* Mesh, USkeleton* Skeleton, const FGcRigSkeletonPlan& Plan,
		const FGeometryScriptSmoothBoneWeightsOptions& Options, int32& OutExcluded, FString& OutError);

	/**
	 * A stack of second hinges (bones at their parent's own place: a thumb's base, HANDS.md Step 2) on a skin made here: the
	 * skin of the bone the stack hangs from and of every hinge but the last goes to the stack's deepest hinge, the one every
	 * hinge's turn carries. Run after every other weight writer, as a kept skin's hinge takes its parent's weights.
	 */
	static bool HandStacksToLastHinge(UDynamicMesh* Mesh, const FGcRigSkeletonPlan& Plan, int32& OutVertices, FString& OutError);

	/** A second hinge (GraphCodeRigPose.cpp): an authored bone standing at its parent's own place, one more way that joint turns. */
	static bool IsSecondHinge(const FGcRigSkeletonPlan& Plan, int32 Index);

	/**
	 * **HOW MANY BONES SHARE THE AVERAGE VERTEX — the whole-field number the rig had no instrument
	 * for**, and the direct answer to ledger §19.11: every measurement this product had was local
	 * and differential, each passed, and the user still called the subject jelly in four words.
	 *
	 * The statistic is the participation ratio `1 / Σw²`, which is what "effectively how many"
	 * means for a normalised set of shares. It is **1.0 when one bone owns a vertex outright** and
	 * rises toward `influences:` as the share spreads. It is not a quality score and must not be
	 * read as one: a trunk wants ~2 and a shoulder legitimately wants more. What it IS good for is
	 * **comparing one build of a subject against another build of the same subject** — which is
	 * exactly the comparison nobody could make while the bind had no reported properties at all.
	 *
	 * `OutWorst` is the softest single vertex, and `OutWorstAt` where it is, so a number that looks
	 * wrong leads somewhere rather than just being alarming.
	 */
	static bool MeasureBindSoftness(UDynamicMesh* Mesh, double& OutMean, double& OutWorst,
		FVector& OutWorstAt, FString& OutError);

	/**
	 * Shape the hand-over every region asked for, over a mesh that has already been bound.
	 *
	 * **THE BONE MAP COMES OFF THE MESH, NOT OFF THE SKELETON AND NEVER OFF THE PLAN'S ORDER.** A
	 * weight carries the index `CopyBonesFromSkeleton` gave the MESH, so the mesh's own bone attribute
	 * is the numbering those indices are in — `GetAllBonesInfo` hands back the name and the parent of
	 * every one of them (ledger section 7, the door nobody had looked through). Deriving the same map
	 * a second way is how this tree's index faults have started before, and it needs no skeleton here
	 * at all.
	 */
	static bool Apply(UDynamicMesh* Mesh, const FGcRigLinkedRig& Rig,
		const TArray<FGcRigRegionSpan>& Spans, FGcRigWeightResult& Out, FString& OutError,
		const FGcRigSkeletonPlan* Plan = nullptr);

	/**
	 * **HOW MUCH OF EACH OF A PATCH'S VERTICES THE REGION OWNS OUTRIGHT — 1 across the patch, falling
	 * to 0 only at an edge the skin actually crosses.**
	 *
	 * **A CLAIM FADES TOWARD A BORDER, AND A PATCH'S EDGE IS ONLY A BORDER WHERE THE SKIN CARRIES ON
	 * PAST IT.** The reason to let go at all is that the vertex just outside keeps its foreign bone,
	 * and a step between neighbours is the shard. Where the MESH ran out before the region did there
	 * is nothing outside to step against — so a patch holding a whole piece of skin, a flap or a tail
	 * tip, is claimed entire however its slab happens to fall. Each of the three edges is asked
	 * separately, because a patch can run off the end of a sheet on one side and be cut on another.
	 *
	 * **THIS IS NOT `FGraphCodeRigRegion::Falloff`, AND THE MEASUREMENT IS WHY.** That fade is the
	 * shape of an AMOUNT, and row 43 is right about amounts. Ownership is not one: a bone with no
	 * business on this skin has none of it in the middle and none at the edge, and the fade — a
	 * product of two linear tapers — leaves about three quarters of it behind everywhere. On the
	 * elephant that read as `ear_fan_L` still holding foreign skin on 424 of its 650 vertices after a
	 * pass that reported 788 vertices moved and was believed.
	 *
	 * **AND IT IS NOT A WIDER SPAN EITHER, WHICH IS THE PART THAT TOOK FOUR BUILDS TO LEARN.** A taper
	 * read off the SLAB puts its zero wherever the slab's edge falls, which on the ear was down the
	 * middle of a swinging sheet: `wide=140` cut a flap that wraps 195 cm of arc, and each ear came
	 * apart into three long blades at 40 degrees of flap. Widening the sector moved the tear rather
	 * than closing it — 400 left the tip torn at 107.9 degrees of a 110.7-degree half-angle, and 520
	 * pushed the worst residual further out again. **Chasing an edge outward is the tell that the edge
	 * was never the right thing to fade toward.**
	 *
	 * The band is the same tenth of the patch a region fades its amounts over. **A narrower one was
	 * tried and photographed**: a tenth of that, about one and a half rings of vertices on this mesh,
	 * is a step rather than a blend, and the ear's tip came apart into slivers again.
	 */
	static void MeasureClaim(UDynamicMesh* Mesh, const FGcRigRegionSpan& Span,
		const TSet<int32>& Island, TMap<int32, double>& OutClaimed);

	/**
	 * How a region's keys reshape ONE vertex's share of its pair — the whole of the pass's arithmetic,
	 * lifted out so a spec can read it without a mesh, a skeleton or an editor.
	 *
	 * `Share` is the child's part of the pair, 0 all the parent's and 1 all the child's. `Fade` is how
	 * far inside the region the vertex sits.
	 *
	 * **`crease=` IS THE FRACTION THE TRANSITION BAND IS NARROWED BY, which is why it needs no chosen
	 * constant.** Gathering rather than stretching IS a narrower hand-over: linear blend skinning
	 * stretches the skin across however wide the blend runs, so closing the blend is what makes flesh
	 * gather into a fold. 0.9 leaves a tenth of the band and 1.0 is a step — and a step is the classic
	 * candy-wrapper pinch, which is why this is an amount and not a flag.
	 *
	 * **`slide=` HANDS THE CHILD'S SHARE BACK TO THE PARENT**, so the skin stays where it was while the
	 * bone turns under it — a kneecap, an elbow point. 1 at a region's middle is skin that does not
	 * move with the joint at all, which is what the words say.
	 *
	 * **`falloff=` RESHAPES THE TRANSITION AND IS APPLIED TO IT, not to the result.** The profile is
	 * the SHAPE of the hand-over (grammar §5), so it reads a narrowed share and returns a reshaped one;
	 * `slide=` is a different sentence — whose the skin is — and comes after, unreshaped.
	 */
	static double ShapeShare(double Share, double Fade, const TOptional<double>& Crease,
		const TOptional<double>& Slide, const FGcRigProfile* Falloff);

	/** `profile(0.0 = 0, 0.65 = 0.92, 1.0 = 1)` read at `At`, piecewise-linear, clamped at both ends. */
	static double ReadProfile(const FGcRigProfile& Profile, double At);
};
