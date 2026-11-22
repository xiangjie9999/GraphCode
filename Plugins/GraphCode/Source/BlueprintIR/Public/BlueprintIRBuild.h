// Blueprint IR - turning statements into nodes.
//
// The write half of the round trip. `BlueprintIRReader` reads a graph into IR; this puts
// IR back into a graph, and the two are only worth having if `read(build(ir)) == ir`.
//
// Split out of BlueprintIRApply.cpp because ApplyGraph's job is POLICY - which mode, is
// the base signature current, does every symbol resolve, does the payload look truncated -
// and this file's job is MECHANISM. Keeping them apart means the round-trip harness can
// drive construction directly without going through the concurrency checks, and means a
// mode added later (Differential, Graft) reuses the phases rather than reimplementing them.
//
// See BlueprintIRBuild.cpp for the three phases and why the middle one is the point.

#pragma once

#include "CoreMinimal.h"
#include "BlueprintIRTypes.h"

class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphNode_Comment;
class UEdGraphPin;
class UEdGraphSchema;
class UEdGraphSchema_K2;

namespace BlueprintIR
{
	/**
	 * The node class an ADR-8 op token names, or null when nothing in this build has that
	 * name or more than one class does.
	 *
	 * Two spellings, and the second is an escape rather than an alternative:
	 *   `CallFunction`                                  the class name, K2Node_ stripped
	 *   `/Script/BlueprintGraph.K2Node_CallFunction`    the full class object path
	 *
	 * The stripped form is unqualified, so two modules can collide on it; such a token
	 * resolves to null, and the path form is the only way to name either class. A token
	 * starting with '/' is always read as a path - an identifier cannot start with one, so
	 * the two forms are distinguishable without trying either speculatively.
	 *
	 * Shared rather than duplicated: the applier asks it to work out a statement's SHAPE
	 * (is this a call? does it need a target?) and the builder asks it to CONSTRUCT the
	 * node. Two copies of this lookup would be two answers to "what is a MakeStruct",
	 * and the validation half would eventually bless a payload the build half cannot make.
	 */
	BLUEPRINTIR_API const UClass* NodeClassForOp(const FString& Op);

	/**
	 * The op token to EMIT for a node class - the inverse of NodeClassForOp, and its partner.
	 *
	 * Returns the stripped name when that name resolves back to this exact class, and the
	 * full class path when it does not. So the reader never emits a token the builder would
	 * refuse as ambiguous, and `NodeClassForOp(OpTokenForNodeClass(C)) == C` holds for every
	 * node class in the build.
	 */
	BLUEPRINTIR_API FString OpTokenForNodeClass(const UClass* NodeClass);

	/**
	 * Is this bare name scoped to the graph's own FUNCTION rather than to a class?
	 *
	 * The reader emits a bare name for two different things - `IsSelfContext()` keeps an
	 * own-class member unqualified, and a non-empty `GetMemberScopeName()` keeps a
	 * function-scoped one unqualified (§16) - so the name alone cannot say which, and every
	 * half of the write path that guesses gets it wrong somewhere. Asked once here, so the
	 * applier's "must this exist as a project symbol?" and the builder's "SetLocalMember or
	 * SetSelfMember?" cannot disagree. They did, and the cost was a graph that validated and
	 * then built its variable node WITHOUT the pin it is named after (§18).
	 *
	 * FUNCTION SCOPE IS ONE CONCEPT, NOT LOCALS PLUS A CASE FOR PARAMETERS. A UFunction's
	 * property chain holds its parameters, its return value and its locals alike, so asking
	 * the function itself answers for all of them at once - including the ones no other
	 * source knows about:
	 *
	 *  - a parameter of an OVERRIDE has no `UserDefinedPins` on the entry node at all, because
	 *    the signature is inherited from the function being overridden
	 *    (`BP_Extrude.GetSurfaceFromPoint`), so reading the payload's entry node would answer
	 *    correctly for ordinary functions and wrongly for overrides;
	 *  - a hidden parameter like `__WorldContext` is declared by nothing the payload can see.
	 *
	 * Scoped by `Ir.Name` because a function graph and its UFunction share a name, and
	 * resolved against the SKELETON class because that is what `FMemberReference::
	 * ResolveMember` will use when the builder writes `SetLocalMember` - validating against
	 * anything else would bless a name the write then cannot resolve.
	 *
	 * The payload's own `local` declarations are consulted FIRST and independently, because a
	 * payload is allowed to declare a local the function does not have yet; that one is true
	 * of the graph being written rather than of the graph that exists.
	 *
	 * A qualified name is never function-scoped: `Class:Member` names a class member by
	 * construction.
	 */
	BLUEPRINTIR_API bool IsFunctionScoped(const FBpIrGraph& Ir, const UBlueprint* Blueprint,
		const FString& Name);

	/**
	 * Builds one graph's worth of nodes from IR.
	 *
	 * Stateful on purpose: the alias -> node map built in phase 1 is what phases 2 and 3
	 * read, and it is also what a differential mode would seed with the nodes it decided to
	 * keep. Errors are accumulated by the caller rather than thrown, because the rule the
	 * whole write path is built on is report EVERY problem, never the first.
	 */
	class BLUEPRINTIR_API FBpGraphBuilder
	{
	public:
		FBpGraphBuilder(UBlueprint* InBlueprint, UEdGraph* InGraph);

		/**
		 * Take every existing node out of the graph, WITHOUT destroying it, and return them.
		 *
		 * Replace mode only - ADR-7 Rule 2. Detach rather than delete because
		 * FScopedTransaction::Cancel discards the undo record instead of applying it, so
		 * rollback has to be something this class can actually do. See the .cpp.
		 */
		TArray<UEdGraphNode*> DetachExistingNodes();

		/**
		 * Differential's counterpart: detach every existing node whose GUID is NOT in
		 * Surviving, and leave the rest exactly where they are - still in the graph, still
		 * wired, not even touched by this call. ADR-7 Rule 2 still governs what comes back:
		 * an existing alias the payload does not re-emit is one of these, headed for
		 * deletion on commit exactly like a Replace's whole node set is.
		 */
		TArray<UEdGraphNode*> DetachOrphanedNodes(const TSet<FGuid>& Surviving);

		/** Put them back and drop everything this builder made. The rollback. */
		void RestoreNodes(const TArray<UEdGraphNode*>& Detached);

		/** Let them go. The commit. */
		void CommitDetached(const TArray<UEdGraphNode*>& Detached);

		/**
		 * Rename every `UEdGraphNode` outered to this graph but absent from `Graph->Nodes` into the
		 * transient package, and return how many there were.
		 *
		 * These are LEAKS from writes made before `CommitDetached` evicted what it replaced: an
		 * unreferenced node is still an export, because `SavePackage` walks outers rather than
		 * references, so it was written into the `.uasset` and reloaded forever after. Invisible to
		 * every projection and to every diff — but not to the engine, which reconstructs it on load
		 * and can log about it.
		 *
		 * CALL IT FROM A WRITE, never from a read. Repairs the asset it is already editing.
		 *
		 * `OutTransient` is ADDED TO with the subset that were `RF_Transient`, which is the half of
		 * the return value that is NOT damage: a transient object is never serialized, so it was
		 * never in the `.uasset` the paragraph above describes. They are scratch a projection left
		 * attached to the graph — see `MakeBaselineNode` — and their number says how often the asset
		 * has been READ, not how broken it is. A caller that reports the raw total as damage will
		 * overstate it by orders of magnitude on any asset somebody has looked at (measured: 2004
		 * reported, eighteen real). Optional, because a caller that never shows the number to anybody
		 * does not have to care which kind it swept.
		 */
		int32 EvictOrphanedNodes(int32* OutTransient = nullptr);

		/**
		 * Phase 1: every node, from its op plus its reference and property channel.
		 *
		 * ReusableByAlias is Differential's seed: an alias present in it, whose node's class
		 * matches the statement's op, gets that EXACT UEdGraphNode back - Modify()'d, GUID
		 * and position untouched - instead of a fresh NewObject. Empty for Replace, where
		 * nothing is ever reused, so every alias falls through to construction exactly as
		 * before.
		 *
		 * An alias present in the map whose class does NOT match is a rebuild: a fresh node,
		 * same alias, but its OLD position is copied onto it directly here, never through
		 * Layout - ADR-7 Rule 1 is explicit that position is preserved and identity is not
		 * (`BP-AUTHORING-DESIGN.md`, the Rule 1 table).
		 */
		bool Construct(const FBpIrGraph& Ir, const TMap<FString, UEdGraphNode*>& ReusableByAlias,
			TArray<FString>& OutErrors);

		/**
		 * Phase 1b: let the ENGINE replace any node it considers deprecated, and say so.
		 *
		 * Some node classes are no longer what the engine wants a graph to hold, and it
		 * rewrites them on load - `MakeStruct` on a struct with a native make function becomes
		 * a `CallFunction` to that function. Building one and stopping there produces a graph
		 * that reads back as what the payload asked for right up until the asset is reloaded,
		 * and then reads back as something else. Doing the substitution HERE makes the written
		 * graph the one a reload would produce, so a read is stable across a trip to disk.
		 *
		 * Each substitution is appended to OutSubstitutions, never silent: the caller asked
		 * for one node class and got another, and the result says which and why.
		 *
		 * Runs between Construct and Settle, and both edges matter. After Construct, because
		 * the property channel has to have set `StructType` before the engine can tell whether
		 * the struct has a native make. Before Settle and Connect, because the engine's own
		 * replacement migrates pin LINKS but not pin DEFAULTS - so converting after literals
		 * were bound would drop every one of them.
		 */
		bool Canonicalize(const FBpIrGraph& Ir, TArray<FString>& OutSubstitutions);

		/** Phase 2: literal pin values, ReconstructNode, literal pin values again. */
		bool Settle(const FBpIrGraph& Ir, TArray<FString>& OutErrors);

		/**
		 * Phase 2b: an alias node's `AliasedStateNodes`, from the statement aliases the document
		 * names.
		 *
		 * Its own pass at the END of Settle rather than a line inside that loop, because every
		 * statement it names must already have survived `ReconstructNode` — an alias addresses
		 * other nodes, and nothing else in phase 2 does.
		 *
		 * THE PROPERTY CHANNEL CANNOT DO THIS AND IS NOT ASKED TO. It carries the engine's own
		 * export, which is a set of absolute paths naming `AnimStateNode_0` in the graph this
		 * build REPLACED; importing those aliases nodes that no longer exist, and
		 * `UAnimStateAliasNode::Serialize` then drops every one of them
		 * (`RebuildAliasedStateNodeReferences`). The write reported success and the alias came
		 * back empty — `GC-STATE-MACHINE-DESIGN.md` §6 2c.
		 */
		void BindAliasedStates(const FBpIrGraph& Ir, TArray<FString>& OutErrors);

		/** Phase 3: data edges then exec edges, each through CanCreateConnection. */
		bool Connect(const FBpIrGraph& Ir, TArray<FString>& OutErrors);

		/**
		 * Phase 3c: the terminator a declared `output { }` has nowhere else to live in.
		 *
		 * A parameter is a pin on `UK2Node_FunctionEntry` and a RETURN is a pin on
		 * `UK2Node_FunctionResult`, so `output { }` reaches the graph only through a result node.
		 * `AddFunctionGraph` creates the entry and nothing creates the other one, so a payload
		 * that declares returns and spells no `FunctionResult` statement had its whole return
		 * signature dropped WITHOUT A WORD - created, saved and projected back with the outputs
		 * gone (`GC-DISCOVERY-CHECKLIST.md` F19, measured on a Blueprint interface, where a
		 * signature with no body is the ordinary thing to write).
		 *
		 * So the node is made here, through the ENGINE'S OWN `FindOrCreateFunctionResultNode` -
		 * the same call the details panel's "add output" runs, including its entry->result exec
		 * link when the entry's `then` is free. This is not an invention: the entry terminator
		 * already arrives without a statement asking for it, and this is that rule applied to the
		 * other one.
		 *
		 * Nothing happens when the graph already holds a node that TAKES the declarations - its
		 * own `FunctionResult` statement, a macro's output tunnel, a Differential apply's kept
		 * node - so this can only ever add the terminator a graph is missing, never move or
		 * replace one.
		 *
		 * AFTER `Connect`, because the engine helper links the entry only while its `then` pin is
		 * unwired, and until phase 3 has run the payload's own wiring does not exist yet.
		 *
		 * @return the node it created, or null when it created none. The caller REPORTS a non-null
		 *         answer: the graph now holds a node the document did not spell, and a caller who
		 *         is not told writes the same payload again forever.
		 */
		UEdGraphNode* EnsureDeclaredOutputsHaveATerminator(
			const FBpIrGraph& Ir, TArray<FString>& OutErrors);

		/**
		 * Phase 3b: state the document's property channel again, now that the graph is WIRED.
		 *
		 * A node class is entitled to react to its own connections, and some of them react by
		 * ERASING a field. `UK2Node_CreateDelegate::HandleAnyChangeWithoutNotifying` clears
		 * `SelectedFunctionName` whenever the node is invalid and its delegate pin has no links -
		 * and `PostReconstructNode` calls it, so Settle's `ReconstructNode()` (phase 2, before any
		 * wire exists) wipes the bound function of every delegate binding in the payload. Nothing
		 * downstream put it back: the write completed, the Blueprint compiled, and four event
		 * bindings in a real widget came back as bare `CreateDelegate()`. Queue row 11r.
		 *
		 * Phase 2 already answers the same shape of problem for pin LITERALS - set, reconstruct,
		 * set again - and this is that ruling extended to the property channel, with wiring rather
		 * than reconstruction as the event that can undo it.
		 *
		 * IT RESTORES WHAT PHASE 1 SETTLED ON, NOT WHAT THE DOCUMENT SAYS, and the distinction is
		 * the whole correctness of the phase. Phase 1 has several steps that deliberately outrank
		 * the property channel - the metadata block resets `MetaData` after the channel wrote it
		 * (ADR-10), the declaration blocks own the signature and the locals - so a repair aimed at
		 * the document would undo them and reinstate a value the payload had asked to drop. That
		 * is not hypothetical: aiming it at the document turned `GraphMetadata.omission resets`
		 * red on its first run, which is exactly the spec written to catch a reset being lost.
		 * `ConstructedProperties` is the baseline.
		 *
		 * @return how many nodes had something put back. Zero on a healthy write, which is the
		 *         normal case: nothing was erased, so nothing is re-stated.
		 */
		int32 ReassertProperties(const FBpIrGraph& Ir, TArray<FString>& OutErrors);

		/**
		 * Say which of the document's statements the built graph does not contain.
		 *
		 * THE ONE CHECK THAT DOES NOT TRUST THIS FILE. Every phase above reports what it could not
		 * do; this reports what nobody noticed - a node a hook destroyed, or one dropped after the
		 * phase that built it. Row 11r exists because an identical payload produced a different
		 * asset and every phase reported success, so a verification derived from the phases
		 * themselves would have agreed with them.
		 *
		 * Its output is ERRORS: a write that would produce a graph missing something the document
		 * asked for is rolled back, not completed and compiled.
		 *
		 * Run it AFTER `ReassertProperties`. Statements only - comments and owned-graph names are
		 * built after this point and have phases of their own.
		 */
		void ReportLostContent(const FBpIrGraph& Ir, TArray<FString>& OutLost) const;

		/**
		 * Positions for nodes that have none, via the shared FBlueprintGraphLayout.
		 *
		 * Runs AFTER Connect, never before: the organiser lays out along the wires, so a
		 * graph whose wires do not exist yet lays out as a stack of disconnected islands.
		 *
		 * Targets is what to place. Empty lays out the whole graph (Replace, which has no
		 * anchor to preserve and nothing outside the set to avoid). Differential passes
		 * exactly its genuinely-new-alias nodes here - a reused node keeps the position it
		 * already had, and a class-changed rebuild already has its captured old position set
		 * in Construct, so neither is ever a Layout target; running either through Organize
		 * as well would silently move a node this whole mechanism exists to hold still.
		 */
		void Layout(const TArray<UEdGraphNode*>& Targets, FString& OutError);

		/**
		 * Post-layout comment phase. Differential reuses boxes by alias/GUID; Replace passes
		 * no base and creates every box. Changed membership refits, while text/color-only
		 * edits keep the authored rectangle exactly.
		 */
		void BuildComments(
			const FBpIrGraph& Ir,
			const FBpIrGraph* LiveIr,
			const TMap<FString, UEdGraphNode_Comment*>& ReusableByAlias);

		int32 GetNodesCreated() const { return NodesCreated; }
		int32 GetNodesDeleted() const { return NodesDeleted; }
		int32 GetNodesReused() const { return NodesReused; }

		/** The node phase 1 built for this alias, or null - e.g. for a truncated payload. */
		UEdGraphNode* GetNode(const FString& Alias) const
		{
			UEdGraphNode* const* Found = Nodes.Find(Alias);
			return Found ? *Found : nullptr;
		}

	private:
		/**
		 * Move one node out of the asset's package, keeping it alive and fully wired.
		 *
		 * The single reason a node stops being saved. Renaming rather than destroying is what lets
		 * `CommitDetached` keep its promise that an undo restores a wired graph and not just a
		 * node count.
		 */
		bool EvictFromPackage(UEdGraphNode* Node);

		/**
		 * Fill in the MemberGuid of every local-variable reference from the payload's own
		 * function entry. The IR names a local but never spells its guid, and the guid is
		 * what survives a later rename - see the .cpp. Runs at the end of Construct.
		 */
		void BindLocalVariableGuids();

		/**
		 * Re-point the GRAPH's own typed references at the nodes this build put in its place.
		 *
		 * A graph class may hold a pointer to one of its own nodes:
		 * `UAnimationStateMachineGraph::EntryNode`, `UAnimationStateGraph::MyResultNode`,
		 * `UAnimationTransitionGraph::MyResultNode`. The engine sets them in
		 * `CreateDefaultNodesForGraph`, and rebuilding the graph's node set leaves them pointing
		 * at a node that is no longer in it - so the machine has no entry state and the state has
		 * no result node, while the projection is byte-identical because the reader reads NODES.
		 * That is the failure ruling 2 exists to make unsayable, and text equality cannot see it.
		 *
		 * Reflection rather than a class list: the rule is *this graph referenced one of its own
		 * nodes*, which the property system can answer for any graph class. A pointer into ANOTHER
		 * graph (`OwnerAnimGraphNode` names the node that owns this one) is left alone, and a plain
		 * `UEdGraph` has no such property at all, so every K2 graph passes through untouched.
		 */
		void RebindGraphOwnedNodeReferences(TArray<FString>& OutErrors);

		void BindLiterals(const FBpIrStatement& Statement, UEdGraphNode* Node, TArray<FString>& OutErrors);
		void MakeConnection(const UEdGraphSchema* Schema, UEdGraphPin* From, UEdGraphPin* To,
			const FString& Where, TArray<FString>& OutErrors);

		UBlueprint* Blueprint = nullptr;
		UEdGraph* Graph = nullptr;

		/**
		 * Name -> guid for every local the graph held BEFORE this build, snapshotted in the
		 * constructor. A local's `local` line cannot spell its guid, so a surviving name's
		 * identity has to be carried in rather than derived - see the constructor and
		 * NodeProperties::ApplyDeclaredLocals.
		 */
		TMap<FName, FGuid> PreexistingLocalGuids;

		/** Alias -> the node it built. Total after phase 1, which is why phase 1 is first. */
		TMap<FString, UEdGraphNode*> Nodes;

		/**
		 * Alias -> the value phase 1 left on each property the document names.
		 *
		 * The baseline `ReassertProperties` compares against, and the reason it compares against
		 * this rather than against the document: several things in phase 1 deliberately outrank the
		 * property channel - the metadata block resets `MetaData` after the channel wrote it
		 * (ADR-10), the declaration blocks own the signature and the locals - so a repair aimed at
		 * the document would undo them. What phase 1 ENDS with is the write's answer for a node;
		 * phases 2 and 3 are not entitled to change it silently.
		 */
		TMap<FString, TArray<TPair<FString, FString>>> ConstructedProperties;

		/**
		 * What DetachExistingNodes/DetachOrphanedNodes left in Graph->Nodes, untouched.
		 * Empty for Replace, where nothing survives - which is what makes RestoreNodes one
		 * implementation for both: Graph->Nodes = KeptNodes then append Detached is exactly
		 * Graph->Nodes = Detached when KeptNodes is empty.
		 */
		TArray<TObjectPtr<UEdGraphNode>> KeptNodes;

		/**
		 * The nodes phase 1 reused in place, whose off-pin state it reset to the constructed
		 * value (`NodeProperties::ResetChannelToConstructed`).
		 *
		 * Phase 2 needs the set because a pin that DISAPPEARS on such a node disappears by the
		 * document's instruction, and the engine's default answer to a vanishing pin carrying a
		 * literal is to keep it as an orphan. On a reused node that ghost is unreachable: the
		 * reader skips orphaned pins, so no `.gc` can name it, and no later edit can remove it.
		 * See `Settle`.
		 */
		TSet<TObjectPtr<UEdGraphNode>> ResetInPlace;

		int32 NodesCreated = 0;
		int32 NodesDeleted = 0;

		/** Statements resolved to an existing node kept in place, rather than constructed. */
		int32 NodesReused = 0;
	};
}
