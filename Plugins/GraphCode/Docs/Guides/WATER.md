# Make convincing water, especially the material

*A GraphCode production guide for streams, rivers, lakes, oceans and waterfalls. Use with
[ASSET-QUALITY.md](ASSET-QUALITY.md) and [ENVIRONMENTS.md](ENVIRONMENTS.md#fit-water-and-caves-to-the-route).
Research checked 2026-10-03 against Epic documentation and the linked production talks.*

**Finish one convincing moving section before expanding the water across the level.** A water
actor, blue surface or successful material compilation is not a finished result. Judge the material,
motion, banks, bed, lighting and interactions together from the game's actual camera. This guide
combines sourced techniques with recommended production checks; it does not establish that a
particular GraphCode project has implemented or passed them.

## Choose the reference and receiving surface

Record the water type, physical size/depth, clarity, current/wind, weather, style, camera distances,
target platform and whether the player enters it. Use moving references showing both the surface
and shore. Identify quiet regions as well as energetic ones; extra foam and stronger normals do
not automatically improve quality. Make a short project plan following
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md) when the implementation is unfamiliar.

Build a small test location containing shallow and deep areas, a curved bank, an intersecting rock
and a distant view. Include a drop or a lake junction only when the intended water needs one.
Hold camera, lighting and exposure fixed for before/after comparisons, then test changed conditions.
Compare a neutral diagnostic surface with the finished material to separate geometry from shading.

Start with the installed Unreal Water system when its body types fit. Epic supplies related river,
lake and ocean material instances; rivers have flow controls and river-to-lake/ocean transitions,
while lakes and oceans use waves. Inspect the actual assigned instance, parent and switches before
making a project-owned variant. Follow [Epic's water rendering reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/water-meshing-system-and-surface-rendering-in-unreal-engine?application_version=5.7).
Preserve a suitable existing shader. A custom pool, stream mesh or waterfall can need a different
surface, but still owes the material and playback checks below.

## Build and tune the material in stages

### 1. Establish optical depth before adding detail

For the conventional Unreal path, inspect **Single Layer Water** first. It provides volume
scattering/absorption and surface reflection/refraction using an Opaque or Masked material with a
special water pass. Do not copy ordinary Translucent-material instructions onto it. Its water
output exposes `Scattering Coefficients`, `Absorption Coefficients`, `PhaseG` and
`Color Scale Behind Water`; main-node Opacity controls the balance of volume and surface response,
not ordinary alpha transparency. See [Epic's Single Layer Water model](https://dev.epicgames.com/documentation/unreal-engine/single-layer-water-shading-model-in-unreal-engine?application_version=5.7).
Check the installed parent: an artist-facing parameter may transform values before these inputs.

Start with quiet normals and foam disabled. Tune against a known bed and shallow/deep test areas:

| Control or observation | Intended decision | Check before increasing it |
|---|---|---|
| Absorption | How strongly the water removes light, by color channel | More red absorption removes red; it is not a red color tint. Check depth and units in the parent. |
| Scattering | The body of the water and its suspended-particle appearance | Excessive scattering can look milky; clear water should retain appropriate bed visibility. |
| PhaseG | Directional character of scattering | Compare light-facing and opposite views, rather than fixing one camera only. |
| Behind-water color scale | Deliberate adjustment of the submerged scene | Avoid using it to conceal incorrect bed lighting or exaggerate fake caustics. |
| Surface/volume balance | The intended combination of reflected surface and water interior | Verify the shader's Opacity semantics before treating it as a transparency slider. |

Choose controls that let an artist express clarity and color without confusing coefficient direction.
Expose ranges and units from the actual shader. Avoid copying universal numbers from an unrelated
pool into a river with different depth, scale and sediment. Base color alone does not establish depth.

### 2. Establish reflections and surface roughness

Water is a dielectric: keep Metallic at 0. Use roughness to control highlight/reflection spread;
do not make water metallic to strengthen reflections or drive Specular arbitrarily to its maximum.
Epic lists 0.255 as a measured water reference for the conventional Specular input; use it only
where that input retains its standard meaning. Preserve correct IOR-based mappings in other
shading workflows. [Epic's PBR reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/physically-based-materials-in-unreal-engine?application_version=5.7)
explains roughness, dielectric specular and their different jobs.

Verify the environment has something meaningful to reflect: sky, banks, canopy or structures.
Test the actual renderer, reflection method and quality tier. Diagnose reflection loss separately
from the material. For a screen-space path, inspect screen-edge and occlusion changes while turning
the camera; its screen data is not a guarantee of complete scene reflection. Consult
[Epic's screen-space reflection controls](https://dev.epicgames.com/documentation/en-us/unreal-engine/screen-space-reflections-in-unreal-engine?application_version=5.7).
Do not add an expensive reflection solution before isolating the failure and measuring the tradeoff.

### 3. Add coherent flowing normals at several scales

Plan broad surface motion, medium ripples and fine highlight breakup separately. Match wavelengths
and speed to the reference and world scale. Combine tangent-space normals with a suitable normal
blend, rather than multiplying their RGB colors. Verify tangent basis, normal import settings and
direction. Reduce unresolved fine detail at distance if it shimmers; inspect the result in motion.

For rivers, use the water body's flow data or an authored flow map so motion follows bends and
obstacles. Valve's [Water Flow in Portal 2](https://cdn.cloudflare.steamstatic.com/apps/valve/2010/siggraph2010_vlachos_waterflow.pdf)
describes direction/speed maps and two offset animation phases blended to conceal each reset.
Use this as a pattern to study, not an assertion that the installed graph already implements it.
Verify vector encoding, coordinate space and timing before connecting a flow map. Let surface
streaks and transported foam agree with the current; inspect loop boundaries and stretched textures.

Avoid using random rotation from a ground-material anti-tiling recipe indiscriminately on water:
rotating a directional sample without its flow and normal directions can break coherence. Follow
[TEXTURE-VARIATION.md](TEXTURE-VARIATION.md) for sampling quality, but preserve the water's motion.
Keep normal strength modest enough that the surface reads as water rather than crumpled foil.

### 4. Tune refraction against real submerged objects

Inspect stones, bank edges and a submerged test object from near-vertical and grazing views.
Start with restrained distortion. Excessive normal amplitude or refraction can produce detached
edges, smears and a thick-glass appearance. Diagnose each independently and inspect screen borders.
The [general refraction reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-refraction-in-unreal-engine?application_version=5.7)
describes available refraction methods; it is not a universal pin recipe for Single Layer Water.

Add caustics only when the reference, water clarity and lighting call for them. Treat projected or
animated caustics as a deliberate approximation: confine them to appropriate submerged surfaces,
check depth/light response and prevent patterns leaking onto dry banks. Do not make the whole bed
emissive to compensate for missing light transport. Confirm caustics improve the scene after the
base optical response already works.

### 5. Make foam a separate surface treatment with a cause

Separate **where foam forms**, **how it travels** and **how it breaks up and disappears**.
Consider obstacle contact, rapids, wave breaking, shore wash and impact zones only where appropriate.
Keep quiet water comparatively clear. Use a coverage mask to drive the intended foam color,
roughness and normal treatment together; a white emissive stripe is not convincing lit foam.

Rare's [Sea of Thieves production paper](https://history.siggraph.org/wp-content/uploads/2022/09/2018-Talks-Ang_The-Technical-Art-of-Sea-of-Thieves.pdf)
describes foam from wave peaks and object intersections, feedback-based dispersal, artist-authored
texture breakup and different treatments for calm and stormy water. The reusable lesson is to
give foam an origin, evolution and art direction; copying that game's whole simulation is unnecessary.

For a simpler stream, start with authored obstacle/rapid masks and advected texture breakup.
Use simulation when changing interactions require it. Depth proximity alone does not explain all
foam: a still pond need not acquire a bright ring around every stone. If screen depth drives a mask,
test camera movement for unstable bands. Check the chosen shader's depth access before using a
translucent `DepthFade` example; material domains and water passes differ.

### 6. Finish the material controls and source handoff

Organize instance controls by optics, surface roughness, normal scales, flow, foam and optional
effects. Reuse functions for shared calculations. Keep meaningful continuous controls adjustable;
use static switches only for intentional shader variants. Record which controls were baked or frozen.
Document each parameter's purpose, units, useful range and the view that proves its effect.

Blendkit/Blender can supply bed materials, foam artwork, ripple textures and shoreline assets through
the [Integrate sourcing workflow](CONTENT.md#use-blendkit-generators-and-materials-through-integrate).
A translated Blender glass/water preview does not establish Unreal water shading, flow, reflection
or interaction support. Check Integrate's conversion report and compose the required native water
material/effects through supported GraphCode authoring. Preserve imported source assets for reimport.
Terrain IHP/RVT may help the surrounding ground; they do not supply a complete water material.

## Compose a recipe for the actual water type

These are starting decisions to test against the brief, not universal defaults:

| Type | Material and motion priorities | Representative section to finish first |
|---|---|---|
| Stream or river | Flow following the channel; coherent ripples/streaks; shallows versus pools; local obstacle/rapid foam | A bend with a rock, shallow margin and deeper channel. Check inlet, middle, outlet and any lake junction. |
| Pond or lake | Restrained wind ripples; credible reflections and clarity; locally appropriate shore detail | A sheltered bank, open water and a submerged object. Reject uniform ocean-like foam or exaggerated current. |
| Ocean | Broad wave shape plus smaller surface detail; wind/sea-state coherence; breaking foam; near/far continuity | Shore, open water and horizon in one camera route. Compare calm and rough states if both are needed. |
| Waterfall | Designed lip, falling sheet/ribbons, breakup, impact foam, spray and mist | The complete upstream-to-pool passage. Check seams, thickness, motion direction, lighting and overdraw. |

An ocean may justify spectral/FFT waves; Rare's paper is a production example. A small pond does
not inherit that requirement. A waterfall needs appropriate geometry and VFX composition; do not
assume a steep river spline automatically produces a convincing falling sheet. Source or author
those assets with the same quality bar and verify their transition into the receiving pool.

Fit bed and banks before decorating them. Use localized wetness, sediment and submerged material
detail suited to the environment. Dirt on a bank, suspended sediment, floating debris and foam are
different phenomena and should not all become one noise overlay. Preserve quieter surfaces.
For carving, Water Zone, mesh tile and shoreline-resolution diagnosis, follow the existing
[environment guide](ENVIRONMENTS.md#fit-water-and-caves-to-the-route); do not duplicate its terrain cut.

## Add interaction and underwater views when needed

Prove a footstep, moving obstacle or boat wake using the game's actual actor. Align visual surface,
queried height, collision and any buoyancy/swimming logic. A shader ripple does not establish physics.
Start with a bounded effect or simulation around meaningful interaction. Epic's
[shallow-water Niagara tutorial](https://dev.epicgames.com/community/learning/tutorials/Ddwx/unreal-engine-shallow-water-simulation-with-niagara-fluids)
is a reference for 2D water and character interaction; inspect its installed dependencies and version.
Do not assume the system is configured merely because a plugin or console flag exists.

If the camera can submerge, verify the waterline, underwater appearance, visibility and sound
transition. Native Water Bodies use underwater post processing with bounds/collision requirements;
crossing between bodies also needs inspection. Consult
[Epic's Water Body documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/water-body-actors-in-unreal-engine?application_version=5.7).
Do not spend on underwater effects when the intended camera never enters water.

## Diagnose the visible failure before changing settings

The entries below are hypotheses to isolate, not diagnoses from a description alone. Disable one
layer at a time, use the same camera, and retain before/after evidence.

| Symptom | First controlled check |
|---|---|
| Flat blue paint or plastic | Quiet normals and no foam; inspect bed depth, optical controls, roughness and reflection support separately. |
| Milky shallows or invisible bed | Inspect scattering, absorption, surface/volume balance and actual bed lighting/depth. |
| Mirror foil or glitter | Reduce normal strength/frequency; check roughness, mip filtering and temporal stability. |
| Conveyor-belt movement | Visualize flow vectors through bends; check UV basis, speed, looping and foam direction. |
| Identical repeated ripples | Compare scales and phases; preserve flow direction while breaking repetition. |
| Bright uniform shore outline | Disable the foam mask and inspect its depth dependence, width, causes and camera stability. |
| Stepped edge, holes or flooding | Inspect the water-only mesh, terrain and collision; separate tile coverage, Water Info and bank geometry. |
| Reflections vanish when turning | Check the actual reflection path and its screen-space/fallback behaviour before changing water color. |
| Distorted halos around stones | Disable refraction, then restore it gradually; isolate normals and depth discontinuities. |
| Different color at a river/lake junction | Inspect body and transition material assignments, parameters and underwater volumes. |
| Poor frame time | Disable foam, extra normal samples, refraction/reflection options and simulation individually; measure the dominant cost. |

## Author, play and verify through GraphCode

Discover the installed water/world, material, material-instance and Niagara authoring support using
tool help and `gc_describe`. Read the assigned assets with `gc_read`; use supported `gc_apply` writes
and inspect the result. Tool help owns exact grammar. A named engine feature or third-party package
does not establish GraphCode coverage. Record any unavailable capability and choose a supported
implementation that still meets the visual goal; do not silently drop the effect or claim it worked.

Complete these checks before expanding the first finished section:

- **Material isolation:** optics, reflections, normals, refraction and foam each have an observed
  purpose; inspect the combined result under the intended lighting and a changed sun/view angle.
- **Continuous playback:** inspect close, middle and far views, grazing angles, camera turns and
  animation resets. Reject shimmer, texture swimming, repeated bands, popping and frozen effects.
- **Connections:** cover every bend, bank, obstacle, body junction, waterfall lip and impact where
  present. Follow the actual player route instead of selecting only flattering viewpoints.
- **Interaction:** exercise relevant footsteps, wakes, entry/exit and repeated triggers. Listen to
  spatial water sound; verify visuals, timing and sound agree.
- **Cost:** record target resolution/quality, GPU frame time, mesh and effect cost, memory and
  representative worst views. Use [Epic's water diagnostics](https://dev.epicgames.com/documentation/en-us/unreal-engine/water-debugging-and-scalability-options-in-unreal-engine?application_version=5.7)
  such as `stat watermesh` and water mesh visualization; use a frame profiler for material cost.
  Raising all resolutions or tessellation settings is not a measured fix.
- **Persistence and delivery:** save/reopen, repeat playback, and test applicable streaming and
  packaged paths. Keep captures, reference comparisons, source/version details, parameter choices
  and unresolved repairs in the project's water note using [VALIDATION.md](VALIDATION.md).

Never accept a static screenshot as proof of flow, foam evolution, interaction or a smooth waterline
transition. The final evidence must include observed playback of the actual authored result.
