// Copyright GraphCode Sdn Bhd. All Rights Reserved.

// The authored-region kernel — `GC-SPATIAL-AUTHORING-DESIGN.md` §21, task R0.
//
// A region is a closed ring of edges over points **it declares itself**, NOT a box component in the
// level. Everything here is pure arithmetic over the structs below: no world, no map, no asset, no
// file. Same contract as `GraphCodeSpatial.h` (Z0) and `GraphCodeSpatialPlacement.h` (P0), and for
// the same reason — the maths is proved before anything can parse or write with it.
//
// WHERE THE FILE IS. Nothing here reads a `.gcw`. R1's parser fills an `FGcRegionSpace` and asks;
// R2 turns the answers into `FGcPlacementTarget` values. That seam is why §19.8's *the project becomes
// askable without opening it* can be true of a region when it was never true of a region actor.
//
// **A REGION OWNS ITS POINTS AND EDGES — §21.1b, user-ruled 2026-08-12, and NOTHING IS SHARED.**
// *"a region is its own. sharing points makes references and easy to break when i want to change one
// region."* So a name lives in ONE region: every room has its own `north` and its own `p1`, and
// `$Hall.north` is a scoped lookup rather than a global one that then asserts membership. The arrays
// below stay flat because indices are cheap and every query here is index-based; ownership is the
// `Region` field on a point and an edge, and it is total — there is no unowned geometry.
//
// TWO CONVENTIONS THAT DECIDE EVERYTHING BELOW, both stated once here:
//
//   1. **A BULGE IS SIGNED AGAINST THE EDGE'S OWN DECLARED DIRECTION, never against the ring's
//      traversal.** `east = p2 -> p3 bulge:0.4` bows to the LEFT of `p2 -> p3` — and it bows the same
//      way whichever way round the ring is authored and whichever way the normalisation below then
//      walks it. A walk that traverses an edge backwards negates the bulge, which is the same shape.
//      Sharing used to be the argument for this; ownership does not weaken it, because the walk is
//      still derived and a spot on a wall may not move when somebody re-authors the ring.
//
//   2. **WINDING IS DERIVED (§21.6, A6), so the authored order carries no meaning at all.** The ring
//      is chained by shared point index, normalised to counter-clockwise by signed area, and only
//      then does *outward* exist: the interior is LEFT of travel, so the outward normal is right of
//      it. A room authored clockwise and the same room authored anticlockwise are one geometry with
//      one set of outward normals, which is what makes reversing a ring mean nothing at all.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeSpatial.h"

/**
 * One declared point, in PLAN — world X/Y, centimetres. Z is the region's, never a point's (§21.5).
 *
 * A POINT IS A COORDINATE. User-ruled 2026-08-11: there is no `@` inside a `.gcw`, so no point is
 * a fraction along an edge. A spot partway along a wall is said where it is USED —
 * `@anchor[target=e1@t=50%]` — and resolved through `FGraphCodeRegionSolver::PointOnEdge`, which is
 * why that function survived the removal of the `OnEdge` origin it was written for.
 */
struct GRAPHCODE_API FGcRegionPoint
{
	/** Minted `p1`, or the name an author renamed it to — §21.3, and it is never a position. */
	FString Name;

	/**
	 * The region that DECLARED it, and the only one that can name it (§21.1b).
	 *
	 * Not a convenience: it is what makes `p1` mean a different corner in each room, so a second room
	 * costs no prefix. Every point has one — a point outside every region cannot be declared.
	 */
	int32 Region = INDEX_NONE;

	/**
	 * THE COORDINATE IS THE WHOLE POINT — §4.3, user-ruled 2026-08-14.
	 *
	 * A promoted corner used to carry `from:"Shelf_01 collision"` beside it. It went because it goes
	 * false two ways: the actor moves, is renamed or is deleted; **or you drag the corner and the note
	 * still credits the actor**, which nothing cleared and a spec asserted. It was also the only
	 * persisted reference out of a `.gcw`, in a format whose invariant is that deleting a room cannot
	 * dangle anything. The trace still reports it — `FGcwPromotion::Receipt`, beside `traced_from`,
	 * `off_cm` and every other finding these operations report rather than write.
	 */
	FVector2D Position = FVector2D::ZeroVector;

	/**
	 * `forward:` in degrees — WHICH WAY THIS PLACE FACES, and the flag is what keeps absence sayable.
	 *
	 * **A LANDMARK USED TO HAVE NO DIRECTION OF ITS OWN, and R13 §2 recorded that as forced rather
	 * than chosen** — *neither has an outward normal of its own, so both take the declaring region's
	 * rotation.* True of the geometry and false of the intent: the altar faces the door, and a room
	 * whose `forward:` is a diagonal made every landmark in it face that diagonal. So the direction is
	 * AUTHORED here when somebody has one to state, and inherited from the region when nobody does.
	 *
	 * **`forward:`, not a new word,** because that is already this format's word for the axis a thing
	 * faces — it is `forward:` on a region and means the same degrees measured the same way
	 * ([[a-new-token-not-a-new-line]]).
	 *
	 * **STATEABLE ON ANY POINT, corner or landmark, and that is not laxity.** Whether a point is a
	 * corner depends on the EDGES DECLARED BELOW IT, so refusing the token on a corner would make one
	 * line's legality depend on a later one — and a corner with a facing is meaningful anyway (the
	 * doorway corner faces out). One rule, no ordering trap, no refusal to keep true.
	 *
	 * The flag exists so an old file round-trips byte-identically: `forward:0` is a stated north and
	 * unstated is *the region's*, which are different answers whenever the region is turned
	 * ([[optionality-can-be-owed-to-the-corpus]]).
	 */
	bool bHasForward = false;
	double ForwardDegrees = 0.0;

	/**
	 * `[round=25.0]` — THIS CORNER'S OWN SHAPE, as a radius in centimetres. Zero is SHARP. §21.6a, R19.
	 *
	 * **THE FILE KEEPS THE INTENT AND NOT THE ARITHMETIC, which is the whole of why this exists.**
	 * Every shape it makes was already sayable — two points either side of the corner and a bowed
	 * edge between them — but that spelling FREEZES the fillet: move either wall and the arc stops
	 * being tangent, and nothing in the file remembers it was ever meant to be. `25` survives the
	 * wall moving, because the tangency is recomputed every time the ring is resolved.
	 *
	 * **A NUMBER RATHER THAN AN ENUM, because it subsumes one** — `sharp` is roundness nought, and a
	 * stated default is a second spelling of silence ([[a-new-token-not-a-new-line]]). So `round=0`
	 * parses and is then written back as nothing at all, exactly as `trigger=load` is.
	 *
	 * **STATEABLE ON ANY POINT AND MEANINGFUL ON ONLY SOME, and unlike `forward:` that asymmetry is
	 * real**: a facing is meaningful on a landmark, and a roundness is not — there is nothing to cut.
	 * It is still not refused at PARSE time, because whether a point is a corner depends on the edges
	 * declared BELOW it and one line's legality must not depend on a later one. `Resolve` refuses it
	 * by name instead, the way an open loop is refused, and says which of the three it hit: a
	 * landmark nothing meets at, a bend inside one wall, or a junction where three walls do.
	 */
	double RoundCm = 0.0;

	/**
	 * `[round=max]` — round this corner by as much as the two walls beside it will allow.
	 *
	 * **IT IS A SEPARATE FLAG AND NOT A SENTINEL NUMBER**, because the two say different things and
	 * the difference has to survive the round trip. A big number means *I want 99999 and will settle*;
	 * `max` means *whatever fits*, and only the second still tracks the walls after somebody moves
	 * one. Writing a resolved radius back for a `max` corner would freeze the answer and quietly stop
	 * it following ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 *
	 * **IT BUYS NO NEW GEOMETRY, and that is why it is a value rather than a feature.**
	 * `SolveCornerFillet` already rescales an over-large radius proportionally onto the largest one
	 * the walls can afford, capped at `FilletWallShare` of either. `max` asks for that answer on
	 * purpose instead of arriving at it by overshooting, and suppresses the was-cut-back note, which
	 * reads as a complaint when being cut back is the whole request ([[refusal-reason-is-a-claim]]).
	 *
	 * Two corners at `max` therefore leave a tenth of each wall straight rather than meeting: the
	 * 45% share exists so a fillet cannot eat past a bend somebody drew, and `max` does not get to
	 * break it.
	 */
	bool bRoundMax = false;

	/**
	 * Was `[round=]` WRITTEN, whatever it said? W9, and it exists for one spelling only.
	 *
	 * **`[round=0]` IS SILENCE IN A SHARP REGION AND A CORNER IN A SMOOTH ONE** (§21.6e). The default
	 * flips with `corners=`, so the same three characters mean *the default, said out loud* on a room
	 * and *break the curve here* on a lake — and a flag that records only the NUMBER cannot tell the
	 * second from a point that said nothing.
	 *
	 * It changes no answer in a sharp region: the writer still renders a stated nought as nothing at
	 * all there, because roundness nought is what silence already says and a second spelling for one
	 * shape is what §20.6 refuses. Only a smooth region reads it.
	 */
	bool bRoundStated = false;

	/**
	 * Does this point ask for a rounded corner at all?
	 *
	 * ONE PREDICATE, because the three places that ask — the solver's early out, the per-region
	 * refusal walk and `gcw_ask` — went wrong in the same way when `max` arrived: each tested
	 * `RoundCm > 0.0` and each would have skipped a `max` corner in silence
	 * ([[a-switch-honoured-at-one-door-only]]).
	 */
	bool WantsRounding() const { return bRoundMax || RoundCm > 0.0; }

	/**
	 * `[z=250.0]` — THIS POINT'S OWN HEIGHT, in world centimetres, read only where a RUN is drawn.
	 *
	 * **A PARAMETER AND NOT A NEW GRAMMAR — user-ruled 2026-08-24**: *"if for varying height, it
	 * should just be another parameter inside the spline body isnt it? instead of one whole new
	 * grammar."* It joins `[round=]` in the bracket a point declaration already carries, so nothing
	 * about the file's shape changes and no reader has to learn a block.
	 *
	 * **IT DOES NOT MAKE A REGION 3D, AND THAT IS THE INVARIANT IT IS BUILT NOT TO BREAK.** A region
	 * is a prism with a flat base at `anchorZ:` — containment, the outline, the ring, `sealed:` and
	 * every one of the 645 `FVector2D` sites in this plugin still see a plan coordinate and are
	 * untouched. What reads this is the COURSE path alone: `$RailTrack@t=40%`, `along=$Hall.spine`,
	 * anything asking where a run goes. A rail may climb; the room it climbs out of stays a prism.
	 *
	 * **THE FLAG IS WHAT MAKES ABSENCE SAYABLE**, and absence is the corpus: no committed point
	 * states a height, and each of them must keep taking its region's `anchorZ:` rather than being
	 * quietly rewritten to a stated `0` that means something different the day the region moves
	 * ([[optionality-can-be-owed-to-the-corpus]]).
	 */
	double Z = 0.0;
	bool bHasZ = false;

	/**
	 * `[linear]` · `[custom, tangent=(in=(600, 0, 0), out=(600, 0, 200))]` — HOW A RUN BENDS HERE.
	 *
	 * **THE THIRD THING THE BRACKET CARRIES, and it is the same bracket for the same reason `[z=]` is
	 * in it** (§21.19b, user-ruled): a run's shape is *"another parameter"* and not a new grammar. The
	 * words are water's, pre-ruled at §21.19d and checked by water's own function, so the two subjects
	 * spell one concept once.
	 *
	 * **READ BY THE COURSE PATH ALONE, exactly as `[z=]` is, and that is the invariant.** A region is
	 * a prism with a flat base: containment, the outline, the ring, `bulge:`, `[round=]` and
	 * `corners=smooth` never see this field. `[round=]` is how the OUTLINE turns a corner and this is
	 * how a RUN bends through it — two different questions about one point, which is why a smooth
	 * region refuses a radius here and does not refuse a type.
	 *
	 * **WITHOUT IT A COURSE COULD ONLY EVER BE SMOOTH.** `ESplinePointType::Curve` is `CIM_CurveAuto`
	 * and `UpdateSpline` recomputes both tangents on every auto point, so a fence written
	 * `spine = course p1 -> p2 -> p3` around a right angle came out rounded and no character in the
	 * file could say otherwise ([[a-silent-no-op-is-worse-than-a-refusal]] — here the silence was in
	 * the grammar rather than in a reader).
	 *
	 * **AN INVENTED VERTEX NEVER TAKES ONE.** Between two declared points a bulged wall is tessellated
	 * into geometry the solver made up, and no author gave those a bend — the same rule `[z=]` states
	 * for heights, one field up, and the reason both are keyed on the DECLARED ordinal.
	 */
	FGcCourseCurve Curve;
};

/** One declared wall: two or more points, straight, jogged or bowed. */
struct GRAPHCODE_API FGcRegionEdge
{
	/**
	 * `north`, or the minted `e1` when nobody said one. **ONE NAME, user-ruled 2026-08-12.**
	 *
	 * The `as:` alias is gone with the pool that needed it. A declaration reads `north = p3 -> p4`, so
	 * the left side is the name: an author writes the word they mean, `e7` is minted only for a wall
	 * nobody named, and a rename is an edit rather than a second spelling every lookup must carry.
	 * `e7` is still the seventh edge ever minted in THIS REGION, and never the seventh in its ring.
	 */
	FString Name;

	/** The region that declared it. See `FGcRegionPoint::Region` — ownership is total. */
	int32 Region = INDEX_NONE;

	/**
	 * `spine = course p5 -> p6 -> p7` — A LINE THE ROOM HOLDS, not a wall of it. §21.12n, R14.
	 *
	 * **THE LINE HALF OF R13, and it is the same distinction one dimension up.** A point no wall runs
	 * through is a landmark; a RUN no wall is made of is a course. A wall bounds the region and moving
	 * it changes the shape; a course is drawn inside and moving it changes nothing about the room.
	 * Terrain's `along=$Valley.spine` is what it is for — a ridge that follows a curve somebody drew,
	 * where a region's walls happen to be the wrong line.
	 *
	 * **A KEYWORD AND NOT A DERIVATION, unlike the landmark point.** A stray POINT is inert, so R13
	 * could infer *no wall runs through it*. A stray EDGE is not: the ring is the ORDER of the
	 * declared edges, and one that does not chain refuses today with `gcw_loop_open`. Inferring which
	 * edges are the ring would turn a mistyped wall — the commonest error this format has — from a
	 * loud refusal into a silently smaller room. So it is said, in the VALUE, where this format
	 * already tells a point from a wall.
	 *
	 * It is EXCLUDED FROM `Region.Loop`, and that is the whole mechanism: every consumer that walks
	 * the ring — containment, the outline, `sealed:`, crack measurement, navigation crossings — skips
	 * courses without being taught what one is. What it shares with a wall is everything geometric,
	 * so `bulge:`, chains, `EdgePolyline` and `@t` all work on it unchanged.
	 *
	 * The four ring keys (`connects:`, `to:`, `from:`, `sealed:`) are refused on one: each states a
	 * fact about getting through a BOUNDARY, and a course is not one.
	 */
	bool bCourse = false;

	/**
	 * TWO OR MORE points, in the DECLARED direction the bulge is signed against — §21.1a's R10,
	 * absorbed into §21.1b.
	 *
	 * A WALL WITH A STEP IN IT IS ONE WALL. Three edges give `$Crypt.north` one third of the north
	 * wall and leave the whole thing unnameable, which is §21's own exit sentence failing on the
	 * commonest shape a room has. `north = p3 -> p4 -> p5` is one name, one outward normal at its
	 * midpoint by arc length, and `@t=25%` still reaches a quarter of the way along the WHOLE run.
	 *
	 * **A CHAIN IS STRAIGHT-SEGMENTED AND A CURVED WALL IS TWO POINTS** (§21.1a, and the parser
	 * refuses the combination by name). One `bulge:` describes one arc; a per-segment list would
	 * re-point every value the moment somebody inserted a point, which is the exact defect §21.3
	 * refuses for names.
	 */
	TArray<int32> Points;

	/**
	 * `tan(θ/4)` — one signed number, `0` is straight. Legal only on a two-point edge.
	 *
	 * **STORED LEFT-OF-DECLARED; WRITTEN OUTWARD.** The file says what the user ruled on 2026-08-21
	 * — *"negative as folding inwards, positive as folding outwards, 0 is straight"* — and this field
	 * says left of the run the author declared. `FGraphCodeRegionSolver::BulgeTextSigns` carries one
	 * to the other at the text boundary and nothing between the two ever sees both.
	 *
	 * **THE TWO FRAMES ARE NOT A CHOICE THAT WENT UNMADE.** Outward is a fact about a RING, and the
	 * functions that read this field — `PointOnEdge`, `EdgePolyline`, `SegmentLength`,
	 * `ClosestOnEdge` — read one wall and never its ring, on purpose, so that a named spot on a wall
	 * does not move when the room around it is re-authored. A field in the file's frame would have
	 * handed every one of them a question a wall cannot answer alone.
	 *
	 * A BULGE AND NOT A RADIUS (§21.6, ruled A5): two arcs fit any chord, and a radius under half the
	 * chord is geometrically impossible, so a radius buys a refusal that a bulge cannot need. `1` is a
	 * semicircle; above `1` is a major arc and still works.
	 */
	double Bulge = 0.0;

	/**
	 * `connects:Crypt, Cellar` · `to:Store` · `from:Hall` — WHERE YOU CAN GO THROUGH THIS WALL (§4.3,
	 * user-directed 2026-08-14). Region names, and a wall may name several in each.
	 *
	 * ON THE WALL BECAUSE THE WALL IS WHAT THE CROSSING IS A FACT ABOUT, and a `key:value` beside
	 * `bulge:` is how this format attaches a fact to something already declared. The draft it replaced
	 * put the destination on a left side — `Crypt = through(east)` — which fails the one thing `=`
	 * means here: the right side describes the left. Crypt is a room, and it is already declared.
	 *
	 * A REGION IS A VALUE AND NOT A KEY, which is the whole reason two doors into one room need no
	 * grammar: two walls both naming `connects:Corridor` are two true statements.
	 *
	 * `To`/`From` are ONE-WAY, in the declaring region's frame — the frame `bulge:` and `bows` already
	 * use. They can only ever be non-empty across a link: the sole direction bit in the Detour tile
	 * format is `DT_OFFMESH_CON_BIDIR` on `dtOffMeshConnection`, set from `ENavLinkDirection`, and
	 * polygon adjacency carries none. A name may appear in only one of the four.
	 */
	TArray<FString> Connects;
	TArray<FString> To;
	TArray<FString> From;

	/**
	 * `sealed:Altar` — THESE TWO ROOMS MEET HERE AND YOU CANNOT GET THROUGH (§4.4, user-ruled
	 * 2026-08-15). The fourth crossing key, and the only one that states an absence.
	 *
	 * IT EXISTS BECAUSE SILENCE CARRIED FIVE MEANINGS. No key at all meant: nothing is over there;
	 * something is but declares no navigation; the two rooms touch and you genuinely cannot pass; the
	 * measurement could not run; or nothing was ever built. The third is the fact the artifact exists
	 * for and it was indistinguishable from the other four.
	 *
	 * AND IT SEPARATES TWO DIFFERENT DIFFS. Sealing a doorway makes `connects:Store` vanish; deleting
	 * the Store region makes the same line vanish. With this key, sealing CHANGES a line where
	 * deleting REMOVES one — in the one file whose purpose is that a topology change shows in a diff.
	 *
	 * ITS PRECONDITION IS STRICTER THAN THE OTHER THREE, and deliberately so: a crossing is true
	 * wherever you can walk, whatever the geometry between, but *these meet and are blocked* is a
	 * claim about the plan. It is written only where the other region's outline actually shares a
	 * stretch of THIS edge (`FGraphCodeRegionSolver::SharedBoundaryCm`) and both regions declare
	 * walkable navigation. So absence now means exactly two things: they do not touch, or one is not
	 * claiming to be walkable — with `[stale]` still covering *not measured*.
	 */
	TArray<FString> Sealed;

	/** The ends, which is what everything but the geometry itself asks for. */
	int32 StartPoint() const { return Points.Num() > 0 ? Points[0] : INDEX_NONE; }
	int32 EndPoint() const { return Points.Num() > 0 ? Points.Last() : INDEX_NONE; }

	/** Straight pieces between the points. A two-point edge has one, and it may be an arc. */
	int32 SegmentCount() const { return FMath::Max(0, Points.Num() - 1); }

	bool IsChain() const { return Points.Num() > 2; }
	bool IsArc() const { return Bulge != 0.0 && Points.Num() == 2; }

	/** Move an end without disturbing the rest of the run — what `split` and `round` do to a wall. */
	void SetStartPoint(int32 PointIndex)
	{
		if (Points.IsEmpty()) { Points.Add(PointIndex); } else { Points[0] = PointIndex; }
	}
	void SetEndPoint(int32 PointIndex)
	{
		if (Points.Num() < 2) { Points.Add(PointIndex); } else { Points.Last() = PointIndex; }
	}
};

/**
 * One editor-owned navigation mesh referenced by a region.
 *
 * `Humanoid = nav(type=walkable, built=96%, shape=…, mesh=…) [stale]`
 *
 * `MeshId` uses the stable Supported Agent name rather than a spawned `RecastNavMesh-*` actor name,
 * which Unreal may regenerate.
 *
 * **A SIZE TURNS THE REFERENCE INTO A DECLARATION.** Without `radius=`/`height=` the ID must already
 * resolve to an `FNavDataConfig::Name` in the open world, and an unknown one is refused by listing
 * what the editor declares — that is the whole of what this line used to be. WITH a size, the file
 * declares the body itself and the editor is made to match it: Unreal generates a separate navmesh
 * per agent size out of the level's own geometry, so how wide a thing the ground accommodates is a
 * fact about the ground, and belongs in the file that describes it.
 */
struct GRAPHCODE_API FGcRegionNav
{
	/**
	 * The Supported Agent name(s) this line is about — ONE CARRIER, however many there are.
	 *
	 * **A WALKABLE LINE NAMES EXACTLY ONE AND A BLOCKED LINE MAY NAME SEVERAL** (`[Human, Ogre] =
	 * nav(type=blocked)`), and the asymmetry is the engine's rather than a convenience: a bounds volume
	 * carries the agent bits itself, so walkable is already per-agent and each line owns its own
	 * `built=` receipt and its own generated actor — merging two would merge the numbers. A blocker
	 * carries a `UNavArea` class instead, and one class per distinct agent set is exactly the
	 * bookkeeping this format exists to delete (`GC-NAVIGATION-DESIGN.md` §7 N2 ruling 2, user 2026-08-17).
	 *
	 * A LIST AND NOT A STRING PLUS A LIST, deliberately: two carriers of one fact is a race
	 * ([[two-carriers-of-one-fact-is-a-race]]), and a single-name line is a list of one that renders
	 * and keys exactly as it always did.
	 */
	TArray<FString> MeshIds;

	/** The name a line about one mesh is about. Empty when the line names a set. */
	const FString& SoleMeshId() const
	{
		static const FString None;
		return MeshIds.Num() == 1 ? MeshIds[0] : None;
	}

	/**
	 * The stable key a generated actor, a name claim and a trivia entry are filed under.
	 *
	 * For one name it IS that name, so every actor and every comment anchored before 2026-08-17 keeps
	 * its identity. For a set it is the names in AUTHORED order joined by `_` — authored rather than
	 * sorted, because the file is what a reader has in front of them and a renderer that re-ordered
	 * them would make the document's bytes depend on something the author cannot see.
	 */
	FString Key() const { return FString::Join(MeshIds, TEXT("_")); }

	enum class EType : uint8
	{
		Walkable,
		Blocked,
	};
	EType Type = EType::Walkable;

	/**
	 * The body this mesh is generated for, in centimetres. AUTHORED, and both or neither — a radius
	 * with no height is half a body, and Unreal keys a mesh on the pair.
	 */
	bool bHasRadius = false;
	double Radius = 0.0;
	bool bHasHeight = false;
	double Height = 0.0;

	bool DeclaresBody() const { return bHasRadius && bHasHeight; }

	bool bHasBuilt = false;
	double BuiltPercent = 0.0;
	FString Area;
	FString Shape;
	FString MeshShape;
	bool bStale = false;

	/**
	 * `[override=$NavArea_Hall_Default]` — the `.gc` entry that owns this volume's SHAPE, because a
	 * hand moved it away from what this line describes. Held without the `$`.
	 *
	 * **NOT `Shape` ABOVE, WHICH IS A DIGEST AND NOT A REFERENCE.** `Shape` is the staleness receipt
	 * for the region's own outline — a `shape=` KEY inside the parens — and the two would be one
	 * word for two jobs ([[a-digest-is-not-a-document]] is the same confusion one file over). That
	 * collision is also why the reference is a bracket modifier rather than a key: `shape=` was
	 * taken before anybody proposed one.
	 *
	 * `GC-WORLD-GENERATION-DESIGN.md` §13.4a has the rule, which is one rule for nav, PCG and water.
	 */
	FString Override;
};

/**
 * `Greens = populate { … }` DECLARED IN A REGION'S BODY — the region's own fine tuning, §14.11.
 *
 * **THE MAIN BLOCK IS THE MACRO PASS AND A REGION BLOCK IS THE FINE TUNING** (user, 2026-08-24:
 * *"you describe macro in main populate or terrain first, fine tune in regions"*), and the
 * POSITION of `$Forest.Greens` in the main block is where that fine tuning runs. That is what let
 * terrain in at all: §9.8 kept it out of region bodies because coarse-to-fine order IS its
 * semantics, and spliced at the reference the order is still one visible sequence in one place.
 *
 * **THIS HOLDS THE DECLARATION AND NOT THE LINES.** A referenced block's lines live in the
 * document's own `populate { }` or terrain stream, each carrying an `FGcwScopeMark` — one carrier,
 * so the two can never disagree ([[two-carriers-of-one-fact-is-a-race]]). What is here is the head:
 * its name, its kind, and the comments around it.
 */
struct GRAPHCODE_API FGcwScopedBlock
{
	/** `Greens`. Unique within its region, like every other declaration in a region body. */
	FString Name;

	/** `terrain { }` rather than `populate { }` — which of the document's two streams it joins. */
	bool bTerrain = false;

	/** Comments above `Greens = populate {`, and the `# …` on that line. */
	TArray<FString> Leading;
	FString Trailing;

	/** Comments above `$Forest.Greens` in the main block, and the `# …` on it. */
	TArray<FString> RefLeading;
	FString RefTrailing;

	/** A reference claimed it. False means it is INERT and the reader was warned — see `Orphan`. */
	bool bReferenced = false;

	/**
	 * WHERE `$Forest.Greens` SAT, used for ONE case only: a referenced block that spliced no lines.
	 *
	 * A block with lines needs no index — its reference renders where its marked run begins. An
	 * EMPTY one has no run to find, and its position cannot be derived from anything. It also cannot
	 * MATTER: an empty block runs nothing, so a stale index moves a no-op line and changes no
	 * outcome. That is the whole reason this is allowed to be an index into a stream the AI rewrites
	 * ([[cannot-read-back-order-you-have-not-written]] is about the case where it would matter).
	 */
	int32 RefIndex = INDEX_NONE;

	/** Which terrain program held the reference. Empty for `populate { }`, which a file has one of. */
	FString RefProgram;

	/**
	 * ITS BODY, VERBATIM, AND ONLY WHEN NOTHING REFERENCED IT.
	 *
	 * **USER-RULED 2026-08-24: an unreferenced block WARNS, it does not refuse** — *"you never
	 * reject a valid grammar. you only warn that isnt referenced"*. So its lines have to survive a
	 * write, and with the main stream as the one carrier they are not in it: nothing spliced them.
	 * They are kept here instead, and no line is ever in both places — which half holds a block is
	 * decided by whether a reference claimed it, exactly once.
	 *
	 * **VERBATIM RATHER THAN PARSED, on this parser's own precedent.** An unreferenced block is not
	 * part of the document's program — no reader consumes it — and `ForeignBlocks` already carries
	 * exactly that kind of text whole, comments and all. Normalising it would be the writer editing
	 * text nothing reads, and parsing it would refuse a whole file over a typo in a block that does
	 * nothing. The warning is what says it is inert; adding the reference is what makes it parse.
	 */
	TArray<FString> Orphan;

	/**
	 * What sat under the last line INSIDE this block's brace, when a reference claimed it.
	 *
	 * **SEPARATE FROM `Orphan` AND NOT THE SAME FIELD REUSED.** One field holding *the whole body*
	 * in one case and *the comments after the body* in the other is two facts under one name, and a
	 * reader who guesses wrong renders a block's entire contents as a trailing comment. The two are
	 * mutually exclusive by construction — `Orphan` is filled only when nothing referenced the
	 * block, this only when something did — and that is exactly why they must not share a name.
	 */
	TArray<FString> Tail;
};
/** One declared region: the points and edges it owns as a closed ring, plus how far up it reaches. */
struct GRAPHCODE_API FGcRegion
{
	FString Name;

	/**
	 * `world { }` — THE ALL REGION, and there is at most one (§21.12r, R17, user-ruled 2026-08-21).
	 *
	 * **IT EXCEPTS NOTHING.** The first shape asked for was *everything outside the regions*, and the
	 * user withdrew the exception in the same breath: this is EVERYTHING, overlapping every room
	 * rather than being cut by it. That is not a simplification of the complement — it is a different
	 * and better answer, because a complement has holes that move when a room moves, and a landmark
	 * two metres outside the crypt would leave the world the day somebody widened the crypt.
	 *
	 * **IT EXISTS BECAUSE THERE WAS NOWHERE TO PUT A LANDMARK THAT IS IN NO ROOM** (the user's own
	 * reason). `$world` has been the anchor every level has since §21.4, but it had no parts —
	 * `$world.altar` named nothing — so a point outside every region could be written as a bare
	 * coordinate and never as a name. Ownership is total in this format (§21.1b): a thing with no
	 * region is a thing with no owner, and this is the owner it was missing.
	 *
	 * **SO IT HAS NO RING, and that is the one structural difference.** Every other region IS a closed
	 * ring of edges; the world is unbounded and a boundary would make it finite — so a WALL is refused
	 * inside it and `Loop` stays empty. It still holds POINTS (landmarks) and COURSES, which is the
	 * whole of what it is for. `FGraphCodeRegionSolver::Resolve` answers it with an empty polygon and
	 * `bUnbounded`, and containment against it is true everywhere.
	 *
	 * **AND THAT MAKES `gcw_not_in_region` VACUOUS HERE RATHER THAN SKIPPED** — §21.12p's rule is *a
	 * region may not be spoken of by a point it does not contain*, and the world contains every point.
	 * The check runs unchanged and passes, which is why no exemption is written into the writer.
	 */
	bool bIsWorld = false;

	/**
	 * `corners=smooth` — A NATURAL SHAPE. W9, §21.6e, user-directed 2026-08-23.
	 *
	 * **ONE GRAMMAR WITH THE DEFAULT FLIPPED, not a second kind of region.** `corners=sharp` is the
	 * default and is today's behaviour exactly, so the corpus does not move. `corners=smooth` says the
	 * points are a curve rather than corners, and `[round=0]` on one of them breaks it back.
	 *
	 * **STATED HERE AND NEVER INFERRED FROM THE ABSENCE OF WALLS.** The first draft read *no edges
	 * declared -> natural*, which makes a point's meaning depend on lines BELOW it: adding one named
	 * wall to a lake would silently square off every other corner. `round=` is already careful about
	 * exactly that, and this is the same care one level up.
	 *
	 * **SO A SMOOTH REGION DECLARES POINTS AND NO WALLS, and the wall is refused by name.** That is
	 * what keeps the one-wall invariant intact rather than merely undisturbed: `PointOnEdge`,
	 * `ClosestOnEdge`, `SegmentLength` and `EdgePolyline` read ONE WALL AND NEVER ITS RING, on purpose,
	 * so that a named spot does not move when the room is re-authored — and a smooth node's shape is
	 * its NEIGHBOURS'. A natural shape names no wall to anchor to, so nothing asks them the question.
	 *
	 * **A COURSE IS STILL ALLOWED IN ONE**, and that is the same distinction §21.12n already draws: a
	 * course is not in `Loop`, bounds nothing and deforms nothing, so a stream drawn across a lake is
	 * a line the region HOLDS rather than a wall it is made of.
	 *
	 * **THE CROSSING KEYS FOLLOW RATHER THAN BEING REFUSED SEPARATELY.** `connects:`, `to:`, `from:`
	 * and `sealed:` live on a wall; a region with no walls states none, and there is no second rule to
	 * keep true.
	 */
	bool bSmoothCorners = false;

	/**
	 * Its own edges, in the order it declared them — **and that order IS the ring** (§21.1b).
	 *
	 * **EMPTY ON A SMOOTH REGION, and that is the one structural difference** (§21.6e). Its ring is its
	 * own POINTS in declared order, so there are no edges to hold — the same shape of exception the
	 * world already is, and `Resolve` answers both before the loop check rather than exempting them
	 * inside it.
	 *
	 * User's words: *"the order of edges itself is a loop"*. So there is no `loop:` line to keep in
	 * step with the declarations, and no way to write a region whose membership and whose ring
	 * disagree. Direction is still derived, never authored (§21.6).
	 */
	TArray<int32> Loop;

	/**
	 * How many points and edges this REGION has ever minted, so a deleted name is never handed out
	 * again — `GraphCodeRegionFile.h`'s note 2, one scope down.
	 *
	 * PER REGION AND NOT PER FILE, which is the whole of §21.1b at the counter: each room mints its
	 * own `p1`, and adding a second room may not renumber the first one's corners.
	 */
	int32 MintedPoints = 0;
	int32 MintedEdges = 0;

	/**
	 * `min:` and `max:` — absolute world Z, and **EITHER MAY BE UNBOUNDED** (§21.18c, user-ruled
	 * 2026-08-12). They bound CONTAINMENT and nothing else.
	 *
	 * THE PAIR REPLACES `Base` + `bHasHeight`, and it is not a rename. That pair made one absence mean
	 * two opposite things — `ContainsPoint` read no-height as *infinitely tall* and `Bounds` read it as
	 * *flat* (§21.18a, measured) — because `Base` was simultaneously the floor of the prism and the Z
	 * that sampling used. Here each side is its own claim: unstated is unbounded on that side, stated
	 * is a plane, and the region's own Z lives on the anchor below where nothing can confuse it for a
	 * bound.
	 *
	 * §21.5's A2 is unchanged and is now said on both sides: a region with neither is a COLUMN and Z is
	 * not tested at all, which is what makes *one room, one floor, nothing above it* need no numbers.
	 */
	bool bHasMin = false;
	double Min = 0.0;
	bool bHasMax = false;
	double Max = 0.0;

	/**
	 * `anchor:` — WHERE THE REGION IS, in plan. Unstated is the centre of the SHAPE (§21.18c).
	 *
	 * *"one anchor… when taking render for the region, it uses the anchor point"*. A region's position
	 * was the centre of its own bounding box until this shipped, and §21.9 allows concave regions — so
	 * the centre of the box around an L-shaped room lands in the notch, the one part of the rectangle
	 * the room deliberately does not occupy, and four shipped readers pointed at it.
	 *
	 * TWO PARAMETERS AND NOT ONE POINT, user-ruled: *"z is specified in itself and xy is also another
	 * parameter"*. Stating a height for a room whose plan centre is already right is the common case,
	 * and a single `(x, y, z)` would make it cost two numbers nobody wanted to choose.
	 */
	bool bHasAnchorPlan = false;
	FVector2D AnchorPlan = FVector2D::ZeroVector;

	/**
	 * `anchorZ:` — the anchor's height. **Defaults to 0 EVEN WHEN `min:` IS STATED**, user-ruled.
	 *
	 * The tempting ladder — *fall back to `min:`, it is the floor* — is refused deliberately: it is the
	 * same inference that put two meanings in one absence and cost §21.18a. One key, one default, and a
	 * room whose floor is at 300 writes `anchorZ:300`.
	 *
	 * `min:` CANNOT SERVE HERE even when it is stated, which is the whole reason this field exists:
	 * *"not the min (which might be unbounded), and definitely not always default to 0"*.
	 */
	double AnchorZ = 0.0;

	/**
	 * `from: FloorSlab_01` — PROMOTION, and it is APPLY-TIME ONLY (§21.8's C3, taken 2026-08-11).
	 *
	 * A payload may name one placed actor instead of four corners; the write resolves its COLLISION
	 * footprint in plan into real points and edges, each carrying a receipt, and clears this. **So a
	 * committed `.gcw` never contains `from:`** — the renderer has no branch for it, which is what
	 * makes that structural rather than a promise.
	 *
	 * **SEVERAL ACTORS ARE ONE BOUNDARY — `from: Wall_N, Wall_E, Wall_S, Wall_W`** (§21.8a's third
	 * ruling, taken as the UNION HULL). Their footprints are gathered together and hulled ONCE, so
	 * four walls trace the room they stand around rather than four separate rooms. It is convex and it
	 * includes the thickness of what it traced; the promotion says so in a note rather than leaving a
	 * reader to discover it, and every corner is then editable. The enclosed-interior reading — the
	 * floor you can actually stand on, which can be concave — is a different feature and needs an
	 * inside to be stated first (§21.8a).
	 *
	 * THE ACTORS ARE A STENCIL AND NOT THE REGION. C2 already ruled that a derived point is resolved
	 * once and frozen: a live link would be §18.7's dependency cycle — walls depend on the region and
	 * the region depends on the walls — and would make nudging a floor slab silently move a room.
	 * Re-running the promotion after the slab moves is one deliberate call.
	 *
	 * A region carrying this declares no points and no edges, and only then, because the ring is what
	 * it produces.
	 *
	 * **A SOURCE IS AN ACTOR WHEN IT IS BARE AND A PLACE IN THIS FILE WHEN IT IS MARKED** — R15,
	 * §21.8b. `from: $Village.altar` takes a landmark, `$Village.east` a wall, `$Village` the region
	 * itself; that is §18.3c's rule unchanged rather than a new one, and a marked source needs no
	 * loaded world because it is arithmetic over this document.
	 */
	TArray<FString> From;

	/**
	 * `radius: 30000` — HOW FAR OUT FROM WHAT `from:` NAMED, in centimetres. R15, §21.8b.
	 *
	 * Zero is what shipped: the footprint itself. Anything greater pushes the traced hull outward by
	 * that distance, straight along each edge and around each corner on a real ARC — so a disc around
	 * one landmark is two bowed edges and not a 32-gon somebody has to live with.
	 *
	 * **APPLY-TIME ONLY, exactly as `from:` is**, and cleared with it. A committed `.gcw` carries the
	 * ring, never the instruction that produced it — which is why a repeat of the derivation is a
	 * deliberate call and why the memory of *why* this region is here belongs in a comment
	 * (user-ruled: *"if the ai wants to remember about this region, it can read comments"*).
	 *
	 * **A DEGENERATE HULL STOPS BEING A REFUSAL WHEN THIS IS SET.** One source point is a spot and two
	 * are a segment, and both are refused for want of area — but a spot pushed out is a disc and a
	 * segment is a stadium, so the area test has to run AFTER the offset rather than before it.
	 */
	double Radius = 0.0;

	/**
	 * `forward:` in degrees — the region's own axis, defaulting to world +X (§21.7, ruled B6).
	 *
	 * NOT THE FIRST EDGE, and not the centroid. The first edge changes when the loop is re-authored,
	 * and then every percentage in every `.gc` silently means something else. A centroid cannot pick a
	 * side on a concave room, which this project has already measured once.
	 */
	double ForwardDegrees = 0.0;

	/** Navigation mesh references keyed by editor-declared Supported Agent / mesh ID. */
	TArray<FGcRegionNav> Navigation;

	/**
	 * `Greens = populate { … }` and `Lake = terrain { … }` declared in this region's body — §14.11.
	 *
	 * **AND THIS IS NOT §12a.3 COMING BACK, which is the objection to answer first.** That ruling
	 * retired `Trees = pcg(…)` as a BARE TOKEN on a region body, where the region WAS the domain and
	 * nothing could say when it ran; that form is still refused by name (`gcw_old_format`) and
	 * nothing here reaches it. A scoped block is NAMED, its lines keep the whole domain vocabulary,
	 * and the main block's reference decides the order — which is precisely what §12a.3 was made of.
	 *
	 * **WHAT IS HERE IS THE DECLARATION.** The lines of a referenced block are in the document's own
	 * streams under `FGcwScopeMark`; only an UNREFERENCED block's body sits here, verbatim.
	 */
	TArray<FGcwScopedBlock> ScopedBlocks;

	// **WHAT FILLS THIS REGION IS NOT HERE ANY MORE** — `FGcRegionGeneration` and its list left with
	// §12a.3 (user-ruled 2026-08-19). A generator is an operation with a DOMAIN, not a property of a
	// region: written here the region WAS the domain, and one region — or one region minus others —
	// is all it could ever say. It is `FGcPopulateBlock` on the document now, and `nav(…)` stayed
	// because it really is a property (this region is walkable), which is the distinction §9.8 drew
	// before either was built.
};

/** Points, edges and regions — one `.gcw`'s geometry, with nothing about the level in it. */
struct GRAPHCODE_API FGcRegionSpace
{
	TArray<FGcRegionPoint> Points;
	TArray<FGcRegionEdge> Edges;
	TArray<FGcRegion> Regions;

	/**
	 * By name, INSIDE ONE REGION — §21.1b, and there is deliberately no unscoped spelling.
	 *
	 * The flat scan these replaced is the defect §21.1b was ruled on: it made an alias unique per FILE,
	 * so the second room to call a wall `north` was refused, and `$Hall.north` found `north` globally
	 * and then asserted membership. A lookup that cannot be asked without saying which room is a
	 * lookup that cannot answer with another room's wall.
	 */
	int32 FindPoint(int32 RegionIndex, const FString& Name) const;

	/** Edges have exactly one name since the `as:` alias went, so this is one comparison. */
	int32 FindEdge(int32 RegionIndex, const FString& Name) const;

	int32 FindRegion(const FString& Name) const;

	/** Which region declared this point or edge, for a caller holding an index and no context. */
	int32 RegionOfPoint(int32 PointIndex) const;
	int32 RegionOfEdge(int32 EdgeIndex) const;

	/** `Hall.north` — a name for a person to read, never one this file parses. */
	FString QualifiedEdgeName(int32 EdgeIndex) const;
	FString QualifiedPointName(int32 PointIndex) const;
};

/**
 * One SEGMENT of one edge as a ring walks it. `bReversed` negates the bulge; the shape is identical.
 *
 * A SEGMENT AND NOT A WHOLE EDGE, since §21.1b made an edge a chain of points. A two-point wall is one
 * step exactly as it always was, and a jogged wall is several — which is what lets every area,
 * containment, bounds and tessellation answer below stay the code that was proved against arcs.
 * `Segment` is last so those answers' two-field construction still means what it says.
 */
struct GRAPHCODE_API FGcRegionStep
{
	int32 Edge = INDEX_NONE;
	bool bReversed = false;

	/** Which piece of a chain, in the edge's own declared order. Zero for an ordinary wall. */
	int32 Segment = 0;

	/**
	 * A ROUNDED CORNER'S ARC — the one step with no wall behind it. `Edge` is `INDEX_NONE` here, and
	 * this is the point whose `[round=]` cut it.
	 *
	 * **THE STEP CARRIES A POINT INDEX AND NOT GEOMETRY**, so it stays a value anybody can copy and
	 * `ArcOf` remains answerable from the space alone. Carrying the arc itself would make a resolved
	 * ring a second statement of a shape the corner already determines
	 * ([[two-carriers-of-one-fact-is-a-race]]) — and every one of these is recomputed from the walls
	 * each time the ring resolves, which is the whole point of declaring the radius rather than the
	 * arc.
	 *
	 * `bReversed` still applies: a ring normalised counter-clockwise flips its fillets with it.
	 */
	int32 FilletPoint = INDEX_NONE;

	bool IsFillet() const { return FilletPoint != INDEX_NONE; }

	/**
	 * **HALF OF ONE SPAN OF A NATURAL SHAPE** — W9, §21.6e. The point the span LEAVES; `Segment` is
	 * `0` for the first arc of the pair and `1` for the second.
	 *
	 * **A SPAN IS TWO ARCS AND NOT ONE, and that is forced rather than chosen.** A circular arc from a
	 * point matches the tangent at ONE of its ends; a curve that is smooth at both needs two, meeting
	 * where their tangents agree. That is a biarc, and it is the whole reason this field exists.
	 *
	 * **AND THEY ARE CIRCULAR ARCS BECAUSE EVERYTHING BELOW ALREADY IS.** Area, containment, the
	 * boundary test, tessellation, bounds and every frame read `FGcRegionArc`; a cubic would have been
	 * a second geometry in all of them, each with its own version of *is this point on the curve*. A
	 * biarc chain is tangent-continuous in the representation this file already proves against, so a
	 * lake and a room are measured by the same code.
	 *
	 * **NO GEOMETRY IS CARRIED HERE, exactly as a fillet carries none.** The pair is recomputed from
	 * the ring each time, which is what lets moving one point re-curve its neighbours
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	int32 SmoothPoint = INDEX_NONE;

	bool IsSmooth() const { return SmoothPoint != INDEX_NONE; }

	/** Neither a wall nor a piece of one — a fillet or a smooth span, both of which have no edge. */
	bool HasEdge() const { return Edge != INDEX_NONE; }
};

/**
 * A region with its loop chained, closed and normalised — build once, ask many times.
 *
 * EVERY QUERY BELOW TAKES ONE OF THESE RATHER THAN A REGION INDEX, so no caller can ask a containment
 * question against an unnormalised walk and get an inside-out answer.
 */
struct GRAPHCODE_API FGcResolvedRegion
{
	int32 RegionIndex = INDEX_NONE;

	/** Counter-clockwise, always. The interior is left of travel and the outward normal is right of it. */
	TArray<FGcRegionStep> Steps;

	/** Square centimetres, positive. Arc segments included — a bowed wall changes the area. */
	double Area = 0.0;

	/**
	 * The authored order ran clockwise and was reversed. Recorded, never refused.
	 *
	 * §21.6's ruling in one field: reversing a loop must mean NOTHING, so this exists to be inspected
	 * and not to change an answer. A spec that authors one loop both ways round asserts on the
	 * geometry being identical while this flag differs.
	 */
	bool bWindingWasReversed = false;

	/**
	 * THE WORLD REGION RESOLVED — no polygon, no area, and containment true everywhere (§21.12r).
	 *
	 * `Steps` is empty and `Area` is zero, which for any other region is the degenerate shape
	 * `Resolve` refuses by name. This flag is what tells the two apart, so a caller that walks `Steps`
	 * and finds none knows whether it is looking at the unbounded region or at a bug.
	 */
	bool bUnbounded = false;

	/**
	 * WHAT THE SOLVER HAD TO CHANGE TO MAKE THE DOCUMENT BUILDABLE — today, only a clamped `[round=]`.
	 *
	 * **A DECLARED RADIUS THAT DOES NOT FIT IS CLAMPED RATHER THAN REFUSED**, and this is where the
	 * author is told. A fillet may eat at most 45% of either wall it touches — one rule, so two
	 * rounded ends of one short wall can never reach each other and the answer does not depend on
	 * which corner was solved first. Refusing instead would make a file that PARSES fail to resolve
	 * over a number with an obvious best answer, and silently shrinking it is the wrong half of
	 * [[a-silent-no-op-is-worse-than-a-refusal]].
	 *
	 * Empty on almost every region. `gcw_ask` prints these beside the cracks.
	 */
	TArray<FString> Adjustments;
};

/** One traversal edge as circular geometry. `bIsArc` false leaves everything but the endpoints unset. */
struct GRAPHCODE_API FGcRegionArc
{
	bool bIsArc = false;

	FVector2D Start = FVector2D::ZeroVector;
	FVector2D End = FVector2D::ZeroVector;

	/** Signed for the TRAVERSAL — the edge's own bulge, negated when the loop walks it backwards. */
	double Bulge = 0.0;

	FVector2D Centre = FVector2D::ZeroVector;
	double Radius = 0.0;

	/** Radians. `Sweep` is signed and is `-4·atan(Bulge)`: a left-bowing arc turns clockwise. */
	double StartAngle = 0.0;
	double Sweep = 0.0;
};

/** A plan triangle from the area-weighted sampler's triangulation. */
struct GRAPHCODE_API FGcRegionTriangle
{
	FVector2D A = FVector2D::ZeroVector;
	FVector2D B = FVector2D::ZeroVector;
	FVector2D C = FVector2D::ZeroVector;

	double Area() const;
};

/** A frame on an edge — §21.7's *along the edge, outward normal, world up*, and it has no volume. */
struct GRAPHCODE_API FGcRegionEdgeFrame
{
	/**
	 * The point on the ARC at the parameter asked for, at the region's ANCHOR Z.
	 *
	 * THE ANCHOR AND NOT MID-HEIGHT, deliberately. A wall's midpoint would be halfway between `min:`
	 * and `max:`, which reads well until either side is unbounded (§21.18c) and there is no such
	 * height — and a Z that depends on whether somebody wrote a bound is a trap, not a convenience.
	 * The anchor is where a door sits and `up=` is already how you leave it.
	 */
	FVector Location = FVector::ZeroVector;

	/** Unit, horizontal, AWAY FROM THE INTERIOR of the region that named the edge. */
	FVector Outward = FVector::ZeroVector;

	/** Unit, horizontal, the direction of travel at that point. */
	FVector Along = FVector::ZeroVector;

	/** X outward, Z world up — so `@facing[relative=$Crypt.north]` faces the way that wall faces. */
	FTransform Transform() const;
};

/**
 * The region half of the spatial surface. Pure, static, and asserted with no editor.
 */
class GRAPHCODE_API FGraphCodeRegionSolver
{
public:
	/**
	 * How many straight pieces one arc becomes when the sampler triangulates — FIXED, not adaptive.
	 *
	 * A count derived from the radius or a chord tolerance would make the triangulation, and therefore
	 * which triangle a seed picks, depend on a number an engine upgrade could round differently. §8's
	 * second-pass-zero rule needs one document to produce one set of transforms forever, so this is a
	 * constant and the accuracy cost is paid by `TessellatedOutline`'s conservatism instead.
	 *
	 * WHAT 32 BUYS, measured rather than guessed: the sampler stays clear of a bowed wall by
	 * `R·(1/cos(θ/32) − 1)`. On the worst case a bulge can state — a semicircle — that is 2.4 cm on a
	 * 5 m radius; on the ordinary gently bowed wall (`bulge:0.2` across 12 m) it is under half a
	 * centimetre. The cost is linear in vertices and quadratic in the ear clipping, which is thousands
	 * of operations for a room.
	 */
	static constexpr int32 ArcSegments = 32;

	/**
	 * A boundary is a physical place, so it gets a physical tolerance — a tenth of a millimetre.
	 *
	 * Z0 compares a box exactly because a box test is three absolute values; a point on a slanted
	 * segment never lands exactly on it in floating point, and *the wall is in the room it bounds*
	 * needs the boundary to answer inside rather than to answer whichever way the last bit fell.
	 */
	static constexpr double BoundaryToleranceCm = 0.01;

	/**
	 * Chain the ring by shared point, close it, and normalise the winding to counter-clockwise.
	 *
	 * ONE STEP PER SEGMENT, so a chain edge contributes several and a walk that runs it backwards
	 * takes its segments in reverse. Everything downstream reads steps and therefore never learns
	 * that a wall can have a step in it.
	 *
	 * THE FIRST EDGE TAKES ITS DIRECTION FROM WHAT THE SECOND ONE TOUCHES, which is the only reading
	 * that makes §21.6 true: walking it forward unconditionally accepts `e1, e2, e3, e4` and refuses
	 * `e4, e3, e2, e1` — the same loop written backwards, which the ruling says must mean nothing at
	 * all. Where both of its ends match the next edge (two arcs over one chord, a circle), either
	 * choice is the same shape read the other way and the winding normalisation settles it.
	 *
	 * Refuses `region_loop_open` (a gap), `region_loop_short` (fewer than two edges),
	 * `region_degenerate` (no area) and `region_unknown_edge` / `region_unknown_point`.
	 */
	static bool Resolve(const FGcRegionSpace& Space, int32 RegionIndex, FGcResolvedRegion& OutResolved,
		FString& OutRefusal);

	/** One traversal step as circular geometry, with the bulge already negated if the walk reversed. */
	static FGcRegionArc ArcOf(const FGcRegionSpace& Space, const FGcRegionStep& Step);

	/**
	 * The points a NATURAL SHAPE curves through, in declared order — empty for a region with walls.
	 *
	 * **PUBLIC BECAUSE TWO QUESTIONS NEED THE SAME ANSWER** (W9, §21.6e). *Is this point a corner or a
	 * landmark* is a ring-membership test, and every caller that asks it of a walled region asks it of
	 * `Loop`. A smooth region has no `Loop`, so a caller left to work it out would decide that every
	 * shore point is a landmark — and two callers working it out separately would eventually disagree
	 * about the shape ([[a-read-answers-through-the-write-path]]).
	 *
	 * A COURSE'S POINTS ARE NOT AMONG THEM: in a natural shape a point is either on the curve or on a
	 * course, never both, which is what lets a stream be drawn across a lake without deforming it.
	 */
	static TArray<int32> CurvePoints(const FGcRegionSpace& Space, int32 RegionIndex);

	/**
	 * How far one step runs, in centimetres.
	 *
	 * **BESIDE `EdgeLength` AND NOT INSTEAD OF IT**, because since R19 a ring holds steps an edge
	 * cannot answer for: a fillet arc has no wall, and a wall beside one is walked trimmed. A caller
	 * measuring the ring adds these; a caller asking how long a WALL is still asks `EdgeLength`.
	 */
	static double ArcLengthOf(const FGcRegionSpace& Space, const FGcRegionStep& Step);

	/**
	 * Per edge, the `+1`/`-1` that carries a bulge between THE FILE'S SIGN and THE MODEL'S.
	 *
	 * **THE FILE SAYS OUTWARD AND THE MODEL SAYS LEFT-OF-DECLARED** (user-ruled 2026-08-21: *"negative
	 * as folding inwards, positive as folding outwards, 0 is straight"*). Those are two frames for one
	 * arc, and this is the whole of the difference between them.
	 *
	 * **THE FILE'S FRAME COULD NOT BE THE MODEL'S, and that is measured rather than preferred.**
	 * `PointOnEdge`, `EdgePolyline`, `SegmentLength` and `ClosestOnEdge` all read ONE WALL AND NEVER
	 * ITS RING — deliberately, so that a named spot on a wall does not move when somebody re-authors
	 * the room around it. Outward is a fact about the ring. Carrying the file's sign inward would
	 * therefore have handed every one of those functions a question it cannot ask, so the carry
	 * happens once at the text boundary and the geometry below it never learns there were two frames.
	 *
	 * **IT IS ITS OWN INVERSE.** A sign is `+1` or `-1`, so parse multiplies and render multiplies by
	 * the same number — one rule, and no second spelling that could drift from the first
	 * ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 *
	 * **THE RING'S DIRECTION IS DECIDED BY ITS CORNERS, NEVER BY HOW ITS WALLS BOW** — forced, not
	 * chosen. Working out which side is outward cannot depend on a number whose meaning is the thing
	 * being established, so the walk is normalised over the chord polygon with every bulge stripped.
	 * A ring the chords alone cannot orient — two arcs over one chord, which is a circle — answers
	 * `-1` for both, so two walls bowing `+` bow away from each other and the circle is the one an
	 * author drawing two outward semicircles meant.
	 *
	 * A COURSE BOUNDS NOTHING, so it has no outward, and its own run is the only travel it has. One
	 * rule still covers both — *positive bows right of travel* — because a wall's travel is its ring's
	 * and a course's is what the author declared.
	 */
	static void BulgeTextSigns(const FGcRegionSpace& Space, TArray<double>& OutSigns);

	/** Every step of one edge in `Region`'s walk, in walk order. A chain answers with several. */
	static TArray<int32> StepsForEdge(const FGcResolvedRegion& Region, int32 EdgeIndex);

	/** Where the walk starts and ends, honouring `bReversed`. */
	static FVector2D StepStart(const FGcRegionSpace& Space, const FGcRegionStep& Step);
	static FVector2D StepEnd(const FGcRegionSpace& Space, const FGcRegionStep& Step);

	/**
	 * `0..1` along an EDGE in its own declared direction — what `target=$Hall.north@t=50%` means.
	 *
	 * By arc length over the WHOLE edge, which for a circular arc is by swept angle and for a chain
	 * runs across every one of its pieces. So `0.5` is the apex of a bow, the midpoint of a straight
	 * wall, and the halfway mark of a jogged one, with one expression — §21.1a's *the frame at the
	 * edge's midpoint by arc length* is this function at `0.5`.
	 *
	 * Traversal-independent by construction: it reads the edge and never a ring, because a named spot
	 * on a wall may not move when somebody re-authors the room around it.
	 */
	static FVector2D PointOnEdge(const FGcRegionSpace& Space, int32 EdgeIndex, double T);

	/**
	 * The inverse: where along an edge a plan point lies, and how far off it is.
	 *
	 * `OutT` is `PointOnEdge`'s own parameter, clamped to the edge — so a point past either end
	 * answers `0` or `1` and a distance measured to that end, rather than an extrapolation. `OutOffCm`
	 * is centimetres from the edge, and it is A DISTANCE AND NOT A VERDICT (§21.14a's ruling one
	 * operation over): *is this point on this wall* is the caller's question, and a number is what
	 * lets them weigh a 0.1 mm rounding error differently from a 5 cm mistake somebody typed.
	 *
	 * Straight edges project onto the segment; a bowed one answers by ANGLE about its own centre,
	 * which is what makes the round trip `PointOnEdge(ClosestOnEdge(P)) == P` hold for both. A chain
	 * asks every piece and keeps the nearest, so the same round trip holds across a jog.
	 */
	static bool ClosestOnEdge(const FGcRegionSpace& Space, int32 EdgeIndex, const FVector2D& Point,
		double& OutT, double& OutOffCm);

	/** Centimetres: the arc, the chord, or every piece of a chain added up. */
	static double EdgeLength(const FGcRegionSpace& Space, int32 EdgeIndex);

	/** One piece of a chain, `0` to `SegmentCount() - 1`, in the edge's declared direction. */
	static double SegmentLength(const FGcRegionSpace& Space, int32 EdgeIndex, int32 Segment);

	/**
	 * WHERE A DECLARED POINT SITS ALONG ITS OWN EDGE, as the `0..1` a `@` takes.
	 *
	 * **THE ONE THING AN AUTHOR CANNOT WORK OUT FOR THEMSELVES, which is why it is a function rather
	 * than arithmetic at each call site.** `north = p3 -> p4 -> p5` is one wall, and `@t` runs by ARC
	 * LENGTH over the whole of it — so the `t` of the bend at `p4` depends on how long the two pieces
	 * happen to be, and moving `p5` changes it. Nobody can write that number down and have it stay
	 * true, so a named bend was unreachable and the alternative was hand arithmetic that goes stale
	 * in silence.
	 *
	 * `Ordinal` indexes the edge's OWN `Points` array, 0 at the start. `INDEX_NONE` for an edge or
	 * ordinal that does not exist, or an edge with no length — a caller refuses on that rather than
	 * placing something at zero.
	 */
	static double PointT(const FGcRegionSpace& Space, int32 EdgeIndex, int32 Ordinal);

	/** Positive counter-clockwise, arc segments included. `Resolve` normalises against this. */
	static double SignedArea(const FGcRegionSpace& Space, const TArray<FGcRegionStep>& Steps);

	/**
	 * The convex hull of plan points, counter-clockwise.
	 *
	 * Andrew's monotone chain — sort, then two passes — deterministic by construction, with no
	 * tolerance to tune beyond the one the caller states.
	 *
	 * A DEGENERATE ANSWER IS RETURNED RATHER THAN EMPTIED: collinear points hull to a segment and
	 * coincident ones to a spot. A caller that needs area checks for three corners; the placement
	 * shadow deliberately does not, because an actor whose bounds are a point still rests on things.
	 *
	 * `ToleranceCm2` IS THE TURN, in cm², and it is why one hull serves two callers. At zero only
	 * exactly-collinear points are dropped, which is what clipping a box wants. A small positive
	 * number also drops a vertex that is really a point on the edge between its neighbours, which is
	 * what an AUTHORED loop wants: a box's eight projected corners have to become four movable
	 * corners rather than four pairs nobody can move independently.
	 */
	static TArray<FVector2D> ConvexHull(TArray<FVector2D> Points, double ToleranceCm2 = 0.0);

	/**
	 * In plan, ignoring Z entirely. The boundary counts as inside — Z0's rule, one shape over.
	 *
	 * Exact against the arcs and not against a tessellation: the chord polygon decides by crossing
	 * number, then each bowed edge's circular segment FLIPS the answer for points inside it. An arc
	 * bowing out adds that segment and an arc bowing in removes it, so one correction serves both.
	 *
	 * The boundary and the CHORDS of bowed edges are answered first, on their own terms, because a
	 * crossing count has nothing to say about a point sitting exactly on the curve it is counting —
	 * and a circle authored as two arcs over one chord is a shape where two corrections cancel and
	 * leave the centre of the room reading outside it.
	 */
	static bool ContainsPlanPoint(const FGcRegionSpace& Space, const FGcResolvedRegion& Region,
		const FVector2D& Point);

	/**
	 * How far a plan point lies from a RUN, and — when asked — how far ALONG it the nearest point
	 * sits, 0 to 1.
	 *
	 * **BY ARC LENGTH AND NOT BY SEGMENT INDEX**, which is the whole difficulty: a course written as
	 * three points with a short hop and a long haul would otherwise put its half-way mark at the end
	 * of the hop, so a profile's `0.5=` would land somewhere different for two courses drawn through
	 * the same ground with a different number of points.
	 *
	 * **PUBLIC BECAUSE TWO PRODUCTS ASK IT OF THE SAME COURSE.** It was a file-static in terrain's
	 * evaluator until §12a.6g's rung 3: `along=$Stream width=600` masks a heightfield in Core and
	 * bounds a scatter's membership in World, and those are *the same strip* — two copies of this
	 * arithmetic would shape terrain over one and plant reeds over a very slightly different one,
	 * which is a disagreement no round trip can see ([[a-read-answers-through-the-write-path]]).
	 */
	static double DistanceToRun(const FVector2D& Point, const TArray<FVector2D>& Run,
		double* OutAlong = nullptr);

	/**
	 * The prism: plan containment, and **each Z side only when that side was stated** (§21.18c).
	 *
	 * Four answers from one expression — neither side tests Z at all, `min:` alone tests a floor,
	 * `max:` alone tests a ceiling, and both make a slab. A column is still the no-numbers case §21.5
	 * ruled; it is now the case where both sides are absent rather than the case where one field is.
	 */
	static bool ContainsPoint(const FGcRegionSpace& Space, const FGcResolvedRegion& Region,
		const FVector& Point);

	/**
	 * WHERE THE REGION IS — the authored anchor, or the centre of the shape when nobody wrote one.
	 *
	 * THE DEFAULT IS THE CENTRE OF THE SHAPE AND NOT THE CENTRE OF ITS BOX, which for a rectangle is
	 * the same point and for the concave rooms §21.9 allows is not: the plan bounds centre when it
	 * lies inside the polygon, and otherwise the centroid of the largest triangle of this region's own
	 * triangulation — deterministic, and inside by construction. So the notch of an L-shaped room
	 * stops being *where the region is* even for a file that never authored an anchor.
	 *
	 * Z is `AnchorZ` unconditionally. `Min` is not consulted, by ruling and not by omission.
	 */
	static FVector Anchor(const FGcRegionSpace& Space, const FGcResolvedRegion& Region);

	/**
	 * Rotation from `forward:`, **location at the ANCHOR** (§21.18c).
	 *
	 * IT IS NO LONGER `Bounds().Frame`, and that coupling was the defect: a bounding box's centre is
	 * the right origin for a percentage and the wrong answer to *where is the crypt*. The box still
	 * answers the first — `up=50%` and `forward=-100%` read `Bounds`, and they must, because a
	 * percentage is defined as reaching the far face and one measured from an off-centre anchor
	 * overshoots on one side. Two functions, two questions, and neither borrowed from the other.
	 */
	static FTransform RegionFrame(const FGcRegionSpace& Space, const FGcResolvedRegion& Region);

	/**
	 * The prism's oriented bounding box, in the region's own `forward:` frame — what a PERCENTAGE
	 * measures against (§21.7's *percentages of the prism*).
	 *
	 * EXACT ACROSS ARCS, not the tessellation's bounds: an inscribed polyline under-covers a bowed
	 * wall, and `forward=-100%` under-covering means *the far corner of the room* stops short of the
	 * room. Each arc contributes its endpoints plus whichever cardinal extremes of its circle the sweep
	 * actually crosses.
	 *
	 * AN UNBOUNDED SIDE GETS A ZERO Z HALF-EXTENT AT THE ANCHOR'S Z, because a prism missing an end has
	 * no half-height and inventing one would let `up=50%` answer with a number nobody wrote (§21.13e's
	 * *volume nobody authored*, one shape over). It is flat at `anchorZ` rather than at the old `base:`,
	 * so the floor plan a column photographs sits at the height the author put the region at.
	 */
	static FGcZoneVolume Bounds(const FGcRegionSpace& Space, const FGcResolvedRegion& Region);

	/** The frame at `T` along the `StepIndex`-th SEGMENT of the walk. `T` is in TRAVERSAL direction. */
	static bool EdgeFrameAt(const FGcRegionSpace& Space, const FGcResolvedRegion& Region, int32 StepIndex,
		double T, FGcRegionEdgeFrame& OutFrame);

	/**
	 * The frame at `T` along a whole EDGE, `T` in the edge's own declared direction.
	 *
	 * THE CONVERSION LIVES HERE AND NOT IN THE CALLER, which is the only place it cannot be forgotten.
	 * A ring walks a wall whichever way its authored order and the counter-clockwise normalisation
	 * happen to land, so a caller handing an authored `0.25` to the segment form above would put
	 * `$Hall.north@t=25%` at a different quarter of the wall depending on how the room was written
	 * down. §21.6's ruling — a parameter is in the EDGE's direction — is enforced by this signature.
	 *
	 * It also spans a chain, so a caller never learns which piece of a jogged wall it landed on.
	 */
	static bool EdgeFrameAlong(const FGcRegionSpace& Space, const FGcResolvedRegion& Region,
		int32 EdgeIndex, double T, FGcRegionEdgeFrame& OutFrame);

	/**
	 * The frame at a point along a COURSE — a run the region declares that is not a wall (§21.12n).
	 *
	 * **TAKES NO RESOLVED REGION, AND CANNOT.** `EdgeFrameAlong` finds its answer by walking the
	 * resolved loop, because which way is OUT of a wall depends on which side the interior lies; a
	 * course is not part of any boundary, so it is not in the walk and there is no interior to be on
	 * a side of. It returns false for one — asserted — rather than answering wrongly.
	 *
	 * `Along` is the DECLARED direction and `Outward` is right of travel, meaning only that. `@t` is
	 * arc length over the whole run, the same measure a wall uses, through the same splitter.
	 */
	static bool CourseFrameAlong(const FGcRegionSpace& Space, int32 EdgeIndex, double T,
		FGcRegionEdgeFrame& OutFrame);

	/**
	 * The plan outline as straight pieces, counter-clockwise — the sampler's input, and CONSERVATIVE.
	 *
	 * **Every piece lies on the INTERIOR side of the arc it approximates**, which is the property the
	 * sampler needs and an ordinary inscribed tessellation does not have. An arc bowing outward is
	 * inscribed, its chords falling inside the region. An arc bowing INWARD has the region on the far
	 * side of the curve, so an inscribed chord cuts through ground the region does not cover; those
	 * vertices are pushed out radially by `R·(sec(Δθ/2) − 1)`, which makes each piece a tangent to the
	 * true arc and puts the whole piece inside the region.
	 *
	 * Without it a sample can land microscopically outside a curved wall, and §21's exit — *the walls
	 * of the crypt* — is exactly the case where that is a goblin inside the masonry.
	 */
	static TArray<FVector2D> TessellatedOutline(const FGcRegionSpace& Space,
		const FGcResolvedRegion& Region);

	/**
	 * How many straight pieces an arc of this SWEEP becomes — `ArcSegments` for every half turn.
	 *
	 * **The one place a piece count is derived from anything**, and it is derived from the SWEEP
	 * alone, never from the radius or a chord tolerance. `ArcSegments`' own reason holds: a count
	 * that moved with the radius would make a triangulation depend on a number an engine upgrade
	 * could round differently. A sweep comes from the document's own coordinates, so the same file
	 * gives the same count forever, and a 5° join stops costing what a semicircle costs.
	 *
	 * It is calibrated so a semicircle is exactly `ArcSegments` pieces — which is what a region's
	 * own bowed edge gets — and therefore a full circle is `2 * ArcSegments`. `DiscOutline` and a
	 * region promoted by `from: $P radius: R` (two bowed edges, §21.12o) tessellate identically.
	 */
	static int32 ArcPieces(double SweepRadians);

	/**
	 * A DISC as a plan ring, counter-clockwise — `at=$Peak radius=1800` given a shape.
	 *
	 * **INSCRIBED, which is `TessellatedOutline`'s conservatism one shape over.** Every vertex sits
	 * ON the circle and every chord inside it, so the ring never claims ground outside the radius
	 * the author stated. A solid raised over it is the same 0.16% under the true area that a bowed
	 * region's is, and in the same direction.
	 */
	static TArray<FVector2D> DiscOutline(const FVector2D& Centre, double Radius);

	/**
	 * A STRIP as a plan ring, counter-clockwise — the Minkowski sum of `Run` and a disc of
	 * `HalfWidth`, which is exactly the ground `DistanceToRun(p) <= HalfWidth` accepts.
	 *
	 * **THE RING AND THE PREDICATE ARE ONE SHAPE, and that is the property to keep.** `along=$Stream
	 * width=600` masks a heightfield in Core, bounds a scatter in World and now raises a solid — if
	 * the ring described slightly different ground from the distance test, terrain would carve one
	 * strip and a volume would fill another, a disagreement no round trip can see
	 * ([[a-read-answers-through-the-write-path]]).
	 *
	 * **EXACT, NOT OFFSET-AND-HOPE.** An OUTER corner is an arc of `HalfWidth` about the run vertex;
	 * an INNER corner is the intersection of the two offset lines, which is on the boundary exactly
	 * while it does not overshoot; each end is a half-circle cap. Where an inner corner DOES
	 * overshoot — a turn too sharp for the width to get round in the length of the wall beside it —
	 * the sum's boundary stops being a simple ring, and this REFUSES naming the vertex rather than
	 * handing a self-crossing outline to a brush builder that would report it as a region whose
	 * walls cross ([[refusal-reason-is-a-claim]]).
	 *
	 * `OutAlong`, when asked, is index-parallel to `OutRing` and carries each ring point's
	 * arc-length fraction along `Run` — the same 0-to-1 `DistanceToRun` reports, so a band stated as
	 * a profile down the course resolves per COLUMN of the solid. A ring point is always generated
	 * at a run VERTEX (a cap, a join, or an end of a side chain), which is what makes the fraction
	 * exact rather than interpolated.
	 */
	static bool StripOutline(const TArray<FVector2D>& Run, double HalfWidth,
		TArray<FVector2D>& OutRing, TArray<double>* OutAlong, FString& OutRefusal);

	/**
	 * `Run` with a vertex inserted at each of `Fractions` — arc-length fractions, 0 to 1.
	 *
	 * **A PROFILE'S STOPS HAVE TO BE COLUMNS OR THE SOLID MISSES THEM.** `min=(0=100, 0.5=900,
	 * 1=100)` down a two-point run gives a ring whose only fractions are 0 and 1, so the lid would
	 * run straight from the first stop to the last and the middle one — the whole point of the
	 * profile — would never appear in the geometry. Subdividing first costs a collinear vertex,
	 * which the offset walk emits as one point and no join.
	 */
	static TArray<FVector2D> SubdivideRunAt(const TArray<FVector2D>& Run,
		const TArray<double>& Fractions);

	/**
	 * One EDGE as straight pieces, in its own declared direction — `TessellatedOutline` for a wall.
	 *
	 * `StartT`/`EndT` are `PointOnEdge`'s parameter, so a caller that has found the stretch of wall it
	 * cares about can ask for that stretch alone. The ends are exact; an arc between them becomes
	 * `ArcSegments` pieces of the same sweep, which keeps it the same tessellation the outline uses.
	 *
	 * `OutDeclaredIndices`, when asked for, comes back with one entry per DECLARED point of the edge —
	 * `Points.Num()` of them — holding that point's index in the returned line, or `INDEX_NONE` where
	 * the clipped range never reached it.
	 *
	 * **IT IS ANSWERED HERE BECAUSE ONLY HERE KNOWS.** Everything between two declared points is
	 * geometry this walk invented, and the counts differ per segment: a straight piece emits one and
	 * an arc emits `ArcSegments`, while `Emit` drops a vertex that lands on the one before it. A
	 * caller re-deriving the mapping from a `t` would be comparing an arc-length parameter against a
	 * chord-length walk and would be right only on straight chains — silently wrong on exactly the
	 * bowed wall this exists to describe ([[a-read-answers-through-the-write-path]]).
	 */
	static TArray<FVector2D> EdgePolyline(const FGcRegionSpace& Space, int32 EdgeIndex,
		double StartT = 0.0, double EndT = 1.0, TArray<int32>* OutDeclaredIndices = nullptr);

	/**
	 * How much of this edge lies AGAINST that outline — the longest unbroken stretch, in centimetres.
	 *
	 * *Do these two rooms meet along here*, which is a question about the plan and never about the
	 * navmesh (§4.4). It is the precondition for `sealed:`, and the reason that key can be believed:
	 * a probe a fixed distance past a wall lands inside the room across a narrow corridor just as
	 * readily as inside the room on the other side of a shared wall, so *these meet* has to be
	 * measured rather than inferred from what is over there.
	 *
	 * A NUMBER AND NOT A VERDICT, for `ClosestOnEdge`'s reason one operation over: how long a stretch
	 * counts as meeting is the caller's ruling, and a corner touch answers `0` here without anybody
	 * having to agree on a minimum first. `OutStartT`/`OutEndT` bound that stretch so the caller can
	 * probe its middle rather than the middle of a wall it shares only one end of.
	 *
	 * `OtherOutline` is passed in rather than resolved here because a caller measuring every wall of a
	 * level against every neighbour would otherwise re-tessellate the same room once per wall.
	 */
	static double SharedBoundaryCm(const FGcRegionSpace& Space, int32 EdgeIndex,
		TArrayView<const FVector2D> OtherOutline, double ToleranceCm,
		double& OutStartT, double& OutEndT);

	/**
	 * Do these two open polylines cross, in plan?
	 *
	 * TOUCHING COUNTS, and that is the case it exists for: a navmesh route string-pulls its corners
	 * onto the corners of the geometry it passes, so a path through a doorway commonly has a vertex
	 * lying exactly on the wall line rather than a segment passing cleanly through it. A strict
	 * interior-crossing test answers *no* for the one route everybody would call going through.
	 */
	static bool PolylinesCross(TArrayView<const FVector2D> Left, TArrayView<const FVector2D> Right);

	/**
	 * Ear clipping over `TessellatedOutline`, deterministic by construction: the first ear in index
	 * order wins, every time, with no tolerance to tune and nothing consulted but the vertices.
	 *
	 * Refuses `region_not_simple` when no ear exists, which is a self-intersecting outline — the one
	 * shape every containment answer here is already undefined for.
	 */
	static bool Triangulate(const FGcRegionSpace& Space, const FGcResolvedRegion& Region,
		TArray<FGcRegionTriangle>& OutTriangles, FString& OutRefusal);

	/**
	 * A deterministic point inside the region — §8's hard requirement, and §21.12's *never
	 * rejection-sample in a loop*.
	 *
	 * AREA-WEIGHTED TRIANGLE CHOICE, which is what makes a concave room sample uniformly instead of
	 * accidentally: pick a triangle with probability proportional to its area, then a uniform point in
	 * it. Rejection sampling against a bounding box gets the same distribution when it terminates and
	 * has no bound on when that is — a thin L-shaped corridor in a large box can miss for a long time,
	 * and a document that sometimes fails to place an actor is worse than one that never does.
	 *
	 * A pure function of `(Seed, Index, geometry)` and of nothing else, through `ElementSeed` — Z0's
	 * written-out mix, reused rather than re-derived so both samplers survive an engine upgrade
	 * together or not at all.
	 *
	 * Z IS THE ANCHOR'S, never a uniform pick inside the prism and never `Min` (§21.18c, user-ruled:
	 * *"it places based on the height of the anchor also, not the min (which might be unbounded), and
	 * definitely not always default to 0"*). Unbounded says *do not test Z*; it cannot also say *put it
	 * here*, and a height that scattered actors through the air would make the ordinary line —
	 * `in=$Crypt` with `@surface[toward=down]` — resolve against a start nobody meant.
	 */
	static bool SamplePoint(const FGcRegionSpace& Space, const FGcResolvedRegion& Region, int32 Seed,
		int32 Index, FVector& OutPoint, FString& OutRefusal);
};
