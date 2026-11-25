# What GraphCode can do, what it cannot, and the workaround

This is the one page for "can GraphCode do this?". For each product it says what you can make, what the
product does not do, and what to do instead. It also maps products to file kinds and source documents.
It never copies parameter schemas or block grammar: ask the installed verb for `{ "help": true }`, then
`gc_describe` for the current kind and block before authoring.

GraphCode 0.1.15 runs on Unreal Engine 5.7 and 5.8 (one download per engine) on 64-bit Windows. Core,
Shape and Integrate are free for personal use; the trial of every product lasts 21 days. Setup is in
[SETUP.md](../../SETUP.md).

Before you tell anyone "GraphCode cannot do that", read the entry guide's "You want → write" table and ask
the tool ([GRAPHCODE.md](../../GRAPHCODE.md) §2): most reported gaps turned out to be a spelling that
already existed. Where this page and the installed build disagree, the installed build wins; the generated
[`graphcode-capabilities.json`](../../Resources/mcp-bridge/graphcode-capabilities.json) is its inventory.

## The questions asked first

| You want | Answer | What to do |
|---|---|---|
| A creature, character, animal or any sculpted organic form | Not Shape's strength. Shape builds parts by construction (profiles, lofts, joins, brushes), not free sculpting of a whole body | Generate the mesh with an AI image or text-to-3D model, import it, clean it with the Market guide **GraphCode/CleanGeneratedMesh**, then rig it with `.gcrig` and animate it with `.gcmove` ([details](#organic-objects-and-characters-the-generated-model-route)) |
| A MetaHuman person | Yes: `.gchuman` fits a native MetaHuman identity; body and face stay native | [GCHUMAN.md](GCHUMAN.md), [CHARACTERS.md](CHARACTERS.md) |
| A hard-surface prop, building, vehicle, weapon, plant kit | Yes, this is Shape | [GCSHAPE.md](GCSHAPE.md); search the Market first |
| A texture, painting, icon or concept image | Not a mesh or `.gc` job; textures are imported | Make the picture with an image generator or your art tool, then `gc_import` it; Shape takes SVG outlines and decals ([ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md)) |
| Voice, sound effects or music | Core authors Sound Cues and MetaSounds; the audio itself comes from outside | Generate or record it (ElevenLabs is the preferred service, [CONTENT.md](CONTENT.md)), import the wave, wire it with `.gc` |
| An animation from a Blender rig or a skeleton | Integrate sends static meshes only | Import the skeletal mesh and animation through Core `gc_import`, or rebuild the skeleton in `.gcrig` and the motion in `.gcmove` |
| A cave or an overhang in terrain | A landscape heightfield cannot form one | Build the cave as real geometry with Shape and place it with World ([ENVIRONMENTS.md](ENVIRONMENTS.md)) |
| Whether a game is fun | A scripted scenario proves correctness, not enjoyment | Play drives and records the run; the person plays and judges |

## How to read the status words

| Word | Meaning |
|---|---|
| **Shipped** | The registry or tool surface has an implementation. It still needs the owning product and any engine plugin. |
| **Read-only** | The projector carries this state but no writer exists. Useful evidence, not apply input. |
| **Conditional** | Works when an installed product, a live editor or a compatible asset is present. |
| **Existing only** | A writer exists for an asset already present; `gc_apply create:true` does not create it. Import is its creation route. |
| **Planned** | Current help or the active board says so. A design document is not a promise. |

Block legend in the kind tables: an unmarked block is writable, **`[R]`** is read-only, **`[W?]`** is
writable only when the asset meets a condition. Ask the block for its omission policy before any
whole-block write.

## Core: assets and the level

**You can make.** Blueprints (including Gameplay Ability System assets), widgets, animation Blueprints,
materials and material functions, data tables, data assets, curves, input actions and mapping contexts,
structs and enums, string tables, Sound Cues, fonts, and the level with the actors, volumes, splines and
sublevels in it.

| Does not | Workaround |
|---|---|
| Carry what a projection omits. The `.gc` `omits:` line names the facets it does not cover | Read `omits:` first; apply only canonical text with a current `sig`; edit what is omitted in the editor |
| Create meshes, textures, sound waves or font faces (existing-only) | Import them with `gc_import`; or build meshes with Shape |
| Write the read-only blocks (`defaults`, `inherited`, `data_layers`, `import`, and the others marked **[R]**) | Treat them as evidence; change the owning source |
| Prove that a successful write looks or plays right | Render it, play it, save and reload ([VALIDATION.md](VALIDATION.md)) |

| Kind | Create | Blocks in the generated registry |
|---|---:|---|
| `blueprint`, `blueprint_interface`, `blueprint_function_library`, `blueprint_macro_library` | yes | `settings`, `defaults [R]`, `properties`, `inherited [R]`, `interfaces`, `dispatchers`, `timelines`, `class_defaults [W?]`, `components [W?]`, `graph`, `functions`, `macros` |
| `widget_blueprint` | yes | the Blueprint blocks plus `widgets`, `animations` |
| `animation_blueprint` | yes | the Blueprint blocks plus `animgraph` |
| `user_struct`, `user_enum` | yes | `settings` with `properties` or `entries` |
| `material`, `material_function` | yes | `settings`, `graph`, `comments` |
| `material_instance` | yes | `settings`, `parameters` |
| `data_asset`, `foliage_type`, `physical_material`, `hlod_layer`, `render_target`, `data_registry` | yes | `values` |
| `level` | yes | `partition [W?]`, `actors`, `objects`, `sublevels`, `volumes`, `splines`, `data_layers [R]`, `level_blueprint` |
| `data_table`, `string_table` | yes | `import [R]` (tables), `settings`, `rows` or `strings` |
| `input_action`, `input_mapping_context` | yes | `settings`, `triggers`, `modifiers`, `player_mappable`, `mappings`, `mapping_profiles` |
| `curve_float`, `curve_vector`, `curve_linear_color`, `curve_table` | yes | `import [R]`, `settings`, `curves` |
| `sound_cue`, `font` | yes | `settings` with `soundcue`, or `typeface` and `font_fallbacks` |
| `skeletal_mesh`, `static_mesh`, `texture_2d`, `sound_wave`, `font_face`, `external_actor`, `import` | no | import, pipelines, settings, materials, sockets, actors |

`kind:import` describes an external source as a file; `gc_import` creates or updates supported assets from
one. The level is Core, not World: Core owns the container and actor placement.

## World: places

**You can make.** Terrain (mountains, valleys, rivers, erosion, caves as geometry, streamed open worlds),
water, forests, grass and plants spread over land, roads, plots and buildings at the right heights,
navigation, and PCG graphs. World owns the `.gcw` region program, the `.gcs` spatial data and `gcw_ask`.

| Does not | Workaround |
|---|---|
| Form caves or overhangs from a heightfield | Real cave geometry (Shape) with collision tested by the player capsule |
| Simulate waves or shores: `erode` is a downhill-droplet and thermal-settling pass that exports no sediment or wetness mask | Put beach grade and deposits in geometry; wet colour and small relief in the material; plan shared masks |
| Prove grounding, navigation, streaming or the camera view from a valid `.gcw` apply | Render headwater, middle and outlet; walk the route with Play |
| Turn an imported tree mesh into an editable native species (Procedural Vegetation Editor, experimental in 5.7 and 5.8) | Customize a native preset, or place the imported tree as foliage or a Shape part ([VEGETATION.md](VEGETATION.md)) |
| Run native Landscape erosion just by placing the brush | Bind and invoke it as [NATIVE-TERRAIN.md](NATIVE-TERRAIN.md) says; enable the Unreal plugin that supplies the brush |
| Add Water's collision profile by enabling the plugin | Set the profile as the [water setup](NATIVE-TERRAIN.md#water-project-setup) says |

| Kind | Create | Blocks |
|---|---:|---|
| `pcg_graph` | yes | `settings`, `parameters`, `graph`, `comments [R]` |
| `pcg_graph_instance` | yes | `settings`, `parameters` |
| `pcg_settings` | yes | `settings` (one complete native value) |

Guides: [ENVIRONMENTS.md](ENVIRONMENTS.md), [WATER.md](WATER.md), [VEGETATION.md](VEGETATION.md),
[ROADS-AND-BUILDINGS.md](ROADS-AND-BUILDINGS.md), [AI-NAVIGATION.md](AI-NAVIGATION.md).

## Play: testing the running game

**You can do.** Start and stop a session, drive it (buttons, HUD, input), take screenshots, capture live
reflected state to a `.gcp`, diff and compare runs, and apply a bounded runtime restore. Play has no asset
kinds; its surface is `gc_play`, and several modes write.

| Does not | Workaround |
|---|---|
| Treat a capture as proof of timers, latent actions, animation blend position, audio playheads or a game's save design | Validate each through [GAMEPLAY.md](GAMEPLAY.md) and [VALIDATION.md](VALIDATION.md); save, reload, then package |
| Run more than one driver per editor (one lease) | Release the lease before another attach |
| Drive play from editor Python | Use the supported `gc_play` route |
| Declare a mechanic enjoyable | The person plays it; Play supplies the evidence |

## Shape: things

**You can make.** Props, furniture, buildings, machines, vehicles and weapons part by part; stone walls,
roofs, timber and floors as kit modules; plants, leaves, ivy and leaf litter; wear and damage (chips,
cracks, broken wood, rubble); game-ready meshes with material slots, sockets, collision and detail levels;
designed contours from SVG; soft surface features (a nose, a cheek, bark) with texture brushes and
displacement. A `.gcshape` is a recipe: the mesh is built from it, and Market packs ship the recipe.

| Does not | Workaround |
|---|---|
| Sculpt a whole organic body (creature, animal, character) from nothing | Generate it, import it, clean it ([the route below](#organic-objects-and-characters-the-generated-model-route)); use Shape for the parts around it |
| Cut precise features (pockets, bores, holes) into a generated surface | Build the interface natively against the imported part's real size |
| Make textures or paint colour from an algorithm | Generate the artwork elsewhere; bring it in as SVG outlines, decals and masks ([ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md)) |
| Build every thin curved plate: one route works (open profile, `revolve(capped=false)`, then `shell`) | Use that route; read the build line; `volume` must be positive ([ASSET-QUALITY.md](ASSET-QUALITY.md)) |
| Let you fix a recipe by hand-editing its built mesh | Change the `.gcshape` and rebuild |
| Skin or animate | `.gcrig` and `.gcmove` own that |

Shape owns no reflective kind: a built mesh projects as a Core `static_mesh` with its provenance.

## Move: motion, effects and decisions

**You can make.** Character animation (walking, running, climbing, reaching, fighting, holding things),
animation montages and composites, blend spaces, retargeters and IK rigs, physics assets, level sequences
and camera moves, Niagara effects (fire, smoke, magic, explosions, portals), Behavior Trees and State
Trees, and offline MetaHuman speech from a recorded voice.

| Does not | Workaround |
|---|---|
| Make a gait for you: you draw the legs, the solver keeps the feet planted | Start from the shipped walk and run examples; judge contact in a side render at 60 fps ([WALK-AND-RUN.md](WALK-AND-RUN.md)) |
| Adapt a baked clip to a level's stairs at runtime | `constraints(collision=world)` for runtime correction, Stride Warping, or a clip authored for the treads ([STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md)) |
| Find a tread edge: plants use the heights you typed | Type one `ground()` per tread; check the side view at every landing |
| Have an expression verb | The FaceExpressions adapter: `drive(smile, by=...)` for `smile`, `blink`, `jaw_open` ([FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md)) |
| Author body and face tracks on two skeletons in one program, or solve speech in real time | Separate Body and Face clips; offline speech through `clip(speech=...)` ([CHARACTERS.md](CHARACTERS.md) §11) |
| Time a different effort for stretches inside one stage | Write them the same, or make the pose between them a rest ([MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md)) |
| Give a quadruped an open mouth, tongue, ears or claws on the test cat's rig | Add those joints in your own `.gcrig` first ([QUADRUPED-PRESETS.md](QUADRUPED-PRESETS.md)) |
| Bend a joint past its real range to reach | Change the pose, the contact or the rig; never the limit ([RIG.md](RIG.md)) |

| Kind | Create | Blocks |
|---|---:|---|
| `skeleton` | no | `settings`, `retargeting`, `virtual_bones`, `sockets`, `slot_groups`, `blend_profiles`, `notifies` |
| `physics_asset` | yes | `settings`, `bodies`, `constraints`, `collision_pairs` |
| `ik_rig`, `ik_retargeter` | yes | goals, solvers, source and target poses, ops, profiles |
| `anim_sequence` | no | `import`, `pipelines`, `settings`, `curves`, `notify_tracks`, `notifies`, `sync_markers` |
| `anim_montage`, `anim_composite`, `blend_space` | yes | slots, sections, track, axes, samples, curves, notifies |
| `level_sequence`, `behavior_tree`, `state_tree` | yes | `sequence`, `behaviortree`, `statetree` |
| `niagara_system`, `niagara_parameter_collection` | yes | `settings`, `user_parameters`, `system`, `emitters`, `parameters` |
| `niagara_sim_cache` | yes | `capture`; `frames`, `system`, `emitters` are **[R]** |

`gc_describe kind:"move"` exposes the `.gcmove` program: it builds sequences against a named skeleton.
Niagara: read [EFFECTS.md](EFFECTS.md) first. Animation index: [GCMOVE.md](GCMOVE.md).

## Rig: bodies that move

**You can make.** Skeletons, skin weights, physics assets and IK rigs over a Shape or imported mesh,
including a bare animal mesh (Route B); hands that grip and feet that roll; sockets, contacts and grips;
morphs that follow a joint; native Control Rigs; a small adapter for a MetaHuman.

| Does not | Workaround |
|---|---|
| Rebuild a native MetaHuman body or face | Use `.gchuman` and the adapter ([METAHUMAN-RIG.md](METAHUMAN-RIG.md)) |
| Preserve an already-rigged arbitrary custom mesh through a rebuild | Route A keeps the source skeleton: read its hierarchy and extend it |
| Guess coordinates | Measure every point with `gc_render mode:"ask"` on the mesh |
| Ship an animal: none is shipped, defaults fit the test cat | Write your animal's joint lines from measured points and the real animal's ranges |
| Reach with a joint bent beyond life | The rig gets a bone, contact or socket; the pose changes ([GCRIG.md](GCRIG.md)) |

`control_rig` is the one reflective kind (create yes; `rig` and `hierarchy` **[R]**, `properties` and `graph`
**[W?]**); see [CONTROL-RIG.md](CONTROL-RIG.md).

## Groom: hair

**You can make.** Hair, fur, eyebrows and stubble, styled and combed; braids, buns and ties; short animal
fur as shells; anime hair as solid locks and cards; stylised and realistic feathers; level-of-detail chains.

| Does not | Workaround |
|---|---|
| Fold or animate a wing, or grow body feather patches | Use a wing as a static piece on a flat base ([FEATHERS.md](FEATHERS.md)) |
| Lay feather rows along a curved base | Grow them on a flat slab, the base that is built |
| Build cards on their own | Declare them through a `lods(...)` path; solid anime locks build alone ([ANIME-HAIR.md](ANIME-HAIR.md)) |
| Use `distribution=relaxed` | Use `jittered` |
| Keep hair alive in a MetaHuman assembly without a level-of-detail chain | Write the `lods(...)` chain, then assemble and look for the hair ([GROOM.md](GROOM.md)) |
| Own interpolation and simulation: Unreal does | Set them on the Groom in Unreal |

`groom` (create no) and `groom_binding` (create no, read-only `.gc`) are the reflective kinds; the recipe
owns construction.

## Garment: clothing

**You can make.** Shirts, trousers, jackets, coats, dresses, cloaks and scarves cut against a named body;
construction detail (pockets, collars, hems, seams, stitches, patches); armour as padded layers, leather
and plates; static, skinned or render-only outfits; cloth that hangs, with build-time settling and runtime
Chaos cloth (an optional plugin).

| Does not | Workaround |
|---|---|
| Be a complete tailoring suite: arbitrary knots, frictional locking, collision-free cloth through every animation, every crop, slant, ruffle or pleat | Keep to what installed help names; a refusal is a boundary, so report the gap instead of approximating ([GARMENT.md](GARMENT.md)) |
| Move cloth through skinning alone | `[as=cloth]` with `hang()` for simulated cloth; test each cloth and armour pairing in motion |
| Fit a plate to a body region, or place one part on a ring or cut edge | Plates fit the cloth under them; copies go over the cloth |
| Turn imported clothing into a parametric design | Keep the source and licence; fit it, or author the garment |
| Change the body | The body is an input; change it in `.gchuman` or the mesh |

## Core Extension: people and frameworks

**You can make.** MetaHuman identities from `.gchuman` (any face and body, a likeness from a drawing, scan
or generated head), crowds, MetaSound sources and patches, Environment Queries, procedural vegetation.
These exist only when the engine plugin is enabled: ask `unreal_info`, then enable the one framework the
project needs.

| Does not | Workaround |
|---|---|
| Hold expressions, raw DNA edits or custom rigs inside a `human {}` body | Expressions: the FaceExpressions adapter; rigs: `.gcrig` ([GCHUMAN.md](GCHUMAN.md)) |
| Reconstruct every clothed body dimension from a reference | Give explicit landmarks and height; check proportion and fit separately |
| Keep a generated mesh's topology, UVs or skeleton when fitting a MetaHuman | Choose `.gcrig` when those must survive ([CHARACTERS.md](CHARACTERS.md)) |
| Write `wardrobe` (read-only on the built asset) | Dress the character with Garment |

| Kind | Create | Blocks |
|---|---:|---|
| `env_query` | yes | `envquery` |
| `procedural_vegetation` | yes | `settings`, `parameters`, `graph`, `comments [R]` |
| `metasound_source`, `metasound_patch` | yes | interfaces, inputs, outputs, variables, `metasound` |
| `metahuman` | no | `face`, `skin`, `eyes`, `makeup`, `head`, `outfit` writable; `settings`, `wardrobe`, `textures`, `pipeline` **[R]** |

## Integrate: Blender

**You can do.** Bring Blender static meshes and their materials into Unreal, update them in place, and use
Blendkit generators and materials; the AI downloads, configures and verifies the connection itself
([BLENDER-SETUP.md](BLENDER-SETUP.md)).

| Does not | Workaround |
|---|---|
| Send skeletons, animation or level placement; armature-deformed meshes are refused | Import skinned content through Core `gc_import`, or rebuild it with `.gcrig` and `.gcmove` |
| See unsaved changes in an open Blender scene from a saved-file export | Save first, or use a live Blender connection |
| Convert a Blender shader exactly | Read every conversion loss, compare renders with the source |
| Replace assembly: do not join character parts in Blender | Assemble in a `.gcshape` ([ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md)) |
| Accept a non-1 Unit Scale | Prepare a copy at Unit Scale 1 (one unit = 100 cm) |

## Market: ready-made recipes (free)

**You can do.** Search, describe and install packs of recipes (`.gcshape`, `.gcmove`, `.gcrig`,
`.gcgroom`, `.gcgarment`, `.gc`), guides and agent skills with `unreal_gc_market`. Search before you author
([MARKET.md](MARKET.md)).

| Does not | Workaround |
|---|---|
| Ship meshes or binaries (only an optional texture set of PNG images) | Build the pack's example assets by running its main file through `gc_apply` |
| Build on install or install two versions of a pack | Install writes text only; one version per project |
| Install from a link alone | The link opens the install confirmation; press Install in the Market tab |

## Organic objects and characters: the generated-model route

Shape is a construction tool, so a creature, an animal or a face is not made by stacking primitives.
Use a generated model as the starting point and GraphCode as the finishing line:

1. **Make the reference and the model.** Write the reference image and prompt down, then generate the
   model with an AI image or text-to-3D service (Tripo, Meshy and Rodin are the ones the guides name).
   Split a character into parts if it must be assembled. [TRIPO3D.md](../Historical/TRIPO3D.md) records one service's
   request settings (geometry-only, face limit, UV export).
2. **Import it.** `gc_import` brings the FBX or GLB in as a static mesh with its texture; nothing later
   edits that asset.
3. **Clean it with the Market guide `GraphCode/CleanGeneratedMesh`** (install it with `unreal_gc_market`).
   One `.gcshape` recipe removes loose pieces, closes holes, drops hidden inner shells, replaces flat
   whisker cards, sets life size and facing, names the material slots, adds collision, **keeps the
   painted texture**, and gives you before and after counts.
4. **Combine parts when you generated parts.** [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md) joins
   head, torso and limbs through sockets and bands in a `.gcshape`.
5. **Rig and move it.** `.gcrig` Route B rigs a bare mesh ([RIG.md](RIG.md)), `.gcmove` animates it.
   For a human target, fit a native MetaHuman from the clean mesh with `.gchuman` instead
   ([CHARACTERS.md](CHARACTERS.md)).
6. **Dress and groom it** with Garment and Groom; they stay separate layers.
7. **Check it** beside real photographs and in the lighting-only view, then ship the recipe.

Generation spends the owner's money and carries the service's licence: record the service, model, prompt
and terms ([CONTENT.md](CONTENT.md)), and ask before paid generation.

## Product tree and composition routes

```text
GraphCode
├─ Core — reflective assets and the level ......................... .gc
├─ Play — the running game ........................................ .gcp / gc_play
├─ Move — animation, time, effects and decisions .................. .gc + .gcmove
├─ World — spatial/environment systems ............................ .gc + .gcw + .gcs
├─ Core Extension (Advanced) — optional engine frameworks ......... .gc + .gchuman
├─ Shape — procedural geometry intent ............................. .gcshape
├─ Integrate — external-source translation/import ................. gc_import
├─ Rig — skeleton, skin and control-rig intent ..................... .gcrig + .gc
├─ Garment — clothing construction intent ......................... .gcgarment
└─ Groom — hair construction plus Groom projections ............... .gcgroom + .gc
```

Optional products stay visible in discovery so a missing install can refuse by name: "listed" does not
mean "installed here". Check `unreal_info`, the product fields of `gc_describe` and `available`.

```text
Need a complete character?
├─ native MetaHuman .......................... kind:metahuman / Core Extension
├─ generated body or head
│  ├─ one mesh ............................... import → CleanGeneratedMesh → inspect
│  ├─ generated as parts ..................... import → Shape organic assembly
│  └─ native result wanted ................... .gchuman reference fit → MetaHuman
├─ bones, skin, sockets and controls ......... .gcrig
├─ clips and body motion ..................... .gcmove
├─ clothing / hair ........................... .gcgarment / .gcgroom
└─ runtime proof ............................. gc_play + render/capture

Need a complete environment?
├─ object geometry ........................... .gcshape
├─ imported object ........................... gc_import / Integrate
├─ actors and level structure ................ kind:level / Core
├─ regions, terrain, water, foliage .......... .gcw / World
├─ PCG ....................................... kind:pcg_graph / World
├─ navigation and decisions .................. World + Move + extension EQS
└─ appearance and traversal proof ............ gc_render + gc_play
```

The owning source is where a correction belongs: do not hand-edit a generated mesh to fix a Shape
recipe, or key an animation output to fix a Move program, unless the workflow transfers ownership.

## Planned, unsupported and unverified are different words

- A product with no reflective kinds is program-first (Shape, Play, Integrate, Garment).
- An existing-only kind has an import or upstream-program creation route.
- A read-only block is a measured boundary; do not invent another writer.
- A missing optional product is unavailable in this installation, not unfinished globally.
- A requirement not exercised in the target project is **unverified**, even when its tool is shipped.

Call something **planned** only when the installed help or the active board does. A parser, apply,
compile, render or play call proves only the surface it observed ([VALIDATION.md](VALIDATION.md)); recovery
of a prior state is `gc_versions` ([GIT.md](GIT.md) for authored files).

## Reporting a genuine gap

Record the installed product and engine plugin, the `gc_describe` status of the kind and block, the verb's
current help and exact refusal, whether the state belongs to an upstream program, a reflective `.gc`, the
live runtime or an external source, and the smallest disposable fixture that reproduces it. Then email
admin@graphcode.dev or [open an issue](https://github.com/xiangjie9999/GraphCode/issues).
