// Copyright GraphCode Sdn Bhd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GraphCodeShapeValue.h"

struct FGcShapeInstructionScope;

/**
 * A PARSED `.gcshape`. `GC-SHAPE-GRAMMAR.md` §1 is the file and it is canonical.
 *
 * **A FILE HAS TWO HALVES AND THEY NEVER MIX.** One `build { }` block lists the assets the file
 * produces, one line each, and holds nothing else; everything below it is reference. That split is
 * the reason this type has two arrays rather than one ordered list — the manifest is not a step.
 *
 * **THE PARSER KNOWS ALL FIVE ENTRY KINDS AND M1 EVALUATES TWO.** `profile()`, `material()` and
 * `plane()` are recognised here because refusals depend on it — *a build line naming a profile, a
 * number or a material* cannot be refused by a parser that cannot tell those apart from a mesh —
 * and because a kind word costs one row where an evaluator costs a milestone.
 *
 * Nothing here resolves a `$name`. That is the DAG, it is M3, and a parse that quietly resolved
 * would make the cycle refusal impossible to write in the one place it belongs.
 */

enum class EGcShapeEntryKind : uint8
{
	/** `Name = 52.0` — a number, in centimetres or degrees where an angle is asked. */
	Number,

	/** `Name = profile(0.0 = 100%, 1.0 = 82%)` — a value at any `t`. M2 evaluates it. */
	Profile,

	/** `Name = material('/Game/…')` — a SLOT; a path is a first-build default only. M7. */
	Material,

	/** `Name = plane() { … }` — an outline. Groups 1, 2 and 5. M4. */
	Plane,

	/** `Name = mesh() { … }` — a shape. All six groups. */
	Mesh,

	/** A reusable sequence applied to the current mesh, without creating a new subject. */
	Instruction,

	/**
	 * `Name = random(min = 10.0, max = 20.0)` — a NUMBER THAT WAS DRAWN. MC4 §2.5.
	 *
	 * **DECLARABLE BECAUSE `profile()` AND `curve()` ALREADY ARE, and a declared value has to be
	 * self-contained** — which is what puts the optional `seed=` INSIDE the parens rather than
	 * leaving it to whatever line happens to use the name.
	 *
	 * **THE SEED'S PRESENCE IS THE SWITCH, and it is the whole of the type.** Written, the draw is
	 * ABSOLUTE — the same number everywhere the name is used, which is how two headlights are made
	 * to match. Omitted, the draw COMPOSES with the seed of the call it rides on, so two walls
	 * sharing one declared value look different. A composing draw therefore has no reading away
	 * from a call, and `FGcShapeProfile::Parse` refuses it there by name.
	 */
	Random,

	/**
	 * `Pillar = shape('/Game/Shapes/Pillar')` — ANOTHER `.gcshape` FILE, brought in under a name.
	 * `GC-SHAPE-GRAMMAR.md` §3.1.
	 *
	 * **IT IS AN ENTRY, NOT A HEADER LINE, and that is what makes `Pillar` a first-class citizen.**
	 * It reads exactly like `material('/path')` — a `Name = verb('/quoted/path')` whose value is an
	 * address on disk — so the machinery that already declares, rebinds and orders the other five
	 * kinds carries this one for free. The alias is the entry's name; the path is its `Literal`,
	 * unquoted, and it is resolved to a sibling `.gcshape` document at LOAD time, never at parse —
	 * cross-file was once out of scope and is in scope now.
	 *
	 * **A MODULE IS REACHED WITH A DOT AND NEVER STANDS ALONE.** `$Pillar.Column` names the entry
	 * `Column` in that file; a bare `$Pillar` is refused, because a whole file is not a shape. The
	 * `$` still means *a name this file declares* — `Pillar` — and the dot reaches a member of it,
	 * which is the one rule that keeps every reference in the format carrying a `$`.
	 *
	 * **AND IT MAY CARRY ARGUMENTS — `shape('/…/Fixings', Thread_R = 0.5)`, §3.1.** Each one rebinds
	 * a top-level NUMBER of the imported file for this import only, so one preset is a family the
	 * way a parameterised `mesh()` already is. They live in `Arguments`; the linker settles them in
	 * the caller's scope, applies them to a PRIVATE copy of the file, and two imports that pass
	 * different numbers are two documents rather than one shared node.
	 */
	Module,

	/**
	 * `Curl = angular profile(0 0 0, 0 20 0, 0 40 20)` — three angles per stop, user-proposed
	 * 2026-09-14. Its reader is `FGcShapeAngularProfile::Parse`, and `raise(angle=)` its one use.
	 */
	AngularProfile,

	/** Reusable ordered scalar rules, bound to a selection's surface at use. */
	Weightage,

	/**
	 * `Ref = reference(side="Refs/side.png", front="Refs/front.png", length=455, width=180)` — pictures a shape is MEASURED
	 * against, never built from (field report A6, 2026-10-29). A build line opts in with `[reference=$Ref]`; the build notes
	 * say per view how the shape's silhouette differs. Its arguments stay in `Arguments` and are read at use, where the
	 * pictures are loaded relative to the `.gcshape`'s own folder (`GraphCodeShapeReference.cpp`).
	 */
	Reference,
};

/** One argument of one instruction. An empty `Name` is THE positional one — at most one per line. */
struct GRAPHCODESHAPE_API FGcShapeArgument
{
	FString Name;
	FString Value;
};

/**
 * One declared parameter of a `mesh()` or `plane()` entry — `Brick = mesh(w = 39.0)`. MC4 §2.1.
 *
 * **`Default` IS NEVER EMPTY, AND THAT IS THE INVARIANT RATHER THAN A CONVENIENCE.** The rule is
 * that a `$name` always resolves — *"global one might accidentally not declare something"* — so an
 * entry parameter carries a default because its value comes from OUTSIDE and the call may omit it.
 * `add($Brick)` therefore always builds. A body-local variable needs no default for the opposite
 * reason: it is assigned at the moment it is declared, so there is no window in which it is unset.
 *
 * **AND THERE IS NO TYPE WORD, user-ruled 2026-08-29 — THE DEFAULT ALREADY NAMES THE TYPE.** The
 * first build spelled these `w: number = 39.0`, and the `number` was redundant with the `39.0` beside
 * it: `(0,0,0)` is a point, `82%` is a percentage, `$Flat` is whatever kind that entry is. A word
 * that only ever repeats its neighbour is a word to delete ([[subtraction-over-addition]]), and the
 * type is still derivable from the default on the day something needs to check one.
 *
 * **DECLARATION IS MANDATORY and the default rule is what forces it**: an inferred parameter — one
 * conjured from any unresolved `$name` in a body — has nowhere to put a default, so `add($Brick)`
 * would leave it dangling. It would also turn a typo into a feature: `$widht` is refused today as
 * *not an entry in this file*, and under inference it would quietly become a parameter nobody passes.
 */
struct GRAPHCODESHAPE_API FGcShapeParameter
{
	FString Name;

	/** The declared default, verbatim and unevaluated. Never empty — see the note above. */
	FString Default;

	int32 Line = 0;

	/**
	 * **K1 — WHERE THIS PARAMETER CAME FROM** (`Docs/GC-SHAPE-DAMAGE-KIT.md` §6, lead-ruled 2026-11-19). Empty on a
	 * parameter the list declares. On one a bare `$Entry` clause TOOK, that clause as written (`$Dmg.Snap`), and
	 * `Default` is then that entry's own — so editing it there changes what this entry builds. Set by the link.
	 */
	FString From;

	/**
	 * K1: on a DECLARED parameter whose name a bare `$Entry` clause also offers, what this list overrides —
	 * `$Dmg.Snap = 0`, the clause and its default (several joined by `; `). The declared default is the one used,
	 * and the value still reaches that entry's lines. Set by the link; empty otherwise.
	 */
	FString Overrides;

	/**
	 * **G29 item 6b — `wvar=0.08 [range=(0, 0.4)]`: THE KNOB ARRIVES CLAMPED.** Every value bound to it (its default, a
	 * call's argument, a forwarded one) is clamped into `[RangeLow, RangeHigh]` before the body reads it, so a K1 wrapper
	 * forwards the clamped value and no `$wvarC = clamp($wvar, …)` local is needed. A value already inside keeps its text.
	 * Taken with the parameter by a K1 clause. `range=(min, max)` is the `.gcrig` joint's spelling.
	 */
	bool bRanged = false;
	double RangeLow = 0.0;
	double RangeHigh = 0.0;
	/** As written, `(0, 0.4)`, for the index and the refusals. */
	FString RangeText;
};

/** One line of an entry's body: a verb, its arguments, and the `[ ]` modifiers to its right. */
struct GRAPHCODESHAPE_API FGcShapeInstruction
{
	/** Runtime-only input region for a reusable instruction. Never serialized. */
	TSharedPtr<const FGcShapeInstructionScope> Scope;
	/**
	 * The optional HANDLE on the line's left — `topCap = add($Cap, at=on())`. Empty on most lines.
	 *
	 * **USER-RULED 2026-08-23: A RELATION MEASURES A PLACEMENT, AND A PLACEMENT HAS TO BE DECLARED.**
	 * `against($Cap, +Z)` names the SHAPE, and a shape can be brought in more than once, so the line
	 * has no answer — *"you can't just use `$cap` inside, something so vague."* A handle names one
	 * `add` line, and one line places one thing, so the question stops existing rather than getting
	 * an arbitrary answer.
	 *
	 * **AN ENTRY NAME MEANS THE SHAPE; A HANDLE MEANS WHERE A COPY OF IT IS.** That split is why
	 * `span($Shaft, X)` keeps taking an entry — how wide the shaft is, is a property of the shaft —
	 * while `against(…)` cannot.
	 *
	 * **IT IS A NEW TOKEN, NOT A NEW LINE** ([[a-new-token-not-a-new-line]]): the `.gc` graph grammar
	 * already writes `n2 = Multiply(in0=n1.out0)`, a statement with a handle on its left, so this is
	 * the vocabulary GraphCode already has rather than a spelling invented for Shape.
	 */
	FString Handle;

	FString Verb;
	TArray<FGcShapeArgument> Arguments;

	/** Each modifier as written, without its brackets: `top`, `!cut`, `below=120`. */
	TArray<FString> Modifiers;

	/**
	 * `if (<comparison>) { … } else { … }` — the ONE instruction that holds other instructions.
	 *
	 * **A BLOCK RATHER THAN A CONDITION PER LINE, AND THE DIFFERENCE IS CORRECTNESS.** Three lines
	 * each carrying `when=random(…) < 0.3` would each DRAW THEIR OWN NUMBER and disagree about
	 * which copy they were talking about — a silent split nothing could see. A block asks once and
	 * every line inside it agrees. User-ruled 2026-09-05.
	 *
	 * **IT IS ASKED WHEN THE ENTRY IS BUILT, AND THAT NEEDED NO NEW RULE.** The condition reads the
	 * entry's own parameters, and a drawn argument on a duplicating line already makes every copy
	 * its own build with its own bindings — so `if ($wear > 0.7)` is asked once per copy for free,
	 * and two copies that draw the same number correctly share one cached mesh.
	 *
	 * `Condition` is the text inside the parentheses, verbatim. Empty on every other instruction,
	 * which is what `IsConditional()` reads.
	 */
	FString Condition;

	/** The instructions inside `{ }`. A conditional and a `loop` both have them. */
	TArray<FGcShapeInstruction> Then;

	/** The instructions inside `else { }`. Empty where there is no `else`. */
	TArray<FGcShapeInstruction> Else;

	/**
	 * `loop (<count | $Set | arrangement>) { … }` — **THE SECOND INSTRUCTION THAT HOLDS OTHERS,
	 * AND IT RUNS THEM AGAIN.**
	 *
	 * **WHY A WORD RATHER THAN A PLURAL ON EACH VERB — USER-RULED 2026-09-23**: *plural can only get
	 * you some verb for looping, `loop(){}` can get you everything.* The proposal it replaces was a
	 * plural SELECTION (`save($Buttons) [repeat=(4,3)]`, then one `displace(to=$Buttons)`), which
	 * buys the plural for one verb at a time and leaves every verb written afterwards without it.
	 * This buys it for all of them at once, including the ones not written yet.
	 *
	 * **THE THING IT BEHAVES LIKE IS `add(){}`, NOT `if` — USER-RULED 2026-09-23.** A group-5 body
	 * under `repeat=` already runs once per copy, and that is exactly this repetition; `if` only
	 * ever chooses a branch and runs it once. So the rules worth inheriting are the part body's:
	 * once per pass, the drawn-copy budget rather than the stamping one, and a body that builds
	 * nothing is a step that built nothing rather than an error.
	 *
	 * **AND IT TAKES `add()`'S WHOLE ARRANGEMENT — USER-RAISED 2026-09-23, THE DAY IT SHIPPED**:
	 * *why isn't it just like `add()`? with repeat, with jitter etc., offset?* An arrangement
	 * produces N frames and a loop wants exactly N frames, so §4.2's `repeat=`, `step=`, `spread=`,
	 * `turn=` + `around=`, `stagger=`, `along=$Path`, `onto=`, `orient=`, `scatter=`/`apart=`,
	 * `jitter=` and `seed=` all sit on this word and are read by §4.2's own reader. **What shipped
	 * first — a bare count — is the positional spelling of `repeat=` and a narrow subset of this**,
	 * not a different idea. The bracket names the faces a cast goes at, as it does on `add`.
	 *
	 * **WHAT THE ARRANGEMENT MEASURES AGAINST IS THE ACCUMULATOR**, which is the same rule one
	 * floor down: `spread=`'s percentage is a fraction of the target's span and `onto=` casts at a
	 * surface, and with no part in the line both resolve against the entry's own shape.
	 *
	 * **AND THE ONE PLACE IT INVERTS `add(){}` IS THE SUBJECT, WHICH IS THE THING THIS GRAMMAR SAYS
	 * DECIDES EVERYTHING ABOUT BRACES.** §5: *a `{ }` body shapes the part on its way in, and the
	 * subject inside it is that part* — nothing of the host is in scope, so `[top]` inside
	 * `add($Nail) { … }` is the NAIL's top. A loop brings no part into being, so there is nothing
	 * for it to scope TO: its subject is the entry's own accumulator, and `[top]` inside a loop is
	 * the shape being built. Same braces, same once-per-pass evaluation, opposite subject.
	 * **That inversion is why it wears a different word rather than an empty `add`** — the reader
	 * needs to see at the line which way the scope points, and it is also what lets a loop do
	 * CUMULATIVE work, since the accumulator it is writing into survives the pass.
	 *
	 * **IT STILL NEVER STAMPS, WHICH IS WHAT KEEPS ITS BUDGET AT THE DRAWN 500.** `repeat=` on a
	 * part may reuse one cached mesh when nothing about a copy varies; a loop re-evaluates its body
	 * every pass by definition, so there is no cached mesh to stamp.
	 *
	 * `Times` is the text inside the parentheses, verbatim. **It is what `IsLoop()` reads and what
	 * a refusal quotes back**; the same text is also parsed into `Arguments`, where the positional
	 * one is the count or the `$Set` and the named ones are the arrangement.
	 */
	FString Times;

	/**
	 * `$Course = $Course + 1` — **A NAME SET INSIDE A BODY, AND THE ONE STATEMENT THAT IS NOT AN
	 * INSTRUCTION.** `SetName` is the name without its `$`; `SetValue` is the text on the right,
	 * verbatim, read the way every other number in this format is read.
	 *
	 * **THE SPELLING IS THE ONE THE FILE ALREADY USES, AND THE SCOPE IS WHAT DECIDES ITS MEANING.**
	 * At document level `$Name = …` re-points a name from that line down and is settled once, as the
	 * file is read, because the DAG up there is walked ON DEMAND and out of order. Inside a body the
	 * order is the author's own, so the same sentence — *from here down, this name means this* — is
	 * executed rather than settled. One reading, two scopes, and it is the split MC2 already made
	 * when it ruled that a top-level `Barrel_R` cannot be reassigned and a captured seam can.
	 *
	 * **A LINE BEGINNING WITH `$` CANNOT BE A VERB LINE**, which is what makes this unambiguous
	 * rather than a second reading of an existing form: an instruction is a BARE name and
	 * parentheses, and a HANDLE is a bare name and an `=`. Neither may start with a `$`.
	 *
	 * **THE ONE OTHER THING A `$` LINE MAY BE IS A CALL — `$Erode(depth=.4) [top]`, 2026-09-11 —
	 * AND THE CHARACTER AFTER THE NAME TELLS THEM APART**: an `=` sets, a `(` calls. A call is not
	 * a set and carries no `SetName`; the parser gives it the `instruction` verb and puts the
	 * recipe in the positional argument, where every reader of a group-5 target already looks.
	 */
	FString SetName;
	FString SetValue;

	/**
	 * `$x[$j] = …` / `$joint[$i][$j] = …` — **G27, AN INDEXED NAME** (2026-11-19). Each bracket's
	 * text, verbatim and in order; empty on every set written before. The name stays `SetName` and
	 * is never substituted; each index is a READ, so it is substituted and evaluated where the set
	 * runs, and the whole number it comes to picks the entry (`x[3]`, `joint[1][-2]`).
	 */
	TArray<FString> SetIndices;

	/**
	 * **G28h — WHAT THIS LINE IS, AS A SEEDLESS DRAW'S KEY** (2026-11-19, ruled). Set by the parser after the file is
	 * whole, never written by an author: the path of `if`/`loop` arms above it, its own signature (the verb, the part
	 * it names, its argument names; a set's name), and its ordinal among identical siblings in one block. The walk adds
	 * the loop passes. It replaces G16's line offset, so a line added above a draw no longer re-rolls it. Empty on a
	 * line no parser keyed (a `select()` read at run time), which keeps the offset.
	 */
	FString DrawKey;

	int32 Line = 0;

	/** True on an `if`, whose payload is other instructions CHOSEN. */
	bool IsConditional() const;

	/** True on a `loop`, whose payload is other instructions RUN AGAIN. */
	bool IsLoop() const;

	/** True on `$Name = …`, which places nothing and builds nothing. */
	bool IsSet() const;

	const FGcShapeArgument* Find(const TCHAR* Name) const;

	/** The one positional argument, or null. */
	const FGcShapeArgument* Positional() const;
};

/** One line of `build { }`: the asset on the left, the entry that becomes it on the right. */
struct GRAPHCODESHAPE_API FGcShapeBuildLine
{
	/** As written — a bare name resolved against `into:`, or a full path. */
	FString AssetName;

	/** The `$Entry` on the right, without its `$`. */
	FString EntryName;

	TArray<FString> Modifiers;
	int32 Line = 0;

	/** `<into:>/Name` for a bare name; the name itself when it is already a path. */
	FString ResolvePath(const FString& Into) const;
};

/**
 * `bake(Shell, resolution=2048, except=$Glass)` inside `build { }` — MB51,
 * `GC-MATERIAL-BAKE-DESIGN.md` §0.
 *
 * **IT IS A STATEMENT RATHER THAN A PER-LINE SETTING BECAUSE A TARGET IS SHARED.** Every build
 * line carrying `[bake=Shell]` collapses into ONE atlas and ONE generated material, so four meshes
 * of one building become one draw call. A setting that lived on the line could only ever give each
 * mesh an atlas of its own, which is the saving nobody needed.
 *
 * **THE NAME IS ALSO THE GENERATED FAMILY'S STEM** — `M_Shell_Baked`, `T_Shell_BaseColor` — so
 * there is no second `name=` to disagree with it.
 */
struct GRAPHCODESHAPE_API FGcShapeBakeTarget
{
	/**
	 * The LEFT-HAND SIDE — the asset this bake produces, as written: `M_Chest = bake(Chest, …)`.
	 *
	 * **USER-RULED 2026-09-22: a bake is an OUTPUT, so it names one the way every other line in
	 * `build { }` does.** It is not decoration. `FMaterialUtilities::CreateProxyMaterialAndTextures`
	 * mints its asset wherever it is told and the first implementation DERIVED that path — which
	 * was wrong on the first GPU run and could not be asserted against. An authored name has
	 * nothing to predict ([[no-unnecessary-hard-coding]]).
	 */
	FString Output;

	/**
	 * The target's name, which build lines point at with `[bake=<Name>]`.
	 *
	 * **`decal` IS RESERVED HERE** — `T_ChestDecals = bake(decal, resolution=1024)` declares the
	 * DECAL SHEET rather than a material target (MB52, `GC-SHAPE-DECAL-DESIGN.md` §3). A material
	 * target may not take that name, which costs an author nothing and keeps one word meaning one
	 * thing.
	 */
	FString Name;

	/** Whether this statement declares the decal sheet rather than a material collapse. */
	bool IsDecalSheet() const { return Name == TEXT("decal"); }

	/** Square, in pixels. Zero is unstated and resolves to `FGcShapeOutput::DefaultBakeResolution`. */
	int32 Resolution = 0;

	/** Requested surface texels/metre; zero inherits the legacy resolution-only layout. */
	double TexelDensity = 0.0;

	/**
	 * Materials that keep their own slot, by name without the `$`.
	 *
	 * **EXCLUSION RATHER THAN INCLUSION IS THE RULED DIRECTION** (§3a): the ordinary case is twenty
	 * materials that bake and one sheet of glass that cannot, and a rule that could only take the
	 * whole mesh refused every building in this corpus.
	 */
	TArray<FString> Except;

	int32 Line = 0;

	/** `<into:>/Output` for a bare name; the name itself when it is already a path. */
	FString ResolvePath(const FString& Into) const;
};

/**
 * **WHAT EACH VERB ACCEPTS — ONE TABLE, AND EVERY READER GOES THROUGH IT.** User-directed
 * 2026-08-29: *"have one source of truth, everyone goes to that table first"*.
 *
 * **THE PROBLEM IT SOLVES IS A HOLE THAT WAS ALREADY OPEN, not one parameters introduced.** Every
 * reader in this plugin asked for the names it knew — `Line.Find(TEXT("at"))`, thirty-eight of them
 * across six files — and NOTHING ever asked whether an argument the author wrote had been claimed by
 * anybody. So `add($Cap, ate=(0,0,100))` parsed, built, reported success, and put the cap at the
 * origin: the placement reader looked for `at`, did not find it, and used its default while the
 * author's typo sat in the instruction unread. Same shape as `mesh(junk)` before MC4 §2.1 closed it.
 *
 * **AND IT IS ENFORCED IN BOTH DIRECTIONS, which is what makes a hand-written table safe.** A table
 * kept beside the readers would drift from them silently; a table they must go through cannot.
 *   - A READER asking for an argument its verb does not declare trips `Find`'s check immediately —
 *     so a table row that is missing something is a failing spec, not a quietly ignored argument.
 *   - A FILE naming an argument the verb does not declare is refused by name after parsing — so a
 *     row that wrongly excludes something is a failing corpus build.
 * Neither direction can fail quietly, which is the property `gc_search.kind` lacked for the whole
 * life of that verb.
 */
namespace GraphCodeShapeVerbs
{
	/** Is this a verb at all? `polygon` is retired and answers false — it refuses by name earlier. */
	GRAPHCODESHAPE_API bool IsVerb(const FString& Verb);

	/** Does `Verb` accept a named argument called `Argument`? Case-insensitive, as `Find` is. */
	GRAPHCODESHAPE_API bool Declares(const FString& Verb, const FString& Argument);

	/** Is `Argument` one of the words that make copies — `FGcShapeArray::Read`'s, whichever verb it is on? */
	GRAPHCODESHAPE_API bool IsArrayWord(const FString& Argument);

	/**
	 * **`output` — THE ONE NAME THE LANGUAGE KEEPS, AND IT IS THE PREFIX THAT ALREADY EXISTED.**
	 *
	 * **USER-RULED 2026-09-23, AND BOTH HALVES OF THE RULING ARE CORRECTIONS OF MINE.** I proposed
	 * `$loop.place` and the user's objection killed it in one line: *`$output` and `output` is
	 * unambiguous, `$loop` and `$loop` is [not]* — because **`loop` IS A KEYWORD**. It is the verb
	 * on `loop (…) { … }`, so `$loop.place` puts a sigil on a word the grammar already spells, and
	 * one word ends up naming both an instruction and a namespace. `output` is no verb, no
	 * modifier and no selector; it collides with nothing.
	 *
	 * **AND A LOOP PASS *IS* AN OUTPUT — THE USER'S OWN WORD FOR IT WHEN THEY ASKED FOR IT**: *we
	 * should have an output to consume within `{}` like position.* So `output.` is not a stretched
	 * reading; it is the word the feature was requested in.
	 *
	 * **WHAT THIS FUNCTION GUARDS IS THE PREFIX, ONCE.** `output` is refused as an entry, a
	 * parameter, a handle and a set, so `output.place` can never mean two things — this format's
	 * own answer three times over (an entry declared twice, a handle against an entry, a parameter
	 * against an entry: the second declaration loses). **That hole was open until now**: nothing
	 * refused `output = mesh() { … }`, which made the SHIPPED `$output.Tip` ambiguous already.
	 *
	 * **THE FOUR MEMBERS ARE GUARDED SEPARATELY AND NARROWLY** — see `IsReservedMember`. Reserving
	 * `place`/`index`/`normal`/`turn` file-wide was measured and refused shipped files:
	 * `AwningFan.gcshape` declares `Arm = mesh(index=0)`, `HandAnatomy.gcshape` names a handle
	 * `index`. Neither is published under `output.`, so neither is touched.
	 *
	 * Asked case-insensitively, because a reservation an author can slip past with a capital is not
	 * a reservation.
	 */
	GRAPHCODESHAPE_API bool IsReservedName(const FString& Name);

	/**
	 * **THE FOUR A LOOP PASS ANSWERS UNDER `output.`** — `place`, `index`, `normal`, `turn`.
	 *
	 * Reserved ONLY as names a part publishes into that namespace, which is the narrowest place
	 * the collision can happen. An author's own `$index`, a parameter called `index` and a handle
	 * called `index` are all still theirs.
	 */
	GRAPHCODESHAPE_API bool IsReservedMember(const FString& Name);

	/** Everything `Verb` accepts, sorted — the list a refusal prints so a near miss is one word away. */
	GRAPHCODESHAPE_API TArray<FString> Arguments(const FString& Verb);

	/**
	 * Does this verb act on EDGES rather than faces? MC2 §3 — *judge on usage*, user-ruled.
	 *
	 * **A NAMED VARIABLE HOLDS A REGION OF FACES AND THE LINE THAT READS IT TAKES THE HALF IT CAN
	 * USE**: `round(4.0) [$Joins]` bevels the region's BORDER, `material($Glass) [$Joins]` paints
	 * its FACES. Every instruction in this format takes exactly one currency, so the verb is the
	 * whole answer and no second name is needed to disambiguate.
	 *
	 * **IT IS IN THIS TABLE RATHER THAN BESIDE IT, FOR THE REASON THE TABLE EXISTS.** A private
	 * verb-to-currency map in the selection layer would be a second answer to *what does this verb
	 * take*, and the copy that drifts is the one nobody is looking at — which is the whole argument
	 * that put the argument names here ([[one-table-every-reader-goes-through]]).
	 */
	GRAPHCODESHAPE_API bool TakesEdges(const FString& Verb);
}

/** An entry can point INTO another document — see `FGcShapeEntry::ForwardTo`, §3.1. */
struct FGcShapeDocument;

/** A top-level entry: a name, a kind, and either a literal or a body. */
struct GRAPHCODESHAPE_API FGcShapeEntry
{
	FString Name;
	EGcShapeEntryKind Kind = EGcShapeEntryKind::Mesh;

	/**
	 * **THE `#` BLOCK WRITTEN DIRECTLY ABOVE THIS ENTRY, AS PROSE — user-ruled 2026-09-11.**
	 *
	 * *"the `#` is just equal to the description."* No new syntax and no second file: the comment an
	 * author already writes above an entry IS what `gc_describe` prints and what
	 * `gc_search kind:"preset"` ranks on. The parser lifts it (`GcShapeDescriptionAbove`) so there is
	 * ONE reader of it, rather than an indexer re-opening the file to scan for text the parse threw
	 * away.
	 *
	 * Empty where nothing is written above the entry, or where a blank line separates the two — a
	 * note that belongs to nobody describes nobody. Leading `#`, and the `-`/`=`/`*` a section header
	 * is ruled with, are trimmed; the author's line breaks are kept.
	 */
	FString Description;

	/** The right-hand side for a kind with no body: the number, or the `material()` argument. */
	FString Literal;

	/** Material-slot bake density in texels/metre; zero inherits the bake target. */
	double TexelDensity = 0.0;

	/** The instructions, top to bottom, for `plane()` and `mesh()`. */
	TArray<FGcShapeInstruction> Body;

	/**
	 * Every instruction in this body, in written order, INCLUDING both branches of every
	 * conditional.
	 *
	 * **THIS IS FOR THE CHECKERS, AND BOTH BRANCHES IS THE POINT.** A census that only walked the
	 * branch a condition happens to take would let a misspelled argument inside the other one
	 * through, and it would surface only for whoever's random draw first went the other way —
	 * a defect that reproduces on one seed in five ([[a-producer-with-no-reader-ships-green-defects]]).
	 * What runs is decided in the evaluator; what is LEGAL is a property of the text.
	 */
	void CollectInstructions(TArray<const FGcShapeInstruction*>& Out) const;

	/**
	 * The declared parameters of a `mesh()` or `plane()` — `Brick = mesh(w = 39.0) { … }`.
	 * Empty for `mesh()`, which is every entry written before MC4 and stays legal unchanged.
	 *
	 * **THIS IS WHAT `Literal` USED TO SWALLOW.** Whatever sat inside a mesh's parens was stored and
	 * then read by nobody, so `mesh(anything at all)` parsed and was silently ignored. Parsing it
	 * closes that hole in the same pass.
	 */
	TArray<FGcShapeParameter> Parameters;

	/**
	 * **K1 — A BARE `$Entry` IN THE PARAMETER LIST TAKES THAT ENTRY'S PARAMETERS** (`Docs/GC-SHAPE-DAMAGE-KIT.md` §6,
	 * lead-ruled 2026-11-19): `BrokenPlank = mesh($Dmg.Snap, at_break=0.8) { … $Dmg.Snap() … }`. Each clause as
	 * written, without its `$`, in order. The parse keeps them; the LINK resolves each (an imported entry is only
	 * known there), appends every parameter it takes to `Parameters` with `From` set, and gives every line that calls
	 * or brings in that entry the ones it does not write, as `name=$name`. So after the link every reader of
	 * `Parameters` sees the whole list and nothing else has to know a clause was written. Empty on nearly every entry.
	 */
	TArray<FString> Inherits;

	/**
	 * **G29 item 6a — `$BrokenBoard [declare]`: THE KNOBS, WITHOUT A CALL.** The clauses of `Inherits` written with
	 * `[declare]`: the entry takes that entry's parameters and defaults though no line of its body calls it, and reads them
	 * as `$name`. A plain clause no line calls is still refused (K1 rule 8: every knob accepted and read by nothing).
	 */
	TArray<FString> InheritsDeclared;

	/** K1: the link has expanded `Inherits` — so a second link, or an entry reached twice, does it once. */
	bool bInheritsLinked = false;

	/**
	 * The `name = value` pairs on an IMPORT — `Fixings = shape('/…/Fixings', Thread_R = 0.5)`.
	 * §3.1. Empty for every other kind, and for every import that passes nothing.
	 *
	 * **THEY ARE ARGUMENTS AND NOT PARAMETERS, WHICH IS WHY THEY ARE NOT IN THE ARRAY ABOVE.** A
	 * parameter is DECLARED and carries a default; an import argument is a value handed ACROSS a
	 * file boundary at the one line that names the file. Storing one in the other's array would
	 * make `FindParameter` answer for a thing the caller wrote, which is the retype
	 * ([[one-case-gets-a-predicate-not-a-retype]]) rather than the extra field.
	 *
	 * **WHAT THEY REBIND IS A TOP-LEVEL NUMBER OF THE IMPORTED FILE**, so every reader inside that
	 * file sees the new value — the entry that names it directly and the entry three hops down that
	 * never mentions it. Checked and applied at the LINK: a parse is a property of ONE file's text
	 * and cannot know what the other file declares.
	 */
	TArray<FGcShapeArgument> Arguments;

	/**
	 * **THIS ENTRY WAS HANDED IN BY WHOEVER IMPORTED THIS FILE** — `shape('/…', Steel = $Brass)`.
	 * §3.1. Null on every entry a file actually declares, which is nearly all of them.
	 *
	 * **IT FORWARDS RATHER THAN COPIES, AND FOR A `mesh()` NOTHING ELSE WOULD WORK.** A copied body
	 * would carry the caller's `$names` into a file where they mean nothing — or worse, something
	 * else. Pointing at the entry where it was WRITTEN keeps §3.1's rule intact in both directions:
	 * an imported body reads its own file, and an argument's body reads the caller's.
	 *
	 * **AND THE EVALUATOR NEEDED NO CHANGE FOR IT**, because `Resolve` already hands back the
	 * document an entry lives in and every caller already does `Run.Into(*Owner)` — the machinery
	 * that made `$Pillar.Column` work carries this for free, in the other direction.
	 * ~~*Needed no change*~~ — true of a shape, false of a slot (MB35): `material($Steel)` numbers
	 * its slot in the file whose body runs it, so `GraphCodeShapeSurface::SlotAcross` translates a
	 * part's slots where it crosses into its caller.
	 *
	 * The pointer is the IMPORTING document, which owns this one through its `Imports` map and so
	 * outlives it.
	 */
	const FGcShapeDocument* ForwardTo = nullptr;

	/** The entry in `ForwardTo` this one now means. Empty unless `ForwardTo` is set. */
	FString ForwardName;

	/**
	 * **G28 — A LIST PARAMETER'S VALUE, AS THIS EVALUATION WAS GIVEN IT** (2026-11-19). Runtime only, never parsed or
	 * serialised: set where the call's bindings are substituted into the body (`GcShapeSubstituted`). Each pair is a
	 * parameter whose DEFAULT is a list (`c=[]`, `w=[1, 2]`) and the list text the call passed, or the default. The
	 * body walk writes them into its own lists before the first line, so `$c[1][0]` reads the caller's entry.
	 */
	TArray<TPair<FString, FString>> BoundLists;

	/** True when this parameter's default is a list (`[…]`): G28, a recipe or part that takes a list. */
	static bool IsListDefault(const FString& Default) { return Default.TrimStart().StartsWith(TEXT("[")); }

	/** A declared parameter by name, or null. */
	const FGcShapeParameter* FindParameter(const FString& InName) const;

	/**
	 * The `[ ]` modifiers on the entry's own line — `Name = profile(…) [linear]`.
	 *
	 * On the ENTRY rather than at each use, deliberately: one profile used by three instructions
	 * cannot be smooth in one of them and straight in the others, and a modifier that could say
	 * otherwise would make `$Silhouette` mean different curves in one file.
	 */
	TArray<FString> Modifiers;

	int32 Line = 0;
};

/**
 * Line one. `gcshape 1` is the format word and the grammar version; the rest are defaults.
 *
 * `smooth:` is a CHORD TOLERANCE in centimetres and not a segment count, deliberately: a count does
 * not scale with size, so a kit authored at `segments=16` has over-tessellated bolts and faceted
 * archways.
 */
struct GRAPHCODESHAPE_API FGcShapeHeader
{
	int32 Version = 1;

	/** Default folder for a build line that gives a bare name. Empty means every line needs a path. */
	FString Into;

	/** Chord tolerance, cm. No polygon edge may deviate from the true curve by more than this. */
	double Smooth = 0.5;

	/** The angle above which an edge shades hard. M7 uses it; the parser only carries it. */
	double Shade = 40.0;

	/**
	 * **THE LOD CHAIN'S COARSENING FACTOR — `lods:3`, row MB47, and 0 means no chain at all.**
	 *
	 * It multiplies `Smooth` once per step down, so LOD1 builds at `Smooth * Lods`, LOD2 at
	 * `Smooth * Lods^2`. **The chain is the PROGRAM RUN AGAIN, coarser, and never a decimation** —
	 * a circle re-solved at ten segments is a regular ten-gon at the right radius where a decimated
	 * one is an irregular inscribed subset of the fine one.
	 *
	 * **IT IS A FACTOR ON A TOLERANCE AND NOT A RATIO OF VERTICES, which are not the same number.**
	 * `SegmentsForArc` solves `pi / acos(1 - tol/r)`, square-root in the tolerance, so a factor of 3
	 * sheds about a third of a ring's segments and not two thirds. The author says how much
	 * COARSER; what that costs is measured and reported per LOD rather than promised here.
	 *
	 * **NO COUNT AND NO BUILD-LINE OVERRIDE, both user-ruled 2026-09-17.** One file is one author's
	 * work at one fidelity, and the chain stops when a step stops paying for itself — which is what
	 * makes a flat-faced crate end at LOD0 instead of writing three identical meshes.
	 * `Docs/GC-SHAPE-LOD-DESIGN.md`.
	 */
	double Lods = 0.0;

	/** The default UV policy for an entry that never says `uv()`. M7's. Empty means unstated. */
	FString Uv;

	/** Pins OUR half — grammar, instruction-to-call mapping, defaults. `1` is never written. */
	int32 Gen = 1;

	/** An explicit shade policy may replace inherited normals; an organic default may not. */
	bool bShadeAuthored = false;

	/**
	 * The `#` block above `build { }` — **what the FILE is for**, where an entry's own
	 * `FGcShapeEntry::Description` says what that entry is for. A preset family's one-line answer to
	 * *why would I import this*, lifted by the same reader and under the same rules.
	 */
	FString Description;
};

struct GRAPHCODESHAPE_API FGcShapeDocument
{
	FGcShapeHeader Header;
	TArray<FGcShapeBuildLine> Build;

	/** The `bake()` statements in `build { }`, in the order written. */
	TArray<FGcShapeBakeTarget> Bakes;

	/**
	 * **WAS `build { }` WRITTEN AT ALL — which is a different question from what it held.** An
	 * EMPTY block is the author saying *this file is a library*: it declares entries for other
	 * files to import and asks for no asset of its own. A file with NO block has said nothing, and
	 * that stays the typo it always was ([[a-silent-no-op-is-worse-than-a-refusal]]).
	 *
	 * **THE AUTHORS REACHED FOR THIS SPELLING BEFORE IT MEANT ANYTHING**: nine files under
	 * `Content/Steampunk/Recipes` already write an empty block beside a comment saying
	 * library-only, and the build refused every one of them. The format now carries what they
	 * meant rather than making them build a throwaway mesh to say it.
	 */
	bool bDeclaredBuild = false;

	TArray<FGcShapeEntry> Entries;

	/**
	 * **G28f — A NAMED FORMULA** (2026-11-19, owner-approved): `Wear = formula(x, k=8) { clamp(1 - abs($x) / $k, 0, 1) }`,
	 * read `$Wear(point.x - 40)` anywhere a number or a warp formula is written. It is the TEXT, never a stored number:
	 * the parser writes every read out inline (`GcShapeExpandFormulas`) before anything else sees the entry, so a line
	 * that reads one builds exactly what the same line written out builds, `point.*` included. Kept beside the
	 * entries, not among them: a formula has no value of its own until it is read.
	 */
	struct FFormula
	{
		FString Name;
		/** Its parameters in declared order; a default is empty where none was written (the read must pass it). */
		TArray<FString> Parameters;
		TArray<FString> Defaults;
		FString Body;
		int32 Line = 0;
	};
	TArray<FFormula> Formulas;

	/**
	 * The `.gcshape` files this one imports, by alias — resolved from disk AFTER the parse, so a
	 * bare parse of a file with imports has this empty. `GraphCodeShapeLink` fills it; the evaluator
	 * reads it. Shared pointers because a sub-document is itself a full `FGcShapeDocument` with its
	 * own imports, and two importers of one file share the parse rather than each re-reading it.
	 *
	 * **KEYED BY THE ALIAS, WHICH IS THE `Module` ENTRY'S NAME.** So `Pillar = shape('/…')` puts a
	 * `Module` entry named `Pillar` in `Entries` AND an `Imports["Pillar"]` beside it — the entry is
	 * what the declaration/rebind/order machinery sees, the map is what resolution walks.
	 *
	 * **KEYED EXACTLY, BECAUSE THE ENTRY IT MIRRORS IS.** `Pillar` and `pillar` are two `Module`
	 * entries by `Find`'s own comparison, so they are two imports of two files; a folding map made
	 * them one, and `$pillar.Column` quietly reached into the file the other line named.
	 */
	GraphCodeShapeValue::TNameMap<TSharedPtr<FGcShapeDocument>> Imports;

	/**
	 * The content path this document was loaded FROM — its own address on disk, `/Game/Shapes/…`.
	 * The top document carries the caller's `asset_path`; an import carries the path its `shape('/…')`
	 * named. Empty only for a top document built from text nobody gave an address to.
	 *
	 * **IT IS THE STABLE IDENTITY OF A FILE, and the evaluator's cross-file cycle check turns on it**:
	 * an entry is *the `X` of the file at this path*, so a loop that leaves a file and comes back is a
	 * repeat of `SelfPath + X` however many import aliases the round trip passed through.
	 */
	FString SelfPath;

	/**
	 * WHICH PARAMETERISATION OF THAT FILE THIS IS — `(Thread_R=0.500000)`, sorted, or empty. §3.1.
	 *
	 * **A PATH STOPPED BEING THE WHOLE IDENTITY THE DAY AN IMPORT COULD CARRY ARGUMENTS.** `Big =
	 * shape('/x', R=10)` and `Small = shape('/x', R=2)` are two documents holding two different sets
	 * of numbers, and they read the same file — so `SelfPath` is equal for both and cannot be what
	 * tells an entry of one from the same-named entry of the other.
	 *
	 * **IT IS A SECOND FIELD RATHER THAN A SUFFIX ON `SelfPath`, because the two are asked different
	 * questions.** The digest fold wants the FILE — two instances are one text and must be folded
	 * once — while the evaluator's cache and cycle stack want the INSTANCE. Overloading one string
	 * would have made whichever reader lost the argument silently wrong.
	 */
	FString Instance;

	/** Runtime instruction calls paint the caller's slots, even when the recipe is imported. */
	TSharedPtr<TMap<FString, int32>> InstructionMaterials;

	/**
	 * **THE STABLE IDENTITY OF THIS DOCUMENT, and everything that must tell two of them apart goes
	 * through it** — the evaluator's mesh cache and its cross-file cycle stack. One carrier, so a
	 * new discriminator lands in both at once ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	FString Identity() const { return SelfPath + Instance; }

	/**
	 * The raw `.gcshape` text this document was parsed from — kept so a dependent's digest can FOLD
	 * every file it reaches. A built asset records one program digest, and if that digest ignored the
	 * imports, a change to `Pillar.gcshape` would leave `Face.gcshape`'s asset reading *the program
	 * still describes me* while its geometry had silently moved. `GraphCodeShapeLink` fills this.
	 */
	FString SourceText;

	/**
	 * The entry called `Name`, or null. **DOTTED NAMES REACH ACROSS FILES**: `Pillar.Column` looks
	 * `Pillar` up in `Imports` and asks that document for `Column`, recursively, so `A.B.C` walks
	 * two hops. A dotted head that is not an import returns null — the caller refuses it by name.
	 */
	const FGcShapeEntry* Find(const FString& Name) const;

	/**
	 * Like `Find`, but also hands back the document the entry LIVES IN — `this` for a local name,
	 * the imported sub-document for a dotted one. The evaluator needs it because an imported shape's
	 * body resolves its own `$locals` against its own file, not against whoever imported it.
	 */
	const FGcShapeEntry* Resolve(const FString& Name, const FGcShapeDocument*& OutOwner) const;
};

class GRAPHCODESHAPE_API FGraphCodeShapeParser
{
public:
	/**
	 * Parse a whole file. False leaves `OutRefusal` saying WHAT was found, with the line number.
	 *
	 * Every refusal in `GC-SHAPE-GRAMMAR.md` §7 that is a property of the TEXT is raised here; the
	 * ones that are properties of the shape (an open surface, a count given a profile) belong to the
	 * evaluator, which is the only thing that can see them.
	 */
	static bool Parse(const FString& Text, FGcShapeDocument& OutDocument, FString& OutRefusal);

	/** Read nested value instructions through the same scanner as ordinary bodies. */
	static bool ParseBody(const FString& Text, TArray<FGcShapeInstruction>& Out, FString& OutRefusal);

	/** Import numbers are bound at their declarations, before later derived numbers are read.
	 * The linker still validates argument names, kinds and reassignment after parsing. */
	static bool Parse(const FString& Text, FGcShapeDocument& OutDocument, FString& OutRefusal,
		const GraphCodeShapeValue::FNameBindings& NumberOverrides);

	/**
	 * The half of `Parse`'s argument check that only a LINKED document can answer — §3.1.
	 *
	 * **A CALL'S ARGUMENTS ARE CHECKED AGAINST THE ENTRY IT BRINGS IN, AND FOR `add($Pillar.Column,
	 * height=600)` THAT ENTRY IS IN ANOTHER FILE.** At parse time `Imports` is empty by design — a
	 * parse is a property of ONE file's text — so `Pillar.Column`'s declared parameters are
	 * unknowable and `Parse` DEFERS every dotted target rather than refusing a legal argument.
	 * This runs the same rule once the linker has bound the imports, so `add($Pillar.Column,
	 * ate=…)` is still refused by name instead of being silently dropped. `GraphCodeShapeLink`
	 * calls it; nothing else needs to.
	 */
	static bool CheckImportedCallArguments(const FGcShapeDocument& Document, FString& OutRefusal);

	/**
	 * **K1 — EXPAND EVERY BARE `$Entry` CLAUSE OF THIS DOCUMENT, ONCE ITS IMPORTS ARE BOUND.** `GraphCodeShapeLink`
	 * calls it per document, after that document's imports (whose own clauses are expanded already) and before
	 * `CheckImportedCallArguments`, which then checks a call into an inheriting entry against the whole list.
	 *
	 * For each clause: the entry it names (local or dotted) is resolved, expanded first if it inherits too, and must
	 * be a `mesh()`, `plane()` or `instruction()` that some line of this body calls or brings in. A parameter of it is
	 * TAKEN unless every such line writes it (an identity forward `deep=$deep` is a forward, not a pin); a taken one
	 * is appended to `Parameters` with its default and `From`, unless this list declares the name, which keeps its
	 * own default and records `Overrides`. Every such line then receives `name=$name` for each parameter of that
	 * entry it does not write. Refused, naming the wrapper, the entry and the line: a clause naming nothing or a kind
	 * with no parameters, one no line calls, a cycle, two clauses disagreeing on a default the list does not settle,
	 * a taken default that names something (v1), and a taken name that is a verb's own word on a line it would reach,
	 * shadows an entry of this file, is the reserved `output`, or is set or named by a `warp(with=)` in the body.
	 */
	static bool LinkInherits(FGcShapeDocument& Document, FString& OutRefusal);

	/** The kind word — `mesh`, `plane`, `profile`, `material`. Numbers have none. */
	static const TCHAR* KindWord(EGcShapeEntryKind Kind);
};
