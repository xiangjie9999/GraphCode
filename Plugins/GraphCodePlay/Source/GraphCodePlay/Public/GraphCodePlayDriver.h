// Is anybody listening? — `PLAY-SESSION-DESIGN.md` §3.1d, `RUNTIME-DRIVE-DESIGN.md` §17 item 4.
//
// This is the whole of the third gate state. §3.1d's three states are **off / instrumented-unattended
// / driver-attached**, and only the third may pause or mutate a world; the first two are settings, and
// this file is the observation that separates the third from the second.
//
// ---------------------------------------------------------------------------
// FOUR THINGS THAT DECIDED THIS FILE, none of them arbitrary
// ---------------------------------------------------------------------------
//
// **1. THE PRESENCE IS PUSHED IN, NEVER PULLED.** `GraphCodePlay` is `Runtime`; the MCP bridge is in
// `UnrealClaude`, which is `Editor`. Play may not depend on it and must never learn to. So a driver
// ATTACHES — the bridge calls in — and Play only ever answers a question about state it was handed.
//
// The consequence is the reason to like it: **a packaged game has no bridge, so nothing can ever
// attach, so a shipped title is permanently instrumented-unattended by CONSTRUCTION.** Not by a
// default, not by a flag somebody could ship inverted, not by a `#if` anybody has to remember. The
// registrar does not exist in that build. That is the strongest available form of §3.1d's safety
// property and it costs nothing.
//
// **2. IT IS A LEASE, BECAUSE THE TRANSPORT IS HTTP.** [USER, 2026-08-15] ruled *"if there's
// connection, wait for reply or next step, if not, or if connection breaks, continue"* — so a broken
// connection must release a parked `Await`. But the bridge is a request/response HTTP server: there
// is no persistent socket whose closure IS the break. A driver that crashes closes nothing; it simply
// stops calling. **Absence of a signal is the only signal available**, so presence has to expire.
// A driver asserts it and must keep asserting it, exactly as `agent-coord` already treats agent
// liveness in this project.
//
// **3. THE CLOCK IS WALL-CLOCK, AND THIS IS THE TRAP.** `Await(Freeze=true)` calls `SetGamePaused`.
// Measure the lease in game time and it stops advancing the moment the world freezes — so a frozen
// world whose driver has died would never expire, and *frozen world plus dead driver* is the exact
// hang the ruling exists to prevent. A deadline must be measured in something that keeps moving when
// the thing it watches is dead.
//
// **4. THERE IS NO `OnDriverLost` DELEGATE, ON PURPOSE.** The only thing that cares is a parked
// `Await`, and a parked `Await` is a latent action already polling every tick — so it can ask. A
// delegate would add a subscription lifetime to manage and a way to fire into a destroyed graph, to
// tell somebody something they were about to look at anyway. `IsAttached()` is a pure query and
// stays one.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodePlayDriver.generated.h"

/**
 * A driver's lease. Opaque, and a GUID for the same reason `FGcPlaySessionHandle` is one: the thing a
 * handle must never do is stay valid past its own lifetime and silently address the next holder.
 */
USTRUCT(BlueprintType)
struct FGcPlayDriverHandle
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GraphCode|Play")
	FGuid Id;

	bool IsValid() const { return Id.IsValid(); }
};

/**
 * Who is listening, and until when.
 *
 * Process-wide rather than per-world: a driver attaches to the *process* it is driving, and it stays
 * attached across a level load exactly as a session does. Static state is therefore the honest shape,
 * not a shortcut.
 */
class GRAPHCODEPLAY_API FGraphCodePlayDriver
{
public:
	/**
	 * How long an unrenewed lease survives. Generous on purpose: an attached AI can legitimately
	 * think for a long time between calls, and a lease that expires under normal latency would
	 * release parked graphs mid-conversation — which reads exactly like the driver dying, and would
	 * make the honest signal untrustworthy.
	 */
	static constexpr double DefaultLeaseSeconds = 120.0;

	/**
	 * Attach, or renew an existing lease.
	 *
	 * Pass a handle from a previous call to renew it; pass a default-constructed one to attach fresh.
	 * Renewal through the same entry point is deliberate — a separate `Renew` would let a caller
	 * attach twice and hold two leases over one driver, and then *is anybody listening* would have to
	 * mean *are any of them listening*.
	 *
	 * @return the lease, or an invalid handle if `Handle` was non-empty and did not match the live
	 *         lease — a stale renewal must not silently become a new attachment.
	 */
	static FGcPlayDriverHandle Attach(const FString& Description,
		const FGcPlayDriverHandle& Handle = FGcPlayDriverHandle(),
		double LeaseSeconds = DefaultLeaseSeconds);

	/** Let go. A handle that is not the live lease does nothing, and says so. */
	static bool Detach(const FGcPlayDriverHandle& Handle);

	/**
	 * Is a driver attached RIGHT NOW — the question §3.1d asks before parking and, per the ruling,
	 * keeps asking while parked. Self-expiring: an unrenewed lease is already gone when read, so
	 * nothing has to tick for this to become false.
	 */
	static bool IsAttached();

	/** What the attached driver called itself, or empty. For the log line, never for a decision. */
	static FString AttachedDescription();

	/**
	 * Did the driver that is no longer here STOP CALLING, rather than let go?
	 *
	 * **[USER, 2026-08-17] made this a decision rather than a diagnostic**: a run that is the agent's
	 * own ends when the agent disappears, and carries on when the agent says *carry on without me*.
	 * `IsAttached()` cannot separate those — both answer false — and folding them would let a polite
	 * `detach` kill the run it was politely stepping back from.
	 *
	 * **IT NEEDS NO NEW STATE, because the lease already holds the difference.** `Detach` clears the
	 * whole record, so the id goes invalid; a lapse leaves the id sitting there with a deadline in
	 * the past. *Remembered but expired* IS the disconnection, and reading it costs a comparison.
	 *
	 * False before any driver has ever attached, which is why the session also tracks whether it ever
	 * saw one: an absence that was always there is not a loss.
	 */
	static bool LastReleaseWasLapse();

	/**
	 * The live lease's id, or an invalid guid.
	 *
	 * For STAMPING, not for authentication — `FGraphCodePlayControl` marks each command with the
	 * lease that posted it so a lapsed driver's instructions can be dropped rather than run. A caller
	 * deciding whether it may act should ask `IsAttached()`, which cannot be confused with *is this
	 * the same driver as before*.
	 */
	static FGuid CurrentLeaseId();

	/** Seconds until the lease lapses, or 0 if nothing is attached. Diagnostic only. */
	static double SecondsRemaining();

	/**
	 * Drop everything. **For test teardown**, so one gate's lease cannot make the next gate believe a
	 * driver is present — the failure [[test-fixtures-leak-into-other-specs]] describes, and this
	 * state is process-wide, which is precisely the kind that leaks.
	 */
	static void Reset();
};
