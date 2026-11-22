# Build an environment the owner can walk through

Use this with the [authoring guide](AUTHORING.md) and [content preparation guide](CONTENT.md).
Ask the installed tools for grammar. A saved world program is the start of verification, not
proof that its terrain, water, plants and collision agree.

For streets, settlements and entrances, follow [roads and buildings](ROADS-AND-BUILDINGS.md)
before laying out repeated assets. Plan the road network, terrain grades, plots, sidewalks and
door thresholds together, then verify one complete street-to-interior route. Use the
[assembly checker](ASSEMBLY-VALIDATION.md) for the supported placed meshes and their connections.

For Unreal Blueprint brush assets and named Landscape edit layers, see
[native Landscape brushes](NATIVE-TERRAIN.md), including its tested boundaries.

## Prepare a small, finished asset palette

For rocks, cliffs and surface materials, actively follow the
[Blendkit generator and material workflow](CONTENT.md#use-blendkit-generators-and-materials-through-integrate):
find/download a suitable source, produce and tune it in Blender, then send through GraphCode
Integrate and verify in Unreal. Build a coherent family from retained generator settings before
scattering it; preserve material scale and the [texture variation checks](TEXTURE-VARIATION.md).

Choose a biome, a player route and a few views before multiplying assets. Keep original source
files and licences. Import individual tree or plant variants rather than an entire source-file
arrangement. Read the imported material slots and connect colour, normal, roughness and opacity
maps by their actual roles. Inspect a mature tree, sapling, fern and grass clump with their real
materials before scaling up the population.

Check scale, pivots and all supplied LODs together. A correct source-file bounding box does not
prove the importer preserved its transform. Reset or repair the asset through the project's
existing preparation tools, then verify the resulting mesh. Avoid compensating for a bad asset
pivot in every placement. Recheck collision after a geometry transform.

For foliage, inspect leaf silhouettes, two-sided lighting, restrained wind, shadows and distant
views. Use actual scene materials when judging appearance: a geometry preview that substitutes
a neutral material cannot verify colour, cutouts or normal response.

## Build ground before scattering onto it

If streaming is required, create a World Partition map with streaming enabled from the outset.
Read the new map before editing its actors; preserve the WorldSettings entry the engine created.
Keep `.gcw` terrain/region intent and the level's reflected `.gc` as their respective records.

Load the part you want to inspect through `gc_open`, after opening the map:

```json
{"action":"load_region","bounds":{"min":[0,0,-1000],"max":[50400,50400,16000]}}
```

Bounds are world centimetres. The editor keeps that region loaded across tool calls, so a
subsequent `gc_read` or `gc_render` can see its actors. Repeating identical bounds reuses the
region. `action:"list_regions"` reports bounds, loaded state and region IDs;
`action:"unload_region", region_id:"<returned id>"` releases one. IDs belong to the current
map/session. Other loaders can keep overlapping actors resident. These actions require
Play/Simulate to be stopped; they do not activate Data Layers or prove runtime streaming.

### Place a reusable PNG height brush

`stamp` samples a **single-channel 8-bit or 16-bit grayscale PNG** at any image resolution of at least
2×2. It places that image on the terrain without resizing the file to the Landscape grid:

```gcw
terrains {
  Ground = terrain(base=flat(z=500)) {
    Hill = stamp(source=file'Brushes/Hill.png', at=(25000,25000), width=20000, length=16000, rotation=25, range=(Min=-500,Max=3000), falloff=2000, strength=0.8, blend=add)
  }
}
```

Black maps to `Min`, white to `Max`, with bilinear sampling and no colour/gamma conversion.
Choose how the image combines with the current ground using `blend`:

| Operation | At full influence | PNG range means |
| --- | --- | --- |
| `add` (default) | Current height + brush height | Signed offsets in cm |
| `subtract` | Current height − brush height | Signed offsets in cm |
| `replace` | Brush height | Absolute world Z in cm |
| `min` | Lower of current and brush heights | Absolute world Z in cm |
| `max` | Higher of current and brush heights | Absolute world Z in cm |
| `average` | Midpoint of current and brush heights | Absolute world Z in cm |

`strength` is 0–1, default 1. Strength and falloff blend from the current height toward the
operation result. Negative `add` values dig; subtracting negative values raises the ground.
`average` uses the current surface at that instruction, not a running average of all stamps.
The old `merge` spelling is still accepted and writes back as `add`. Min/max act like surface
intersection/union; these heightfield operations cannot create caves or overhangs.

`width` and `length` are the full local X/Y footprint in centimetres, with positive rotation
turning +X toward +Y. The first image row is local -Y, and the first column is local -X.
`at` also accepts existing named anchors.

`smoothness=200` softens the image over a 200 cm radius before sampling, independently of
boundary falloff. The default is zero, preserving the source. This can soften steps in 8-bit
inputs, but cannot recover missing precision. Prefer 16-bit heightmaps for detailed relief;
recorded `sculpt` files still require exact 16-bit data. Smoothness uses a separable moving
average with fractional pixel weights, measured using each axis's world spacing.

`falloff` is the distance inward from the closest rectangular edge: a smooth transition from
zero influence at the edge to full influence that far inside. It defaults to 10% of the smaller
footprint dimension. Zero gives a hard edge; a value larger than half the smaller dimension
reduces the maximum influence. Existing terrain filters can restrict the stamp further.
Noise, warp, radius and scalar profiles are refused for this operation.

Use [TextureBrush.gcw](../../Resources/Examples/NativeTerrain/TextureBrush.gcw) and its neighboring
`AsymmetricHeightStamp_Gray16.png` for a reusable hill, ridge and depression example. Copy both
together beside your world program or adjust `source`. Recorded `sculpt` files retain their
exact whole-grid replay behavior; recording later hand edits leaves the reusable brush untouched.
The [mountain recipe](../../Resources/Examples/NativeTerrain/TextureMountain.gcw) enlarges and
reuses the same image for a main summit and shoulder, then adds surface variation and erosion.
The [operations recipe](../../Resources/Examples/NativeTerrain/TerrainOperations.gcw) combines
all six modes to build a mountain with a shoulder, valley, lookout terrace, and foothills.

### Shape a mountain system, not a single peak

Terrain instructions run **in order** over one heightfield. One `raise` with a large radius
produces a rounded hill. For a walkable mountain landscape, start with a changing ridgeline,
branch into lower spurs, add locally shaped summits and drainages, then add detail and weather
the result. Reserve the route, river and shelter while choosing those landforms. Use actual
centimetres and inspect the silhouette and slopes at player height after each pass.

```gcw
gcw 1  level:/Game/MyGame/Maps/L_Valley

world {
  spine_a = (2000, 44000)
  spine_b = (15500, 39200)
  spine_c = (30000, 40500)
  spine_d = (50000, 45500)
  spur_b = (9000, 30000)
  spur_c = (12000, 23000)
  gully_a = (23500, 38500)
  gully_b = (25000, 31500)
  gully_c = (28000, 27000)
  route_a = (8000, 8000)
  route_b = (15000, 16000)
  route_c = (28000, 23000)
  spine = course spine_a -> spine_b -> spine_c -> spine_d
  spur = course spine_b -> spur_b -> spur_c
  gully = course gully_a -> gully_b -> gully_c
  route = course route_a -> route_b -> route_c
}
terrains {
  Valley = terrain(base=flat(z=0), stream=tiles(components=4)) {
    Roll = perlin(seed=12, frequency=0.00005, amplitude=180, octaves=3)
    Spine = ridge(along=$world.spine, width=30000, height=(0=900, 0.35=2400, 0.65=3300, 1=1400), falloff=15000, slope=(0=0, 0.3=0.08, 0.7=0.62, 1=1), noise=(seed=13, amplitude=140, frequency=0.0004), warp=(seed=16, amplitude=600, frequency=0.0002))
    Spur = ridge(along=$world.spur, width=13000, height=(0=1600, 0.5=800, 1=100), falloff=6500)
    Fracture = ridged(seed=14, frequency=0.00035, amplitude=330, octaves=4) [all(height=(Min=1400))]
    Drainage = carve(along=$world.gully, width=4500, depth=(0=40, 0.6=200, 1=90), falloff=2250)
    Weathering = erode(seed=15, iterations=25000, strength=0.22, talus=35, deposition=0.35, evaporation=0.05) [all(height=(Min=750))]
    RouteGrade = smooth(along=$world.route, width=6000, iterations=4, strength=0.58)
  }
}
```

Keep each instruction on one line in the `.gcw`, as shown.
Declare named points before a `world` course and connect their names. Inline coordinates in a
`world` course are refused as `gcw_bad_edge`; inline point pairs remain valid for a terrain
instruction's `along=` domain.
`width` on a written course spans the full corridor, so `width=13000` reaches about 65 m to
each side. `radius` or half the course `width` is the outer reach; `falloff` is the width of
its outer ramp. When `falloff` covers the full reach, the whole shape ramps to its crest.
`height=(0=…, 1=…)` and `depth=(0=…, 1=…)` vary **along** the declared course;
`slope=(0=…, 1=…)` varies **across** it from edge to crest. `raise` has a smooth default flank;
`ridge` and `carve` use a straight flank unless `slope` shapes it. `noise` adds local vertical
variation to one instruction; `warp` shifts its sample position sideways. Independent
`perlin`, `ridged` and `billow` lines add broad, folded and rounded relief respectively.
`frequency` is sampled against world coordinates in centimetres: `0.0004` yields a feature
scale of roughly 25 m. A terrain instruction's `slope=(0=…, 1=…)` is a unitless flank
profile; a filter's `slope=(Min=18)` is **18 degrees** on the ground already built.

Noise instructions can stack. Use separate names and seeds for broad variation, medium
breakup and fine ground relief so each contribution can be inspected and adjusted alone.
For example, `frequency=0.000041666667` is approximately 240 m, `0.00025` is 40 m and
`0.00125` is 8 m. Reduce height amplitude as the features become smaller. Place erosion
after the landforms it should weather and fine relief before the final camp/trail grade.

Current limits: the `perlin` spelling uses smooth value noise internally. `ridged` and
`billow` fold each octave of that sampler. Successive octaves always double frequency and
halve amplitude; their sum is normalized. Inline `noise=(...)` and `warp=(...)` each use
one octave. Gain, lacunarity, directional stretching and custom remapping are not exposed.
At 100 cm landscape spacing, an 8 m band has several samples across it; centimetre-scale
grain needs material/mesh detail. Check the smallest octave's scale, not only the first.

On a directed course, noise `amplitude` and `frequency` can also be profiles, for example
`amplitude=(0=0, 1=180)`. This varies the quantity along the course, while the domain
width/falloff still controls its reach across it. A frequency profile scales the sampled
world coordinates at each station; it is not a constant-spacing sweep along the course.
The 23 September repair makes these already accepted profiles reach the heightfield;
older builds could accept them while reading a missing constant as zero.

Filters such as `[all(height=(Min=1400), slope=(Min=18))]` read the ground left by the
instructions **above** them, so their order matters. `smooth` and `erode` also compute from
the current heightfield, then blend their result through the stated domain/filter. Use
bounded erosion and inspect whether it removed the shapes you intended to keep.
For `erode`, `iterations` counts hydraulic droplets across the whole heightfield, not passes
over every vertex. A value such as 8 barely affects a large terrain. `talus` is the angle of
repose in **degrees**, clamped to 1–89; a value above that range effectively turns off
thermal settling. Start with a moderate angle such as 35 degrees and a droplet count
appropriate to the grid, then inspect the result. A Water body that carves a river owns its
narrow channel; a separate broad terrain valley may
describe the surrounding land, but carving the same channel twice makes a trench.

Measure a river course's height **in flow order**, including its outlet. `carve(depth=…)`
subtracts from existing ground; decreasing depth downstream does not guarantee downhill
drainage. A watershed trial descended to a confluence and then climbed toward its exit.
Use measured heights to author the final grade and check it again after erosion/smoothing.
Likewise, survey a trail before choosing `flatten(to=…)`: a rising profile through low ground
can build an implausible embankment even when every instruction works correctly.
Height and slope filters have hard boundaries. Large displacement behind a threshold can
produce a ledge at that boundary; use smaller contributions and deliberate blending, and
inspect silhouettes with a neutral material before spending time on surface detail.

For an eroded shore, decide which effect changes the ground: bank retreat, channels,
beach grade and sizeable sediment deposits belong in geometry; wet colour, roughness,
grain and small apparent relief belong in the material. Plan shared masks so visible
sediment agrees with the geometry and planting. Current `erode` is a downhill-droplet
and thermal-settling operation, not a wave/shore simulator, and does not export sediment
or wetness masks. Its domain blends the simulated result into the original field; it
does not confine simulated flow to that domain. Do not promise otherwise from its name.

Recorded sculpts store their heights in **single-channel, 16-bit grayscale PNG** under
`/Game/_GraphCode/Terrain/<level>/`. Each pixel holds one exact terrain sample, not three
RGB copies. The `.gcw` keeps the height range, dimensions and base hash:

```gc
  Ground = sculpt(source=file'/Game/_GraphCode/Terrain/Maps/Overworld/Ground.png', size=(505, 505), range=(Min=-25600.0, Max=25599.2188))
```

A sculpt in the program's first instruction position replaces the ground; later sculpts add their contribution.
A saved sculpt replays after the program's stamps above it, so where it opposes one (a pit stamped after the landscape was last saved flat) it overrides that stamp on every apply; the save names each such stamp in a warning.
Keep PNGs at 16-bit grayscale and at the declared dimensions when editing them externally;
color conversion, normalization and resizing change the heights. PNG compression itself is
lossless. Existing raw `.r16` references still work and retain their encoding when another
sculpt merges into them. Commit the referenced height files along with the `.gcw`.

The example is a syntax and composition starting point, not a finished heightmap. Add
asymmetric tributary spurs, local rock geometry, surface changes and planting clusters
where the actual camera and walk route require them. Measure crest heights, ground slopes,
water banks and route collision after realisation. If a profile or filter refuses, use the
tool's grammar/refusal and a dry run to correct the line before applying the world.

### Make the landform read as geology, not instructions

Judge three scales separately: the whole watershed from above, the next 50–100 m along the
route, and the ground around the player's feet. A regular chain of ridge, spur and gully
stamps remains regular even after small noise is added. Start with an asymmetric catchment:
unequal tributary lengths, bends and junctions, offset saddles, a locally broad floodplain,
and a few steep rock faces. Use `ridge`, `carve`, `perlin`/`ridged`, `warp` and bounded `erode`
for different spatial scales; do not repeat one profile at a uniform spacing. Keep the route
and water course physically related to the drainage instead of laying an independent straight
path across it. Local `flatten` and `smooth` are for small authored places, not the main
source of relief. Compare an overhead render with a player-height view before planting hides
the shape.

This is a useful production pattern, not a claim that one formula builds a finished world.

Judge erosion against a deliberate outcome: connected drainage, a retained rock feature,
a plausible deposit and a protected route. Compare with the same ground before erosion.
Record the grid spacing and compare at player height as well as overhead. A repeatable
result that merely changes many samples does not establish a useful weathering process.
More droplets increase the current operation's coverage; they do not extend each droplet's
fixed travel limit. Recheck the river outlet and walking slopes after every substantial pass.

Keep a useful recipe with the level: the named regions, operation order, seeds, units,
before/after views and measured collision. Diagnose geometry with a neutral material,
then evaluate actual PBR surfaces and vegetation. A mask used to colour deposited soil
must agree with the landform; a height-change mask alone does not prove flow or wetness.
[Ubisoft's Far Cry 5 world-generation talk](https://www.gdcvault.com/play/1025557/Procedural-World-Gen)
describes tools for biomes, terrain texturing, freshwater and cliff rocks that still permit
local tuning. [Ironwood's Pacific Drive talk](https://gdcvault.com/play/1035387/Twisting-Terrain-and-Populating-Forests)
describes refining rough sculpts into shippable landscapes with material-layer maps, biome
blending, roads and foliage driven by the same terrain data. Use that shared-data principle:
height, slope, drainage/moisture and path distance should inform both material and planting.

### Build a complete PBR landscape surface

Use a small palette of real surfaces such as leaf litter, grass, wet mud and rock. For each
surface import its matching base colour, normal and roughness; use AO where supplied and a
height map for height-aware blending or material relief. Import normal maps as normal maps
with the correct DirectX/OpenGL orientation, and keep roughness, AO and height linear rather
than sRGB. Verify one material on a known mesh before distributing it across the landscape.
A filename containing `normal` does not prove that Unreal imported it with the right settings.

The same principle used for Steampunk wear applies to nature. **Macro** variation covers the
landform, vegetation zone, drainage and exposure over tens of metres. **Meso** variation
breaks those areas into damp pockets, scree, leaf fall, paths and rock outcrops. **Micro**
detail supplies soil grain, cracks, pores, loose leaves and roughness. Keep the three scales
independent. More micro noise cannot repair a regular mountain silhouette, and uniform dirt
cannot substitute for water-driven deposits. Broad masks must not form repeating sine bands;
fine textures must not tile visibly across the valley.

Use [TEXTURE-VARIATION.md](TEXTURE-VARIATION.md) to choose and verify a repetition-reduction method:
`TextureVariation`, reusable near/far surface functions, `CameraDepthFade` driving
`Blend Material Attributes`, and independent macro variation where useful. Preserve normal
orientation and meaningful texture scale; inspect the transition in motion and measure shader cost.

Blend base colour, normal, roughness and AO with the **same** surface mask. Let ground cover,
damp banks and exposed rock follow meaningful height, slope and drainage conditions; soften
their boundaries with texture height and noise. On steep surfaces use triplanar or suitably
oriented rock sampling so horizontal UVs do not stretch. Discarding the rock normal or giving
all layers one roughness makes a rock face read as painted dirt. AO is restrained contact
information, not a second dark colour layer. Inspect at noon and in softer side lighting:
a bright directional light can conceal flat normal response and wash plausible green into tan.

The material can use procedural masks, or named Landscape paint layers if the installed
`gc_describe`/`gc_apply` surface can create their Layer Info assets and apply the weightmaps.
Check those operations before writing an example that assumes they exist. Unreal's
[Landscape Materials](https://dev.epicgames.com/documentation/unreal-engine/landscape-materials-in-unreal-engine)
guide explains weight and height blending. The visible material, Layer Info assets and
weights on the actual Landscape must agree; a valid graph alone proves none of that.

For extra apparent relief, distinguish three mechanisms. A height-aware layer blend changes
which material appears at a texel. Parallax or heightfield projection changes perceived
surface depth but leaves collision and river-bank geometry alone. Nanite tessellation or
additional geometry changes visible shape and has a separate rebuild/performance cost.
[RVT](https://dev.epicgames.com/documentation/unreal-engine/runtimevirtual-texturing-quick-start-in-unreal-engine)
can cache complex landscape shading and help static rock, road and landscape materials share
a blend, but its asset, material outputs, volume and component bindings all need to be
configured and verified. The IHP heightfield-projection example is a proprietary UE 5.8
plugin; it is an optional reference for material depth, not a prerequisite or a replacement
for terrain and collision. Check your engine and plugin version before depending on it.
For researched selection criteria, camera-dependent shading versus RVT caching, and the terrain
comparison test, read [IHP and RVT guidance](TEXTURE-VARIATION.md#consider-ihp-and-rvt-for-terrain-relief-and-integration).

### Bake the parts that benefit from it

Keep the editable source for every bake. GraphCode Shape's mesh-material bake can collapse a
static, UV-unwrapped rock or cave assembly into a shared atlas: mark each contributing build
line `[bake=Outcrop]` and declare `M_Outcrop = bake(Outcrop, resolution=2048)` in `build { }`.
Ask the installed Shape tool help for the exact document and output contract before running
it. This bake needs a GPU editor, an appropriate UV0 unwrap, and enough resolution for the
player's closest view. Check the baked colour, normal and roughness on the **placed** meshes;
an atlas that compiles but smears at the entrance is a failed bake. Keep separately moving,
translucent or gameplay-relevant parts out of a static atlas when the tool reports them as
incompatible.

GraphCode Integrate can translate supported Blender shader nodes and selectively bake
unsupported UV branches. Its mesh-dependent option must use the same evaluated mesh and UVs
later imported into Unreal. Read its conversion report for frozen controls and approximations.
Neither mesh bake is a blanket operation on a World Partition Landscape, and an RVT is a
runtime shading cache rather than an offline material atlas. Use the route whose output can
be inspected and reauthored without losing the source.
If `gc_apply` reports `gcw_write_failed` after a level save, take its wording literally:
the landscape or foliage may already have changed even though the `.gcw` write did not
commit. Read the current `.gcw`, run a fresh dry-run to inspect changed or removed entries,
then apply again only when that plan matches your intent. This occurred in the Wilderness
trial when level-save projection ran during `.gcw` apply. The recorder guard was extended to
foliage, terrain and water, and a subsequent public river edit wrote the `.gcw` and saved the
level in one apply. Confirm the final level is saved and the authored `.gcw` matches it before
further edits.

Use environment trials to expose and close missing GraphCode capabilities. If completing a
step requires manual editor control, implement and verify the missing public tool path and
update its help before continuing the trial. A manual workaround does not pass that capability.

## Populate the environment

First realise the terrain and apply its landscape material. Inspect the visible surface and walk
or trace the ground after Unreal has updated its landscape layers and collision. Compare the
program's computed heights with the actual landscape, including its streaming proxies. A zero
height range on the root actor alone does not establish that a partitioned terrain is flat.
Do not infer a missing rendering device from that number.

To author the plants, use [VEGETATION.md](VEGETATION.md): UE5.7 presets/external plants and
UE5.8 growth/extraction, species components, materials, wind and performance. Choose a coherent plant family
and finish one representative patch before expanding it. The scatter below consumes finished
static meshes and foliage types; PVE graph authoring, native mesh export and animated skinned
placement are separate operations.

Reserve the route, clearing, water bank and cave entrance as explicit planting exclusions.
Generated foliage traces supporting world geometry and skips existing foliage, including actor
foliage. Otherwise a collision-enabled plant can become the next apply's ground, lifting the
population on repetition and causing the save recorder to mistake that height difference for
a hand edit. Enable the intended foliage collision and test repetition with it enabled; a
successful test using non-colliding plants does not cover this case. A mesh placed as ordinary
world geometry can still support planting.
In a foliage type's canonical `values` block, collision uses separate reflected paths:

```gc
  BodyInstance.CollisionEnabled = QueryAndPhysics
  BodyInstance.CollisionProfileName = BlockAll
```

Read the current `.gc`, preserve its signature and other fields, and dry-run the edit first.
An abbreviated `BodyInstance = (...)` is not the projected shape of this values block.
The mesh also needs suitable collision geometry: a foliage profile alone cannot create it.
Set cull distance beyond the test camera distance before interpreting missing trees as lost
instances; a saved count and a rendered count answer different questions.

For a generated `foliage(...)` scatter, `height=(Min=…, Max=…)` tests the traced ground's world Z
in centimetres, and `slope=(Min=…, Max=…)` tests its surface normal in degrees from horizontal.
The bounds are inclusive; omit either bound to leave that end open. For example,
`[all(not(inside=$Trail), height=(Max=1800), slope=(Max=28))]` leaves the trail empty and plants
only below 18 m on slopes up to 28 degrees. Ground tests can combine with region exclusions
in `all(...)`. A mixed numeric and region test inside `any(...)` or `not(...)` is refused;
`pcg(...)` and the volume verbs still refuse per-sample height/slope masks. A painted
`foliage(source=...)` forest already has fixed positions and takes no filter. Verify actual
instances and the tool's installed help rather than assuming a terrain filter and a foliage
filter read the same surface.

Run a small population first and examine its ground contact. Then increase density in measured
steps. Keep the seed and configuration with the evidence. Repeat unchanged input, change one
setting, save and reopen; check that instances neither duplicate nor drift. Verify the actual
instances, because the foliage-type asset alone cannot establish placement behaviour.

## Fit water and caves to the route

Before authoring the water's appearance, read [WATER.md](WATER.md). It covers the material first:
optical depth, reflections/roughness, flowing normals, refraction, foam and shoreline treatment,
then water-type recipes and playback checks. Finish one convincing moving section before expanding.

Let one mechanism own river carving. If the Water body carves the landscape, an additional
terrain carve on the same course can apply the cut twice. Inspect the water zone, surface,
channel, banks and collision together; a saved river spline does not prove visible flowing water.
Before creating the first water body, GraphCode adds Unreal's `Water` edit layer above the
existing landscape layers. A dry run reports that planned layer addition without changing the
landscape; a repeated apply keeps the same layer and body. Inspect the final layer stack and
confirm the water operation saved.

At a stream only a few metres wide, inspect the shoreline close up and from above. A
stair-step every landscape quad is a geometry or brush-resolution symptom; higher-resolution
water shading alone cannot move the bank or its collision. Check actual landscape vertex
spacing, the Water Mesh tile size and tessellation, the Water Zone's
`RenderTargetResolution` relative to `ZoneExtent`, spline width, and the water surface's
height against collision along **every** bend. As a diagnostic, divide each zone extent by
its Water Info target resolution: a 110,000 cm zone at Unreal's default 512 pixels gives
about 215 cm per pixel in the zone's water data, coarse for water-depth and flow detail
on a 280–420 cm stream. This calculation does **not** identify the cause of a stepped
surface outline: Unreal's Water Mesh also builds river coverage from collision bounds
on a tile grid. Author
`RenderTargetResolution=(X=…,Y=…)` with the `zone(...)` in `.gcw` and inspect the
saved result; do not assume that a higher Water Info resolution fixes a mesh edge until
the rendered silhouette improves. GraphCode notifies Unreal when these zone properties
change, so the engine updates derived bounds and requests the relevant water rebuild.
`ZoneExtent` is the full span: a 75,000 cm extent produces a 37,500 cm box half-extent.
In the Wilderness control, 2048 pixels visibly refined the water-only edge compared with
512 at that same extent, but the result still had steps. More pixels are a measured tuning
choice, not a guarantee of a smooth or well-fitted shoreline.
The zone's native `WaterMesh` component can be read and adjusted
through its placed actor `.gc`; `TileSize` and `TessellationFactor` are separate from
the Water Info resolution. [Unreal's Water Mesh guide](https://dev.epicgames.com/documentation/unreal-engine/water-meshing-system-and-surface-rendering-in-unreal-engine)
documents tile size, level of detail and the wireframe view. To distinguish landscape
occlusion from the water surface itself, render the same view with the landscape streaming
proxy in `ignore`, then compare the water-only outline. The Wilderness outlet remained
stepped with its landscape hidden, ruling out landscape occlusion and terrain colour as
the cause of that outline. Keep water shading, Water Info data and mesh coverage as separate
possibilities until a controlled edit identifies their effect. A smaller mesh tile size
is not automatically better: the 300 cm / tessellation 8 trial worsened this view and was reset.
`r.Water.WaterMesh.ShowWireframe 1` is a useful diagnostic where the console is available.
Fit the channel to the terrain, then use bank stones, sediment and bank vegetation to soften
the transition; do not hide a broken surface behind props. A close render at one bend does
not prove the full river or the route view works. Trace or render headwater, middle, outlet
and player approach.

A landscape heightfield cannot form an overhang. Use actual cave geometry and an explicit
entrance solution; only claim landscape visibility-hole authoring after exercising that route.
Inspect the cave from outside and inside. A single convex collision hull can close a perfectly
visible entrance. Configure suitable static collision, then test the mouth, walls, roof, floor
and narrowest passage with the actual player capsule. Complex collision on a static cave is a
different choice from collision for a simulated rigid body.

## Finish and hand over the whole route

Place lighting, atmosphere, exposure and the player start. Check that the intended pawn and
GameMode actually run, and walk from the clearing through the forest and water approach to the
shelter. Inspect close foliage, canopy silhouettes, terrain transitions and the cave lighting
with a player-height camera. Fix floating plants, submerged paths and collision traps before
calling the level usable.

Finally save, reopen and repeat the route. For streaming claims, cross actual runtime cell
boundaries in both directions and observe loading, collision and references. A small map wholly
inside the loading range is not that test. Keep HLOD, data layers, navigation, performance and
packaged playback as separate acceptance checks in the [validation record](VALIDATION.md).
