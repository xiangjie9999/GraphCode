# Create professional vegetation for UE5.7 and UE5.8

Use `kind:procedural_vegetation` to read, create and edit a native Unreal Procedural Vegetation
Editor (PVE) graph. Its `.gc` is an editable representation of that graph, like Blueprint IR.
`gc_apply` changes the native asset; `gc_render` evaluates a temporary copy to show the tree.

PVE supplies plant structure and places reusable foliage meshes. A finished vegetation family
also needs convincing species anatomy, leaf and bark assets, materials, coherent motion,
ecological placement and acceptable cost in the actual level. Follow
[ASSET-QUALITY.md](ASSET-QUALITY.md) throughout; generating many plants does not finish them.

For UE5.7, use PVE presets or author custom plants externally. UE5.8 adds native growth/extraction
as another PVE route. Then follow the shared material, export, wind, placement and validation
steps. **The release target is UE5.8.** The executable Hazel recipe was verified previously on
UE5.7.4; the new growth route requires a release-engine GraphCode proof before production.

The saved recipe describes what generates the vegetation. Editor layout, selection, preview
and debug state are outside its required coverage; BPIR can arrange the graph again. Native
node classes and properties are discovered through Unreal reflection, with adapters for special
values such as ramps. The sample node list is not a fixed whitelist.

## UE5.8 and the two authoring routes

| Route | Choose it when | Structure workflow |
|---|---|---|
| **Way 1 Customize a species preset** | A suitable native preset or Megaplants species already supplies the anatomy and component kit | Preset variation → structural modifiers → meshing → foliage → output |
| **Way 2 Grow or extract a new species** | The species, growth habit or required silhouette needs a new structure | Seed/growth or extracted structure → growth and grafting → refinement → meshing → foliage → output |

Both produce editable native PVE graphs. They are authoring choices, not different file formats.
UE5.8 adds native growth, extraction and grafting while PVE remains experimental.
[Epic release notes](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-8-release-notes?lang=en-US)
and the [UE5.8 presentation](https://www.youtube.com/watch?v=zfFq5e8Pxz0) establish these native features.

| Capability | What agents can rely on |
|---|---|
| Preset graph creation, editing, organisation and static preview | Existing GraphCode route below; prior verification used UE5.7.4 |
| New UE5.8 nodes and authored state | Discover on the installed engine and prove preservation, evaluation and save/reload |
| Native PVE mesh export | Separate operation; graph apply and preview do not bake packages |
| World `foliage()` | Finished static mesh through a compatible foliage type |
| Animated Nanite foliage | Native skeletal output, suitable instanced skinned placement and a wind transform provider; verify installed integration |
| Non-Nanite platform delivery | Plan explicit compatible assets and distance representations; do not rely on a future PVE LOD roadmap |

Before a release-engine task, record the actual engine build, GraphCode version/products,
native plugin availability, target platform/RHI and chosen material/rendering pipeline. Keep
UE5.7-only evidence scoped to that version. Reflection admitting a class does not establish
coverage for a new nested object, manual edit record, subgraph or settings slab.

## UE5.7 and its two production routes

UE5.7 PVE starts from species presets and their reusable components. It supports structural
modifiers, custom foliage/materials and native output for level use. Its initial public workflow
does not include UE5.8's from-scratch Grower, mesh/image extraction and grafting toolset.
[Epic UE5.7 PVE workflows](https://dev.epicgames.com/documentation/en-us/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine?application_version=5.7).

| UE5.7 route | How agents make the vegetation |
|---|---|
| **Customize a native PVE preset** | Follow Way 1 below: select species/variation, modify structure, finish bark and foliage, export, then place the compatible output |
| **Author a custom plant externally** | Build the species/component kit in Blender or another suitable DCC, import finished meshes and materials, then use foliage/PCG or a separately verified animation path |

For the external route, follow [CONTENT.md](CONTENT.md#import-and-provenance). Preserve the
editable source, finish leaf/twig/branch construction, and verify the imported material slots,
scale, pivots, UVs, collision and distance representation before scattering. The shared craft,
biome and performance sections apply to both routes.

A regular imported tree mesh does not become an editable PVE species structure in UE5.7.
The native `PVImporter` is a specialized plant-data importer, not a general FBX-to-PVE converter.
Treat mesh/image extraction in Way 2 as UE5.8 functionality. External plants can still supply
finished vegetation or compatible reusable foliage components for a preset.

GraphCode's prior UE5.7.4 evidence covers graph creation/editing, organisation and static preview.
Native mesh export, wind playback and packaged performance need their own production checks;
that graph evidence does not establish them.

## Plan the plant and its biome

Write a species brief before choosing graph values. Reference the whole plant, its habitat,
trunk base, branch forks, twigs, both sides of leaves and natural motion. Include a scale cue and
the intended close, player and distant cameras. Quixel's
[reference photography guidance](https://dev.epicgames.com/community/learning/courses/qAQ/unreal-engine-realityscan-scouting-by-quixel/7JEm/unreal-engine-realityscan-reference-photos?locale=ko-kr)
explains why habitat views matter alongside isolated details.

| Brief item | Decision it controls |
|---|---|
| Species, biome, season and art direction | Leaf anatomy, bark, color range and companion plants |
| Age, health and exposure | Height, trunk taper, crown shape, dead branches and foliage distribution |
| Branching habit | Forks, branch hierarchy, bud priority and leaf arrangement |
| Leaf size and attachment | Petiole, pivot, orientation, clump size and silhouette |
| Ground contact and gameplay | Root flare, burial, clearance, collision and interaction |
| Camera distance and target hardware | Geometry/card choice, texture detail, motion and distance policy |

Create a family with structural reasons for variation: sapling, mature plant, shaded plant,
exposed plant or a damaged specimen as the brief requires. Match crown density to those causes.
Random scaling and hue changes alone leave identical branch patterns visible across a grove.
Keep a stable identity and selected seed for each accepted variant.

Choose a representative dense patch early. A path edge, canopy gap and shaded understory together
expose more defects than one isolated tree. Approve the distant mass and canopy coverage before
spending all the budget on close leaf detail. This adapts the production approach explained in
[Horizon's vegetation talk, 36:52–42:27](https://www.youtube.com/watch?v=wavnKZNSYqU&t=2212s);
its older card/billboard implementation is not a UE5.8 Nanite prescription.

## Build reusable plant components

Prefer a suitable scanned or authored species kit when it fits the brief. Inspect its editable
source, dimensions, pivots, textures and materials before reuse. For external DCC content, use
the import and material translation workflow in [CONTENT.md](CONTENT.md#import-and-provenance).
For unique leaf contours and supporting geometry, use
[ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md), with GraphCode Shape where the installed tools support it.

### Choose the representation before detailing

| Representation | Production choice |
|---|---|
| Nanite geometry with reusable components | Model the silhouettes that must survive close views; reuse branch/twig/leaf parts instead of duplicating every complete tree's geometry |
| Conventional masked cards or clumps | Fit geometry to the visible coverage and build explicit distance representations; inspect overlap, mip loss and shadows |
| Hybrid kit | Assign different representations to tree canopy, small herbs, distant cover or hero details based on measured visual value and cost |

Epic's [Nanite foliage presentation, 01:54–11:14](https://www.youtube.com/watch?v=aZr-mWAzoTg&t=114s)
explains the motivation for modeled foliage, component reuse and skinning. Nanite does not make
poorly bounded motion, costly materials or arbitrary amounts of repeated unique data free.
Keep conventional delivery explicit for targets that need it.

### Make a representative leaf and twig

1. Author the species silhouette first, including its tip, lobes or serration where the camera
   resolves them. For an awkward unique contour, reuse or create front-on artwork, isolate the
   mask, convert it to named SVG paths and inspect the conversion preview before forming geometry.
   Use the [artwork to SVG workflow](ARTISTIC-SHAPES.md#convert-texture-artwork-to-svg).
2. Give the leaf appropriate cup, curl, twist and a central ridge. Let the broad silhouette and
   bend read before adding vein texture. Keep thin geometry intentional; avoid a thick extruded
   badge or an identically flat polygon for every leaf.
3. Attach the petiole to a real twig connection. Put the component pivot at its attachment and
   document the forward/up axes expected by the distributor. Check transformed instances, not
   just the unrotated source. Separate aim along the twig from facing the leaf surface.
4. Prepare healthy, older or damaged leaves only where the brief needs them. Tie damage, color
   and placement to plant state; uncontrolled random holes and colors can obscure species identity.
5. Finish one twig or clump with useful negative space. Inspect it from below, above and edge-on.
   Check that several copies do not reveal identical planes or repeated texture landmarks.
6. Bake fine veins, pores and small surface relief where geometry would add little visible value.
   Keep editable artwork, UVs, textures and the accepted physical dimensions with the source kit.

For cards, inspect opacity coverage and edge padding in distant mip levels. Remove unintended
background fringes without eroding narrow stems. For geometry, inspect the actual contour and
shading at the final bake; the SVG preview does not prove the formed leaf is correct.

### Build branches and bark that join

Use a readable trunk taper, root flare, branch hierarchy and twig thickness. Inspect every major
fork for cracks, abrupt radii, floating pieces or intersections visible from the player camera.
Give bark a consistent physical scale and direction. Check seams around the trunk and through
branch transitions; repeating a scan does not automatically match its adjoining UVs.

Reserve geometry for silhouette, deep fissures and visible volume. Use material/bake detail or
decals for finer bark, scars, lichen and moss, with masks tied to the brief's exposure and age.
Preserve a scanned hero trunk as its own asset if its unique surface is essential: UE5.8 structure
extraction does not preserve that original surface.

Nanite assemblies reuse parts within a plant asset. They differ from world-level instancing,
and their parts follow the base skeleton rigidly. Shape reusable branch variants appropriately
for their crown position rather than expecting each part's vertices to bend independently.
See [Epic's assembly documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-assemblies).

## Enable and discover

Install GraphCode Core and **GraphCode Core Extension** (formerly Advanced). Enable Unreal's
optional **Procedural Vegetation Editor** plugin (`ProceduralVegetationEditor`) through
Edit > Plugins, accept its dependency prompts and restart. It remains experimental in UE5.8.
The release contains the adapter; using it does not require a GraphCode source build.

For the Nanite foliage route, enable the project's **Rendering > Nanite Foliage** setting and
restart as the native documentation requires. Verify PCG and DynamicWind availability when
the selected placement/motion path needs them. Conventional static delivery has its own
representation and material choices; do not assume these settings convert it automatically.

Ask the installed tools before composing a graph:

```text
gc_describe {"kind":"procedural_vegetation","include_example":"full"}
gc_describe {"kind":"procedural_vegetation","block":"graph"}
gc_apply {"help":"create","expanded":true}
gc_apply {"help":"organise","expanded":true}
gc_render {"help":"kind","kind":"scene"}
```

Your MCP client may expose these names with an `unreal_` prefix. Use the names it lists.
The kind help describes the native graph grammar; the tool help describes the operation's
parameters. Follow their availability diagnostics if the product or engine plugin is disabled.

For a new UE5.8 operation, obtain its exact native class path from the installed editor/class
discovery or a live reference graph, then inspect that type with `gc_describe`. PVE-specific settings
derive from `/Script/ProceduralVegetation.PVBaseSettings`. This is a native settings graph;
the Blueprint action database is not its node catalogue.

## Way 1 Customize a species preset

Choose a species with the intended branching habit and foliage kit. Inspect the supplied
variations before editing. Set the broad scale and crown shape first, then gravity, branch
removal or carving where available. Finish bark and leaf distribution after the structure reads.
The [Epic preset walkthrough](https://dev.epicgames.com/documentation/en-us/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine?application_version=5.7)
explains the existing modifier, mesh and foliage stages.

Work through [PVE-PRESETS.md](PVE-PRESETS.md) for the native preset lesson: data flow,
structure controls, meshing/bark, palette slots, distribution, phyllotaxy, ramps and diagnosis.
It includes a complete distribution study graph and controlled exercises before making a family.

### Reusing settings across trees

GraphCode World adds `kind:pcg_settings` for a standalone native settings asset. PVE node classes
also need the PVE plugin and Core Extension. Create the shared asset first through `gc_apply`:

```gc
gc 1 kind:pcg_settings asset:/Game/Vegetation/SharedScale

settings {
  value = PVScale() [Scale=0.9]
}
```

A tree then references it with a node such as
`n1 = PVScale(In=n0.Common_Hazel_01) [settings=obj'/Game/Vegetation/SharedScale.SharedScale']`.
Edit the shared asset's own document to change its instances. A referencing graph carries no
inline properties or bodies for that node. Each graph keeps its own node enabled flag.

The `value` declaration is one complete generation value: omitted properties and bodies return
to native defaults, and its class cannot change after creation. Nested arrays and ramps use the
same grammar as inline nodes, with each array appearing once as an ordered body.

### A complete tree with inline settings

Write this file directly as `Content/Vegetation/PVE_Hazel.gc` in the customer's project:

```gc
gc 1 kind:procedural_vegetation asset:/Game/Vegetation/PVE_Hazel

graph {
  input = GraphInputOutput()
  n0 = PVPresetLoader() [Preset=obj'/ProceduralVegetationEditor/SampleAssets/Tree_Common_Hazel_01/PVE_CommonHazel_Data.PVE_CommonHazel_Data']
  n1 = PVScale(In=n0.Common_Hazel_01) [Scale=0.9]
  n2 = PVGravity(In=n1.Out) [GravitySettings.Gravity=0.35]
  n3 = PVMeshBuilder(In=n2.Out)
  n4 = PVFoliagePalette(In=n3.Out)
  n5 = PVOutput(In=n4.Out)
  output = GraphInputOutput()
}
```

This is a new-asset recipe, so it has no invented `sig`, `bin` or `omits` values. The first
projection supplies those. The `asset:` address must agree with the file's location.

The preset is an Unreal plugin dependency, not content supplied by this guide. Verify it exists
in the installed engine's plugin content. If using another preset, read its native graph or
inspect its pins: `Common_Hazel_01` is this preset's actual output pin, not a universal name.
This is the UE5.7-compatible preset route, not the new UE5.8 simulated-growth route.

Plan the creation, inspect its result, then create:

```text
gc_apply {"asset_path":"/Game/Vegetation/PVE_Hazel","create":true,"dry_run":true}
gc_apply {"asset_path":"/Game/Vegetation/PVE_Hazel","create":true}
```

Pass neither `block` nor `ir` for this file route. Do not add `compile:true`: creation is its
own operation. Check the result's errors, creation caveats and save report, then read it back:

```text
gc_read {"asset_path":"/Game/Vegetation/PVE_Hazel","source":"live","blocks":["graph"]}
```

The two `GraphInputOutput()` nodes are native hidden graph terminals. Keep them in the document.
`PVOutput` is the visible vegetation output and is needed for the tree preview.

### Refine the species family

Make separate accepted structures for the brief's life stages and exposure conditions. Preserve
the original species reference and compare every variant beside it. Excessive carving, gravity
or scale can turn a good preset into a plant with impossible forks and taper.

Inspect the preset's foliage palette and native placement attributes. Select component variants
that belong to the species, then tune density, generation, attachment and orientation. Keep
separate controls for structural changes and leaf redistribution. A graph's inherited seed is
not automatically the foliage distributor's random seed.

Use [the preset lesson's distribution controls](PVE-PRESETS.md#choose-preserved-or-regenerated-distribution)
to distinguish preserved placements, regenerated sites and component selection. Confirm the
activation gate and native ramp coordinate before tuning a family.

## Way 2 Grow or extract a new species in UE5.8

This is a native workflow description, **not executable GraphCode syntax**. The exact node
classes, pin names and fields must come from the installed release engine.

```text
Seed generation → Grower → optional grafted twigs/branches → refinement
                           → meshing → foliage distribution → output

Mesh/image extraction → extracted plant structure → optional Grower/grafting
                                                 → same finishing stages
```

Epic demonstrates this progression in
[Growing Trees in Unreal Engine, 02:44–30:09](https://www.youtube.com/watch?v=zfFq5e8Pxz0&t=164s).
Start with a readable main structure, add secondary growth and polish the result. Keep each stage
inspectable; one huge graph makes both botanical errors and integration gaps harder to isolate.

### Grow the main structure

1. Start from the species brief's branching and leaf arrangement. Phyllotaxy means how buds or
   leaves alternate, oppose, spiral or form whorls; choose it from reference.
2. Use native seed generation and a Grower to establish the trunk and primary branches. Tune
   apical/axillary priorities, forks, taper, branch length and growth progress before fine foliage.
   Growth cycles are simulation steps; do not label them as years without a species calibration.
3. Introduce gravity, light response, elasticity and shedding deliberately. The demonstrated
   illumination is a growth simulation, not automatic live-level sunlight. Keep its preview
   shadow cards separate from the final renderable leaf representation.
4. Inspect exposed structure and native diagnostic views. Look for self-intersection, tangled
   forks, equally thick generations, abrupt terminations and excessive branches in the crown.
5. Generate younger and mature structures through meaningful growth states. Keep the accepted
   seeds and settings with each variant instead of relying on a lucky unsaved preview.

### Extract structure from reference

Use image extraction for a clear plant silhouette, or mesh extraction for an existing three
dimensional plant structure. Isolate the intended trunk/branch input as the native extractor
requires. Clean unrelated background or mesh parts before extraction.

Extraction gives a structure that PVE can modify and remesh. Check its depth, branching, scale
and material boundaries against the original reference. A structure extracted from a flat image
needs deliberate depth; inspect side views before distributing foliage. Optional regrowth can
extend the structure, but must preserve the brief's recognisable silhouette.
[Epic extraction demo, 16:46–18:35](https://www.youtube.com/watch?v=zfFq5e8Pxz0&t=1006s).

### Graft and refine

Grafting places reusable growth structures on another growth structure, such as secondary twigs
on primary branches. It is plant assembly, separate from scattering completed trees in a level.
Inspect attachment position, axes, generation, spacing and local shape before adding more copies.
Use later growth only when it improves the grafted connections and the species silhouette.

Apply native avoidance/carving against explicit interaction inputs for a trained hedge, building
edge or obstacle. It does not establish that a PVE asset automatically reacts to world geometry.
Put manual rotation, trim or removal late in the graph and recheck it after upstream topology changes.

Finish the mesher and trunk material compositor, then distribute leaves using the intended
generation, orientation and condition. Distinguish plant-wide gradients from coordinates within
each branch. Use additional distribution passes for different organs or a deliberate mask.
The talk's **Refit** example is a custom subgraph, not a universal native node token.

Expose a small set of bounded artist controls once the structure is accepted. Keep safe ranges
and dependencies visible. Prove GraphCode preservation of native slabs, parameter connections,
embedded subgraphs and manual edits before making them production dependencies.
Native growth-settings slabs are not automatically the shared `pcg_settings` assets used in Way 1.

### Prove the release engine path before a batch

Create one disposable native reference containing the new operation and its required inputs.
Read it with current GraphCode help, inspect coverage/`omits:`, then dry-run a small change.
Apply, read live again, evaluate, save and reload. Compare both stored values and visible output.
Repeat an unchanged apply to check stability. Verify any ramps, arrays, referenced settings or
manual records that the production design will rely on.

If a needed facet is omitted or fails the proof, preserve the native asset and identify the exact
integration gap. Keep that facet in a native/DCC route with project-owned source and dependencies,
or resolve the adapter coverage before scaling. Do not replace missing state with guessed fields,
invent signatures or advertise the route as release-tested.

## Edit the graph and organise it

Read the current asset first. Edit its existing `.gc` directly, retain its identity/header and
apply by `asset_path`. For example, change `Scale=0.9` to `Scale=1.1` while keeping the rest of
the graph, then dry-run and apply. **A supplied PVE graph block is the complete graph:** omitting
a node removes it, even under the tool's default patch mode. Matching aliases/classes are reused.

For unsaved editor work, use `source:"live"` and the current signature as the tool help describes.
Opening PVE's own editor can populate material defaults and foliage entries during its native
evaluation. Read that live state again before editing; do not assume it equals the earlier saved
sidecar just because the package is still reported clean.

Arrange the graph through the existing BPIR organiser:

```text
gc_apply {"asset_path":"/Game/Vegetation/PVE_Hazel","organise":true,"dry_run":true}
gc_apply {"asset_path":"/Game/Vegetation/PVE_Hazel","organise":true}
```

`organise:["graph"]` also selects its one embedded graph. Layout moves native node positions
without changing settings, connections or aliases. Inspect the `organise` report for moves;
positions are outside the `.gc` signature, so an unchanged signature is expected. A repeat is
stable. A comment refit that would change its `covers=` membership is refused before moving nodes.
For PVE use `gc_apply organise`; `gc_render mode:"layout"` is not the PVE layout route.

## Render and inspect

Render the native asset directly:

```text
gc_render {
  "asset":"/Game/Vegetation/PVE_Hazel",
  "kind":"scene",
  "width":1400,
  "height":1600,
  "camera":{"azimuth":35,"elevation":7,"zoom":0},
  "output":{"lifetime":"persistent","path":"Saved/Vegetation/Hazel/","overwrite":"replace"}
}
```

Here `kind:"scene"` selects the renderer; `procedural_vegetation` belongs in the `.gc` header and
`gc_describe`. PVE does not need a Content Browser thumbnail renderer or an exported static mesh.
The normal camera and named-view controls apply; ask `gc_render` for their help when needed.

The adapter evaluates a temporary graph copy and renders the **first `PVOutput` in graph order**,
matching native editor opening behaviour. It displays trunk/branch geometry and instanced foliage
with their materials. It preserves the source graph and the user's preview settings. It needs an
editor with graphics enabled; a `-nullrhi` host cannot produce the image.

On a cold render, materials may still be compiling. Treat `incomplete` or a pending `readiness`
report as unfinished and render again after compilation settles. Inspect the whole silhouette
and a close view of branch connections, bark, leaf edges and gaps. A successful call alone is not
proof of a finished image. Native Nanite foliage may use Core's reported full-detail source
geometry preview path; it retains the materials and UVs without modifying the mesh asset.

The current adapter is a static preview. It does not provide native wind playback or PVE geometry
queries. No output means there is no `PVOutput`; empty output means the selected output did not
produce renderable geometry. Check the preset, native pin names and connected chain in those cases.

## Choose the next operation

| Intent | Route |
|---|---|
| Change this tree's native PVE graph | Edit its `.gc`, then `gc_apply` |
| See the tree while authoring | `gc_render` with the PVE asset and the scene renderer |
| Arrange its graph nodes | `gc_apply organise:true` |
| Produce mesh packages for use downstream | A separate native PVE bake/export operation |
| Scatter finished trees in a level | A suitable baked mesh and foliage type, then GraphCode World |
| Make bespoke supporting geometry | GraphCode Shape or an imported mesh, referenced where PVE supports it |

`PVOutput` export fields configure a future native bake; applying the graph does not perform that
bake. For placement follow [ENVIRONMENTS.md](ENVIRONMENTS.md), including collision, grounding,
repeat-apply, distant appearance and runtime checks. A PVE preview does not prove those behaviours.

## Finish materials in the chosen rendering pipeline

Reuse an appropriate native species master or build a small material family whose controls match
the project's conventional or Substrate pipeline. Inspect the master before creating instances;
a parameter name in another pack does not establish the same meaning here. Keep leaf, bark and
optional seasonal states consistent across the family.

For conventional thin leaves, Unreal's **Two Sided Foliage** shading model supplies transmitted
light through Subsurface Color. Shape that transmission with thickness/vein information and
inspect both surfaces under front, side and back lighting.
[Epic shading models](https://dev.epicgames.com/documentation/en-us/unreal-engine/shading-models-in-unreal-engine).
Use the equivalent supported native setup when the project uses another shading pipeline.

Follow [CONTENT.md](CONTENT.md#a-texture-set-is-several-maps-for-one-surface) for complete PBR
texture sets, import settings and packed channels. Check the normal convention and real bark/leaf
scale. A generated color image alone does not establish roughness, normal, thickness or height.
Avoid unintended baked sunlight and shadows in albedo; test material response under a moving light.

Keep roughness and transmission tied to the plant surfaces. Leaves, dry bark, exposed wood and
wet patches should not share one convenient glossy response. Use controlled masks for moss,
lichen, age and moisture. Inspect tile repetition and gradients around forks, not just a flat swatch.

Reserve material complexity for visible effects. Test the leaf shader over dense overlapping
coverage, where a modest per-pixel cost can become expensive. A cheap bark shader and a small
number of coherent instances often provide a better starting point than a universal material
with every effect enabled.

For Nanite Foliage, vertex color **alpha** has a native normal-distribution role. Do not copy an
older wind/AO packing scheme into it without checking the asset contract.
[Epic Nanite Foliage](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-foliage).
Assembly part identity also limits per-part instance material data; plan supported variation at
the component, graph or whole-plant level.

## Bake usable meshes and inspect the result

Choose static output for the verified World foliage path, or skeletal output for an appropriate
animated instanced skinned path. Inspect the native output mode and dependency requirements on
the installed release engine before configuring it.

1. Finish the PVE graph and inspect a ready preview. Select its native output and configure the
   export destination, mesh type, materials, assembly/Nanite choice and required companions.
2. Inspect replacement policy and every affected package. Preserve useful existing work and
   project-owned dependencies. Use the native PVE editor's export operation, or an installed
   tool whose current help explicitly supports that same bake.
3. Check the actual saved mesh, material slots, source dimensions, bounds and pivot. For skeletal
   output also inspect the skeleton, simulation groups and wind data required by the chosen path.
4. Inspect the baked result in a level against the PVE preview. Verify close leaves, forks, UVs,
   missing materials and distant canopy coverage. Save and reload the outputs and their references.
5. Add collision and placement intentionally. An output checkbox or collision profile cannot
   prove that the baked mesh has suitable usable collision.

Use trunk-focused collision or another gameplay-appropriate simple shape where possible.
Inspect convex hulls for gaps they bridge between branches, unintended walls and blocked paths.
Leaves rarely need full collision. Prove player clearance, navigation and required traces on the
actual exported asset.

Keep the native PVE source and reusable components editable beside the finished outputs. Export
is a production stage that can fail independently; report the created package paths and inspection
results, not just a successful graph apply.

## Make wind coherent and verify playback

Treat motion as part of species identity. Plan main trunk response, branch bending and leaf
flutter with different scales. Anchor the base; let the branch hierarchy transmit motion coherently.
Add phase variation between plants and branches without making every leaf move independently.
Light wind should preserve the species' silhouette, while gusts should change it plausibly.
The [Horizon motion chapters, 11:47–15:43](https://www.youtube.com/watch?v=wavnKZNSYqU&t=707s)
show the production reasoning; their channel encoding belongs to that engine.

| Motion route | What to verify |
|---|---|
| Skeletal Nanite foliage with DynamicWind | Native exported wind data, suitable instanced skinned component and the required transform provider |
| Conventional material WPO | Correct local pivots/weights, root anchoring, bounded deformation and shadow/GI behavior |
| Gameplay bending or hit response | An explicit interaction system integrated with the chosen representation |

DynamicWind's documented path supplies global wind; it does not automatically add player or
object collision response. Its PVE wind profile and the level's wind controls are different
settings. Do not guess scene wind speed fields on a profile or assume any skeletal mesh will
animate when scattered.

Play the actual level in calm, steady wind and gust conditions. Inspect the base, major forks,
leaf attachment and neighboring plants from several angles. Look for sliding roots, detached
leaves, stretching, jitter, clipping and synchronized repetition. Check start/stop transitions and
distance changes as well as a dramatic gust.

The current `gc_render` PVE image is static. Use native playback and captures for motion
acceptance, then inspect the packaged runtime. A still image cannot validate wind, collision
response or temporal stability.

## Place an ecological vegetation family

Use a plant family to build a biome, with a large-scale composition, medium-scale clusters and
small ground detail. Start with canopy trees and gaps, then place saplings/shrubs, herbs and grass,
followed by litter or deadwood where the brief calls for them. Keep density responsive to site
conditions, exposure, moisture and disturbance supported by reference.

Design correlations as well as variation: seedlings around an appropriate edge, companions in
patches, litter near its source and clearings with a reason. Avoid uniform noise across every layer.
Do not treat every species as interchangeable green filler or every shaded location as bare ground.
Use believable transition zones between wet banks, exposed edges and the grove interior.

Keep player routes, entrances, water and other exclusions explicit. Trees usually stay upright
on slopes; small ground plants may align to the surface. Set the root offset so trunks contact the
actual terrain without floating or hiding the entire flare. Exclusion distance needs to account
for crown size and player clearance, not just the point's center.

### Choose a native placement path

| Path | Use and boundary |
|---|---|
| GraphCode World `foliage()` | Repeatable placement of finished static meshes through foliage types |
| Native PCG | Attribute/mask-driven relationships, priority and streaming/generation decisions; inspect current GraphCode graph coverage |
| Instanced skinned placement | Animated plants through the compatible native component/spawner plus wind setup |
| Landscape Grass | Landscape material masks and Grass Types for suitable surface cover |
| Manual hero placement | Deliberate focal plants and exceptional intersections, kept consistent with the biome |

[Epic's PCG Biome example](https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-pcg-biome-core-and-sample-plugins-reference-guide-in-unreal-engine)
demonstrates layered generators and exclusions;
[Landscape Grass](https://dev.epicgames.com/documentation/en-us/unreal-engine/grass-quick-start-in-unreal-engine)
uses a separate material-driven route. Neither establishes that all its native controls have an
equivalent World declaration. Prove the selected authoring and runtime path before scaling it.

### Put a baked static tree into GraphCode World

After native export has produced `/Game/Vegetation/Hazel/SM_Hazel`, create the compatible foliage
type through the normal file/create workflow:

```gc
gc 1 kind:foliage_type asset:/Game/Vegetation/Hazel/FT_Hazel
parent: /Script/Foliage.FoliageType_InstancedStaticMesh

values {
  Mesh = obj'/Game/Vegetation/Hazel/SM_Hazel.SM_Hazel'
  Density = 0.5
  ScaleX = (Min=0.85,Max=1.15)
  AlignToNormal = false
}
```

This reuses the existing Hazel placement example. Inspect native foliage-type properties with
current help, then tune density and scaling against the species brief. World `density` is a
multiplier of the foliage type's density, not a universal plants-per-square-metre input.

Merge this population into an existing level document with collision-bearing ground, preserving
its other declarations. The example neither creates a map nor constructs terrain:

```gcw
gcw 1 level:/Game/Maps/L_HazelGrove

regions {
  Grove = region() {
    p1 = (0.0, 0.0)
    p2 = (6000.0, 0.0)
    p3 = (6000.0, 6000.0)
    p4 = (0.0, 6000.0)
    south = p1 -> p2
    east = p2 -> p3
    north = p3 -> p4
    west = p4 -> p1
  }
}

populate {
  Hazels = foliage(types=(/Game/Vegetation/Hazel/FT_Hazel), in=$Grove, density=1.0, seed=9231) [slope=(Max=30.0)]
}
```

World coordinates are centimetres; the grove is 60 metres across. Retain stable seeds while
adjusting the art. Add named planting/exclusion regions through the installed World grammar.
Follow [ENVIRONMENTS.md](ENVIRONMENTS.md#populate-the-environment) for supported grounding and
filters, using current tool help for the exact declaration and apply parameters.

Ground the terrain and update collision first. Apply the population, inspect actual ground
contact, repeat an unchanged apply, then save/reopen and inspect again. Supporting traces must
skip existing foliage so repeated placement does not climb onto previous plants.

The foliage-type Mesh slot above is a **static mesh** slot. Do not substitute a skeletal output.
For skinned plants, establish a separate native instanced skinned/PCG path and its DynamicWind
provider. Static placement success does not prove that path.

## Measure dense vegetation on the release target

Set a frame-time and memory budget from the project's actual target hardware, resolution,
quality settings and representative density. Use both traversal and a stationary dense-canopy
view. Keep the camera and accepted plant seeds consistent during comparisons.

Profile costs separately so a fix reaches the actual bottleneck:

- Compare canopy geometry, material coverage and texture/geometry memory before adding motion.
- Enable wind and inspect deformation cost, bounds, shadows and temporal artifacts.
- Inspect Virtual Shadow Maps and cache invalidation around moving vegetation.
- Inspect Lumen/ray-tracing representation and cost for the chosen static/skinned path.
- Traverse streaming boundaries and measure generation, upload or shader hitches.
- Verify the packaged build with debug overlays disabled, after expected warm-up.

Moving/deformed geometry invalidates VSM cache pages; tight bounds and an intentional motion
distance policy matter. Software Lumen distance fields do not represent WPO.
[Epic VSM](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine?application_version=5.7),
[Epic Lumen](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine).
Inspect the chosen release-engine tracing mode instead of assuming visible mesh motion implies
matching lighting and shadows.

For conventional foliage, validate card overdraw, silhouette/coverage through LODs, texture mips,
culling and fade transitions. For Nanite foliage, validate geometry/voxel transitions and native
animation distance behavior. Start from the native voxel/shape-preservation defaults and change
them only to resolve an observed defect with a measured tradeoff. Conventional foliage cull/fade
fields do not establish Nanite visibility; check the installed engine's behavior.
[Epic foliage mode](https://dev.epicgames.com/documentation/en-us/unreal-engine/foliage-mode-in-unreal-engine?application_version=5.7).

Disable or simplify distant motion only through the selected path's supported controls, with
smooth transitions and appropriate shadows. Do not mark moving casters static merely to gain
cache reuse. Density scaling should preserve navigation and gameplay expectations; a setting
that removes collidable trees can change the playable space.

Change one cause at a time and record the image and timing together. Prefer a better component
kit, simpler dense shader or improved placement before reducing every plant to the same sparse
shape. Epic's demonstration counts and timings describe its demo, not a budget for this project.

## Agent work sequence and acceptance

For each production task, save the task-specific plan required by
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md). Resolve the representation, export, placement
and motion paths before a large asset batch.

1. Write the species/biome brief, cameras, variant intent and runtime budget.
2. Discover installed capabilities and prove any new release-engine operation.
3. Finish one plant's structure and component kit, materials and baked mesh.
4. Verify the chosen motion and placement in a representative patch.
5. Expand the accepted family with stable identities and meaningful variations.
6. Inspect, repair, save/reload and profile the populated level, then verify packaged behavior.

| Acceptance view or operation | Required observation |
|---|---|
| Whole plant from several angles | Species silhouette, taper, hierarchy and intentional crown gaps |
| Close trunk/fork/leaf views | Joined branches, believable attachment, clean seams, UVs and material slots |
| Front/back lighting | Leaf transmission, surface response and readable bark without baked lighting conflicts |
| Player camera and ground contact | Plausible scale, anchored roots, route clearance and appropriate collision |
| Wind playback | Anchored base, coherent hierarchy, attached foliage, clean gust and distance transitions |
| Distant canopy and traversal | Stable coverage, acceptable transitions, shadows/GI and streaming behavior |
| Repeated placement and save/reload | Stable plant identities, no duplication/lift, retained settings and references |
| Packaged target build | Required dependencies, appearance, interaction and measured cost on target settings |

Report which native graph and mesh packages were produced, the engine version, selected route,
captures inspected, playback conditions and measurements. State unresolved coverage or runtime
checks beside the affected behavior. A ready render, successful bake and measured playable patch
are different pieces of evidence; keep each attached to the result it proves.

## Study the workflows

Use these primary sources to learn the native operations and production reasoning. Links and
captions were checked on 2026-10-07; consult the installed engine for version-specific controls.

| Source | Useful chapters or purpose |
|---|---|
| [Epic UE5.8 PVE presentation](https://www.youtube.com/watch?v=zfFq5e8Pxz0) | 02:44 growth workflow; 16:46 extraction; 18:54 grafting; 21:25 manual edits; 23:00 meshing/materials; 25:39 foliage; 30:09 artist controls |
| [Epic The Future of Nanite Foliage](https://www.youtube.com/watch?v=aZr-mWAzoTg) | 01:54 representation; 04:25 assemblies; 08:40 skinning; 11:14 voxels; 17:56 preset workflow; 26:57 native export |
| [Gilbert Sanders Between Tech and Art](https://www.youtube.com/watch?v=wavnKZNSYqU) | 11:47 motion; 29:40 shading; 36:52 component production; 43:08 shadow/distance reasoning in Horizon's historical renderer |
| [Jacob Norris vegetation tutorial](https://www.purepolygons.com/uploads/6/0/5/5/60558851/tdw216.tutorial.pdf) | Species reference, component kits and biome consistency; original 2017 rendering advice needs current-platform judgment |
| [Stephane Biava PVE tutorial and chapter list](https://sb-arts.artstation.com/projects/OvYdzk) | Short preset-to-export overview; use it as an additional visual walkthrough |

The Epic and Horizon video chapters above were studied through available captions. The Biava
video's transcript was unavailable in this research; its creator's chapter list was inspected.
Do not treat a saved caption as proof that a demonstration's final visual quality was reviewed.
