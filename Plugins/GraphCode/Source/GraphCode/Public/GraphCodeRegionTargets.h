// R2 — the target producer. A `.gcw`'s names become placement targets, and the resolver is untouched.
//
// §21.12's R2, and the whole point of it is that it adds NO mechanism. `Resolve` already takes
// `TMap<FString, FGcPlacementTarget>`; a region, a wall and a corner are frames-with-or-without-a-
// volume, which is the object that map already holds. P3 proved the thin case when a socket turned
// out to be *the frame-without-a-volume the struct already carries*. An edge is the same object.
//
// THREE THINGS THIS FILE DECIDES, none of which §21 states:
//
//   1. **A TARGET'S FORWARD POINTS INTO THE REGION.** User-ruled 2026-08-11: *an anchor is a point on
//      an edge, and forward of it is the tangent of the point moving into the region.* So the frame's
//      +X is the INWARD normal, and `forward=100` off a wall walks into the room rather than through
//      it. `FGcRegionEdgeFrame::Outward` is untouched and still means what it says — this negates it
//      when building the target, so the kernel keeps one truth and the placement surface gets the
//      direction an author means. Nothing shipped changes: no target for `$Crypt.north` existed
//      before this file.
//
//   2. **A STATION IS PARSED HERE, BECAUSE HERE IS THE USE SITE.** Same ruling: there is no `@`
//      inside a `.gcw`, so `FGraphCodeRegionFile` refuses one and this file accepts one.
//      `$Crypt.e2@t=1500` is an edge and a station, and the pair is the target — not a percentage of
//      anything, which is why an edge needs no volume for it to work.
//
//   3. **THE ENUMERABLE SET AND THE CONTINUOUS ONE ARE DIFFERENT CALLS.** `Build` returns every name
//      a document can list — regions, edges by minted name and by alias, corners. `@t` is continuous
//      and cannot be enumerated, so `ResolveOne` exists for the names a payload actually asks for.
//      A `Build` that tried to guess parameters would be inventing places nobody wrote.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"
#include "GraphCodeSpatialPlacement.h"

/** Refusal tokens for the producer, so a caller branches on the reason and not on the sentence. */
namespace GcwTargetToken
{
	inline constexpr const TCHAR* BadParameter = TEXT("gcw_bad_edge_parameter");
	inline constexpr const TCHAR* ParameterNeedsEdge = TEXT("gcw_parameter_needs_edge");
}

/**
 * What an author wrote after `@` — §22.9f, user-ruled 2026-09-12.
 *
 * **`@t=34%` IS A SHARE OF THE RUN AND `@t=1500` IS ABSOLUTE**, which reverses what a bare number in
 * this slot used to mean and gives it a key it never had. Both halves were measured rather than
 * chosen:
 *
 *   * **A BARE NUMBER IS ABSOLUTE EVERYWHERE ELSE IN GRAPHCODE** — Shape's `w=40.0` and `step=`,
 *     region points, `min:`/`max:`, `[round=25]`, `@anchor[forward=-50]`, terrain heights — and `%`
 *     is already the established explicit fraction, 439 literals of it in the committed corpus.
 *     *Bare means fraction* was the odd one out.
 *   * **AND NO UNIT IS SPELLED** (user, *"no more cm or m"*). None of those slots writes one, so a
 *     `cm`/`m` suffix here would have been the only unit in the language.
 *
 * **THE MIGRATION COSTS NOTHING**: there is not one `@` token in any committed `.gcw` or `.gc`,
 * measured 2026-09-12. The corpus for this slot is the test suite.
 *
 * **PROFILE STOPS ARE UNTOUCHED AND THAT IS NOT AN INCONSISTENCY**: a profile is defined over 0..1 by
 * its nature and its keys have no unit they could be confused with. A `t` along a real run does.
 */
enum class EGcwStationKind : uint8
{
	/** `@t=34%` — a share of the run's length, 0 at one end and 1 at the other. */
	Fraction,

	/** `@t=1500` — absolute distance from the run's start, in its declared direction. */
	Distance,

	/** `@t=p4` — a point the run passes through, whose `t` only the edge itself can supply. */
	Point,
};

/** One parsed station. Exactly one of `Value` and `PointName` carries the request. */
struct GRAPHCODE_API FGcwStation
{
	EGcwStationKind Kind = EGcwStationKind::Fraction;

	/** The fraction 0..1, or the absolute distance. Which one `Kind` says. */
	double Value = 0.0;

	FString PointName;
};

/** The `.gcw` half of the placement surface: names in, `FGcPlacementTarget`s out. Pure. */
class GRAPHCODE_API FGraphCodeRegionTargets
{
public:
	/**
	 * Every name this document can LIST, as targets, keyed exactly as an author writes them.
	 *
	 * `$Crypt`, `$Crypt.e2`, `$Crypt.north` (the alias, as its own key pointing at the same wall),
	 * `$Crypt.p2`. A shared edge appears under BOTH regions and the two entries face opposite ways,
	 * which is §21.2's B4 falling out of each loop being normalised on its own rather than being
	 * arranged for here.
	 *
	 * Askable is not listable (§14.1): `@t` is askable through `ResolveOne` and appears in nothing.
	 */
	static bool Build(const FGcwDocument& Document, TMap<FString, FGcPlacementTarget>& OutTargets,
		FString& OutRefusal);

	/**
	 * One name, including the forms `Build` cannot enumerate.
	 *
	 * `$Crypt.e2@t=25%` and `$Crypt.north@t=50%` — an edge and a parameter 0 to 1 along it, in the
	 * edge's own declared direction. Everything `Build` produces also resolves here.
	 */
	static bool ResolveOne(const FGcwDocument& Document, const FString& Name,
		FGcPlacementTarget& OutTarget, FString& OutRefusal);

	/**
	 * Split `$Crypt.e2@t=1500` into `$Crypt.e2` and a station. False leaves `OutBase` as `Name`.
	 *
	 * **THE KEY IS PART OF THE STATION AND THERE ARE THREE VALUE FORMS** — `@t=34%`, `@t=1500` and
	 * `@t=p4`. What tells them apart is the `%` and `IsNumeric` together; `IsNumeric` alone used to
	 * carry the whole discrimination and could only do so while a bare number meant a fraction.
	 *
	 * **A BARE `@34%` IS REFUSED RATHER THAN READ AS A SECOND SPELLING.** This slot already carries
	 * three value forms, and one whose key is ALSO optional is one where a reader cannot tell a
	 * station from whatever else somebody later hangs off an `@`.
	 *
	 * **WHY A NAME IS WORTH A FORM: on a chain, a fraction cannot reach a bend.**
	 * `north = p3 -> p4 -> p5` is ONE wall and a station runs by arc length over the whole of it, so
	 * the `t` at `p4` is a ratio of two segment lengths — a number nobody can write down and that
	 * moving `p5` silently changes. `@t=p4` is that place, stated as the thing it is.
	 *
	 * A DISTANCE AND A NAME BOTH NEED THE EDGE, which this does not have; `StationT` is where both
	 * become a `t`. Public because a caller collecting the `$` names out of a payload needs to know
	 * which ones are parameterised before it can decide whether `Build`'s map already answers them.
	 */
	static bool SplitParameter(const FString& Name, FString& OutBase, FGcwStation& OutStation,
		FString& OutError);

	/**
	 * One station on one run, as the `0..1` every frame function downstream takes.
	 *
	 * **THE CONVERSION LIVES HERE AND NOWHERE ELSE**, because a distance and a named bend both need
	 * the edge's own length and both were otherwise going to be worked out at each call site. A
	 * fraction passes through untouched, which is what keeps the three forms one request.
	 *
	 * **A DISTANCE IS MEASURED IN PLAN**, along the same arc length `EdgePolyline` parameterises by,
	 * so `@t=1500` on a run that climbs is 1500 across the map and not 1500 of slope. Every other
	 * reader of a `t` on this surface already measures that way; a second ruler would put two answers
	 * about one place in the file.
	 *
	 * A distance PAST THE END refuses rather than clamping: `@t=9400` on a 9,000 run is an author who
	 * has the length wrong, and a silently clamped station is a house on top of the last one.
	 */
	static bool StationT(const FGcRegionSpace& Space, int32 EdgeIndex, const FGcwStation& Station,
		double& OutT, FString& OutError);

	/** What `MeasureStation` found. */
	enum class EStation : uint8
	{
		/** Not a whole course — a wall, a landmark, an actor, or a name that already carries a station. */
		NotACourse,
		Measured,
		Refused,
	};

	/**
	 * Where along a COURSE a point stands — §22.17's inverse, and the one door both the `.gcs` generator
	 * and `gcw_ask` walk through, so what one writes the other reads.
	 *
	 * **THROUGH `ResolveOne`, the door the level writer uses.** An entry is lowered to
	 * `@anchor[target=<course>@t=<text>]`, so this resolves the very TEXT it hands back and returns the
	 * frame that came out; measuring against `OutAtStation` undoes exactly what the apply will do. The
	 * search only chooses which text. It is seeded by the run's nearest point IN PLAN — a house beside a
	 * climbing road belongs to the piece of road beside it — and refined for a bowed course, whose
	 * tessellation runs short of the true arc by the chord error.
	 *
	 * **A WHOLE COURSE ONLY.** A wall keeps its middle: its frame faces into the room, and wall-anchored
	 * slices were never asked to change.
	 */
	static EStation MeasureStation(const FGcwDocument& Document, const FString& Course,
		const FGcPlacementTarget& Run, const FVector& Point, FString& OutStation,
		FGcPlacementTarget& OutAtStation, FString& OutRefusal);

	/**
	 * Along a course, a leftover under this after a printed station is the print and not a departure.
	 * `FloatToText` spends nine significant digits, so a station can name a frame a few micrometres from
	 * the true nearest point; a member standing past an end is metres out and keeps its `forward=`.
	 */
	static constexpr double StationPrintResidue = 1.e-3;
};
