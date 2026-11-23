// Copyright OraStudio. All Rights Reserved.
//
// The GraphCode Market's test seam (Docs/GC-MARKET-DESIGN.md R8): a spec builds a catalog in a temp folder and
// signs it with an ephemeral P-256 key it registers through GcReleaseTrust's seam. The key lives only in this
// process, is honoured only by the market and only while an automation run is in progress, and is forgotten
// when the FTestKey goes. Nothing here touches the network or the person's key store.
#pragma once

#include "CoreMinimal.h"
#include "GraphCodeMarket.h"

namespace GcMarket::Testing
{
	class GRAPHCODE_API FTestKey
	{
	public:
		FTestKey();
		~FTestKey();
		FTestKey(const FTestKey&) = delete;
		FTestKey& operator=(const FTestKey&) = delete;

		bool IsValid() const { return !X.IsEmpty(); }
		const FString& Kid() const { return KidText; }
		/** ES256, base64url P1363, over exactly these bytes. Empty when signing failed. */
		FString Sign(const TArray<uint8>& Bytes) const;

		FString X;
		FString Y;

	private:
		FString KidText;
		void* Handle = nullptr;
	};

	/** One pack of a spec's catalog: its §3 fields and its files' text. */
	struct GRAPHCODE_API FSpecPack
	{
		FString Id;
		FString Version = TEXT("1.0.0");
		FString Kind = TEXT("shape");
		FString Title;
		FString Summary;
		FString Description;
		FString Main;
		FString Published = TEXT("2026-11-19T00:00:00Z");
		TArray<FString> Tags;
		TArray<TPair<FString, FString>> Files;
		TArray<FDep> Depends;
		TArray<FRequire> Requires;
		TArray<FEntry> Entries;
		/** R17's build choices, written as the index's `choices`. */
		TArray<FChoice> Choices;
		/** R18/R19: form, library and a Move library's use line, written when set. */
		FString Form;
		bool bLibrary = false;
		FString Use;
		/** R20: `materials: true`, and the optional texture set (paths among Files) written as `textures`. */
		bool bMaterials = false;
		TArray<FString> TextureFiles;
		/** A thumbnail's bytes, written as thumb.png when set. */
		TArray<uint8> Thumbnail;
	};

	/**
	 * Write `<Root>/packs/<P>/<N>/<v>/<file>`, `<Root>/index.json` and `<Root>/index.sig.json`, signed by Key
	 * over "graphcode-market-index-v1\n" + the index bytes. The index lists each file's size and SHA-256.
	 */
	GRAPHCODE_API bool WriteCatalog(const FString& Root, const TArray<FSpecPack>& Packs, const FTestKey& Key, FString& OutError);

	/** A fresh temp folder under Saved/Automation, with a Content folder when asked (a project for FPlace). */
	GRAPHCODE_API FString MakeTempFolder(const FString& Label, bool bWithContent);

	/**
	 * Make every market write or move to a path the predicate accepts fail, for the scope: how the specs prove the
	 * rollbacks (a failure on file 2 of 3 leaves the old version whole; a remove that cannot move a file keeps it).
	 */
	class GRAPHCODE_API FScopedWriteFault
	{
	public:
		explicit FScopedWriteFault(TFunction<bool(const FString& Path)> Fails);
		~FScopedWriteFault();
	};

	/** Points the session at a catalog root for the scope (R8), and puts back what was there. */
	class GRAPHCODE_API FScopedCatalogRoot
	{
	public:
		explicit FScopedCatalogRoot(const FString& Root);
		~FScopedCatalogRoot();

	private:
		FString Previous;
	};
}
