// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGraphCodeGarment, Log, All);

/**
 * GraphCode Garment — the ninth product. `.gcgarment`: rings on a body and the garments between them
 * (`Docs/GC-GARMENT-GRAMMAR.md`; the build order is `Docs/GC-GARMENT-BUILD-PLAN.md`). Its own plugin,
 * user-ruled 2026-09-24: *"gc garment is again its own module, just like shape, move etc."*
 */
class FGraphCodeGarmentModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
