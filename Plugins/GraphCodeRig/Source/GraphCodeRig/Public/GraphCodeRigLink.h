// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigDocument.h"

/**
 * THE LINKER: one rig out of a chain of files, and every rule that spans two lines.
 *
 * `GC-RIG-GRAMMAR.md` §2 and §12.2. The parser refuses what is wrong with ONE line; everything that
 * needs a second line to see is here, because a line may be a partial override of its parent's or of
 * its own mirror twin's — so only the merged rig can say whether two lines agree. Splitting these the
 * other way would put half a rule in each of two places.
 *
 * **THREE STAGES, AND THE ORDER IS FORCED.**
 *
 *   1. **MERGE** — the `parent:` chain, one KEY at a time (ledger row 44: *"just inherit the point
 *      then … so you can just use the rigging as a whole without overriding a single parameter"*).
 *      Correcting a knee's `range=` must not drop the `closes=` beside it, which whole-line
 *      replacement would.
 *   2. **MIRROR** — a `_L` name implies its `_R` twin, derived and never written, and **overridden the
 *      moment the file declares `_R` itself**. It runs after the merge because a child may be the file
 *      that declares the twin.
 *   3. **CHECK** — the hierarchy (one root, one parent per bone, no cycle, no orphan) and every
 *      reference (`turns_in=`, `falloff=`, `along=`, a region's centre, a chain's `goal=`).
 *
 * **WHAT IS NOT HERE, AND WHY: anything that needs the MESH.** A joint's point must have a closed loop
 * round it, a region's centre must sit on the skin, and `closes=` must agree with the side its region
 * sits on — three checks ledger rows 44 and 47 ask for, and all three read geometry. They belong to the
 * builder, which has the mesh open. The linker's job is to make sure the TEXT is one consistent rig
 * before a triangle is touched.
 */

/** One bone of the merged rig — `.gcrig`'s subject, and what the skeleton is written from. */
struct FGcRigLinkedJoint
{
	FString Name;

	/** Empty for the root, and exactly one bone may have that. */
	FString Parent;

	/** **A BONE WITH NO POINT IS LEGAL IN A FILE THAT BUILDS NOTHING** — that file is a preset. */
	bool bHasPoint = false;
	FVector At = FVector::ZeroVector;

	FString TurnsIn;
	/** `twists_in=`: the band its roll is spread along and its twist helpers stand in; empty means `TurnsIn`. */
	FString TwistsIn;
	TOptional<FQuat> Orientation;

	/** `points_to=` as written, without the `$`, so a child file can override or inherit it. */
	FString PointsTo;

	/** That name resolved: where this bone aims, when the chain's own child is not the answer. */
	bool bHasAim = false;
	FVector Aim = FVector::ZeroVector;

	FGcRigRange Range;
	FGcRigRange Side;
	FGcRigRange Twists;
	/** `twists_raised=` (FGcRigJoint::TwistsRaised): merged per key, cleared wherever `Twists` is cleared, and DROPPED where a later file
	 * restates `twists=` without it (it was written against the earlier pair); copied whole to a mirror twin. */
	FGcRigRange TwistsRaised;
	EGcRigCloses Closes = EGcRigCloses::Unset;
	TOptional<double> Roll;
	TOptional<double> RestFold;
	TOptional<double> RestSide;
	/** As FGcRigJoint::RestTwist / bRestTwistForward; a derived twist is filled in by FGraphCodeRigLimits::Resolve. */
	TOptional<double> RestTwist;
	bool bRestTwistForward = false;

	/** `velocity=`, `accel=`, `ramp=`: this character's normal full effort at this bone (deg/s, deg/s^2, seconds), merged per
	 * key like `range=` and copied whole to a mirror twin. Unset is unknown; the header's default is read at the record. */
	TOptional<double> Velocity;
	TOptional<double> Accel;
	TOptional<double> Ramp;
	/** `twist_velocity=`, `twist_accel=` (FGcRigJoint): the twist's own reference, merged per key and copied to a mirror twin like `velocity=`. */
	TOptional<double> TwistVelocity;
	TOptional<double> TwistAccel;

	/** `dynamic=`: how freely this bone's physics body moves, 0..1 (FGcRigJoint::Dynamic). Merged per key like `velocity=` and copied
	 * whole to a mirror twin; unset is a joint that declares nothing, which the physics asset writer leaves exactly as it was. */
	TOptional<double> Dynamic;

	/**
	 * **DERIVED, NEVER WRITTEN** (GC-RIG-BIND-OWNERSHIP-PLAN.md, Rev 3, user-ruled 2026-10-06): the skin past
	 * `TurnsIn`'s band belongs to this joint and the bones below it. True when the merged joint names a band and NO
	 * OTHER linked joint names the same one, counted after the mirror stage so a twin owns its own twin band. A band
	 * several joints share is a bend zone across them with no one hand-over point, so those joints own nothing and
	 * keep the bind's blend; so does a joint with no band, and one with an `Orientation` (which cannot sit beside
	 * `turns_in=`). The old word `owns_skin` is retired: the parser refuses it.
	 */
	bool bOwnsSkin = false;

	/** Derived from a `_L` twin rather than written. Never round-tripped into a file. */
	bool bMirrored = false;

	/** A bare authored `joints { name }` line, retained across inheritance even when it overlays an imported frame. */
	bool bExplicitBare = false;

	/** The line it was last written on, for a refusal. */
	int32 Line = 0;

	bool HasLimits() const { return Range.bSet || Side.bSet || Twists.bSet; }
};

/**
 * A point of the merged rig, in the leaf mesh's own centimetres.
 *
 * **KEPT AS ITS OWN TABLE EVEN THOUGH A JOINT CARRIES ITS OWN POSITION**, because a point nothing is
 * named after is a region's centre, and two regions may share one (grammar §4). The builder needs to
 * look one up by name; a joint's copy is a convenience for the skeleton walk.
 */
struct FGcRigLinkedPoint
{
	FString Name;
	FVector At = FVector::ZeroVector;
	bool bMirrored = false;
	int32 Line = 0;
};

/** Where a patch of skin is — merged, and mirrored. */
struct FGcRigLinkedRegion
{
	FString Name;
	FString Centre;
	FGcRigScalar Long;
	FGcRigScalar Wide;
	TArray<FString> Along;
	TOptional<double> Tilt;
	bool bMirrored = false;
	int32 Line = 0;
};

/**
 * How it behaves — **a WHOLE VALUE, and a child's line replaces its parent's entirely**
 * (user-ruled 2026-09-16, ledger row 58). A key the child omits is OFF, not inherited, and a bare
 * name is the spelling for *nothing at all*. `FGcRigLinkedJoint` still merges key by key, because a
 * joint's `turns_in=` is its only tie to the region it moves in.
 */
struct FGcRigLinkedSkin
{
	FString Name;
	TOptional<double> Crease;
	TOptional<int32> Folds;
	TOptional<double> Bulge;
	TOptional<double> Travel;
	/** How far, in cm, a held thing may press into this region's skin — `gives=`. */
	TOptional<double> Gives;
	TOptional<double> MaxStep;
	TOptional<double> Slide;

	/** How much this flesh keeps moving after the bone stopped — `jiggle=`, 0..1. */
	TOptional<double> Jiggle;

	/** How much of its thickness the limb keeps through the fold — `keep_volume=`, 0..1. */
	TOptional<double> KeepVolume;
	TOptional<int32> Wedge;
	TOptional<double> WringAt;
	TOptional<double> Crowd;
	TOptional<int32> Loops;
	TOptional<FGcRigScalar> Density;
	FString Falloff;
	TOptional<bool> bKeep;
	TOptional<bool> bRelay;
	bool bMirrored = false;
	int32 Line = 0;
};

/**
 * Parentage, and the IK chain's name.
 *
 * **A CHAIN IS A WHOLE VALUE AND A CHILD REPLACES IT** — unlike everything else here, which merges by
 * key. A partial chain has no meaning: `hip -> knee` over a parent's `pelvis -> hip -> knee -> hock`
 * could be a shorter chain or a correction to its middle, and nothing in the text says which. Enhanced
 * Input's lists are whole values for the same reason.
 */
struct FGcRigLinkedChain
{
	FString Name;
	TArray<FString> Joints;
	TOptional<double> Dynamic;
	FString Goal;
	bool bMirrored = false;
	/** From an import's read-out (`FGcRigDocument::bImported`), not written by an author; a replacement takes its own
	 * document's flag. The rest pose lets an authored chain decide a bone's line first. */
	bool bImported = false;
	int32 Line = 0;
};

struct FGcRigLinkedSocket
{
	FString Name;
	FString Bone;
	bool bHasOffset = false;
	FVector At = FVector::ZeroVector;
	bool bOnSurface = false;
	/** `toward=` and `up=`: where the socket's +X and +Z point, in the bone's frame. Zero where unsaid. */
	FVector Toward = FVector::ZeroVector;
	FVector Up = FVector::ZeroVector;
	/**
	 * `up=($heel_r, $web_r)`: the same directions stated by two of the rig's points, without the `$`, and the way
	 * between them in reference component space once the linker has found them. The skeleton plan turns that
	 * into the bone's frame, the one place that knows where the bone stands. Empty where a direction is an axis.
	 */
	FString TowardFrom, TowardTo, UpFrom, UpTo;
	FVector TowardLine = FVector::ZeroVector;
	FVector UpLine = FVector::ZeroVector;
	/** `at=$web_r`: the named point it stands at, without the `$`. Empty where `at=` is numbers or unsaid. */
	FString AtPoint;
	/** That point in reference component space, filled by the linker once it has found it. */
	FVector PointAt = FVector::ZeroVector;
	/**
	 * `bone=grip_r`: the socket stands on a bone of its own, a skinless leaf on `Bone` at the socket's finished
	 * frame, so a clip can move what hangs on it (Epic's `weapon_r`). Empty where the socket rides `Bone` itself.
	 */
	FString OwnBone;
	bool bMirrored = false;
	int32 Line = 0;
};

/**
 * ONE RIG, from one file or from a chain of them.
 *
 * Joints come out **parents before children**, because that is the order a `FReferenceSkeletonModifier`
 * has to be handed them in and there is no reason to make the builder sort what the linker already
 * walked.
 */
struct GRAPHCODERIG_API FGcRigLinkedRig
{
	FGcRigHeader Header;
	TArray<FGcRigBuildLine> Build;
	TArray<FGcRigProfile> Profiles;
	TArray<FGcRigLinkedPoint> Points;
	TArray<FGcRigLinkedJoint> Joints;
	TArray<FGcRigLinkedRegion> Regions;
	TArray<FGcRigLinkedSkin> Skin;
	TArray<FGcRigLinkedChain> Chains;
	TArray<FGcRigLinkedSocket> Sockets;
	TArray<FGcCharacterControl> Controls;
	TMap<FString, bool> ControlOverrides;
	TArray<FGcCharacterContact> Contacts;
	TArray<FGcCharacterGrip> Grips;

	/**
	 * `morphs { }`, merged by name as whole values (GC-MORPH-DESIGN.md §3.1). Each is also a control of its own name,
	 * `curve.<Name>=(0, 1)`, added to `Controls` by the linker unless `controls { }` declares one playing that curve
	 * (ruling R2), so a clip reaches it with `drive(<Name>, by=…)` through either route a control travels.
	 */
	TArray<FGcRigMorph> Morphs;

	/** The symmetry plane's normal, from `faces:` or from `mirror_axis:`. Zero when mirroring is off and the file
	 * declares neither — `mirror:false` alone stops the twins, not a midline the file stated. */
	FVector MirrorNormal = FVector::ZeroVector;

	/**
	 * **THE LEAF NAMES A MESH, SO THIS RIG IS MEANT TO BUILD** — and that is what decides how hard the
	 * reference checks bite. A preset's `turns_in=$knee_back_L` names a region the preset does not
	 * place, on purpose (ledger row 47); the same dangling name in a file that builds is a defect.
	 */
	bool bBuilds = false;

	/** Worth saying and not worth refusing. A rigid bone with a region, a socket on a leaf. */
	TArray<FString> Notes;

	const FGcRigLinkedPoint* FindPoint(const FString& Name) const;
	const FGcRigLinkedJoint* FindJoint(const FString& Name) const;
	const FGcRigLinkedRegion* FindRegion(const FString& Name) const;
	const FGcRigLinkedSkin* FindSkin(const FString& Name) const;
	const FGcRigLinkedChain* FindChain(const FString& Name) const;
	const FGcRigLinkedSocket* FindSocket(const FString& Name) const;
	const FGcRigMorph* FindMorph(const FString& Name) const;
};

class GRAPHCODERIG_API FGraphCodeRigLinker
{
public:
	/**
	 * Merge, mirror and check.
	 *
	 * `Chain` is **root-first**: the outermost `parent:` at index 0 and the file itself last. Resolving
	 * a `parent:` path to text is the caller's job, because the linker reads no assets — which is what
	 * lets every rule below be proved without one.
	 */
	static bool Link(const TArray<FGcRigDocument>& Chain, FGcRigLinkedRig& Out, FString& OutRefusal);

	/** One document, the common case. */
	static bool Link(const FGcRigDocument& Document, FGcRigLinkedRig& Out, FString& OutRefusal);

	/**
	 * `knee_B_L` -> `knee_B_R`, and empty for a name with no side.
	 *
	 * **SUFFIX ONLY, AND `_L`/`_R` ONLY** — this tree's convention and the Pudu's. Whether UE's own
	 * lower-case `_l`/`_r` should also be read is open (grammar §11.4), and it is one line here when it
	 * is answered.
	 */
	static FString MirrorName(const FString& Name);
};
