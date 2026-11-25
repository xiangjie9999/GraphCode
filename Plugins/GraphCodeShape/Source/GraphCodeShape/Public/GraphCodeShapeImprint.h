// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"

struct FGcShapeDocument;
struct FGcShapeHeader;
struct FGcShapeInstruction;
class IGcShapeVolumes;
namespace GraphCodeShapeSurface { struct FGcShapeSlot; }

/**
 * **`imprint(below=<cm>)`, `imprint(none)` — O1b: WHICH GEOMETRY IS SHALLOW DETAIL** (`GC-SHAPE-MESH-QUALITY-PROPOSAL.md`
 * §8, "O1b design"; the spelling is PROPOSED for the owner's ruling).
 *
 * A group-6 line that marks the faces its bracket chooses with a depth: on a light build (a build line with `[maps=]`),
 * a relief line whose depth is at or below the mark on its faces is left out, and the maps baked from the full build
 * carry it instead. `imprint(none)` marks faces that are never left out. Later lines win on the faces they choose. The
 * mark is a triangle attribute, so it rides `add` (a preset's own `imprint()` lines come with the part) and every merge
 * that carries material ids.
 *
 * **THE DECISION IS BY LINE, AND THE RECIPE SAYS WHAT A FEATURE IS.** A relief line is a feature: `raise`, `displace`,
 * `roughen` and `round` (on a mesh) STATE their depth (`by=`, the radius); `add` and `subtract` (on a mesh) have it
 * MEASURED, as how far the surface the line leaves stands from the surface just before it (its base). The full pass of a
 * build with a `[maps=]` line stamps each line's faces; the light pass is the program run again on a fresh context,
 * skipping the lines found shallow, so everything else is built exactly as before, at full tessellation.
 */
namespace GraphCodeShapeImprint
{
	using UE::Geometry::FDynamicMesh3;

	// ---------------------------------------------------------------------------------------------------------------
	// The verb and its mark
	// ---------------------------------------------------------------------------------------------------------------

	/**
	 * Group 6: write the mark on the faces the bracket chooses (a weighted bracket scales `below=` by the weight).
	 * `imprint(below=…, narrow=flatten)` (O1c, PROPOSED) also lets a feature narrower than the maps can draw be flattened.
	 */
	GRAPHCODESHAPE_API bool Run(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes, const FGcShapeInstruction& Line,
		const FGcShapeHeader& Header, FDynamicMesh3& Mesh, FString& OutError);

	/** Does any face carry a mark (a depth or `none`)? */
	GRAPHCODESHAPE_API bool HasMarks(const FDynamicMesh3& Mesh);

	/** A face's mark: the depth in cm, -1 for `none`, 0 where no line marked it. */
	GRAPHCODESHAPE_API double MarkOf(const FDynamicMesh3& Mesh, int32 Triangle);

	/** Before a merge: where either side carries an imprint attribute, both do, so the merge keeps it (`MatchColour`'s reason). */
	GRAPHCODESHAPE_API void Match(FDynamicMesh3& A, FDynamicMesh3& B);

	/** Every attribute this file attaches is named under this prefix (the append helper carries them). */
	GRAPHCODESHAPE_API bool IsOurs(const FName& AttributeName);

	// ---------------------------------------------------------------------------------------------------------------
	// Features: what the full pass records and the light pass skips
	// ---------------------------------------------------------------------------------------------------------------

	/** `raise`, `displace`, `roughen`, `round`, `add`, `subtract`: the verbs whose line can be a feature (on a mesh). */
	GRAPHCODESHAPE_API bool IsReliefVerb(const FString& Verb);

	/**
	 * One instruction of one file: `<document path>#<line>#<verb>#<digest of its arguments and bracket>`. The line alone is
	 * not enough: `add($Emblem, …)  add($Boss, …)` on one line are two features.
	 */
	GRAPHCODESHAPE_API FString KeyOf(const FString& DocumentPath, const FGcShapeInstruction& Line);

	struct FFeature
	{
		FString Key;
		FString Verb;
		/** `$Emblem` for an `add`/`subtract`, empty otherwise. */
		FString Operand;
		FString Entry;
		FString Document;
		int32 Line = 0;
		/** True for `add` (a placed part), whose faces are the part's own; false for a line that reshapes faces. */
		bool bPart = false;
		/** The depth the line states (cm), -1 where it states none the reader can use. */
		double Stated = -1.0;
		/** The depth measured off the base surface (cm), the most over every time the line ran. */
		double Measured = 0.0;
		/** No base within reach (nothing under the part, or nothing measured): never shallow. */
		bool bUnbounded = false;
		int32 Runs = 0;

		double Depth() const { return Stated >= 0.0 ? Stated : (bUnbounded ? TNumericLimits<double>::Max() : Measured); }
	};

	/** Hung on the evaluator's context: the full pass records (`bRecord`), the light pass skips (`Flatten`). */
	struct FPass
	{
		bool bRecord = false;
		TSet<FString> Flatten;
		TArray<FFeature> Features;
		TMap<FString, int32> IndexOf;
		/** How many times the light pass skipped a line (a line in a part added N times counts N). */
		int32 Skipped = 0;
	};

	/**
	 * The surface before a relief line: each triangle by where its corners are, the depth the line states, and (where it
	 * states none) the geometry its depth is measured against.
	 */
	struct FBefore
	{
		TSet<uint64> Triangles;
		double Stated = -1.0;
		FDynamicMesh3 Mesh;
		bool bMesh = false;
		int32 MaxOwner = 0;
	};

	GRAPHCODESHAPE_API void Snapshot(const FGcShapeDocument& Document, const FGcShapeInstruction& Line,
		const FDynamicMesh3& Mesh, FBefore& Out);

	/** After the line ran: stamp the faces it made or moved, and note its depth. */
	GRAPHCODESHAPE_API void Record(FPass& Pass, const FGcShapeDocument& Document, const FString& Entry,
		const FGcShapeInstruction& Line, const FBefore& Before, FDynamicMesh3& After);

	// ---------------------------------------------------------------------------------------------------------------
	// The light build
	// ---------------------------------------------------------------------------------------------------------------

	struct FVerdict
	{
		int32 Feature = INDEX_NONE;
		bool bFlatten = false;
		bool bNone = false;
		/** The smallest `below=` on the feature's faces (cm). */
		double Below = 0.0;
		int32 Faces = 0;
		/** O1c: shallow enough, but narrower than the maps can draw, so kept as geometry. */
		bool bNarrow = false;
		/** O1c: the feature's narrowest width (cm, -1 unmeasured) and how many texels that is at the planned density. */
		double Width = -1.0;
		double Texels = 0.0;
	};

	struct FDecision
	{
		TSet<FString> Flatten;
		TArray<FVerdict> Verdicts;
		int32 Flattened = 0;
		int32 Kept = 0;
		/** The largest and smallest `below=` any face of the full build carries (cm); 0 where none does. */
		double Largest = 0.0;
		double Smallest = 0.0;
		int32 Skipped = 0;
		int32 FullTriangles = 0;
		FString BudgetNote;
		/** O1c: of `Kept`, how many only because they are narrower than the maps can draw. */
		int32 Narrow = 0;
		/** O1c: the texel density (px/m) the narrow rule read, planned before the bake, and the one the bake achieved. */
		double PlannedDensity = 0.0;
		double AchievedDensity = 0.0;
		/** O1c: how many light builds it took to settle which features are narrow. */
		int32 Rounds = 0;
	};

	/** Which marked features the light build leaves out, read off the full build's faces. */
	GRAPHCODESHAPE_API void Decide(const FPass& Recorded, const FDynamicMesh3& Full, FDecision& Out);

	/**
	 * **O1c: TOO NARROW FOR THE MAPS.** A shallow feature the maps cannot draw — narrower than `NarrowTexels()` texels at
	 * the light build's texel density — stays as geometry, reported "narrower than maps", unless every mark on its faces
	 * says `narrow=flatten`.
	 *
	 * **THE WIDTH IS THE FEATURE'S STROKE, SEEN ALONG ITS DEPTH.** Each connected piece of the feature's own faces is
	 * looked at along the way it stands off the surface (the area-weighted facing of an open patch; for a closed part,
	 * the axis along which it is thinnest); its faces turned that way are projected flat, and its width is twice the
	 * farthest any of them lies from that patch's border: the radial width of a ring, the short side of a tick, the
	 * diameter of a stud. A feature's width is its narrowest piece's. A shape with one thick part and thin strokes reads
	 * as thick: the rule keeps what would vanish whole, not every thin edge.
	 */
	struct FWidth
	{
		/** cm; negative where the feature has no face that can be measured (never narrow). */
		double Narrowest = -1.0;
		/** Every mark on its faces says `narrow=flatten`. */
		bool bFlattenAnyway = false;
	};

	/** Texels across below which a shallow feature stays as geometry (2.5). */
	GRAPHCODESHAPE_API double NarrowTexels();

	/** The width of every feature `Decision` flattens, off the full build's faces; indexed by feature. */
	GRAPHCODESHAPE_API void MeasureWidths(const FPass& Recorded, const FDynamicMesh3& Full, const FDecision& Decision,
		TArray<FWidth>& Out);

	/** Turn every flattened feature narrower than `NarrowTexels()` at `Density` (px/m) into a kept one; how many turned. */
	GRAPHCODESHAPE_API int32 KeepNarrow(const FPass& Recorded, const TArray<FWidth>& Widths, double Density, FDecision& InOut);

	/**
	 * `[triangles=]` on a light build: the quadric budget, with every collapse held within `Tolerance` cm of the light
	 * surface it starts from. A budget that cannot be met inside it is refused with the count it reached.
	 */
	GRAPHCODESHAPE_API bool Budget(FDynamicMesh3& Light, int32 Triangles, double Tolerance, const FString& AssetName,
		FString& OutNote, FString& OutError);

	/** A part the light pass skipped may have brought a material: point the light mesh's arrived slots at the full list. */
	GRAPHCODESHAPE_API void RemapArrived(FDynamicMesh3& Light, const TArray<GraphCodeShapeSurface::FGcShapeSlot>& LightArrived,
		TArray<GraphCodeShapeSurface::FGcShapeSlot>& FullArrived);

	/**
	 * The build line's field: `imprint: triangles L light / F full, outline X max / Y mean cm (limit Z), features A flattened
	 * / B kept`, and `(C narrower than maps)` where the O1c rule kept any.
	 */
	GRAPHCODESHAPE_API FString LineField(const FDecision& Decision, int32 LightTriangles, double OutlineMax, double OutlineMean);

	/** The note naming every marked feature, its depth, the mark it was read against and what became of it. */
	GRAPHCODESHAPE_API FString Note(const FString& EntryName, const FPass& Recorded, const FDecision& Decision);
}
