// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeRigProgram.h"
#include "GraphCodeRigSkeleton.h"

/**
 * THE BUILDER: a `.gcrig` and a mesh in, a skeleton and a skinned mesh out.
 *
 * **THIS IS THE ONE STAGE THAT TOUCHES ASSETS**, and everything in front of it was written so that
 * this file has no judgement left to make: the parser says what one line means, the linker merges the
 * chain, the pose derives every frame, the plan adds the bones nobody typed. What is left here is
 * loading a mesh, handing the engine a bone list, and writing packages.
 *
 * **AND THAT IS WHY THE FIRST THING IT DOES IS RUN ALL FOUR OF THOSE.** A refusal from any of them
 * comes back unchanged, so a bad file fails with the same sentence whether it was applied, dry-run, or
 * read by a corpus sweep.
 *
 * **WHAT IT WRITES TODAY** is the skeleton and the skeletal mesh — pipeline 1.1 end to end, which the
 * grammar says ships on its own. The physics asset, the IK rig and the retargeter are RESOLVED and
 * reported (a dry run names every path it would write) and are not yet created; all three kinds create
 * through Core today, so what is missing is the call, not the door.
 */

/** One asset a build would write, or wrote. */
struct FGcRigBuildOutput
{
	/** `skeletal_mesh`, `skeleton`, `physics_asset`, `ik_rig`, `ik_retargeter`. */
	FString Kind;

	FString PackagePath;

	/** False where the path names something that already exists and is being reused, not made. */
	bool bCreates = true;

	/** False where this build does not write the kind yet — reported rather than silently skipped. */
	bool bWritten = true;

	/** The other IK rig named by this particular `ik_retargeter(onto=...)` output. */
	FString RetargetOnto;

	/** True when this particular retargeter carries `[out]`. */
	bool bRetargetOut = false;

	/** Explicit `(this file's chain, onto rig's chain)` pairs, after parsing `map=(...)`. */
	TArray<TPair<FString, FString>> RetargetChainMap;
	/** Zero keeps the native solver default; otherwise the authored full-body IK budget. */
	int32 IkIterations = 0;
	/** `physics_asset() [bodies=hull]`: one convex hull per bone, and bodies that collide with each other. */
	bool bHullBodies = false;
	/** `[whole=(...)]` beside it: bones whose body stays one hull however far it stands off the skin. */
	TArray<FString> WholeBodies;
	/** `[gives=]` beside it: how far, in cm, a held thing may press into the skin. Zero is rigid. */
	double Gives = 0.0;
	/** Header override:false preserves an existing Control Rig output; missing outputs are created. */
	bool bOverride = true;
};

/**
 * **WHAT A JOINT'S `dynamic=` WRITES INTO THE PHYSICS ASSET** (MOVE-RELAX1, Docs/GC-MOVE-RELAX-PLAN.md 2.2), in one place so the
 * writer, its notes and the specs read the same numbers.
 *
 * Between 0 and 1 the joint gets an angular drive in acceleration mode, stiffness `K = H (1 - d)^p` and damping `B = 2 z0 sqrt(K)`,
 * and every declared joint above 0 gets body angular damping `D0`. `K` and `B` here are what the solver should feel, per unit of
 * the body's inertia (rad/s^2 per radian, 1/s); the asset stores them divided by the engine's own conversion factors
 * (`Chaos::ConstraintSettings::AngularDriveStiffnessScale()`/`DampingScale()`, both 1.5 in UE 5.7, ChaosConstraintSettings.cpp:15-16),
 * which the solver multiplies back in (ImmediatePhysicsAdapters.cpp:517-523). The factors are read when the rig is built, so a
 * machine with those console variables changed builds different numbers from the same text; S1's calibration records their value.
 * `H` is said as a frequency only to name the scale: the solver divides the drive by the body's inertia about its own centre of
 * mass (PBDJointCachedSolverGaussSeidel.cpp:2546-2575), so a limb hinged at one end swings slower than `H` implies, by a factor
 * its shape sets. No note prints a frequency for that reason.
 *
 * **THESE ARE STARTING VALUES WITH NO SOURCE, AND S1'S CALIBRATION TABLE REPLACES THEM** (plan 2.2: "H, the mapping, z0 and D0 are
 * set from that table, never by hand"). Change them here and only here.
 */
struct GRAPHCODERIG_API FGcRigJointDrive
{
	/** `H`, as the natural frequency the law gives at `d = 0`, in Hz; never written itself, since 0 is a kinematic body. NO SOURCE (plan 2.2). */
	static constexpr double FullHoldHz = 12.0;
	/** `p`: how fast the hold falls as `dynamic=` rises. NO SOURCE; S1 may replace the power law itself (plan 2.2). */
	static constexpr double Exponent = 3.0;
	/** `z0`: the drive's damping ratio at weight 0. NO SOURCE (plan 2.2, 2.4). */
	static constexpr double DampingRatio = 0.5;
	/** `D0`: the body's angular damping, per second, the part that survives a 100% `physics()` release. NO SOURCE (plan 1.2). */
	static constexpr double BodyDamping = 6.0;

	/** The solver's stiffness for this amount, rad/s^2 per radian. 0 at `dynamic=1`. */
	static double SolverStiffness(double Dynamic);
	/** The solver's damping for this amount, 1/s. */
	static double SolverDamping(double Dynamic);
	/** What the asset stores: the solver's values over the engine's conversion factors. */
	static float AssetStiffness(double Dynamic);
	static float AssetDamping(double Dynamic);
};

/** Where a build's four assets land, worked out from `build { }` or from the engine's convention. */
struct GRAPHCODERIG_API FGcRigOutputs
{
	TArray<FGcRigBuildOutput> Assets;

	FString SkeletalMeshPath;
	FString SkeletonPath;

	/** True where `skeleton=` named an existing skeleton to share rather than one to make. */
	bool bSharesSkeleton = false;

	/** `[triangles=]` — a CEILING, exactly as Shape's is. Zero for no budget. */
	int32 TriangleBudget = 0;

	const FGcRigBuildOutput* Find(const TCHAR* Kind) const;
};

class GRAPHCODERIG_API FGraphCodeRigBuilder
{
public:
	/** Core's seam. Parse, link, pose, plan, then write — or report what it would write. */
	static bool Build(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcRigBuildResult& OutResult, FString& OutError);

	/**
	 * The corpus sweep's question, answered off the text alone.
	 *
	 * **IT CHECKS THE NAME ALWAYS AND THE FOLDER ONLY WHERE `into:` STATES ONE**, and the gap is the
	 * format's rather than this function's: with no `into:` a `.gcrig`'s assets land beside the
	 * document, and the program text never says where the document is. So `/Game/Anywhere/SK_Leg` is
	 * a path such a program really could have produced, from a copy of itself in that folder. The
	 * signature matches Shape's and Move's, which do not have the problem because their build lines
	 * carry no document-derived default.
	 */
	static bool HoldsRecipeFor(const FString& ProgramText, const FString& PackagePath,
		FString& OutReason);

	/**
	 * Where this document's assets go — from `build { }` where it has one, and from `into:` plus the
	 * engine's own `SK_<file>` convention where it has not (grammar §3).
	 *
	 * **PUBLIC BECAUSE IT IS THE HALF A DRY RUN AND THE SWEEP BOTH NEED WITHOUT AN EDITOR**, which is
	 * also what lets it be proved by a spec that writes nothing.
	 */
	static bool ResolveOutputs(const FString& DocumentPath, const FGcRigLinkedRig& Rig,
		FGcRigOutputs& Out, FString& OutRefusal);
};
