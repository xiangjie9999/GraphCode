#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * GraphCode Move — Animation, state machines, and the AI graphs that decide and drive motion.
 *
 * `animgraph { }` on `kind:animation_blueprint` was the first facet declared to this product and is
 * joined by `animations { }` on Widget Blueprints. Animation Montages, Blend Spaces, Skeletons and
 * their sockets, Control Rig, Physics Assets, IK Rig/Retargeter, Niagara, cinematic/Level Sequence
 * authoring, Sound Cue sequencing and the Behavior Tree / Blackboard / StateTree family follow.
 * MetaSound explicitly belongs to Core Extension. `GC-PRODUCT-SPLIT.md` §4 owns the boundary.
 *
 * **WHY THIS MODULE EXISTS BEFORE ITS CODE DOES.** A product is installed when its module has
 * REGISTERED with Core, not when its folder is on disk (`GC-PRODUCT-PACKAGING-DESIGN.md` §3), and
 * until something registers, `FGraphCodeProducts::IsInstalled` is a constant and the gate that
 * decides what a Core-only customer sees cannot be exercised anywhere. This module is that
 * registration. It is also where the codecs will register, in the same call, so a product's
 * presence and its implementations cannot come apart.
 *
 * Currently blocked on: GC-PRODUCT-SPLIT.md §4.1 steps 1-3 are in flight in Core and move here once they land.
 */
class FGraphCodeMoveModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
