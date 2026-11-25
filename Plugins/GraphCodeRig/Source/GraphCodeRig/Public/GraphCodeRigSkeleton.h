// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigPose.h"

class USkeleton;

/**
 * THE SKELETON PLAN: the bone list an asset is actually written from, helpers and all.
 *
 * **WHY IT IS NOT JUST THE REST POSE.** The pose answers *where does each bone the author named
 * stand*. A skeleton holds more bones than that: `twists=(-35, 35)` on a wrist means a forearm that
 * rolls, and a forearm rolls by handing its roll to helper bones down its length — the MetaHuman body
 * measured in ledger §0b carries exactly these, holding all of the calf's and forearm's weight.
 * **Nobody types a helper**, so the count and the placement are derived here (grammar §6), and the
 * bone list that comes out is what `FReferenceSkeletonModifier` is fed.
 *
 * **AND IT IS STILL PURE.** It reads the linked rig and the pose and touches no asset, which is what
 * lets the helper rule be argued against a spec rather than against a character in a viewport. The two
 * jobs that need the editor are next door in the writer: creating the assets, and checking a shared
 * `skeleton=` really contains every bone this plan names.
 *
 * **SOCKETS SPLIT IN HALF HERE, ON PURPOSE.** `saddle = spine_01 [at=(0, 0, 6)]` is an offset in a
 * frame this file already has, so it resolves now. `collar = neck_01 [on=surface]` has to be pushed
 * out to the skin, and there is no skin until the mesh half — so it is planned, marked, and left.
 */

/**
 * What a bone IS — typed rather than flagged, because there are two kinds of derived bone now and
 * *not authored* stopped being an answer the moment the second one existed.
 */
enum class EGcRigHelperKind : uint8
{
	/** A bone a `joints { }` line names. */
	Authored,

	/** `twists=` — a bone that carries a segment's roll, one per 90 degrees of range. */
	Twist,

	/**
	 * `jiggle=` — a bone with a spring on it, carrying flesh that keeps moving after the body stops.
	 *
	 * **IT RIDES A BONE AND ASKS FOR NO JOINT**, which is what separates it from a bulge helper: a
	 * belly wobbles because the body moved, not because a fold closed, and a belly bends at nothing.
	 */
	Jiggle,

	/** `bulge=` — the bone the flesh rolls outward on as the joint closes. */
	Bulge,

	/**
	 * A socket's `bone=` — a leaf standing at the socket's frame for what hangs there, with NO skin, no limit and
	 * no body: what a clip moves to seat a held thing, the hand staying on its wrist (Epic's `weapon_r`; the VR
	 * hand's `palm_r` is one already). `HelperFor` is the socket's name.
	 */
	Attach,
};

/** One bone of the finished skeleton — authored, or derived and saying so. */
struct FGcRigSkeletonBone
{
	FString Name;

	/** `INDEX_NONE` for the root. Indices are into this plan's own array, parents before children. */
	int32 ParentIndex = INDEX_NONE;

	FTransform World = FTransform::Identity;

	/** What `FReferenceSkeletonModifier::Add` takes. */
	FTransform Local = FTransform::Identity;

	/** Authored, or which kind of helper this is. */
	EGcRigHelperKind Kind = EGcRigHelperKind::Authored;

	/**
	 * The bone this one is a helper FOR — whose roll it carries, or whose fold it bulges at. Empty on
	 * an authored bone.
	 *
	 * **KEPT AS A NAME RATHER THAN INFERRED FROM THE PARENT INDEX**, because the mesh half has to ask
	 * *which vertices hand their weight to this helper* and that is a question about the segment the
	 * helper divides, not about the tree.
	 */
	FString HelperFor;

	/** Where along its parent the helper sits, 0 at the parent's head and 1 at its child. */
	double AlongParent = 0.0;

	/**
	 * **THE FLESH PATCH A JIGGLE HELPER STANDS FOR, in centimetres: X is along the region, Y across
	 * it.** Zero on every other bone.
	 *
	 * It is here because the spring has to be SIZED to it and only the planner ever holds the
	 * resolved region. `FAnimPhysBodyDefinition::BoxExtents` defaults to a 10 cm cube, and
	 * `GraphCodeRigDriver` never set it — so a belly 260 cm across and a jowl 6 cm across were
	 * simulated as the same body.
	 *
	 * **WHAT THAT ACTUALLY COSTS IS NARROWER THAN IT LOOKS, and the narrow version is the reason to
	 * fix it.** `FAnimPhysRigidBody` hardcodes `Mass(1.0f)` and never derives mass from the shape's
	 * volume, so the box does not change the LINEAR spring at all — only the inertia tensor, which
	 * for unit mass goes as the square of the size. A 10 cm box under a 240 cm patch is ~576x too
	 * little inertia, so the angular spring snaps the flesh back almost at once and the angular half
	 * of `jiggle=` barely moves. **The defect is not that it wobbles; it is that it does not**, and
	 * that `jiggle=0.6` therefore means a different softness on every subject size.
	 */
	FVector2D JigglePatch = FVector2D::ZeroVector;
};

/** A socket, with the half of its transform that could be worked out without a mesh. */
struct FGcRigSkeletonSocket
{
	FString Name;
	FString Bone;

	/** In the bone's own frame. Identity for one still waiting on the surface. */
	FTransform Relative = FTransform::Identity;

	/** `on=surface` — the mesh half pushes it out along the bone's frame and clears this. */
	bool bNeedsSurface = false;

	/** `up=` in the bone's frame, kept for a surface socket: its +X is measured later, off the skin. */
	FVector Up = FVector::ZeroVector;

	/**
	 * A socket on a socket: the one whose finished frame it keeps, and whose bone `Bone` already is.
	 * Empty for a socket on a bone. Its rotation is read when written, after a surface parent is measured.
	 */
	FString OnSocket;

	/** `at=$point`: where it stands, in reference component space, put into the bone's frame when written. */
	bool bAtPoint = false;
	FVector PointAt = FVector::ZeroVector;
};

/** Every bone and socket of one rig, ready to be written. */
struct GRAPHCODERIG_API FGcRigSkeletonPlan
{
	TArray<FGcRigSkeletonBone> Bones;
	TArray<FGcRigSkeletonSocket> Sockets;

	/** Worth saying and not worth refusing — a twist with no room to put a helper, and the like. */
	TArray<FString> Notes;

	/** Every bone's name in order — what a shared `skeleton=` is checked against. */
	TArray<FString> BoneNames() const;

	int32 IndexOf(const FString& Name) const;
	const FGcRigSkeletonBone* Find(const FString& Name) const;

	/** How many bones no line named. Zero on a rig where nothing twists. */
	int32 DerivedCount() const;
};

class GRAPHCODERIG_API FGraphCodeRigSkeleton
{
public:
	/** The authored bones, then the helpers their twist ranges call for, then the sockets. bKeptSkin: the file keeps an
	 *  already-rigged mesh's skin, where twist helpers are dropped after the plan (`twists=` is a limit only), so a twist
	 *  helper whose name the mesh already carries is that same bone, not a clash. */
	static bool Plan(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		FGcRigSkeletonPlan& Out, FString& OutRefusal, bool bKeptSkin = false);

	/**
	 * **PUT THIS BONE LIST ON THAT `USkeleton`, WHATEVER IT ALREADY HOLDS — and the object is never
	 * replaced.** One door for both cases the writer has, which is the point of it existing at all:
	 * a fresh skeleton and a rebuilt one now take the same path through the same modifier, so the
	 * rebuild cannot drift from the create.
	 *
	 * **THE ORPHANING DEFECT IS WHY (ledger §7e, 2026-09-16).** A rebuild used to rename the standing
	 * `USkeleton` into `/Engine/Transient` and make a new one at its name, so every `UAnimSequence`
	 * built on it followed the old object out of the project — `gc_render` then said *no skeletal mesh
	 * uses its skeleton `/Engine/Transient.SK_Hand_Skeleton`*, and re-running the `.gcmove` to repair
	 * them refused, correctly, because the clip named a skeleton the file did not. Rig, animate, look,
	 * adjust, look again is the ordinary workflow and it broke on the first adjustment. Keeping the
	 * OBJECT and rewriting its bones leaves every one of those references pointing at a real asset at
	 * a real path.
	 *
	 * **IT STRIPS FROM THE END, WHICH IS ALWAYS A LEAF.** A reference skeleton stores parents before
	 * children, so the last raw bone has no children and removing it never re-parents anything. The
	 * root survives the strip because the engine refuses to remove the only root (`FReferenceSkeleton::
	 * Remove`) — it is renamed and re-posed in place instead, which is also what keeps an animation's
	 * root track bound where the name did not change.
	 *
	 * ~~The `USkeleton`'s own `BoneTree` is NOT written here, and that is the status quo rather than a
	 * decision: nothing in this product has ever filled it, and every reader of it is index-guarded.~~
	 * **IT IS WRITTEN HERE NOW, 2026-09-17, AND THE PARAGRAPH ABOVE WAS HALF RIGHT IN THE DANGEROUS
	 * HALF.** Every READER is index-guarded; `USkeleton::SetBoneTranslationRetargetingMode` is not,
	 * so a `retargeting { }` block resolving its bone against the REFERENCE skeleton — which is where
	 * a bone name is supposed to be resolved — held an index that was out of bounds for the array it
	 * then wrote. A skeleton with a short tree is also malformed by the engine's own check in
	 * `MergeBonesToBoneTree`. The sizing is `FGcSkeletonBoneTree::Cover`, in Core beside the axis
	 * record, because Rig writes it and Move reads it. Ledger §11.
	 */
	static void WriteBonesInto(USkeleton* Skeleton, const TArray<FGcRigSkeletonBone>& Bones);

	/**
	 * **HOW MANY HELPERS A TWIST RANGE NEEDS — one per 90° of range, at most three, none at all when
	 * the bone does not twist.**
	 *
	 * A CALL MADE WITHOUT A RULING (grammar §11.3): §6 says the count is derived from the range and
	 * stops there, and no measurement in this tree fixes the constant. What anchors 90° is what the
	 * shipped rigs do — a forearm at about ±35° carries one twist bone and a film-grade body's
	 * segments carry one or two — and the cap at three is a platform budget, not an anatomical claim:
	 * every helper is a bone in the influence count `influences:` is already rationing.
	 */
	static int32 TwistHelpersFor(const FGcRigRange& Twists);

	/**
	 * **THE NAME A REGION'S BULGE HELPER TAKES**, so the weighting can find the bone the plan derived
	 * without the two agreeing by accident.
	 */
	static FString BulgeHelperName(const FString& Bone);

	/** `<region>_jiggle` — named after the REGION, because a bone may carry several loose patches. */
	static FString JiggleHelperName(const FString& Region);

	/**
	 * Where helper `Index` of `Count` sits along its bone, 0 at the head and 1 at the child.
	 *
	 * **UNSET `wring_at=` SPREADS THEM EVENLY** — `i/(Count+1)`, so one helper lands mid-bone and
	 * never on top of a joint. **A STATED ONE IS WHERE THE TWIST IS TAKEN UP BY**, so the last helper
	 * lands exactly on it and the others lead up to it: `w·i/Count`. Both are one expression, neither
	 * carries a magic blend, and the stated form degenerates to the author's own number when they ask
	 * for a single helper.
	 */
	static double TwistHelperAlong(int32 Index, int32 Count, const TOptional<double>& WringAt);
};
