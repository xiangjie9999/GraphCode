#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FGraphCodeModule : public IModuleInterface
{
public:
	virtual ~FGraphCodeModule() override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Flush all save-triggered work synchronously. Used by pre-exit and commandlets. */
	GRAPHCODE_API void DrainPendingSidecars();

private:
	TUniquePtr<class FGraphCodeService> Service;
};
