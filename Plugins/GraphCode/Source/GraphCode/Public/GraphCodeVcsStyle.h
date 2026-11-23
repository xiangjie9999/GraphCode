#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"

class UBorder;
class UButton;
class UComboBoxString;
class UTextBlock;
class UWidget;

/**
 * Every colour the version-control panel draws, in one place.
 *
 * WHY THIS FILE EXISTS. The first panel spelled its colours as literals inside `WBP_GraphCodeVcs.gc`
 * — forty-odd `SpecifiedColor=(R=...)` tuples, one per widget, no two of which any tool could tell
 * apart from a typo. Two consequences, and the second is the one a person actually met: nothing could
 * be restyled without editing forty lines, and the BUTTONS were never styled at all. A `Button()`
 * with no `WidgetStyle` gets UMG's default light-grey brush, and the `TextBlock` inside it gets the
 * default WHITE — which is how a panel ships with eleven controls whose labels cannot be read.
 *
 * So the split is: the `.gc` carries STRUCTURE and this file carries MEANING. A layout says "this is
 * a primary action"; this file says what a primary action looks like. It is the same rule
 * `UGraphCodeVcsRow::StateWord` already followed for words — one table, so nothing drifts — applied
 * to the other half of what a row says.
 *
 * ROUNDED BOXES RATHER THAN `FAppStyle` BRUSHES. `FAppStyle` is the right source for an editor tool's
 * feel and the brief (§8) asks for it, but its button brushes are nine-slice images tuned for the
 * toolbar's own background, and the panel needs a guaranteed contrast ratio against a background it
 * chooses. A `FSlateRoundedBoxBrush` is a solid fill with a radius: it has no resource to outlive it,
 * it renders identically under `-nullrhi` geometry passes, and its contrast is a number in this file
 * rather than a property of whatever theme is loaded.
 */

/**
 * What a button is FOR, which is the only thing a layout should have to know.
 *
 * `Take` is deliberately ONE role and not `TakeOurs`/`TakeTheirs`. The brief's §6 rule is that ours
 * and theirs are symmetrical and neither is visually recommended; two roles is an invitation to make
 * one of them greener than the other, and a person choosing under time pressure reads that as advice.
 * The two buttons differ in the direction of their arrow and in nothing else.
 */
enum class EGcButtonRole : uint8
{
	/** The ordinary action. Refresh, the overflow menu, the details toggle. */
	Default,

	/** The one action the current state is asking for. At most one is visible at a time. */
	Primary,

	/** Sits on a surface and only appears on hover. Chips, toggles, close buttons. */
	Ghost,

	/** Resolving a conflict, either side. See the note above about why there is one of these. */
	Take,

	/** A statement about coverage, not an error. The uncompared-facets chip. */
	Warning,
};

enum class EGcTextRole : uint8
{
	/** What the panel is telling you. */
	Primary,

	/** Context for it: paths, counts, the second half of a sentence. */
	Secondary,

	/** Column captions and field labels. Present, not read. */
	Faint,
};

/**
 * What a NUMBER counts, which is the only thing that decides its colour.
 *
 * THE PANEL'S NUMBERS ARE ITS SHORTEST SENTENCES. `+12` in the added colour beside a file name says
 * what a paragraph would; an amber `3` in the header says three decisions are waiting without the
 * word "decisions" having to be read at all. This enum is the second half of the icon+word+colour
 * rule for the case where the "word" is a quantity — the colours are the diff's own, so a green
 * number and a green line mean the same thing on the same screen.
 */
enum class EGcCountRole : uint8
{
	/** Lines gained, and a file git has never seen. */
	Added,

	/** Lines lost, and a file that is gone. */
	Removed,

	/** Decisions waiting. Amber, because a conflict has no preferred side to colour. */
	Decision,

	/** A count with no urgency in it: how many files were touched at all. */
	Quiet,
};

/** Which side of a comparison a diff cell is, which decides its fill and its text colour. */
enum class EGcDiffTone : uint8
{
	/** Unchanged. No fill; the text recedes so the changed lines are what the eye finds. */
	Context,

	/** Present on the left and not on the right. */
	Removed,

	/** Present on the right and not on the left. */
	Added,

	/**
	 * An unresolved decision. AMBER ON BOTH SIDES, and that is the ruling this enum exists to record:
	 * red/green is a factual asymmetry in a diff — one text is older than the other — and in a
	 * conflict there is no older side. Colouring ours red and theirs green would turn the panel into
	 * a recommendation, which §6 forbids.
	 */
	Conflict,

	/** A section heading inside the list. Spans both columns. */
	Heading,
};

struct GRAPHCODE_API FGcVcsStyle
{
	// ---- surfaces ---------------------------------------------------------------------------

	/** The page. Everything else is measured for contrast against this. */
	static FLinearColor Surface();

	/** Header and footer: lifted, so the chrome reads as chrome. */
	static FLinearColor SurfaceRaised();

	/** The file list and the details strip: recessed, so the diff is the lit part of the panel. */
	static FLinearColor SurfaceSunken();

	static FLinearColor Divider();

	/** The row under the pointer, and the row that is selected. */
	static FLinearColor RowHover();
	static FLinearColor RowSelected();

	// ---- text -------------------------------------------------------------------------------

	static FLinearColor Text(EGcTextRole Role);

	/** The ink a number gets. One table with `DiffText`, so a green number and a green line agree. */
	static FLinearColor Count(EGcCountRole Role);

	// ---- diff -------------------------------------------------------------------------------

	/** The band behind a cell. Transparent for context, so unchanged text sits on the page. */
	static FLinearColor DiffFill(EGcDiffTone Tone);

	/**
	 * The base a comparison column sits on, before any per-line tone.
	 *
	 * THE TWO COLUMNS WERE THE SAME BLACK, and a side-by-side whose halves are indistinguishable is
	 * two files' worth of text in one rectangle. Most of a comparison is CONTEXT — identical lines,
	 * no fill, both sides drawn straight onto the page — so on a file whose differences are three
	 * lines out of four hundred there was nothing on screen to say where one side ended and the
	 * other began. The right column is lifted a little, and a divider runs between them.
	 */
	static FLinearColor DiffColumn(bool bRightSide);

	static FLinearColor DiffText(EGcDiffTone Tone);

	/** Line numbers and the `+`/`-` column: the tone's colour, dimmed, so it guides without shouting. */
	static FLinearColor DiffGutter(EGcDiffTone Tone);

	// ---- sidecar state ----------------------------------------------------------------------

	/**
	 * The colour a `.gc`'s own state gets, wherever it is drawn.
	 *
	 * Moved here from `UGraphCodeVcsRow` because the row is no longer the only thing that draws it —
	 * the details strip and the file list both do, and a table with two homes has one home too many.
	 */
	static FLinearColor StateColor(uint8 State);

	// ---- controls ---------------------------------------------------------------------------

	static FButtonStyle ButtonStyle(EGcButtonRole Role);

	/**
	 * Style a button and the label inside it.
	 *
	 * THE LABEL IS FOUND, NOT BOUND. A button's text is a child widget with its own name, and binding
	 * eleven of them into C++ purely to set their colour is eleven `BindWidget` properties that say
	 * nothing. `GetChildAt(0)` is what a `UButton` has exactly one of.
	 */
	static void Apply(UButton* Button, EGcButtonRole Role);

	static void Apply(UTextBlock* Block, EGcTextRole Role);

	/**
	 * **NO LABEL IN THIS PANEL MAY DRAW PAST ITS BOX.**
	 *
	 * User-reported 2026-08-05 — *"it overflows. check everything to ensure nothing overflows"* —
	 * over the left column at its default 300px holding `COMPARE · local ↔ branch
	 * claude/gc-local-history-restore`. A Slate `STextBlock` does not clip by default: it asks for
	 * the width its text needs, is arranged into whatever its slot has, and then DRAWS THE WHOLE
	 * STRING ANYWAY. Inside a `SizeBox` with a hard override that is a sentence lying across the
	 * divider and over the diff.
	 *
	 * Every string that reaches this panel comes from somewhere with no length limit — a repository
	 * path, a branch name, an asset name, a summary the apply wrote — so the answer cannot be shorter
	 * labels. It is one rule, applied wherever a label is styled, and `Apply` above calls it so that
	 * "styled through the table" and "cannot overflow" are the same act.
	 *
	 * A wrapping block is left alone: it already has an answer to a long sentence, and an ellipsis
	 * would cut off the last line it just made room for.
	 */
	static void NeverOverflow(UTextBlock* Block);

	/** Paint a surface. `Tint` is one of the surface colours above. */
	static void Apply(UBorder* Border, const FLinearColor& Tint);

	/**
	 * The comparison picker, which is a control and not a button and so needs its own pass.
	 *
	 * It shipped on UMG's default style: the largest, palest text on screen, announcing itself as the
	 * most important thing in the panel when it is a selector.
	 *
	 * ONLY THE BUTTON STYLE IS SET HERE. `UComboBoxString`'s `Font` and `ForegroundColor` are
	 * construction-time only — the header says so, and `InitFont`/`InitForegroundColor` are the only
	 * ways in — so those two live in the layout. Setting them at construct time silently does nothing,
	 * which is worse than not offering it.
	 */
	static void Apply(UComboBoxString* Box);

	/**
	 * The typed-moment box, and it is the LAST control in this panel that was still wearing UMG's.
	 *
	 * **REPORTED AS UNREADABLE, NOT AS UGLY** — *"i couldnt even read it. can you?"*, over a
	 * screenshot of `05 Aug 2026 15:52` in the stock grey on the stock near-white field, sitting in
	 * a panel whose every other surface is near-black. That is a light control in a dark room: the
	 * field itself is the brightest rectangle on screen and the text inside it is the dimmest thing
	 * in that rectangle, so it reads as a disabled box with placeholder junk in it.
	 *
	 * It is the same hole `Apply(UComboBoxString*)` was written to close, one control along: every
	 * OTHER surface here comes from the table above, and each one that does not opens a gap in the
	 * middle of it. This one hid longer because the box only appears while the comparison is the
	 * timeline.
	 *
	 * UNLIKE THE COMBO BOX, THE FONT IS SETTABLE HERE AND IS STILL NOT SET. `UEditableTextBox`'s
	 * `WidgetStyle.TextStyle.Font` is a real runtime property, but the layout already carries the
	 * size — 9pt, to stop it drawing at the UMG default and clipping its own value — and a size set
	 * in two places is a size that disagrees with itself the first time either moves.
	 */
	static void Apply(class UEditableTextBox* Box);

	/**
	 * Say that a thing can be clicked.
	 *
	 * NOTHING in the first panel changed the cursor, so a file row — the one control the whole screen
	 * depends on being used — did not read as a target at all. A pointer that never changes shape is a
	 * screen where nothing looks interactive.
	 */
	static void MakeClickable(UWidget* Widget);
};
