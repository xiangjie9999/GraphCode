#pragma once

#include "CoreMinimal.h"

class UUserDefinedEnum;
class UUserDefinedStruct;

struct BLUEPRINTIR_API FSchemaSettings
{
	FString Description;

	/**
	 * The translation identity behind `Description`, carried because nothing derives it.
	 *
	 * `UUserDefinedEnum::EnumDescription` is an `FText`, and `FTextHistory_Base::Serialize` gives
	 * any text saved with an empty key a fresh `FGuid::NewGuid()` (TextHistory.cpp:872). So a
	 * description written back without its key lands on different bytes at every save: the package
	 * is rewritten for no authored change, and whatever translation pointed at the old key is
	 * orphaned. A key is an identity rather than a value - there is no rule that recomputes it -
	 * so the only way it survives a round trip through text is to be in the text. `widgets { }`
	 * carries it for the same reason.
	 *
	 * The namespace is only the part somebody CHOSE. The engine appends the owning package's
	 * localisation id and regenerates it on duplicate, which is what stops a copied asset from
	 * colliding with the original's translation entries; carrying that half would freeze a value
	 * the engine owns and point a hand-copied line at the package it came from.
	 *
	 * Both empty for a struct, whose description is tooltip METADATA and holds no text identity,
	 * and for an enum whose description is empty - which, measured, is every enum in the corpus
	 * but one. This is why the plain `description = "…"` form stays: it is not a legacy spelling.
	 */
	FString DescriptionNamespace;
	FString DescriptionKey;
};

struct BLUEPRINTIR_API FSchemaStructMember
{
	FString Name;
	FString Type;
	FString DefaultValue;
	FString Tooltip;
	bool bEditableOnInstance = true;
	bool bSaveGame = false;
	bool bMultilineText = false;
	bool bShow3dWidget = false;
	TMap<FString, FString> Metadata;
};

struct BLUEPRINTIR_API FSchemaEnumEntry
{
	FString Name;
	int64 Value = 0;
	FString Tooltip;
};

/** Canonical, bridge-neutral schema projection used by GraphCode and future MCP adapters. */
namespace BlueprintIR::Schema
{
	/** Render a user-defined struct as GraphCode `settings` and `properties` blocks. */
	BLUEPRINTIR_API bool StructToBlocks(
		const UUserDefinedStruct* Struct,
		TArray<FString>& OutBlocks,
		FString& OutError);

	/** Render a user-defined enum as GraphCode `settings` and `entries` blocks. */
	BLUEPRINTIR_API bool EnumToBlocks(
		const UUserDefinedEnum* Enum,
		TArray<FString>& OutBlocks,
		FString& OutError);

	/** Parse the body of `settings { }`. Unknown fields are refused, never ignored. */
	BLUEPRINTIR_API bool SettingsFromBody(
		const FString& Body,
		FSchemaSettings& OutSettings,
		FString& OutError);

	/** Parse the body of a user struct's complete `properties { }` block. */
	BLUEPRINTIR_API bool StructPropertiesFromBody(
		const FString& Body,
		TArray<FSchemaStructMember>& OutMembers,
		FString& OutError);

	/** Parse the body of a user enum's complete `entries { }` block. */
	BLUEPRINTIR_API bool EnumEntriesFromBody(
		const FString& Body,
		TArray<FSchemaEnumEntry>& OutEntries,
		FString& OutError);

	/** Legacy flat projection retained only for migration diagnostics. */
	/** Render a user-defined struct without loading or mutating any dependent Blueprint. */
	BLUEPRINTIR_API bool StructToText(const UUserDefinedStruct* Struct, FString& OutText, FString& OutError);

	/** Render a user-defined enum, excluding Unreal's synthetic trailing _MAX entry. */
	BLUEPRINTIR_API bool EnumToText(const UUserDefinedEnum* Enum, FString& OutText, FString& OutError);
}
