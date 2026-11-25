// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGcShapeArgument;
struct FGcShapeDocument;
struct FGcShapeHeader;
struct FGcShapeInstruction;
struct FGcShapeOutline;
struct FGcShapeStroke;
struct FGcShapeWeightedSelection;

namespace UE::Geometry { class FDynamicMesh3; }

/** A call's face membership follows mesh edits; its stroke retains the profile coordinates. */
struct GRAPHCODESHAPE_API FGcShapeInstructionScope
{
	int32 RegionLayer = INDEX_NONE;
	TSharedPtr<FGcShapeStroke> Stroke;
	TSharedPtr<const FGcShapeWeightedSelection> Weightage;
	double Fade = 0.0;
	double Smooth = 0.0;
};

/**
 * **WHICH PART OF THE SHAPE AN INSTRUCTION ACTS ON** — `GC-SHAPE-GRAMMAR.md` §5, the `[ ]` bracket.
 *
 * **NO MODIFIER AT ALL MEANS THE WHOLE SHAPE**, and that is a state rather than a full triangle
 * list: `bWhole` says the line never narrowed anything, which is what lets `trim()` refuse the one
 * case where *the whole shape* would mean *delete everything*.
 *
 * **`[fade=]` IS NOT A SELECTION AND DOES NOT PARTICIPATE IN THE AND.** It rides along on the same
 * bracket and softens whatever the others chose, so it is carried here rather than folded in.
 */
struct GRAPHCODESHAPE_API FGcShapeSelection
{
    /** Explicit recipe, independent of the bracket's hard membership and legacy fade. */
    TSharedPtr<const FGcShapeWeightedSelection> Weightage;
    /** Present only for moving projected brushes. Fixed regions keep their existing semantics. */
    TSharedPtr<FGcShapeStroke> Stroke;

	/**
	 * A reusable longitudinal coordinate carried by a measured or previously saved region.
	 * Kept separate from `Stroke` while membership is read: a field supplies profile coordinates,
	 * whereas a projected stroke also supplies a footprint that narrows the selected faces.
	 */
	TSharedPtr<FGcShapeStroke> Field;
	/** The line carried no selecting modifier. Neither list means anything. */
	bool bWhole = true;

	/**
	 * The bracket named EDGES rather than faces — `[rim]`, `[sharp]`.
	 *
	 * **THE TWO KINDS DO NOT MIX IN ONE BRACKET AND THE REFUSAL SAYS SO.** `[top]` and `[rim]` are
	 * not two narrowings of one set: one is a set of triangles and the other a set of edges, and
	 * *the rim that is also the top cap* has no meaning. What DOES apply to both is a height —
	 * `[rim, above=120]` is the part of the mouth above the waist — so `above=`/`below=` narrow
	 * whichever kind the bracket is already carrying.
	 */
	bool bEdges = false;

	/**
	 * The chosen triangles when `bWhole` is false and `bEdges` is false. Never empty — see `Read` — with ONE
	 * exception: `EmptiedByFacing` below.
	 */
	TArray<int32> Triangles;

	/**
	 * **N1 (`Docs/GC-SHAPE-DAMAGE-KIT.md` §6, lead-ruled 2026-11-19): A FACING SPLIT'S EMPTY PART.** Set, with
	 * `bWhole` false and `Triangles` empty, only for a partition verb (`uv`) whose bracket's other terms chose
	 * faces and whose `facing=`/`!facing=` terms left none of them. It is the note the consumer reports, and the
	 * consumer does nothing else: `Read` has no notes of its own, so this field is its channel. Empty on every
	 * other selection, so a consumer that never reads it is unchanged.
	 */
	FString EmptiedByFacing;

	/** conform's second face selection: follows the landing rather than landing itself. */
	FString Carry;

	/** The chosen edges when `bEdges` is true. Never empty — an empty edge set is refused. */
	TArray<int32> Edges;

	/**
	 * `[fade=40]` in centimetres, or zero. Only `taper` and `roughen` can honour it. **Under `value=` (G30a) it is in the
	 * formula's units and `ValueRamp` carries what it means**; every verb that refuses a fade still sees it here and
	 * refuses, and every place that reads it as a LENGTH reads `FadeLength()` instead.
	 */
	double Fade = 0.0;

	/**
	 * **G30a: `[value=<formula>, above=, fade=]`'s WEIGHT PER VERTEX** (indexed by vertex id), set only when the bracket has
	 * `value=` and a `[fade=]`: 0 at the threshold rising to 1 at threshold + fade (below: threshold - fade), INSIDE the
	 * selection, never outward. `Weights` reads it instead of the border distance.
	 */
	TSharedPtr<const TArray<double>> ValueRamp;

	/** `Fade` as a distance in centimetres: zero when it is a `value=` fade, which is no length and buys no points. */
	double FadeLength() const { return ValueRamp.IsValid() ? 0.0 : Fade; }

	/**
	 * **THE BRACKET NARROWED BY A FACE TAG** — `[top]`, `[!bottom]`, `[side]`, `[cut]` and their
	 * `!` forms — rather than only by a height, a volume or a stroke. Board row MR2.
	 *
	 * **THE DISTINCTION IS WHERE THE BORDER FALLS, AND IT IS STRUCTURAL FOR ONE OF THEM ONLY.** A
	 * face tag is a polygroup, so the ring where the selection stops is a real arris — the rim
	 * a boulder's `[!bottom]` shares with its flat base. `above=`/`below=` cut mid-face and the
	 * centre test leaves an unmoved band there for free, which is why MR2's own four-bracket
	 * measurement found `[above=10]` clean and `[!bottom]` folded 36 times on one shape.
	 *
	 * So the two want different treatment at their border, and this is the only field that can
	 * tell them apart by the time `Weights` runs.
	 */
	bool bFaceTagged = false;

	/**
	 * `[smooth=3.0]` in centimetres, or zero — **the density OVERRIDE**, board row MB13,
	 * user-raised: *a user might want to allow more vertices before doing a transformation*.
	 *
	 * **THE DEFAULT STAYS DERIVED AND THAT RULING IS ROW MV'S, UNTOUCHED.** `roughen(scale=)` and
	 * `[fade=]` buy the vertices their own numbers need, because *a field of wavelength S sampled
	 * on edges longer than S/2 does not come back coarser, it comes back arbitrary* — nobody is
	 * choosing a look there, so §5.0's rule applies and the file never mentions it. **What MV never
	 * covered is that the refinement is ONE-WAY** — an edge already short enough is left exactly as
	 * it is — **and that an author who wants MORE than the derived rule buys had no way to say so.**
	 * This is that sentence, and nothing else: absent, every shape in the corpus is bit-identical.
	 *
	 * **IT IS A LENGTH BECAUSE EVERY DENSITY STATEMENT IN THIS FORMAT IS A LENGTH** — `smooth:` is
	 * a chord tolerance, `roughen(scale=)` and `remesh(<size>)` are feature sizes, `relax(<cm>)` is
	 * a cap. The build line's `[triangles=]` is the one count in the format and it stays what it is:
	 * a contract about the delivered ASSET, beside `[bounds=]`.
	 *
	 * **AND IT ONLY EVER GOES FINER.** A `[smooth=]` COARSER than the number the line already
	 * derives is refused rather than honoured, because honouring it would make that line's own
	 * `scale=` arbitrary — which is the one thing row MV's arithmetic exists to prevent.
	 */
	double Smooth = 0.0;

	/**
	 * `[$Joins = seam]` — the variable this line CAPTURES into, or empty. MC2 §3.
	 *
	 * **A CAPTURE IS NOT A SELECTION AND RIDES ALONG BESIDE ONE**, exactly as `[fade=]` does: the
	 * line still acts on whatever the rest of the bracket chose, and a bracket that only captures
	 * leaves `bWhole` true. `add($Lens) [$Joins = seam]` adds the lens to the whole shape and names
	 * what it made.
	 */
	FString CaptureInto;

	/**
	 * What `CaptureInto` is being set to: `seam`, `wall`, `affected`, or a face word.
	 *
	 * **THE EVALUATOR PERFORMS THE CAPTURE, NOT THIS LAYER, AND THE REASON IS TIMING.** A face
	 * predicate is true of the shape as it stands, so `CaptureTriangles` below is already the
	 * answer; **`seam` is what the line is ABOUT TO MAKE** and cannot be known until the boolean
	 * has run. One field says which, and the evaluator reads it at the moment that applies.
	 */
	FString CaptureSource;

	/** Input triangles for `affected`, or triangles matching a face predicate. Empty for `seam`. */
	TArray<int32> CaptureTriangles;

	/** True where the source is `seam` — known only after the line's own boolean. */
	bool bCaptureIsSeam = false;

	/**
	 * `[$Ledge = wall]` — the source is what this line's own `raise` BUILT. Board row MR7.
	 *
	 * **THE SAME DEFERRAL `seam` NEEDS, FOR THE SAME REASON.** A raise's side walls do not exist
	 * until `FOffsetMeshRegion` has stitched them, so the evaluator performs this capture once the
	 * instruction has run and hands the triangles in. A `wall` on a line with no `raise` is refused
	 * where `seam` on a line with no boolean is.
	 */
	bool bCaptureIsWall = false;

	/**
	 * `$Recipe(...) [$v1 = Seam]` — the source names something the RECIPE holds, so only the call
	 * can resolve it. Board row MB34, user-ruled 2026-09-12: *it's just like the `[$v1=seam]` isn't
	 * it*, and *so they can pass on any variable inside the instruction*.
	 *
	 * **THE THIRD DEFERRAL, AND IT IS `seam`'S AND `wall`'S FOR A THIRD REASON.** A face word is
	 * true of the shape as it stands, `seam` is what the line is about to make, `wall` is what its
	 * own `raise` will build — and this one is a name in a scope that does not exist yet. Set only
	 * on the `instruction` verb, where an unrecognised word is a recipe's name rather than a
	 * mistake; on every other verb the vocabulary is closed and the refusal stands.
	 */
	bool bCaptureIsExport = false;

	/** How many triangles the line acts on. Zero on an edge selection, which counts edges instead. */
	int32 Count(const UE::Geometry::FDynamicMesh3& Mesh) const;
};

/**
 * **HOW `[in=$Shape]` REACHES ANOTHER ENTRY** — `GC-SHAPE-GRAMMAR.md` §5, row MI.
 *
 * The selection layer knows a mesh and a bracket; only the EVALUATOR knows what `$Splitter` is, has
 * built it once, and can refuse a cycle. So the reach is a parameter rather than a dependency, and
 * the evaluator hands one down.
 *
 * **IT IS PASSED TO EVERY SELECTING INSTRUCTION, NOT JUST THE ONE THE CORPUS NEEDED.** §5 lists
 * `[in=]` beside the four tag words as a modifier like any other, and `[top, in=$Tool]` is its own
 * example — a modifier honoured at one door only is a grammar that reads the same and behaves
 * differently line to line ([[a-switch-honoured-at-one-door-only]]).
 */
class IGcShapeVolumes
{
public:
	virtual ~IGcShapeVolumes() = default;

	/**
	 * The finished LOCAL mesh of `$Name`, or false with a reason an author can act on.
	 *
	 * **THE SHAPE IS USED WHERE IT SITS.** An entry already says where it starts — the waterfall's
	 * `Splitter` is a `circle(at=(60, -120, -180))` — so `[in=]` takes no placement of its own and
	 * a volume that needs moving is moved by editing the entry.
	 */
	virtual bool Volume(const FString& Name, const UE::Geometry::FDynamicMesh3*& OutMesh,
		FString& OutError) = 0;

	/**
	 * The triangles a named variable holds — `[$Joins]`. MC2 §3. False with a reason when the name
	 * is not one this body captured.
	 *
	 * **THIS IS THE SAME WINDOW `[in=$Tool]` ALREADY USES AND THAT IS WHY IT IS HERE.** The
	 * selection layer knows a mesh and a bracket; only the EVALUATOR knows what a body has captured,
	 * where it is stored and whether the name exists ([[ask-the-interface-not-the-struct]]). A
	 * fourth parameter on `Read` would have been the alternative, and `Read` has eight call sites.
	 */
	virtual bool Region(const FString& Name, TArray<int32>& OutTriangles, FString& OutError) = 0;

	/** The optional profile-coordinate field saved with a region. */
	virtual TSharedPtr<FGcShapeStroke> RegionField(const FString& /*Name*/) const { return nullptr; }
	virtual TSharedPtr<const FGcShapeWeightedSelection> RegionWeightage(const FString& /*Name*/) const { return nullptr; }

	/**
	 * A socket's resolved frame for `measure=$Socket`. The origin is station zero and the
	 * direction is the socket's own +Z. False with an empty error means this name is not a socket,
	 * so the shared coordinate reader may still try an ordinary directed path of the same spelling.
	 */
	virtual bool SocketFrame(const FString& /*Name*/,
		UE::Math::TVector<double>& /*OutOrigin*/, UE::Math::TVector<double>& /*OutDirection*/,
		FString& /*OutError*/) { return false; }

	/** Does this body already hold a variable of this name? A capture may create OR set one. */
	virtual bool HasRegion(const FString& Name) const = 0;

	/**
	 * Add faces to a region this body captured — `drip()` bends a region's edge, and a `[$glaze]`
	 * that stopped at the old edge would leave the runs out of the next line that reads it. False
	 * with a reason where the name holds no region.
	 */
	virtual bool GrowRegion(const FString& Name, const TArray<int32>& /*Triangles*/, FString& OutError)
	{
		OutError = FString::Printf(TEXT("'$%s' cannot be grown here: nothing in this body holds it"), *Name);
		return false;
	}

	/** The name of a slot a part brought from another file (MB35), or empty — only an evaluator knows. */
	virtual FString ArrivedSlot(int32 /*Id*/) const { return FString(); }

	/**
	 * `$Name` as the bracket's own `at=`/`rotate=`/`scale=`/`mirror=`/`align=` place it. MC3.
	 *
	 * **THE EVALUATOR PLACES IT BECAUSE A RELATION MEASURES THE ACCUMULATOR.** `at=on()` and
	 * `align=` are answers about where the shape being built already is, and this layer has only a
	 * mesh and a bracket. Handing the placement through the window rather than resolving it here is
	 * the same move `Volume` and `Region` already make ([[ask-the-interface-not-the-struct]]).
	 *
	 * **AND IT CLOSES A GAP THE CAR FOUND BY EYE.** `[in=$Tool]` read its volume in the tool's own
	 * local coordinates, so a recess was hand-placed and widening the body left it wrong — legal,
	 * building, and silently in the wrong place.
	 */
	virtual bool PlacedVolume(const FString& Name, const TArray<FGcShapeArgument>& Placement,
		const UE::Geometry::FDynamicMesh3*& OutMesh, FString& OutError) = 0;

	/**
	 * **WHAT A `displace(to=$Name)` AIMS AT — a `mesh()`'s SURFACE or a `plane()`'s PATH.** MC3.
	 * Exactly one of the two comes back non-null.
	 *
	 * **A `plane()` IS ACCEPTED HERE AND REFUSED BY `Volume`, AND THAT IS THE POINT.** `Volume` is
	 * group 5's window: its refusal says *a flat region has no volume to combine with a solid,
	 * either give it depth or combine the outlines above this entry's depth instruction*, which is
	 * true of a boolean and nonsense on a `to=` line — there is no boolean and nothing to move
	 * ([[a-shared-pass-lends-one-consumer-s-constraint-to-another]]). A ridge is the ONE case §4.1a
	 * says genuinely needs `to=`, so the pane that reads a target reads both kinds.
	 *
	 * **AND A PATH NEEDS NO THICKNESS.** The obvious build is to widen the outline into a hairline
	 * ribbon and hand back a mesh, which keeps one distance query — but a widen has to know which
	 * way is sideways, and a 3D path has no answer ([[a-symmetric-subject-hides-an-unstated-convention]]
	 * is the same trap one door along). Distance to a SEGMENT is what that ribbon would have
	 * measured anyway, exactly, in three dimensions, and it is a dozen lines with no convention in
	 * them.
	 */
	virtual bool Target(const FString& Name, const UE::Geometry::FDynamicMesh3*& OutMesh,
		const FGcShapeOutline*& OutPath, FString& OutError) = 0;

	/**
	 * **THE PLACES A SAVED CAST HOLDS — `displace(to=$Buttons)`.** False where the name is
	 * not a set this body cast, which is every name that existed before sets did.
	 *
	 * **A SEPARATE PANE RATHER THAN A THIRD OUT-PARAMETER ON `Target`, AND THE REASON IS
	 * THAT A SET IS NOT AN ENTRY.** `Target` resolves a `mesh()` or a `plane()` — a
	 * declaration another file could import. A cast is body-local, it is made by a line
	 * rather than declared, and only the evaluator has ever known what a body captured
	 * ([[ask-the-interface-not-the-struct]], which is `Region`'s own reason one pane up).
	 */
	virtual bool Places(const FString& /*Name*/,
		TArray<UE::Math::TVector<double>>& /*OutPlaces*/) const { return false; }
};

namespace GraphCodeShapeSelect
{
	/**
	 * Read one instruction's `[ ]` bracket against the shape as it currently stands.
	 *
	 * **TWO MODIFIERS IN ONE BRACKET AND TOGETHER** (§5). Union is deliberately unspellable, because
	 * the two cases anyone reaches for — *either cap* and *the whole thing* — are `[!side]` and no
	 * bracket at all. The AND is a set intersection here rather than
	 * `CombineMeshSelections`: the engine's door takes a `UDynamicMesh*` and everything on this side
	 * of the seam is a raw `FDynamicMesh3`, so wrapping one to intersect two integer sets would buy
	 * an allocation and a UObject, not a capability.
	 *
	 * **AN UNKNOWN MODIFIER IS REFUSED RATHER THAN IGNORED.** Before this, the parser collected the
	 * bracket and nothing read it, so `[!bottum]` selected the whole shape and reported success —
	 * the silent no-op §7 exists to prevent.
	 *
	 * **A BRACKET THAT SELECTS NOTHING IS REFUSED HERE, FOR EVERY CONSUMER AT ONCE.** §5 describes
	 * `[rim]` on a closed solid as selecting nothing and *the instruction does nothing*, and the
	 * measure line's face counts were shipped so that a zero beside the word would be the author's
	 * only warning. A warning nobody reads is not the answer R20 asks for — *a line that does
	 * nothing is worse than a missing line* — and the check belongs on this side of the seam because
	 * every consumer wants it: `trim()` shipped at M6.2 deleting zero triangles and reporting
	 * success, and `taper`/`roughen` would have moved zero vertices the same way.
	 *
	 * **AND THAT RULING RETIRED A SENTENCE §5 HAD SHIPPED ABOUT `[rim]`.** The table said a closed
	 * solid has no rim, *so `[rim]` on one selects nothing and the instruction does nothing* — which
	 * is precisely the outcome M6.2 stopped accepting. It refuses now, and the message names
	 * `cut(fill=false)` and `shell()`, the two instructions that give a shape a rim to begin with.
	 *
	 * **THE HEADER IS READ BECAUSE `[sharp]` NEEDS AN ANGLE AND THE FILE ALREADY DECLARES ONE.**
	 * `shade:` is *the angle above which an edge shades hard*, default 40 — so the edges an author
	 * would call sharp and the edges the asset will shade sharp are ONE set, named once. Inventing a
	 * second threshold here would let a kit round an edge it then shaded smooth
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * **AND THE DOCUMENT IS READ BECAUSE A BRACKET'S NUMBERS ARE NUMBERS.** MC4 §2.6: `[fade=]` and
	 * `[above=]`/`[below=]` parsed their own with `Atod` and so could not see a `$name`, let alone an
	 * expression — which made them the two places in the format where a measurement had to be
	 * retyped rather than named. They go through `GraphCodeShapeValue::ReadLength` now, which is the
	 * one reader, so `[above=$Sill]` and `[fade=$Wall / 4]` work for the same reason `height=` does.
	 * **A PERCENTAGE IS UNTOUCHED**: `[above=40%]` is a mark on the shape's own Z span (R22), read
	 * before this and still a literal written at the value site, exactly as §2.6 ruled.
	 *
	 * **THE MESH IS TAKEN BY REFERENCE BECAUSE `[in=]` BUYS THE DENSITY ITS OWN BOUNDARY NEEDS** —
	 * board row MR14, and it is the one thing in this file that writes. A volume's SURFACE is a
	 * feature exactly as `roughen(scale=)` and `[fade=]` are, and a feature no shorter than the
	 * mesh's own edges cannot be read at all: the answer comes back as the triangulation rather
	 * than as the volume. It was reported twice as two different bugs — a boulder and a Cliff where
	 * `[in=]` *selects nothing*, and a brick where a y-limited one *selects a rectangle nobody
	 * drew* — and both are that one missing purchase.
	 *
	 * **REFINE-ONLY, SO A READ THAT NEEDS NOTHING COSTS NOTHING AND A SECOND READ IS FREE.** A mesh
	 * already fine enough for the volume comes back untouched, which is what makes the double read
	 * in `GcShapeFormPreamble` — and the capture read the evaluator does after the line — safe.
	 */
	GRAPHCODESHAPE_API bool Read(const FGcShapeDocument& Document,
		const FGcShapeInstruction& Line, const TCHAR* Verb,
		const FGcShapeHeader& Header, IGcShapeVolumes& Volumes,
		UE::Geometry::FDynamicMesh3& Mesh, FGcShapeSelection& Out, FString& OutError,
		bool bAllowRefinement = true, double WeightAccuracy = 0.0);

	/**
	 * **HOW MANY TIMES `Read` HAS REFUSED A BRACKET FOR CHOOSING NOTHING, on this thread** (faces or edges: the
	 * generic "selects no faces/edges, so the line would do nothing"). The evaluator compares it before and after a
	 * probe read to tell THAT refusal from every other one without matching its words: inside a fracture piece's
	 * body a bracket that is empty on this piece only does nothing for it (2026-11-20, the stone-wall author).
	 */
	GRAPHCODESHAPE_API int32 EmptyBracketRefusals();

	/**
	 * **HOW MANY TIMES `Weights` HAS REFUSED A CHOSEN PART FOR LEAVING NOTHING TO MOVE, on this thread** (fault 7,
	 * 2026-11-20): every chosen vertex on the rim a pinned border holds still (a flat cap triangulated from its rim),
	 * or a `[fade=]` wider than the part. Counted for the evaluator as `EmptyBracketRefusals` is: inside a fracture
	 * piece's body such a line does nothing for THAT piece and is skipped there.
	 */
	GRAPHCODESHAPE_API int32 NothingToMoveRefusals();

	/**
	 * **HOW MUCH OF THE INSTRUCTION EACH VERTEX GETS — 1, 0, or a ramp between them** (§5,
	 * `[fade=]`). The `[ ]` layer chooses TRIANGLES; an instruction that moves geometry needs a
	 * per-VERTEX answer, and a fade needs that answer to be continuous.
	 *
	 * **A VERTEX IS IN WHEN ANY TRIANGLE AROUND IT IS, AND MEASURING SETTLED IT.** *Every* triangle
	 * reads as the safer rule — it would leave the shared rim of the excluded faces exactly where
	 * they left it — and it is unusable: a cap is a FLAT TRIANGULATION over its own rim and has no
	 * interior vertex at all, so `[top]` on every capped primitive in this grammar would have
	 * selected nothing. What protects the rim instead is the fade, whose zero sits on it.
	 *
	 * **THE FADE RAMPS INWARD FROM THE SELECTION'S OWN EDGE, NEVER OUTWARD, AND THAT IS THE WHOLE
	 * RULING.** Both directions are continuous at the boundary, so *no seam* does not decide it —
	 * what decides it is that an outward ramp would move geometry the bracket did not select, which
	 * makes `[!bottom]` a lie and `[above=52%]` weather the intact half of a broken column. So the
	 * weight is `min(1, distance to the edge / fade)`: full deep inside, zero at the boundary,
	 * meeting the untouched part at zero.
	 *
	 * Refused, rather than answered: a `[fade=]` with no selecting modifier (there is no edge to
	 * soften), and a fade wide enough that nothing is left at full strength.
	 *
	 * **`bPinBorder` IS THE NO-FADE HALF OF THAT SAME RULING, AND IT IS ROW MR2.** With a fade the
	 * border ring sits at zero and the untouched part stays untouched; with no fade the same ring
	 * moved at FULL strength, and on `roughen(...) [!bottom]` — the grammar's own weathered-stone
	 * idiom — every rim vertex folded a sliver against the flat base it shares. The flag pins that
	 * one ring and nothing else: **it is not an implicit fade**, which is the candidate that was
	 * built, gated green and then refuted by a render, because a ramp is sized in centimetres and
	 * ate a 20 cm plinth whole. A ring is sized in vertices and cannot eat anything.
	 *
	 * **ONLY `roughen` PASSES IT, AND THE REASON IS THE DIRECTION THE VERB MOVES A VERTEX.** A
	 * `taper` or a `displace` moves the region as a body along one axis, so its rim rides with it
	 * and a pinned ring would be a tent where the author wrote a lift. Noise moves each vertex
	 * along its OWN normal, which at a shared rim points into the face that is staying put.
	 */
	GRAPHCODESHAPE_API bool Weights(const UE::Geometry::FDynamicMesh3& Mesh,
		const FGcShapeSelection& Selection, const TCHAR* Verb, TArray<double>& OutWeights,
		FString& OutError, const bool bPinBorder = false);

	/**
	 * **HOW DEEP INSIDE ITS OWN BORDER EACH CHOSEN VERTEX SITS, IN CENTIMETRES** — MC3's `displace`,
	 * and `[fade=]` has been asking the same question since M6.3.
	 *
	 * `OutDepth` is indexed by vertex and carries **-1 for every vertex the bracket did not choose**,
	 * which is what tells *outside the selection* apart from *on its border*. `OutDeepest` is the
	 * largest depth, so a caller wanting `t` from 0 at the border to 1 at the middle divides by it
	 * — and `Weights` divides by `[fade=]` instead. **Two consumers, one measurement**
	 * ([[two-carriers-of-one-fact-is-a-race]]): a fade and a slope disagreeing about where a border
	 * is would put a ramp and a bump on two different rings.
	 *
	 * **`bOutHasBorder` FALSE IS A STATE AND NOT A ZERO.** The whole closed shell was chosen — by no
	 * bracket at all, or by predicates that between them left nothing out — so there is no ring where
	 * the selection meets what it did not choose. Every chosen vertex is then handed the same depth,
	 * which reads as 1 everywhere once normalised: the only honest answer a shape with no edge has.
	 * A caller whose argument is DEFINED from the border (`slope=`, `to=`) refuses on this flag
	 * rather than silently flattening.
	 *
	 * **THE BORDER IS A RING OF POSITIONS AND THE DISTANCE IS STRAIGHT-LINE.** A geodesic across the
	 * surface is the truer measure and is not what any of this needs: a region flat enough to raise a
	 * bump on has the two agreeing, and one folded enough for them to differ has a `slope=` whose
	 * author could not predict either.
	 */
	GRAPHCODESHAPE_API void Reach(const UE::Geometry::FDynamicMesh3& Mesh,
		const FGcShapeSelection& Selection, TArray<double>& OutDepth, double& OutDeepest,
		bool& bOutHasBorder);

	/**
	 * **THE DISTANCE `Reach` HANDS EACH CHOSEN VERTEX, ASKED OF ANY POINT — G19** (`Docs/GC-SHAPE-WOOD-DAMAGE.md`,
	 * 2026-11-18). The same border, gathered by the same code, so a refinement that only needs the band a
	 * `[fade=]` ramps across can ask how deep a vertex it has just made sits. Unset where there is no border to
	 * measure from: an edge set, the whole shape, or a selection that left nothing out.
	 */
	GRAPHCODESHAPE_API TFunction<double(const UE::Math::TVector<double>&)> BorderDistance(
		const UE::Geometry::FDynamicMesh3& Mesh, const FGcShapeSelection& Selection);

	/**
	 * **A MOUND'S OWN COORDINATE ACROSS THE REGION, 0 ON THE BORDER AND 1 AT THE SUMMIT — row MB46.**
	 *
	 * **THE DISTANCE `Reach` HANDS BACK CREASES ALONG ITS MEDIAL AXIS, AND NO SLOPE PROFILE CAN
	 * SMOOTH THAT OUT.** Where two separate pieces of border are equally near, which one wins
	 * switches, and the distance turns a corner there however fine the mesh is. On a 49 x 51 rounded
	 * rectangle that set is a diagonal cross — which is what a user brought back as *broad diagonal
	 * bands and triangular creases* across a seat cushion, with `open 0`, no folds and no crossings.
	 * `f(d(x))` is creased wherever `d` is, and the crease is WORST in mid-slope where smoothstep's
	 * own gradient peaks, then vanishes at the summit where it falls to nothing.
	 *
	 * **SO THIS IS A MEMBRANE UNDER LOAD INSTEAD — `laplacian(u) = -1`, `u = 0` on the border — READ
	 * BACK THROUGH `1 - sqrt(1 - u)`.** A solution of Poisson's equation is analytic in the interior
	 * whatever the border looks like, so there is no ridge to remove; and the map is the exact
	 * inverse of the shape change that got a bare membrane reverted. For a DISC the membrane is
	 * `(R^2 - r^2)/4` and the map returns `1 - r/R`, the distance reading *identically*; for a strip
	 * of width `W` it is `d(W - d)/2` and the map returns `2d/W`, identically again. The two shapes
	 * this grammar's regions are made of keep the profile they have always had, and only the ridge
	 * between them goes.
	 *
	 * **MEASURED, AS THE CONCENTRATION OF THE SURFACE'S OWN SECOND DIFFERENCE** — sharpest over
	 * typical, which is what tells a crease from a dome's honest curvature: the user's pad reads
	 * **50.18x through `Reach` and 3.49x through this**, and a disc, which has no medial axis at all
	 * and is therefore the floor anything can be asked to reach, reads **10.33x and 3.38x**. The area
	 * standing above 25/50/75% of `by` on `conform`'s own disc fixture: a bare membrane 1.46x/1.97x/
	 * 3.01x against today, this **0.98x/0.98x/0.98x**.
	 *
	 * **IT IS SEPARATE FROM `Reach` ON PURPOSE AND THAT IS THE HALF THAT MAKES IT SHIPPABLE.** A
	 * membrane written INTO `Reach` moves `[fade=]` with it — one producer, two consumers — and
	 * `conform`, which reads how a centre's strength tracks its footprint, came back `folds 20`. A
	 * fade is a ramp of a stated WIDTH in centimetres and has to stay linear in them or the number on
	 * the line is a lie; a mound is not measured in centimetres at all. Two questions, so two
	 * producers, and the border ring they start from is still the one ([[one-case-gets-a-predicate-
	 * not-a-retype]]).
	 *
	 * `Depth` is `Reach`'s own output and says who is in, who is on the border and who is outside.
	 * `OutAcross` carries **-1 wherever `Depth` did**. **False means the caller keeps the distance
	 * reading** — no interior to solve over, or the solve did not converge — because an under-solved
	 * Poisson problem KEEPS THE CREASE, which is the defect three earlier operators on this row
	 * shipped.
	 */
	GRAPHCODESHAPE_API bool Membrane(const UE::Geometry::FDynamicMesh3& Mesh,
		const FGcShapeSelection& Selection, const TArray<double>& Depth,
		TArray<double>& OutAcross);
}
