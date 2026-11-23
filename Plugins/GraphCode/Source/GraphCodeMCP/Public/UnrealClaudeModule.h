// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "UnrealClaudeConstants.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealClaude, Log, All);

class FUnrealClaudeMCPServer;

class FUnrealClaudeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Get the singleton instance */
	static FUnrealClaudeModule& Get();

	/** Check if module is available */
	static bool IsAvailable();

	/** Get the MCP server instance */
	TSharedPtr<FUnrealClaudeMCPServer> GetMCPServer() const { return MCPServer; }

	/** Get the active MCP server port, or the configured preferred port before startup. */
	static uint32 GetMCPServerPort();

private:
	void StartMCPServer();
	void StopMCPServer();

	// NO UI HANDLES. The command list and the dock tab that used to live here belonged to the
	// Claude Assistant panel, dropped 2026-08-27 — see the note at the top of the .cpp.
	TSharedPtr<FUnrealClaudeMCPServer> MCPServer;
};
