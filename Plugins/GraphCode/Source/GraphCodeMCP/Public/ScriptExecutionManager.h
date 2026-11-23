// Copyright Natali Caggiano. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ScriptTypes.h"

/**
 * Manages script execution pipeline:
 * - Permission dialog flow
 * - Script compilation (C++ via Live Coding)
 * - Script execution (Python, Console, Editor Utility)
 * - Persistent history log
 * - Auto-retry on build failure
 */
class GRAPHCODEMCP_API FScriptExecutionManager
{
public:
	/** Get singleton instance */
	static FScriptExecutionManager& Get();

	FScriptExecutionManager();
	~FScriptExecutionManager();

	/**
	 * Execute a script with permission flow
	 * @param Type - Script type (Cpp, Python, Console)
	 * @param ScriptContent - The script code (MUST include header comment with @Description)
	 * @param Description - Brief description (used if not in header)
	 * @return Execution result
	 */
	FScriptExecutionResult ExecuteScript(
		EScriptType Type,
		const FString& ScriptContent,
		const FString& Description = TEXT("")
	);

	/**
	 * Get recent script history for Claude context
	 * @param Count - Number of recent scripts to return
	 * @return Array of history entries
	 */
	TArray<FScriptHistoryEntry> GetRecentScripts(int32 Count = 10) const;

	/**
	 * Format recent scripts for Claude context prompt
	 * @param Count - Number of recent scripts to include
	 * @return Formatted string for system prompt
	 */
	FString FormatHistoryForContext(int32 Count = 10) const;

	/** Clear all script history */
	void ClearHistory();

	/** Save history to disk */
	bool SaveHistory();

	/** Load history from disk */
	bool LoadHistory();

	/**
	 * Cleanup all generated scripts and history
	 * @return Result message
	 */
	FString CleanupAll();

	/** Get path to script history JSON file */
	FString GetHistoryFilePath() const;

	/** Get directory for generated C++ scripts */
	FString GetCppScriptDirectory() const;

	/**
	 * Get directory for Python/other scripts — `Saved/UnrealClaude/Scripts/`.
	 *
	 * **MOVED OUT OF `Content/` ON 2026-08-16, and the old name said where it was rather than what
	 * it held.** A generated script is not a UE asset: it is run by absolute path (`py "<path>"`),
	 * nothing loads it as a package, and the history that indexes it has always lived under
	 * `Saved/UnrealClaude/`. What `Content/` cost was a folder in every user's project — the
	 * constructor created it at startup whether or not a script was ever run — inside the one
	 * directory a user ships, source-controls and browses. `Saved/` is where a tool's own working
	 * files belong, and it is already gitignored by every UE project template.
	 */
	FString GetScriptDirectory() const;

	/**
	 * The file-name stem a script's description becomes (gap report 13 Nov, V3; card P294).
	 *
	 * **A DOT IS NOT A NAME CHARACTER HERE.** The engine's `py` command finds the FIRST ".py" in its
	 * argument (`PythonScriptPlugin.cpp`, `TryExtractPathnameAndCommand`), so a description holding
	 * ".py" made `x.py_041.py`, the command was read as Python code instead of a file, and the quoted
	 * path evaluated as a string: nothing ran and nothing failed. Only ASCII letters, digits, `-` and
	 * `_` survive; a trailing ".py" is dropped first; runs of `_` fold; at most 30 characters; empty
	 * becomes "Script".
	 */
	static FString SanitizeScriptBaseName(const FString& Description);

	/** What the runner's closing line prints for this token — the positive evidence a run looks for. */
	static FString PythonEndSentinelText(const FString& Token);

	/** The closing line the runner appends to every Python script it writes. */
	static FString PythonEndSentinelLine(const FString& Token);

	/**
	 * Run one Python file through `py "<file>"` and judge it by evidence, not by the absence of an error.
	 *
	 * Success needs the file's closing line (`PythonEndSentinelLine(EndToken)`) in the captured output.
	 * With no closing line and no error marker the run is a FAILURE whose message starts
	 * "Python script did not run" and names the file. Writes no history; `ExecutePython` does.
	 */
	static FScriptExecutionResult RunPythonFile(const FString& FilePath, const FString& EndToken);

private:
	/** Execute C++ script via Live Coding */
	FScriptExecutionResult ExecuteCpp(const FString& ScriptContent, const FString& Description);

	/** Execute Python script */
	FScriptExecutionResult ExecutePython(const FString& ScriptContent, const FString& Description);

	/** Execute console command(s) */
	FScriptExecutionResult ExecuteConsole(const FString& ScriptContent, const FString& Description);

	/**
	 * Show permission dialog to user
	 * @param ScriptPreview - Code to display in dialog
	 * @param Type - Script type
	 * @param Description - Script description
	 * @param OutRefusal - Why approval failed, phrased for the caller. A dialog that was shown and
	 *        declined says the user declined; a host that could not show one says so instead.
	 * @return true if user approved, false if denied
	 */
	bool ShowPermissionDialog(const FString& ScriptPreview, EScriptType Type, const FString& Description, FString& OutRefusal);

	/**
	 * Trigger Live Coding compilation
	 * @param OutErrorLog - Compilation error output
	 * @return true if compilation succeeded
	 */
	bool TriggerLiveCodingCompile(FString& OutErrorLog);

	/**
	 * Write script file to disk
	 * @param Content - Script content
	 * @param Type - Script type (determines directory)
	 * @param ScriptName - Name for the script file
	 * @return Full path to written file, or empty on failure
	 */
	FString WriteScriptFile(const FString& Content, EScriptType Type, const FString& ScriptName);

	/**
	 * Generate unique script name
	 * @param Type - Script type
	 * @param Description - Description to base name on
	 * @return Sanitized unique script name
	 */
	FString GenerateScriptName(EScriptType Type, const FString& Description);

	/**
	 * Add entry to history
	 */
	void AddToHistory(const FScriptHistoryEntry& Entry);

	/** Script execution history */
	TArray<FScriptHistoryEntry> History;

	/** Maximum history entries to keep */
	int32 MaxHistorySize;

	/** Script counter for unique naming */
	int32 ScriptCounter;
};
