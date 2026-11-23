#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GraphCodeCommandlets.generated.h"

/** Regenerate all eligible .gc files. Pass -Prune to delete orphan sidecars. */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcRegenerateCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcRegenerateCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/** Fail non-zero when any eligible .gc file is absent, stale, invalid, or orphaned. */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcCheckCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcCheckCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * Write down the alias identity behind every statement number, project-wide. The one-time
 * migration onto minted aliases. `-Asset=/Game/Path/Name` for one asset, `-DryRun` to count only.
 *
 * Alias numbers used to be traversal POSITIONS, so deleting `n2` renamed n3 to n2, n4 to n3, and
 * every reference to them - one node removed rewrote most of a graph's lines and `git blame` on a
 * `.gc` body was worthless. A number is now minted once and stored in the node's own NodeGuid
 * (AuthoringAlias.h), and this is what stores it for graphs that predate the change.
 *
 * IT MODIFIES AND SAVES `.uasset` FILES - that is the whole job, since NodeGuid is where the
 * number lives. Every write path already stamps what it touches, so this is only needed for the
 * migration and for graphs authored entirely in the editor. Idempotent: a second run reports 0.
 *
 * No pin, no wire, no literal and no property is touched, and for almost every node a NodeGuid is
 * identity rather than content - the projected `.gc` text comes back byte-identical apart from an
 * envelope `bin:` that moved because the package was re-saved.
 *
 * ONE NODE CLASS IS NOT PURELY IDENTITY, and it is worth knowing before running this: a CUSTOM
 * EVENT's NodeGuid IS its function GUID (`BlueprintEditorUtils.cpp:9626` returns `EventNode->
 * NodeGuid` for one, where a real function returns its graph's `GraphGuid`). So stamping a custom
 * event moves the GUID that every `CreateDelegate` and `CallFunction` bound to it has cached.
 * `FMemberReference::ResolveMember` re-resolves by name and re-caches, so a reference in an asset
 * this run loads is repaired and saved with it; the project-wide run of 2026-08-03 moved three, all
 * self-referential, all repaired, and `-run=CompileAllBlueprints` afterwards was 0 errors and
 * 0 warnings over the whole project. A reference held by an asset the run does NOT load keeps the
 * stale GUID until it is next loaded, and is likewise repaired by name - the residual risk is a
 * custom event renamed in the window between, where neither handle resolves.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcStampCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcStampCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * How many lines does a ONE-NODE change move? `-Asset=/Game/Path/Name`, `-Limit=N`, `-Verbose`.
 *
 * GC-VCS-MERGE-DESIGN.md M0.3, and the doc calls it "the number that says whether the rest is
 * worth building". A merge driver is only worth having if a small semantic change produces a small
 * textual one. If deleting a node reflows its whole block, two branches touching one graph collide
 * on lines neither of them meant to change, every merge conflicts spuriously, and no driver rescues
 * that - the fix would have to be in the emitter instead.
 *
 * DELETION, NOT INSERTION, and that is the sharper test rather than the convenient one. Inserting
 * needs a node class and pin wiring, which varies per graph; deleting needs neither. More to the
 * point, deletion is the case that USED to reflow: alias numbers were traversal positions until
 * 2026-07-30, so removing `n2` renamed n3 to n2, n4 to n3 and every reference to them. That was
 * fixed for `git blame` and has never been measured for merge, which is exactly the gap this fills.
 *
 * IT NEVER SAVES. The node is destroyed in memory, the projection is taken from the dirty object -
 * the same path `gc_read source:"live"` uses - and the process exits without writing a package.
 * Nothing touches disk, so a run leaves the corpus byte-identical.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcDiffProbeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcDiffProbeCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * Push one asset's edited `.gc` back into the `.uasset`. `-Asset=/Game/Path/Name`, `-DryRun`.
 *
 * The point of having this at all is that nothing about editing a `.gc` should require MCP, an
 * AI, or the editor UI. Whatever changed the text - `sed`, a patch, a person - the way back in
 * is one command with one argument, and it either lands or it prints why and leaves the asset
 * exactly as it was.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcApplyCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcApplyCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * The git merge driver for `.gc` files. GC-VCS-MERGE-DESIGN.md M1.7-M1.9 and M1.11.
 *
 *   -run=GraphCode.GcMerge -Base=%O -Ours=%A -Theirs=%B
 *
 * Git's own convention: the RESULT is written over `-Ours`, and the exit code is the only thing
 * that decides whether the path stays unmerged. Exit 0 means clean and committable; anything else
 * leaves the conflict in the index, which is §2.3 - a conflict encoded only in the file is a
 * conflict that can be pushed.
 *
 * TEXT ONLY. Settled item 1: it merges text, recomputes `sig:` over the merged body, states what it
 * merged from, and stops. The engine requirement belongs to the apply, where an engine exists. It
 * is a commandlet because that is how this module is reachable, not because it needs the editor -
 * and that costs real startup time per invoked file, which is the price of not having a standalone
 * binary yet.
 *
 * THE GOLDEN RULE (§3.6) IS THE POINT OF THE WRAPPER, and it is implemented with one deliberate
 * deviation. The design says "on any internal failure keep ours byte-for-byte, prepend one conflict
 * comment, exit 1". Prepending anything to a `.gc` breaks its header - the first line must begin
 * with `gc` - so a prepend would make the file unparseable, which is the exact failure the rule
 * exists to prevent. So: ours byte-for-byte, NOTHING prepended, the reason logged, exit 1. Git
 * still marks the path unmerged, which is what stops the commit.
 *
 * `-Ours` is copied to `<file>.gc.ours.orig` before anything is written (§4.10), so no state this
 * touches is ever unrecoverable by hand.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcMergeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcMergeCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * Read and resolve the conflicts in one `.gc`. GC-VCS-MERGE-DESIGN.md M1b.8.
 *
 *   -run=GraphCode.GcResolve -File=<path>                       list every conflict
 *   -run=GraphCode.GcResolve -File=<path> -Status               how many are left
 *   -run=GraphCode.GcResolve -File=<path> -Pick=2 -Take=theirs  resolve one
 *   -run=GraphCode.GcResolve -File=<path> -Pick=2 -Value=7.5    resolve one to a third value
 *
 * PROPOSES; DOES NOT COMMIT. It edits the file and stops - no asset is touched and nothing is
 * committed, because resolving a merge conflict is a semantic decision about code the resolver did
 * not write and cannot run. `gc_apply` is the next step and has its own gates.
 *
 * It does not enumerate the project (`GcCheck` does, and CI already runs it) and it does not render
 * diffs (`git diff` does, on text this format exists to make diffable). §4.9.1 assessed a broader
 * tool and cut both halves; what was genuinely missing was this one.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcResolveCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcResolveCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * How often would the merge driver have been right? GC-EDITOR-VCS-DESIGN.md E3.2.
 *
 *   -run=GraphCode.GcShadowReport                 the human report
 *   -run=GraphCode.GcShadowReport -Json=<path>    the same numbers, for something that is not a person
 *
 * THIS IS THE EVIDENCE FOR M1.10 AND NOTHING ELSE READS IT. `.gitattributes` still says
 * `merge=binary`, and GC-VCS-MERGE-DESIGN.md §4.10 says not to change that until the driver has been
 * watched over real merges — because flipping early swaps a known failure, a plain conflict a human
 * resolves, for an unmeasured one. Every merge the driver computes leaves a record under
 * `Saved/GraphCode/Shadow/`; this reads them and asks, per merge, whether the file the human ended
 * up with is the file the driver proposed.
 *
 * ALWAYS EXITS 0, INCLUDING ON A DIVERGENCE. It is a measurement, not a gate: a divergence is the
 * finding this exists to surface, and a report that failed a build for producing its own output
 * would simply be turned off. The decision it feeds — E3.3, throw the switch or do not — belongs to
 * a person reading the numbers, and the report is written to make that reading hard to do carelessly.
 *
 * It loads no asset and needs no editor state, so it runs headless and with the project closed.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcShadowReportCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcShadowReportCommandlet();
	virtual int32 Main(const FString& Params) override;
};

/**
 * Write the capability registry out as JSON. GRAPHCODE-CORE-QUEUE.md C1.
 *
 *   -run=GraphCode.GcCapabilities [-Out=<path>]
 *
 * The registry lives in C++ because the code is the truth and the specs assert it against a real
 * projection. The bridge is JavaScript and cannot call into it, so the two sides need one artifact
 * between them — this writes it, and a Node test compares `graphcode-help.js` against it.
 *
 * Committed on purpose. `GC-CAPABILITY-TRUTH-AUDIT.md` measured twenty-nine places where prose and
 * code disagreed; the only thing that stops a thirtieth is a file both sides are checked against,
 * and a generated file nobody commits is a file nobody checks.
 *
 * Loads nothing and needs no editor state, so it runs headless with the project closed.
 */
UCLASS(Config=Editor)
class GRAPHCODE_API UGcCapabilitiesCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGcCapabilitiesCommandlet();
	virtual int32 Main(const FString& Params) override;
};
