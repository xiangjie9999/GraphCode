#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

/** The first Material grammar version, shared by projection and differential graph apply. */
#define MATERIALIR_VERSION 1

enum class EMaterialIRParameterAssociation : uint8
{
	Global,
	Layer,
	Blend,
};

enum class EMaterialIRParameterType : uint8
{
	Scalar,
	Vector,
	DoubleVector,
	Texture,
	TextureCollection,
	ParameterCollection,
	RuntimeVirtualTexture,
	SparseVolumeTexture,
	Font,
	StaticSwitch,
	StaticComponentMask,
};

/** One enabled override, identified by the same three fields Unreal uses. */
struct MATERIALIR_API FMaterialIRParameter
{
	FString Name;
	EMaterialIRParameterAssociation Association = EMaterialIRParameterAssociation::Global;
	int32 Index = INDEX_NONE;
	EMaterialIRParameterType Type = EMaterialIRParameterType::Scalar;
	FString Value;

	/** Type-specific authored state such as a font page. */
	TArray<FAuthoringProperty> Metadata;
};

/**
 * One exact expression-input connection.
 *
 * Indexed by default, because a stock expression's input names come from its C++ class and nothing
 * makes them unique — `Custom` lets an author type whatever they like. A MATERIAL FUNCTION's do not
 * work that way: `UMaterialExpressionFunctionInput::ValidateName` exists specifically to "prevent
 * duplicate inputs" (`MaterialExpressionFunctionInput.h:139`), so within one function interface the
 * name IS a key — and a better one than the index, which `SortPriority` lets an author reorder.
 * A call site therefore keys by name and sets bKeyedByName; the index is resolved against the live
 * function at apply time.
 */
struct MATERIALIR_API FMaterialIRConnection
{
	int32 InputIndex = INDEX_NONE;
	FString InputName;
	/** The payload keyed this argument by NAME; InputIndex is unknown until the callee is loaded. */
	bool bKeyedByName = false;
	FString SourceAlias;
	int32 OutputIndex = INDEX_NONE;
	bool bMasked = false;
	bool bMaskR = false;
	bool bMaskG = false;
	bool bMaskB = false;
	bool bMaskA = false;
};

struct MATERIALIR_API FMaterialIRExpression
{
	FString Alias;

	/**
	 * The expression class, as the SHORT token a statement leads with — `Floor`, `AppendVector`.
	 *
	 * Not a path, which is why it is not called one. Resolve it with
	 * MaterialIR::LookupExpressionClass and compare RESOLVED CLASSES, never this string against
	 * a `GetPathName()`: an ambiguous name falls back to the full path, so the same class can
	 * legitimately reach here spelled either way.
	 */
	FString ClassToken;

	/**
	 * For a MaterialFunctionCall, the called function's object path, written in the TYPE position -
	 * `n66 = MaterialFunctionCall /Fab/MaterialFunctions/QMF_Foo.QMF_Foo(Saturation=n62.out0)`.
	 *
	 * The same shape BlueprintIR has always used for a call site (`CallFunction <path>:<Name>`), and
	 * the reason the callee is not an ordinary property: the inputs a statement may name depend on
	 * it, so it has to be readable before the arguments are.
	 */
	FString FunctionPath;

	/** Immediate Composite owner, or empty for the root Material graph. */
	FString SubgraphAlias;
	TArray<FAuthoringProperty> Properties;
	TArray<FMaterialIRConnection> Inputs;
};

/** A synthetic MaterialResult input; Material Functions use their real output expressions. */
struct MATERIALIR_API FMaterialIRResultBinding
{
	FString Name;
	FString SourceAlias;
	int32 OutputIndex = INDEX_NONE;
	bool bMasked = false;
	bool bMaskR = false;
	bool bMaskG = false;
	bool bMaskB = false;
	bool bMaskA = false;
};

/** One addressable Material Editor comment box. Geometry is expressed semantically by Covers. */
struct MATERIALIR_API FMaterialIRComment
{
	FString Alias;
	FString Text;
	/** Immediate Composite owner, or empty for a root-graph comment. */
	FString SubgraphAlias;
	TArray<FString> Covers;
	FLinearColor Color = FLinearColor::White;
	int32 FontSize = 18;
	bool bShowBubble = false;
	bool bColorBubble = false;
	bool bGroupMode = true;
};

struct MATERIALIR_API FMaterialIRDocument
{
	int32 Version = MATERIALIR_VERSION;
	FString AssetPath;
	FString ParentPath;

	/** Reserved now so Material, Function, and Instance documents share one block contract. */
	TArray<FAuthoringProperty> Settings;
	TArray<FMaterialIRParameter> Parameters;
	TArray<FMaterialIRExpression> Expressions;
	TArray<FMaterialIRResultBinding> Results;
	TArray<FMaterialIRComment> Comments;
	TArray<FAuthoringDiagnostic> Diagnostics;
};

/**
 * The one ordering rule for parameters, used by both the reader and the serializer.
 *
 * It existed twice - once in each - as byte-identical copies. That is worse than ordinary
 * duplication, because this order is what makes a projection byte-stable: a rule changed in one
 * copy and not the other would desynchronize read order from write order, and the only symptom
 * would be a sidecar that no longer matches itself. One definition cannot drift from itself.
 *
 * Name alone is ambiguous for layered materials, so association and index sort first.
 */
inline bool MaterialIRParameterLess(const FMaterialIRParameter& A, const FMaterialIRParameter& B)
{
	if (A.Association != B.Association)
	{
		return static_cast<uint8>(A.Association) < static_cast<uint8>(B.Association);
	}
	if (A.Index != B.Index)
	{
		return A.Index < B.Index;
	}
	if (!A.Name.Equals(B.Name, ESearchCase::CaseSensitive))
	{
		return A.Name < B.Name;
	}
	return static_cast<uint8>(A.Type) < static_cast<uint8>(B.Type);
}
