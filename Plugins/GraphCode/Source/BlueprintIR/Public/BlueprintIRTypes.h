// Blueprint IR - core data model.
//
// This header defines the intermediate representation itself. It is deliberately
// engine-flavoured but bridge-agnostic: nothing here knows what MCP is.
//
// ---------------------------------------------------------------------------
// IR SHAPE (ADR-1): exec-labelled, with code-like sugar for the linear case.
// ---------------------------------------------------------------------------
// Pure "code-like" output reads beautifully for linear and branch flow and then
// falls apart on Sequence, multi-output latent nodes, and multi-exec macros.
// Exec-labelled is uglier but total. We emit exec labels always, and let the
// linear case degrade into something that still reads like code.
//
// Serialised text form, one statement per line. This is EXACTLY what StatementToText
// emits and what GraphFromText parses - the two are a matched pair and the grammar below
// is the contract between them. Do not "tidy" an example here without changing both.
//
//   eventgraph EventGraph
//     c1 = comment("Spawn placement") [covers=(n2,n3), color="0.200,0.600,1.000,1.000"]
//     n1 = Event BeginPlay() : then->n2
//     // Drop the actor onto the floor plane before anything reads its transform
//     n2 = CallFunction /Script/Engine.Actor:K2_SetActorLocation(NewLocation=(X=0,Y=0,Z=100), bSweep=false) : then->n3
//     n3 = IfThenElse(Condition=CallFunction /Script/Engine.KismetMathLibrary:Greater_FloatFloat(A=n2.ReturnValue, B=0.0)) : then->n4, else->_
//     n4 = ExecutionSequence() : then_0->n6, then_1->n7
//     n6 = CallFunction /Script/Engine.KismetSystemLibrary:PrintString(InString="probe") [enabled=false] : then->_
//     n7 = MakeStruct() : then->_
//
// Numbers start at 1 and have GAPS - `n5` is missing above, and that is what a minted
// identity looks like after something was deleted. A gap is never reused.
//
// Reading rules:
//   * line 1        `<graphkind> <name>`, plus `  owner:<alias>` when a statement owns it.
//                   NO `sig:` since 2026-08-21 - the document digest a caller sends as
//                   base_sig moves whenever any graph in the asset moves, so a per-graph
//                   one could only agree with it. Still ACCEPTED on input and ignored, so
//                   every .gc written before then still reads. Optional on input anyway:
//                   a graft fragment is bare statements and the envelope carries base_sig
//   * `nK =`        statement alias. A MINTED identity stored in the node, not a position:
//                   delete n2 and the file reads n1, n3, n4 (ADR-3 below, AuthoringAlias.h)
//   * `op`          the NODE CLASS with `K2Node_` stripped: CallFunction, IfThenElse,
//                   ExecutionSequence, MakeStruct, SpawnActorFromClass (ADR-8). The set is
//                   OPEN - any class name is legal, including one from a plugin this build
//                   has never seen - which is what makes coverage total. The old closed
//                   vocabulary (`call`, `branch`, `seq`, ...) is still ACCEPTED on input and
//                   re-emitted unchanged, so older IR keeps round-tripping; it is never
//                   emitted for a node read from a graph. Legacy tokens are recognised only
//                   in lowercase, because `Event` the class and `event` the legacy token are
//                   different things
//   * `(...)`       input pin bindings, `PinName=Value`. ALWAYS `=`, never `:`
//   * `[n=v, ...]`  off-pin node state, between the pins and the exec edges. Omitted
//                   entirely when there is none, which is the common case. `enabled` is
//                   the modelled member (`false` / `compiled`); everything else is an
//                   engine property name. See FBpIrStatement::Properties
//   * `: label->nK` exec output edges, one per exec-out pin, in pin order.
//                   `->_` is an exec pin that exists and goes nowhere
//   * `  local X: float = 0.0`      graph-local variable declaration
//   * `  !orphaned[A, B]`           orphaned pins, surfaced not dropped
//   * `  !types[>Out="(...)"]`      exact settled types for graph-derived data pins
//   * `  # UNSUPPORTED: text`       a node the reader could not model
//
// Comments come in two forms, and the difference is whether the thing is addressable:
//   * `  cK = comment("text") [covers=(nA,nB), color="r,g,b,a"]`
//                   a COMMENT BOX: a node in its own right, emitted before the
//                   statements, `color` omitted when white, `covers` when empty.
//                   Edited by handle through blueprint_modify update_comment
//   * `  // text`   a NODE comment, on the line(s) ABOVE the statement it belongs to,
//                   exactly as in source. One `//` line per line of comment. Not
//                   addressable - it is a property of its node, written with
//                   blueprint_modify set_node_comment
//   * ONE NODE, ONE STATEMENT. Every node that is a statement at all gets an alias and a
//     line, pure or not, however few its consumers. Nesting a node inside a pin value is a
//     parse ERROR, in both notations: a nested node has no alias, and the alias is what an
//     apply addresses (ADR-7 Rule 1). Data flows by pin reference, `nK.PinName`, only
//   * pure nodes with more than one consumer get their own statement, keeping their own
//     kind - a hoisted variable read is `get`, a hoisted call is `call` - because
//     duplicating them would not survive the round-trip invariant
//
// Value forms, exactly as ValueToText renders them:
//   scalar     100, 0.5, true            enum   /Game/E_C.E_C::Bedroom
//   string     "hi \"there\""            struct (X=1.0,Y=2.0)
//   text       text("Hi", ns="UI", key="K")
//   object     obj'/Game/A.A'            soft   soft'/Game/A.A'
//   null       null                      pinref n3.ReturnValue
//
// The "code-like sugar" ADR-1 mentions is NOT implemented: there is no `gt(...)` or
// infix spelling, and inlined calls use the same `call <canonical>` form as statements.
//
// ---------------------------------------------------------------------------
// EDGE DIRECTION: exec flows FORWARD, data flows BACKWARD. Deliberate, not an
// inconsistency.
// ---------------------------------------------------------------------------
// Every edge is declared exactly once, at whichever end makes it unambiguous.
//
//   exec  declared at the SOURCE:       n2 = … : then->n3
//   data  declared at the CONSUMER:     n3 = set Health(Health=n2.Health)
//
// The asymmetry follows from the pins. An exec edge leaves ONE named out-pin and
// the only open question is where it goes, so the source end carries it and the
// line reads in execution order, top to bottom. A data edge arrives at ONE named
// in-pin, and the open question is which of the producer's possibly-many outputs
// feeds it - so the consumer end carries it and names both halves at one site:
// its own pin on the left of the `=`, the producer's pin on the right.
//
// Declaring data forward instead would still have to name both pins (`n2.Health ->
// n3.Health`), so it buys nothing and costs two things: the line already carrying
// exec flow would carry data flow too, and INLINING becomes impossible. Inlining a
// pure single-consumer node into its consumer's expression is where most of the
// compaction comes from, and it only works because the consumer is the one doing
// the referencing.
//
// Consequence worth stating: an exec out-pin that goes nowhere is written `->_`
// explicitly, because flow stopping there is meaningful. An output data pin that
// nothing reads is simply never mentioned, because nobody reading it is not a
// fact about the graph. Same reason, opposite conclusion.
//
// THIRD CASE: delegate and event bindings are BACKWARD, like data - not forward.
//
// This is the one that is easy to get wrong, because an event *looks* like a source:
// execution starts at it, so instinct says the event should declare who it calls.
// It must not. In a Blueprint graph a dispatcher does not know its listeners. The
// binding lives entirely at the BIND SITE, which names both the delegate it is
// binding to and the handler it is binding - a Delegate input pin fed by a
// CreateDelegate output. Nothing is recorded on the event or the dispatcher.
//
//   n5 = AddDelegate OnDoorOpened(Delegate=n6.OutputDelegate) : then->n7
//   n6 = CreateDelegate(SelectedFunction=HandleDoorOpened)
//   n9 = CustomEvent HandleDoorOpened() : then->n10     <- says nothing about n5
//
// So an event statement carries no listener list, and the IR must never synthesise
// one. Emitting "who binds to this" on the event would be inventing a forward edge
// the graph does not contain, and it would have two declaration sites that can
// disagree - the exact failure the one-edge-one-site rule above exists to prevent.
//
// Summary: exec forward, data backward, bindings backward. Only exec is forward,
// because only exec has a source that genuinely owns the edge.
//
// ---------------------------------------------------------------------------
// CANONICAL NAMES (ADR-2): full object paths. Unique, stable, already what the
// engine speaks. Never display-only strings, except where the engine itself
// mangles the internal name (BP-authored enums/structs - see BlueprintIRNames.h).
// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------
// NODE IDENTITY (ADR-3, SUPERSEDED 2026-07-30): aliases are MINTED, and stored.
// ---------------------------------------------------------------------------
// ADR-3 used to read: aliases are STRUCTURE-derived, never GUID-derived, because
// apply() mints fresh GUIDs - so a GUID-derived numbering would make
// read(apply(read(G))) produce a different numbering for a semantically identical
// graph and the round-trip gate could never hold.
//
// That was true of an applier which randomises identity. Ours no longer does, and
// the cost of positional names turned out to be the one that mattered: DELETING n2
// RENAMED n3 TO n2, n4 TO n3, and every reference to them anywhere in the graph. One
// node removed from a 200-node graph rewrote most of 200 lines - and these documents
// are committed to git, where that destroys both the diff and `git blame`.
//
// So a number is now minted ONCE and written into the node's own NodeGuid, whose four
// parts carry the owning asset, the number, the node's kind, and a checksum over the
// three. Delete n2 and the file reads `n1, n3, n4`. The full layout, what invalidates
// an identity, and why there is no "version" part are in AuthoringAlias.h.
//
// What did NOT change: statement ORDER is still structural - entry points sorted by a
// stable key, then depth-first along data inputs and exec outputs. Order follows the
// program so the file reads like the program; only the naming left the traversal. The
// two are separate jobs and used to be one function.
//
// The round trip holds by construction rather than by renumbering: Differential reuses
// a matched node in place, and BlueprintIR::StampGraphIdentities writes down whatever
// the reader would have called each node. A read never writes, so a node authored in
// the editor carries a PROVISIONAL number - correct, reproducible, and positional -
// until some write stamps it.

#pragma once

#include "AuthoringCoreTypes.h"
#include "CoreMinimal.h"

class FJsonObject;

/** How a value bound to an input pin is expressed. */
enum class EBpIrValueKind : uint8
{
	/** No connection and no meaningful default - pin is omitted from output entirely. */
	None = 0,

	/** A literal default: number, bool, string, enum entry, struct literal, asset path. */
	Literal,

	/**
	 * Output pin of another aliased statement, e.g. `n3.ReturnValue`.
	 *
	 * The only way to express a data link. There used to be an `Inline` kind that nested a
	 * pure node inside its consumer's expression; it was removed with pure-node inlining
	 * (ADR-8's corollary) because a nested node has no alias, and under ADR-7 Rule 1 the
	 * alias is the node's identity — so an apply could neither address it nor preserve it.
	 */
	PinRef,
};

/** What kind of literal we are looking at. Drives quoting and round-trip parsing. */
enum class EBpIrLiteralKind : uint8
{
	/** Bare token: numbers, true/false. */
	Scalar = 0,
	/** Quoted string. */
	String,
	/** FText - carries namespace/key/source and must not be flattened to a string. */
	Text,
	/** Enum entry, rendered as `EnumPath::DisplayEntryName`. */
	EnumEntry,
	/** Struct literal, rendered as `(A=1,B=2)`. Engine's own export text form. */
	Struct,
	/** Hard object reference - full asset path. */
	ObjectRef,
	/** Soft object/class reference - full asset path, distinguished from hard. */
	SoftRef,
	/** Explicit null for object-typed pins. */
	Null,
};

/**
 * A value bound to an input pin.
 *
 * Deliberately a plain struct with a Kind tag rather than a variant hierarchy:
 * this gets serialised to both line-oriented text and JSON, and a flat shape
 * keeps both serialisers honest about handling every case.
 */
struct BLUEPRINTIR_API FBpIrValue
{
	EBpIrValueKind Kind = EBpIrValueKind::None;

	// --- Kind == Literal ---
	EBpIrLiteralKind LiteralKind = EBpIrLiteralKind::Scalar;
	/** Rendered literal text. For Struct this is the engine's export-text form. */
	FString Literal;
	/** FText only: namespace/key/source, so localisation identity survives round-trip. */
	FString TextNamespace;
	FString TextKey;

	// --- Kind == PinRef ---
	/** Alias of the producing statement, e.g. "n3". */
	FString RefAlias;
	/** Exact pin name on that statement, e.g. "ReturnValue". Never a display label. */
	FString RefPin;

	static FBpIrValue MakeLiteral(EBpIrLiteralKind InKind, const FString& InText)
	{
		FBpIrValue V;
		V.Kind = EBpIrValueKind::Literal;
		V.LiteralKind = InKind;
		V.Literal = InText;
		return V;
	}

	static FBpIrValue MakePinRef(const FString& InAlias, const FString& InPin)
	{
		FBpIrValue V;
		V.Kind = EBpIrValueKind::PinRef;
		V.RefAlias = InAlias;
		V.RefPin = InPin;
		return V;
	}

	bool IsSet() const { return Kind != EBpIrValueKind::None; }
};

/** One `PinName = Value` binding. */
struct BLUEPRINTIR_API FBpIrPinBinding
{
	/** Exact pin name from UEdGraphPin::PinName. Never the display label. */
	FString PinName;
	FBpIrValue Value;
};

/**
 * Exact settled type for a pin whose type is graph-derived rather than declaration-derived.
 *
 * `Type` is FEdGraphPinType's own ExportText representation. Keeping the engine-shaped
 * value here makes this channel open to new categories, containers and terminal types
 * without teaching BlueprintIR another type system. Hints are emitted only for connected
 * data pins and split roots: the places where a later connection/default names a shape that
 * a freshly allocated wildcard node cannot recover from its class alone.
 */
struct BLUEPRINTIR_API FBpIrPinTypeHint
{
	FString PinName;
	bool bOutput = false;
	FString Type;
};

/** An exec output edge: `label->targetAlias`, or `label->targetAlias.TargetPin`. */
struct BLUEPRINTIR_API FBpIrExecEdge
{
	/**
	 * Exec-out pin name. "then" for the common single case, "then"/"else" for a
	 * branch, "then_0".."then_N" for Sequence, latent pins keep their real names
	 * (e.g. "Completed"). Always the engine pin name, never invented.
	 */
	FString Label;

	/** Alias of the target statement, or empty if the exec pin is unconnected. */
	FString TargetAlias;

	/**
	 * Which exec INPUT pin of the target the wire lands on. Empty means "the target has
	 * only one, so there is nothing to say".
	 *
	 * Omitted for the overwhelming majority of nodes, which have exactly one exec input
	 * named `execute`. It is not optional for the nodes that have several, and until
	 * 2026-07-25 it did not exist at all — so a Timeline wired to `Play` read identically
	 * to the same Timeline wired to `Stop`, and a round trip was free to pick either.
	 * That is the same class of silent semantic loss as the disabled-node flag: it changes
	 * what the graph DOES and it read clean.
	 *
	 * Symmetry note: a data edge already names both ends (`Health=n2.Health`), because the
	 * consumer declares it. An exec edge is declared at the SOURCE, so the source end is
	 * the only place the landing pin can be written down.
	 */
	FString TargetPin;
};

/** Statement kind. Kept coarse - the fine detail lives in Target. */
enum class EBpIrStatementKind : uint8
{
	/** Event entry (BeginPlay, custom event, interface event). */
	Event,
	/** Function graph entry node. */
	FunctionEntry,
	/** Function graph return node. A graph may legitimately have several. */
	Return,
	/** Impure function call. */
	Call,
	/**
	 * A variable read (UK2Node_VariableGet) standing as its own statement, because it
	 * has more than one consumer and duplicating it would not survive the round trip.
	 *
	 * Emitted and parsed as `get`, matching the inline spelling of the same node. The
	 * enumerator keeps the name `Let` only to avoid a rename across both serialisers;
	 * `let` is still ACCEPTED on input as a legacy alias. Other pure node kinds hoisted
	 * for the same reason keep their own kind (a hoisted call is still `call`).
	 */
	Let,
	/** Branch / if. */
	Branch,
	/** Sequence. */
	Sequence,
	/** Macro instance (ForEachLoop, etc). */
	Macro,
	/** Cast, including the failure exec pin. */
	Cast,
	/** Variable set. */
	SetVar,
	/** Timeline node. */
	Timeline,
	/** Switch on enum/int/string. */
	Switch,
	/** Delegate bind / unbind / call. */
	Delegate,
	/** Anything we can read but do not yet model precisely. Never silently dropped. */
	Opaque,
};

/**
 * One IR statement.
 *
 * Corresponds to exactly one UEdGraphNode. No exceptions remain: pure-node inlining and
 * knot traversal were both removed, for the same reason - a node with no alias cannot be
 * addressed by an apply, so a whole-graph rewrite silently deletes it. A comment BOX is
 * not a statement, but that is because it is not a node in any sense that compiles; it
 * has its own alias and its own line.
 */
struct BLUEPRINTIR_API FBpIrStatement
{
	/** Minted stable alias, e.g. "n7" - the node's own identity, not its position. See ADR-3. */
	FString Alias;

	/**
	 * The node's class name with the `K2Node_` prefix stripped: `CallFunction`,
	 * `IfThenElse`, `MakeStruct`, `SpawnActorFromClass`. **This is the op token, and it is
	 * the statement's identity** (ADR-8, Docs/BP-AUTHORING-DESIGN.md §8.7).
	 *
	 * Mechanically derived from `Node->GetClass()->GetName()`, never chosen, so coverage is
	 * total by construction: a node class from a plugin written after this file has a
	 * representation without anyone adding a case for it. It is also the same name the
	 * engine's own clipboard writes — `ExportNodesToText` emits
	 * `Begin Object Class=/Script/BlueprintGraph.K2Node_IfThenElse` — whereas the editor
	 * label ("Branch") is localised FText, non-unique, and sometimes embeds its own
	 * argument ("Cast To BP_Door"). The class is identity; the title is presentation.
	 *
	 * EMPTY means "no node class recorded", which happens for statements built by hand
	 * rather than read from a graph. Both serialisers then fall back to Kind's legacy
	 * lowercase token, so older IR and hand-written fragments keep round-tripping
	 * byte-identically. The reader always fills this in.
	 */
	FString Op;

	/**
	 * Coarse shape hint, NOT the notation — see Op above.
	 *
	 * Under ADR-8 this stopped being the vocabulary and became an internal convenience for
	 * the few places that must branch on shape (exec-pin naming, pure-node inlining,
	 * ExpectedEntityKindFor). `Opaque` therefore no longer means "cannot be represented";
	 * it means "no special shape handling", and such a statement still carries a real Op,
	 * real args and real pins.
	 */
	EBpIrStatementKind Kind = EBpIrStatementKind::Opaque;

	/**
	 * Canonical identity of what this statement invokes.
	 *   Call    -> "/Script/Engine.Actor:K2_SetActorLocation"
	 *   Macro   -> "/Engine/EditorBlueprintResources/StandardMacros.StandardMacros:ForEachLoop"
	 *   SetVar  -> variable name
	 *   Event   -> event name
	 * Empty for kinds where it carries no meaning (Branch, Sequence).
	 */
	FString Target;

	/** Input pin bindings, in declared pin order. */
	TArray<FBpIrPinBinding> Args;

	/** Exec output edges, in declared pin order. */
	TArray<FBpIrExecEdge> ExecOut;

	/**
	 * Output pin names this statement exposes, in declared order, so a reader can
	 * validate `nK.PinName` references without re-resolving the function.
	 */
	TArray<FString> OutPins;

	/** Settled types needed to recreate wildcard-derived output and split-pin shape. */
	TArray<FBpIrPinTypeHint> PinTypeHints;

	/**
	 * Orphaned pins, surfaced explicitly. These appear when a function signature
	 * changes underneath a call site. Never silently dropped - a dropped orphan is
	 * a silent semantic change, which is exactly the failure mode this project exists
	 * to eliminate.
	 */
	TArray<FString> OrphanedPins;

	/**
	 * Node state that is NOT carried by any pin.
	 *
	 * The first entry in what ADR-8's second half calls the property channel. Names are
	 * engine names, not a curated vocabulary, so this can grow without a per-node-class
	 * table (see Docs/BP-AUTHORING-DESIGN.md §8.7).
	 *
	 * Rendered after the pins as `[name=value, …]`, omitted entirely when empty so the
	 * common case costs nothing.
	 */
	TArray<TPair<FString, FString>> Properties;

	/**
	 * Whether the node is enabled, disabled, or disabled-but-still-compiled.
	 *
	 * Held separately from Properties because it is the one piece of off-pin state that
	 * changes what the graph DOES rather than how it looks, and it was invisible until
	 * 2026-07-25: a disabled node read identically to an enabled one, so a round trip
	 * silently re-enabled it. Every default event on a fresh Actor Blueprint is disabled,
	 * so this was not a rare case.
	 *
	 * Default is Enabled, which is also the engine's default, so it renders nothing.
	 */
	enum class EEnabled : uint8 { Enabled, Disabled, DisabledButCompiled } EnabledState = EEnabled::Enabled;

	/**
	 * The states a `UAnimStateAliasNode` stands for, as the ALIASES of the statements naming
	 * them — `{"n2", "n4"}`, never object paths.
	 *
	 * Held here rather than in Properties because the engine's own value cannot be written back.
	 * `AliasedStateNodes` is a `TSet<TWeakObjectPtr<UAnimStateNodeBase>>`, and the property
	 * channel exports it as absolute paths naming `AnimStateNode_0` inside the graph a write is
	 * about to REBUILD. `UAnimStateAliasNode::Serialize` then calls
	 * `RebuildAliasedStateNodeReferences`, which keeps only nodes still present in the owning
	 * graph — so every exported path is dropped on load and the alias comes back aliasing
	 * nothing, while the write reports success
	 * (`GraphCode.AnimGraph.Write.AnAliasNodesStateSetAfterARoundTrip`, 2026-08-16).
	 *
	 * An alias is document-local and survives the rebuild, which is the same reason a transition
	 * is `Idle -> Run` and not a pair of paths. Only the reader can fill this in: it is the one
	 * place holding both the live node and the alias map.
	 *
	 * EMPTY means "this statement is not an alias node, or its set is genuinely empty", and the
	 * applier writes nothing in that case rather than clearing what is there.
	 */
	TArray<FString> AliasedStates;

	/** Source node GUID. Excluded from round-trip equality; kept for diagnostics. */
	FGuid SourceGuid;

	/** Node comment, if any. Preserved - it is authored intent. */
	FString Comment;
};

/** A variable declaration - member, local, or inherited. */
struct BLUEPRINTIR_API FBpIrVariable
{
	FString Name;
	/** Canonical type string, e.g. "float", "Actor*", "TArray<Vector>". */
	FString Type;
	/** Rendered default value, empty if none. */
	FString DefaultValue;

	enum class EScope : uint8 { Member, Local, Inherited } Scope = EScope::Member;

	/** Inherited variables come from a native or BP parent class; record which. */
	FString DeclaringClass;

	bool bIsArray = false;
	bool bIsSet = false;
	bool bIsMap = false;

	/**
	 * The variable's details-panel switches, as the panel itself resolves them.
	 *
	 * These are semantics, not cosmetics, which is why they are here and the category, the
	 * tooltip and the slider range are not — those are declared in `omits:` instead. The set
	 * stops exactly where the writer stops: every flag below is one `update_variables` can
	 * actually set. Carrying a flag no writer honours is worse than omitting it, because the
	 * text would then invite an edit that parses, applies, reports success and changes
	 * nothing — which is the failure the function parameter list was just rescued from.
	 *
	 * `bInstanceEditable` is NOT `CPF_Edit`. Every Blueprint variable is created with
	 * `CPF_Edit | CPF_BlueprintVisible | CPF_DisableEditOnInstance` together
	 * (BlueprintEditorUtils.cpp:4671) and the checkbox toggles only the third
	 * (BlueprintEditorUtils.cpp:3730). So `CPF_Edit` is set on every variable in every
	 * Blueprint and answers nothing; the panel asks `!CPF_DisableEditOnInstance`
	 * (BlueprintDetailsCustomization.cpp:1955). Reading the first is what made all 955
	 * declarations in the corpus claim to be instance-editable. An inherited native property
	 * needs both halves — `BlueprintReadWrite` alone sets neither flag — so the predicate is
	 * `CPF_Edit && !CPF_DisableEditOnInstance` everywhere, which reduces to the panel's
	 * question for a Blueprint variable and stays correct for a native one.
	 *
	 * `bExposeOnSpawn` has two homes and they can disagree: the `ExposeOnSpawn` metadata,
	 * which is what the panel reads and writes (BlueprintDetailsCustomization.cpp:2188,
	 * :2201), and `CPF_ExposeOnSpawn`. The engine resolves the pair with OR and logs
	 * "ExposeOnSpawn ambiguity" when they differ (EdGraphSchema_K2.cpp:1292-1308), so the
	 * reader ORs them too and the writer sets both rather than minting an ambiguity.
	 *
	 * `bPrivate` is metadata as well (`BlueprintPrivate`), not a property flag.
	 */
	bool bReplicated = false;
	bool bInstanceEditable = false;
	bool bBlueprintReadOnly = false;
	bool bExposeOnSpawn = false;
	bool bSaveGame = false;
	bool bPrivate = false;

	/**
	 * C4.3, 2026-08-06. The rest of the details panel a switch cannot say, so these are KEYED
	 * rather than bare: `category="Stats"`, `repnotify=OnRep_Health`. Two of them are prose, and
	 * a bare token cannot hold prose — which is why the whole group was declared in `omits:`
	 * instead of carried, rather than because nobody had got to it.
	 *
	 * Empty means "the panel's own default" on every one, so an untouched variable renders
	 * exactly as it did before these existed. `Category` is normalised: the engine stores the
	 * literal "Default" for an uncategorised variable, and carrying that would put a key that
	 * says nothing on every declaration in every file.
	 *
	 * `RepNotifyFunction` names a graph that must exist. Setting the flag without one compiles
	 * to an error, so the writer refuses and names the missing function rather than creating it
	 * — creating it is C4.6's lifecycle work.
	 *
	 * `ReplicationCondition` is the `ELifetimeCondition` token, spelled as the engine spells it.
	 *
	 * There is no friendly name here, and that is a measurement rather than an omission to fill in
	 * later. `FBPVariableDescription::FriendlyName` is written by the engine on create and on every
	 * rename, always as `FName::NameToDisplayString(VarName)`
	 * (BlueprintEditorUtils.cpp:4668, :4946, :5214), and the variable details panel neither shows
	 * nor edits it. Carrying it would put a key in the text that the editor never displays and the
	 * next rename overwrites. A user-defined STRUCT member is the opposite case — there the field
	 * IS the display name (StructureEditorUtils.cpp:510) — and that is schemacode's block, not this
	 * one.
	 */
	FString Category;
	FString Tooltip;
	FString RepNotifyFunction;
	FString ReplicationCondition;
	bool bAdvancedDisplay = false;

	/**
	 * C4.3b, 2026-08-06. The rest of the details panel's advanced section, which closes
	 * `variable_metadata` down to the friendly name alone.
	 *
	 * These were held back for one stated reason — no writer, and a flag the text can state but
	 * no writer honours is worse than an omitted one — and that reason expired rather than being
	 * overruled: they are `PropertyFlags` bits and metadata entries, the two things
	 * `update_variables` was already setting for the switches above.
	 *
	 * FOUR of them are property flags (`CPF_Config`, `CPF_Transient`, `CPF_Interp`,
	 * `CPF_Deprecated`) and TWO are metadata, and the two metadata ones do not agree on what a
	 * switch even is. `MultiLine` holds the string "true" and the panel asks
	 * `GetBoolMetaData` (BlueprintDetailsCustomization.cpp:2993); `Bitmask` is written with an
	 * EMPTY value and the panel asks `HasMetaData` (`:2563`, set at `:2578`). Reading Bitmask as
	 * a bool would report every bitmask variable in the project as false — the predicate is
	 * copied from the panel per field, not shared across them.
	 *
	 * The keyed half is the values those switches qualify. `BitmaskEnum` is an asset path and
	 * `DeprecationMessage` is prose, so only the second is quoted. The four range fields are the
	 * panel's Slider Range and Value Range: they are TEXT in the engine, not numbers — the boxes
	 * accept an empty string to mean unbounded, and parsing them into floats here would make an
	 * unset maximum indistinguishable from a maximum of zero.
	 */
	bool bConfig = false;
	bool bTransient = false;
	bool bExposeToCinematics = false;
	bool bDeprecated = false;
	bool bMultiLine = false;
	bool bBitmask = false;

	FString BitmaskEnum;
	FString DeprecationMessage;
	FString ClampMin;
	FString ClampMax;
	FString UIMin;
	FString UIMax;
};

/** One graph within a Blueprint. */
/**
 * A comment box - a labelled region drawn around part of a graph.
 *
 * This is the highest-value prose in a Blueprint and the only place a lot of design
 * intent is written down: "everything below here runs on the server", "legacy path,
 * delete after 5.8". A read that omits it hands the model a graph stripped of the one
 * thing that explains it, so it belongs in the whole-graph extract rather than behind a
 * separate query the model has to know to make.
 *
 * A comment box is NOT a statement. It has no pins, no exec flow, and compiles to
 * nothing; modelling it as one made every box read back as an Opaque statement, which is
 * both noise and a round-trip hazard.
 *
 * It is addressed like a node and emitted like one, at the top of its graph:
 *
 *     c0 = comment("Door state machine - server only") [covers=(n1,n2), color="0.200,0.600,1.000,1.000"]
 *
 * A box is a thing you edit, so it needs a handle; a per-node comment is not, so it has
 * none and renders as a plain `// text` line above its statement. The asymmetry is the
 * point - two kinds of annotation that behave differently should not look identical.
 */
struct BLUEPRINTIR_API FBpIrComment
{
	/** Graph-local handle, "c0", "c1". Parallel to the n0/n1 statement aliases. */
	FString Alias;

	/** The box text, verbatim. Escaped, so embedded newlines survive the round trip. */
	FString Text;

	/**
	 * Aliases of the statements the box encloses, in statement order.
	 *
	 * Geometric on the way out: a statement is covered when its node sits inside the
	 * box. Emitted explicitly rather than implied by position, so a box in the middle of
	 * a long graph still says what it is about after being hoisted to the top.
	 */
	TArray<FString> Covers;

	/** Box colour. Omitted from the text form when it is the default white. */
	FLinearColor Color = FLinearColor::White;

	/**
	 * Source comment node GUID - the `comment_id` that blueprint_modify's
	 * update_comment / remove_comment take. Excluded from round-trip equality, exactly
	 * as FBpIrStatement::SourceGuid is: it identifies an asset, not a meaning.
	 */
	FGuid SourceGuid;
};

/**
 * The details panel beside a function or macro. ADR-10, `BP-AUTHORING-DESIGN.md` §10.
 *
 * Values only. WHERE each one lives is deliberately not modelled here, because the storage is not
 * uniform across carriers and the IR is not the place that knows: `UK2Node_FunctionEntry`,
 * `UK2Node_Tunnel` and `UK2Node_CustomEvent` each declare their own `MetaData`, and a custom event
 * ALSO keeps its own `bIsDeprecated`/`bCallInEditor`/`DeprecationMessage` UPROPERTYs which are the
 * live ones for it while the struct's copies are inert. The reader and the applier share one
 * routing table; §10.2 has it with the `file:line` it was measured from.
 *
 * `HasLatentFunctions` is absent on purpose - `UPROPERTY(transient)` and recomputed by the
 * compiler, so carrying it would state a cache.
 *
 * DEFAULTS ARE THE ENGINE CONSTRUCTOR'S, NOT ZERO. `InstanceTitleColor` starts WHITE, so a reset
 * written as a memzero turns every function title transparent black. Omitting a specifier resets to
 * the value here, which is why these initialisers must keep matching
 * FKismetUserDeclaredFunctionMetadata's.
 */
struct BLUEPRINTIR_API FBpIrGraphMetadata
{
	/** FKismetUserDeclaredFunctionMetadata::ToolTip. Spelled `description=` - the panel's word. */
	FString Description;
	FString Category;
	FString Keywords;
	FString CompactNodeTitle;

	/** Engine default is White, and it is emitted only when it deviates. */
	FLinearColor TitleColor = FLinearColor::White;

	// FUNCTION-ONLY, all five. Every engine reader and writer casts to UK2Node_FunctionEntry, and
	// for the first three the details panel does not draw the row for a macro at all (§10.2 has
	// each `file:line`). A macro leaves them at the defaults below, and the grammar refuses to let
	// a macro's document state one.
	FString DeprecationMessage;
	bool bDeprecated = false;
	bool bCallInEditor = false;
	bool bThreadSafe = false;
	bool bUnsafeDuringConstruction = false;

	/** True when every field is at its default, so the emitter writes no specifier list at all. */
	bool IsDefault() const
	{
		return Description.IsEmpty() && Category.IsEmpty() && Keywords.IsEmpty()
			&& CompactNodeTitle.IsEmpty() && DeprecationMessage.IsEmpty()
			&& TitleColor.Equals(FLinearColor::White) && !bDeprecated && !bCallInEditor
			&& !bThreadSafe && !bUnsafeDuringConstruction;
	}

	bool operator==(const FBpIrGraphMetadata& Other) const
	{
		return Description.Equals(Other.Description, ESearchCase::CaseSensitive)
			&& Category.Equals(Other.Category, ESearchCase::CaseSensitive)
			&& Keywords.Equals(Other.Keywords, ESearchCase::CaseSensitive)
			&& CompactNodeTitle.Equals(Other.CompactNodeTitle, ESearchCase::CaseSensitive)
			&& DeprecationMessage.Equals(Other.DeprecationMessage, ESearchCase::CaseSensitive)
			&& TitleColor.Equals(Other.TitleColor) && bDeprecated == Other.bDeprecated
			&& bCallInEditor == Other.bCallInEditor && bThreadSafe == Other.bThreadSafe
			&& bUnsafeDuringConstruction == Other.bUnsafeDuringConstruction;
	}
	bool operator!=(const FBpIrGraphMetadata& Other) const { return !(*this == Other); }
};

struct BLUEPRINTIR_API FBpIrGraph
{
	FString Name;

	/**
	 * Alias of the node in the containing graph that owns this graph. Empty only for a
	 * top-level graph. Ownership is structural rather than name-based: graph names are
	 * author-facing labels and can change, while the owning statement is already the IR's
	 * identity boundary (ADR-7).
	 */
	FString OwnerAlias;

	/**
	 * Content signature. See ADR-5 at the bottom of this header, and
	 * ComputeContentSignature for what it actually digests.
	 *
	 * A digest over this graph's CANONICAL RENDERED CONTENT - the same text
	 * GraphsEquivalent compares, so the guard is exactly as strong as the equality
	 * predicate the whole project is built on. Costs one line regardless of graph
	 * size. Passed back to bp_apply to prove the read is still current.
	 *
	 * It was a CRC32 over the ordered (alias, NodeGuid) pairs until 2026-07-26, and
	 * that was not a concurrency guard at all: editing a pin literal, a node comment
	 * or a property, or rewiring existing nodes, leaves every GUID untouched, so a
	 * stale edit passed the check and overwrote a human's change. See
	 * ComputeContentSignature.
	 */
	FString Signature;

	/**
	 * `AnimGraph` is appended rather than slotted in beside `Function`, and it exists because
	 * membership in `Blueprint->FunctionGraphs` is NOT the same question as "is this a function".
	 *
	 * Unreal stores an Animation Blueprint's AnimGraph — and every animation layer — in
	 * `FunctionGraphs`, so before 2026-08-07 they all rendered as `function AnimGraph { … }` inside
	 * `functions { }`. Nothing decided that; the array answered a question it does not know. The
	 * cost was not cosmetic: `functions { }` carries a lifecycle whose omission removes, so an
	 * AnimGraph inherited a deletion rule written for functions. Giving it its own kind is what
	 * makes that unsayable rather than merely guarded. Truth-audit §6 Q1.
	 *
	 * `StateMachine` is appended for the same reason one step further in, and the measurement is the
	 * mirror image: `GraphKindOf` asks `IsAnimationGraphSchema`, and `UAnimationStateMachineSchema`
	 * derives from plain `UEdGraphSchema` while `UAnimationStateGraphSchema` derives from the anim
	 * one — so a state machine's own graph answered NO to that question, fell through the three
	 * array checks it is a member of none of, and landed on `Collapsed`. One family, two kind
	 * tokens, and a machine indistinguishable from a person's collapsed graph by its header.
	 *
	 * It matters the moment a writer dispatches on `Kind`, which `BuildOwnedGraphs` already does at
	 * its `ChildIr.Kind != GraphKindOf(ChildGraph)` check. `GC-STATE-MACHINE-DESIGN.md` §0 R10.
	 */
	enum class EKind : uint8 { EventGraph, Function, Macro, Construction, Collapsed, AnimGraph, StateMachine } Kind = EKind::EventGraph;

	/**
	 * THE LAST ENUMERATOR, AND IT HAS TO BE STATED BESIDE THE ENUM BECAUSE THE PARSER SCANS TO IT.
	 *
	 * `KindFromToken` walks `0 .. Last` inclusive, so the sentinel IS the set of tokens the text
	 * form accepts. Appending `AnimGraph` on 2026-08-07 without moving the two call sites left the
	 * parser scanning up to `Collapsed`, so `GraphToText` emitted `animgraph X { … }` and
	 * `GraphFromText` — documented as its exact inverse — read line 1 as a malformed STATEMENT and
	 * refused the whole graph. Nothing noticed for a day: an anim graph is refused by the write
	 * gate long before anything parses one back, so the only caller that would have met it did not
	 * exist yet.
	 *
	 * `EveryGraphKindTokenParsesBackToItsOwnKind` walks the enum and fails on the next append.
	 */
	static constexpr EKind LastKind = EKind::StateMachine;

	/**
	 * The graph's declared signature: parameters in, returns out.
	 *
	 * A parameter is an OUTPUT pin on UK2Node_FunctionEntry and a return is an INPUT pin on
	 * UK2Node_FunctionResult, and statement arguments are input bindings only. So without
	 * these a parameter reaches the text exactly where something reads it, and a parameter
	 * nothing reads is simply absent — indistinguishable from one that was never declared.
	 * These blocks state the signature once, and the entry/result statements keep their
	 * alias and their execution wiring and refer to it.
	 *
	 * Empty means "this graph declares none", not "we did not look": a graph kind with no
	 * signature never writes the block at all.
	 */
	TArray<FBpIrVariable> Inputs;
	TArray<FBpIrVariable> Outputs;

	/** Local variables declared in this graph (function graphs only). */
	TArray<FBpIrVariable> Locals;

	/**
	 * Parameter renames this payload STATES: `rename param <Old> -> <New>`, C23 §6 site 9.
	 *
	 * QUALIFIED because a graph body holds two renameable kinds at the same depth. A parameter's
	 * NAME is its identity — `FUserPinInfo` has no GUID — and a rename costs every call site's WIRE
	 * unless `RenameUserDefinedPin` carries it across; a `local` beside it has a GUID and its rename
	 * mints a fresh one on purpose. A bare token would have two readings with different costs.
	 *
	 * Apply-only, so it is outside the graph's `sig:` for the reason every other directive is: after
	 * the rename there is no old name left for a projection to carry.
	 */
	TArray<AuthoringCore::FAuthoringRename> ParamRenames;

	/**
	 * Aliases this payload REMOVES from the graph: `delete n42` on its own line inside the braces.
	 *
	 * The third verb at statement depth, and it exists because ADR-7 Rule 2 was retired
	 * (user-directed 2026-08-30). While omission WAS deletion the payload had to be the complete
	 * graph, so a removal needed no token — dropping the line was the token. Under
	 * `omission:"patch"` an omitted statement is now left alone, which makes a fragment payload
	 * expressible and leaves removal with nothing to say it. This says it.
	 *
	 * Apply-only, exactly like `ParamRenames` and for the same reason: no projection contains an
	 * imperative, so nothing here is inside the graph's `sig:` and the writer never emits one.
	 *
	 * Refused rather than ignored under `omission:"snapshot"`, where the payload IS the whole set
	 * and the omission already says it — the same shape as `delete <Name>` at block depth.
	 */
	TArray<FString> StatementDeletes;

	/**
	 * The graph's details panel. ADR-10; empty and all-default on every kind that has no such
	 * panel, which is what keeps an eventgraph's header line unchanged.
	 */
	FBpIrGraphMetadata Metadata;

	/** Statements in deterministic traversal order. */
	TArray<FBpIrStatement> Statements;

	/** Comment boxes over this graph, in the order they are emitted. */
	TArray<FBpIrComment> Comments;

	/**
	 * Graphs owned by nodes in this graph, in GetSubGraphs() order per owning node.
	 *
	 * A collapsed graph is the measured case, but neither end tests for
	 * UK2Node_Composite. The engine's virtual GetSubGraphs() is the contract, so a plugin
	 * node which owns a graph participates without being added to a class list. Children
	 * are recursive because Unreal permits composites inside composites.
	 */
	TArray<FBpIrGraph> SubGraphs;

	/**
	 * Nodes we could not model and could not safely drop. Presence here is a
	 * loud signal that this graph will not survive the round-trip gate.
	 */
	TArray<FString> UnsupportedNotes;

	/**
	 * Does this graph declare `Name` as one of its own local variables?
	 *
	 * The question two halves of the write path both have to ask, so it is asked once here
	 * rather than answered twice. The applier asks it to decide whether a symbol has to
	 * exist in the project index (it must not - a local is scoped to one function, and
	 * `BP_Utils` declares `local Sum: real` in one and `local Sum: Vector2D` in another, so
	 * a project-wide local index manufactures ambiguity out of two variables never in the
	 * same scope). The builder asks it to decide whether a variable node gets
	 * `SetLocalMember` or `SetSelfMember`, which is what decides whether it gets a pin at
	 * all. Two answers to that would be a graph that validates and then builds pinless.
	 *
	 * A qualified name is never a local: `Class:Member` names a class member by construction.
	 */
	bool DeclaresLocal(const FString& VariableName) const
	{
		if (VariableName.Contains(TEXT(":")))
		{
			return false;
		}

		return Locals.ContainsByPredicate([&VariableName](const FBpIrVariable& Local)
		{
			return Local.Scope == FBpIrVariable::EScope::Local
				&& Local.Name.Equals(VariableName, ESearchCase::CaseSensitive);
		});
	}
};

/** A component from the Simple Construction Script - these live in no graph. */
struct BLUEPRINTIR_API FBpIrComponent
{
	FString Name;
	FString ClassPath;
	FString ParentName;
};

/**
 * The specifier defaults a document was written under — the `defaults { }` block.
 *
 * A declaration line carries a specifier only when its value DIFFERS from the default
 * here, which is what keeps the common variable down to `Health: float = 100.0`. That
 * makes the block authoritative for the parser rather than decorative: a `.gc` from four
 * hundred commits ago has to keep parsing correctly after the tool changes a default, or
 * `git show` of an old revision renders something untrue. So the block is written on the
 * way out and believed on the way in, never assumed.
 *
 * Every default is false today, so only the positive form is ever emitted. `[!name]`
 * exists so flipping a default later is not a format break.
 *
 * `instance_editable` is the one worth watching. A fresh Blueprint variable is created NOT
 * instance-editable (BlueprintEditorUtils.cpp:4671 sets CPF_DisableEditOnInstance), so false
 * is genuinely the common case and the specifier now appears only on variables somebody
 * ticked the box for. It read the other way round until 2026-07-30, when it was reading a
 * flag that is set on every variable ever made — so a sidecar written before that date
 * carries `[instance_editable]` on declarations that never were.
 */
struct BLUEPRINTIR_API FBpIrSpecifierDefaults
{
	bool bReplicated = false;
	bool bInstanceEditable = false;
	bool bBlueprintReadOnly = false;
	bool bExposeOnSpawn = false;
	bool bSaveGame = false;
	bool bPrivate = false;
	bool bAdvancedDisplay = false;
	bool bConfig = false;
	bool bTransient = false;
	bool bExposeToCinematics = false;
	bool bDeprecated = false;
	bool bMultiLine = false;
	bool bBitmask = false;
};

/**
 * One interface a Blueprint implements, and what it implemented for it.
 *
 * The graphs are the reason this is a struct rather than the bare path it used to be.
 * `FBPInterfaceDescription::Graphs` is a FOURTH graph array on UBlueprint, beside
 * UbergraphPages, FunctionGraphs and MacroGraphs — `UBlueprint::GetAllGraphs` walks all four
 * (`Blueprint.cpp:2001`) and `ReadBlueprintObject` walked three, so an interface function's
 * implementation was invisible.
 *
 * It stayed invisible because of which functions land there. An interface function with NO
 * return value is implemented as an event node in the Ubergraph, which the eventgraph block
 * already carries; only a function WITH a return value gets its own graph here. This project's
 * one interface, `BPI_PawnAnim`, declares `HideUnhideHand(bHide, bRightHand)` with no outputs —
 * so every implementer looked complete and the array was empty everywhere anyone looked.
 */
struct BLUEPRINTIR_API FBpIrInterface
{
	/** The interface's path as the Blueprint stores it, e.g. "/Game/UI/BPI_Click.BPI_Click_C". */
	FString Path;

	/** Graphs from FBPInterfaceDescription::Graphs. Empty for an event-only interface. */
	TArray<FBpIrGraph> Graphs;
};

/**
 * One event dispatcher declaration: the variable entry plus its signature.
 *
 * A dispatcher is TWO engine objects made to look like one declaration
 * (FBlueprintEditor::OnAddNewDelegate, BlueprintEditor.cpp:9607): an FBPVariableDescription
 * whose VarType is PC_MCDelegate, and a non-editable graph in
 * UBlueprint::DelegateSignatureGraphs holding the parameter list on its entry node's
 * UserDefinedPins. DelegateSignatureGraphs is the fourth of the five graph arrays
 * UBlueprint::GetAllGraphs walks (Blueprint.cpp:2001), and the one ReadBlueprintObject did
 * not - so a dispatcher projected as a bare `Name: MCDelegate` variable with its
 * parameters, the part a bind site actually needs, simply absent.
 *
 * It is its own struct rather than a variable with an odd type for the same reason an
 * interface is: the declaration writer behind `properties { }` (update_variables) creates
 * and removes ORDINARY variables and has no signature-graph machinery, so a dispatcher in
 * that block would be a deletion it cannot perform honestly. `dispatchers { }` is
 * projected for review, exactly like `interfaces { }`, and gc_apply refuses it by name.
 */
struct BLUEPRINTIR_API FBpIrDispatcher
{
	/** The variable name, which is also the signature graph's name. */
	FString Name;

	/** The declared parameters, from the signature graph's function entry. May be empty. */
	TArray<FBpIrVariable> Inputs;
};

/**
 * One track on a timeline: a curve, and what the curve drives.
 *
 * The four engine arrays (`EventTracks`, `FloatTracks`, `VectorTracks`, `LinearColorTracks`)
 * collapse into one list with a Kind, because they are one thing to a reader — a named track with
 * a curve — and four blocks that differ only in which array they came from would be four spellings
 * of the same shape. The Kind is written in the text, so the split is recoverable.
 */
struct BLUEPRINTIR_API FBpIrTimelineTrack
{
	enum class EKind : uint8 { Event, Float, Vector, LinearColor };
	EKind Kind = EKind::Float;

	/** The track's authored name. It is also the name of the pin the node exposes for it. */
	FString Name;

	/**
	 * What the track drives: the FUNCTION for an event track, the PROPERTY for the other three.
	 *
	 * One field rather than two because a track is never both, and because the engine's own
	 * structs make the same choice - FTTEventTrack has FunctionName, FTTPropertyTrack has
	 * PropertyName, and no track type has both.
	 */
	FString Binding;

	/**
	 * An EXTERNAL curve is a separate asset the timeline merely points at; this is its path.
	 *
	 * `bIsExternalCurve` is the engine's flag and this field is its projection: non-empty means
	 * external. The distinction matters for writing, not just reading - an external curve must be
	 * re-pointed and never rebuilt, because its keys belong to an asset this Blueprint does not
	 * own and would be someone else's edit to lose.
	 */
	FString ExternalCurvePath;

	/**
	 * An OWNED curve's keys, one entry per component: 1 for float, 3 for vector, 4 for colour.
	 * Empty when the curve is external, and empty for an event track with no keys.
	 *
	 * Each entry is an engine-shaped `FRichCurveKey` array literal, exported and imported through
	 * the struct's own reflection. That is deliberate and it is the same bet the property channel
	 * makes: a key carries an interp mode, a tangent mode, a weight mode, two tangents and two
	 * tangent weights, and a hand-written field list for them is a list that silently stops
	 * matching the engine the first time a field is added. Reflection cannot drift.
	 */
	TArray<FString> Curves;
};

/**
 * One `UTimelineTemplate` — the half of a Timeline that does not live on its node.
 *
 * This is why `timelines` sat in every Blueprint's `omits:` and why a graph holding a Timeline
 * could not be written at all: `UK2Node_Timeline` carries a `TimelineName` and nothing else, and
 * the curves, the length and the flags live in a UTimelineTemplate on the Blueprint. A reader
 * walking the node's properties reaches none of it, so no amount of node-level fidelity helped.
 *
 * The name is the join. `UK2Node_Timeline::TimelineName` and the template's variable name are the
 * same FName, which is what lets the node round-trip as an ordinary statement while its template
 * travels here.
 */
struct BLUEPRINTIR_API FBpIrTimeline
{
	/** The timeline's variable name — the same FName the node's `TimelineName` carries. */
	FString Name;

	float Length = 5.0f;

	/** `ETimelineLengthMode` as its token, e.g. "Keyframe" or "LastKeyFrame". */
	FString LengthMode;

	bool bAutoPlay = false;
	bool bLoop = false;
	bool bReplicated = false;
	bool bIgnoreTimeDilation = false;

	/** `ETickingGroup` as its token, e.g. "TG_PrePhysics". */
	FString TickGroup;

	TArray<FBpIrTimelineTrack> Tracks;
};

/** Whole-Blueprint IR. */
struct BLUEPRINTIR_API FBpIrBlueprint
{
	/** Asset path, e.g. "/Game/BP_Player.BP_Player". */
	FString Path;
	/** Generated class path, e.g. "/Game/BP_Player.BP_Player_C". */
	FString GeneratedClassPath;
	/** Parent class canonical path. */
	FString ParentClassPath;

	/** The specifier defaults every declaration in this document is written against. */
	FBpIrSpecifierDefaults Defaults;

	TArray<FBpIrVariable> Variables;
	TArray<FBpIrComponent> Components;
	TArray<FBpIrGraph> Graphs;

	/** Interfaces this Blueprint implements, each with the function graphs it implemented. */
	TArray<FBpIrInterface> Interfaces;

	/** Event dispatchers this Blueprint declares, each with its signature's parameters. */
	TArray<FBpIrDispatcher> Dispatchers;

	/** Timeline templates this Blueprint owns, each with its tracks and their curve keys. */
	TArray<FBpIrTimeline> Timelines;

	/** True when the Blueprint has no graphs at all (data-only BP). Valid, not an error. */
	bool bDataOnly = false;
};

// ---------------------------------------------------------------------------
// ADR-5: layout signature and differential apply.
// ---------------------------------------------------------------------------
// Recomputing the whole layout on every edit is correct and awful. The human
// opens the Blueprint after a one-line change and every node has moved; their
// spatial memory of the graph is gone. Separately, nothing in the original design
// stops the model applying an edit it computed against a stale read.
//
// One mechanism fixes both.
//
// The key observation is that the applier ALREADY HOLDS THE LIVE GRAPH. It does
// not need the model to send positions back to it. All it needs is to know which
// incoming statement corresponds to which existing node - an identity mapping.
// And because the reader assigns aliases deterministically (ADR-3), the applier can
// simply re-run that assignment on the live graph to rebuild the mapping for free.
// Still true now that a number comes from the node rather than from its position -
// what changed is that the mapping also survives a deletion elsewhere in the graph.
//
// So the only thing that has to travel is proof that the graph has not changed
// since the read. That is what Signature is: a digest over the graph's canonical
// rendered content. One short line, O(1) in tokens no matter how big the graph is.
//
// AMENDED 2026-07-26. The digest was originally specified here as being over the
// ordered (alias, NodeGuid) pairs, and shipped that way. That is enough to prove
// the ALIAS MAPPING is still valid, which is the differential-apply requirement
// this ADR was reasoning about - and it is not enough to prove the graph has not
// changed, which is the sentence above and the safety property that was claimed.
// Editing a pin literal, a node comment, a property or the wiring between existing
// nodes changes no GUID at all, so a stale edit passed the check and clobbered the
// human's change with a clean result. The digest is now over content
// (ComputeContentSignature), which proves both. Positions are still excluded, and
// still for the reason below.
//
// bp_apply then behaves as follows:
//
//   base_sig absent    -> full rebuild, full relayout. Simple and destructive.
//
//   base_sig matches   -> differential apply. Statements that are unchanged keep
//                         their existing node AND its position. Changed statements
//                         are mutated in place. New statements are placed by the
//                         layout pass. Deleted statements are removed. Only nodes
//                         that actually changed ever move.
//
//   base_sig mismatch  -> refuse, and return the current signature. The graph moved
//                         under the model since it read. This is optimistic
//                         concurrency control, and it is the safety property the
//                         design was otherwise missing entirely.
//
// Note the framing: nothing is explicitly "pinned". Position preservation falls
// out of the diff rather than out of a freeze list the model has to maintain -
// so it cannot accidentally pin a node it meant to move, and it cannot forget to
// pin one it meant to keep.
//
// Consequence for the layout pass (M6): layout becomes incremental. It must place
// new nodes into the gaps around existing ones rather than assuming a blank canvas.
// That is strictly harder than a clean layered pass and is the right trade.
//
// ---------------------------------------------------------------------------
// Serialisation (ADR-4): line-oriented text OUT, JSON IN.
//
// Text out is diffable and materially cheaper in tokens, which is the entire
// point of the project. JSON in is easier to validate, and the write path needs
// validation far more than it needs prettiness.
// ---------------------------------------------------------------------------

namespace BlueprintIR
{
	/** Render a whole Blueprint to the line-oriented text form. */
	BLUEPRINTIR_API FString ToText(const FBpIrBlueprint& Ir);

	/**
	 * The same blocks `ToText` writes, without the `blueprint`/`parent`/`implements` header.
	 *
	 * For a caller that already owns the identity header and needs to interleave blocks of
	 * its own — GraphCode's `.gc`, where one document holds a widget tree and a graph and
	 * the asset is named once. Splitting them here rather than letting that caller assemble
	 * two finished documents is what stops the same asset acquiring two descriptions.
	 *
	 * `defaults` is separate because it must be written before anything that reads it.
	 */
	BLUEPRINTIR_API FString DefaultsBlock(const FBpIrSpecifierDefaults& Defaults);

	/** One named set of specifier defaults, as `defaults { }` names it. */
	struct FBpIrDefaultSetInfo
	{
		FString Name;
		/** Specifiers this name defaults to TRUE. Everything it does not name is false. */
		TArray<FString> True;
	};

	/**
	 * What `defaults { }` can name, what each name contains, and which specifiers the block
	 * states on their own line whatever a name says.
	 *
	 * Published because a name that cannot be expanded is a name a reader has to take on
	 * faith. `gc_describe block:"defaults"` answers it, so "what does blueprint_defaults
	 * mean" is a question with an address rather than a source file to go and read.
	 *
	 * All three rosters come from the one binding table the writer and parser share, so a
	 * specifier cannot be carried by the format and missing from its own description.
	 */
	BLUEPRINTIR_API TArray<FBpIrDefaultSetInfo> DefaultSetRoster();
	BLUEPRINTIR_API TArray<FString> SpecifierRoster();
	BLUEPRINTIR_API TArray<FString> AlwaysStatedSpecifiers();

	/**
	 * One specifier a `[…]` group can carry — a graph's header line, or a DECLARATION.
	 *
	 * **IT WAS `FBpIrGraphSpecifierInfo` UNTIL 2026-08-22 AND THE NAME WAS THE SMALLER HALF OF A
	 * REAL GAP.** `kind:"specifiers"` promises that *"the per-block set of ACCEPTED specifiers is on
	 * the block's own answer as `entry_specifiers`, generated from the writer's table, so it can
	 * never advertise one no writer honours"* — and the roster behind it existed for a function
	 * HEADER only. `properties { }` carried none, in any of the six Blueprint kinds, and neither did
	 * the `input`/`output`/`local` declarations inside a function body, which share its grammar
	 * exactly. So 23 tokens were advertised as discoverable and were not (CP2 F17).
	 *
	 * The struct describes a specifier, not a graph's specifier. Two rosters fill it now, from the
	 * two writer tables, and the shape is identical because the QUESTION is: what may I write in
	 * this bracket, and what does leaving it out do.
	 */
	struct FBpIrSpecifierInfo
	{
		/** The token as written, e.g. `category`. */
		FString Token;
		/** What it sets, by the details panel's own label. */
		FString Sets;
		/**
		 * `switch`, `text`, `token` or `color`. A switch is bare; the others take `=value`.
		 *
		 * `text` and `token` are one distinction and it is the one `kind:"specifiers"` states in
		 * prose — *"quoting follows the value"*. `category="Combat"` is prose and quotes;
		 * `repnotify=OnRep_Health` is an identifier and `clamp_min=0` is a number, and neither does.
		 * Publishing it per specifier makes that rule mechanical instead of something a reader has
		 * to apply, which matters here because getting it wrong is a parse failure rather than a
		 * no-op.
		 */
		FString Type;
		/**
		 * What OMITTING it resets the field to, rendered the way the document would write it.
		 *
		 * Published because omission is a reset rather than a "leave alone", so a caller who
		 * cannot see this cannot predict what dropping a specifier does. `title_color` is the
		 * case that makes it necessary: it defaults to WHITE, not to the zero a reader assumes,
		 * and a prose rule cannot answer that without being parsed as English.
		 */
		FString Default;
	};

	/**
	 * Every specifier THIS KIND accepts, from the one table the emitter and parser use.
	 *
	 * Per-kind rather than one list with a footnote, because the answer for a macro has to OMIT
	 * the five function-only specifiers rather than list them with a caveat — a caveat is a rule
	 * the reader has to apply, and the failure it guards against is a payload that parses,
	 * applies, reports success and changes nothing.
	 *
	 * Empty for any kind with no details panel.
	 */
	BLUEPRINTIR_API TArray<FBpIrSpecifierInfo> GraphSpecifierRoster(FBpIrGraph::EKind Kind);

	/**
	 * Every specifier a DECLARATION's `[…]` group accepts — CP2 F17, 2026-08-22.
	 *
	 * `SpecifierRoster()` above already returns these tokens and returns ONLY the tokens: thirteen
	 * bare names, no label, no type, no default. That is enough to validate and not enough to
	 * author, which is why `gc_describe` could not publish it as `entry_specifiers` and why the
	 * shipped `specifiers` topic promised a table that did not exist for this bracket.
	 *
	 * ONE ROSTER FOR FIVE BLOCKS, and that is a fact about the writer rather than a simplification:
	 * `DeclarationToText` is a single function and `AppendDeclarationBlock` runs it for
	 * `properties`, `input`, `output`, `local` and `inherited` alike. A per-block roster would be
	 * five copies of one answer, and the day they disagreed one of them would be wrong.
	 *
	 * NOT per-kind, unlike `GraphSpecifierRoster`. That one splits because the engine reads five of
	 * its specifiers only from a `UK2Node_FunctionEntry`, so a macro must be offered fewer. Nothing
	 * equivalent narrows a declaration: the same `FBpIrVariable` fields are read wherever one is
	 * declared, so a split here would be a distinction the writer does not make.
	 */
	BLUEPRINTIR_API TArray<FBpIrSpecifierInfo> DeclarationSpecifierRoster();

	BLUEPRINTIR_API void DeclarationBlocks(const FBpIrBlueprint& Ir, TArray<FString>& OutBlocks);
	BLUEPRINTIR_API void GraphBlocks(const FBpIrBlueprint& Ir, TArray<FString>& OutBlocks);

	/**
	 * `interfaces { }` — which interfaces are implemented, and their function graphs.
	 *
	 * This replaced the `implements:` preamble line rather than joining it. Two spellings for
	 * one piece of state is how a `.gc` and a bp_read come to describe the same asset
	 * differently, which GC-SIDECAR-DESIGN §13.1 exists to prevent — and a block says strictly
	 * more, because a header line has nowhere to put a graph. The line is still recognised by
	 * IsBlueprintPreambleLine so text written by an older build keeps parsing.
	 */
	BLUEPRINTIR_API void InterfaceBlocks(const FBpIrBlueprint& Ir, TArray<FString>& OutBlocks);

	/**
	 * `dispatchers { }` — event dispatcher declarations with their parameter lists.
	 *
	 * Projected for review, read-only, on the `interfaces { }` precedent: a dispatcher is a
	 * variable entry plus a DelegateSignatureGraph, and neither half has a writer behind a
	 * text block yet. Graph statements that bind, unbind or call one — AddDelegate,
	 * RemoveDelegate, CallDelegate, ClearDelegate, AssignDelegate — ARE ordinary graph
	 * statements and round-trip through the graph blocks.
	 */
	BLUEPRINTIR_API void DispatcherBlocks(const FBpIrBlueprint& Ir, TArray<FString>& OutBlocks);

	/**
	 * `timelines { }` — the UTimelineTemplate half of every Timeline node.
	 *
	 * Rule 7 throughout: a timeline is `Name = Timeline(props) { tracks }` and a track is
	 * `Name = FloatTrack(property=…, keys=…)`. A track naming an EXTERNAL curve carries
	 * `curve=<path>` and no keys, because those keys are another asset's content.
	 */
	BLUEPRINTIR_API void TimelineBlocks(const FBpIrBlueprint& Ir, TArray<FString>& OutBlocks);

	/**
	 * Parse a `timelines { }` block back into templates — the exact inverse of TimelineBlocks.
	 *
	 * Takes the WHOLE block including its `timelines {` opener and closing brace, which is what
	 * TimelineBlocks emits and what a sidecar's block text is, so the round-trip gate can be
	 * written as one composition of the two.
	 *
	 * Purely syntactic, deliberately. A `length_mode` or `tick_group` token is carried through as
	 * written and resolved against the engine's enum by the WRITER, on the same division the graph
	 * grammar already draws: the parser decides whether the text is this language, and apply decides
	 * whether the names in it exist. Nothing here loads a curve asset or touches a UObject.
	 *
	 * `length` is required because the emitter always writes it, so absence is not this grammar
	 * rather than a defaulted timeline. `tick_group` absent DOES mean TG_PrePhysics — the emitter
	 * elides exactly that value, and a parser that did not restore it would fail the round trip on
	 * every timeline in the corpus. Every flag absent means false for the same reason.
	 */
	BLUEPRINTIR_API bool TimelinesFromText(const FString& Text, TArray<FBpIrTimeline>& OutTimelines,
		FString& OutError);

	/** Render a single graph to the line-oriented text form. */
	BLUEPRINTIR_API FString GraphToText(const FBpIrGraph& Graph);

	/**
	 * Does this declaration come back unchanged after being written down and read again?
	 *
	 * Asked by the reader before it lets `input { }` / `output { }` become the ONLY statement of
	 * a function's signature. Rendering a declaration was always safe when something else carried
	 * the same fact; once the block is the only copy, a value the line-oriented form cannot
	 * survive stops being cosmetic and starts being a parameter written back wrong.
	 *
	 * The failure it is actually guarding is a DEFAULT VALUE, because that is the one field
	 * nothing else proves: a name goes through QuoteIdentifier/ParseIdentifier and a pin type
	 * through CompactPinTypeToken, and both of those pairs already check themselves. A default
	 * holding a newline ends the line early, and one holding a bracket at paren depth zero is
	 * read as a specifier list.
	 *
	 * Written as a round trip rather than as a list of forbidden characters for the same reason
	 * CompactPinTypeToken is: the refusal cannot drift out of step with the parser, because it IS
	 * the parser.
	 */
	BLUEPRINTIR_API bool DeclarationSurvivesText(const FBpIrVariable& Declaration);
	/** Shared declaration grammar for native graph adapters. Values remain in native storage form. */
	BLUEPRINTIR_API FString DeclarationToText(const FBpIrVariable& Variable, const FBpIrSpecifierDefaults& Defaults);
	BLUEPRINTIR_API bool DeclarationFromText(const FString& Line, const FString& Where,
		const FBpIrSpecifierDefaults& Defaults, FBpIrVariable& OutVariable, FString& OutError);

	/** Render a single statement to one line. */
	BLUEPRINTIR_API FString StatementToText(const FBpIrStatement& Stmt);
	/** Shared node syntax, also used by native graph adapters. */
	BLUEPRINTIR_API bool StatementFromText(const FString& Line, const FString& Where,
		FBpIrStatement& OutStatement, FString& OutError);

	/** Render a value to its expression form. */
	BLUEPRINTIR_API FString ValueToText(const FBpIrValue& Value);

	/** JSON form, used for bp_apply input and for structured tool results. */
	BLUEPRINTIR_API TSharedPtr<FJsonObject> ToJson(const FBpIrBlueprint& Ir);
	BLUEPRINTIR_API TSharedPtr<FJsonObject> GraphToJson(const FBpIrGraph& Graph);

	/** Parse the JSON form back into a graph. Returns false and fills OutError on failure. */
	BLUEPRINTIR_API bool GraphFromJson(const TSharedPtr<FJsonObject>& Json, FBpIrGraph& OutGraph, FString& OutError);

	/**
	 * Parse the line-oriented text form back into a graph — the exact inverse of
	 * GraphToText, and the PRIMARY write-path input (ADR-6, Docs/BP-AUTHORING-DESIGN.md
	 * §6b). Same notation in as out, so a model never has to translate its own read
	 * output; JSON carries the envelope, this carries the logic.
	 *
	 * The header line is optional: a graft fragment is bare statements and its base
	 * signature travels in the envelope. Errors are located by line and fill OutError.
	 *
	 * Empty input is a legal empty graph. Guarding a destructive apply is the envelope's
	 * job (an explicit mode), not this parser's.
	 */
	BLUEPRINTIR_API bool GraphFromText(const FString& Text, FBpIrGraph& OutGraph, FString& OutError);

	/**
	 * Parse a whole bp_read DOCUMENT: blueprint-level preamble plus any number of graphs.
	 *
	 * This is what makes "read, edit, write back" literally true. `ToText` emits
	 * `blueprint /Game/BP_X.BP_X` on line 1, which is not a graph kind, so handing
	 * bp_read's own output to GraphFromText used to fail on the first line and the caller
	 * had to slice a graph block out by hand. Under ADR-7, where the payload must be the
	 * COMPLETE graph and omission means deletion, a hand-sliced payload is a hazard, not
	 * an inconvenience.
	 *
	 * Preamble lines (`blueprint`, `parent`, `implements`, `var`, `component`) are skipped
	 * rather than parsed: the write unit is one graph, so a graph edit must not be able to
	 * declare a member variable as a side effect.
	 *
	 * Line numbers in errors refer to the original text throughout.
	 */
	BLUEPRINTIR_API bool GraphsFromText(const FString& Text, TArray<FBpIrGraph>& OutGraphs, FString& OutError);

	/**
	 * Every state and conduit each state machine in this tree declares, keyed by the machine graph's
	 * name. Machines nest, so the walk is recursive and a name collision keeps the first.
	 *
	 * **DECLARED BESIDE THE GRAMMAR RATHER THAN BESIDE ITS CALLER, and that is the whole reason it
	 * is a function instead of six lines in `BlueprintIRApply.cpp`.** "Which statement is a state"
	 * is answered by the four op tokens `statemachine { }` is built on, and those live once, in
	 * `BlueprintIRSerialize.cpp`. A second copy in the applier would be a second place to update the
	 * day a node class is added — the shape [[one-concept-two-grammars]] names, and the specific
	 * failure it produces here is a truncation guard that stops seeing a kind of state.
	 *
	 * Symmetric over the read and the write side by construction: the reader emits these ops and
	 * `MaterialiseMachine` parses the grammar back into them, so a live projection and a parsed
	 * payload are comparable without either being rendered to text.
	 *
	 * A state's NAME is its bound graph's name (`GC-STATE-MACHINE-DESIGN.md` §4.2), so a state with
	 * no bound graph contributes nothing — it has no name to contribute, and the grammar refuses to
	 * project one anyway.
	 */
	BLUEPRINTIR_API void CollectStateMachineStates(
		const FBpIrGraph& Graph, TMap<FString, TArray<FString>>& OutStatesByMachine);

	/**
	 * Structural equality for the round-trip gate.
	 * Ignores node GUIDs and positions. Ignores NOTHING else - that narrowness is
	 * what makes the invariant worth having.
	 */
	BLUEPRINTIR_API bool GraphsEquivalent(const FBpIrGraph& A, const FBpIrGraph& B, FString& OutFirstDifference);

	/**
	 * The ADR-5 concurrency digest: BLAKE3 over this graph's canonical rendered text,
	 * with `sig:` itself excluded.
	 *
	 * Deliberately the SAME text GraphsEquivalent compares, and it lives beside it for
	 * that reason. Two graphs have the same signature exactly when they are equivalent,
	 * so "base_sig still matches" and "the graph you read is the graph that is there"
	 * are one statement rather than two that can disagree. Positions fall out excluded
	 * because the renderer never emits them - not because of a skip list that could be
	 * forgotten - which is precisely what ADR-5 requires: nudging a node by hand must
	 * not force a re-read.
	 *
	 * What that buys over the (alias, NodeGuid) digest it replaced: a change to a pin
	 * literal, a node comment, a property-channel entry, a node's enabled state, a
	 * local variable, a comment box, or the wiring between existing nodes all move the
	 * signature. None of them moved the old one, because none of them changes a GUID.
	 *
	 * Rendered as 16 hex characters of digest plus the statement count. 16 and not 8
	 * because 8 is the 32-bit space this replaced, and a signature collision is a
	 * silent wrong write - the one failure mode with no diagnostic anywhere. The count
	 * suffix is for the human reading a diff; nothing parses it.
	 */
	BLUEPRINTIR_API FString ComputeContentSignature(const FBpIrGraph& Graph);
}
