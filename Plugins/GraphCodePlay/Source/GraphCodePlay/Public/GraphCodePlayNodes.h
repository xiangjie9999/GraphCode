// The nodes a test graph actually places — `PLAY-SESSION-DESIGN.md` §3.1's table, item 3's half.
//
// **THE TEST IS A GRAPH.** `RUNTIME-DRIVE-DESIGN.md` §14.2's inversion is the reason this file
// exists at all: the runner does not sequence a scenario, the game narrates itself, and these are
// the words it narrates with. They are the only half of Play that survives packaging.
//
// A FUNCTION LIBRARY OVER THE SUBSYSTEM, not methods on the subsystem itself, for one reason a
// caller can see: `Begin Play Session` should be a node you can drop on a wire without first
// getting a subsystem. The library resolves the world context and forwards; every rule lives in
// `UGraphCodePlaySession`.
//
// WHAT IS NOT HERE, so nobody reads the set as final. `Await` and `On End Requested` are their own
// files because they LISTEN — an async action each, which a function library cannot express. `End
// Slice` was deleted rather than deferred (see below).
//
// **`Mark` DOES CARRY A `Shot` INPUT, AND THE LINE THAT SAID IT COULD NOT WAS STALE FOR TWO DAYS.**
// It read *"there is no measured runtime screenshot path in this repo"* — which §3.1a measured FALSE
// on 2026-08-17 (`FScreenshotRequest` is runtime and public), and `GraphCodePlayShot.h` shipped that
// day. The refusal outlived its own reason in the one place a reader would look for the pin
// ([[refusal-reason-is-a-claim]]). It is now an `FGcPlayShot` rather than a boolean — §17 P7a-ii —
// and the two pure nodes at the bottom of this class are what build one.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodePlaySession.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "GraphCodePlayNodes.generated.h"

UCLASS()
class GRAPHCODEPLAY_API UGraphCodePlayNodes : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Open a session AND its first slice: the run's name, its folder, and the log everything else
	 * appends to.
	 *
	 * **A SESSION CANNOT START WITHOUT A SLICE** [USER, ruled 2026-08-15], which is why `Slice` is a
	 * required pin and not a convenience. Every record in the file lives inside a `Slice(…) {` block,
	 * so a session was always going to open one — it used to invent the name `main`. Naming it here
	 * makes the first slice authored like every other one, and it gives the run's opening stretch the
	 * one thing an invented slice could never have: a limit.
	 *
	 * **`Options` CARRIES TWO LIMITS AND THEY DO OPPOSITE THINGS.** `SliceLimit` is §3.1f's ping
	 * cadence for that first slice and it ends nothing; `SessionLimit` is §3.1b's deadline for the
	 * whole run and it is the only clock in the product that does. They were one pin called
	 * `TimeLimit` until they had to sit together, and the rename is the point: a generic name beside a
	 * specific one gets read as the general case of it.
	 *
	 * **A STRUCT RATHER THAN PINS, closing §3.1a's `[OPEN]` where that section said it would come
	 * due** — *"four is where Mark's argument starts to apply"*, and `SessionLimit` is the fifth.
	 * `Name` and `Slice` stay as their own pins because they are required, and a required input folded
	 * into an options struct is how a required thing comes to look optional.
	 *
	 * `Started` is false when instrumentation is off in this build, when a session is already
	 * running, when no slice was named, or when the folder could not be made — `Error` says which. A
	 * graph is free to ignore all of them: every node below is inert without a session rather than
	 * dangerous.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Begin Play Session",
			AutoCreateRefTerm = "Options"))
	static bool BeginPlaySession(const UObject* WorldContextObject, const FString& Name,
		const FString& Slice, const FGcPlaySessionOptions& Options, FGcPlaySessionHandle& Handle,
		FString& SessionId, FString& SessionLogPath, FString& Error);

	/**
	 * A moment: a name, and whatever media it carries — prose, a capture, a wake signal.
	 *
	 * **ONE NODE FOR ALL OF THEM**, which is §3.1's rule that a medium is a property of a moment and
	 * not a node of its own. Fifty of these in a run is fine; three with `Notify` is not.
	 *
	 * SYNCHRONOUS AND WITH NO FAILURE PIN. A failed capture writes `failed=` on its line and the run
	 * carries on — a test graph should not carry error handling for disk I/O.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Mark",
			AutoCreateRefTerm = "Options"))
	static FGcPlayMarkResult Mark(const UObject* WorldContextObject, const FString& Name,
		const FGcPlayMarkOptions& Options);

	/**
	 * Close the session with a verdict, and close the log.
	 *
	 * The verdict is the GRAPH'S, which is §3.1b's ruling: the test knows whether it passed and the
	 * runner does not. A session the game instance takes down instead ends `stopped`, so a missing
	 * `End` line means a crash rather than an ordinary exit.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "End Play Session",
			AdvancedDisplay = "Comment"))
	static bool EndPlaySession(const UObject* WorldContextObject, const FGcPlaySessionHandle& Handle,
		EGcPlaySessionResult Result, const FString& Comment = TEXT(""));

	/**
	 * Open a slice, and close the one that was running — §3.6a's one block kind, §3.6c's close rule.
	 *
	 * **THIS NODE IS ALSO THE MARKER.** An event meant to be entered cold is an event whose chain
	 * opens with this, so there is no allowlist and no declared list of entries anywhere: the graph
	 * already says which events are entry points, in the place where it is true (§3.6a-i).
	 *
	 * Calling it a second time is ordinary, not a fault — that IS the progression case. The previous
	 * slice closes `by=progression` and this one opens. **It carries no result**: the slice verdict
	 * was deleted [USER, ruled 2026-08-15] and `by=` is the whole record, so the `superseded` this
	 * line used to name is a token nothing writes any more.
	 *
	 * `SliceLimit` is §3.1f's slice clock, in seconds of wall clock, and **it ends nothing**. Each
	 * time it comes round the AI is told what has changed since it last looked; a slow slice and a
	 * dead slice look identical from outside, so the clock hands the question over instead of
	 * answering it. Zero — the default — means no clock at all. **Named to match
	 * `FGcPlaySessionOptions::SliceLimit`**, since one concept spelled two ways across two nodes is
	 * the drift that rename existed to stop.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Begin Slice",
			AdvancedDisplay = "SliceLimit"))
	static bool BeginSlice(const UObject* WorldContextObject, const FString& Name, FString& Error,
		float SliceLimit = 0.0f);

	// THERE IS NO `End Slice` NODE, and its absence is a ruling rather than a gap.
	//
	// **[USER, 2026-08-15]** *"you can't end a slice, you can only end a session. you end slice by
	// placing another slice on top."* A slice is where the run currently is, so *nowhere* is not a
	// state it can be in: the graph is always inside some slice until the session closes. That makes
	// the log's block structure total by construction rather than by a caller remembering to close
	// what it opened — and a forgotten close is spelled exactly like a crash (§5.13), which is the
	// failure this deletes outright.

	/**
	 * Go back to a captured state and enter it again — §3.4, and **`Load` IS what branching means**.
	 *
	 * *"you only put a world back in order to try something else from there"*, so `Branch` is not an
	 * annotation on this node, it is what the node does. Place it at the END of a chain: `Begin Slice`
	 * establishes where a slice starts, and this one says *go back and try again, altered*. They sit at
	 * opposite ends of a slice and are not interchangeable — one causes the other, and X causing Y is
	 * not X being Y ([[causing-a-thing-is-not-being-it]]).
	 *
	 * **BOTH THE GRAPH AND AN AI CAN CALL IT** [USER, 2026-08-15], and the graph half is the one that
	 * survives packaging: a harness whose fight ends in `Load` back to its own start is a retry loop
	 * that needs no driver, no lease and no bridge. The driver half is `gcp_run mode:"load"` and it
	 * lands in exactly the same queue.
	 *
	 * `Entry` is `Actor.Event` — an entry is an ordinary custom event, so the actor holding it is part
	 * of its address, and an unqualified name is refused with the candidates named.
	 *
	 * **`Staged` DOES NOT MEAN LOADED.** Nothing has moved when this returns: the state is resolved and
	 * validated, and the entry fires on the NEXT tick. That deferral is what makes a loop a loop rather
	 * than a recursion, and it means anything wired after this node runs once more in the OLD world —
	 * so the honest place for a graph's next act is inside the entry's own chain.
	 *
	 * **THERE IS NO `Patch?` PIN, and that is a ruling rather than a gap (§3.4a, §5.14a): you patch by
	 * editing the `.gcp` you load.** A variant file is the input; the computed `patch { }` in the log
	 * is the output. So a graph varies a retry by holding several paths and loading a different one —
	 * `From` is a string, and that needs nothing new. Adaptive variation (*ten percent less each time*)
	 * belongs to the AI, because writing the next file is what deciding the next value amounts to.
	 *
	 * False means nothing was queued and `Error` says why: the `.gcp` was not found or would not plan
	 * against this world, the entry does not resolve, it takes parameters, or the state would destroy
	 * the actor holding it.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Load",
			AdvancedDisplay = "Description"))
	static bool Load(const UObject* WorldContextObject, const FString& From, const FString& Entry,
		FString& Error, const FString& Description = TEXT(""));

	/**
	 * Which slice is running, and what state it started from.
	 *
	 * PURE, and it reads the SESSION rather than an event's pins — §3.6a's cost of making a slice
	 * entry an ordinary custom event: a plain event has no `From` output, so the context lives where
	 * every entry can reach it whatever its signature.
	 */
	UFUNCTION(BlueprintPure, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Get Current Slice"))
	static void GetCurrentSlice(const UObject* WorldContextObject, FString& Name, FString& From);

	/** Whether a session is open right now. */
	UFUNCTION(BlueprintPure, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Is Play Session Running"))
	static bool IsPlaySessionRunning(const UObject* WorldContextObject);

	// **THERE ARE NO `Shot`, `Rolling Shots` OR `Record` MAKER NODES, AND THAT IS THE POINT** [USER,
	// 2026-08-21]: *"see redundant nodes? rolling shots, shot and record all is within mark."* All
	// three were `BlueprintPure` makers for structs nested inside `FGcPlayMarkOptions`, and that
	// nesting is what its own *keep it flat as it grows* rule forbids — a nested struct is a pin the
	// author has to split twice, and the second split is not discoverable, so each one had to be
	// handed a node to fill it. Reflection generated four more `Make`/`Break` nodes on top.
	//
	// **THE FIX IS THE RULE THAT WAS ALREADY WRITTEN**: cluster by NAMING, never by nesting. The
	// fields are `ShotMode`/`ShotEvery`/`ShotLimit`/`ShotKeep` and `RecordMode`/`RecordWatch`/
	// `RecordEvery`/`RecordLimit`/`RecordMax` on `Mark` itself, which is where a medium of a moment
	// belongs. Seven nodes gone, six pins gained, nothing lost — and the C++ tidiness of nesting was
	// being paid for at every call site anyway.
};
