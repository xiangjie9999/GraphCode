// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Constants used throughout the UnrealClaude plugin
 * Centralizes magic numbers and configuration values
 */
namespace UnrealClaudeConstants
{
	namespace Process
	{
		/** Buffer size for reading process output */
		constexpr int32 OutputBufferSize = 4096;

		/** Timeout in milliseconds when waiting for process */
		constexpr int32 WaitTimeoutMs = 100;

		/** Default timeout in seconds for Claude CLI execution */
		constexpr float DefaultTimeoutSeconds = 300.0f;
	}

	namespace UI
	{
		/** Maximum height for input text area */
		constexpr float MaxInputHeight = 300.0f;

		/** Minimum height for input text area */
		constexpr float MinInputHeight = 60.0f;

		/** Default permission dialog width */
		constexpr float PermissionDialogWidth = 700.0f;

		/** Default permission dialog height */
		constexpr float PermissionDialogHeight = 500.0f;

		/** Maximum script preview length in characters */
		constexpr int32 MaxScriptPreviewLength = 2000;
	}

	namespace Session
	{
		/** Maximum number of exchanges to store in history */
		constexpr int32 MaxHistorySize = 50;

		/** Maximum number of history exchanges to include in prompt */
		constexpr int32 MaxHistoryInPrompt = 10;
	}

	namespace Context
	{
		/** Maximum UCLASS definitions to parse per search */
		constexpr int32 MaxUClassSearchLimit = 500;

		/** Maximum classes to format in context output */
		constexpr int32 MaxClassesToFormat = 30;

		/** Maximum directories to show in context output */
		constexpr int32 MaxDirectoriesToShow = 10;

		/** Maximum asset types to show in context output */
		constexpr int32 MaxAssetTypesToShow = 15;

		/** Maximum distance (in characters) to search for 'class' keyword after UCLASS macro */
		constexpr int32 MaxUClassToClassKeywordDistance = 500;

		/** Maximum distance (in characters) to search for parent class after class name */
		constexpr int32 MaxClassNameToInheritanceDistance = 50;
	}

	namespace AnimDiagram
	{
		/** Initial X position for condition nodes in transition graphs */
		constexpr int32 ConditionNodeStartX = 100;

		/** Initial Y position for condition nodes in transition graphs */
		constexpr int32 ConditionNodeStartY = 100;

		/** Horizontal spacing between condition nodes (e.g., TimeRemaining to comparison) */
		constexpr int32 ConditionNodeSpacing = 200;

		/** Width of each state cell in ASCII diagram output */
		constexpr int32 DiagramCellWidth = 20;

		/** Height of each state cell row in ASCII diagram output */
		constexpr int32 DiagramCellHeight = 3;

		/** Maximum state name length before truncation in diagrams */
		constexpr int32 MaxStateNameDisplayLength = 12;
	}

	namespace MCPValidation
	{
		/** Characters that are dangerous in actor names, paths, and class paths
		 *  Used for injection attack prevention and input sanitization
		 */
		inline constexpr const TCHAR* DangerousChars = TEXT("<>|&;`$(){}[]!*?~");

		/** Maximum length for actor names */
		constexpr int32 MaxActorNameLength = 256;

		/** Maximum length for property paths */
		constexpr int32 MaxPropertyPathLength = 512;

		/** Maximum length for class paths */
		constexpr int32 MaxClassPathLength = 1024;

		/** Maximum length for console commands */
		constexpr int32 MaxCommandLength = 2048;

		/** Maximum length for filter strings */
		constexpr int32 MaxFilterLength = 256;

		/** Default actor query limit */
		constexpr int32 DefaultActorLimit = 100;
	}

	namespace NumericBounds
	{
		/** Maximum absolute value for coordinate values */
		constexpr double MaxCoordinateValue = 1e10;
	}

	namespace ScriptExecution
	{
		/** Maximum wait time for Live Coding compilation in seconds */
		constexpr float MaxCompileWaitSeconds = 60.0f;

		/** Poll interval when waiting for compilation in seconds */
		constexpr float CompilePollIntervalSeconds = 0.5f;

		/** Maximum script history entries to retain */
		constexpr int32 MaxHistorySize = 100;

		/** Default number of recent scripts to return */
		constexpr int32 DefaultHistoryCount = 10;

		/** Maximum scripts to return in history query */
		constexpr int32 MaxHistoryQueryCount = 50;
	}

	namespace MCPServer
	{
		/** Default port for MCP HTTP server */
		constexpr uint32 DefaultPort = 3000;

		/** Timeout for game thread execution in milliseconds */
		constexpr uint32 GameThreadTimeoutMs = 30000;

		/** Default output log lines to return */
		constexpr int32 DefaultOutputLogLines = 100;

		/** Maximum output log lines to return */
		constexpr int32 MaxOutputLogLines = 1000;

		/**
		 * Maximum HTTP request body size in bytes (4 MB; 1 MB until B1, 13 Nov gap report P253).
		 *
		 * WHAT ACTUALLY BOUNDS A BODY, read rather than assumed. The engine's HTTP server sets no cap of
		 * its own: it trusts Content-Length (an int32) and reads the whole body into memory, ONE 64 KB
		 * `Recv` per connection per editor tick (`FHttpConnectionRequestReadContext::ReadStream`). A
		 * backgrounded editor ticks at 3 Hz (`UEditorEngine::GetMaxTickRate` when CPU-throttled), so a
		 * body arrives at about 192 KB/s there, and the bridge gives the whole request 30 s by default
		 * (`MCP_REQUEST_TIMEOUT_MS`). 4 MB is ~21 s of that, leaving the tool its own time; a larger
		 * cap would accept bodies a background editor cannot finish reading before the bridge gives up.
		 * Memory is not the binding limit: the handler holds the body, a null-terminated copy, its
		 * UTF-16 string and the parsed JSON — about 6x, so ~25 MB at the cap.
		 *
		 * A large GraphCode write never needs a large body (`RequestBodyTooLargeMessage` says how).
		 */
		constexpr int32 MaxRequestBodySize = 4 * 1024 * 1024;

		/**
		 * Expected MCP tools that should be registered at startup.
		 *
		 * EVERY NAME HERE MUST BE ONE `FMCPToolRegistry` ACTUALLY REGISTERS. The editor's health
		 * panel walks this list against the live registry and prints what it cannot find, so a name
		 * that outlives its tool is not a stale comment — it is a permanent warning about a tool
		 * nobody is missing. `bp_read`, `bp_describe_node` and `bp_apply` were deleted by C22 T2 on
		 * 2026-08-06 and sat here for a fortnight after it, so the panel reported three missing
		 * tools to every user who opened it and its "all available" branch could not be reached.
		 * `UnrealClaude.MCP.Registry.EveryExpectedToolIsRegistered` holds the list to the registry
		 * now (CP2 F24).
		 */
		inline const TArray<FString> ExpectedTools = {
			// Actor tools
			TEXT("spawn_actor"),
			TEXT("get_level_actors"),
			TEXT("delete_actors"),
			TEXT("move_actor"),
			TEXT("set_property"),
			// Utility tools
			TEXT("run_console_command"),
			TEXT("get_output_log"),
			TEXT("capture_viewport"),
			TEXT("execute_script"),
			TEXT("cleanup_scripts"),
			TEXT("get_script_history"),
			// Blueprint tools
			TEXT("blueprint_query"),
			TEXT("blueprint_modify"),
			TEXT("anim_blueprint_modify"),
			// Blueprint IR adapter tools. The two that read and wrote whole graphs, and the one
			// that built a hypothetical node, merged into gc_read/gc_apply/gc_describe; these two
			// stayed because gc_search kind:"node"/"identity" forwards to them.
			TEXT("bp_find"),
			TEXT("bp_search_nodes"),
			// Asset tools
			TEXT("asset_search"),
			TEXT("asset_dependencies"),
			TEXT("asset_referencers"),
			TEXT("asset"),
			// Character tools
			TEXT("character"),
			TEXT("character_data"),
			// Enhanced Input tools
			TEXT("enhanced_input"),
			// Material tools
			TEXT("material"),
			// Level management tools
			TEXT("open_level"),
			// Task queue tools
			TEXT("task_submit"),
			TEXT("task_status"),
			TEXT("task_result"),
			TEXT("task_list"),
			TEXT("task_cancel")
		};
	}

	namespace ClipboardImage
	{
		/** Maximum age of saved clipboard screenshots before cleanup (seconds) */
		constexpr double MaxScreenshotAgeSeconds = 3600.0;

		/** Subdirectory under Saved/UnrealClaude/ for clipboard screenshots */
		inline constexpr const TCHAR* ScreenshotSubdirectory = TEXT("screenshots");

		/** Thumbnail preview size in the input area (pixels) */
		constexpr float ThumbnailSize = 64.0f;

		/** Maximum number of images that can be attached to a single message */
		constexpr int32 MaxImagesPerMessage = 5;

		/** Maximum file size per image in bytes (4.5 MB; Claude API limit is 5 MB) */
		constexpr int64 MaxImageFileSize = 4608 * 1024;

		/** Maximum total image payload size in bytes (20 MB) */
		constexpr int64 MaxTotalImagePayloadSize = 20 * 1024 * 1024;

		/** Spacing between thumbnail previews in the image strip (pixels) */
		constexpr float ThumbnailSpacing = 4.0f;
	}
}
