// The picture — `PLAY-SESSION-DESIGN.md` §3.1a, `RUNTIME-DRIVE-DESIGN.md` §17 P7.
//
// **THE FOURTH MEDIUM, AND THE ONLY ONE A `.gcp` CANNOT CARRY.** A capture says a door's
// `bIsOpen` is true; it cannot say the door is behind a pillar, that the boss is off screen, or
// that the HUD is covering the thing the test is about. §17 P7 wanted *renders and captures*, and
// until this file existed the first noun had no producer.
//
// ---------------------------------------------------------------------------
// WHY THIS IS A FILE AND NOT THREE LINES IN THE SESSION
// ---------------------------------------------------------------------------
//
// **A SHOT IS ASYNCHRONOUS AND EVERYTHING ELSE A `Mark` DOES IS NOT.** The world read behind
// `Capture` is synchronous — one tick, into a buffer — so the log line can name the file it just
// wrote. A screenshot is taken by the render thread at the end of a frame and the file appears
// afterwards, so the two halves (*ask* and *it exists*) are separated by time, and something has to
// hold the gap. That is the whole of this file: ask, and say when it landed.
//
// **THE REFUSAL IS THE FEATURE, AND IT HAS A PRECEDENT.** `gc_render` already refuses under
// `-nullrhi` by name rather than returning a blank image, and this asks the same question through
// the same predicate — `FApp::CanEverRender()`, which is what `GraphCodeRenderService.cpp` reads.
// A second implementation of *can we produce pixels* would drift from the one Core enforces
// ([[a-read-answers-through-the-write-path]]), and the honest reading matters more here than
// anywhere: the automated gate runs with the renderer off, so the refusal path is the one every
// headless run takes.
//
// ---------------------------------------------------------------------------
// THE ONE ENGINE TRAP, MEASURED RATHER THAN ASSUMED
// ---------------------------------------------------------------------------
//
// **`OnScreenshotCaptured` REPLACES THE FILE WRITE; `OnScreenshotRequestProcessed` FOLLOWS IT.**
// `UGameViewportClient::ProcessScreenshotData` is skipped entirely when the captured delegate is
// bound and `r.ScreenshotDelegate` is on (`GameViewportClient.cpp:2426`) — so a listener on the
// wrong delegate silently takes the picture away from the disk it was meant for. Nothing here binds
// it. The processed delegate broadcasts after the write (`:2452`), which makes it the only honest
// signal for *the picture exists now* — and because another subsystem may still have swallowed the
// bytes, the completion checks the file is really there before naming it.

#pragma once

#include "CoreMinimal.h"

/**
 * Ask the game for a picture of itself.
 *
 * Static, like the driver lease and the control channel, and for the same reason: a screenshot
 * request is process state (the engine holds exactly one), so pretending it is per-session would be
 * a second copy of a fact the engine already owns.
 */
class GRAPHCODEPLAY_API FGraphCodePlayShot
{
public:
	/**
	 * Can this process produce pixels at all?
	 *
	 * `OutReason` is written only when the answer is no, and it is written for a log line rather than
	 * for a programmer: it names what is missing, because a run whose shots are all refused should say
	 * why once per shot rather than leave a reader to infer it from silence.
	 */
	static bool CanRender(FString& OutReason);

	/** Is the engine already taking one? It holds a single request, so a second would overwrite it. */
	static bool IsBusy();

	/**
	 * Take the player's own view, UI included, and write it to `AbsolutePath` — which must end `.png`.
	 *
	 * Returns false with a reason when there is nothing to photograph. True means the request was
	 * ACCEPTED, never that the file exists: subscribe to `OnLanded` for that.
	 */
	static bool RequestPlayerView(const FString& AbsolutePath, FString& OutReason);

	/**
	 * Fired after the engine has processed a request — the file is written by then, or never will be.
	 *
	 * One multicast rather than a per-request callback, because the engine's own signal carries no
	 * identity: whoever is waiting knows which request it made.
	 */
	DECLARE_MULTICAST_DELEGATE(FGcPlayShotLanded);
	static FGcPlayShotLanded& OnLanded();

	/** Test teardown, and the same process-wide leak risk the lease and the channel both carry. */
	static void Reset();

	// -----------------------------------------------------------------------------------------------
	// A CLEAN STILL (gap report 13 Nov, V2 item 4; card P027)
	// -----------------------------------------------------------------------------------------------
	//
	// **THE GAME'S HUD STAYS; THE ENGINE'S CHATTER GOES.** Play's pictures carried the red and green
	// Print String lines and the `showdebug` box, and the field's own fix stripped every UMG widget,
	// which also took the crosshair a player needs. `gc_render` already holds screen messages off for
	// its own draws; this does the same for ONE player-view frame and touches no widget.
	//
	// **THE ORDER IS THE ENGINE'S, AND IT IS LOAD-BEARING.** `FScreenshotRequest::RequestScreenshot`
	// copies `GAreScreenMessagesEnabled` into `GScreenMessagesRestoreState`, and the viewport puts that
	// copy back after it has processed the request (`GameViewportClient.cpp`). So the messages go off
	// AFTER the request is made: the engine then restores the caller's own value when the frame is
	// taken, and this class restores the same value when it is not (`Reset`, or the bound below).

	/**
	 * The same request with screen messages (`GAreScreenMessagesEnabled`: Print String to screen, map
	 * warnings) and each local player's `showdebug` overlay off for the frame it captures. `bClean`
	 * false is exactly the two-argument form. Widgets and the HUD's own drawing are not touched.
	 */
	static bool RequestPlayerView(const FString& AbsolutePath, bool bClean, FString& OutReason);

	/** A clean request is in flight: what it turned off is still off. */
	static bool IsCleanInFlight();

	/**
	 * Wall-clock seconds after which a clean request that never landed gives back what it turned off
	 * anyway. Wall clock for `FGraphCodePlayPie`'s reason: what is guarded against is the frame never
	 * being drawn at all, and a frame count on a viewport that does not draw waits forever.
	 */
	static constexpr double CleanBoundSeconds = 5.0;
};
