// `On End Requested` — the one door for every ending that is not the graph's own.
// `PLAY-SESSION-DESIGN.md` §3.1, §3.1b; `RUNTIME-DRIVE-DESIGN.md` §17 item 5.
//
// A DRIVER'S `end` AND THE SESSION'S DEADLINE ARRIVE HERE AS THE SAME EVENT, differing only in
// `Reason`. That is §3.1b's ruling and it is worth stating as a design property rather than an
// implementation detail: *stop this run* is one thing to handle, and the work a graph does about it —
// capture the last state, write a verdict, close — is the same work whoever asked.
//
// ---------------------------------------------------------------------------
// WHY AN EVENT AND NOT AN EXEC PIN
// ---------------------------------------------------------------------------
//
// §3.1b: *"It cannot be an exec pin on `Begin Play Session`. A sync node's exec fires once, at the
// start; an expiry fires later."* So it is a node that listens, in the shape `Await` already
// established — an async action watched for the life of the game instance.
//
// ---------------------------------------------------------------------------
// THE RUN STILL ENDS IF NOBODY LISTENS, AND THAT IS NOT THIS NODE'S JOB
// ---------------------------------------------------------------------------
//
// §14.5 is absolute: a run never ends silently. Handing the ending to the graph is exactly how you
// would accidentally break that, so the session starts a grace clock when it raises this and closes
// the run itself with `result=expired_unhandled` if the graph does nothing.
//
// **"Handled" is measured as *the run ended*, never as *somebody was listening*** — deliberately. A
// listener that fires and does nothing is indistinguishable from no listener at the only point that
// matters, and a graph that binds this node and then takes a branch which never reaches
// `End Play Session` is precisely the run the backstop exists for.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodePlaySession.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "GraphCodePlayEndRequested.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGcPlayEndRequestedOutput, EGcPlayEndReason, Reason);

/**
 * Fires when something asks the run to stop — a driver's `end`, or §3.1b's session limit.
 *
 * Place it on `BeginPlay`, above or below `Begin Play Session`; the event it listens to is
 * process-wide precisely so that either order works.
 *
 * **It fires every time a run is asked to stop, not once.** A graph that ends its session and opens
 * another keeps this listener, which is what makes the node usable in a harness that loops (§3.4).
 */
UCLASS()
class GRAPHCODEPLAY_API UGcPlayOnEndRequested : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Somebody asked the run to stop. `Reason` says who — and it is the only difference. */
	UPROPERTY(BlueprintAssignable)
	FGcPlayEndRequestedOutput Requested;

	UFUNCTION(BlueprintCallable, Category = "GraphCode|Play",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true",
			DisplayName = "On End Requested"))
	static UGcPlayOnEndRequested* OnEndRequested(const UObject* WorldContextObject);

	virtual void Activate() override;
	virtual void BeginDestroy() override;

private:
	void Raise(EGcPlayEndReason Reason);

	FDelegateHandle Binding;
};
