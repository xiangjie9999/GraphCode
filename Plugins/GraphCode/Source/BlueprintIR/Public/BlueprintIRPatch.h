// Blueprint IR - the STATEMENT-LEVEL PATCH, and it is the retirement of ADR-7 Rule 2.
//
// Rule 2 said the payload is the complete graph and omission is deletion, so a caller who wanted
// to change one pin inside a 214-line function had to echo the whole function back. That was the
// simplest rule while there was no way to say "remove this node", and it stopped being the simplest
// rule the moment there was one. USER-DIRECTED 2026-08-30:
//
//   "the patch mode should allow just a node change, like addition of just a node because any
//    deletion needs an explicit delete operation, so the old 'you need to produce the whole graph'
//    concept is outdated, now if index and node matches, it's a change, if not, it's an add"
//
// So under `omission:"patch"` the payload is a FRAGMENT and this file is what turns it back into
// the complete graph the writer has always taken. Nothing downstream changes: the merged graph goes
// into the same differential apply, the same truncation guard, the same layout pass, and the same
// deletion pricing. That is deliberate - a merge that produced a complete graph is a merge whose
// output every existing guard still checks.
//
// THREE RULES, and the third is the one that costs a refusal rather than a guess:
//
//   alias is not live                 -> ADD. A new node, placed by the layout pass.
//   alias is live, same node class    -> CHANGE. That node, mutated in place.
//   alias is live, different class    -> REFUSED, both readings named.
//
// The third case is where the user's "if not, it's an add" cannot be taken literally, because an
// add whose alias is already taken is not an add - it is two nodes claiming one identity. Under
// snapshot that line meant "destroy and rebuild at the old position" (ADR-7 Rule 1) and it could,
// because the payload was the whole graph and the old node was leaving anyway. Under patch nothing
// leaves without being named, so the same line has two readings that differ by a destroyed node.
// It is refused with both spelled out, which is the same ruling `rename` got at block depth
// (GC-OMISSION-CONTRACT-DESIGN.md §6.4): the fallback is not a better guess, it is a
// delete-plus-create nobody asked for.
//
// WHAT A REMOVAL COSTS IS REPORTED, NEVER SILENT. `delete n42` breaks every wire into and out of
// n42, exactly as deleting it in the editor would, and every broken wire is named in the report.
// Refusing instead was the first design and it was wrong: it would make `delete` unusable on any
// node with a consumer, which is nearly all of them.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintIRTypes.h"

/**
 * What a merge did, so the caller can report it rather than infer it.
 *
 * A patch is the one write where "nothing was said about it" and "it was left alone" are the same
 * bytes, so the counts below are the only place that distinction is visible. `Carried` in
 * particular is what stops a one-line payload reading as a one-node graph.
 */
struct BLUEPRINTIR_API FBpPatchMergeReport
{
	/** Aliases the payload restated over a live node of the same class. */
	TArray<FString> Changed;

	/** Aliases the payload introduced. These get their final numbering from the applier. */
	TArray<FString> Added;

	/** Aliases a `delete` line removed. */
	TArray<FString> Removed;

	/** Live statements the payload never mentioned, carried into the merged graph untouched. */
	int32 Carried = 0;

	/**
	 * One line per wire a removal broke, as `n50.Value read n42.ReturnValue`.
	 *
	 * Reported and then done: the wire is gone in the merged graph, so the write that follows is
	 * the one the caller can see here rather than one it finds out about from a compile error.
	 */
	TArray<FString> BrokenLinks;

	/** Non-empty means nothing merged and the caller must refuse. */
	TArray<FString> Refusals;

	/**
	 * Did the merged graph end up holding anything the caller did not send?
	 *
	 * The reporting predicate, and it is deliberately NOT "did anything happen". A COMPLETE payload
	 * changes every statement in it and expands to itself, so reporting on `Changed` would put a
	 * merge summary in front of every whole-block apply in the project — noise on the route that
	 * has worked for months. What a caller cannot see for themselves is what was CARRIED and what
	 * was REMOVED, and those are exactly the two a complete payload has none of.
	 */
	bool ExpandedThePayload() const { return Carried > 0 || !Removed.IsEmpty(); }
};

namespace BlueprintIR
{
	/**
	 * Merge a PATCH payload over the live projection of the same graph.
	 *
	 * `Live` must be the projection this patch was computed against - the block text `gc_read`
	 * handed out, or a fresh canonical read of the same graph. The aliases in it are the identities
	 * the payload is addressing, so a `Live` from anywhere else silently re-points every line.
	 * `base_sig` one level up is what proves it is still current; this function does not re-check.
	 *
	 * Returns false with `OutReport.Refusals` filled and `OutMerged` untouched. Returns true with
	 * `OutMerged` holding the COMPLETE graph, which is what every caller downstream expects.
	 *
	 * Recursive into `SubGraphs`: a collapsed graph the payload restates is merged over the live
	 * one of the same `OwnerAlias`, and one the payload never mentions is carried whole. Without
	 * that, restating a composite node's statement alone would empty the graph it owns.
	 */
	BLUEPRINTIR_API bool MergeStatementPatch(const FBpIrGraph& Payload, const FBpIrGraph& Live,
		FBpIrGraph& OutMerged, FBpPatchMergeReport& OutReport);

	/**
	 * The refusal a `delete` line earns under `omission:"snapshot"`, where the payload is already
	 * the whole set and omitting the statement says the same thing.
	 *
	 * Here rather than at the call site because both graph routes need the same sentence, and
	 * because it is the exact mirror of the block-depth rule `FGraphCodeGraphLifecycle` applies to
	 * `delete <Name>`.
	 */
	BLUEPRINTIR_API FString StatementDeleteUnderSnapshotRefusal(const FString& Alias);
}
