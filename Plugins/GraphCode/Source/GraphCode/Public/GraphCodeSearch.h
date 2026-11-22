#pragma once

#include "CoreMinimal.h"

struct FAssetData;

/**
 * Project-wide search over GraphCode text. `Docs/GC-SEARCH-DESIGN.md`.
 *
 * **LIVE ONLY, by ruling U2** — *"always do a source live. if source saved, ai can just grep"*.
 * There is no saved-corpus mode and no `source:` parameter: searching the `.gc` bytes on disk is
 * what grep already does, so a second implementation of it inside the bridge would be a slower
 * grep. Everything here projects through `FGraphCodeProjector::BuildTextForPackage`, which sees
 * unsaved edits and lets the Asset Registry narrow the candidate set first.
 */

/** One way the target can legitimately be written. `Meaning` is reported as `matched_as`. */
struct GRAPHCODE_API FGcSearchSpelling
{
	FString Token;
	FString Meaning;

	/** Owner class path for a member spelling, so `Owner:Member` can be recognised as qualified. */
	FString QualifiedBy;
};

/** One line of the ancestor chain (U3). Either a real line, or the `…` standing for elided siblings. */
struct GRAPHCODE_API FGcSearchExcerptLine
{
	int32 Depth = 0;
	FString Text;
	bool bElision = false;
	bool bHit = false;
};

struct GRAPHCODE_API FGcSearchHit
{
	FString PackageName;

	/** The top-level block the hit sits in: `graph`, `widgets`, `properties`, … */
	FString BlockName;

	/** 1-based line in the LIVE projection. For navigation only — the text is not on disk. */
	int32 Line = 0;

	/**
	 * `reference`: which spelling matched — `member`, `member (qualified)`, `package`, `object`,
	 * `class`, `declaration`. `entity`: always `declaration`. `description`: where the words were
	 * authored — `comment`, `text`, `value`, `name`.
	 */
	FString MatchedAs;

	/**
	 * The thing that matched, verbatim: the declared name for `entity`, the authored words for
	 * `description`. Empty for `reference`, whose answer is the excerpt.
	 *
	 * This is the half of §6 the excerpt cannot carry — *"under what exact spelling"* is a question
	 * about one token, and a reader should not have to find it again inside a statement.
	 */
	FString Name;

	/**
	 * `exact` or `fuzzy` — whether this hit is the thing asked for or something near it (§10.3).
	 *
	 * The second half of U9. Tolerance stopped being a parameter because the right value is decided
	 * by the `kind` beside it; what a caller actually needs is not the lever but the answer to
	 * *which one did I get*, per hit, where a reader is already looking.
	 */
	FString Match = TEXT("exact");

	/**
	 * Why this hit is where it is in the list — the number `max_results` cuts on (§10.6).
	 *
	 * **`max_results` ASKS FOR THE TOP HITS AND USED TO RETURN THE FIRST ONES FOUND.** The corpus is
	 * walked in sorted package order, so `max_results:10` over a project-wide scan answered with the
	 * ten alphabetically-earliest matches and reported the rest as a count. That is not a small
	 * answer to the question; it is an answer to a different one.
	 *
	 * Reported per hit rather than kept internal, for the reason `Match` and `scope_read` are: a
	 * ranked answer whose ranking cannot be seen is a decision made on the caller's behalf with no
	 * way to check it. The components are per kind and `FGcSearchResult::RankedBy` names them.
	 */
	float Score = 0.f;

	/** The enclosing chain, outermost first, with the hit line last but for its closers. */
	TArray<FGcSearchExcerptLine> Excerpt;

	/** The chain as one indented string — what a reader sees. */
	FString RenderExcerpt() const;
};

/**
 * A candidate that was NOT searched, and why (U4).
 *
 * The reason an empty answer can be trusted. Without this, "nothing references this" and "the only
 * thing referencing this is an asset kind GraphCode has never read" are the same empty list.
 */
struct GRAPHCODE_API FGcSearchOmission
{
	FString PackageName;

	/** `everything`, or the facet token out of that kind's `omits:`. */
	FString Omits;

	/** The sentence a caller acts on. */
	FString Reason;

	/**
	 * How many packages this row stands for; `PackageName` is then one example of them.
	 *
	 * A corpus scan rolls identical reasons up. `reference` looks at a handful of candidates and
	 * names each, but a project-wide scan meets nine hundred assets with no GraphCode kind, and nine
	 * hundred rows saying so is a ledger nobody reads — which is the same as not having one.
	 */
	int32 Count = 1;
};

struct GRAPHCODE_API FGcSearchResult
{
	TArray<FGcSearchHit> Hits;
	TArray<FGcSearchOmission> Omissions;

	/** Stated once per answer, not per result: what this search cannot see at all. */
	TArray<FString> Caveats;

	/** Names close to the query that DO exist, reported even when there are exact hits. */
	TArray<FString> NearMisses;

	/** `description`: query terms that matched something, and terms that matched nothing at all. */
	TArray<FString> MatchedTerms;
	TArray<FString> UnmatchedTerms;

	/** The spellings the target expanded to, so a caller can see what was actually looked for. */
	TArray<FString> SearchedFor;

	/**
	 * What decided the order, heaviest first — the components of `FGcSearchHit::Score` for this kind.
	 *
	 * Set by the `Find*` that ran, because the signals differ: `reference` ranks on which spelling
	 * matched, `entity` on whether the case is the one asked for, `description` on how much of the
	 * matched line the query accounts for. Stated in the reply so the ordering is auditable.
	 */
	TArray<FString> RankedBy;

	int32 PackagesSearched = 0;

	/**
	 * The ranking saw a floor rather than every match — `GcMaxRankedCandidates` was reached.
	 *
	 * The one bound left in this file, and it is on the CANDIDATE SET rather than on the corpus: the
	 * scan is complete now (U10), but every candidate carries a rendered excerpt, so an unbounded
	 * list would trade a bounded scan for unbounded memory. It has never fired on this project — the
	 * broadest `description` query here matches in the hundreds — and when it does the caveat says
	 * the ranking is over a floor, because a ranked answer over an arbitrary subset is exactly the
	 * first-found result this ranking exists to stop being.
	 */
	bool bRankingCeilingHit = false;

	/** Every row, declarations included. */
	int32 TotalHits = 0;

	/**
	 * Rows that are the thing being DECLARED rather than a use of it.
	 *
	 * `TotalHits - Declarations` is the number that answers "is this used?", and separating them is
	 * the difference between "cited twice" and "declared once, bound once" for a button nothing
	 * else touches.
	 */
	int32 Declarations = 0;

	int32 Citations() const { return TotalHits - Declarations; }

	bool bTruncated = false;
};

struct GRAPHCODE_API FGcReferenceQuery
{
	/** `/Game/UI/WBP_Menu`, `/Game/UI/WBP_Menu.RestartButton`, or `WBP_Menu.RestartButton`. */
	FString Target;

	int32 MaxResults = 50;

	/** Report near misses for a member name the asset does not hold. */
	bool bFuzzy = true;
};

/**
 * A scan with no target to narrow it — `entity` and `description`.
 *
 * **THIS IS THE EXPENSIVE ONE, AND IT IS COMPLETE.** `reference` gets its candidate set from
 * `IAssetRegistry::GetReferencers` and projects the handful of packages that actually cite the
 * target. Nothing narrows "which asset declares a name I half-remember" or "which comment says
 * this", so the candidate set is every projectable asset under `Scope` and the cost is one live
 * projection each.
 *
 * **`MaxAssets` IS GONE (U10, §10.6), AND THE REASON IS THAT IT MADE `MaxResults` MEAN THE WRONG
 * THING.** It stopped the walk at 400 of this project's 1,741 packages, in sorted package order, so
 * an answer was assembled out of the alphabetically-earliest corner of the project and the rest was
 * reported as a floor. Capping the SCAN and capping the ANSWER are different jobs: the scan is
 * exhaustive now, and `MaxResults` cuts a RANKED list, so a short answer is the best hits rather
 * than the first ones met. `Scope` remains the lever, and it narrows before anything is projected.
 */
struct GRAPHCODE_API FGcCorpusQuery
{
	/** `entity`: the name. `description`: the words. Empty on `entity` lists what `AssetPath` holds. */
	FString Query;

	/** Long package path to search under. Empty means `/Game`. */
	FString Scope;

	/**
	 * Narrow the corpus to one class, recursively. Empty is no narrowing, the ordinary path.
	 *
	 * The other half of `scope:` (§10.4). A caller holds ONE question — *which assets should you
	 * even look at* — and answers it with a folder or a type, so the same word carries both and the
	 * scan resolves it before projecting anything. `scope:`'s third shape, a skeleton, is
	 * `kind:"asset"` only and is refused here by name: the compatibility predicate lives in
	 * `asset_search` in another module, and a second copy of it would be the one thing worse than
	 * not answering ([[corpus-agreeing-with-itself-proves-nothing]]).
	 */
	FString ClassPath;

	/** One package instead of a scope, when the caller already knows the asset. */
	FString AssetPath;

	/** How many RANKED hits come back. Not a bound on what is searched — nothing is. */
	int32 MaxResults = 50;

	/**
	 * `entity`: report near-miss names beside exact matches. `description`: ALSO loosen the match
	 * itself to a bounded edit distance, which is why it defaults on for that kind alone (§7).
	 */
	bool bFuzzy = true;
};

/** One named thing a document declares. `entity`'s unit, and the near-miss corpus for `reference`. */
struct GRAPHCODE_API FGcDeclaration
{
	FString Name;
	int32 Line = 0;

	/** `assignment` (`Name = …`), `typed` (`Name: Type …`), `group` (`function Name … {`). */
	FString Shape;

	/** `n15`, `c2` — a minted graph identity rather than a name anybody searches for. §`entity`. */
	bool bAlias = false;
};

/** One run of authored words on a line, and where the format put it. `description`'s unit. */
struct GRAPHCODE_API FGcAuthoredText
{
	FString Text;

	/** `comment`, `text`, `message`, `value` — the call the quoted span sits in. */
	FString Source;
};

/**
 * What a corpus scan carries between documents, beside the hits.
 *
 * It exists so the per-document half of `entity` and `description` can be a pure function over
 * text — the same split `SearchText` already makes for `reference`, and for the same reason: the
 * half of this feature with the interesting rules is the half that needs no editor.
 */
struct GRAPHCODE_API FGcCorpusScan
{
	/** `entity`: every name that did NOT match, and the first package it was seen in. */
	TArray<FString> OtherNames;
	TMap<FString, FString> DeclaredIn;

	/** `description`: whether each term has matched anywhere yet, and the words met so far. */
	TArray<bool> TermMatched;
	TSet<FString> Vocabulary;
};

class GRAPHCODE_API FGcSearch
{
public:
	// -- S0: what the target can be written as -------------------------------------------------

	/**
	 * Split `Target` into package and member. The member is whatever follows the LAST `.` when that
	 * tail is not part of the object path — `/Game/A/B.Member` and `B.Member` both split; a bare
	 * `/Game/A/B` does not.
	 */
	static void SplitTarget(const FString& Target, FString& OutPackagePart, FString& OutMember);

	/** Every spelling a citation of this package (and optional member) may use. */
	static TArray<FGcSearchSpelling> ExpandSpellings(const FString& PackageName, const FString& Member);

	// -- S1: whole-token matching --------------------------------------------------------------

	/**
	 * The tokens of one line of GraphCode text, for reference matching.
	 *
	 * Splits on the punctuation the format uses as structure, `:` included — which is what turns
	 * `VariableGet /Game/A.B_C:Total` into three tokens and makes `Owner`-then-`Member` adjacency
	 * the test for a qualified citation. `.` is NOT a separator: `/Game/A.B_C` is one name.
	 *
	 * **Double-quoted spans are dropped.** A comment saying "RestartButton - Set Loading Screen" is
	 * prose about the button, not a citation of it; prose is `kind:"description"`. Single quotes
	 * survive, because `obj'/Game/…'` is exactly a citation.
	 */
	static TArray<FString> TokenizeForReference(const FString& Line);

	/** Does this line cite any spelling? `OutMatchedAs` names which, most specific first. */
	static bool LineCites(
		const FString& Line, const TArray<FGcSearchSpelling>& Spellings, FString& OutMatchedAs);

	// -- S2: trace the bracket up (U3) ----------------------------------------------------------

	/**
	 * The ancestor chain of a one-based line: every line that opened a brace still open at it.
	 *
	 * Brace depth over the text, ignoring braces inside double-quoted strings. A line that opens and
	 * closes on itself is not an ancestor. Elisions are inserted where siblings were dropped.
	 */
	static bool BuildExcerpt(
		const FString& Text, int32 OneBasedLine, TArray<FGcSearchExcerptLine>& OutLines);

	/** Which top-level block owns a one-based line, or empty for the envelope. */
	static FString BlockOwningLine(const FString& Text, int32 OneBasedLine);

	/**
	 * Everything a document DECLARES, with the line and the shape it was written in.
	 *
	 * Four shapes, and the format uses all four: `Name = value`, `Name: Type` with no value at all,
	 * `Name: Type = value`, and a group header like `function Foo  sig:… {`. Reading only the first
	 * is how `BP_Pistol.PistolFireHapticEffect` — declared `PistolFireHapticEffect:
	 * HapticFeedbackEffect_Base* [multiline]`, with no `=` anywhere on the line — came back as *"is
	 * NOT declared"*, which is §4.1's dangerous answer arriving through the check written to prevent
	 * it.
	 *
	 * Only at brace depth ≥ 1 and paren depth 0. The envelope's `parent:`/`omits:`/`source:` are not
	 * declarations, and neither is `MaterialInstanceFloor=obj'…'` continuing a level actor's value
	 * across lines — both are `Name<punct>value` and neither names anything.
	 */
	static TArray<FGcDeclaration> Declarations(const FString& Text);

	/**
	 * The declared names alone, aliases dropped.
	 *
	 * The candidate set for a near miss, and how an empty answer tells "nothing cites this member"
	 * apart from "there is no such member" — the same empty list otherwise, and only one of them is
	 * safe to act on.
	 */
	static TArray<FString> DeclaredNames(const FString& Text);

	/**
	 * The authored words on one line: every double-quoted span, with what it was written into.
	 *
	 * **A LOCALIZATION KEY IS NOT AUTHORED TEXT.** `text("Restart", key="0119…44E")` holds one
	 * thing a person typed and one 32-digit digest, both in double quotes, and a corpus of prose
	 * carrying every key in the project answers a fuzzy query with hex.
	 */
	static TArray<FGcAuthoredText> AuthoredTextOn(const FString& Line);

	/**
	 * The words in a name or a phrase, lowercased: `RestartButton` → `restart`, `button`.
	 *
	 * Splits on punctuation, on `_`, and on camelCase and letter/digit boundaries, which is what
	 * lets *"restart button"* find `RestartButton` — the thing a person remembers is a phrase and
	 * the thing in the file is an identifier.
	 */
	static TArray<FString> WordsOf(const FString& Text);

	// -- S3: the omission ledger (U4) -----------------------------------------------------------

	/**
	 * Can this candidate be searched at all, and if not, what does the answer have to say?
	 *
	 * Decided from registry class data alone — no package is loaded to answer it, which is what
	 * keeps a project-wide sweep affordable.
	 */
	static bool ClassifyCandidate(const FAssetData& AssetData, FGcSearchOmission& OutOmission);

	// -- S4: the search -------------------------------------------------------------------------

	/** Every citation of `Query.Target` in the project, live. */
	static bool FindReferences(const FGcReferenceQuery& Query, FGcSearchResult& OutResult, FString& OutError);

	/**
	 * Search one already-projected document. The half with no editor in it, so it can be specced.
	 *
	 * Appends EVERY match to `OutResult`, scored — it does not know what else the corpus holds, and
	 * a per-document cut is what made `max_results` mean "the first ones found". `RankAndTrim` is
	 * the one place the answer is shortened.
	 */
	static void SearchText(
		const FString& PackageName,
		const FString& Text,
		const TArray<FGcSearchSpelling>& Spellings,
		FGcSearchResult& OutResult);

	// -- S12: ranking ---------------------------------------------------------------------------

	/**
	 * Order every collected hit by `Score`, spread the answer across packages, and cut to `MaxResults`.
	 *
	 * **THE ONE PLACE THE ANSWER IS SHORTENED, AND IT RUNS AFTER THE WHOLE CORPUS HAS BEEN SEEN.**
	 * The truncation used to live in each per-document search, three copies of the same `if
	 * (Hits.Num() >= MaxResults)`, which meant the surviving hits were whichever ones the walk order
	 * reached first — sorted package order, so the answer to a project-wide question came out of
	 * `/Game/A…`. Scoring cannot fix that from inside a document, because a document does not know
	 * what the next one holds.
	 *
	 * `bTruncated` becomes true when hits were cut, and `TotalHits` is the honest total either way.
	 */
	static void RankAndTrim(FGcSearchResult& OutResult, int32 MaxResults);

	/**
	 * The spread term, applied by `RankAndTrim`: the Nth-best hit from one package is demoted by N.
	 *
	 * **FIFTY LINES OUT OF ONE FILE IS A WORSE ANSWER TO "WHERE IS THIS USED" THAN TWENTY FILES.**
	 * Held small on purpose — it reorders within a tier and does not let a weak hit outrank a strong
	 * one, so it breaks a concentration without inventing a relevance claim it cannot support.
	 */
	static float SamePackagePenalty(int32 RankWithinPackage);

	// -- S5 / S8: the corpus kinds ----------------------------------------------------------------

	/**
	 * Project every searchable package under the scope and hand each one's text to `Visit`.
	 *
	 * The shared half of `entity` and `description`, including the omission ledger (U4) — §6's claim
	 * is that both kinds share it, and a second copy of `ClassifyCandidate`'s answer would drift
	 * from the first the day a kind is added.
	 */
	static bool ScanCorpus(
		const FGcCorpusQuery& Query,
		TFunctionRef<void(const FString& PackageName, const FString& Text)> Visit,
		FGcSearchResult& OutResult,
		FString& OutError);

	/** Where is this declared, under what exact spelling, and in which block (§6). */
	static bool FindEntities(const FGcCorpusQuery& Query, FGcSearchResult& OutResult, FString& OutError);

	/** Which authored text in this project talks about this (§7). */
	static bool FindDescriptions(
		const FGcCorpusQuery& Query, FGcSearchResult& OutResult, FString& OutError);

	/**
	 * `entity` over one already-projected document. An empty `Name` lists everything it declares.
	 *
	 * A name differing only in case is a HIT, labelled `declaration (different case)`. This is the
	 * kind whose question is *"what exactly is it called"*, so answering "no such entity" over one
	 * capital is the regression a tightened lookup makes wherever a miss is silent.
	 */
	static void SearchDeclarations(
		const FString& PackageName,
		const FString& Text,
		const FString& Name,
		FGcSearchResult& OutResult,
		FGcCorpusScan& OutScan);

	/** `description` over one already-projected document. Every term must appear on the line. */
	static void SearchAuthoredText(
		const FString& PackageName,
		const FString& Text,
		const TArray<FString>& Terms,
		bool bFuzzy,
		FGcSearchResult& OutResult,
		FGcCorpusScan& OutScan);

	/** The distinct words of a `description` query, single characters dropped. */
	static TArray<FString> QueryTerms(const FString& Query);

	// -- fuzzy (U5) -----------------------------------------------------------------------------

	/** Bounded edit distance. Lexical, never an embedding — the tool card has to be able to say so. */
	static int32 EditDistance(const FString& A, const FString& B, int32 Ceiling);

	/** The entries closest to `Query`, nearest first, at most `Max`, within a length-scaled budget. */
	static TArray<FString> NearMissesFor(
		const FString& Query, const TArray<FString>& Candidates, int32 Max = 5);
};
