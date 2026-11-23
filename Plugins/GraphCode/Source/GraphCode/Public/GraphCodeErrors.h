#pragma once

#include "CoreMinimal.h"

/**
 * The `errors { }` block's types, in a header BOTH the reader and the writer can include. E11.7a,
 * and moved here on 2026-08-07 when the projector gained a reason to write the block rather than
 * only annotate a file after the fact (`GC-PRODUCT-PACKAGING-DESIGN.md`).
 *
 * Nothing about them changed in the move. They lived in `GraphCodeSidecarReader.h` because reading
 * an annotation was the only thing anybody did with one, and `GraphCodeSidecarWriter.h` cannot
 * include that header — the dependency runs the other way.
 */

/**
 * What an `errors { }` entry MEANS for the document it sits in. E11.7a.
 *
 * **USER-DIRECTED 2026-08-05:** *"the error kinds tell the parser, whether to even parse the whole
 * document, because it might just be some recommendations to fix"*. Exactly — and without this the
 * block would have been a trap in both directions: a stub sidecar for an asset whose plugin nobody
 * has would be handed to writers that cannot possibly succeed, and a note reading *"this graph would
 * be clearer split in two"* would silently take an asset out of every restore in the project.
 *
 * So the KIND decides, and the kind is the one token that has to be classified rather than kept
 * verbatim. See `SeverityOf` for what an unrecognised one gets.
 */
enum class EGcErrorSeverity : uint8
{
	/**
	 * Something worth fixing. The document is still a valid projection and everything still applies.
	 *
	 * A reader shows it; no gate acts on it.
	 */
	Advice,

	/**
	 * Nothing may be read out of this document's body, and nothing may be applied from it.
	 *
	 * The strong form, and it is strong on purpose: it is what a sidecar for an asset this editor
	 * cannot even load has to say. The body is not to be trusted as a description of anything —
	 * `bBodyUnread` is the case where it could not be split at all and only this block was recovered.
	 */
	Blocking,
};

/**
 * One reason this document cannot become an asset. An entry of the `errors { }` block. E11.7a.
 *
 * **USER-DIRECTED 2026-08-05:** *"maybe call errors instead of error? then e1 =
 * GeneralError(Name:"Not Supported", Message:"")"*. Both halves of that are the right shape and the
 * singular block written first was not:
 *
 *  - **Plural, because there is rarely one reason.** A sidecar for an asset from a plugin nobody has
 *    is blocked by the missing module AND by a block GraphCode cannot write; recording only the
 *    first sends somebody to install a plugin that does not fix it.
 *  - **An aliased entry, because that is what every other list in this format is.** `n1 = Event …`,
 *    `c1 = …`. An entry with an alias can be named by a refusal, cited in a report, and matched
 *    across two versions of the file; a bare paragraph of prose can only be re-read.
 *
 * `Kind` is the constructor — `GeneralError`, `MissingModule`, `Recommendation`. It is kept
 * VERBATIM as well as classified: a file written by a later build may name a kind this one has never
 * heard of, and the whole purpose of the block is that such a file is skipped with its reason
 * readable, not rejected as unparsable.
 */
struct GRAPHCODE_API FGcSidecarError
{
	/** `e1`, `e2` — document order, minted from 1, the same convention as `n1`/`c1`. */
	FString Alias;

	/** `GeneralError`, `MissingModule`, `Recommendation`, or whatever a later build minted. */
	FString Kind;

	/** What `Kind` means for the document. `GcErrorKind::SeverityOf` decides it. */
	EGcErrorSeverity Severity = EGcErrorSeverity::Blocking;

	/** The short label a list can show: `Not supported`, `VRExpansionPlugin`. */
	FString Name;

	/** The sentence a person acts on. */
	FString Message;
};

/**
 * The kinds THIS build mints, and what each one does to the document.
 *
 * **A CLOSED VOCABULARY WITH AN OPEN DOOR.** The tokens below are the ones GraphCode writes and the
 * ones `SeverityOf` classifies; an entry naming anything else is still READ — alias, name and
 * message all survive — and treated as `Blocking`. That default is the asymmetry, not timidity: an
 * unknown kind that turns out to have been advice costs one asset skipped and said so, and an
 * unknown kind that turns out to have been blocking costs a write into an asset a later build knew
 * could not take one.
 */
namespace GcErrorKind
{
	/** No narrower kind fits, and it stops the document. `Name` carries the short label. */
	inline const TCHAR* const General = TEXT("GeneralError");

	/**
	 * A plugin or module this project does not have, so the asset's class cannot be resolved.
	 *
	 * Also the WHOLE-DOCUMENT form of a missing GraphCode product: a stub sidecar for a kind Core
	 * cannot read at all. Its advisory sibling is `RequiresProduct`, and the split is deliberate —
	 * see there.
	 */
	inline const TCHAR* const MissingModule = TEXT("MissingModule");

	/** GraphCode itself cannot write the facet yet. Ours to fix, not the reader's. Blocking. */
	inline const TCHAR* const NotImplemented = TEXT("NotImplemented");

	/**
	 * ADVICE. Something worth fixing that stops nothing — the reason severity exists.
	 *
	 * A note reading *"three of these properties are unused"* is worth carrying in the file and must
	 * never take the asset out of a restore.
	 */
	inline const TCHAR* const Recommendation = TEXT("Recommendation");

	/** ADVICE. A deprecation a later build will refuse; today it is a warning and nothing more. */
	inline const TCHAR* const Deprecated = TEXT("Deprecated");

	/**
	 * ADVICE. A NAMED FACET of this document is carried by a GraphCode product that is not
	 * installed. Everything else in the file projects and applies normally.
	 *
	 * **USER-DIRECTED 2026-08-07:** *"like having omit and non blocking errors, just to let them
	 * know the specific graphs like animfunction cant be compiled, need move"*.
	 *
	 * The advisory sibling of `MissingModule`, and the split is the whole point. `MissingModule` is
	 * for an asset whose class this editor cannot resolve at all — nothing may be read out of it.
	 * This one is for an Animation Blueprint on a Core-only install: its variables, event graph,
	 * ordinary functions, components and class defaults are all real, all diff and all apply, and
	 * only `animgraph { }` is absent. Marking that BLOCKING would take a whole asset out of version
	 * control over one block.
	 *
	 * The machine-readable half is `omits: <facet>@<product>`; this is the sentence beside it.
	 * `GC-PRODUCT-PACKAGING-DESIGN.md` §2.
	 */
	inline const TCHAR* const RequiresProduct = TEXT("RequiresProduct");

	/**
	 * ADVICE. What the engine's compiler said about this asset the last time it was compiled.
	 * §11.1b, and this is the one kind whose severity is a RULING rather than a description.
	 *
	 * **ADVISORY, NEVER BLOCKING, and the deadlock is the argument.** A `Blocking` entry makes
	 * `gc_apply` skip the document. A failed compile that stamped one would make the asset that
	 * most needs a fix the one asset GraphCode cannot write — reachable only by fixing the thing by
	 * hand, which is the loop the text surface exists to escape. So a broken Blueprint carries its
	 * errors in the file AND stays writable, and `GraphCodeApplyTests`' gate spec pins that.
	 *
	 * **THE COMPILE OWNS THESE ENTRIES AND NOTHING ELSE TOUCHES THEM.** Every compile replaces its
	 * own and a clean one removes them; entries of any other kind in the same block are left
	 * exactly as they were. An `errors { }` that outlives the compile that fixed it is a file that
	 * lies, and staleness — not writing — is the row.
	 *
	 * Unlike every other token here this one is about the ASSET rather than about the document,
	 * which is the subject widening §11.1b ruling 3 called for. Same block, distinct token, so the
	 * two can never be read as each other.
	 */
	inline const TCHAR* const CompileError = TEXT("CompileError");

	/**
	 * ADVICE. What the ENGINE said while LOADING this asset's package — and it is the one thing a
	 * clean compile cannot tell you.
	 *
	 * **MEASURED 2026-08-13, and the gap is why this token exists.** `UW_LightZoneInfo` logged three
	 * `FStructProperty::Serialize … Unknown structure` errors every time its package was loaded,
	 * while `CompileError` stayed empty, the projection stayed `canonical` and the referenced struct
	 * read clean on its own. `FCompilerResultsLog` is the compiler's listing; a serialization failure
	 * happens during LOAD, before the compiler is involved, so it can never enter that listing. Every
	 * GraphCode instrument said the asset was healthy while the engine was logging errors reading it.
	 *
	 * **ADVISORY FOR THE SAME REASON `CompileError` IS**, and more sharply: an asset that fails to
	 * load is exactly the asset somebody needs to write to. Blocking would shut the only door out.
	 *
	 * **A SESSION RECORD, NOT A FILE FACT — and that governs when it is stamped.** A package loads
	 * once per session, so the errors are emitted once and never again no matter how many times the
	 * asset is projected. The ledger behind this token is filled by whoever loads first and read
	 * afterwards, which is what makes the note survive to the write. It follows that the entries are
	 * only meaningful next to a projection that actually loaded the asset — so that is the only place
	 * they are written, and a clean load there CLEARS them.
	 */
	inline const TCHAR* const LoadError = TEXT("LoadError");

	/** `Advice` for the five advisory tokens above; `Blocking` for everything else, known or not. */
	GRAPHCODE_API EGcErrorSeverity SeverityOf(const FString& Kind);
}

/** Rendering the block, in the one place both the projector and the annotator reach for it. */
class GRAPHCODE_API FGcErrorBlock
{
public:
	/** One entry, exactly as the block writes it. `e1 = MissingModule(Name:"…", Message:"…")`. */
	static FString RenderEntry(const FGcSidecarError& Error);

	/**
	 * The whole `errors { }` block, aliases minted `e1` upward, no trailing newline. Empty in,
	 * empty out.
	 *
	 * **THE ALIAS IS MINTED HERE, never taken from the caller.** `n1`/`c1`'s rule: an identity two
	 * callers can each invent is an identity that collides the first time two of them write.
	 */
	static FString Render(const TArray<FGcSidecarError>& Errors);

	/**
	 * Replace every entry of ONE kind with `Entries`, leaving every other kind exactly as it was.
	 *
	 * **ONE IMPLEMENTATION BECAUSE THERE ARE TWO PRODUCERS NOW.** The compile annotator owned this
	 * logic while it was the only writer; `LoadError` made it the second, and two copies of *replace
	 * my kind, keep yours* is how one producer starts eating the other's entries after somebody edits
	 * only one of them. The kind is a parameter for exactly that reason.
	 *
	 * Returns false and leaves `OutText` EMPTY when nothing would change. The caller must not write
	 * then: a rewrite moves the file's mtime and the panel's working-tree fingerprint reads mtime, so
	 * an idle pass would make the next refresh look like somebody had worked.
	 */
	static bool MergeKind(
		const FString& Text,
		const TCHAR* Kind,
		const TArray<FGcSidecarError>& Entries,
		FString& OutText,
		FString& OutError);
};
