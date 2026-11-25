// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeErrors.h"

/**
 * WHAT THE ENGINE SAID WHILE LOADING A PACKAGE, carried into `errors { }` as `LoadError`.
 *
 * **THE GAP THIS FILLS WAS MEASURED, not guessed.** `UW_LightZoneInfo` logged three
 * `FStructProperty::Serialize … Unknown structure` errors on every load of its package, and every
 * instrument GraphCode had said the asset was healthy: the sidecar projected `canonical:true`, the
 * referenced struct read clean on its own, and `gc_apply compile:true` returned 0 errors and 0
 * warnings. `FGcCompileErrors` reads `FCompilerResultsLog` — the COMPILER's listing — and a
 * serialization failure happens during LOAD, before the compiler is involved. It could never appear
 * there. A whole class of defect was invisible to the text surface.
 *
 * **A LEDGER AND NOT A PROBE, because a package loads exactly once per session.** The errors are
 * emitted on first load and never again, no matter how many times the asset is projected afterwards
 * — which is also why they used to land on whichever automation spec happened to run at that moment
 * ([[uncommitted-corpus-fails-restore-specs]]). So nothing can ask *load this and tell me what
 * happened*; the only reliable design is to be listening BEFORE the load, record what arrives, and
 * read the record later. The device is installed at module startup for that reason.
 *
 * **ATTRIBUTION IS BY THE PATH IN THE MESSAGE, and that is a deliberate limit.** A load error names
 * the object it failed on — `/Game/Blueprints/UI/Properties/UW_LightZoneInfo.UW_LightZoneInfo_C:…`
 * — so the package is already in the text. The alternative, tracking whichever package is mid-load,
 * is wrong for the case that matters: loading one asset pulls its dependencies, and their errors
 * would all be charged to whatever the caller asked for. A message with no path in it is DROPPED
 * rather than charged to a guess.
 */
class GRAPHCODE_API FGcLoadErrors
{
public:
	/**
	 * Start listening. Idempotent, and called from the module's startup so it precedes every load
	 * this editor performs — an error emitted before this runs is one nothing can recover.
	 */
	static void Install();

	/** Stop listening and drop the ledger. Called from module shutdown. */
	static void Remove();

	/**
	 * The package a log line is about, or empty when it names none.
	 *
	 * PURE AND PUBLIC BECAUSE IT IS THE PART THAT CAN BE WRONG. Everything else here is plumbing; the
	 * extraction is a claim about the engine's message text, and it is the one thing a spec can hold
	 * to account with no editor running.
	 *
	 * Takes the first `/Mount/Path` token and stops at the object separator: `/Game/UI/W.W_C:Prop`
	 * is the package `/Game/UI/W`. A trailing `'` or `"` from a quoted path is trimmed.
	 */
	static FString PackageFromMessage(const FString& Message);

	/** What was logged against this package this session, deduplicated, in the order seen. */
	static TArray<FGcSidecarError> CollectFor(const FString& PackageName);

	/**
	 * Drop what this session recorded for one package, so the next `RecordFor` states what is true NOW.
	 *
	 * **WITHOUT THIS, NO REPAIR CAN EVER CLEAR ITS OWN NOTE — measured 2026-08-14.** The ledger is
	 * append-only for the life of the process, and the sidecar's `errors { }` is merged from it. So a
	 * session that loads a broken asset, repairs it, and re-projects writes the PRE-REPAIR errors back
	 * into the file: the tool worked, the file says it did not, and the only way to see the truth was
	 * to restart the editor. `EUW_RoomBuilder` carried a stale block through three repair attempts on
	 * exactly this.
	 *
	 * **CALL IT WHEN THE PACKAGE HAS ACTUALLY BEEN REWRITTEN, NEVER TO TIDY UP.** Forgetting without a
	 * repair does not make an asset healthy — it makes the next `RecordFor` erase a true note, which is
	 * the failure the "call only after a projection that loaded the asset" rule on `RecordFor` exists
	 * to prevent. A repair is the one event that makes the old record genuinely out of date.
	 */
	static void Forget(const FString& PackageName);

	/**
	 * Stamp this package's load errors into its sidecar, replacing what a previous pass left.
	 *
	 * **CALL THIS ONLY AFTER A PROJECTION THAT LOADED THE ASSET**, which is what makes an empty
	 * ledger mean *it loaded clean* rather than *nobody has looked*. That distinction is the whole
	 * staleness rule: on the next session after a fix, the asset loads without complaint, the ledger
	 * is empty, and the stale entries are REMOVED. Called anywhere else it would clear a true note
	 * because the asset happened not to be loaded yet.
	 *
	 * Writes the file directly rather than through the sidecar writer — an errors-only rewrite is not
	 * an authored change, so it mints no history version, exactly as the compile annotator does.
	 */
	static bool RecordFor(const FString& PackageName);
};
