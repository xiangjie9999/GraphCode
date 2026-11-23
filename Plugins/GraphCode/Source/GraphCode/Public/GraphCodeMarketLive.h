// Copyright OraStudio. All Rights Reserved.
//
// GraphCode Market 2 (Docs/GC-MARKET2-UPLOAD-DESIGN.md §7): the editor's verifier of the per-version wire formats and
// its copy of the live market. Written ONCE against the frozen vectors (Tools/Market/test/vectors/), in §7.2's and
// §7.3's order, and the codes are the vectors' `expect` words.
//
// What is trusted: a version RECORD (the exact UTF-8 text the release key signs, ES256 over
// "graphcode-market-pack-v1\n" + the text), its MANIFEST (pack.json bytes, trusted only through the record's
// manifest_sha256) and the HEAD (serial + blocklist, over "graphcode-market-head-v1\n" + the text). Every other field
// of an API answer (cards, states, serials) is display data and steers nothing that writes.
//
// The live copy (FState) is what the tab and the verb read: `changes?since=<serial>` applied page by page, each listed
// record verified with its manifest before it lands; tombstones (yanked, hidden, blocked) keep the record seen before
// so an installed copy stays verifiable; the highest head serial accepted is kept and a lower one refused
// (head_rollback). The copy is cached for offline use and re-verified, record by record, when it is read back.
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "GraphCodeMarket.h"

namespace GcMarket::Live
{
	/** The detached signature, release.sig.json's form: exactly {kid, alg, sig}. */
	struct FSignature
	{
		FString Kid;
		FString Alg;
		FString Sig;
	};

	/** §7.1's record, as verified. */
	struct FRecord
	{
		FString Id;
		FString Publisher;
		FString Name;
		FString Version;
		FString Kind;
		FString ManifestSha256;
		FString Signed;
		TArray<FFile> Files;
		TOptional<FFile> Thumbnail;
		TArray<FDep> Depends;
		/** Signed by a delegated key (reserved for accounts; refused unless the caller allows it). */
		bool bDelegated = false;
	};

	/** §7.3's head, as verified. */
	struct FHead
	{
		int64 Serial = 0;
		FString Issued;
		/** RESERVED: absent in this cut; a head carrying it is refused after that time. */
		FString Expires;
		TArray<FBlocked> Blocked;
	};

	struct FVerifyOptions
	{
		/** A spec's key (GcReleaseTrust's seam): honoured only while an automation run is in progress. */
		bool bAcceptTestKeys = false;
		/** A record signed under a delegation (§7.1). The live market refuses them in this cut, and so does the editor. */
		bool bAllowDelegation = false;
	};

	extern GRAPHCODE_API const TCHAR* const PackDomain;
	extern GRAPHCODE_API const TCHAR* const HeadDomain;
	extern GRAPHCODE_API const TCHAR* const DelegationDomain;

	/** The signature object's shape only (strings for kid/alg/sig and nothing else); its form is checked by Verify*. */
	GRAPHCODE_API bool ReadSignature(const TSharedPtr<FJsonValue>& Value, FSignature& Out, bool& bOutExactKeys);

	/** The exact bytes a signature covers: the domain line, then the text's UTF-8 bytes. */
	GRAPHCODE_API TArray<uint8> SigningBytes(const TCHAR* Domain, const FString& Text);

	/** SHA-256 of the record text's UTF-8 bytes (the "bundle hash" the head and the API name). */
	GRAPHCODE_API FString RecordSha256(const FString& RecordText);

	/**
	 * §7.2, steps 1-5 in order: the signature's form (malformed_signature), the record's rules (malformed_record), the
	 * key (delegation_invalid | unknown_key), ES256 (bad_signature), then, when Manifest is given, the manifest against
	 * the record (manifest_mismatch). Empty when valid, else that code; OutDetail says which rule.
	 */
	GRAPHCODE_API FString VerifyRecord(const FString& RecordText, const FSignature& Signature, bool bSignatureExactKeys,
		const FString* Manifest, const FVerifyOptions& Options, FRecord& Out, FString& OutDetail);

	/**
	 * §7.3: the signature's form, the key, ES256, the head's rules (malformed_head), then serial >= LastSerial
	 * (head_rollback; equal is fine) and, only when `expires` is present, NowUtc <= expires (head_expired). NowUtc is
	 * "YYYY-MM-DDTHH:MM:SSZ"; empty means now.
	 */
	GRAPHCODE_API FString VerifyHead(const FString& HeadText, const FSignature& Signature, bool bSignatureExactKeys, int64 LastSerial,
		const FString& NowUtc, bool bAcceptTestKeys, FHead& Out, FString& OutDetail);

	/** §7.3: an installed version is blocked when (id, version) OR its record_sha256 matches an entry. */
	GRAPHCODE_API const FBlocked* FindBlocked(const TArray<FBlocked>& Blocked, const FString& Id, const FString& Version,
		const FString& RecordSha256);

	// ---- The live copy ----

	/** One version the live market has said something about. Record/Signature/Manifest are kept once verified. */
	struct FVersion
	{
		FString Id;
		FString Version;
		/** listed | yanked | hidden | blocked */
		FString State;
		int64 Serial = 0;
		FString RecordSha256;
		FString Record;
		FSignature Signature;
		FString Manifest;
	};

	struct GRAPHCODE_API FState
	{
		/** The `changes` serial applied (next_since). */
		int64 Cursor = 0;
		/** The market serial the last answer carried (display). */
		int64 MarketSerial = 0;
		/** The highest verified head, with its text and signature to keep it verifiable. */
		TOptional<FHead> Head;
		FString HeadText;
		FSignature HeadSignature;
		/** "<id>@<version>" -> what the market said last. */
		TMap<FString, FVersion> Versions;
		FDateTime SyncedAtUtc;

		int64 HeadSerial() const { return Head.IsSet() ? Head->Serial : 0; }
	};

	/**
	 * One `changes` page into State. A listed version is believed only when its record verifies with its manifest; a
	 * page with one that does not changes nothing and says why (a broken or forged answer is never half-applied).
	 * Tombstones keep the record seen before. OutNextSince / bOutMore: where the next page starts and whether to ask.
	 */
	GRAPHCODE_API bool ApplyChanges(FState& State, const FString& ReplyJson, const FVerifyOptions& Options, int64& OutNextSince,
		bool& bOutMore, FString& OutError);

	/**
	 * A `head` answer {serial, head:{text, signature}|null}: verified, and refused below Highest (the highest serial
	 * this PC has accepted, head_rollback). A null head while none was ever accepted means nothing is blocked.
	 */
	GRAPHCODE_API bool ApplyHead(FState& State, const FString& ReplyJson, int64 Highest, bool bAcceptTestKeys, FString& OutError);

	/**
	 * The catalog the tab and the verb read: every LISTED version the head does not block, each pack from its verified
	 * manifest with the record's files and thumbnail (sizes included). States and the blocklist ride along.
	 */
	GRAPHCODE_API bool BuildCatalog(const FState& State, FCatalog& Out);

	/** The offline copy, and reading it back: every record and the head verified again, exactly as when received. */
	GRAPHCODE_API bool SaveState(const FString& Path, const FState& State);
	GRAPHCODE_API bool LoadState(const FString& Path, const FVerifyOptions& Options, FState& Out, FString& OutError);

	/** The highest head this PC has accepted, kept per user so every project sees a takedown and none rolls back. */
	/** Never over a held head with a higher serial (read under the same key rule); false when nothing was written. */
	GRAPHCODE_API bool SaveHead(const FString& Path, const FState& State, bool bAcceptTestKeys = false);
	GRAPHCODE_API bool LoadHead(const FString& Path, bool bAcceptTestKeys, FState& Out);
	/** `<user settings>/GraphCode/Market/head.json`, or a spec's folder while one is set. */
	GRAPHCODE_API FString HeadStorePath();
	GRAPHCODE_API void SetHeadStoreForSpecs(const FString& Path);
}
