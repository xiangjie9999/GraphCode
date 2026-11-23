// Copyright OraStudio. All Rights Reserved.
//
// What a product knows about ONE ACTOR CLASS that the reflective walk cannot see —
// `GC-TERRAIN-DESIGN.md` §7 and row L0a.
//
// **THE FOURTH SEAM OF THIS SHAPE, after region navigation, region generation and the terrain
// realizer, and the reason is the same one four times.** Core walks a level's actors and writes
// every entry; `ALandscape` lives in `GraphCodeWorld`, so Core cannot ask it anything. A product
// registers what it knows, and Core consults the registry without ever naming the class.
//
// **IT IS NOT A KIND, AND THAT IS THE RULING THIS FILE EXISTS TO RECORD.** §7 is headed *"Format 1 —
// `kind:landscape`"*, and the measurement says otherwise: a landscape is an ACTOR. There is no
// `ULandscape` asset to open, nothing for `gc_read asset:` to name, and `kind:external_actor`
// already carries a partitioned proxy. So its record is an ENTRY IN A LEVEL'S `actors { }` BLOCK,
// and what was missing was never a document — it was four fields and a filter
// ([[a-misplaced-capability-costs-a-name]]: a kind here would have cost a name and answered nothing
// `kind:level` does not already answer).
//
// **TWO HALVES, BECAUSE THE DEFECT HAS TWO HALVES** (measured 2026-08-19, §12f): the entry said
// nothing about what the landscape IS, and said twenty lines about components the engine had just
// created. One delegate adds the first, the other drops the second. A product that only wants one
// of them binds one.
//
// **A THIRD DELEGATE WAS BUILT HERE ON 2026-08-20 AND REMOVED THE SAME DAY — read this before
// re-inventing it.** It asked *is this actor's shape authored in another document*, so that a level
// would stop recording a brush a `.gcw` already describes. **No other document describes one.**
//
// The user's correction, which is the whole of it: ***"gcw don't save volume."*** MEASURED — a
// `.gcw` holds a REGION (a named place) and a `nav()`/`pcg()` line (an intent). The volume actor's
// name, class, centre, size, outline, area class, agent bits and tags are every one of them COMPUTED
// at realise time (`GraphCodeWorldRegionNavigation.cpp:566-624`). A `.gcw` is not a volume store, so
// it can never be the other carrier this delegate was written to detect.
//
// What follows is that **the `volumes { }` decision is LOCAL TO THE `.gc`**: the level records every
// volume in the level, always, and the shape sits ONCE inside that file — on the actor entry's
// `BrushBuilder` line while the builder describes the solid, in `volumes { }` once a hand edit moves
// it away. `BuilderDescribes`/`BuilderDiverges`/`NoBuilder` is already the entire rule, asked of the
// brush in front of it. Nothing is excluded from a level on account of another file.
//
// **And REGENERABLE IS NOT RECORDED.** The follow-on proposal — drop `.gcw`-generated actors from
// `actors { }` — was put to the user and DECLINED: recovering one would need a realise PASS rather
// than a read, and a `.gc` that omits actors really present stops describing its level.
// `GC-WORLD-GENERATION-DESIGN.md` §12.9h.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeProducts.h"

class AActor;
class UActorComponent;
class FProperty;

/**
 * Extra `@directive[…]` lines for an actor this product recognises. Returns the number of TOP-LEVEL
 * FIELDS written — `found:` is a checksum over fields, so the callee, which knows its own count by
 * construction, is the one that says it.
 *
 * **ZERO AND NO LINES IS THE ANSWER FOR AN ACTOR THIS PRODUCT DOES NOT RECOGNISE**, which is the
 * common case: the provider is consulted for every actor in the level, not for a pre-filtered set.
 * Core cannot pre-filter — filtering means naming the class.
 */
using FGcActorFacetProvider =
	TDelegate<int32(const AActor* /*Actor*/, int32 /*Depth*/, TArray<FString>& /*OutLines*/)>;

/**
 * True for a component the ENGINE built from the owning actor's own data, rather than one somebody
 * added. Consulted for every component of every actor, so it answers false by default.
 */
using FGcActorDerivedComponentTest = TDelegate<bool(const UActorComponent* /*Component*/)>;

/**
 * True for a field this product AUTHORS even though reflection calls it read-only.
 *
 * **THE THIRD SHAPE OF THE SAME DEFECT, and the first one Core cannot name its way out of**
 * ([[reflective-policy-misses-the-authored-facets]]). The generic component rule drops a property
 * that is `CPF_EditConst`, which is what `VisibleAnywhere` sets — and `UPCGComponent::GraphInstance`
 * is `BlueprintReadOnly, VisibleAnywhere, Instanced` (MEASURED, `PCGComponent.h:633`), so a PCG
 * volume projected a correctly-shaped box that ran NOTHING: the outline, the height and the seed all
 * landed, and which graph to run did not (`GC-LEVEL-RECONSTRUCTION.md` gap 2).
 *
 * **A DELEGATE RATHER THAN A NAMED EXCEPTION, and that is the difference from the two precedents.**
 * `IsAuthoredBrushShape` and `IsAuthoredStreamingStateImpl` both name their owner class inline in
 * Core, which they may: `ABrush` and `ULevelStreaming` are ENGINE classes. `UPCGComponent` belongs
 * to a plugin only GraphCodeWorld links, so Core naming it would not compile in a Core-only install
 * — the registry exists for exactly this and this is its third question.
 *
 * **KEY BY OWNER CLASS AND MEMBER, never by member alone.** A property called `GraphInstance` on
 * something that is not a PCG component is somebody else's property and this has nothing to say
 * about it — the rule both precedents state and the one that keeps an inclusion from widening
 * silently as the corpus grows.
 *
 * Consulted for every property of every component, so it answers false by default.
 */
using FGcActorAuthoredFieldTest = TDelegate<bool(const FProperty* /*Property*/)>;

/**
 * The upstream document's name for the shape this product GENERATED on this actor, or empty.
 *
 * **THE FOURTH QUESTION, AND THE FIRST WHERE THE ANSWER SHORTENS THE ENTRY RATHER THAN COMPLETING
 * IT.** The three above all ADD — a directive line, a component the walk should stop writing, a
 * field it wrongly dropped. This one says a value already in the entry is not this document's to
 * state, because a product computed it from something the author actually wrote somewhere else.
 *
 * **MEASURED, on `L_HollowVale`.** `L_HollowVale.gcw` states `Meadow` as four corners with
 * `[round=800.0]` on two of them; `GraphCodeWorldRegionPrismBuilder` tessellates those fillets and
 * the level document wrote the resulting **seventy** points as a 2,251-character `BrushBuilder=`
 * on one line — 14% of a 16 KB file, for three volumes, with not one authored number among them.
 * The user's spline ruling already covers it: *"for engine recompute datas, should not be stored by
 * us"* (2026-08-20, `GraphCodeSplineCurve.h`).
 *
 * **A NAME, NOT A BOOL, because the entry has to say WHICH.** `BrushBuilder = $Meadow` reads back
 * to a `regions { }` entry in the sibling `.gcw`, and `$` is the same *declared elsewhere* marker
 * `splines { }` writes for a curve and the `.gcw` itself writes in `in=$Meadow`.
 *
 * **AND THE PRODUCT ANSWERS EMPTY THE MOMENT SOMEBODY HAS EDITED THE SHAPE.** A hand-dragged corner
 * makes the brush's geometry diverge from its builder, which is the case `volumes { }` exists for —
 * and that block's delta is measured *against the builder this same document writes on this actor's
 * line*, so a reference there would leave the delta with no base. Generated-and-untouched points
 * upstream; edited keeps its builder and its `volumes { }` entry. The product owns that test
 * because the product owns the builder.
 *
 * Consulted for every actor of the level, so it answers empty by default.
 */
using FGcActorRegionReferenceTest = TDelegate<FString(const AActor* /*Actor*/)>;

class GRAPHCODE_API FGraphCodeActorFacets
{
public:
	/**
	 * Both delegates in ONE registration rather than two seams, and this is the one place in
	 * GraphCode where that is the right call — contrast the three that came before, which are
	 * deliberately separate.
	 *
	 * The rule those three obey is that a Core-only install must refuse each capability BY ITS OWN
	 * NAME ([[a-refusal-s-reason-is-a-claim]]), because a caller who asked to shape a landscape
	 * should not be told about PCG. Nothing here is ever refused: a projection with no facet
	 * registered writes the entry it always wrote. And the two halves are not two capabilities but
	 * one sentence about one actor — *say what it is, and stop saying what it derived* — where
	 * binding one without the other produces a document that is measurably worse than either end
	 * state: identity plus the flood, or a silent hole where the components were.
	 *
	 * `Core` may not register. A facet is a product's knowledge of its own classes, and Core has no
	 * classes of its own to know.
	 */
	static bool Register(EGcProduct Product, FGcActorFacetProvider Provider,
		FGcActorDerivedComponentTest DerivedTest, FString& OutError);

	/**
	 * The same registration carrying the authored-field test as well.
	 *
	 * **AN OVERLOAD RATHER THAN A THIRD SEAM, and rather than a defaulted parameter.** The two
	 * delegates above ship together because binding one without the other is measurably worse than
	 * either end state; that argument does NOT extend to this one, which answers a question about a
	 * PROPERTY rather than about an actor and is independently useful. What keeps it here anyway is
	 * the file's own subject — *what a product knows that the reflective walk cannot see* — and the
	 * registry's shape: one answerer per product, so a second registry would mean a product
	 * registering twice and `Unregister` taking away half of it.
	 */
	static bool Register(EGcProduct Product, FGcActorFacetProvider Provider,
		FGcActorDerivedComponentTest DerivedTest, FGcActorAuthoredFieldTest AuthoredFieldTest,
		FString& OutError);

	/**
	 * The same registration carrying the region-reference test as well.
	 *
	 * **A FOURTH OVERLOAD ON THE THIRD'S ARGUMENT, and the alternative was worse than it looks.**
	 * One answerer per product is the registry's shape, so a separate seam would mean World
	 * registering twice and `Unregister` taking away half of it — which is the reason the authored-
	 * field test folded in here rather than standing alone, and nothing about this question weakens
	 * it. What it does add is an overload chain, and that is the honest cost: a product binding
	 * three of the four still calls the three-argument form and nothing changes for it.
	 */
	static bool Register(EGcProduct Product, FGcActorFacetProvider Provider,
		FGcActorDerivedComponentTest DerivedTest, FGcActorAuthoredFieldTest AuthoredFieldTest,
		FGcActorRegionReferenceTest RegionTest, FString& OutError);

	static void Unregister(EGcProduct Product);

	/** True when at least one INSTALLED product has registered. */
	static bool IsAvailable();

	/**
	 * Every registered provider, in registration order, appending to one list.
	 *
	 * More than one may answer for one actor and that is not an error: they are different products
	 * describing different facets of the same thing, exactly as two `@directive` lines already sit
	 * beside each other on an entry.
	 */
	static int32 Describe(const AActor* Actor, int32 Depth, TArray<FString>& OutLines);

	/** True when ANY registered product calls this component derived. */
	static bool IsDerivedComponent(const UActorComponent* Component);

	/**
	 * True when ANY registered product calls this field authored despite reflection.
	 *
	 * **AN INCLUSION, so it is asked BEFORE the generic rule and never after it.** Asked after, it
	 * could only ever re-drop what the generic rule already kept; the whole point is to admit what
	 * the generic rule refuses. Both precedents are written the same way round.
	 */
	static bool IsAuthoredField(const FProperty* Property);

	/**
	 * The first non-empty region name any registered product gives this actor, or empty.
	 *
	 * **FIRST ANSWER WINS, unlike `Describe` which lets every product speak.** A directive line is
	 * additive and two products may each have something to say about one actor; a `BrushBuilder`
	 * has ONE value, so a second name could only overwrite the first. Two products both claiming to
	 * have generated one brush is a bug in one of them, and silently taking the later registration
	 * would hide it.
	 */
	static FString RegionReference(const AActor* Actor);
};
