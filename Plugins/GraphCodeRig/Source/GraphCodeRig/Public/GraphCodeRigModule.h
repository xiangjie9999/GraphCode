// Copyright OraStudio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/** The product's own log. `Verbose` is where a measurement lives and `Log` is where a refusal does. */
DECLARE_LOG_CATEGORY_EXTERN(LogGraphCodeRig, Log, All);

/**
 * GraphCode Rig — the eighth product. `.gcrig`, a program that makes a mesh animatable.
 *
 * `GC-RIG-GRAMMAR.md` is the grammar and it is canonical; `GC-RIG-EXAMPLE.md` is the worked file and
 * `GC-RIG-LEDGER.md` §0 carries every user ruling behind both.
 *
 * **IT IS ITS OWN PLUGIN BECAUSE THE USER RULED SO, AND THE RULING MADE THE DEPENDENCIES CLEAN.**
 * A `.gcrig` that built its own surface would have needed Shape's geometry engine, and
 * `GraphCodeRig -> GraphCodeShape` would have been the first product-to-product edge in the tree.
 * Ruling 8 — *"apply gc shape first, then gc rig, or straightaway use gc rig only on imported
 * meshes"* — means the mesh arrives already built, so this module depends on exactly what Shape
 * depends on and on Shape not at all. **Shape and Rig compose by handing an asset between them, not
 * by linking**, and a customer can buy either.
 *
 * **THIS STEP IS THE PARSER, and it deliberately builds nothing.** The build order in the grammar's
 * §12 puts setting A first — bind what arrives, write the skeleton, the limits, the physics asset,
 * the IK rig and the sockets — because that is pipeline 1.1 end to end and it carries no unproven
 * claim. A parser with refusals is what everything after it stands on.
 */
class FGraphCodeRigModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
