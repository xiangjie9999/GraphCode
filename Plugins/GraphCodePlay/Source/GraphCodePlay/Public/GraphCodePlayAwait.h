// `Await` — the node that stops and asks. `PLAY-SESSION-DESIGN.md` §3.1, §3.1d;
// `RUNTIME-DRIVE-DESIGN.md` §17 item 4.
//
// EVERY OTHER NODE NARRATES; THIS ONE LISTENS. `Mark` says a thing happened and runs on. `Await`
// hands the run to whoever is driving it and waits to be told what happens next — which is the whole
// point of the control channel and the only place in Play where a graph can stop.
//
// ---------------------------------------------------------------------------
// THE THREE THINGS THAT DECIDED THIS FILE
// ---------------------------------------------------------------------------
//
// **1. IT ASKS WHETHER ANYONE IS LISTENING BEFORE IT PARKS, AND KEEPS ASKING WHILE PARKED.** §3.1d,
// [USER]: *"if there's connection, wait for reply or next step, if not, or if connection breaks,
// continue"*. No driver attached → this passes straight through and the log says nobody was watching.
// A driver that attaches, parks the game and then dies → the lease lapses and the park releases. The
// failure both halves exist to prevent is the same one: **a frozen world in front of a human tester
// with nothing left alive to unfreeze it.**
//
// **2. A PAUSED WORLD RUNS NO LATENT ACTIONS, WHICH RULES OUT THE OBVIOUS IMPLEMENTATION.**
// `Freeze` calls `SetGamePaused`, and `UWorld::Tick` processes latent actions only while it is doing
// actor ticks — so a `FPendingLatentAction` polling for its own release would stop being polled by
// the very pin that asked it to freeze. The watcher therefore runs on `FTSTicker`, which is not gated
// on the world at all, and the world is unpaused before the graph is resumed. This is also why the
// driver's lease is wall-clock: both halves of the freeze case need a clock the freeze cannot stop.
//
// **3. IT DECLINES RATHER THAN SWALLOWS.** The control channel is one ordered queue with more than
// one consumer: `continue` is this node's, and `end` and `load` belong to the session's own pump. So a
// park PEEKS — it takes the `continue` addressed to it, and for any other verb it releases the world
// and leaves the command at the front of the queue for the consumer that owns it. Holding a frozen
// game behind an instruction somebody else has to carry out is the hang of point 1 wearing a
// different hat.
//
// *(Written when this node was the queue's ONLY consumer and `end`/`load` could be executed by
// nothing at all. The rule did not have to change when they gained one, which is the argument for
// having built it as `peek, decide, take only what is yours` rather than as a special case.)*
//
// ---------------------------------------------------------------------------
// TWO EXEC PINS, AND WHY NOT THREE
// ---------------------------------------------------------------------------
//
// `Continued` and `TimedOut`, per §3.1's table. **Never parked, and parked-then-abandoned, both take
// `Continued`** — [USER] said *continue* for both, and a graph should not branch on the AI's
// plumbing. The distinction is real and it lives in the log, where `skipped=true` and
// `released=driver_lost` are different sentences: a run that waited and lost its driver is evidence
// of something a run that never waited is not.

#pragma once

#include "CoreMinimal.h"

#include "Containers/Ticker.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "GraphCodePlayAwait.generated.h"

class UGraphCodePlaySession;
class UWorld;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGcPlayAwaitOutput);

/**
 * How a park ended. Internal to the node and to its log line — a graph sees two pins, not four.
 */
enum class EGcPlayAwaitOutcome : uint8
{
	/** Nobody was listening, so nothing was parked. §3.1d's unattended row. */
	Unattended,

	/** A driver said `continue`. */
	Continued,

	/** The deadline passed with nothing said. */
	TimedOut,

	/** Parked, and the driver's lease lapsed under it. */
	DriverLost,

	/** A verb that is not this park's to execute reached the front of the queue (`end`, `load`). */
	Declined,

	/**
	 * The session closed underneath the park.
	 *
	 * **THE CASE THIS EXISTS FOR IS AN ORDERING, NOT AN ERROR.** `end` now has a consumer — the
	 * session's own pump — and both it and this node read the same queue. When the pump wins the race
	 * the park never sees the command, the graph's `On End Requested` handler closes the run on
	 * another exec chain, and a park with `Freeze=true` would hold a frozen world open with no session
	 * left to release it. That is §3.1d's hang arriving through the one door §3.1d did not have.
	 *
	 * Checked rather than signalled, so it also covers every other way a run can end under a park.
	 */
	SessionEnded,
};

/**
 * Park the graph until the driver answers.
 *
 * `Timeout` is in seconds and **zero or less means no deadline** — the park then ends only when the
 * driver answers or stops existing, which is the right shape for a run supervised by something that
 * thinks slowly, and is safe precisely because driver presence is checked continuously.
 *
 * ~~`Freeze` pauses the world while parked. **It is honoured only when a driver is attached**: pausing
 * an unattended game is the one thing §3.1d forbids outright, so the pin is ignored rather than
 * refused when nobody is listening.~~ **THE PIN IS GONE, 2026-08-16 — parking pauses the world,
 * full stop, and the unattended run still never pauses because it never parks.** §3.1h on the method
 * below.
 */
UCLASS()
class GRAPHCODEPLAY_API UGcPlayAwait : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** The driver answered, or there was nobody to answer, or the one that parked it went away. */
	UPROPERTY(BlueprintAssignable)
	FGcPlayAwaitOutput Continued;

	/** The deadline passed. A parked run that nobody came back to is a finding, not an error. */
	UPROPERTY(BlueprintAssignable)
	FGcPlayAwaitOutput TimedOut;

	/**
	 * **THERE IS NO `Freeze` PIN, AND THAT IS THE NODE'S DEFINITION RATHER THAN A DEFAULT [USER,
	 * 2026-08-16]:** *"await for AI will surely pause the game, or else it's just notify."* §3.1's
	 * three rungs are narrate (`Mark`), attend (`Notify=true`) and handshake (this) — and a park that
	 * lets the world run is the middle rung wearing this one's name. The AI would be deciding about a
	 * world that had already moved past its decision, and any injection would race a live one.
	 *
	 * A park still never pauses an UNATTENDED game (§3.1d) — that branch returns before the pause,
	 * so the rule holds structurally instead of through a flag somebody has to set correctly. And
	 * whether the pause actually took is MEASURED and written as `froze=`, because the engine can
	 * refuse it.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true",
			DisplayName = "Await", AdvancedDisplay = "Comment"))
	static UGcPlayAwait* Await(const UObject* WorldContextObject, const FString& Name,
		float Timeout = 300.0f, const FString& Comment = TEXT(""));

	virtual void Activate() override;

	/**
	 * Drive one poll of every parked `Await` in this process, as the ticker would.
	 *
	 * FOR SPECS, and it exists because the alternative is a spec that sleeps: the watcher runs on the
	 * core ticker, and an automation test that wants to assert what a park does when its driver
	 * vanishes should not have to wait a real two minutes to find out.
	 */
	static void PumpForTesting();

private:
	/** The ticker body. Returns false when this park is finished and the ticker should be dropped. */
	bool Poll(float DeltaSeconds);

	/** Unfreeze if we froze, write the resolving line, fire the pin, and let go. */
	void Resolve(EGcPlayAwaitOutcome Outcome);

	UWorld* WorldOf() const;

	UPROPERTY()
	TWeakObjectPtr<UWorld> World;

	FString Name;
	FString Comment;

	/**
	 * What the driver said WHEN IT ANSWERED, as opposed to `Comment`, which is what the graph said
	 * when it asked. Two sides of one conversation, and the record carries both.
	 *
	 * Kept here rather than read inside `Resolve`, because the command is consumed on the way past
	 * and `Resolve` runs after it is gone.
	 */
	FString Answer;

	double Deadline = 0.0;
	double ParkedAt = 0.0;
	/** Whether the pause ACTUALLY took — what `Resolve` unpauses on. Never an intention. */
	bool bFroze = false;
	bool bResolved = false;

	/**
	 * Whether a session was running when this parked.
	 *
	 * Without it, *the run ended under me* and *there was never a run* would be the same observation,
	 * and the second is ordinary: a graph may park in a build with no session open at all.
	 */
	bool bParkedInSession = false;

	FTSTicker::FDelegateHandle Ticker;
};
