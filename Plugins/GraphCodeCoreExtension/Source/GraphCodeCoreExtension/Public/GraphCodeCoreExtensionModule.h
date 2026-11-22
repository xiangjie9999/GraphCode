#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * GraphCode Core Extension — framework-backed authoring that extends the universal Core surface.
 *
 * Owns MVVM, MetaSound, CommonUI and Gameplay Ability System. Each engine plugin remains optional:
 * installing this product never turns a framework on in a customer's project, and an adapter only
 * registers when its framework is available.
 *
 * The module registers the fifth product now. Codec migration/implementation follows through the
 * existing product seam; until a codec has actually moved, Core must keep reporting its present
 * implementation honestly rather than claiming an unavailable `@extension` facet.
 */
class FGraphCodeCoreExtensionModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
