// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

struct FGcShapeDocument;
struct FGcShapeHeader;
struct FGcShapeInstruction;
class IGcShapeVolumes;

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **GROUP 6 — WHAT THE SHAPE WEARS, AND THE TWO CHORES NOBODY WRITES** (`GC-SHAPE-GRAMMAR.md` §4
 * group 6, §5, §6; M7).
 *
 * **A `material()` ENTRY IS A SLOT AND A PATH IS A FIRST-BUILD DEFAULT** — user-caught 2026-08-23,
 * because `GC-PRODUCT-SPLIT.md` §5.2 already gave *what the asset wears* to `kind:static_mesh`'s
 * `.gc`. Two carriers of one fact is a race whose losing side was already picked: paint the material
 * in the editor, the `.gc` projects it, and a rebuild that re-applied the path would silently revert
 * somebody's work ([[two-carriers-of-one-fact-is-a-race]]).
 *
 * **A TRIANGLE'S MATERIAL ID IS THE FILE-WIDE DECLARATION ORDINAL PLUS ONE, AND ZERO IS UNPAINTED.**
 * That offset is the whole mechanism and it is what makes three separate rules fall out rather than
 * be arranged for: slot ORDER is declaration order because the id already is; `add($Stud)` brings
 * the stud's own paint with it because a merge copies the attribute and the ordinals mean the same
 * thing in every entry of one file; and *a file declaring three materials gives one asset two slots
 * and another one* because the asset's slots are the DISTINCT ids its own triangles carry, compacted
 * at the door. A per-entry numbering would have needed a remap at every merge, and the first one
 * missed would paint a stud with the wall's stone and report success.
 *
 * **AN ORDINAL MEANS THE SAME THING ONLY INSIDE ONE FILE**, so a part from another file is
 * renumbered once, where it is handed over — `SlotAcross`, row MB35. Until then a nut from a kit
 * arrived wearing the caller's first material, whatever the import had passed.
 *
 * **UNPAINTED IS A SLOT TOO.** A static mesh with no assignment has one empty slot anyway, so a file
 * that never says `material()` is not incomplete — it has handed that decision to the `.gc`, where a
 * person can already change it.
 */
namespace GraphCodeShapeSurface
{
	/** One slot of the finished asset: the entry's name, and the path that fills it on a CREATE. */
	struct FGcShapeSlot
	{
		/** The `material()` entry's name — which is the slot's name in the asset. */
		FString Name;

		/** `'/Game/…'` as written, unquoted. Empty for a declared-but-empty slot. */
		FString Path;

		double TexelDensity = 0.0;
	};

	/** Ids at or above this are slots a part brought from another file: `Arrived[Id - ArrivedSlotBase]`. MB35. */
	inline constexpr int32 ArrivedSlotBase = 1 << 20;

	/** `uv(scale=)`'s default, centimetres per tile (`GC-SHAPE-GRAMMAR.md` §4 group 6): what the header's policy spends, and what a verb writing its own new faces' UVs matches. */
	inline constexpr double CentimetresPerTile = 100.0;

	/**
	 * `Id` as `From` painted it, in `To`'s numbering — the one remap a file boundary owes (MB35). A
	 * slot an import or a call filled follows its forward to `To`; else `To`'s slot of the same name
	 * claims it; else it arrives as its own slot, added to `Arrived` once per name. Zero stays zero.
	 */
	GRAPHCODESHAPE_API int32 SlotAcross(const FGcShapeDocument& From, int32 Id, const FGcShapeDocument& To,
		TArray<FGcShapeSlot>& Arrived);

	/**
	 * `material($Slot)` — which faces wear which slot. §4 group 6.
	 *
	 * Aimed by the `[ ]` bracket, and **where two lines cover one face the LAST one wins**, because
	 * a material id is one integer per triangle and there is nothing else it could mean. That is not
	 * a rule this function enforces — it is what writing the id twice does.
	 */
	GRAPHCODESHAPE_API bool RunMaterial(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * `uv(keep|auto|box|cylinder|plane|none, at=, facing=, up=, scale=, offset=)` — CHANNEL
	 * 0, and never channel 1. The five named arguments belong to the three projections alone.
	 *
	 * A lightmap needs a second, non-overlapping channel and that is a BUILD setting on the asset —
	 * `bGenerateLightmapUVs`, `LightMapCoordinateIndex` — which the same §5.2 seam gives to
	 * `kind:static_mesh`'s `.gc`.
	 *
	 * **THE FRAME IS SHAPE'S OWN: `at=` ORIGIN, `facing=` LOCAL +Z, `up=` LOCAL +X.** The default is
	 * the old bounds-centred identity frame. `scale=` remains centimetres per tile: scalar broadcasts,
	 * while plane takes `(u,v)`, box `(x,y,z)` and cylinder `(around,along)`. `offset=(u,v)` is in
	 * unitless tiles and transforms only elements produced by this projection. `auto`, `keep` and
	 * `none` have no projection frame and refuse every one of these named arguments.
	 */
	GRAPHCODESHAPE_API bool RunUv(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, UE::Geometry::FDynamicMesh3& Mesh, FString& OutError,
		TArray<FString>* Notes = nullptr);

	/**
	 * `colour(<value>, channel=r|g|b|a|rgb, mix=set|add|multiply|max, strength=, per=vertex|copy,
	 * min=, max=, seed=) [ ]` — the primary vertex colour, the one a material's `Vertex Color` node
	 * reads. `GraphCodeShapeColour.cpp` has the rules.
	 *
	 * Aimed by the same bracket every surface verb reads, so a `select($Region, weight=$Field)` carries
	 * its `weightage()` rules in: the weight at each vertex is how far the line moves the colour toward
	 * its value. `per=copy` reads ONE value per placed copy — the triangle's lineage owner, which
	 * `CopyOf` names as a stable string — drawn from `random` with `seed=`, or the mean weight over it.
	 */
	GRAPHCODESHAPE_API bool RunColour(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header, UE::Geometry::FDynamicMesh3& Mesh,
		const TFunctionRef<FString(int32)> CopyOf, FString& OutError);

	/** A white, opaque colour per vertex where the mesh has none: what an unwritten channel reads. */
	GRAPHCODESHAPE_API void EnsureColour(UE::Geometry::FDynamicMesh3& Mesh);

	/** `MatchPaint` for colour: both sides of a merge carry the overlay, or one side's colour is dropped. */
	GRAPHCODESHAPE_API void MatchColour(UE::Geometry::FDynamicMesh3& A, UE::Geometry::FDynamicMesh3& B);

	/** How many colour elements the mesh carries — zero where no `colour()` ran. The measure line's `colours`. */
	GRAPHCODESHAPE_API int32 ColourCount(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * The header's `uv:` policy, run ONCE after an entry's final instruction — and only for an entry
	 * that never said `uv()` itself.
	 *
	 * **ITS DEFAULT IS ALWAYS `keep`.** It used to read `auto` where a boolean ran and `keep`
	 * otherwise, on the theory that a boolean destroys UVs and a carved shape wants unwrapping — but
	 * `auto` (`FPatchBasedMeshUVGenerator`) does not land on the same island partition twice, even
	 * with its own merge stage made deterministic (see `GcShapeAutoComputeUvsDeterministic` in the
	 * `.cpp`): patch SEEDING races too (`FPolygroupsGenerator::FindPolygroupsFromAreaDensityPointSampling`'s
	 * `ParallelForWithExistingTaskContext`), so a booleaned `.gcshape` was re-unwrapping itself on
	 * every rebuild (`Docs/NEXT.md`, `GraphCode.Shape.ABooleanedShapeRebuildsToTheSameMeasureLine`).
	 * An author who wants the auto unwrap still has it — `uv(auto)` on the build line, unaffected by
	 * this default and strictly more deterministic than before — but nothing silent takes that path.
	 */
	GRAPHCODESHAPE_API bool RunDefaultUv(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeHeader& Header, UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * **BOTH SIDES OF A MERGE CARRY MATERIAL IDS, OR ONE SIDE'S PAINT IS DROPPED SILENTLY.**
	 *
	 * `FDynamicMeshEditor::AppendMesh` and `FMeshBoolean` copy the attribute where BOTH meshes have
	 * it; where only one does, the ids it carried do not survive into the result. An unpainted part
	 * added to a painted wall is the ordinary case, so this is called before every group-5 merge
	 * rather than left to whichever pair happens to match.
	 */
	GRAPHCODESHAPE_API void MatchPaint(UE::Geometry::FDynamicMesh3& A,
		UE::Geometry::FDynamicMesh3& B);

	/**
	 * **NORMALS BY CONSTRUCTION AND BY ANGLE — `shade:`, the angle above which an edge shades HARD.**
	 *
	 * One number does the whole job: a cylinder's segments meet at ~4 degrees and shade smooth, a
	 * `segments=6` nut's faces meet at 60 and stay faceted, a cap meets its side at 90 and stays
	 * crisp. `shade:0` is fully faceted and `shade:80` is nearly all-smooth.
	 *
	 * It is a chore (R4), so the evaluator does it and the file never mentions it beyond the header.
	 * Run once on the finished asset mesh rather than per instruction: the result is the same and a
	 * merge would have invalidated an earlier pass anyway.
	 */
	GRAPHCODESHAPE_API void Shade(const FGcShapeHeader& Header, UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * **WHETHER TWO FACES SHARE A NORMAL — THE ONE PREDICATE `Shade` AND CONFORM'S WALK BOTH CALL**
	 * (`GC-SHAPE-FEATURE-LINES-DESIGN.md` §5.4), so the two cannot drift. True while the faces' normals
	 * open no wider than `shade:` (`dot >= Threshold`, the cosine of `shade:`) and the edge they share
	 * carries no hard mark a `@sharp` key made. A mesh with no mark answers by the angle alone, exactly
	 * as before. `NormalA`/`NormalB` are the two faces' own normals, passed so a caller holding them
	 * computes nothing twice.
	 */
	GRAPHCODESHAPE_API bool SharesNormal(const UE::Geometry::FDynamicMesh3& Mesh, int32 TriA, int32 TriB,
		const FVector3d& NormalA, const FVector3d& NormalB, double Threshold);

	/**
	 * **TANGENTS FROM THE FINISHED UVS**, after group 6, so a normal map lands right — the other
	 * chore. MEASURED: the asset door's `bEnableRecomputeNormals` and `bEnableRecomputeTangents`
	 * both default to false, which is correct, because we supply both and the asset must not
	 * overwrite them.
	 */
	GRAPHCODESHAPE_API bool Tangents(UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * **THE ASSET'S SLOTS: the distinct ids this mesh actually paints, compacted at the door.**
	 *
	 * §4 group 6: *an asset's slots are the materials its entry names, DIRECTLY OR THROUGH ANY ENTRY
	 * IT ADDS, in the order those `material()` entries are declared in the file.* Both halves of that
	 * are the ids themselves — what the mesh carries is what reached it, and the ordinals are already
	 * in declaration order — so this walks the triangles rather than the document.
	 *
	 * **THE MESH IS REWRITTEN**, because a slot index is an index into the asset's array and the file
	 * ordinal is not: a file declaring `Water`, `Foam` and `Rock` gives the sheet ids 1 and 2 and the
	 * lip id 3, and the lip's one slot is index 0. `Arrived` names the ids at or above
	 * `ArrivedSlotBase` (MB35).
	 *
	 * **NO TWO SLOTS SHARE A NAME** (G7, 2026-11-13): a slot arriving under a name already taken merges into
	 * it when it wears the same material, and otherwise takes `Name_2`; each rename is said in `OutRenamed`.
	 */
	GRAPHCODESHAPE_API void Slots(const FGcShapeDocument& Document, const TArray<FGcShapeSlot>& Arrived,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FGcShapeSlot>& OutSlots, TArray<FString>* OutRenamed = nullptr);

	/**
	 * How many slots this mesh would take, and how many UV elements channel 0 holds — the two
	 * numbers the measure line carries.
	 *
	 * **A `material()` OR A `uv()` IS OTHERWISE UNANSWERABLE FROM OUTSIDE THE EDITOR**, exactly as
	 * §5's face counts were before M6.2: every other number the measure line reports is identical
	 * whether the line ran or did nothing at all.
	 */
	GRAPHCODESHAPE_API int32 SlotCount(const UE::Geometry::FDynamicMesh3& Mesh);
	GRAPHCODESHAPE_API int32 UvCount(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * How many normal elements channel 0 holds — **the one number `shade:` moves.**
	 *
	 * A split normal is a second element on one vertex, so a fully faceted shape carries three per
	 * triangle and a fully smooth one carries one per vertex. Nothing else on the measure line
	 * changes at all when the angle does.
	 */
	GRAPHCODESHAPE_API int32 NormalCount(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * **WHAT THE UNWRAP DOES TO A TEXTURE — U4** (`GC-SHAPE-MESH-QUALITY-PROPOSAL.md` §7.4).
	 *
	 * Per UV0 triangle, the map from its 3D shape to its UVs, normalised by the scale that triangle was
	 * DECLARED at (`GraphCodeShape.UvScale`, which `uv()` writes) or, where none was, by its own island's
	 * average density. Its two singular values σ1 ≥ σ2 give three numbers, 1 being perfect in each:
	 * **stretch** max(σ1, 1/σ2), the most any direction is stretched or squashed; **area** max(σ1σ2,
	 * 1/σ1σ2); **angle** σ1/σ2. Each is reported as the value exceeded on only 1% of the surface by area,
	 * because a plain maximum is set by the worst sliver, which nobody can see.
	 */
	struct FUvMeasure
	{
		/** UV0 triangles with a 3D area. */
		int32 Measured = 0;
		/** Of those, how many carry a declared scale. */
		int32 Declared = 0;
		int32 Islands = 0;
		/** Mirrored against their island's majority, or lying on another triangle of it they share no corner with. */
		int32 Folded = 0;
		/**
		 * **G18b: THE TWO HALVES OF `Folded`, WHICH HAVE DIFFERENT CAUSES** (`Docs/GC-SHAPE-DAMAGE-KIT.md` §6, lead-ruled
		 * 2026-11-19). `Mirrored`: wound against their island's majority. `Overlapping`: lying on another triangle of
		 * their island they share no UV corner with. A triangle can be both, so `Folded` is the union, not the sum.
		 */
		int32 Mirrored = 0;
		int32 Overlapping = 0;
		/**
		 * G18b: islands holding a triangle a `uv(box)` detached because its island lay on itself (`UvSplitAttributeName`).
		 * A split island is still overlapped in UV space until a pack moves it, so `Folded` 0 with `Split` above 0 is not a
		 * valid unwrap; a bake lays out its own (`uv_channel=auto`).
		 */
		int32 Split = 0;
		double Stretch = 0.0;
		double StretchMean = 0.0;
		double Area = 0.0;
		double Angle = 0.0;
		/** Where a scale was declared: the 5th and 95th percentile of the texel density, as a fraction of it. */
		double DensityLow = 0.0;
		double DensityHigh = 0.0;
		UE::Math::TVector<double> WorstAt = UE::Math::TVector<double>::ZeroVector;
		/** The folded triangles' centres, capped, for the address clustering. */
		TArray<UE::Math::TVector<double>> FoldedAt;
	};

	/** False where the mesh has no UV0 triangle to measure (`uv(none)`, no attributes). */
	GRAPHCODESHAPE_API bool MeasureUv(const UE::Geometry::FDynamicMesh3& Mesh, FUvMeasure& Out);

	/**
	 * **UV0 FACES THAT SHIP AS A DOT OR A LINE — G13** (wood damage, 2026-11-18). `MeasureUv` skips the
	 * default `keep`'s placeholders on purpose (a placeholder is no authoring decision), and so it could not
	 * see the one placeholder fault that is not a matter of taste: a face with no UV area at all. A critic
	 * measured 775 such faces on one debris shard, every island collapsed to (0, 0), and the build line said
	 * nothing. This counts them on EVERY UV0 triangle with a real 3D area, placeholder or not: corners that
	 * coincide or lie in a line (a UV triangle under a thousandth as fat as its 3D triangle), and triangles a
	 * `keep` placeholder left unset, which ship at (0, 0). A triangle `uv(none)` cleared is not counted.
	 */
	struct FUvDegenerate
	{
		/** UV0 triangles with a 3D area that were asked: set, or unset under a `keep` placeholder mark. */
		int32 Considered = 0;
		/** Of those, the ones with no UV area. */
		int32 Triangles = 0;
		/** Of `Triangles`, the ones never set at all. */
		int32 Unset = 0;
		/** UV islands among the set triangles, and how many of them have no UV area anywhere. */
		int32 Islands = 0;
		int32 CollapsedIslands = 0;
		/** The degenerate triangles' centres, capped, for the address clustering. */
		TArray<UE::Math::TVector<double>> At;
	};

	/** False where there is nothing to ask: no UV0 overlay, or no triangle `FUvDegenerate::Considered` counts. */
	GRAPHCODESHAPE_API bool MeasureDegenerateUvs(const UE::Geometry::FDynamicMesh3& Mesh, FUvDegenerate& Out);

	/**
	 * **THE SCALE EACH TRIANGLE'S UVS WERE DECLARED AT** — a two-float triangle attribute, centimetres per
	 * tile along U and along V, 0 meaning undeclared. `RunUv` writes it for a projection and clears it for
	 * `auto`/`none`; the default `keep` rescale writes its 100. Carried across an append by
	 * `GraphCodeShapeTopology::PrepareAppend`, as the topology policy is.
	 */
	GRAPHCODESHAPE_API FName UvScaleAttributeName();

	/**
	 * **DOES THIS MESH CARRY AN AUTHORED UV LAYOUT** — a UV0 triangle that `uv()` declared, or that is
	 * undeclared but not a `keep` placeholder (`uv(auto)`, a boolean's new faces). A mesh with UV0 and no
	 * `GraphCodeShape.UvScale` at all (an imported operand) is read as authored, conservatively. G8 slice 1:
	 * the budget keeps a UV projection affine (midpoint collapse) only where this is true.
	 */
	GRAPHCODESHAPE_API bool HasAuthoredUvs(const UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * Give `Into` an empty `GraphCodeShape.UvScale` when `From` has one, so a boolean's append copies it across — and,
	 * since G18b, an empty `GraphCodeShape.UvSplit` likewise.
	 */
	GRAPHCODESHAPE_API void CarryUvScale(UE::Geometry::FDynamicMesh3& Into, const UE::Geometry::FDynamicMesh3& From);

	/**
	 * **G18b: WHICH TRIANGLES A `uv(box)` MAPPED, AND WHICH THE SPLIT DETACHED** — a one-int triangle attribute: 1 where
	 * the last `uv()` to map the face was a `box`, 2 on a group `SplitBoxOverlaps` detached, 0 elsewhere. Every `uv()`
	 * line resets it on what it maps. The measure counts the islands holding a 2, `uvsplit` on the build line
	 * (`Docs/GC-SHAPE-DAMAGE-KIT.md` §6, lead-ruled). Gone again from a finished mesh nothing was split on.
	 */
	GRAPHCODESHAPE_API FName UvSplitAttributeName();

	/**
	 * **G18b (2): SPLIT EVERY `uv(box)` ISLAND THAT LIES ON ITSELF, ON THE FINISHED MESH** — after the budget, the pivot and
	 * every clean-up, before the shading, the tangents, a bake and the measure, so nothing downstream ever re-triangulates
	 * around a new seam (measured: run at the `uv()` line, the budget's seam-aware collapses moved faces on every split
	 * mesh). Only the elements change: every position, triangle and UV value is what it was. `gc.Shape.BoxSplitOverlap 0`
	 * leaves the mesh as it came.
	 */
	GRAPHCODESHAPE_API void SplitBoxOverlaps(UE::Geometry::FDynamicMesh3& Mesh);

	/**
	 * **G21: SPLIT EVERY OVERLAY'S BOWTIES IN VERTEX ORDER, SO ONE TEXT NUMBERS ITS ELEMENTS ONE WAY EVERY BUILD.** The
	 * engine's `FDynamicMeshAttributeSet::SplitAllBowties()` calls each overlay's `SplitBowties()` with its parallel
	 * default even when asked serially: bowtie vertices are gathered in a `ParallelFor` under a lock and split in arrival
	 * order, so new element ids follow thread timing (measured: BrokenTimber built twice, the same values, other element
	 * ids on all seven meshes). This splits every UV layer, normal layer and the colour overlay serially, in vertex order.
	 */
	GRAPHCODESHAPE_API void SplitAllBowtiesInOrder(UE::Geometry::FDynamicMesh3& Mesh);

	/** What the `uv(box)` repairs did since the counters were reset (G18, G18b) — for specs and the step-0 diagnostic. */
	struct FBoxRepairCounts
	{
		/** Faces G18 projected again because a merge left them steeper than 54.7 degrees. */
		int32 Steep = 0;
		/** Groups G18b (2) detached, and the islands of the lines it ran on before and after. */
		int32 Detached = 0;
		int32 IslandsBefore = 0;
		int32 IslandsAfter = 0;
		/** Islands whose overlap test stopped at its 4,000,000-test cap, so some overlap may be left. */
		int32 Capped = 0;
	};
	GRAPHCODESHAPE_API FBoxRepairCounts BoxRepairCountsForTests(bool bReset);

	/**
	 * **THE ID A `material($Name)` PAINTS: that entry's file-wide ordinal, plus one.** `INDEX_NONE`
	 * where the name is not a `material()` entry, which the caller names in its own sentence.
	 *
	 * **PUBLIC BECAUSE THE BRACKET READS IT TOO — board row MB10.** `[material=$Stone]` asks the
	 * same question `material($Stone)` asks, and a second walk of the document over there would be
	 * two carriers of one fact ([[two-carriers-of-one-fact-is-a-race]]): the day a slot's ordinal
	 * stops being its declaration index, the writer and the reader would disagree silently and the
	 * selection would land on the wrong faces.
	 *
	 * **ZERO IS NEVER RETURNED**, because zero is unpainted and no entry declares it.
	 */
	GRAPHCODESHAPE_API int32 MaterialOrdinal(const FGcShapeDocument& Document, const FString& Name);

	/**
	 * The other direction — the entry NAME an id belongs to, for a refusal that has to say which
	 * slots a shape is actually wearing. Empty where the id is unpainted or declared by no entry.
	 */
	GRAPHCODESHAPE_API FString MaterialNameAt(const FGcShapeDocument& Document, int32 Ordinal);

	/** `'/Game/M_X.M_X'` as an entry writes it, without its quotes. Empty stays empty. */
	GRAPHCODESHAPE_API FString UnquotePath(const FString& Literal);
}
