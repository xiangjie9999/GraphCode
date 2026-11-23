// Copyright OraStudio. All Rights Reserved.

// W0 — MEMBERSHIP. Which actors a `.gcs` declaration's groups claim, and it is PURE.
//
// `GC-SPATIAL-AUTHORING-DESIGN.md` §18.3f (the rulings this could not be built without) and §18.3i
// (the flat grammar). The declarations themselves parse in `GraphCodeRegionFile`; this file answers
// what they SELECT.
//
// **§18.3i DELETED HALF OF WHAT THIS FILE USED TO DO, AND THAT IS THE POINT.** A group used to narrow
// a document's `track:` mouth, so a `Compose` folded the two into the selector that actually ran, and
// a `Select` could report the mouth's remainder — the actors in the room that matched nothing. With
// no mouth, a group's selector IS the selector, and the composition rules §18.3e worked out (per-field
// override and the radius pair as one field) do not need fixing because there is
// nothing left for them to compose.
//
// **`untracked` IS BACK, USER-DIRECTED 2026-08-13.** Flattening removed the mouth that used to define
// the document's wider scope. The replacement is derived only from EXPLICIT information: the file's
// tracked universe is the UNION of every `in:` scope its flat group lines state. An actor in that
// universe which no group claims is untracked. A line with no `in:` still selects normally but cannot
// silently widen the completeness claim; when no line states `in:`, the document honestly keeps
// `omits: untracked` instead of rendering an empty block that would claim completeness.
//
// SAME SEAM AS THE PLACEMENT RESOLVER, and deliberately: a caller resolves names to
// `FGcPlacementTarget` values and hands over a flat list of candidates, so every ruling below is
// assertable with no map loaded. The world-walking half — `IsProjectedActor` for the candidate set,
// the level's actors as targets — is `GraphCodeLevelPlacement`'s shape one document over, and it
// supplies inputs rather than answering questions. Containment needs no world at all: a region is
// authored geometry and `FGraphCodeRegion::ContainsPoint` is arithmetic.
//
// THREE RULINGS THIS FILE IS BUILT ON, none of which are its own to change:
//
//   1. **AN ACTOR IN TWO GROUPS IS WRITTEN IN BOTH** (§18.3f.1, user-ruled 2026-08-13, superseding
//      §18.11's *reported and never arbitrated*). A `.gcs` is a TRACKER: two measurements of one
//      actor from two anchors are both true, and a tracker that suppressed one would hide a fact
//      about the level to protect an apply that has its own ordering rule. So there is no contested
//      list here to read — the actor appearing twice IS the report.
//
//   2. **MEMBERSHIP MEASURES THE ACTOR'S STORED LOCATION** (§18.3f.3) — its pivot, wherever the
//      artist put it — for `in:` and for `near:` alike. ONE point for the whole
//      filter line is the reason: two filters on one line answering about two different points on
//      one actor is the kind of split that gets discovered rather than read. It is also what `in:`
//      already tested before this file existed, and what `gc_search` measures for the same word
//      (`GraphCodeSpatialQuery.cpp:416`), so `near:` means one thing in both surfaces.
//
//      **THIS IS NOT §20.2's MIDPOINT AND THE DIFFERENCE IS INTENDED.** Placement asks *how do two
//      solid things fit together*, where a pivot is an artist's accident that must not reach the
//      arithmetic; membership asks *where is it*, where the stored location is the answer the engine
//      and the outliner already give. §18.3f.4 rules that the point becomes sayable — `mid`,
//      `bottom`, `top` — with the pivot as the default when nobody says; that parameter is NOT built
//      and this file is its default.
//
//   3. **THE CANDIDATE SET IS THE LEVEL'S OWN** (§18.3e) — `IsProjectedActor`, enforced by the
//      caller because only the caller has a world. A `.gcs` exists to be applied back, so tracking
//      something the level document cannot name would write entries that can never be applied.

#pragma once

#include "AuthoringCoreTypes.h"
#include "CoreMinimal.h"
#include "GraphCodeRegionFile.h"
#include "GraphCodeSpatialPlacement.h"

/** Refusal tokens, so a caller branches on the reason and not on the sentence. */
namespace GcsToken
{
	/** `near:` naming something neither the `.gcw` nor the level knows. */
	inline constexpr const TCHAR* UnknownAnchor = TEXT("gcs_unknown_anchor");
}

/**
 * One actor as membership sees it — a DESCRIPTION rather than an actor, which is what keeps this pure.
 *
 * `Placed` carries the frame and the box because the same struct is what §18.9's `Measure` takes when
 * the entry's values are computed a step later. Two structs would be two chances for the set that got
 * selected and the set that got measured to disagree about which actor is which.
 */
struct GRAPHCODE_API FGcsCandidate
{
	/** The level document's handle, and what an entry is written as. */
	FString Name;

	/** Matched by `name:` as well — §18.3b defers to `.gcp`'s selector, which takes a name OR a label. */
	FString Label;

	/** `class:BP_Minion`. */
	FString ClassName;

	/**
	 * Every legal short/path token in the actor's superclass chain.
	 *
	 * Empty keeps the pure fixture's historical exact-token behaviour. A live collector fills this
	 * so `class:BP_Minion` selects instances of `BP_Minion_C` and its derived classes instead of
	 * comparing a Blueprint asset token to an Unreal generated-class suffix and matching nothing.
	 */
	TArray<FString> ClassNames;

	TArray<FString> Tags;

	FGcPlacementTarget Placed;

	/**
	 * Canonical deviations from the actor's archetype, before a declaration's `fields:` list narrows
	 * them. The generator keeps a property when its top-level path is named by that list. Keeping the
	 * values on the same description membership selected prevents a second actor walk from measuring
	 * a different instant of the level.
	 */
	TArray<FAuthoringProperty> Properties;
};

/** The membership half of a `.gcs`. Pure, static, and asserted with no editor. */
class GRAPHCODE_API FGraphCodeSliceMembership
{
public:
	/**
	 * The point every filter measures — the actor's STORED LOCATION (§18.3f.3, this file's note 2).
	 *
	 * A named function rather than an inlined `Frame.GetLocation()` at four call sites, because the
	 * ruling is *one point for the whole filter line* and four call sites is four chances for that to
	 * stop being true. §18.3f.4's `mid`/`bottom`/`top` parameter lands here when it is built.
	 */
	static FVector MembershipPoint(const FGcsCandidate& Candidate);

	/**
	 * One candidate against one composed selector. The unit the specs assert.
	 *
	 * `Targets` answers `near:` and is the resolver's own map — `$Crypt.door` and `BP_Boss_01` in one
	 * namespace, exactly as `Resolve` takes them. `Document` answers `in:`, which is a region and
	 * never an actor (§18.3c), so the two lookups cannot be confused for each other.
	 *
	 * AN UNSTATED FILTER FILTERS NOTHING, which is what makes an empty selector match everything and
	 * a group of one a group.
	 */
	static bool Matches(const FGcwDocument& Document, const FGcwSelector& Selector,
		const FGcsCandidate& Candidate, const TMap<FString, FGcPlacementTarget>& Targets,
		bool& bOutMatches, FString& OutRefusal);

	/**
	 * A whole declaration against a whole candidate list.
	 *
	 * `OutGroups` IS INDEX FOR INDEX WITH `Declaration.Groups`, which is the only identity a group has
	 * left (§18.3i): the name went because nothing read it, and a parallel list is what remains when
	 * the thing being described has no name to key on. A group claiming nobody is an empty entry and
	 * not a missing one, so the two arrays never come apart.
	 *
	 * EACH LIST IS SORTED BY NAME, not left in the order the world walk found them. An actor iterator's
	 * order is not stable across sessions and a `.gcs` is regenerated constantly, so level order would
	 * put a diff in the file every time nothing had changed.
	 *
	 * **AN ACTOR CLAIMED BY TWO GROUPS IS IN BOTH LISTS** — this file's opening note 1. Call the
	 * overload below when the generated slice also needs its explicit-scope `untracked` remainder.
	 */
	static bool Select(const FGcwDocument& Document, const FGcwDeclaration& Declaration,
		const TArray<FGcsCandidate>& Candidates, const TMap<FString, FGcPlacementTarget>& Targets,
		TArray<TArray<FString>>& OutGroups, FString& OutRefusal);

	/**
	 * The same selection plus the file-level remainder restored after §18.3i flattened the grammar.
	 *
	 * `bOutScoped` says whether at least one group stated `in:`. When false, `OutUntracked` is empty
	 * because the file has no declared universe to be complete about — it does NOT mean every actor
	 * was tracked. Declaration-wide exclusions are removed before both group claims and this list.
	 */
	static bool Select(const FGcwDocument& Document, const FGcwDeclaration& Declaration,
		const TArray<FGcsCandidate>& Candidates, const TMap<FString, FGcPlacementTarget>& Targets,
		TArray<TArray<FString>>& OutGroups, TArray<FString>& OutUntracked, bool& bOutScoped,
		FString& OutRefusal);
};
