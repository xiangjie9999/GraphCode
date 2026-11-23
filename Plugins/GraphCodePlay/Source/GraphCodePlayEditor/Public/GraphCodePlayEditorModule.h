#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Registers GraphCode Play with Core, and holds whatever else genuinely needs an editor.
 *
 * **A PRODUCT IS INSTALLED WHEN ITS MODULE HAS REGISTERED, not when its folder is on disk**
 * (`GC-PRODUCT-PACKAGING-DESIGN.md` §3). Until something registers,
 * `FGraphCodeProducts::IsInstalled` is a constant and the gate that decides what a Core-only
 * customer sees cannot be exercised anywhere. This module is that registration, and it is also
 * where Play's codecs will register, in the same call, so a product's presence and its
 * implementations cannot come apart.
 *
 * **WHY THE REGISTRATION IS HERE AND NOT IN THE RUNTIME MODULE.** It used to be in
 * `GraphCodePlay`, which is what kept that module `Editor` and therefore kept Play out of a
 * packaged build entirely. The registry is read by the capability manifest and the MCP bridge;
 * a shipped title has neither, so it has nothing to answer and nothing to register. The runtime
 * half registers nothing, and that is a statement about what the registry is for rather than a
 * workaround for a build rule.
 */
class FGraphCodePlayEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
