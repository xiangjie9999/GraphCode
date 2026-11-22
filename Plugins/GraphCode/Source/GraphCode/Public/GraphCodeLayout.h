// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UMaterialExpression;

/** One identity-preserving graph-node move, addressed by the alias gc_read emitted. */
struct GRAPHCODE_API FGraphCodeLayoutMove
{
	FString Alias;
	FString NodeGuid;
	FIntPoint OldPosition = FIntPoint::ZeroValue;
	FIntPoint NewPosition = FIntPoint::ZeroValue;
	int32 Column = 0;
	int32 Component = 0;
};

/** One layout-only comment-box move/resize. */
struct GRAPHCODE_API FGraphCodeLayoutCommentMove
{
	FString Alias;
	FString NodeGuid;
	FIntPoint OldPosition = FIntPoint::ZeroValue;
	FIntPoint NewPosition = FIntPoint::ZeroValue;
	FIntPoint OldSize = FIntPoint::ZeroValue;
	FIntPoint NewSize = FIntPoint::ZeroValue;
};

struct GRAPHCODE_API FGraphCodeLayoutOptions
{
	/**
	 * The DOCUMENT `sig:` — the one on the `gc 1` line of the gc_read the aliases came from.
	 * Required, because aliases are scoped to that exact read and are renumbered by the next one.
	 *
	 * It was the graph's own sig until 2026-08-21, when graph headers stopped stating one; there is
	 * now a single sig on this surface and this is it, the same value `gc_apply` takes.
	 */
	FString BaseSignature;

	/** Empty means the whole graph; otherwise every alias must be a live statement alias. */
	TArray<FString> Aliases;

	bool bDryRun = false;
	int32 HorizontalSpacing = 120;
	int32 VerticalSpacing = 64;

	/**
	 * Save the package when nodes actually moved. **DEFAULT TRUE — A WRITE FINISHES ITSELF.** C24.1.
	 *
	 * The same parameter and the same default as `FGraphCodeApplyOptions::bSave`, because §11.1's
	 * ruling is about every `gc_` verb that writes and this one writes. A dry run ignores it —
	 * [[dry-run-must-not-write-anything]].
	 */
	bool bSave = true;

	/**
	 * The caller already holds this Blueprint in memory, just mutated it, and is laying out a WHOLE
	 * graph — so skip the `BaseSignature` guard.
	 *
	 * That guard protects ALIAS BINDING: an external caller reads a graph, notes `n4`, and if the
	 * asset moved before the layout call `n4` might now name a different node. `gc_apply organise:`
	 * has no such read — it wrote the graph in this same call and passes NO aliases (empty means the
	 * whole graph), so there is nothing to bind stale. Requiring a `sig:` here would force a
	 * redundant re-projection of an asset the caller is holding, purely to satisfy a check whose
	 * hazard cannot arise. Set ONLY by that post-write path; the MCP layout tool never sets it and
	 * its guard is unchanged. Illegal beside a non-empty `Aliases` — a subset still needs the sig.
	 */
	bool bInMemoryWholeGraph = false;
};

struct GRAPHCODE_API FGraphCodeLayoutResult
{
	FString AssetPath;
	FString GraphName;

	/**
	 * What the asset projects to now — unchanged by this call, since positions are absent from
	 * `.gc`. Reported so a caller can chain a second layout without re-reading: what comes back
	 * here is exactly what goes out as the next `BaseSignature`.
	 */
	FString DocumentSignature;
	bool bDryRun = false;
	bool bChanged = false;
	int32 ConsideredNodes = 0;
	int32 ColumnCount = 0;
	int32 ComponentCount = 0;
	int32 UnresolvedOverlaps = 0;
	TArray<FGraphCodeLayoutMove> Moves;
	TArray<FGraphCodeLayoutCommentMove> CommentMoves;

	/** Literally "a save ran". False always carries `SaveSkipped` — there is no silent false. */
	bool bSaved = false;

	/** Why `bSaved` is false, in the caller's words. Empty exactly when `bSaved` is true. */
	FString SaveSkipped;
};

/**
 * GraphCode-facing, headless graph layout.
 *
 * Positions intentionally do not exist in `.gc`, so this operation edits only the binary asset and
 * reports every resulting move. It never compiles, reconstructs a node, writes a sidecar, or mints
 * a GUID — but as of C24.1 it does SAVE what it moved, because a node position that dies with the
 * editor is not a layout anybody asked for.
 */
class GRAPHCODE_API FGraphCodeLayout
{
public:
	static bool Apply(const FString& AssetPath, const FString& GraphName,
		const FGraphCodeLayoutOptions& Options, FGraphCodeLayoutResult& OutResult,
		FString& OutError);

	/** In-memory entry point used by focused automation tests and already-loaded callers. */
	static bool ApplyToBlueprint(UBlueprint* Blueprint, const FString& GraphName,
		const FGraphCodeLayoutOptions& Options, FGraphCodeLayoutResult& OutResult,
		FString& OutError);

	/**
	 * The same layout, over a material or a material function.
	 *
	 * A material has no named graphs - it has exactly one, and the `.gc` `graph { }` block that
	 * projects it carries no name - so this takes no `GraphName` and `FGraphCodeLayoutResult::GraphName`
	 * comes back empty. `Options.Aliases` still selects a subset, bound through the same MaterialIR
	 * read `gc_read` projects through.
	 *
	 * IT MOVES ROWS AND NOTHING ELSE. A material keeps positions on its EXPRESSIONS, so the UEdGraph
	 * the organiser needs is built for the call and thrown away; the engine's own write-back
	 * (`LinkMaterialExpressionsFromGraph`) is deliberately NOT used, because it reconnects every
	 * expression input from the graph on its way past and a layout may not touch a wire.
	 */
	static bool ApplyToMaterial(UObject* MaterialOrFunction,
		const FGraphCodeLayoutOptions& Options, FGraphCodeLayoutResult& OutResult,
		FString& OutError);

	/**
	 * Lay out what a material WRITER just created - the step a Blueprint write already takes in
	 * `FBpGraphBuilder::Layout`.
	 *
	 * When `Created` is every expression the asset now holds (a create, or a replace that kept
	 * nothing) the whole graph is laid out, output node included. Otherwise only `Created` moves,
	 * seeded beside the nodes it is wired to, and nothing the write reused changes its row.
	 *
	 * Runs inside the caller's transaction and never saves. It is not `ApplyToMaterial` because that
	 * cancels a transaction of its own when nothing moves, and the engine cannot cancel a nested
	 * transaction without dropping the caller's with it (`UTransBuffer::Cancel`).
	 */
	static bool PlaceCreatedExpressions(UObject* MaterialOrFunction,
		TConstArrayView<UMaterialExpression*> Created, FString& OutError);

	/**
	 * Tidy several WHOLE graphs of one Blueprint as a single post-write cleanup — the engine behind
	 * `gc_apply organise:true`.
	 *
	 * `GraphNames` empty means every graph the Blueprint has; otherwise exactly those, by name. Each
	 * graph is laid out whole through `ApplyToBlueprint` with `bInMemoryWholeGraph`, so no `sig:` is
	 * needed. **Never saves** — `bSave` is forced false on each call so the caller folds every move
	 * into its own one save.
	 *
	 * BEST-EFFORT, because it runs AFTER a successful write: a `GraphNames` entry the Blueprint does
	 * not have, or an `AssetPath` that is not a Blueprint, is added to `OutWarnings` and skipped
	 * rather than failing — undoing the write over a mistyped graph name would be the wrong trade.
	 * `OutResults` carries one entry per graph actually laid out. Returns false only when nothing
	 * could be attempted at all (no Blueprint), with the reason in `OutWarnings`.
	 */
	static bool OrganiseGraphs(const FString& AssetPath, TConstArrayView<FString> GraphNames,
		bool bDryRun, TArray<FGraphCodeLayoutResult>& OutResults, TArray<FString>& OutWarnings);
};
