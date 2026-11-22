// Blueprint IR - canonical naming and the BP-authored name mangling problem.
//
// ---------------------------------------------------------------------------
// THE TRAP THIS FILE EXISTS TO CONTAIN
// ---------------------------------------------------------------------------
// Blueprint-authored enums and structs do not use the names you see in the editor:
//
//   * UUserDefinedEnum entries are internally NewEnumerator0, NewEnumerator1, ...
//     The labels you typed ("Idle", "Walking") live in a separate display-name map.
//
//   * UUserDefinedStruct member properties are mangled: MemberName_2_<GUID>,
//     with the friendly name carried in metadata.
//
// A model cannot reason about `NewEnumerator3`. So the IR must EMIT display names
// while the resolver maps them back to internal names. That is a genuine
// bidirectional mapping, and it exists ONLY for BP-authored types - native UENUM
// and USTRUCT names are exactly as written.
//
// The uniform escape hatch: UEnum::GetAuthoredNameStringByIndex and
// UStruct::GetAuthoredNameForField are virtual and overridden by the UserDefined
// variants. Going through them means one code path handles both flavours, so we
// never branch on "is this a BP type" at the call site and never get it half-right.
// Do NOT reach into UUserDefinedEnum::DisplayNameMap directly.

#pragma once

#include "CoreMinimal.h"

class UBlueprint;
class UEdGraph;
class UEdGraphPin;
class UEnum;
class UFunction;
class UScriptStruct;
class UStruct;

namespace BlueprintIR::Names
{
	/**
	 * Canonical function identity: "<OwnerScope>:<FunctionName>".
	 *   native: "KismetMathLibrary:MakeVector"
	 *   native, ambiguous bare name: "/Script/Engine.Actor:K2_SetActorLocation"
	 *   BP:     "/Game/BP_Player.BP_Player_C:MyFunc"
	 *
	 * The bare class name is emitted only when it names that class and nothing else in the
	 * build; anything else keeps its full path. `/Script/Engine.` is fourteen characters of
	 * package on every engine call and the package is not identity.
	 */
	BLUEPRINTIR_API FString CanonicalFunction(const UFunction* Function);

	/**
	 * Split a canonical function name back into owner path and function name, expanding a bare
	 * class name to its path. Both spellings are accepted, as is an optional `$` marker.
	 */
	BLUEPRINTIR_API bool ParseCanonicalFunction(const FString& Canonical, FString& OutOwnerPath, FString& OutFunctionName);

	/**
	 * Expand one owner scope token - `KismetMathLibrary`, `$KismetMathLibrary` or a full path -
	 * into a class path. An unresolved or ambiguous token comes back unchanged so the caller's
	 * own error names what the document actually said.
	 */
	BLUEPRINTIR_API FString ResolveOwnerScopeToken(const FString& Token);

	/**
	 * The inverse, for the TEXT form only: `/Script/Engine.KismetMathLibrary:MakeVector` becomes
	 * `KismetMathLibrary:MakeVector` when that bare name is the class's alone.
	 *
	 * Presentation, deliberately not identity. CanonicalFunction keeps the full path because the
	 * search index and the resolver key on it; only the document people read gets shorter, and
	 * ParseCanonicalFunction expands it again on the way back in.
	 */
	BLUEPRINTIR_API FString ShortenScopeToken(const FString& Token);

	/**
	 * The inverse of `ShortenScopeToken`, applied where a statement's target enters the IR from
	 * text or JSON.
	 *
	 * Expanding at the front door is the whole design: inside the IR a target is always the full
	 * path, so the resolver, the search index and the graph signature keep comparing the string
	 * they always compared, and the short spelling never reaches anything that would have to
	 * learn about it. A token that is already a path, or names nothing, is returned unchanged.
	 */
	BLUEPRINTIR_API FString ExpandScopeToken(const FString& Token);

	/**
	 * Canonical variable identity: "<OwnerClassPath>:<VarName>", the same shape as
	 * CanonicalFunction and for the same reason - the resolver scores an exact match on
	 * the tail after the last ':' at the "this candidate IS named the query" tier, so a
	 * bare `Health` (which is what bp_read emits for a variable) resolves without the
	 * caller having to spell the full path.
	 *
	 * Returns empty when the property has no owning class, which is the case for local
	 * variables and sidecar structures. Those are not globally addressable and must not
	 * be given a canonical that implies they are.
	 */
	BLUEPRINTIR_API FString CanonicalVariable(const FProperty* Property);

	/**
	 * `Vector` for `/Script/CoreUObject.Vector`; the path itself for everything else.
	 *
	 * ShortenScopePath's rule, applied to the other place a type is named. A call target was
	 * already written `KismetMathLibrary:MakeVector` while a pin of the same class was written
	 * `object'/Script/Engine.StaticMeshComponent'` on the same line, so one file spelled one
	 * type two ways.
	 *
	 * Classes, structs and enums, because all three appear as a pin's PinSubCategoryObject. Only
	 * a type in a SCRIPT package shortens: a content path IS that asset's identity, and it starts
	 * with `/`, which is the same discriminator that lets a bare class name be told from a path
	 * with no sigil.
	 */
	BLUEPRINTIR_API FString ShortenTypePath(const FString& Path);

	/**
	 * The inverse: a bare type name or a path, resolved to the type it names. Null when the token
	 * names nothing, or names more than one thing.
	 *
	 * Refusing an ambiguous name rather than picking is what lets the writer's round-trip proof
	 * mean something - a token this cannot resolve is a token ShortenTypePath never emits.
	 */
	BLUEPRINTIR_API UObject* ResolveTypeToken(const FString& Token);

	/** Canonical enum path, e.g. "/Script/Engine.ECollisionChannel" or "/Game/E_State.E_State". */
	BLUEPRINTIR_API FString CanonicalEnum(const UEnum* Enum);

	/** Canonical struct path. */
	BLUEPRINTIR_API FString CanonicalStruct(const UScriptStruct* Struct);

	/**
	 * Canonical macro identity: "<MacroLibraryPath>:<GraphName>".
	 * Reached from UK2Node_MacroInstance::GetMacroGraph() -> outer blueprint.
	 */
	BLUEPRINTIR_API FString CanonicalMacro(const UEdGraph* MacroGraph);

	// -----------------------------------------------------------------------
	// Enum entries: display <-> internal
	// -----------------------------------------------------------------------

	/**
	 * Display name for an enum entry by index, e.g. "Idle".
	 * Uses GetAuthoredNameStringByIndex, so it is correct for native and
	 * BP-authored enums alike. Skips the synthetic _MAX entry.
	 */
	BLUEPRINTIR_API FString EnumEntryDisplayName(const UEnum* Enum, int32 Index);

	/**
	 * Internal name for an enum entry by index, e.g. "NewEnumerator3" for a BP
	 * enum or "Idle" for a native one. This is what actually goes on the pin.
	 */
	BLUEPRINTIR_API FString EnumEntryInternalName(const UEnum* Enum, int32 Index);

	/**
	 * Resolve a display name back to the internal name the engine expects.
	 * Returns false when the display name does not match any entry - callers must
	 * treat that as "unresolved", never as "close enough".
	 */
	BLUEPRINTIR_API bool ResolveEnumEntry(const UEnum* Enum, const FString& DisplayName, FString& OutInternalName, int32& OutIndex);

	/**
	 * A DECLARATION's default value, in the authored spelling and back — `= Doors`, never
	 * `= NewEnumerator4`, and `= (Name="Grey")` rather than `= (Name_2_562432FC…="Grey")`.
	 *
	 * A graph LITERAL has been spelled this way for a long time; a declaration's default was not,
	 * so one file said `Doors` in a statement and `NewEnumerator0` in the declaration above it, and
	 * the value line was unreadable: `entries { }` publishes authored name -> ordinal and the
	 * stored names are deliberately unpublished, so nothing in the corpus mapped one to the other.
	 *
	 * DRIVEN BY THE TYPE TEXT, not by a single resolved enum. The predecessor took a `UEnum*` found
	 * by loading the type text when that text WAS a path, so a scalar enum translated and the same
	 * enum inside `TArray<…>` or `TMap<…>` did not - 11 files in this corpus, each holding the
	 * spelling the row exists to remove. A struct's members ride the same walk, which is what
	 * reaches a mangled member inside a container literal.
	 *
	 * Hooked at the TEXT boundary and nowhere else. Every in-memory carrier — a pin default, an
	 * `FBPVariableDescription`, an `FStructVariableDescription` — keeps the stored spelling,
	 * because that is what the engine reads.
	 *
	 * One pair for both declaration renderers (blueprintir/2's `input`/`output`/`local` and
	 * schemacode/1's struct members). Two copies of a translation are two that can disagree, and a
	 * disagreement here reads as a default that silently changed.
	 */
	BLUEPRINTIR_API FString DefaultValueToText(const FString& TypeText, const FString& StoredValue);
	BLUEPRINTIR_API FString DefaultValueFromText(const FString& TypeText, const FString& Text);

	BLUEPRINTIR_API void EnumEntries(const UEnum* Enum, TArray<TPair<FString, FString>>& OutEntries);

	// -----------------------------------------------------------------------
	// Struct members: display <-> internal
	// -----------------------------------------------------------------------

	/**
	 * Authored (friendly) name of a struct member, e.g. "Health" rather than
	 * "Health_2_A1B2C3...". Correct for native and BP-authored structs alike.
	 */
	BLUEPRINTIR_API FString StructMemberDisplayName(const UStruct* Struct, const FProperty* Property);

	/**
	 * Resolve a friendly member name back to the real FProperty. Returns nullptr
	 * when unmatched - callers must not guess.
	 */
	BLUEPRINTIR_API const FProperty* ResolveStructMember(const UStruct* Struct, const FString& DisplayName);

	// -----------------------------------------------------------------------
	// Pin names: the third carrier of the same mangling
	// -----------------------------------------------------------------------

	/**
	 * The name a `.gc` writes for one pin, which is the AUTHORED one wherever the two differ.
	 *
	 * This finishes the job the top of this file describes. A VALUE has been spelled `Doors` since
	 * 2026-07-31 and a DECLARATION's default since the regeneration; the pin naming that value was
	 * still `NewEnumerator0=`, and a struct member's pin still `Vertices_3_5B3B…`. One statement
	 * therefore carried both spellings of one name.
	 *
	 * Two rules, and they are not the same shape:
	 *
	 *  - AN ENUM ENTRY IS A LOOKUP. `NewEnumerator0` does not contain `Doors`, so the enum has to
	 *    be asked. `INodeDependingOnEnumInterface` is the generic door - Select and SwitchEnum both
	 *    implement it - so this never branches on a concrete node class.
	 *  - A STRUCT MEMBER IS A STRIP. `FStructureEditorUtils` builds `Friendly_Index_GUID`, so the
	 *    authored name IS the prefix and no lookup is needed. It is removed wherever it appears
	 *    rather than only at the end, because a split sub-pin carries it in the middle:
	 *    `ChildrenCompartments_Compartments_3_6C99…`.
	 *
	 * AMBIGUITY FALLS BACK TO THE STORED NAME, which is the ruling `EnumAuthoredName` already
	 * makes for colliding display names: if spelling this pin would collide with a name another
	 * pin on the same node stores, the stored name is written instead. Unreadable beats wrong.
	 */
	BLUEPRINTIR_API FString PinSpelling(const UEdGraphPin* Pin);

	/**
	 * The strip on its own, for the one caller that holds a NAME rather than a pin: a split pin's
	 * leaf, which the reader has to take apart before it can spell either half.
	 */
	BLUEPRINTIR_API FString UnmangleMemberName(const FString& Name);

	/** Every member as (display, internal, type) triples. */
	struct FMemberInfo
	{
		FString DisplayName;
		FString InternalName;
		FString TypeString;
	};
	BLUEPRINTIR_API void StructMembers(const UStruct* Struct, TArray<FMemberInfo>& OutMembers);

	/** True when this type is Blueprint-authored, i.e. subject to name mangling. */
	BLUEPRINTIR_API bool IsUserDefined(const UEnum* Enum);
	BLUEPRINTIR_API bool IsUserDefined(const UScriptStruct* Struct);
}
