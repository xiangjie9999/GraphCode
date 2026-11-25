// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UObject;

/**
 * **THE SETUP THAT CAN NEVER WORK IN THE GAME, SAID WHEN IT IS WRITTEN AND WHEN IT IS READ.**
 * Package K8 of `Docs/GC-GAP-REPORT-1113-PLAN.md` (the lead's cut, need 3; cards P012, P059, P113,
 * P128, P065, P005, P016, P096).
 *
 * Every function here is ADVISORY. It reads the loaded asset, changes nothing, never fails a write,
 * and returns zero or more whole sentences. Each sentence names the asset, the component or node,
 * what will not work in the game, and the usual fix, in words an owner new to Unreal can follow.
 * Each one is silent whenever it cannot be certain: a note that is sometimes wrong teaches people to
 * skip every note.
 *
 * In this header because the live read (`gc_read_live`, GraphCodeMCP) prints them too, and that
 * module cannot include a Private header of this one.
 */
namespace GraphCodeSetupNotes
{
	/**
	 * 1 (P012, P113, P128). A Character Blueprint whose own Mesh (`CharacterMesh0`, what
	 * `ACharacter::GetMesh()` returns) has no skeletal mesh, while another SkeletalMeshComponent of
	 * the class has a mesh and an AnimClass.
	 */
	GRAPHCODE_API TArray<FString> CharacterMeshIsNotTheBody(const UBlueprint* Blueprint);

	/**
	 * 2 (P059). An Anim Blueprint whose class-default Root Motion Mode neither extracts nor locks a
	 * clip its graph plays directly (sequence players and evaluators, blend space samples), or a
	 * montage it names, that has root motion enabled.
	 */
	GRAPHCODE_API TArray<FString> RootMotionNeitherExtractedNorLocked(const UBlueprint* Blueprint);

	/** The same check with the mode given rather than read from the class default (an int holding
	 *  `ERootMotionMode::Type`). The overload above calls this; specs call it for the modes the
	 *  class default cannot be set to without compiling an Anim Blueprint. */
	GRAPHCODE_API TArray<FString> RootMotionNeitherExtractedNorLocked(const UBlueprint* Blueprint, int32 RootMotionMode);

	/**
	 * 3 (P065). A skeletal mesh component with a physics asset and a non-uniform relative scale; and
	 * an anim graph Transform (Modify) Bone node that applies a scale, in an Anim Blueprint whose
	 * preview mesh has a physics asset.
	 */
	GRAPHCODE_API TArray<FString> ScaledBodyWithPhysicsAsset(const UBlueprint* Blueprint);

	/**
	 * 4 (P005). An attach call (`K2_AttachToComponent` on an actor or a component, and the deprecated
	 * `K2_AttachTo` / `K2_AttachRootComponentTo`) whose socket is a literal naming a bone (or a mesh
	 * socket on a bone) with no body in the physics asset of the parent component, where that parent
	 * is a component of this same class. Unresolvable parents are silent.
	 */
	GRAPHCODE_API TArray<FString> AttachedToBoneWithoutBody(const UBlueprint* Blueprint);

	/**
	 * 5 (P016). An anim graph Two Bone IK node whose joint target is a bone of its own chain in bone
	 * or parent-bone space, or whose effector is pinned to another bone while neither rotation hold is
	 * on.
	 */
	GRAPHCODE_API TArray<FString> TwoBoneIKUnstableElbow(const UBlueprint* Blueprint);

	/**
	 * 6 (P096). A SkeletalMeshComponent named Face, with a mesh, no AnimClass, and no call anywhere in
	 * the class that sets a leader pose component on it. Silent when any such call's target cannot
	 * be resolved.
	 */
	GRAPHCODE_API TArray<FString> FaceFollowsNothing(const UBlueprint* Blueprint);

	/** All six, in the order above. Empty for anything that is not a Blueprint. */
	GRAPHCODE_API TArray<FString> ForBlueprint(const UBlueprint* Blueprint);
	GRAPHCODE_API TArray<FString> ForAsset(const UObject* Asset);
}
