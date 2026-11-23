// The MCP client configuration for THIS project's editor — the four lines a customer would
// otherwise have to compose by hand, and the single producer of them.
//
// ---------------------------------------------------------------------------
// WHY THIS FILE EXISTS
// ---------------------------------------------------------------------------
// The plugin has always been able to write a correct MCP config: `ClaudeCodeRunner` generates one
// every time the in-editor panel launches Claude Code, and hands it over with `--mcp-config`. So a
// customer using the panel is configured automatically and a customer using their OWN client — the
// Claude Code CLI in a terminal, Claude Desktop, Cursor, Codex — had to hand-write the same JSON
// with an absolute path to a file four directories inside the plugin, while the editor sitting in
// front of them already knew every value in it.
//
// This is that generator, lifted out so both callers share it. **Two carriers of one fact is a
// race** ([[two-carriers-of-one-fact-is-a-race]]): a second copy of this JSON would drift from the
// runner's the first time a key was renamed, and the copy that goes stale is the one a customer
// pastes.
//
// ---------------------------------------------------------------------------
// WHY THE ENTRY POINT IS PROVIDER-NEUTRAL
// ---------------------------------------------------------------------------
// Connected checkouts and shipped packages use the installed `graphcode-bridge.exe` so
// the implementation source is not distributed. They are the same provider-neutral bridge.
//
// ---------------------------------------------------------------------------
// WHY `UNREAL_PROJECT_PATH` IS ALWAYS WRITTEN
// ---------------------------------------------------------------------------
// The bridge finds the editor by reading `Saved/UnrealAIBridge/endpoint.json` under the project,
// and falls back to a port scan. With no explicit path it looks under its own WORKING DIRECTORY,
// which is the MCP client's — the project folder for a CLI launched there, and somewhere
// arbitrary for a desktop app. Writing it always is what makes one config work in both.

#pragma once

#include "CoreMinimal.h"

namespace UnrealClaude::MCPClientConfig
{
	/**
	 * The `mcpServers` JSON an MCP client needs to reach this editor, ready to paste.
	 *
	 * Reflects the CURRENT port, which is not necessarily the configured one: the server scans
	 * upward from the preferred port when it is taken, so a second editor is on a different port
	 * and a config generated from settings rather than from reality would point at the first one.
	 */
	GRAPHCODEMCP_API FString BuildJson();

	/** Where `Write` puts it: beside `endpoint.json`, which is the other file about reaching here. */
	GRAPHCODEMCP_API FString FilePath();

	/**
	 * Write it, and hand back what was written.
	 *
	 * @return false with `OutError` set when the bridge is missing from the plugin or the file
	 *         cannot be written. A caller that cannot write still gets `OutJson`, because the
	 *         clipboard is the half that matters and a read-only project should not lose it.
	 */
	GRAPHCODEMCP_API bool Write(FString& OutJson, FString& OutPath, FString& OutError);

	/** The native shipped bridge when present, otherwise the development JS entry point. */
	GRAPHCODEMCP_API FString BridgeEntryPointPath();
}
