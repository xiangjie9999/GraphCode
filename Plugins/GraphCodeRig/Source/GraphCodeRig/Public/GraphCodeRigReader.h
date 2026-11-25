// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * READING A RIG BACK — the step that turns `.gcrig` from a way of AUTHORING a rig into a way of
 * EDITING one.
 *
 * A reference skeleton holds each bone's name, parent, place and exact rest orientation. A rig
 * somebody else made — a Mixamo download, an FBX, a MetaHuman — therefore reads out as a `.gcrig`
 * that rebuilds those relationships, and from there it can be corrected in text, diffed, reviewed
 * and restored (ledger section 7, section 8).
 *
 * **IT NEEDS NO PIXELS AND NO INSTRUMENT**, which is why it was worth doing before the joint proposer:
 * the proposer has to GUESS where joints go on a mesh that has none, and this only has to read what is
 * written down.
 *
 * **WHAT A SKELETON CANNOT TELL YOU IS EVERY REASON THE FORMAT EXISTS**, and the readout says so
 * rather than inventing it: how far a joint may travel, which way it closes, where the skin gathers,
 * which patch of surface belongs to it. A reference skeleton carries none of that — it is a tree of
 * transforms. `orientation=` transcribes that transform only where it differs from the frame the
 * points would derive. Replacing it with `turns_in=` is the deliberate move from measured fact to
 * authored intent.
 *
 * **THE DOOR WAS RULED 2026-09-16: `gc_read kind:"rig"`, source:"live".** The user weighed a verb of
 * its own and a form that writes the file, and chose the read verb *"since then, it's up to the ai to
 * decide what to replace"* — which is the safety as well as the ergonomics. This hands back TEXT and
 * writes nothing, so a second readout can never land on top of the ranges, regions and skin somebody
 * authored onto the first; merging the two is the author's job and always visible.
 *
 * **AND IT IS THE ONLY PLACE THIS PRODUCT RUNS THE PIPE BACKWARDS.** Everywhere else a declaration
 * builds an asset and the `.gc` reflects it. Core carries the seam as `FGraphCodeRigProgram::Read`
 * because the MCP layer may not see this plugin's types.
 */

/** A rig read back out of an asset. */
struct GRAPHCODERIG_API FGcRigReadOut
{
	/** The `.gcrig` text — a file that would rebuild the skeleton it was read from. */
	FString Text;

	int32 Bones = 0;

	/** Bones with no child: where a chain ends. */
	int32 Leaves = 0;

	/**
	 * What the asset could not say, and what was found in it worth knowing.
	 *
	 * **THE FIRST NOTE IS ALWAYS THE SAME ONE** — a skeleton carries no ranges, no regions and no
	 * skin — because a reader who does not know that will think the rig lost something in the trip.
	 */
	TArray<FString> Notes;
};

class GRAPHCODERIG_API FGraphCodeRigReader
{
public:
	/**
	 * Read a `USkeletalMesh` or a `USkeleton` at this package path into `.gcrig` text.
	 *
	 * **A SKELETAL MESH IS THE BETTER INPUT AND BOTH ARE ACCEPTED**: it names a mesh, so the readout
	 * can carry `mesh:` and the file it produces builds something. A skeleton alone reads out as a
	 * file with no mesh — which is exactly what the grammar calls a preset, made to be inherited.
	 */
	static bool Read(const FString& AssetPath, FGcRigReadOut& Out, FString& OutError);

	/**
	 * The same, off a reference skeleton already in hand.
	 *
	 * **CHAINS ARE DERIVED ONE PER LEAF, root to tip, and named after the leaf.** A tree does not say
	 * where one chain ends and the next begins; what it does say is where a path STOPS, and the
	 * grammar's own chains are paths that end at a foot, a hand, a tail tip. Chains that share a
	 * prefix are legal and the linker already says so — *two chains may cross where they agree*.
	 */
	static void FromSkeleton(const FReferenceSkeleton& Reference, const FString& MeshPath,
		FGcRigReadOut& Out, bool bEveryFrame = false);
};
