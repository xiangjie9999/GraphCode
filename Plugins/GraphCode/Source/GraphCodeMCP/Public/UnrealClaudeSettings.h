// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UnrealClaudeSettings.generated.h"

/**
 * Project-level settings for the UnrealClaude plugin.
 *
 * Edit in: Project Settings -> Plugins -> Unreal Claude.
 * Stored in: Config/DefaultEditor.ini under [/Script/GraphCodeMCP.UnrealClaudeSettings].
 */
UCLASS(Config=Editor, DefaultConfig, meta=(DisplayName="Unreal Claude"))
class GRAPHCODEMCP_API UUnrealClaudeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUnrealClaudeSettings() = default;

	/**
	 * Auto-approve all script execution requests instead of showing the per-call permission dialog.
	 *
	 * When OFF (default), every Python / C++ / Console / Editor Utility script triggered through the
	 * MCP bridge or the in-editor Claude Assistant chat must be approved by the user via a modal dialog.
	 * That is the safest default and matches the existing behaviour.
	 *
	 * Turn ON to opt into a "trusted" workflow where the host (Claude Code session, an MCP-driven agent,
	 * or an automated test harness) drives many scripts in sequence and the per-script confirmation is
	 * the dominant friction. Approvals are skipped silently and an INFO log line is written instead so
	 * the audit trail is preserved.
	 *
	 * WARNING: enabling this means every script the connected agent submits runs immediately without
	 * human confirmation. Only enable on machines and projects where the connected agent is trusted.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Scripts", meta=(DisplayName="Auto-approve script execution"))
	bool bAutoApproveScripts = false;

	/**
	 * Show the in-editor Claude Assistant panel, its toolbar button and its Tools-menu entries.
	 *
	 * **OFF BY DEFAULT, AND THAT IS THE PRODUCT DECISION RATHER THAN A PREFERENCE.** What ships is
	 * a bridge: a customer connects their OWN MCP client — Claude Code, Claude Desktop, Cursor,
	 * Codex — and the editor is the thing being driven, not a place to chat. The panel is a second,
	 * provider-specific way in that needs the `claude` CLI on PATH, and a customer who never asked
	 * for it should not find a "Claude Assistant" tab in a plugin they installed for `.gc`
	 * sidecars.
	 *
	 * Nothing about the MCP server depends on this. The HTTP endpoint, the tool surface and
	 * `Saved/UnrealAIBridge/` are unaffected either way — this switch reaches the Slate
	 * registration and nothing else.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Assistant",
		meta=(DisplayName="Show the in-editor Claude Assistant panel"))
	bool bShowAssistantPanel = false;

	/** Preferred localhost port for the provider-neutral MCP HTTP bridge. */
	UPROPERTY(EditAnywhere, Config, Category="Bridge",
		meta=(DisplayName="MCP server port", ClampMin="1024", ClampMax="65535", UIMin="1024", UIMax="65535"))
	int32 MCPServerPort = 3000;

	/**
	 * Number of consecutive ports to try when the preferred port is already in use.
	 * The selected endpoint is published to Saved/UnrealAIBridge/endpoint.json so
	 * MCP clients can discover it without provider-specific coordination.
	 */
	UPROPERTY(EditAnywhere, Config, Category="Bridge",
		meta=(DisplayName="MCP port scan count", ClampMin="1", ClampMax="32", UIMin="1", UIMax="16"))
	int32 MCPPortScanCount = 11;

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

#if WITH_EDITOR
	virtual FText GetSectionText() const override { return NSLOCTEXT("UnrealClaude", "SettingsSectionText", "Unreal Claude"); }
#endif
};
