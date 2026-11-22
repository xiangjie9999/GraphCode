#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "EngineUtils.h"
#include "GameFramework/Actor.h"

/**
 * The half of `GraphCodeTestBlocks.h` that knows about a WORLD rather than about a document.
 *
 * **SPLIT OUT 2026-08-14, when the capture writer moved to GraphCode Play.** That spec is one of
 * `FindActor`'s callers and it now lives in a different plugin, so the choice was between a second
 * definition and a shared home. The header this came from is explicit about which of those is
 * correct — it exists *because* three specs had each declared their own `FindActor`, and it records
 * that the first attempt at that fix renamed a copy instead of deleting it: **"A rename moves a
 * latent duplicate; only one definition removes it."** A fourth copy in another module would not
 * even collide in a unity build, which makes it worse rather than better: it could drift for months
 * with nothing failing.
 *
 * Here rather than beside `FindBlock` because the two have different reach. `FindBlock` and
 * `Joined` read `FGraphCodeDocument`, which is Core's; this one reads a `UWorld`, which every
 * product has. `GraphCodeRuntime` is the module Core and Play already share, so it is the only
 * place one definition can serve both.
 *
 * Test-only, and the guard is the whole file: nothing here reaches a Shipping build.
 */
namespace GraphCodeTest
{
	/** By the actor's internal name - the handle `actors { }` spells (WORLD-GC-DESIGN.md §13). */
	inline AActor* FindActor(UWorld* World, const TCHAR* Name)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (IsValid(*It) && It->GetName() == Name)
			{
				return *It;
			}
		}
		return nullptr;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
