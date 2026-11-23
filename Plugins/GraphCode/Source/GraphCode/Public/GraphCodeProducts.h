#pragma once

#include "CoreMinimal.h"

#include "GraphCodeSidecarWriter.h"

/**
 * WHICH GRAPHCODE PRODUCT CARRIES A KIND OR A FACET, AND WHETHER THIS INSTALL HAS IT.
 * `GC-PRODUCT-PACKAGING-DESIGN.md`; `GC-PRODUCT-SPLIT.md` rules where the boundaries fall.
 *
 * **THE VOCABULARY IS CORE'S AND THE CODECS ARE THE PRODUCTS'** — the one ruling everything else
	 * here follows from. Core knows the NAME of every kind in all five products even when it holds none
 * of their code, because the alternative is measured and bad: `TryParseKind` failing on
 * `animation_blueprint` answers *"unsupported asset kind"*, and the sentence a person can act on is
 * *"that kind is projected by GraphCode Move, which is not installed"*. Only a build that knows the
 * word can say the second one.
 *
 * SO THIS FILE IS THE ONLY OWNERSHIP TABLE. Every surface — projection, apply, describe, the
 * capability JSON, the help tree, the panel — asks it rather than restating it, for the reason
 * `GraphCodeCapabilities.h` sets out at length: nobody keeps four hand-written surfaces in step by
 * reading them.
 */
enum class EGcProduct : uint8
{
	/** The container and the language. Always installed — it is the code asking. */
	Core,

	/** Animation, state machines, and the AI graphs that decide motion. */
	Move,

	/** Runtime capture, restore and scenario play. Owns `.gcp` and the Blueprint-facing nodes. */
	Play,

	/** Environment systems and the level facets that only matter at streaming scale. */
	World,

	/** Framework-backed authoring: MVVM, MetaSound, CommonUI and Gameplay Ability System. */
	CoreExtension,

	/**
	 * Shapes made from nothing — `.gcshape`, a declarative program that builds a mesh asset.
	 *
	 * The sixth, and the first product that owns a WHOLE DOCUMENT rather than a kind or a block:
	 * `GraphCodeShapeProgram.h` is Core's entire surface for it. `GC-PRODUCT-SPLIT.md` §5.2 keeps
	 * the boundary — the `.gcshape` decides shape and slots, and `kind:static_mesh`'s `.gc` decides
	 * what the asset wears and how it builds.
	 */
	Shape,

	/** Blender shader translation and selective baking. Existing material assets remain Core's. */
	Integrate,

	/**
	 * A mesh made animatable — `.gcrig` declares where it bends, and the skeleton, skin weights,
	 * physics bodies and IK rig all come from that one declaration.
	 *
	 * The eighth, and the second product that owns a WHOLE DOCUMENT rather than a kind or a block.
	 * **It takes a mesh IN and builds none** (`GC-RIG-LEDGER.md` §0 ruling 8), which is what keeps
	 * it off Shape: the two compose by handing an asset between them, never by linking.
	 */
	Rig,

	/**
	 * Clothing made on a body — `.gcgarment` declares rings on a mesh and the garments between them,
	 * and builds a mesh from that (`GC-GARMENT-GRAMMAR.md`). The ninth, and like Rig it takes a mesh
	 * IN — the body — and composes with Shape and Rig only by handing assets between them.
	 */
	Garment,

	/**
	 * Hair authored from a `.gcgroom` recipe and published as native Groom and Groom Binding assets.
	 * A separate product because it ships independently; its only GraphCode dependency is Core.
	 */
	Groom,

	Count,
};

/**
 * How far a missing product reaches into one document, which is NOT one answer.
 *
 * **USER-DIRECTED 2026-08-07:** *"like having omit and non blocking errors, just to let them know
 * the specific graphs like animfunction cant be compiled, need move"*. That is the middle row, and
 * it is the common one: an Animation Blueprint on a Core-only install still has variables, an event
 * graph, ordinary functions, components and class defaults, and every one of them still diffs,
 * blames and applies. Treating the absence as fatal would take the whole asset out of version
 * control over one block.
 */
enum class EGcProductScope : uint8
{
	/** Nothing is absent. The document projects in full. */
	Available,

	/**
	 * One block or sub-facet is carried by an absent product; the rest of the document is real.
	 *
	 * `omits:` gets `<facet>@<product>` and `errors { }` gets an ADVISORY `RequiresProduct` entry.
	 * The file applies normally.
	 */
	Facet,

	/**
	 * The whole asset belongs to an absent product and Core cannot usefully read any of it.
	 *
	 * The file is a STUB — header, `bin:`, `omits: all@<product>` — with a BLOCKING `MissingModule`
	 * entry. Rarer than it looks, and still worth writing: the alternative is an asset silently
	 * invisible to version control, which is the state this whole design was commissioned to end.
	 */
	Kind,
};

/**
 * One block a non-Core product owns, as a ROW rather than as a branch of a switch.
 *
 * **THIS TABLE IS THE ONE CORE EDIT A NEW PRODUCT FACET COSTS** (`GC-PRODUCT-PACKAGING-DESIGN.md`
 * §5b). Everything else — the reader, the writer, the block's full declaration — is registered by
 * the product at module start and is absent when the product is.
 *
 * It has to be ENUMERABLE and not merely askable, which is the shape change 11d made. `omits:
 * <facet>@<product>` and the `RequiresProduct` advisory have to be written for a product that is
 * NOT HERE, so they cannot be derived from anything the product registers; the only thing that
 * survives its absence is this table, so this table has to be able to list what is missing.
 */
struct GRAPHCODE_API FGcOwnedFacet
{
	EGraphCodeAssetKind Kind = EGraphCodeAssetKind::Blueprint;

	FString BlockName;

	EGcProduct Product = EGcProduct::Core;

	/**
	 * What Core can say about the block with the product's code absent.
	 *
	 * Short on purpose: the product's own registration carries the full declaration, and this is
	 * only what is left when there is no registration to read. It is still better than a bare name —
	 * a caller reading `animgraph { }` in an `omits:` line needs to know what it would have held.
	 */
	FString Note;
};

class GRAPHCODE_API FGraphCodeProducts
{
public:
	/** `core`, `move`, `play`, `world`, `extension`, `shape`, `groom`, etc. */
	static const TCHAR* Token(EGcProduct Product);

	/** `GraphCode Move`. The name a person reads and buys. */
	static FString DisplayName(EGcProduct Product);

	/** False for a token this build does not know, which cannot happen while the enum is closed. */
	static bool TryParseToken(const FString& Token, EGcProduct& OutProduct);

	/**
	 * A product is installed when its MODULE HAS REGISTERED, not when its folder is present.
	 *
	 * A plugin whose folder is on disk but whose module failed to load is not installed, and that is
	 * the answer a caller needs: the facet is unavailable either way, and a predicate that said
	 * otherwise would send somebody to check an installation that is already there.
	 *
	 * The same call is the extension point the codecs will register through, so a product's presence
	 * and its implementations cannot come apart.
	 */
	static void Register(EGcProduct Product);
	static void Unregister(EGcProduct Product);
	static bool IsInstalled(EGcProduct Product);

	/**
	 * Force a registered product to answer NOT INSTALLED. `-GcWithoutProduct=move,world`, or
	 * `UGraphCodeSettings::SimulateMissingProducts`.
	 *
	 * **EVERY PRODUCT IS INSTALLED IN THIS CHECKOUT**, because all five are developed here, so
	 * without this the gate would ship untested. It warns on every query while active: a simulated
	 * absence that is silent is a debugging session lost to a switch nobody set.
	 */
	static bool IsSimulatedMissing(EGcProduct Product);

	/** Which product ships the codec for a kind. `Core` for everything it does not name. */
	static EGcProduct OwnerOf(EGraphCodeAssetKind Kind);

	/**
	 * Which product ships one BLOCK of a kind. `Core` unless the block is named.
	 *
	 * Distinct from `OwnerOf` and not derivable from it: `animgraph { }` belongs to Move on a kind
	 * that may itself belong to Move, and `sublevels` will belong to World on `level`, which is
	 * Core's by an explicit ruling (`GC-PRODUCT-SPLIT.md` §2).
	 */
	static EGcProduct OwnerOfBlock(EGraphCodeAssetKind Kind, const FString& BlockName);

	/** Every non-Core facet this build knows the NAME of, installed or not. The whole table. */
	static const TArray<FGcOwnedFacet>& OwnedFacets();

	/** The rows for one kind, in table order. Empty for a kind with no product facets. */
	static TArray<FGcOwnedFacet> OwnedFacetsFor(EGraphCodeAssetKind Kind);

	/** The one row for a block, or null when Core owns it. */
	static const FGcOwnedFacet* FindOwnedFacet(EGraphCodeAssetKind Kind, const FString& BlockName);

	/** How far an absent product reaches into a document of this kind, right now. */
	static EGcProductScope ScopeFor(EGraphCodeAssetKind Kind);

	/** The absent product behind a `Kind`-scope answer. Only meaningful when `ScopeFor` says `Kind`. */
	static EGcProduct BlockingProductFor(EGraphCodeAssetKind Kind);

	/** Every block of this kind whose product is absent, in projection order. Usually empty. */
	static TArray<FString> UnavailableBlocks(EGraphCodeAssetKind Kind);

	/**
	 * `animgraph@move`. The `omits:` spelling that says a PRODUCT carries this, not that nobody does.
	 *
	 * `GC-PRODUCT-SPLIT.md` §6 requires the distinction and states the cost of missing it: a Core
	 * user who cannot tell a missing product from a missing feature has an `omits:` that is no longer
	 * a contract. One character does it, `IsToken` already accepts `@`, and a reader that has never
	 * heard of the qualifier reads the whole thing as one opaque facet name and concludes "omitted",
	 * which is true.
	 */
	static FString QualifyOmission(const FString& Facet, EGcProduct Product);

	/** Split `animgraph@move` back apart. False, and `OutFacet = Token`, when there is no qualifier. */
	static bool TryParseOmission(const FString& Token, FString& OutFacet, EGcProduct& OutProduct);

	/**
	 * The sentence every refusal, tool card and help entry uses, so there is one of it.
	 *
	 * **USER-DIRECTED 2026-08-07:** *"basically let ai tell the user, i cant do move yet, you need to
	 * buy move"*. It names the product, what is unavailable, and that installing is the fix — a
	 * refusal a model can relay without inventing the remedy.
	 */
	static FString UnavailableReason(EGcProduct Product, const FString& WhatIsUnavailable);
};

// The codec seam itself — what a product REGISTERS, as opposed to what it owns — is
// `FGraphCodeCodecs` in `GraphCodeCodecs.h`. It sits below this header because a registration
// carries a block's full capability declaration and a kind's projector, and both of those are
// types this file is included by.
