// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AuthoringCoreTypes.h"
#include "AuthoringPropertyCodec.h"
#include "GraphCodeCodecs.h"

/**
 * `kind:pcg_graph` — GraphCode World's projection of a `UPCGGraph`, and **the first whole kind World
 * has ever owned**. `GC-WORLD-GENERATION-DESIGN.md` §4, row G2.
 *
 * ## The four blocks
 *
 * - `settings { }` — the graph's own reflective fields as deviations (hierarchical generation, the
 *   grid, the landscape and cooking flags);
 * - `parameters { }` — `UserParameters`, the `FInstancedPropertyBag` that is the graph's public API,
 *   as `Name: type = value`;
 * - `graph { }` — the nodes and the edges between them;
 * - `comments { }` — the editor's comment boxes, keyed on what they enclose rather than on where
 *   they are, which is `kind:material`'s rule and not a new one.
 *
 * ## The one thing to read before changing anything here: A PIN IS NAMED
 *
 * The obvious precedent for a dataflow graph in this corpus is `kind:material`, whose edges are
 * `in0=n0.out0`. **It does not transfer, and four engine measurements say so** (§4.0):
 *
 * 1. `FPCGPinProperties` carries `FName Label`, and the node's own lookup is
 *    `UPCGNode::GetInputPin(const FName& Label)` — index is not identity anywhere in PCG.
 * 2. `UPCGNode::UpdatePins` reconciles pins *on a name basis*, and `RemoveDuplicates` beside it
 *    enforces label uniqueness within a node and direction. So a label is a guaranteed unique key —
 *    exactly the guarantee a name-addressed grammar needs and an index cannot give.
 * 3. A node's pin SET is computed from its settings, and the overridable-property pins are APPENDED
 *    to the same array (`UPCGSettings::FillOverridableParamsPins`). So a pin's INDEX moves when an
 *    unrelated property becomes overridable, and a positional edge would silently re-bind.
 * 4. `UPCGEdge` still carries `InboundLabel_DEPRECATED`/`OutboundLabel_DEPRECATED`: the edge was
 *    label-keyed outright before it became a pair of pin pointers.
 *
 * A material expression really is index-addressed — `GetInput(int32)`, with `GetInputName(int32)`
 * DERIVING the name from the index. PCG inverts that, so an edge here is `Label=alias.Label` and a
 * label containing a space is quoted.
 *
 * ## Read-only, and every gap is a named `omits:` token
 *
 * Write and greenfield follow through the seams Move has already proven
 * ([[gc-product-kind-writer-seam]], [[gc-product-kind-greenfield]]). What this phase owes them is a
 * projection that is INVERTIBLE — which is why the type token is a key rather than a display name,
 * and why `PcgSettingsClassForToken` exists beside `PcgSettingsToken` rather than a lookup table
 * being written down twice ([[a-read-answers-through-the-write-path]]).
 *
 * World owns the codec; Core owns the token and the class path (a literal, so Core takes no PCG
 * dependency), which is what lets a build without World — or without the PCG plugin at all — write
 * the `omits: all@world` stub.
 */
class UPCGGraph;
class UPCGNode;
class UPCGSettings;
class UPCGPin;
struct FGraphCodeParsedDocument;

namespace GraphCodeWorld
{
	/**
	 * True for the exact class, which is doing work rather than being cautious.
	 *
	 * `UPCGGraphInstance` is a SIBLING under `UPCGGraphInterface`, not a subclass — it is `parent:`
	 * plus parameter overrides, `kind:material_instance`'s shape, and its own kind when §4.3 lands.
	 * An ancestor test would swallow it into this one and project an instance's empty node list as
	 * though the graph it points at had no nodes.
	 */
	GRAPHCODEPCGAUTHORING_API bool IsProjectablePcgGraph(const UObject* Asset);

	/** Everything World registers for `kind:pcg_graph`: the projector, the envelope, the blocks. */
	GRAPHCODEPCGAUTHORING_API FGcProductKindCodec PcgGraphCodec();

	/**
	 * The node's type token: the settings class with `UPCG` off the front and `Settings` off the
	 * back, so `UPCGSurfaceSamplerSettings` is `SurfaceSampler`.
	 *
	 * **NOT `GetDefaultNodeName()`, AND THE REASON IS MEASURED RATHER THAN STYLISTIC.** That virtual
	 * is the friendlier string — it would spell this node `SurfaceSampler` too, and
	 * `UPCGUserParameterGetSettings` as the much nicer `GetGraphParameter`. Three things disqualify
	 * it as an identity:
	 *
	 * - **It is not a function of the class.** `UPCGGraphInputOutputSettings::GetDefaultNodeName`
	 *   returns `InputNode` or `OutputNode` depending on the `bIsInput` member — one class, two
	 *   names, and it is the class every graph has two of.
	 * - **It is not a legal token.** `UPCGBooleanSelectSettings` answers `Select` and
	 *   `UPCGMultiSelectSettings` answers `Select (Multi)`: a space and a bracket, in a position the
	 *   grammar reads as a statement's type.
	 * - **It is not reversible.** Going back from a payload to a `UClass` means scanning 199 CDOs
	 *   and hoping the answer is unique — and `UPCGBlueprintSettings` puts a USER-CHOSEN name in
	 *   that space, so a Blueprint node could collide with an engine one and the collision would be
	 *   content, not code.
	 *
	 * A class name is unique by the language's own rules, so the token is a key by construction.
	 * `kind:material` and `kind:niagara_system` both already derive their type tokens this way; this
	 * is that rule, not a new one.
	 *
	 * I checked the mutability charge before making it and it did not hold: every other
	 * `GetDefaultNodeName` I measured is a per-class constant, including the maths and comparison
	 * ops, which return `AttributeMathsOp` rather than the operation. The case against it rests on
	 * the three points above and not on a fourth I could not measure.
	 */
	GRAPHCODEPCGAUTHORING_API FString PcgSettingsToken(const UClass* SettingsClass);

	/**
	 * The inverse, and it is a SCAN rather than a table for the reason Niagara's renderer token is:
	 * asking every candidate to spell itself and comparing that against the payload makes the reader
	 * the single authority on the token. A class the reader can print is then a class a writer can
	 * name — including one from a plugin this file has never heard of
	 * ([[branch-on-data-not-on-caller-identity]]).
	 */
	GRAPHCODEPCGAUTHORING_API UClass* PcgSettingsClassForToken(const FString& Token);

	/**
	 * The same inverse over any base, which is what a `{ }` body needs and the settings-only form
	 * cannot give it.
	 *
	 * A contained sub-object writes its class the same way a node does — `MeshSelectorParameters =
	 * MeshSelectorWeighted` — but `UPCGMeshSelectorWeighted` is not a `UPCGSettings`, so the scan
	 * above answers null for it. Constraining the scan by the PROPERTY'S OWN declared class is both
	 * the fix and the better question: the property already says which family the value must come
	 * from, so a token is resolved against the set it could legally name rather than against every
	 * class in the engine.
	 */
	GRAPHCODEPCGAUTHORING_API UClass* PcgClassForToken(const UClass* Base, const FString& Token);

	/**
	 * True where §4.1 gives a property a `{ }` BODY instead of a place in the `[ … ]` bracket: a
	 * series (an array of structs or objects) or an instanced sub-object authored inline.
	 *
	 * Exported because the writer has to reset exactly what the statement carries, and the statement
	 * carries these in its body — `PcgNodePolicy` excludes them precisely so they appear once, which
	 * means the policy alone cannot tell a writer whether a field is unmanaged or merely elsewhere.
	 */
	GRAPHCODEPCGAUTHORING_API bool PcgOpensBody(const FProperty* Property);

	/**
	 * The baseline one node's properties are measured against: a freshly CONSTRUCTED peer, never the
	 * class default.
	 *
	 * Exported for the reason it exists at all. `UPCGSettings::Seed` is initialised from a class-name
	 * hash per instance, so the CDO and a real node differ on a field nobody typed — and a writer
	 * that reset against the CDO would write that hash back as though it were authored. Reader and
	 * writer share this or a round trip stops being one ([[two-carriers-of-one-fact-is-a-race]]).
	 *
	 * Transient and unrooted: read within the call that asks for it.
	 */
	GRAPHCODEPCGAUTHORING_API UObject* PcgFreshPeerOf(const UObject* Object);

	/**
	 * Every node's handle — `input`, `output`, then `n0..nk` in the graph's stored order.
	 *
	 * The writer resolves a payload's aliases through the SAME map the projection assigned, which is
	 * what makes `n3` in a file and `n3` in an apply the same node. A second numbering here would
	 * reuse nodes in one order and print them in another.
	 */
	GRAPHCODEPCGAUTHORING_API TMap<const UPCGNode*, FString> PcgGraphAliases(UPCGGraph* Graph);

	/**
	 * The four block bodies, exported so a writer renders its own result through the SAME functions
	 * the projector does and refuses unless the two agree — Niagara's validation strategy, which is
	 * what makes a silent drop structurally impossible instead of merely intended against.
	 */
	GRAPHCODEPCGAUTHORING_API FString PcgSettingsBlock(UPCGGraph* Graph);
	GRAPHCODEPCGAUTHORING_API FString PcgParametersBlock(UPCGGraph* Graph);
	GRAPHCODEPCGAUTHORING_API FString PcgGraphBlock(UPCGGraph* Graph);
	GRAPHCODEPCGAUTHORING_API FString PcgCommentsBlock(UPCGGraph* Graph);

	/**
	 * One node's statement, without its alias — `SurfaceSampler(In=n1.Out) [PointSteepness=0.7]`,
	 * plus the `{ }` body when §4.1's structural test fires.
	 *
	 * `Aliases` resolves an upstream node to the handle this document gave it. It is passed in
	 * rather than derived because a statement is rendered while the alias map is being built, and a
	 * node that could not be found is written as a refusal marker rather than as a plausible name.
	 */
	GRAPHCODEPCGAUTHORING_API FString PcgNodeStatement(const UPCGNode* Node, const TMap<const UPCGNode*, FString>& Aliases,
		int32 Depth);

	/**
	 * The policy a node's `[ … ]` props are exported under: `CPF_Edit` and nothing debug, transient
	 * or already carried by the statement's own spelling.
	 */
	GRAPHCODEPCGAUTHORING_API AuthoringCore::FPropertyExportPolicy PcgNodePolicy(const UPCGSettings* Settings);

	/**
	 * The policy `settings { }` is exported AND IMPORTED under — which fields of the graph itself
	 * this block manages, and therefore which ones an omission resets.
	 *
	 * **SHARED BECAUSE THE WRITER IS THE SECOND CALLER, NOT BECAUSE IT IS TIDY.** The exclusion list
	 * is the block's whole contract: it names the members that have a block of their own (`Nodes`,
	 * the two terminals, `CommentNodes`, `UserParameters`) and the ones that are editor or cook
	 * state. A writer with its own copy would eventually reset a field the reader never printed —
	 * the `Nodes` array, in the worst case — and a snapshot apply would empty the graph while
	 * reporting that it wrote the settings ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	GRAPHCODEPCGAUTHORING_API AuthoringCore::FPropertyExportPolicy PcgGraphSettingsPolicy();

	/**
	 * The writers `PcgGraphCodec()` hangs on its `BlockWriters`, keyed by block name.
	 *
	 * `comments` is deliberately absent and that is a RULING rather than an omission: its `covers=`
	 * is derived from where the nodes ended up, and `omits: node_layout` means the document carries
	 * no position to put them back at — so the box a payload asks for cannot be drawn without
	 * inventing geometry the file never stated
	 * ([[inverting-a-lossy-projection-can-be-impossible]]).
	 */
	GRAPHCODEPCGAUTHORING_API TMap<FString, FGcProductBlockWriter> PcgGraphBlockWriters();

	/**
	 * `gc_apply create:true`'s greenfield half — a `UPCGGraph` from a document.
	 *
	 * **THE FLOOR IS THE HEADER LINE ALONE, AND THAT IS MEASURED RATHER THAN CHOSEN.** A freshly
	 * constructed `UPCGGraph` already carries both terminal nodes, so it projects as a document this
	 * kind reads without anything being written into it — which is what `Creation.RequiredBody` is
	 * for on the kinds where it is not true (`behaviortree`, whose factory leaves the authored graph
	 * null). Nothing is required, so nothing is demanded.
	 *
	 * The package is built directly. `UPCGGraphFactory` is `PCGEditor/Private` and this module does
	 * not link PCGEditor; what that factory adds is a template picker and a modal dialog, so a
	 * headless apply loses nothing by not reaching it.
	 */
	GRAPHCODEPCGAUTHORING_API bool CreatePcgGraph(const FGraphCodeParsedDocument& Document, const FString& PackageName,
		UObject*& OutAsset, FString& OutError);

	/** Standalone settings use the same typed value as an inline graph node, without edges. */
	GRAPHCODEPCGAUTHORING_API FString PcgStandaloneSettingsBlock(const UPCGSettings* Settings);
	GRAPHCODEPCGAUTHORING_API FGcProductKindCodec PcgSettingsCodec();
	GRAPHCODEPCGAUTHORING_API FGcProductBlockWriter PcgStandaloneSettingsWriter();
	GRAPHCODEPCGAUTHORING_API bool PcgSettingsValueClass(const FString& Body, UClass*& Class, FString& Error);
}
