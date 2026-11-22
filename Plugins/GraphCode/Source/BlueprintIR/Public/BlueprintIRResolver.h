// Blueprint IR - the resolver.
//
// ---------------------------------------------------------------------------
// WHY THIS IS ONE CLASS AND NOT TWO
// ---------------------------------------------------------------------------
// Signature -> text is easy: you hold the object and print a canonical name.
// Text -> signature is not a converter, it is search plus disambiguation against
// everything currently loaded in the editor. That IS the discovery index.
//
// So bp_find and bp_apply do not get separate implementations. bp_apply's
// resolution step calls the same resolver bp_find exposes. One code path means
// the model's lookups and the write path's validation can never disagree - which
// is exactly the failure this project is trying to kill.
//
// ---------------------------------------------------------------------------
// WHY WE DO NOT HOLD POINTERS
// ---------------------------------------------------------------------------
// Two reasons, the second fatal:
//
//   1. GC. A bare UFunction*/UEnum* held across calls needs rooting, or it is a
//      crash waiting to happen.
//
//   2. Blueprint recompilation REPLACES UClass and its UFunction objects. That
//      happens constantly, and bp_apply triggers it itself. Cached pointers into
//      a class go stale on every write - you would be invalidating your own cache
//      on each call you make.
//
// The rule: resolve fresh per tool call. Hold pointers only within a single
// game-thread apply - resolve everything up front, validate, then apply, all
// inside one transaction. Pointers are transaction-scoped locals, never state.
//
// What we DO cache is the search index, which is made of strings and does not need
// to be correct-by-pointer. A Blueprint compile replaces only the action-database
// entry that named that Blueprint, so that source is replaced incrementally. Schema-
// wide asset changes and hot reload still invalidate the whole index.

#pragma once

#include "CoreMinimal.h"

class FProperty;
class UBlueprint;
class UEdGraph;
class UEnum;
class UFunction;
class UScriptStruct;

/** What kind of thing a candidate refers to. */
enum class EBpEntityKind : uint8
{
	Function,
	Macro,
	Enum,
	EnumEntry,
	Struct,
	StructMember,
	Variable,
	Event,
	/** Casts, operators, and the other synthetic actions the action database supplies. */
	Action,
};

BLUEPRINTIR_API const TCHAR* LexToString(EBpEntityKind Kind);

/**
 * Does a graph get to see this property? The palette's own rule, and the WHOLE of it.
 *
 * Public because it has three askers in two files and the third one had it wrong. The resolver's
 * class-chain collector and its exact-path recovery both mirror it (see the definition for the
 * dispatcher case that forced the second arm); the READER's inherited-property pass mirrored the
 * flag alone until 2026-09-01, so no inherited event dispatcher could be projected at all while
 * `gc_search` had been finding them since the same fix. A rule with more than one carrier is the
 * shape this board keeps closing at one site and leaving standing at another — so it has one
 * carrier now, and the next asker takes it from here rather than re-deriving it.
 */
BLUEPRINTIR_API bool IsPropertyVisibleToGraphs(const FProperty* Property);

/**
 * One search hit / resolution candidate.
 *
 * Strings only - deliberately. This struct is what lives in the cached index, and
 * it must survive a Blueprint recompile that invalidates every UObject in sight.
 */
struct BLUEPRINTIR_API FBpCandidate
{
	EBpEntityKind Kind = EBpEntityKind::Function;

	/** Canonical path form - the identity the IR uses. See BlueprintIRNames.h. */
	FString Canonical;

	/** Node display title as the editor shows it, e.g. "Set Actor Location". */
	FString DisplayTitle;

	/**
	 * The ADR-8 op token this candidate would be written as: the spawner's node class with
	 * the `K2Node_` prefix stripped. Empty for entity kinds that are not nodes at all
	 * (enums, structs and their members).
	 *
	 * This is what makes discovery and authoring share one vocabulary. Before it, a search
	 * hit for a non-function node reported only its display title, which is precisely the
	 * localised, non-unique string ADR-2 forbids as identity - so `bp_find` could tell the
	 * model a node existed while giving it nothing writable.
	 */
	FString Op;

	/** Compact title for operators, e.g. "+" for Add_VectorVector. */
	FString CompactTitle;

	/** Palette category, used for disambiguating same-named entries. */
	FString Category;

	/** Owning class/struct/enum path. */
	FString OwnerPath;

	/**
	 * Exact pin names and types, rendered as "In: A(Vector), B(Vector) -> Out: ReturnValue(Vector)".
	 * Exact names, never display labels: the write path consumes these directly.
	 */
	FString Signature;

	/** Keywords the action database exposes, folded into the search text. */
	FString Keywords;

	/** Unreal's tooltip/description for the action or reflected member. */
	FString Description;

	/** Query terms that contributed to an intent-search result. Not cached. */
	TArray<FString> MatchedTerms;

	/** True when this came from a BP-authored type and therefore has mangled internals. */
	bool bUserDefined = false;

	/** Relevance score for ranking. Higher is better. Not exposed in tool output. */
	float Score = 0.0f;
};

enum class EBpResolveStatus : uint8
{
	/** Exactly one match. Best is valid. */
	Resolved,
	/** No match at all. Candidates is empty, and stays empty - we do not offer near-misses. */
	NotFound,
	/** More than one match. Candidates carries them with disambiguating info. */
	Ambiguous,
};

/**
 * Resolution outcome.
 *
 * Ambiguous is a first-class result, not an error to paper over. When a name is
 * ambiguous the resolver returns candidates with disambiguating info and never a
 * best guess - silent picking is the hallucination failure wearing a different hat.
 */
struct BLUEPRINTIR_API FBpResolveResult
{
	EBpResolveStatus Status = EBpResolveStatus::NotFound;

	/** Valid only when Status == Resolved. */
	FBpCandidate Best;

	/** Populated when Status == Ambiguous. */
	TArray<FBpCandidate> Candidates;

	/** Human-readable explanation, safe to surface verbatim in a tool error. */
	FString Message;

	bool IsResolved() const { return Status == EBpResolveStatus::Resolved; }
};

/**
 * Context for a resolution or search.
 *
 * Without context filtering, results are unusable - every class-specific function
 * in the project shows up for every target.
 */
struct BLUEPRINTIR_API FBpResolveContext
{
	/** Blueprint asset path the call is being made in, e.g. "/Game/BP_Player.BP_Player". */
	FString BlueprintPath;

	/** Graph name within that Blueprint, if the caller knows it. */
	FString GraphName;

	/** Restrict to these kinds. Empty means all kinds. */
	TArray<EBpEntityKind> KindFilter;

	/** Cap on returned candidates. */
	int32 MaxResults = 25;

	/**
	 * When true, only exact canonical-path matches count. bp_apply uses this for
	 * identifiers the IR already carries in canonical form, so the write path never
	 * pays for fuzzy matching it does not want.
	 */
	bool bExactOnly = false;
};

/**
 * The resolver / discovery index.
 *
 * Editor-lifetime singleton. Holds only strings. Rebuilds lazily.
 */
class BLUEPRINTIR_API FBpResolver
{
public:
	static FBpResolver& Get();

	/**
	 * Free-text search. This is what bp_find exposes.
	 *
	 * OutTotalMatches, when supplied, receives the match count BEFORE Context.MaxResults trims
	 * the array. Without it a caller cannot tell 25-of-25 from 25-of-5000, which is the whole
	 * difference between an answer and a sample.
	 */
	void Search(const FString& Query, const FBpResolveContext& Context,
		TArray<FBpCandidate>& OutCandidates, int32* OutTotalMatches = nullptr);

	/**
	 * Intent-oriented search over node-producing actions. Unlike Search(), this tokenizes
	 * natural phrases, searches Unreal tooltips/signatures as well as names, preserves
	 * distinct op variants, and never returns bare enum/struct entities.
	 *
	 * OutUnmatchedTerms, when supplied, receives every query term that matched NOTHING in the whole
	 * index. Computed over the FULL match set before MaxResults trims it, which is the only honest
	 * place: a term that matched only results on page two is matched, and computing this from the
	 * returned page would call it unmatched. `bp_search_nodes`'s card promised "unmatched terms are
	 * visible" for a long time while nothing computed them; this is that promise.
	 */
	void SearchNodes(const FString& Query, const FBpResolveContext& Context,
		TArray<FBpCandidate>& OutCandidates, int32& OutTotalMatches,
		TArray<FString>* OutUnmatchedTerms = nullptr);

	/**
	 * Resolve one identifier to one entity. This is what bp_apply calls before
	 * touching a graph. Same index, same matching rules as Search - by construction.
	 */
	FBpResolveResult Resolve(const FString& Query, EBpEntityKind ExpectedKind, const FBpResolveContext& Context);

	/**
	 * Live-pointer lookups. ONLY legal inside a single game-thread apply, and the
	 * result must never outlive it. Naming is deliberately blunt so that a caller
	 * stashing one in a member is obviously wrong at the call site.
	 */
	static UFunction* ResolveFunctionPointer_TransactionScoped(const FString& Canonical);
	static UEnum* ResolveEnumPointer_TransactionScoped(const FString& Canonical);
	static UScriptStruct* ResolveStructPointer_TransactionScoped(const FString& Canonical);
	static UEdGraph* ResolveMacroGraphPointer_TransactionScoped(const FString& Canonical);

	/** Drop the cached index. Cheap - it is rebuilt on next use. */
	void InvalidateIndex();

	/** True when the index is currently populated. */
	bool IsIndexBuilt() const { return bIndexBuilt; }

	/** Number of indexed records, for diagnostics. */
	int32 GetIndexSize() const { return Index.Num(); }

	/** Hook/unhook the invalidation delegates. Called by the module. */
	void RegisterInvalidationHooks();
	void UnregisterInvalidationHooks();

private:
	FBpResolver() = default;

	/**
	 * Build the index from FBlueprintActionDatabase plus explicit enum/struct/macro
	 * reflection. Must run on the game thread.
	 */
	void BuildIndex();

	/** Actions: functions, casts, operators, macros, variable get/set, custom events. */
	void IndexBlueprintActions(UObject* OnlySource = nullptr);

	/** Replace one action-database source without rebuilding the other ~113k records. */
	void RefreshActionSource(UObject* Source, bool bRemoveOnly = false);

	/**
	 * Enums and structs. The action database does not cover these, and neither does
	 * registry discovery: a UUserDefinedEnum shows up as an asset path but its members do
	 * not. This pass loads registry-discovered user types to index their schemas and also
	 * walks compiled reflection for native UENUM/USTRUCT types. Without both sources,
	 * bp_find changes its answer according to what the editor happened to open.
	 */
	void IndexEnumsAndStructs();

	void AddCandidate(FBpCandidate&& Candidate, const FString& Source = FString());

	/** Score a candidate against a lowercased query. Returns <= 0 for no match. */
	static float ScoreCandidate(const FBpCandidate& Candidate, const FString& LoweredQuery);

	TArray<FBpCandidate> Index;

	/** Lowercased haystack per index entry, built once so search does not re-lower. */
	TArray<FString> SearchText;

	/** Action-database object path that produced each entry; empty for reflected type entries. */
	TArray<FString> IndexSources;

	bool bIndexBuilt = false;

	FDelegateHandle OnActionEntryUpdatedHandle;
	FDelegateHandle OnActionEntryRemovedHandle;
	FDelegateHandle OnAssetAddedHandle;
	FDelegateHandle OnAssetRemovedHandle;
	FDelegateHandle OnAssetRenamedHandle;
	FDelegateHandle OnReloadCompleteHandle;
};
