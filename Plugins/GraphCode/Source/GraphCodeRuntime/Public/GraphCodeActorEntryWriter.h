// One `actors { }` entry, written the same way by a capture and by a projection.
//
// WORLD-GC-DESIGN.md §4 and §10.1. `.gcp` (a capture of a running or resting world) and a level
// `.gc` (a projection of a saved one) write the same object grammar — `Name = Class(props)`,
// `@placement[…]`, components nested in braces — and differ on exactly one axis, the BASELINE:
//
//   baseline: none       a capture. Every visible property, absolute, quantised to `precision:`,
//                        and wrapped one per line because 167 of them do not fit on a line.
//   baseline: archetype  a projection. Only deviation from the archetype, at full authored
//                        fidelity, laid out the way every other `.gc` block lays out properties.
//
// THE TWO MODES DO NOT SHARE A VALUE ENGINE, and that is the one place this file departs from what
// WORLD-GC-DESIGN §4 predicted when it said "a value that formats correctly in a snapshot formats
// correctly in a sidecar". A capture quantises, because it exists to be diffed against another
// capture and a resting body wobbling in the sixth decimal is the noise `precision:` was invented
// to remove. A projection must NOT quantise: its numbers are authored, a later `gc_apply` writes
// them back, and rounding an actor from X=150.375 to X=150.38 would MOVE it. So what is genuinely
// shared is the framing, the property rule, the sort order, the placement rule and the layout
// rule; the value spelling follows the baseline, and the projection path goes through
// AuthoringCore's codec — the same one every other `.gc` block already speaks.
//
// NOT `FGraphCodeActorWriter`, which is taken. That one applies ActorIR's `components { }` and
// `class_defaults { }` blocks BACK to a Blueprint. This one only ever produces text.

#pragma once

#include "AuthoringPropertyCodec.h"
#include "CoreMinimal.h"

class AActor;
class UActorComponent;
class ULevel;

/** What an absent property line means — and therefore how the entry is laid out (§10.1). */
enum class EGraphCodeActorBaseline : uint8
{
	/** A capture. Every visible property is printed; absent means dropped by `omits:`. */
	None,

	/** A projection. Only deviation from the archetype; absent means equal to it. */
	Archetype,
};

/**
 * The categories the property rule drops, in the order `omits:` names them.
 *
 * A capture accumulates these across the walk and reports what it actually hit. A projection
 * declares them statically from the KIND, because `FGraphCodeEnvelopeExpectation` has to be
 * answerable from the Asset Registry without loading the package.
 */
struct GRAPHCODERUNTIME_API FGraphCodeActorOmissions
{
	bool bTransient = false;
	bool bDeprecated = false;
	bool bEditorOnly = false;
	bool bNonVisible = false;

	/**
	 * Which `Show`/`AlwaysShow` names were actually met during the walk. Not an omission — it rides
	 * here because this struct is the one accumulator that already spans the whole capture, and "did
	 * anything in this world have a property called that" is a whole-capture question.
	 *
	 * Recorded on the MATCH, not on the write, so a `Show` name that existed everywhere and happened
	 * to be at its default everywhere still counts as found. Otherwise a resting world would report
	 * a correct request as a misspelling.
	 */
	TSet<FName> ShownFound;

	/**
	 * Every property name the walk passed, so a name that matched nothing can be answered with the
	 * names that exist instead of just "no".
	 *
	 * **THIS IS THE DISCOVERY PATH, and it exists because there was no other one.** `gc_describe`
	 * cannot answer *what properties does this have, and which are transient*: AuthoringCore's
	 * export policy excludes `CPF_Transient` before describe sees a property
	 * (`AuthoringPropertyCodec.cpp:26`), so `Velocity` is invisible to it by construction. That
	 * made `show:`/`always:` a parameter you could only use if you already knew the answer.
	 *
	 * Populated ONLY when a name was actually requested — a capture that asked for nothing pays
	 * nothing, and a capture that asked pays one `FName` per reflected property, which is cheap
	 * beside the walk that produced it.
	 */
	TSet<FName> NamesSeen;

	/** Never empty: a walk that dropped nothing writes `none` rather than saying nothing. */
	TArray<FString> Tokens() const;
};

struct FGraphCodeActorEntryOptions
{
	EGraphCodeActorBaseline Baseline = EGraphCodeActorBaseline::None;

	/** Capture only. A projection prints authored values at full fidelity; see the file header. */
	double Precision = 0.01;
	int32 Decimals = 2;

	/** Capture only. Turn on every reflected property instead of the visibility rule. */
	bool bAllProperties = false;

	/**
	 * Capture only. Admit these past the CATEGORY rules — but they still have to differ.
	 *
	 * **The middle setting between "the visible set" and "everything" — user-directed 2026-08-14.**
	 * `properties:all` answers "show me all 2,915" when the question was "show me Velocity", and
	 * `Velocity` is transient, so the narrow request had no spelling at all.
	 *
	 * This lifts transient, deprecated, editor-only and non-visible for the named property, and
	 * leaves the deviation rule in place. So it means *"tell me about Velocity when there is
	 * something to tell"* — a resting world writes nothing, which is what makes a diff of fifty
	 * actors readable.
	 */
	TSet<FName> Show;

	/**
	 * Capture only. Write these unconditionally, deviating or not.
	 *
	 * **The second half of the same direction, and it is a genuinely different question** — user
	 * directed 2026-08-14: *"I should be able to decide whether I should show ALL velocity, no
	 * matter default or not."*
	 *
	 *   `Show`        is "surface it when it changed"  — the reading for a diff.
	 *   `AlwaysShow`  is "surface it, full stop"       — the reading for a REPORT.
	 *
	 * The difference matters because absence carries meaning here. Under `Show`, a missing
	 * `Velocity` means *it was at rest*, which is usually the answer you wanted. Under `AlwaysShow`
	 * it cannot go missing at all, so every actor's line is present to be compared and a table can
	 * be built from the file. Collapsing the two would force every caller to accept whichever
	 * reading the writer happened to prefer.
	 *
	 * Beats every rule including the at-default check, and beats `Show` when a name is in both.
	 *
	 * **HOW BOTH SETS MATCH, because the shape of the lookup is the whole answer to "will it catch
	 * `LinearVelocity`":** they are `TSet<FName>` membership tests on `FProperty::GetFName()`, so
	 * the match is on the WHOLE name and never a substring — `Velocity` finds `Velocity` and not
	 * `LinearVelocity`, `AngularVelocity` or `MaxAngularVelocity`. It is also **case-insensitive**,
	 * which is not a decision made here: `FName::operator==` compares `ComparisonIndex` and
	 * `IsEqual` defaults to `ENameCase::IgnoreCase` (`NameTypes.h:526`, `:801`), so `velocity`
	 * matches. Stated because the first version of this documentation claimed the opposite, and a
	 * lookup rule nobody can see from the outside is one people guess at.
	 */
	TSet<FName> AlwaysShow;

	/**
	 * Capture only. Extra `@directive[…]` lines for ONE component — the execution-state seam.
	 *
	 * **A SEAM RATHER THAN CODE HERE, and the module graph is the whole reason**
	 * (`RUNTIME-DRIVE-DESIGN.md` §20.4 item 3, ruled by building it). A blackboard's values and a
	 * Behavior Tree's active branch are read through `AIModule`; this module is the object grammar
	 * that Core (`Editor`) and Play (`Runtime`) BOTH depend on, so an `AIModule` dependency here
	 * would put AI on the level projection, on `describe`, on import and on all three merge tiers —
	 * none of which have a running AI to ask. `GraphCodeMove` could not host it either: it is
	 * `Type: "Editor"` and a `Runtime` capture may not call it.
	 *
	 * So the knowledge lives in `GraphCodePlay` (`FGraphCodeExecutionState`), which already owns the
	 * capture and can take the dependency, and what crosses into the shared grammar is the SHAPE of
	 * a directive line and nothing about what is in it.
	 *
	 * The callee appends whole lines, already indented to `Depth` — `Indent` is public for exactly
	 * that. **A component that has no properties but does have directives is still written**, which
	 * is the case §20.7 turns on: a brain that was never running deviates in nothing and is the one
	 * an empty `@behaviorTree[none]` exists to report.
	 *
	 * **IT RETURNS THE NUMBER OF TOP-LEVEL FIELDS IT WROTE, AND THAT RETURN IS NOT BOOKKEEPING.**
	 * `found:` is a checksum the reader ENFORCES, counting directive FIELDS and never directive
	 * lines — one per `@label`, three per `@placement`. A seam that handed back only text would make
	 * the writer count by re-parsing what it had just formatted, which is a second implementation of
	 * `CountTopLevelFields` living on the other side of the file. So the callee, which knows its own
	 * field count by construction, says it. The first version returned nothing and produced a
	 * capture whose header disagreed with its body by exactly the number of blackboard keys —
	 * refused by the reader, which is the honest failure and still a failure.
	 */
	TFunction<int32(const UActorComponent*, int32 /*Depth*/, TArray<FString>& /*OutLines*/)>
		ComponentDirectives;

	/**
	 * Projection only. Extra `@directive[…]` lines for the ACTOR ITSELF — the product-facet seam.
	 *
	 * **THE SAME SHAPE AS `ComponentDirectives` ABOVE AND FOR THE SAME REASON, one level up.** A
	 * landscape's creation-time grid — `ComponentSizeQuads`, `SubsectionSizeQuads`,
	 * `NumSubsections`, `LandscapeGuid` — carries no `EditAnywhere`, so the reflective walk writes
	 * NONE of it ([[reflective-policy-misses-the-authored-facets]]); and `ALandscape` lives in
	 * `GraphCodeWorld`, which this module may not name. So what crosses the seam is the SHAPE of a
	 * directive line and nothing about landscapes.
	 *
	 * Called AFTER `@placement` (`GC-TERRAIN-DESIGN.md` §7's own order: what the actor is, where it
	 * stands, then what it was made as), and the callee appends whole lines already indented to
	 * `Depth`.
	 *
	 * **IT RETURNS THE NUMBER OF TOP-LEVEL FIELDS IT WROTE**, for the reason `ComponentDirectives`
	 * does: `found:` is a checksum the reader enforces over directive FIELDS, and a seam handing
	 * back only text would make the writer re-parse what it had just formatted.
	 */
	TFunction<int32(const AActor*, int32 /*Depth*/, TArray<FString>& /*OutLines*/)> ActorDirectives;

	/**
	 * Projection only. True for a component the ENGINE created from the actor's own data.
	 *
	 * **THE SECOND HALF OF THE SAME SEAM, and a distinct question from the directive above**: one
	 * adds what the walk cannot see, this drops what the walk sees and should not have. A
	 * `ULandscapeComponent` is not a visualisation component — the rule directly below it — and is
	 * every bit as derived: the engine makes one per grid cell at import, so the SMALLEST legal
	 * landscape projected twenty lines of them and the largest projects hundreds
	 * (MEASURED 2026-08-19, `GC-TERRAIN-DESIGN.md` §12f).
	 *
	 * The loss is declared, never silent — `omits: landscape_grid` on the level.
	 */
	TFunction<bool(const UActorComponent*)> IsDerivedComponent;

	/** Indentation of the actor entry itself. Components sit one level deeper. */
	int32 Depth = 1;

	/**
	 * The World Outliner label, when it is worth stating. Empty writes no `@label` line.
	 *
	 * PASSED IN rather than read here, and that is deliberate: `AActor::GetActorLabel()` is
	 * `#if WITH_EDITOR` (`Actor.h:2733`), and this file is the one three products share — a runtime
	 * capture in GraphCode Play compiles it too (`GC-PRODUCT-SPLIT.md` §1). The editor-only lookup,
	 * and the decision that a label equal to the actor's name is not worth a line, both belong to
	 * the caller. See WORLD-GC-DESIGN.md §13.4 for why the label is projected at all and why it is
	 * NEVER the handle.
	 */
	FString Label;

	/**
	 * The World Outliner folder path, empty for an actor at the root. Empty writes no `@folder`.
	 *
	 * PASSED IN for the same reason `Label` is: `AActor::GetFolderPath()` lives in `ActorEditor.cpp`
	 * behind `WITH_EDITOR`, and this file is the one three products share. Unlike the label there is
	 * no default to suppress against — an actor is at the root or it is not — so the caller only has
	 * to read it.
	 */
	FString Folder;

	/**
	 * Projection only. A proven-unambiguous short token for a component class.
	 *
	 * Rule 11 allows a short name only where a resolver has proved it unique, so this is a COUNT
	 * of every loaded component class by short name, not a guess. A capture passes none and gets
	 * full paths, because it ran against a live world with no such resolver (§10.1).
	 */
	TMap<FString, int32> ComponentShortNameCounts;

	/**
	 * Projection only. Every actor of the level counted by the name a document would give it —
	 * what lets an entry drop a World Partition `_UAID_…` suffix. Row L9.
	 *
	 * **THE SAME MECHANISM AS `ComponentShortNameCounts` DIRECTLY ABOVE, and deliberately so.** Rule
	 * 11's test is *a resolver PROVED it unambiguous*, and this is that test applied to actors: a
	 * base name is written short only where exactly one actor in the level would claim it.
	 *
	 * **EMPTY MEANS FULL NAMES, WHICH IS WHY IT IS SAFE BY DEFAULT.** A caller who does not fill it
	 * — a `.gcp` capture, a spec — proves nothing about uniqueness and gets exactly the names it got
	 * before this row.
	 */
	TMap<FString, int32> ActorDocumentNameCounts;

	/**
	 * Projection only. The `splines { }` name to reference for this component, or empty to inline.
	 *
	 * **THE THIRD SHAPE OF `IsCollisionGeometry`'S PROBLEM, and the first one where the answer is
	 * MOVE rather than DROP.** `USplineComponent::SplineCurves` is `UPROPERTY(EditAnywhere,
	 * Replicated)` (`SplineComponent.h:242`), so `CPF_Edit` admits the whole struct: one 20-point
	 * spline writes **14,055 characters** into an actors block (MEASURED 2026-08-20). Collision
	 * geometry is derived and simply leaves; a spline's curve is AUTHORED, so it has to land
	 * somewhere — and that somewhere is a named file-scope block, on the user's ruling that a
	 * spline should work *"just like volume"* (§12.10).
	 *
	 * **A DELEGATE RATHER THAN A BOOL, because only the caller knows whether its document HAS a
	 * `splines { }` block to point at.** A level does; an Actor Blueprint's `components { }` and a
	 * `.gcp` capture do not, and excluding the field there would delete a curve with nowhere to put
	 * it. Unbound is the safe default and reproduces the previous bytes exactly — which is what
	 * keeps this from deciding the Blueprint question by accident, in the direction of loss.
	 */
	TFunction<FString(const UActorComponent*)> SplineReference;

	/**
	 * Projection only. The upstream region name to reference for this actor's `BrushBuilder`, or
	 * empty to write the builder out as usual.
	 *
	 * **`SplineReference`'S SHAPE ON THE ONE VALUE THAT IS NOT AUTHORED ANYWHERE IN THIS DOCUMENT.**
	 * A spline's curve MOVES because it is large and authored; this one is large and RECOMPUTED, so
	 * it does not move — it leaves, and a name stays behind. `L_HollowVale.gcw` states `Meadow` as
	 * four corners with `[round=800.0]` on two of them; `GraphCodeWorldRegionPrismBuilder` tessellates
	 * those fillets into **seventy** points, and the level document was writing all seventy on one
	 * 2,251-character line (MEASURED 2026-09-07, three such volumes in that one level). Nobody
	 * authored a point of it, which is the user's own second rule from the spline ruling — *"for
	 * engine recompute datas, should not be stored by us"* (2026-08-20, `GraphCodeSplineCurve.h`).
	 *
	 * **`$Name`, THE GRAMMAR'S *a name declared elsewhere in this document set*.** The same character
	 * `splines { }` writes for a curve and the same one the `.gcw` itself writes in `in=$Meadow`, so
	 * this is one vocabulary rather than a third ([[one-concept-two-grammars]]).
	 *
	 * **A DELEGATE FOR `SplineReference`'S REASON, and the knowledge is a PRODUCT'S rather than this
	 * module's.** The `GraphCode.Region:<name>` tag is minted by `GraphCodeWorldGeneratedVolume`,
	 * which is GraphCodeWorld; this module is GraphCodeRuntime and never depends on a product. So
	 * the caller that knows what a region is binds it, and a `.gcp` capture — which has to restore
	 * the world with no `.gcw` in reach — binds nothing and keeps the real builder.
	 *
	 * **ONLY WHERE THE BUILDER STILL DESCRIBES THE SHAPE.** A hand-edited volume's geometry is
	 * carried by `volumes { }` as a `moved(…)` delta whose base is *the `BrushBuilder` this same
	 * document already writes on this actor's line* (`GraphCodeVolumeShape.h`), so replacing that
	 * builder with a reference would leave the delta with no base in the file. The level projection
	 * binds this only for `EShapeSource::BuilderDescribes` and writes the whole shape otherwise.
	 */
	TFunction<FString(const AActor*)> RegionReference;

	/**
	 * Projection only. True for a component field this caller AUTHORS even though reflection calls
	 * it read-only. Unbound drops nothing extra and keeps the previous bytes exactly.
	 *
	 * **PASSED IN FOR THE REASON `SplineReference` IS, AND NOT FOR THE REASON `Label` IS.** The
	 * knowledge lives in `FGraphCodeActorFacets`, which is GraphCode's; this module is
	 * GraphCodeRuntime, which three products share and which depends on AuthoringCore and the engine
	 * and NEVER on GraphCode. So the predicate cannot be looked up here even in principle — the
	 * caller that has a registry binds it, and a `.gcp` capture of a live world binds nothing and is
	 * unchanged ([[gc-play-one-name]]).
	 *
	 * **THE LEVEL READ ARRIVES THROUGH HERE AND NOT THROUGH THE POLICY FORWARDER**, which is worth
	 * saying because the obvious wiring is wrong: `GraphCodeLevelProjection.cpp:630` calls `Write`,
	 * and `Write` builds its own component policy internally, so a predicate bound only on
	 * `FGraphCodeActorProjection::ComponentPolicy` reaches every WRITE path and none of the read.
	 * Both are bound, deliberately, so the two answer alike
	 * ([[a-read-answers-through-the-write-path]]).
	 */
	TFunction<bool(const FProperty*)> IsAuthoredField;

	/**
	 * Projection only. True for an object DECLARED in this document's `objects { }` block, so an
	 * entry holding it writes a reference instead of the object's body.
	 *
	 * **`SplineReference`'S ARGUMENT, ON THE ONE THING THAT CAN BE REACHED FROM TWO ENTRIES.** A
	 * curve belongs to exactly one component, so moving it out is only about size. An owned
	 * sub-object is pointed at by whoever needs it: MEASURED on `L_MyLevel.gc`, `BP_Wall_C_5`
	 * inlines the `BP_Surface` objects it outers and `BP_ItemManager_C_2` and four
	 * `BP_FloorCeiling`s reference the same ones by path. Inlining at every site writes one
	 * object's fields once per pointer and gives the file no single place to edit them.
	 *
	 * **BOUND BY THE LEVEL AND BY NOTHING ELSE.** `kind:external_actor` calls this same writer and
	 * has no `objects { }` block, a `.gcp` capture has no resolver, and an Actor Blueprint's
	 * `components { }` does not come through here at all. Unbound reproduces the previous bytes
	 * exactly, which is what keeps this from deciding those three questions by accident.
	 */
	TFunction<bool(const UObject*)> DeclaredElsewhere;
};

/** One rendered entry, kept apart from its neighbours until the sort has run. */
struct FGraphCodeActorEntry
{
	FString ClassPath;
	FString Name;
	FString Text;
	int32 PropertyCount = 0;
};

class GRAPHCODERUNTIME_API FGraphCodeActorEntryWriter
{
public:
	/**
	 * Render one actor. Returns false only when there is nothing to render.
	 *
	 * Omissions accumulate across a whole walk, so the same struct is passed to every call.
	 * Warnings and Diagnostics may be null when the caller does not report them.
	 */
	static bool Write(const AActor* Actor, const FGraphCodeActorEntryOptions& Options,
		FGraphCodeActorOmissions& InOutOmissions, TArray<FString>* OutWarnings,
		TArray<FAuthoringDiagnostic>* OutDiagnostics, FGraphCodeActorEntry& OutEntry);

	/**
	 * By class path, then by name.
	 *
	 * RUNTIME-DRIVE-DESIGN §5.5.3, the noise source that actually bites: neither `TActorIterator`
	 * nor `ULevel::Actors` promises an order that survives a reload, so an unchanged world can
	 * otherwise produce a completely reordered file.
	 */
	static void SortEntries(TArray<FGraphCodeActorEntry>& Entries);

	/**
	 * One spec's property list, ordered and flattened for comparison against another's.
	 *
	 * Not part of the file format - nothing here reaches a `.gc`. Both write doors that diff actor
	 * specs (GraphCodeActorWriter for a Blueprint's `components { }`, GraphCodeLevelWriter for a
	 * level's `actors { }`) reported their changes through a byte-identical private copy of these
	 * two, until a UNITY REPARTITION put the two files in one translation unit and the duplicate
	 * definitions became a compile error - which is the third time that has happened in this module
	 * and the reason the collision is fixed by hoisting rather than by renaming one of them. Two
	 * identical bodies are two bodies that can drift; one is one. Next to the sort it pairs with.
	 *
	 * NOT `AuthoringCore::RenderPropertyList`, deliberately: that one joins with `, ` because it
	 * produces grammar a reader sees, and this one joins with `,` because it produces a comparison
	 * key inside an error message. Folding them would make an error message's spacing part of the
	 * file format.
	 */
	static void SortFields(TArray<FAuthoringProperty>& Fields);
	static FString FieldsText(const TArray<FAuthoringProperty>& Fields);

	/** Every loaded component class counted by short name, for `ComponentShortNameCounts`. */
	static TMap<FString, int32> ComponentShortNameCounts();

	/**
	 * Every actor of a level counted by the name a document would give it — row L9's resolver.
	 *
	 * COUNTS EVERY ACTOR AND NOT ONLY THE PROJECTED ONES, which is the conservative direction: an
	 * extra claimant only ever stops a chop, and the alternative — asking Core which actors project
	 * — would point this module at the one above it for an answer that can only make the name
	 * shorter.
	 */
	static TMap<FString, int32> ActorDocumentNameCounts(const ULevel* Level);

	/**
	 * The same rule over a list of names somebody else gathered — World Partition row 8d.
	 *
	 * **THE RULE AND ITS INPUT ARE NOT THE SAME THING, and separating them is the whole fix.**
	 * `Level->Actors` is what is IN MEMORY, and on a partitioned map that is a fraction of the map:
	 * the overload above counts a base name as having ONE claimant while a second actor claiming it
	 * sits in an unloaded cell, and the chop then hands two actors one handle. The editor module has
	 * the level's own record and can supply every name (`FGraphCodeActorRoster`); this module cannot
	 * reach it and must not grow a second copy of the counting rule to make up for that
	 * ([[two-carriers-of-one-fact-is-a-race]]).
	 */
	static TMap<FString, int32> ActorDocumentNameCountsFor(TArrayView<const FName> Names);

	/**
	 * The name a `.gc` calls this actor — user-directed 2026-08-19, *"remove the `_UAID_` suffix…
	 * just dont show in gc"*.
	 *
	 * **A PARTITIONED WORLD RE-MINTS EVERY REQUESTED NAME AND THE SUFFIX IS NOT REMOVABLE AT SPAWN**
	 * (`UWorld::SpawnActor`, `LevelActor.cpp:576`): `bNeedGloballyUniqueName` turns `Overworld` into
	 * `Overworld_UAID_50EE3283BF648EF902_1901135432`, which is what stops names colliding across
	 * separate external packages. So this is about the DOCUMENT, not the engine — the level keeps
	 * the suffix and the `.gc` does not show it.
	 *
	 * **CHOPPED ONLY WHERE THE BASE NAME IS PROVED UNIQUE, and that is the load-bearing half.** An
	 * actor's name is the HANDLE — the entry's key, the target of `@attachedTo[Parent=…]`, of
	 * `delete <Name>` and of `RestrictToActor`. Two actors in one partitioned world may share a base
	 * name, so chopping unconditionally would make two entries collide SILENTLY and the document
	 * would stop being applyable ([[identity-needs-a-declared-direction]]). Where the count says
	 * two, both keep their full names and nothing is lost.
	 *
	 * **AND ONLY A GLOBALLY UNIQUE NAME IS TOUCHED.** `FActorSpawnUtils::GetBaseName` strips the
	 * trailing NUMBER as well as the identifier, so applying it to an ordinary `StaticMeshActor_3`
	 * would throw away the `3` that tells two cubes apart. On a globally unique name that number is
	 * the spawn epoch — machine noise like the identifier beside it — so dropping both is right
	 * there and wrong everywhere else.
	 *
	 * **BOTH ENDS MOVE TOGETHER OR NEITHER DOES.** Whatever projects a chopped name has to resolve
	 * one back, or the document reads correctly and refuses to apply
	 * ([[a-read-answers-through-the-write-path]]).
	 */
	static FString ActorDocumentName(const AActor* Actor, const TMap<FString, int32>& Counts);

	/**
	 * The same name, from the actor's NAME alone — for an actor that is not in memory.
	 *
	 * **THE RULE ABOVE NEVER NEEDED THE OBJECT, and until WP row 8b nobody could ask it without
	 * one.** A partitioned level's record names actors in cells nothing has loaded, so anything
	 * reasoning about a document entry from the record alone has a name and no actor. Splitting the
	 * arithmetic out is what stops such a pass from spelling the handle a second way
	 * ([[two-carriers-of-one-fact-is-a-race]]); `ActorDocumentName` is this function plus a validity
	 * check, and the chopping rule finally has specs of its own — including the *two claimants keep
	 * their whole names* branch, which no content in this corpus can produce.
	 */
	static FString ActorDocumentNameFor(FName Name, const TMap<FString, int32>& Counts);

	/**
	 * The `splines { }` handle for one component — `<Actor>.<Component>` — or empty when this grammar
	 * cannot spell it.
	 *
	 * **ONE FUNCTION SO THE BLOCK AND EVERY REFERENCE TO IT CANNOT DISAGREE.** The component's line
	 * says `SplineCurves=$<this>` and the block declares `<this> = points(…)`; computing that name in
	 * a second place is how a document ends up referencing an entry nobody wrote
	 * ([[a-read-answers-through-the-write-path]]).
	 *
	 * **IT SITS HERE, BESIDE `ActorDocumentName`, AND NOT IN THE LEVEL PROJECTOR THAT USED TO OWN IT**
	 * — moved 2026-08-21 for §13.4a. It is the component half of the name whose actor half is already
	 * this class's, and the projector's copy lived in a PRIVATE header, so the one other pass that
	 * needs to spell a handle — World's water reflection, naming the `.gc` entry that now owns a
	 * dragged course — could not reach it without either reimplementing it or promoting the whole
	 * projector. Reimplementing it is the defect the first paragraph names
	 * ([[two-carriers-of-one-fact-is-a-race]]); a capability reachable from one module and needed by
	 * two belongs where both can see it ([[misplaced-capability-costs-a-name]]).
	 *
	 * Empty for a non-spline, for a spline with no points, and for a component whose owner has no
	 * document name — in every one of those cases the curve stays inline and nothing changes.
	 */
	static FString SplineDocumentName(const UActorComponent* Component,
		const TMap<FString, int32>& Counts);

	/**
	 * A component's simple collision primitives — `FKAggregateGeom`, declared as
	 * `collision_geometry` and dropped by every policy that walks a component.
	 *
	 * Here rather than in one policy because there are THREE that must agree: a level's placed
	 * actors, a capture's, and an Actor Blueprint's `components { }`. It is `EditAnywhere`, so the
	 * visibility rule admits it, and a convex element carries its whole vertex list — one
	 * UDynamicMeshComponent in this project wrote 32,875 characters on a single line, 1.4% of the
	 * entire corpus, generated by the mesh builder with nothing authored behind it.
	 */
	static bool IsCollisionGeometry(const FProperty* Property);
	static bool IsSkinnedAssetField(const FProperty* Property);
	static bool IsDeprecatedSkeletalMeshField(const FProperty* Property);

	/**
	 * `ABrush::BrushBuilder` — a volume's SHAPE, and the one property this writer carries against
	 * its own visibility rule.
	 *
	 * **The builder is AUTHORED and the model is DERIVED.** A brush holds both: `BrushBuilder` is
	 * the box somebody typed (for a cube, `X`/`Y`/`Z`), and `Brush` is the `UModel` of polygons the
	 * engine computes from it. GraphCode carries authored state and rebuilds derived bytes, so the
	 * builder belongs in the document and `bsp_geometry` stays omitted forever.
	 *
	 * **Why it needs an exception at all, MEASURED 2026-08-14 rather than inherited.**
	 * `RUNTIME-DRIVE-DESIGN.md` §16.2 says the extent is dropped as `editor_only`, which is true of
	 * the CAPTURE path and is not what happens here: `PlacedActorPolicy` never tests
	 * `CPF_EditorOnly`, and `AuthoringCore` does not either. The property is
	 * `UPROPERTY(VisibleAnywhere, Instanced)`, so what excludes it from a level document is
	 * **`CPF_EditConst`** — the generic *a value no details panel lets you type is not authored*
	 * rule, which is right about almost everything and wrong about this one, because a brush's
	 * extent has its own editor tool instead of a text box. See
	 * [[reflective-policy-misses-the-authored-facets]].
	 *
	 * **Deliberately NOT extended to a capture.** In a packaged game `WITH_EDITORONLY_DATA` is 0 and
	 * the member does not exist, so a `.gcp` that carried it in PIE and could not outside would
	 * break Play's own constraint that both spell the same world identically.
	 *
	 * By OWNER CLASS and member, not by name alone: `BrushBuilder` on something that is not a brush
	 * is somebody else's property and this rule has nothing to say about it.
	 */
	static bool IsAuthoredBrushShape(const FProperty* Property);

	/**
	 * THE CAPTURE'S ADMISSION RULE, as a codec policy — so a RESTORE can reset exactly the set a
	 * capture would have written, and not one property more.
	 *
	 * Exposed 2026-08-14, when `FGraphCodeSnapshotRestore` was built and the first version of it
	 * imported a `.gcp` through `PlacedActorPolicy`. That is the PROJECTION's rule and it is a
	 * different set — it drops component references, which a capture writes — so the import refused
	 * lines the capture had legitimately produced. The two directions must share one rule or the
	 * round trip is not a round trip; this is the same argument that put `PlacedActorPolicy` and
	 * `DenormalisePlacementNames` on this class, arriving from the capture side.
	 *
	 * It matters most for what is NOT in the file. Under `baseline: archetype` an absent line means
	 * *that property was at its default*, so a restore resets it — and the set it resets has to be
	 * the set the capture considered. Reset more and it acts on state the file never showed its
	 * reader, including facets `omits:` declares the text cannot carry.
	 *
	 * `Options` is reconstructible from the envelope a reader already parses: `omits:` and
	 * `properties:all` give `bAllProperties`, and `shows:`/`always:` give the two named sets. The
	 * file states the parameters of its own admission, which is what makes this answerable at all.
	 *
	 * `bIsRootComponent` carries the one rule that is positional rather than per-property: a root's
	 * relative transform is not written as a property because it is already the actor's
	 * `@placement`, so restoring it as one would fight the placement import.
	 */
	static AuthoringCore::FPropertyExportPolicy CapturePolicy(
		const FGraphCodeActorEntryOptions& Options, bool bIsRootComponent);

	/**
	 * A pointer at one of the owner's own components — directly, or as an array of them.
	 *
	 * **Identity, not an authored value**, which is why `PlacedActorPolicy` drops it. A CAPTURE
	 * writes it anyway and is right to: *this actor's `Trigger` is gone* is a real observation and
	 * a diff should show it. A RESTORE must not write it back, and the codec enforces that on its
	 * own — an owned object is spelled `instanced(ClassPath,Identity,(fields))`, so a captured
	 * component PATH cannot be imported into one at all. The refusal is correct and the two
	 * directions genuinely differ here, so `CapturePolicy` is admission for READING and a restore
	 * subtracts this from it.
	 */
	static bool IsComponentReference(const FProperty* Property);

	/**
	 * The short class name when Counts proves it unique in the loaded set, the full path otherwise.
	 *
	 * Rule 11's "a short token only when the resolver proves it unambiguous", with the count as the
	 * proof. A capture has no such resolver and passes an empty map, which falls through to paths.
	 */
	static FString ClassToken(const UClass* Class, const TMap<FString, int32>& Counts);

	// -----------------------------------------------------------------------
	// Shared spellings. Exposed so a capture and a projection cannot drift apart on a number,
	// and so the snapshot's envelope can quantise its own `time` field the same way.
	// -----------------------------------------------------------------------

	/**
	 * The two policies a placed actor projects under. Exposed for the level WRITER, which must
	 * import through the same policy it exported under or the round trip is not a round trip —
	 * the same argument that put `AppendFields` here.
	 *
	 * `Placed` drops component references (an actor's pointers to its own components are identity,
	 * not authored values) and anything the Details panel refuses to show on an INSTANCE —
	 * `CPF_EditConst` or `CPF_DisableEditOnInstance`. `Placement` keeps the root's relative
	 * transform and nothing else, with `bIncludeDefaults` on, so an actor sitting exactly at its
	 * archetype's origin still states it.
	 */
	static AuthoringCore::FPropertyExportPolicy PlacedActorPolicy();
	static AuthoringCore::FPropertyExportPolicy PlacementPolicy();

	/**
	 * What `reset` means for this object, and which fields a component's walk admits.
	 *
	 * **MOVED HERE FROM `FGraphCodeActorProjection` ON 2026-08-14, and a game-target build is what
	 * demanded it.** These two are the only part of that class a capture ever wanted; the rest of it
	 * takes a `UBlueprint` and reaches `UInheritableComponentHandler`, both of which are editor-only
	 * — `FindKey`/`FindBestArchetype` do not exist in a game target at all, so the class cannot
	 * compile into one. Its own header had already drawn this line, calling exactly these
	 * "shared admission/reset rules"; the build only decided which side of it each half was on.
	 *
	 * `FGraphCodeActorProjection` still declares both names, forwarding here, so the sixteen
	 * editor-side call sites did not have to change. One definition, reachable by the name each
	 * caller already knew.
	 *
	 * `BaselineFor` is deliberately NOT `GetArchetype` alone: `DuplicateObject` makes a scratch
	 * template's archetype the live source object, which is right for duplication and wrong for
	 * omission — reset must mean the same authored default in projection, scratch and live writes.
	 */
	static const UObject* BaselineFor(const UObject* Object);
	/**
	 * The component walk's field policy.
	 *
	 * `IsAuthoredField` is an INCLUSION consulted BEFORE the generic rule: true admits a property
	 * the generic rule would drop. Unbound is the default and reproduces the previous bytes exactly,
	 * which is what lets this module stay below the one that owns the registry — GraphCodeRuntime
	 * depends on AuthoringCore and the engine, never on GraphCode, so the predicate is THREADED IN
	 * by the caller that has one rather than looked up here.
	 *
	 * **THE SAME POLICY SERVES THE READ AND THE WRITE**, which is not incidental: fourteen call
	 * sites across `GraphCodeActorProjection`, `GraphCodeActorWriter` and `GraphCodeLevelWriter`
	 * take it, so a field admitted here is admitted in both directions in one change and the two
	 * cannot drift ([[a-read-answers-through-the-write-path]]).
	 */
	static AuthoringCore::FPropertyExportPolicy ComponentPolicy(bool bAttachment,
		TFunction<bool(const FProperty*)> IsAuthoredField = {});

	/**
	 * `RelativeLocation`/`RelativeRotation`/`RelativeScale3D` <-> `Location`/`Rotation`/`Scale`.
	 *
	 * `@placement` renames the storage; a write has to rename it back before importing, exactly as
	 * `actorir/1`'s `@attach[RelativeScale=…]` does. One pair of functions so the two directions
	 * cannot learn different names for the same field.
	 */
	static void NormalisePlacementNames(TArray<FAuthoringProperty>& Fields);
	static void DenormalisePlacementNames(TArray<FAuthoringProperty>& Fields);

	static FString Quantise(double Value, double Precision, int32 Decimals);
	static FString FormatVector(const FVector& Vector, double Precision, int32 Decimals);
	static FString FormatRotator(const FRotator& Rotator, double Precision, int32 Decimals);

	/**
	 * `Prefix` + a property list + `Suffix`, on ONE LINE, however long it turns out to be.
	 *
	 * Lifted out of GraphCodeActorProjection so `components { }` in a Blueprint `.gc` and
	 * `actors { }` in a level `.gc` cannot end up with two spellings of the same shape. It used to
	 * wrap past two fields or a hundred columns and no longer wraps at all, so this is the one
	 * overload every projector wants; see `EFieldLayout` for what that ruling was and why.
	 */
	static void AppendFields(const FString& Prefix, const FString& Suffix,
		const TArray<FAuthoringProperty>& Fields, int32 Depth, TArray<FString>& OutLines);

	/**
	 * The same two spellings, with the CHOICE handed in rather than made.
	 *
	 * **There is no third `Auto` value any more, and its removal is the point.** It used to hold a
	 * width-and-count rule that decided a statement's shape from its own rendered length, which made
	 * the shape of a line a function of its content: one added field, or one longer path, reflowed a
	 * statement between one line and N+2 with no fact having changed. That is a diff hazard in a
	 * format whose claim is a legible diff, so the projectors now emit `Inline` unconditionally and
	 * the rule is gone rather than retuned (user ruling 2026-08-18; `AppendFields` carries the why).
	 *
	 * `Wrapped` survives for the one caller that must reproduce text it READ instead of text it
	 * computed: the Tier A merge, which keeps whatever shape the file already had, because a merge
	 * must not relayout text it did not touch. Every `.gc` committed before the ruling is wrapped,
	 * so that path — and the reader behind it — stays load-bearing.
	 */
	enum class EFieldLayout : uint8
	{
		Inline,
		Wrapped,
	};
	static void AppendFields(const FString& Prefix, const FString& Suffix,
		const TArray<FAuthoringProperty>& Fields, int32 Depth, EFieldLayout Layout,
		TArray<FString>& OutLines);

	static FString Indent(int32 Depth);
};
