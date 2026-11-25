// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;

/**
 * One comment box as a document states it: the alias that names it, its text, and what it encloses.
 *
 * `Covered` and `Anchor` are INDICES into the node array the caller passed, never pointers, so this
 * struct names no node type and every kind can use it with its own.
 */
struct GRAPHCODE_API FGraphCodeCommentBox
{
	/** `c1`, `c2`, … in the deterministic order below. Block-scoped, the way a guard alias is. */
	FString Alias;

	FString Text;

	FLinearColor Color = FLinearColor::White;

	/** Indices into the caller's `NodesInOrder`, ascending — so itself in the caller's order. */
	TArray<int32> Covered;

	/**
	 * `Covered[0]`: the enclosed node that comes FIRST in the caller's order, which is the node the
	 * box renders above. `INDEX_NONE` when the box encloses nothing, and a caller must handle that
	 * rather than assume every box has a home — an author may draw a box over empty canvas.
	 */
	int32 Anchor = INDEX_NONE;
};

/**
 * Every comment box in an authored graph, ordered, with what each one encloses.
 *
 * **This is the shared body `GC-AI-AUTHORING-DESIGN.md` §5.1 measured**, lifted out of
 * `kind:behavior_tree` so `kind:env_query` uses it rather than a second copy. It names no AI type —
 * a comment box is `UEdGraphNode_Comment` and containment is a rectangle, so nothing here knows
 * which kind is asking, and Core needs no module it did not already have.
 *
 * **Order is Y, then X, then GUID, and all three matter.** Y-then-X is reading order, which is what
 * makes `c1` the top-left box rather than whichever one the graph happens to store first. The GUID
 * is the tie-break that keeps two boxes at the same point from swapping aliases between reads and
 * moving a `sig:` that nothing authored.
 *
 * **Containment is the node's ORIGIN, not its rectangle** — a node counts as inside when its
 * top-left corner is within the box, which is the rule the Behavior Tree codec shipped and the one
 * the editor's own drag-select matches. A node overhanging the right edge is still inside.
 *
 * @param Graph         the authored graph; comment boxes are ordinary nodes in it
 * @param NodesInOrder  the nodes the caller is projecting, in the caller's OWN precedence order —
 *                      `Anchor` is defined by that order, so a caller passing them unsorted gets an
 *                      anchor that is merely the first one it happened to list
 * @param OutBoxes      reset and filled; empty when the graph has no boxes
 */
GRAPHCODE_API void GraphCodeCollectCommentBoxes(
	const UEdGraph* Graph,
	TArrayView<UEdGraphNode* const> NodesInOrder,
	TArray<FGraphCodeCommentBox>& OutBoxes);
