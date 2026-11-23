// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// `relax(<distance>)` — GC Shape's rule (GC-SHAPE-GRAMMAR.md, row MR22), shared by Shape and Garment,
// so it is Core's. Plain arrays, so Core needs no geometry module; each product builds the points and
// their neighbours from its own mesh, and says which may not move.

/** Points to settle: where each started, where it is, how much it may move, and what it touches. */
struct FGcSettleGraph
{
	/** Where each point started, which is what the cap is measured from. */
	TArray<FVector> Origin;

	/** Where each point is now. */
	TArray<FVector> Current;

	/** How much of each step a point takes, 0 to 1. */
	TArray<double> Weight;

	/** The points one edge away. */
	TArray<TArray<int32>> Neighbours;

	/** True where a point may not move: an open edge, or the ring a local settle shares with the rest. */
	TArray<bool> bPinned;
};

/** How a settle ended: the passes it took, the furthest any point went, and how many reached the cap. */
struct FGcSettleResult
{
	int32 Passes = 0;
	double Furthest = 0.0;
	int32 AtTheCap = 0;
};

/**
 * Cloth to pull (`pull()`, GC-GARMENT-SHAPE-PARTS.md §4): its points, the edges between them that keep their
 * length, the points that may not move, and the handle the pull takes.
 */
struct FGcPullGraph
{
	TArray<FVector> Current;

	/** Held where it is: cloth the pull does not move. */
	TArray<bool> bFixed;

	/** Two points, and the length the edge between them keeps. */
	TArray<FIntPoint> Edges;
	TArray<double> Rest;

	/** The points the pull takes, and where it takes each. */
	TArray<int32> Handle;
	TArray<FVector> HandleTo;

	/**
	 * `Tighten` only: for each handle point, the point one step back along its cloth — the way it is drawn on is from
	 * there to it — and which edge it belongs to, since two ends pulled together go their own ways; and the runs of
	 * cloth from a held root to a handle, each with its edge and the length it keeps, which say when that edge is taut.
	 * No groups: one edge.
	 */
	TArray<int32> Behind;
	TArray<int32> Group;
	TArray<TArray<int32>> Runs;
	TArray<int32> RunGroup;

	/**
	 * Where along each run its stretch starts to count, and the length that stretch of it keeps: from there to the
	 * handle. Nearer the root a run may lie tight round something it passes with slack still to come out further on.
	 * No entry: from its first point.
	 */
	TArray<int32> RunTautFrom;
	TArray<double> RunRest;
};

/** How a pull ended: the steps the handle was walked in, the passes, and the most any edge is longer than it was. */
struct FGcPullResult
{
	int32 Steps = 0;
	int32 Passes = 0;
	double Stretched = 0.0;

	/** `Tighten`: how far the handle was drawn on before the cloth went taut, in centimetres. */
	double Drawn = 0.0;
};

class GRAPHCODE_API FGraphCodeSettle
{
public:
	/**
	 * **THE HALF-STEP IS WHAT KEEPS A LAPLACIAN PASS STABLE.** A full step to the neighbourhood
	 * average oscillates on a checkerboard of quads — every point swaps with its own average and the
	 * surface rings instead of settling — and half of it is the standard damping, not a taste.
	 */
	static constexpr double Step = 0.5;

	/**
	 * The ceiling on passes. **A backstop, not the instruction**: the loop stops when the surface has
	 * finished moving, and a cap that binds everywhere finishes in far fewer than this.
	 */
	static constexpr int32 MostPasses = 64;

	/**
	 * Settle every point toward the middle of its neighbours, a half step a pass, **never letting it
	 * end further than `CapAt(its origin)` from where it started** — the cap is on the whole journey,
	 * not the pass, so it is the one promise a build line can check. `Keep` may move a wanted place
	 * further — a garment holds its cloth off the body — before the cap is checked again. Stops when
	 * nothing moves more than `Distance * 0.005`. `Distance` is the largest cap, which a point at the
	 * cap is counted against.
	 */
	static FGcSettleResult Relax(FGcSettleGraph& Graph, double Distance,
		TFunctionRef<double(const FVector& Origin)> CapAt, TFunctionRef<void(int32 Point, FVector& Wanted)> Keep);

	/** How far the handle moves in one step of a pull, in centimetres: small enough that nothing it meets is passed through. */
	static constexpr double PullStepCm = 0.25;

	/**
	 * Take the handle to where it is going a step at a time, every edge kept at its length — a pull never makes
	 * cloth — and the fixed points held. `Keep` moves every point out of whatever it may not be in: the product's
	 * body, its other cloth, the parts on it; the handle too, so it is laid over what is under it. Settles once the
	 * handle has arrived and nothing moves more than `Settled`. Where the cloth cannot reach, its edges stretch, and
	 * the result says by how much.
	 */
	static FGcPullResult Pull(FGcPullGraph& Graph, double Settled, TFunctionRef<void(TArray<FVector>& Points)> Keep);

	/**
	 * Draw pairs of points together (`sew()` on the cloth, GC-GARMENT-CUT-SEW.md §F): `Ties` are edges of the graph, each
	 * shortened from the length it keeps to `To` — or left, where it is that short already — a `PullStepCm` at a time, the
	 * fixed points held, a few passes a step as a pull walks its handle; then it settles once. There is no handle: where
	 * each pair meets is the cloth's to say, so cloth that can move comes to cloth that cannot. A settle a step instead took
	 * minutes on a skirt.
	 *
	 * **An edge here only ever draws its ends in: cloth resists being stretched, and nothing else.** An edge shorter than it
	 * keeps is slack and is left slack, for a settle under gravity to hang later. Kept at its length both ways, as a pull
	 * keeps it, cloth with slack in it — new cloth grown a little wider than the opening it fills — has no one shape that
	 * satisfies every edge, and the passes never came to rest: the cap reached on a skirt, a line of 57.2 cm left 58.4 long
	 * in millimetre zigzags (GC-GARMENT-CUT-SEW.md §F). At rest once sixteen passes together move nothing more than
	 * `Settled`. `Steps` is how many it walked; `Stretched` the most
	 * any edge is longer than it keeps, a tie included — what the ties join being out of the cloth's reach.
	 */
	static FGcPullResult Draw(FGcPullGraph& Graph, TConstArrayView<int32> Ties, double To, double Settled,
		TFunctionRef<void(TArray<FVector>& Points)> Keep);

	/** How much longer than it keeps any run may be drawn before the cloth counts as taut, in centimetres. */
	static constexpr double TautCm = 0.05;

	/**
	 * Draw each edge of the handle on, a `PullStepCm` at a time, the way its cloth lies at it — from `Behind` to it, turned
	 * by `Along` to follow whatever it lies on, `Keep` laying it over whatever is there — until one more step would leave
	 * one of its runs longer than it keeps: that edge's slack is out and its cloth is snug, and the others go on. Each
	 * draw also eases every run toward straight, as tension does, so a hump comes out rather than standing. The last
	 * step that fitted is kept, then it settles. Every edge keeps its length throughout; the held points hold. `Drawn`
	 * is the furthest any edge went.
	 */
	static FGcPullResult Tighten(FGcPullGraph& Graph, double Settled, TFunctionRef<void(TArray<FVector>& Points)> Keep,
		TFunctionRef<FVector(const FVector& At, const FVector& Ahead)> Along);
};
