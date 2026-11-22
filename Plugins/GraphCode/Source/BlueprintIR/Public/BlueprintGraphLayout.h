// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UEdGraph;
class UEdGraphNode;
class UEdGraphNode_Comment;

/**
 * Headless layout for Blueprint graphs.
 *
 * Nodes created over MCP land wherever the caller guessed, which is usually a stack at
 * the origin or a hand-computed grid that stops being right the moment a node gains a
 * pin. This lays them out the way a person would: exec flow left to right, each node to
 * the right of everything that feeds it, one band per disjoint chain.
 *
 * It lives in BlueprintIR rather than in UnrealClaude because a graph rebuilt from IR has
 * no positions AT ALL and needs exactly this, and because a second layout implementation
 * is the kind of thing that quietly diverges until `organize_nodes` and `bp_apply` produce
 * different graphs from the same nodes. There is one organiser. It has no MCP dependency
 * and never had one - only UEdGraph - so moving it down cost nothing.
 *
 * It is deliberately NOT the engine's own arrangement code. UE's alignment commands live
 * on SGraphEditor and need an open graph editor tab with a selection; every MCP call
 * runs headless against the asset, so there is nothing to ask for a widget's size. Node
 * extents are therefore ESTIMATED (see EstimateNodeSize) and spacing is generous enough
 * that an estimate being wrong by a pin's height cannot cause an overlap.
 *
 * Layering tolerates cycles. Blueprint exec flow is not acyclic - a loop body wiring back
 * into its own condition is ordinary - so back-edges are detected during the depth-first
 * pass and excluded from the longest-path computation rather than being allowed to
 * inflate the column count or hang the loop.
 *
 * NO NODE SITS LEFT OF SOMETHING THAT FEEDS IT, and that holds for a data wire exactly as
 * it does for an exec one. A pure chain hanging off the spine - `Save Project` -> `Append`
 * -> `Select` -> `Print String` - therefore gets its own columns BETWEEN the two spine
 * nodes, pushing the consumer as far right as the chain is long, rather than being folded
 * back into the one column of slack the exec flow happened to leave.
 *
 * A COLUMN IS A SINGLE LEFT EDGE. Every node at a given depth starts on it, and the
 * gutter to the next column is the same rule everywhere: the widest node in the left
 * column plus HorizontalSpacing. Data-only ("pure") nodes - variable gets, math, selects,
 * anything with no exec pins - are not part of the spine, but they take a column too:
 * counted BACK from the consumer they feed, so a bare get sits against the call that uses
 * it instead of at the far left, and then pushed forward again off their own producers, so
 * a chain of them cannot land on top of the spine that feeds it. A fan-in shares one edge
 * with the spine's own grid. They used to be placed at `consumerX - spacing - theirOwnWidth`, which
 * right-aligns them: two nodes feeding one call landed on two different left edges purely
 * because they were different widths, the ragged edge made them overlap, and the collision
 * slide then pushed each one lower than the last - the diagonal smear a deep pure chain
 * produced. A component with no exec-bearing node at all reads its columns forward off its
 * own data flow, because there is no consumer to count back from.
 *
 * ROW ORDER IS PIN ORDER, in both directions. Along the spine it is the BRANCH RANK: the
 * lexicographic depth-first path from the roots that descends each node's successors in
 * output-pin order, so a fork's upper lane and its whole subtree sit above the lower lane's,
 * several columns deep, and two lanes equally far from their fork cannot tie the way a
 * barycentre average did (a tie fell through to a GUID, and which lane landed on top was
 * arbitrary). In the data fan-in it is the INPUT PIN the node feeds: the feeder of pin 0
 * sits above the feeder of pin 1, which is how the wires are drawn.
 *
 * Rows are then aligned PIN TO PIN rather than centre to centre - a node starts level with
 * the row of the pin that feeds it and is packed down only as far as the node above it
 * forces - so a straight chain stays on one line and a wire runs flat.
 *
 * THAT FIRST ROW IS A STARTING POINT, NOT THE ANSWER. Placing a column top to bottom behind a
 * monotonic cursor, and sliding a tucked feeder DOWN past whatever is already placed, each get
 * exactly one guess: a node whose pin-aligned row was taken lands below the cursor and nothing
 * ever brings it back up when the obstruction turns out to have been three columns away. That
 * is what left a bare `Children` get 300+ units below the `Add` it feeds, and a band 640 tall
 * for a graph whose tallest column holds four nodes - a long diagonal wire and a field of empty
 * space being the same defect seen twice. So a final pass REFINES THE Y COORDINATE, leaving the
 * order alone: each node is pulled toward the MEDIAN of the rows its neighbours' pins actually
 * sit on, sweeping left to right off producers and right to left off consumers. A node CLAIMS
 * FREE SPACE and never pushes another node: letting it shove a lighter neighbour aside reads as
 * the obvious generalisation and it ratchets instead of converging, walking an event node 110
 * units off its own spine. Clamped between the neighbours it already has, the pass is a bounded
 * coordinate descent that settles on a fixed point - which is also why a second organise moves
 * nothing. Priority decides only who gets first refusal on that free space, weight being the
 * node's own degree with the spine outranking a tuck. Ordering and positioning are separate
 * questions: the barycentre this file rejected was rejected as an ORDER, where ties fall
 * through to a GUID, and no move here can reorder a column.
 */
struct BLUEPRINTIR_API FBpLayoutOptions
{
	/** Gap between columns, on top of the widest node in the column. */
	int32 HorizontalSpacing = 120;

	/** Gap between rows within a column. */
	int32 VerticalSpacing = 64;

	/** Gap between disjoint chains (separate events, orphaned fragments). */
	int32 ComponentSpacing = 160;

	/** Positions snap to this grid, matching the editor's own snap. */
	int32 GridSnap = 16;

	/**
	 * Anchor the result at the top-left the target nodes already occupy, instead of at
	 * the graph origin. Without this, organising three new nodes would yank them across
	 * the graph on top of whatever lives at (0,0).
	 */
	bool bPreserveOrigin = true;

	/**
	 * When laying out a SUBSET, push the finished block clear of nodes that were not in
	 * the subset. Irrelevant when organising a whole graph, where there is nothing else.
	 */
	bool bAvoidExistingNodes = true;

	/** Compute placements and report them without writing any of them. */
	bool bDryRun = false;

	/**
	 * Refit existing comment boxes whose members actually move.
	 *
	 * bp_apply disables this because its IR carries explicit covers[] membership and applies
	 * comments in a dedicated post-layout phase. Generic organize_nodes calls keep it on.
	 */
	bool bRefitComments = true;
};

/** Where one node ended up. */
struct BLUEPRINTIR_API FBpNodePlacement
{
	UEdGraphNode* Node = nullptr;
	FIntPoint OldPosition = FIntPoint::ZeroValue;
	FIntPoint NewPosition = FIntPoint::ZeroValue;
	int32 Column = 0;
	int32 Component = 0;

	bool Moved() const { return OldPosition != NewPosition; }
};

/** Where a comment box ended up after following the nodes it encloses. */
struct BLUEPRINTIR_API FBpCommentPlacement
{
	UEdGraphNode* Comment = nullptr;
	FIntPoint OldPosition = FIntPoint::ZeroValue;
	FIntPoint NewPosition = FIntPoint::ZeroValue;
	FIntPoint OldSize = FIntPoint::ZeroValue;
	FIntPoint NewSize = FIntPoint::ZeroValue;

	bool Changed() const { return OldPosition != NewPosition || OldSize != NewSize; }
};

struct BLUEPRINTIR_API FBpLayoutResult
{
	TArray<FBpNodePlacement> Placements;

	/** Exact comment moves/resizes, populated for real and dry-run layouts alike. */
	TArray<FBpCommentPlacement> CommentPlacements;
	int32 MovedCount = 0;
	int32 ColumnCount = 0;
	int32 ComponentCount = 0;

	/** Comment boxes resized to keep enclosing the nodes that moved out from under them. */
	int32 RefittedComments = 0;

	/**
	 * Placements still overlapping after a collision slide gave up (its iteration bound
	 * was hit). Zero in every ordinary run; anything else means the result contains a
	 * REAL overlap, and the caller should hear about it here rather than the user
	 * discovering it visually. The placement is kept either way - a bounded slide that
	 * refuses to write would leave the node at the origin, which is worse.
	 */
	int32 UnresolvedOverlaps = 0;
};

class BLUEPRINTIR_API FBlueprintGraphLayout
{
public:
	/**
	 * Arrange nodes.
	 *
	 * @param Graph    Graph to arrange. Required.
	 * @param Targets  Nodes to move. EMPTY means every node in the graph except comment
	 *                 boxes, which are never laid out - they are containers, and are
	 *                 refitted around their contents afterwards instead.
	 * @param Options  Spacing and behaviour.
	 * @param OutResult Placements, whether or not they were applied.
	 * @param OutError Failure reason; only set when this returns false.
	 * @return false only for a request that cannot be honoured (null graph, a target
	 *         that is not in this graph). An empty graph is a success with no placements.
	 */
	static bool Organize(
		UEdGraph* Graph,
		const TArray<UEdGraphNode*>& Targets,
		const FBpLayoutOptions& Options,
		FBpLayoutResult& OutResult,
		FString& OutError
	);

	/**
	 * Estimated on-screen extent of a node, in graph units.
	 *
	 * Comment boxes report their real stored size. Everything else is estimated from
	 * title length and pin count, because a node's true size is computed by its Slate
	 * widget and no widget exists in a headless editor. Estimates run generous: too wide
	 * costs whitespace, too narrow costs an overlap.
	 */
	static FIntPoint EstimateNodeSize(const UEdGraphNode* Node);

	/**
	 * Nodes whose centre falls inside a comment box's rectangle.
	 *
	 * Centre-inside rather than fully-contained on purpose: node extents are estimated,
	 * and a strict test would drop a node from its own comment because the estimate ran
	 * a few pixels wide.
	 */
	static TArray<UEdGraphNode*> NodesInsideComment(UEdGraph* Graph, const UEdGraphNode* CommentNode);

	/** Resize a comment box so it encloses Members, with padding and room for the title bar. */
	static void FitCommentToNodes(UEdGraphNode* CommentNode, const TArray<UEdGraphNode*>& Members);

	/**
	 * Place newly-unbound comment boxes clear of graph content and one another.
	 *
	 * The boxes are allocated in one horizontal row alongside the graph. Existing content is
	 * measured once, so placing N boxes is O(graph nodes + N), rather than a collision scan
	 * repeated for every candidate position. Input order is preserved left-to-right.
	 */
	static void PlaceUnboundComments(
		UEdGraph* Graph, const TArray<UEdGraphNode_Comment*>& Comments);

	/** Padding a fitted comment box leaves around its contents. */
	static constexpr int32 CommentPadding = 40;

	/** Extra headroom above the contents for the comment's title bar. */
	static constexpr int32 CommentTitleHeight = 48;

	/** Gap between an allocated unbound box and graph content / its neighbour. */
	static constexpr int32 CommentAllocationSpacing = 64;
};
