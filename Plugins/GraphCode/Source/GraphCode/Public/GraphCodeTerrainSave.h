// Copyright GraphCode Sdn Bhd. All Rights Reserved.
//
// A person changes the ground and saves; that is a SCULPT — `GC-TERRAIN-DESIGN.md` §6, and the
// user's own words 2026-08-19: *"when user change and save, it's called sculpt"*, *"the delta is
// hooked to level save"*.
//
// **THERE IS NO SEPARATE ACT CALLED A CAPTURE.** The grammar already had the word: a person's change
// becomes a `sculpt(…)` line and the delta file it reads, and a sculpt of nothing records nothing.
//
// **AND THERE IS NO VERB BESIDE IT** — user-ruled 2026-08-20: *"isn't sculpt equals to delta? auto
// update due to user change on the terrain isn't it? … why is it in GC apply."* A `gc_apply`
// parameter that did the same thing on demand shipped for a day and was a second door onto one act
// ([[a-misplaced-capability-costs-a-name]], twice on this row: first the invented word, then the
// surface that survived the rename). Saving IS the trigger, so this file is the whole surface.
//
// **PRE-SAVE AND NOT POST-SAVE, WHICH IS THE WHOLE REASON THIS IS NOT IN `FGraphCodeService`.**
// Recording flattens the ground it took and re-runs the program, so it MODIFIES the level. Done
// after the bytes are written, every save would leave the level dirty again and the next save would
// find the same work waiting — a level that can never be saved clean. Done before, the edit is part
// of the save that triggered it.
//
// **AND IT CAN NEVER FAIL A SAVE.** Everything here is advisory: a refusal is logged and the save
// proceeds, exactly as the Blueprint compile hook is advisory about sidecars. A person's save is
// not ours to reject.

#pragma once

#include "CoreMinimal.h"

class UWorld;
struct FGcwDocument;
struct FGcTerrainProgram;
struct FGcTerrainField;

class GRAPHCODE_API FGraphCodeTerrainSave
{
public:
	/** Bind the pre-save hook. Idempotent. */
	static void Install();
	static void Uninstall();

	/**
	 * Do the whole act for one world, or say why not — the body of the hook, exposed so it is
	 * testable without saving anything.
	 *
	 * Returns false ONLY on a refusal a caller should hear about. *Nothing had changed* and *this
	 * level has no terrain program* are successes with `OutRecorded` false: they are the ordinary
	 * answers, and a hook that logged a warning on every save of every level would be noise nobody
	 * reads ([[a-silent-no-op-is-worse-than-a-refusal]] cuts the other way when the no-op is the
	 * expected case and is reported as one).
	 */
	static bool RecordForWorld(UWorld* World, bool& OutRecorded, FString& OutSummary,
		FString& OutError);

	/**
	 * The name a save gives a delta it has to mint — `Sculpt_01`, `Sculpt_02`, …
	 *
	 * **THE LOWEST FREE NUMBER, not a timestamp and not a hash.** A save-minted name lands in a
	 * committed document and in a file name beside it, so it has to read as something a person can
	 * talk about and has to be stable in a diff. Most saves mint nothing at all, because a record
	 * that follows an existing `sculpt` MERGES into it (user-ruled: no two consecutive sculpts).
	 */
	static FString MintName(const TArray<FString>& TakenNames);

	/**
	 * The same act, and the advisory notes it found beside it (W1, card P234). `OutNotes` is never a
	 * failure: it says what a person should know about the ground they just saved, and the save
	 * proceeds exactly as before. The four-argument form above calls this and drops the notes.
	 */
	static bool RecordForWorld(UWorld* World, bool& OutRecorded, FString& OutSummary,
		TArray<FString>& OutNotes, FString& OutError);

	/**
	 * Where the saved sculpt `SculptName` OPPOSES one of the program's own `stamp(…)` lines that runs
	 * before it — one note per stamp and direction, naming both:
	 * *"Sculpt_01 raises 312 cm where stamp SafeHousePit lowers it, over 40 cells"*.
	 *
	 * **WHY IT IS SAID AT ALL.** A save records live-minus-program as a sculpt, and a sculpt replays
	 * AFTER every line above it. Where the live ground had not yet taken a stamp (a pit added to the
	 * program while the landscape was still flat there), the record is the stamp's inverse, and every
	 * later apply quietly undoes the stamp. Reported, never repaired and never refused: the same
	 * sculpt usually holds real work elsewhere, and masking it is a grammar question.
	 *
	 * Each contribution is measured with the evaluator itself: a stamp's is the program up to the
	 * sculpt minus the same program without that stamp; the sculpt's is the program through the sculpt
	 * minus the program before it. `Placement` is the field the landscape is evaluated on (size,
	 * origin, spacing). A stamp line that does not run before the sculpt is not compared.
	 *
	 * A cell counts when the two have opposite signs, each is at least
	 * `OpposedStampMinCentimetres`, and the sculpt undoes at least `OpposedStampCancelFraction` of the
	 * stamp there; a note needs at least `OpposedStampMinCells` such cells.
	 */
	static void FindOpposedStamps(const FGcwDocument& Document, const FGcTerrainProgram& Program,
		const FString& SculptName, const FGcTerrainField& Placement, const FString& SourceDir,
		TArray<FString>& OutNotes);

	/**
	 * One whole centimetre: the rounding `FGraphCodeTerrainDelta::HashField` already rules is "the
	 * same ground", and at or above the one stored step (0.78 cm at Z scale 100) the recorder itself
	 * treats as worth recording.
	 */
	static constexpr double OpposedStampMinCentimetres = 1.0;

	/** The sculpt undoes at least half the stamp in that cell: cancelling it, not trimming its edge. */
	static constexpr double OpposedStampCancelFraction = 0.5;

	/**
	 * Four cells, a 2x2 patch. The recorder keeps a single changed sample, but one sample of
	 * opposition is what a stamp's soft edge and a brush's falloff produce where they merely touch;
	 * a refilled pit or a flattened mound covers many.
	 */
	static constexpr int32 OpposedStampMinCells = 4;
};
