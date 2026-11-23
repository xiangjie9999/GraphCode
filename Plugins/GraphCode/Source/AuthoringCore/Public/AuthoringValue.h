#pragma once

#include "CoreMinimal.h"

/**
 * Canonical spelling for one reflected leaf value.
 *
 * The spellings are BlueprintIR's, deliberately unchanged: `text("Label", ns="…", key="…")`,
 * `obj'/Game/…'`, `soft'/Game/…'`, `null`, `true`, a backtick-quoted enum entry, and engine export
 * text for everything else. One project, one value grammar - a reader should not have to know
 * which IR produced a line to know what it means.
 *
 * A FLOAT IS SPELLED HERE TOO, and it was the last value that was not. Engine export text is
 * `LexToString`, which is `%f`, which is six decimals always - so `0.5` arrived as `0.500000` and
 * `1450` as `1450.000000`. The digest argument for passing scalars through was right about the
 * shape of the rule and wrong about which spelling was canonical: there was already a second one,
 * because a pin default and a variable default are STORED strings and read back as `0.5`. One
 * value had two spellings in one file (`GC-GRAMMAR-REFERENCE.md` §5). FloatToText is now the
 * single one, and it is the shorter of the two.
 */
namespace AuthoringCore
{
	/**
	 * Which fields belong inside an owned instanced-object literal.
	 *
	 * The containing facet still decides its structural exclusions. Passing that same predicate
	 * through the value grammar is what stops a nested object from quietly re-introducing a field
	 * the outer block owns as a relation. Transient/deprecated/delegate fields remain excluded by
	 * the value codec itself at every object boundary.
	 */
	struct FInstancedValuePolicy
	{
		TFunction<bool(const FProperty*)> IsExcludedField;
		bool bCarryDuplicateTransient = false;
		int32 MaxDepth = 8;

		/**
		 * True for an object this DOCUMENT declares somewhere else, so the value here is a
		 * REFERENCE rather than a body. Unbound inlines exactly as before.
		 *
		 * **THE SAME ARGUMENT `FGraphCodeActorEntryOptions::SplineReference` MAKES, one level
		 * down.** A curve is authored state that has to land somewhere, so it moves to a named
		 * block and the component keeps a name; an owned sub-object is authored state with the
		 * same problem and one more on top of it — it can be reached from TWO entries. A
		 * `BP_Surface` outered to `BP_Wall_C_5` is inlined by the wall and referenced by the
		 * `BP_ItemManager` beside it, and inlining at every site would write one object's fields
		 * as many times as something points at it ([[two-carriers-of-one-fact-is-a-race]]).
		 *
		 * **A DELEGATE RATHER THAN A BOOL, because only the caller knows whether its document HAS
		 * a block to point at.** A level does — `objects { }`. An Actor Blueprint's
		 * `components { }`, a `.gcp` capture and a `kind:external_actor` sidecar do not, and
		 * hoisting there would write a reference to something nothing describes, which is the
		 * defect this exists to remove rather than to move.
		 *
		 * The spelling it falls back to is the ordinary object path, byte for byte what
		 * `ExportValueImpl` writes for a plain reference — so nothing new has to be taught to the
		 * IMPORT side, which already resolves that spelling and refuses an unresolvable one by
		 * name.
		 */
		TFunction<bool(const UObject*)> IsDeclaredElsewhere;
	};

	/** Whether Property contains an owned UObject reference at any container/struct depth. */
	AUTHORINGCORE_API bool IsInstancedSubobjectValue(const FProperty* Property);

	/**
	 * The EXPORT-side question, and it is not the one above.
	 *
	 * `IsInstancedSubobjectValue` asks what the property was DECLARED as — `UPROPERTY(Instanced)`,
	 * or a `DefaultToInstanced` class. This asks what the value ACTUALLY IS: an object outered
	 * strictly below `Owner`, which is ownership in fact whatever the declaration says. True when
	 * either holds.
	 *
	 * The two differ on exactly the case this exists for. `BP_SelectionArea.Properties` is a plain
	 * `BP_Properties_C*` that the actor genuinely owns, so the declared question answers no and a
	 * level document wrote `obj'…PersistentLevel.BP_SelectionArea_C_37.BP_DoorProperties_C_0'` — a
	 * path to an object nothing then described. Measured on `L_OraStudio.gc`: 80 such references
	 * naming 27 objects the document never declared.
	 *
	 * **NOT SYMMETRIC WITH THE IMPORT SIDE, deliberately.** An import has the TEXT and not yet a
	 * value, so it cannot ask this; it dispatches on the spelling instead
	 * ([[IsInstancedSubobjectText]]). Routing both sides through one property-only predicate is what
	 * made widening the export alone silently unreadable.
	 */
	AUTHORINGCORE_API bool IsOwnedSubobjectValue(const FProperty* Property, const void* ValuePtr,
		const UObject* Owner);

	/**
	 * The IMPORT-side counterpart: does this TEXT carry the owned-subobject spelling?
	 *
	 * The text is self-describing — `instanced(…)` and `instanced_ref(…)` are the two forms the
	 * exporter emits — so an import can route on it without knowing how the property was declared.
	 * That is what lets a value the exporter chose to write in full be read back in full.
	 */
	AUTHORINGCORE_API bool IsInstancedSubobjectText(const FString& Text);

	/**
	 * Canonical value spelling for an owned subobject graph.
	 *
	 * Direct objects are `instanced(ClassPath,RelativeIdentity,(Field=Value,...))`; containers and
	 * structs keep the ordinary value grammar around those leaves. Relative identity is rooted at
	 * Owner, so a read never bakes a package or scratch-object name into authored text.
	 */
	AUTHORINGCORE_API bool ExportInstancedSubobjectValue(const FProperty* Property,
		const void* ValuePtr, const UObject* Owner, const FInstancedValuePolicy& Policy,
		FString& OutValue, FString& OutUnsupported);

	/** Inverse of ExportInstancedSubobjectValue; creates/reuses objects owned below Owner. */
	AUTHORINGCORE_API bool ImportInstancedSubobjectValue(const FProperty* Property,
		const FString& Text, void* ValuePtr, UObject* Owner, const FInstancedValuePolicy& Policy,
		FString& OutError);

	/**
	 * Canonical spelling of the value at ValuePtr. Returns false and fills OutUnsupported when the
	 * value has no spelling this grammar can carry, which is a refusal, never a silent skip.
	 *
	 * Owner is the object the value lives in; it resolves relative object references during export.
	 */
	AUTHORINGCORE_API bool ExportValue(const FProperty* Property, const void* ValuePtr,
		const UObject* Owner, FString& OutValue, FString& OutUnsupported);

	/**
	 * Whether engine export text for this value survives export -> import -> export byte-identically
	 * on a disposable instance of the property's own type.
	 *
	 * This is the admission test for the untyped fallback: a value that fails it cannot be written
	 * back, so a document containing it is unsupported rather than optimistically accepted. It
	 * constructs and destroys one scratch value and touches nothing else, but it can resolve asset
	 * references, so callers run it at write admission rather than on every read.
	 */
	AUTHORINGCORE_API bool IsValueRoundTrippable(const FProperty* Property, const void* ValuePtr,
		const UObject* Owner, FString& OutReason);

	/**
	 * `TArray<FVector2D>` — the type a caller has to be able to spell, in full.
	 *
	 * **`GetCPPType()` WITH NO EXTENDED TEXT DROPS THE ELEMENT TYPE OF EVERY CONTAINER**: the
	 * engine returns `TArray` and hands `<FVector2D>` back through the out parameter this passes.
	 * Measured 2026-09-05 — `gc_invoke`'s `callables` list advertised `TArray Points` and
	 * `TMap MaterialSlots`, which names nothing a caller could write a value for, and every refusal
	 * about such a value quoted the same half-type back.
	 */
	AUTHORINGCORE_API FString DescribeType(const FProperty* Property);

	/**
	 * Inverse of ExportValue: write the canonical spelling in Text into the value at ValuePtr.
	 *
	 * The sugar forms are lowered back to the engine's own export text and handed to ImportText
	 * rather than reconstructed by hand, so the engine's parser stays the single authority on what
	 * a value means and there is no second implementation to disagree with it.
	 */
	AUTHORINGCORE_API bool ImportValue(const FProperty* Property, const FString& Text,
		void* ValuePtr, UObject* Owner, FString& OutError);

	/**
	 * Whether two values of one property mean the same thing TO THIS GRAMMAR - which is not the
	 * same question as whether they are the same number.
	 *
	 * A leaf is written because it deviates from its baseline, and it is filtered on the next read
	 * because it no longer does. Those were two different tests: deviation was `Identical` on the
	 * live values, and survival was whether the text could carry the difference. A deviation
	 * smaller than the spelling could print therefore produced a line that appeared and vanished on
	 * alternate reads, and `gc_apply` refused the block because the read-back did not match the
	 * file it was given (`GC-CODEC-HANDOFF.md` §4b). Comparing what will actually be WRITTEN makes
	 * "gets written" and "survives a round trip" one predicate, by construction.
	 *
	 * `Identical` is still asked first and still answers for every value that agrees, so the second
	 * rendering only ever costs the leaves that were going to be rendered anyway.
	 */
	AUTHORINGCORE_API bool ValuesAgreeAsText(const FProperty* Property, const void* A, const void* B,
		const UObject* Owner);

	/**
	 * The canonical spelling of one floating-point number, and the ONLY one this project writes.
	 *
	 * Shortest wins, but never at the cost of the value: the renderer walks from one significant
	 * digit up to nine and stops at the first spelling that reads back as the same number through
	 * the very function the importer uses. So a value a person typed spells itself - `0.5`, `1.0`,
	 * `1450.0` - and a value carrying real IEEE noise spells the noise, because at that point the
	 * digits are the number. Nothing is ever padded: `1.0` cannot become `1.0000001`, since the
	 * ladder stops the moment `1.0` round-trips.
	 *
	 * NINE significant digits is the cap, and it is exactly the guarantee for a `float` - so a
	 * single-precision value is always spelled exactly. A `double` is not: it needs seventeen, and
	 * the eight we decline are worth less than the diff churn they would cost. That loss is real
	 * and is what ValuesAgreeAsText exists to keep consistent.
	 *
	 * FIXED NOTATION ONLY, deliberately, and this is the one place the engine constrains us: its
	 * own float parser advances over `+ - . 0-9` and nothing else (PropertyNumeric.cpp:143), so
	 * text carrying an `e` stops the scan mid-token wherever the ENGINE is the one reading - a pin
	 * default, a struct this codec hands over whole. Fixed reaches 1e-17 with the decimal cap,
	 * which is below anything the corpus holds and below anything a centimetre can mean, and the
	 * one value in 9,483 that scientific would have shortened is FLT_MAX.
	 *
	 * @param bSinglePrecision  Whether the property is a `float`. The round-trip is proved at the
	 *                          property's OWN width, or a float's 0.1 would be spelled with a
	 *                          double's seventeen digits of tail.
	 */
	AUTHORINGCORE_API FString FloatToText(double Value, bool bSinglePrecision);

	/**
	 * Trailing zeros off a rendered number, and never the last one.
	 *
	 * `100.0` rather than `100` is not indecision about efficiency. A declaration default and a pin
	 * literal are STORED strings that already read `100.0`, and the point of spelling floats in one
	 * place is that those two agree; dropping the fractional digit would have swapped one
	 * disagreement for another. It is also what `FString::SanitizeFloat` does by default, for the
	 * same reason - a real that reads as an integer is the one confusion a typed grammar can still
	 * cause a person, and this project has already paid for it once on the way IN
	 * (`BlueprintIRSerialize.cpp`: a pin default of 100 must not reach the graph as `100.000000`).
	 *
	 * Exported because a `.gcp` capture rounds to its own stated precision first and so cannot use
	 * FloatToText - but the tail it prints is the same grammar, and one grammar means one function.
	 */
	AUTHORINGCORE_API void TrimTrailingZeros(FString& Text);

	/**
	 * One struct rendered inline as `(Name=value,…)` without a property to hang it on.
	 *
	 * Exported for the caller that has a `UScriptStruct` and a pointer and no `FProperty` at all -
	 * a timeline's curve keys are an array the reader walks itself. Delta is what an omitted field
	 * means, and it is the caller's to choose: the engine's own rule is a default-constructed
	 * instance, and a caller wanting every field emitted passes the value itself.
	 */
	AUTHORINGCORE_API bool ExportStructValue(const UScriptStruct* Struct, const void* ValuePtr,
		const void* Delta, const UObject* Owner, FString& OutValue, FString& OutUnsupported);

	/**
	 * What this text MEANS, spelled the way a read-back would spell it.
	 *
	 * A payload's text and a projection's text are written by different hands, so comparing them
	 * as strings asks the wrong question. `0.500000` and `0.5` are one value; a sidecar committed
	 * before the float grammar landed is full of the first spelling and every read-back now writes
	 * the second. Import into a scratch instance and export again, and both sides reduce to the
	 * one spelling the emitter would have produced.
	 *
	 * Note that this IMPORTS, so an object reference in the text is resolved - and resolving loads
	 * the asset. Callers reach it only when two strings already differ.
	 */
	AUTHORINGCORE_API bool CanonicaliseValue(const FProperty* Property, const FString& Text,
		UObject* Owner, FString& OutCanonical, FString& OutError);

	/** A value the grammar carries verbatim because no codec owns its type. */
	AUTHORINGCORE_API FString RawValue(const FString& TypePath, const FString& ExportText);

	/**
	 * The spelling ExportValue gives one enum entry.
	 *
	 * Exported because a describe surface has to offer the options in the exact spelling a write
	 * will accept. Two implementations of this rule would eventually disagree, and the disagreement
	 * would look like a model inventing an entry name.
	 */
	AUTHORINGCORE_API FString EnumEntrySpelling(const UEnum* Enum, int64 Value);

	/**
	 * The name the AUTHOR gave one entry, when that differs from the one the engine stores.
	 *
	 * Empty for a native enum (the two are the same), for a value no entry names, and for a display
	 * name two entries share - see the implementation for why an ambiguous one is not resolved.
	 * This is the pair that makes `Type = Doors` readable where `Type = NewEnumerator4` was not.
	 */
	AUTHORINGCORE_API FString EnumAuthoredName(const UEnum* Enum, int64 Value);
	AUTHORINGCORE_API bool EnumValueByAuthoredName(const UEnum* Enum, const FString& Name,
		int64& OutValue);

	/**
	 * The spelling a `.gc` gives one struct member, and the enum pair above is its precedent.
	 *
	 * A Blueprint struct stores `BaseYield` as `BaseYield_2_9F3A…`; a native one stores what the
	 * header says. `GetAuthoredName` is the engine's own answer to that difference - it routes
	 * through `UStruct::GetAuthoredNameForField`, which UUserDefinedStruct overrides and everything
	 * else leaves returning `GetName()` - so this is one call rather than a mangling rule of ours.
	 *
	 * The mapping is 1:1 in both directions: `FStructureEditorUtils::IsUniqueVariableFriendlyName`
	 * enforces unique friendly names per struct, so no two members can answer to one spelling.
	 */
	AUTHORINGCORE_API FString FieldSpelling(const FProperty* Property);

	/**
	 * The inverse, and it accepts BOTH spellings on purpose.
	 *
	 * The stored name is tried FIRST and matched exactly, which is what keeps every `.gc` written
	 * before this rule applying unchanged: those files say `BaseYield_2_9F3A…` and mean it. Only a
	 * name no property stores falls through to the authored walk.
	 */
	AUTHORINGCORE_API FProperty* FindFieldBySpelling(const UStruct* Struct, const FString& Name);
}
