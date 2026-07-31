# GraphCode

**Unreal Engine, as text you can review.**

Unreal's authored content is binary. Code review stops at the content boundary, `git blame` has no
answer, and merges are a coin flip. GraphCode writes deterministic text beside the binary — so
content lives in code review, in `git log`, in CI, and in a grep.

GraphCode is **four products over one format**. One grammar, one determinism guarantee, one
honesty contract. They differ in what they point at.

---

## The product line

| | Points at | Extension | Status |
|---|---|---|---|
| **GraphCode** | authored assets — Blueprints, widgets, materials, schemas | `.gc` | **shipping** |
| **GraphCode Play** | the *running* game — capture, replay, branch, restore | `.gcp` | snapshot writer built; runner designed |
| **GraphCode Move** | animation, navigation, AI behaviour | `.gc` | partial; AnimGraph projects today |
| **GraphCode World** | levels and the heavy families — Niagara, Groom, PCG, audio, data | `.gc` / `.gcw` | designed; level writer started |

**GraphCode is the essential one.** The other three are worthless without it — they all emit the
same grammar, inherit the same determinism gates, and are read by the same tools. Play, Move and
World are scope, not architecture.

Read [§1](#1-the-shared-foundation) once. Then read the product you care about.

---

## Table of contents

- [1. The shared foundation](#1-the-shared-foundation) — what all four inherit
- [2. GraphCode — assets as text](#2-graphcode--assets-as-text) *(the core product)*
- [3. GraphCode Play — the running game as text](#3-graphcode-play--the-running-game-as-text)
- [4. GraphCode Move — animation, navigation, AI](#4-graphcode-move--animation-navigation-ai)
- [5. GraphCode World — levels and the heavy families](#5-graphcode-world--levels-and-the-heavy-families)
- [6. Extensions and naming](#6-extensions-and-naming)
- [7. Architecture](#7-architecture)
- [8. Status at a glance](#8-status-at-a-glance)
- [9. Packaging four products](#9-packaging-four-products)
- [10. Website source material](#10-website-source-material)
- [11. Document index](#11-document-index)

---

# 1. The shared foundation

Everything below is true of all four products. It is the reason they are one product line rather
than four tools with a shared prefix.

## 1.1 Determinism, or the whole thing is noise

A text dump of a `.uasset` is worthless for diffing. The bytes carry authoring order, node GUIDs,
node positions and export-table indices — all of which move for reasons that have nothing to do
with what the content *means*. Open a Blueprint, drag a node, save: a naive dump diffs.

GraphCode's readers were built to a round-trip invariant, and the properties that invariant needed
are exactly the properties a git projection needs:

| Property | Why it matters |
|---|---|
| Aliases `n0, n1, …` assigned by **structural traversal**, never array position | authoring order is not reproducible; a re-save must not diff |
| Entry points sorted by title, GUID only as a tie-break | a graph with two events reads the same in either authoring order |
| **No node positions in the emitted text** | moving a node is not a change, and produces no diff |
| **No GUIDs in the emitted text** | a write path minting fresh GUIDs must not rewrite the file |
| `sig:` is a BLAKE3 digest of a full canonical read | one line that moves exactly when content moves |
| `read(apply(read(G))) == read(G)` is an automated spec suite | the projection is stable under a write cycle |
| Values equal to the correct baseline are omitted | adding an unrelated object must not rewrite its neighbours |

The acceptance test for the entire product line is one line: **a no-op re-save must be
byte-identical.** If a re-save diffs, every claim above is void and the tool is a noise generator.

## 1.2 `omits:` — the honesty contract

This is the part that decides whether a projection tool is useful or actively harmful.

> **A diff that shows nothing must mean "nothing changed."**

It can only mean that for the facets the file actually represents. So a facet may be missing — but
it may never be **missing *and* undeclared**. Every gap is named in `omits:`, in the file, on every
write. Silence about a gap is the failure mode that makes a tool like this worse than none.

Concretely: a Blueprint whose Timeline curves changed produces no diff. That is why every
Blueprint's envelope carries `omits: … timelines …`, and why the write path refuses a graph that
owns a Timeline rather than writing a partial one.

The rule is enforced as a test, not a convention: one spec asserts the declared omission ledger
token-by-token, and a second asserts that a **real projection** declares what the ledger promises.

## 1.3 One grammar, three statement shapes

Every product emits the same language. An audit against the fifteen inherited rules, over the
219-file corpus and every emitter, confirmed that three statement shapes carry every asset kind:

```gc
Health: float = 100.0 [instance_editable]        ← a declaration; lives in the block that
                                                   says what it is, carries no keyword

Title = TextBlock(Font.Size=32)                  ← an authored object, with { children }
  @slot[Padding.Bottom=24.0]                       and @slot / @placement / @attach relations

n0 = CallFunction Actor:K2_SetActorRotation(NewRotation=n1.ReturnValue) : ->n2
                                                 ← a graph statement
```

Which is why a Material graph, a widget tree, a Niagara emitter stack and a placed-actor list are
one language and not four. A reviewer learns it once.

## 1.4 The envelope

```
gc 1  kind:<kind>  asset:<package>  sig:<body digest>  bin:<uasset digest>
parent: <class path>
omits: <token ...>
source: <project> <engine version>  ir: <irname/version ...>
```

- **Line 1 owns identity.** The format version comes first so a parser can refuse a future format.
  A stray file is self-describing and a filename/asset mismatch is detectable.
- **`sig:` is the body digest; `bin:` is the `.uasset` digest.** Two independent questions — "was
  this file edited?" and "did the asset move underneath it?" — get two independent answers. `bin:`
  equal means *definitely* unchanged; `bin:` different means *may* have changed. Never wrong in
  the direction that matters.
- **Block order is fixed, and a covered-but-empty facet writes an empty block.** Presence means
  coverage. A facet with no block at all is named in `omits:`.
- **Deterministic bytes:** fixed block order, fixed field order, structural traversal, LF, UTF-8
  without BOM, trailing newline.

## 1.5 Properties every product keeps

- **Reads work with the editor closed.** The committed text is the read path.
- **The binary always wins.** A projection is generated and safe to delete. If text and asset
  disagree, the asset is right.
- **A projection failure never fails an asset save.** It is logged, retried within a bounded
  queue, and the check commandlet is the enforcement point.
- **No telemetry, no cloud account, no network path.** The AI layer is optional and loopback-only.

---

# 2. GraphCode — assets as text

**The essential product.** Everything else depends on it.

```
Content/Blueprints/BP_Door.uasset      ← binary. `git diff` says "Binary files differ".
Content/Blueprints/BP_Door.gc          ← the same authored meaning as text. Diffable. Blameable.
```

Written automatically on every save. Works with no Node process, no network, no account, and no AI.

## 2.1 Show me

Real files from the reference project. Nothing here is hand-written.

**A user enum** — reorder two entries and the diff is two lines:

```gc
gc 1  kind:user_enum  asset:/Game/Blueprints/Enums/BevelType  sig:82e3645134a35bcc…  bin:65124fbaf619b784…
omits: none
source: OraStudio 5.7.4-51494982+++UE5+Release-5.7  ir: schemacode/1

settings {
  description = ""
}

entries {
  `Corner Bevel` = 0
  `Horizontal Edge Bevel` = 1
  `Verticel Edge Bevel` = 2
}
```

**An Actor Blueprint** — component tree, class defaults, comments and a real event graph, abridged
from `BP_TeleportVisualizer.gc`:

```gc
gc 1  kind:blueprint  asset:/Game/XRFramework/Blueprints/BP_TeleportVisualizer  sig:9bf3fd44…  bin:a8127795…
parent: /Script/Engine.Actor
omits: timelines blueprint_extensions blueprint_settings fixed_size_arrays
source: OraStudio 5.7.4…  ir: blueprintir/2 actorir/1

class_defaults {
  PrimaryActorTick.bCanEverTick = true
}

components {
  Root = SceneComponent()
  {
    NS_PlayAreaBounds = NiagaraComponent(
      Asset=obj'/Game/XRFramework/VFX/NS_PlayAreaBounds.NS_PlayAreaBounds'
    )
  }
}

graph {
  eventgraph EventGraph  sig:30cb4e31e002cc2a-16 {
    c0 = comment("See NPC_VRTemplate for TeleportColor") color(0.00,0.32,0.81,1.00)
    c2 = comment("Move the Play Area Bounds relative to the Players HMD Location")
         color(0.03,0.03,0.03,1.00) covers[n4, n5, n6, n7]

    n0 = Event ReceiveBeginPlay() [bOverrideFunction=True] : ->n1
    n1 = CallFunction HeadMountedDisplayFunctionLibrary:GetPlayAreaBounds() : ->n2
    n2 = CallFunction NiagaraComponent:SetVariableVec3(self=n3.NS_PlayAreaBounds,
           InVariableName="User.PlayAreaBounds",
           InValue.InValue_X=n1.ReturnValue_X) : ->_
    n3 = VariableGet NS_PlayAreaBounds()
    // Get the Pawn's relative camera location
    n8 = CallFunction KismetMathLibrary:InverseTransformLocation(T=n9.ReturnValue)
  }
}
```

Note what is *not* there: no node positions, no GUIDs, no export-table indices, no compiled
products. Drag a node three pixels and save — the file does not change.

**A Widget Blueprint** is one document carrying **both** the Designer tree and the graph. The
authored name is the left-hand side and the document's only handle on the widget:

```gc
widgets {
  design DesignTimeSize=(X=1920.000000,Y=1080.000000)

  Root = CanvasPanel()
  {
    Form = VerticalBox(variable=true)
      @slot[Anchors=(0.500000,0.500000,0.500000,0.500000)]
    {
      Title = TextBlock(Font.Size=32, Text=text("Sign in"))
        @slot[Padding.Bottom=24.000000, HorizontalAlignment=HAlign_Center]
      Submit = Button(variable=true, IsFocusable=false)
    }
  }
}
```

**A Material Instance** is its override list and nothing else:

```gc
gc 1  kind:material_instance  asset:/Game/Materials/Instances/Fluted/M_FlutedOak  sig:490e42d8…  bin:97bb581c…
parent: /Game/Materials/M_Fluted
omits: instance_settings material_layers scalar_atlas

parameters {
  RefractionDepthBias: float = 0.000000
}
```

## 2.2 Coverage

Seven kinds project. In the reference project that is **251 `.gc` files**: 100 Blueprints,
54 Widget Blueprints, 32 Materials, 30 enums, 18 Material Instances, 15 structs, 2 Material
Functions.

| Kind | IR | `omits:` | Writable blocks |
|---|---|---|---|
| `blueprint` | `blueprintir/2` | components, cdo_defaults, timelines, blueprint_extensions, blueprint_settings | graphs, properties |
| `blueprint` (Actor parent) | `blueprintir/2` + `actorir/1` | timelines, blueprint_extensions, blueprint_settings, fixed_size_arrays | graphs, properties, **components, class_defaults** |
| `widget_blueprint` | `widgetir/2` + `blueprintir/2` | the Blueprint tokens + widget_bindings, widget_animations, widget_navigation, orphan_widgets, fixed_size_arrays | graphs, properties, widgets |
| `user_struct` | `schemacode/1` | none | settings, properties |
| `user_enum` | `schemacode/1` | none | settings, entries |
| `material_instance` | `materialir/1` | instance_settings, material_layers, scalar_atlas | parameters (runtime families; static permutations refuse) |
| `material` | `materialir/1` | comments, exec_wires, material_settings, parameter_declarations, subgraphs | settings; graph (identity-preserving edits + expression create/delete) |
| `material_function` | `materialir/1` | comments, exec_wires, parameter_declarations, subgraphs | settings; graph |

`interfaces { }` and `dispatchers { }` project for every Blueprint and are **read-only** — each
hides a two-object engine operation that a text patch cannot express safely, so the write path
refuses them by name with that reason rather than a generic error. Binding, unbinding and calling
a dispatcher *are* ordinary graph statements and *are* writable.

## 2.3 Text back into the asset

The input is **the file on disk**, whatever edited it. `sed`, a patch, a person, an AI: they all
produce the same artefact — a changed file — and one command lands it.

```bash
UnrealEditor-Cmd.exe MyProject.uproject -run=GcApply -Asset=/Game/Blueprints/BP_Door [-DryRun]
```

- **It is a block diff against the live asset, never a rebuild.** `omits:` means the text is not
  the whole asset, so the writers are identity-preserving. An unchanged block is not rewritten —
  it is not even parsed.
- **Two staleness questions, both answered before anything is written.** Was the *file* edited?
  (body digest vs. `sig:`). Did the *asset* move? (`sig:` vs. what the asset projects to **now**).
  The second is the race that matters: read a `.gc`, edit it for two minutes, and if a designer
  saved that asset meanwhile, a naive apply silently overwrites their work. It is refused instead.
- **Every refusal is collected before anything is written.** A block with no writer, a block the
  file invented, a block the file dropped — all found first, and any one means nothing at all is
  written. A refusal discovered after the third block landed is a half-applied asset.
- **Block order is not cosmetic.** Declarations, then the Designer tree, then anything that can
  reference either — a graph statement can read a variable `properties` declares.
- **Renames preserve identity.** A rename is never lowered to remove + add. UE repairs a stale
  variable reference from the variable's GUID, which lets referencing assets that were never
  loaded fix themselves; a fresh GUID breaks them silently, with nothing logged anywhere.
- **Destructive edits are gated.** Removals and type changes with live references need an explicit
  `allow_breaking_changes` flag, and a removal reports every node it orphans — blueprint, graph,
  GUID, and the lost declaration's name/type/default — on refusals and dry runs too, because after
  the apply it cannot be recomputed.

Blocks with **no writer**: `inherited { }`, `defaults { }`, `interfaces { }`, `dispatchers { }`,
and any graph that owns a Timeline.

## 2.4 Lifecycle

The projection is triggered from **one place in the engine** —
`UPackage::PackageSavedWithContextEvent` — not from the tools that write assets. Every save reaches
it: Ctrl+S, Save All, the Content Browser, and whatever writes a package next year. A new write
tool inherits the behaviour without knowing this feature exists.

| Asset event | Sidecar result |
|---|---|
| successful manual save | enqueue a canonical projection beside the `.uasset` |
| autosave, cook, procedural or failed save | do nothing |
| rename or move within eligible content | move the `.gc`, then regenerate so its envelope names the new package |
| move into an excluded prefix | remove the old `.gc`, create none |
| asset deletion | cancel queued work and remove the matching `.gc` |
| deletion while the editor was closed | report an orphan in `GcCheck`; remove only with explicit `-Prune` |

That last row is deliberately not automatic startup cleanup. Source-control operations leave the
tree temporarily inconsistent during a checkout, merge or branch switch. Detection is safe;
unrequested deletion is not.

**Save All is the performance case that decides adoption.** A full canonical read costs roughly
half a second for a substantial Blueprint; across 300 assets, inside a save the user is waiting on,
that is minutes — and it lands as "the editor hangs when I save", which is how a feature gets
switched off and never switched back on. So the save delegate does **no work**: it enqueues,
deduplicated by package, and a ticker services the queue against a per-frame budget (20 ms).

## 2.5 Command line and CI

```bash
# Rewrite every eligible sidecar. Orphans preserved; exits non-zero unless you pass -Prune.
UnrealEditor-Cmd.exe MyProject.uproject -run=GcRegenerate [-Prune]

# The same pass, writing nothing. Non-zero if any sidecar is absent, stale, invalid or orphaned.
# This is your pre-commit hook and your CI gate.
UnrealEditor-Cmd.exe MyProject.uproject -run=GcCheck

# Push one asset's edited .gc back into the .uasset.
UnrealEditor-Cmd.exe MyProject.uproject -run=GcApply -Asset=/Game/Path/Name [-DryRun]
```

`GcCheck` deliberately re-projects rather than trusting `bin:`. A matching binary digest proves the
*binary* is unchanged, not that the *projection* of it is — change the projector and the same bytes
yield different text while `bin:` still matches. A digest-only check would report "current" for a
sidecar that is now wrong, which is precisely the silent drift the check exists to catch.

## 2.6 Git setup

```gitattributes
*.gc      text eol=lf merge=binary
*.uasset  binary
*.umap    binary
```

**`merge=binary` on a text file is deliberate.** Git marks a conflict rather than producing a
text-merged file that describes no asset that exists. The resolution is to merge the `.uasset` (by
picking a side, as today) and re-save; the sidecar regenerates. A `union` or `ours` merge would
produce a plausible-looking lie.

**Sidecars are committed, not generated-and-ignored.** Ignoring them keeps the tree clean and gives
up review history and blame, which are the entire value.

**The `.gitignore` trap, which will bite you exactly once:** git cannot re-include a file whose
*parent directory* is excluded. `!Content/Fab/**/*.gc` does nothing if `/Content/Fab/` is ignored.
The fix is not a gitignore trick — it is **not writing them there**. The excluded-prefix list
defaults to the directories already excluded from the repo.

## 2.7 Settings

Project Settings → Plugins → GraphCode (`UGraphCodeSettings`, `Config=Editor`):

| Setting | Default | Meaning |
|---|---|---|
| `bEnableSidecars` | `true` | master switch |
| `ExcludedPackagePrefixes` | project-seeded | long-package prefixes that never receive a `.gc` |
| `TickBudgetMilliseconds` | `20.0` | per-frame game-thread budget for save-triggered projection |
| `MaxWriteRetries` | `3` | failed writes retried this many times before the next real save |

## 2.8 Limitations

- **Timelines do not round-trip, and a graph holding one cannot be written.** A `UK2Node_Timeline`
  reads as a graph statement, but the `UTimelineTemplate` holding its tracks and curves is not
  projected. The write path re-reads the live graph and refuses the whole-graph write when it finds
  one, *before* the mode branch, so a differential write is refused too. **The refusal is per
  graph**: other graphs in the same Blueprint stay writable. This is the largest declared gap.
- **A C-array `UPROPERTY`** — `FLinearColor Tints[8]` — is declared as `fixed_size_arrays`. The
  grammar has no spelling for an indexed slot, so the diagnostic carries the array's *length*,
  never its values: editing one element produces no diff.
- **Widget bindings, animations, navigation and orphan widgets are declared but not carried.** The
  in-body diagnostic is a **count**: adding or deleting a binding moves it; *repointing* one at a
  different property does not.
- **`blueprint_settings` and `blueprint_extensions` are omitted for every Blueprint** — the asset's
  description/category/namespace/deprecation, a subclass's own fields, and
  `UBlueprint::GetExtensions()`, where MVVM keeps its entire viewmodel graph.
- **A `MathExpression`'s collapsed body is not projected, and that is deliberate.** The node
  re-parses its own `Expression` string into a fresh inner graph on every reconstruct, so the body
  is derived state — reading it back made two projections of an unchanged asset disagree, and was
  the one measured byte-stability failure in the corpus. The statement carries `[Expression=…]`,
  which is the whole input. Nothing authored is missing, so this is not an `omits:` facet.
- **Open guard.** The sync fast path compares kind, IR versions, `omits:`, project and engine
  version, then trusts `bin:`. None of those move when the **projector** changes — so a projector
  fix leaves committed sidecars certifying themselves as current while having been generated by
  code that no longer exists. A projector-version field in the envelope closes it, at the cost of
  one header line every sidecar rewrites once. Not yet decided.

---

# 3. GraphCode Play — the running game as text

**Scope:** the *verification* loop. Core closes authoring — read logic, change it, apply it.
It does not answer whether the door actually opens when the player steps on the plate. That
question has only ever been answered by a human with the editor open.

> Start the game, act on it, read the resulting world state as text, and stop — in one submitted
> unit of work, with a guaranteed stop.

If that exists, the loop is **build → play → act → observe → fix**, and the observation step costs
tokens comparable to reading a Blueprint, not a screenshot.

Acting on a game is a solved engine problem with published APIs. **Reading the result cheaply and
honestly is where the design work is**, and it is most of what has been built.

## 3.1 The capture — `.gcp`

A `.gcp` is one world, one selector set, one quantisation, at one instant.

```
gcp 1  world:/Game/Maps/Test  pie:UEDPIE_0_Test
at: frame 1284  time 21.35
select: class:BP_Door class:BP_PressurePlate name:PlayerCharacter
found: 4 actors, 31 properties
precision: 0.01
baseline: none
omits: transient non_visible

actors {
  BP_Door_1 : BP_Door {
    Location = (150.00, 200.00, 0.00)
    Rotation = (0.00, 90.00, 0.00)
    IsOpen = true
    LinkedPlate = /Game/Maps/Test.Test:PersistentLevel.BP_PressurePlate_1
    OpenAngle = 90.00
    DoorMesh : StaticMeshComponent {
      RelativeLocation = (0.00, 0.00, 0.00)
      Visible = true
    }
  }

  BP_PressurePlate_1 : BP_PressurePlate {
    Location = (150.00, 200.00, 0.00)
    IsPressed = true
    Overlapping = [/Game/Maps/Test.Test:PersistentLevel.PlayerCharacter_0]
  }
}
```

Every envelope field exists to prevent one specific wrong reading:

- **`at:` is on its own line**, and that placement is load-bearing. Two captures a second apart are
  at different frames *by construction*, so a whole-file comparison of an unchanged world would
  always fail. Everything else in the envelope is a function of the *request*, not the *instant* —
  so "nothing changed" reads as exactly one differing line.
- **`select:` is echoed back.** Without it, a file with no matches reads as "nothing happened"
  instead of "looked for BP_Door and found none".
- **`found:`** makes a mis-typed selector obvious at a glance rather than after a diff.
- **`precision:`** states the quantisation *actually used*, written on every capture including the
  default one — a field that appears only when overridden is a field nobody checks.
- **`baseline: none`** says absent-means-*filtered*, not absent-means-default. A capture prints
  every visible property, always, because two captures get diffed and both sides need the line to
  be there. This is the opposite of the core sidecar's rule, which is why the header must declare
  which rule produced the file.
- **No `sig:`, no `bin:`, no `asset:`.** A capture is not a projection of an asset; their absence
  is the point.

**The selector is mandatory and there is no default.** `class:` (subclass-inclusive), `name:`,
`tag:`, `near:<actor>,<radius>`, `widget:`, and `*` — which is opt-in, warns, and is a debugging
tool rather than a test tool.

One deliberate divergence from the core value rules: **`FText` writes its displayed string, not its
key.** A sidecar carries the key because it must round-trip without orphaning the translation. A
capture answers "what does the score text say", so it writes `"1250"`.

## 3.2 The scenario runner — designed, not built

One tool, not five. A closed nine-step grammar inside a single submitted unit of work:

```jsonc
run_scenario {
  ceiling_seconds: 30,
  steps: [
    { start:      { mode: "viewport", map: "/Game/Maps/Test" } },
    { wait:       { seconds: 1 } },
    { snapshot:   { as: "before", select: "class:BP_Door" } },
    { input:      { layer: "player", key: "W", hold_seconds: 2 } },
    { wait_until: { property: "BP_Door_1.IsOpen", equals: true, timeout_seconds: 5 } },
    { snapshot:   { as: "after", select: "class:BP_Door" } },
    { stop:       {} }
  ]
}
```

`start`, `stop`, `wait`, `wait_until`, `input`, `snapshot`, `capture`, `log_mark`, `console`.
The set is **closed** — a step list that can invoke any registered tool is arbitrary script
execution with extra syntax, and inherits its permission problem.

Design points that are not obvious and are already settled:

- **`stop` runs on every exit path** — success, step failure, ceiling, cancellation, exception. A
  scenario that leaves PIE running has failed even if every step passed. The transcript reports
  `pie_stopped` as an assertion rather than an assumption.
- **A timed-out predicate reports the value it actually observed**, not just "timed out". That is
  the difference between a useful failure and a useless one.
- **Every step reports `frames` alongside `seconds`.** Two seconds and zero frames is a frozen
  engine; two seconds and 120 frames with no movement is a gameplay bug. Those must be
  distinguishable without anyone having to diagnose it.
- **`input` exists only inside a scenario.** A one-shot "press W" returns before the engine has
  ticked, so it can report that a key was delivered and nothing about what the key did. That is a
  tool whose success value is misleading in the exact case people reach for it.

## 3.3 The save/load/branch direction — the open decision

The product goal is larger than capture: **a diffable save/load system** — snapshot a game state,
branch it, restore it, diff two branches.

**This requires reopening a ruling, and the counter-argument has to be answered rather than
skipped.** The current design says a capture is **never** written back, for two reasons that are
still true:

1. A capture has no asset to write back *to*. It names a moment, not a package.
2. A capture's property rule is **absolute** — it prints every visible property, including ones
   equal to their default. Applying such a file would write hundreds of properties nobody authored,
   which is a very different operation from the core product's identity-preserving block diff.

Neither of those forbids save/load. They say that **restore is a different mechanism from apply**,
and it needs its own design:

- **Restore is state, not authoring.** It targets a live world, not a `.uasset`. It should not
  route through `gc_apply`, and `gc_apply` should keep refusing the extension.
- **The text stays the review artefact.** What makes this a GraphCode product rather than a save
  system is that the branch point is *readable* — you diff two states and see which nine properties
  differ, then choose.
- **The property rule needs a third mode.** Capture is absolute, projection is delta-against-
  archetype; a restorable save probably wants delta-against-*a named baseline capture*, so a branch
  is small and a diff is meaningful. That is a `baseline:` value that does not exist yet.
- **Identity across a restore is the hard question**, and it is the same question the level format
  faces: an actor's label at capture time is not a stable identity across a reload.

Recommendation: build capture-diff first (it is nearly done), ship it as the reviewable-playtest
product, and treat restore as its own milestone with its own baseline mode — rather than widening
the capture format and discovering the property rule was wrong after files exist.

## 3.4 Status

| Piece | State |
|---|---|
| Capture writer (`FGraphCodeWorldSnapshot`) | **built and observed** — 11 actors / 1,778 properties on first live run |
| `export_world_snapshot` tool, registered and catalogued | **built; called live over HTTP, refusals and warnings exercised** |
| Determinism gates | **3 specs, 0 failures** — including an idle gate that waits with a latent command, not a sleep |
| Runtime fixture (`Content/__RuntimeFixture/`) | built, loaded and captured |
| PIE capture (`pie:`) | **not written** — next milestone |
| PIE lifecycle, input, scenario runner | designed, unstarted |
| Save / restore / branch | **not designed** — §3.3 |

**What the first live run found**, since the value of running something is what it tells you that
reading it did not: two captures 13 seconds and 1,567 frames apart were byte-identical apart from
`at:`. No property moved on its own — the two predictions on record about which one would were both
wrong, and are kept rather than quietly corrected. One real defect surfaced that only execution
could have found: an *unset* soft object reference printed `<unsupported>`, because its empty
`SubPathString` exports no text.

## 3.5 The constraint anyone estimating this must read first

**Every tool call today is a blocking game-thread call.** "Hold W for two seconds" inside a tool
freezes the engine instead of moving anything. And the task queue runs **one tool, not a step
list**. The scenario runner is therefore a new capability, not a new parameter — that is the single
biggest reason Play is a separate product with its own milestones rather than three more tools.

Two engine traps worth knowing before you start: there is no `PIE` console command, and enabling
the Slate automation driver **takes the user's real mouse and keyboard away** until it is disabled.

---

# 4. GraphCode Move — animation, navigation, AI

**Scope:** how things move and decide. Animation Blueprints, state machines, montages, skeletons
and control rigs; navigation; behaviour trees and blackboards; and the input bindings that drive
them.

This is the product with the most **already true and least noticed**.

## 4.1 What already works

**An Animation Blueprint gets a `.gc` today.** `UAnimBlueprint` derives from `UBlueprint`, so it
projects as `kind:blueprint` and its AnimGraph renders in full — observed on a real asset with
`AnimGraphNode_LayeredBoneBlend`, `AnimGraphNode_SequencePlayer`, `AnimGraphNode_TwoWayBlend`,
`AnimGraphNode_Mirror` and `AnimGraphNode_Root`, each carrying its complete `[Node=(…)]` reflection
property channel, with **zero unsupported notes**.

That is not a special case anyone wrote. The reflection-driven property channel built so that a
*plugin's* custom node could carry its own state handles anim nodes with no per-class table — the
design working as intended. It is the strongest single piece of evidence that Move is a scope
expansion rather than a rewrite.

The MCP layer also already carries `anim`, `character` and `enhanced_input` domains.

## 4.2 What is missing, honestly

| Gap | State |
|---|---|
| **State machines** | **unmeasured, not unsupported.** The reader recurses through `GetSubGraphs()`, which both `UAnimGraphNode_StateMachine` and `UAnimStateNode` implement, so the graphs are reachable. Whether transitions and state structure round-trip is an open probe. **Do not write it into `omits:` until it is measured** — that is the rule the coverage ledger exists to enforce. |
| **Target skeleton, preview mesh** | omitted, under `blueprint_settings`. An Animation Blueprint's own subclass fields are not projected — each reader assumed the other owned them |
| Behavior Tree, Blackboard | no projection at all |
| Navigation settings, NavMesh volumes | no projection at all |
| Control Rig, Physics Asset, Skeleton, Animation Montage | no projection at all |
| Input Action, Input Mapping Context | no projection at all |

## 4.3 The first question to answer

**Should an Animation Blueprint keep projecting as `kind:blueprint`?**

Right now it does, which means the write path can route a graph write into an AnimGraph on the
assumption it is a K2 graph. Nobody has proved that assumption holds. Giving Anim Blueprints their
own kind is the first decision in this product, and it is a decision about safety rather than
taxonomy.

Then, in order: measure state machines; close `blueprint_settings` so a skeleton change is
reviewable; then Behavior Tree and Blackboard, which are the two families with the clearest text
form and the highest review value — an AI decision tree is exactly the kind of logic that gets
changed by one person and breaks for another.

---

# 5. GraphCode World — levels and the heavy families

**Scope:** the content families that are complex, specific, and currently invisible in review —
levels and placed actors, Niagara, Groom, PCG, audio, and the data families (Data Assets, Data
Tables, curves).

## 5.1 The level

`Content/MainLevel.umap` is binary, so a placed-actor change is unreviewable — git shows a size
delta and nothing else. Every argument for a Blueprint sidecar applies unchanged to a level. Levels
were scoped out of v1 as a *timing* decision, not a finding that they are unprojectable.

A level `.gc` and a Play capture both write actors-with-properties, which is exactly why the seam
had to be decided before the second one was built:

| | Play capture (`.gcp`) | Level projection (`.gc`) |
|---|---|---|
| names | a moment: `at: frame 16290 time 42.40` | an asset: `asset:`, `sig:` |
| property rule | **absolute** — every visible property, always | **delta** — only deviation from the archetype |
| one `BP_FixtureDoor` costs | 167 lines | around four |
| committed | no | yes, beside the `.umap` |
| applied back | never | that is the entire point |

**The property rule is the load-bearing row**, and the two rules are opposites. So the header
declares which one produced the file — `baseline: none` for a capture, `baseline: archetype` for a
projection — because "this property is not in the file" otherwise means two different things, and a
reader that cannot tell them apart is worse than two separate formats.

**What genuinely merges is the writer**, with a baseline parameter: given an actor, a baseline
object (or none), a precision and a property policy, emit one `actors {}` entry. A value that
formats correctly in a capture then formats correctly in a projection, the sort order and
quantisation are solved once, and `omits:` stays one vocabulary.

**Started:** that unified writer's grammar landed 2026-07-29 — actors write as
`Name = FullClassPath(props)` with braces for child components, keyed `(X=,Y=,Z=)` / `(Pitch=,Yaw=,Roll=)`
values, the world transform once as `@placement`, and `baseline:` / `source:` / `ir:` in the header.
Four specs pass. The one grammar extension it forced is worth knowing about: rule 7's inline form
makes one captured actor an 8 KB line, so **layout follows from baseline** — `archetype` writes
inline, `none` writes the same tokens wrapped one per line.

**World Partition is refused by name** rather than guessed at. Splitting an actor per package under
`__ExternalActors__` is a storage scheme, not a second syntax, and it needs its own ruling.

## 5.2 Niagara

Fully specified, not built. The design decision that matters: **model the authored stack, not a raw
node dump.** A System projects as `emitters {}` handles with per-handle overrides plus
`renderers {}`; Scratch Pad internals are deliberately out of the first slice.

## 5.3 The data families

Data Assets (`values {}`), Data Tables (`rows {}`), Curve Tables and standalone curves
(`curves {}`) are specified and unbuilt. They are the **cheapest real wins in the whole product
line** — a Data Table is already tabular, the value codec already exists, and "which balance number
changed in this patch" is a question every team asks and no team can currently answer from a diff.

## 5.4 Groom, PCG, audio

**Not designed, not proposed, no code.** They are named here because they are in the product's
scope, and naming them without a design is more honest than a matrix that implies otherwise.

Before any of them is built, one policy question needs a ruling, and it applies to this whole
product: **a Texture or a Static Mesh has no authored *text* worth diffing, only import settings.**
`omits: everything` is a file that costs a save and says nothing. "Every asset gets a `.gc`" is a
promise worth keeping only where the projection is meaningful — so World's supported list must be
chosen, not enumerated.

## 5.5 Order

1. Levels — the highest-value gap, and the writer is started.
2. Data Assets / Tables / curves — cheapest, on the shared value and curve codecs.
3. Niagara System and Emitter — specified, no code.
4. Actor identity and World Partition storage — **before** claiming any level write support.
5. Groom, PCG, audio — after the §5.4 ruling.

---

# 6. Extensions and naming

| Extension | Product | Committed? | Applied back? |
|---|---|---|---|
| `.gc` | Core, Move, World | **yes**, beside the asset | yes |
| `.gcp` | Play | no — written under `/Saved/` | no (see §3.3) |
| `.gcw` | World, for level projection | yes, beside the `.umap` | that is the point |

**Note for anyone reading the source before the rename lands:** the in-tree runtime capture code,
its specs and `.gitattributes` still spell the Play format `.gcw`, from the original ruling that
read it as "GraphCode World". Under the four-product line that name belongs to World's level
projection, and the capture is `.gcp`. The rename is cheap precisely because captures are never
committed — there is no corpus to migrate, only the writer, the extension refusal in the apply
path, and one `.gitattributes` line.

Why separate extensions at all, rather than one format with a `kind:`:

- The sidecar reader **requires** `kind`, `asset` and `sig`, and refuses without all three.
  `asset:` names the package the file projects, and the apply path writes the file back to it. A
  capture has no asset. To share the extension it would have to name an asset it is not a
  projection of, or force that check to be relaxed — and that check is the identity contract that
  makes the write path safe to point at an arbitrary edited file.
- `GcCheck` and `GcRegenerate` walk `.gc` files and verify each against its asset. A shared
  extension means both need a kind filter forever, and the first time someone forgets it, CI fails
  trying to verify a capture against an asset that never existed.
- `.gitattributes` and `.gitignore` make "projections are committed, captures are not" a property
  of the **filename** rather than of a path convention somebody has to remember.

**The grammar is shared and should be.** One value formatter is what makes `(150.00, 200.00, 0.00)`
render identically in a sidecar and a capture, which is what makes them comparable by eye. Share
the formatter. Do not share the envelope, the reader, or the write door.

---

# 7. Architecture

```
GraphCode/
  Source/
    GraphCode/            product startup, settings, save hook, sidecar writer/reader,
                          projector, sync, apply, commandlets, world snapshot
    AuthoringCore/        shared property/value/signature primitives + reflection codec
    BlueprintIR/          deterministic Blueprint read/write language  (blueprintir/2)
    WidgetIR/             deterministic Widget Designer language      (widgetir/2)
    MaterialIR/           deterministic Material language             (materialir/1)
    UnrealClaude/         bundled MCP/HTTP backend  (optional)
  Resources/
    mcp-bridge/           provider-neutral Node/stdio bridge (optional)
    Fixtures/             .gc fixtures for the reader/writer specs
```

Dependency arrows are one-way and enforced:

- Sidecar generation may call the IR modules.
- The MCP backend may adapt the IR modules into tools.
- **The IR modules never include MCP, HTTP, product UI, or backend types.**
- **The sidecar module never includes MCP backend headers.** They merely ship in the same package,
  so `.gc` still works when the MCP integration is disabled or absent.

| Type | Role |
|---|---|
| `FGraphCodeService` | save/rename/delete hooks, the dedup queue, the budgeted ticker |
| `FGraphCodeProjector` | asset → one canonical document; owns the canonical option set |
| `FGraphCodeSidecarWriter` | document → bytes; envelope, BLAKE3 body digest, atomic write |
| `FGraphCodeSidecarReader` | bytes → envelope + named blocks, digest recomputed |
| `FGraphCodeApply` | block routing, staleness checks, refusal collection |
| `FGraphCodeActorWriter` | actor emission — shared by `components{}`, captures, and level projection |
| `FGraphCodeWorldSnapshot` | Play's capture writer |
| `FGraphCodePolicy` | eligibility: kinds, excluded prefixes, transient/procedural saves |

The sidecar reader **deliberately does not understand a statement.** `widgets { … }` is widgetir/2
and `graph { … }` is blueprintir/2, and both already have a parser that owns their grammar. A third
one here would be a third place for the language to drift. A block's body comes back as bytes, and
whoever wants to interpret it hands it to the IR that wrote it.

One thing that fell out of building that reader: **a brace is not always a brace.** A designer's
label can be `"{ not a block }"`, a UMG display name can be `` `Handle (Pull) }` ``, and an
unsupported-note is free text. The scanner tracks strings, backtick identifiers and comments —
counting braces without that state truncates a document at the wrong byte, and since this feeds a
write path the symptom would be a half-applied asset rather than a parse error.

## The optional AI layer

All four products expose a provider-neutral MCP backend, so an agent can read and write through the
same text. It is **bundled, not required** — the sidecar pipeline never calls HTTP and never
depends on the permission system.

| Tool | Does |
|---|---|
| `gc_help` | the grammar, on demand, per kind and per block. **Works with the editor closed.** |
| `gc_read` | one asset as text. `source:"saved"` works with the editor closed; `source:"live"` projects the running editor including unsaved edits |
| `gc_describe` | what a prospective node or widget would really look like — real property paths, value sets, the slot class the parent actually creates |
| `gc_sync` | create or refresh one asset's `.gc` |
| `gc_apply` | write text back, whole file or one block, with `graph_name:` to target a single graph |

Two design points worth stealing regardless of the AI angle: **"live vs saved" is a parameter, not
an architecture** — the same projector serves both, which is why editor-closed reads work at all.
And **a `.gc` has exactly one legal rendering per asset**, which is what makes `sig:` a digest — so
any diagnostic knob producing something other than that rendering returns `sig: "none"` plus a list
naming what did it. Text that is *not* the file can never be mistaken for the file.

Security baseline, all products: loopback binding only, browser-origin mutation rejected, bounded
request bodies, arbitrary script and console execution hidden and deny-by-default, asset deletion
recoverable and reference-checked, no telemetry, no cloud account.

---

# 8. Status at a glance

| | Built and verified | Designed | Not designed |
|---|---|---|---|
| **GraphCode** | 7 asset kinds, 251-file corpus, save/rename/delete lifecycle, reader, binary binding, block-filtered read, file-is-the-input write path, 3 commandlets, 47 specs | timelines, non-Actor components, `blueprint_settings` | — |
| **Play** | capture writer, export tool, 3 determinism gates, runtime fixture | PIE capture, PIE lifecycle, input, 9-step scenario runner | save / restore / branch |
| **Move** | AnimGraph projection (as `kind:blueprint`), anim/character/input MCP domains | own kind for Anim Blueprints, `blueprint_settings` | Behavior Tree, Blackboard, nav, Control Rig, Physics Asset, Skeleton, Montage |
| **World** | unified actor writer grammar, 4 specs | level `.gc`, Niagara System/Emitter, Data Asset/Table/curve, actor identity | Groom, PCG, audio, World Partition storage |

Plugin: `VersionName 0.1.0`, `IsBetaVersion: true`, UE 5.7. Win64, Linux and Mac are declared —
with the honest caveat that only Win64 has an automated verification gate today.

---

# 9. Packaging four products

This is a structural decision worth taking before the website, because the site's pricing page and
the repository layout have to agree.

**The constraint:** a marketplace listing is one ZIP containing one self-contained plugin. Four
products cannot be four listings that all require the buyer to also find and install a fifth thing
by hand — and equally, Play/Move/World cannot each bundle their own copy of the core, or a buyer
with two of them has two projectors racing for the same save hook.

**Recommendation: one core plugin, three extension plugins that declare it as a dependency.**

```
graphcode/                        ← one repository
  GraphCode/                      ← the core plugin. Standalone, sells alone.
  GraphCodePlay/                  ← declares GraphCode as a plugin dependency
  GraphCodeMove/
  GraphCodeWorld/
  docs/
  examples/                       ← real .gc files next to the assets they came from
  .github/workflows/              ← GcCheck as a CI gate
  LICENSE  THIRD_PARTY_NOTICES  CHANGELOG.md  README.md
```

What that buys, and what it costs:

- **The core sells alone and is the upgrade path.** Every extension listing's description can say
  "requires GraphCode", which is a normal and well-understood marketplace shape.
- **One save hook, one projector, one settings page.** Extensions register asset kinds *into* the
  core's projector rather than owning a pipeline. This is the load-bearing reason not to ship four
  independent plugins.
- **The IR modules live in the core**, so an extension is genuinely small — a projector, a writer,
  a help topic, a capability entry, and specs.
- **The cost is a per-engine-version build matrix multiplied by four.** Marketplaces require a new
  code-plugin package for each engine release, so four products is four submissions per engine
  version. Budget for it, and start the supported matrix at one engine version and one platform.

**Before any of it ships**, the core has to become extractable from the host project: fold the IR
modules and the MCP backend in as `Source/` modules keeping their existing module and API names
(the move must not become a rewrite); move project-specific excluded prefixes into project config,
because a customer path must never be silently excluded for sharing a name with a common
marketplace folder; and turn the development descriptor into a release descriptor — real version,
vendor, documentation and support URLs, `EnabledByDefault: false`, no sibling plugin dependencies.

**Add a capability manifest per product**: one release-owned record per kind — IR version,
implemented blocks, supported operations, `covers`/`omits`, minimum engine version, the help
revision that describes it — generated or validated in CI and used as the source for the offline
help index. A proposed topic may be visible only when it is labelled proposed *and* explicitly
rejected as write input. **This is the mechanism that stops four products' marketing from drifting
away from four products' code.**

---

# 10. Website source material

## 10.1 The line for the whole line

**Unreal you can actually review.** GraphCode turns binary content into deterministic text — so
Blueprints, materials, playtests, animation and levels live in code review, in `git blame`, and in
CI, instead of behind `Binary files differ`.

## 10.2 One line each

| | Line | The promise underneath |
|---|---|---|
| **GraphCode** | *Your assets, in your diff.* | A Blueprint change shows up as changed lines, automatically, on every save. |
| **GraphCode Play** | *Playtests you can diff.* | Capture the running game as text. Two runs, one diff, and you see which nine properties differed. |
| **GraphCode Move** | *Animation and AI, in review.* | The state machine, the behaviour tree and the montage stop being a black box in the PR. |
| **GraphCode World** | *Levels and effects, finally reviewable.* | A moved actor, a retuned emitter, a changed balance row — all of it as text. |

## 10.3 The three things the landing page has to land

1. **The before/after.** One screenshot of `Binary files differ`, one of a real `.gc` diff showing
   a single changed statement. That contrast is the entire pitch and needs no explanation. Put it
   above everything, including the product line.
2. **"It does not lie to you."** Every gap is declared in the file, on every write, enforced by
   tests. This is the trust argument and it is the one that separates GraphCode from a `.uasset`
   dumper. The sceptical reader's first question is *"how do I know the diff is complete?"* and the
   answer is a line in the file.
3. **"It works without the AI."** Sidecars, diffs, CI checks and the commandlets need no Node, no
   network, no account. The AI layer is an upgrade path, not a dependency. Say so above the fold —
   a plugin that appears to require a cloud subscription is dead in a studio procurement review.

## 10.4 Structure the page around the core, not the line

A four-product grid at the top is a mistake. Someone who does not yet believe that a Blueprint can
be meaningfully diffed will not be persuaded by learning there are four of them.

Suggested order: the before/after → what the core does → the honesty contract → **then** the
product line as "and the same thing, pointed at four places" → per-product pages.

Each product page should carry its own honest status band. "Capture built, runner designed" is not
a weakness on a page whose entire credibility rests on declaring gaps — it is the argument.

## 10.5 Objections to answer

| Objection | Answer |
|---|---|
| "Unreal already has an asset diff tool." | It is one asset, in a GUI, for someone who already has the project open. It is not in your PR, your CI, your `git log`, or your grep. |
| "How do I know the text is complete?" | You do not have to trust it — read `omits:`. Every gap is in the file, on every write, asserted by two automated specs. |
| "Will this slow down Save All?" | The save hook does no work. It enqueues, deduplicates by package, and a ticker services the queue against a per-frame budget. |
| "What happens on a merge conflict?" | The projection is marked `merge=binary`, so git conflicts rather than producing a text merge describing no asset that exists. Merge the binary, re-save, the text regenerates. |
| "Does it need an AI subscription?" | No. The core has no network path at all. |
| "What if it's wrong?" | The binary always wins. A projection is generated and safe to delete. |
| "Why four products instead of one?" | They are one format and one guarantee. They differ in what they point at, and in how much of each is built — which the status band on every page tells you. |

## 10.6 What the page must not claim

Coverage it does not have. The kind matrix in §2.2 and the limitations in §2.8 are the marketing
copy too — an accurate supported-list is worth more than a broad one, because the first customer
whose Timeline changes vanish silently is also the last.

Specifically: do not put Groom, PCG or audio on a feature list. They are scope, not features. Do
not imply Play can restore state — it captures. Do not imply Move covers behaviour trees.

---

# 11. Document index

The design record. Each of these is a working document, not marketing.

| Document | Product | What it is |
|---|---|---|
| `GC-SIDECAR-DESIGN.md` | Core | the core spec. §13 the merged document format, §14 reading a `.gc` back, §14.6 the write door, §15 what the projection leaves out |
| `GRAPHCODE-ROADMAP.md` | Core | product and migration plan. **Track G is the coverage ledger** — read it before claiming any asset or facet is covered |
| `GRAPHCODE-FAB-RELEASE-PLAN.md` | Core | release architecture, package profile, verification matrix, submission materials |
| `GRAPHCODE-SIDECAR-LIFECYCLE.md` | Core | the save/rename/delete contract and its required specs |
| `GC-GRAMMAR-AUDIT.md` | All | is the grammar one language across kinds? Audit against the fifteen rules, with the deliberate divergences recorded so nobody "fixes" them |
| `GC-MERGE-HANDOFF.md` | Core | pick-up-here state: what landed, what it disproved, ordered next steps |
| `BLUEPRINT-IR-STATUS.md` | Core, Move | the Blueprint IR itself |
| `UI-AUTHORING-DESIGN.md` | Core | the Widget IR. §26 is the current grammar; §5/§10/§21 describe the superseded one |
| `GRAPHCODE-MATERIAL-PLAN.md` | Core | the Material IR, and §11's open identity questions |
| `RUNTIME-DRIVE-DESIGN.md` | **Play** | the whole product. §5 the capture spec, §5.5 the five sources of diff noise, §6 the scenario runner, §12.0 pick-up-here |
| `WORLD-GC-DESIGN.md` | **World** | the level as text, and where it meets the capture. §10 the conformance audit, §11 the task list |
| `GRAPHCODE-EXTENDED-ASSET-SYNTAX-PROPOSAL.md` | **World** | Niagara (§4), placed actors (§6), timelines (§7), data families (§8). Non-authoritative except the implemented Actor subset |
