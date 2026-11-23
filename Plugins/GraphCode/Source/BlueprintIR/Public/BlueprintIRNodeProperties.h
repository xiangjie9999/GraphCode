// Blueprint IR - the property channel (ADR-8's second half).
//
// ---------------------------------------------------------------------------
// WHAT THIS IS
// ---------------------------------------------------------------------------
// A node's state lives in three places, and only one of them is pins:
//
//   1. its CLASS            -> the op token (ADR-8). Reader: OpTokenForNode
//   2. its PINS             -> Args / ExecOut / OutPins
//   3. its own UPROPERTYs   -> THIS FILE
//
// (3) is what makes `MakeStruct` a MakeStruct *of something*. The class says
// "UK2Node_MakeStruct"; the `StructType` property says which struct, and without it the
// node has no pins at all because its pins ARE that struct's members. Same for
// `SwitchEnum`'s `Enum`, `Select`'s `NumOptionPins`, `CreateDelegate`'s
// `SelectedFunctionName`, `CustomEvent`'s `bCallInEditor`, a function entry's `ExtraFlags`
// (which is where replication and access specifier live).
//
// ---------------------------------------------------------------------------
// THE RULE: REFLECTION DECIDES, NOT A TABLE
// ---------------------------------------------------------------------------
// There is no list of node classes here and there must never be one. The channel is
// defined by walking `Node->GetClass()`'s properties and asking each one type-level and
// flag-level questions. A node class from a plugin written after this file therefore
// round-trips its own state with nobody adding a case for it - which is the same argument
// ADR-8 made for the op token, applied to the state instead of the identity.
//
// Concretely, a property is IN the channel unless one of these says otherwise, and every
// one of them is a question about the property, never about the node:
//
//   Structural       declared by UEdGraphNode or above. Pins, position, comment, GUID,
//                    enabled state. The IR models these itself or omits them on purpose
//                    (positions, ADR-5), so carrying them twice would give one fact two
//                    declaration sites that can disagree
//   Transient        CPF_Transient. Not persisted by the engine either; a value derived
//                    at load. Writing one back is writing to a scratch variable
//   CarriedElsewhere the same fact is already in the statement. Exactly two types:
//                    FMemberReference and FGraphReference, both rendered as `Target`
//   Default          equal to what the statement already implies - see MakeBaselineNode, which
//                    is a node of this class pointed at this target, not a bare CDO. Emitting
//                    it costs tokens on every line and says nothing
//
// EVERYTHING ELSE IS EMITTED, whatever its type, using the engine's own
// ExportTextItem/ImportText pair - so the value form is the engine's and the two
// directions are inverses by construction rather than by a matched pair of hand-written
// renderers that can drift.
//
// ---------------------------------------------------------------------------
// THE BOUNDARY, AND WHY IT IS LOUD
// ---------------------------------------------------------------------------
// Reflection reaches a node's own properties. Two kinds of state escape it, and both get
// an explicit per-item note rather than a clean-looking statement with the state gone:
//
//   * SUB-GRAPHS. A collapsed graph's `BoundGraph` is a whole UEdGraph hanging off the
//     node. The reference round-trips (it is an object path); its CONTENTS are a different
//     graph and are not in this graph's IR.
//   * STATE THE NODE KEEPS SOMEWHERE ELSE. A Timeline's curves live in a
//     UTimelineTemplate on the Blueprint, not on the node, so no amount of walking the
//     node's properties will find them. This is the one category that cannot be detected
//     generically, because there is nothing on the node to detect.
//
// ---------------------------------------------------------------------------
// ONE EXPLICIT CASE, AND WHY IT EARNS ITS PLACE
// ---------------------------------------------------------------------------
// `UK2Node_EditablePinBase::UserDefinedPins` is a `TArray<TSharedPtr<FUserPinInfo>>` -
// deliberately NOT a UPROPERTY, hand-serialised by the engine. Reflection cannot see it.
//
// It is also **a function's parameter list**: an entry node's user-defined pins are the
// function's inputs, a result node's are its outputs, a custom event's are its arguments.
// Without it a function signature round-trips half-missing, which the representability
// audit ranked as silent semantic loss.
//
// So it is read and written by hand - once, at `UK2Node_EditablePinBase`, which is the
// base of every node that has user-defined pins (FunctionEntry, FunctionResult,
// CustomEvent, Tunnel). It is a type-level case, not a per-class one, and it is the only
// hand-written entry in this file.

#pragma once

#include "CoreMinimal.h"

#include "BlueprintIRTypes.h"

class FProperty;
class UEdGraph;
class UEdGraphNode;

namespace BlueprintIR::NodeProperties
{
	/**
	 * WHILE THIS IS ACTIVE, ANYTHING THE ENGINE LOGS IS ABOUT A NODE THIS CODE JUST BUILT.
	 *
	 * **The defect it exists to end, measured 2026-08-14.** `MakeBaselineNode` constructs a bare
	 * scratch node of the same class and calls `AllocateDefaultPins()` on it. For a
	 * `UK2Node_BaseAsyncTask` that reaches `GetFactoryFunction()`, which logs an Error because the
	 * scratch has no proxy factory — by design, since the baseline is what a node looks like before
	 * anything is written to it. The scratch is outered to the REAL graph, so the message names the
	 * real asset:
	 *
	 *     FactoryFunction None null in K2Node_AsyncAction
	 *       /Game/Blueprints/EUW_RoomBuilder.EUW_RoomBuilder:EventGraph.K2Node_AsyncAction_1
	 *
	 * GraphCode's load-error listener attributes a message to the package named in it, so it wrote
	 * that line into `EUW_RoomBuilder.gc`'s `errors { }` — **a healthy asset accused of damage by the
	 * act of reading it**, growing by two entries per read, unfixable by any repair because the next
	 * read recreated it. It also failed an unrelated capability spec and charged log Errors to
	 * whichever automation spec happened to be running.
	 *
	 * A SCOPE RATHER THAN A FILTER ON THE MESSAGE TEXT, because the text is genuinely
	 * indistinguishable: a real broken node in that graph logs the identical sentence. The only thing
	 * that can tell them apart is knowing that WE built this one, and the only place that is known is
	 * here ([[reading-must-not-write]] — a read may not mutate the asset, and it may not slander it
	 * either).
	 *
	 * Reentrant, and counted rather than boolean: baselines are built inside baselines.
	 */
	class BLUEPRINTIR_API FScratchNodeScope
	{
	public:
		FScratchNodeScope();
		~FScratchNodeScope();

		/** True while any scope is open on this thread. Asked by log listeners. */
		static bool IsActive();

	private:
		FScratchNodeScope(const FScratchNodeScope&) = delete;
		FScratchNodeScope& operator=(const FScratchNodeScope&) = delete;
	};

	/**
	 * Which of a graph's two declaration blocks a node's user-defined pins ARE.
	 *
	 * `input { }` and `output { }` and this node's `UserDefinedPins` were two spellings of one
	 * fact for as long as both existed: 467 sites across 74 sidecars, and the readable one was
	 * dead - `FBpGraphBuilder` built a function's parameters entirely from the property blob, so a
	 * human who edited `input { Foo: int }` and applied it had the edit silently ignored.
	 *
	 * This is the one definition of "this node declares the graph's signature", and both ends use
	 * it: the reader to fill the blocks, the builder to put them back. Two answers to that
	 * question is how a signature comes to disagree with itself.
	 *
	 * THE DIRECTION IS THE ENGINE'S ANSWER, NOT A TABLE. `CanCreateUserDefinedPin` is asked in
	 * each direction, and a node that accepts exactly one is a terminator of that kind: a function
	 * entry takes only EGPD_Output, so its user pins are the graph's INPUTS; a function result
	 * takes only EGPD_Input, so its are the OUTPUTS; a tunnel answers from its own
	 * `bCanHaveInputs`/`bCanHaveOutputs`, which is why a macro's two ends need no case here.
	 *
	 * It also settles two cases that would otherwise need naming. `UK2Node_Composite` and
	 * `UK2Node_MacroInstance` both DERIVE from `UK2Node_Tunnel`, and both answer the same in both
	 * directions - a composite accepts either, a macro instance refuses both - so both come back
	 * `None` and neither has to be excluded by name.
	 *
	 * The class check that remains is `UK2Node_CustomEvent`, and it earns its place: a custom
	 * event accepts EGPD_Output exactly like a function entry, but it is not a graph terminator -
	 * its parameters have no declaration block anywhere. It is harmless in an event graph, which
	 * has no blocks to confuse it with, and NOT harmless inside a collapsed graph, which does.
	 * There is one such node in this project's corpus.
	 */
	enum class EDeclaredSignatureRole : uint8 { None, Inputs, Outputs };

	/** @see EDeclaredSignatureRole. Null and non-terminator nodes answer None. */
	BLUEPRINTIR_API EDeclaredSignatureRole DeclaredSignatureRoleOf(const UEdGraphNode* Node);

	/** Does this statement's property channel still state the `UserDefinedPins` blob? */
	BLUEPRINTIR_API bool StatesUserDefinedPins(
		const TArray<TPair<FString, FString>>& Properties);

	/**
	 * Rebuild a terminator's `UserDefinedPins` from the graph's declaration blocks.
	 *
	 * Runs AFTER the property channel on both sides, and callers skip it entirely when
	 * StatesUserDefinedPins says the blob is still there - so a document that carries the blob
	 * wins, which is what keeps a signature the declarations cannot spell (a delegate pin's
	 * member reference, a single-precision float) travelling exactly as it did before.
	 *
	 * AFTER, and this is not a preference. The role below is decided by asking the node which
	 * pin directions it accepts, and a function terminator accepts NEITHER until `bIsEditable`
	 * is true (K2Node_FunctionTerminator.cpp:112) - a flag that arrives in the property channel.
	 * Called first, a freshly constructed entry node answers "not a terminator" and its whole
	 * signature is dropped without a word. Skipping on the blob's presence is what keeps the
	 * precedence right now that the order is this way round; the two are never both
	 * authoritative, because the reader removes the blob exactly when the blocks reproduce it.
	 *
	 * A NON-EDITABLE node CLEARS the list rather than keeping it, and board row T-13 is why. This
	 * runs only where the statement states no blob, so by ADR-7 the answer is the constructed
	 * value - and `ResetChannelToConstructed` cannot supply it, because `UserDefinedPins` is a
	 * channel key and not a UPROPERTY. It is also the only place that undoes `PostPasteNode`'s pin
	 * promotion on an override, which `RestoreInheritedSignature` un-marks editable again a moment
	 * later. **The test is `IsEditable`, not the `None` role**, and the difference is a custom
	 * event: it is role `None` too, its parameters ARE its `UserDefinedPins`, and no other channel
	 * carries them - see the implementation. An EMPTY block is applied, not skipped - "declares
	 * none" is a signature, and a differential apply reuses a node that already has pins on it.
	 *
	 * @return false when a declaration's type does not resolve. That is an error the caller must
	 *         report: a parameter silently left at the wrong type compiles into a function that
	 *         does the wrong thing, which is the same rule ParseTerminalToken already follows.
	 */
	BLUEPRINTIR_API bool ApplyDeclaredSignature(UEdGraphNode* Node,
		const TArray<FBpIrVariable>& Inputs, const TArray<FBpIrVariable>& Outputs);

	/**
	 * Put a STATED `UserDefinedPins` back after the paste hook has grown it.
	 *
	 * The mirror of `ApplyDeclaredSignature`'s `None` branch and the same board row: between
	 * Construct's import and the reconstruct that turns the list into pins,
	 * `UK2Node_FunctionEntry::PostPasteNode` promotes every non-`then` pin the node still carries
	 * into `UserDefinedPins`. It is additive, so a reused node tops the payload's list up with the
	 * previous edit's entries and a document that SHRANK the blob applies as if it had not.
	 *
	 * A no-op on a node with no user pins and on a statement that states no blob, which is what
	 * lets the caller run it unconditionally.
	 *
	 * @return false with `OutError` set when the stated blob does not import, exactly as `Write`
	 *         would have reported it in Construct.
	 */
	BLUEPRINTIR_API bool ReapplyStatedUserDefinedPins(UEdGraphNode* Node,
		const TArray<TPair<FString, FString>>& Properties, FString& OutError);

	/**
	 * Does rebuilding from the declarations reproduce this node's live `UserDefinedPins` EXACTLY?
	 *
	 * This is the whole suppression rule, and it is the same rule `CompactPinTypeToken` uses on
	 * itself: the reduced form is parsed straight back and compared to what it came from, so
	 * anything the declaration channel cannot carry fails the comparison and keeps the blob. No
	 * list of carryable fields exists anywhere, and none can drift out of step with the parser,
	 * because it IS the parser.
	 *
	 * The four delegate pins in this project's corpus (`PinSubCategoryMemberReference`, which has
	 * no spelling in this grammar at all) and the five single-precision floats fail here and keep
	 * their blob. Nothing had to know that in advance - with one exception that had to be found
	 * the hard way: `FEdGraphPinType::operator==` is not a whole-struct comparison. It skips
	 * `bSerializeAsSinglePrecisionFloat` and `bIsUObjectWrapper`, so the single-precision case
	 * compared equal and lost its blob until those two were asked about by name.
	 */
	BLUEPRINTIR_API bool DeclaredSignatureReproduces(const UEdGraphNode* Node,
		const TArray<FBpIrVariable>& Inputs, const TArray<FBpIrVariable>& Outputs);

	// ---------------------------------------------------------------------------
	// The same exchange, one block down: a function's LOCALS
	// ---------------------------------------------------------------------------
	// `UK2Node_FunctionEntry::LocalVariables` and the graph's `local` lines were the same
	// duplication `UserDefinedPins` was, and the bigger one: 356 locals across 50 sidecars,
	// 125 KB - 5.2% of the whole committed corpus - including one 18,857-character line, because
	// the blob restates each local's DEFAULT VALUE and this project has four `ButtonStyle`
	// locals. The trio below is the same shape as the trio above, and deliberately so.
	//
	// It is NOT the same in one respect, and it is the respect that made this a feature rather
	// than a line of plumbing: `UserDefinedPins` has nothing in it the declaration cannot spell
	// once the type token is rich enough, while a local carries a `VarGuid` - its identity, which
	// nothing in the text can name and which the engine repairs stale references from
	// (MemberReference.cpp:468-472). So the rebuild is handed the guids as data.

	/** Does this statement's property channel still state the `LocalVariables` blob? */
	BLUEPRINTIR_API bool StatesLocalVariables(
		const TArray<TPair<FString, FString>>& Properties);

	/**
	 * Every local guid a graph currently holds, name -> guid, read off its function entry node(s).
	 *
	 * Taken BEFORE a build begins - `FBpGraphBuilder`'s constructor - because that is the last
	 * moment the graph still holds the nodes the payload is replacing. Under Replace the entry
	 * node is destroyed and rebuilt, so by the time `ApplyDeclaredLocals` runs there is nothing
	 * left to read a surviving local's identity off.
	 *
	 * Empty for a graph with no entry node, and for the preflight's scratch graph, where a fresh
	 * guid is minted onto a transient node nothing reads.
	 */
	BLUEPRINTIR_API TMap<FName, FGuid> ReadLocalVariableGuids(const UEdGraph* Graph);

	/**
	 * Rebuild a function entry's `LocalVariables` from the graph's `local` declarations.
	 *
	 * Runs in the same position `ApplyDeclaredSignature` does, and is skipped by the same rule:
	 * a statement that still states the blob wins, which is what keeps a `.gc` written before
	 * this applying exactly as it did. A node that is not a function entry is a no-op.
	 *
	 * @param GuidByName  from `ReadLocalVariableGuids`, taken before the build. A surviving name
	 *        keeps its guid; a name the payload adds gets a fresh one, exactly as the editor's +
	 *        button would give it one.
	 *
	 *        A RENAME therefore mints, and that is the ruling rather than an oversight. Keeping a
	 *        guid across a rename would mean pairing leftover names positionally, which is what
	 *        `properties { }` does for MEMBER variables - and it does it because a member's
	 *        referencers live in other assets that may never be loaded, so only the guid can
	 *        repair them. A local has no such referencer: every node that can name it is inside
	 *        the function being written, and `BindLocalVariableGuids` rebinds every one of them
	 *        in this same pass. So there is nothing for the old guid to save, and a positional
	 *        guess about which rename is which would be identity invented from nothing.
	 *
	 * @return false when a declaration's type does not resolve. The caller must report it: a
	 *         local silently left at the wrong type compiles into a function that does the wrong
	 *         thing, the same rule ApplyDeclaredSignature follows.
	 */
	BLUEPRINTIR_API bool ApplyDeclaredLocals(UEdGraphNode* Node,
		const TArray<FBpIrVariable>& Locals, const TMap<FName, FGuid>& GuidByName);

	/**
	 * The graph's details panel onto whichever node carries it. ADR-10.
	 *
	 * A no-op for any node that is not the carrier, exactly like ApplyDeclaredLocals, so the caller
	 * can run it per statement without knowing which one the entry is.
	 *
	 * WRITES EVERY FIELD, INCLUDING THE DEFAULTS. Omission resets (§10.1), and a differential apply
	 * hands this a reused node still carrying the previous edit's panel — so writing only the stated
	 * fields would make a dropped `category=` mean "leave it", which is the opposite of the rule.
	 *
	 * The five entry-only specifiers are written only for a function. On a macro the engine reads
	 * them from nowhere, and the parser refuses to let a macro payload state one, so writing them
	 * here could only ever write a value no document could have asked for.
	 */
	BLUEPRINTIR_API bool ApplyGraphMetadata(UEdGraphNode* Node, FBpIrGraph::EKind Kind,
		const FBpIrGraphMetadata& Metadata);

	/**
	 * Does rebuilding from the `local` declarations reproduce this node's live `LocalVariables`
	 * EXACTLY?
	 *
	 * The whole suppression rule, and the same proof shape as `DeclaredSignatureReproduces`: the
	 * reduced form is rebuilt and compared field by field, so nothing here has to hold a list of
	 * which fields the declaration channel can carry. A local with a tooltip, a category somebody
	 * typed or a RepNotify function fails the comparison and keeps its blob, and nobody had to
	 * know that in advance.
	 *
	 * WHAT IT ACTUALLY REFUSED, measured over the corpus rather than predicted - nine locals in
	 * 349 sidecars. **All three causes are resolved as of 2026-07-31 and NO local in this project
	 * keeps a blob.** The three are kept here because the resolutions differ, and the difference is
	 * the reusable part: one was a real gap in the grammar, two were the rule being asked the wrong
	 * question.
	 *
	 *  - **A single-precision float** (3, all in the XR framework) - A REAL GAP, fixed by adding a
	 *    spelling. The type token grew a third, STORAGE segment: `real:double:single`, a double in
	 *    the graph that serialises as 32 bits. It is NOT the same token as `real:float`, a pin that
	 *    is 32 bits throughout; both exist in this corpus, which is why one spelling could not serve
	 *    both. Note what it took: `bSerializeAsSinglePrecisionFloat` is not compared by
	 *    `FEdGraphPinType::operator==` (EdGraphPin.h:162-173), so `CompactPinTypeToken`'s round-trip
	 *    proof PASSED on a token that had silently dropped it - two `Select` hints in the committed
	 *    corpus stated `real:double` for pins that were single-precision. The proof now asks by name.
	 *  - **`CPF_DisableEditOnTemplate` on a local that is not an object at all** (5: a bool, an
	 *    int, a Vector, two doubles) - NOT a gap. The engine sets it for an Actor-typed variable and
	 *    clears it again only inside `if (new type is object/interface/soft-object)`
	 *    (`BlueprintEditorUtils.cpp:5762-5768`), so a local that WAS an Actor reference and was then
	 *    retyped keeps a flag nothing can clear. A local has no template and no instance, so nobody
	 *    authored it and the details panel never offered it. `LocalsAgree` masks that one bit out.
	 *  - **A friendly name that is not `NameToDisplayString`'s** (1: a bool called `bIsCleanEar`,
	 *    whose stored friendly name still has the `b`) - NOT a gap either, and the same shape. The
	 *    engine strips a leading `b` only when the variable is a bool AT THE MOMENT THE NAME IS
	 *    COMPUTED, so a retype leaves the two out of step. `LocalsAgree` does not compare it.
	 *
	 * THE LESSON THE LAST TWO SHARE, because it will come up again: **a field the engine DERIVES and
	 * then fails to recompute is not authored state, and comparing it keeps a blob over a difference
	 * that carries no information.** Both were tempting to spell in the grammar - and spelling either
	 * would have invited an edit that applies clean and changes nothing, since the engine re-derives
	 * both from the type regardless. The test to apply is not "can the text carry this field" but
	 * "did a person put it there".
	 *
	 * The blob is one property value, so this is all-or-nothing per entry node: one such local
	 * keeps the whole function's list. That is a real cost - and it is the honest one, because the
	 * alternative is a document that reads as complete while an unspellable flag quietly goes.
	 *
	 * The guids used for the rebuild are the LIVE node's own, which is what the applier will have
	 * snapshotted - so the comparison is against the list the applier would actually produce,
	 * rather than against a hypothetical one. It is not vacuous: two locals sharing a name
	 * collapse in the map, both rebuilt entries take one guid, and the blob correctly stays.
	 */
	BLUEPRINTIR_API bool DeclaredLocalsReproduce(const UEdGraphNode* Node,
		const TArray<FBpIrVariable>& Locals);

	/**
	 * The node a statement already describes: this class, this target, these properties, its
	 * own pins allocated from them. Nothing authored beyond that.
	 *
	 * This is the baseline BOTH channels measure against, and it is the same object the applier
	 * builds - reference, then properties, then `AllocateDefaultPins` (BlueprintIRBuild.cpp,
	 * "Reference and properties BEFORE AllocateDefaultPins"). So "the document already implies
	 * this" is not an argument anyone had to make per node class; it is a comparison against a
	 * node constructed the way the reader's own output constructs one.
	 *
	 * @param Properties  the property channel to apply, or null for the reference alone. The
	 *        property channel passes null because the properties are what it is deciding: a
	 *        baseline handed them would agree with every one and the channel would go silent.
	 *        The pin-shape channel passes the properties it has just read, because by then they
	 *        are part of what the statement says.
	 *
	 * @param Signature  the graph's `input`/`output` blocks, or null when the caller has none to
	 *        offer. Applied in the same position the applier applies it - after the property
	 *        channel, and not at all when that channel still states the blob - for exactly the
	 *        reason the parameter above exists: once a terminator's signature stops travelling as
	 *        a property, a baseline that was not told the blocks allocates a node with no
	 *        parameter pins, and the pin-shape channel then writes a type hint for every
	 *        parameter in the graph to restate what the block already says.
	 *
	 * Null when the node has no graph, and - for the reference-only form - when the class holds
	 * no reference at all, where the CDO already is this and building a node to say so is work
	 * for no answer.
	 *
	 * Outered to the real graph, because AllocateDefaultPins reaches GetGraph()/GetBlueprint()
	 * and a transient-package outer would null-deref. `NewObject` does not add to `Graph->Nodes`,
	 * so the graph is unchanged - the same split FGraphNodeCreator relies on.
	 *
	 * The caller releases it with `ReleaseScratchNode` once it has read it (C1, G150).
	 */
	struct FDeclaredSignature
	{
		const TArray<FBpIrVariable>& Inputs;
		const TArray<FBpIrVariable>& Outputs;
	};

	BLUEPRINTIR_API UEdGraphNode* MakeBaselineNode(const UEdGraphNode* Node,
		const TArray<TPair<FString, FString>>* Properties,
		const FDeclaredSignature* Signature = nullptr);

	/**
	 * End a `MakeBaselineNode` scratch's attachment to the real graph, once its caller has read it.
	 *
	 * C1 (G150, 17 Nov). The scratch is outered to the REAL graph so the node can find its
	 * Blueprint while it is built (see above), and nothing ended that: one node per node read stayed
	 * attached until a write's orphan sweep or a garbage collect, so every sweep carried a count
	 * that tracked how often the graph had been LOOKED at - 1008 in a 171-node graph on 0.1.8, the
	 * 275 / 60 / 1089 before the field's crashes. Renamed into the transient package, exactly as
	 * `FBpGraphBuilder::EvictFromPackage` does, and left for GC: the object and its pins stay valid,
	 * so nothing a caller still holds dangles. Not recorded (`REN_NonTransactional`, no `Modify`):
	 * it was never in the file, and an Undo that put it back would only re-leak it.
	 *
	 * Takes only a transient node that its graph does not list, so a real node handed here by
	 * mistake is refused and stays where it is. True when the node is no longer outered to a graph.
	 * Every caller of `MakeBaselineNode` in this module releases its scratch through this, and the
	 * write's sweep releases a transient orphan through it too.
	 */
	BLUEPRINTIR_API bool ReleaseScratchNode(const UEdGraphNode* Scratch);

	/**
	 * State a node derives from its own target, applied after that target is set.
	 *
	 * THE EDITOR NEVER ASSIGNS A TARGET REFERENCE DIRECTLY. It goes through the node's setter -
	 * `UK2Node_CallFunction::SetFromFunction`, `UK2Node_Variable::SetFromProperty` - and each of
	 * those sets a flag on the way past. This project assigns the reference directly, and it is
	 * right to: `ApplyTargetReference`'s rules about self, local and external scope are not the
	 * setters' rules, and calling a setter would throw them away. The cost was that nothing
	 * derived the flags, so they had to travel in the document - `SelfContextInfo=NotSelfContext`
	 * on 1184 lines and `bDefaultsToPureFunc=True` on every one of 716 promotable operators,
	 * 52 KiB of a corpus restating what the target already decides.
	 *
	 * So: keep assigning the reference, then derive exactly what the setter would have. Called
	 * from BOTH the applier and MakeBaselineNode, which is the whole point - a value only leaves
	 * the document when the baseline reproduces it, and the baseline only reproduces it if both
	 * sides run the same step.
	 *
	 * Runs BEFORE the property channel is written, so a document that states one of these
	 * explicitly still wins. Deriving afterwards would silently overwrite an authored value.
	 *
	 * This names node classes, and that is not ADR-8 drift: it is not a table of what a class
	 * MEANS, it is the engine's own derivation copied from the setter this file bypasses. A
	 * class with no such setter is untouched and keeps carrying its state in the document.
	 */
	BLUEPRINTIR_API void DeriveFromTarget(UEdGraphNode* Node);

	/**
	 * Append the graphs this node owns through a CONTAINER property, beside the ones it reports
	 * through `GetSubGraphs()`. Row 11y.
	 *
	 * A blend space node reports only its dummy `UBlendSpaceGraph` through the virtual, while the
	 * graph per blend SAMPLE lives in a `TArray<TObjectPtr<UEdGraph>>` on the node. Nothing reached
	 * those, and the property channel meanwhile exported their object paths — which name objects a
	 * write rebuilds, so the reference died and nothing else in the document mentioned the samples
	 * at all. The channel now drops such a property and the reader carries its graphs from here, as
	 * ordinary children of the owning statement.
	 *
	 * **`UEdGraph::SubGraphs` IS DELIBERATELY NOT THE SOURCE, and the difference was measured.**
	 * That list retains abandoned graphs: on a two-state machine the state nodes reported
	 * `Idle, Run` while the machine graph's `SubGraphs` still held a dead `State` and `Transition`
	 * from `PostPlacedNewNode`. A pass over it resurrects graphs nothing references. The node's own
	 * container is what the engine maintains (`AddGraph`/`RemoveGraph`).
	 *
	 * Appends rather than assigns, and `AddUnique`s, so a graph a node reports BOTH ways is carried
	 * once. In-asset graphs only: a container entry in another package is a reference, not a child.
	 */
	BLUEPRINTIR_API void AppendGraphsHeldInContainers(const UEdGraphNode* Node,
		TArray<UEdGraph*>& OutGraphs);

	/**
	 * Give a function terminator back the parent signature an OVERRIDE never stores.
	 *
	 * A Blueprint that DECLARES a function keeps `UserDefinedPins` on the terminator, and
	 * `UK2Node_EditablePinBase::AllocateDefaultPins` builds the pins from it. A Blueprint that
	 * OVERRIDES one keeps neither that nor anything else the IR can see: the engine sets
	 * `FunctionReference` to the parent's function and lets allocation resolve it, and the
	 * projection cannot carry that reference because the reader renders no `Target` for a
	 * terminator and `MakeBaselineNode` copies the reference onto the baseline before the
	 * property channel compares against it. So a rebuilt entry node had `then` and nothing else,
	 * and every `n0.<param>` in the payload failed against it.
	 *
	 * DERIVED RATHER THAN CARRIED, deliberately. The whole value is recoverable - member name
	 * from the graph's name, parent from the superclass declaring a function of that name, GUID
	 * from `GetFunctionGuidFromClassByFieldName` - so carrying it would put a
	 * `FunctionReference=(…)` on the entry statement of EVERY function graph in the corpus to
	 * fix the fraction that are overrides.
	 *
	 * NOT part of DeriveFromTarget, and the ordering is why. This must run AFTER
	 * `PostPasteNode`, which resets every terminator's reference to name the graph and no class,
	 * and which promotes any non-exec pin it finds into `UserDefinedPins` - so a signature
	 * restored any earlier is both wiped and copied into the child as its own declaration.
	 * MakeBaselineNode does not need it either: it copies the real node's reference, which for a
	 * genuine override already names the parent.
	 *
	 * `InFunctionName` is the name the PAYLOAD gives the graph. Pass it whenever the caller has it:
	 * the node may be sitting in a preflight scratch graph created with `NAME_None`, and reading
	 * the name off that finds no inherited function at all. Omit it and the graph's own name is
	 * used, which is right for a node already in its final home.
	 *
	 * Inherited means the SUPERCLASS or an IMPLEMENTED INTERFACE. Both declare the signature
	 * elsewhere and leave this Blueprint only the body.
	 */
	BLUEPRINTIR_API void RestoreInheritedSignature(UEdGraphNode* Node,
		const FName InFunctionName = NAME_None);

	/**
	 * `ExtraFlags` as words - `BlueprintCallable|BlueprintEvent|Public` instead of `201457664`.
	 *
	 * **THE SECOND HAND-WRITTEN ENTRY IN THIS FILE, and unlike `UserDefinedPins` it is keyed on a
	 * PROPERTY NAME rather than a type, so it is the weaker of the two and says so.** There is no
	 * type-level question available: `UK2Node_FunctionEntry::ExtraFlags` is a plain `int32` and the
	 * `EFunctionFlags` it holds is a C++ `enum class` with no `UENUM()`, so nothing in reflection
	 * connects the field to the enum. That missing link is the whole reason the number was
	 * unreadable - `gc_describe type_path:"/Script/CoreUObject.EFunctionFlags"` refuses, correctly,
	 * because the type does not exist to the reflection system (`GC-DISCOVERY-DESIGN.md` §4b item 8).
	 *
	 * **User-ruled 2026-08-24: spell the flags as words.** The measured case for it: 465 lines across
	 * 107 sidecars carry this key and they hold SIX distinct values between them, built from six bits.
	 * The rejected alternative was teaching a tool to decode the number, which leaves the file itself
	 * unreadable and puts the meaning one call away from every reader forever.
	 *
	 * **The table is engine constants, never literals.** A future engine that moves a bit's VALUE
	 * cannot drift this silently, and one that RENAMES a flag breaks the build instead - which is the
	 * failure mode a hand-written table of numbers does not have.
	 *
	 * Unset is not a failure. It is the same round-trip proof `CompactPinTypeToken` applies to itself:
	 * the token is parsed back and compared, so a bit no name covers - either of `Script.h`'s two
	 * unused ones, or a flag added after this was written - declines and the caller writes the plain
	 * number exactly as before.
	 */
	BLUEPRINTIR_API TOptional<FString> FunctionFlagsToken(int32 Flags);

	/**
	 * The inverse. False for a plain number, which is what every document written before
	 * 2026-08-24 contains - so which spelling a file uses never has to be asked, and both stay
	 * readable for good.
	 */
	BLUEPRINTIR_API bool ParseFunctionFlagsToken(const FString& Token, int32& OutFlags);

	/**
	 * Is this the property `FunctionFlagsToken` spells? Asked at both ends so the two directions
	 * cannot disagree about which field is a flags field.
	 */
	BLUEPRINTIR_API bool IsFunctionFlagsProperty(const FProperty* Property);

	/**
	 * **THE PORT FLAGS THIS CHANNEL SPELLS ONE TOP-LEVEL PROPERTY WITH, and the only type that
	 * needs a choice is a string.**
	 *
	 * `[Key=Value]` is a comma-separated list read by a quote-aware splitter, and `PPF_None` asks
	 * `FStrProperty` for its RAW characters: a value holding a comma ends its field early, one
	 * holding a quote swallows the rest of the list, and one holding a bracket closes the list.
	 * `PPF_Delimited` is the engine's own answer — quoted, with `\"` escapes the splitter already
	 * understands — and it is what the array-element half of this channel has always passed, for
	 * exactly this reason. The top-level walk was the half that did not.
	 *
	 * **FNAME IS DELIBERATELY NOT HERE.** It has the same hole and a different price: 43 corpus
	 * values would change spelling, which is a projector bump and a regeneration sweep rather than
	 * a codec edit. That is its own row.
	 *
	 * Asked at both ends, like `IsFunctionFlagsProperty`, so the two directions cannot disagree —
	 * except that the READ side must also accept the older bare spelling, which is what
	 * `ChannelValueIsDelimited` is for.
	 */
	BLUEPRINTIR_API uint32 ChannelPortFlags(const FProperty* Property);

	/**
	 * Was this text written by the delimited speller above?
	 *
	 * Decided from the TEXT rather than from the property, so a document written before this
	 * spelling existed still reads whole — the same both-spellings-stay-readable rule the
	 * function-flag and compact pin-type tokens follow.
	 */
	BLUEPRINTIR_API bool ChannelValueIsDelimited(const FProperty* Property, const FString& Value);

	/**
	 * Read a node's off-pin state into the property channel.
	 *
	 * @param bTargetCarriesReference  true when the statement's `Target` is non-empty, i.e.
	 *        the reader already rendered this node's member/graph reference. Passed rather
	 *        than inferred so the skip is a question about the STATEMENT ("do we already
	 *        say this?") instead of a question about the node class.
	 * @param OutNotes  per-item boundary notes, appended. These are the loud half - a state
	 *        the IR cannot carry must say so by name, never read clean.
	 */
	BLUEPRINTIR_API void Read(const UEdGraphNode* Node, bool bTargetCarriesReference,
		TArray<TPair<FString, FString>>& OutProperties, TArray<FString>& OutNotes);

	/**
	 * Put every property this channel can speak about back to the class default, so that a node
	 * being REUSED starts where a freshly constructed one starts.
	 *
	 * THIS IS THE READER'S OMISSION CONTRACT, ENFORCED AT THE OTHER END. `Read` emits a property
	 * only when it differs from `MakeBaselineNode`, and that baseline is a `NewObject` of the same
	 * class carrying nothing but the statement's own reference. So an absent key does not mean
	 * "leave it alone" - it means "the constructed value", and `Write` cannot supply that because
	 * it only ever visits keys the payload names. Replace got this for free, because every node
	 * there is new. Differential did not, and the difference was silent: whatever the previous
	 * edit left on the node survived a document that had stopped saying it.
	 *
	 * MEASURED, on `BP_ProceduralBlock::CutHole` (board row T-9). A `Multiply_VectorVector` had
	 * been given a third input by hand, so it carried `NumAdditionalInputs=1`; the payload spelled
	 * two arguments and named no arity. The reused node kept the 1, `Settle`'s `ReconstructNode`
	 * rebuilt the `C` pin from it, the link the write had broken did not come back, and an
	 * unconnected pin defaulting to `(0,0,0)` scaled the result - so the extrude direction went to
	 * zero for every surface type, not just the one being edited. The compile succeeded.
	 *
	 * Exactly `ChannelKeys`, which is exactly `Read`'s filter: a structural link, an owned
	 * sub-object or a graph container is not this channel's to reset, because the reader could
	 * never have named it and so an omission says nothing about it. `UserDefinedPins` is not a
	 * UPROPERTY and is not here either - `ApplyDeclaredSignature` already rewrites it whole from
	 * the graph's own `input`/`output` blocks, on the same omission-resets ruling (ADR-7).
	 *
	 * CALL IT BEFORE THE REFERENCE AND BEFORE `DeriveFromTarget`, so the derived defaults land on
	 * top of the reset and the payload lands on top of those - the order a fresh node gets by
	 * construction.
	 *
	 * A MEMBER OR GRAPH REFERENCE IS NEVER RESET, whatever the statement says. It is the one
	 * field with two carriers, and the reader's own baseline is why it has to be left alone: the
	 * baseline COPIES the live reference before comparing (`MakeBaselineNode`), so a node whose
	 * target the reader failed to render has its reference elided from the channel as well - it
	 * is in neither half of the document, and a reset here would be the only thing that ever
	 * removed it. `ApplyTargetReference` and `Write` both overwrite it afterwards, so nothing is
	 * lost by not clearing it first.
	 *
	 * @return how many properties were actually put back, for a caller that wants to say so.
	 */
	BLUEPRINTIR_API int32 ResetChannelToConstructed(UEdGraphNode* Node);

	/**
	 * Write the property channel back onto a freshly constructed node.
	 *
	 * Every failure is reported and none is swallowed: a property name the class does not
	 * have, a name that is not part of the channel, a value the engine will not parse. A
	 * write helper that skips what it cannot do reports success for work it did not do -
	 * BRIDGE-EXPANSION-QUEUE.md §5f, and this is not becoming another instance.
	 *
	 * @param OutDeferred  where an element-wise array overlay is put aside for `WriteDeferred`.
	 *        NOT optional in practice: pass null and an overlay becomes a reported ERROR, not a
	 *        skip, because this function runs before the node has the array such a value is a
	 *        delta against and a caller with nowhere to defer it to cannot apply it later.
	 *
	 * @return true when every property applied. An entry routed to OutDeferred has not applied
	 *         yet, so a `true` here is only half the answer for those - see `WriteDeferred`.
	 */
	BLUEPRINTIR_API bool Write(UEdGraphNode* Node, const TArray<TPair<FString, FString>>& Properties,
		TArray<FString>& OutErrors, TArray<TPair<FString, FString>>* OutDeferred = nullptr);

	/**
	 * The second half of `Write`: an array delta, applied once the node has the array it is a
	 * delta against.
	 *
	 * `Write` clears before importing, and the long comment there is why: the reader elides
	 * against zero, so zero is the only valid baseline. An element-wise payload breaks that
	 * assumption by construction - it is a delta against what the node DERIVES, and for
	 * `ShowPinForProperties` the derivation is `FOptionalPinManager::RebuildPropertyList`
	 * running inside `AllocateDefaultPins`, which is AFTER `Write`. So the overlay waits.
	 *
	 * Overlay, not replace: each named index is imported onto the live element without clearing
	 * it, so every member the document did not name keeps the value the node just derived. That
	 * is the whole saving - a `bShowPin` the author toggled travels alone instead of dragging
	 * the struct's friendly names, categories and C++ doc comments with it.
	 *
	 * Ends with `ReconstructNode()` when anything applied, because an array like this one
	 * DECIDES the node's pins and the pins were built from the pre-overlay value. The engine's
	 * own rebuild carries the overlay forward rather than undoing it: `RebuildProperty` reads
	 * the previous entry's visibility out of `OldSettings` and reapplies it (K2Node.cpp:1808),
	 * which is the same mechanism that makes a designer's toggle survive a reconstruct.
	 *
	 * Call it in the same position on both sides - after `AllocateDefaultPins` in the applier,
	 * after `AllocateDefaultPins` in `MakeBaselineNode`. That symmetry is not tidiness: a value
	 * only leaves the document when the baseline reproduces it, and the baseline only
	 * reproduces it if both sides run the same step in the same place.
	 *
	 * @return true when every overlay applied.
	 */
	BLUEPRINTIR_API bool WriteDeferred(UEdGraphNode* Node,
		const TArray<TPair<FString, FString>>& Deferred, TArray<FString>& OutErrors);

	/**
	 * Re-assert every user-defined pin's own stored default onto its live UEdGraphPin.
	 *
	 * `AllocateDefaultPins()` builds a pin FROM `UserDefinedPins`, but at least one node
	 * class turns straight back around and overwrites what it just built:
	 * `UK2Node_FunctionResult::AllocateDefaultPins` ends by calling
	 * `FFillDefaultPinValueHelper::FillAll`, which fills ANY currently-blank, unconnected
	 * value pin with a type-based default - `0` for an int, `false` for a bool - with no
	 * way to tell "blank because the author never gave it one" from "blank because this
	 * build just created the pin and hasn't set anything yet". A payload round-tripped
	 * through `bp_read` carries the first kind exclusively (see `RenderPinDefault`: a pin
	 * this channel emits with no explicit value was genuinely blank when read), so the
	 * live pin has to be put back in agreement with `UserDefinedPins` - this channel's one
	 * source of truth for these pins (see the file header) - or a Replace apply invents a
	 * literal the author never wrote.
	 *
	 * Call this once, after the node's LAST `ReconstructNode()` in a build pass. A pin
	 * whose value already agrees with its own `PinInfo` is untouched by construction, and
	 * an explicit statement literal for the same pin - applied by a later `BindLiterals`
	 * pass - overwrites whatever this leaves, so ordering the two the other way round
	 * would be wrong: this undoes engine-side invention, nothing it runs after should be
	 * allowed to undo the author's own value.
	 */
	BLUEPRINTIR_API void ReassertUserDefinedPinDefaults(UEdGraphNode* Node);

	/**
	 * The OTHER direction: put `UserDefinedPins` back in agreement with the live pins, after the
	 * applier has finished writing this node's literals.
	 *
	 * `SetPinLiteral` writes `Pin->DefaultValue` directly and deliberately - going through the
	 * schema's `TrySetDefaultValue` would normalise the author's spelling, which a write path
	 * whose output must match its input cannot afford. The cost of bypassing the schema is that
	 * `UK2Node_EditablePinBase::PinDefaultValueChanged` never fires, and that is the engine hook
	 * which would otherwise have called `ModifyUserDefinedPinDefaultValue` and kept the two in
	 * step (`K2Node_EditablePinBase.cpp:403-419`). So the applier left `FUserPinInfo` stale on
	 * every user-defined pin it wrote a literal onto.
	 *
	 * This calls the engine's own `UpdateUserDefinedPinDefaultValues()`, which is the same
	 * function the engine itself runs at save time, from `FixupPinStringDataReferences` under
	 * `if (SavingArchive)` - verified in all four overrides (`K2Node_FunctionResult.cpp:207`,
	 * `K2Node_FunctionEntry.cpp:1091`, `K2Node_Tunnel.cpp:232`, `K2Node_CustomEvent.cpp:423`),
	 * and NOT on the load path, which passes `nullptr` (`K2Node.cpp:131` vs `:145`).
	 *
	 * That is what makes this safe rather than a new policy: **the value on disk is unchanged
	 * either way.** The struct was always going to converge to the live pin at save; this only
	 * moves the convergence earlier, to the end of the write that caused the divergence. What it
	 * buys is that there is no longer a window in which a resident Blueprint has an authored pin
	 * and a stale `PinInfo` - which is the whole reason the repair below used to need a caller
	 * that could prove the Blueprint had just come off disk.
	 *
	 * Call this once, AFTER the final `BindLiterals` pass. It uses `DoesDefaultValueMatch` and is
	 * a no-op where the two already agree, and it reads through `UEdGraphPin::GetDefaultAsString`,
	 * so an object, text or soft-reference default converts the way the engine converts it rather
	 * than the way this build would guess.
	 */
	BLUEPRINTIR_API void ConvergeUserDefinedPinInfo(UEdGraphNode* Node);

	/**
	 * The same repair as `ReassertUserDefinedPinDefaults`, applied to every graph of a Blueprint.
	 * See BLUEPRINT-IR-STATUS.md §11 for the measurement this exists to answer.
	 *
	 * The engine re-invents the literal on the way back in. A Blueprint loading with
	 * `bIsRegeneratingOnLoad` goes through `FBlueprintEditorUtils::ReconstructAllNodes`, which
	 * re-runs `AllocateDefaultPins` and therefore `FillAll`, and nothing on that path re-asserts
	 * `UserDefinedPins` because it is the engine's path and not this build's. Measured, not
	 * inferred: a function's blank int return saved blank reads back as `Doubled=0`, and the
	 * graph signature moves with it, so the first edit after an editor restart is refused as
	 * stale on a graph nobody touched.
	 *
	 * What makes the repair safe here is that a freshly-deserialised `UserDefinedPins` is
	 * authoritative, and that is also measured: it comes back byte-identical, because
	 * `FixupPinStringDataReferences` calls `UpdateUserDefinedPinDefaultValues` at save time and
	 * so *converges* the struct with the live pin rather than diverging from it. Nothing is
	 * lost in the package; only the live pin drifts.
	 *
	 * UNCONDITIONAL, and that is a change: this used to run only when the calling `bp_read` /
	 * `bp_apply` was itself what brought the Blueprint off disk, guarded by a `FindObject`
	 * before the `LoadObject`. The guard existed because the applier left `FUserPinInfo` stale
	 * on every pin it wrote a literal onto, so on a resident Blueprint the live pin outranked
	 * the struct and re-asserting would have destroyed authored values.
	 *
	 * `ConvergeUserDefinedPinInfo` above removes that producer - the applier now leaves the two
	 * in agreement - so the only remaining source of divergence is the engine's own invention on
	 * load, which is exactly what this repairs. The guard is therefore gone, and with it the
	 * gap it created: a Blueprint the editor already had open was resident, so the repair never
	 * ran on it and `bp_read` reported the invented literal on the one configuration a human is
	 * actually working in. No load hook was needed to close it.
	 *
	 * The precedence that remains is between the two things this build writes, and it is
	 * unchanged: within a build pass `ReassertUserDefinedPinDefaults` runs BEFORE the final
	 * `BindLiterals`, so an explicit statement literal still outranks the re-asserted struct.
	 */
	BLUEPRINTIR_API void RepairUserDefinedPinDefaultsOnLoad(UBlueprint* Blueprint);
}
