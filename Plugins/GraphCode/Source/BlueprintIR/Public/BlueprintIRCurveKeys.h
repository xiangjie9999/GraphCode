// The ONE spelling of a rich curve's keys, in both directions.
//
// These two functions were an anonymous-namespace pair each — the export in BlueprintIRReader.cpp
// and the import in BlueprintIRTimelineApply.cpp — for as long as a Timeline was the only thing in
// the project that had curve keys. `kind:curve_float` / `curve_vector` / `curve_linear_color` made
// that false, and the two are now the same bytes rather than two implementations that agree today:
//
//   timelines { DoorSwing = Timeline(…) { Alpha = FloatTrack(property=Alpha, keys=(…)) } }
//   curves    { Value = RichCurve(keys=(…)) }
//
// which matters more here than it usually does, because a Timeline track can point at a curve
// ASSET (`curve=/Game/Curves/C_Tint.C_Tint`). The same UObject's keys are therefore reachable
// through both surfaces, and two spellings would mean one curve reading two ways depending on
// which file you opened. `GC-GRAMMAR-AUDIT.md` §3 is a list of findings of exactly that shape.
//
// The spelling is `FRichCurveKey`'s OWN reflection, not a hand-written field list. A key carries an
// interp mode, a tangent mode, a weight mode, two tangents and two tangent weights, and a
// hand-written list is one that stops matching the engine the first time a field is added —
// silently, because the missing field reads back as its default.

#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "Curves/SimpleCurve.h"

namespace BlueprintIR
{
	/**
	 * One curve's keys as `((Time=…,Value=…),(…))`.
	 *
	 * TIME AND VALUE ARE ALWAYS WRITTEN. `ExportText` is a DELTA against the defaults it is given,
	 * and a key at time 0 with value 0 is a default-constructed `FRichCurveKey` — so passing the
	 * honest defaults rendered the first key of two curves in this project as `()`, and a reader
	 * could not see that a key was there at all. The fix is a defaults instance whose Time and
	 * Value no real key can match, which makes exactly those two always differ and leaves every
	 * other field deltaing normally. A plain key stays short — `(Time=1.000000,Value=1.000000)` —
	 * while the two fields that DEFINE a key are never elided.
	 */
	BLUEPRINTIR_API FString ExportCurveKeys(const FRichCurve& Curve);

	/**
	 * The inverse. `()` is a curve with no keys, which is a real state and not the same thing as a
	 * track with no curve at all.
	 *
	 * The export's sentinel needs no mirror here: importing a delta into a default-constructed key
	 * reproduces the original either way, which is the whole reason the sentinel was safe to
	 * introduce on the write-out side.
	 */
	BLUEPRINTIR_API bool ImportCurveKeys(const FString& Literal, TArray<FRichCurveKey>& OutKeys,
		FString& OutError);

	/**
	 * The same pair for an `FSimpleCurve`, added 2026-08-21 for `kind:curve_table`.
	 *
	 * A simple curve is a rich curve with the per-key interpolation lifted out: its keys carry Time
	 * and Value ONLY, and the one `InterpMode` belongs to the curve rather than to any key. So the
	 * key list is the same statement — `((Time=…,Value=…),…)` — and cannot be produced by the pair
	 * above, because `FRichCurveKey` and `FSimpleCurveKey` are different structs with different
	 * reflection.
	 *
	 * HERE RATHER THAN IN THE CURVE TABLE, which is the whole point of this file: a curve table in
	 * SimpleCurves mode holds exactly these, and the day anything else does — a Timeline track, a
	 * new curve asset class — it must spell them the same way rather than discovering that the
	 * shared file only covered half the engine's curves.
	 */
	BLUEPRINTIR_API FString ExportSimpleCurveKeys(const FSimpleCurve& Curve);
	BLUEPRINTIR_API bool ImportSimpleCurveKeys(const FString& Literal,
		TArray<FSimpleCurveKey>& OutKeys, FString& OutError);

	/**
	 * Split on Separator at paren depth zero. A key literal holds no quotes, only parens.
	 *
	 * Exported alongside the pair because a caller that has a `keys=` list of its own to take
	 * apart — a vector curve's three, a colour curve's four — must split it the same way.
	 */
	BLUEPRINTIR_API TArray<FString> SplitCurveElements(const FString& Text, TCHAR Separator);
}
