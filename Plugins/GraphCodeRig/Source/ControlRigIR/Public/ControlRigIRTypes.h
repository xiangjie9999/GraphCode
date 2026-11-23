#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

/** Control Rig grammar version, shared by projection and native graph apply. */
#define CONTROLRIGIR_VERSION 4

/**
 * WHAT A RIG ELEMENT IS. The engine's `ERigElementType` without the bitmask, because a document
 * names ONE type per line and the mask's `All`/`First`/`Last` entries are iteration helpers rather
 * than things an element can be.
 *
 * `Space` is deliberately absent: the engine keeps it as a hidden alias of `Null` for old assets,
 * so spelling both would give one element two possible tokens and make the format ambiguous about
 * a thing it can already say.
 */
enum class EControlRigIRElementType : uint8
{
	Bone,
	Null,
	Control,
	Curve,
	Physics,
	Reference,
	Connector,
	Socket,
};

/**
 * WHAT A NODE IS, and the list is closed on purpose.
 *
 * A RigVM graph can hold node classes this reader does not understand, such as a collapse node. **None of them is silently dropped**: the reader records an `Unsupported`
 * diagnostic naming the node and its class, and the projector decides what that means for the
 * document. That is the rule `GC-RIG-LEDGER.md` §12.10 states for the round trip and the one
 * MaterialIR already follows — a reader that quietly loses a node is worse than no reader.
 */
enum class EControlRigIRNodeKind : uint8
{
	/** A `URigVMUnitNode` — a struct-backed stock node, which is all a generated rig contains. */
	Unit,

	/** A `URigVMDispatchNode` — a template resolved to one permutation, named by its notation. */
	Dispatch,

	/** A `URigVMVariableNode` — a get or a set of a graph variable. */
	Variable,

	/** A `URigVMRerouteNode` — a wire elbow, which carries a value and no behaviour. */
	Reroute,

	/** A `URigVMCommentNode` — prose over the canvas. */
	Comment,
	Branch,
	FunctionReference,
	Entry,
	Return,
};

/** A data link at its consuming pin. Segments can address nested struct or array pins. */
struct CONTROLRIGIR_API FControlRigIRLink
{
	/** The consuming pin, relative to the node that owns this link — `Input`, `Transform.Rotation`. */
	FString TargetSegment;

	/** The node the value comes from. */
	FString SourceNode;

	/** The producing pin, relative to `SourceNode`. */
	FString SourceSegment;
};

/** An execution edge at its producing pin, using the shared `: Pin->Target.Pin` grammar. */
struct CONTROLRIGIR_API FControlRigIRExecEdge
{
	FString SourceSegment;
	FString TargetNode;
	FString TargetSegment;
};

struct CONTROLRIGIR_API FControlRigIRPinType
{
	FString Pin;
	bool bOutput = false;
	FString Type;
};

/** A native node, identified by its stable RigVM name within its graph. */
struct CONTROLRIGIR_API FControlRigIRNode
{
	FString Alias;
	EControlRigIRNodeKind Kind = EControlRigIRNodeKind::Unit;

	/**
	 * The short token a statement leads with — `GetTransform`, `MathVectorAdd`.
	 *
	 * Resolve it with `ControlRigIR::LookupNodeStruct` and compare RESOLVED STRUCTS, never this
	 * string against a path: an ambiguous short name falls back to the full path, so the same
	 * struct can legitimately reach here spelled either way. Exactly MaterialIR's `ClassToken`
	 * ruling, with a different prefix table.
	 */
	FString TypeToken;

	/**
	 * Pin default values, as `<segment path> = <value>` — only where the pin deviates from what the
	 * node was born with, the same subtraction every other block in this language makes.
	 *
	 * A LINKED PIN CARRIES NO DEFAULT. The engine keeps the last typed value behind a wire and
	 * hands it back from `GetDefaultValue`, so writing both would put a number in the file that
	 * nothing reads and that a rebuild would not reproduce.
	 */
	TArray<FAuthoringProperty> Defaults;

	/** Every data link that lands on this node, ordered by target segment. */
	TArray<FControlRigIRLink> Inputs;
	TArray<FControlRigIRExecEdge> ExecOut;
	TArray<FControlRigIRPinType> PinTypes;

	/** A comment node's prose; empty for every other kind. */
	FString Text;
};

/**
 * A MEMBER VARIABLE, FUNCTION LOCAL OR SIGNATURE ARGUMENT.
 *
 * **IT IS HERE BECAUSE A `.gc` REFLECTS WHAT YOU AUTHOR AND SEE IN THE EDITOR** (user-ruled). A
 * variable NODE is a use; this is the DECLARATION, and a graph carrying only the uses would
 * describe something nobody could rebuild — and would silently lose a variable that is declared and
 * not yet read anywhere.
 *
 * **ONLY A FUNCTION HAS LOCALS, AND THAT IS THE ENGINE'S RULE RATHER THAN THIS READER'S** — measured
 * 2026-09-17 after a spec fixture could not declare one on the rig's own model.
 * `URigVMController::AddLocalVariable` returns an empty description unless the graph's outer is a
 * `URigVMLibraryNode` whose own outer is a `URigVMFunctionLibrary`, so a `solve` body never carries
 * a `local` line and its absence there is not a gap.
 *
 * Member declarations from `UBlueprint::NewVariables` live in the document's Variables table and
 * project in `properties {}`. These are the inputs an Animation Blueprint can expose.
 * Function arguments reuse the type/default fields with their direction in Category.
 */
struct CONTROLRIGIR_API FControlRigIRVariable
{
	FString Name;

	/** The C++ type as the model states it — `FVector`, `float`, `TArray<FTransform>`. */
	FString Type;

	FString Default;
	FString TypeObject;
	bool bPublic = false;
	bool bReadOnly = false;
	FString Category;
	FString Tooltip;
};

/**
 * ONE GRAPH — the rig's own model, or one of its functions.
 *
 * The default model is named by the engine and is the graph a Forwards Solve lives in. Functions
 * are listed beside it rather than nested, because a function is addressable on its own and
 * nesting would make its statements depend on where the reader happened to be.
 */
struct CONTROLRIGIR_API FControlRigIRGraph
{
	FString Name;
	bool bIsFunction = false;
	bool bMutable = false;
	/** Function signature, with input/output/io in Category. */
	TArray<FControlRigIRVariable> Arguments;

	/** Declared before the statements, because a statement may read one. Sorted by name. */
	TArray<FControlRigIRVariable> Variables;

	TArray<FControlRigIRNode> Nodes;
};

/**
 * ONE HIERARCHY ELEMENT — a bone, a control, a null, a socket.
 *
 * `Parent` is a KEY rather than a name: two elements of different types may share a name, and the
 * engine's own `FRigElementKey` is (type, name) for exactly that reason. It is spelled
 * `<type> <name>` and is empty for a root.
 */
struct CONTROLRIGIR_API FControlRigIRElement
{
	EControlRigIRElementType Type = EControlRigIRElementType::Bone;
	FString Name;
	FString Parent;

	/**
	 * The element's INITIAL local transform — where it sits before any solve runs.
	 *
	 * Initial and not current, because the current one is whatever the last evaluation left behind
	 * and is not asset state. Local and not global, because a hierarchy states each element against
	 * its parent and a global value would change under a parent's edit without this line changing.
	 */
	FTransform InitialLocal = FTransform::Identity;

	/**
	 * A CONTROL'S OFFSET AND SHAPE TRANSFORMS. Identity on every other element type.
	 *
	 * **A CONTROL IS THREE TRANSFORMS AND CARRYING ONE OF THEM WOULD BE THE SILENT LOSS.** `InitialLocal`
	 * above is where the control's VALUE sits; the OFFSET is the frame that value is measured against —
	 * what a rigger sets when they line a control up with the bone it drives — and the SHAPE transform
	 * is where the drawn gizmo sits relative to that, which is what *"make this control bigger"* moves.
	 * All three are authored by hand in the Control Rig editor and none is derived from the others, so
	 * a document holding only the first describes a rig whose controls are all in the wrong place.
	 *
	 * Local and initial, for the reason `InitialLocal` states: a global value moves when a parent is
	 * edited and a current one is whatever the last evaluation left behind.
	 */
	FTransform ControlOffset = FTransform::Identity;
	FTransform ControlShape = FTransform::Identity;

	/**
	 * True where a bone came from a skeleton rather than from a rigger's hand.
	 *
	 * **It is the one fact about a bone that a rebuild needs and cannot derive.** An imported bone
	 * is replaced wholesale the next time the skeleton is imported; a user bone is not, and
	 * confusing the two loses hand-authored structure on the next import.
	 */
	bool bImported = false;

	/** Control settings — shape, colour, limits — for a control, and empty for everything else. */
	TArray<FAuthoringProperty> Settings;
};

struct CONTROLRIGIR_API FControlRigIRDocument
{
	int32 Version = CONTROLRIGIR_VERSION;
	FString AssetPath;

	/** The rig's generated class parent — `UControlRig` unless a rigger chose otherwise. */
	FString ParentPath;

	TArray<FAuthoringProperty> Settings;
	TArray<FControlRigIRElement> Hierarchy;
	TArray<FControlRigIRGraph> Graphs;
	/** Member declarations precede solve/function bodies in graph{}. */
	TArray<FControlRigIRVariable> Variables;
	/**
	 * WHAT COULD NOT BE READ, **KEPT PER BLOCK RATHER THAN IN ONE PILE.**
	 *
	 * A diagnostic is written into the document as a `# UNSUPPORTED:` comment inside the block it
	 * belongs to, which is the convention every other GraphCode block follows — so the reader has to
	 * know which block that is. One aggregate list could not say: a multi-parented control and an
	 * unreadable node class are both *"something in this rig"*, and putting either under the wrong
	 * brace sends a reader to look for a line that is not there.
	 */
	TArray<FAuthoringDiagnostic> SettingsDiagnostics;
	TArray<FAuthoringDiagnostic> HierarchyDiagnostics;
	TArray<FAuthoringDiagnostic> GraphDiagnostics;
};

/**
 * THE ONE ORDERING RULE FOR NODES, used by both the reader and the serializer.
 *
 * It is here rather than in either of them for the reason `MaterialIRParameterLess` gives: this
 * order is what makes a projection byte-stable, and a rule that drifted between read order and
 * write order would show up only as a sidecar that no longer matches itself.
 *
 * **BY NAME, AND NEVER BY THE MODEL'S OWN ARRAY ORDER.** `URigVMGraph::GetNodes` returns creation
 * order, so deleting one node and re-adding it — which is what editing a rig IS — would rewrite
 * every line below it and turn a one-node edit into a whole-file diff.
 */
inline bool ControlRigIRNodeLess(const FControlRigIRNode& A, const FControlRigIRNode& B)
{
	return A.Alias.Compare(B.Alias, ESearchCase::CaseSensitive) < 0;
}

/**
 * THE ORDER `FControlRigIRDocument::Hierarchy` IS IN, stated here because it is not a comparator
 * and cannot be one.
 *
 * **PARENTS BEFORE CHILDREN, SIBLINGS BY NAME.** Not a flat sort, because a flat sort cannot
 * express it: sorting by name alone puts `pelvis` after `neck` and a rebuild then has nothing to
 * attach the neck to, and sorting by type first separates a socket from the bone it hangs on.
 * `ControlRigIR::ReadHierarchy` walks the tree to produce it, and the sibling rule is what keeps a
 * rename from moving every line around it.
 */

