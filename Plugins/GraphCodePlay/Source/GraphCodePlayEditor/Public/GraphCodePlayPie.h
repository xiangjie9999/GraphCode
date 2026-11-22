// Pressing Play from a tool — `RUNTIME-DRIVE-DESIGN.md` §17 P2's first box, §2.2, §14.5.
//
// **THIS IS THE LAST MANUAL STEP IN THE LOOP.** Everything else an agent needs to drive a run
// already exists: it can attach, capture, apply, answer a parked graph and ask a run to stop. What
// it could not do was start the game — a person had to press Play in the editor first, which meant
// no run could ever begin unattended. One call, and the loop closes.
//
// ---------------------------------------------------------------------------
// THE ONE FACT THAT SHAPES ALL OF IT: PIE CONTROL IS QUEUED (§2.2)
// ---------------------------------------------------------------------------
//
// `RequestPlaySession` STORES a request; the editor services it on a later tick. A caller that
// requests a session and then reads `GEditor->PlayWorld` in the same call reads null, every time,
// and the same is true of `RequestEndPlayMap`. So nothing here can return *it is playing* — it
// returns *it was asked*, and the caller polls. That is §2.2's deferral honoured rather than
// papered over, and it is why `Start` below has a WATCHER rather than a return value.
//
// **The deadline is wall clock, for §14.5's reason**: what is being guarded against is the editor
// never servicing the request at all, and a frame-count deadline on a dead editor waits forever.
//
// ---------------------------------------------------------------------------
// WHY THE SESSION CAN RIDE ALONG, AND WHY THAT IS ONE ACT RATHER THAN TWO
// ---------------------------------------------------------------------------
//
// `PLAY-SESSION-DESIGN.md` §13 ruled `start`'s two jobs apart by their arguments — a name means
// *begin a session*, no name means *start the game* — and then said what happens when the second
// half lands: *"`start name:"…"` against a dead editor stops refusing and simply does both."* It
// cannot be two tool calls, because the thing between them is a tick the caller cannot take. So the
// request carries the session it wants, and the watcher opens it the moment the world is genuinely
// playing.
//
// **`HasBegunPlay()` AND NOT MERELY A NON-NULL `PlayWorld`.** A world exists a tick or two before it
// begins play, and in that window a session opened here would land ahead of any the game's own
// `BeginPlay` opens — so the graph's `Begin Play Session` would meet the *one is already live*
// refusal and the run would look broken for a reason nothing in the log explains. Waiting means the
// graph gets there first when it has an opinion, and §3.1g's end-and-start rule governs the
// collision honestly: with the lease we replace, without it we are told.
//
// ---------------------------------------------------------------------------
// WHERE IT LIVES
// ---------------------------------------------------------------------------
//
// `GraphCodePlayEditor`, because `GEditor` is editor-only and `GraphCodePlay` is `Runtime` — a
// packaged game has no PIE to press. That is the same split the module already exists for.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodePlaySession.h"

class UWorld;

/**
 * What one start is asking for: the game, and optionally the run recorded over it.
 *
 * A STRUCT RATHER THAN SIX PARAMETERS because the session half is five values that must travel
 * together — a name with no slice is not a session, and half of one arriving at the watcher a tick
 * later is the shape that produces a run recorded under a name nobody chose.
 */
struct FGcPlayPieStartRequest
{
	/** False starts the game and nothing else — §13's *no name means start the game*. */
	bool bBeginSession = false;

	/** The run's name, and the folder it is written to. Required when `bBeginSession`. */
	FString SessionName;

	/** The stretch the run opens in. A session cannot open without one (§3.6a). */
	FGcPlaySliceSpec Slice;

	/** The prose, the caller's id, and §3.1b's session deadline. */
	FGcPlaySessionSpec Session;

	/**
	 * The caller holds the driver lease, so a session already running when the world comes up may be
	 * ENDED and replaced — §3.1g, and the same test `gcp_run start` already applies to a live world.
	 *
	 * False means *refuse and say so* rather than *take it*: the collision is normal (the game's own
	 * graph may open a session in `BeginPlay`), and only a driver may end somebody's run.
	 */
	bool bMayReplace = false;
};

/** What `Stop` actually did — the two are separate because both can be true in one call. */
struct FGcPlayPieStopResult
{
	/** A live play world was asked to end. It ends on a later editor tick, never here. */
	bool bEndRequested = false;

	/**
	 * A start that had been queued and not yet serviced was dropped.
	 *
	 * **WITHOUT THIS, `stop` HAS A HOLE THE SIZE OF A TICK**: a caller that starts and immediately
	 * stops would get *nothing was playing*, and then PIE would come up two ticks later because the
	 * queued request was still in flight.
	 */
	bool bPendingCancelled = false;
};

/**
 * One console variable set through the tool while GraphCode's own Play ran (V2 item 5, card P287).
 * `Before` is the value it held when first set during that Play, `During` the last value the tool
 * set, `Restored` what it reads after being put back (empty until then).
 */
struct FGcPlayPieConsoleVariable
{
	FString Name;
	FString Before;
	FString During;
	FString Restored;
};

/**
 * Start and stop Play In Editor, and open a session over it when the world is genuinely playing.
 *
 * STATIC AND PROCESS-WIDE, like the driver lease and the control channel next door, and for the
 * same reason: there is one editor and therefore one PIE session, so a per-object owner would be an
 * object every caller had to find first.
 */
class GRAPHCODEPLAYEDITOR_API FGraphCodePlayPie
{
public:
	/**
	 * Ask the editor to start playing, and arm the watcher.
	 *
	 * **A SECOND START WHILE ONE IS IN PROGRESS IS REFUSED, NOT STACKED** (§17 P2). `IsPlaying()`
	 * covers a running world and `IsStarting()` covers a queued request, and both must be checked:
	 * a request that is queued and not yet serviced is a start that has already happened as far as
	 * the caller is concerned, and answering *nothing is playing* to it would produce two.
	 *
	 * True means the request went out, NOT that a game is running. Poll `IsPlaying()`.
	 */
	static bool Start(const FGcPlayPieStartRequest& Request, FString& OutError);

	/**
	 * Ask a running game to end, and drop any start that had not been serviced yet.
	 *
	 * **IT DOES NOT END THE SESSION ITSELF, and it must not.** A session open when the game instance
	 * goes is closed by `UGraphCodePlaySession::Deinitialize` with `result=stopped` — the one writer
	 * rule (§5): two writers on an append-only log is a torn-line generator, and reaching in from
	 * the editor to write an `End` line the game is about to write too is exactly that.
	 */
	static FGcPlayPieStopResult Stop();

	/** A play world exists and has begun play. The question `is_playing` asks. */
	static bool IsPlaying();

	/**
	 * The play world running now is the one a `Start` here brought up, not a person's Play button.
	 *
	 * **A TEST RUNNER STOPPED THE OWNER'S OWN PLAY (field report E3, 2026-10-28).** The lease says who may drive,
	 * and `attach` is free, so holding it was not evidence of having started the game. `gcp_run` stops or ends only
	 * a game this answers true for.
	 */
	static bool IsPlayingOwnStart();

	/**
	 * For specs only, settable for the reason `StartDeadlineSeconds` is: a stand-in world counts as this class's own start, so
	 * what `IsPlayingOwnStart` allows (stop, end, spawn) can be asserted without pressing Play. Null clears it.
	 */
	static void MarkOwnStartForTest(UWorld* World);

	/** A start is queued or in flight and the world has not begun play yet — the third state. */
	static bool IsStarting();

	/** The session name a queued start is carrying, or empty. */
	static FString StartingSessionName();

	/**
	 * How the last start ended, as a token — `started`, `timed_out`, `session_refused`, `cancelled`,
	 * `not_ticking` (the world began play and its clock did not move within `StartTickBound` editor ticks).
	 * `started` is written once the world has ADVANCED, not when it exists.
	 *
	 * **A DEFERRED ACT OWES AN OUTCOME SOMEWHERE THE CALLER LOOKS.** The tool call returned before
	 * anything happened, so a start that never completed would otherwise be indistinguishable from
	 * one nobody made: same null world, same absent session, no reason anywhere. Empty until a start
	 * has finished one way or the other.
	 */
	static const FString& LastStartOutcome();

	/** The sentence behind the token — what refused, or how long it waited. */
	static const FString& LastStartNote();

	/**
	 * The wall-clock budget for a start, and settable for the reason `EndGraceSeconds` is: a spec
	 * asserting the timeout would otherwise have to sleep through the real one.
	 */
	static double& StartDeadlineSeconds();

	/**
	 * Drop the watcher and cancel a start this class queued. For specs, and for module shutdown.
	 *
	 * It cancels ONLY a request this class made — a play session somebody else queued is not ours to
	 * throw away, and a `Reset` that reached further would make the suite's cleanup a way to kill the
	 * user's own playtest.
	 */
	static void Reset();

	// -----------------------------------------------------------------------------------------------
	// WHETHER THE GAME TICKS (gap report 13 Nov, V2 item 2; card P293)
	// -----------------------------------------------------------------------------------------------
	//
	// **A WORLD THAT EXISTS IS NOT A GAME THAT RUNS.** After some Anim Blueprint writes every Play had
	// a world that had begun play and never advanced — game time 0, not paused — and `last_start`
	// said `started`, because it was written the moment `HasBegunPlay()` turned true. Now a start
	// waits for the clock to move before it says `started`, and `status` can ask the same question of
	// any Play, a person's included.
	//
	// "ADVANCED" MEANS THE GAME CLOCK MOVED, or, for a paused world, the real clock did. A paused game
	// is a game that ticks; a world whose game time stands still while nothing paused it is the fault.

	/**
	 * Editor ticks a start waits, after the world has begun play, for its clock to move before the
	 * start is reported `not_ticking`. COUNTED IN EDITOR TICKS, not wall clock, and the header's
	 * reason for wall clock does not reach here: the watcher counting them runs on the core ticker,
	 * so while it counts the editor is demonstrably ticking, and a PIE world is ticked in the same
	 * editor tick. A healthy start advances within one or two; 60 leaves room for first-frame hitches
	 * (shader and asset loads land inside a tick, they do not skip one).
	 */
	static constexpr int32 StartTickBound = 60;

	/** `IsPlayWorldTicking` answers true when the play world advanced within this many editor ticks. */
	static constexpr int32 TickingWithinFrames = 10;

	/** Editor ticks since the running play world last advanced; -1 when none runs or it never has. */
	static int64 FramesSincePlayWorldAdvanced();

	/** A game is playing and its clock moved within the last `TickingWithinFrames` editor ticks. */
	static bool IsPlayWorldTicking();

	/** Hook (true) or unhook (false) the per-tick observer. The module does it at startup and shutdown. */
	static void WatchPlayWorldTicks(bool bWatch);

	/** For specs: run the observer on a stand-in world as the end of one of its ticks would. */
	static void NotePlayWorldTickForTest(UWorld* World);

	/** For specs: one step of a pending start's watcher, as one editor tick would run it. False if none is pending. */
	static bool StepStartWatchForTest();

	// -----------------------------------------------------------------------------------------------
	// CONSOLE VARIABLES PUT BACK WHEN GRAPHCODE'S PLAY ENDS (gap report 13 Nov, V2 item 5; card P287)
	// -----------------------------------------------------------------------------------------------
	//
	// **A VARIABLE IS PROCESS STATE, A PLAY IS NOT.** A harness set `r.ScreenPercentage` through the
	// console route during its Play and every other session's renders came out at the lower quality
	// for twelve minutes. The console route records the value a variable held before the tool first
	// set it during GraphCode's OWN Play, and that value is put back when the Play ends, by any route:
	// `Stop`, and `EndPIE` for `end`, the game quitting itself, or a person pressing Stop.
	//
	// **WHAT IT DOES NOT COVER, said rather than implied:** a variable a game's own Blueprint or code
	// sets (`Execute Console Command`, `Set Console Variable`), one set through any other door (the
	// editor's console, a `.ini`, Python, `execute_script` spelled natively), and one set while no
	// GraphCode Play was running. Those are recorded nowhere and nothing puts them back.

	/**
	 * The console route set `Name` from `Before` to `During`. Recorded only while `IsPlayingOwnStart`;
	 * the first `Before` of a Play is kept, later sets update `During`. True if it was recorded.
	 */
	static bool NoteConsoleVariableSet(const FString& Name, const FString& Before, const FString& During);

	/** The variables the running Play will put back, in the order they were first set. */
	static const TArray<FGcPlayPieConsoleVariable>& HeldConsoleVariables();

	/** What the last restore put back, with `Restored` filled. Kept until the next `Start`. */
	static const TArray<FGcPlayPieConsoleVariable>& LastRestoredConsoleVariables();

	/** Put every held variable back now. Returns how many were held. `Stop`, `EndPIE` and `Reset` call it. */
	static int32 RestoreConsoleVariables();

	/** Hook (true) or unhook (false) the end-of-Play restore. The module does it at startup and shutdown. */
	static void WatchPlayEnd(bool bWatch);
};
