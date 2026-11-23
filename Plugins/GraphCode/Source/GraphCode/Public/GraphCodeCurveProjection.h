// `kind:curve_float` / `curve_vector` / `curve_linear_color` — a standalone curve asset as
// `import { }`, `settings { }` and `curves { }`. GRAPHCODE-EXTENDED-ASSET-SYNTAX-PROPOSAL.md §8.3,
// corrected where the timeline implementation had already settled the question.
//
// THE KEY SPELLING IS NOT NEW, AND THAT IS THE WHOLE POINT. §8.3 proposed a nested block —
// `keys { 0.000000 = 0.000000 [interp=Linear] }` — and it was written before Timelines shipped. A
// Timeline track already renders its curve as `keys=((Time=…,Value=…),…)`, through
// `FRichCurveKey`'s own reflection, and a track can point at a curve ASSET
// (`curve=/Game/Curves/C_Tint.C_Tint`). So the same UObject's keys are reachable through both
// surfaces, and two spellings would mean one curve reading two ways depending on which file you
// opened. `BlueprintIR::ExportCurveKeys` is now the single implementation both call.
//
// THE CHANNEL NAMES ARE THE ENGINE'S OWN. `FCurveOwnerInterface::GetCurves()` returns each
// `FRichCurve*` paired with the `CurveName` the curve editor labels it with — `X`/`Y`/`Z` for a
// vector, `R`/`G`/`B`/`A` for a colour. A hand-written table would be a second answer to a question
// the engine already answers, and it would not extend to a `UCurveBase` subclass nobody has
// written yet. The one gap is a single-channel curve, which the engine leaves unnamed; that is
// `Value`, and it is the only name invented here.
//
// A NAMED CHANNEL ALSO GIVES THE C ARRAY ITS DOMAIN SPELLING. `UCurveVector::FloatCurves` is
// `FRichCurve FloatCurves[3]`. The generic property codec can now address fixed-array slots by
// index, but this projection deliberately uses the engine's semantic channel names instead. It is
// not `fixed_size_arrays` in `omits:` because no element is dropped.

#pragma once

#include "AuthoringPropertyCodec.h"
#include "CoreMinimal.h"
#include "GraphCodeSidecarWriter.h"

class UCurveBase;
struct FRichCurve;
struct FSimpleCurve;

class GRAPHCODE_API FGraphCodeCurveProjection
{
public:
	/**
	 * The type token every channel statement carries, on both sides of the round trip.
	 *
	 * Here rather than once in the projection and again in the writer: the writer's whole check is
	 * "is this the token the projection wrote", and two spellings of that would be a payload the
	 * writer refuses for a reason the file cannot show. Two private copies existed until a unity
	 * repartition put both files in one blob and made them a redefinition error.
	 */
	static constexpr const TCHAR* ChannelTypeToken = TEXT("RichCurve");

	/**
	 * The sibling token, for an `FSimpleCurve`. No curve ASSET holds one — this exists for
	 * `kind:curve_table`, whose rows are one or the other, and it lives here so both spellings sit
	 * in the file that already owns the first.
	 *
	 * A row's token is also how a curve table's `CurveTableMode` reaches the document without a
	 * field of its own: the two cannot coexist in one table, so naming the shape names the mode.
	 */
	static constexpr const TCHAR* SimpleChannelTypeToken = TEXT("SimpleCurve");

	/**
	 * Which curve kind a class projects as, EXACT CLASS ONLY.
	 *
	 * `UCurveLinearColorAtlas` is a `UCurveLinearColor` with a baked texture and its own gradient
	 * bookkeeping, so an `IsA` test would project it as an ordinary colour curve and quietly claim
	 * coverage of state this does not read. Same rule as `UCompositeDataTable` and `kind:level`,
	 * and it keeps the answer decidable from the Asset Registry without loading.
	 */
	static bool TryGetCurveKind(const UClass* Class, EGraphCodeAssetKind& OutKind);
	static bool IsProjectableCurve(const UObject* Object);

	/** What a curve projection does not carry. Static, because `omits:` is about the KIND. */
	static TArray<FString> Omissions();

	/**
	 * The asset's own settings, against the class CDO.
	 *
	 * NO VISIBILITY RULE, unlike `class_defaults` or a Data Table's settings. The reflected surface
	 * of a curve asset is four things — the curve data, the import record, a deprecated path and a
	 * handful of authored scalars — so there is no engine bookkeeping for a visibility rule to
	 * separate out, and applying one would drop `UCurveFloat::bIsEventCurve`, which is a mode
	 * somebody set and not a detail the panel happens to hide.
	 *
	 * What IS excluded is the two facets rendered as blocks of their own: any `FRichCurve` (the
	 * `curves { }` block) and the instanced `UAssetImportData` (the `import { }` block). Excluded
	 * structurally rather than by property name, so a future subclass needs no line here.
	 */
	static AuthoringCore::FPropertyExportPolicy SettingsPolicy();

	/**
	 * One channel's own fields — `DefaultValue`, `PreInfinityExtrap`, `PostInfinityExtrap`.
	 *
	 * `Keys` is excluded because it is rendered as `keys=`, through the shared curve-key spelling.
	 * Letting the codec export it too would put the same array in the line twice, in two different
	 * spellings — and the codec's would lose a key at time 0 with value 0, which is the exact
	 * failure the sentinel in `ExportCurveKeys` exists to prevent.
	 */
	static AuthoringCore::FPropertyExportPolicy ChannelPolicy();

	/** Each channel's authored name paired with its curve, in the engine's own order. */
	static void Channels(UCurveBase* Curve, TArray<TTuple<FString, FRichCurve*>>& Out);

	/** `import { }`, `settings { }`, then `curves { }`. */
	static bool BuildBlocks(UObject* Asset, TArray<FString>& OutBlocks, FString& OutError);

	/** One channel line, shared with the writer so a change report cannot disagree with a read. */
	static FString RenderChannel(const FString& Name, const FRichCurve& Curve,
		TArray<FAuthoringDiagnostic>& OutDiagnostics);

	/**
	 * The same line for a simple curve, sharing `ChannelPolicy` and differing only in which struct
	 * it deltas and which key spelling it calls.
	 *
	 * `InterpMode` appears only when it deviates: `FSimpleCurve`'s constructor defaults it to
	 * `RCIM_Linear` (`SimpleCurve.h:74-77`), so the common row is `SimpleCurve(keys=…)` and the
	 * token carries the rest.
	 */
	static FString RenderSimpleChannel(const FString& Name, const FSimpleCurve& Curve,
		TArray<FAuthoringDiagnostic>& OutDiagnostics);
};
