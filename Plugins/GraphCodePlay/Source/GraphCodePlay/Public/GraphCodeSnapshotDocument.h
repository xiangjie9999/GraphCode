// A `.gcp` read back: the document model, and the reader that produces it.
//
// `RUNTIME-DRIVE-DESIGN.md` §17 P1 — *"the `.gcp` reader. It has never had one, and
// `GC-PRODUCT-SPLIT.md` §3 calls it Play's real work."* Everything after it — diff, restore, the
// save/load test, the Blueprint nodes — reads a capture through this.
//
// ---------------------------------------------------------------------------
// THREE RULINGS THIS FILE MAKES, because each closes a question the design left open
// ---------------------------------------------------------------------------
//
// **1. A value stays TEXT, and is never resolved to a UObject here.** §17 P1 says "resolve every
// reference", and there are two readings. Resolving a path to a live object needs a world, and this
// module is the one that has to work in a packaged game with no editor and — during a diff of two
// files captured last week — no matching world at all. So references are resolved WITHIN THE
// DOCUMENT: §5.4 already promises that a reference to an actor inside the snapshot's own selector
// set spells a path matching another entry in the same file, and `FGcpDocument::ReferencedActor`
// is that promise made queryable. Binding to live objects is restore's job (P3), which by
// definition has a world in hand.
//
// **2. Text is the value, VERBATIM, including its spacing.** A capture's numbers are already
// quantised by `precision:`, so re-formatting them here would be a second opinion on a question
// §5.8 already settled, and any normalisation would silently make two files compare equal that do
// not. The reader's own round-trip gate depends on it: re-rendering a parsed document must return
// the input byte for byte, which is the cheapest honest proof that nothing was dropped.
//
// **3. A `.gc` is refused on its first line, not diagnosed later.** §5.1's whole argument for a
// separate extension is that a sidecar carries `asset:` and `sig:` and has a write door, and a
// capture has neither. A reader that limped along on a sidecar would be the beginning of the door
// §18 says Play never gets.
//
// ---------------------------------------------------------------------------
// HOW FAR THE LEVEL `.gc` GRAMMAR IS SHARED — measured 2026-08-14 against `Content/SimBlank/…`
// ---------------------------------------------------------------------------
//
// `GraphCodeActorEntryWriter.h` claims a capture and a level projection write the same object
// grammar and differ on one axis, the baseline. **Checked against a real level sidecar, and it is
// true of the STRUCTURE and false of the LAYOUT** — which matters, because the two failures look
// nothing alike.
//
//   SHARED   `Name = Class(props)`; the five `@name[payload]` directives; the `{ }` component
//            block, present only when there are components; dotted property paths like
//            `Settings.AutoExposureMinBrightness`; and the one-line `Name = Class()` empty form.
//   DIFFERS  A projection goes through `AppendFields` and wraps with TRAILING COMMAS
//            (`FogDensity=0.0436,`), may put a short entry on a single line, uses a SHORT component
//            class token where a resolver proved it unique, and puts no blank line between entries.
//            A capture goes through `OpenCaptureObject`: one property per line, no commas, full
//            class paths always, blank line between entries.
//
// So this reader reads `baseline: none` and **refuses `baseline: archetype` by name** rather than
// mis-reading it — parsed here, a projection's trailing comma would land inside every value, a
// wrong answer that looks like a right one. Projections are Core's `gc_read kind:"level"`.
//
// The empty form was the useful half of that comparison: it appears in both, the writer emits it
// routinely under the default `omits:`, and the first version of this reader rejected it.

#pragma once

#include "CoreMinimal.h"
#include "GraphCodeValueDelta.h"

/** One `Key=Value` line, with the value exactly as it was written. */
struct FGcpProperty
{
	FString Name;

	/**
	 * The authored spelling, untouched — `(X=150.0,Y=200.0,Z=0.0)`, `true`, `null`, `[]`, a path.
	 *
	 * Not parsed into a variant, and that is a decision rather than laziness. A capture is evidence
	 * to be compared, and the comparison that matters is against another capture written by the
	 * same formatter at the same `precision:`. Two texts that differ are a real difference; two
	 * variants that differ might be this reader's rounding.
	 */
	FString Value;
};

/**
 * An `@name[payload]` line — `@label`, `@folder`, `@attachedTo`, `@placement`, `@attach`.
 *
 * **Parsed GENERICALLY, by shape rather than by a list of the five the writer emits today.** All
 * five already share one spelling, and a reader that recognised them by name would silently drop the
 * sixth — which is exactly the failure mode `omits:` exists to prevent elsewhere, arriving through a
 * parser instead. Order is preserved because the writer's order is meaningful: `@attachedTo` sits
 * directly above `@placement` precisely because it says what that placement is relative to.
 */
struct FGcpDirective
{
	/** Without the `@` and without the brackets — `placement`, `attachedTo`. */
	FString Name;
	FString Payload;

	/**
	 * How a CHANGE to this payload is spelled — set by whoever produced it, never inferred.
	 *
	 * **THE ALTERNATIVE WAS A NAME LIST IN THE DIFFER, AND THIS STRUCT'S OWN PARSING RULE ALREADY
	 * REJECTS THAT SHAPE**: directives are read generically, by shape rather than by a list of the
	 * five the writer emits today, precisely so the sixth is not silently dropped. A differ keyed
	 * on `Name == "tags"` would have reintroduced that failure one layer up — the seventh set-valued
	 * directive would arrive spelled as an arrow and nobody would be told.
	 *
	 * A directive PARSED from a file keeps the default, and that is correct rather than a gap: a
	 * `.gcp` states what IS, so there is no change on disk whose spelling could be recovered. The
	 * field matters only between two live `Collect` results, which is the one place a delta is made.
	 */
	EGcpValueDeltaStyle Style = EGcpValueDeltaStyle::Arrow;
};

/** A component, nested under its actor inside the `{ }` that follows the actor's directives. */
struct FGcpComponent
{
	FString Name;
	FString ClassPath;
	TArray<FGcpProperty> Properties;

	/** `@attach[...]` for a non-root component. Empty for the root, whose transform is the actor's. */
	TArray<FGcpDirective> Directives;

	GRAPHCODEPLAY_API const FGcpProperty* FindProperty(const FString& Name) const;
	GRAPHCODEPLAY_API const FGcpDirective* FindDirective(const FString& Name) const;
};

/**
 * What an entry asserts about EXISTENCE — `PLAY-SESSION-DESIGN.md` §4.5.
 *
 * Only ever anything but `Changed` in a delta (`baseline: previous`), where absence already means
 * *unchanged* and therefore cannot also mean *gone*. That collision is the entire reason this exists:
 * under `archetype` absence means at-default, under `previous` it means untouched, and neither can
 * carry a third meaning without one of them becoming unreadable.
 *
 * **`New`/`Gone`, deliberately not `Spawned`/`Destroyed`.** A diff holds two sets of objects and a
 * name present in one of them. It never saw a call, so it cannot name a mechanism: a streaming level
 * load, a deferred spawn finishing registration, or a policy shift all produce an appearance with no
 * `SpawnActor` anywhere. The session log's `Load(spawned=2, destroyed=1)` may use the other words
 * because the restore *acted*; this only watched.
 */
enum class EGcpChange : uint8
{
	/** The only value an ordinary capture ever has. In a delta: present in both, and something differed. */
	Changed,

	/**
	 * `new Name = Class(...)` — absent from the previous state.
	 *
	 * There is nothing to diff it against, so its properties are written **against the archetype**,
	 * like an ordinary capture entry. Without the marker one file would silently hold entries on two
	 * different baselines, which is the one thing `baseline:` exists to prevent.
	 */
	New,

	/** `gone Name` — present before, absent now. A bare name: nothing else is knowable, or needed. */
	Gone,
};

/** One `actors { }` entry. */
struct FGcpActor
{
	/** The internal name — the handle, per `GC-PRODUCT-SPLIT.md` §1 fact 3. Never the label. */
	FString Name;

	/** Empty only when `Change == Gone`, which carries a bare name and no class. */
	FString ClassPath;

	/** Always `Changed` outside a delta; the reader refuses the other two under any other baseline. */
	EGcpChange Change = EGcpChange::Changed;

	TArray<FGcpProperty> Properties;

	/**
	 * `@label`, `@folder`, `@attachedTo`, `@placement` — in the order written.
	 *
	 * Payloads are kept whole rather than split into location/rotation/scale: the writer emits them
	 * through the same `AppendFields` path as everything else, so splitting here would be this file
	 * guessing at a shape `FGraphCodeActorEntryWriter` owns. A caller that wants the three parts
	 * should ask that writer's normaliser, not re-derive them.
	 */
	TArray<FGcpDirective> Directives;

	TArray<FGcpComponent> Components;

	GRAPHCODEPLAY_API const FGcpProperty* FindProperty(const FString& Name) const;
	GRAPHCODEPLAY_API const FGcpComponent* FindComponent(const FString& Name) const;
	GRAPHCODEPLAY_API const FGcpDirective* FindDirective(const FString& Name) const;
};

/** The envelope — §5.2. Every field exists to prevent one specific wrong reading. */
struct FGcpEnvelope
{
	int32 Version = 0;

	/** The EDITOR-world map path, even for a PIE capture (§5.6). */
	FString World;

	/** The PIE instance, empty for an editor-world capture. */
	FString Pie;

	/**
	 * WHICH RUN — the run's START time, identical in every file that run produced. Empty outside one.
	 *
	 * **§5.6 said `pie:` would answer this and it cannot, measured 2026-08-16.** `pie:` is the world
	 * package leaf (`UEDPIE_0_Test`) and the PIE instance counter restarts at zero on every press of
	 * Play, so two captures from two different runs of one map carry the SAME `pie:`. The one pair it
	 * does separate is an editor-world capture from a playing one — which R2 requires to be
	 * comparable. It distinguishes exactly the wrong pair.
	 *
	 * **A TIMESTAMP RATHER THAN AN OPAQUE ID [USER, 2026-08-16]**, because it answers *which run* and
	 * *when was that* in one field, sorts, and is legible to whoever opens the file. The run folder
	 * still carries the session's 8-hex id; aligning those two is a separate pass.
	 *
	 * The point is that a reader can SEE it. ~~Nothing in this product compares two files a caller
	 * names — there is no such verb~~ — **`gc_play mode:"diff"` is that verb as of 2026-08-16, and
	 * this field is what makes its refusal possible rather than what it replaced.** The tool
	 * subtracts a pair the differ accepts and refuses a cross-run one THROUGH `AreComparable`, on
	 * this field; the sentence that sends a caller to read the two files is still the answer for two
	 * runs. So the defect this closed remains the same one — two files that never said they were
	 * unrelated — and the verb that arrived later could only be built because they now do.
	 */
	FString Run;

	int64 Frame = 0;
	double Time = 0.0;

	/**
	 * `span:` — the frames and seconds between the two full records a DELTA covers. §4.5e, P6's
	 * latency row.
	 *
	 * **`at:` says only when the interval ENDED.** A delta inherits the after-capture's envelope
	 * wholesale, so its clock is the later capture's; the earlier one is in the file `from:` names,
	 * which the rolling channel overwrites on the very next wake. Without this field the one thing
	 * `previous.gcp`/`current.gcp` were built to measure — how long a change took — is gone by the
	 * time anybody reads the delta. Written beside `at:` so the start is subtractable in place.
	 *
	 * **It is a PRODUCER'S field and must never be re-derived by subtracting consecutive pings'
	 * `at:`.** `Compute` refuses a non-comparable pair, writes no delta, and the caller rotates
	 * anyway — and `ping_NN` numbering does not skip, because the counter only advances when a delta
	 * is actually written. So a refused wake is invisible to a reader, and the subtraction silently
	 * returns an interval twice as long. This is the mirror of the rule that struck `slice=` from
	 * the `End` line: derivability was the test there, and this fails it.
	 *
	 * **`bHasSpan` exists for the 406 deltas already on disk, not for the producer.** Every delta
	 * `Compute` builds carries a span — both documents are in hand and both state a frame and a
	 * time, so there is no reachable path that produces one without it. What needs to be
	 * representable is ABSENCE, because a file written before this field must keep parsing and must
	 * re-render byte-identical, the same rule `run:` follows.
	 *
	 * **Negative is legal and is information**: it means the two captures were handed over in the
	 * wrong order, so every `new` in the body is really a `gone`. Nothing else in the document says
	 * so.
	 */
	bool bHasSpan = false;
	int64 SpanFrames = 0;
	double SpanSeconds = 0.0;

	/** Echoed back, so a file with no matches reads as "looked and found none" (§5.2). */
	TArray<FString> Selectors;

	int32 FoundActors = 0;
	int32 FoundProperties = 0;

	/** The quantisation ACTUALLY used. Two files are only comparable when it matches (§5.8). */
	double Precision = 0.0;

	/**
	 * What an ABSENT property line means. Three values, and they are not degrees of one thing.
	 *
	 *   `none`       every property that exists is written; absence means dropped by `omits:`.
	 *   `archetype`  absence means equal to the class default — the ordinary capture.
	 *   `previous`   absence means UNCHANGED since the previous wake — a delta (§4.5).
	 *
	 * The third arrived with the rolling state files and is the reason `EGcpChange` exists: it is the
	 * only baseline under which the file must also state existence, because its absence-meaning is
	 * already spoken for.
	 */
	FString Baseline;

	/**
	 * The state this document is a delta AGAINST. Required under `baseline: previous`, refused otherwise.
	 *
	 * A delta with no `from:` is unreadable in the way that matters — every absent line means
	 * *unchanged since something*, and without naming the something the file asserts nothing at all.
	 * The same instinct as §3.6b's `from:continuous`: the fallback is fine, the silence is not.
	 */
	FString From;

	/**
	 * `from_select:` — the `select:` the `from:` file was taken at, written ONLY when it differs
	 * from this document's own.
	 *
	 * **A delta across two scopes is valid only inside their intersection, and this is the half a
	 * reader cannot otherwise get.** `select:` names the after-capture's scope; the before-capture's
	 * lives in a file `from:` names and the rolling channel overwrites on the next wake — the same
	 * argument that made the span a producer's job rather than a reader's subtraction. Without it an
	 * absent entry is unreadable: *unchanged*, or *never in scope on one side*.
	 *
	 * **Absent when the two scopes match**, which is every delta this product had written before the
	 * field existed and every one the rolling channel writes now (both sides are `*`). A default
	 * costs less than a grammar, and it keeps the whole corpus round-tripping unchanged.
	 *
	 * Its own line rather than a token on `from:`, because `from:` carries a FILENAME and filenames
	 * take spaces — appending to it would make one legal name unparseable.
	 */
	FString FromSelect;

	/**
	 * `new:` / `gone:` — how many entries assert an existence change. Both zero outside a delta.
	 *
	 * Deliberately BESIDE `found:` rather than replacing it. `found:` stays a checksum over every
	 * entry in the file, which is what lets a truncated delta fail loudly; these two say how that
	 * total breaks down, so `changed` is derivable and never separately stated. One number that can
	 * disagree with another is a number that eventually will.
	 */
	int32 NewActors = 0;
	int32 GoneActors = 0;

	/** Categories deliberately dropped. Never empty. */
	TArray<FString> Omits;

	/**
	 * `shows:` — named, surfaced past the category rules, but still only when they differ.
	 *
	 * **Read this before believing `omits:`.** A capture can legitimately say `omits: transient` and
	 * still carry `Velocity`: the category was dropped and one member of it was named back in. The
	 * two lines are both true and only make sense together.
	 *
	 * A name here that has no line on some actor means that actor's value was at its default —
	 * information, not a gap.
	 */
	TArray<FString> Shows;

	/**
	 * `always:` — named and written unconditionally, deviating or not.
	 *
	 * The distinction from `Shows` is entirely about what ABSENCE means, which is the one thing a
	 * reader cannot recover from the body. A name here MUST have a line on every object that has the
	 * property; a gap is a defect rather than a resting value.
	 */
	TArray<FString> Always;

	/** `source:` and `ir:`, kept whole; they identify the writer, not the world. */
	FString Source;
	FString Ir;
};

/** A parse failure, with the line that caused it. */
struct FGcpParseError
{
	/** 1-based, so it matches what an editor shows. 0 when the fault is the document as a whole. */
	int32 Line = 0;
	FString Message;

	GRAPHCODEPLAY_API FString ToString() const;
};

/** A whole capture, read back. */
struct FGcpDocument
{
	FGcpEnvelope Envelope;
	TArray<FGcpActor> Actors;

	GRAPHCODEPLAY_API const FGcpActor* FindActor(const FString& Name) const;

	/**
	 * The actor a property value points at, when that actor is in THIS document.
	 *
	 * §5.4's promise from the reading side: a reference inside the capture's own selector set spells
	 * a path that matches another entry in the same file. Null means the reference left the
	 * document — which is a fact about the capture's selectors, not an error.
	 */
	GRAPHCODEPLAY_API const FGcpActor* ReferencedActor(const FGcpProperty& Property) const;

	/**
	 * `found:` against what was actually parsed.
	 *
	 * A capture states its own counts, so the file carries its own checksum for free. The reader
	 * enforces it (see `Parse`), and this exposes the same comparison to a caller holding a document
	 * it did not parse itself.
	 */
	GRAPHCODEPLAY_API void CountContents(int32& OutActors, int32& OutProperties) const;
};

/** §5's reader. Reads a capture; never writes one, and never opens a door onto an asset (§18). */
class FGraphCodeSnapshotReader
{
public:
	/**
	 * Parse Text. Returns false and fills OutErrors when anything is wrong; OutDocument is then
	 * left untouched.
	 *
	 * **Plan-then-apply, like every other GraphCode reader** — the whole document is parsed and
	 * validated into a scratch before a single field of OutDocument is written, so a caller can
	 * never be handed half a capture and a failure. And **every** error is collected rather than the
	 * first: a capture is 165 lines of machine output, and reporting one fault per run turns a
	 * malformed file into a queue of round trips.
	 *
	 * Validated, beyond the grammar: the version line, the required envelope fields, that
	 * `found:` matches what was actually parsed, and that no actor name repeats — the name is the
	 * handle, so two entries sharing one would make `FindActor` ambiguous and a diff meaningless.
	 */
	GRAPHCODEPLAY_API static bool Parse(const FString& Text, FGcpDocument& OutDocument,
		TArray<FGcpParseError>& OutErrors);

	/** Parse the file at Path. A missing or unreadable file is one error, not a crash. */
	GRAPHCODEPLAY_API static bool ParseFile(const FString& Path, FGcpDocument& OutDocument,
		TArray<FGcpParseError>& OutErrors);

	/**
	 * Render a parsed document back to text.
	 *
	 * **Here for the round-trip gate, and deliberately not exposed as a way to author a capture.**
	 * §18: a `.gcp` is evidence, never input to a write. What this buys is the reader's own proof —
	 * `Render(Parse(text)) == text`, byte for byte, is the only cheap check that catches a field the
	 * parser silently dropped. A field nothing reads is invisible to every other kind of spec.
	 */
	GRAPHCODEPLAY_API static FString Render(const FGcpDocument& Document);
};
