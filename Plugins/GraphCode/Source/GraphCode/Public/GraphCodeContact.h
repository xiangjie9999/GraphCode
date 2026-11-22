// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// Cloth and cord touching cloth and cord, each side as thick as it is (Docs/GC-CONTACT.md). Chaos's cloth self-collision
// gives point–face springs only; a knot's strips crossing at right angles touch edge to edge with every vertex far from
// the other strip, and a cord is segments, not faces. Plain arrays, as `GraphCodeSettle.h`, so Core needs no geometry
// module; each product builds the mesh from its own and says which points are held.

/** What touches: points, how freely each moves, how thick each is, and the faces and cord segments between them. */
struct FGcContactMesh
{
	/** Where each point is, in centimetres. */
	TArray<FVector> Points;

	/** How freely each point moves: 0 holds it where it is. */
	TArray<double> InverseMass;

	/** How far the material reaches each side of its midsurface at each point, in centimetres. */
	TArray<double> HalfThickness;

	/** Cloth. Its edges are the faces' edges. */
	TArray<FIntVector> Faces;

	/** A cord's centreline, one segment a pair. */
	TArray<FIntPoint> Segments;

	/**
	 * A face's declared outward normal where it belongs to a closed solid (a body, a seated part), any length; zero, or no
	 * entry, where it does not. The one thing that gives a pair touching at zero distance a side to part along: a closest
	 * point difference of zero says nothing, and being held says nothing (codex). Each declared normal is only a candidate,
	 * proved by `HeldApart` before it is trusted.
	 */
	TArray<FVector> Outward;
};

enum class EGcContactKind : uint8
{
	/** A point against a face it is not a corner of. */
	PointFace,

	/** An edge or cord segment against one it shares no point with. */
	EdgeEdge,
};

/** Why a pair has no witness, when it has none. */
enum class EGcContactProblem : uint8
{
	None,

	/** A face with no area or an edge with no length: there is no closest point to speak of. */
	Degenerate,

	/** The two closest points are the same point, so nothing says which way is out. Never guessed. */
	NoDirection,
};

/**
 * The closest points of two features and how far apart they must be. `Points` and `Weights` say where each closest point
 * is: the first side's weights sum to +1 and the second side's to −1, so `Σ Weights[i] · Points[i]` runs from the second
 * closest point to the first. PointFace: the point at +1, then the face's corners at −b₀, −b₁, −b₂. EdgeEdge: the first
 * edge's ends at 1−s, s, then the second's at −(1−t), −t.
 */
struct FGcContactWitness
{
	EGcContactKind Kind = EGcContactKind::PointFace;
	int32 Points[4] = { INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	double Weights[4] = { 0.0, 0.0, 0.0, 0.0 };

	/** Unit, from the second side's closest point to the first's. Zero when `Problem` is `NoDirection`. */
	FVector Normal = FVector::ZeroVector;

	double Distance = 0.0;

	/** Each side's half thickness at its closest point, summed: the distance the two must keep. */
	double Gap = 0.0;

	EGcContactProblem Problem = EGcContactProblem::None;
};

/** How contact ended. A pass cap is never rest. */
enum class EGcContactStatus : uint8
{
	/** Every pair is at least its gap apart, within the tolerance asked. Contact only; it says nothing of the cloth. */
	Converged,

	/** The passes ran out with a pair still too close. */
	IterationLimit,

	/** A pair is too close and every point of it is held, so nothing can part them. */
	InfeasibleBoundary,

	/** A pair's closest points coincide, so there is no way out to push along. */
	InvalidInitialContact,

	/** A move that no fraction of makes safe. */
	ContactFailure,
};

/** How much of a move keeps every pair apart, and the pair that stops the rest. */
struct FGcContactSweep
{
	/**
	 * The fraction of the move, 0 to 1, along which every pair stays at least its gap apart the whole way: 1 when the
	 * whole move is clear, strictly before the first touch when it is not, 0 when the start is already wrong.
	 */
	double Fraction = 1.0;

	/**
	 * `Converged`: every pair was answered, whatever the fraction. `IterationLimit`: a query ran out of refinements
	 * and its fraction is the safe side of what it reached, or a pair of varying thickness starts inside the envelope the
	 * query takes though clear where its closest points are (fraction 0, unresolved, not wrong).
	 * `InvalidInitialContact`: a pair starts closer than its gap.
	 * `ContactFailure`: a feature has no area or length, or a pair touches with no positive progress.
	 */
	EGcContactStatus Status = EGcContactStatus::Converged;

	/** The pair that sets the fraction, as it stood at the start. Unset (`Points` all `INDEX_NONE`) when nothing does. */
	FGcContactWitness Limit;
};

/**
 * A declared closed solid, ready for the signed distance of points to it: its faces (those with a declared outward normal),
 * each face's unit normal turned to its declared side, and the angle-weighted pseudo-normals at its edges and corners that
 * decide inside from outside there (Bærentzen and Aanæs 2005). Built from where the solid is; a solid is held, so once.
 */
struct FGcContactSolid
{
	TArray<int32> Faces;
	TArray<FVector> FaceNormal;
	TMap<FIntPoint, FVector> EdgeNormal;
	TMap<int32, FVector> CornerNormal;
	TSet<int32> Corners;

	/** Each corner's index as the first corner at the same place: facets that carry their own corners are welded here. */
	TMap<int32, int32> Weld;
};

/** Where a point stands against a declared closed solid (codex's signed contact, `contact_solid.json`). */
struct FGcSolidDistance
{
	bool bFound = false;

	/** Negative inside, `σ·|p − q|`, `q` the nearest point of the whole boundary. */
	double Signed = 0.0;
	FVector Closest = FVector::ZeroVector;

	/**
	 * Which way it is pushed out: `σ·(p − q)/|p − q|`, the distance's own gradient, which at an edge or corner is not the
	 * pseudo-normal that only gives the sign (codex). On the boundary itself, the declared outward cone's pseudo-normal.
	 */
	FVector Gradient = FVector::ZeroVector;
	int32 Face = INDEX_NONE;
};

class GRAPHCODE_API FGraphCodeContact
{
public:
	/**
	 * How far along a straight move, every point from where it is in `From` to its place in `To`, nothing comes closer
	 * than its gap — certified, by Tight-Inclusion (Wang et al. 2021; vendored, Private/ThirdParty/TightInclusion), whose
	 * "clear" is a proof within its arithmetic bounds, not a sample. Endpoints alone prove nothing: two sheets can be
	 * clear at both ends and pass through each other between (codex's control, GC-CONTACT.md §2).
	 *
	 * **The gap a pair keeps over the move is each side's thickest point, summed** — a constant, since the query takes one.
	 * Where a side's thickness varies, this can call a start invalid that is valid where the closest points actually are;
	 * it never calls a touch clear. Equal thickness on each side (one fabric) is exact. The minimum separation is never
	 * halved to escape a touch at the start (upstream's `no_zero_toi` stays off).
	 */
	static FGcContactSweep Sweep(const FGcContactMesh& From, TConstArrayView<FVector> To,
		TFunctionRef<bool(int32 A, int32 B)> Ignore);

	/**
	 * Codex's fixed-axis certificate (`contact_resting.json`): two features are kept apart over a whole straight move when,
	 * along one direction `Axis` (any length), every vertex of A is beyond every vertex of B by their radii at the start
	 * and at the end, `s = n·(Aᵢ − Bⱼ) ≥ 0` and `s² ≥ (rᵢ + rⱼ)²·(n·n)`. The separation is linear in time and in the
	 * features' weights, so it holds throughout, sliding along a touch included. Decided exactly from the doubles given
	 * (expansion arithmetic), since a contact resting at exactly its gap is the case that matters and a rounded
	 * comparison would put it on either side. False proves nothing: the pair still needs the swept check.
	 */
	static bool HeldApart(TConstArrayView<FVector> AStart, TConstArrayView<FVector> AEnd, TConstArrayView<FVector> BStart,
		TConstArrayView<FVector> BEnd, TConstArrayView<double> RadiusA, TConstArrayView<double> RadiusB, const FVector& Axis);

	/**
	 * Codex's turning certificate (rule 10, `garment-ccd-tolerance-and-metric.md`): the separating direction turns with the
	 * move, `n(t) = (1 − t)·AxisFrom + t·AxisTo`, so a pair resting at its gap while one side pivots over the other is
	 * proved, where no fixed axis and no swept check can (Tight-Inclusion's separation is a box, not a distance, and finds
	 * an oblique resting pair touching at t = 0 at any tolerance). Each vertex pair's `s(t) = n(t)·(Aᵢ − Bⱼ)(t)` is
	 * quadratic and `s² − (rᵢ + rⱼ)²·|n|²` quartic; both are held non-negative on [0, 1] by their Bernstein coefficients,
	 * the interval halved only where those cannot decide, ten times at most. False proves nothing.
	 */
	static bool HeldApartTurning(TConstArrayView<FVector> AStart, TConstArrayView<FVector> AEnd, TConstArrayView<FVector> BStart,
		TConstArrayView<FVector> BEnd, TConstArrayView<double> RadiusA, TConstArrayView<double> RadiusB, const FVector& AxisFrom,
		const FVector& AxisTo);

	/**
	 * The closest point of a face to a point, and the gap they keep. Fills `Out.Problem` rather than returning false when
	 * the face has no area or the point lies on it, so a caller can say which it was.
	 */
	static FGcContactWitness PointFace(const FGcContactMesh& Mesh, int32 Point, const FIntVector& Face);

	/**
	 * The closest points of two edges, and the gap they keep. **Parallel edges that overlap take the middle of their
	 * overlap**, the same on both, since any point of it is as close and only the middle is the same read from either
	 * edge (codex's control rule, GC-CONTACT.md §2).
	 */
	static FGcContactWitness EdgeEdge(const FGcContactMesh& Mesh, const FIntPoint& First, const FIntPoint& Second);

	/** Every edge once, lowest point first: the faces' edges, then the cord's segments, sorted. */
	static TArray<FIntPoint> Edges(const FGcContactMesh& Mesh);

	/**
	 * Every pair closer than its gap plus `Margin`: each point against each face it is not a corner of, each edge against
	 * each it shares no point with. `Ignore(A, B)` drops a pair the caller excludes on purpose — the same strand within a
	 * cut distance — asked of every point pair across the two features, and a pair goes if any is ignored. Nothing is
	 * dropped by an index window. Sorted by kind then points, so the order does not hang on a hash.
	 */
	static TArray<FGcContactWitness> Find(const FGcContactMesh& Mesh, double Margin,
		TFunctionRef<bool(int32 A, int32 B)> Ignore);
	static TArray<FGcContactWitness> Find(const FGcContactMesh& Mesh, double Margin);

	/** The declared closed solid in `Mesh` (its faces with a nonzero `Outward`); empty `Faces` when there is none. */
	static FGcContactSolid Solid(const FGcContactMesh& Mesh);

	/**
	 * A point against a declared closed solid, signed by the boundary's own inside and outside (codex's rule): the nearest
	 * point over every face of the solid, never one face's plane alone, so a point behind one facet but above its neighbour
	 * is outside and left alone, and one behind both is inside and pushed out.
	 */
	static FGcSolidDistance SolidDistance(const FGcContactMesh& Mesh, const FGcContactSolid& Solid, const FVector& Point);

	/**
	 * Part one pair: hard, frictionless, no compliance, and only ever apart. The weights, gap and normal are the
	 * witness's, frozen; how far apart the closest points are along that normal is read again from where the points are
	 * now, so projections in one pass see each other, and a pair another projection has pushed through is not pushed
	 * further through. Point i moves `−wᵢ·cᵢ·n·C / Σ wⱼ·cⱼ²`, with `C` that distance less the gap, `wᵢ` its inverse mass and
	 * `cᵢ` its weight — at the points the witness was found from, the distance's own gradient and mass, not the face
	 * normal. Returns `Converged` when the pair is apart or now parted, `InfeasibleBoundary` when it is too close and held,
	 * `InvalidInitialContact` when it has no direction, `ContactFailure` when a feature has no area or length.
	 */
	static EGcContactStatus Project(FGcContactMesh& Mesh, const FGcContactWitness& Witness);

	/**
	 * Find and part every pair, a pass at a time, until every pair keeps its gap less `Tolerance`, or `MostPasses` run out.
	 * Contact alone: a cloth or cord solve owns its own residuals and calls `Project` within its pass.
	 */
	static EGcContactStatus Resolve(FGcContactMesh& Mesh, double Tolerance, int32 MostPasses,
		TFunctionRef<bool(int32 A, int32 B)> Ignore);
};
