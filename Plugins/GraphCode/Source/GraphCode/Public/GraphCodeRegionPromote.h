// Copyright OraStudio. All Rights Reserved.

// PROMOTION — `region Crypt  from: FloorSlab_01 { }`. §21.8's C3, taken by the user 2026-08-11.
//
// `from:` sits on the region's HEAD LINE since §21.1b, beside `base:` and `height:`, because the
// braces hold declarations and a region being promoted has none to hold.
//
// The bootstrap problem §21.8 exists for: a region is points and points are coordinates, which is the
// one thing §1 says a model should not have to type. Three sources answer it, and this is the third
// one made one line long — instead of pointing at four corners with
// `@anchor[target=FloorSlab_01, forward=-100%, right=-100%]` and its three siblings, name the actor
// once and take its footprint whole.
//
// **THE ACTOR IS A STENCIL, NOT THE REGION**, which is C2 applied rather than a new ruling. The
// footprint is traced ONCE into real points and edges carrying receipts; afterwards the region is
// ordinary authored geometry, editable corner by corner, and the actor may move or be deleted without
// touching it. The live reading — the region keeps pointing at the actor — is refused for the two
// reasons C2 gives: it is §18.7's dependency cycle (walls depend on the region, the region depends on
// the walls) and *nudging a floor slab would silently move a room*. It is also §4a, which §21
// replaced. Re-running a promotion after the slab moves is one deliberate call.
//
// **AND `from:` NEVER PERSISTS.** It is resolved at apply time and cleared, so a committed `.gcw`
// cannot contain a half-resolved instruction to something that has since moved. The renderer has no
// branch for it, which makes that structural rather than a promise.
//
// ~~THIS IS THE ONE PIECE OF THE REGION SURFACE THAT NEEDS A LOADED WORLD~~ — **NARROWED BY R15,
// §21.8b: AN ACTOR NEEDS ONE AND A PROMOTION DOES NOT.** A `$`-marked source is a place this file
// already declares, so `from: $Village.altar  radius: 30000` is arithmetic over the document and
// resolves headless. Only a BARE name has to be found in a level, and the world is asked for when
// one is named rather than whenever a `from:` exists — otherwise the cheapest form of the feature
// would be the most demanding. Everything else — parsing, planning, questions, the crack list —
// answers from the file, as it always did.
//
// **AND `radius:` IS A MODIFIER ON `from:`, NOT A SECOND WAY TO DERIVE A RING** (§21.8b, ruling 1).
// The sources are gathered once, hulled once, and pushed out once into ONE closed ring with a real
// arc at every corner — so a single place is a disc of two bowed walls and two are a stadium. Both
// keys are cleared at apply, exactly as `from:` always was.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"

class UWorld;

/** Refusal tokens for promotion, so a caller branches on the reason and not on the sentence. */
namespace GcwPromoteToken
{
	inline constexpr const TCHAR* NeedsWorld = TEXT("gcw_promote_needs_world");

	/** The editor has a level open and it is not the one this file belongs beside. */
	inline constexpr const TCHAR* OtherLevel = TEXT("gcw_promote_other_level");

	inline constexpr const TCHAR* UnknownActor = TEXT("gcw_promote_unknown_actor");
	inline constexpr const TCHAR* NoFootprint = TEXT("gcw_promote_no_footprint");
	inline constexpr const TCHAR* AlreadyLooped = TEXT("gcw_promote_loop_and_from");
}

/** One promotion that happened, so an apply can report what it traced rather than only its result. */
struct GRAPHCODE_API FGcwPromotion
{
	FString Region;

	/**
	 * The sources it was traced from, in the order they were named. A receipt, and receipts are never
	 * authority (§18.3a). One for `from: FloorSlab_01`, four for a room's four walls — and since R15
	 * a `$`-marked entry is a place this file declares rather than an actor, kept with its mark
	 * because the mark is what says which of the two it was.
	 */
	TArray<FString> Actors;

	/** Minted names, in loop order, so a caller can see what it now owns. */
	TArray<FString> Points;
	TArray<FString> Edges;

	/** How many corners the footprint turned out to have. Four for a slab; more for a platform. */
	int32 CornerCount = 0;

	/**
	 * `collision`, `bounds`, `declared`, or `mixed` when a LIST disagreed with itself — never left to
	 * be inferred.
	 *
	 * `mixed` is a summary and deliberately not the whole answer: `Receipt` below says which source
	 * answered which way, because *traced from what the player stands on* and *traced from a box
	 * around it* are different claims and a list can hold both at once.
	 *
	 * **`declared` IS R15's, and it is a third claim rather than a shade of the other two**: a
	 * `$`-marked source was read from neither collision nor a bounding box, it came out of this
	 * document — and calling that `bounds` would be a receipt that reads true and is false.
	 */
	FString Source;

	/**
	 * Exactly what was stamped on every minted point: one `<actor> <source>` term per actor, joined
	 * with ` + `. One actor reads `FloorSlab_01 collision`, which is what it always read.
	 */
	FString Receipt;

	/**
	 * What a UNION cost, said out loud — and since R15 what a STAND-OFF cost too, so it is empty only
	 * for one source pushed out by nothing.
	 *
	 * The hull of several footprints is CONVEX and contains everything between them — for four walls
	 * that is the room plus the thickness of its walls (§21.8a). It is right for the common case and
	 * wrong visibly rather than silently, in a file where every corner is editable, so it is stated
	 * rather than refused.
	 */
	FString Note;

	/** Plan area of the traced footprint, cm². What the region will actually measure. */
	double AreaCm2 = 0.0;

	/**
	 * Where `anchorZ:` came from when the author stated none. Empty when they did.
	 *
	 * STILL CALLED A BASE RECEIPT because that is what a reader of a promotion calls it, and because
	 * the inference did not change when the field it fills did (§21.18c): the top of the tallest thing
	 * traced is where the region IS, and how far up it reaches is still nobody's guess but the author's.
	 */
	FString BaseReceipt;
};

/** Resolve every `from:` in a document against a loaded world. The one impure step of R5b. */
class GRAPHCODE_API FGraphCodeRegionPromote
{
public:
	/** True when any region in the document still carries an unresolved `from:`. */
	static bool NeedsPromotion(const FGcwDocument& Document);

	/**
	 * Trace each `from:` actor's COLLISION footprint in plan into points and edges.
	 *
	 * **SEVERAL ACTORS ARE ONE BOUNDARY, AND IT IS THE UNION HULL** — §21.8a's third ruling, user's
	 * words: *"could declare multiple actors, they could act as boundaries, so a region can trace the
	 * actors."* Every named actor's footprint goes into one gather and is hulled ONCE, so `from:
	 * Wall_N, Wall_E, Wall_S, Wall_W` is the room those walls stand around and not four rooms. It is
	 * the reading that was priced as *right for what somebody actually types* and wrong only by the
	 * wall thickness — visibly, in a file whose every corner is editable — and the promotion carries
	 * a `Note` saying so. The other reading, the enclosed INTERIOR, needs an inside stated before it
	 * can be found and is a different task; §21.8a names both so neither is mistaken for the other.
	 *
	 * **AN ACTOR THAT CONTRIBUTES NOTHING IS REFUSED BY NAME**, rather than dropped from the gather.
	 * A boundary that silently went missing leaves a hull that is still convex, still plausible, and
	 * around somewhere else.
	 *
	 * **COLLISION, NOT THE BOUNDING BOX — user-directed 2026-08-11: *maybe use collision instead of
	 * box so it's more accurate*.** It is more accurate twice over. A slab rotated 30 degrees has an
	 * AABB noticeably larger than itself, so a box-traced region claims floor the slab does not
	 * cover; and a shape that is not a rectangle at all — a hexagonal platform, an angled walkway —
	 * has a real footprint that a box can only approximate. The simple collision aggregate is walked
	 * (boxes, convex hulls, spheres and capsules), every vertex is taken to world space, and their
	 * plan hull is the footprint. So the region ends up bounded by *what the player can stand on*.
	 *
	 * **AN ACTOR WITH NO SIMPLE COLLISION FALLS BACK TO ITS ORIENTED BOX, and the receipt says so**
	 * rather than the two cases being spelled the same. Three ways to have none, each a real asset
	 * somebody ships: query collision switched off, an empty aggregate, or no body setup at all —
	 * and `CTF_UseComplexAsSimple` is a fourth, because the aggregate is then not what the physics
	 * world answers with and this walk cannot read what is.
	 *
	 * **`base:` IS THE TOP OF WHAT THE FOOTPRINT WAS TRACED FROM when the author states none**, and
	 * the receipt says so. This is the one inference here and it is made deliberately: `base` is
	 * where sampling puts things (§21.5), so leaving it at 0 would scatter actors on the ground under
	 * a slab at Z=300 — a silent wrong answer where this is a visible, editable number. An explicit
	 * `base:` always wins.
	 *
	 * `LevelPath` is the level this file belongs beside, and the world has to BE it. Tracing
	 * `FloorSlab_01` out of whichever map happens to be open would write another level's geometry
	 * into this one's regions, with every coordinate plausible and nothing erroring anywhere.
	 *
	 * **AND THEN PUSH THAT BOUNDARY OUT BY `radius:` — R15, §21.8b.** One gather, one hull, one
	 * offset, one closed ring: the fillet at each corner is a REAL ARC carried on the minted edge's
	 * bulge, so a disc is two bowed walls rather than a thirty-two-sided approximation nobody can
	 * edit. A spot and a segment stop being refusals when there is a radius — one place pushed out is
	 * a disc and two are a stadium — so the *no footprint* test asks AFTER the offset, not before it.
	 *
	 * `World` may be null when every source is `$`-marked, and that is the point rather than a
	 * tolerance: a place is arithmetic over the document.
	 *
	 * `Existing` is the file this payload will land on, or null for a create. It is read for one
	 * thing only: a minted name must never be one that file has already spent.
	 *
	 * Counter-clockwise in plan, so the loop needs no winding fix later — though
	 * `FGraphCodeRegionSolver::Resolve` normalises anyway and a spec asserts both agree.
	 */
	static bool Resolve(UWorld* World, const FString& LevelPath, FGcwDocument& Document,
		const FGcwDocument* Existing, TArray<FGcwPromotion>& OutPromotions, FString& OutRefusal);
};
