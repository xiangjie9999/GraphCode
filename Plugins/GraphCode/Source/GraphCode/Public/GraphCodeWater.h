// Copyright GraphCode Sdn Bhd. All Rights Reserved.
//
// The `water { }` block — `GC-TERRAIN-DESIGN.md` §12i, row L8.
//
// **THE PRIMARY JOB IS REFLECTION, NOT AUTHORING** (§12i.0), and that is the opposite emphasis from
// terrain. A hand-sculpted hill is a quarter-million height samples with no intent attached, so all
// terrain can honestly keep is the bytes. A water body is a spline of a few dozen control points
// carrying named curves — the intent is already symbolic — so it comes back as readable parametric
// text with nothing lost. Authoring from scratch is the bonus.
//
// **AN ENTRY IS S3, THE LANGUAGE'S COMMONEST SHAPE** (`GC-GRAMMAR-REFERENCE.md` §3), and it is the
// user's own rule that derives it: children → `{ }`, properties of its own → `Class(…)`, a name →
// `Name = Class(…)`. So a body is ONE LINE when it has no children, and grows a body only when it
// carries a captured spline. Nothing switches form between the two — the braces simply appear when
// there is something to put in them (§12i.10).
//
// **POSITION MEANS NOTHING AT THE BODY LEVEL AND EVERYTHING INSIDE A SPLINE** (§12i.9, §12i.11).
// `AWaterBrushManager::SortWaterBodiesForBrushRender_Implementation` sorts bodies itself before
// carving — Ocean, Lake, River, Custom, ties on the actor's full name — and overlap is decided by
// three explicit priority fields, so a document that made line order mean something would state an
// order the engine discards. A spline's point order IS the shape of the river, so those lines are
// ordered exactly as a terrain program's are.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeTerrain.h"

/**
 * What an entry makes. One keyword, not five, because they differ only in the constructor name and
 * share every other token (§12i.6).
 *
 * **`Pool` DOES NOT FOLD INTO `Lake`, and the reason is measured** (§12i.8/3):
 * `UWaterBodyCustomComponent` overrides `CanEverAffectWaterMesh()` and `CanEverAffectWaterInfo()` to
 * false and renders through its own `UStaticMeshComponent`. For that one type the mesh is AUTHORED
 * and REQUIRED — the exact inversion of the seam every other body sits on — so filing it under a
 * keyword whose whole meaning is the zone mesh would be wrong in the one place it matters. Its
 * engine spelling is `EWaterBodyType::Transition`, display name "Custom"; `pool` is a rename this
 * grammar owns and maps both ways.
 *
 * **THE LAST THREE ARE NOT BODIES, AND THE BLOCK HOLDS THEM ANYWAY** (user-ruled 2026-08-20:
 * *"this water { } block encompasses water entities from different scopes"*). They sit at three
 * different levels of the engine's own model and one document is still the right place for all of
 * them, because a person asking *what water is in this level* means every one:
 *
 * - `Island` is `AWaterBodyIsland`, a hole in the water. It is a SIBLING of the bodies rather than
 *   a child of one, and that is measured: `UpdateOverlappingWaterBodies` calls `AddIsland` on EVERY
 *   body it overlaps, so its membership is derived from geometry (§12i.10).
 * - `Zone` is `AWaterZone`, which owns the derived quadtree mesh for every body inside it. A level
 *   may hold several, and a body may name one instead of taking the one it happens to sit in.
 * - `Exclude` is `AWaterBodyExclusionVolume`, which takes water AWAY — a mode and a list of bodies.
 *   It is subordinate to none of them, which is why it is an entry and not a key on one.
 *
 * **What none of them changes is the seam.** The zone's water mesh, a body's collision and HLOD
 * meshes, and the `WaterBodyIslands` list are all DERIVED and none is ever written (§12i.3).
 */
enum class EGcWaterKind : uint8
{
	River,
	Lake,
	Ocean,
	Pool,
	Island,
	Zone,
	Exclude,
};

/**
 * One control point of a captured spline — `(12400, 200, 40)  [linear, width=3600]`.
 *
 * **A BARE `(x, y, z)` IS MEASURABLY LOSSY, which is why the modifiers exist** (§12i.8/2).
 * `UWaterSplineComponent::GetEnabledSplinePointTypes` returns four types including
 * `CurveCustomTangent`, so a hand-tuned river captured as positions alone comes back re-curved,
 * silently — the one failure a mirror may not have. `AllowsSplinePointScaleEditing()` returns false,
 * so scale is the one facet that never needs capturing.
 *
 * **ONLY DEVIATIONS ARE WRITTEN.** A point whose type is the ordinary `Curve` with engine-derived
 * tangents and untouched curve values renders as three numbers, which is what almost every point
 * is. The engine hands us the discriminator for the values half:
 * `UWaterSplineComponent::PreviousWaterSplineDefaults` exists to say *"whether spline points were
 * modified by users or exist at a current default value"* ([[empty-is-not-unauthored]], one level
 * down).
 */
struct GRAPHCODE_API FGcWaterPoint
{
	/** World centimetres, the frame the terrain program is already in. */
	FVector Position = FVector::ZeroVector;

	/**
	 * `linear` / `clamped` / `custom` — absent means the ordinary `Curve`.
	 *
	 * Held as a word rather than the engine's enum because this header is Core's and `ESplinePointType`
	 * is the engine's; the realiser maps it, and the four legal words are checked on the way in so a
	 * typo refuses here rather than becoming a straight river three steps later.
	 */
	FString Type;

	bool bHasArrive = false;
	FVector Arrive = FVector::ZeroVector;

	bool bHasLeave = false;
	FVector Leave = FVector::ZeroVector;

	/**
	 * `width=`, `depth=`, `velocity=`, `audio=` AT THIS POINT, in authored order.
	 *
	 * **ON THE POINT RATHER THAN IN A PROFILE ACROSS THE WHOLE RIVER, for a captured spline only.**
	 * The engine stores these as `FInterpCurveFloat` keyed by the point's own input key, so a
	 * captured value belongs to a point exactly; keying it by a fraction of the way along would
	 * invent an arc-length conversion for a fact that already has an exact home, and it diffs worse
	 * — one dragged point should be one changed line. An AUTHORED body, which has no points of its
	 * own, still spells the same quantity as a profile on the entry line.
	 */
	TArray<TPair<FString, double>> Values;

	/** The author's own grouping, exactly as a terrain instruction carries it. */
	TArray<FString> Leading;
	FString Trailing;

	bool IsPlain() const
	{
		return Type.IsEmpty() && !bHasArrive && !bHasLeave && Values.IsEmpty();
	}
};

/**
 * One entry: `Rill = river(along=$Valley.spine, width=(0=380, 1=420))`, with an optional body.
 *
 * **THE LEFT-HAND SIDE IS AN AUTHORED NAME AND IT IS APPLIED TO THE ACTOR** (user-ruled
 * 2026-08-20). The AI decides it, realisation spawns or renames the water body to match, and
 * **omitting it means the default name** — which is what a body someone dragged out by hand
 * projects as, since Unreal's own `WaterBodyRiver`/`WaterBodyRiver2` says nothing a document is
 * better for carrying. So an unnamed entry is not an incomplete one; it is the ordinary state of
 * reflected water.
 */
struct GRAPHCODE_API FGcWaterBody
{
	/** Empty means *use the default name* — see the struct comment. Never a refusal. */
	FString Name;

	EGcWaterKind Kind = EGcWaterKind::River;

	/**
	 * `along=$Valley.spine` (a river's course) or `in=$Basin` (a lake's outline) — terrain's own
	 * spatial language, parsed by terrain's own code, so a water line and a terrain line say a place
	 * with the same characters because they run the same function ([[one-concept-two-grammars]]).
	 *
	 * Empty on a reflected body, which states its geometry as `spline { }` instead. The two forms
	 * differ ONLY here: profiles and the long tail are identical grammar, because intent and capture
	 * are one object at two resolutions (§12i.4).
	 */
	FGcTerrainDomain Domain;

	/**
	 * `[override=$Bend.WaterSpline]` — the level `.gc`'s `splines { }` entry that owns this body's
	 * COURSE, once a person dragged it away from what `along=`/`in=` draws. Held without the `$`.
	 *
	 * **THIS REPLACES CAPTURING THE POINTS, and the change is a user ruling** (2026-08-20): *"why is
	 * there two splines? it should only be one and it should be in .gc. the river etc refer to gc
	 * spline."* The old capture copied every control point into `Spline` below and pushed the intent
	 * into a `#` comment, so a hand-drawn river's numbers sat in TWO files — the level's own
	 * `splines { }` writes every spline component and has no water exclusion.
	 *
	 * **THE INTENT STAYS ON THE LINE BESIDE IT.** `along=$Valley.spine [override=$Bend.WaterSpline]`
	 * says both *what was asked for* and *what is actually there*, which a `#` comment could not.
	 * And REMOVING the override is how an author says *put my course back* — the reset this grammar
	 * had no way to spell (`GC-TERRAIN-DESIGN.md` §12i.4a, rule in
	 * `GC-WORLD-GENERATION-DESIGN.md` §13.4a).
	 */
	FString Override;

	/** `level=` — the water surface's world Z in centimetres. A lake and an ocean are flat. */
	bool bHasLevel = false;
	double Level = 0.0;

	/**
	 * `carves=true` — `bAffectsLandscape`, i.e. **which of the two machines cuts the bed** (§12i.5).
	 *
	 * False (the default) means water only fills and the AI writes a terrain `carve()` itself, which
	 * is ordered against every other instruction. True hands the cut to the plugin's own brush, which
	 * buys its blend modes, falloff curves and bank painting at the price of landing as one
	 * indivisible edit layer named `Water` that our whole program sits above or below (§12i.11).
	 *
	 * **A PROPERTY, NOT A `[modifier]`.** `bAffectsLandscape` is an authored `UPROPERTY`; brackets in
	 * this language carry what nobody authored (`nav(…) [stale]`). Getting this backwards was caught
	 * by the user's own children/properties/name rule, which is why the rule is worth stating twice.
	 */
	bool bHasCarves = false;
	bool bCarves = false;

	/**
	 * `width=(0=380, 0.5=560, 1=420)` — terrain's shipped profile spelling, arc-length keyed, reused
	 * rather than reinvented ([[a-new-token-not-a-new-line]]).
	 *
	 * Four curves exist and the design named three: `UWaterSplineMetadata` carries `Depth`,
	 * `RiverWidth`, `WaterVelocityScalar` **and `AudioIntensity`** (§12i.8/4), all
	 * `FInterpCurveFloat`, so `audio` is here for the cost of a word.
	 */
	TArray<TPair<FString, TArray<TPair<double, double>>>> Profiles;

	/**
	 * `zone=Main` — the `AWaterZone` this body belongs to, when it does not take the one it sits in.
	 *
	 * **A BARE NAME, NOT `$Main`.** The mark means *declared in this `.gcw`* and reads as a region;
	 * §18.3c's rule is that an unmarked name is an ACTOR, and a zone is exactly that. Empty is the
	 * ordinary state — `OwningWaterZone` is derived from overlap and the zone's `OverlapPriority`,
	 * and only `WaterZoneOverride` is authored.
	 */
	FString Zone;

	/**
	 * `bodies=(Rill, Sea)` — on an `exclude`, the bodies its mode adds or removes.
	 *
	 * **FIRST-CLASS RATHER THAN LEFT IN THE TAIL, because these are IDENTITIES and not values.** A
	 * misspelt material is a missing material; a misspelt body name is an exclusion volume that
	 * silently excludes nothing, which looks exactly like one that works. So the names are held
	 * where the reader can check them against the block they were written beside.
	 *
	 * A body with no name of its own cannot be listed here, and that is a real limit rather than an
	 * oversight: an entry that took the engine's name has not said which one it is.
	 */
	TArray<FString> Bodies;

	/**
	 * Everything else, by name and in authored order, with the value held as the text it was
	 * written as — materials, `priority`, `blend_radius`, `carve=(…)`, `waves=gerstner(…)`, the
	 * weightmap paints, the underwater post-process, nav and collision.
	 *
	 * **RAW TEXT ON PURPOSE, AND ONLY FOR THE LONG TAIL.** The user's ruling for this block is
	 * *surface as many properties as possible and curate nothing*, so the tail is open-ended by
	 * design and will be produced and consumed by AuthoringCore's value codec against the CDO
	 * ([[gc-property-bag-codec]]) — which is canonical by construction. Typing it twice here would
	 * be a second spelling of every value in the engine. What IS typed above is the SHAPE: the
	 * things a person thinks in, and the things this reader must refuse a typo in.
	 */
	TArray<TPair<FString, FString>> Args;

	/** True when the entry opened a body at all, so an empty `{ }` round-trips as one. */
	bool bHasBody = false;

	bool bHasSpline = false;
	TArray<FGcWaterPoint> Spline;

	/** Whatever sits above `spline {`, and the comments left under its last point. */
	TArray<FString> SplineLeading;
	TArray<FString> SplineTrailing;

	/**
	 * The `# …` on the `spline {` line itself.
	 *
	 * Its own field rather than dropped, because a reader that drops a comment is stable AND lossy
	 * at once — both passes lose the same words, so a byte-identity gate stays green while the file
	 * quietly flattens. That is exactly how the terrain block lost its headings for a day.
	 */
	FString SplineNote;

	/** Comments at the bottom of the entry's own body, belonging to no point. */
	TArray<FString> BodyTrailing;

	/** The author's own grouping — full lines above this entry, `#` comments and blanks, in order. */
	TArray<FString> Leading;
	FString Trailing;

	bool HasProfile(const TCHAR* Key) const
	{
		return Profiles.ContainsByPredicate(
			[Key](const TPair<FString, TArray<TPair<double, double>>>& Pair)
			{ return Pair.Key == Key; });
	}

	const FString* Arg(const TCHAR* Key) const
	{
		for (const TPair<FString, FString>& Pair : Args)
		{
			if (Pair.Key == Key)
			{
				return &Pair.Value;
			}
		}
		return nullptr;
	}
};

/**
 * `water { … }` — the whole block.
 *
 * **A BARE KEYWORD WITH NO NAME**, like `populate { }` and unlike `terrain Overworld { }`: a terrain
 * program names the landscape it shapes, and there is one per landscape, but water names nothing —
 * the zone is an ACTOR with its own entry in the level's `.gc` (L0a again, §12i.8/7), not a thing
 * this block is about. A file has one `water { }` and a name would be a second identity for
 * something already identified by being the only one.
 */
struct GRAPHCODE_API FGcWaterBlock
{
	TArray<FGcWaterBody> Bodies;

	/**
	 * The `# …` on the `water {` line itself.
	 *
	 * **HERE RATHER THAN IN THE DOCUMENT'S TRIVIA MAP**, which is where the head's LEADING lines go.
	 * The map's `Trailing` is written by the region reader and read by nobody, so a note put there
	 * is stored and dropped — and both passes dropping it keeps a byte-identity gate green while
	 * the file quietly loses a line.
	 */
	FString HeadNote;

	/**
	 * Comments left at the BOTTOM of the block, under every entry and inside the closing brace.
	 *
	 * Its own field because it belongs to no entry: attaching it to the last one would move it when
	 * that entry moved, and a note about the water as a whole would follow one river up the file.
	 */
	TArray<FString> Trailing;

	bool IsEmpty() const
	{
		return Bodies.IsEmpty() && Trailing.IsEmpty() && HeadNote.IsEmpty();
	}
};

namespace GcWater
{
	/** `river` / `lake` / `ocean` / `pool` / `island` — the word this kind is written as. */
	GRAPHCODE_API const TCHAR* KeywordOf(EGcWaterKind Kind);

	/** The inverse. False when the word names no kind, which is a refusal and never a default. */
	GRAPHCODE_API bool KindFromKeyword(const FString& Word, EGcWaterKind& OutKind);

	/**
	 * The four per-point curve keys, which are also the four profile keys — `width`, `depth`,
	 * `velocity`, `audio`.
	 *
	 * ONE LIST FOR BOTH SPELLINGS, because they are one set of quantities written two ways: on a
	 * point when the spline is captured, as a profile when the body follows a named course. Two
	 * lists would let the two drift and a reader would have to know which it was looking at.
	 */
	GRAPHCODE_API const TArray<FString>& CurveKeys();

	/** `curve` (the default, never written), `linear`, `clamped`, `custom`. */
	GRAPHCODE_API bool IsLegalPointType(const FString& Word);
}
