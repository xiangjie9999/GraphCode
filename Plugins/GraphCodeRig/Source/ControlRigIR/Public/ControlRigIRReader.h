#pragma once

#include "CoreMinimal.h"
#include "ControlRigIRTypes.h"

class UControlRigBlueprint;
class URigHierarchy;
class URigVMGraph;
class URigVMNode;
namespace AuthoringCore { struct FPropertyExportPolicy; }

namespace ControlRigIR
{
	/**
	 * The token a statement leads with, e.g. `GetTransform`, `MathVectorAdd`.
	 *
	 * **THE SAME RULE BlueprintIR APPLIES TO `K2Node_` AND MaterialIR TO `MaterialExpression`**, and
	 * it matters more here than in either: every stock rig node is `FRigUnit_…` or
	 * `FRigVMFunction_…`, so a graph written in full paths is thirty characters of
	 * `/Script/ControlRig.RigUnit_` on every line.
	 *
	 * **A STRIPPED NAME THAT NAMES TWO STRUCTS FALLS BACK TO THE FULL PATH**, so the token always
	 * names exactly one thing. That is not hypothetical here — `FRigUnit_GetTransform` and
	 * `FRigVMFunction_GetTransform` would both strip to `GetTransform` — which is why the caller
	 * must resolve with `LookupNodeStruct` rather than compare this string against a path.
	 */
	CONTROLRIGIR_API FString NodeTypeToken(const UScriptStruct* Struct);

	/**
	 * The struct a token names, or null. The exact inverse of `NodeTypeToken`.
	 *
	 * A token starting with `/` is a full struct object path and bypasses the stripped index
	 * entirely — the discriminator MaterialIR uses, and for the same reason: a package path always
	 * begins with one and a C++ identifier never can.
	 */
	CONTROLRIGIR_API UScriptStruct* LookupNodeStruct(const FString& Token);

	/** The token/enum pair for an element type, and its exact inverse. */
	CONTROLRIGIR_API const TCHAR* ElementTypeToken(EControlRigIRElementType Type);
	CONTROLRIGIR_API bool TryParseElementType(const FString& Token, EControlRigIRElementType& OutType);

	/** The token/enum pair for a node kind, and its exact inverse. */
	CONTROLRIGIR_API const TCHAR* NodeKindToken(EControlRigIRNodeKind Kind);
	CONTROLRIGIR_API bool TryParseNodeKind(const FString& Token, EControlRigIRNodeKind& OutKind);

	/** An element key as a document spells it — `bone pelvis`, `control Hand_L`. Empty for none. */
	CONTROLRIGIR_API FString ElementKeyText(EControlRigIRElementType Type, const FString& Name);

	/** Editable semantic fields carried by a Control Rig's `settings` block. */
	CONTROLRIGIR_API AuthoringCore::FPropertyExportPolicy RigSettingsPropertyPolicy();

	/**
	 * Read one rig hierarchy: every bone, control, null and socket, parents before children.
	 *
	 * Nothing here is authored, and the hierarchy is nonetheless taken NON-CONST. That is the
	 * engine's own const-correctness rather than laxness on this side:
	 * `URigHierarchy::GetControlOffsetTransform` takes a mutable `FRigControlElement*` because
	 * reading a local transform it holds only globally COMPUTES and CACHES it on the element. A
	 * `const_cast` at the call would hide a real mutation behind a promise this signature cannot
	 * keep; `UControlRigBlueprint::GetHierarchy() const` hands back a mutable hierarchy anyway, so
	 * the honest spelling costs the caller nothing.
	 *
	 * `OutElements` is APPENDED to, so a caller assembling a document from more than one source
	 * keeps what it already had.
	 */
	CONTROLRIGIR_API bool ReadHierarchy(
		URigHierarchy* Hierarchy,
		TArray<FControlRigIRElement>& OutElements,
		TArray<FAuthoringDiagnostic>& OutDiagnostics,
		FString& OutError);

	/**
	 * Read one RigVM graph — its nodes, their deviating pin defaults, and every link, spelled at
	 * the pin that consumes it.
	 *
	 * **A NODE CLASS THIS DOES NOT UNDERSTAND IS A DIAGNOSTIC, NEVER A SKIP.** See
	 * `EControlRigIRNodeKind`: the whole point of the kind list being closed is that the reader can
	 * say what it could not read.
	 */
	CONTROLRIGIR_API bool ReadGraph(
		const URigVMGraph* Graph,
		FControlRigIRGraph& OutGraph,
		TArray<FAuthoringDiagnostic>& OutDiagnostics,
		FString& OutError);

	/**
	 * Read a whole Control Rig Blueprint: settings, hierarchy, the default model and its functions.
	 *
	 * **THE MODEL AND NOT THE EDGRAPH.** `URigVMEdGraphSchema` is not a `UEdGraphSchema_K2`, so
	 * BlueprintIR refuses this asset and is right to — the EdGraph is a view that the RigVM model
	 * generates, and reading the view would make the document depend on whether an editor had ever
	 * been opened on the asset.
	 */
	CONTROLRIGIR_API bool ReadControlRig(
		const UControlRigBlueprint* Rig,
		FControlRigIRDocument& OutDocument,
		FString& OutError);
}
