// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigLimits.h"
#include "GraphCodeRigSkeleton.h"
#include "GraphCodeRigTwist.h"

class UAnimBlueprint;
class UControlRigBlueprint;
class USkeleton;

/**
 * **WHAT MOVES THE BULGE HELPER — the volume driver, user-ruled IN 2026-09-17 and routed twice the
 * same day.**
 *
 * `bulge=` already builds the only bone that can fix volume at a hard fold: a skinned vertex is a
 * weighted average of where each of its bones would put it, so it is trapped inside the shape those
 * answers form while the correct surface is outside it. **Nothing moved that bone**, and a 145
 * degree fold lost about 30% of its volume however the geometry was spent (ledger §0b.3).
 *
 * **A GENERATED CONTROL RIG, ruled 2026-09-17 (row 68).** The first route emitted
 * `Bone Driven Controller` nodes, and its gate found the wall: those read ONE Euler component of a
 * delta, and an `FRotator`'s Pitch is a decomposition bounded at ninety degrees that **turns back**
 * rather than clamping — a knee declared 145 reads 35, so the bulge would deflate as the leg closed.
 * Two of four ordinary limbs land on that axis, a character's leg among them (§12.6). Control Rig
 * compares whole rotations, so a fold is its own angle whichever way the limb runs.
 *
 * **AND THE COST THAT RULED CONTROL RIG OUT FIRST TIME DOES NOT EXIST:** the rig is EMITTED, not
 * hand-authored, so `gc_versions` still governs it and a rebuild still reproduces it (§12.7).
 *
 * ─────────────────────────────────────────────────────────────────────────────────────────────────
 * **TWO ASSETS, AND THE SECOND ONE IS NOT OPTIONAL.** `USkeletalMesh::PostProcessAnimBlueprint` is a
 * `TSubclassOf<UAnimInstance>` and a Control Rig is not one, so the rig is reached through a
 * generated Animation Blueprint holding a single `AnimGraphNode_ControlRig`. That blueprint is the
 * socket, not the logic.
 *
 * **EVERY NODE IS THE ENGINE'S OWN, and that is a product rule rather than a preference.** One
 * custom `FRigUnit` of ours would replace the whole graph and be far easier to write — and it would
 * live in a runtime module, so **every rig this tool generated would need this tool installed to
 * animate.** Everything GraphCode emits is plain engine data that survives the plugin being
 * removed. Stock nodes also leave the rig READABLE: somebody can open it and see what drives what.
 * ─────────────────────────────────────────────────────────────────────────────────────────────────
 */

/** One helper bone, and the fold that pushes it out. Worked out from the file and the rest pose. */
struct GRAPHCODERIG_API FGcRigVolumeDriver
{
	/** The turning bone whose fold drives this. */
	FString DriverBone;

	/** `<DriverBone>_bulge` — the helper `bulge=` derived. */
	FString DrivenBone;

	/** The region whose `bulge=` asked for it, for a note or a refusal to name. */
	FString Region;

	/**
	 * The driver bone's LOCAL rotation at rest, and at the full fold its `range=` declares.
	 *
	 * **TWO POSES RATHER THAN AN AXIS AND AN ANGLE, because that is what the engine interpolates
	 * between.** `RBF Quaternion to Vector` takes a list of *(rotation, value)* and blends by
	 * distance in quaternion space — so the rig is handed *here is rest and the flesh has not
	 * moved; here is the leg folded and the flesh has moved this far*, and every angle between is
	 * the solver's business. **No decomposition anywhere**, which is the whole reason this route
	 * exists.
	 *
	 * Local because that is the space a fold happens in: a bone's rotation against its parent.
	 */
	FQuat RestRotation = FQuat::Identity;
	FQuat FoldedRotation = FQuat::Identity;

	/** The fold the file declared, in degrees, for a note to quote. */
	double FullFold = 0.0;

	/**
	 * Centimetres at full fold, in the DRIVEN bone's parent frame — the turning bone's axes, which
	 * is the space a local translation is stated in.
	 *
	 * **`radius x bulge` by default, `travel=` to override** (rows 67 and 69), where the radius is
	 * the region's own offset from the bone's line.
	 */
	FVector Travel = FVector::ZeroVector;

	/** True where `travel=` named the distance rather than the region's radius deriving it. */
	bool bTravelWasStated = false;
};

/**
 * **A CORRECTIVE: A MORPH WHOSE WEIGHT A JOINT'S FOLD SETS AT RUNTIME** (`morph(follows=elbow_L.fold, range=(30, 140))`,
 * Docs/GC-MORPH-DESIGN.md §3.5, ruling R3), on the same generated post-process Control Rig that drives `bulge=`, so blends,
 * runtime IK and ragdoll bring the shape in too. Stock nodes only (the rule at the top of this file).
 *
 * **THE FOLD IS AN ANGLE, READ EXACTLY, NOT BLENDED**: the bone's length, turned by its current local rotation, is measured
 * against where it lies at rest about the fold axis (`Atan2` of the two projections), plus the bend it rests at, which is its
 * fold from straight; `Remap` with its clamp then makes it 0 at `From` and 1 at `To`. A roll about the bone's own length leaves
 * its length where it is, so a forearm's twist does not read as a fold.
 */
struct GRAPHCODERIG_API FGcRigCorrective
{
	/** The morph, and the curve the rig sets: one name. */
	FString Curve;
	/** The joint whose fold it follows. */
	FString Bone;
	/** The fold axis in the bone's OWN frame, positive the way the joint closes (a turn about it folds the joint further). */
	FVector Fold = FVector::ZeroVector;
	/** A direction square to it in the bone's own frame: the bone's length where that is square enough, the probe the angle is read with. */
	FVector Probe = FVector::ZeroVector;
	/** The fold the joint rests at, degrees from straight (`FGcRigJointLimit::RestBend`). */
	double RestBend = 0.0;
	/** `range=`: weight 0 at `From` degrees of fold from straight, 1 at `To`. */
	double From = 0.0;
	double To = 0.0;
};

class GRAPHCODERIG_API FGraphCodeRigDriver
{
public:
	static TArray<FGcRigTwist> PlanTwists(const FGcRigRestPose& Pose, const FGcRigSkeletonPlan& Skeleton);

	/**
	 * One corrective per `follows=` morph of the rig (its derived twins included), read off the limits the build resolved: the
	 * joint's fold axis and the bend it rests at. A joint the limits give no fold is refused naming the morph; the linker has
	 * already refused one with no `range=`.
	 */
	static bool PlanCorrectives(const FGcRigLinkedRig& Rig, const TArray<FGcRigJointLimit>& Limits,
		TArray<FGcRigCorrective>& Out, FString& OutRefusal);
	/**
	 * One driver per bulge helper the skeleton plan carries.
	 *
	 * **IT REFUSES NOTHING THE SKELETON DID NOT ALREADY ACCEPT.** Every helper in the plan got there
	 * by passing the region and closing-side checks, so anything this cannot work out — a joint that
	 * declared no `range=`, a region whose centre sits on the bone's own line — is a NOTE naming the
	 * region and a driver that is simply not emitted. A rig that bulges without declaring a fold
	 * still builds; it just has nothing to drive the bulge with, and is told so.
	 */
	static bool Plan(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		const FGcRigSkeletonPlan& Skeleton, const TArray<FGcRigJointLimit>& Limits,
		TArray<FGcRigVolumeDriver>& Out, TArray<FString>& OutNotes, FString& OutRefusal);

	/**
	 * The Control Rig itself: a hierarchy off `Skeleton`, and five stock nodes per driver.
	 *
	 * **THE GRAPH IS Forwards Solve -> (read the fold, blend it into an offset, add it to where the
	 * helper already is, set that) per helper**, the writes chained in one execution order. It ADDS
	 * to the incoming translation rather than replacing it, which is ruling 66: a clip's key on a
	 * driven bone means *more than the driver gives, here*.
	 */
	static UControlRigBlueprint* WriteControlRig(const TArray<FGcRigVolumeDriver>& Drivers,
		USkeleton* Skeleton, UPackage* Package, const FString& AssetName, FString& OutError,
		const TArray<FGcRigTwist>& Twists = {}, const TArray<FGcRigCorrective>& Correctives = {});

	/**
	 * The Animation Blueprint that carries it: Input Pose -> Control Rig -> Result, one node.
	 *
	 * **A SOCKET RATHER THAN LOGIC.** It exists because a mesh's post-process slot takes a
	 * `UAnimInstance` subclass and a Control Rig is not one.
	 */
	/**
	 * **ONE LOOSE PATCH, AND WHAT THE SPRING ON IT IS WORTH.** `jiggle=` is the whole of what an
	 * author states; stiffness and damping are derived from it, because *how springy in newton
	 * metres* is not a sentence anybody writes about a belly.
	 */
	struct FGcRigJiggle
	{
		/** The helper bone the spring moves — `<region>_jiggle`. */
		FString Bone;

		/** The region it carries, for anything that has to name it. */
		FString Region;

		/** `jiggle=`, 0..1. */
		double Amount = 0.0;

		/**
		 * The flesh patch this spring stands for, in centimetres — X along the region, Y across.
		 * Zero when the region is a band, whose `wide=` is a fraction of the way round a limb and
		 * cannot become a length without the mesh; the driver then keeps the engine's own body.
		 */
		FVector2D Patch = FVector2D::ZeroVector;
	};

	/**
	 * **THE BODY A LOOSE PATCH IS SIMULATED AS, and the angular spring that keeps `jiggle=`'s
	 * meaning while it changes.**
	 *
	 * `FAnimPhysRigidBody` hardcodes `Mass(1.0f)`, so `BoxExtents` moves the inertia tensor and
	 * nothing else: for unit mass a box's inertia goes as the SQUARE of its size. Sizing a 10 cm
	 * cube up to a 240 cm patch therefore multiplies the inertia by ~576 and, at an unchanged
	 * spring constant, would divide the angular restoring acceleration by the same — turning a
	 * patch that barely moves into one that never comes back.
	 *
	 * So the constant is scaled by the same ratio. What that BUYS is the point: `jiggle=0.6` then
	 * means one softness on a mouse and on an elephant, where today it means whatever the subject's
	 * size happens to make of a fixed 10 cm body. Lifted out of the writer so a spec can read it
	 * without an editor, an anim graph or a skeleton.
	 *
	 * `OutExtents` is the half-extent vector `BoxExtents` takes; `OutAngularScale` multiplies
	 * `AngularSpringConstant`. A zero or degenerate patch leaves both at the engine's default and
	 * returns false, which is how a band says *keep what you had*.
	 */
	static bool FitSpringToPatch(const FVector2D& Patch, FVector& OutExtents,
		float& OutAngularScale);

	/**
	 * Every loose patch this rig declares, read off the skeleton the plan just made.
	 *
	 * **IT READS THE PLAN RATHER THAN THE TEXT, so a patch whose helper was refused or dropped is
	 * not driven.** A spring bound to a bone that does not exist is a node that silently does
	 * nothing, which is this product's named recurring defect.
	 */
	static TArray<FGcRigJiggle> PlanJiggles(const FGcRigLinkedRig& Rig,
		const FGcRigSkeletonPlan& Skeleton);

	static UAnimBlueprint* WriteHost(UControlRigBlueprint* Rig, USkeleton* Skeleton,
		TArray<FString>& OutNotes,
		const TArray<FGcRigJiggle>& Jiggles, UPackage* Package, const FString& AssetName,
		FString& OutError);

	/** The suffixes the two assets take, shared with the build's name list. */
	static const TCHAR* RigNameSuffix();
	static const TCHAR* HostNameSuffix();
};
