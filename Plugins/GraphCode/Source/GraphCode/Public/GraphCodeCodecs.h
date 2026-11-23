#pragma once

#include "CoreMinimal.h"

#include "GraphCodeCapabilities.h"
#include "GraphCodeProducts.h"
#include "GraphCodeProjector.h"
#include "GraphCodeSchemaWriter.h"

/** The PAYLOAD side of a document — `GraphCodeSidecarReader.h`. A factory's input, never its output. */
struct FGraphCodeParsedDocument;
struct FGraphCodeCompileResult;
struct FGraphCodeLayoutResult;

/** A product arranges its native graphs inside the caller's write; never saves. */
using FGcProductKindOrganiser = TDelegate<bool(UObject*, TConstArrayView<FString>, bool,
	TArray<FGraphCodeLayoutResult>&, TArray<FString>&)>;

/**
 * WHAT A PRODUCT PLUGIN HANDS CORE, AND THE RULES FOR HANDING IT OVER.
 * `GC-PRODUCT-PACKAGING-DESIGN.md` §5b; user-directed 2026-08-08:
 *
 * > *"make them a universal seam, for writer and reader, not just for animbp. like make them easy
 * > to implement new scopes next time in move, or world or play"*
 *
 * `GraphCodeProducts.h` says what each product OWNS — a table Core keeps whether or not the
 * product's code is here, because `omits: <facet>@<product>` has to be writable in its absence.
 * This says what an installed product SUPPLIES. Four registrations:
 *
 * | registration | supplies | first caller |
 * |---|---|---|
 * | block reader | one block of an existing kind | Move — `animgraph` on `animation_blueprint` |
 * | block writer | applying one block of an existing kind | Move — the same block |
 * | kind codec | a whole kind: projector, envelope, block list | World — Niagara, Landscape, PCG |
 * | factory | how a document becomes a NEW asset | Move and World both |
 *
 * THREE INVARIANTS, and the first is the one that is easy to get backwards.
 *
 * 1. **A reader may be registered without a writer. A writer may NOT be registered without a
 *    reader.** Reader-without-writer is the ordinary state of a facet whose projection is proved
 *    and whose authoring is not — where `animgraph` sits. Writer-without-reader is a block that can
 *    be written and never diffed: a write with no round trip and no way to see what it did.
 * 2. **Registration is refused for a block or kind the product does not own.** This is what keeps
 *    the seam from being a general plugin hook — Move may supply `animgraph`, and may not supply
 *    `properties` and outrank Core's writer for it.
 * 3. **Absence is the ordinary path, not an error path.** Nothing registered means the block is
 *    never built; the `omits:` token and the advisory follow from the ownership table, which Core
 *    reads without the product being there. Nothing has to detect a failure.
 *
 * EVERY LOOKUP IS GATED ON `IsInstalled`, including the simulation switch. A product whose module
 * registered before `-GcWithoutProduct` was set still has its delegates bound, and answering with
 * one would simulate half an absence: the card says unavailable and the write succeeds.
 */

/** What a product's block writer reports back. The subset of `FGraphCodeApplyResult` it can fill. */
struct GRAPHCODE_API FGcProductWriteResult
{
	TArray<FString> Changes;
	TArray<FString> Errors;
	/**
	 * What the write accepted and the engine expects not to work as written — a Niagara force with no
	 * solver after it (NIAGARA-RPT N1). Never a refusal; `gc_apply` reports them as `write_warnings`.
	 */
	TArray<FString> Warnings;
	FString Message;

	/**
	 * Every entity this write removes, named, so the dry run can price it. S7b.
	 *
	 * **THE ONE FIELD A PRODUCT MUST FILL RATHER THAN MAY.** Core's eleven writers each return this
	 * and `gc_apply`'s dry run prices the union of them project-wide; a product block that removes
	 * something and reports nothing here is removed for free, and an unpriced deletion looks exactly
	 * like a deletion that costs nothing. `GC-SEARCH-DESIGN.md` §8.1.
	 *
	 * Leave `Block` empty — the apply seam stamps it, because only it knows which block routed here.
	 * A removal nothing outside the asset can cite carries `IntraAssetReason` instead of being left
	 * out, so the answer is "no search can tell you" rather than a confident zero.
	 *
	 * Empty is the ordinary case and the honest one for a block that cannot remove: `animgraph { }`,
	 * the seam's first caller, edits and never deletes.
	 */
	TArray<FGraphCodeRemoval> Removals;
};

/**
 * A reader for one block, supplied by the product that owns it.
 *
 * Returns zero or more block STRINGS, the same currency `FGraphCodeDocument::Blocks` already holds
 * — an empty array is a legitimate answer meaning "this asset has none", not a failure. It takes
 * the projection options because a product block is subject to the same canonicality rules as
 * Core's: `GraphName` narrows a graph-shaped block, and a reader that ignored it would put a
 * non-canonical document's worth of text into a narrowed read.
 */
using FGcProductBlockReader = TDelegate<bool(
	UObject* /*Asset*/,
	const FGraphCodeProjectionOptions& /*Options*/,
	TArray<FString>& /*OutBlocks*/,
	FString& /*OutError*/)>;

/**
 * A writer for one block, supplied by the product that owns it.
 *
 * The parameter list is copied from the eleven block writers already in `GraphCodeApply.cpp` rather
 * than invented — asset, block name, body, whether omission means deletion, and a result. A seam
 * whose shape does not match the callers it will sit beside is a seam the first real writer has to
 * be bent to fit.
 *
 * **`bDryRun` WAS MISSING UNTIL THE FIRST REAL WRITER ARRIVED, 2026-08-08**, and its absence was not
 * a smaller contract - it was a wrong one. Every Core writer takes the flag and honours it; a
 * product writer that never received it would have made `gc_apply dry_run:true` WRITE on the one
 * route that could not say no. [[dry-run-must-not-write-anything]]. A seam is only proved by a
 * caller, and this parameter is what the first caller found.
 */
using FGcProductBlockWriter = TDelegate<bool(
	UObject* /*Asset*/,
	const FString& /*BlockName*/,
	const FString& /*BlockBody*/,
	bool /*bSnapshot*/,
	bool /*bDryRun*/,
	FGcProductWriteResult& /*OutResult*/)>;

/** Atomic write of dependent blocks, with the same dry-run contract as a block writer. */
using FGcProductCoupledWriter = TDelegate<bool(UObject*, const TMap<FString, FString>&,
	bool /*bSnapshot*/, bool /*bDryRun*/, FGcProductWriteResult&)>;

/** Projects a whole kind. The registered replacement for one branch of Core's cast chain. */
using FGcProductKindProjector = TDelegate<bool(
	UObject* /*Asset*/,
	const FGraphCodeProjectionOptions& /*Options*/,
	FGraphCodeDocument& /*OutDocument*/,
	FString& /*OutError*/)>;

/**
 * Builds a new asset of a kind from a document. `gc_apply create:true`'s greenfield half.
 *
 * Separate from the projector rather than a mode of it, because the two fail differently: a
 * projector that cannot read an asset has a bug, and a factory that cannot build one from a given
 * document may simply have been handed a document that is not a sufficient recipe.
 *
 * **THE DOCUMENT IS THE PARSED ONE, AND THAT CHANGED WHEN THE FIRST CALLER ARRIVED, 2026-08-16.** It
 * was declared as `FGraphCodeDocument` — the type a PROJECTION is built into — which no greenfield
 * caller has or could have: `FGcGreenfield::Plan` holds the payload somebody wrote, which is an
 * `FGraphCodeParsedDocument`. Same correction the `bDryRun` note above records one seam over, and
 * for the same reason: a seam is only proved by a caller.
 *
 * **`PackageName` IS THE FULL `/Game/…` NAME AND THE PACKAGE DOES NOT EXIST YET.** The factory
 * creates both, exactly as `IAssetTools::CreateAsset` does for Core's kinds — it is handed the
 * scratch package during a plan and the real one on the way through, and must not care which.
 */
using FGcProductKindFactory = TDelegate<bool(
	const FGraphCodeParsedDocument& /*Document*/,
	const FString& /*PackageName*/,
	UObject*& /*OutAsset*/,
	FString& /*OutError*/)>;

/**
 * `gc_apply compile:true` for a product kind whose asset has its own compiler (a Niagara system): compile it, wait, and
 * fill the result as a Blueprint compile does (`Status` succeeded/failed, the errors and warnings split). False only
 * when nothing could be compiled; a compile that failed is a true return with `Status` failed.
 */
using FGcProductKindCompiler = TDelegate<bool(UObject* /*Asset*/, FGraphCodeCompileResult& /*OutResult*/, FString& /*OutError*/)>;

/** Everything a product supplies for one whole kind. `Blocks` is that kind's capability list. */
struct GRAPHCODE_API FGcProductKindCodec
{
	FGcProductKindProjector Projector;
	FGcProductKindOrganiser Organiser;

	/** The `omits:` and `ir:` tokens this kind's documents carry. `ExpectedEnvelope`'s per-kind row. */
	TArray<FString> Omits;
	TArray<FString> IrVersions;

	/** In projection order, which is the order the projector above writes them. */
	TArray<FGcBlockCapability> Blocks;

	/**
	 * The writers for this kind's own blocks, keyed by block name. Empty for a read-only kind.
	 *
	 * **HERE RATHER THAN THROUGH `RegisterBlockWriter`, BECAUSE THAT DOOR IS SHUT TO A WHOLE KIND.**
	 * That call demands a per-block READER — invariant 1, and rightly — and a kind codec registers
	 * none: its blocks come from the projector above, one function for all of them. So the first
	 * product kind that wanted a writer found the registry saying its block had no reader, on a kind
	 * whose every block is projected. Registering a stub reader per block to satisfy the check would
	 * have been a lie told to a guard, which is how the guard stops meaning anything.
	 *
	 * **INVARIANT 1 IS NOT WEAKENED, IT IS ENFORCED STRUCTURALLY INSTEAD.** A kind codec cannot
	 * register without a projector, and `RegisterKindCodec` refuses a writer whose block is not in
	 * `Blocks` above — so a writable block is necessarily one this kind also projects, which is the
	 * whole of what the reader requirement was buying.
	 */
	TMap<FString, FGcProductBlockWriter> BlockWriters;
	TSet<FString> CoupledBlocks;
	FGcProductCoupledWriter CoupledWriter;

	/** Optional. A kind with no factory simply cannot be created, which is most of them. */
	FGcProductKindFactory Factory;

	/** Optional. Only a kind whose asset the engine compiles has one; `compile:true` on any other kind is refused. */
	FGcProductKindCompiler Compiler;

	/**
	 * What `gc_describe` and `graphcode-capabilities.json` SAY about creating this kind.
	 *
	 * **A FACTORY WITH NO DESCRIPTION WAS INVISIBLE, WHICH IS THE GAP `GraphCodeCapabilities.cpp`
	 * WROTE DOWN AND THE FIRST PRODUCT FACTORY CAME BACK FOR (2026-08-16).** `CreationFor` is Core's
	 * switch over kinds it knows how to build; a product kind fell to its `default:` and was
	 * advertised as *"no greenfield factory is declared for this kind"* however many it had declared.
	 * Shipping the ability without the announcement is the stale-tool-card failure with the halves
	 * swapped, and a caller that reads the registry to decide what to offer would never offer it.
	 *
	 * **`RegisterKindCodec` REFUSES A CODEC WHOSE TWO HALVES DISAGREE**, so this cannot rot into a
	 * card that promises what no delegate backs, or a factory nothing announces. `ParentRequirement`
	 * and `RequiredBody` carry the same contract they do for Core's kinds: concrete text a spec runs
	 * verbatim, never a description of text.
	 */
	FGcLifecycleCapability Creation;
};

class GRAPHCODE_API FGraphCodeCodecs
{
public:
	/**
	 * A product supplies the reader for a block it owns.
	 *
	 * `Capability` is the block's full declaration — access, omission policy, the note a caller
	 * reads. It travels with the reader rather than being declared in Core, which is what makes
	 * `gc_describe` and `graphcode-capabilities.json` describe a product block with no Core edit.
	 *
	 * @return false, with `OutError` set, when the product does not own that block or the names
	 *         disagree.
	 */
	static bool RegisterBlockReader(
		EGcProduct Product,
		EGraphCodeAssetKind Kind,
		const FGcBlockCapability& Capability,
		FGcProductBlockReader Reader,
		FString& OutError);

	static void UnregisterBlockReader(EGraphCodeAssetKind Kind, const FString& BlockName);

	static const FGcProductBlockReader* FindBlockReader(
		EGraphCodeAssetKind Kind, const FString& BlockName);

	/**
	 * A product supplies the writer for a block it owns AND ALREADY READS.
	 *
	 * The reader requirement is invariant 1 and is enforced here rather than documented: a block
	 * that writes without projecting cannot be diffed, so nothing downstream could show what the
	 * write did.
	 *
	 * @return false, with `OutError` set, when the product does not own the block or has not
	 *         registered a reader for it.
	 */
	static bool RegisterBlockWriter(
		EGcProduct Product,
		EGraphCodeAssetKind Kind,
		const FString& BlockName,
		FGcProductBlockWriter Writer,
		FString& OutError);

	static void UnregisterBlockWriter(EGraphCodeAssetKind Kind, const FString& BlockName);

	static const FGcProductBlockWriter* FindBlockWriter(
		EGraphCodeAssetKind Kind, const FString& BlockName);

	static bool HasBlockWriter(EGraphCodeAssetKind Kind, const FString& BlockName);

	/** A product supplies the codec for a whole kind it owns. World's Niagara, Landscape and PCG. */
	static bool RegisterKindCodec(
		EGcProduct Product,
		EGraphCodeAssetKind Kind,
		const FGcProductKindCodec& Codec,
		FString& OutError);

	static void UnregisterKindCodec(EGraphCodeAssetKind Kind);

	static const FGcProductKindCodec* FindKindCodec(EGraphCodeAssetKind Kind);

	/**
	 * The block declarations for a kind, in projection order, for the capability registry to append
	 * to Core's own list.
	 *
	 * **A BLOCK AN ABSENT PRODUCT OWNS STILL APPEARS**, built from the ownership table's row, and
	 * that is user-directed rather than incidental: *"they should be shown as available in the tool,
	 * but the tool should show unavailable due to move not available"*. Dropping it would say the
	 * facet does not exist, which is a different sentence from "you do not have it here".
	 */
	static TArray<FGcBlockCapability> BlockCapabilitiesFor(EGraphCodeAssetKind Kind);

	/**
	 * Every registered block of a kind, read in projection order, appended to `OutBlocks`.
	 *
	 * ORDER IS PART OF THE FORMAT, so it is fixed rather than incidental: product enum order, then
	 * block name. Registration order would be module LOAD order, which is not a thing a committed
	 * file may depend on. Product blocks land after Core's, which is where `animgraph` already sits;
	 * a block that has to interleave with Core's is not expressible today and would need an
	 * explicit anchor in the ownership row.
	 */
	static bool ReadProductBlocks(
		UObject* Asset,
		EGraphCodeAssetKind Kind,
		const FGraphCodeProjectionOptions& Options,
		TArray<FString>& OutBlocks,
		FString& OutError);

	/**
	 * Bumped by every registration change, so a surface that CACHES a derived view can notice.
	 *
	 * `FGraphCodeCapabilities::All()` is the one that does. It was built once and its own comment
	 * said nothing it reads can change under a running editor — true while every block was declared
	 * in Core, and false the moment a product declares one, because a module registering after the
	 * first query would have been invisible to every surface for the rest of the session.
	 */
	static uint32 Generation();

	/** Drops every registration. For a spec that has to start from a known-empty seam. */
	static void UnregisterAll();
};
