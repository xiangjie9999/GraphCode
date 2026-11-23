// Copyright OraStudio. All Rights Reserved.

// A curtain's words (CLOTH-CURTAIN1 slice A1, Docs/GC-CLOTH-CURTAIN-DESIGN.md sections 5 and 9; the code plan
// Saved/Research/CLOTH-CURTAIN1/SLICE-A-CODE-PLAN.md section 1 with the lead's decisions of its section 8):
// `Pole = track(...)`, `Left = panel(width=, drop=, fabric=) { heading(...) rim(...) line(...) press(...) settle(...) }`.
//
// Why an include guard and no `#pragma once`: FGcGarmentDocument (GraphCodeGarmentDocument.h) holds the tracks and panels declared
// here, and a panel's body reuses that header's FGcGarmentStep, so these types must be defined INSIDE that header, after
// FGcGarmentStep and before FGcGarmentDocument. GraphCodeGarmentDocument.h defines GRAPHCODEGARMENT_DOCUMENT_CURTAIN_POINT at that
// spot and includes this file there. Included first on its own, this file includes GraphCodeGarmentDocument.h, which comes back here
// at that spot; so the two headers may be included in either order.

#ifndef GRAPHCODEGARMENT_CURTAIN_H
#ifndef GRAPHCODEGARMENT_DOCUMENT_CURTAIN_POINT

#include "GraphCodeGarmentDocument.h"

#else
#define GRAPHCODEGARMENT_CURTAIN_H

#include "CoreMinimal.h"

struct FGcGarmentDocument;

namespace GcGarmentCurtain
{
	/**
	 * The build's mesh step along the cloth, centimetres: C0/C1's reading; printed, never written. The limits that are "one mesh
	 * step" or "two" (R8, R10, R18) are measured in it.
	 */
	inline constexpr double SpacingCm = 2.5;

	/** Plan section 8, Q2: a hang's time when none is written - seconds each run may take to come to rest. */
	inline constexpr double DefaultHangSeconds = 6.0;

	/** Plan section 8, Q2: the hooks' drawing speed along the pole when no `slide=` is written, from the research bracket. */
	inline constexpr double DefaultSlideCmPerSecond = 15.0;

	/** R15: the longest `time=` a run may take. */
	inline constexpr double MostHangSeconds = 10.0;

	/** R19: the least crease= any measured fabric holds - GraphCodeGarmentCloth.cpp's `LeastMeasuredCreaseDeg`, kept equal by hand. */
	inline constexpr double LeastCreaseDeg = 35.0;
}

/**
 * `Pole = track((-62.5, 16, 263.4), (62.5, 16, 263.4))` (design 9.3): the pole or track a curtain hangs from, through points typed
 * on the set, centimetres in the set mesh's own space. Not `line()`, which already names a path through places on rings. Slice A
 * hangs from a straight pole: two points (a bay through more is slice C).
 */
struct GRAPHCODEGARMENT_API FGcGarmentTrack
{
	FString Name;
	TArray<FVector> Points;
	int32 Line = 0;

	/** Along its points, end to end, in centimetres: the stretch a panel covers (design 9.1, "S"). */
	double Length() const;
};

/** What holds a panel's top (design 9.3). Slice A: hooks with free cloth between, and pinch pleats; wave, eyelets, pencil and rod pocket come later. */
enum class EGcCurtainHeading : uint8
{
	/** `heading(hooks, on=$Pole, every=12.5cm)`: one hook a pole pitch, the cloth between them free. */
	Hooks,

	/** `heading(pinch(3), on=$Pole, every=10.2cm)`: two or three folds sewn down a band at each hook (by default), flat cloth between. */
	Pinch,
};

/**
 * `heading(pinch(3), on=$Pole, every=10.2cm, depth=10cm, fabric=$Buckram)` (design 9.1 / 9.2). `every=` is always the pitch ON THE
 * POLE, what the rings set; `hooks=13` may be written instead (one of the two, never both: plan section 8, Q1b). Neither is a
 * cloth measurement: the cloth follows (`FGcCurtainRule`).
 */
struct GRAPHCODEGARMENT_API FGcCurtainHeading
{
	EGcCurtainHeading Style = EGcCurtainHeading::Hooks;

	/** `pinch(n)`: folds a pleat, 2 or 3. 0 for hooks. */
	int32 Folds = 0;

	/**
	 * `pinch(3, ends=flat)` (plan section 8, Q1): the two end hooks hold flat cloth and only the hooks between carry a pleat, so
	 * pleats = hooks - 2. False, the default (`ends=pleat` says it aloud): a pleat at EVERY hook, flats between - the one workroom
	 * read, 12 pleats and 11 flats.
	 */
	bool bEndsFlat = false;

	/** `on=$Pole`: the track, without its `$`. */
	FString On;

	/** `every=12.5cm` as written: the pole pitch asked for. 0 when `hooks=` is written instead. */
	double EveryCm = 0.0;

	/** `hooks=13`: the hook count asked for, in place of `every=`. */
	TOptional<int32> HooksCount;

	/** `depth=10cm`: how far down the heading's band reaches from the top. Unset: no band. */
	TOptional<double> DepthCm;

	/** `fabric=$Buckram`: the band's stiffening, without its `$` (written only with `depth=`). Empty: the band is the panel's own cloth. */
	FString Fabric;

	/** `pleat=15cm` on a pinch heading: the cloth per pleat the author expects; it must agree with the cut within one mesh step (R10). */
	TOptional<double> PleatCm;

	int32 Line = 0;
};

/**
 * How a panel is hung (plan section 8, Q2, the owner's "make it flexible"): with no `settle()` it hangs fresh with defaults - flat on
 * its hooks, the hooks slid along the pole, settled - each run up to `DefaultHangSeconds`, the hooks sliding at
 * `DefaultSlideCmPerSecond`, both printed as defaults; `settle(time=4, slide=3)` overrides either; `settle(off)` lays it flat and runs
 * no Chaos. `slide=` is seconds the hooks take along the pole (`slide=3` or `slide=3s`, design 9.2's spelling) or a drawing speed
 * (`slide=15cm/s`): a speed is what the research bracket gives, and seconds are what an author times.
 */
struct GRAPHCODEGARMENT_API FGcCurtainHang
{
	/** `settle(off)`: laid flat, no hang. */
	bool bOff = false;

	/** `time=`: seconds each run may take to come to rest. */
	double TimeSeconds = GcGarmentCurtain::DefaultHangSeconds;
	bool bTimeDefaulted = true;

	/** `slide=3`: seconds the hooks take along the pole, the same for every hook. 0: by `SlideCmPerSecond` instead. */
	double SlideSeconds = 0.0;

	/** `slide=15cm/s`, or the default: the hooks move at this speed, each over its own distance. */
	double SlideCmPerSecond = GcGarmentCurtain::DefaultSlideCmPerSecond;
	bool bSlideDefaulted = true;

	/** The `settle()` line; 0 when none is written (every value then a default). */
	int32 Line = 0;

	/** Seconds a hook takes over `DistanceCm` of pole: the written seconds, else the distance at the speed. */
	double SlideSecondsOver(const double DistanceCm) const
	{
		return SlideSeconds > 0.0 ? SlideSeconds : (SlideCmPerSecond > 0.0 ? DistanceCm / SlideCmPerSecond : 0.0);
	}
};

/**
 * `Left = panel(width=307, drop=230, fabric=$Cloth) { ... }` (design 9.2): a flat cut of a stated size in a declared fabric, NOT a
 * garment (FGcGarmentPanel is the surface's panel, plan F3, hence FGcCurtain*). Its edges are `$Left.Top`, `.Hem`, `.Left` and `.Right`
 * (Left as seen from the room); Top and Hem count from Left, Left and Right from Top.
 */
struct GRAPHCODEGARMENT_API FGcCurtainPanel
{
	FString Name;
	double WidthCm = 0.0;
	double DropCm = 0.0;

	/** `fabric=$Cloth`, without its `$`. */
	FString Fabric;

	TOptional<FGcCurtainHeading> Heading;

	/** Always present: defaults when no `settle()` is written (`Hang.Line` 0). */
	FGcCurtainHang Hang;

	/**
	 * `rim($Left.Hem, ...)`, `Pack1 = line($Left.Left@76cm, $Left.Right@76cm)`, `press($Pack1)`, read by the garment body's own reader
	 * (AddGarmentStep, ReadRimFields), in order. A rim's `Target` and a line's position targets are dotted: "Left.Hem".
	 */
	TArray<FGcGarmentStep> Steps;

	int32 Line = 0;
};

/**
 * Design 9.1's cloth-to-pole rule (G1), worked once for one panel on its track. Every number is printed, never written: the author
 * writes the cut's `width=`, the stretch `on=` and the pole pitch `every=` (or `hooks=`).
 */
struct GRAPHCODEGARMENT_API FGcCurtainRule
{
	/** S: the track's length. */
	double StretchCm = 0.0;

	/** W / S. */
	double Fullness = 0.0;

	/** What the author wrote: `every=` (0 when `hooks=` was written) and `hooks=` (unset when `every=` was). */
	double EveryWrittenCm = 0.0;
	TOptional<int32> HooksWritten;

	/** U = round(S / every), or hooks= - 1: whole pitches, since a hook sits at each end of each pitch. */
	int32 Pitches = 0;

	/** S / U: the pitch built. Differs from `EveryWrittenCm` when every= does not divide the stretch (`bPitchChanged`). */
	double PolePitchCm = 0.0;

	/** U + 1. */
	int32 Hooks = 0;

	/** W / U: the cloth between two hooks, along the top. */
	double ClothPitchCm = 0.0;

	/** Pinch: hooks (a pleat at every hook), or hooks - 2 with `ends=flat`. 0 for hooks. */
	int32 Pleats = 0;

	/** Pinch: (W - S) / pleats, the cloth taken into each pleat. 0 for hooks. */
	double ClothPerPleatCm = 0.0;

	/** Plan section 8, Q1b: `every=` did not divide the stretch, so the nearest whole number of pitches was built; the note says so. */
	bool bPitchChanged = false;

	/** Copied from the heading so the note reads alone. */
	EGcCurtainHeading Style = EGcCurtainHeading::Hooks;
	FString Track;
};

namespace GcGarmentCurtain
{
	/**
	 * `Left = panel(width=250, drop=230, fabric=$Cloth)`'s brackets into `Out` (name, line, cut, fabric). Refused with `line N: ...`:
	 * an argument that is not one (R1), a missing width=, drop= or fabric= (R2), a size of zero or less.
	 */
	GRAPHCODEGARMENT_API bool ReadPanel(const FString& Name, const TArray<FString>& Arguments, int32 Line, FGcCurtainPanel& Out, FString& OutRefusal);

	/**
	 * One instruction of a panel's body that is the curtain's own: `heading(...)` (R4, R5, R6, and the settings' own refusals) or
	 * `settle(...)` (R15). Any other word is refused (R3); `rim`, `line` and `press` are the garment body's reader's, called by the
	 * parser's bridge (GraphCodeGarmentDocument.cpp, AddCurtainPanel).
	 */
	GRAPHCODEGARMENT_API bool ReadPanelInstruction(const FString& Word, const TArray<FString>& Arguments, int32 Line, FGcCurtainPanel& Panel,
		FString& OutRefusal);

	/** R3, the one refusal for a word a panel's body does not take. */
	GRAPHCODEGARMENT_API FString NotAPanelInstruction(const FString& Word, int32 Line);

	/**
	 * `Pole = track((x, y, z), (x, y, z))`'s points (read by the parser's own point reader) into `Out`. Refused (R12): other than two
	 * points, or two that coincide.
	 */
	GRAPHCODEGARMENT_API bool ReadTrack(const FString& Name, const TArray<FVector>& Points, int32 Line, FGcGarmentTrack& Out, FString& OutRefusal);

	/** True for "Panel.Edge" with Edge one of Top, Hem, Left, Right: the dotted target the parser keeps for `$Left.Left@76cm`. */
	GRAPHCODEGARMENT_API bool IsEdgeTarget(const FString& Target);

	/**
	 * `$Left.Hem` (with or without its `$`, and no `@`) into the panel and the edge. Refused (R16) with `line N: ...` when it is not
	 * Panel.Edge, when Edge is not one of the four, or when `Panel` is not empty and the text names another panel's edge.
	 */
	GRAPHCODEGARMENT_API bool SplitEdge(const FString& Text, int32 Line, const FString& Panel, FString& OutPanel, FString& OutEdge,
		FString& OutRefusal);

	/**
	 * A `line()`'s arguments in panel `Panel`, before the garment reader reads them: each `$X.Y@...` names this panel's edge (R16), so
	 * a wrong edge word is refused by the curtain's sentence rather than the reader's "not a position".
	 */
	GRAPHCODEGARMENT_API bool CheckEdgePlaces(const TArray<FString>& Arguments, const FString& Panel, int32 Line, FString& OutRefusal);

	/**
	 * Where a place on a panel's edge lies, in centimetres along that edge from its start (Top and Hem from Left, Left and Right from
	 * Top): `@76cm` is 76, `@0.25` a quarter of the edge's length (the width for Top and Hem, the drop for Left and Right), either plus
	 * its `+3cm` / `-3cm`. False when the position's target is not one of this panel's edges.
	 */
	GRAPHCODEGARMENT_API bool EdgePlaceCm(const FGcCurtainPanel& Panel, const FGcGarmentPosition& Position, FString& OutEdge, double& OutCm);

	/**
	 * Design 9.1 worked for `Panel` on `Track` at mesh step `SpacingCm`. Refused with `line N: ...` in `OutError`: no heading; fullness
	 * under 1 (R7); a pole pitch under two mesh steps (R8); a pinch with no cloth to pleat or no hook to pleat at; `pleat=` more than one
	 * mesh step off what the cut gives (R10); `depth=` over the drop (R11). An `every=` that does not divide the stretch is NOT refused
	 * (plan section 8, Q1b): the nearest whole number of pitches is built and `bPitchChanged` says so.
	 */
	GRAPHCODEGARMENT_API bool Rule(const FGcCurtainPanel& Panel, const FGcGarmentTrack& Track, double MeshSpacingCm, FGcCurtainRule& Out,
		FString& OutError);

	/**
	 * Everything about the file's tracks and panels that needs the whole file, after the garment checks: a file of panels holds no
	 * rings or garments (R21); each panel's fabric, the heading's track (R13) and band fabric, the rule (R7-R11), a hang's fabric fields
	 * (R20), its rims' and lines' edges (R16), a crease straight across (R17) and clear of the top, the hem and the band (R18), a press
	 * on a line of the panel in a fabric with a measured crease= (R19), and its build lines (R22).
	 */
	GRAPHCODEGARMENT_API bool Check(const FGcGarmentDocument& Document, FString& OutRefusal);

	/**
	 * One line of the build's note for the rule: "stretch 125.0 cm on $Pole, fullness 2.00, 11 hooks 12.50 cm apart on the pole
	 * (every=12.5cm), 25.00 cm of cloth a pitch", with the pleats for a pinch and, when every= did not divide the stretch, the change.
	 */
	GRAPHCODEGARMENT_API FString RuleNote(const FGcCurtainRule& Worked);

	/** One line for the hang: "laid flat (settle(off))", or "hung fresh: 6 s each run (default), the hooks slid at 15 cm/s (default)". */
	GRAPHCODEGARMENT_API FString HangNote(const FGcCurtainHang& Hang);
}

#endif
#endif
