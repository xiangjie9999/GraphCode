#pragma once

#include "CoreMinimal.h"
// `FGcSidecarError`, `EGcErrorSeverity` and `GcErrorKind` were declared here until 2026-08-07 and
// live in `GraphCodeErrors.h` now, unchanged. The writer needs them too — the projector emits the
// block rather than only annotating a file afterwards — and it cannot include this header.
#include "GraphCodeErrors.h"
#include "GraphCodeSidecarWriter.h"

// Declared, not included: `Parse` takes it by pointer to default two derived header fields, and
// pulling `GraphCodeProjector.h` in here would make every reader of a `.gc` depend on the thing
// that WRITES one. The projector's header includes this one's neighbours already.
struct FGraphCodeEnvelopeExpectation;

/**
 * One top-level named block, kept verbatim.
 *
 * This splitter deliberately does NOT understand a statement. `widgets { … }` is widgetir/3 and
 * `graph { … }` is blueprintir/2, and both already have a parser that owns their grammar. A third
 * one here would be a third place for the language to drift. So a block's body comes back as the
 * bytes that were in the file, and whoever wants to interpret it hands it to the IR that wrote it.
 */
struct GRAPHCODE_API FGraphCodeBlock
{
	/** The identifier before the brace: `widgets`, `graph`, `properties`, `defaults`, … */
	FString Name;

	/**
	 * EVERYTHING before the brace, which for a LABELLED block is more than the name:
	 * `soundcue SoundCueGraph  sig:02c339…`, `behaviortree BTGraph  sig:…`, `envquery EdGraph  sig:…`.
	 * Equal to `Name` for every plain block, so a caller that reassembles can use this always.
	 *
	 * **THE READER RECOGNISED THE LABEL AND KEPT NOTHING THAT COULD SPELL IT BACK** until
	 * 2026-08-24. `Name` is `soundcue` by design — it is the routing key, and every other consumer
	 * wants exactly that — but the document merge reassembles a block as `Name + " {"`, so a no-op
	 * merge of the corpus's one sound cue came back as `soundcue {` and lost the label and the
	 * graph's own digest. The corpus had no labelled block to catch it until Fire_Cue.gc landed on
	 * 2026-08-23 ([[a-reader-that-spells-owes-the-writer-an-inverse]]).
	 *
	 * The label and its `sig:` are the PROJECTOR's stamp — three plugins write them with three IR
	 * prefixes — so nothing downstream of the reader can recompute one. Carrying it is the only
	 * inverse there is.
	 */
	FString Declaration;

	/** The whole block, from the first character of the name through the matching `}`. */
	FString Text;

	/** What is between the braces, with one leading newline and the trailing indent removed. */
	FString Body;

	/** 1-based lines in the file, for an error a person can navigate to. */
	int32 FirstLine = 0;
	int32 LastLine = 0;

	/** Byte range of `Text` inside FGraphCodeParsedDocument::NormalizedText. */
	int32 Offset = 0;
	int32 Length = 0;
};


/**
 * A parsed `.gc` file.
 *
 * The format shipped write-only: four files project an asset into text and nothing ever read one
 * back, so nothing could notice a header that disagreed with its own body. This is that reader.
 */
struct GRAPHCODE_API FGraphCodeParsedDocument
{
	int32 FormatVersion = 0;

	/** Kept as written. An unrecognised kind is reportable rather than silently coerced. */
	FString KindToken;
	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::Blueprint;
	bool bKnownKind = false;

	FString PackageName;
	FString ParentClassPath;
	FString BlackboardAssetPath;
	/** StateTree `schema:` header class path; empty for every kind but `state_tree`. */
	FString SchemaClassPath;

	/**
	 * The `shape:` header line — the `.gcshape` a generated mesh was built from, and the digest of
	 * the program that built it. Both empty for every asset nobody generated (row M8).
	 *
	 * **READ, AND NEVER APPLIED.** It is a fact about the asset that `kind:static_mesh` reflects; a
	 * payload that states it is not making a claim `gc_apply` can honour, because only a `.gcshape`
	 * build can put the stamp on an asset.
	 */
	FString ShapeSource;
	FString ShapeProgram;

	/** The `move:` line — `shape:`'s twin on a clip a `.gcmove` built. Read, and never applied. */
	FString MoveSource;
	FString MoveProgram;

	/** The `human:` line — the same twin on a character a `.gchuman` built. Read, never applied. */
	FString HumanSource;
	FString HumanProgram;

	/** The `groom:` line — the `.gcgroom` that owns generated Groom/Binding data. Read, never applied. */
	FString GroomSource;
	FString GroomProgram;

	/**
	 * What an ABSENT property line means, when the document says. Empty for every kind but `level`.
	 *
	 * Not defaulted to "archetype" on absence: most kinds have several facet-specific baselines and
	 * no single answer, which is precisely why GC-GRAMMAR-AUDIT.md D6 declined to put the line on
	 * every file. Empty means "this document does not make one global claim", never "archetype".
	 */
	FString Baseline;

	TArray<FString> Omits;
	FString SourceName;
	FString SourceVersion;
	TArray<FString> IrVersions;

	/**
	 * Which revision of the projection code wrote this file — `projector:N` on the `source:` line.
	 *
	 * ZERO MEANS THE FILE DOES NOT SAY, never "the current one". Every sidecar written before the
	 * stamp existed states none, so silence has to read as "go and re-project to find out" — the
	 * same direction an absent `bin:` reads in, and for the same reason.
	 *
	 * Nothing refuses a document over this number. See FGraphCodeProjector::ProjectionRevision for
	 * why it is provenance rather than a compatibility version, and gc_sync for the one gate that
	 * compares it.
	 */
	int32 StatedProjectorRevision = 0;

	/** The digest printed in the header. */
	FString StatedSignature;

	/**
	 * The digest of the `.uasset` this projection was taken from, if the file states one.
	 *
	 * HARD BINDING, and optional on purpose. Every sidecar written before this field existed
	 * has none, and an absent digest means "this file predates the binding, go and re-project
	 * to find out" rather than "the asset is unchanged". Silence must never read as agreement.
	 *
	 * See GC-SIDECAR-DESIGN §14.3. The writer does not emit it yet.
	 */
	FString StatedBinaryDigest;

	/**
	 * The `seal:` value at the end of the first line, verbatim, or empty. Read and kept, never judged
	 * here: what it proves is checked only where a check is asked for (Docs/GC-LICENSING-SEALS-PLAN.md,
	 * "The seal"). A projection never writes one; a sync that would change nothing else keeps it.
	 */
	FString StatedSeal;

	/**
	 * The two parent `sig:` values this document was merged from, when it was merged. Empty is the
	 * normal case, and it is the whole point of the field: **a proposal that says so.**
	 *
	 * GC-VCS-MERGE-DESIGN.md §4.4, ruled in §4.12.1 as that section's option 2. A merged file's
	 * `sig:` is recomputed over the merged body so the FILE is self-consistent, which is right and
	 * has a cost: at rest it is then byte-indistinguishable from an ordinary sidecar whose asset
	 * simply moved. That is the CLEAN-merge hazard, and it is the more dangerous one precisely
	 * because it carries no conflict marker for any grep to find — the developer resolves the
	 * binary by taking ours, saves the asset for an unrelated reason, and the regeneration takes
	 * the merge with it, silently.
	 *
	 * So the predicate the save hook needs is "the `.gc` on disk is a proposal this asset has not
	 * absorbed", conflicted or not, and only a durable statement in the file can answer it. This is
	 * that statement. It says nothing about whether the merge conflicted.
	 *
	 * Cleared by the next regeneration, which is correct: once the asset absorbs the proposal, the
	 * file is a projection again and has no parents.
	 */
	TArray<FString> MergedFromSignatures;

	bool IsMergeProposal() const { return MergedFromSignatures.Num() > 0; }

	/** Exactly the text the digest is taken over: everything below the header's blank line. */
	FString Body;

	/** What that body actually hashes to now. */
	FString ComputedSignature;

	/**
	 * Whether the two agree — that is, whether the TEXT has been edited since it was generated.
	 *
	 * This is half of the sync question and it is worth being exact about which half. Two
	 * independent facts, each with its own digest:
	 *
	 *   this flag             the text still describes the asset GraphCode projected
	 *   StatedBinaryDigest    that asset is still the one on disk
	 *
	 * False here does NOT mean "protect this file". Every save regenerates the sidecar, always,
	 * with no exception — that reflection is the entire product claim, and a hook that skipped
	 * a write to preserve someone's edit would make the file a source of truth, which §9 says
	 * it is not. What false means is that this text describes no asset that exists yet: it is a
	 * proposal in flight, not a record. Applying it makes it real and the next save restamps it;
	 * abandoning it makes regeneration correct. Either way the flag resolves itself.
	 */
	bool bSignatureMatches = false;

	/**
	 * 1-based line numbers of every unresolved `conflict { }` in the body. Empty is the normal case.
	 *
	 * GC-VCS-MERGE-DESIGN.md §4.12. A conflict is GRAMMAR here, not a `//` comment, and the
	 * difference is the whole point: a comment is something every consumer has to remember to look
	 * for, and a construct is something each of them can refuse BY NAME. The four that must —
	 * `gc_apply`, the `GcApply` commandlet, `gc_sync` and `GcCheck` — all read this.
	 *
	 * A CONFLICT IS NOT A PARSE FAILURE, for the same reason a signature mismatch is not: returning
	 * false would leave a caller unable to tell a conflicted file from a corrupt one, and those are
	 * opposite situations with opposite correct responses. The resolve UI in particular has to be
	 * able to READ a conflicted file — refusing to parse it would make the one tool whose whole job
	 * is fixing conflicts the one tool that cannot open them.
	 *
	 * `sig:` over a conflicted body is meaningless, so `bSignatureMatches` is forced false whenever
	 * this is non-empty. A file at rest never contains one; there is no `ir:` bump, because this is
	 * a transient state rather than a dialect.
	 */
	TArray<int32> ConflictLines;

	bool HasConflicts() const { return ConflictLines.Num() > 0; }

	/** The file with CRLF folded to LF, so every offset above indexes into this. */
	FString NormalizedText;

	TArray<FGraphCodeBlock> Blocks;

	/**
	 * Body content belonging to no block, trimmed.
	 *
	 * Empty for everything this build writes. It exists to read what OLDER builds wrote: a user
	 * struct or enum used to project as a flat `struct … / member 0 …` body with no braces
	 * anywhere, and those files are committed, so a repository's history is full of them.
	 *
	 * Driven off the TEXT rather than off the asset kind, deliberately. That is what lets one
	 * scanner stay honest for both shapes, and it means the day a new asset kind projects flat
	 * nothing here needs changing.
	 */
	FString UnblockedText;

	const FGraphCodeBlock* FindBlock(const FString& Name) const;
	bool HasBlock(const FString& Name) const { return FindBlock(Name) != nullptr; }

	/**
	 * Every reason this document is KNOWN not to become an asset, in document order.
	 *
	 * **E11.7a, user-directed:** *"for the gc files that are not compilable yet, have an errors{}
	 * block that states the message of why, and they will be skipped of changes (mostly due to other
	 * modules not bought yet or our functionality not done yet)"*.
	 *
	 * The two facts a caller needs and could not otherwise get:
	 *
	 *  - **It is not the same as "this failed to parse".** An annotated document parses perfectly; a
	 *    tool has written down that turning it into an asset does not work, and re-deriving that
	 *    costs a package load, a factory run or a plugin nobody has.
	 *  - **It is outside `sig:`** (see `SignedBody`), so annotating a file changes no digest and
	 *    trips no staleness gate. That is what lets the note be durable rather than a log line.
	 */
	TArray<FGcSidecarError> Errors;

	bool HasErrors() const { return Errors.Num() > 0; }

	/**
	 * At least one entry says nothing may be read out of this body or written from it.
	 *
	 * **THE ONE PREDICATE EVERY CONSUMER ASKS**, so that "which kinds stop a document" is decided in
	 * `GcErrorKind::SeverityOf` and nowhere else. A caller testing `HasErrors()` instead would skip
	 * a file over a recommendation.
	 */
	bool HasBlockingError() const;

	/**
	 * The body could not be split into blocks at all, and only `errors { }` was recovered from it.
	 *
	 * True only alongside `HasBlockingError()`, and it is the case the severity split exists for: a
	 * stub sidecar for an asset whose plugin nobody has does not have to pretend to hold a coherent
	 * body. `Blocks` holds the one block that was legible; nothing else about the document is known.
	 */
	bool bBodyUnread = false;

	/** Every entry as one line, for a footer or a tooltip. Empty when there are none. */
	FString ErrorSummary() const;

	/** The blocking entries only, one line, for a refusal that has to say what stopped it. */
	FString BlockingErrorSummary() const;
};

/** Reads a `.gc` envelope and splits its named blocks. The inverse of FGraphCodeSidecarWriter. */
class GRAPHCODE_API FGraphCodeSidecarReader
{
public:
	/**
	 * Parse a complete document.
	 *
	 * A signature mismatch is NOT a parse failure. It is the answer to a question the caller
	 * asked, and returning false would leave that caller unable to tell an edited file from a
	 * corrupt one — which are opposite situations with opposite correct responses.
	 *
	 * @param bRecipe  this document describes an asset that does not exist, so `omits:` and
	 *                 `source:` are EXCUSED. C21.5, §16.2.
	 *
	 *                 **BOTH ARE FACTS ABOUT A PROJECTION, and a recipe is not one.** `omits:` is
	 *                 the created asset's own envelope and `source:` is the provenance of a
	 *                 projection that has not happened, so requiring them of a create demands two
	 *                 things the author cannot know before the asset exists. Everywhere else they
	 *                 stay mandatory and for the original reason: a document that does not say what
	 *                 it leaves out cannot be read as "nothing changed" when it shows no diff.
	 *
	 *                 It does NOT relax anything else. `gc 1`, `kind:` and `asset:` are still
	 *                 required, a malformed line is still a refusal, and the signature is still
	 *                 computed and compared.
	 *
	 * @param EnvelopeDefaults  what the projector would write for THIS asset, or null. When the
	 *                 document states no `omits:`/`source:` and this is supplied, the two derived
	 *                 fields are filled from it instead of the document being refused.
	 *
	 *                 **ABSENT MEANS "USE CURRENT"; STATED MEANS "HONOUR IT".** A stated `omits:`
	 *                 is still parsed, kept and compared, so the cross-version signal is untouched
	 *                 — a stored sidecar always carries the line, because the writer always writes
	 *                 one. The fill only ever reaches a document that claimed nothing, which is the
	 *                 hand-written case where the claim would have been transcription.
	 *
	 *                 **IT IS NOT AN AUTHOR'S CLAIM TO BEGIN WITH.** `omits:` is derived from the
	 *                 projector — `FGraphCodeProjector::ExpectedEnvelope`, decided by the KIND
	 *                 alone — so on an edit the thing applying the document already knows the
	 *                 answer, and demanding it asks the author to tell the tool a fact about
	 *                 itself. C21.5 made exactly this move for a create; this is the same rule one
	 *                 case further along, and the same seam carries it.
	 *
	 *                 **ONLY THE TWO DERIVED FIELDS ARE FILLED — never the provenance.**
	 *                 `SourceName`, `SourceVersion` and `StatedProjectorRevision` stay as the
	 *                 document left them, because zero on that last one is documented to mean "the
	 *                 file does not say" and must keep meaning it. Filling provenance would
	 *                 manufacture a claim about which projector wrote text that projector never saw.
	 */
	static bool Parse(
		const FString& Text,
		FGraphCodeParsedDocument& OutDocument,
		FString& OutError,
		bool bRecipe = false,
		const FGraphCodeEnvelopeExpectation* EnvelopeDefaults = nullptr);

	static bool ParseFile(
		const FString& SidecarPath,
		FGraphCodeParsedDocument& OutDocument,
		FString& OutError,
		bool bRecipe = false,
		const FGraphCodeEnvelopeExpectation* EnvelopeDefaults = nullptr);

	/**
	 * Substitute one named block and leave every other byte of the file alone.
	 *
	 * The header's `sig:` is deliberately left stale, because the result is a proposal and not
	 * a projection. Restamping would let text that matches no asset claim to be a faithful
	 * mirror of one, which is the single thing this format cannot be allowed to do. The stale
	 * digest is how the next reader — a tool, a commit hook, a person — can tell that this file
	 * is waiting on an apply. It clears itself when the applied asset is saved and the sidecar
	 * is regenerated from the binary, which is the only way a digest here is ever legitimate.
	 *
	 * `NewBlockText` must itself be exactly one block with the same name, so a `graph { … }`
	 * cannot be spliced into the slot where `widgets { … }` was.
	 */
	static bool ReplaceBlock(
		const FString& Text,
		const FString& BlockName,
		const FString& NewBlockText,
		FString& OutText,
		FString& OutError);

	/**
	 * Does this rendered block text open the block called `Name`?
	 *
	 * The name is matched as a TOKEN — name, then whitespace or `{` — because a header may carry a
	 * label: `soundcue SoundCueGraph  sig:<digest> {`. `ScanBlocks` admits that shape from any kind,
	 * so a caller matching the `Name + " {"` prefix instead silently fails to see one.
	 */
	static bool IsBlockNamed(const FString& BlockText, const FString& Name);

	/**
	 * Take the braces off a standalone block, or pass a bare body through unchanged.
	 *
	 * A caller that edited a gc_read result holds `widgets { … }`; one that assembled a tree by
	 * hand usually holds the lines alone. Both are the same patch, so both are accepted — but a
	 * wrapper naming a DIFFERENT block is refused rather than unwrapped, because that is a patch
	 * aimed at the wrong slot and unwrapping it silently is how it lands there.
	 */
	static bool UnwrapBlock(
		const FString& Text,
		const FString& ExpectedName,
		FString& OutBody,
		FString& OutError);

	/** `blueprint` → EGraphCodeAssetKind::Blueprint. False for a token this build does not know. */
	static bool TryParseKind(const FString& Token, EGraphCodeAssetKind& OutKind);

	/**
	 * Put an `errors { }` block on a document, replacing any it already has. E11.7a.
	 *
	 * **`sig:` IS NOT RESTAMPED AND MUST NOT BE.** The block is outside the digest by construction
	 * (`SignedBody`), so the returned text states exactly the signature it arrived with — which is
	 * the whole reason a durable note is possible at all. Restamping would make the file claim to be
	 * a projection of a state that no asset has.
	 *
	 * It goes LAST, after every projected block, because it is the only block that is about the
	 * DOCUMENT rather than about the asset, and because a reader scrolling to the bottom of a file
	 * they were told is broken should find the reason there.
	 *
	 * Aliases are minted here, `e1` upward in the order given, so a caller never has to invent one
	 * and two callers cannot invent different ones.
	 */
	static bool SetErrors(
		const FString& Text,
		const TArray<FGcSidecarError>& Errors,
		FString& OutText,
		FString& OutError);

	/** Take the block off. A no-op returning the input when there is none. */
	static bool ClearErrors(const FString& Text, FString& OutText, FString& OutError);

	/** Render one entry the way the block writes it, so a spec can compare against one string. */
	static FString RenderError(const FGcSidecarError& Error);
};
