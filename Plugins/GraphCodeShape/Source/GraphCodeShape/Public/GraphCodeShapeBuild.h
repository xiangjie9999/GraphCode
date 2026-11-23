// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeShapeProgram.h"

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * A `socket()` a part declared, as `BuildPart` hands it over: in the part's own frame, where it is, the way it faces
 * (the socket's +Z) and its up (+X) — what another module places the part by, never a body's working-out.
 */
struct FGcShapePartSocket
{
	FString Name;
	FVector Location = FVector::ZeroVector;
	FVector Facing = FVector::UpVector;
	FVector Up = FVector::ForwardVector;
};

/**
 * What another module's `add` of a part asked Shape to make copies with — its array words, read by Shape
 * (`GC-SHAPE-GRAMMAR.md` §4.2) — and how the copies are found: from one placed copy, over a surface, or down a path.
 */
struct FGcShapePartArrangement
{
	enum class EKind : uint8
	{
		/** No copies: the caller places the one part itself. */
		One,
		/** `repeat=` with `step=`/`spread=`, or `turn=`/`around=`: grown from the one the caller placed. */
		Lattice,
		/** `scatter=` or `onto=`: over the caller's surface. */
		Surface,
		/** `along=$Name`: down one of the caller's paths. */
		Path,
	};

	EKind Kind = EKind::One;
	int32 Count = 1;

	/** `orient=keep`: each copy keeps the arrangement's own turn rather than following the surface or path. */
	bool bKeep = false;

	/** `scatter=` rather than `onto=`, on a surface. */
	bool bScatter = false;

	/** `onto=`'s direction, and its `gap=`. */
	FVector Onto = FVector::ZeroVector;
	double OntoGap = 0.0;

	/** `along=`'s path name, without its `$`, and whether `spread=` fills only part of it — the middle, Shape's rule. */
	FString Along;
	bool bSpread = false;

	/** The array words as written, for `ArrangePart`. */
	TArray<TPair<FString, FString>> Words;
};

/** Where a caller's own surface and paths are, for `ArrangePart`. Every field is optional; one an arrangement needs and lacks is refused. */
struct FGcShapePartSurface
{
	/** What `onto=` casts at and `scatter=` draws over, and which of its triangles — chosen by the caller, so no `[ ]` is asked for. */
	const UE::Geometry::FDynamicMesh3* Mesh = nullptr;
	TArray<int32> Triangles;

	/** `along=$Name`'s paths, by name: points in order, and whether the last joins back to the first. */
	struct FPath
	{
		TArray<FVector> Points;
		bool bClosed = true;
	};
	TMap<FString, FPath> Paths;

	/** Where the one copy a lattice grows from was placed, and the way it faces. */
	FVector Placed = FVector::ZeroVector;
	FVector PlacedFacing = FVector::UpVector;

	/** What a lattice's `spread=` percentage is a fraction of. */
	FBox Extent = FBox(ForceInit);

	/** What keys `jitter=` and `scatter=`, as an entry's name does in a `.gcshape`: two lines with one `seed=` are two draws. */
	FString Key;
};

/** One copy, as a caller that seats copies by its own rule reads it. */
struct FGcShapePartCopy
{
	/** Where it goes: the hit, the point on the path, or where the lattice moved the placed copy to. */
	FVector Place = FVector::ZeroVector;

	/** The way that place faces: the hit's outward normal, the path's direction, or the placed copy's facing turned with it. */
	FVector Facing = FVector::UpVector;

	/**
	 * On a surface or a path, the copy's own turn inside the frame it is seated in — `jitter=`'s, and under `orient=keep` the
	 * whole of it, in the caller's frame. In a lattice, its turn from the copy it grew from (the orbit, the spin, the jitter).
	 */
	FQuat Turn = FQuat::Identity;

	/** `jitter=(scale=)`'s. */
	double Scale = 1.0;
};

/**
 * RUN A `.gcshape`: parse it, evaluate what its `build { }` block names, and put the assets on disk.
 * `GC-SHAPE-AUTHORING-DESIGN.md` §15 M1; `GC-SHAPE-GRAMMAR.md` §1–§2 is the grammar.
 *
 * **THE MANIFEST IS THE ONLY REACHABILITY RULE.** An entry becomes an asset only by a build line
 * naming it; an entry no build line reaches is REPORTED and skipped, never built on spec.
 *
 * **M1 IS THE SIMPLEST WHOLE PATH AND IT IS DELIBERATELY NARROW** — `circle`, `rect` and
 * `extrude(height=)`. What that proves is the part every later milestone rests on: text in, a real
 * `UStaticMesh` on disk, and every refusal that is a property of the FILE rather than of the shape.
 * M2 changes what a number is, M3 changes what a name is, and both would be a rewrite of this if
 * taken first.
 */
class GRAPHCODESHAPE_API FGraphCodeShapeBuild
{
public:
	/**
	 * The seam's body. `DocumentPath` is the caller's `asset_path`, still unread — M1 does not need
	 * it, because a `.gcshape` says where its own assets go (`into:` and the build lines).
	 *
	 * **A DRY RUN EVALUATES EVERYTHING AND WRITES NOTHING** — it parses, resolves every build line,
	 * builds every mesh and reports the triangle counts. That is the only kind of dry run worth
	 * having here: a plan that stopped at the parse would report success on a file whose geometry
	 * cannot be built ([[dry-run-must-not-write-anything]] is the other half, and it holds).
	 */
	static bool Run(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		FGcShapeBuildResult& OutResult, FString& OutError);

	/**
	 * `Run` in its stages (GC-APPLY-ASYNC-PLAN.md E), the seam's starter. Game thread. False is a refusal before
	 * anything left the game thread; true with no job is a result already whole (a library); true with a job is a build
	 * evaluating on a worker (`gc.Shape.EvaluateOnWorker`, default 1) that writes each target on the game thread before
	 * the next. `Run` is this, then a wait that serves the job's game-thread work, then `Finish`.
	 */
	static bool Begin(const FString& DocumentPath, const FString& ProgramText, bool bDryRun,
		TSharedPtr<FGcShapeBuildJob>& OutJob, FGcShapeBuildResult& OutResult, FString& OutError);

	/**
	 * Does this program's `build { }` still produce `PackagePath`, from an entry that exists?
	 *
	 * **THE SAME TWO QUESTIONS `Run`'S STATIC PASS ASKS, and deliberately no more.** `Run` resolves
	 * every build line before evaluating any of them, so a typo on the last line refuses having
	 * written none of the earlier ones — which means the H28 check was already written and simply
	 * had no caller until something rebuilt the file. This is that caller, and it stops where the
	 * static pass stops asking about the TEXT: it does not evaluate a shape, so it cannot and does
	 * not claim the entry would still build.
	 *
	 * A parse failure is `false` with the parser's own reason, unchanged. That is the right answer
	 * to *is the recipe here* for a file nobody can read.
	 */
	static bool HoldsRecipeFor(const FString& ProgramText, const FString& PackagePath,
		FString& OutReason);

	/**
	 * One entry of a `.gcshape`, built as a part another module places — Garment's
	 * `Buckle = shape('/…', Width = 6)` then `add($Buckle.Buckle)`. Reached exactly as a `.gcshape`
	 * reaches it, an import line and an `add` into an empty mesh, so every rule Shape has for crossing
	 * a file applies unchanged. `OutMesh` is in the part's own frame, its hard edges shaded as its file
	 * asks; `OutFoldedText` is every file it was built from, for the placing asset's digest. `OutSockets`,
	 * where given, gains the entry's sockets in the same frame — a buckle's openings, which a garment's
	 * `guide()` passes through.
	 *
	 * `Host`, where given, is what the part is seated on, in its own frame: an entry declaring a `Host` parameter is
	 * handed it — the ruled spelling, `Plate = mesh(Host = $Flat) { … conform(to=$Host) [bottom] }`
	 * (GC-GARMENT-SHAPE-PARTS.md §7 item 3) — and one that does not builds as if none were given. `bOutTakesHost` says
	 * which, so a caller builds a part once per place only when it fits itself.
	 */
	static bool BuildPart(const FString& Alias, const FString& ShapePath,
		const TArray<TPair<FString, FString>>& ImportArguments, const FString& Entry,
		const TArray<TPair<FString, FString>>& EntryArguments, UE::Geometry::FDynamicMesh3& OutMesh,
		FString& OutFoldedText, TArray<FString>& OutNotes, FString& OutError,
		TArray<FGcShapePartSocket>* OutSockets = nullptr, const UE::Geometry::FDynamicMesh3* Host = nullptr,
		bool* bOutTakesHost = nullptr);

	/**
	 * `BuildPart`'s other half: another module's `add` of a part, with Shape's array words on it — Garment's
	 * `add($Hardware.Rivet, scatter=60)` (GC-GARMENT-SHAPE-PARTS.md §7). Keeps the words that make copies, read by
	 * Shape's own reader so every refusal is Shape's, and hands back the rest as the entry's arguments. The caller
	 * never learns the words; it asks `ArrangePart` where the copies go and seats each by its own rule.
	 */
	static bool ReadArrangement(const TArray<TPair<FString, FString>>& Arguments, FGcShapePartArrangement& Out,
		TArray<TPair<FString, FString>>& OutEntryArguments, FString& OutError);

	/**
	 * Where each copy `Arrangement` asks for goes on the caller's surface and paths — Shape's own arranging
	 * (`GraphCodeShapeArray::Resolve`), the copies handed back as places rather than placed, since the caller seats them.
	 * An `onto=` ray that hits nothing places nothing; none landing is refused.
	 */
	static bool ArrangePart(const FGcShapePartArrangement& Arrangement, const FGcShapePartSurface& On,
		TArray<FGcShapePartCopy>& OutCopies, FString& OutError);

	/**
	 * LOWER THE PROGRAM-WIDE TRIANGLE CEILING FOR ONE SPEC. Zero restores the shipped number.
	 *
	 * **THE SAME BARGAIN `GraphCodeShapeRefine::ForFeature` ALREADY MAKES WITH ITS OWN CEILING**
	 * (row MR25). The shipped budget is eight million triangles standing at once — about two
	 * gigabytes — and a spec that had to reach it to watch the guard fire would allocate more than a
	 * suite anybody keeps running should. A small budget produces the identical refusal, because
	 * there is one sentence and it prints whatever number is in force.
	 *
	 * **NO CALLER IN THE PRODUCT USES IT.** The shipped number lives in one `constexpr` and this
	 * only shadows it, so there is no second place for it to drift to.
	 */
	static void SetProgramTriangleBudgetForTests(int64 Budget);

	/**
	 * LOWER THE PROGRAM-WIDE BOOLEAN-WORK CEILING FOR ONE SPEC. Zero restores the shipped number.
	 *
	 * **THE SAME BARGAIN, FOR THE OTHER BUDGET** (row MV10). The shipped number is sixty-four
	 * million triangles through mesh booleans — fifteen times the largest program in the corpus, and
	 * five to seven minutes of solid boolean work — so a spec that reached it honestly would be the
	 * hang it exists to prevent. The refusal is one sentence and prints whatever number is in force.
	 */
	static void SetProgramBooleanWorkBudgetForTests(int64 Budget);

	/**
	 * TURN THE BOOLEAN RE-CUT OFF FOR ONE SPEC, AND BACK ON. True is the shipped behaviour.
	 *
	 * A `subtract`, an `intersect` or a hard `add` that leaves new `open` or `seam` after its repairs is cut
	 * again with its two operands balanced where they cross (`GC-SHAPE-BOOLEAN-ON-LOFTS-DIAGNOSIS.md` §5,
	 * stage 1). With this false every boolean runs once, as it did before the re-cut existed, so a spec can
	 * build one program both ways: a clean boolean must give the same mesh digest either way, and a fixture
	 * the re-cut mends must still be damaged without it, or the spec is testing nothing.
	 *
	 * **NO CALLER IN THE PRODUCT USES IT.** It returns the setting it replaced, and a spec restores THAT value
	 * before it returns rather than assuming the shipped one. The flag is atomic: a build may evaluate on a
	 * worker thread.
	 */
	static bool SetBooleanRecutForTests(bool bEnabled);

	/**
	 * What the boolean re-cut did since the counters were last reset, FOR SPECS ONLY: how many booleans passed
	 * its damage gate, how many re-cuts ran, how many of those were kept and rejected, and how many were skipped
	 * because the file's re-cut allowance was spent. The product path pays one relaxed atomic increment per event.
	 */
	struct FBooleanRecutCounts
	{
		int32 Gates = 0;
		int32 Tried = 0;
		int32 Kept = 0;
		int32 Rejected = 0;
		int32 Skipped = 0;
	};

	/** Read the counters; with `bReset`, zero them after reading. No caller in the product. */
	static FBooleanRecutCounts BooleanRecutCountsForTests(bool bReset = false);

	/**
	 * Called with every build target's FINISHED mesh (shaded, tangents made, pivot applied), dry run
	 * or not, on the game thread just before the target is written: the program's path, the asset's
	 * package path, the entry and the mesh. `GraphCode.Shape.ZZCorpusDigestProbe` reads the corpus
	 * through it (`GC-SHAPE-FEATURE-LINES-DESIGN.md` Slice 0). An empty function removes it. No caller
	 * in the product sets it.
	 */
	using FTargetMeshObserver = TFunction<void(const FString& /*DocumentPath*/, const FString& /*PackagePath*/,
		const FString& /*Entry*/, const UE::Geometry::FDynamicMesh3& /*Mesh*/)>;
	static void SetTargetMeshObserverForTests(FTargetMeshObserver Observer);

	/**
	 * **TEST-ONLY: LIFTS "A BUILT ASSET NEVER GOES INTO A PLUGIN" FOR AS LONG AS IT LIVES** (owner, 2026-11-20). Every
	 * build target and bake output must lie under `/Game/`; a spec that has to watch a target outside it pass the
	 * resolver holds one of these. No caller in the product, the bridge or a tool makes one, and nothing a file or a
	 * request says can open it.
	 */
	struct GRAPHCODESHAPE_API FPluginTargetsForTests
	{
		FPluginTargetsForTests();
		~FPluginTargetsForTests();
		FPluginTargetsForTests(const FPluginTargetsForTests&) = delete;
		FPluginTargetsForTests& operator=(const FPluginTargetsForTests&) = delete;
	};
};
