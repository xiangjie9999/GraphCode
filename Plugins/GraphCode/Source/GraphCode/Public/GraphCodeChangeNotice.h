#pragma once

#include "CoreMinimal.h"

/**
 * E11.10 — ONE PLACE THAT SAYS "SOMETHING MOVED", so the panel can stop being the last to know.
 *
 * The version-control panel re-read when its tab was brought forward and at the end of every action
 * it took itself, which left one hole and it was the one that mattered: a change made by anything
 * ELSE while the panel was already frontmost — a save, an agent's write over MCP, an asset created
 * or deleted — was invisible until you clicked away and clicked back. User-directed, 2026-08-05.
 *
 * A REFRESH BUTTON IS STILL NOT THE ANSWER. E9.5 deleted it on the grounds that a control whose job
 * is "show me what is actually there" is the panel admitting it might be showing something else.
 * The fix for a stale view is to stop being stale, not to add a button that asks.
 *
 * WHY A HUB AND NOT FIVE DIRECT CALLS INTO THE WIDGET. The producers are a save hook, an asset
 * registry callback and an MCP dispatch loop in another module; none of them may know a UMG widget
 * exists, and two of them (the service, the bridge) run in editors where no panel is open at all.
 * A broadcast that usually reaches nobody costs nothing.
 */
enum class EGcChangeScope : uint8
{
	/**
	 * A `.gc` moved on disk — written, deleted or renamed.
	 *
	 * `UGraphCodeVcsLibrary::WorkingTreeFingerprint` already sees this kind by itself, by `stat`, so
	 * a poke that turns out to have changed nothing the panel is showing costs ten milliseconds
	 * rather than a full rescan. That is the whole reason the scope is a parameter.
	 */
	Disk,

	/**
	 * The editor moved and no file did.
	 *
	 * An agent's `bp_apply` rewrites a Blueprint and marks its package dirty; the `.gc` is not
	 * regenerated until somebody saves. Nothing on disk records that, so the fingerprint cannot see
	 * it and the panel's `editor (unsaved)` comparison — the one no git client can even ask — would
	 * stay wrong indefinitely. A poke at this scope advances a session counter that the fingerprint
	 * folds in, which is what makes the next refresh a real one exactly once.
	 */
	EditorState,
};

class GRAPHCODE_API FGcChangeNotice
{
public:
	/**
	 * @param Reason  What moved, in a few words — it is logged and it is what a listener may show.
	 * @param Serial  The editor-state counter at the moment the settle ran.
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSettled, const FString& /*Reason*/, uint64 /*Serial*/);
	static FOnSettled& OnSettled();

	/**
	 * Say that something changed. Cheap, non-blocking, and safe to call in a loop.
	 *
	 * **IT DOES NOT BROADCAST.** A bulk operation pokes once per asset — deleting forty things, or
	 * a save that regenerates a folder — and a listener that rescanned every sidecar in the project
	 * forty times would be slower than the operation it is describing. The first poke arms a timer;
	 * everything inside that window rides along on it and the broadcast happens once, after the
	 * burst has stopped. `AutoRefreshSettleSeconds` is the window.
	 *
	 * Callable from any thread. The broadcast itself always runs on the game thread, because it is
	 * the core ticker that raises it.
	 */
	static void Poke(EGcChangeScope Scope, const FString& Reason);

	/**
	 * How many editor-state changes this session has seen. Monotonic, never reset.
	 *
	 * Folded into `WorkingTreeFingerprint` so that a comparison against unsaved editor state is
	 * re-read after a write that no file recorded — and, just as importantly, is NOT re-read when
	 * nothing has happened since. It advances at `Poke` rather than at settle, so a listener that
	 * refreshes for its own reasons in between still captures the new value and the settle that
	 * follows finds its work already done.
	 */
	static uint64 EditorStateSerial();

	/** Is a burst still riding a timer? Specs assert the coalescing with this. */
	static bool IsSettlePending();

	/** Fire the pending settle now instead of when the timer says. Nothing happens if none is armed. */
	static void SettleNow();

	/** Drop any armed timer. Module shutdown — a ticker that outlives its listeners is a crash. */
	static void Shutdown();
};
