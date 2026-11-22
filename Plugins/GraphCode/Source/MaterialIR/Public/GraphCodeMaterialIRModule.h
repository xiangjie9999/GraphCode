#pragma once

#include "Modules/ModuleManager.h"

/** Bootstrap only; named away from UE's internal shader-compiler FMaterialIRModule. */
class FGraphCodeMaterialIRModule final : public IModuleInterface
{
};
