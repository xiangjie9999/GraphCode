// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeShapeMeshBake.h"
#include "GraphCodeShapeTopology.h"

struct FGcShapeBuildLine;
struct FGcShapeDocument;

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **WHAT A BUILD LINE'S `[ ]` SAYS ABOUT THE ASSET, NOT ABOUT THE SHAPE** — `GC-SHAPE-GRAMMAR.md`
 * §1, `build { }`; M7.
 *
 * **MODIFIERS HERE DESCRIBE THE GEOMETRY THIS FILE GENERATES** — where
 * the origin sits, how big the result may be, how many triangles to spend. Collision geometry also
 * belongs here; its settings remain in the asset's .gc. Nanite, LODs
 * and lightmap resolution are *how the asset builds* and belong to `kind:static_mesh`'s `.gc`
 * (`GC-PRODUCT-SPLIT.md` §5.2), which is the same seam that made a `material()` path a first-build
 * default rather than a fact this file owns.
 *
 * **UNTIL M7 THE PARSER COLLECTED THIS BRACKET AND NOTHING READ IT.** `[pivot=corner]` on a modular
 * kit piece placed every asset at `pivot=base` and reported success — the silent no-op R20 is about,
 * and the one the corpus already writes: `GC-SHAPE-EXAMPLE.md` §1 and `GC-SHAPE-SCENARIOS.md` §9
 * both carry pivots today.
 *
 * **`[bounds=]` IS THE ONE THAT CANNOT BE SEEN ANY OTHER WAY.** A modular piece is pushed off its
 * grid by its own `roughen()` — invisible in the asset, obvious only once fifty are placed — so it
 * is a CONTRACT the build checks rather than a number the author re-measures
 * (`GC-SHAPE-SCENARIOS.md` §9, gap G6).
 */
struct GRAPHCODESHAPE_API FGcShapeOutput
{
	enum class EPivot : uint8
	{
		/** Minimum Z, centred in X and Y — **the default**, and what a thing standing on ground wants. */
		Base,
		/** The bounding-box centre. */
		Center,
		/** Minimum X, Y and Z — for grid-snapped modular pieces. */
		Corner,
		/** An explicit point in the entry's own space. */
		Point,
		/**
		 * `[pivot=Web]`: one of the built entry's own sockets, by name. The build turns it into a
		 * `Point` once the sockets have resolved, so a prop's origin is the place it names rather than
		 * that place retyped (GC-MOVE-GRIP-SEATING.md, ruled 2026-10-02: web meets web).
		 */
		Socket,
	};

	EPivot Pivot = EPivot::Base;

	/** The `(x, y, z)` of `EPivot::Point`, unread otherwise. */
	UE::Math::TVector<double> Point = UE::Math::TVector<double>::ZeroVector;

	/** The socket name of `EPivot::Socket`, unread otherwise. */
	FString PivotSocket;

	/** `[triangles=600]`, or zero for no budget. A budget is a CEILING, never a target to grow to. */
	int32 Triangles = 0;

	/** Convex entries used only as this asset's simple collision, in the render entry's coordinates. */
	TArray<FString> CollisionEntries;

	/** `[bounds=(400, 400, *)]` in centimetres, with a NEGATIVE axis meaning `*` — that axis is free. */
	UE::Math::TVector<double> Bounds = UE::Math::TVector<double>(-1.0, -1.0, -1.0);
	bool bHasBounds = false;

	/**
	 * `[bake=Shell]` — the bake TARGET this output contributes to, empty when it does not bake.
	 * MB51, `GC-MATERIAL-BAKE-DESIGN.md` §0.
	 *
	 * **THE LINE NAMES A TARGET AND CARRIES NO SETTINGS.** Resolution and exclusions live on the
	 * `bake(Shell, …)` statement below, because a target is SHARED: every line naming it collapses
	 * into one atlas and one generated material. Settings on the line could only ever give each
	 * mesh an atlas of its own, which is not the saving anybody wanted.
	 *
	 * **IT IS A BUILD-OUTPUT MODIFIER AND NOT AN INSTRUCTION INSIDE `mesh()`**, which is the one
	 * place this bracket's own rule bends: merely USING an entry as an operand must never mint
	 * texture assets as a side effect.
	 */
	FString BakeTarget;

	/** `[reference=$Ref]` (A6): the `reference()` entry, without its `$`, the built shape is measured against. */
	FString ReferenceEntry;

	/** `[maps=(normal, ao), from=$Detail, resolution=2048]` (O1): the maps this line bakes for its own mesh. */
	FGcShapeMeshMaps Maps;

	/** What an omitted `resolution=` on a `bake()` statement means. */
	static constexpr int32 DefaultBakeResolution = 1024;

	/** Optional override on the final output copy; incoming operands retain their own defaults. */
	EGcShapeTopology Topology = EGcShapeTopology::Inherit;

	/**
	 * **THE INTEGRITY CEILINGS — `[folds=]`, `[crossings=]`, `[nonmanifold=]`** (G5,
	 * `GC-SHAPE-MESH-QUALITY-PROPOSAL.md` §7.3). Each is the measure line's own field name and a CEILING,
	 * the shape `[triangles=]` already has. **Unstated is 0**: a triangle that passes through its own
	 * shell, a shell entered twice without leaving it, or an edge three faces share has no reading under
	 * which it is sound, so a build that has one refuses unless its line allows that many. Unset means
	 * the author said nothing, which is what lets a declared allowance that is no longer needed be named.
	 */
	TOptional<int32> MaxFolds;
	TOptional<int32> MaxCrossings;
	TOptional<int32> MaxNonManifold;

	/**
	 * **THE UV CEILINGS — `[stretch=1.3]`, `[uvfolds=0]`** (U4, §7.4). **OPT-IN, unlike the three above**:
	 * a stretched or folded unwrap is reported on the line and never refused unless the line sets one,
	 * because a clay prop with placeholder UVs is not wrong for having them.
	 */
	TOptional<double> MaxStretch;
	TOptional<int32> MaxUvFolds;

	/**
	 * `[uvflat=0]` (P-G13, agent-spelled 2026-11-18, Sonnet-reviewed, lead-approved): the most UV0 triangles with
	 * no UV area (a dot or a line in UV space) this asset may carry. Opt-in like the two above; no default.
	 */
	TOptional<int32> MaxUvFlat;

	/**
	 * Read one build line's bracket. **An unknown modifier is refused rather than ignored**, which
	 * is the rule §5's face bracket already holds and the reason this function exists at all.
	 *
	 * **AND THE DOCUMENT IS READ BECAUSE A BUILD LINE'S NUMBERS ARE NUMBERS.** MC4 §2.6: `[pivot=]`,
	 * `[bounds=]` and `[triangles=]` parsed their own with `Atod`/`Atoi`, so a build line was the one
	 * place left in the format where a measurement had to be retyped rather than named — which is
	 * exactly the trap a size CONTRACT exists to catch. `[bounds=($Bay, $Bay, $Storey)]` now says
	 * what a kit piece really has to fit into, and cannot drift from the shape that has to fit.
	 */
	static bool Read(const FGcShapeDocument& Document, const FGcShapeBuildLine& Line,
		FGcShapeOutput& Out, FString& OutError);

	/** Resolve the shared pivot vocabulary without moving or simplifying the mesh. */
	FVector3d PivotPoint(const UE::Geometry::FDynamicMesh3& Mesh) const;

	/**
	 * Spend the budget, move the origin, then hold the contract — **in that order, and it is the
	 * only order that is true.** A simplification changes the extents, so a `[bounds=]` checked
	 * before it would pass a mesh that overshoots; and a pivot moves the shape without resizing it,
	 * so it cannot change the answer either way. `AssetName` is only for the refusal's first words.
	 */
	bool Apply(UE::Geometry::FDynamicMesh3& Mesh, const FString& AssetName, FString& OutError,
		FVector3d* OutAppliedTranslation = nullptr, FString* OutBudgetNote = nullptr) const;
};
