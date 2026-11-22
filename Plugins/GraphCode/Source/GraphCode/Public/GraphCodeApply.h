#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "AuthoringPropertyCodec.h"
#include "GraphCodeSchemaWriter.h"
#include "GraphCodeLayout.h"

/**
 * Push one `.gc` block back into the asset it was projected from.
 *
 * The blocks are grammatically independent, so a patch is a block substitution — and what
 * compiles a block into the `.uasset` is not new code. `graph { }` and `functions { }` go to
 * bp_apply one graph at a time, `widgets { }` goes to ui_apply, and user struct/enum schema
 * blocks use the registered identity-preserving writer. A Blueprint `properties { }` block uses
 * the registered declaration writer; other declaration blocks remain explicit refusals.
 *
 * One claim from that section that does NOT survive contact and is not relied on here: that
 * editing `graph { }` cannot break `widgets { }`. True of the file, false of the asset — a graph
 * node reading a removed widget still breaks. The safety comes from bp_apply and ui_apply
 * validating against the LIVE asset, never from block isolation.
 */
enum class EGraphCodeApplyRoute : uint8
{
	/** One or more graphs, each through bp_apply against its own `sig:`. */
	Graphs,
	/** The whole Designer tree, through ui_apply. */
	Widgets,
	/** A block handled by the registered schema/material writer. */
	Schema,
	/** Blueprint declarations. `properties` has a writer; inherited/default grammar does not. */
	Declaration,
	/** Actor Blueprint component templates and inherited/native class-default deviations. */
	Actor,
	/**
	 * A level's placed actors — properties, `@placement` and `@label` on actors that already exist.
	 *
	 * Its own route because the target is a `UWorld` rather than an asset with a generated class,
	 * and because the identity rules are the level's: the handle is the actor's internal name,
	 * omission is not deletion, and create/delete/rename are W5. WORLD-GC-DESIGN.md §13.
	 */
	Level,
	/**
	 * A map's own Level Blueprint — `level_blueprint { }` on kind:level. W6.1.
	 *
	 * Its own route rather than Level, because the two share a `.umap` and nothing else: Level
	 * writes placed actors through the level writer, and this decomposes a nested Blueprint document
	 * into the ordinary per-block writers with an explicit target beside them. It is the one route
	 * whose writers are all somebody else's — what it contributes is the ADDRESS, since `gc_apply`
	 * resolves a target by package path and a `ULevelScriptBlueprint` is a subobject of the map.
	 *
	 * **IT ARRIVED HERE FIVE DAYS AFTER THE WRITER DID, 2026-08-16, which is this table's recurring
	 * defect and the fourth instance of it** — `player_mappable`, `materials` and `sockets` are the
	 * others, each recorded beside its own branch. The write shipped on 2026-08-11 with four green
	 * specs; the registry still called the block read-only and this table had no case for it, so
	 * `gc_describe` advertised a refusal the apply path had stopped making.
	 */
	LevelBlueprint,
	/**
	 * UTimelineTemplate objects — a timeline's length, flags, tracks and curve keys.
	 *
	 * Its own route rather than part of Graphs because a template is not in a graph: the node
	 * carries a name and the state hangs off the Blueprint. The two meet only at that name, which
	 * is also why this block is ranked ahead of `graph` — a graph write that destroys a Timeline
	 * node destroys its template with it (UK2Node_Timeline::DestroyNode), so the template edit has
	 * to land while the node is still there.
	 */
	Timelines,
	/**
	 * A Data Table's `settings { }` and `rows { }`.
	 *
	 * Its own route rather than Schema, even though `settings` is shared vocabulary, because the
	 * target is neither a generated class nor a graph: a row is raw memory in `UDataTable::RowMap`
	 * and the two blocks are written by one writer that has to know the row struct is the same on
	 * both sides. GC-IMPORT-DESIGN.md §2.
	 */
	Table,
	/**
	 * An Enhanced Input asset's `settings`, `triggers`, `modifiers` and `mappings`.
	 *
	 * Its own route because the identity rules are neither the level's nor the table's: a trigger, a
	 * modifier and a mapping have no name and no GUID, so the LIST is the value and a write rebuilds
	 * it wholesale. No `delete` line, no create-by-presence, no rename, and nothing for
	 * `allow_breaking_changes` to gate. GC-INPUT-DESIGN.md §4 and §7.
	 */
	Input,

	/**
	 * A curve asset's `settings { }` and `curves { }`.
	 *
	 * Its own route beside Table rather than folded into it, because the identity rules are
	 * genuinely different: a table's row set is OPEN, so it needs create-by-presence and an
	 * explicit delete; a curve's channel set is CLOSED by its class, so both are refusals.
	 */
	Curve,
	/**
	 * A string table's `settings { }` and `strings { }`. C10.
	 *
	 * Its own route rather than Table's, because nothing about the target is a table's: there is no
	 * row struct to agree on, no raw memory, and the entries are key→string pairs in a
	 * hand-serialized `FStringTable`. What it shares with Table is only the IDENTITY rules —
	 * create by presence, explicit `delete <Key>`, no rename — and those are a paragraph, not a
	 * writer.
	 */
	StringTable,
	/**
	 * A curve table's `settings { }` and `curves { }`. C13.
	 *
	 * Its own route rather than Curve's, and the block name is the same word — which is exactly why
	 * it cannot be folded in. `TryRouteBlock` decides by name alone and would send both here; the
	 * kind-aware pass is what separates them, the same correction a data table's `settings` needs.
	 * The rules differ at both ends: a curve asset's channel set is CLOSED by its class, a table's
	 * row set is OPEN, and a table additionally has ONE SHAPE for every row that the engine
	 * enforces with an assert.
	 */
	CurveTable,

	/** A generic Data Asset instance's complete deviation set in `values { }`. */
	DataAsset,

	/**
	 * A mesh's material slots — `materials { }` on a skeletal or static mesh. Row 11j.
	 *
	 * Its own route rather than Schema or DataAsset, because the target is neither a class nor a
	 * deviation set: a slot is an element of an array the GEOMETRY owns, so the writer can change
	 * what is in a slot and can never change how many there are. Every other property route here
	 * has an open set on at least one axis.
	 */
	Mesh,

	/**
	 * A texture's `settings { }` — the whole of what `kind:texture_2d` writes on its own. C7.
	 *
	 * **ITS OWN ROUTE RATHER THAN Schema, AND THAT IS A CORRECTION TO A DEFECT THIS KIND ALMOST
	 * INHERITED.** `settings` is routable by NAME to Schema, and Schema's valid-target list names
	 * struct, enum, material and material_instance only — so a kind whose settings writer exists but
	 * that is missing from that list is refused with *"not writable for GraphCode kind X"* while its
	 * capability row advertises the block as Writable. Measured on `static_mesh`, 2026-08-13: row 11p's
	 * settings writer is reachable from its specs, which call `FGraphCodeMeshWriter::ApplyBlock`
	 * directly, and NOT through `gc_apply`. Going through a kind-aware route instead of widening
	 * Schema's list is what makes this reachable by construction rather than by remembering.
	 */
	Texture,
	/**
	 * A sound wave's `settings { }` — the whole of what `kind:sound_wave` writes on its own. C13.
	 *
	 * Its own route rather than Schema, for the reason the entry above spells out at length and which
	 * this kind is not going to re-derive: `settings` routes by NAME to Schema, and Schema's
	 * valid-target list would refuse this kind with *"not writable for GraphCode kind X"* while its
	 * capability row advertised the block as Writable. Row 11v's invariant — a block the registry
	 * calls Writable must have a route that accepts its kind — is satisfied here from the first
	 * commit rather than five days late, which is the defect that table has kept repeating.
	 */
	Sound,

	/**
	 * A sound cue's `soundcue { }` graph and the `settings { }` beside it. C13 r3.
	 *
	 * **ITS OWN ROUTE RATHER THAN `Sound`'s, EVEN THOUGH ONE OF ITS TWO BLOCKS ENDS AT THE WAVE'S
	 * WRITER.** What a route contributes beyond dispatch is the SENTENCE it reports with, and the
	 * wave's is *"the sound settings were applied and the wave re-encoded"* — false here, measurably:
	 * a cue holds no samples and re-encodes nothing, which is the one thing this kind's capability
	 * card promises about writing to it. A shared route would have had to branch on the class to tell
	 * the truth, and then it is two routes with one name.
	 *
	 * The graph block would need this route regardless: `soundcue` belongs to exactly one kind, so it
	 * is routable by name the way `strings` is, and nothing else in the language spells it.
	 */
	SoundCue,

	/**
	 * A font's `settings { }` and `typeface { }`, and a font face's `settings { }`. C7.
	 *
	 * **ONE ROUTE FOR TWO KINDS AND TWO BLOCKS, because the target class decides everything and the
	 * block names do not collide.** A `UFont` and a `UFontFace` are different assets — one names
	 * faces, the other IS one — but a write to either goes through the same writer, which picks the
	 * settings policy from the class it was handed. `typeface` on a face is refused there by name
	 * rather than being unroutable here, so the caller hears which asset the block belongs to.
	 *
	 * Its own route rather than Schema for the reason every neighbour above gives: `settings` is
	 * routable by NAME to Schema, whose valid-target list is struct/enum/material/material_instance,
	 * so a font settings payload would be refused two layers above the writer that was waiting for
	 * it — the defect row 11v names and this table keeps re-learning.
	 */
	Font,

	/**
	 * A static mesh's stored source and Interchange pipelines, applied together as one reimport.
	 *
	 * `import { }` and `pipelines { }` are two blocks because GraphCode has no nested top-level
	 * blocks, but they are one write: the former chooses and guards the bytes, the latter states the
	 * settings used to turn those bytes into the mesh. Whole-document apply couples them so editing
	 * both never runs two reimports.
	 */
	Reimport,

	/**
	 * A Blueprint's own Class Settings panel — `settings { }` on kind:blueprint.
	 *
	 * Its own route rather than Actor, even though both end at one UObject's reflected fields,
	 * because the TARGET differs: ActorIR writes the CDO of the generated class, and this writes
	 * the UBlueprint asset itself. Three of its fields also reach the class only through a
	 * recompile, which no other property block in the language has to do. C4.2.
	 */
	ClassSettings,

	/**
	 * The interfaces a Blueprint implements — `interfaces { }`. C4.4.
	 *
	 * Its own route because neither neighbour fits: it is not a property bag, and the graphs it
	 * relocates are not addressed by name the way every graph route addresses them.
	 *
	 * IT WRITES THE LIST AND, SINCE ROW 11q, THE BODIES. An interface function's graph lives in
	 * `FBPInterfaceDescription::Graphs`, a fourth graph array a bare name resolves against nowhere —
	 * so the route hands it to the ordinary graph writer with the INTERFACE named beside the
	 * function, which is the one addressing form that cannot collide with a member function.
	 */
	Interfaces,

	/**
	 * Event dispatchers — `dispatchers { }`. C4.5.
	 *
	 * Its own route rather than Declaration, even though a dispatcher IS a `NewVariables` entry,
	 * because the declaration writer behind `properties { }` creates and removes ordinary variables
	 * and has no signature-graph machinery: a dispatcher there would apply as a deletion it cannot
	 * honestly perform. The second object — a graph in `DelegateSignatureGraphs` whose entry node
	 * carries the parameters — is what this route knows about and that one does not.
	 */
	Dispatchers,

	/**
	 * A block a PRODUCT owns, written by the writer that product registered.
	 *
	 * The one route with no writer in this file. It exists so the route table can ANSWER for a
	 * product block instead of falling through: `TryRouteBlock` knows only Core's names, so before
	 * this a block Move owned was unroutable by construction and the generic *"not a writable
	 * block"* refusal reached it first — a sentence that reads as "you named something that does
	 * not exist" about a block `gc_read` had just produced.
	 */
	Product,
};

/**
 * What an OMITTED entry means in this payload. `GC-VCS-MERGE-DESIGN.md` §4.3, M2.1.
 *
 * The two cases are not a preference, they are two different documents. A hand-authored patch is
 * partial, so omission is genuinely ambiguous and the blocks that can afford to say so refuse it.
 * A complete projection is not ambiguous — and a merged `.gc` is a complete projection, which is
 * what forced this: the merge driver has no way to spell "this actor was deleted on their branch"
 * in a file the projector will regenerate, because `delete X` is an imperative token no projection
 * contains. The mode moves that statement from the file to the CALL, where it belongs.
 */
enum class EGraphCodeApplyMode : uint8
{
	/** The payload is a partial statement. Omission preserves, and removal needs its block's token. */
	Patch,

	/**
	 * The payload is the complete authored state of the block. Omission IS deletion.
	 *
	 * NEVER THE DEFAULT, per §4.3 rule 1: a hand-written partial payload under this mode deletes
	 * everything it forgot. And per rule 2 the authority is per BLOCK and never per asset — `omits:`
	 * means the file is not the whole asset, so "this document is complete" is false by
	 * construction and only "this block is complete" is claimable.
	 */
	Snapshot,
};

struct GRAPHCODE_API FGraphCodeApplyOptions
{
	/** Validate everything and touch nothing. Both IRs have a real preflight; this uses it. */
	bool bDryRun = false;

	/**
	 * The omission rule for this call. UNSET means `Patch` — and unset is not the same as
	 * `Patch` passed explicitly.
	 *
	 * The difference is what gets REPORTED. A block whose omission rule is fixed by its own
	 * identity model cannot honour both modes: a graph deletes by omission and always has, an
	 * input list is the value so truncation is silent, a curve's channel set is closed by its
	 * class. §4.3 rule 5 says such a block must SAY "already snapshot" rather than accept `patch`
	 * and quietly do nothing. If the default counted as an explicit ask, every ordinary graph
	 * apply in the project would carry that notice, so the distinction is load-bearing rather
	 * than fussy: it is what lets the notice mean "you asked for something this block cannot do".
	 */
	TOptional<EGraphCodeApplyMode> Mode;

	/** `Patch` unless the caller said otherwise. */
	EGraphCodeApplyMode EffectiveMode() const { return Mode.Get(EGraphCodeApplyMode::Patch); }

	/**
	 * The document `sig:` of the `.gc` this patch was computed against.
	 *
	 * Empty skips the check. Non-empty and stale is refused: the asset moved after the read
	 * this edit was written against, and applying anyway is how one edit silently eats another.
	 * For `widgets { }` this is the ONLY staleness check there is — the block carries no
	 * signature of its own (§13.6), so the document's stands in for it.
	 */
	FString BaseDocumentSignature;

	/** bp_apply's `replace`: rebuild every node with a fresh GUID and re-run layout. */
	bool bReplaceMode = false;

	/** ui_apply's override. It suppresses the refusal, never the report. */
	bool bAllowBreakingChanges = false;


	/**
	 * `on_failure:"skip"`. One graph that will not validate no longer takes its whole block down.
	 *
	 * **THE ATOMICITY THIS RELAXES WAS ALREADY INCONSISTENT, and that is the argument for the
	 * parameter rather than a wish for one.** `ApplyGraphBlock` preflights only when
	 * `Payload.Num() > 1`, so the SAME edits sent as N single-graph calls write what they can and
	 * sent as one block call write nothing — the behaviour is decided by how the caller batched,
	 * which is not a property anyone chose. This makes the block route able to say what the
	 * single-graph route has always done, instead of leaving it reachable only by giving up the
	 * block.
	 *
	 * **THE DEFAULT DOES NOT MOVE.** All-or-nothing is right when the caller can retry cheaply, and
	 * it is the rule that stops a two-graph payload leaving its first graph written with no way
	 * back. This is opt-in for the caller who has just been told which graph is bad and wants the
	 * other fifteen to land rather than re-sending them verbatim.
	 *
	 * **IT SKIPS, IT NEVER REPAIRS.** A skipped graph is untouched, keeps its old content, and
	 * comes back in `graphs[]` with `mutation_status:"skipped"` and its full error list — the same
	 * entry the refusal would have carried. Nothing here writes a partial graph: `bp_apply` is
	 * still atomic per graph and still rolls its own transaction back.
	 *
	 * When EVERY graph in the payload fails, this changes nothing: there is no survivor to write,
	 * so the block is refused exactly as it would have been. A refusal that wrote nothing and a
	 * skip that wrote nothing are the same answer, and the caller gets the louder one.
	 */
	bool bSkipFailedGraphs = false;

	/**
	 * Project the "before" state from the RUNNING EDITOR instead of syncing a `.gc` first.
	 *
	 * `gc_apply source:"live"`, GC-TOOL-SURFACE-DESIGN.md §7 and T1. This is the parameter that ends
	 * the one exception keeping `bp_apply` and `ui_apply` callable: both retirement notes said
	 * *"gc_apply syncs before it writes and the sync refuses an asset outside /Game or on the
	 * exclusion list, so this is the only write route to such an asset."* That was never a capability
	 * those tools had. It was a gap in this call's own first step.
	 *
	 * **THE TWO PATHS DIFFER IN ONE THING, AND IT IS NOT WHERE THE STATE COMES FROM.** A sync also
	 * projects the live asset — ApplyBlock deliberately reads `Sync.ProjectedText` and never the file
	 * — so both paths hand the writers a fresh projection of the same in-memory object. The
	 * difference is that the sync WRITES that projection to a `.gc` on the way past, and it is the
	 * writing that the eligibility rule governs: `ExcludedFolders` is *"content folders that get no
	 * `.gc`"*. A call that generates no file is not what that setting is about, which is why
	 * `gc_read source:"live"` already opts out of the same check for the same reason. **This is the
	 * exception the setting's own doc names**, and the two say the same thing on purpose — renamed
	 * from `ExcludedPackagePrefixes` 2026-08-24, S1.2.
	 *
	 * So this is subtraction, not a new capability: the same projection, minus the file.
	 *
	 * The asymmetry to design against is that a live projection has no committed `sig:` to be stale
	 * against. `BaseDocumentSignature` is therefore checked against the signature of the live read
	 * taken in THIS call — which is exactly what the saved path does with the sync's own projection,
	 * so the guard is not weakened by having no file behind it.
	 *
	 * The sidecar is still re-projected afterwards when the package is eligible: `RewriteSidecar`
	 * routes through `FGraphCodeProjector::BuildText`, which asks the eligibility question itself and
	 * declines quietly. An eligible asset written live still ends with a current `.gc`; an ineligible
	 * one never had a file to leave stale.
	 */
	bool bLiveSource = false;

	/**
	 * A dry run that would DELETE something first asks who cites it, project-wide. U6, 2026-08-06.
	 *
	 * **DEFAULT TRUE, AND OFF FOR BATCH — that split is the ruling, not an optimisation.** Deletion
	 * in this format is by omission for several blocks, which makes it the one edit a caller can
	 * perform by accident, and the dry run is the last place to notice. So the individual call pays
	 * for the answer.
	 *
	 * The VCS panel's batch apply and the restore planner set this false. Twenty documents would
	 * otherwise cost twenty Asset Registry sweeps to redraw one panel — and that panel already shows
	 * the user both sides of every change, which is the affordance this check substitutes for when
	 * there is no panel.
	 *
	 * Free when nothing is deleted: the search runs only if the plan holds a removal, and only on a
	 * dry run. A real apply has already been dry-run by anyone who wanted this answer.
	 */
	bool bCheckReferencesOnDelete = true;

	/**
	 * The caller is prepared for this document to CREATE the asset it names. C21.
	 *
	 * **IT SAYS WHAT THE CALLER IS PREPARED FOR, NOT WHAT MUST HAPPEN.** Against a path that already
	 * holds an asset it changes nothing and this is an ordinary apply; against an empty one, without
	 * it, the create is PLANNED AND REPORTED and nothing is built.
	 *
	 * The flag exists because the alternative — create by presence, the way a level's `actors { }`
	 * does it — is safe there and is not safe here. An actor label is cheap and local; an asset path
	 * is neither, and `asset:/Game/Blueprints/BP_Romo` would quietly mint a second Blueprint beside
	 * `BP_Room` and report success. Today's "not found" refusal is doing spell-checker duty nobody
	 * designed it for, and this keeps that job while adding the capability.
	 */
	bool bCreate = false;

	/**
	 * Save the package when the write succeeds. **DEFAULT TRUE — A WRITE FINISHES ITSELF.** C24.0.
	 *
	 * User-directed, and it is a ruling about the surface rather than about saving: *"cant you make
	 * them together? ... and if you dont want to save and want apply only, just use a parameter
	 * instead of using two tools."* Two TOOLS is what was forbidden, not two behaviours, so this is
	 * the parameter and `false` is the caller who genuinely wants the asset left dirty.
	 *
	 * A DRY RUN IGNORES IT ENTIRELY — [[dry-run-must-not-write-anything]]. Nothing here is a reason
	 * for the one path that may touch nothing to touch a package.
	 *
	 * `ApplyDocument` sets it false on its per-block calls and saves ONCE at the end. Five blocks
	 * are one edit to one package, and five saves would be five `.uasset` writes, five history
	 * decisions and five chances for a partial one.
	 */
	bool bSave = true;

	/**
	 * Apply THIS text instead of the `.gc` on disk. `ApplyDocument` only. E11.6.
	 *
	 * **THE FILE STOPS BEING THE ONLY WAY IN, AND THAT IS WHAT A REVERT NEEDED.** A restore's desired
	 * document is assembled in memory — the live projection with one block, or several, replaced by
	 * an older version of them — and there are exactly two ways to hand that to a writer: write it
	 * over the working `.gc` first, or pass it. Writing it first is the option this project already
	 * rejected twice: `GC-EDITOR-VCS-DESIGN.md` §6 forbids the panel writing quietly, and a restore
	 * that failed halfway would leave somebody's working tree holding a proposal they never asked
	 * for. The sidecar is still re-projected from the asset AFTER a successful write, by the same
	 * `RewriteSidecar` every other route uses, so the file ends up canonical either way — the
	 * difference is only whether it passes through a state nobody chose.
	 *
	 * **IT IS NOT A WAY PAST THE STALENESS GATES.** The text is parsed and checked exactly as a file
	 * is: its `sig:` must still equal what the asset projects to, it may hold no `conflict { }`, and
	 * a `merged-from:` header is gated on its ours-parent. A caller assembling a desired document
	 * from an OLD version must therefore rebase it onto the live header first, which is
	 * `GC-LOCAL-HISTORY-DESIGN.md` §6's rule stated as a mechanism: old headers are provenance, not
	 * an apply base.
	 */
	TOptional<FString> DocumentTextOverride;

	/**
	 * Graph blocks only: patch exactly this one graph and leave the block's others alone.
	 *
	 * Without it the block-level truncation guard requires the payload to carry EVERY graph in
	 * `graph { }`, because a dropped graph is indistinguishable from a deletion. That rule is
	 * right for a whole-block patch and wrong as the only option — it made changing one event
	 * graph mean echoing back all of them, which was the last thing bp_apply could do that
	 * gc_apply could not, and therefore the last reason to keep two write tools.
	 *
	 * Naming the graph supplies the missing information: the caller has SAID the scope is one
	 * graph, so a payload holding only that graph is complete rather than truncated. The guard
	 * still runs, narrowed to this name — a payload containing anything else is still refused,
	 * and omission-is-deletion still applies to the statements inside the named graph, which is
	 * bp_apply's own rule and is untouched.
	 */
	FString GraphName;

	/**
	 * Did this payload arrive as a whole `.gc` document rather than as an inline block?
	 *
	 * Set by `ApplyDocument` and by nothing else. It exists for ONE decision — how `GraphName`
	 * treats a payload carrying graphs it did not name — and the two sources want opposite answers.
	 *
	 * An inline `ir` is typed by the caller, so a second graph in it is a patch whose scope
	 * disagrees with its own declaration, and refusing it is right. A FILE is the projection of the
	 * whole asset: carrying every graph is its natural shape, not a disagreement. Refusing it made
	 * `graph_name` unreachable from the only payload a reader actually has, so changing one pin
	 * inside one function meant applying the whole block — and the whole-block apply is the
	 * operation that silently damaged BP_Utils and BP_ItemManager (NEXT.md rows 97-99).
	 *
	 * A document payload is NARROWED to the named graph instead. Everything downstream already
	 * works against a one-entry set, because that is exactly what the inline route hands it.
	 */
	bool bPayloadIsWholeDocument = false;

	/**
	 * The variables this DOCUMENT's own `properties` block adds and removes, for the graph blocks
	 * that are written after it.
	 *
	 * Set by `ApplyDocument` and by nothing else, exactly like `bPayloadIsWholeDocument` above: a
	 * block-scoped `gc_apply` carries one block and declares nothing beyond it, so an inline call
	 * leaves both empty and every check downstream is a no-op.
	 *
	 * They exist because the document knows something no single block does — `properties` sorts
	 * ahead of the graph blocks (`BlockApplyRank`) and its writer regenerates the skeleton, so the
	 * WRITE resolves a name the PREVIEW cannot. Measured 2026-08-30 on `BP_Brazier`: a document
	 * adding `Fuel` and reading it in `functions` was refused by `dry_run:true` and applied clean
	 * wet, in the same session. `FBpApplyOptions::DeclaredVariables` carries the pair the one level
	 * further down that consumes it, and states what each half is for.
	 */
	TArray<FString> DocumentAddedVariables;
	TArray<FString> DocumentRemovedVariables;

	/**
	 * After the write succeeds, re-lay-out the target Blueprint's graphs before the ONE save.
	 * Opt-in and default OFF — the standing rule that placed nodes do not move is the default, and
	 * this is the author explicitly asking for movement, so it does not reopen that ruling. The
	 * engine is `FGraphCodeLayout`, the identity-preserving pass `gc_render mode:"layout"` already
	 * ships; positions are absent from `.gc`, so a re-layout never moves `sig:`. It dirties the
	 * `.uasset` and rides the same save the write was already going to do ([[a-write-finishes-itself]]).
	 *
	 * Reach is set by `OrganiseGraphs`:
	 *   empty     — EVERY graph in the Blueprint is tidied, so `gc_apply` becomes a cleanup pass.
	 *   non-empty — only the named graphs are tidied.
	 *
	 * A dry run reports what WOULD move and writes nothing. A non-Blueprint target has no graphs,
	 * so this is a reported no-op there rather than a case to remember.
	 */
	bool bOrganise = false;

	/** The graphs `bOrganise` tidies. Empty means every graph; otherwise exactly these, by name. */
	TArray<FString> OrganiseGraphs;

	/**
	 * `allow_broken:true`. K1 (13 Nov gap report, P245): a Blueprint that compiled before a write and
	 * does not compile after it is NOT saved by default - the write stays in memory, the file on disk
	 * keeps the version that compiled, and the reply is a failure. This keeps the old behaviour (save
	 * it anyway) for an author who means it. An asset that was already broken before the write is
	 * saved either way, so that a broken Blueprint stays fixable (P227).
	 *
	 * **LAST IN THE STRUCT ON PURPOSE, AND A NEW FIELD GOES AFTER IT.** A product module is often rebuilt
	 * alone and run beside the published core (`BUILD-AND-TEST.md`, the routed manifest). A field added
	 * in the middle moves every later one, and that module then reads the wrong option out of a struct
	 * the older core laid out. Added at the end, every existing offset stays where the published core
	 * has it.
	 */
	bool bAllowBroken = false;
};

/** What happened to one graph inside a `graph { }` or `functions { }` patch. */
struct GRAPHCODE_API FGraphCodeApplyGraphOutcome
{
	FString GraphName;
	bool bSuccess = false;
	FString MutationStatus;
	FString CompileStatus;
	FString NewSignature;
	FString Message;

	/** Empty when the payload's graph carried no `sig:`, which means no staleness check ran. */
	FString BaseSignature;

	int32 NodesCreated = 0;
	int32 NodesDeleted = 0;
	int32 NodesModified = 0;

	TArray<FString> ResolutionErrors;
	TArray<FString> UnvalidatedStatements;
	TArray<FString> ConnectionErrors;
	TArray<FString> CompileErrors;
	TArray<FString> CompileWarnings;

	/**
	 * The same compile's messages that provably belong to another graph in this asset — see
	 * `FBpApplyResult::CompileErrorsElsewhere`. `bSuccess` does NOT follow these.
	 */
	TArray<FString> CompileErrorsElsewhere;
	TArray<FString> CompileWarningsElsewhere;

	TArray<FString> Substitutions;

	/** `n_new -> n41`: an alias the payload chose and the reader numbers differently; see `FBpApplyResult::AliasesRenamed`.
	 * The next edit must use the right-hand names (field report D3: computed since the reconciliation, never reported). */
	TArray<FString> AliasesRenamed;

	/** See `FBpApplyResult::OrphansRemoved`: earlier writes' leftovers this write swept out of the graph. */
	int32 OrphansRemoved = 0;

	/** C1a. Valid-but-probably-wrong findings; see `FBpApplyResult::Diagnostics`. Never a refusal. */
	TArray<FString> Diagnostics;

	/**
	 * DRY RUN ONLY: what this write would reach outside the graph it names — see
	 * `FBpApplyResult::CallSitePreview`. Empty on a real write, which reports what it DID.
	 */
	TArray<FString> CallSitePreview;
};

/**
 * What compiling an asset — and writing nothing — found.
 *
 * A compile is the one mutation in this file that carries no payload, which is why it has its own
 * result rather than an empty `FGraphCodeApplyResult`: every field of that one describes a write
 * that did not happen here, and a caller reading `nodes_created:0` off a compile would be reading
 * an answer to a question nobody asked.
 */
struct GRAPHCODE_API FGraphCodeCompileResult
{
	FString PackageName;

	/** "succeeded" or "failed" — the same two tokens a graph outcome reports, for the same reason. */
	FString Status;

	TArray<FString> Errors;
	TArray<FString> Warnings;

	/**
	 * Whether the package is dirty AFTER the compile, asked of the package itself.
	 *
	 * Usually false, and that is not a bug: FBlueprintCompilationManager snapshots the dirty flag
	 * around a compile and restores it, deliberately, so that recompiling an untouched asset does
	 * not make it look edited. A compile that reports clean has still regenerated the class.
	 */
	bool bAssetDirty = false;

	/**
	 * How many orphaned graph nodes the re-save took out of the package. Zero for every healthy
	 * asset, and zero for every kind that is not a Blueprint.
	 *
	 * REPORTED RATHER THAN SILENT because it is the only evidence the repair happened: an orphan is
	 * invisible to every projection, so the sidecar reads identically before and after and `sig:`
	 * cannot move. A caller that is not told a number has no way to distinguish *this asset was
	 * clean* from *the sweep did not run*, which is exactly the confusion that let the damage sit in
	 * `EUW_RoomBuilder` for four days.
	 */
	int32 OrphansRemoved = 0;

	/**
	 * How many of `OrphansRemoved` were `RF_Transient`, and therefore were never in the file at all.
	 *
	 * **SEPARATE BECAUSE THE TWO NUMBERS MEAN OPPOSITE THINGS, and merging them made this tool lie by
	 * two orders of magnitude.** `SavePackage` does not serialize a transient object, so a transient
	 * orphan is not damage: it is scratch a projection left attached to a graph. What `OrphansRemoved`
	 * is documented to be evidence OF — bytes in the package that no projection can see — is
	 * `OrphansRemoved - TransientOrphansRemoved`.
	 *
	 * MEASURED 2026-08-14: `EUW_RoomBuilder` reported 2004 and was carrying EIGHTEEN. The other 1986
	 * were one `MakeBaselineNode` scratch per node read, and the count grew by 1986 for every
	 * additional `gc_read_live` before the repair — so the headline number tracked how often the asset
	 * had been LOOKED at rather than how damaged it was.
	 *
	 * A NON-ZERO VALUE HERE IS NORMAL and needs no action.
	 */
	int32 TransientOrphansRemoved = 0;
};

/**
 * What the three-way merge behind a stale patch produced — M6.3 and M6.4, GC-VCS-MERGE-DESIGN.md
 * §5.9.
 *
 * **A STALE APPLY WAS ALREADY A THREE-WAY COMPARISON; ITS MERGE POLICY WAS *CONFLICT ON
 * EVERYTHING*.** That is the right floor and the wrong ceiling: safe, and it made a caller recompute
 * a whole block whenever somebody else touched anything else in the same asset. The seat now runs
 * `FGcDocumentMerge` first and refuses only when the merge cannot decide — so the refusal is not
 * removed, it is demoted to the fallback §4.10 requires it to stay.
 *
 * PRESENT ONLY WHEN THE PATCH WAS STALE. A patch whose base matches never reaches the merge and an
 * empty `base_sig` never asked for the check, so `bAttempted` false is the ordinary case rather
 * than a failure.
 */
struct GRAPHCODE_API FGraphCodeApplyMerge
{
	/** The patch was stale and the three-way ran. False on every apply that was never stale. */
	bool bAttempted = false;

	/**
	 * The merge came back with no conflicts, and what this apply wrote is `MergedBlock`.
	 *
	 * False beside `bAttempted` true is the refusal: the two sides disagree (`Conflicts`) or no
	 * merge could be attempted at all (`Unavailable`).
	 */
	bool bMerged = false;

	/** The document `sig:` the patch was computed against — the base seat, named by the caller. */
	FString BaseSignature;

	/**
	 * Where the base's BYTES came from: the `.gc` on disk rather than local history.
	 *
	 * **REPORTED BECAUSE THE TWO CARRIERS CAN DISAGREE ABOUT WHAT IS RECOVERABLE, and a reader
	 * looking at a successful merge cannot otherwise tell which one answered.** History archives a
	 * predecessor, so it holds a base only after something has superseded it; a projection nobody
	 * has superseded yet lives only in the file. The commonest stale apply — sync, read, edit,
	 * apply — is exactly that case, and it used to refuse.
	 *
	 * False is the ordinary answer and says history had it, which is also the only one the store's
	 * budget and pruning can take away.
	 */
	bool bBaseFromSidecar = false;

	/**
	 * Whether the base came from the projection this build last wrote, remembered beside the
	 * store rather than in it — the THIRD carrier, and the only one that survives the caller
	 * editing the file it was handed.
	 *
	 * This is the document seat's answer. There ours IS the file, so the sidecar above cannot be
	 * the base by construction, and history never held it because nothing superseded it. Measured
	 * 2026-08-25: five of the walk's seven unreachable bases were at that seat.
	 */
	bool bBaseFromRemembered = false;

	/** What the asset projects right now — the theirs seat. */
	FString TheirsSignature;

	/**
	 * The block as the merge resolved it: the caller's edit with theirs' folded in.
	 *
	 * THIS IS WHAT WAS WRITTEN, and it is reported because the caller sent something else. A patch
	 * that lands as a different text without saying so is the same silent substitution the stale
	 * refusal exists to prevent, arriving from the other direction.
	 */
	FString MergedBlock;

	/**
	 * The whole merged document, on a dry run and on a conflict.
	 *
	 * M6.4 — a dry run used to answer *would this apply* and this makes it answer *here is the
	 * document that would result, and here is what the two of you disagree about*. On a conflict it
	 * is that same text with `conflict { }` constructs in it and the literal `sig:conflicted` on the
	 * header, which is the shape §4.12 already taught every door to read.
	 *
	 * ABSENT ON A CLEAN LIVE APPLY, deliberately. There the merged block was written and the
	 * sidecar re-projected over it, so `NewDocumentSignature` is the honest answer and this would be
	 * a second copy of a document that is already on disk — and a stale one the moment a later
	 * block writes.
	 */
	FString MergedDocument;

	/** One line per disagreement — entity, property, and the three values. §4.7. */
	TArray<FString> Conflicts;

	/**
	 * Decisions the merge made that nobody has to act on, including what it took from theirs.
	 *
	 * The non-blocking severity §4.7 needs to exist at all: without it every merge that folded in
	 * one unrelated edit would read like a merge that needed looking at.
	 */
	TArray<FString> Notes;

	/** The `omits:` tokens no text could be compared over. §4.6 hole 1, stated rather than hidden. */
	TArray<FString> UncomparedFacets;

	/**
	 * Why no merge could be attempted at all, in the caller's words. Empty when one ran.
	 *
	 * The commonest is a base that has aged out of local history, and that is a fault nowhere: the
	 * store is budgeted and pruned. It is one of the concrete reasons the refusal has to survive as
	 * the fallback instead of being replaced by the merge.
	 */
	FString Unavailable;
};

struct GRAPHCODE_API FGraphCodeApplyResult
{
	bool bSuccess = false;
	FString BlockName;
	FString PackageName;
	EGraphCodeApplyRoute Route = EGraphCodeApplyRoute::Graphs;
	FString RouteTool;

	/** Why the call was refused before anything was touched. Empty on a call that ran. */
	TArray<FString> Refusals;

	/**
	 * The three-way merge behind a stale patch — M6.3/M6.4. `bAttempted` false unless it ran.
	 *
	 * ON REFUSALS AS WELL AS ON SUCCESS, and the refusal is the case it exists for: a caller told
	 * only *the asset moved* has to recompute the block blind, while the same caller told which
	 * three lines the two of them disagree about has one edit to make.
	 */
	FGraphCodeApplyMerge Merge;

	FString Message;
	bool bAssetDirty = false;

	TArray<FGraphCodeApplyGraphOutcome> Graphs;

	/** ui_apply's diff, when the route was Widgets. */
	TArray<FString> WidgetsCreated;
	TArray<FString> WidgetsDeleted;
	TArray<FString> WidgetsRenamed;

	/**
	 * The block as the projector spells it, when the route was Widgets.
	 *
	 * ON REFUSAL as well as on success, and the refusal is the case it exists for. A widgets payload
	 * is usually rejected for a spelling the reader would never emit — a property left at its class
	 * default, or state the engine keeps on the child's slot — and no amount of reading the class
	 * tells the author which. Without this they diff their payload against a one-line complaint and
	 * guess; with it they copy.
	 */
	FString CanonicalDocument;
	TArray<FString> RepairedReferences;
	TArray<FString> BrokenReferences;
	TArray<FString> Errors;
	TArray<FString> CompileMessages;
	TArray<FString> Unsupported;

	/**
	 * Whether the ASSET compiles as this call returns — "succeeded", "failed", or empty.
	 *
	 * **THE PER-GRAPH ANSWER IS NOT THIS ANSWER, AND READING IT AS ONE COST A DAY.** `Graphs[]`
	 * carries a compile status each, attributed positive-only: a message that names a node in
	 * another graph moves to that graph's `CompileErrorsElsewhere`, so a payload whose own graphs
	 * are all clean reports twenty-five successes beside a class that does not compile. Measured
	 * 2026-08-30 on `BP_Compartment` — every function returned null and the loudest line in the
	 * reply said each graph was fine.
	 *
	 * **DERIVED FROM THE LAST COMPILE, NEVER THE UNION OF THEM.** A block writing four graphs
	 * compiles the whole asset four times and each one supersedes the last, so a repair that took
	 * the error count 12 -> 0 would report twelve if these were added up — the same defect
	 * `FBpApplyResult::CompileErrors` was split to avoid, arriving one level up. The asset's errors
	 * are the final compile's two halves put back together (`CompileErrors` + `CompileErrorsElsewhere`
	 * of the last outcome that actually compiled).
	 *
	 * EMPTY WHERE NO COMPILE WAS OWED — every kind that is not a Blueprint, and a dry run. Empty is
	 * *nobody asked*, and it is deliberately not a third token: a curve table reply carrying
	 * `compile_status:"not_attempted"` teaches a reader to look for a compiler that does not exist.
	 */
	FString CompileStatus;

	/**
	 * What that compile said about the ASSET, errors and warnings kept apart. Empty when clean.
	 *
	 * Separate from `CompileMessages`, which several non-graph writers fill with their own compile's
	 * output undifferentiated. These two are the graph route's whole-asset log, and `CompileStatus`
	 * follows `AssetCompileErrors` alone.
	 */
	TArray<FString> AssetCompileErrors;
	TArray<FString> AssetCompileWarnings;

	/** Writer changes and schema impact, when the route was Schema. */
	TArray<FString> SchemaChanges;
	/** A product writer's `FGcProductWriteResult::Warnings`: written, and expected not to work as written. */
	TArray<FString> WriteWarnings;
	TArray<FString> References;
	TArray<FString> UnknownRiskReferencers;

	/**
	 * Every graph node left dangling by a declaration this apply removed, with the declaration
	 * it wanted. Populated on refusals and dry runs as well as on success — a refusal is when
	 * this is most worth reading, and after a successful apply it cannot be recomputed.
	 */
	TArray<FGraphCodeOrphanedReference> OrphanedReferences;

	/**
	 * One line per entity this write changed — the result's single per-field change list.
	 *
	 * NOT the Actor route alone, despite the member's name: `ClassSettings`, `Interface`,
	 * `Dispatcher` and `Level` all move their own `Changes` into it, so it reads "updated X" for a
	 * placed actor and "BlueprintCategory = Routed" for a Widget Blueprint's Class Settings. It
	 * reaches the caller as `changes` for that reason, and `routed_to` says which writer wrote it.
	 *
	 * Filled on dry runs, which is the case it earns its keep in: a dry run computes exactly what it
	 * would touch and then touches nothing, so this is the only account of it.
	 */
	TArray<FString> ActorChanges;

	/**
	 * Where a placement directive actually put something — `GC-SPATIAL-AUTHORING-DESIGN.md` §20.12.
	 *
	 * One line per directive echoed with what it RESOLVED to, then the transform. Empty unless the
	 * payload wrote an `@anchor` / `@surface` / `@facing` / `@scale`, so a level in coordinates pays
	 * nothing for it. Filled on dry runs and refusals as well: the resolved contact is the part a
	 * caller cannot infer from their own text, and a dry run is when they most want to see it.
	 */
	TArray<FString> Placements;

	/**
	 * `A and B overlap by 240.0 cm` — entries of this payload that ended up inside one another.
	 *
	 * **§22.9e, USER-RULED: it warns and never refuses.** Overlapping is a legal thing to author — a
	 * porch enters its house — so the apply lands. What nobody authors on purpose is a row of houses
	 * standing in each other, and that is the case this exists to catch before a save makes it the
	 * level. Deepest first, so the number tells a typo from a mistake.
	 *
	 * Filled on dry runs, which is where it is worth the most: nothing has been written yet.
	 */
	TArray<FString> Overlaps;

	/**
	 * Every entity this apply removes BY OMISSION, named. M2.3.
	 *
	 * ON SUCCESS, ON DRY RUNS **AND ON REFUSALS**, which is the whole point and the same rule
	 * `OrphanedReferences` follows: after the apply it cannot be recomputed, and a refusal is when
	 * it is most worth reading — the caller is being told the payload would have deleted these, so
	 * the list has to be in the refusal or the refusal is unactionable.
	 *
	 * ONLY omission-deletions, not every deletion. An explicit `delete X` is already in the
	 * payload the caller wrote and in the route's own change list; repeating it here would bury
	 * the entries that are here precisely because NOTHING in the document names them.
	 */
	TArray<FString> Deleted;

	/**
	 * Every entity this apply removes, however the payload asked — the input to the check below. S7b.
	 *
	 * A SUPERSET OF `Deleted`, and the two answer different questions. `Deleted` is a report, and it
	 * carries omission-deletions alone so that the entries nothing in the document names are not
	 * buried under the ones the caller wrote themselves. This is not a report: it is what the
	 * project-wide check runs over, so an explicit `delete X` and a removed declaration belong in it
	 * — they cost exactly as much as an omitted one.
	 *
	 * Filled on refusals and dry runs as well as on success, like every other list here: after the
	 * apply it cannot be recomputed.
	 */
	TArray<FGraphCodeRemoval> Removals;

	/**
	 * What each entry of `Removals` would cost, asked project-wide before anything happens. U6.
	 *
	 * One line per removed entity: how many citations `gc_search kind:"reference"` found and in
	 * which assets, plus what the search could not read. Populated on DRY RUNS ONLY and only when
	 * `Options::bCheckReferencesOnDelete` is set, which is the point — the answer has to arrive
	 * before the write, not with it.
	 *
	 * Empty next to a non-empty `Removals` means the check was turned OFF (batch), never that the
	 * deletions are free. `GC-SEARCH-DESIGN.md` §8.
	 *
	 * NOTE: the whole-document result carries the populated copy. A per-block list would ask the
	 * caller to assemble the total from a result they may only be seeing because the run refused.
	 */
	TArray<FString> DeletionImpact;

	/**
	 * The mode a block could not honour, and what it does instead. Empty unless a mode was passed
	 * EXPLICITLY and the block's identity model fixes its omission rule — see `Options::Mode`.
	 */
	TArray<FString> ModeNotices;

	/** One line per timeline operation the plan held, when the route is Timelines. */
	TArray<FString> TimelineChanges;

	/**
	 * What a timeline edit costs that survives it: a Timeline node whose template is gone, a
	 * wired pin whose track is being removed or renamed.
	 *
	 * Not folded into OrphanedReferences, which is a removed DECLARATION plus the node that read
	 * it — carried in that shape precisely so a caller can put the declaration back. A dropped
	 * wire has no declaration to restore, so it would arrive there as a struct with five empty
	 * fields, and "no variable was removed" would be indistinguishable from "the writer did not
	 * fill this in".
	 */
	TArray<FString> TimelineOrphans;

	/**
	 * The sidecar was re-projected from the now-dirty asset, and this is its new document `sig:`.
	 *
	 * Written with no `bin:` WHEN THE PACKAGE IS LEFT DIRTY — `save:false` — because its on-disk
	 * bytes are then not what this text was read from, and stamping them would claim a binding that
	 * does not hold. On the ordinary saved path the rewrite happens AFTER the save, so both digests
	 * are stamped and the pair on disk is complete. C24.0.
	 */
	FString NewDocumentSignature;
	bool bSidecarRewritten = false;

	/**
	 * A route wrote something and its sidecar rewrite is waiting for the save. C24.0, seam only.
	 *
	 * **IT IS THE ROUTES' OWN "I WROTE SOMETHING" SIGNAL, CARRIED RATHER THAN RE-DERIVED.** The
	 * wrapper could ask whether the package is dirty instead, and that answer is wrong in the case
	 * that matters: a refusal that wrote nothing against a package somebody ELSE left dirty would
	 * look identical, and this apply would save a stranger's unsaved work and re-project a sidecar
	 * for an asset it did not change.
	 *
	 * False after `ApplyBlock` returns either way — the wrapper has resolved it by then.
	 */
	bool bSidecarRewritePending = false;

	/**
	 * The external actor packages this write dirtied, waiting for the same save. WP §7 row 4a.
	 *
	 * **A MAP WHOSE ACTORS ARE NOT IN IT NEEDS MORE THAN ONE PACKAGE SAVED, and until 2026-08-21 an
	 * apply saved exactly one.** `actors { }` spawning an actor on a partitioned map put it in its
	 * OWN package; saving the `.umap` committed nothing about it, so the actor existed in memory
	 * with a `.gc` on disk describing it and no `.uasset` behind either. That is
	 * `GC-WORLD-PARTITION-AUDIT.md` §2.2's defect, the one rows 1a/1b closed for the `.gcw` and the
	 * `.gcs` — and `GraphCodeExternalPackages.h` says in its own header that it sits there because
	 * it has a second caller. This is that caller.
	 *
	 * **IT IS A SEAM AND NOT A REPORT, for the reason `bSidecarRewritePending` is one:** the route
	 * knows the world and the wrapper owns the save (C24.0), so the answer has to cross between
	 * them. Names rather than packages, because a pointer taken before the write and read after the
	 * save is a pointer held across both.
	 *
	 * Empty on every kind but a level — on an unpartitioned map there is nothing to put in it, which
	 * is what makes the pair a no-op there rather than a case anybody has to remember.
	 */
	TArray<FName> ExternalPackagesToSave;

	/**
	 * A save ran and succeeded, so this package is on disk in the state the apply left it. C24.0.
	 *
	 * **LITERALLY "A SAVE RAN", NOT "THE PACKAGE IS CLEAN"** — `bAssetDirty` is the second fact and
	 * answers that one. Conflating them would let a package nobody saved report `saved:true` because
	 * the apply happened to change nothing, which is a true-sounding answer to a question nobody
	 * asked ([[a-lossy-comparison-reports-no-change]]).
	 *
	 * FALSE ALWAYS CARRIES `SaveSkipped`. There is no silent false.
	 */
	bool bSaved = false;

	/**
	 * Why `bSaved` is false, in the caller's words. Empty exactly when `bSaved` is true. C24.0.
	 *
	 * Three shapes, and the third is the only one that is a problem: the caller passed
	 * `save:false`; nothing was dirty so there was nothing to write; or the save was attempted and
	 * REFUSED, in which case this carries the engine's reason. A read-only file and a checkout that
	 * is not held are the two usual ones, and the apply has already happened in memory — so the
	 * result reports this rather than reporting plain success.
	 */
	FString SaveSkipped;

	/**
	 * What an `organise:true` post-write re-layout moved, one entry per graph it tidied. Empty
	 * unless organise was requested and had a Blueprint graph to lay out. On a dry run it reports
	 * what WOULD move; on a real apply the moves rode the same save the write did. See
	 * `FGraphCodeApplyOptions::bOrganise`.
	 */
	TArray<FGraphCodeLayoutResult> OrganiseResults;

	/**
	 * Why an `organise:true` reach did not fully land — a named graph the Blueprint does not have,
	 * or a target that is not a Blueprint at all. Not a refusal: the write already succeeded, so a
	 * bad organise reach is reported beside it rather than undoing it ([[a-silent-no-op-is-worse-than-a-refusal]]).
	 */
	TArray<FString> OrganiseWarnings;

	/**
	 * A level `actors` write that was REFUSED AFTER it had already changed the live level (gap report
	 * 1113, P262). The level writer checks before it writes and has no rollback, so a read-back that
	 * differs leaves the change live and the map dirty. That was said only as a sentence inside
	 * `Errors`; these carry it as data. `LiveChangedActors` names every actor that may now differ
	 * from the file. Both stay empty on every other route.
	 *
	 * **LAST IN THE STRUCT ON PURPOSE, AND A NEW FIELD GOES AFTER THEM**, for the reason given on
	 * `FGraphCodeApplyOptions::bAllowBroken`: a product module rebuilt alone runs beside the published
	 * core, and a field added in the middle moves every later one under it.
	 */
	bool bLiveChanged = false;
	TArray<FString> LiveChangedActors;
};

/** One block a document apply found changed, and what happened when it was written. */
struct GRAPHCODE_API FGraphCodeDocumentBlockOutcome
{
	FString BlockName;
	bool bChanged = false;
	bool bApplied = false;
	FGraphCodeApplyResult Result;
	FString Refusal;
};

struct GRAPHCODE_API FGraphCodeDocumentApplyResult
{
	bool bSuccess = false;
	FString PackageName;
	FString SidecarPath;
	FString Message;

	/** The `sig:` the file's header states — the projection its edits were computed against. */
	FString StatedSignature;
	/** What the live asset projects to right now. */
	FString AssetSignature;
	/** The digest of the file's body as it stands, which differs from StatedSignature if edited. */
	FString ProposalSignature;

	/**
	 * The file declared `merged-from:`, so it was gated on its ours-parent and not on its own
	 * `sig:` (M2.10). Reported because the two gates refuse for different reasons and a caller
	 * reading only the message cannot otherwise tell which one it met.
	 */
	bool bMergeProposal = false;

	/**
	 * The parent a proposal must still be applied onto — `merged-from:`'s first signature, ours.
	 * Empty for an ordinary document.
	 */
	FString OursParentSignature;

	/**
	 * The three-way merge behind a stale FILE — M6.3/M6.4, the document seat.
	 *
	 * The block seat's twin, and the commoner of the two: read a `.gc`, edit it for two minutes,
	 * and meanwhile somebody saves that asset in the editor. `bAttempted` false unless that
	 * happened. On success the blocks below are the MERGED document's, not the file's, which is why
	 * a block the file never touched can appear here as changed — theirs changed it.
	 */
	FGraphCodeApplyMerge Merge;

	/** Every block the file holds, whether or not it changed. */
	TArray<FGraphCodeDocumentBlockOutcome> Blocks;

	/** Blocks that changed but have no writer. Non-empty means nothing at all was written. */
	TArray<FString> Refusals;

	/**
	 * Every omission-deletion across every block, prefixed with the block that caused it. M2.3/M2.6.
	 *
	 * The whole-file apply threads ONE mode to every block, so under `snapshot` the deletions can
	 * come from several of them at once, and a per-block list would make the caller assemble the
	 * total themselves — from a result they may only be seeing because the run was refused.
	 */
	TArray<FString> Deleted;

	/**
	 * What every removal this document performs would cost, asked project-wide first. U6, S7b.
	 *
	 * One line per removed entity: how many citations `gc_search kind:"reference"` found and in
	 * which assets, plus what the search could not read. DRY RUNS ONLY, and only when
	 * `Options::bCheckReferencesOnDelete` is set — the answer has to arrive before the write.
	 *
	 * LONGER THAN `Deleted` ON PURPOSE. That list is the omission report; this prices every removal
	 * a block performs, including the explicit `delete` tokens and the declarations the schema
	 * writers drop, which `Deleted` deliberately never carried.
	 *
	 * Empty beside a non-empty `Deleted` means the check was turned OFF (batch), never that the
	 * deletions are free. `GC-SEARCH-DESIGN.md` §8.
	 */
	TArray<FString> DeletionImpact;

	/** Blocks that could not honour an explicitly requested mode. See `FGraphCodeApplyResult`. */
	TArray<FString> ModeNotices;

	/**
	 * Every per-entity change across every block, prefixed with the block that made it.
	 *
	 * The document form is how a whole `.gc` is applied, so a level's `actors` block reaches a caller
	 * through here and not through `FGraphCodeApplyResult`. Prefixed because several blocks of one
	 * Blueprint document — `settings`, `interface`, `dispatchers` — all report into this one list.
	 *
	 * Rolled up on refusals and dry runs as well, like `Deleted`: after the write it cannot be
	 * recomputed, and a partial run's earlier blocks really did change what they say they did.
	 */
	TArray<FString> Changes;

	/**
	 * What the compiler said, once, for the whole document. §11.1c.
	 *
	 * **DOCUMENT-LEVEL BECAUSE THE COMPILE IS.** Before this row each block write compiled and each
	 * block outcome carried its own `CompileErrors`/`CompileWarnings`. Now the document compiles its
	 * asset once after the last block, so there is no honest way to attribute a message to a
	 * particular block — the class those errors are about is the one all of them wrote to. Putting
	 * them here says that; splitting them across the outcomes would be a guess wearing the shape of
	 * a fact.
	 *
	 * A single-block apply through `ApplyBlock` is unchanged and still reports on its own result.
	 *
	 * **THIS IS WHERE `deferred` RESOLVES, AND UNTIL 2026-08-30 IT RESOLVED NOWHERE.** Every graph
	 * this route writes reports `compile_status:"deferred"` — correctly, because the scope has not
	 * compiled yet — and the flush below fills these three fields with the answer. The defect was
	 * that nothing serialized them: the reply carried twenty-five deferrals, twenty-five
	 * *"This graph compiles."* sentences and no asset answer at all, so `deferred` meant *nobody
	 * checked* and read as *fine*. Measured on `BP_Compartment`, ~5h.
	 *
	 * **THE ERRORS ARE KEPT APART FROM THE WARNINGS** because `CompileStatus` follows the errors
	 * alone, and the single merged list this replaced could not say which of its lines was which.
	 * A caller who has to grep prose for the word "error" has been handed a log, not a status.
	 */
	FString CompileStatus;
	TArray<FString> AssetCompileErrors;
	TArray<FString> AssetCompileWarnings;

	bool bAssetDirty = false;

	/**
	 * The document's ONE save, and why it did not happen when it did not. C24.0.
	 *
	 * A document apply saves ONCE at the end rather than once per block: five blocks are one edit to
	 * one package, and five saves would be five `.uasset` writes and five history decisions for a
	 * change the caller made as a single act. The per-block results therefore all say
	 * `saved:false, "the document apply saves once, at the end"` — true of each of them — and this
	 * is the field that answers for the call.
	 */
	bool bSaved = false;
	FString SaveSkipped;

	/** What an `organise:true` post-write re-layout moved. See `FGraphCodeApplyResult::OrganiseResults`. */
	TArray<FGraphCodeLayoutResult> OrganiseResults;

	/** Why an `organise:true` reach did not fully land. See `FGraphCodeApplyResult::OrganiseWarnings`. */
	TArray<FString> OrganiseWarnings;

	FString NewDocumentSignature;

	/**
	 * There was no asset at this path and the document is a recipe for one. C21.1.
	 *
	 * True on the ANNOUNCEMENT and on the create itself; `bCreated` is what separates them. A caller
	 * that sees `bWouldCreate` with `bCreated` false has been told the plan and nothing has happened.
	 */
	bool bWouldCreate = false;

	/** The asset was actually built. Implies `bWouldCreate`, and never true on a dry run. */
	bool bCreated = false;

	/** The kind token the document declares, and the constructor argument resolved out of it. */
	FString CreateKind;
	FString CreateConstructorArgument;

	/**
	 * Everything true about the create that the plan's block list cannot carry: a facet the format
	 * omits, a block a fresh asset has no slot for, a read-only block the created asset keeps the
	 * factory's value for.
	 *
	 * **REPORTED ON THE ANNOUNCEMENT, not only after the fact**, because a create is the one write
	 * with no previous version to diff against — there is nothing to compare it to afterwards.
	 */
	TArray<FString> CreateCaveats;

	/**
	 * What the created asset would project back to. C21.3, and only on a DRY RUN.
	 *
	 * The dry run is the request: it is the call that says *show me, do not do it*, and producing
	 * this costs a scratch build. See `FGcGreenfieldPlan::ProjectedText`.
	 */
	FString CreateProjectedText;

	/**
	 * What became of the recipe's `sig:`. `not_applicable` or `ignored`. C21.5, §16.6.
	 *
	 * **A CREATE HAS NOTHING FOR A SIGNATURE TO BE STALE AGAINST, so the staleness question is not
	 * merely unanswerable but meaningless** — and both answers have to be SAID rather than done.
	 *
	 * `not_applicable` rather than `false`, because elsewhere `false` means the document went in
	 * with no staleness check and invites a `gc_sync` that cannot help here. `ignored` rather than
	 * silence, because a `sig:` restored from git belongs to the asset as it was before somebody
	 * deleted it, and a parameter read, understood and quietly not honoured is the same defect as
	 * the `mode`/`omission` collision this file already paid for once.
	 */
	FString CreateSignatureDisposition;

	/**
	 * Advisory notes about the asset as it now stands (gap report 1113, K8): a setup the write
	 * accepted and the game can never use, said when it is written. Never a refusal. Last in the
	 * struct on purpose; a new field goes after it (see `FGraphCodeApplyOptions::bAllowBroken`).
	 */
	TArray<FString> WriteWarnings;
};

class GRAPHCODE_API FGraphCodeApply
{
public:
	/**
	 * Apply the `.gc` file on disk, whatever edited it.
	 *
	 * This is the door the format was actually for. `sed`, a bash diff, an AI, or a person in
	 * a text editor all leave the same artefact behind — a changed file — and none of them
	 * should have to speak a protocol to get it into the asset. So the input is the path and
	 * nothing else, and the diff is computed here from (file on disk, asset as it is now).
	 * Nothing is carried between calls; two identical invocations do the same thing.
	 *
	 * **It is not a compiler, and must not become one.** `omits:` remains load-bearing. ActorIR
	 * writes only its two covered facets, resets omitted managed values to their archetype, and
	 * requires explicit `delete`/`reset` intent for component identity. What lands is a block diff
	 * against the live asset through the same identity-preserving writers ApplyBlock uses. An unchanged
	 * block is not rewritten — it is not even parsed.
	 *
	 * Two staleness questions, and both are answered before anything is written:
	 *
	 *   was the FILE edited?      body digest vs. the header's `sig:`
	 *   did the ASSET move?       the header's `sig:` vs. what the asset projects to NOW
	 *
	 * The second is the race that matters. Read a `.gc`, spend two minutes editing it, and if a
	 * designer saved that asset in the meantime, applying would silently overwrite their work.
	 * A mismatch there is refused, and the remedy named is gc_sync — which will discard the
	 * pending edit, because the save hook regenerates unconditionally (§14.3) and a proposal
	 * that describes an asset nobody has any more was never real.
	 *
	 * The file is read ONCE, up front, into memory. ApplyBlock re-projects the sidecar after a
	 * successful write, so a second read partway through a multi-block apply would find the
	 * remaining edits already overwritten by the projection of the block just applied.
	 */
	static bool ApplyDocument(
		const FString& AssetPath,
		const FGraphCodeApplyOptions& Options,
		FGraphCodeDocumentApplyResult& OutResult,
		FString& OutError);

	/**
	 * `BlockText` is one block, with or without its `name { … }` wrapper. The name is what routes
	 * the call, so it is passed explicitly rather than sniffed out of the text: a payload whose
	 * wrapper disagrees with the block being patched is a mis-aimed patch, not a rename.
	 */
	static bool ApplyBlock(
		const FString& AssetPath,
		const FString& BlockName,
		const FString& BlockText,
		const FGraphCodeApplyOptions& Options,
		FGraphCodeApplyResult& OutResult,
		FString& OutError);

	/**
	 * Compile the asset and report what the compiler said. Writes nothing.
	 *
	 * Every other route here compiles as the LAST step of a write, which answers "did what I just
	 * send survive?" and cannot answer "is this asset broken right now?" — the question you have
	 * after Live Coding, after a parent class changed, or after somebody edited the graph in the
	 * editor. That gap is why `anim compile` and `anim validate_blueprint` outlived four earlier
	 * retirement rows.
	 *
	 * Blueprint-derived kinds only. Anything else is refused by name rather than silently reported
	 * as compiling clean, because "no errors" and "never compiled" are the same sentence to a
	 * caller and only one of them is true.
	 */
	static bool CompileAsset(
		const FString& AssetPath, FGraphCodeCompileResult& OutResult, FString& OutError);

	/**
	 * RE-SERIALIZE the package: load it, compile it if it has a compiler, and save it unchanged.
	 *
	 * **THE ONE REPAIR THE TEXT SURFACE CANNOT SPELL, §11.1c.** A `.gc` describes AUTHORED state, and
	 * a package can be broken in its DERIVED bytes while every authored line is correct — a compiled
	 * class holding a struct reference that no longer resolves, so the asset logs errors on every
	 * load and compiles clean afterwards. Measured on `UW_LightZoneInfo` 2026-08-13.
	 *
	 * That defect is unreachable through `gc_apply`'s ordinary door, and correctly so: re-sending the
	 * identical block is answered *already matches the Blueprint; nothing changed*, because the block
	 * really does match. The repair is not an edit, it is a WRITE — the same bytes, serialized again
	 * by a build that can resolve what the old ones could not.
	 *
	 * **IT CHANGES NO AUTHORED STATE AND THE DIGESTS PROVE IT.** `sig:` is computed from the document
	 * and is unchanged by definition; `bin:` is the package's bytes and is what moves. A caller that
	 * sees `sig` move has found a bug in this function, not a repaired asset.
	 *
	 * The dirty flag is FORCED rather than hoped for: `FBlueprintCompilationManager` snapshots and
	 * restores it around a compile precisely so that recompiling an untouched asset does not dirty
	 * it, which is the right default everywhere except here, where re-writing the bytes is the point.
	 */
	static bool ResaveAsset(
		const FString& AssetPath, FGraphCodeCompileResult& OutResult, FString& OutError);

	/** Which writer a block name routes to, and whether it has one at all. Core's names only. */
	static bool TryRouteBlock(const FString& BlockName, EGraphCodeApplyRoute& OutRoute);

	/**
	 * The names a `properties` body declares, in the spelling a graph statement's target carries.
	 *
	 * `ApplyDocument` asks it twice - once of the asset's projection and once of the proposal - and
	 * the difference is the seeded symbol table a graph block is then checked against. The long
	 * note on the definition says what it does and does not parse, and why backticks come off.
	 */
	static TArray<FString> DeclaredNamesFromPropertiesBody(const FString& Body);

	/**
	 * K7 (13 Nov gap report, P149) — the refusal for an asset path `LoadObject` returned null for, led
	 * by a stable code: `asset_path_invalid:` (not an object path — the Git Bash `C:/…` case),
	 * `asset_not_found:` (nothing there, on disk or in memory) or `asset_load_failed:` (a package is
	 * there and did not load; the log has the reason). Public because its second caller is a spec:
	 * the third cause cannot be reached through `ApplyBlock`, whose projection fails first.
	 */
	static FString DescribeAssetLoadFailure(const FString& AssetPath);

	/**
	 * Take a PROPERTY BAG's directives off `InOutBody` and lower `Mode` into the one the property
	 * codec speaks. C23 §3.
	 *
	 * **PUBLIC BECAUSE FOUR BLOCKS IN FOUR WRITERS ASK THE SAME QUESTION** — `settings`,
	 * `class_defaults`, `values`, `parameters` — and a bag's entries are its properties in every one
	 * of them, so the tokens mean the same thing and four copies is how four spellings start. Each
	 * writer calls this where it parses its body, which is where `components { }` splits its own.
	 *
	 * `reset <Path>` is patch's only way to return one property to its baseline, and is refused
	 * under snapshot where the omissions already are the resets. `delete` and `rename` are refused
	 * by name in both modes: a reflected property cannot be removed, and its name comes from C++ or
	 * from a variable declaration rather than from this block.
	 */
	static bool SplitPropertyBagDirectives(FString& InOutBody, const FString& BlockName,
		EGraphCodeApplyMode Mode, AuthoringCore::FAuthoringImportMode& OutImport, FString& OutError);

	/**
	 * The same question with the KIND known, so a product's registered writer can answer it.
	 *
	 * Core's table is keyed by name alone and a product's writers are keyed by (kind, block), so
	 * the registry fallback is only reachable through this overload. Prefer it wherever the kind is
	 * in hand; the name-only one remains for the callers that genuinely do not have it.
	 */
	static bool TryRouteBlock(
		EGraphCodeAssetKind Kind, const FString& BlockName, EGraphCodeApplyRoute& OutRoute);

	/**
	 * Which (kind, block) pairs the SCHEMA route actually serves — struct, enum, Material,
	 * Material Function and Material Instance, and only certain blocks on each.
	 *
	 * **PUBLIC BECAUSE A ROUTE EXISTING AND A ROUTE ACCEPTING YOU ARE DIFFERENT QUESTIONS, and until
	 * row 11v only the first one was askable.** `settings` is the most shared block name in the
	 * grammar, so the name-only routing table sends every one of them here and each kind that means
	 * something else by it is corrected later, once the asset is loaded and the kind is known. Miss
	 * one of those corrections and the payload arrives at a writer that does not serve the kind — the
	 * registry says `Writable`, `TryRouteBlock` says "yes, Schema", and the caller is still refused.
	 * That is precisely how a mesh's `settings` writer sat unreachable for three days with a green
	 * suite over it: `NothingIsCalledWritableWithoutAWriterToRouteItTo` asked whether a route
	 * existed, and one did.
	 *
	 * So the list is no longer an inline expression inside `ApplyBlockRouted`. The invariant reads it
	 * too, which is what turns "is there a destination" into "does the destination take this".
	 */
	static bool SchemaRouteAcceptsKind(EGraphCodeAssetKind Kind, const FString& BlockName);

	/**
	 * `animgraph { }` on a document nobody has supplied a writer for.
	 *
	 * PUBLIC AND SHARED because the same question is asked on two paths — a block-scoped
	 * `ApplyBlock` and a whole-document apply — and for a while only the first one answered it. The
	 * second has no `TryRouteBlock` case for `animgraph`, so it fell through to the generic
	 * no-writer refusal, which talks about inherited declarations and `defaults`. Both paths refuse
	 * either way; what differed was whether the caller was told why.
	 */
	static bool IsUnwritableAnimGraphBlock(EGraphCodeAssetKind Kind, const FString& BlockName);

	/** The one sentence both refusal sites give, so they cannot drift apart again. */
	static FString AnimGraphReadOnlyRefusal();

	/**
	 * Save one package a write has just dirtied, and say why not when it does not. C24.1.
	 *
	 * **PUBLIC BECAUSE THE INVARIANT IS WIDER THAN `gc_apply`, WHICH IS THE HALF C24.0 LEFT.**
	 * §11.1 rules that no `gc_` verb leaves a package dirty, and an apply is not the only writer:
	 * `gc_render mode:"layout"` moves nodes, `gc_import` builds assets, `gc_versions mode:"restore"`
	 * rewrites blocks. Each of those had its own ending — and three private copies of one decision
	 * is three chances to disagree about what a refused save says.
	 *
	 * **THE EDITOR'S OWN SAVE, NOT A `UPackage::Save`**, so the pre-save hooks, the source-control
	 * state and GraphCode's `HandlePackageSaved` all run. That last one is why a direct save would
	 * be wrong here rather than merely different: it is what stamps the sidecar's digests.
	 *
	 * Returns false with a reason in `OutSkippedReason` rather than throwing it away, because by the
	 * time a save is refused the edit has ALREADY happened in memory. "The edit is real and it is
	 * not on disk" is a third sentence, and it is the one the caller needs.
	 */
	static bool SavePackageAfterWrite(const FString& PackageName, FString& OutSkippedReason);

	/**
	 * The same save, plus the external actor packages this write dirtied. WP §7 row 4a.
	 *
	 * **ONE SAVE AND NOT N, which is the whole reason it is a parameter rather than a loop at the
	 * call site.** `UEditorLoadingAndSavingUtils::SavePackages` takes an array and runs the pipeline
	 * once over it; saving the map and then each actor would be N source-control decisions, N
	 * chances at a partial write, and N `HandlePackageSaved` passes re-projecting one sidecar.
	 *
	 * `AlsoSave` is `GraphCodeExternalPackages::DirtiedByWrite`'s answer, carried on the apply
	 * result. Each name is re-resolved here rather than trusted: a package that has gone away or
	 * come clean between the write and the save is dropped, and `PackageName` still decides whether
	 * this call refuses.
	 *
	 * **AND THE DIRTY CHECK IS NOW ABOUT THE WHOLE SET.** With the map's own package clean and an
	 * actor's dirty — a pure external-actor edit, which is the common case on a partitioned map —
	 * the single-package form answered *nothing was dirty* and saved nothing at all.
	 */
	static bool SavePackageAfterWrite(
		const FString& PackageName, TConstArrayView<FName> AlsoSave, FString& OutSkippedReason);
};
