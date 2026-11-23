# GraphCode capability and product map

Use this map to choose a product, kind and source document. It deliberately does not duplicate
parameter schemas or block grammar. Ask the installed verb for `{ "help": true }`, then ask
`gc_describe` for the current kind and block before authoring.

This roster is grounded in the generated
[`graphcode-capabilities.json`](../../Resources/mcp-bridge/graphcode-capabilities.json) in this
checkout. The generated file and connected build remain authoritative if this guide and a newer
installation disagree.

## Read the status words literally

| Word | Meaning |
|---|---|
| **Shipped** | The current generated registry or advertised tool surface has an implementation. It still needs the owning product and any engine/plugin dependency. |
| **Read-only** | The projector carries this state, but no writer exists for that kind or block. It is useful evidence, not valid apply input. |
| **Conditional** | The implementation exists but depends on an installed product, a live editor, a compatible asset/class, or another condition named by help. |
| **Existing only** | A writer exists for an asset already present, but `gc_apply create:true` does not create that kind. Import or an upstream program may be its creation route. |
| **Planned** | Current help or the active project board explicitly marks it as future work. A design document, absent kind or remembered proposal does not make it shipped or planned. |

Block legend below: an unmarked block is writable; **`[R]`** is read-only; **`[W?]`** is writable
only when the asset meets the registry's condition. `Create` means greenfield creation through
`gc_apply create:true`; `no` does not make the existing asset read-only. Always ask the block for
its omission policy—`refuses`, `preserves`, `deletes`, `truncates`, `fixed` and reset-to-default are
materially different writes.

## The product tree

```text
GraphCode
├─ Core — reflective assets and the level ......................... .gc
├─ Play — the running game ........................................ .gcp / gc_play
├─ Move — animation, time, effects and decisions .................. .gc + .gcmove
├─ World — spatial/environment systems ............................ .gc + .gcw + .gcs
├─ Core Extension (Advanced) — optional engine frameworks ......... .gc + .gchuman
├─ Shape — procedural geometry intent ............................. .gcshape
├─ Integrate — supported external-source translation/import ....... gc_import
├─ Rig — skeleton, skin and control-rig intent ..................... .gcrig + .gc
├─ Garment — clothing construction intent ......................... .gcgarment
└─ Groom — hair construction plus Groom projections ............... .gcgroom + .gc
```

The level is **Core**, not World. Core owns the container and actor placement. World adds spatial
regions, terrain, water, foliage, navigation and PCG systems used by that level.

Optional products remain visible in discovery so a missing install can refuse by name. Therefore
"listed" does not mean "installed here." Check `unreal_info`, the product fields returned by
`gc_describe`, and `available` before promising a path.

## Core — asset and level authoring

Core is required. Its `.gc` is a deterministic projection of an Unreal asset; `omits:` states what
that projection does not carry. Core currently registers these kinds:

| Kind | Create | Blocks in the generated registry |
|---|---:|---|
| `blueprint`, `blueprint_interface`, `blueprint_function_library`, `blueprint_macro_library` | yes | `settings`, `defaults [R]`, `properties`, `inherited [R]`, `interfaces`, `dispatchers`, `timelines`, `class_defaults [W?]`, `components [W?]`, `graph`, `functions`, `macros` |
| `widget_blueprint` | yes | the Blueprint blocks above plus `widgets`, `animations` |
| `animation_blueprint` | yes | the Blueprint blocks above plus `animgraph` |
| `user_struct` | yes | `settings`, `properties` |
| `user_enum` | yes | `settings`, `entries` |
| `material_instance` | yes | `settings`, `parameters` |
| `material`, `material_function` | yes | `settings`, `graph`, `comments` |
| `data_asset` | yes | `values` |
| `level` | yes | `partition [W?]`, `actors`, `objects`, `sublevels`, `volumes`, `splines`, `data_layers [R]`, `level_blueprint` |
| `data_table` | yes | `import [R]`, `settings`, `rows` |
| `string_table` | yes | `settings`, `strings` |
| `input_action` | yes | `settings`, `triggers`, `modifiers`, `player_mappable` |
| `input_mapping_context` | yes | `settings`, `mappings`, `mapping_profiles` |
| `curve_float`, `curve_vector`, `curve_linear_color` | yes | `import [R]`, `settings`, `curves` |
| `curve_table` | yes | `import [R]`, `settings`, `curves` |
| `skeletal_mesh`, `static_mesh` | no | `import`, `pipelines`, `settings`, `materials`, `sockets` |
| `texture_2d`, `sound_wave` | no | `import`, `pipelines`, `settings` |
| `sound_cue` | yes | `settings`, `soundcue` |
| `external_actor` | no | `actors`, `data_layers [R]` |
| `font` | yes | `settings`, `typeface`, `font_fallbacks` |
| `font_face` | no | `settings` |
| `foliage_type`, `physical_material`, `hlod_layer`, `render_target`, `data_registry` | yes | `values` |
| `import` | no | `import [R]`, `pipelines [R]`, `settings [R]`, `factory [R]`, `members [R]` |

`kind:import` describes an external source/archive as a file; `gc_import` is the operation that
creates or updates supported Unreal assets from one. Meshes, textures, sound waves and font faces
are existing-only through `.gc`, but can have an import or upstream-program creation path.

## Move — motion, effects and decisions

Move owns time-based and decision assets, plus the upstream `.gcmove` animation program.

| Kind | Create | Blocks in the generated registry |
|---|---:|---|
| `skeleton` | no | `settings`, `retargeting`, `virtual_bones`, `sockets`, `slot_groups`, `blend_profiles`, `notifies` |
| `physics_asset` | yes | `settings`, `bodies`, `constraints`, `collision_pairs` |
| `ik_rig` | yes | `settings`, `goals`, `retarget_definition`, `solvers` |
| `ik_retargeter` | yes | `settings`, `source_poses`, `target_poses`, `ops`, `profiles` |
| `anim_sequence` | no | `import`, `pipelines`, `settings`, `curves`, `notify_tracks`, `notifies`, `sync_markers` |
| `anim_montage` | yes | `settings`, `slots`, `sections`, `curves`, `notify_tracks`, `notifies` |
| `anim_composite` | yes | `settings`, `track`, `curves`, `notify_tracks`, `notifies` |
| `blend_space` | yes | `settings`, `axes`, `samples` |
| `level_sequence` | yes | `sequence` |
| `behavior_tree` | yes | `behaviortree` |
| `state_tree` | yes | `statetree` |
| `niagara_system` | yes | `settings`, `user_parameters`, `system`, `emitters` |
| `niagara_parameter_collection` | yes | `settings`, `parameters` |
| `niagara_sim_cache` | yes | `capture`, `frames [R]`, `system [R]`, `emitters [R]` |

For a Niagara system (`niagara_system`, `niagara_parameter_collection`) read [`EFFECTS.md`](EFFECTS.md) first: its
"You want → write" table holds the spellings (`set(…)`, forces with Solve Forces and Velocity, user parameters) and
how to see an effect.

`gc_describe kind:"move"` exposes the shipped `.gcmove` program. It builds animation sequences
against a named skeleton; the built sequence's `.gc` reflects settings, curves, notifies and its
program provenance rather than duplicating bone animation. Use [`CHARACTERS.md`](CHARACTERS.md)
when body animation, facial performance and character identity must be composed.

## Play — the running game

Play intentionally has no asset kinds in the capability registry. Its shipped surface is
`gc_play`, whose current modes cover runtime status and driver lease, start/stop, execution,
capture, diff, apply, follow, collect, compare and process recovery through a signed plan. Ask
`gc_play` for mode help because several modes are writes and the merged tool is marked destructive.

`.gcp` captures live reflected state under `Saved/`; they carry their own omissions. Capture/diff
is evidence, and apply is a bounded runtime restore. Neither is proof of timers, latent actions,
animation blend position, audio playheads or the project's durable save-game design. Treat those
as separate gameplay capabilities and validate them through [`GAMEPLAY.md`](GAMEPLAY.md) and
[`VALIDATION.md`](VALIDATION.md).

## World — authored space and environment systems

World owns the `.gcw` region/environment program, `.gcs` spatial data and `gcw_ask`. Its reflective
asset kinds are PCG:

| Kind | Create | Blocks in the generated registry |
|---|---:|---|
| `pcg_graph` | yes | `settings`, `parameters`, `graph`, `comments [R]` |
| `pcg_graph_instance` | yes | `settings`, `parameters` |
| `pcg_settings` | yes | `settings` (one complete native typed value, shared by graph references) |

Use [`ENVIRONMENTS.md`](ENVIRONMENTS.md) for terrain, water, foliage, caves and World Partition, and
[`AI-NAVIGATION.md`](AI-NAVIGATION.md) when spatial state feeds a decision and moving pawn. A valid
`.gcw` apply does not by itself prove grounding, navigation, streaming or the camera-visible result.

## Core Extension / Advanced — optional engine frameworks

This product contains integrations that exist only when their engine plugin/framework is present.
The product token is `extension`; the module/plugin name is `GraphCodeCoreExtension`.

| Kind | Create | Blocks in the generated registry |
|---|---:|---|
| `env_query` | yes | `envquery` |
| `procedural_vegetation` | yes | `settings`, `parameters`, `graph`, `comments [R]` |
| `metasound_source` | yes | `settings`, `interfaces`, `inputs`, `outputs`, `variables`, `metasound` |
| `metasound_patch` | yes | `interfaces`, `inputs`, `outputs`, `variables`, `metasound` |
| `metahuman` | no | `settings [R]`, `face`, `skin`, `eyes`, `makeup`, `head`, `wardrobe [R]`, `outfit`, `textures [R]`, `pipeline [R]` |

For native trees, [VEGETATION.md](VEGETATION.md) covers PVE plugin setup, a complete `.gc` example,
BPIR organisation and direct `gc_render` preview. Mesh baking and World placement are separate.

`gc_describe kind:"human"` exposes the shipped `.gchuman` identity program; `kind:"metahuman"`
describes its reflective built asset. They are different documents. Read [`GCHUMAN.md`](GCHUMAN.md) before writing a
`.gchuman`: its "You want → write" table says which of the two documents each setting belongs in. Follow
[`CHARACTERS.md`](CHARACTERS.md) for the Human → Rig → Move pipeline and the current boundary
between shipped facial/body composition and work that remains proposed.

## Shape, Integrate, Rig, Garment and Groom

Some products are program-first, so zero asset kinds is not zero capability.

| Product | Shipped authoring surface | Reflective kinds |
|---|---|---|
| **Shape** | `gc_describe kind:"shape"`; `.gcshape` builds procedural mesh assets through `gc_apply` | none owned by Shape; built meshes project as Core `static_mesh` and record provenance |
| **Integrate** | `gc_import` and its installed source/material translation routes | none; imports become the owning product's asset kind |
| **Rig** | `gc_describe kind:"rig"`; `.gcrig` builds skeleton/skin/control assets | `control_rig` |
| **Garment** | `gc_describe kind:"garment"`; `.gcgarment` builds clothing through `gc_apply` | none in the current registry |
| **Groom** | `.gcgroom` builds native Groom and binding assets; use [`GROOM.md`](GROOM.md) for the current program recipe | `groom`, `groom_binding` |

Rig and Groom's reflective blocks are:

| Kind | Create | Blocks in the generated registry |
|---|---:|---|
| `control_rig` | yes | `settings`, `rig [R]`, `hierarchy [R]`, `properties [W?]`, `graph [W?]` |
| `groom` | no | `settings`, `groups [R]`, `group_materials`, `rendering`, `physics`, `interpolation`, `lods`, `cards`, `meshes`, `provenance [R]` |
| `groom_binding` | no | `binding [R]`, `groups [R]`, `provenance [R]` |

`control_rig` creates native rigs and writes settings, member variables and supported RigVM graphs.
Changed `properties` and `graph` blocks validate together; unsupported native graph content is
preserved by patch and prevents snapshot replacement. Prepared setup and hierarchy remain read-only.
See [Control Rig authoring](CONTROL-RIG.md) for `.gcrig` preparation, header `override:true|false`,
per-control protection and preservation of unrelated functions. The downstream `program` block is
retired. `groom_binding` is wholly read-only through its reflective `.gc`; its upstream Groom recipe
owns construction.

## Composition routes

```text
Need a complete character?
├─ identity source
│  ├─ existing native MetaHuman .......... kind:metahuman / Core Extension
│  └─ clean 3D-AI body/head mesh
│     ├─ supplied as one mesh ............. import → inspect
│     ├─ generated as parts ............... import → Shape organic assembly
│     │  └─ workflow ...................... ORGANIC-PART-ASSEMBLY.md
│     └─ native result wanted ............. .gchuman reference fit → MetaHuman
├─ custom body that must remain custom ... Core import or .gcshape → .gcrig
├─ bones, skin, sockets and controls ...... .gcrig / Rig
├─ clips and body motion .................. .gcmove / Move
├─ facial performance ..................... Human/MetaHuman route named by CHARACTERS.md
├─ clothing ............................... .gcgarment / Garment
├─ hair ................................... .gcgroom / Groom
└─ runtime proof .......................... gc_play + render/capture
```

```text
Need a complete environment?
├─ generated object geometry .............. .gcshape / Shape
├─ imported object source ................. gc_import / Integrate or Core
├─ actor placement and level structure .... kind:level / Core
├─ regions, terrain, water and foliage .... .gcw / World
├─ PCG graph and parameters ............... kind:pcg_graph / World
├─ navigation and decision assets ......... World + Move + extension EQS
└─ appearance and traversal proof ......... gc_render + gc_play
```

The owning source is where a correction belongs. Do not hand-edit a generated mesh to fix a Shape
recipe, or key an animation output to fix a Move program, unless the workflow explicitly transfers
ownership and records that decision.

## Planned, unsupported and unverified are not synonyms

This guide freezes no roadmap list. Planned work changes more quickly than a user guide and belongs
on the active board or in an explicit `proposed` answer from current help. In particular:

- a product with no reflective kinds may be program-first or tool-first, as Shape, Play, Integrate
  and Garment are above;
- an existing-only kind may have an import or upstream-program creation route rather than a missing
  `create:true` implementation;
- a read-only block is a measured boundary, not permission to invent another writer;
- a missing optional product is unavailable in this installation, not unfinished globally; and
- a requirement that has not been exercised in the target project is **unverified**, even when its
  underlying tool is shipped.

Use the exact current word in reports. Only call a feature **planned** when the installed help or
active board does.

## Validation and recovery tree

```text
What went wrong?
├─ call refused or apply failed
│  ├─ read the structured refusal
│  ├─ re-read source:"live"; a failed write may have changed it
│  └─ correct the source, dry-run again, then apply once
├─ text and asset disagree
│  ├─ check saved versus live
│  ├─ inspect sig:, bin:, omits: and provenance
│  └─ gc_sync only when the goal is sidecar freshness
├─ appearance is wrong
│  ├─ gc_render the intended subject and camera
│  └─ inspect the real GPU/runtime view for materials, hair and motion
├─ behaviour is wrong
│  ├─ gc_play the actual path, including failure and retry
│  └─ capture action, result and final state—not only a started session
├─ need an earlier asset state
│  ├─ gc_versions list/compare first
│  ├─ restore only after inspecting its signed plan
│  └─ re-read and re-run the affected proof
├─ asset removed
│  └─ recover from GraphCode's package/recovery trash; never empty it for the user
└─ authored recipe changed
   └─ recover the recipe from file/Git history separately, then rebuild and verify its outputs
```

Use [`VALIDATION.md`](VALIDATION.md) to match evidence to the claim and [`GIT.md`](GIT.md) for merge
and authored-file recovery. A successful parser, apply, compile, render or play call proves only the
surface it actually observed.

## How to report a genuine gap

Before calling something missing, record:

1. installed product and engine/plugin dependency;
2. `gc_describe` kind/block status and creation field;
3. the verb's current help and exact refusal;
4. whether the needed state belongs to an upstream program, reflective `.gc`, live runtime or
   external source;
5. the smallest disposable fixture that reproduces the gap.

Call it **planned** only when current help or the active board does. Otherwise use **unsupported**,
**read-only**, **existing-only**, **product unavailable** or **not yet verified**—whichever the
evidence actually says.
