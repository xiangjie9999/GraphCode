// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeContact.h"

// Cloth and cord drawn tight against each other, every piece keeping its cut (Docs/GC-STRIP-SOLVE-PLAN.md): the solve a
// knot, a drawstring and a clamp's bundle are tightened by. Authoring-time construction, as `GraphCodeSettle.h`'s `Pull`
// is; Chaos stays the dynamics for a garment's settle and its game cloth. A cord's stretch, cloth's yarn stretch and shear
// (triangle strain in the cut's grain basis, Strain Based Dynamics §3.3–3.6, codex's model in
// Docs/Research/garment-strip-shear-model.md), contact, pins and the product's `Keep`. Bending is the plan's next slice.

/** What a strip is made of. A solve never changes it. */
struct FGcStripMaterial
{
	/** Each point's place in the flat pattern, in centimetres. A stretch edge keeps the length it has here. */
	TArray<FVector2D> Cut;

	/** Cloth and cord, as the contact layer takes them. */
	TArray<FIntVector> Faces;
	TArray<FIntPoint> Segments;

	/** How far the material reaches each side of its midsurface at each point, in centimetres. */
	TArray<double> HalfThickness;

	/** Declared outward normals of faces that belong to a closed solid (`FGcContactMesh::Outward`): an obstacle, not cloth. */
	TArray<FVector> FaceOutward;

	/** The pattern's along-the-grain direction in `Cut`: the basis each face's warp, weft and shear are measured in. */
	FVector2D Grain = FVector2D(1.0, 0.0);

	/**
	 * How hard the cloth resists shearing, as a surface modulus in g/s² (force per length): a face of rest area `A` sheared
	 * by `C` stores `½·μ·A·C²`. 0: shear is free. Not yet read from `fabric()`: a bias stretch percentage with no stated
	 * test load and gauge does not fix it (codex), so the product passes a named value.
	 */
	double SurfaceShearModulus = 0.0;

	/**
	 * Cloth's bending rigidity per unit width `G`, g·cm²/s², the same in every direction for now: two bending lengths, along
	 * and across the grain, cannot fix the stiffness at an angle to it (codex). From `fabric()` it is Peirce's
	 * `G = weight·g·c³` with `c` the bending length. A hinge across an interior edge `e` between faces of rest areas
	 * `A₁`, `A₂` stores `½·k·θ²` with `k = G·|e|² / (A₁ + A₂)` (codex's E3: three times that, the discrete-shell weight, is
	 * three times too stiff on a band). `Relax` projects each hinge toward flat as cut (signed angle and gradients as
	 * codex's E3b fixes them); `BendEnergy` measures it. 0: no bending.
	 */
	double BendRigidity = 0.0;

	/**
	 * A cord's flexural rigidity `B`, g·cm³/s² (a strip drawn as its centre line: `G` × its width). Each joint between two
	 * segments of rest lengths `L₁`, `L₂` stores `½·k·|κb|²`, `k = 2B / (L₁ + L₂)`, with `κb` the integrated curvature
	 * binormal (Bergou et al. 2008: magnitude 2·tan(θ/2), defined at straight, isotropic, no twist). Each cord line is
	 * solved as one banded block, its bends and stretches together (GC-STRIP-SOLVE-PLAN.md §5b). 0: a cord bends freely.
	 */
	double CordBendRigidity = 0.0;

	/**
	 * Points where a cord leaves along a held direction: a clamped root. Its first segment against that direction is a
	 * joint like any other, with `k = 2B / L` (codex's cantilever model).
	 */
	TMap<int32, FVector> HeldTangents;
};

/** One face's strain in the cut's grain basis, from its deformation `F = Ds·Dm⁻¹` with columns warp `a` and weft `b`. */
struct FGcStripFaceStrain
{
	/** `|a| − 1` and `|b| − 1`: how much longer the yarns are than cut. */
	double Warp = 0.0;
	double Weft = 0.0;

	/** `a·b / (|a||b|)`: the cosine of the yarns' angle, 0 square. */
	double Shear = 0.0;

	/** `F`'s singular values. A sheared face with yarns at length has `√(1 ± Shear)`, which is shear, not stretch. */
	double MostStretch = 1.0;
	double LeastStretch = 1.0;

	double RestArea = 0.0;

	/** `½·μ·A·Shear²`, in g·cm²/s². */
	double ShearEnergy = 0.0;
};

/** What one hinge projection did, corners in the canonical order `i < j`, `k` left and `l` right of the edge in the cut. */
struct FGcStripHingeProjection
{
	int32 Stencil[4] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	double Theta = 0.0;
	FVector Gradient[4] = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };
	double Stiffness = 0.0;
	double ScaledCompliance = 0.0;
	double Denominator = 0.0;
	double DeltaLambda = 0.0;

	/** Folded exactly onto itself (θ = ±π, no side chosen): nothing moved. */
	bool bAmbiguous = false;
};

/** What one block iteration did to a cord, rows in codex's order `B₀x, B₀y, B₀z, S₀, B₁x, …`. */
struct FGcStripCordProjection
{
	bool bSolved = false;
	TArray<double> C;
	TArray<double> ScaledCompliance;
	TArray<double> DeltaLambda;
	double Scale = 0.0;
	double MeritBefore = 0.0;
	double MeritAfter = 0.0;
};

/**
 * What the cloth's block did (GC-STRIP-SOLVE-PLAN.md §5b.2): how many rows of each kind, and the Newton steps it took on
 * `C + α̃λ = 0`.
 */
struct FGcStripClothProjection
{
	/** Every step was taken: nothing singular, and some share of each step lowered the residual. */
	bool bStepped = false;

	/** Every row ended within the block's tolerance, 1e-9. */
	bool bSolved = false;

	/** Yarns kept to their cut: an edge's length where the grain runs along it, otherwise the face's yarn. */
	int32 HardRows = 0;
	int32 ShearRows = 0;
	int32 HingeRows = 0;
	int32 Iterations = 0;

	/** The share of the last step taken. */
	double Scale = 0.0;

	/** `Σ (C + α̃λ)²` before the first step and after the last. */
	double MeritBefore = 0.0;
	double MeritAfter = 0.0;

	/** The largest `|C + α̃λ|` where the block stopped. */
	double WorstResidual = 0.0;

	/** Why the block could not take a step, when it could not. */
	const TCHAR* Failure = TEXT("");

	/** The contacts' active set: its steps (one factorisation each) over every Newton step, and how long the block took. */
	int32 ActiveSetSteps = 0;
	double Milliseconds = 0.0;

	/** Of those steps, how many faces the sparse factorisation could not solve and the rank-revealing routes did (slow). */
	int32 RevealedFaces = 0;

	/** The material factor reuse (`garment-active-set-reuse-and-warm-start.md`): lazy column solves, one per contact when it first joins a face. */
	int32 SchurColumns = 0;

	/** Contacts added to or removed from the working contact factor in place, instead of the whole face factored again. */
	int32 SchurEdits = 0;

	/** Fresh factorisations of the active contacts: the upkeep after enough edits, and the retry after a failed one. */
	int32 SchurRefactors = 0;

	/** Newton calls that gave the reuse up and solved every later face whole. */
	int32 SchurFallbacks = 0;
};

/**
 * A thick cord's shape against its own thickness, the static polygonal model codex measured (`cord_self_contact.json`,
 * after Ashton and Cantarella's closest approach): every joint bends no tighter than the radius, and every pair of segments
 * whose closest points are closest from both sides keeps twice the radius. A pair that is merely near along a straight run
 * is not a contact: one of its witnesses can slide nearer, so it is not a closest approach. Static only: it says nothing
 * about the motion between two shapes (codex).
 */
struct FGcCordShape
{
	/** A joint turned exactly back on itself, where no curvature is defined. */
	bool bFolded = false;

	/** The tightest joint: `min(|a|, |b|) / |κb|`, infinite on a straight cord. */
	double LeastBendRadiusCm = TNumericLimits<double>::Max();

	/** Segment pairs, not neighbours, whose closest points are critical from both sides; the nearest; and those within 2r. */
	int32 ClosestPairs = 0;
	double LeastClosestCm = TNumericLimits<double>::Max();
	int32 Pressing = 0;

	/** Pairs nearer than 2r, and of them those that are not closest approaches (a straight run's neighbours, say). */
	int32 NearPairs = 0;
	int32 NearNotClosest = 0;
};

/** What one shear projection did: codex's isolated control checks each number. */
struct FGcStripShearProjection
{
	double C = 0.0;
	FVector Gradient[3] = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };
	double ScaledCompliance = 0.0;
	double Denominator = 0.0;
	double DeltaLambda = 0.0;
};

/** Why a pull's handle stopped. */
enum class EGcStripHandleStop : uint8
{
	Going,

	/** It reached its target. */
	Arrived,

	/**
	 * Proven: one more step would put some held point further from it, by a lower bound on every route, than the material
	 * between them reaches (codex's E8 certificate). The slack is out.
	 */
	Taut,

	/** No share of a step could be certified clear. */
	ContactFailure,

	/** The step's relax ran to its cap without the cloth being out of length: not taut, not done; the step is undone. */
	Unresolved,
};

/** A point a pull draws: where it is drawn toward, how far it has gone, and whether it still goes. */
struct FGcStripHandle
{
	int32 Point = INDEX_NONE;
	FVector Target = FVector::ZeroVector;
	double Advanced = 0.0;
	bool bActive = true;
	EGcStripHandleStop Stop = EGcStripHandleStop::Going;

	/**
	 * Handles of one group move together, each along its own straight way by the same share of what it has left: a cloth
	 * end's held row is carried as one, never stretched between its own points. `INDEX_NONE`: alone.
	 */
	int32 Group = INDEX_NONE;

	/**
	 * How far, in cm, this handle's last step asked to go that relaxed, and whether it relaxed at its first try. A pull starts the
	 * next step there, twice as far after a first try that relaxed, never past its own step: a knot that draws in only an
	 * eighth of a step at a time no longer spends three failed relaxes finding that out on every step. Zero: the pull's step.
	 */
	double Stride = 0.0;
	bool bStrideEasy = false;

	/**
	 * Why it stopped, where the stop alone does not say: `PullWorkLimit` for an `Unresolved` handle whose group spent its
	 * work (codex, `garment-pull-work-budget.md`: stopped at the work limit, the last certified shape kept; neither jammed
	 * nor taut), or the relax or the backstop that ended it. Empty for `Arrived` and a proven `Taut`.
	 */
	const TCHAR* Why = TEXT("");

	/** Candidate relaxes this handle's group spent in the last pull, and how many it was allowed. */
	int32 RelaxesUsed = 0;
	int32 RelaxesAllowed = 0;
};

/** What a solve moves. */
struct FGcStripState
{
	TArray<FVector> Points;

	/** 0 holds a point where it is: that place is its pin. */
	TArray<double> InverseMass;

	/** Centimetres a second; `Step` reads and writes them, `Relax` leaves them alone. Empty: at rest. */
	TArray<FVector> Velocities;

	/** A force on each point, g·cm/s², that `Step` applies; empty, none. A load hung on a corner, say. */
	TArray<FVector> Forces;

	/** Each face's shear multiplier (XPBD). `Step` starts each step at 0; `Relax` carries what it is given. */
	TArray<double> ShearLambda;

	/** The points a pull draws. A rolled-back step puts them back with everything else. */
	TArray<FGcStripHandle> Handles;
};

/** One try at a step: the share of it tried, what the swept check said, and whether it was taken. */
struct FGcStripAttempt
{
	double Scale = 1.0;
	EGcContactStatus Check = EGcContactStatus::Converged;
	double Fraction = 0.0;
	bool bTaken = false;

	/** The pair that limited the swept check: the one that touched first, or the start it found wrong. */
	FGcContactWitness Limit;

	/** Where the limiting pair's four points started and where the candidate puts them. */
	FVector LimitFrom[4] = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };
	FVector LimitTo[4] = { FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector };

	/** How the relax that made this candidate ended, where the proposal relaxed one (a pull's does). */
	EGcContactStatus Relaxed = EGcContactStatus::Converged;
};

/** How a certified step ended. */
struct FGcStripCertified
{
	/** `Converged`: a share of the step was taken whole. `ContactFailure`: none was, and the accepted state is kept. */
	EGcContactStatus Status = EGcContactStatus::Converged;

	/** When none was taken, what the last check said (`IterationLimit` for an unresolved certificate, say). */
	EGcContactStatus Reason = EGcContactStatus::Converged;

	/** The share of the step taken; 0 when none was. */
	double Scale = 0.0;

	TArray<FGcStripAttempt> Attempts;
};

/** When a solve is done, and how long it may take to get there. */
struct FGcStripSettings
{
	/**
	 * The candidate relaxes each moving group of a pull may spend, every call counted, a retried or rejected one too, never
	 * given back (codex, `garment-pull-work-budget.md`). 0: four for each of its steps, `4·(⌈distance / step⌉ + 1)`, the
	 * group's own distance at the start. A cost the author bounds, in work, never seconds, so a build gives the same shape
	 * on every machine; not a proof of adequacy.
	 */
	int32 PullWorkBudget = 0;

	/** The most any stretch edge may be longer or shorter than its cut, as a share of it. */
	double StrainTolerance = 1e-3;

	/** Rest is judged over this many passes together, never one: forward and back passes can swap a point for ever. */
	int32 RestWindow = 16;

	/** And the furthest any point may move over the window, in centimetres. */
	double WindowMotionCm = 0.005;

	/** How far a held point may be from its pin, in centimetres. */
	double PinToleranceCm = 1e-6;

	/** How far into each other two pieces may press, as a share of the gap they keep. */
	double PenetrationShare = 0.01;

	/** The pass cap. Reaching it is `IterationLimit`, never rest. */
	int32 MostPasses = 3000;

	/** The step a compliance is scaled by, `α̃ = α / h²`, in seconds; `Step` sets it to its own. Fixed and named, so a
	 *  modulus means the same at every resolution. */
	double H = 1.0 / 60.0;

	/** Whether the pieces touch at all (a lone chain has nothing to touch). */
	bool bContact = true;

	/** The length a taut proof allows past the material's own, in centimetres, beside `StrainTolerance` (codex's E8: 1e-6). */
	double TautAbsoluteCm = 1e-6;

	/**
	 * An intentional clearance contact is parted to beyond the physical gap, in centimetres: a resting contact then stands a
	 * hair off its gap instead of on it, so the fixed-axis certificate proves the next move clear with a margin far above
	 * rounding. Named and separate from the gap, as codex asks; the gap itself is never changed.
	 */
	double ContactClearanceCm = 1e-6;

	/**
	 * The clearance as a share of the pair's gap, where that is more: a pair that pivots while it rests dips between two
	 * relaxed states by the chord of its turn, and codex measured 4.9e-5 cm needed at a 0.1 cm gap for a 0.0625 rad turn
	 * of a 0.5 cm face, 7.3e-5 at 0.15, 1.5e-4 at 0.3 (rule 10). 1e-3 of the gap gives 1e-4 at 0.1 and 3e-4 at 0.3. Like
	 * the clearance, never an allowance on the gap: a resting shape then stands that far off it.
	 */
	double ContactClearanceShare = 1e-3;

	/**
	 * Contacts solved in the cloth's block, as inequality rows, with the stretch-only cords beside them, instead of projected
	 * after it. Off until codex's coupled active-set rule (rule 13, P0) is built: the plain version here fails its line
	 * search once 252 contact rows of one resting touch depend on each other (2026-10-05).
	 */
	bool bContactInBlock = false;

	/**
	 * How near its target a handle counts as arrived, in centimetres: the caller's declared tolerance, 1e-9 unless it says
	 * otherwise. Codex (rule 13, `ribbon_arrival.json`): a target at the material's reach is a numerical target, not one a
	 * solve can be held to exactly, and a caller may accept arrival within a tolerance it declares once every feasibility,
	 * complementarity and swept check has passed; it is never a licence to call a stop taut.
	 */
	double ArrivalCm = 1e-9;

	/**
	 * For research only: where a coupled block that fails (every time), or the first Newton call whose contacts' active set takes twenty steps or more,
	 * writes the problem as JSON (each row's kind, stencil, gradient, value and compliance; the weights, positions,
	 * multipliers, scales and the proximal term it started from; how many rows are equalities). Empty writes nothing;
	 * nothing shipped sets it.
	 */
	FString BlockDumpPath;

	/**
	 * A body the cloth is held off, as a field (GC-STRIP-SOLVE-PLAN.md §10 S3a): for a point, its signed distance from the
	 * skin, negative inside, and the way out. Unset, there is none. A body of a hundred thousand triangles enters as this
	 * query, never as faces of the strip, which every pass would then walk. Each free point is kept its half thickness plus
	 * `BodyClearanceCm` off it: a one-sided row in the block, and pushed out after it as a declared solid's points are.
	 */
	TFunction<bool(const FVector& Point, double& OutSigned, FVector& OutGradient)> Body;

	/** The air the garment keeps off the skin, in centimetres, beside the cloth's own half thickness: the author's, not the solver's. */
	double BodyClearanceCm = 0.0;
};

/** How a solve ended, and every residual it was judged by. */
struct FGcStripResult
{
	EGcContactStatus Status = EGcContactStatus::Converged;

	/** Stretch edges' length against their cut, as a share: the worst either way, and the most short and most long. */
	double WorstStrain = 0.0;
	double LeastStrain = 0.0;
	double MostStrain = 0.0;

	/** The worst soft residual `C + α̃·λ` over the faces' shear: 0 where shear and whatever holds it balance. */
	double WorstShearResidual = 0.0;

	/** The same over the cords' curvature binormal rows. */
	double WorstBendResidual = 0.0;

	/** The deepest any pair presses past its gap, in centimetres; 0 apart. */
	double WorstPenetrationCm = 0.0;

	/** The furthest a held point is from its pin. */
	double PinErrorCm = 0.0;

	/** The furthest any point moved over the last rest window. */
	double WindowMotionCm = 0.0;

	/**
	 * Chain solves that found their system singular and moved nothing: a straight chain squeezed between held ends, which
	 * has bent shapes of the same lengths but no gradient pointing at them. Not `InfeasibleBoundary`; it needs bending or
	 * a nudge out of line, and the status stays `IterationLimit` until something gives it one.
	 */
	int32 SingularSolves = 0;

	/** Why the last block that could not be solved stopped (`FGcStripClothProjection::Failure`), its worst row, its contacts. */
	const TCHAR* BlockFailure = TEXT("");
	double BlockResidual = 0.0;
	int32 BlockContacts = 0;

	/** Over every pass: the contacts' active-set steps (one factorisation each) and the time the cloth's block took. */
	int32 ActiveSetSteps = 0;
	double BlockMilliseconds = 0.0;

	/** Hinges left out of the block because the cloth lies folded exactly onto itself, where opening it either way is a choice. */
	int32 AmbiguousHinges = 0;

	int32 Passes = 0;
	double Milliseconds = 0.0;
};

/** How a pull ended. */
struct FGcStripPullResult
{
	/** The handle steps taken. */
	int32 Steps = 0;

	/** The relax after every handle stopped. */
	FGcStripResult Final;

	/**
	 * Whether that relax's points were kept: only when it converged and the whole of its motion was certified, as every step's
	 * is. Otherwise the state is the last certified one, exactly, and `Final` reports a relax that was not kept.
	 */
	bool bFinalPublished = false;

	/** The last certificate that stopped a handle with `ContactFailure`: every share tried, and the pair that refused it. */
	FGcStripCertified Refused;

	/** The relax of the last step undone because it did not converge (`Unresolved`). */
	FGcStripResult Undone;
};

class GRAPHCODE_API FGraphCodeStrip
{
public:
	/**
	 * Passes until every residual is within its tolerance over a whole rest window, or the cap: each cord solved as one
	 * chain, directly — its stretch constraints' multiplier system is tridiagonal and Thomas' algorithm
	 * solves it, exactly for a straight line and as one linearised step for a bent one (Deul et al. 2018) — then the
	 * other stretch edges one by one, each face's warp and weft (hard) and shear (soft), contact (`FGraphCodeContact::Find` and `Project`), the pins, and `Keep` last, its
	 * correction inside the next pass's residuals rather than after them. Before any pass: an edge too long or too short
	 * with both ends held is `InfeasibleBoundary`, and pieces pressed into each other are `InvalidInitialContact`; the
	 * points are then left as they were. Gauss–Seidel alone took 5,476 passes on a 64-edge chain and was still 0.02 cm
	 * off (codex, GC-STRIP-SOLVE-PLAN.md §3); the chain solve is why this one does not.
	 */
	static FGcStripResult Relax(const FGcStripMaterial& Material, FGcStripState& State, const FGcStripSettings& Settings,
		TFunctionRef<void(TArray<FVector>& Points)> Keep);
	static FGcStripResult Relax(const FGcStripMaterial& Material, FGcStripState& State, const FGcStripSettings& Settings);

	/**
	 * One step of hanging, `H` seconds: each free point predicted as `x + e^(−Damping·H)·v·H + H²·Gravity`, `Relax`ed,
	 * then its velocity is where it went over `H`. Codex's measured control (`strip_gravity.json`); without the velocity a
	 * link falls to the same place by a different path, which is not the measured motion.
	 */
	static FGcStripResult Step(const FGcStripMaterial& Material, FGcStripState& State, const FGcStripSettings& Settings,
		const FVector& Gravity, double H, double DampingPerSecond, TFunctionRef<void(TArray<FVector>& Points)> Keep);
	static FGcStripResult Step(const FGcStripMaterial& Material, FGcStripState& State, const FGcStripSettings& Settings,
		const FVector& Gravity, double H, double DampingPerSecond);

	/**
	 * Bending's stored energy where the points are now, g·cm²/s²: every cloth hinge `½·G·|e|²/(A₁+A₂)·θ²` and every cord
	 * joint and held root `½·k·θ²`, each angle measured from straight or flat as cut.
	 */
	static double BendEnergy(const FGcStripMaterial& Material, TConstArrayView<FVector> Points);

	/**
	 * A step taken only when the whole of it is certified (GC-STRIP-SOLVE-PLAN.md §5; codex's E6). `Propose` turns a copy
	 * of the accepted state into the candidate for a share `Scale` of the step (handles advanced, gravity predicted, then
	 * relaxed). The move from the accepted points to the candidate's is checked by `Check` (`FGraphCodeContact::Sweep`
	 * by default), and taken only if the check resolved (`Converged`) and certified all of it (fraction 1): a fraction
	 * alone, or a fraction of 1 from a check that ran out of refinements, is no certificate. Otherwise **everything** goes
	 * back to the accepted state — every strand's points, velocities, multipliers and handles, since restoring one strand
	 * after its neighbour moved changes the accepted state — the share is halved, and it is tried again from there,
	 * `MostHalvings` times. None certified: `ContactFailure`, the state exactly as it was.
	 */
	static FGcStripCertified Certify(const FGcStripMaterial& Material, FGcStripState& State,
		TFunctionRef<void(FGcStripState& Candidate, double Scale)> Propose,
		TFunctionRef<FGcContactSweep(const FGcContactMesh& From, TConstArrayView<FVector> To)> Check, int32 MostHalvings = 4);
	static FGcStripCertified Certify(const FGcStripMaterial& Material, FGcStripState& State,
		TFunctionRef<void(FGcStripState& Candidate, double Scale)> Propose, int32 MostHalvings = 4);

	/**
	 * Draw every active handle toward its target (GC-STRIP-SOLVE-PLAN.md §5a, codex's E8), one handle (or one group, in
	 * lockstep) at a time and round robin. Every handle holds its point throughout, moving or not, and through the final relax. Each step's travel is
	 * clipped so that no held point is provably out of reach: `LowerBound(from, to)` bounds every route between two points
	 * from below (straight distance unless the caller knows an obstacle), and the material between them is
	 * `MaterialDistances`; the step may go only as far as `D ≤ L·(1 + StrainTolerance) + TautAbsoluteCm` everywhere. No room
	 * left is `Taut`, proven. A step that cannot be certified clear stops the handle (`ContactFailure`); one whose relax
	 * does not converge is undone (`Unresolved`), since no proof stands behind it.
	 */
	static FGcStripPullResult Pull(const FGcStripMaterial& Material, FGcStripState& State, const FGcStripSettings& Settings,
		double StepCm, TFunctionRef<void(TArray<FVector>& Points)> Keep,
		TFunctionRef<double(const FVector& From, const FVector& To)> LowerBound);
	static FGcStripPullResult Pull(const FGcStripMaterial& Material, FGcStripState& State, const FGcStripSettings& Settings,
		double StepCm, TFunctionRef<void(TArray<FVector>& Points)> Keep);

	/** `D > L·(1 + StrainTolerance) + AbsoluteCm`: a lower bound on every route longer than the material allows. */
	static bool ProvenTooShort(double LowerBoundCm, double RestLengthCm, double StrainTolerance, double AbsoluteCm);

	/** The material from `From` to every point along the cut's edges, faces' and cord's: an upper bound on what lies between. */
	static TArray<double> MaterialDistances(const FGcStripMaterial& Material, int32 From);

	/** One block iteration on the material's first cord, every row's multiplier in `Lambda`: what `Relax` does each pass. */
	static FGcStripCordProjection ProjectCord(const FGcStripMaterial& Material, TArray<FVector>& Points,
		TConstArrayView<double> InverseMass, double H, TArray<double>& Lambda);

	/**
	 * The cloth's rows solved together from multipliers at nothing, at most `MostIterations` Newton steps: what `Relax`
	 * does each pass with 64. One step is what codex's single-projection control checks.
	 */
	static FGcStripClothProjection ProjectCloth(const FGcStripMaterial& Material, TArray<FVector>& Points,
		TConstArrayView<double> InverseMass, double H, int32 MostIterations);

	/** The pairs of a cord's segments, by place along its line, that contact treats as its own neighbourhood (`FCordArc`). */
	static TArray<FIntPoint> CordSelfExclusions(const FGcStripMaterial& Material);

	/** A polyline cord of radius `RadiusCm` against its own thickness (`FGcCordShape`). */
	static FGcCordShape CordShape(TConstArrayView<FVector> Points, double RadiusCm);

	/**
	 * The two bend-radius rows at the joint `P1` (codex's unilateral MinRad rows, each held `≥ 0`): `|a|² − r²|κb|²` and
	 * `|b|² − r²|κb|²`, `a = P1 − P0`, `b = P2 − P1`, and their gradients at the three points. False where the joint turns
	 * exactly back, which has no curvature to bound.
	 */
	static bool BendRadiusRows(const FVector& P0, const FVector& P1, const FVector& P2, double RadiusCm, double OutC[2],
		FVector OutGradient[2][3]);

	/** One hinge projection on the hinge across `Edge`: the single update codex's E3b control checks. */
	static FGcStripHingeProjection ProjectHinge(const FGcStripMaterial& Material, const FIntPoint& Edge, TArray<FVector>& Points,
		TConstArrayView<double> InverseMass, double H, double& Lambda);

	/** Every face's strain where the points are now. */
	static TArray<FGcStripFaceStrain> FaceStrains(const FGcStripMaterial& Material, TConstArrayView<FVector> Points);

	/**
	 * One XPBD shear projection on one face: `Δλ = (−C − α̃·λ) / (Σ wᵢ|∇ᵢC|² + α̃)`, `λ += Δλ`, `xᵢ += wᵢ·∇ᵢC·Δλ`, with
	 * `α̃ = 1 / (μ·A·H²)` and codex's gradients. What `Relax` does to each face's shear in every pass.
	 */
	static FGcStripShearProjection ProjectShear(const FGcStripMaterial& Material, int32 Face, TArray<FVector>& Points,
		TConstArrayView<double> InverseMass, double H, double& Lambda);
};
