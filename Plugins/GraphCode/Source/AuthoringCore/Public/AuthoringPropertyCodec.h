#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"

/**
 * Reflected property export, compacted against a baseline and flattened to leaves.
 *
 * The flattening is the point. A UMG widget's authored difference from its class default is
 * usually one number inside a large style struct, and emitting the whole struct because one leaf
 * moved is what makes naive UMG serialisation unreadable. `Font.Size=32` says what changed;
 * `Font=(FontObject=…,TypefaceFontName=…,Size=32,LetterSpacing=…,OutlineSettings=(…))` does not.
 *
 * It also gives a later differential apply the right unit of work: reset and re-import one leaf
 * rather than one whole struct, so an untouched sibling field cannot be collateral damage.
 *
 * The flattening stops where the fields stop meaning anything separately. A struct of two to four
 * plain numbers is one value, and `TintColor.SpecifiedColor=(R=…,G=…,B=…,A=…)` is one line where
 * `.R`, `.G` and `.B` were three that could only ever change together. Decomposing that far also
 * lost the alpha to the baseline filter, so the document showed a colour with a channel missing.
 *
 * There is no list of struct types here. A struct that implements its own export text owns its
 * spelling; a struct that is only numbers is its value; every other struct is decomposed.
 * Reflection decides. Emission is the only thing affected: ResolvePropertyPath still walks to any
 * depth, so a hand-written `…SpecifiedColor.R=0.5` in an existing document still imports.
 */
namespace AuthoringCore
{
	struct FPropertyExportPolicy
	{
		/**
		 * Consulted for the OUTERMOST fields only. Structural ownership - a parent link, a child
		 * collection - is a property of the owning class, not of a field nested inside a struct,
		 * so a caller cannot accidentally suppress a leaf by naming a common word.
		 */
		TFunction<bool(const FProperty*)> IsExcludedField;

		/** Opt in when the same ownership policy applies inside structs (for example PCG bodies). */
		bool bApplyExclusionToNestedFields = false;

		/** Exact fields whose owner serializes them explicitly despite transient flags. */
		TFunction<bool(const FProperty*)> IsCustomSerializedField;

		/**
		 * Why an excluded field was excluded, for the refusal a payload naming one gets back. A
		 * policy excludes for several unrelated reasons and only it knows which applied; the codec
		 * would otherwise call a Blueprint's own variable "structural or transient state", which is
		 * true of nothing and sends the caller nowhere. Return empty to keep the generic message.
		 */
		TFunction<FString(const FProperty*)> ExplainExclusion;

		/** Emit every managed field, not only deviations. Diagnostic; never the write form. */
		bool bIncludeDefaults = false;

		/**
		 * Carry `CPF_DuplicateTransient` fields instead of dropping them with the transient ones.
		 *
		 * THE FLAG DOES NOT MEAN WHAT THE OTHER FOUR MEAN. `Transient`, `NonPIEDuplicateTransient`
		 * and `TextExportTransient` all say a value is not saved; `Deprecated` says it should not be
		 * read. `DuplicateTransient` says the value IS serialized and only asks not to be copied onto
		 * a duplicate — so it is authored state, and dropping it made a `.gc` unable to state
		 * something a person had typed. A Blueprint's `BlueprintDescription` and
		 * `BlueprintDisplayName` are the visible case.
		 *
		 * Opt-in per policy rather than a change to the shared rule, because the predicate runs at
		 * every depth for every kind: `settings` on six kinds, `class_defaults`, `values` and
		 * `parameters` all share it, and only the block that has measured what its own fields mean
		 * should widen. A policy that leaves this false is byte-identical to before.
		 *
		 * `IsValueStruct` deliberately does NOT consult it — whether a struct spells as one value is
		 * a fact about the struct, and letting a policy change it would change how a colour is
		 * written depending on who asked.
		 */
		bool bCarryDuplicateTransient = false;

		/**
		 * Prove each value's engine text survives export -> import -> export before emitting it.
		 * Off by default: it constructs a scratch value per leaf and can resolve asset references,
		 * so it belongs at write admission rather than on every read.
		 */
		bool bVerifyRoundTrip = false;

		/**
		 * True for an object this DOCUMENT declares in another block, so a leaf holding it writes
		 * a reference instead of the object's body. Unbound is byte-identical to before.
		 *
		 * Forwarded verbatim to `FInstancedValuePolicy::IsDeclaredElsewhere`, which carries the
		 * argument. It lives on the export policy as well because the OWNED branch is entered from
		 * here — `WalkValue` asks `IsOwnedSubobjectValue` and then hands the instanced exporter a
		 * policy built by `InstancedPolicy`, so a predicate bound only on the inner struct would
		 * never reach a caller that builds its policy at this level, which is all of them.
		 */
		TFunction<bool(const UObject*)> IsDeclaredElsewhere;

		/** Struct nesting bound. Exceeding it is a diagnostic, never a silent truncation. */
		int32 MaxDepth = 8;

		/** How many closed-set options a describe lists inline. Ignored by the export path. */
		int32 MaxInlineOptions = 64;
	};

	/**
	 * Export Object's managed authoring state as canonical `Path=Value` pairs in sorted order.
	 *
	 * Baseline must be the archetype that also defines reset - the same object on both sides, or a
	 * later apply resurrects CDO members it never meant to write. A null Baseline is a diagnostic,
	 * not a licence to export everything.
	 */
	AUTHORINGCORE_API void ExportDeviations(const UObject* Object, const UObject* Baseline,
		const FPropertyExportPolicy& Policy, const FString& DiagnosticPath,
		TArray<FAuthoringProperty>& OutProperties, TArray<FAuthoringDiagnostic>& OutDiagnostics);

	/**
	 * The same export over a bare STRUCT instance - memory plus the UScriptStruct describing it.
	 *
	 * A Data Table row is not a UObject: `UDataTable::RowMap` is `TMap<FName, uint8*>`, so the row is
	 * allocated memory and the row struct is the only thing that says what is in it. The object entry
	 * point above cannot reach it, and the walk it runs is already container-based - it took
	 * `Object->GetClass()` and `Object` and never needed either to be an object again below the top
	 * frame. So this is the SAME traversal with the class supplied instead of derived, not a second
	 * exporter: a struct walker of its own would eventually disagree with the object one about which
	 * leaves exist, and the disagreement would surface as a property path a write then refuses.
	 *
	 * `Owner` resolves relative object references during export and may be null; for a row it is the
	 * owning UDataTable. `Baseline` is a default-constructed instance of the same struct, and it must
	 * be, for the same reason the object form insists: emission and reset share one baseline or an
	 * omitted field has no defined value.
	 */
	AUTHORINGCORE_API void ExportStructDeviations(const UStruct* Struct, const void* Container,
		const void* Baseline, const UObject* Owner, const FPropertyExportPolicy& Policy,
		const FString& DiagnosticPath, TArray<FAuthoringProperty>& OutProperties,
		TArray<FAuthoringDiagnostic>& OutDiagnostics);

	/**
	 * **WHAT LEAVING A FIELD OUT OF THE PROPERTY LIST MEANS.** C23 §3, 2026-08-10.
	 *
	 * A property bag's entries ARE its properties, so the block-level question §3 asks of every
	 * writable block - *can you name one to keep it, and name one to remove it?* - lands here, and
	 * the answer had only ever been written one way.
	 *
	 * `bOmissionResets` true is SNAPSHOT and is what this codec has always done: every managed field
	 * returns to `Baseline` and then the payload's deviations are written, so a field the document
	 * does not mention ends at its archetype value rather than at whatever the object held. False is
	 * PATCH: nothing is reset, the payload's fields are written, and every other field is left
	 * exactly alone.
	 *
	 * **THE RESET WAS A BEHAVIOUR BEFORE IT WAS A CHOICE, WHICH IS THE SAME DEFECT `timelines` HAD.**
	 * `settings`, `class_defaults`, `values` and `parameters` were declared
	 * `EGcOmissionPolicy::ResetsToDefault`, whose supported mode is `patch` - and `patch`'s own
	 * definition is *omission preserves*. So the card offered the one mode the codec could not
	 * honour, and a caller who named one setting silently returned every other setting on the asset
	 * to its default.
	 *
	 * Spelled as a plain struct rather than as GraphCode's `EGraphCodeApplyMode` for the reason
	 * BlueprintIR's timeline flag is a bool: AuthoringCore sits BELOW GraphCode and taking that enum
	 * would invert the dependency. The lowering happens at the `gc_apply` seam that knows both.
	 */
	struct FAuthoringImportMode
	{
		/** True is snapshot - reset every managed field first. False is patch - reset nothing. */
		bool bOmissionResets = true;

		/**
		 * Paths the payload asked to return to `Baseline` out loud: patch's `delete <Path>`.
		 *
		 * **PATCH'S ONLY WAY TO UNSET ONE, and refused under snapshot** where the omissions already
		 * are the resets - the rule `timelines { }` and `widgets { }` follow, and for the same
		 * reason: a payload that could say a removal twice could also say it inconsistently.
		 *
		 * A reflected property cannot be deleted, so `delete` here means *delete the deviation* and
		 * ends where omission ends under snapshot - at the archetype value.
		 */
		TArray<FString> Reset;
	};

	/**
	 * The inverse of the export, and the reason omission is unambiguous: under snapshot every
	 * managed field is first reset to Baseline, then the desired deviations are imported, so a field
	 * the document does not mention ends at its archetype value rather than at whatever the object
	 * happened to hold. Under patch nothing is reset and `Mode.Reset` names what should be.
	 *
	 * Baseline must be the same object ExportDeviations compacted against - under patch too, since
	 * that is where an explicit reset takes its value from. Excluded and unmanaged fields are never
	 * touched - structural state belongs to the relations, not to this codec.
	 *
	 * All independent failures are collected; a single bad path does not hide the rest.
	 */
	AUTHORINGCORE_API bool ImportDeviations(UObject* Object, const UObject* Baseline,
		const TArray<FAuthoringProperty>& Properties, const FPropertyExportPolicy& Policy,
		const FAuthoringImportMode& Mode, TArray<FString>& OutErrors);

	/**
	 * The same import in SNAPSHOT, which is what this codec did before there was a choice.
	 *
	 * Not a convenience: it is the right call for every caller whose block's entries are not the
	 * properties themselves. A component template, a socket, a curve's settings and an input element
	 * are all reached through a block that decides omission ONE LEVEL UP - the entry is the
	 * component or the socket, and its property list is a whole value. Only the four property bags
	 * - `settings`, `class_defaults`, `values`, `parameters` - hand the caller's mode down here,
	 * because only there is a property what the payload names.
	 */
	AUTHORINGCORE_API bool ImportDeviations(UObject* Object, const UObject* Baseline,
		const TArray<FAuthoringProperty>& Properties, const FPropertyExportPolicy& Policy,
		TArray<FString>& OutErrors);

	/** ImportDeviations over a bare struct instance. See ExportStructDeviations for why it exists. */
	AUTHORINGCORE_API bool ImportStructDeviations(const UStruct* Struct, void* Container,
		const void* Baseline, UObject* Owner, const TArray<FAuthoringProperty>& Properties,
		const FPropertyExportPolicy& Policy, const FAuthoringImportMode& Mode,
		TArray<FString>& OutErrors);

	/** The struct form in snapshot. See the object overload above for when that is the right call. */
	AUTHORINGCORE_API bool ImportStructDeviations(const UStruct* Struct, void* Container,
		const void* Baseline, UObject* Owner, const TArray<FAuthoringProperty>& Properties,
		const FPropertyExportPolicy& Policy, TArray<FString>& OutErrors);

	/**
	 * Walk a flattened `Outer.Inner` path to the leaf it names. Returns false with a located error
	 * rather than a near match - design invariant 5 forbids guessing which field was meant.
	 */
	AUTHORINGCORE_API bool ResolvePropertyPath(const UStruct* Struct, void* Container,
		const FString& Path, const FProperty*& OutProperty, void*& OutValuePtr, FString& OutError);

	/**
	 * WHY the two lists disagreed - the half `PropertyListsAgree` used to throw away.
	 *
	 * A bare `false` costs whoever reads the refusal a bisect, because the shapes that fail here are
	 * not guessable from the payload: an `instanced(...)` field group is a COMPLETE literal, so
	 * omitting one field of one sub-object refuses while the same entry carrying it applies. Six
	 * payloads were spent finding a missing `bDefaultValue=false` on 2026-08-15
	 * (`Docs/GC-NAVIGATION-DESIGN.md` §7, N3). Naming the path and both renderings makes that one
	 * read.
	 *
	 * The two sides are deliberately NOT called "proposed" and "read-back": callers pass them in
	 * both orders (`GraphCodeMeshWriter` compares Current against Wanted), so `Describe` takes the
	 * labels and each call site names its own sides honestly.
	 */
	struct FAuthoringDisagreement
	{
		enum class EKind : uint8
		{
			None,
			/** The lists are different lengths, so one names a path the other never mentions. */
			CountDiffers,
			/** A path in the left list is absent from the right one. */
			PathMissing,
			/** A path that neither side can walk - a genuine disagreement, never a benefit of the doubt. */
			PathUnresolvable,
			/** Both sides have the path and still mean different things after the codec reduced them. */
			ValueDiffers,
		};

		EKind Kind = EKind::None;
		/** The flattened path that disagreed. Empty only for `CountDiffers`. */
		FString Path;
		/** The left list's rendering, i.e. the `A` argument's. */
		FString Left;
		/** The right list's rendering. Empty when the path is missing from the right list. */
		FString Right;
		/** The resolve error, for `PathUnresolvable` only. */
		FString Detail;
		/** Set for `CountDiffers`, where there is no single path to blame. */
		int32 LeftNum = 0;
		int32 RightNum = 0;

		/**
		 * One sentence ready to drop into a refusal, e.g.
		 * `Keys: stated '...(bDefaultValue=false)...' but the engine read back '...()...'`.
		 *
		 * Long values are elided around the point they first diverge rather than from the front,
		 * because the shape that fails here is a 600-character array literal whose one wrong field
		 * sits in the middle - truncating the head would print two identical prefixes.
		 */
		AUTHORINGCORE_API FString Describe(const TCHAR* LeftLabel, const TCHAR* RightLabel) const;
	};

	/**
	 * Whether two property lists say the same thing - the question every read-back check is really
	 * asking, and not the one string equality answers.
	 *
	 * A write proves itself by projecting the asset again and comparing the result against what the
	 * payload asked for. Those two texts come from different hands: one is the emitter's, the other
	 * is whatever a person, a patch or a five-year-old committed sidecar wrote. So `0.500000`
	 * against a read-back of `0.5` is a payload that changed nothing being refused for changing
	 * everything. Paths are matched as a set and only a differing VALUE is reduced through the
	 * codec, which keeps the cost off every line that already agreed.
	 *
	 * Container is walked to resolve nested paths and is never written to; a class default object
	 * serves as well as a live instance.
	 *
	 * `OutDisagreement` is optional and is only written when the answer is false; it names the FIRST
	 * disagreement in `A`'s order, so the message is stable across runs.
	 */
	AUTHORINGCORE_API bool PropertyListsAgree(const UStruct* Struct, void* Container, UObject* Owner,
		const TArray<FAuthoringProperty>& A, const TArray<FAuthoringProperty>& B,
		FAuthoringDisagreement* OutDisagreement = nullptr);

	/**
	 * One managed leaf, described rather than compacted: what it is called, what it holds, and what
	 * kind of thing may be written into it.
	 */
	struct FAuthoringPropertyDescription
	{
		FString Path;
		FString Type;
		/** The value this object currently holds, in the same spelling a read would emit. */
		FString Value;
		/** The archetype value, so a caller can see what omitting the leaf would restore. */
		FString Default;
		/** True when Value differs from Default and the compact authored form therefore emits it. */
		bool bAuthored = false;
		FString Category;
		FString ToolTip;

		/** "enum", "class" or "object" when the value is chosen from a set; empty otherwise. */
		FString SelectionKind;
		/** The enum, meta class or object class the set is drawn from, by path. */
		FString SelectionType;
		/** Complete for an enum, in the value grammar's own spelling. Empty for the open sets. */
		TArray<FString> Options;

		bool bEditable = true;
		bool bAdvanced = false;
	};

	/**
	 * Enumerate the leaves ExportDeviations would manage, with their types and value sets.
	 *
	 * This runs the SAME traversal as the export, not a parallel one. A second walker would
	 * eventually disagree about which leaves exist, and the disagreement would teach a model a
	 * property path that a write then refuses - the exact failure a describe surface exists to
	 * prevent.
	 */
	AUTHORINGCORE_API void DescribeManagedProperties(const UObject* Object, const UObject* Baseline,
		const FPropertyExportPolicy& Policy, const FString& DiagnosticPath,
		TArray<FAuthoringPropertyDescription>& OutProperties,
		TArray<FAuthoringDiagnostic>& OutDiagnostics);

	/**
	 * **WHAT AN OBJECT HELD BEFORE A WRITE, so the propagation below can ask the engine's question.**
	 *
	 * One entry per path that resolved; a path that does not resolve is simply absent, because the
	 * caller is naming the paths its write is ABOUT to touch and a miss there is the write's problem
	 * to report, not this one's.
	 */
	AUTHORINGCORE_API void CaptureValues(const UObject* Object, const TArray<FString>& Paths,
		TMap<FString, FString>& OutValues);

	/**
	 * **PUSH A CHANGE ALREADY WRITTEN TO `Archetype` OUT TO THE INSTANCES THAT WERE FOLLOWING IT.**
	 *
	 * Writing a Blueprint's component template or CDO moves the ARCHETYPE and nothing else: a placed
	 * actor keeps whatever its own component was serialised with, and a compile does not help,
	 * because reinstancing copies the old instance's values onto the new one. So the value the
	 * author just wrote is invisible in the viewport until the level is reloaded - CP1 G9, measured
	 * repeatedly and filed as a compile-options fault it never was.
	 *
	 * The rule is the editor's own, `FPropertyNode::PropagatePropertyChange`: an instance takes the
	 * new value only where it still holds the PREVIOUS one, so a per-instance override is left
	 * standing. `Previous` is what `CaptureValues` read before the write; a path whose current value
	 * still equals it did not move and is skipped, which is what makes this safe to hand the whole
	 * managed set rather than the payload's own list.
	 *
	 * DIRECT INSTANCES ONLY, and that is narrower than the details panel on purpose. The engine
	 * walks the archetype chain, which reaches another asset's override template and dirties it; a
	 * tool that edited a second asset nobody named would be the worse bug. A subclass keeps
	 * answering to its own template here.
	 *
	 * Nothing is marked dirty: an instance that follows its archetype stores no delta, so the level
	 * is the same file after this runs as before it. Returns how many instances were moved.
	 */
	AUTHORINGCORE_API int32 PropagateToInstances(UObject* Archetype,
		const TMap<FString, FString>& Previous);
}
