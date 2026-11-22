#pragma once

#include "CoreMinimal.h"

#include "GraphCodeProducts.h"
#include "GraphCodeSidecarWriter.h"

/**
 * What GraphCode can actually do, in one place, so nothing else has to say it. C1 of
 * `GRAPHCODE-CORE-QUEUE.md`; `GC-CAPABILITY-TRUTH-AUDIT.md` is the measurement that forced it.
 *
 * WHY THIS EXISTS. The audit enumerated the domain — every kind, every block, every tool card, every
 * `omits:` token — and found **twenty-nine** places where committed prose states a capability
 * differently from the code. Seven of them can cost you work: `gc_describe` telling a caller that a
 * block the projector emits is omitted, which is the false no-diff `omits:` exists to forbid; and
 * `gc_sync`'s catalog card, the model's only pre-editor view, saying GraphCode covers "Blueprints,
 * Widget Blueprints, user structs and user enums only" when 133 of this project's 350 sidecars
 * belong to kinds that sentence says do not exist.
 *
 * None of that was carelessness. Six of the discrepancies are in one file and three of those six
 * contradict another paragraph of the same file, which is the finding in one sentence: **nobody
 * keeps four hand-written surfaces in step by reading them, because reading them is how you learn
 * they agree with each other rather than with the code.**
 *
 * WHAT IS DERIVED AND WHAT IS DECLARED, because the split is the whole design.
 *
 * DERIVED, and never restated here: the kind set (walked `0 .. EGraphCodeAssetKind::Count`), the
 * token (`FGraphCodeSidecarWriter::KindToken`), `omits:` and `ir:`
 * (`FGraphCodeProjector::ExpectedEnvelope`). A second copy of any of those would be a second thing
 * to keep in step, which is the disease.
 *
 * DECLARED, because it cannot be derived: the block set and each block's write status. The block
 * set exists only as the sequence of `Blocks.Add` calls in `BuildDocument`, which is imperative and
 * whose ORDER is load-bearing (`GC-SIDECAR-DESIGN.md` §13.9); write status is decided by eleven
 * kind gates inside `ApplyBlock`, and routable is not writable. So the table below is written down —
 * and `GraphCode.Capabilities.*` asserts it against a real projection and against the router, which
 * works for the reason the omission ledger's spec works: **it compares a declaration to a
 * projection, never a declaration to another declaration.**
 *
 * NOTHING HERE DECIDES ANYTHING. No projector, writer or applier consults this table; they remain
 * the truth and this describes them. A registry that started gating writes would be a second
 * implementation of the eleven gates, and the day the two disagreed the safer one would lose.
 */

/** Whether `gc_apply` writes a block, and if not, why not. */
enum class EGcBlockAccess : uint8
{
	/** Projected, and `gc_apply` writes it. */
	Writable,

	/**
	 * Projected, and `gc_apply` refuses it BY NAME.
	 *
	 * By name matters and is not decoration: `settings` is writable on structs, enums, base
	 * Materials, Material Functions, data tables, curves and both input kinds, so a Blueprint
	 * `settings` payload that merely fell through would reach a writer that resolves nothing and
	 * reports success.
	 */
	ReadOnly,

	/**
	 * Projected on every asset of the kind; written only where `WriteCondition` holds.
	 *
	 * One case exists and it is the one every stale document gets wrong. `components` and
	 * `class_defaults` widened to every Blueprint on 2026-08-02 and WRITING did not, so they are the
	 * only thing in GraphCode whose read scope and write scope differ. A two-value status cannot say
	 * that, and every surface that tried said "omitted" instead — which is the opposite of true.
	 */
	WritableIf,
};

/**
 * What OMITTING an entry from a payload means for this block. `GC-VCS-MERGE-DESIGN.md` §4.2, M2.2.
 *
 * §4.2 measured this across the writers and called it "five deletion policies, undeclared". This is
 * the declaration, and building it found the measurement wrong in two ways that matter — both of
 * the kind this file exists to catch, a document agreeing with another document rather than with
 * the code:
 *
 *   - `actors` is listed there as omission-means-KEEP. It has refused since 2026-07-31, two days
 *     BEFORE that table was written; the level writer says "omission is not deletion" by name.
 *   - the table names two blocks that refuse. FOUR do: `actors`, `rows`, `components` and
 *     `widgets`, each with its own `delete` token. That is not a detail, because `Refuses` is
 *     exactly the set where `mode:"snapshot"` changes anything — so the mode's reach was
 *     understated by half, and a merged Blueprint or Widget `.gc` would still have had no way to
 *     express a deletion.
 *
 * NOTHING CONSULTS THIS TO DECIDE. Same invariant as the rest of the file: the writers own the
 * behaviour and this describes it, with `GraphCode.Capabilities.*` asserting the two agree.
 */
enum class EGcOmissionPolicy : uint8
{
	/** The block has no writer, so omitting from it means nothing. Every `ReadOnly` block. */
	NotWritable,

	/**
	 * A property bag whose absent line means the BASELINE rather than a removal, and which has NOT
	 * been given the choice §3 says it should have. `settings`, `values`, `parameters`.
	 *
	 * **NOT "a mode is meaningless here" — that was this comment's claim and C23 §3 measured it
	 * false.** A bag's entries are its PROPERTIES, so there is something named to return to its
	 * default and both modes are expressible: patch touches only what the payload states, snapshot
	 * reads the payload as the whole set of deviations.
	 *
	 * **NO BLOCK CARRIES THIS POLICY ANY MORE, as of 2026-08-10.** C23 §3 emptied it: `class_defaults`,
	 * then `settings` on all seven of its routes, then `values`, then `parameters`. It is kept rather
	 * than deleted because the token `resets_to_default` is published — it is quoted in
	 * `graphcode-capabilities.json`, in every tool card generated from it and in committed docs — so
	 * removing the value is a breaking vocabulary change and a doc sweep, not an enum edit
	 * (see the `vocabulary-change-hides-from-green-suite` treatment). **Do not reintroduce it for a
	 * new block**: a property bag whose entries can be named has both modes, and that is what
	 * `Refuses` says.
	 */
	ResetsToDefault,

	/** Omitting a named entry REMOVES it. Already snapshot semantics: `graph`, `properties`. */
	Deletes,

	/**
	 * Omitting a named entry is REFUSED, and the refusal names it: `mode:"snapshot"` is what changes
	 * that, and this is the only policy where the mode changes behaviour rather than restating it.
	 *
	 * `actors`, `rows`, `components`, `widgets`, `timelines`, `functions`, `macros`, `sockets`,
	 * `mapping_profiles`, `class_defaults` — the count grows, which is the point: this is *the
	 * caller chooses*, and a block joins it when its entries turn out to have identity.
	 *
	 * **THE REMOVAL TOKEN IS NOT ALWAYS `delete`.** It is whatever undoes an entry in that block:
	 * `delete <Name>` where the entry is an entity the document owns, and `reset <Path>` where the
	 * entry is a property, which cannot be deleted and can only go back to its baseline.
	 */
	Refuses,

	/** Omitting leaves the entry alone. `interfaces` — the class owns the implementation list. */
	Preserves,

	/**
	 * The elements have NO names, so the LIST is the value and a short payload silently shortens
	 * it. The input lists. Snapshot by construction and unable to be anything else: "the payload
	 * omits an element" is not a sentence you can write without names.
	 */
	Truncates,

	/**
	 * The entry set is CLOSED by the asset's class: adding is refused, dropping is a truncated
	 * payload, and there is no `delete`. `curves`. Supports neither mode.
	 */
	Fixed,
};

/**
 * One specifier an ENTRY in a block can carry, and what dropping it does.
 *
 * A block's `omission` says what dropping a whole ENTRY does; this says what dropping one specifier
 * off an entry does, which is a different question with a different answer. Both are needed to
 * predict a payload: `functions` refuses a missing function, and a missing `category=` on a function
 * that IS there resets the category.
 *
 * `Default` is the point of the struct. Omission RESETS, so a caller who cannot see the default
 * cannot predict what dropping a specifier does — and `title_color` defaults to WHITE rather than
 * the zero a reader assumes, which is exactly why prose cannot carry this and a table must.
 */
struct GRAPHCODE_API FGcEntitySpecifier
{
	/** The token as written, e.g. `category`. */
	FString Token;
	/** What it sets, by the engine panel's own label. */
	FString Sets;
	/** `switch`, `text` or `color`. A switch is bare; the others take `=value`. */
	FString Type;
	/** What OMITTING it resets the field to, spelled the way the document would write it. */
	FString Default;
};

struct GRAPHCODE_API FGcBlockCapability
{
	FString Name;

	EGcBlockAccess Access = EGcBlockAccess::ReadOnly;

	/**
	 * REQUIRED of every writable block, by the constructor helpers, so a block cannot be added
	 * without deciding. A default here would be the additive-list failure again: the wrong answer,
	 * silently, for whichever block the next person adds.
	 */
	EGcOmissionPolicy Omission = EGcOmissionPolicy::NotWritable;

	/** Non-empty only for `WritableIf`. A stable token, e.g. `actor_blueprint`. */
	FString WriteCondition;

	/**
	 * What the writer refuses INSIDE an otherwise-writable block.
	 *
	 * Carried because "writable" with a silent hole in it is the same class of half-truth this file
	 * exists to end: a Material Instance's `parameters` writes runtime override families and refuses
	 * static permutations, and a caller who reads only the status word plans the wrong edit.
	 */
	TArray<FString> Refuses;

	/** One sentence, for a human or a model. Empty where the status says everything. */
	FString Note;

	/**
	 * The specifiers one ENTRY in this block may carry, where the block has entries that carry any.
	 *
	 * GENERATED from the same table the emitter and parser read, never hand-written beside them.
	 * A hand-kept copy is additive — it can list what exists and cannot notice what it is missing —
	 * and here the missing thing has a specific cost: `functions` and `macros` accept DIFFERENT
	 * sets, so a single list with a footnote would offer a macro five specifiers the engine reads
	 * from nowhere, and a payload stating one applies clean and changes nothing.
	 */
	TArray<FGcEntitySpecifier> EntrySpecifiers;

	/** True when the block appears only on a non-canonical live read, never in a committed `.gc`. */
	bool bNonCanonicalOnly = false;

	/**
	 * True when a committed `.gc` carries this block ONLY IF THE ASSET HAS THE THING IT DESCRIBES.
	 *
	 * THE REGISTRY HAD TWO ANSWERS FOR PRESENCE AND NEEDED THREE. `bNonCanonicalOnly` says *never in
	 * a file*; its absence said *in every file of this kind*. `level_blueprint` is neither: a map with
	 * a script projects one and a map without projects nothing, and **most maps have no script**, so
	 * the commonest level in any project was being described by a registry that promised a block it
	 * would not find.
	 *
	 * IT IS A FACT ABOUT THE ASSET, NOT ABOUT THE WRITER, which is why it is not another
	 * `EGcBlockAccess`. `WritableIf` already says *this block can be written when a condition holds*;
	 * this says *this block EXISTS when the asset does*. A caller planning an edit needs the first; a
	 * caller reading a file needs the second, and reading the first as the second is what made an
	 * unscripted map look like a projector defect.
	 *
	 * WHAT IT DOES NOT LOOSEN: order, and absence of anything else. A conditional block that IS
	 * present must still appear where the registry lists it, and every unconditional block must still
	 * be there. It buys exactly one thing — absence, for the block that declares it.
	 */
	bool bConditionallyPresent = false;

	/**
	 * Which GraphCode product ships this block's codec. `GC-PRODUCT-PACKAGING-DESIGN.md`.
	 *
	 * DERIVED from `FGraphCodeProducts::OwnerOfBlock` and never declared in the table below, because
	 * one ownership table is the whole rule. `animgraph` is Move's on a kind that is Core's, which
	 * is why the field is here rather than only on `FGcKindCapability`.
	 */
	EGcProduct Product = EGcProduct::Core;
};

/**
 * WHETHER GRAPHCODE CAN BUILD ONE OF THESE FROM NOTHING, OR PUT ONE BEYOND REACH RECOVERABLY.
 * `GC-LOCAL-HISTORY-DESIGN.md` §2.7, E11.7a.
 *
 * **THREE CAPABILITIES, NOT THREE SPELLINGS OF ONE.** `gc_apply` mutates a resolved existing asset.
 * It cannot prove that a `.gc` is a sufficient RECIPE for a kind — a Data Asset's whole authored
 * content is `values { }`, which has no writer, so a created one would be an empty shell wearing the
 * right class — and it certainly cannot prove that destroying one is safe. So the registry answers
 * them separately, and a planner that wants to offer `Create <Asset>` has to find `create` here
 * rather than infer it from `apply`.
 *
 * WHAT THIS FIELD IS AND IS NOT. It is the STATIC question: does this kind have a greenfield factory,
 * and does its `.gc` carry the facts that factory needs? The DYNAMIC question — is *this particular
 * document* a sufficient recipe — is answered by actually building one on a scratch package and
 * reading it back (`FGcGreenfield::Plan`). Splitting them is what keeps this table honest: a `true`
 * here is a claim about the kind, and the scratch run is the claim about the file.
 */
struct GRAPHCODE_API FGcLifecycleCapability
{
	bool bSupported = false;

	/**
	 * Why not, in the words a reader can act on. **Non-empty exactly when `bSupported` is false**,
	 * because "no" with no reason is the refusal a person re-derives every time they meet it.
	 */
	FString Refusal;

	/** One sentence about what the supported path actually does. Empty where the word says it all. */
	FString Note;

	/**
	 * A `parent:` value that WORKS for this kind. EMPTY MEANS THE KIND NEEDS NONE. C21.4.
	 *
	 * **THE FLOOR IS PER KIND, WHICH IS WHY THIS IS A FIELD RATHER THAN A SENTENCE IN `Note`.** A
	 * `user_struct` needs nothing beyond the header, a Blueprint needs a class, a `material_instance`
	 * needs a loadable material — so "what is the least I can write" has a different answer per kind
	 * and a refusal that teaches has to state THAT kind's. Machine-readable in the only way that
	 * matters here: empty or not.
	 *
	 * **A CONCRETE VALUE, NEVER A DESCRIPTION OF ONE, and that is measured rather than stylistic.**
	 * The first version held prose — *"a class this editor can load and parent a Blueprint to"* — and
	 * the refusal built from it printed a recipe with an angle-bracketed blank where the class goes.
	 * A spec that ran that recipe verbatim could not create anything, which is the whole test of
	 * whether a teaching refusal teaches: if the lines it hands back do not work, it is worse than
	 * the parser diagnostic it replaced, because a caller acts on it.
	 */
	FString ParentRequirement;

	/**
	 * The BLOCK lines a document of this kind needs on top of the header, verbatim. Empty for every
	 * kind whose floor the header covers, which is most of them. 11e step 6.
	 *
	 * **THE FIELD ABOVE ASSUMED A KIND'S FLOOR IS ONE `parent:` LINE, AND `animation_blueprint` IS
	 * WHERE THAT STOPPED BEING TRUE.** A skeleton is a second constructor argument and it lives in
	 * `settings { }`, so a recipe built from `ParentRequirement` alone is a document that refuses —
	 * and `HeaderTemplate` hands exactly that recipe back as the lines that WOULD have worked. Which
	 * is worse than a parser diagnostic, for the reason the comment above already spells out.
	 *
	 * `data_table` names a `settings { }` value too and needs nothing here: its factory starts from
	 * `FTableRowBase` and the snapshot apply moves it, so a header-only document does build. The
	 * distinction is whether the kind has a SAFE DEFAULT for the fact, not whether the fact is in a
	 * block — a skeleton has none, and picking one would bind an Animation Blueprint to a skeleton
	 * nobody named.
	 *
	 * Concrete text, on `ParentRequirement`'s rule and for its reason: a spec runs `HeaderTemplate`
	 * verbatim, so a placeholder here is a recipe that cannot be executed.
	 */
	FString RequiredBody;

	/**
	 * HEADER lines this kind's recipe needs beyond `parent:`, verbatim and complete (`key: value`).
	 *
	 * **`ParentRequirement` ASSUMED THE ONLY REQUIRED HEADER LINE IS `parent:`, AND `state_tree` IS
	 * WHERE THAT STOPPED BEING TRUE.** `UStateTreeEditorData::Schema` is `Instanced, NoClear` with
	 * no default and the engine's own factory returns NOTHING without one, so `schema:` is not a
	 * setting with a safe fallback — it is a constructor argument. And it cannot ride in
	 * `RequiredBody`, which is inserted after the header's terminating blank line and is therefore
	 * body.
	 *
	 * The same test the two fields above take: concrete values, never placeholders, because a spec
	 * runs the template verbatim and a recipe that refuses is worse than no recipe at all.
	 */
	TArray<FString> HeaderLines;
};

struct GRAPHCODE_API FGcKindCapability
{
	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::Blueprint;

	/** Derived from `FGraphCodeSidecarWriter::KindToken`. */
	FString Token;

	/**
	 * Greenfield construction: the target has this asset and the project does not (§2.7 `Create`).
	 *
	 * Refused for a kind whose `.gc` is not a sufficient recipe — an imported or binary-backed kind
	 * whose bytes are the asset, or a partially represented one whose writable blocks do not add up
	 * to the thing a person would recognise.
	 */
	FGcLifecycleCapability Creation;

	/**
	 * Recoverable removal: the project has this asset and the target does not (§2.7 `Remove`).
	 *
	 * **NEVER A FILE DELETE**, which is why the field is not called `delete`. The asset moves to an
	 * excluded content trash and its sidecar under `Saved/GraphCode/RecoveryTrash/`, with a manifest
	 * recording both. A `.gcc` omits whatever the format omits, so the moved `.uasset` — not the text
	 * snapshot — is what preserves the binary and read-only facets. GraphCode never empties it.
	 */
	FGcLifecycleCapability Removal;

	/**
	 * False for `Import` alone: it describes an asset that does not exist yet, nothing writes one on
	 * save, and `gc_sync` has nothing to sync.
	 */
	bool bPersistentSidecar = true;

	/**
	 * Whether an ordinary editor save regenerates this kind's sidecar.
	 *
	 * **TRUE FOR EVERY KIND THAT HAS AN ASSET, INCLUDING `level`.** This field said `false` for
	 * levels until 2026-08-05, on the stated grounds that `IsRealProjectAssetSave` demands the
	 * package's real `.uasset` and a `.umap` fails that check. It does not: the predicate takes
	 * `bContainsMap` and picks `GetMapPackageExtension()` for it, `f714f14` made that so on
	 * 2026-08-03, and `GraphCode.Policy.RealProjectSavesOnly` had been asserting it ever since.
	 *
	 * A REFUTED EXPLANATION IS NOT A REFUTED CONCLUSION, so it was measured rather than reasoned
	 * about: `SavingALevelRegeneratesItsSidecarLikeEveryOtherKind` saves a probe map, gets a
	 * `kind:level` sidecar, spawns one actor, saves again, and the sidecar changes. The second save
	 * is the load-bearing half — the first could have been the CREATE hook with the save path dead.
	 *
	 * FALSE remains meaningful and is currently `Import` alone, which has no asset to save.
	 */
	bool bRegeneratedOnSave = true;

	/**
	 * Which GraphCode product ships this kind's codec. `GC-PRODUCT-PACKAGING-DESIGN.md`.
	 *
	 * DERIVED from `FGraphCodeProducts::OwnerOf`, which is the only ownership table. Core for every
	 * kind that exists today, and that is a measurement rather than a placeholder — the kinds Move
	 * and World will own do not exist yet, and `animation_blueprint` is deliberately Core's with
	 * only its `animgraph` block declared to Move.
	 */
	EGcProduct Product = EGcProduct::Core;

	/** In projection order, which is the order a `.gc` writes them. */
	TArray<FGcBlockCapability> Blocks;

	/** Derived from `FGraphCodeProjector::ExpectedEnvelope`, never restated. */
	TArray<FString> Omits;
	TArray<FString> IrVersions;
};

class GRAPHCODE_API FGraphCodeCapabilities
{
public:
	/**
	 * Every kind, in enum order, built by walking `0 .. Count`.
	 *
	 * A new enumerator with no row here produces a row with an empty block list, and
	 * `GraphCode.Capabilities.EveryKindIsDescribed` fails on it. That is the property
	 * `KindTokenRoundTrip`'s comment claimed and its hand-written list could not deliver.
	 *
	 * **REBUILT WHEN A CODEC REGISTRATION MOVES, so a pointer from `Find`/`FindBlock` is only good
	 * until the next one.** In practice registrations happen at module start, long before anything
	 * queries this; a spec that registers mid-run has to re-`Find` or copy, and copying is what the
	 * product specs do.
	 */
	static const TArray<FGcKindCapability>& All();

	static const FGcKindCapability* Find(EGraphCodeAssetKind Kind);
	static const FGcKindCapability* Find(const FString& KindToken);
	static const FGcBlockCapability* FindBlock(EGraphCodeAssetKind Kind, const FString& BlockName);

	/**
	 * `covered` or `omitted` — `FGraphCodeDescribeResult::Coverage`'s vocabulary.
	 *
	 * A block this kind projects is covered even when it comes back empty: an empty
	 * `components { }` on a data-only Blueprint is a coverage STATEMENT, not a missing facet, and
	 * conflating the two is exactly how the old describe message came to tell 65 of this project's
	 * Blueprints that their Class Defaults were invisible.
	 */
	static FString CoverageToken(EGraphCodeAssetKind Kind, const FString& BlockName);

	/**
	 * `writable`, `projection_only` or `not_projected` — `FGraphCodeDescribeResult::WriteStatus`.
	 *
	 * `Asset` resolves a `WritableIf` condition and may be null; with no asset a conditional block
	 * answers `projection_only`, which is the safe direction — it under-promises a write rather than
	 * inviting one that will refuse.
	 */
	static FString WriteStatusToken(
		EGraphCodeAssetKind Kind, const FString& BlockName, const UObject* Asset);

	/** Whether a `WritableIf` condition holds for this asset. Unknown conditions answer false. */
	static bool ConditionHolds(const FString& ConditionToken, const UObject* Asset);

	/** `not_writable`, `resets_to_default`, `deletes`, `refuses`, `preserves`, `truncates`, `fixed`. */
	static FString OmissionToken(EGcOmissionPolicy Policy);

	/**
	 * WHETHER THIS INSTALL HAS THE CODE, as opposed to whether GraphCode has the capability.
	 * `GC-PRODUCT-PACKAGING-DESIGN.md` §4.
	 *
	 * **A FUNCTION AND NOT A FIELD, because `All()` is built once and this is not static.** Product
	 * OWNERSHIP is a pure switch and lives in the table; INSTALLEDNESS depends on which modules
	 * registered and on the simulation switch, and a bool frozen at the registry's first use would
	 * be a stale answer that every surface then copied.
	 *
	 * **A block can be `Writable` and unavailable at the same time, and both facts have to be
	 * told.** Collapsing them loses whichever one the reader needed: a card that hid an unavailable
	 * block says the facet does not exist, and one that reported it `writable` invites a call that
	 * refuses.
	 */
	static bool IsAvailable(EGraphCodeAssetKind Kind);
	static bool IsAvailable(EGraphCodeAssetKind Kind, const FString& BlockName);

	/**
	 * Empty when available. Otherwise the ONE sentence a tool card, a help entry and a refusal all
	 * print, so a caller reads the same words before the call and after it.
	 *
	 * **USER-DIRECTED 2026-08-07:** *"they should be shown as available in the tool, but the tool
	 * should show unavailable due to move not available, and calling the function with that specific
	 * parameter should also say not available"*. Both halves are needed and neither is redundant:
	 * the card lets a model plan around the absence, and the refusal catches a model working from a
	 * card it read before the install changed.
	 */
	static FString UnavailableReason(EGraphCodeAssetKind Kind);
	static FString UnavailableReason(EGraphCodeAssetKind Kind, const FString& BlockName);

	/**
	 * The `mode:` values this block accepts, for `gc_describe` and the help topics. M2.2.
	 *
	 * `Refuses` is the only policy that returns both, and that is the finding rather than an
	 * implementation detail: everywhere else the block's own identity model already fixes what
	 * omission means, so a mode would be a request the writer cannot honour. §4.3 rule 5 says such
	 * a block must SAY "already snapshot" rather than accept `patch` and quietly do nothing, which
	 * is what this is for — an empty list for `curves` means neither, and `{snapshot}` for the
	 * input lists means the parameter is not a choice there.
	 */
	static TArray<FString> SupportedModes(EGcOmissionPolicy Policy);

	/** The whole registry as JSON — what the bridge, the catalog and the help topics generate from. */
	static FString ToJson();
};
