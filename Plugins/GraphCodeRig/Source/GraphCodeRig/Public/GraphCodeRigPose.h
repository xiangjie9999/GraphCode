// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigLink.h"

/**
 * THE REST POSE: every bone's transform, derived from authored intent or transcribed exactly.
 *
 * `GC-RIG-GRAMMAR.md` §6 is the ruling this file implements, and it is one line long: *a bone points
 * at its child; `roll=` overrides the default up.* Everything here follows from that plus the
 * header's `faces:`. An imported readout instead carries `orientation=` on only the bones whose
 * source frame differs from that derivation; replacing it with `turns_in=` returns that joint to the
 * authored path.
 *
 * **WHY IT IS ITS OWN FILE RATHER THAN THE FIRST PAGE OF THE SKELETON WRITER.** Four outputs need
 * these frames and only one of them is the skeleton: the physics bodies need a bone's length and
 * direction to size a capsule, the ring cut needs the axis it cuts square to, and `.gcmove`'s
 * `swing(knee_B_L, axis=X, …)` needs the very same frame or its axis letter means something else.
 * A frame computed twice is a frame that drifts, and that is the whole argument of ledger row 28.
 *
 * **AND IT NEEDS NO MESH, NO ASSET AND NO EDITOR**, which is what lets the spec next door bend a
 * three-bone leg and read the angles back rather than trusting a `true`.
 */

/** One bone, resolved. The linker's `FGcRigLinkedJoint` says what it may DO; this says where it IS. */
struct FGcRigPoseBone
{
	FString Name;

	/** `INDEX_NONE` for the root, and the linker has already proved exactly one bone has that. */
	int32 ParentIndex = INDEX_NONE;

	/** Component space — the mesh's own centimetres, which is the currency `points { }` is typed in. */
	FTransform World = FTransform::Identity;

	/**
	 * Relative to the parent. **This is the one `FReferenceSkeletonModifier::Add` takes**, and the
	 * reason both are kept is that every check worth writing is easier in one of them than the other:
	 * a bone's length is a local translation's size, and whether a point sits inside the mesh is a
	 * world question.
	 */
	FTransform Local = FTransform::Identity;

	/**
	 * The unit +X this bone was given — the authored length axis, or the imported frame's exact +X.
	 *
	 * Kept rather than recovered from `World` so a refusal can name it, and because the skin half asks
	 * this question far more often than it asks for a whole transform.
	 */
	FVector Direction = FVector::ForwardVector;

	/**
	 * The unit way the chain runs on from this bone, as its points place it — the bone's real length.
	 * The same as `Direction` on an authored bone. On an imported frame it can be any of the frame's
	 * axes or none: Epic's XR hand has +X across each finger, as its hinge (GC-RIG-IMPORTED-MESH.md).
	 * The plane a joint bends in and the axis it twists about are asked of this, never of +X.
	 */
	FVector Along = FVector::ForwardVector;

	/**
	 * **TRUE MEANS NOTHING DOWNSTREAM POINTED THE BONE AND IT TOOK ITS PARENT'S DIRECTION** — a leaf
	 * (grammar §11.3d), or a bone whose several children cancelled. It is a convention, not a
	 * measurement, so it is flagged rather than hidden: a tip that turns out to fold is exactly the
	 * case §11.3d says may want refusing instead.
	 */
	bool bDirectionInherited = false;

	/**
	 * True where the bone stands at its parent's own place: a second hinge of the same joint
	 * (GC-RIG-IMPORTED-MESH.md). It wears its parent's frame unless the file gave it one.
	 */
	bool bAtParent = false;

	/** Zero on a leaf. Centimetres, and what a physics capsule is sized from. */
	double Length = 0.0;
};

/** Every bone of one rig, **parents before children** — the order the linker already walked. */
struct GRAPHCODERIG_API FGcRigRestPose
{
	TArray<FGcRigPoseBone> Bones;

	/** The frame every direction in the file was read against, carried so no consumer re-derives it. */
	FVector Forward = FVector::ForwardVector;
	FVector Up = FVector::UpVector;

	/** What the solve did that a reader of the build should know (a bone turned about its length to fold about one of its axes). */
	TArray<FString> Notes;

	int32 IndexOf(const FString& Name) const;
	const FGcRigPoseBone* Find(const FString& Name) const;
};

class GRAPHCODERIG_API FGraphCodeRigPose
{
public:
	/**
	 * Solve every bone's frame, or refuse by the name of the bone that could not be pointed.
	 *
	 * **IT REFUSES ON A MISSING POINT AND ON A ZERO-LENGTH BONE, and on nothing else.** Both are
	 * defects of the text that only arithmetic can see: the linker lets a bone carry no point because
	 * a preset's bones carry none (grammar §3, `mesh:` absent), and two joints typed at the same
	 * coordinate parse and link perfectly while naming a bone with no direction at all.
	 */
	static bool Solve(const FGcRigLinkedRig& Rig, FGcRigRestPose& Out, FString& OutRefusal);

	/** `faces:-Y` -> `(0, -1, 0)`. */
	static FVector FacingForward(const FGcRigFacing& Facing);

	/** The header's up, which the parser keeps square to the facing so the two can never collapse. */
	static FVector FacingUp(const FGcRigFacing& Facing);

	/**
	 * A bone's orientation: **+X along `Direction`**, the rest of the frame settled by `Up`, then
	 * `RollDegrees` turned about the bone's own length.
	 *
	 * **+X IS THE BONE'S LENGTH BECAUSE THAT IS WHAT THE FORMAT ALREADY SAYS**: `twists=` is *how far
	 * it rolls about its own length*, and UE's own skeletons point down +X, so a `.gcmove` written
	 * against one reads the same on the other.
	 *
	 * **`Up` IS A HINT AND MAY BE PARALLEL TO THE BONE** — a spine on a `faces:+X` creature points
	 * straight up the world's up. When it is, the frame falls back to the axis least aligned with the
	 * bone, which keeps the result continuous rather than flipping the roll by 90° at the pole.
	 */
	static FQuat OrientationFrom(const FVector& Direction, const FVector& Up, double RollDegrees);
};
