# Author and verify with each GraphCode product

This guide carries lessons about **workflow and evidence**. It is not a syntax sheet. Ask
`gc_describe` for the installed kind and block grammar and `{ "help": true }` on the verb before
writing. For `.gcgroom` recipes, use the [Groom guide](GROOM.md) because their program grammar is
not yet in `gc_describe`. An optional product may be absent, and a present product may still declare `omits:` or
refuse a particular operation. Treat that answer as the boundary of the installed build.
Use the [capability map](CAPABILITY-MAP.md) when product or document ownership is unclear, and the
[lessons tree](LESSONS.md) when a clean write does not match the visible or runtime result.

For unfamiliar or complex work in any product, use [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md)
before implementation. Research the underlying technique and its dependencies, verify the actual
authoring path, and write a detailed plan. The product summaries below are not a substitute for
understanding how to achieve the requested quality and behaviour.

For every content or presentation medium, follow [ASSET-QUALITY.md](ASSET-QUALITY.md#apply-the-quality-bar-to-every-medium):
exceptional craft within the theme, explicit design choices and repeated observation of the actual
result. Models, animation, VFX, UI and audio all need this, using renders, playback, interaction or
listening appropriate to the claim. A successful authoring operation does not establish quality.

## Core: assets and the level document

Core gives an asset a readable `.gc`, and `gc_apply` is the common write path. A `.gc` is a
projection of a binary Unreal asset; it can deliberately omit authored facets. Read its `omits:`
before treating a diff as complete. Preserve aliases and `sig:` from a current canonical read.
Patch semantics vary by block: a graph patch carries lines you omit and removes an alias through
an explicit `delete` inside its own graph; a whole-value list may truncate on omission. Ask the
block's help every time the difference matters. Read back, compile and save the actual asset.
For a level, verify actor identity and placement in the open world, then render the scene.

## Shape: make geometry from intent

Read [ASSET-QUALITY.md](ASSET-QUALITY.md) before building anything visible. Every `.gcshape` uses
precedents and high-level intent, never low-level execution: reuse project recipes and the shipped
presets (`gc_search kind:"preset"`), state relations and patterns, derive values from named owners.

Choose [curved edge finishes](SHAPE-EDGE-FINISH.md) explicitly. After combining parts in a
Blueprint or level, run [assembly validation](ASSEMBLY-VALIDATION.md); its reusable editor
checker measures placed meshes without adding runtime project logic.

A `.gcshape` recipe is upstream of the built mesh; the mesh's `.gc` reflects what was produced.
Keep reusable dimensions, sockets and relations in the recipe instead of copying derived values
into several meshes. A clean topology count does not establish that the object looks right: render
the silhouette, joins, surfaces, material coverage and camera-facing details. Look at the actual
render before tuning a solver from numeric reports. A smoothing or density control can add a
large mesh cost, so inspect the resulting triangle count as well as the picture. Save and reload
the mesh to confirm the built artifact, not only the in-memory recipe.

When a 3D-AI model is generated as separate body, head or limb meshes, use the published
[organic part assembly guide](ORGANIC-PART-ASSEMBLY.md). It defines the shared generation contract,
import checks, receiver transitions, incoming root sockets, organic join order, downstream custom
Rig versus MetaHuman choice, and the visual/deformation gates that a parsed recipe cannot prove.

## World: place and relate things in space

Core's level document owns asset placement; World adds authored `.gcw` regions, terrain, water,
foliage, PCG and spatial questions. Name meaningful places and routes, then use `gcw_ask` to check
which region or object a point resolves to. Keep coordinates in the correct frame; a local offset
and a world position are different statements. Check grounding, collision, navigation and what a
camera can actually see. An authored `.gcw` is source text, while its built terrain or actors are
the visible result; preserve both and test a cold reload.

For terrain, realistic vegetation, streams, caves and World Partition, follow the
[environment workflow](ENVIRONMENTS.md). It carries import, ground-contact and layer-update
lessons that a successful source apply does not verify by itself.

For built environments, [roads and buildings](ROADS-AND-BUILDINGS.md) plans the network,
junctions, shared elevations, foundations and door approaches as one connected result.

For navigation, verify the pawn's selected agent and the bounds volume's supported agents agree.
A build can produce a valid mesh for one named agent while leaving the default agent with no
surface. Query the intended agent's mesh at both route endpoints, then drive the actual pawn.
A completed rebuild alone does not prove that pawn has usable navigation.
For a robot that makes decisions about changing routes or goals, follow the
[AI navigation guide](AI-NAVIGATION.md) across World, Move, EQS and runtime validation.

## Move: motion and decisions over time

Move covers supported animation, cinematic, Niagara and gameplay-AI assets. Read the exact kind:
an AnimGraph, montage, behaviour tree and Level Sequence have different identity and deletion
rules. Verify the asset's compiled state, then run the character or effect in a Play session.
A correct animation graph can still select the wrong pose, fail to transition or miss its target
at runtime. Check a representative start, transition, interruption and end state. Use a real mesh,
skeleton and camera when judging motion; a synthetic fixture only proves the codec's path.
For retargeting and for combining a Human or MetaHuman's body and face, follow the
[character composition guide](CHARACTERS.md).

## Play: ask whether the game works

`gc_play` starts and drives a running session; captures and diffs answer what changed in that
session. A successful editor compile does not prove input, HUD, navigation or saved runtime state.
Write a small scenario with a player-visible success and a failure path. Capture before and after,
inspect the state and image, and assert the outcome. A capture is one moment, so use actions and
waits to observe a transition. Treat restore/apply modes as state-changing operations and check
their impact first. Where a release claims packaged runtime support, launch and test the package.

## Rig: fit the mesh, joints and skin together

A `.gcrig` can start from a Shape result or an imported mesh. It describes rigging intent over
that mesh rather than generating the source surface by guesswork. Read the actual mesh and skeleton
before placing joints; a bounding box is a seed for fitting, not an anatomy measurement. Inspect
landmarks and joint centres in an overlay with the surface, then test bend ranges and deformed
silhouettes. One misplaced joint can spoil several downstream assets even if the build and reload
are clean. Verify what the saved mesh, skeleton and bound asset do in an animation, not only what
the rig document says.
For the handoff from Human identity to rigging, retargeting, body motion and facial animation, use
the [character composition guide](CHARACTERS.md).

## Groom: hair must be seen

Read the [Groom recipe guide](GROOM.md) before writing a whole `.gcgroom` document. The recipe
grows and styles a native Groom; its constraints, regions, flow, length and
guide preparation are intent, and the rendered strands are the result. First inspect the scalp,
flow directions and root distribution, then render the hairstyle from multiple angles. For a
gathered or braided design, confirm the actual constraint, route and pull order from the current
recipe and dry-run response; an ungathered first pull cannot establish a useful cross-section. A guide, contact count
or mean error can be green while a crown is flat or a layer intersects another. Judge the image
first, then use the numeric report to locate the cause. Check the Groom and Binding after a fresh
reload and, for an animated character, through the intended deformation and runtime path.

## Core Extension and Human: optional frameworks and identity

For trees, Core Extension owns the native `procedural_vegetation` graph adapter. Read
[VEGETATION.md](VEGETATION.md) to create/edit a tree, apply BPIR layout and preview it directly
with `gc_render`. World places the finished tree assets; Shape can supply supporting geometry.

Core Extension (`GraphCodeCoreExtension`, formerly Advanced) handles installed framework-backed kinds. Human authoring lives here:
`.gchuman` carries identity instructions and `kind:metahuman` reflects the resulting character.
Ask for the installed grammar and required engine plugins before using either. Verify the actual
character, materials, rig and expression or animation in a real render and runtime context. A
parameter round trip does not prove the resulting human looks or moves as intended. Keep licences
and provenance for any imported identity source or third-party content.
The [character composition guide](CHARACTERS.md) gives the cold-start order for Human, MetaHuman,
Rig and Move and separates body animation from the facial control path.

## Garment: fit clothing to the intended body

A `.gcgarment` owns clothing intent; its built garment or mesh is the result to inspect. Start with
the intended body, pose and outfit purpose. A render-only outfit does not need simulation merely
because a simulated workflow exists, while a simulated garment needs the supported cloth data,
collision and runtime proof. Check fit, coverage, material assignment, deformation, clipping and
silhouette from representative poses and camera angles, then save and reload the built artifacts.
Use [the Garment guide](GARMENT.md) for the full decision and evidence route. Armour is a garment:
Garment fits and skins the layers and Shape makes the hard parts it places.

## Integrate: bring an external source across a boundary

For a new machine or missing connection, follow [Blender setup](BLENDER-SETUP.md): the AI discovers
and installs the required components, configures the target project and verifies the connection.

**When importing from Blender, including BlenderKit assets or materials, use GraphCode Integrate
for material translation and managed reimport.** Start with the installed
`Plugins/GraphCodeIntegrate/README.md` for the companion, export and import workflow. Check that
Core and Integrate are installed and enabled; if Integrate is unavailable, report the missing
dependency before choosing an alternate material workflow. Do not silently replace this route
with a hand-built Unreal material or a full-material bake.

Export through the Blender companion or its documented command line, inspect the conversion
report and import plan, then deliver the `integrate.json` bundle through Integrate. Supported
nodes stay editable; use its selective bake fallback where available for untranslated branches
and inspect frozen controls, approximations and unmet requirements. A raw `.blend` passed to
`gc_import` is not this material conversion workflow.

**AI performs the transfer.** Run `export-mesh.py` for named objects in a saved Blender file, or
call the installed export API in a connected Blender session, then plan/apply/render with Integrate.
Handle installation, project configuration, selection and export preparation within the authorized
task. Do not stop at a ZIP link or tell the owner to click through the sidebar. The companion's
3D View panel is an optional manual interface, not a required human step.
The mesh bundle carries evaluated geometry, UV channels and converted material-slot assignments;
Integrate uses Core's native import and signed writes to deliver and reimport it. The AI verifies
the required Unit Scale 1 and physical dimensions without silently rescaling the original scene.
Rotation/scale are baked around each object's pivot; scene placement, skeletons and animation are
outside this static-mesh path. Material-only mesh baking remains available in the Shader Editor.

Compare the assembled Unreal result under real lighting: texture coordinates, colour, roughness,
normals and displacement can differ even when the graph compiles. Keep the original Blender
source and `Saved/GraphCodeIntegrate/imports` ownership records for managed reimport. Respect
external-edit protection and do not advertise unmapped source features as transferred.
