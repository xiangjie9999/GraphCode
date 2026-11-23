#pragma once

#include "CoreMinimal.h"
#include "MaterialIRTypes.h"

namespace MaterialIR
{
	MATERIALIR_API const TCHAR* ParameterTypeToken(EMaterialIRParameterType Type);
	MATERIALIR_API const TCHAR* ParameterAssociationToken(EMaterialIRParameterAssociation Association);

	/** Token -> enum, the exact inverse of the two functions above. False for an unknown token. */
	MATERIALIR_API bool TryParseParameterType(const FString& Token, EMaterialIRParameterType& OutType);
	MATERIALIR_API bool TryParseParameterAssociation(
		const FString& Token, EMaterialIRParameterAssociation& OutAssociation);

	/** Canonical named blocks, ready for FGraphCodeDocument::Blocks. */
	MATERIALIR_API FString SettingsBlock(const FMaterialIRDocument& Document);
	MATERIALIR_API FString ParametersBlock(const FMaterialIRDocument& Document);
	MATERIALIR_API FString GraphBlock(const FMaterialIRDocument& Document);
	MATERIALIR_API FString CommentsBlock(const FMaterialIRDocument& Document);

	/** Parse the canonical reflected `Property.Path = Value` settings body. */
	MATERIALIR_API bool SettingsFromBody(
		const FString& Body, TArray<FAuthoringProperty>& OutSettings, FString& OutError);

	/**
	 * Read a `parameters { }` body back into parameters - the exact inverse of ParametersBlock.
	 *
	 * It lives beside the renderer for the same reason MaterialIRParameterLess does: a format has
	 * to be read and written by one file or the two drift, and the only symptom of that drift is a
	 * sidecar that no longer round-trips. `Body` is the block's INNER text, without the
	 * `parameters {` header and closing brace - which is what FGraphCodeSidecarReader hands over.
	 *
	 * Order is not preserved and is not meant to be: ParametersBlock sorts by
	 * MaterialIRParameterLess before rendering, so the file's order is derived, never authored.
	 * Identity is (Name, Association, Index) - the same three fields Unreal uses, and the same
	 * three the sort keys on.
	 */
	MATERIALIR_API bool ParametersFromBody(
		const FString& Body, TArray<FMaterialIRParameter>& OutParameters, FString& OutError);

	/**
	 * Read a base Material `graph { }` body back into expressions and MaterialResult bindings.
	 *
	 * This is the exact inverse of GraphBlock for the material form. It resolves only textual
	 * structure: class loading, reflected property validation, pin bounds and asset mutation stay
	 * in the editor writer, where the target UMaterial and its expression classes are available.
	 */
	MATERIALIR_API bool GraphFromBody(
		const FString& Body, FMaterialIRDocument& OutDocument, FString& OutError);

	/** Exact inverse of CommentsBlock for a `comments { }` body. */
	MATERIALIR_API bool CommentsFromBody(
		const FString& Body, TArray<FMaterialIRComment>& OutComments, FString& OutError);
}
