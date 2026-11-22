// Blueprint IR - the write path. POLICY here, MECHANISM in BlueprintIRBuild.h.
//
// Replace mode is BUILT. Differential is not, and is refused rather than downgraded.
//
// What ApplyGraph decides, in order, and none of it touches the graph:
//   1. the Blueprint and the named graph exist
//   2. base_sig still matches - optimistic concurrency (ADR-5). A stale edit is refused
//      outright, because applying against a stale read is how one edit clobbers another
//   3. every alias the payload references is defined by the payload. THE TRUNCATION GUARD:
//      under ADR-7 Rule 2 a payload cut short is not a smaller edit, it is a mass deletion
//   4. every symbol the payload names resolves to exactly one entity
// and only then does FBpGraphBuilder run, inside one transaction, all-or-nothing.
//
// Notes that survived contact with the implementation, because they turned out to matter:
//
//  - Nodes are constructed by NewObject + PostPlacedNewNode + AllocateDefaultPins, NOT via
//    UBlueprintNodeSpawner. The original note here said the opposite, and it was wrong for
//    this caller: a spawner is keyed by ACTION, and looking one up for an arbitrary op
//    token means searching FBlueprintActionDatabase for something that may not be there at
//    all. ADR-8's whole point is that the class is the identity, and the class is
//    constructible directly. The per-node-type setup a spawner would have done is exactly
//    what ReconstructNode does in phase 2.
//
//  - CanCreateConnection runs before every connection and its FPinConnectionResponse is
//    surfaced VERBATIM. "Float is not compatible with Vector" is the highest-value
//    diagnostic in the system; paraphrasing it throws the signal away.
//
//  - Wildcard pins (Select, MakeArray, the container makers, a reroute) resolve their type
//    ON CONNECT, so connection order is significant and is therefore fixed: data edges in
//    statement order, then exec edges. Statement order puts a producer before its consumer.
//
//  - Compile errors do NOT roll the write back, deliberately. A graph that does not compile
//    is one the author can open and fix; discarding it leaves them an error message and
//    nothing to look at. bSuccess is still false.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintIRTypes.h"

class UBlueprint;
class UEdGraph;

/** How bp_apply should treat existing content. See ADR-5 in BlueprintIRTypes.h. */
enum class EBpApplyMode : uint8
{
	/**
	 * Rebuild the graph from the supplied IR and lay it out from scratch.
	 * Simple, and destructive to any hand-tuned layout.
	 */
	Replace,

	/**
	 * Match incoming statements against existing nodes via the alias mapping,
	 * mutate in place, and place only what is new. Requires a matching BaseSignature.
	 */
	Differential,
};

struct BLUEPRINTIR_API FBpApplyOptions
{
	/** Target graph name within the Blueprint. */
	FString GraphName;

	/**
	 * The interface whose `Graphs` array holds the target, when the target is an interface
	 * function's BODY rather than a member graph. Empty for every ordinary write.
	 *
	 * A SEPARATE FIELD RATHER THAN A WIDER `FindGraphByName`, and that is the whole safety
	 * argument for queue row 11q. `ImplementedInterfaces[].Graphs` is a fourth graph array; adding
	 * it to the name search would let a `functions { }` payload naming `Interact` resolve to an
	 * interface's graph, which is indistinguishable from declaring a member method of that name.
	 * Set here, the lookup searches THAT interface's array and nothing else, so an interface body
	 * is addressed by interface path plus function name and can be reached no other way.
	 *
	 * The full class path, as `interfaces { }` spells it:
	 * `/Game/UI/BPI_Interactable.BPI_Interactable_C`.
	 */
	FString InterfacePath;

	EBpApplyMode Mode = EBpApplyMode::Differential;

	/**
	 * Signature from the bp_read that this edit was computed against.
	 *
	 * Empty forces Replace mode. Non-empty and stale is a hard failure, not a
	 * warning: applying an edit computed against a graph that has since changed is
	 * how you silently clobber someone else's work.
	 */
	FString BaseSignature;

	/**
	 * Function names the DOCUMENT this graph belongs to is about to create, which the class does
	 * not carry yet. Empty for a single-graph write, which declares nothing beyond itself.
	 *
	 * Board row T-14: a new function and its first caller could not be written in one apply,
	 * because the callers were validated against the pre-write class while the preflight was
	 * simultaneously reporting `would create <name>`. The whole block is in hand at the document
	 * layer and its declared set is known before any node is built, so this carries that set the
	 * one level down that needs it. See `NamesDocumentDeclaredFunction` for how narrowly it is
	 * read - a bare name here still only admits a target whose OWNER is this Blueprint's class.
	 */
	TArray<FString> DeclaredFunctions;

	/**
	 * Variable names the DOCUMENT this graph belongs to ADDS in its own `properties` block, which
	 * the class does not carry yet. Empty for a single-block write, which declares nothing beyond
	 * itself.
	 *
	 * `DeclaredFunctions`' argument for the other half of the symbol table, and it closes a DRY RUN
	 * that disagreed with the write it was previewing. Measured 2026-08-30 on `BP_Brazier`: a
	 * document adding `Fuel` to `properties` and reading it in `functions` was refused by
	 * `dry_run:true` with *'Fuel' does not exist as a variable*, and the SAME document applied
	 * wet without complaint - because `properties` sorts ahead of the graph blocks
	 * (`BlockApplyRank`) and its writer marks the Blueprint structurally modified, so the real
	 * write regenerates the skeleton and the name resolves by the time the graph block is read.
	 * Only the preview lacked that, having written nothing. A preflight that refuses what the
	 * apply accepts is worse than no preflight: it teaches the caller to split one honest
	 * document into two writes it never needed.
	 *
	 * REMOVAL IS DELIBERATELY NOT THE MIRROR OF THIS, because it is already answered one layer up.
	 * A variable the payload drops from `properties` while something still reads it is caught by
	 * the declaration writer's own impact scan - `deletion_impact` and `orphaned_references` on the
	 * dry run, a refusal without `allow_breaking_changes` on the write - so a second check here
	 * would be a second carrier of one fact. What this set does NOT do is admit a name nobody
	 * declared: it is seeded from the payload's own block, so a misspelling is still a refusal.
	 *
	 * Read exactly as narrowly as its neighbour - see `NamesDocumentDeclaredVariable`.
	 */
	TArray<FString> DeclaredVariables;

	/**
	 * Variable names the DOCUMENT REMOVES in its own `properties` block, so a graph statement in
	 * the same payload that still reads one is refused rather than resolved against a live class
	 * the write is about to change.
	 *
	 * The narrow case the impact scan above cannot see, and the exact inverse of `DeclaredVariables`:
	 * the scan reads LIVE nodes, so a payload that removes a variable AND adds a NEW statement
	 * reading it has nothing live to report - the dry run passed and the write then refused. This
	 * is what makes the pair a seeded symbol table rather than a suppressed check.
	 */
	TArray<FString> RemovedVariables;

	/**
	 * CustomEvent names the DOCUMENT this graph belongs to declares in its OTHER graphs. Empty for
	 * a single-graph write, which declares nothing beyond itself.
	 *
	 * The fourth member of the seeded table, and the only one whose declaration is a NODE rather
	 * than an earlier block - which is why it is the only one that needed a realisation step
	 * inside the graph write (`RealiseDeclaredEvents`) rather than an earlier writer to sit
	 * behind. This set covers the OTHER half: an event declared in a different graph of the same
	 * payload, which the graph writing the caller cannot see in its own statements.
	 *
	 * WHAT IT ADMITS IS NARROWER THAN WHAT IT NAMES, and the difference is the ordering rule.
	 * Graphs are written in document order and each write regenerates the skeleton, so a caller
	 * AFTER the declaring graph resolves the real UFunction and never consults this set at all.
	 * Reaching it on a real write means the declaring graph is listed later, which no write can
	 * fix - so it is refused there with the order named. On a DRY RUN nothing has been written at
	 * all, so it is admitted and the graph declines its own preview instead.
	 *
	 * Read exactly as narrowly as its neighbours - see `NamesDocumentDeclaredMember`.
	 */
	TArray<FString> DeclaredEvents;

	/** Compile after applying and report the results log. */
	bool bCompile = true;

	/** Save the asset after a successful compile. */
	bool bSave = false;

	/** Validate and report what would happen without mutating anything. */
	bool bDryRun = false;

	/**
	 * Whether a payload that leaves out one of a state machine's STATES removes it, or is refused
	 * with the state named. `GC-STATE-MACHINE-DESIGN.md` §0 R12, and it is the block's `snapshot`
	 * mode reaching the one layer that can see a state.
	 *
	 * **FALSE IS THE SAFE DEFAULT AND IT MATCHES `patch`, WHICH IS `gc_apply`'S DEFAULT MODE.**
	 * Before this existed the answer was "removed, silently, in both modes":
	 * `GraphCode.AnimGraph.Write.AnOmittedStateIsAnsweredTheSameWayInBothModes` was written to ask
	 * and got exactly that. A whole-graph Replace is complete by construction, so the deletion was
	 * not a decision anybody made — it was the absence of a question.
	 *
	 * **SCOPED TO STATES, NOT TO NODES, AND THAT IS THE RULING RATHER THAN A CONVENIENCE.** Dropping
	 * a node from a transition's rule graph is an ordinary edit this block has supported since
	 * `ATransitionsConditionLogicSurvivesItsOwnProjection`, and a guard that refused every deletion
	 * under patch would break it. R12 rules about the entries that have IDENTITY — a state has a
	 * name a second payload can address, a node in a rule graph has an alias the writer re-mints.
	 *
	 * Inert for every asset without a state machine, which is every asset that is not an Animation
	 * Blueprint and most of those.
	 */
	bool bOmittedStatesAreRemoved = false;
};

struct BLUEPRINTIR_API FBpApplyResult
{
	/**
	 * Did the call do what was asked. NOT "was the graph left alone" - see Mutation.
	 *
	 * These are genuinely independent, and conflating them is the bug this struct grew a
	 * status enum to prevent. A compile failure is bSuccess == false with
	 * Mutation == Written: the graph WAS replaced and does not compile. An agent that
	 * reads failure as "nothing happened" and retries is then rewriting a graph it
	 * already rewrote, against a base_sig that is now stale.
	 */
	bool bSuccess = false;

	/** What happened to the graph. Reported, never inferred from which error array is empty. */
	enum class EMutation : uint8
	{
		/** The graph was not touched. Validation refused, or this was a dry run. */
		NotAttempted,
		/** Construction began and failed; the original nodes were put back. */
		RolledBack,
		/** The graph is now the payload's graph. True even when the compile then failed. */
		Written,
	} Mutation = EMutation::NotAttempted;

	/**
	 * The preview was DECLINED rather than failed: this call cannot model the payload without
	 * writing it, so it formed no opinion about whether the payload is good.
	 *
	 * **`bSuccess` IS FALSE BESIDE IT, AND SEPARATING THE TWO IS THE WHOLE POINT.** Nothing
	 * validated, so nothing may be reported as validated — but "checked and refused" and "not
	 * checked" are different answers, and a caller preflighting SEVERAL graphs has to tell them
	 * apart. One graph nobody could preview is not a reason to refuse its fifteen siblings, which is
	 * exactly what the whole-file route was doing (T-3): a `functions` block carrying sixteen graphs
	 * was refused entire because one of them declared a new local.
	 *
	 * READ AS A FLAG, NEVER MATCHED OUT OF `Message`. The same paragraph in `ApplyGraph` that
	 * declines the check says why: matching another file's wording couples to it and stops working
	 * silently when it changes.
	 *
	 * One cause today — a payload declaring a local the function does not have yet. See the
	 * `NewlyDeclaredLocalNames` branch in `ApplyGraph`.
	 */
	bool bPreviewUnavailable = false;

	/** Whether a compile ran, and what it said. */
	enum class ECompile : uint8
	{
		/** No compile ran: nothing was written, or bCompile was false. */
		NotAttempted,
		Succeeded,
		/** Compiled with errors. The graph is still Written - see the note at the top. */
		Failed,
		/**
		 * A compile was asked for and a `FBpDeferredCompileScope` took it.
		 *
		 * NOT `NotAttempted`, which means nobody asked, and emphatically not `Succeeded`, which is
		 * what this used to report: with a scope open the compiler has not looked at this graph
		 * yet, so an empty error list here means nothing at all. The scope compiles the asset once
		 * at `Flush` and the caller that opened it reports what came back - `ApplyDocument` puts it
		 * in the document's `compile_messages`. Board row T-13.
		 */
		Deferred,
	} Compile = ECompile::NotAttempted;

	/**
	 * Whether the asset has unsaved changes as this call returns.
	 *
	 * A property of the ASSET, not of this call - it can be true because of an edit made
	 * before this one. Read it together with Mutation, which is what says whether this
	 * call is responsible. Nothing in this file ever saves.
	 */
	bool bDirty = false;

	/** Human-readable summary. */
	FString Message;

	/** Signature of the graph AFTER the apply, for chaining edits without a re-read. */
	FString NewSignature;

	/**
	 * Symbols that did not resolve to exactly one entity, one entry per statement.
	 *
	 * NotFound and Ambiguous both land here and both carry the alias so the caller can
	 * locate them. Ambiguous entries list the candidates: the resolver never picks, and
	 * neither does this - a silent pick is the hallucination failure wearing a hat.
	 */
	TArray<FString> ResolutionErrors;

	/**
	 * Statements carrying a target this validator does not yet check, one per statement.
	 *
	 * Reported rather than passed over in silence. "Validated" and "not looked at" must
	 * never be indistinguishable in a result, or the caller reads a clean pass as a
	 * guarantee it never received.
	 */
	TArray<FString> UnvalidatedStatements;

	/**
	 * Schema rejection reasons, verbatim from FPinConnectionResponse. Never
	 * reworded - the exact string is the diagnostic.
	 */
	TArray<FString> ConnectionErrors;

	/**
	 * Compiler errors and warnings from the post-apply compile that belong to THIS graph.
	 *
	 * The compile is whole-asset - there is no other kind - so its log is whole-asset too, and
	 * reporting all of it here made every write against an asset with a pre-existing error read as
	 * a failure. Measured while repairing BP_ItemManager: each successful fix returned
	 * "Not every graph in the block was written" with the full remaining list, when the actual news
	 * was that the count went 12 -> 10 -> 8 -> 6 -> 4 -> 0. Writing three overrides in one payload
	 * was worse: every not-yet-written override was still missing when the first one compiled, so a
	 * call that ended 0 errors / 0 warnings reported nine and failed. A caller reading that would
	 * roll back a write that had succeeded.
	 *
	 * Attribution is POSITIVE-ONLY: a message moves to the Elsewhere pair below when a token in it
	 * names a node or graph that provably belongs to another graph. A message naming nothing
	 * resolvable - "the Blueprint could not be compiled" - stays here, because the alternative is
	 * exonerating a write on an absence of evidence. bSuccess follows this array alone.
	 */
	TArray<FString> CompileErrors;
	TArray<FString> CompileWarnings;

	/**
	 * The same compile's errors and warnings that provably belong to some OTHER graph in the asset.
	 *
	 * Reported and not blamed. They are usually pre-existing, and where they are not - the second
	 * of three overrides written from one payload - they are about work this call has not reached
	 * yet. Either way the caller needs to see them: an asset that still will not compile is news,
	 * it is just not this graph's failure.
	 */
	TArray<FString> CompileErrorsElsewhere;
	TArray<FString> CompileWarningsElsewhere;

	/**
	 * Legal graphs that are probably not what the caller meant — C1a, and they INFORM rather than
	 * refuse.
	 *
	 * The distinction from `ConnectionErrors` is the whole design. Everything there is a payload
	 * the engine will not accept; everything here is a payload it accepts happily, that the editor
	 * itself would let a human build, and that will still not do what the text looks like it does.
	 * An impure node whose exec input nothing reaches is the case that motivated this: it is valid,
	 * it compiles, and it returns its type default forever.
	 *
	 * REFUSING THESE WAS THE FIRST DESIGN AND IT WAS WRONG. A disconnected branch is ordinary
	 * mid-construction state, and a writer that rejected it would refuse graphs the Blueprint editor
	 * creates every day. The ruling is §7's: make the surprising thing explicit rather than
	 * impossible — report what was found, name it precisely enough to act on, and then do as you
	 * were told.
	 *
	 * Computed from the CONSTRUCTED nodes, never from the payload text, because purity and pin
	 * requirements are facts about the node Unreal built and nothing the caller writes can restate
	 * them.
	 */
	TArray<FString> Diagnostics;

	/**
	 * What a DRY RUN found this write would reach outside the graph it names, one line each.
	 *
	 * Its own field rather than a line in `Diagnostics`, which means *this graph is legal and
	 * probably not what you meant*. This is not about the payload at all: a signature change
	 * reconstructs every call site of the graph, and a call site in another Blueprint is another
	 * asset's unsaved edit made by a write that never named it. The real write reports those
	 * after the fact, in its message; a caller preflighting the same payload could not find out
	 * beforehand, which is what this answers.
	 *
	 * Empty on a real write - `RepairCallSites` reports what it DID, and a field that meant
	 * "would" in one call and "did" in another would mean neither.
	 */
	TArray<FString> CallSitePreview;

	/**
	 * Node classes the ENGINE replaced during the build, one line each.
	 *
	 * Not errors, and not warnings about something that might happen: the substitution has
	 * already been made and the graph holds the replacement. `MakeStruct` on a struct with a
	 * native make function is the case that exists today - the engine rewrites it on every
	 * load, so writing one produces a graph that reads back differently the first time the
	 * asset is opened. Doing it at build time makes the write stable; reporting it is what
	 * stops the caller believing its payload was written verbatim when it was not.
	 */
	TArray<FString> Substitutions;

	/**
	 * Aliases the payload asked for that the written graph does not use, one line each as
	 * `authored -> minted`.
	 *
	 * An alias is a persistent identity now (AuthoringAlias.h), and the applier writes it down.
	 * It writes down the number the READER would give the node, not necessarily the one the
	 * payload picked - a hand-authored `n_temp`, or `n1000000`, or a number already claimed by a
	 * node this payload kept. Every such case is reported here rather than applied silently,
	 * because an agent that composed the payload will reference those aliases in its next edit
	 * and has no other way to learn what they became.
	 *
	 * Empty is the normal case: a payload derived from a read re-uses names already written down,
	 * and those are stamped back unchanged.
	 *
	 * Covers the graph named by the call. A COLLAPSED graph's statements are stamped too but its
	 * renames are not listed here - it is built by its own builder, which is gone before the
	 * reconciliation runs. Re-read the asset to see what a collapsed graph's nodes are called.
	 */
	TArray<FString> AliasesRenamed;

	/** Nodes an EARLIER write left outered to this graph and absent from it, swept before this write
	 * (`FBpGraphBuilder::EvictOrphanedNodes`). A repair, not a change the payload asked for. */
	int32 OrphansRemoved = 0;

	int32 NodesCreated = 0;
	int32 NodesDeleted = 0;

	/**
	 * DIFFERENTIAL ONLY, and therefore always zero today.
	 *
	 * Replace rebuilds every node, so nothing is modified-in-place and nothing is left
	 * untouched. Callers must not report these while they are unpopulated: a result
	 * carrying `nodes_untouched: 0` reads as "the whole graph moved", which is true, next
	 * to `nodes_modified: 0`, which reads as "no changes were made", which is not.
	 * Zero-as-unimplemented and zero-as-measured must not be spelled the same.
	 */
	int32 NodesModified = 0;
	int32 NodesUntouched = 0;
};

namespace BlueprintIR
{
	/**
	 * Whether this writer may author `Graph` at all, decided by the graph's SCHEMA.
	 *
	 * An ALLOW-LIST of exact classes, and both halves of that are load-bearing.
	 *
	 * Exact, because the obvious test is useless here. `UAnimationGraphSchema`,
	 * `UAnimationTransitionSchema` and `UAnimationConduitGraphSchema` all derive from
	 * `UEdGraphSchema_K2`, so `IsChildOf(UEdGraphSchema_K2)` answers TRUE for an AnimGraph and a
	 * guard written that way would pass exactly the case it exists to stop. (`UAnimationStateGraphSchema`
	 * derives from the anim schema and `UAnimationStateMachineSchema` from plain `UEdGraphSchema`,
	 * so the family is not even consistent about its own base.)
	 *
	 * A list rather than `== UEdGraphSchema_K2`, because a Widget Blueprint's graphs are
	 * `UWidgetGraphSchema` (`WidgetBlueprint.cpp:588`) and this writer authors them today. An exact
	 * match on K2 alone would refuse every WBP graph write - a shipping path - to fix an
	 * Animation Blueprint one. Measured: the engine and its plugins define five K2 descendants in
	 * all, three anim, one property bag, one widget.
	 *
	 * ALLOW-LIST rather than deny-list because of which way each one fails. An unknown schema is
	 * one nobody has proved this writer understands, and the failure it is protecting against is
	 * silent: see the note on the refusal in ApplyGraphImpl. A deny-list would author it anyway
	 * and find out afterwards.
	 *
	 * Says nothing about READING. An AnimGraph reads in full today, with zero unsupported notes
	 * (`GRAPHCODE-ROADMAP.md` G-3), and this must not be reached for to take that away.
	 */
	BLUEPRINTIR_API bool IsAuthorableGraphSchema(const UEdGraph* Graph);

	/** The refusal `IsAuthorableGraphSchema` earns, naming the schema found. */
	BLUEPRINTIR_API FString UnauthorableGraphRefusal(const UEdGraph* Graph);

	/**
	 * Apply IR to a Blueprint graph.
	 *
	 * Validates in full first and writes nothing unless everything passed. Replace mode
	 * rebuilds the graph; Differential is refused with an explanation, never downgraded.
	 * Nothing here saves - the asset is left dirty and the result says so.
	 */
	/**
	 * Every CustomEvent name these graph payloads declare.
	 *
	 * Exported because the DOCUMENT layer has to seed `FBpApplyOptions::DeclaredEvents` and must
	 * not answer "is this statement a declaration" for itself - that rule lives with the applier
	 * and has one subtlety worth stating twice being enough to get wrong: a `ComponentBoundEvent`
	 * is an event shape and is NOT a declaration.
	 */
	BLUEPRINTIR_API TArray<FString> DeclaredCustomEventNames(const TArray<FBpIrGraph>& Graphs);

	BLUEPRINTIR_API FBpApplyResult ApplyGraph(const FString& BlueprintPath, const FBpIrGraph& Ir,
		const FBpApplyOptions& Options);
}
