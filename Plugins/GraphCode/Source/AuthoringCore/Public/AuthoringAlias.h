#pragma once

#include "CoreMinimal.h"

/**
 * Stable aliases: `n1`, `n2`, `c1` - the handle a text IR uses to name one object in a document.
 *
 * ---------------------------------------------------------------------------
 * WHY THIS FILE EXISTS (supersedes BlueprintIR ADR-3)
 * ---------------------------------------------------------------------------
 * The tokens LOOK exactly like the ordinals they replace. What changed is where the number comes
 * from. It used to be the traversal POSITION, which is deterministic, round-trips, and was the
 * right call for two years. It has one cost, and the cost is the whole reason these documents are
 * committed to git:
 *
 *   Delete n2 and every node after it is renamed.
 *
 * Not "moved" - RENAMED. n3 becomes n2, n4 becomes n3, and so does every reference to them
 * anywhere in the graph, including references that appear ABOVE the deletion. One node removed
 * from a 200-node graph rewrites most of 200 lines. So a one-node diff is unreadable, `git blame`
 * credits the whole graph to whoever last deleted anything, and two agents editing opposite ends
 * of one graph conflict on every line between them. Insertion is the same story: the numbering
 * follows a traversal, so adding one input pin renumbers everything reached after it.
 *
 * The number is now MINTED ONCE and stored in the node's own `NodeGuid`. Delete n2 and the file
 * reads `n1, n3, n4` - nothing else moves.
 *
 * This does not reintroduce the failure ADR-3 was written to prevent. ADR-3 argued that apply()
 * mints fresh GUIDs, so a GUID-derived alias could not satisfy `read(apply(read(G))) == read(G)`.
 * True of an applier that randomises identity; ours no longer does. Differential reuses a matched
 * node in place, and a CREATED node is stamped with the identity its own alias names.
 *
 * ---------------------------------------------------------------------------
 * THE LAYOUT: FOUR PARTS, THREE OF THEM CHECKED
 * ---------------------------------------------------------------------------
 * An FGuid is four uint32s, and `UEdGraphNode::NodeGuid` is one. A node whose GUID this scheme
 * owns reads:
 *
 *   A = ASSET   Part A of the owning package's UPackage::PersistentGuid.
 *   B = INDEX   the alias number. `n7` is B == 7. 1-based; 0 means "no identity".
 *   C = KIND    hash of the node's class AND its owning graph's UEdGraph::GraphGuid.
 *   D = CHECK   hash of (A, B, C) under SchemeVersion.
 *
 * Part A is real, engine-maintained, and persistent: UPackage generates it for every new package
 * (Package.cpp:138), SavePackage writes it into the file summary (SavePackage2.cpp:1286), and
 * LinkerLoad reads it back (LinkerLoad.cpp:1854). It is regenerated in exactly one case -
 * PreSavePackage does so when the target path differs from the loaded path (SavePackage2.cpp:281),
 * i.e. rename, move, Save As. Which means A is stable across ordinary saves and NOT stable across
 * a rename. See "what invalidates an identity" below; that turns out to cost nothing.
 *
 * D is the part that makes the scheme safe rather than merely tidy. Without it there would be no
 * way to tell a minted B from the middle 32 bits of a random GUID, and a node the user dragged
 * onto the canvas would claim to be `n3141592653`. With it, a random GUID validates with
 * probability 2^-32 - so an unrecognised node is reliably recognised as unrecognised.
 *
 * C carries the owning graph as well as the class for two independent reasons. Uniqueness: indices
 * restart per graph (they always did, and small numbers are the point), so without a graph
 * discriminator `n1 = CallFunction` in EventGraph and in a function would compose the SAME GUID -
 * and NodeGuid has to stay unique across a whole Blueprint, because the engine searches on it
 * Blueprint-wide (BlueprintEditorUtils.cpp:9563 walks every ubergraph matching a custom event's
 * NodeGuid, which IS that event's function identity). Correctness: a node cut into a different
 * graph then fails validation instead of arriving with a borrowed number.
 *
 * The graph's GUID, though, NOT its name. That distinction was got wrong first and it matters:
 * scoping by name meant renaming a function invalidated every node in it, so a rename produced a
 * whole-body rewrite where the diff should have been one line. UEdGraph::GraphGuid is minted per
 * graph in PostInitProperties and serialised, and the engine's own comment on the sibling field in
 * FGraphReference is "so we can refind it if it has been renamed" - which is exactly this job.
 * BlueprintIR::GraphIdentityScope is the one place that decides it, with the graph's name as a
 * fallback for a graph old enough to load an invalid GraphGuid.
 *
 * ---------------------------------------------------------------------------
 * WHAT INVALIDATES AN IDENTITY (and why each is right)
 * ---------------------------------------------------------------------------
 * Any part failing its check means the node has NO identity: it is assigned a provisional index
 * and re-stamped by the next write. Reads never mutate the asset, so a projection is always
 * complete - some of its numbers are just not durable yet.
 *
 *   - A mismatch: the node came from a different asset (copy/paste across Blueprints), or this
 *     asset was renamed. A pasted node SHOULD lose its old number. A rename renumbers the whole
 *     graph, which sounds expensive and is not: renaming an asset renames its `.gc` too and
 *     rewrites the envelope's `asset:` line, so git already sees a new file.
 *   - C mismatch: the node's class changed, or it moved between graphs. NOT a graph rename - C
 *     holds the graph's GUID, so renaming a function leaves every number in it alone. A class
 *     change at a surviving alias is the one case the applier repairs rather than re-mints - it
 *     re-stamps the same B with the new C, because Differential promises that alias keeps meaning
 *     that node.
 *   - D mismatch: not ours. An editor-authored node, or a node from before this scheme.
 *   - B == 0, or two validated nodes in one graph claiming the same B: identity is the alias, so
 *     a duplicate would make every reference ambiguous. First in document order keeps it.
 *
 * Note what does NOT invalidate: duplicating an asset. A duplicate keeps its source's
 * PersistentGuid until it is saved somewhere else, and the aliases carry over intact - which is
 * what a faithful copy should do.
 *
 * ---------------------------------------------------------------------------
 * WHAT AN ALIAS IS NOT
 * ---------------------------------------------------------------------------
 * There is no "version" part. A version that incremented on edit would change the alias of an
 * edited node, and therefore every reference to it - which is the churn this file removes.
 * SchemeVersion below versions the LAYOUT, not any node.
 *
 * The parser's rules are the only rules an alias must obey: it starts with a letter or underscore
 * (so `1.5` cannot parse as pin 5 of node 1), contains no '.' (the pin separator) and no
 * whitespace, and is unique in its graph. An applier accepts ANY token meeting those - a
 * hand-written `n_temp` is legal, and comes back reported with the number it was minted as.
 */
namespace AuthoringCore::Alias
{
	/** Version of the four-part LAYOUT. Folded into Parts C and D, so a bump invalidates every
	 *  existing identity and renumbers every graph once. Bump only for a real layout change. */
	inline constexpr uint32 SchemeVersion = 1;

	/** Part A: the owning package's identity. 0 when ObjectInPackage has no package. */
	AUTHORINGCORE_API uint32 AssetTag(const UObject* ObjectInPackage);

	/**
	 * Part C: what this object is and where it lives.
	 *
	 * Scope names the container the index is numbered within, and it must be that container's
	 * PERSISTENT identity, never its display name. A Blueprint graph passes its
	 * `UEdGraph::GraphGuid` - the engine mints one per graph in PostInitProperties and serialises
	 * it, and its sibling field in FGraphReference is commented "so we can refind it if it has been
	 * renamed", which is exactly the property wanted here. Passing the graph's NAME instead would
	 * make renaming a function renumber its entire body, turning a one-line diff into a rewrite.
	 *
	 * May be empty where there is only one container, as for a material's flat expression list.
	 */
	AUTHORINGCORE_API uint32 KindTag(const UClass* Class, const FString& Scope);

	/** Part D: binds A, B and C so a random GUID does not read as an identity. */
	AUTHORINGCORE_API uint32 Check(uint32 AssetTag, uint32 Index, uint32 KindTag);

	/** The GUID an object with this identity must carry. */
	AUTHORINGCORE_API FGuid Compose(uint32 AssetTag, uint32 Index, uint32 KindTag);

	/**
	 * The index Guid carries, or 0 if it is not an identity for something of KindTag inside the
	 * package tagged AssetTag. Does not check uniqueness - only the caller sees the whole graph.
	 */
	AUTHORINGCORE_API uint32 IndexOf(const FGuid& Guid, uint32 AssetTag, uint32 KindTag);

	/** `n7` from 7. */
	AUTHORINGCORE_API FString Format(TCHAR Prefix, uint32 Index);

	/** 7 from `n7`. Returns 0 for anything else, including the negative and the overlong. */
	AUTHORINGCORE_API uint32 ParseIndex(const FString& Token, TCHAR Prefix);

	/**
	 * Resolve one graph's indices: validated identities keep their number, everything else takes
	 * the next free one above the highest validated index.
	 *
	 * Assigning above rather than into gaps is deliberate on both counts. Above, so adding a node
	 * cannot renumber an existing one. Never reusing a gap, so `n2` in an old commit and `n2` in a
	 * new one are the same node or no node - never two different ones.
	 *
	 * @param InGuids     one per object, in document order. Order decides only who wins a
	 *                    duplicate-index collision and what provisional numbers fall out.
	 * @param AssetTag    Part A every entry must carry.
	 * @param InKindTags  Part C each entry must carry, parallel to InGuids.
	 * @param OutIndices  the resolved index per entry, never 0.
	 * @param OutbDurable per entry: true if the object's GUID already held this index, false if it
	 *                    is provisional and a write should stamp it. Parallel to InGuids.
	 */
	AUTHORINGCORE_API void ResolveIndices(const TArray<FGuid>& InGuids, uint32 AssetTag,
		const TArray<uint32>& InKindTags, TArray<uint32>& OutIndices, TArray<bool>& OutbDurable);
}
