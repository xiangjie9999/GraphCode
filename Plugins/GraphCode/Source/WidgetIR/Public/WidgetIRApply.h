#pragma once

#include "CoreMinimal.h"
#include "WidgetIRTypes.h"

class UWidget;
class UWidgetBlueprint;
class UWidgetTree;

namespace WidgetIR
{
	/**
	 * Independent statuses, because design section 10.5 forbids inferring what happened from one
	 * boolean. A write can validate, build a scratch copy, mutate, compile, verify and roll back,
	 * and each of those can succeed or fail on its own.
	 */
	enum class EWidgetIRStatus : uint8
	{
		NotRun,
		Passed,
		Failed,
	};

	struct WIDGETIR_API FWidgetIRApplyResult
	{
		EWidgetIRStatus ValidationStatus = EWidgetIRStatus::NotRun;
		EWidgetIRStatus ScratchStatus = EWidgetIRStatus::NotRun;
		EWidgetIRStatus MutationStatus = EWidgetIRStatus::NotRun;
		EWidgetIRStatus CompileStatus = EWidgetIRStatus::NotRun;
		EWidgetIRStatus EquivalenceStatus = EWidgetIRStatus::NotRun;
		EWidgetIRStatus RollbackStatus = EWidgetIRStatus::NotRun;

		bool bAssetDirty = false;
		/**
		 * Always false, and that is still right — but not for the reason this line used to give.
		 *
		 * It said `asset.save_asset` persists, which C24.0 abolished and C24.2 retires. The engine
		 * genuinely does not save: `gc_apply`'s wrapper saves ONCE, after the last block of a
		 * document, so a widget write that saved itself would make a five-block apply five saves.
		 * Design invariant 9 survives; the sentence pointing at a dead operation does not.
		 */
		bool bSaved = false;

		FString OldSignature;
		FString NewSignature;

		/** Widget names by what happened to them, for the diff summary. */
		TArray<FString> ReusedNames;
		TArray<FString> CreatedNames;
		TArray<FString> DeletedNames;
		TArray<FString> MutatedNames;

		/**
		 * Widgets removed because the payload did not name them — `bOmissionIsDeletion` only.
		 * `GC-VCS-MERGE-DESIGN.md` M2.3/M2.5.
		 *
		 * Separate from `DeletedNames`, which is the `delete` list the caller wrote. These are the
		 * ones NOTHING in the document names, so they are exactly the ones re-reading the payload
		 * cannot reveal — and the reason the list is filled on refusals and dry runs too.
		 */
		TArray<FString> OmittedDeletions;

		/** `Old -> New` for every widget the payload renamed by editing its name. */
		TArray<FString> RenamedNames;

		/**
		 * `<Widget>.<Property>` for every designer binding this write removed — by an `unbind` line,
		 * or by omission under `bOmissionIsDeletion`.
		 *
		 * Reported for the reason `OmittedDeletions` is: under snapshot these are exactly the
		 * removals nothing in the payload names, so re-reading what was sent cannot reveal them.
		 */
		TArray<FString> RemovedBindings;

		/** References a rename carried across with it, so nothing repaired is invisible. */
		TArray<FString> RepairedReferences;

		/**
		 * References this write cannot carry: a native BindWidget contract, another asset's
		 * named-slot fill, or anything that resolved a name the payload removed. Populated whether
		 * or not the write proceeded - allow_breaking_changes suppresses the refusal, never the
		 * report.
		 */
		TArray<FString> BrokenReferences;

		TArray<FString> Errors;
		TArray<FString> CompileMessages;
		TArray<FString> Unsupported;

		/**
		 * Authored property hints that produced the requested effective value but disappeared from
		 * the compact readback because that value is the widget or slot archetype default.
		 *
		 * These are accepted, not refused: the input document is a proposal, while ResultDocument is
		 * the canonical projection that will be written back. Reporting each normalization keeps the
		 * compaction visible without asking a caller to predict Unreal's per-class defaults.
		 */
		TArray<FString> CanonicalizationNotices;

		/**
		 * Canonical document read back from the asset after a successful apply — and, after a
		 * scratch refusal, the document the engine built instead.
		 *
		 * Filled on that one failure deliberately. A scratch mismatch is usually a spelling the
		 * reader would never emit rather than a wrong tree, and the correction is a fact about the
		 * engine that the caller has no way to derive. The asset is untouched either way; read
		 * `ScratchStatus` to know which of the two this is.
		 */
		FString ResultDocument;

		bool Succeeded() const
		{
			return ValidationStatus == EWidgetIRStatus::Passed
				&& ScratchStatus == EWidgetIRStatus::Passed
				&& MutationStatus == EWidgetIRStatus::Passed
				&& CompileStatus == EWidgetIRStatus::Passed
				&& EquivalenceStatus == EWidgetIRStatus::Passed;
		}

		/**
		 * A DRY RUN THAT WENT AS FAR AS A DRY RUN GOES.
		 *
		 * `Succeeded()` above is the wrong question to ask of one, and asking it made every passing
		 * dry run on the `gc_apply block:"widgets"` route report a refusal with an EMPTY error list —
		 * a tool declining for no stated reason, which is the worst answer available. A dry run stops
		 * after the scratch copy on purpose: mutation, compile and equivalence are `NotRun`, which is
		 * a statement that they were never attempted and not a statement that they failed.
		 */
		bool SucceededDryRun() const
		{
			return ValidationStatus == EWidgetIRStatus::Passed
				&& ScratchStatus == EWidgetIRStatus::Passed;
		}
	};

	struct WIDGETIR_API FWidgetIRApplyOptions
	{
		/** Validate and build the scratch copy, then stop. The real asset is never touched. */
		bool bDryRun = false;

		/**
		 * The signature the caller last read. Empty skips the check, which is only safe for a
		 * caller that just read the asset in the same breath.
		 */
		FString BaseSignature;

		/**
		 * Proceed even though named references will be left resolving nothing.
		 *
		 * The refusal comes first and lists every one, so this is an answer to a question the
		 * caller has already been asked. It does not cover a graph reference to a name the payload
		 * REMOVED: this write path does not edit graph content, and the asset would not compile, so
		 * that one is reported with its remedy instead of overridden.
		 */
		bool bAllowBreakingChanges = false;

		/**
		 * The payload is the COMPLETE Designer tree, so a widget it does not name is deleted rather
		 * than refused. `GC-VCS-MERGE-DESIGN.md` §4.3, M2.5 — GraphCode's `mode:"snapshot"`.
		 *
		 * A BOOL RATHER THAN GraphCode's ENUM, deliberately. WidgetIR sits below GraphCode and
		 * knows nothing about it; taking `EGraphCodeApplyMode` here would invert the dependency to
		 * spell one bit. The name says what it controls rather than which caller sets it.
		 *
		 * WHAT IT DOES NOT TURN OFF, and this is the whole safety argument: `CheckNameReferences`
		 * already asks its question of `Identity.Removed`, which is the omitted set — so a removed
		 * widget a graph node resolves is a hard break under either spelling, and
		 * `bAllowBreakingChanges` remains the only thing that suppresses that refusal. This flag
		 * answers "did the caller mean to remove it", never "is removing it safe".
		 *
		 * A `delete` line alongside it is REFUSED rather than accepted as redundant: the token says
		 * the payload is partial and this says it is complete, and a caller holding both beliefs is
		 * about to be surprised by one of them.
		 */
		bool bOmissionIsDeletion = false;
	};

	/** Everything a build needs beyond the document, and everything it has to report back. */
	struct WIDGETIR_API FWidgetIRBuildContext
	{
		/**
		 * Desired widget name -> the existing object that keeps its identity.
		 *
		 * The object may currently answer to a different name; that is a rename, and the builder
		 * performs it through the engine's own sequence rather than a raw UObject::Rename.
		 */
		TMap<FString, UWidget*> Reuse;

		/** The widget each name ended up naming. A describe surface reports the objects the engine
		 *  actually produced rather than the ones it hoped for. */
		TMap<FString, UWidget*>* OutBuilt = nullptr;

		TArray<FString> RepairedReferences;
		TArray<FString> RemovedNames;
		TArray<FString> RenamedNames;
	};

	/**
	 * Materialise Document's widget tree into WidgetTree, replacing its contents.
	 *
	 * Slot classes are never named by the payload - the parent creates the slot and its reflected
	 * state is applied to whatever it produced. Anything the tree still owns afterwards is content
	 * the document dropped, and it leaves through the engine's deletion path.
	 */
	WIDGETIR_API bool BuildWidgetTree(const FWidgetIRDocument& Document, UWidgetTree* WidgetTree,
		FWidgetIRBuildContext& Context, TArray<FString>& OutErrors);

	/**
	 * Validate and apply one complete desired document to a Widget Blueprint.
	 *
	 * Nothing is mutated until the same document has been built into a disposable duplicate,
	 * compiled there, read back and found equivalent. On any live failure the asset is restored
	 * from a full snapshot and the restoration is proved by re-reading its original signature.
	 * The asset is left dirty and unsaved.
	 */
	WIDGETIR_API void ApplyDocument(const FString& AssetPath, const FWidgetIRDocument& Desired,
		const FWidgetIRApplyOptions& Options, FWidgetIRApplyResult& OutResult);
}
