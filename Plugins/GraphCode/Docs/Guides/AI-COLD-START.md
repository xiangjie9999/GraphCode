# Cold-start GraphCode as an AI

Read this when you have entered a GraphCode project with no useful conversation history. It is a
route, not a second parameter manual. The installed tools and their generated capability registry
are newer than any remembered list.

If the user asked you to configure a newly installed project, start with [`SETUP.md`](../../SETUP.md)
instead. It covers involvement, project documentation, sidecars, Git, the first playable slice and
the setup stamp. Return here when setup is already complete and you need to do product work.

**Before the first line of a `.gcmove`, `.gcrig` or `.gcshape`, read its entry guide:
[`GCMOVE.md`](GCMOVE.md) (the animation index), [`GCRIG.md`](GCRIG.md) or [`GCSHAPE.md`](GCSHAPE.md)**, then every
guide and shipped example it lists for what your file does. A step a guide states is never skipped because the rig or
tool lacks something: add the missing bone, contact or range in the `.gcrig`, or report the tool's refusal.

The other kinds have entry guides too: [`GROOM.md`](GROOM.md) (`.gcgroom`), [`GARMENT.md`](GARMENT.md)
(`.gcgarment`), [`GCHUMAN.md`](GCHUMAN.md) (`.gchuman`: a MetaHuman identity, likeness, skin, face) and
[`EFFECTS.md`](EFFECTS.md) (a Niagara system); a Blueprint, level or other `.gc` asset starts at
[`TOOLS.md`](TOOLS.md). **Each opens with a "You want → write" table. Read it before you say GraphCode cannot
do something**: about half of the gaps reported from the field were a spelling that already existed. If the
table does not hold it, ask the tool (`gc_describe kind:"<kind>"` for the topic index, then the block), and
only then call it a gap, quoting the help answer that shows it.

**Before making game content or presentation, read [`ASSET-QUALITY.md`](ASSET-QUALITY.md).** It is
not optional: references first, a written plan, one representative part at final quality, then the
rest. A quick shape that only resembles the reference is a failed task, not a first draft.
Default to richer, purposeful detail that fits the whole theme unless simplicity is explicitly
requested. Straight edges and simple forms, including a bare sphere, need an explicit design reason.
Follow its repeated seam/detail render loop and the equivalent playback, listening and interaction
checks for animation, VFX, UI, audio and other media. Inspect every relevant phase, state and
transition; a successful build or one beauty shot is insufficient.
For realistic surfaces, plan [material-appropriate imperfections](ASSET-QUALITY.md#realistic-surfaces-carry-believable-imperfections)
and choose geometry, material+bake or decal+bake by the effect; verify the final output.

For game design or mechanics, read [DESIGNING.md](DESIGNING.md), then [GAMEPLAY.md](GAMEPLAY.md).
For game UI, read [UI-UX.md](UI-UX.md) before authoring widgets and [UI-TOOLS.md](UI-TOOLS.md)
when an outside design or browser tool would help. A polished UI needs designed states and
runtime interaction, not only a successful widget write. The
[worked slice](GAME-SLICE-EXAMPLE.md) shows how these parts become one playable experience.

**Importing from Blender or Blendkit (formerly BlenderKit)? Use GraphCode Integrate for static
meshes, their materials and managed reimport.** The AI handles setup and transfer. Follow
[Blender setup](BLENDER-SETUP.md) for required downloads, configuration and verification, then
the [Blender import workflow](AUTHORING.md#integrate-bring-an-external-source-across-a-boundary).

## Establish the truth before changing anything

**If any part of the task is unfamiliar or complex, follow
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md) before implementation.** Learn the underlying system,
research current primary sources and working examples, check installed capabilities, and save a
detailed plan with dependencies, implementation stages and acceptance evidence. Tool familiarity
alone does not establish subject knowledge. Before each step, resolve its remaining ambiguities
and record the approach and success check; do not spend production effort on an unresolved step.
Continue already authorized work after planning.

```text
cold start
├─ read the project's own agent instructions and current board
├─ inspect the environment with unreal_info
├─ ask each verb for {"help":true}; never recall a schema
├─ call gc_describe with no kind for the installed topic index
├─ call gc_describe for only the kind and block you need
├─ identify the exact asset, source state and owning product
└─ then read, plan, write and verify
```

The useful order of authority is:

1. **The connected build:** per-verb help, `gc_describe`, `unreal_info` and the current asset read.
2. **The generated inventory:** [`graphcode-capabilities.json`](../../Resources/mcp-bridge/graphcode-capabilities.json)
   when auditing or developing this plugin. It records product, kind, block access and creation.
3. **The guides:** workflow, safety, composition and evidence that should outlive a parameter name.
4. **Design and roadmap documents:** intent and planned work, never proof that a capability shipped.

An old example loses to a current refusal or capability entry. A missing registry entry is not
automatically planned work; it may be a program format, a tool capability or simply unsupported.
Use the [capability map](CAPABILITY-MAP.md) to distinguish those cases.

## Choose the subject before the verb

```text
What are you trying to change or learn?
├─ an Unreal asset or level
│  ├─ inspect it ................................ gc_read
│  ├─ learn its grammar or native type .......... gc_describe
│  ├─ find an asset, declaration or callable .... gc_search
│  ├─ create or change its authored state ....... gc_apply
│  └─ rename, move, copy or explicitly remove ... gc_asset
├─ an authored program
│  ├─ geometry ................................. .gcshape / Shape
│  ├─ spatial world ............................ .gcw / World
│  ├─ motion ................................... .gcmove / Move
│  ├─ character identity ....................... .gchuman / Core Extension
│  ├─ skeleton and skin intent ................. .gcrig / Rig
│  ├─ clothing ................................. .gcgarment / Garment
│  └─ hair ..................................... .gcgroom / Groom
├─ a file outside Unreal ......................... gc_import
├─ the running game .............................. gc_play
├─ appearance, a widget or graph layout .......... gc_render
├─ an authored action on a live object ........... gc_invoke
├─ a spatial question ............................ gcw_ask
├─ saved sidecar freshness ....................... gc_sync
├─ local history, comparison or recovery ......... gc_versions
└─ what the editor should show .................... gc_open
```

The MCP client may expose these with an `unreal_` prefix. `unreal_info` is the environment verb.
For the exact question each GraphCode verb answers, read [`TOOLS.md`](TOOLS.md).

## Learn a grammar without loading the whole system

Use the help tree in increasing depth:

```json
{ "help": true }
{ "help": "kind" }
{ "help": "kind", "kind": "blueprint" }
{ "help": ["mode", "kind"], "expanded": true }
```

Then use the description tree:

```text
gc_describe
└─ kind
   ├─ product and availability
   ├─ creation support
   ├─ block names and access
   └─ block
      ├─ syntax and identity
      ├─ omission/deletion policy
      ├─ entry specifiers
      ├─ example or full envelope
      └─ the write route
```

Ask `gc_search` for names that must already exist in the project. Ask `gc_describe type_path:` for
native reflected enums or structs. A plausible class, pin, property or enum spelling is not an
address. Do not invent one from an analogy.

The `.gcgroom` program is the current exception to the description tree because `kind:"groom"`
describes the reflective Groom asset. Use the tested recipe in [`GROOM.md`](GROOM.md), then ask
`gc_apply` for current validation and dry-run behaviour. The same word naming an asset kind does
not make the program and the projection interchangeable.

## The safe authoring loop

```text
identify -> read -> describe -> edit -> plan -> apply -> read back -> see/play -> save -> reload
```

1. **Identify the real target.** Record its full package or live object path, kind and product.
   A bare actor or object name can resolve to more than one instance.
2. **Choose one state.** `saved` is the file on disk. `live` is the editor, including unsaved
   changes. Read `live` before changing an asset another person may have open.
3. **Get a canonical source.** Preserve `sig:`, aliases, provenance and `omits:`. A diagnostic cut
   carrying `sig:none` or `non_canonical` is for inspection, never for apply.
4. **Describe the affected block.** Its identity and omission rules decide whether an absent line
   is preserved, reset, truncated, refused or deleted. There is no global deletion rule.
5. **Edit the source that owns the result.** Edit an existing sidecar or program file directly.
   For composed text or an asset with no sidecar, use the route the current `gc_apply` help names.
6. **Plan first.** Use dry-run, impact or a signed plan where that route offers one. Check the
   exact asset, block, graph, actors, referencers and any delete list.
7. **Apply once, then re-read `live`.** A refusal can still leave live editor state changed. Never
   stack a second attempt on an assumed rollback.
8. **Verify the claim, not only the call.** Compile where relevant. Render appearance. Drive
   runtime behaviour. Save, then read `saved`; unload/reload when durability is part of the claim.

For an existing asset, prefer the file route: edit the `.gc` beside it and call `gc_apply` with the
asset path. For a new supported asset, write a complete document based on the installed kind's
full example and use `gc_apply create:true`. Creation support is per kind; an editable existing
asset is not necessarily greenfield-creatable.

## The safety boundaries worth memorising

- **`omits:` is a loss boundary.** It names facets the text does not carry. Keep the binary asset
  and upstream sources wherever those facets matter.
- **An upstream recipe and a reflective `.gc` are different history.** `.gcshape`, `.gcmove`,
  `.gchuman`, `.gcrig`, `.gcgarment`, `.gcgroom` and `.gcw` declare intent. The built asset's `.gc`
  reflects the result and may name its recipe. Restore the source and output deliberately.
- **Deletion is explicit.** Verify the exact target and referencers. Asset removal is recoverable;
  never empty GraphCode's trash for the user.
- **Appearance and behaviour are separate proofs.** `gc_read` proves authored structure,
  `gc_render` shows appearance, and `gc_play` exercises a running game. One green surface does not
  certify the others.
- **A `.gcp` is runtime evidence, not the game's durable save format.** Read its omissions and
  `not_restored` result before treating capture/apply as exact recovery.
- **Do not learn a risky operation on the owner's only asset.** Use a new disposable design, then
  save, reload and compare before applying the lesson to valuable work.

## Load only the guide the job needs

```text
project or milestone
├─ unfamiliar or complex work in ANY area ............ RESEARCH-AND-PLAN.md
├─ ANY visible asset, before anything else .......... ASSET-QUALITY.md
├─ new installation ................................ SETUP.md
├─ production slice and build order ................. PRODUCTION.md
├─ verbs, read/write loop and tool evidence .......... TOOLS.md
├─ products and complete kind/block roster ........... CAPABILITY-MAP.md
├─ per-product authoring workflow ..................... AUTHORING.md
├─ generated 3D parts + Shape organic assembly ........ ORGANIC-PART-ASSEMBLY.md
├─ Human -> Rig -> Move and facial/body composition ... CHARACTERS.md
├─ a .gchuman: identity, likeness, skin, assembly ....... GCHUMAN.md
├─ a Niagara system: fire, smoke, sparks, magic ........ EFFECTS.md
├─ clothing fit, construction and simulation decisions  GARMENT.md
├─ gameplay ownership, persistence and networking ..... GAMEPLAY.md
├─ experience, decisions, progression and playtests ..... DESIGNING.md
├─ game UI, visual language, focus and accessibility ... UI-UX.md
├─ external UI tools and browser-to-Unreal handoff ..... UI-TOOLS.md
├─ one connected game-making example .................. GAME-SLICE-EXAMPLE.md
├─ robot decisions, EQS and route recovery ............ AI-NAVIGATION.md
├─ terrain, foliage, water and large environments ..... ENVIRONMENTS.md
├─ native Landscape brush workflow .................... NATIVE-TERRAIN.md
├─ hair recipe and appearance checks .................. GROOM.md
├─ asset provenance, quality and generation ........... CONTENT.md
├─ recurring authoring and verification lessons ........ LESSONS.md
├─ sidecars and exclusions ............................ SETTINGS.md
├─ Git and merge recovery ............................. GIT.md
├─ shared editor/build coordination ................... TEAM.md
└─ milestone, package and performance evidence ........ VALIDATION.md
```

The linked index for every leaf is [`README.md`](README.md); the tree above is the fast route by
question.

Start at one leaf. Do not eagerly load every guide or turn a cold start into a documentation sweep.
When the work crosses products, add the next leaf at the boundary—for example, open
[`CHARACTERS.md`](CHARACTERS.md) when a Human identity, rig, body animation and facial performance
must become one character, rather than trying to infer that composition from four grammars.
