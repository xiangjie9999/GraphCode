// Copyright OraStudio. All Rights Reserved.
//
// "Get in Unreal" (Docs/GC-MARKET-DESIGN.md §0, owner 2026-11-20): a pack page on GraphCode.dev/marketplace links
// `graphcode://market/install/<Publisher>/<Name>`. Windows starts the handler shipped in Production
// (graphcode-bridge.exe --market-link "<link>", registered per user under HKCU\Software\Classes\graphcode by the
// editor itself, the product's install step). The handler refuses anything but that one form, then either
// hands the link to every open GraphCode editor (POST /graphcode/market/link on the editor's MCP port, found
// through the per-user editor descriptors below) or, with none open, leaves it in the per-user store for the
// next editor to take at its start (a day at most). The editor opens Window > GraphCode Market on the pack with
// its install confirmation already up: ONE click installs, and a link alone never installs (lead's ruling).
//
// Everything here lives under %LOCALAPPDATA%\GraphCode\MarketLinks; nothing writes into a project.
#pragma once

#include "CoreMinimal.h"

namespace GcMarket::Link
{
	/** The handler's argument in the registered command: `"<exe>" --market-link "%1"`. */
	GRAPHCODE_API extern const TCHAR* const HandlerArgument;

	/** A stored request older than this is dropped unseen (the browser click was a day ago). */
	GRAPHCODE_API FTimespan MaxAge();

	/**
	 * The ONE accepted form: `graphcode://market/install/<Publisher>/<Name>`, an optional trailing `/`. The scheme,
	 * `market` and `install` are read without case; each id segment is percent-decoded ONCE and must then be R2's
	 * PascalCase alphanumerics (1-64, A-Z first). Refused, with the reason in plain words: another scheme or host,
	 * user/port, a query or fragment, a backslash, `.`/`..`, an empty or extra segment, an encoded `/`, a bad or
	 * double escape, control characters, anything over 512 characters.
	 */
	GRAPHCODE_API bool Parse(const FString& Link, FString& OutId, FString& OutReason);

	/** `graphcode://market/install/<Publisher>/<Name>` for a valid id (what the site writes). */
	GRAPHCODE_API FString LinkFor(const FString& Id);

	// ---- The per-user store (the handler writes it; the editor takes it) ----

	/** `%LOCALAPPDATA%/GraphCode/MarketLinks`: pending links `link-*.json`, open editors `editors/<pid>.json`. */
	GRAPHCODE_API FString StoreDir();

	/**
	 * Write one pending request `link-<utc ticks>-<guid>.json` {format:1, link, pack, created}, the handler's format.
	 * An earlier pending request for the same pack is replaced. Refuses a link Parse refuses (nothing written).
	 */
	GRAPHCODE_API bool StorePending(const FString& Dir, const FString& Link, const FDateTime& NowUtc, FString& OutError);

	struct FTakeResult
	{
		/** The newest valid request, when there is one. */
		FString Id;
		FString Link;
		FDateTime Created;
		/** Older than MaxAge (or dated in the future), dropped. */
		int32 Expired = 0;
		/** Unreadable, or a link Parse refuses: dropped, never acted on. */
		int32 Refused = 0;
		/** Valid but older than the newest: dropped (one confirmation at a time). */
		int32 Replaced = 0;
		/** One line per dropped file, for the log. */
		TArray<FString> Notes;

		bool HasLink() const { return !Id.IsEmpty(); }
	};

	/**
	 * Claim every pending request (each file is renamed to a claim name first, so two editors never take the same one),
	 * delete it, and return the newest that Parse accepts and MaxAge allows. Files that are not `link-*.json` are left.
	 */
	GRAPHCODE_API FTakeResult TakePending(const FString& Dir, const FDateTime& NowUtc);

	/** Delete pending requests for Id (an editor that has just shown it). Returns how many. */
	GRAPHCODE_API int32 DropPendingFor(const FString& Dir, const FString& Id);

	/** `<Dir>/editors/<pid>.json` {format:1, url, port, processId, projectDir, projectName}: this editor can show links. */
	GRAPHCODE_API bool AnnounceEditor(const FString& Dir, uint32 Port, FString& OutError);
	/** Remove this process's descriptor (only its own). */
	GRAPHCODE_API void WithdrawEditor(const FString& Dir);

	/** Whether this process can show the tab: a Slate editor, not a commandlet, not unattended, not an automation run. */
	GRAPHCODE_API bool CanShowLinks();

	// ---- The editor ----

	/**
	 * Open Window > GraphCode Market on Id with its install confirmation up (GraphCodeMarketTab.cpp). OutState is
	 * the tab's word (confirm | installed | update | unknown | refused | no_catalog | loading), OutWords what it says.
	 * False with the reason when this process has no window to show it in.
	 */
	GRAPHCODE_API bool ShowInEditor(const FString& Id, FString& OutState, FString& OutWords);

	/**
	 * The `/graphcode/market/link` route's work, without HTTP: Body is `{"link":"graphcode://..."}`. Returns the reply
	 * JSON {shown, pack, state, words} or {shown:false, error}; OutCode is 200, 400 (refused link, nothing done) or
	 * 409 (this editor has no window). Pending requests for the same pack are dropped when it is shown.
	 */
	GRAPHCODE_API FString ReceiveRequest(const FString& Body, int32& OutCode);

	// ---- Registration (the product's install step, per user, no admin) ----

	/**
	 * The handler an INSTALLED GraphCode ships, found from the loaded GraphCode module's file: the plugin folder
	 * holding it (GraphCode.uplugin) must carry RELEASE-MANIFEST.json (Production, or a recipient's copy of it) and
	 * `Resources/mcp-bridge/graphcode-bridge.exe`. False with the reason otherwise (a source checkout's own build).
	 */
	GRAPHCODE_API bool InstalledHandler(const FString& LoadedModuleFile, FString& OutExe, FString& OutWhyNot);

	/** `"<exe>" --market-link "%1"`. */
	GRAPHCODE_API FString CommandFor(const FString& Exe);

	/**
	 * Write HKCU\<ClassesKey> as a URL protocol whose open command is CommandFor(Exe): (Default) "URL:GraphCode link",
	 * "URL Protocol" "", DefaultIcon, shell\open\command. Rewrites only what differs; bOutChanged says whether it did.
	 * The product passes `Software\Classes\graphcode`; a spec passes a scratch key and deletes it.
	 */
	GRAPHCODE_API bool Register(const FString& ClassesKey, const FString& Exe, bool& bOutChanged, FString& OutError);

	/** Read back the open command under HKCU\<ClassesKey>; empty when absent. */
	GRAPHCODE_API FString RegisteredCommand(const FString& ClassesKey);

	/** Delete HKCU\<ClassesKey> and everything under it (specs only). */
	GRAPHCODE_API bool Unregister(const FString& ClassesKey);

	/**
	 * Module startup (GUI editors only, CanShowLinks): register the scheme to the installed handler, then, once the
	 * editor is up, take a pending request and show it.
	 */
	void InstallForEditor();
	void UninstallForEditor();
}
