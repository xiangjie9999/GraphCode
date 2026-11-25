// Copyright GraphCode Sdn Bhd. All Rights Reserved.

// Does a brush's BUILDER still describe the brush's GEOMETRY?
//
// Row G7's load-bearing question — `GC-WORLD-GENERATION-DESIGN.md` §12.8b. A level document carries
// `BrushBuilder` and omits the model, on the rule that *the builder is AUTHORED and the model is
// DERIVED* (`GraphCodeActorEntryWriter.h`). That rule is only true while the builder and the model
// agree, and **nothing in the engine keeps them agreeing**:
//
// - `FGeomObject::SendToSource` (GeometryMode) writes dragged vertices straight into
//   `Polys->Element` and **never touches `BrushBuilder`**, so a hand-edited brush keeps a builder
//   confidently describing a shape it no longer has. MEASURED 2026-08-20 in UE 5.7; GeometryMode is
//   `"EnabledByDefault": true`, so this is a stock editor's behaviour and not an exotic branch.
// - `Matches` in `GraphCodeWorldGeneratedVolume.cpp` compares builder to builder and never reads the
//   polys, so the generator cannot see the divergence either.
//
// **WHY THIS IS A DATA-LOSS FIX AND NOT A TIDINESS ONE.** `GraphCodeLevelWriter`'s apply calls
// `Brush->BrushBuilder->Build(World, Brush)`, which OVERWRITES `Polys` from the builder. So a
// hand-edited brush projects as its stale builder, and the next apply rebuilds the brush from that
// builder and destroys the edit — silently, completely, and with the document reporting success.
//
// **THE TEST IS SEMANTIC, NOT TEXTUAL.** The user's ruling, 2026-08-20: *"it doesnt need to be exact
// match, just that when converted, both values are the same."* So this rebuilds the builder into a
// scratch brush and compares the resulting SOLIDS under a tolerance. A text comparison would read
// green while a volume quietly changed what it contains — §12.2's *the gate is the hulls came back,
// never the text*, applied to the write side.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

class ABrush;
class UWorld;

namespace GraphCodeVolumeShape
{
	/** Which of §12.5's three brush states this brush is in. */
	enum class EShapeSource : uint8
	{
		/** No model, or no polygons in it — there is nothing to compare and nothing to record. */
		NoGeometry,

		/** The builder rebuilds to the same solid. Rung 1: write its parameters, omit the model. */
		BuilderDescribes,

		/**
		 * A builder is present and the solid has MOVED AWAY from it — the hand-edited case.
		 *
		 * Rung 2/3: the document must record the geometry, and an apply must not rebuild from the
		 * builder alone. Writing only the builder here states a shape the world does not have.
		 */
		BuilderDiverges,

		/** Nothing claims the shape. Rung 3: the geometry is the only honest record. */
		NoBuilder,

		/**
		 * The builder refused to run, so the question has no answer.
		 *
		 * Deliberately NOT folded into `BuilderDiverges`: *we could not tell* and *we checked and it
		 * differs* are different claims, and a caller that silently recorded geometry on a failed
		 * rebuild would turn a transient refusal into a permanent poly dump
		 * ([[refusal-reason-is-a-claim]]).
		 */
		Indeterminate,
	};

	/** What the comparison found, so a caller can report a number rather than an impression. */
	struct GRAPHCODE_API FComparison
	{
		EShapeSource Source = EShapeSource::NoGeometry;

		/** Polygons in the brush as it stands. */
		int32 ActualPolys = 0;

		/** Polygons the builder produced on the scratch brush. */
		int32 RebuiltPolys = 0;

		/** Polygons whose vertex ring did not survive the comparison. */
		int32 DivergentPolys = 0;

		/** Individual vertices that moved — the size of the delta rung 2 would write. */
		int32 DivergentVertices = 0;

		/** Why, when `Source` is `Indeterminate`. */
		FString Error;

		bool DescribesItsGeometry() const { return Source == EShapeSource::BuilderDescribes; }
	};

	/**
	 * One scratch brush, held across a whole projection pass.
	 *
	 * **MEASURED, not a micro-optimisation** — `GC-WORLD-GENERATION-DESIGN.md` §12.8c. Spawning the
	 * scratch actor, giving it a `UModel` and destroying it costs **0.0801 ms**, against 0.1535 ms
	 * for an entire cube comparison: roughly half the cost of a small brush, and nearly all of it in
	 * the commonest case, paid once per brush regardless of how many polygons it has. A caller
	 * walking a level holds one of these and pays that once for the whole level.
	 *
	 * The single-brush `Compare` still works and still spawns its own, so no call site is obliged to
	 * know this exists ([[a-write-finishes-itself]]).
	 */
	class GRAPHCODE_API FScratchBrush
	{
	public:
		explicit FScratchBrush(UWorld& InWorld);
		~FScratchBrush();

		FScratchBrush(const FScratchBrush&) = delete;
		FScratchBrush& operator=(const FScratchBrush&) = delete;

		/** The brush to build into, or null if it could not be created or has been garbage collected. */
		ABrush* Get() const;

	private:
		TWeakObjectPtr<ABrush> Scratch;
	};

	/**
	 * Compare a brush's geometry against a rebuild of its own builder.
	 *
	 * `Tolerance` is in Unreal units and defaults to the same 0.1 the surrounding shape comparisons
	 * use (`UGraphCodeWorldRegionPrismBuilder::HasSameShape`, `Matches`). **The vertices being
	 * compared are `FVector3f` — FLOAT** (`Polys.h`), so an exact comparison would fail on ordinary
	 * float drift even where nothing was touched.
	 *
	 * Non-destructive: the rebuild happens on a transient scratch brush and the brush passed in is
	 * never modified ([[reading-must-not-write]]).
	 */
	GRAPHCODE_API FComparison Compare(const ABrush& Brush, double Tolerance = 0.1);

	/** The same comparison, reusing a scratch brush the caller holds across many calls. */
	GRAPHCODE_API FComparison Compare(const ABrush& Brush, FScratchBrush& Reusable,
		double Tolerance = 0.1);

	// ---- The memo: answer from content, so an unchanged brush is never rebuilt twice ------------
	//
	// **THE USER'S DESIGN, 2026-08-23** — *"the volume itself can save the hash or something like
	// that. then only compare the hash, before and after, not compare with our current shape."*
	//
	// §12.8c deferred a stored digest on three grounds and the third was *a digest is a second
	// carrier of the geometry fact, whose dangerous failure direction is `says unchanged when
	// changed`*. **That objection is about a digest STORED BESIDE the geometry and kept in step by
	// hand.** This one is not stored anywhere: it is computed from the brush every call, and it is
	// only ever used to look up an answer we already worked out for that exact content. There is no
	// second carrier to go stale, which is why it is safe where §12.8c's was not
	// ([[two-carriers-of-one-fact-is-a-race]] is about the copy, not about the hashing).
	//
	// **UNREAL'S OWN DIGEST WAS CHECKED FIRST AND CANNOT DO THIS JOB — §12.8d.**
	// `UModel::LightingGuid` really is a deterministic Blake3 over the model, rebuilt in `PreSave`.
	// But `ConstructLightingGuid` hashes `Polys` through the `TObjectPtr` overload, which records the
	// PATH NAME ONLY — and `Polys` is exactly where a brush's editable shape lives. So it moves for
	// BSP-derived arrays and would sit still through the hand edit this whole file exists to catch.
	// `UBodySetup::BodySetupGuid` is worse: its own comment admits it is `FGuid::NewGuid()` per
	// invalidation, a change indicator rather than a content hash.

	/**
	 * A digest of everything the comparison reads: the tolerance, the brush's transform, its
	 * builder's class and properties, and every polygon's vertices and flags.
	 *
	 * **IT MUST COVER A SUPERSET, NEVER A SUBSET.** Hashing something the comparison ignores costs a
	 * spurious recompute, which is correct and slower. Missing something the comparison reads returns
	 * a stale answer, which is silent loss — the exact failure this file exists to prevent. When in
	 * doubt about a field, hash it.
	 */
	struct GRAPHCODE_API FShapeDigest
	{
		uint64 Low = 0;
		uint64 High = 0;

		bool IsValid() const { return Low != 0 || High != 0; }
		bool operator==(const FShapeDigest& Other) const
		{
			return Low == Other.Low && High == Other.High;
		}
		bool operator!=(const FShapeDigest& Other) const { return !(*this == Other); }
	};

	/** Compute that digest. Reads the brush and writes nothing ([[reading-must-not-write]]). */
	GRAPHCODE_API FShapeDigest Digest(const ABrush& Brush, double Tolerance = 0.1);

	/** What the memo has done, so a caller can PROVE it is working rather than assume it. */
	struct GRAPHCODE_API FMemoStats
	{
		/** Comparisons answered from the memo — a rebuild and a scratch actor not paid for. */
		int32 Hits = 0;

		/** Comparisons that had to do the real work, and were recorded. */
		int32 Misses = 0;

		/** Brushes currently remembered. */
		int32 Entries = 0;
	};

	GRAPHCODE_API FMemoStats MemoStats();

	/**
	 * Forget everything remembered.
	 *
	 * For tests, and for anybody who wants the next comparison to be the real one. Correctness never
	 * depends on this being called: the digest decides every hit, so a memory that is merely OLD is
	 * still right.
	 */
	GRAPHCODE_API void ForgetMemo();

	// ---- Rung 3: the geometry itself, for the `volumes { }` block --------------------------------
	//
	// When `Compare` answers `BuilderDiverges` or `NoBuilder`, the builder is no record of the shape
	// and the document must carry the geometry directly (`GC-WORLD-GENERATION-DESIGN.md` §12.9d). The
	// form is INDEXED — a vertex table plus faces as index lists — because the flat poly list the
	// engine emits repeats every position ~5x (§12.8 result 2), and the brush is already built that
	// way internally (`Corners` + `Faces`), so this is the shape as CONSTRUCTED, not a new invention.

	/**
	 * A brush's geometry as an indexed mesh: distinct positions, faces referencing them by index.
	 *
	 * **`Points` is `FVector3f` — FLOAT — because `FPoly::Vertices` is** (`Polys.h`). Writing the
	 * document at double precision would spell digits the source does not have (§12.8 result 4). No
	 * `Base`, `Normal`, `TextureU` or `TextureV`: `FPoly::Finalize` derives all four, and storing them
	 * costs +291–347% (§12.8 result 3).
	 */
	struct GRAPHCODE_API FIndexedGeometry
	{
		TArray<FVector3f> Points;

		/** One entry per polygon: the ordered ring of indices into `Points`. */
		TArray<TArray<int32>> Faces;

		bool IsEmpty() const { return Faces.Num() == 0; }
	};

	/**
	 * Read a brush's polygons into the indexed form the `volumes { }` block records.
	 *
	 * Vertices within `WeldTolerance` of one already seen collapse to the same index — that is what
	 * turns the builder's ~5x-duplicated flat list back into distinct positions. The tolerance is
	 * DELIBERATELY far tighter than the 0.1 hull comparison uses: welding is meant to undo bit-exact
	 * corner sharing and float drift, never to merge two positions a shape actually distinguishes.
	 */
	GRAPHCODE_API FIndexedGeometry Extract(const ABrush& Brush, float WeldTolerance = 0.01f);

	/**
	 * Write indexed geometry into a brush's model, replacing its polygons.
	 *
	 * Each face becomes an `FPoly` whose `Finalize` derives the metadata this form deliberately does
	 * not store. **This does NOT run the CSG pass** — collision comes from `Nodes` via
	 * `UBodySetup::CreateFromModel`, not from `Polys`, so the caller must run
	 * `FBSPOps::csgPrepMovingBrush` after this for a volume's hulls to reflect the new shape (§12.9d).
	 * Kept out of here so this stays a pure poly write that a test can inspect before the CSG runs.
	 *
	 * Returns the number of faces written; a face that `Finalize` rejects as degenerate is dropped and
	 * NAMED in `OutDropped` rather than silently skipped ([[refusal-reason-is-a-claim]]).
	 */
	GRAPHCODE_API int32 Rebuild(ABrush& Brush, const FIndexedGeometry& Geometry,
		TArray<int32>* OutDroppedFaces = nullptr);

	// ---- Rung 2: the builder, plus the corners that moved ----------------------------------------
	//
	// Rung 3 above states a WHOLE SHAPE. But state 3 is normally reached by a person nudging a brush
	// a builder made, and the honest description of that world is *the prism, with these corners
	// moved* (`GC-WORLD-GENERATION-DESIGN.md` §12.8a). **MEASURED: a one-vertex edit on a 64-point
	// disc is 36 characters against 6,271 — 175x — and a delta's cost scales with the EDIT, not with
	// the shape.**
	//
	// **THE BASE IS THE BUILDER ON THE SAME LINE OF THE SAME DOCUMENT**, never a pointer into a
	// `.gcw`, so §12.1's ruling that a `.gc` holds absolute values is untouched — nothing here names
	// a region. It is also why a delta is only ever a COMPRESSION: `ResolveDelta` turns it back into
	// a whole shape before anything is written, so the apply path below rung 2 and rung 3 is one
	// path and cannot drift into two behaviours.

	/** One corner that is not where the builder put it. */
	struct FMovedCorner
	{
		/** Into the point table of the geometry the BUILDER produces — not of the brush as it stands. */
		int32 Index = 0;

		/** Where it actually is. `FVector3f` for `FPoly::Vertices`' own reason. */
		FVector3f Position = FVector3f::ZeroVector;
	};

	/** A whole hand edit, as the set of corners it moved. */
	struct GRAPHCODE_API FVertexDelta
	{
		TArray<FMovedCorner> Corners;

		bool IsEmpty() const { return Corners.Num() == 0; }
	};

	/**
	 * The geometry this brush's BUILDER describes — the base a `moved(…)` entry counts into.
	 *
	 * Rebuilt onto the scratch, welded by `Extract`'s own rule so the two tables are numbered the
	 * same way, and the live brush is never touched ([[reading-must-not-write]]). False, with a
	 * reason, when there is no builder or it refuses to run — the two cases `Compare` answers
	 * `NoBuilder` and `Indeterminate` for, and neither has a base to count into.
	 *
	 * **It rebuilds a second time rather than borrowing `Compare`'s rebuild**, deliberately: only a
	 * brush that has already answered `BuilderDiverges` ever reaches here, which on any real corpus
	 * is a handful of brushes, and the alternative is carrying a geometry table out of every one of
	 * the thousands of comparisons that answer `BuilderDescribes` and want nothing.
	 */
	GRAPHCODE_API bool BuilderGeometry(const ABrush& Brush, FScratchBrush& Reusable,
		FIndexedGeometry& OutGeometry, FString& OutError, float WeldTolerance = 0.01f);

	/**
	 * Which corners moved, between what the builder describes and what the brush holds.
	 *
	 * **False means rung 3 is owed, and it is not an error.** A delta can only say *this corner is
	 * elsewhere*; it cannot say a face was added, a ring gained a vertex, or two corners that used to
	 * be one have come apart. So the two tables must be index-compatible — same faces, same rings,
	 * same point count — and `OutReason` names which of those failed so the fall-back is explicable
	 * rather than mysterious.
	 *
	 * A moved corner is ONE entry however many faces share it: `FGeomObject::SendToSource` writes a
	 * dragged vertex into every copy, so a welded table sees one point move, which is exactly why
	 * §12.8a's 36 bytes is 36 and not 108.
	 */
	GRAPHCODE_API bool MeasureDelta(const FIndexedGeometry& Base, const FIndexedGeometry& Actual,
		FVertexDelta& OutDelta, FString& OutReason, double Tolerance = 0.1);

	/**
	 * The base with the delta's corners moved onto it — a whole shape again.
	 *
	 * The inverse of `MeasureDelta` and the apply side's first step. An index the base has no point
	 * for is a NAMED failure, never a dropped move: a delta that half-applies leaves a solid nobody
	 * authored, which is worse than one that refuses ([[a-writer-that-validates-as-it-writes]]).
	 */
	GRAPHCODE_API bool ResolveDelta(const FIndexedGeometry& Base, const FVertexDelta& Delta,
		FIndexedGeometry& OutGeometry, FString& OutError);

	// ---- The text spelling, kept beside the model so the two cannot drift ------------------------
	// [[a-reader-that-spells-owes-the-writer-an-inverse]]: one file owns both directions.

	/**
	 * One volume's entry — **ONE LINE, whole**. USER-RULED 2026-08-20.
	 *
	 *   <Name> = points(<x> <y> <z>, <x> <y> <z>, …) faces(<i> <j> <k> …, <i> <j> <k> …)
	 *
	 * The first design gave each point and each face its own line inside nested `points { }` and
	 * `faces { }` blocks, on the argument that a one-point edit would then be a one-line diff. **The
	 * user replaced it with a one-liner and the merge argument does not survive contact with the
	 * data**: the faces reference points BY INDEX, so any insertion that is not an append renumbers
	 * every face anyway — the per-line form bought a clean diff only for the case where a coordinate
	 * changes and nothing moves. A brush is one object and this is one statement about it.
	 *
	 * `Name = value(…)` is the level document's existing entry shape — `actors { }` and
	 * `sublevels { }` both spell an entry that way — so this joins the vocabulary rather than
	 * inventing a second one ([[a-new-token-not-a-new-line]]).
	 *
	 * Points stay UNNAMED and ORDERED: nothing references them by name, and naming them would
	 * renumber the whole entry on an insert. Written at float precision.
	 */
	GRAPHCODE_API FString FormatEntry(const FString& Name, const FIndexedGeometry& Geometry);

	/**
	 * The same entry shape for rung 2 — **ONE LINE, and one clause instead of two**.
	 *
	 *   <Name> = moved(<i> <x> <y> <z>, <i> <x> <y> <z>, …)
	 *
	 * An index then a position, spaces inside an element and commas between them: `points(…)`'s own
	 * spelling with one more number in front, so this is a new TOKEN and not a new grammar
	 * ([[a-new-token-not-a-new-line]]). A reader who knows what `points(…)` means can read this.
	 *
	 * **The clause that is absent is the load-bearing one.** There is no `points(…)` here and no
	 * `from(…)` naming a base, because the base is not a choice: it is the `BrushBuilder` the same
	 * document already writes on this actor's line, and a second way to name it would be a second
	 * carrier of one fact ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	GRAPHCODE_API FString FormatDelta(const FString& Name, const FVertexDelta& Delta);

	/** One parsed `volumes { }` entry: the name, and EITHER a whole shape or the corners that moved. */
	struct FParsedVolume
	{
		FString Name;

		/** Rung 3. Empty when this entry is a delta. */
		FIndexedGeometry Geometry;

		/** Rung 2. Empty when this entry is a whole shape. */
		FVertexDelta Delta;

		/** Which rung this entry is, asked the only way that cannot disagree with what it holds. */
		bool IsDelta() const { return !Delta.IsEmpty(); }
	};

	/**
	 * Parse a `volumes { }` block body (without the outer `volumes { }`) into its entries.
	 *
	 * The exact inverse of `FormatEntry` and `FormatDelta` — ONE LINE PER VOLUME — and it validates
	 * as it reads: a face index outside the point table, a point without three numbers, a missing
	 * `points(…)`/`faces(…)`, a `moved(…)` element without four numbers, an entry that is BOTH forms
	 * or NEITHER, or a duplicate volume name is a named failure, never a silently dropped line
	 * ([[a-writer-that-validates-as-it-writes]]).
	 */
	GRAPHCODE_API bool ParseBlock(const FString& BlockBody, TArray<FParsedVolume>& OutVolumes,
		FString& OutError);
}
