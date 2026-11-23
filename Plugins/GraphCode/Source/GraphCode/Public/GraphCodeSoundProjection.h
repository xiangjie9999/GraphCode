// `kind:sound_wave` — an imported sound as `import { }`, `pipelines { }` and `settings { }`. C13.
//
// The samples are NOT here and say so on `omits: source_audio`. What the document carries is every
// decision somebody made ABOUT those samples — the compression type and quality, streaming, looping,
// the sound class, attenuation, submix and its sends, concurrency, priority, virtualization — which
// is the whole of what the Sound Wave editor edits and none of what it plays.
//
// **THE BLOCK SET IS `kind:texture_2d`'s EXACTLY, AND THAT IS THE POINT RATHER THAN A SHORTCUT**
// (user-directed 2026-08-19: *"ensure consistency with the grammar with all other gc files"*). An
// imported asset whose binary half stays behind has one shape in this language, and a reader who
// knows a texture's sidecar can read a wave's without being told anything. The three blocks mean
// here exactly what they mean there: where the file came from, how the importer was configured, and
// what the asset became.
//
// **`pipelines { }` IS PRESENT AND WILL USUALLY SAY `# UNSUPPORTED:`, AND THAT IS DELIBERATE.** No
// Interchange translator claims audio in a stock build — every `Interchange.FeatureFlags.Import.
// Audio.*` defaults false — so a wave normally arrives through `USoundFactory` and has no stored
// pipeline stack. The block is still written, for the reason the texture kind gives: an absent block
// and an empty one are different claims, and *"nobody looked"* is not the same as *"there is
// nothing"*. It also costs nothing to be right later — those flags are CVars, and a project that
// turns them on gets a populated block here with no change to this file.
//
// **THE `factory { }` BLOCK IS NOT ON THIS KIND, and the distinction is worth stating because the
// import proposal DOES carry one** (`GC-IMPORT-DESIGN.md` §8). A `UFactory` is transient: its
// settings configure one import and are never serialized onto the asset, so there is no stored
// factory state for a sidecar to project. `bAutoCreateCue` is answerable only by looking for a
// `USoundCue` next door, which is a fact about the package and not about this asset. So `factory { }`
// belongs to the document that RUNS an import and never to the document that describes its result.
//
// EXACT CLASS `USoundWave`, and the trap here is live rather than theoretical — this is
// `kind:texture_2d`'s `UCurveLinearColorAtlas` finding a second time, in a worse form. Measured
// 2026-08-19: `UMetaSoundSource` derives from `USoundWaveProcedural`, which derives from
// `USoundWave` (`MetasoundSource.h:88`). An `IsChildOf` rule would therefore hand every MetaSound in
// the project to this kind and project a graph document as if it were a file's settings. It is worse
// than the atlas case because MetaSound belongs to a DIFFERENT PRODUCT — Core Extension
// (`GC-PRODUCT-SPLIT.md` §2.1) — so the mistake would not merely give one asset two kinds, it would
// let Core silently claim another product's asset. Both classes are refused by name, naming that
// product.
//
// `USoundCue` is refused by name too, and for a reason that is not "unsupported": a cue holds no
// audio at all, only a `UEdGraph` of nodes that reference waves. **It is `kind:sound_cue`, built
// 2026-08-21** (`GC-SOUND-CUE-DESIGN.md`), and `SharedSoundBaseFieldCount` below is what that kind
// inherited — the ~25 `USoundBase` settings, exported by this file's own `SettingsPolicy()`. The
// deferral this comment used to describe is over; the count is now a live dependency rather than a
// note for whoever reopened the row.

#pragma once

#include "AuthoringPropertyCodec.h"
#include "CoreMinimal.h"

#include "GraphCodeSidecarWriter.h"

// **PUBLIC SINCE 2026-09-10, AND FOR EXACTLY ONE CALLER OUTSIDE CORE.** `kind:metasound_source` is
// Core Extension's, and a `UMetaSoundSource` IS a `USoundWave` — so its `settings { }` is these same
// ~25 `USoundBase` fields plus the wave's own, decided by the same `SettingsPolicy()` below. Every
// other product plugin declares a policy of its own, which is right when the fields are its own;
// here the fields are Core's, and a second policy over them is the two-answers defect this project
// keeps paying for. `SharedSoundBaseFieldCount`'s comment already promised this inheritance to
// `kind:sound_cue`; MetaSound is the second kind to take it and the first to take it from outside.
class GRAPHCODE_API FGraphCodeSoundProjection
{
public:
	/** True for a USoundWave and nothing else — see the header on why this is not `IsChildOf`. */
	static bool IsProjectableSoundClass(const UClass* Class);
	static bool IsProjectableSound(const UObject* Object);

	/**
	 * Why another sound class is refused, in a sentence naming its successor. Empty when `Class` is
	 * not a sound at all, so a caller can tell "not this kind" from "this kind, deliberately not
	 * yours".
	 *
	 * Every branch names a PRODUCT or a QUEUE ROW rather than saying unsupported, because each of
	 * these classes is a real asset somebody owns and the reason differs per class.
	 */
	static FString RefusalForSoundClass(const UClass* Class);

	/**
	 * ONE TOKEN, `source_audio`, plus the shared `reimport_history`.
	 *
	 * A wave holds two binary things: the editor-only source in `FEditorAudioBulkData RawData`, and
	 * the cooked/compressed platform chunks. Only the first is authored content the document cannot
	 * carry — it is `UTexture::Source`'s exact analogue. The compressed chunks are DERIVED from it,
	 * and every setting in `settings { }` is an input to the encode that produces them, so declaring
	 * them lost would state a loss that does not happen. That is the same measurement that kept
	 * `platform_data` off `kind:texture_2d` and took `blueprint_extensions` off
	 * `kind:animation_blueprint`; it is applied here rather than re-derived.
	 *
	 * `reimport_history` is `data_table`'s and the texture's, unchanged: `import { }` carries the
	 * first source file and its hash, and a re-pointed source leaves later entries behind.
	 */
	static TArray<FString> Omissions();

	/**
	 * The wave's own settings, against the class CDO.
	 *
	 * `CPF_Edit` and then three exclusions, of which the deprecation one is the only novel gate in
	 * this kind — see `SettingsPolicy`'s body for what each measured.
	 */
	static AuthoringCore::FPropertyExportPolicy SettingsPolicy();

	/**
	 * How many of the exported fields come from `USoundBase` rather than from `USoundWave`.
	 *
	 * Not decoration and not a statistic: it is the deferred `kind:sound_cue`'s inheritance, made
	 * measurable now so that nobody re-derives it later. A `USoundCue` is a `USoundBase` too, so
	 * every one of these fields means the same thing on a cue and is written by the same policy and
	 * the same codec. Whoever opens that row gets the settings half finished and owes only the graph.
	 *
	 * A spec asserts this is non-zero, which is what stops the claim rotting silently if the engine
	 * moves a field between the two classes.
	 */
	static int32 SharedSoundBaseFieldCount();

	/** `import { }`, `pipelines { }` and `settings { }`, in that order. */
	static bool BuildBlocks(UObject* Asset, TArray<FString>& OutBlocks, FString& OutError);
};
