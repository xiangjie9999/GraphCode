#pragma once

#include "CoreMinimal.h"
#include "GraphCodeErrors.h"

class UBlueprint;

/**
 * §11.1b. A COMPILE FINISHES ITSELF, the way a save does.
 *
 * User-directed 2026-08-08: *"saves and compiles are separate during their own save and compile
 * operation in visual editor. saves add the gc sidecar, compile adds the error"*. The Blueprint
 * editor has two buttons and each has its own product — save produces the `.gc` (C24.0, done), and
 * compile produces the `errors { }` block. Two hooks, two products, neither waiting on the other.
 *
 * **THE FORMAT ALREADY ACCOMMODATES IT, MEASURED RATHER THAN ASSUMED.** `SignedBody` excludes
 * `errors { }` from the `sig:` digest (E11.7a), so stamping compile errors moves no signature,
 * makes no file read as *edited since generated*, invalidates no pending apply and changes no
 * committed sidecar's digest. Without that property this would be unbuildable; with it, it is a
 * merge and a hook.
 *
 * **ADVISORY, NEVER BLOCKING** — `GcErrorKind::CompileError` and the argument is there. A failed
 * compile that wrote a blocking entry would make the asset that most needs a fix the one asset
 * GraphCode cannot write.
 *
 * **THE COMPILE OWNS ITS OWN ENTRIES AND NOTHING ELSE'S.** Writing is the easy half; STALENESS is
 * the row. Every compile replaces the `CompileError` entries and a clean one removes them, while
 * entries of every other kind — `MissingModule`, `RequiresProduct`, a `Recommendation` somebody
 * else left — survive untouched. An `errors { }` that outlives the compile that fixed it is a file
 * that lies.
 *
 * **AND THE REWRITE IS NOT AN AUTHORED EDIT.** It writes the file directly rather than through the
 * sidecar writer, so it mints no history entry and preserves no proposal — the same treatment
 * `GraphCodeProjectRestore`'s annotator already takes, and [[gc-sweep-vs-authored-write]]'s rule.
 */
class GRAPHCODE_API FGcCompileErrors
{
public:
	/**
	 * What this Blueprint's last compile said, as `errors { }` entries. Reads the engine's own
	 * message log for the asset, so a human pressing Compile is recorded exactly as our own call is.
	 *
	 * Empty means the compile was clean, which is a real answer and not a failure to read one.
	 */
	static TArray<FGcSidecarError> CollectFrom(UBlueprint* Blueprint);

	/**
	 * Merge `Compiled` into the text's `errors { }`, replacing what a previous compile left and
	 * leaving every other kind alone.
	 *
	 * Returns false and leaves `OutText` untouched when nothing would change — the caller must not
	 * write in that case, because rewriting moves the file's mtime and the panel's working-tree
	 * fingerprint reads mtime, so an idle compile would make every refresh after it look like work.
	 */
	static bool MergeInto(
		const FString& Text,
		const TArray<FGcSidecarError>& Compiled,
		FString& OutText,
		FString& OutError);

	/**
	 * Read the compile, merge it, write the sidecar. The whole hook in one call, so the service and
	 * a spec exercise the same path.
	 *
	 * Returns true only when the file changed. An asset with no sidecar is not an error: there is
	 * nowhere durable to put the note and nothing has gone wrong.
	 */
	static bool RecordFor(UBlueprint* Blueprint);
};
