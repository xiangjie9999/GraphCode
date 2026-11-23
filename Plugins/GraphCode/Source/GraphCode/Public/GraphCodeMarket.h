// Copyright OraStudio. All Rights Reserved.
//
// The GraphCode Market client (Docs/GC-MARKET-DESIGN.md R7-R12, §3, §5, §9). ONE client, two faces: the
// editor tab and the `gc_market` MCP verb both call this and neither has logic of its own (R9).
//
// What it does, in order, and the order is the safety: read a catalog (a local folder, R8, or the hosted
// routes, R7); verify the index's ES256 signature over "graphcode-market-index-v1\n" + its bytes before a
// single field of it is believed; resolve the exact-version dependency closure (R4); fetch every file and
// check every SHA-256 against the SIGNED index before anything is written; write each pack into
// Content/GraphCodeMarket/<Publisher>/<Name>/; record the lock (R10). Update refuses over files the person
// edited; remove moves the pack's text to the trash and refuses while another pack needs it. Every §5
// refusal carries a code, the pack, the file and the reason.
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "HAL/ThreadSafeBool.h"

namespace GcMarket
{
	/** §5's refusals, by name. `Code()` is the word the verb and the tab print. */
	enum class ERefusal : uint8
	{
		BadSignature,
		HashMismatch,
		BadId,
		PathEscape,
		Cycle,
		VersionConflict,
		MissingProduct,
		EditedFiles,
		NeededBy,
		NoContentFolder,
		MissingPack,
		NotInstalled,
		BadCatalog,
		BadQuery,
		Unreachable,
		WriteFailed,
		/** The person pressed Cancel before anything was written (§9.4). */
		Cancelled,
		/** market.lock.json cannot be read, or holds a row this client refuses; it is never rewritten then. */
		BadLock,
		/** A skill pack would overwrite a .claude/skills/<Name> folder the market did not write (R12). */
		SkillExists,
		/** R3 (corrected 2026-11-19): a pack file writes outside the pack's own folder, or reads outside it, its
		 *  dependencies' folders, /Engine and its products' content. Checked on the signed bytes, again at Build. */
		OutsidePack,
		/** MARKET2 §7.3: the signed head lists this version (a takedown): not installed, and an installed copy's Build is
		 *  refused. The project's files are never deleted for the person. */
		Blocked,
	};

	GRAPHCODE_API const TCHAR* RefusalCode(ERefusal Code);

	/** a.b.c numerically: A is newer than B. */
	GRAPHCODE_API bool IsNewer(const FString& A, const FString& B);

	struct GRAPHCODE_API FRefusal
	{
		ERefusal Code = ERefusal::BadCatalog;
		FString Pack;
		FString File;
		FString Reason;

		/** "hash_mismatch: GraphCode/Damage, Damage.gcshape: <reason>" */
		FString ToString() const;
	};

	struct FFile
	{
		FString Path;
		int64 Size = 0;
		FString Sha256;
	};

	/** R4 (changed 2026-11-19): Version is a MINIMUM within the same major; install takes the newest that fits. */
	struct FDep
	{
		FString Id;
		FString Version;
	};

	/** A product a pack needs; Min is optional (omitted in the first cut) and checked when present. */
	struct FRequire
	{
		FString Product;
		FString Min;
	};

	/** One entry of a pack (§3 `entries`): what an author types to use it. */
	/** R18: one parameter a user can tweak, as the preset reader returns it: its name and its default. */
	struct FSetting
	{
		FString Name;
		FString Default;
	};

	struct FEntry
	{
		FString Name;
		FString Kind;
		FString Import;
		FString Call;
		/** Every parameter name, in the reader's order. */
		TArray<FString> Parameters;
		/** R18: every parameter, name and default, in the file's order. */
		TArray<FSetting> Settings;
		/** R18: arithmetic other entries call (the file's mark, or a Move pose with no range): describe only. */
		bool bHelper = false;
	};

	/**
	 * R17: one BUILD CHOICE of a pack: a file of the pack whose `build { }` lines are built when it is ticked. The
	 * default choice is the pack's own main file; the others (a state, a tier, maps) are separate files so no build
	 * runs long. `Time` is shown only when the file's own header states it (the packer checks the words).
	 */
	struct FChoice
	{
		FString File;
		FString Label;
		FString Time;
		bool bDefault = false;
		/** The asset names its build lines make, in order. */
		TArray<FString> Builds;
	};

	/** MARKET2 §7.3: one entry of the signed head's blocklist (a takedown). */
	struct FBlocked
	{
		FString Id;
		FString Version;
		FString RecordSha256;
		FString Reason;
	};

	/** One version of one pack, as the signed index lists it. */
	struct GRAPHCODE_API FPack
	{
		FString Id;
		FString Version;
		FString Kind;
		FString Title;
		FString Summary;
		FString Description;
		FString Main;
		/** The pack's own publish date when the index carries one, else the index's (§9.2 "Recently updated"). */
		FString Published;
		TArray<FString> Tags;
		TArray<FFile> Files;
		TOptional<FFile> Thumbnail;
		TArray<FDep> Depends;
		TArray<FRequire> Requires;
		TArray<FEntry> Entries;
		/** R17: what the pack builds, by choice; empty for a pack with no build lines. */
		TArray<FChoice> Choices;
		/** R18: Operation | Mesh | Mesh + Operation | Poses | Guide | Skill (FormOf fills one for an older index). */
		FString Form;
		/** R19: every file builds nothing; installed, it is used from the person's own file. */
		bool bLibrary = false;
		/** R19: a Move library's header line ("presets:/Game/GraphCodeMarket/..."); empty otherwise. */
		FString Use;
		/** R20: the pack brings its own materials (`Materials/*.gc`), applied by Build before its recipe. */
		bool bMaterials = false;
		/**
		 * R20: the optional texture set, pack-relative paths that are all in Files (signed like every file), fetched
		 * only when the buyer ticks Textures. TexturesSize is their bytes together.
		 */
		TArray<FString> TextureFiles;
		int64 TexturesSize = 0;
		bool IsTexture(const FString& Path) const { return TextureFiles.Contains(Path); }
		/** The exact object from the index; written beside the files as pack.json. */
		TSharedPtr<FJsonObject> Json;
		/** MARKET2: SHA-256 of the signed version record this pack was read from (empty for the legacy index). */
		FString RecordSha256;
		/** MARKET2: the verified manifest text; when set, pack.json is written as exactly these bytes. */
		FString Manifest;

		FString Publisher() const;
		FString Name() const;
		int64 Size() const;
		/** `/Game/GraphCodeMarket/<Publisher>/<Name>`. */
		FString ContentFolder() const;
	};

	/** A verified catalog. Nothing is in here unless the index signature checked. */
	struct GRAPHCODE_API FCatalog
	{
		/** "local:<folder>" or "hosted". */
		FString Source;
		FString LocalRoot;
		bool bHosted = false;
		FString Published;
		FString IndexSha256;
		FString Kid;
		TArray<FPack> Packs;
		/** Packs the index lists that this client refuses (an id or path outside R2), each with its reason. */
		TArray<FRefusal> Invalid;
		/** MARKET2: read from the live per-version API (every record verified), not from a signed index. */
		bool bLive = false;
		/** MARKET2: the market serial this catalog was synced to, and the serial of the signed head it holds (0: none). */
		int64 Serial = 0;
		int64 HeadSerial = 0;
		/** MARKET2 §7.3: the verified head's blocklist; it applies to installed packs whatever catalog is shown. */
		TArray<FBlocked> Blocked;
		/** MARKET2: every version the live market has said anything about, "<id>@<version>" -> listed | yanked | hidden | blocked. */
		TMap<FString, FString> States;
		/** Why this catalog and not the live one (the old index, or an offline copy); empty when it is the live market. */
		FString Note;

		/** The newest version of Id, or exactly Version when given. Null when absent. */
		const FPack* Find(const FString& Id, const FString& Version = FString()) const;
		/** Every id once, at its newest version. */
		TArray<const FPack*> Latest() const;
		const FRefusal* FindInvalid(const FString& Id) const;
		/** §7.3: the head's entry blocking (Id, Version) or a version whose record hashes to RecordSha256; null when none. */
		const FBlocked* FindBlocked(const FString& Id, const FString& Version, const FString& RecordSha256 = FString()) const;
		/** listed | yanked | hidden | blocked, or "" when the live market has said nothing about it. */
		FString StateOf(const FString& Id, const FString& Version) const;
	};

	/** One installed pack, as the lock records it (R10). */
	struct FInstalled
	{
		FString Id;
		FString Version;
		FString Kind;
		FString Main;
		FString IndexSha256;
		FString InstalledAt;
		TArray<FDep> Depends;
		TArray<FFile> Files;
		/** `.claude/skills/<Name>` relative to the project, for a skill pack (R12). */
		FString SkillDir;
		/** MARKET2: SHA-256 of the signed version record it was installed from (empty when from the legacy index). */
		FString RecordSha256;
	};

	/**
	 * Where a project is. `Current()` is this editor's project; a spec points one at a temp folder so its
	 * installs never touch the real Content (the seam every client operation takes).
	 */
	struct GRAPHCODE_API FPlace
	{
		FString ProjectDir;

		static FPlace Current();
		FString ContentDir() const;
		FString MarketDir() const;
		FString LockPath() const;
		FString PackDir(const FString& Id) const;
		/** `Content/__MCPTrash/GraphCodeMarket`: /Game/__MCPTrash, like every other GraphCode delete. */
		FString TrashDir() const;
		FString SkillsDir() const;
	};

	/** The installed GraphCode products and their versions (§3 `requires`). */
	struct GRAPHCODE_API FProducts
	{
		TMap<FString, FString> Versions;
		static FProducts Current();
		/** Empty when met, else why not: "needs GraphCodeShape 0.1.8; you have 0.1.7". */
		FString Unmet(const FRequire& Require) const;
	};

	// ---- The catalog ----

	/** "graphcode-market-index-v1\n" + index bytes: what the signature covers (R7). */
	GRAPHCODE_API TArray<uint8> SignedIndexBytes(const TArray<uint8>& IndexBytes);

	/**
	 * Signature first, then the shape. DevX/DevY: a key the person chose to trust for a LOCAL catalog
	 * (`DevKeyFile`), honoured only when its kid matches; empty for the hosted route.
	 */
	GRAPHCODE_API bool AcceptIndex(const TArray<uint8>& IndexBytes, const FString& Kid, const FString& Alg,
		const FString& Sig, const FString& DevKid, const FString& DevX, const FString& DevY, FCatalog& Out, FRefusal& OutRefusal,
		bool bAcceptTestKeys = false);

	/** `<Root>/index.json` + `<Root>/index.sig.json` {kid, alg, sig}, checked as AcceptIndex checks. */
	GRAPHCODE_API bool ReadLocalCatalog(const FString& Root, FCatalog& Out, FRefusal& OutRefusal);

	/**
	 * MARKET2: a pack from a verified version manifest (pack.json bytes as text) under the same rules as an index
	 * entry. The caller has checked the manifest against its signed record (GraphCodeMarketLive.h) first.
	 */
	GRAPHCODE_API bool PackFromManifest(const FString& Manifest, FPack& Out, FRefusal& OutRefusal);

	/** A `/v1/market/index` reply {result:{index, signature:{kid,alg,sig}}} to its verified catalog. */
	GRAPHCODE_API bool AcceptIndexReply(const FString& ReplyJson, FCatalog& Out, FRefusal& OutRefusal);

	/** The `market_index` body (canonical JSON, sorted keys) exactly as the service hashes it. */
	GRAPHCODE_API FString IndexBody(const FString& DeviceId, const FString& LicenceId, const FString& RequestId,
		const TOptional<FString>& SessionSecret);

	/** The `market_download` body for one file the current index lists. */
	GRAPHCODE_API FString DownloadBody(const FString& DeviceId, const FString& LicenceId, const FString& RequestId,
		const TOptional<FString>& SessionSecret, const FString& PackId, const FString& Version, const FString& Path);

	/** A `/v1/market/download` reply to its short-lived https URL, refused unless it echoes the signed file. */
	GRAPHCODE_API bool AcceptDownloadReply(const FString& ReplyJson, const FFile& Signed, FString& OutUrl, FString& OutError);

	// ---- The lock ----

	/**
	 * False with OutError (naming the file and the row) when the lock cannot be read or holds a row with a bad id,
	 * an unsafe path or a skill folder that is not `.claude/skills/<Name>` of its pack. A lock that fails is never
	 * treated as empty and never rewritten: every write refuses `bad_lock` until a person fixes the file.
	 */
	GRAPHCODE_API bool ReadLock(const FPlace& Place, TArray<FInstalled>& Out, FString& OutError);
	GRAPHCODE_API const FInstalled* FindInstalled(const TArray<FInstalled>& Installed, const FString& Id);

	/** Files of an installed pack whose bytes on disk no longer match the lock (edited, or gone). */
	GRAPHCODE_API TArray<FString> EditedFiles(const FPlace& Place, const FInstalled& Pack);

	// ---- Search (§9.2): one query for the tab and the verb ----

	struct FQuery
	{
		/** Plain words, matched over title, entry names, tags and summary. */
		FString Words;
		FString Kind;
		FString Tag;
		/** "" | installed | updates | available */
		FString State;
		/** A pack id: only packs that need it. */
		FString Needs;
		/** "" (relevance with words, else updated) | relevance | name | updated */
		FString Sort;
		int32 MaxResults = 50;
	};

	/** The tab's box: `kind:shape tag:damage is:installed is:updates needs:<id>` and plain words. */
	GRAPHCODE_API bool ParseQueryText(const FString& BoxText, FQuery& Out, FString& OutError);
	/** The box text for a query, so ticking a filter writes its word into the box (§9.2). */
	GRAPHCODE_API FString QueryText(const FQuery& Query);

	struct FHit
	{
		const FPack* Pack = nullptr;
		FString InstalledVersion;
		bool bUpdate = false;
		int32 Score = 0;
	};

	struct FSearchResult
	{
		TArray<FHit> Hits;
		int32 TotalMatching = 0;
		bool bTruncated = false;
		/** Facet counts over the current result (§9.2), and the three views over the words alone. */
		TMap<FString, int32> TagCounts;
		TMap<FString, int32> KindCounts;
		int32 NumAll = 0;
		int32 NumInstalled = 0;
		int32 NumUpdates = 0;
	};

	/** Unknown kind/state/sort words refused by name (BadQuery). Hits point into Catalog. */
	GRAPHCODE_API bool Search(const FCatalog& Catalog, const TArray<FInstalled>& Installed, const FQuery& Query,
		FSearchResult& Out, FRefusal& OutRefusal);

	/** §9.4: everything the detail pane and `describe` say about one pack. */
	struct FDetail
	{
		const FPack* Pack = nullptr;
		FString InstalledVersion;
		bool bUpdate = false;
		/** Needs, each with whether it is installed at the version needed. */
		TArray<TPair<FDep, bool>> Needs;
		/** Installed packs that depend on this one. */
		TArray<FString> UsedBy;
		/** Requires, each with "" when met or the reason. */
		TArray<TPair<FRequire, FString>> Requires;
		/** Why Install is disabled, or empty. */
		FString Blocked;
	};

	GRAPHCODE_API bool Describe(const FCatalog& Catalog, const TArray<FInstalled>& Installed, const FProducts& Products,
		const FString& Id, FDetail& Out, FRefusal& OutRefusal);

	// ---- Operations ----

	struct FStep
	{
		FString Id;
		FString Version;
		/** install | update | remove | already_installed */
		FString Action;
		FString FromVersion;
		int64 Bytes = 0;
	};

	/**
	 * R17: the files a Build runs, in order: each ticked choice (the default first, then the pack's order), every one a
	 * file the installed pack holds. Ticked names choice FILES. False with the reason when nothing is ticked or a
	 * ticked file is not a choice of the pack. A pack with no choices builds its main file.
	 */
	GRAPHCODE_API bool BuildFiles(const FInstalled& Row, const TArray<FChoice>& Choices, const TArray<FString>& Ticked,
		TArray<FString>& OutFiles, FString& OutError);

	/**
	 * R18 (corrected 19 Nov, owner: "the parameters users can change easily"): an entry's settings as the tab lists
	 * them, in the file's order: no `rig_*` body measurement and no `*_min`/`*_max` range end; a default that is a
	 * formula of other values reads "automatic". The one place this rule lives (a later slider panel reuses it).
	 */
	GRAPHCODE_API TArray<FSetting> ChangeableSettings(const FEntry& Entry);

	/**
	 * R3 (corrected 2026-11-19): the reasons a pack file's text is outside its pack, empty when none. WRITES (the
	 * header's `into:` and every build target written as a path, which GcShapeResolveAssetPath takes as given) must
	 * be inside Pack.ContentFolder(); READS (quoted paths; presets:, controls:, character:, ik_rig:, physics_asset:)
	 * inside it, a dependency's folder, /Engine or a required product's content. Comments are not read.
	 */
	GRAPHCODE_API TArray<FString> OutsidePackPaths(const FPack& Pack, const FString& Text);

	/** R18: the pack's chip word: its own form, else one read from its kind and entries. Never a product name. */
	GRAPHCODE_API FString FormOf(const FPack& Pack);

	/** The choices ticked when nothing was asked: the default ones. */
	GRAPHCODE_API TArray<FString> DefaultTicks(const TArray<FChoice>& Choices);

	/**
	 * R17 "What you get", in plain words, for the tab: one line per asset a choice builds, its detail tiers grouped
	 * ("Wood deck, broken, 300: game, high and ultra detail"). Never an import line, a call or an entry kind.
	 */
	GRAPHCODE_API TArray<FString> WhatYouGet(const FChoice& Choice);

	/**
	 * MARKET2 §7.3, in plain words, the one place: "GraphCode/X 1.0.0 was taken down by the GraphCode Market: <reason>.
	 * Its Build is refused. Remove it from this project; nothing in your project is deleted for you."
	 */
	GRAPHCODE_API FString TakedownText(const FString& Id, const FString& Version, const FBlocked& Entry);

	/** R19: whether the installed version is a library (its pack.json says so): Build refuses it. */
	GRAPHCODE_API bool IsInstalledLibrary(const FPlace& Place, const FInstalled& Row);

	/** The installed version's choices, read from its pack.json; empty when it has none or cannot be read. */
	GRAPHCODE_API TArray<FChoice> InstalledChoices(const FPlace& Place, const FInstalled& Row);

	/** The installed version's pack.json, read as an index entry is; false when it is missing or does not parse. */
	GRAPHCODE_API bool ReadInstalledPack(const FPlace& Place, const FInstalled& Row, FPack& Out);

	/** R20: every one of the pack's texture files is installed (in the lock and on disk). False for a pack with none. */
	GRAPHCODE_API bool HasTextures(const FPlace& Place, const FInstalled& Row, const FPack& Pack);

	/** R20: one step of a Build's material phase: import one texture, or apply one material document. */
	struct FMaterialStep
	{
		/** "texture" | "material" */
		FString Kind;
		/** Pack-relative: `Textures/T_X.png`, `Materials/Parents/M_X.gc`, `Materials/Flat/MI_X.gc`, `Materials/Maps/MI_X.gc`. */
		FString File;
		/** The package it makes or changes: `<pack folder>/Textures/T_X`, `<pack folder>/Materials/<Name>`. */
		FString PackagePath;
	};

	/**
	 * R20: the material phase of a Build, in order: with every texture installed, each texture's import, then the
	 * parents (`Materials/Parents/*.gc`), then the `Maps/` instances; without them the parents, then the `Flat/` instances.
	 * Empty for a pack with no materials. False with the reason when the installed pack cannot be read.
	 */
	GRAPHCODE_API bool MaterialSteps(const FPlace& Place, const FInstalled& Row, TArray<FMaterialStep>& Out, FString& OutError);

	/**
	 * R20: run one step on the game thread through the ordinary write paths (R16: an authoring write): a texture through
	 * gc_import's plan and apply, its settings{} from `Textures/textures.json` (a texture already there is kept); a
	 * material with gc_apply's create, or, when the asset exists, applied over it with its header rebased on the live
	 * projection (how Flat instances become Maps ones in place). OutMessage says what was done.
	 */
	GRAPHCODE_API bool RunMaterialStep(const FPlace& Place, const FInstalled& Row, const FMaterialStep& Step, FString& OutMessage,
		FString& OutError);

	/**
	 * R20, gc_apply's door (the tab's Build runs the same steps): when ContentPath is a file of an installed pack that
	 * brings materials and any of its texture or material assets does not exist yet, the whole material phase runs
	 * now, synchronously, so the recipe's first build finds them. True with nothing said when there is nothing to do.
	 */
	GRAPHCODE_API bool PrepareMaterialsFor(const FString& ContentPath, FString& OutSaid, FString& OutError);

	/**
	 * R20 corrected: the installed MATERIAL PACKS a pack's build needs, dependencies first: every installed pack in Row's
	 * dependency closure (Row included) whose pack.json says it brings materials. Rows is the lock.
	 */
	GRAPHCODE_API TArray<FInstalled> MaterialPacksFor(const FPlace& Place, const FInstalled& Row, const TArray<FInstalled>& Rows);

	/** R20: Row's material steps when they must run now (never made, made in the other state, or an asset gone); else empty. */
	GRAPHCODE_API bool MaterialStepsDue(const FPlace& Place, const FInstalled& Row, TArray<FMaterialStep>& Out, FString& OutError);

	/** One Build of several choice files, built in turn (GraphCodeMarket.cpp). */
	struct FBuildChain;

	/** MARKET2: the session's copy of the live market (GraphCodeMarket.cpp). */
	struct FLiveHolder;

	struct GRAPHCODE_API FOutcome
	{
		bool bOk = false;
		bool bDryRun = false;
		TArray<FStep> Steps;
		TArray<FRefusal> Refusals;
		/** Files written, project-relative. */
		TArray<FString> Written;
		/** Where removed files went, project-relative. */
		TArray<FString> Trashed;
		/** Files written OUTSIDE the pack folder (R12's skill copy), said in every answer. */
		TArray<FString> OutsidePackFolder;
		/**
		 * R20: the packs whose optional textures this operation fetches and writes: the one the buyer ticked Textures
		 * for, and every installed pack that already has them (an update keeps them). Every other pack's texture
		 * files are left out of the fetch, the write and the lock.
		 */
		TSet<FString> TexturesFor;
		FString Message;

		/** Every refusal's line, joined. */
		FString RefusalText() const;
	};

	/**
	 * Set from any thread to stop an install or update. It is honoured up to the first write: a cancelled operation
	 * leaves no file and no lock entry; once writing has begun it finishes (a local write takes milliseconds, and a
	 * half-written pack is the one state worse than either end).
	 */
	using FCancelToken = TSharedPtr<FThreadSafeBool, ESPMode::ThreadSafe>;

	/** One file's bytes, synchronously. A local catalog reads its folder; the hosted path pre-fetches. */
	using FFetch = TFunction<bool(const FPack& Pack, const FFile& File, TArray<uint8>& OutBytes, FString& OutError)>;

	/** `<Root>/packs/<Publisher>/<Name>/<version>/<file>`. */
	GRAPHCODE_API FFetch LocalFetch(const FString& Root);

	/** Resolve without fetching: the steps, and every refusal that needs no bytes. */
	GRAPHCODE_API FOutcome PlanInstall(const FCatalog& Catalog, const FPlace& Place, const FProducts& Products,
		const FString& Id, const FString& Version, bool bTextures = false);
	/** Pack empty: every installed pack that has a newer version. */
	GRAPHCODE_API FOutcome PlanUpdate(const FCatalog& Catalog, const FPlace& Place, const FProducts& Products, const FString& Id);
	GRAPHCODE_API FOutcome PlanRemove(const FPlace& Place, const FString& Id);

	/** The files the planned steps need, for a pre-fetch. */
	GRAPHCODE_API TArray<TPair<const FPack*, FFile>> FilesToFetch(const FCatalog& Catalog, const FOutcome& Plan);

	/** Plan, fetch, verify every byte, then write. Nothing is written when any byte fails. Off-thread safe. */
	GRAPHCODE_API FOutcome Install(const FCatalog& Catalog, const FPlace& Place, const FProducts& Products,
		const FString& Id, const FString& Version, bool bDryRun, const FFetch& Fetch, const FCancelToken& Cancel = nullptr,
		bool bTextures = false);
	GRAPHCODE_API FOutcome Update(const FCatalog& Catalog, const FPlace& Place, const FProducts& Products,
		const FString& Id, bool bDryRun, const FFetch& Fetch, const FCancelToken& Cancel = nullptr);
	GRAPHCODE_API FOutcome Remove(const FPlace& Place, const FString& Id, bool bDryRun);

	/** The spec seam behind GcMarket::Testing::FScopedWriteFault. Never set outside an automation run. */
	GRAPHCODE_API void SetWriteFaultForSpecs(TFunction<bool(const FString& Path)> Fails);

	// ---- R11: what search reads ----

	/** `/Game/GraphCodeMarket/<P>/<N>` of every installed shape pack in this project. */
	GRAPHCODE_API TArray<FString> InstalledShapeRoots();
	/** The installed pack a content path (`/Game/GraphCodeMarket/P/N/File`) belongs to. */
	GRAPHCODE_API bool FindInstalledForContentPath(const FString& ContentPath, FString& OutId, FString& OutVersion);

	/**
	 * MARKET2 §7.3: why a build written to this content path is refused (it is a file of an installed pack the signed head
	 * takes down: gc_apply's shape and move paths ask this, the tab's Build too), or empty. Game thread.
	 */
	GRAPHCODE_API FString TakedownForContentPath(const FString& ContentPath);

	// ---- The session both faces share (game thread) ----

	/** One pack's activity for the cards and the status bar (§9.5, §9.6). */
	struct FActivity
	{
		/** "" | installing | updating | removing | building | failed | done */
		FString State;
		float Fraction = 0.f;
		FString Message;
		TArray<FString> BuiltAssets;
	};

	DECLARE_MULTICAST_DELEGATE(FOnMarketChanged);

	class GRAPHCODE_API FSession
	{
	public:
		static FSession& Get();

		/** The configured catalog: `CatalogRoot` when set (R8), else the live market (MARKET2), else the old index. */
		bool IsLocal() const;
		FString CatalogRoot() const;

		/** The last verified catalog (cached across launches); null before the first. */
		const FCatalog* Catalog() const;
		FString LastError() const { return LastErrorText; }
		bool IsRefreshing() const { return bRefreshing; }
		FDateTime RefreshedAt() const { return RefreshedAtUtc; }

		/** Local: at once. Hosted: asynchronously; Done runs on the game thread either way. */
		void Refresh(TFunction<void(bool bOk, const FString& Error)> Done);
		/** Local catalogs only: refresh now and say why not. */
		bool RefreshNow(FString& OutError);

		/** The lock's rows; empty with LockError() set when the lock cannot be read. */
		TArray<FInstalled> Installed() const;
		FString LockError() const { return LockErrorText; }

		/** Run on the current catalog. Local: the work runs off the game thread; Done on the game thread. */
		/** R20: bTextures also fetches the pack's optional textures (and adds them to a pack installed without them). */
		void StartInstall(const FString& Id, const FString& Version, bool bDryRun, TFunction<void(const FOutcome&)> Done,
			bool bTextures = false);
		void StartUpdate(const FString& Id, bool bDryRun, TFunction<void(const FOutcome&)> Done);
		void StartRemove(const FString& Id, bool bDryRun, TFunction<void(const FOutcome&)> Done);

		/** Cancel the running install or update of Id ("" = Update all). True when one was running. */
		bool Cancel(const FString& Id);

		/**
		 * The pack's own `build { }` through the ordinary apply path (R16: an authoring write). A `.gcshape`
		 * main file is built as gc_apply builds one; Done says what was built or the build's own notes.
		 */
		void StartBuild(const FString& Id, TFunction<void(bool bOk, const TArray<FString>& Built, const FString& Message)> Done);

		/** R17: build the ticked choices (choice file names) in order, one after the other, as one Build. */
		void StartBuild(const FString& Id, const TArray<FString>& Ticked,
			TFunction<void(bool bOk, const TArray<FString>& Built, const FString& Message)> Done);

		FActivity ActivityOf(const FString& Id) const;

		/**
		 * MARKET2 §7.3: the head's entry when this installed version is taken down, else null. Reads the highest verified
		 * head this PC holds (per user), whether or not a catalog has been read in this session.
		 */
		TOptional<FBlocked> TakedownOf(const FInstalled& Row);
		/** The highest head serial this PC has accepted (0: none yet). */
		int64 HeadSerial();

		/**
		 * Whether moving an activity to NewState is a change the views rebuild for (OnChanged): a new state is; a
		 * progress tick in the same state (fraction, the n/m text) is not, and is read by the widgets as they paint.
		 */
		static bool IsStateChange(const FActivity& Before, const FString& NewState);
		FOnMarketChanged& OnChanged() { return Changed; }

		/** The thumbnail's cached PNG path (verified against the signed SHA-256), fetching it when needed. */
		void Thumbnail(const FPack& Pack, TFunction<void(const FString& PngPath)> Done);

		/** The live market's base URL (the release service); a spec may point it at a test server. */
		FString LiveBase() const;
		void SetLiveBaseOverride(const FString& Base) { LiveBaseOverride = Base; }

		/** For specs and tools: a catalog root that wins over the setting while set. */
		void SetCatalogRootOverride(const FString& Root) { RootOverride = Root; Current.Reset(); }
		const FString& CatalogRootOverride() const { return RootOverride; }

	private:
		void SetActivity(const FString& Id, const FString& State, float Fraction, const FString& Message);
		/** Builds the chain's next file, then the one after, then reports the whole Build (R17). */
		void BuildNextOf(const TSharedRef<FBuildChain>& Chain);
		void Run(const FString& Id, const FString& State, TFunction<FOutcome(const FCatalog&, const FFetch&, const FCancelToken&)> Work,
			const FOutcome& Plan, TFunction<void(const FOutcome&)> Done);
		/** MARKET2: head, then changes since the held serial; on an unreachable API the offline copy, else the old index. */
		void RefreshLive(TFunction<void(bool bOk, const FString& Error)> Done);
		/** The old signed index (MARKET1), with Note said beside it. */
		void RefreshLegacy(const FString& Note, TFunction<void(bool bOk, const FString& Error)> Done);
		void LoadLive();
		void SetLiveCatalog(const FString& Note);
		TSharedPtr<FLiveHolder> Live;
		FString LiveBaseOverride;

		TSharedPtr<FCatalog> Current;
		FString LastErrorText;
		bool bRefreshing = false;
		FDateTime RefreshedAtUtc;
		FString RootOverride;
		mutable FCriticalSection ActivityLock;
		TMap<FString, FActivity> Activities;
		/** The running operations' cancel tokens, by pack id ("*" for Update all). Game thread only. */
		TMap<FString, FCancelToken> CancelTokens;
		mutable FString LockErrorText;
		/** Hosted thumbnails wait here and download at most four at a time (game thread). */
		TArray<TPair<FPack, TFunction<void(const FString&)>>> ThumbQueue;
		int32 ThumbsInFlight = 0;
		void PumpThumbnails();
		FOnMarketChanged Changed;
	};
}
