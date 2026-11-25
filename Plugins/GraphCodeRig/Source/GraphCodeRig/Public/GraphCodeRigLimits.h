// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigProgram.h"
#include "GraphCodeRigRegion.h"

/**
 * WHAT `range=` AND `twists=` ACTUALLY CONSTRAIN — the numbers reaching something that enforces them.
 *
 * **THEY REACHED ONE THING AND IT WAS NOT A LIMIT.** `range=(-5, 145)` was read once, to work out how
 * many rings a bend needs, and then dropped: the physics asset came out with the engine's default
 * limits and the IK rig with none at all. So a knee declared to travel 150 degrees could be bent to
 * 300 by anything that bent it. **User-ruled 2026-09-16, and the argument is one line:** *"or else why
 * do we even have maximum and minimum rotation?"*
 *
 * **A LIMIT IS AN ANGLE ABOUT AN AXIS, AND THE AXIS IS THE WHOLE DIFFICULTY.** The engine states
 * limits per bone about that bone's own X, Y and Z. This format never types an axis — a hinge's axis
 * is a fact about the geometry, derived from the chain that runs through it (the ruling that removed
 * `bend=X`). So the job here is to take a derived direction and say which of three named axes it IS,
 * **and to say so out loud when it is not really any of them.**
 *
 * **A LIMIT ON THE WRONG AXIS IS SILENT AND IT IS THE DANGEROUS CASE.** A knee whose limit landed on
 * its twist axis has an unconstrained bend and a clamped roll; it looks perfect at rest and is wrong
 * the moment anything moves it. That is the failure this project has already paid for once, so the
 * mismatch is measured in degrees and reported rather than rounded away.
 */

/** Which of a bone's own three axes a limit is about. */
enum class EGcRigLimitAxis : uint8
{
	AlongTheBone,   // the bone's own length: what `twists=` turns about
	Across,         // the fold axis for a hinge: what `range=` opens about
	Sideways,       // the third: what `side=` allows
};

/** One bone's limits, worked out from the file and the pose. No asset involved. */
struct GRAPHCODERIG_API FGcRigJointLimit
{
	FString Bone;

	/**
	 * Which of the bone's own axes each stated pair turns about — X, Y or Z as the engine numbers
	 * them, chosen by asking which one the derived direction is closest to.
	 */
	int32 FoldAxis = INDEX_NONE;
	int32 TwistAxis = 0;

	/** The fold's travel in degrees, already turned round if the axis it landed on points the other way. */
	double FoldLow = 0.0;
	double FoldHigh = 0.0;
	/** Bind bend used for straight-relative authored angles. */
	double RestBend = 0.0;
	/** Default animation offset, independent of the mesh's bind and enforcement frames. */
	FQuat RestRotation = FQuat::Identity;
	/** The twist RestRotation carries, stated (`rest=(twist=N)`) or worked out (`rest=(twist=forward)`), for the record and the notes. */
	TOptional<double> RestTwistDegrees;
	bool bHasFold = false;

	/** The twist's travel in degrees, about the bone's own length (never turned round). */
	double TwistLow = 0.0;
	double TwistHigh = 0.0;
	bool bHasTwist = false;

	/**
	 * The bone's length in its own frame, which is what a twist turns about, before it was snapped to
	 * `TwistAxis`. An imported frame, or a second hinge wearing its parent's, need not run along any of
	 * its axes (a thumb's roll at its base). `bTwistTurnedRound` is true where `TwistAxis` points
	 * against it, so a pair stated about that axis is the twist's pair turned round.
	 */
	FVector TwistLocal = FVector::ZeroVector;
	bool bTwistTurnedRound = false;

	/**
	 * `side=` — the third axis, the one that is neither the bone's length nor its fold.
	 *
	 * **ITS SIGN IS THE ONE THING HERE THAT IS NOT A MEASUREMENT.** A fold's positive half is the way
	 * the joint is already bent, which the rest pose shows; nothing leans sideways at rest, so there is
	 * no such tell. A symmetric pair does not care, and an asymmetric one is reported.
	 */
	int32 SideAxis = INDEX_NONE;
	double SideLow = 0.0;
	double SideHigh = 0.0;
	bool bHasSide = false;

	/**
	 * **HOW FAR THE DERIVED FOLD DIRECTION IS FROM THE AXIS IT WAS WRITTEN ONTO.** Zero is a clean
	 * hinge. Anything large means the bone's own frame does not line up with the way this joint really
	 * folds, and the limit is an approximation somebody should know about.
	 */
	double DegreesOff = 0.0;

	/**
	 * **THE FOLD DIRECTION ITSELF, IN THE BONE'S OWN FRAME, BEFORE IT WAS SNAPPED TO AN AXIS** —
	 * already turned round so it points the way the positive half of the range travels.
	 *
	 * **SNAPPING IS RIGHT FOR A NAME AND WRONG FOR A FRAME.** `FoldAxis` exists because a `.gcmove`
	 * says `axis=fold` and an IK rig names one of three axes, and neither can take a vector. A
	 * physics constraint can: its reference frame is an arbitrary rotation, and writing the snapped
	 * axis into it threw away a measurement for no reason. An elephant's ear folds 43 degrees off
	 * its nearest axis, so it was being limited about a hinge it does not turn on.
	 *
	 * Zero where no fold was derived. `DegreesOff` is the angle between this and `FoldAxis`.
	 */
	FVector FoldLocal = FVector::ZeroVector;

	/** True where the fold was flipped to sit on the positive side of the axis it landed on. */
	bool bTurnedRound = false;
};

/**
 * **ONE JOINT'S `twists_raised=`, AS THE IK RIG CARRIES IT FOR A `.gcmove`** (MOVE-CUT1, Saved/Research/MOVE-CUT1/SHOULDER-TWIST-ELEVATION-DESIGN.md
 * sections 2 R6, 3 and 4). The IK rig's own limit keeps `twists=`, the arm hanging, so every native reader (an AnimBP IK, a retarget) stays
 * where it was; this pair travels beside it as one package-metadata key on the IK rig asset (never the skeleton: a MetaHuman's is shared),
 * read by Move's own reader (GraphCodeMoveProgramSolve.cpp, GcMoveSolve::TwistsRaisedFromText), which spells the same key and the same text:
 * change both together, as `GraphCodeRig.Dynamic` is kept.
 */
struct GRAPHCODERIG_API FGcRigTwistsRaisedEntry
{
	FString Bone;
	/** The file's `twists_raised=` pair, as written. */
	double Low = 0.0;
	double High = 0.0;
	/** The IK rig axis the twist is limited about (FGcRigJointLimit::TwistAxis) and the raised pair on it as the full-body solver's Min/Max
	 * read it: turned round with the axis, Y read about the negative axis, clamped round the rest and held at the float the asset stores, as
	 * the hanging pair is written (GraphCodeRigBuild.cpp, the IK rig's `Put`). */
	int32 Axis = 0;
	double NativeLow = 0.0;
	double NativeHigh = 0.0;
	/** The bone's long axis in its own bind frame, the direction its twist turns about (the record's twist axis), unit length. */
	FVector Along = FVector::ZeroVector;
	/** The joint's grandparent (an upper arm's spine_05, the clavicle's parent): the trunk whose bind-pose -Z is "down" for its elevation. */
	FString Trunk;

	/** `GraphCodeRig.TwistsRaised`: the metadata key, one key holding every entry, replaced whole and removed where the file has none. */
	static const TCHAR* Key();
	/** `bone=low,high,axis,nativelow,nativehigh,alongx,alongy,alongz,trunk` joined with ';', sorted by bone (case-sensitively); each
	 * number printed so it parses back to the same double (the `GraphCodeRig.Dynamic` rule). Empty for none. */
	static FString ToText(const TArray<FGcRigTwistsRaisedEntry>& Entries);
	static bool FromText(const FString& Text, TArray<FGcRigTwistsRaisedEntry>& Out, FString& OutError);
	/** The line an author writes for it: `upperarm_r [twists_raised=(-90, 73.8)]`. */
	FString AsJointLine() const;
};

class GRAPHCODERIG_API FGraphCodeRigLimits
{
public:
	/**
	 * Every joint's `twists_raised=` in the IK rig's terms, from what `Resolve` worked out for its `twists=`. Empty, and true, for a rig
	 * that declares none (every rig before MOVE-CUT1's shoulder slices). Refused where a joint with the key has no limit resolved, no
	 * twist, or no grandparent bone to be its trunk.
	 */
	static bool TwistsRaised(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose, const TArray<FGcRigJointLimit>& Limits,
		TArray<FGcRigTwistsRaisedEntry>& Out, FString& OutRefusal);

	/**
	 * Work out every joint's limits from the file and the rest pose.
	 *
	 * **THE FOLD AXIS COMES FROM THE CHAIN FIRST AND FROM `closes=` ONLY WHERE THE CHAIN IS
	 * STRAIGHT** — the order the grammar already set. A hinge's axis is perpendicular to the plane its
	 * two bones lie in, so `cross(parent direction, own direction)` is it; three bones in a line define
	 * no plane, and that is exactly the case `closes=` exists to answer.
	 *
	 * **AND POSITIVE MEANS FURTHER THE WAY IT ALREADY BENDS**, which is not a convention pulled out of
	 * the air: a knee, a hock and an elbow are all slightly folded at rest, and that fold is the sign.
	 */
	static bool Resolve(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose,
		TArray<FGcRigJointLimit>& Out, TArray<FString>& OutNotes, FString& OutRefusal);

	/**
	 * The direction one joint with a `range=` folds about, in component space: the plane its bone and its parent's lie in, or where
	 * that is straight, the side its region closes on or its `closes=`. Positions and lengths only, never a bone's roll, so the pose
	 * can ask it before its frames are final (FGraphCodeRigPose::Solve turns each authored bone to fold about one of its own axes).
	 */
	static bool FoldDirection(const FGcRigLinkedRig& Rig, const FGcRigRestPose& Pose, const FGcRigLinkedJoint& Joint, int32 Index,
		FVector& OutFold, TArray<FString>& OutNotes, FString& OutRefusal);

	/**
	 * The axis record a `.gcmove` reads, from what `Resolve` worked out: each axis as the direction itself, signed the
	 * way an animator turns it. ONE writer for the record a build leaves on its skeleton and the one a `controls:`
	 * adapter with joints hands Move (GC-MOVE-METAHUMAN-BODY-ADAPTER-PLAN.md item 2) — ledger §7e's sideways knuckles
	 * came from a second derivation of exactly this.
	 */
	static FGcRigAxisRecord ToAxisRecord(const TArray<FGcRigJointLimit>& Limits);

	/**
	 * Which of a bone's own axes a direction is, and how far off it was — the one piece of arithmetic
	 * both the physics asset and the IK rig depend on, kept in one place so they cannot disagree about
	 * which axis a knee bends around.
	 *
	 * Returns 0, 1 or 2 for X, Y or Z; `bOpposite` says the direction points down the axis's negative
	 * half, which is what turns a range round.
	 */
	static int32 NearestAxis(const FQuat& BoneOrientation, const FVector& Direction,
		bool& bOpposite, double& OutDegreesOff);

	/**
	 * A pair stated about a bone's POSITIVE axis, turned into what the engine's full-body solver reads
	 * on that axis.
	 *
	 * The solver reads X and Z about the positive axis and Y about the NEGATIVE one
	 * (`FJointConstraint::DecomposeRotationAngles`: `AngleY = -SignedAngleBetweenNormals(ZA, ZBProjOnY,
	 * YA)`, corrected about `-YA`). On Y the same travel is therefore written backwards. Until
	 * 2026-10-01 it was written straight, and a knuckle on Y that the file allowed (-20, 90) curled 20
	 * and bent back 90 (GC-RIG-IMPORTED-MESH.md).
	 */
	static void AsTheSolverReads(int32 Axis, double& Low, double& High);

	/**
	 * A turn of `Degrees` about a bone's positive axis, as the solver's preferred-angle triple. The
	 * solver builds that turn from Euler roll, pitch and yaw, and roll and pitch run about the negative
	 * axis, so the engine's own conversion is asked instead of a sign table kept here.
	 */
	static FVector PreferredAngles(int32 Axis, double Degrees);

	/**
	 * **BEYOND THIS THE AXIS IS NOT REALLY AN AXIS.** Fifteen degrees is a quarter of the way to the
	 * next axis at 90, and a limit written that far off is measurably not the motion the author
	 * described. A call made without a ruling (grammar section 11.3), and it reports rather than
	 * refuses: a rig with an odd frame still builds, and its author is told.
	 */
	static double TooFarOff() { return FGcRigAxisRecord::TooFarOffAnAxis(); }
};
