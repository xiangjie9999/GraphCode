// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "AuthoringCoreTypes.h"
#include "AuthoringPropertyCodec.h"
#include "GraphCodeCodecs.h"

/**
 * `kind:pcg_graph_instance` — a `UPCGGraphInstance`, which is a GRAPH PLUS WHAT ONE CALL SITE
 * CHANGED. `GC-WORLD-GENERATION-DESIGN.md` §4.3, row G3b.
 *
 * ## It is `kind:material_instance`'s shape, one asset over
 *
 * ```gc
 * gc 1  kind:pcg_graph_instance  asset:/Game/PCG/PCG_Forest_Dense  sig:<digest>  bin:<digest>
 * parent: /Game/PCG/PCG_Forest
 * omits: editor_customization
 *
 * settings {
 *   bOverrideDescription = true
 *   Description = NSLOCTEXT("", "", "Denser understory for the north valley")
 * }
 * parameters {
 *   Density = 1.6
 * }
 * ```
 *
 * **`parent:` NAMES AN ASSET, NOT A `/Script` CLASS**, which is the one thing that makes this kind
 * read differently from every other World kind — and it is exactly what `kind:material_instance`
 * already does, so it is a precedent rather than an invention. The class is fixed by the kind token;
 * what a document has to state is WHICH graph this instances, and a graph is content.
 *
 * ## `parameters { }` IS A MEMBERSHIP LIST, AND THE DESIGN SAID BOTH THINGS
 *
 * §4.3 wrote *"an override that is not written is not overridden"* and labelled that
 * `omission: preserves` in the same sentence. **Those are opposite policies and the first one is the
 * ruling** — an override IS its membership in `FPCGOverrideInstancedPropertyBag::PropertiesIDsOverridden`,
 * so a block that is the complete set of them means an omitted parameter stops being overridden and
 * returns to the parent graph's value. Declared `Deletes` for that reason.
 *
 * It is also the SAME sentence the `overrides { }` body on a subgraph node says
 * (`GraphCodeWorldPcgGraph.cpp`), and deliberately: a subgraph node and a graph instance asset are
 * the same mechanism at two scales, so they get one grammar and one omission rule
 * ([[one-concept-two-grammars]] is the failure this avoids).
 *
 * ## What this kind does NOT carry
 *
 * The child graph's own nodes, parameters and settings. They are the PARENT's document, and an
 * instance that restated them would put one fact in two files — the same reason a subgraph node is
 * never inlined (§4.4 trap 3).
 */
class UPCGGraphInstance;
struct FGraphCodeParsedDocument;

namespace GraphCodeWorld
{
	/**
	 * The exact class, for `IsProjectablePcgGraph`'s reason read backwards: `UPCGGraph` is the
	 * SIBLING here, and an ancestor test on `UPCGGraphInterface` would swallow it.
	 */
	GRAPHCODEPCGAUTHORING_API bool IsProjectablePcgGraphInstance(const UObject* Asset);

	/** Everything World registers for `kind:pcg_graph_instance`. */
	GRAPHCODEPCGAUTHORING_API FGcProductKindCodec PcgGraphInstanceCodec();

	/**
	 * The policy `settings { }` is exported AND IMPORTED under — shared for the reason
	 * `PcgGraphSettingsPolicy` is: the exclusion list IS the block's contract, and a writer with its
	 * own copy would eventually reset a field the reader never printed
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	GRAPHCODEPCGAUTHORING_API AuthoringCore::FPropertyExportPolicy PcgGraphInstanceSettingsPolicy();

	GRAPHCODEPCGAUTHORING_API FString PcgGraphInstanceSettingsBlock(UPCGGraphInstance* Instance);
	GRAPHCODEPCGAUTHORING_API FString PcgGraphInstanceParametersBlock(UPCGGraphInstance* Instance);

	GRAPHCODEPCGAUTHORING_API TMap<FString, FGcProductBlockWriter> PcgGraphInstanceBlockWriters();

	/**
	 * `gc_apply create:true`'s greenfield half.
	 *
	 * **NO `ParentRequirement`, AND THAT IS MEASURED AGAINST THE ENGINE RATHER THAN ASSUMED.**
	 * `UPCGGraphInstanceFactory::FactoryCreateNew` sets the graph only `if (ParentGraph)` — an
	 * instance pointing at nothing is a state the editor itself creates, and it projects as a
	 * document this kind reads. So `parent:` is honoured when stated and never demanded, which also
	 * keeps the published template free of a `/Game/…` path that would exist in this project and in
	 * no other ([[the-project-is-not-the-customer]]).
	 */
	GRAPHCODEPCGAUTHORING_API bool CreatePcgGraphInstance(const FGraphCodeParsedDocument& Document, const FString& PackageName,
		UObject*& OutAsset, FString& OutError);

	/**
	 * The shared half of an override block: which members of a bag are marked, and their values.
	 *
	 * Exported because a subgraph node's `overrides { }` body and this kind's `parameters { }` block
	 * are the same sentence at two scales, and a second copy of it would drift
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	GRAPHCODEPCGAUTHORING_API AuthoringCore::FPropertyExportPolicy PcgOverridePolicy(const UPCGGraphInstance* Instance);
	GRAPHCODEPCGAUTHORING_API void PcgAppendOverrideLines(const UPCGGraphInstance* Instance, int32 Depth,
		TArray<FString>& OutLines, TArray<FAuthoringDiagnostic>& OutDiagnostics);
	GRAPHCODEPCGAUTHORING_API bool PcgApplyOverrideLines(UPCGGraphInstance* Instance, const TArray<FString>& Entries,
		const TCHAR* Where, TArray<FString>& OutErrors);
}
