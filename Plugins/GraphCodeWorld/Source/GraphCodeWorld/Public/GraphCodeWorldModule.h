#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * GraphCode World — Environment systems, and the level facets that only matter at streaming scale.
 *
 * Niagara systems and emitters, Sound Cues and attenuation, PCG graphs, Landscape, Foliage
 * types, HLOD setups and Level Sequences — plus the three level facets Core deliberately
 * leaves out: `sublevels`, World Partition and `built_data`. `GC-PRODUCT-SPLIT.md` §5.
 *
 * The level DOCUMENT stays Core's by an explicit ruling (§2), so World's level work is
 * `<facet>@world` on a document Core writes — the first real user of the FACET scope.
 *
 * **WHY THIS MODULE EXISTS BEFORE ITS CODE DOES.** A product is installed when its module has
 * REGISTERED with Core, not when its folder is on disk (`GC-PRODUCT-PACKAGING-DESIGN.md` §3), and
 * until something registers, `FGraphCodeProducts::IsInstalled` is a constant and the gate that
 * decides what a Core-only customer sees cannot be exercised anywhere. This module is that
 * registration. It is also where the codecs will register, in the same call, so a product's
 * presence and its implementations cannot come apart.
 *
 * Its first registered implementation is N2a region navigation: stable bounds/blocker actors,
 * Recast build, coverage and stale receipts. Connectivity measurement remains N2.
 */
class FGraphCodeWorldModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
