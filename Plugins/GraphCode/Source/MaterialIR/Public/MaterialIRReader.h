#pragma once

#include "CoreMinimal.h"
#include "MaterialIRTypes.h"

class UMaterialInstanceConstant;
class UMaterial;
class UMaterialFunction;
class UMaterialExpression;
class UMaterialExpressionComment;
namespace AuthoringCore { struct FPropertyExportPolicy; }

namespace MaterialIR
{
	/**
	 * The class token a `graph { }` statement leads with, e.g. `Floor`, `AppendVector`.
	 *
	 * A projection has a resolver and may use short names; a CAPTURE may not, and that is the
	 * rule this implements (WORLD-GC-DESIGN §10 — the full class path is capture-only, because a
	 * `.gcp` has nothing to resolve against). Every statement in a Material graph used to lead
	 * with `/Script/Engine.MaterialExpression…`, which is 30 characters of the same prefix on
	 * every line of the largest blocks in the corpus.
	 *
	 * Exactly BlueprintIR's op-token rule with a different prefix: strip `MaterialExpression`
	 * the way `LookupNodeClass` strips `K2Node_`, and fall back to the full object path when the
	 * stripped name is ambiguous, so the token always names one class.
	 */
	MATERIALIR_API FString ExpressionClassToken(const UClass* ExpressionClass);

	/**
	 * The expression class a token names, or null. The inverse of ExpressionClassToken.
	 *
	 * A token starting with `/` is a full class object path and bypasses the stripped index
	 * entirely, which is what keeps every class in the build addressable even when two modules
	 * strip to the same name. `/` is the discriminator because a package path always begins with
	 * one and a C++ identifier never can.
	 */
	MATERIALIR_API UClass* LookupExpressionClass(const FString& Token);

	/** The exact reflected-property policy shared by graph projection and differential apply. */
	MATERIALIR_API AuthoringCore::FPropertyExportPolicy ExpressionPropertyPolicy();

	/**
	 * A named reroute usage names the declaration it reads, by the name the author typed.
	 *
	 * Both fields the engine uses for that link - the usage's `DeclarationGuid` and the
	 * declaration's `VariableGuid` - are intra-asset and reconstructible from the link itself, so
	 * the property codec does not own them and the reader states this instead. The value is the
	 * declaration's `Name`; `DeclarationGuidProperty` is the escape for the one case a name cannot
	 * resolve, two declarations sharing a name, where a token would have to pick one silently.
	 *
	 * Both are EXCLUDED from the policy above, so an import must resolve them against the live
	 * graph rather than hand them to ImportDeviations, which refuses an excluded path by design.
	 */
	inline constexpr const TCHAR* DeclarationProperty = TEXT("Declaration");
	inline constexpr const TCHAR* DeclarationGuidProperty = TEXT("DeclarationGuid");

	/** Editable semantic fields carried by a base Material's `settings` block. */
	MATERIALIR_API AuthoringCore::FPropertyExportPolicy MaterialSettingsPropertyPolicy();

	/** Editable semantic fields carried by a Material Function's `settings` block. */
	MATERIALIR_API AuthoringCore::FPropertyExportPolicy MaterialFunctionSettingsPropertyPolicy();

	/** Editable non-parameter fields carried by a Material Instance's `settings` block. */
	MATERIALIR_API AuthoringCore::FPropertyExportPolicy MaterialInstanceSettingsPropertyPolicy();

	/** Atomic synthetic settings path for the ordered Material Layer/Blend stack. */
	inline constexpr const TCHAR* MaterialLayersProperty = TEXT("MaterialLayers");

	/** Append the instance's local Material Layer override, if any, to a settings projection. */
	MATERIALIR_API bool AppendMaterialLayerSetting(
		const UMaterialInstanceConstant* Instance,
		TArray<FAuthoringProperty>& InOutSettings,
		FString& OutError);

	/** Remove cached base-property values whose adjacent override flag is disabled. */
	MATERIALIR_API void PruneInactiveMaterialInstanceSettings(
		const UMaterialInstanceConstant* Instance,
		TArray<FAuthoringProperty>& InOutSettings);

	/** Read enabled Material Instance overrides. The asset is never modified. */
	MATERIALIR_API bool ReadMaterialInstance(
		const UMaterialInstanceConstant* Instance,
		FMaterialIRDocument& OutDocument,
		FString& OutError);

	/** Read a base Material's expression graph without editor layout or compiled state. */
	MATERIALIR_API bool ReadMaterial(
		const UMaterial* Material,
		FMaterialIRDocument& OutDocument,
		FString& OutError,
		TMap<FString, UMaterialExpression*>* OutExpressionsByAlias = nullptr,
		TMap<FString, UMaterialExpressionComment*>* OutCommentsByAlias = nullptr);

	/** Read a Material Function's expression graph. */
	MATERIALIR_API bool ReadMaterialFunction(
		const UMaterialFunction* Function,
		FMaterialIRDocument& OutDocument,
		FString& OutError,
		TMap<FString, UMaterialExpression*>* OutExpressionsByAlias = nullptr,
		TMap<FString, UMaterialExpressionComment*>* OutCommentsByAlias = nullptr);
}
