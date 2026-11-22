// Blueprint IR — the `timelines { }` write path.
//
// Separate from BlueprintIRApply because a timeline is not a graph. `bp_apply` rebuilds or
// diffs NODES inside one UEdGraph; this writes the UTimelineTemplate objects hanging off the
// Blueprint, which is where a Timeline's curves, length and flags actually live. The two meet
// only at the name: UK2Node_Timeline::TimelineName is how a node finds its template.
//
// THE TRAP THIS FILE EXISTS INSIDE. UK2Node_Timeline::DestroyNode calls
// FBlueprintEditorUtils::RemoveTimeline and renames the template into the transient package
// (K2Node_Timeline.cpp:204-218). So destroying a Timeline node destroys its curves, and that
// is why ApplyGraph refuses any write that would destroy one. Nothing here changes that; this
// file is the other direction — the template as the thing being edited.
//
// What it is given, and why it is given both sides:
//
//   Current  — the `timelines` block as the asset projects RIGHT NOW
//   Desired  — the same block as the file says it should be
//
// Comparing text to text rather than text to objects is what makes a curve-key diff a string
// comparison instead of a second export. gc_apply syncs before it calls, so Current is this
// build's projection of this asset; the plan pass still checks that every name in Current is
// really on the Blueprint, because a caller that skipped the sync would otherwise plan against
// a fiction.
//
// The three identity operations map one-to-one onto engine calls, and the middle one is the
// one that matters:
//
//   a name Desired adds     -> FBlueprintEditorUtils::AddNewTimeline
//   a name Desired drops    -> FBlueprintEditorUtils::RemoveTimeline
//   a stated `rename A -> B` -> FBlueprintEditorUtils::RenameTimeline
//
// THE THIRD ONE USED TO BE "leftovers, paired", BY LINE ORDER, and that is what changed on
// 2026-08-09 (GC-OMISSION-CONTRACT-DESIGN.md sec 6, step 4). The block is a set, so nothing told a
// caller that the order of its lines decided which template kept which identity; renaming two
// timelines in one payload while reordering them crossed the pairing silently. A rename is stated
// now. The one inference kept is exactly one vanished name against exactly one arrived one, which
// cannot be silently wrong because it declines the moment either side has a second candidate.
//
// A RENAME IS NEVER LOWERED TO REMOVE+ADD. For a variable the reason is the GUID; here it is
// sharper — every UK2Node_Timeline finds its template by NAME, so a remove+add mints a fresh
// template and leaves the node pointing at nothing. RenameTimeline is the identity-preserving
// path: it moves the node's TimelineName, repairs the component get/set nodes, and
// UpdateCachedNames re-derives every track's backing property name.
//
// DERIVED STATE, READ-ONLY. A track's `property=` / `function=` is not authored. The engine
// computes it from the timeline's variable name, the track's name and the timeline's GUID
// (FTTPropertyTrack::SetTrackName, TimelineTemplate.cpp:461-468), and recomputes it whenever
// either name moves. So this writer never writes it — it compares it, and refuses text that
// changes it on a track that is neither new nor renamed. Ignoring it silently is the bug
// `input{}`/`output{}` was just rescued from: an edit that applies cleanly and changes nothing.

#pragma once

#include "CoreMinimal.h"
#include "AuthoringCoreTypes.h"
#include "BlueprintIRTypes.h"

class UBlueprint;

struct BLUEPRINTIR_API FBpTimelineApplyOptions
{
	/** Validate and report what would happen without mutating anything. */
	bool bDryRun = false;

	/**
	 * Permit the edits that lose something no rename can carry.
	 *
	 * Gated on the loss actually existing, never on the operation's name: removing a timeline
	 * no node references breaks nothing, and renaming one is repaired by the engine. What is
	 * gated is a removal whose node survives, and a track removal or rename whose pin has
	 * wires - because a pin has no GUID and nothing can re-find it.
	 */
	bool bAllowBreakingChanges = false;

	/** Compile after applying and report the results log. */
	bool bCompile = true;

	/**
	 * `rename <Old> -> <New>` at the block's own depth: which TIMELINE is which.
	 *
	 * Carried in rather than parsed here because this function is handed two parsed
	 * `FBpIrTimeline` arrays and never sees the text. The token is split off at the `gc_apply` seam
	 * where the block is unwrapped, which is also the only place that knows the block's name.
	 */
	TArray<AuthoringCore::FAuthoringRename> Renames;

	/**
	 * The same token one level in, keyed by the timeline whose braces it sat in: which TRACK is
	 * which. A track name is unique only inside its timeline, so this cannot be one flat list.
	 *
	 * Keyed by the timeline's name AS THE PAYLOAD SPELLS IT - after any timeline rename, since that
	 * is the name the entry is written under.
	 */
	TMap<FString, TArray<AuthoringCore::FAuthoringRename>> TrackRenames;

	/**
	 * **WHAT LEAVING A TIMELINE OUT OF THE PAYLOAD MEANS.** C23 §3, 2026-08-10.
	 *
	 * False - the default, and `gc_apply`'s default mode - is PATCH: a current timeline the payload
	 * does not name is left exactly alone, and removing one is said out loud with `Deletes`. True is
	 * SNAPSHOT: the payload is the whole set and every timeline missing from it is a removal.
	 *
	 * **THIS EXISTED AS A BEHAVIOUR BEFORE IT EXISTED AS A CHOICE, AND THAT WAS THE DEFECT.** The
	 * block was declared `EGcOmissionPolicy::Preserves` - *"omitting leaves the entry alone"* - and
	 * removed every unnamed timeline anyway. Where the timeline still had a `UK2Node_Timeline` the
	 * breaking gate caught it; a timeline nothing referenced went silently, against a contract that
	 * said nothing would happen. Spelled as a bool for the same reason WidgetIR's is: this plugin
	 * sits below GraphCode and taking its mode enum would invert the dependency, so the lowering
	 * happens at the `gc_apply` seam that knows both vocabularies.
	 */
	bool bOmissionIsDeletion = false;

	/**
	 * `delete <Name>` at the block's own depth: remove this timeline, said rather than implied.
	 *
	 * **PATCH'S ONLY WAY TO REMOVE ONE, and refused under snapshot** where the omissions already are
	 * the removals - the same rule `widgets { }` follows, and for the same reason: a payload that
	 * could say a removal twice could also say it inconsistently.
	 */
	TArray<FString> Deletes;
};

struct BLUEPRINTIR_API FBpTimelineApplyResult
{
	bool bSuccess = false;

	/** Human-readable summary. */
	FString Message;

	/** One line per operation the plan holds, in the order it would run. */
	TArray<FString> Changes;

	/** Why the plan was refused. Non-empty means nothing was touched. */
	TArray<FString> Refusals;

	/** A step that failed during execution, after the plan had passed. */
	TArray<FString> Errors;

	/**
	 * What an edit costs that survives it: a node whose template is being removed, a wired pin
	 * whose track is being removed or renamed.
	 *
	 * Reported on refusals and dry runs too, for the same reason a removed declaration is:
	 * once the apply has happened it cannot be recomputed, and this is the only record of what
	 * the caller has to rebuild.
	 */
	TArray<FString> OrphanedReferences;

	TArray<FString> CompileErrors;
	TArray<FString> CompileWarnings;

	/** Whether the asset has unsaved changes as this call returns. Nothing here ever saves. */
	bool bDirty = false;

	int32 TimelinesCreated = 0;
	int32 TimelinesRemoved = 0;
	int32 TimelinesRenamed = 0;
	int32 TimelinesModified = 0;
	int32 TracksCreated = 0;
	int32 TracksRemoved = 0;
	int32 TracksRenamed = 0;
	int32 TracksModified = 0;
};

namespace BlueprintIR
{
	/**
	 * Apply a whole `timelines { }` block: creates, removals, renames, field and key edits.
	 *
	 * Plans the entire batch and validates it end to end before touching anything, following
	 * update_variables. There is no rollback and this file does not pretend otherwise —
	 * FScopedTransaction::Cancel discards the undo record rather than applying it — so every
	 * failure the plan can detect is detected there, and a failure during execution reports
	 * which steps had already run rather than claiming the asset is untouched.
	 *
	 * Nothing here saves. The asset is left dirty and the result says so.
	 */
	BLUEPRINTIR_API FBpTimelineApplyResult ApplyTimelines(
		const FString& BlueprintPath,
		const TArray<FBpIrTimeline>& Current,
		const TArray<FBpIrTimeline>& Desired,
		const FBpTimelineApplyOptions& Options);
}
