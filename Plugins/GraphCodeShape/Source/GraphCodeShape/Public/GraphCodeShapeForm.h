// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGcShapeDocument;
struct FGcShapeHeader;
struct FGcShapeInstruction;
class IGcShapeVolumes;

namespace UE::Geometry { class FDynamicMesh3; }

/**
 * **GROUP 4 — CHANGE A SHAPE THAT ALREADY EXISTS** — `GC-SHAPE-GRAMMAR.md` §4 group 4.
 *
 * `taper` and `roughen` are the two the grammar calls **ours**, and M6.3 is where the reason for
 * that becomes code: both take a profile, both take a `[ ]` selection and both take `[fade=]`, and
 * every one of those three is a per-VERTEX weight the engine's own doors have no way to accept.
 *
 * **A GROUP-4 PROFILE'S `t` RUNS ALONG `along=`, MEASURED WHEN THE INSTRUCTION RUNS** (§3), not at
 * the end of the entry. A `roughen` written before an `add()` that extends the shape downward is
 * normalised over the smaller range, and moving the line changes the result — the one place in this
 * grammar where a profile's meaning depends on where its line sits.
 *
 * **NEITHER OF THESE TOUCHES A TAG.** They move vertices and leave every polygroup alone, which is
 * what makes §5's *a tag never moves* true through a taper as well as through a bend.
 */
namespace GraphCodeShapeForm
{
	/**
	 * `taper(to=, along=, at=)` — scale the shape about an axis, by an amount that varies along it.
	 *
	 * **IT IS THE SILHOUETTE INSTRUCTION**: one profile is every narrowing, swelling and waisting at
	 * once, which is why a Doric column's entasis is one line rather than a stack of lofts.
	 *
	 * **`to=` READS THE TWO WAYS `extrude(to=)` READS, AND DELIBERATELY THE SAME TWO.** A constant is
	 * *the far end's scale* and runs from full at `t=0` to that value at `t=1`; a profile is the
	 * whole silhouette, read directly at each vertex. Same word, same instruction family, same
	 * meaning — the alternative was `taper(to=82%)` meaning *scale everything by 82%*, which is not
	 * a taper at all and already has a spelling in `scale=`.
	 *
	 * **`at=` IS A POINT AND IT ONLY MOVES THE AXIS SIDEWAYS.** Scaling is about a LINE — the one
	 * through `at=` pointing down `along=` — so where that point sits along the axis cannot matter
	 * and only its offset across it does. Unstated, the axis runs through the shape's current bounds
	 * centre, which is what makes a column taper about itself.
	 */
	GRAPHCODESHAPE_API bool RunTaper(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * `roughen(by=, scale=, seed=, along=)` — Perlin displacement along each vertex's own normal.
	 *
	 * **THE TWO NUMBERS ARE BOTH CENTIMETRES AND THEY MEAN OPPOSITE THINGS** (§4). `by=` is how far
	 * a vertex moves; `scale=` is how big a bump is. The engine carries `Magnitude` and
	 * **`Frequency`**, so `by=` passes through and `scale=` is INVERTED — a feature size is the
	 * number an author has an intuition for, and a frequency is not.
	 *
	 * **THE NOISE IS THE ENGINE'S, RE-STATED ON THIS SIDE OF THE SEAM RATHER THAN CALLED.** MEASURED
	 * in `MeshDeformFunctions.cpp:243`, `ApplyPerlinNoiseToMesh2` is `FMath::PerlinNoise3D` at
	 * `Frequency * (Pos + Offset)` displaced along the vertex normal, with `Offset` the first draw
	 * of an `FRandomStream`; that is twenty lines and it takes a `UDynamicMesh*`, where everything
	 * here is a raw `FDynamicMesh3`. **Calling it for the constant case and doing our own for the
	 * varying one would be two producers of one shape**, so `roughen(by=2.0)` and
	 * `roughen(by=profile(0=2.0, 1=2.0))` could differ — the drift this codebase refuses everywhere
	 * else ([[two-carriers-of-one-fact-is-a-race]]). One path, and it is the engine's arithmetic.
	 *
	 * **`seed=` IS KEYED BY THE ENTRY'S OWN NAME** (§4), so inserting one entry above another does
	 * not re-roll it ([[a-sequential-random-stream-couples-unrelated-items]]).
	 *
	 * **`Notes` IS HOW MUCH ROOM THE SURFACE HAD, AND IT IS OPTIONAL BECAUSE IT IS A REPORT**
	 * (board row MS11). Two facing surfaces a gap apart, each pushed by up to `by`, pass through
	 * each other when `2 * by` exceeds the gap — that is the whole precondition, and it is the
	 * defect that shredded `GC-SHAPE-EXAMPLE.md` §8's column behind `open 0, seam 0`. What the
	 * gap measures across the shapes people have actually written is what decides whether this
	 * becomes a refusal, so the number is reported first and read off the corpus sweep.
	 */
	GRAPHCODESHAPE_API bool RunRoughen(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header, const FString& EntryName,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * `weather(amount=, coverage=, variation=, scale=, chips=, seed=)` — wear exposed convex
	 * arrises into irregular planar facets while leaving the support face exactly untouched.
	 *
	 * This is deliberately not `round()` with a random radius. A radius makes one manufactured
	 * cross-section and repeating it with noise only makes that manufacture untidy. Weathering
	 * removes material with a seeded family of half-space cuts whose depths and attitudes vary
	 * along the exposed edge. Broad source faces therefore remain planes, new wear is faceted, and
	 * the result stays a closed solid. Any proposed cut that reaches a `[bottom]` vertex is rejected;
	 * the grounded face and its complete boundary are a geometry invariant rather than a fortunate
	 * consequence of the seed.
	 *
	 * No bracket means `[convex]`. An explicit edge bracket may narrow that set by height or a saved
	 * border, but inward folds and open rims are never weathered by this operation.
	 */
	GRAPHCODESHAPE_API bool RunWeather(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header, const FString& EntryName,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * **`displace(by=, along=, slope=, to=)` — THE SURFACE'S OWN VERTICES MOVE, BY A VARYING
	 * AMOUNT.** MC3, `GC-SHAPE-MC-DESIGN.md` §4.1 and §4.1a.
	 *
	 * **ONE SENTENCE IS THE WHOLE MECHANISM:** `t` is 0 at the region's BORDER and 1 at its TARGET,
	 * and the distance a point moves is `by × slope(t)`. The four objects §4.1a names — a mound, a
	 * cone, a spike, a mesa — differ in `slope=` and `to=` and in nothing else, which is what makes
	 * this one verb rather than four.
	 *
	 * **IT IS NOT `roughen` WITH A DIFFERENT AMOUNT SOURCE, AND THE DOMAIN IS WHY.** `roughen`'s
	 * amount is a noise field evaluated at the POINT IN SPACE and keyed by a seed — nothing in it
	 * knows the shape of the selection, which only gates who gets the field. A displacement's amount
	 * is a function of WHERE A POINT SITS INSIDE THE REGION, measured from the border the bracket
	 * drew. Two different coordinates, so two verbs. **The measurement that settles it is `along=`**:
	 * on `roughen` it is the axis the `by=` profile is read DOWN, and here it is the direction points
	 * MOVE. Folding them together would put both readings on one word in one verb, which is the
	 * defect MT13b measured ([[one-concept-two-grammars]]).
	 *
	 * **WHAT THEY DO SHARE IS SHARED AS CODE.** The border distance is `GraphCodeShapeSelect::Reach`,
	 * which `[fade=]` has been asking for since M6.3, and the move itself is `GcShapeMovePositions`
	 * — row MS1's rule that a displacement is a function of the POINT and never of the vertex that
	 * happens to sit there. One producer each ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * **IT BUYS ITS OWN DENSITY OR IT IS A SILENT NO-OP ON ITS COMMONEST INPUT.** A flat `[top]` is
	 * two triangles whose four vertices are ALL on the border, so a mound would move nothing at all
	 * and report success. The refinement is priced by the SLOPE's own `Stations` at the header's
	 * tolerance, scaled by the region's reach — row MV's currency, and no new number decides a
	 * triangle count.
	 *
	 * **A SLOPE AND A TARGET BOTH NEED A BORDER, so a whole-shape selection refuses them both** and
	 * names the one reading that needs none: a bare `displace(by=)`, every vertex the same distance
	 * along its own normal.
	 */
	GRAPHCODESHAPE_API bool RunDisplace(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * `coat(by=, edge=) [region]` — a liquid layer: the region's own vertices moved out along their
	 * normals by `by=`, rising from nothing at the region's border as a quarter round over `edge=`
	 * (default `by=`). User-raised 2026-09-28: *"can you make a spline based coating?"*
	 * (`GC-SHAPE-GRAMMAR.md`, `coat`).
	 *
	 * **THE SURFACE ITSELF, NOT A SECOND SOLID WELDED ON**, so a glaze and its dough are one skin and
	 * a run `drip()` bent into the region is the same layer as the rest of it. Thickness is measured
	 * from the border in centimetres (`Reach`, `[fade=]`'s distance) and not normalised, so a run
	 * narrower than two edges is thinner than the film by construction.
	 */
	GRAPHCODESHAPE_API bool RunCoat(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * **`conform(to=, by=, within=, mode=, gap=, project=, at=)` — EVERY CHOSEN VERTEX MOVES TOWARD
	 * A TARGET, BY A FRACTION OF ITS OWN DISTANCE TO IT.** `GC-SHAPE-PRESET-LIBRARY.md` §6.4,
	 * user-directed into
	 * scope 2026-09-11. It is what the capability audit ranked as `conform` and `flatten` both, and
	 * the two are one verb here.
	 *
	 * **THE FRACTION IS OF EACH VERTEX'S OWN DISTANCE, AND THAT SINGLE SENTENCE IS THE WHOLE VERB.**
	 * `displace` and `raise` move a region by ONE offset, which cannot close different distances
	 * everywhere — a nail laid on a fingertip floats at the middle or sinks at the ends, whichever
	 * number is chosen. `by=100%` lands every chosen vertex ON the target however far it started, so
	 * changing a footprint's width cannot silently change its centre strength: that invariant is
	 * what the audit demanded and it is a property of the arithmetic rather than of a spec.
	 *
	 * **`flatten` FOLDS IN AS A TARGET RATHER THAN SURVIVING AS A VERB.** Both are *move each
	 * selected vertex toward a target, by its own distance to it*, and a plane is a target; the
	 * audit proposed two because Blender has two families, which is Blender's history and not a
	 * geometric distinction. Where flatten genuinely differs is that its plane is DERIVED rather
	 * than named — and a derived default owes an override, which is what `to=Z` and `at=` are.
	 *
	 * **`to=` IS A LADDER OF THREE READINGS, EACH RUNG TAKING ONE DECISION BACK OFF THE EVALUATOR.**
	 * `to=plane` derives the normal AND the position from the chosen faces; an axis or a vector
	 * fixes the normal and leaves the position derived; `at=` fixes the position too. `to=$Name` is
	 * a `mesh()` entry's surface, used where it sits, which is the shrinkwrap case.
	 *
	 * **`pinch` IS NOT FOLDED IN, AND THE TARGET'S DIMENSION IS WHY** (§6.2). This moves a surface
	 * toward a SURFACE, off it; drawing vertices toward a curve or a point happens ALONG the
	 * surface. Same selection machinery, genuinely different question — so a `plane()` entry, which
	 * is a PATH in this grammar, is refused by name rather than being given an invented reading.
	 *
	 * **IT REFINES ON REQUEST AND NEVER ON ITS OWN, WHICH IS A RULING AND NOT A GAP.** `displace`
	 * prices its own density because the curve it spends is on its own line; how finely a landing
	 * has to be sampled is a property of the TARGET's curvature, which lives in somebody else's
	 * entry. **So the shortfall is MEASURED AND REPORTED instead**: every moved vertex sits on the
	 * target by construction, so `Notes` carries how far the FACES between them still stand off —
	 * the one place a coarse landing is visible, and the number that says whether `[smooth=]` is
	 * owed.
	 *
	 * **`gap=<cm>` AND `project=<direction>` JOINED 2026-09-14.** A gap stops the landing that far
	 * in front of the target along its own normal (negative sinks in); a projection sends every
	 * vertex one way instead of to its nearest point, so artwork keeps its shape. The note now names
	 * the `[smooth=]` that would bring the stand-off under the header's own `smooth:`, and counts
	 * which door each mesh landing left `GcShapeLandOnShaded` by.
	 */
	/** Shrink a marked region radially inside a placed mesh envelope, preserving UVs. */
	GRAPHCODESHAPE_API bool RunFit(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	GRAPHCODESHAPE_API bool RunConform(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * **`pinch(to=, by=, within=)` — EVERY CHOSEN VERTEX SLIDES ACROSS THE SURFACE TOWARD A CURVE
	 * OR A POINT, BY A FRACTION OF ITS OWN SIDEWAYS DISTANCE TO IT.**
	 * `GC-SHAPE-PRESET-LIBRARY.md` §6.2 and §6.4.
	 *
	 * **IT IS `conform`'S SENTENCE ONE DIMENSION DOWN, AND THE TARGET'S DIMENSION IS THE WHOLE
	 * DIFFERENCE.** `conform` moves a surface onto a SURFACE, OFF it; this draws a surface toward a
	 * CURVE, ALONG it. Same selection machinery, same fraction-of-its-own-distance arithmetic,
	 * genuinely different question — so a `mesh()` target here is refused with the verb that wants
	 * one, and a `plane()` target on `conform` is refused with this one. The pair is what makes
	 * either refusal learnable.
	 *
	 * **THE MOVE IS THE TANGENTIAL COMPONENT AND NOTHING ELSE, WHICH IS THE ONE SENTENCE THAT MAKES
	 * THIS A PINCH.** The whole vector to the target would lift the region off the shape and land
	 * it on the curve — that is `conform`'s move against a 1D target, and it swallows a surface
	 * rather than gathering it. Taking the component along the normal out leaves the part that
	 * slides ACROSS, so the material stays on the shape and closes up.
	 *
	 * **AND THE DIRECTION IS A FUNCTION OF THE POSITION, NEVER OF THE VERTEX.** A tangential move
	 * depends on which way the surface faces, and two vertices at one point on a seam carry
	 * different normals — handed their own, the two halves slide apart and the shape cracks along
	 * the seam while every headline number reads perfect. Normals are summed per POSITION, exactly
	 * as the selection weight is.
	 *
	 * **`to=line` IS REFUSED BY NAME RATHER THAN DERIVED**, on §6.5's own warning: a region's
	 * derived centreline is STRAIGHT, gathering toward a straight axis is `taper`'s case, and a
	 * word whose only honest use is one another verb already covers is not worth adding.
	 */
	GRAPHCODESHAPE_API bool RunPinch(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * **A REGION'S CENTRE AND THE DIRECTION IT FACES, both weighted by AREA** — MB6, and shared
	 * since `pull` needed the second half for its default direction.
	 *
	 * **AREA AND NOT TRIANGLE COUNT, because a triangulation is not a measurement.** A cut face
	 * arrives as a fan or a strip depending on what cut it, so counting triangles would move a
	 * socket when a `remesh` above it changed nothing about the surface.
	 *
	 * `OutFacing` is the average normal, UNNORMALISED: its length against `OutArea` is exactly
	 * *how much these faces agree*, and a caller spends that rather than making a second pass.
	 * `socket()` reads it to aim an attachment; `pull` reads it to decide whether a handle has one
	 * direction at all.
	 */
	GRAPHCODESHAPE_API void RegionFrame(const UE::Geometry::FDynamicMesh3& Mesh,
		const TArray<int32>& Triangles, UE::Math::TVector<double>& OutCentre,
		UE::Math::TVector<double>& OutFacing, double& OutArea);

	/**
	 * `pull(by=, along=, slope=) [handle, carry=influence]` — move a named region RIGIDLY and let a
	 * second named region follow it.
	 *
	 * **THE BRACKET IS THE HANDLE AND EVERY VERTEX OF IT TAKES THE SAME OFFSET**, which is the one
	 * thing `displace` cannot do. A displacement spends `by=` through a slope read off the distance
	 * to its own border, so the plateau of `slope=profile(0=0%, 0.55=100%, 1=100%)` is a LEVEL SET
	 * and not a patch anybody named: on an extruded boss the point furthest from the base ring is
	 * the CENTRE of the cap, so the cap's rim gets less than full travel and the cap domes. No
	 * profile keeps a named face flat while its surround eases, and that is this verb.
	 *
	 * **AND THE OTHER THREE NEIGHBOURS EACH HOLD ONE PIECE OF IT** (§4.0's table): `raise` moves a
	 * region rigidly and stitches a crisp WALL, along the normal; `conform` takes a second region
	 * through `carry=` but lands on a PLACE, so a slanted cap flattens onto the target rather than
	 * translating; `pinch` gathers sideways. Rigid handle, authored direction, soft follow — three
	 * verbs, three of the four, and none with all of them.
	 *
	 * **THE FOLLOWING IS `conform`'S, UNCHANGED AND NOT REIMPLEMENTED** — `GcShapeCarryLanding`
	 * never asked HOW its landing moved, so a second copy of it would have inherited none of the
	 * fixes MB36 and MB41 bought ([[a-second-writer-of-one-rule-inherits-none-of-its-fixes]]). What
	 * this verb adds there is one line: `slope=` reshapes a border-to-fixed ratio already computed.
	 *
	 * **`carry=` IS REQUIRED AND THE REFUSAL NAMES `raise`.** A rigid move with nothing following
	 * tears the single ring it shares with its neighbours — the *ramp one triangle wide* §4.0 calls
	 * a modelling error — and a crisp border is what `raise` stitches a wall for.
	 */
	GRAPHCODESHAPE_API bool RunPull(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * `warp(to=(x, y, z))`, `warp(by=(dx, dy, dz))`, `warp(by=d)` — **EVERY POINT MOVED BY A FORMULA OF
	 * ITSELF** (2026-10-30; user: *"find a way to let authoring a sport car possible, even consider refer
	 * to threejs approach"*, and *"it should be flexible and easy to use"*).
	 *
	 * **THE LOW-LEVEL BLOCK THE GRAMMAR LACKED, AND IT IS THE ONE BLENDER AND THREE.JS BOTH BUILD ON.**
	 * Geometry Nodes' *Set Position* with a field; three.js's loop over a geometry's `position` and then
	 * `computeVertexNormals()`. Every arithmetic value here was one number per LINE; this is the first
	 * that is one number per POINT. A recipe (`instruction()`) is how an author makes a verb of it.
	 *
	 * **IT MOVES POINTS AND NEVER MAKES OR BREAKS A TRIANGLE, WHICH IS WHAT KEEPS IT SAFE TO HAND AN
	 * AI.** The topology is whatever the verbs above it built, so a closed body stays closed — the rule
	 * `GC-SHAPE-BLOCKS-AND-EFFECTS.md` §2 holds every effect to (*nobody writes raw vertices*).
	 *
	 * The per-point words are bare, as `output.` is: `point.x/y/z` where the point is now (cm),
	 * `point.normal.x/y/z` which way the surface faces there, and `point.t.x/y/z` from 0 to 1 across the
	 * region being warped — the `t` a profile reads, so `$Roof(point.t.x)` is a roofline along the length.
	 *
	 * **WHAT IT SAYS:** a per-point failure names the point; a note gives how many points moved and
	 * the farthest, and names the triangles a move turned inside out, where they are.
	 */
	GRAPHCODESHAPE_API bool RunWarp(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * **G30a: A BRACKET'S `value=<formula>`, READ AT EVERY POINT** (2026-11-20, `Docs/GC-SHAPE-SNOW-MOSS.md`). The formula
	 * is a warp formula (`point.*`, `noise()`, `cell()`, `dist()`, `sky()`, `$Curve(t)`), compiled once and read once per
	 * POSITION, so two vertices at one place get one answer (MS1). `OutByVertex` is indexed by vertex id. `point.t` runs
	 * 0..1 across the whole shape's bounds; `point.normal` is the area-weighted normal of every face round the point.
	 */
	GRAPHCODESHAPE_API bool ValueAtPoints(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes, const TCHAR* Verb,
		int32 LineNumber, const FString& Formula, const UE::Geometry::FDynamicMesh3& Mesh, TArray<double>& OutByVertex,
		FString& OutError);

	/**
	 * `cut(plane=, at=, fill=)` — take a plane to the shape and keep one half of it.
	 *
	 * **IT KEEPS THE SIDE THE AXIS POINTS AWAY FROM** (§4): `plane=Z` keeps the part below,
	 * `plane=-Z` the part above. MEASURED, that falls out of the door rather than needing a flip —
	 * `FMeshPlaneCut::Cut` *"deletes any triangles on the positive side of the cutting plane"*, so
	 * handing it the named axis as the normal is already the grammar's sentence.
	 *
	 * **`at=` IS A POINT OR A FRACTION AND BOTH ARE IN THE CORPUS.** `cut(plane=Z, at=(0,0,340))`
	 * is a height somebody measured; `cut(plane=Z, at=65%)` is *snapped at two thirds, whatever it
	 * is* — the ruined column, which has to keep working when the shaft's height changes.
	 *
	 * **`fill=true` CLOSES THE CUT AND THE NEW FACE IS TAGGED `cut`** — §5's table, the same tag a
	 * boolean writes, because it is the same kind of face. `fill=false` leaves the shape open, which
	 * is how a column is snapped off rather than sawn.
	 */
	GRAPHCODESHAPE_API bool RunCut(const FGcShapeDocument& Document,
		const FGcShapeInstruction& Line, UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * `bend(angle=, around=, extent=, at=)` — curve the shape about an axis.
	 *
	 * **`around=` IS REQUIRED AND IS NOT DEFAULTED, for `widen(about=)`'s reason exactly**: a column
	 * told to lean 30 degrees has not been told which way. Up is privileged in this grammar and
	 * nothing gives X or Y that status, so there is no honest default to pick.
	 *
	 * **THIS ONE CALLS THE DOOR RATHER THAN RESTATING IT, AND THE TRADE IS THE OPPOSITE OF
	 * `roughen`'s.** Wrapping a raw `FDynamicMesh3` in a `UDynamicMesh` buys an allocation and a
	 * UObject; what it buys HERE is Barr's bend with its own analytic normal transform — the
	 * inverse Jacobian of the deformation, which is exactly right where a recompute is only nearly
	 * right — and a hundred lines of matrix work we would otherwise own a second copy of.
	 * `roughen`'s door bought twenty lines and would have made one shape have two producers.
	 *
	 * **`extent=` IS THE ENGINE'S OWN FALLOFF** (§5), which is why `[fade=]` means nothing here: the
	 * bend is distributed over `±extent` about `at=` and the material beyond it rides along rigidly.
	 * Unstated, the extent is half the shape's span, so the whole thing curves.
	 *
	 * **AND IT BUYS ITS OWN RINGS FIRST, FOR `taper`'S REASON AND MORE SHARPLY.** MEASURED: a 90
	 * degree bend of a plain two-ring column comes out a WEDGE — its two end faces rigidly rotated
	 * and straight walls between them — and the bounding box then grows in the direction the curve
	 * was supposed to shorten, which is how this was caught. A bend IS a curve, so the rings come
	 * from the same place every other curve's do: the arc's radius is `2 * extent / angle` and
	 * `smooth:` says how many segments that radius earns.
	 */
	/**
	 * `round(2.5) [ ]` on a SHAPE — bevel the chosen edges. Group 4's half of the one word in two
	 * groups; group 2's half rounds an outline's corners and lives in `GraphCodeShapeOutline`.
	 *
	 * **THE DOOR IS RAW AND THE GRAMMAR HAD PRICED A HIGHER ONE.** §11 named
	 * `ApplyMeshBevelEdgeSelection`, which is `UDynamicMesh`-shaped and wants an
	 * `FGeometryScriptMeshSelection` of edges. MEASURED, `FMeshBevel` itself takes a raw
	 * `FDynamicMesh3` and carries `InitializeFromTriangleSet` — *border loops of the selected
	 * triangles* — so **`round(2.5) [top]` needs no edge-selection type and no wrapper: the face
	 * selection this plugin already ships IS the input.** The edge layer `[rim]` and `[sharp]`
	 * needed turned out to be theirs alone, not `round`'s.
	 *
	 * **NO BRACKET MEANS EVERY GROUP BOUNDARY, WHICH IS NOT EVERY HARD EDGE, AND THE DIFFERENCE IS
	 * AUTHORABLE.** A box extruded from a `rect` has ONE `side` group, so its four vertical corners
	 * are interior to a group and a bare `round(2.5)` leaves them — it softens the two rims. The
	 * corners are `round(2.5) [sharp]`, which is exactly the want §5 gave that word.
	 *
	 * **`Subdivisions` COMES FROM `smooth:` OR THE WORD IS A LIE.** MEASURED, the engine defaults it
	 * to 0 and `RoundWeight`'s own comment says *"Ignored if Subdivisions = 0"*, so the door as
	 * shipped produces a single flat facet. The bevel's cross-section is a quarter turn of radius
	 * `distance`, so it is priced through `SegmentsForArc` like every other curve here, and
	 * `segments=` overrides it — `round(2.5) [top, segments=1]` is a deliberate chamfer.
	 *
	 * **AND EVERY NEW FACE IS RE-TAGGED, BECAUSE THE ENGINE'S INFERENCE IS ABOUT MATERIAL IDS.**
	 * `FMeshBevel::MaterialIDMode` infers a MaterialID from the two adjacent faces; §5's tags are
	 * POLYGROUPS and the bevel allocates fresh group IDs for its strips. Left alone, a rounded shape
	 * would carry groups outside the four-value set and every bracket downstream would stop working
	 * — so §5's rule (*tagged like the face on each side where those agree, else `side`*) is applied
	 * here, over the operation's own `NewTriangles`.
	 */
	GRAPHCODESHAPE_API bool RunRound(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header, const FString& EntryName,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * `shell(8.0) [ ]` — give a surface thickness, or hollow a solid.
	 *
	 * **THE PRICED DOOR IS THE WRONG ONE FOR AN AUTHORED SHAPE, AND ITS OWN HEADER SAYS SO.** §11
	 * named `ApplyMeshShell`. MEASURED, that takes `FGeometryScriptMeshOffsetOptions` — `SolveSteps`
	 * 5, `SmoothAlpha`, `BoundaryAlpha` — an ITERATIVE SMOOTHING solver along averaged vertex
	 * normals, and Epic's comment on its sibling reads *"intended for high-res meshes, for
	 * polymodeling-style offsets, ApplyMeshOffsetFaces will produce better results"*. On a box, an
	 * averaged vertex normal at a corner points diagonally, so `shell(8.0)` would put roughly 4.6 cm
	 * of wall where the author wrote 8 — a number that is wrong without ever looking wrong.
	 *
	 * **SO THE DOOR IS `FOffsetMeshRegion` WITH THE PARALLEL-FACE VECTOR**, which is what
	 * GeometryScript's own `ParallelFaceOffset` resolves to
	 * (`SelectionTriNormalsAngleWeightedAdjusted`): the per-vertex length is adjusted so the offset
	 * faces stay PARALLEL to where they were, which is what a wall thickness means. It is raw, so no
	 * wrapper. **`bOffsetFullComponentsAsSolids` is what makes this `shell` rather than `offset`** —
	 * a selection that is a whole closed component is thickened into a solid rather than moved.
	 *
	 * ~~**AND THE ENGINE ANSWERS BOTH HALVES OF §5's TAG RULE ITSELF.**~~ **MEASURED FALSE
	 * 2026-08-24, AND IT COST A WHOLE OBJECT** (board row MW). `FOffsetInfo::OffsetTids` really are
	 * *the same Tids that were originally in the region* — that half is true and is what the fix
	 * stands on — but the door RENUMBERS the group on every one of them, and its
	 * `bSingleGroupPerArea` default collapses the whole region into ONE new id. One tapered cylinder
	 * went `top 59 / bottom 59 / side 8052` -> `top 0 / bottom 0 / side 16340`, so every `[ ]`
	 * bracket after a `shell()` selected everything and reported success. **A bracket that selects
	 * nothing is noticed in one build; one that selects everything looks like a shape somebody
	 * chose.**
	 *
	 * **SO THE TAGS ARE CARRIED ACROSS THE DOOR RATHER THAN LEFT TO IT**: the flag goes off, which
	 * makes the renumbering one-to-one, and the id each tag landed on is read back off the triangles
	 * that went in wearing it. `StitchTriangles` are the wall closing the mouth and are still set to
	 * `side`, allocated from a different pool so the two passes cannot fight.
	 *
	 * **ONE MEASURED LIMIT WORTH THE AUTHOR KNOWING**: `MaxScaleForAdjustingTriNormalsOffset` is
	 * 4.0, so at a corner sharper than that the parallel adjustment is CLAMPED and the wall is
	 * thinner there than asked. A clamp is not an error and reports nothing
	 * ([[a-clamped-measurement-breaks-a-proportional-loop]]).
	 */
	/**
	 * **`raise(by=, inset=, wall=)` — A REGION MOVES OFF THE SURFACE AROUND IT.** MC3, and `wall=`
	 * at MS3.
	 *
	 * **TWO AXES, ONE VERB, AND THAT IS THE RULING RATHER THAN A SAVING.** `by=` moves the region
	 * PERPENDICULAR to the surface and `inset=` moves its border WITHIN it — which is why every
	 * modeller separates them — but nobody insets without raising, so two optional numbers cover a
	 * panel, a vent, a badge and a recess. A negative `by=` sinks it, which is a recess.
	 *
	 * **THREE PROFILES, AND THE THIRD COST A WORD RATHER THAN A NUMBER — MS3, user-raised** (*"like
	 * a bump or like a slope... or like a cliff"*). No `inset=` is a CLIFF; `inset=` is a STEP, a
	 * shoulder in the surface and then a vertical wall; `wall=sloped` spends the same inset ON THE
	 * WAY UP and leaves one leaning wall — a batter. The two numbers already say how far in and how
	 * far up, so what separates a step from a batter is WHEN the inset happens, which no third
	 * number can say. **And the build is the inset's own skirt, tilted**: lifting the ring the
	 * inset left, and nothing else, turns that strip into the wall — so the mitre arithmetic at
	 * every corner stays the engine's rather than becoming ours.
	 *
	 * **IT SHARES `shell()`'S ENGINE DOOR AND SETS THE OPPOSITE FLAG.**
	 * `bOffsetFullComponentsAsSolids` is what `shell` turns ON so a closed component is thickened
	 * into a SOLID; a raise leaves it OFF, so the region MOVES and side walls stitch it to what
	 * stayed put. Same call, opposite answer, and it is why these are two verbs.
	 *
	 * **A RAISE NEEDS A REGION AND REFUSES A BARE BRACKET.** Every face moving is not a panel, it
	 * is an inflate — and `shell()` is the instruction that gives a whole surface thickness.
	 */
	/**
	 * `OutWalls` is the side walls this raise stitched — board row MR7, and what `[$N = wall]` names.
	 *
	 * **A RAISE UP TAGS ITS WALLS `side`, WHICH IS THE SURFACE THEY STAND ON**, so no face word can
	 * pick them out; a raise DOWN tags them `cut`, so `[!cut]` reaches those by accident of sign.
	 * MC2 gave a boolean's contribution a name (`seam`) with the argument that a predicate changes
	 * meaning as a file runs while a captured name freezes the set at the moment it was true. The
	 * same sentence is true here, measured 2026-09-02: with no way to say *roughen everything except
	 * the ledges I just made*, roughen-last shreds the walls and roughen-first gives the raise a
	 * ragged border, and there is no third ordering.
	 */
	GRAPHCODESHAPE_API bool RunRaise(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError,
		TArray<int32>* OutWalls = nullptr);

	GRAPHCODESHAPE_API bool RunShell(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/** G29: a bracket limits the bend to the faces it chooses, `[fade=]` and `[smooth=]` as on `warp`. */
	GRAPHCODESHAPE_API bool RunBend(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);
	/** The bend with no file to reach into (a spec's direct call): a bracket that names another entry is refused. */
	GRAPHCODESHAPE_API bool RunBend(const FGcShapeDocument& Document,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, FString& OutError);

	/**
	 * `relax(<distance>)` — settle the surface toward its own neighbourhood. Board row MR22.
	 *
	 * **THE NUMBER IS A CAP, NOT AN AMOUNT, AND THAT IS WHAT MAKES IT AUTHORABLE.** An iteration
	 * count and a per-pass alpha are the two numbers every smoothing door actually takes, and
	 * neither is a thing an author knows about their object; a DISTANCE is. `relax(3.0)` says *let
	 * the surface settle as far as it can without moving any point more than three centimetres from
	 * where it started* — so features smaller than the cap dissolve, features larger than it
	 * survive, and the promise is one the build line can check because it bounds the bounds.
	 *
	 * **WHY THIS IS THE CREATE-SIDE KEYSTONE AND NOT MESH HYGIENE** (MR22, user-raised): an authored
	 * shape looks authored, because every crease announces the boolean that put it there. A union of
	 * jittered balls reads as a union of jittered balls. Photogrammetry looks right for free
	 * precisely because it carries no construction history, and **the only operation that launders
	 * construction history is a relax pass** — which is why it sits above `remesh` on the CREATE
	 * track rather than beside it in the cleanup family.
	 *
	 * **IT IS OURS RATHER THAN `ApplyIterativeSmoothingToMesh` FOR THE REASON `roughen` IS OURS.**
	 * The door takes neither a `[ ]` selection nor a `[fade=]`, and both are per-VERTEX weights;
	 * and the cap has to be measured from each point's ORIGINAL position, which an iteration door
	 * that only ever sees the current one cannot do.
	 *
	 * **AND IT MOVES POINTS, NOT VERTICES** — MS1's rule, shared with `roughen` and `displace`: two
	 * vertices at one position are a pinch, and moving them apart tears a closed mesh into a holed
	 * one. `Notes` carries how far the surface actually settled, because a cap that never bound and
	 * a surface that has finished settling are the same picture and different instructions.
	 */
	GRAPHCODESHAPE_API bool RunRelax(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * `remesh(<size>)` — rebuild the whole shape from a voxel grid at that feature size. Row MR11.
	 *
	 * **IT CHANGES THE AUTHORING RULE RATHER THAN FIXING A MESH.** Without it the rule is *displace
	 * gently enough not to tear*, which is the constraint that cost the fifth round seventeen
	 * builds; with it the rule is *displace as hard as you like, then rebuild*. A voxel wrap is
	 * closed by construction, so self-intersection, a torn skirt, a shattered island count and an
	 * overlapping shelf are all one repair — and it also erases the LATHE, because after a rebuild
	 * nothing can tell a revolved dome from a sphere.
	 *
	 * **THE SIZE IS `roughen(scale=)`'S NUMBER AND MEANS THE SAME THING**: the finest feature that
	 * survives. It is the grid cell, so cost is `1/size³` and the refusal names both numbers rather
	 * than letting the editor stop — the same door `roughen` already has, one dimension worse.
	 *
	 * **A REBUILD RESAMPLES RATHER THAN RENUMBERS, WHICH IS THE COST THE ROW DID NOT PRICE.** Every
	 * polygroup dies: §5's four face tags, `[$Joins = seam]`, `[$Ledge = wall]`, and with them every
	 * bracket downstream. So the tags are RE-DERIVED BY PROXIMITY against the pre-rebuild surface —
	 * each new triangle takes the tag of the nearest old one — which is the only handle left once
	 * identity is gone, and is what the board says a carried LABEL cannot survive.
	 *
	 * Existing UV charts are projected onto the rebuilt surface. Chart boundaries follow the
	 * new triangles; features finer than the voxel size cannot retain their original outline.
	 */
	GRAPHCODESHAPE_API bool RunRemesh(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * **EVERY SEMANTIC CHANNEL RE-DERIVED FROM WHERE IT WAS, because a voxel pass resamples rather
	 * than renumbers.** Each new triangle takes the tag of the nearest old one — §5's four face
	 * tags, every `[$Name = seam]` capture layer, and (row MB12) the material id — which is the only
	 * handle left once identity is gone. Saved coordinate fields are vertex weights, so each new
	 * vertex takes the barycentrically interpolated value on the nearest old triangle. An unmatched
	 * element takes `side`, slot 0 or field value 0 as appropriate.
	 *
	 * **PUBLIC BECAUSE `join=blend` RESAMPLES EXACTLY THIS WAY — row MB11.** `remesh` and a blend
	 * are one machine asked two questions, and a second copy of this walk would be a second answer
	 * to *what does a rebuild lose* ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	GRAPHCODESHAPE_API void CarryTagsAcross(const UE::Geometry::FDynamicMesh3& Before,
		UE::Geometry::FDynamicMesh3& After);

	/** Project UVs within source charts, keeping chart seams separate on the new mesh. */
	GRAPHCODESHAPE_API void CarryUVsAcross(const UE::Geometry::FDynamicMesh3& Before,
		UE::Geometry::FDynamicMesh3& After);

	/**
	 * `fill() [rim, …]` on a shape that already has depth — close its open borders with smooth
	 * patches tagged `cut`, each wearing the material around it. S4,
	 * `Docs/GC-SHAPE-FIELD-REPORT-2026-10-13.md`: an imported open shell closes without egg plugs
	 * or a voxel re-melt, so its detail survives.
	 */
	GRAPHCODESHAPE_API bool RunFillHoles(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header, UE::Geometry::FDynamicMesh3& Mesh,
		TArray<FString>* Notes, FString& OutError);

	/** Keep the edge-connected shell with greatest surface area; ties keep the first shell. */
	GRAPHCODESHAPE_API bool RunKeep(const FGcShapeDocument& Document, IGcShapeVolumes& Volumes,
		const FGcShapeInstruction& Line, const FGcShapeHeader& Header,
		UE::Geometry::FDynamicMesh3& Mesh, TArray<FString>* Notes, FString& OutError);

	/**
	 * **`add(join=blend, falloff=)` — ONE VOXEL PASS OVER EVERY OPERAND AT ONCE.** Row MB11.
	 *
	 * **N-ARY BY THE ENGINE'S OWN CHOICE, NOT BY OURS.** `TImplicitBlend::Sources` is an array, so
	 * `add($Blob, repeat=3, join=blend)` blends four shapes in ONE pass — which is both far cheaper
	 * than three rebuilds and the right meaning: three blobs reading as one rock is exactly
	 * all-at-once, and pairwise blending gives a different and worse shape.
	 *
	 * **A BLEND IS NOT LOCAL AND THE GRAMMAR SAYS SO OUT LOUD.** Every shape handed in is rebuilt
	 * from a distance field, so the whole result is re-tessellated at grid density and detail finer
	 * than a voxel is gone. `CarryTagsAcross` recovers the tags, materials and saved coordinate
	 * fields; nothing recovers the topology or the triangle count. That is a property of the
	 * technique and is why this is a machine spelled beside `add` rather than a mode of it.
	 *
	 * **THE GRID IS DERIVED FROM `falloff=` AND THE FILE NEVER MENTIONS IT** (§5.0) — row MV's
	 * argument, one product along: a band sampled on cells wider than the band does not come back
	 * coarser, it comes back arbitrary. `BlendPower` is deliberately not offered; one number that
	 * means a distance in centimetres is this format's habit and a second dimensionless *how soft*
	 * would be a second answer to one question.
	 */
	/**
	 * **`add(join=fillet, radius=)` — THE CREASE THE UNION MADE, SOFTENED BY AN ARC.** Row MB11.
	 *
	 * **ASSEMBLED ENTIRELY FROM SHIPPED PARTS, AND IT EARNS ITS WORD ON A MEASUREMENT.**
	 * `add($X) [$J = seam]` then `round(r) [$J]` is the composition MC2 built the seam capture FOR,
	 * and ZERO of the corpus's 43 `.gcshape` files contain a capture of ANY kind — not one. By
	 * MB4's own test (*if the mechanism is there and nobody ever wrote the composition, the missing
	 * thing is the WORD*) it wants one. **The caveat is kept**: 43 files are mostly fixtures, so
	 * *nobody needed a fillet* is a competing reading of the same zero.
	 *
	 * **`Region` IS THE PRODUCER'S OWN ANSWER, never a post-hoc one.** `FMeshBoolean` reports which
	 * input each result triangle came from, so the seam arrives handed over rather than inferred
	 * from a normal or a boundary walk.
	 */
	GRAPHCODESHAPE_API bool FilletRegion(UE::Geometry::FDynamicMesh3& Mesh,
		const UE::Geometry::FDynamicMesh3& Host, const UE::Geometry::FDynamicMesh3& Part,
		const TArray<int32>& Region, double Radius, double Smooth, double ShadeDegrees,
		const FString& Where, TArray<FString>* Notes, FString& OutError);

	GRAPHCODESHAPE_API bool Blend(const TArray<const UE::Geometry::FDynamicMesh3*>& Sources,
		double Falloff, const FString& Where, UE::Geometry::FDynamicMesh3& Out, FString& OutError);

	/**
	 * Organic assembly blend. Sources[0] is the receiver; later sources blend with that receiver but
	 * not directly with one another. This prevents nearby parallel attachments from growing a bar
	 * between their shafts while still allowing receiver skin to rise into a shared saddle at roots.
	 */
	GRAPHCODESHAPE_API bool BlendReceiverMediated(
		const TArray<const UE::Geometry::FDynamicMesh3*>& Sources, double Falloff,
		const FString& Where, UE::Geometry::FDynamicMesh3& Out, FString& OutError);

	/** A measured transition/collar support in [0,1]; zero is the unblended union. */
	GRAPHCODESHAPE_API bool BlendReceiverMediated(
		const TArray<const UE::Geometry::FDynamicMesh3*>& Sources, double Falloff,
		const TFunction<double(const FVector3d&)>& SupportWeight, const FString& Where,
		UE::Geometry::FDynamicMesh3& Out, FString& OutError);

	/** Incoming ownership rises from the buried root to the authored distal mesh. */
	GRAPHCODESHAPE_API bool BlendReceiverMediated(
		const TArray<const UE::Geometry::FDynamicMesh3*>& Sources, double Falloff,
		const TFunction<double(const FVector3d&)>& SupportWeight,
		const TFunction<double(const FVector3d&, int32)>& IncomingOwnership,
		const FString& Where, UE::Geometry::FDynamicMesh3& Out, FString& OutError);
}
