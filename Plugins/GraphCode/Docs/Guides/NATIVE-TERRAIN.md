# Native Landscape brushes through GraphCode

Use Unreal's brush assets when they offer the landform controls you need. GraphCode World
can bind a placed `LandscapeBlueprintBrushBase` actor to an explicit Landscape edit layer.
The engine owns brush rendering. This bridge does not replace the `.gcw` terrain evaluator.

## Water project setup

Only do this when the project chooses Unreal Water. Enable Water and accept its dependencies,
then check whether the project has the collision profile named by Water's runtime settings
(`WaterBodyCollision` by default). Preserve an existing profile. Unreal's missing-profile
message provides an **Add entry to DefaultEngine.ini** action; it installs the engine defaults.

For an AI setting up a project while the editor is closed, the equivalent UE 5.7 defaults below
can be appended to `Config/DefaultEngine.ini` if the default profile is absent. Keep existing
sections and profiles; do not duplicate the named profile. Restart before using water collision.

```ini
[/Script/Engine.CollisionProfile]
+Profiles=(Name="WaterBodyCollision",CollisionEnabled=QueryOnly,bCanModify=False,ObjectTypeName="WorldStatic",CustomResponses=((Channel="Visibility",Response=ECR_Ignore),(Channel="Camera",Response=ECR_Ignore),(Channel="WorldDynamic",Response=ECR_Overlap),(Channel="Pawn",Response=ECR_Overlap),(Channel="PhysicsBody",Response=ECR_Overlap),(Channel="Destructible",Response=ECR_Overlap),(Channel="Vehicle",Response=ECR_Overlap)),HelpMessage="Default Water Collision Profile (Created by Water Plugin)")
```

## Inspect and place the brush

Enable the Unreal plugin which supplies the brush, then restart the editor. For Landmass,
read `/Landmass/Landscape/BlueprintBrushes/Brushes/LandmassBrush_Erosion` with
`gc_read source:"live"`. Use `organise:true` to understand its graphs. Engine/plugin assets
can be read even when `sidecar:"none"` is reported; write your own assets under `/Game`.

Read the brush's properties and its user-defined structs before choosing values. For example,
Landmass's **Water Levelling** mode and **Water Levelling** settings are separate fields in
each erosion keyframe. Merely placing the brush or setting an iteration count does not run it.
Use `gc_apply` to place its Blueprint class and author a nonempty keyframe array.

For the bounded rainfall control below, add this **new actor entry** to the level's
`actors` block, preserving the existing WorldSettings and other projected entries:

```gc
  RainfallBrush = /Landmass/Landscape/BlueprintBrushes/Brushes/LandmassBrush_Erosion.LandmassBrush_Erosion_C(`Erosion Keyframes`=((Key Name=text("Bounded rainfall"),Erosion Mode=Water Levelling,Water Levelling=(Iterations=256,Starting Water Amount=100,Rain Per Iteration=1,Solubility=0.5,Evaporation=0.005))), `Iterations Per Tick`=32)
    @placement[Location=(X=0,Y=0,Z=0)]
```

Dry-run and apply, then bind that placed actor as described below. This example is a
diagnostic starting point; it does not claim realistic erosion at those settings.

For a World Partition actor, use its full object path from `gc_read` when editing it:
`/Game/Maps/L_Valley.L_Valley:PersistentLevel.ErosionBrush`. The level `.gc` does not include
its external actors. A level block is therefore not a replacement for that actor's document.
When changing an existing actor, preserve its full projected entry and edit the intended fields.
After simulation this can include generated state and preview components. Sending a short entry
can fail readback after the requested property already changed; read the reported live state
before retrying. A dry run resolves names but does not prove the final property readback.

## Attach it to the intended ground

Call `unreal_gc_invoke` with this shape, substituting actual paths returned by your reads:

```json
{
  "asset_path": "/Script/GraphCodeWorld.GraphCodeLandscapeEditorSubsystem",
  "function": "BindLandscapeBrush",
  "args": "Brush=obj'/Game/Maps/L_Valley.L_Valley:PersistentLevel.ErosionBrush',Landscape=obj'/Game/Maps/L_Valley.L_Valley:PersistentLevel.Catchment',LayerName=Weathering,bCreateLayer=true"
}
```

- `Brush`: a placed native or Blueprint Landscape brush actor.
- `Landscape`: a placed Landscape in the same editor world; never inferred from selection.
- `LayerName`: the destination layer's exact name. Layer order changes the result. A newly
  created layer is appended above existing layers; binding to an existing layer preserves it.
- `bCreateLayer`: `true` allows creation if that name is missing; `false` requires it to exist.

Check `observed`: `ReturnValue=true` and an empty `Error` mean binding succeeded. The tool's
`invoked:true` alone only means the function ran. `dry_run:true` validates the call and arguments
without executing its layer checks or creating anything.

Repeating the same binding keeps brush order and creates no duplicate. Moving a brush preserves
the old layer's other brushes and sculpt data. Locked source/destination layers refuse changes.
An invalid destination is checked before the brush is detached from its current layer.

Read the Landscape actor again. Its `@layers` entries show `Brushes=(obj'…')` in engine order.
That directive is a readout; change the binding with this function. To detach without deleting
the brush or its layer, invoke `UnbindLandscapeBrush` with `args:"Brush=obj'<actor path>'"`.

## Run the brush and prove its output

Binding schedules a native Landscape update; it does not run an erosion simulation. Invoke
the brush's own function, discovered in its live `.gc`. In the installed UE 5.7 Landmass erosion
asset that function is `Simulate Erosion`. Its completion is asynchronous. A headless editor
may require explicit `ReceiveTick(DeltaSeconds=0.016667)` calls through `gc_invoke`; keep the
iteration budget small and read completion state. Do not assume wall-clock waiting advances it.

After recompiling a placed brush Blueprint, refresh its input before simulating again:
invoke `RequestLandscapeUpdate(bInUserTriggered=true)` and let the editor service that update
(a subsequent scene render does so in the tool workflow). Detaching and rebinding also requests
the update. Recompilation can replace transient render targets; their mere existence does not
prove the Landscape has filled them. An immediate simulation from an empty input can flatten
the result to the bottom of the Landscape height range. The saved base layer is still recoverable
by detaching the brush.

Record collision heights before and after with `gc_render` followed by `mode:"ask"`. Verify
the hit components are Landscape collision, not the brush's preview meshes. Inspect the visible
ground too. Simulation completion and a generated texture are not proof that the ground changed.
Save with `gc_open action:"save_as"` to the current map path after verifying the result, then
cold-open and repeat the measurements. Saving a generated result does not make an untested brush
reconstructible from text; texture payloads and package dependencies need their own replay check.

## Reusable erosion example

The [BP_LandmassErosion recipe](../../Resources/Examples/NativeTerrain/BP_LandmassErosion.gc)
derives from Epic's installed Landmass brush. It keeps the native simulation, materials and
saved-height rendering. `Initialize Size ` (including the trailing space) sizes the particle
section grid from the simulation render target and `ComponentRes`. `Set MPC Values` uses
the Landscape's quad extent for the native sampling boundary, keeping the last sample valid.
It does not modify engine content.

To create it, enable Landmass, copy that recipe to
`Content/GraphCodeExamples/Terrain/BP_LandmassErosion.gc`, then call:

```json
{"asset_path":"/Game/GraphCodeExamples/Terrain/BP_LandmassErosion","create":true,"dry_run":true}
```

Repeat without `dry_run` after the recipe resolves. Place
`/Game/GraphCodeExamples/Terrain/BP_LandmassErosion.BP_LandmassErosion_C`, bind it to the
intended layer, and invoke its inherited `Simulate Erosion` function. No instance overrides
are needed for the example preset. Its two keyframes run in order:

| Pass | Controls |
|---|---|
| Particle Erosion | 1,024 iterations; Starting Water Amount=150; Delta T=0.033333; Solubility=0.75; Evaporation=0.02; Velocity Damping=0.4; Resolution Divisor=1 |
| Slope Smoothing | 64 iterations; Smooth Slopes Greater Than=0.7; Smooth Slopes Less Than=0; Opacity=1 |

These are artistic simulation controls, not calibrated rainfall units. The native slope
threshold is used by its shader as a height difference divided by grid spacing, not degrees.
Slope smoothing removes steep deposit peaks; it is not a mass-conserving sediment transport
pass. Inspect both the drainage and the deposition region before accepting a result.

The measured size is 505×505 Landscape samples at 100 cm spacing, with a 512×512 native
simulation target and four particle-mesh instances. Keep Resolution Divisor=1 for this
recipe. Smaller targets, other divisors, rectangular grids, other scales and other GPUs
need their own coverage and visual check; this example does not certify them.

The [catchment recipe](../../Resources/Examples/NativeTerrain/Catchment.gcw) prepares the
same base used for the visual check. Its explicit `smooth(iterations=512, strength=1)`
softens creases from the piecewise straight ridge/carve construction before adding the
fine noise and simulating erosion. This changes the base terrain; it must be present in
both the erosion-off and erosion-on comparison. Do not count that smoothing as hydraulic
erosion. Use identical cameras and a plain material to inspect the actual surface.

The September 24 acceptance pass inspected four direct Unreal views, including the far
boundary. The corrected preset forms visible converging channels and small rounded deposits
in this catchment. Against the same cleaned base, 650 of 1,107 Landscape collision probes
changed by more than 1 cm; sampled changes ranged from -62.491 to +20.306 cm. Fifty additional
boundary probes ranged from -38.279 to +43.743 cm, without the previous edge collapse.
Detaching restored all 1,107 base heights and normals exactly. Rebinding restored the completed
result, and a fresh editor process reproduced all 1,107 result probes plus all 50 boundary
probes exactly, without rerunning the simulation. This establishes saved-result replay for
the tested fixture, not physically accurate sediment transport or cross-GPU regeneration.

### Why the earlier result barely changed

The original native `Initialize Size ` divides each Landscape dimension by 255 with integer
division. A 505-sample side therefore produces only one section. At Resolution Divisor=2,
the particle spawner divides that one section again and creates a **zero-by-zero grid**.
The simulation can finish and save a texture while no particles affect the Landscape.
Changing only the divisor to 1 creates particles but leaves incomplete coverage. The example
uses `RT Resolution / ComponentRes` instead: 512 / 256 gives two sections per axis.

The earlier Water Levelling control is a different case. Its native material subtracts
`RainAmount * Solubility * deltaT` per iteration and adds a deposition term from evaporation.
Those small settings produce centimetre-scale lowering on a landscape with roughly 50 m
of relief. Completion is not evidence of visible drainage. Raising every setting is also
not a solution: the aggressive particle control produced tall sediment spikes. The gentler
particle preset and a separate native smoothing keyframe address that visual failure.

The original native UV clamp also adds half a pixel to `TotalSize / RTWorldSize`.
With TotalSize calculated from 505 samples rather than 504 quads, that reaches sample index
505, beyond the valid 0–504 range. Smoothing then propagates the empty padding into the
terrain edge. The example supplies `(LandscapeSize - 1) * LandscapeScale` for TotalSizeX/Y,
making the clamp land at the last valid sample centre. Check the far edges as well as the
hero camera when adjusting this brush.

Landmass uses a shared material parameter collection within one editor world. Run these
simulations sequentially in that world. Separate editor processes have separate state.

## Reusable height stamps with Landscape Texture Patch

Use Unreal's `LandscapeTexturePatch` component when a landform must remain a movable,
rotatable and resizable authored object rather than becoming another `.gcw` terrain
instruction. Enable the **Landscape Patch** plugin and restart the editor first. The patch
component is editor-only; the evaluated Landscape height and collision are what persist.
It is not a runtime terrain-deformation system.

The example source is
[`AsymmetricHeightStamp_RGBA16.png`](../../Resources/Examples/NativeTerrain/AsymmetricHeightStamp_RGBA16.png).
It is a 257×257 RGBA16 texture: R carries height, A carries a rounded-rectangle soft mask,
0.5 means zero height, and the full encoded range is 4,000 cm. The actual authored values
range from -375.567 to +995.387 cm. The deliberately offset mound, opposite basin and
diagonal ridge make incorrect rotation or mirroring visible. Its
[`JSON manifest`](../../Resources/Examples/NativeTerrain/AsymmetricHeightStamp_RGBA16.json)
records those units; the adjacent `.mjs` regenerates the exact PNG.

Import the PNG with `unreal_gc_import`'s normal two-step plan/import workflow. In the
returned `kind:import` document, keep the generated source hash and pipeline, then add:

```gc
settings {
  CompressionSettings = TC_HDR
  MipGenSettings = TMGS_NoMipmaps
  SRGB = false
}
```

Import it as `/Game/GraphCodeExamples/Terrain/T_GC_AsymmetricHeightStamp`, copy
[`BP_HeightStamp.gc`](../../Resources/Examples/NativeTerrain/BP_HeightStamp.gc) to
`Content/GraphCodeExamples/Terrain/BP_HeightStamp.gc`, then create the Blueprint with a
dry run followed by the write:

```json
{"asset_path":"/Game/GraphCodeExamples/Terrain/BP_HeightStamp","create":true,"dry_run":true}
```

The recipe uses `HeightSourceMode=TextureAsset`, `HeightEncoding=ZeroToOne`,
`ZeroInEncoding=0.5`, `WorldSpaceEncodingScale=4000`, the texture alpha, additive blend,
1,800 cm native falloff and 26,000×26,000 cm unscaled coverage. Place its class in the
level and state the intended Landscape on the component instead of relying on selection:

```gc
  HeightStamp = /Game/GraphCodeExamples/Terrain/BP_HeightStamp.BP_HeightStamp_C()
    @placement[Location=(X=25200,Y=25200,Z=100), Rotation=(Pitch=0,Yaw=0,Roll=0), Scale=(X=1,Y=1,Z=1)]
  {
    Patch = LandscapeTexturePatch(Landscape=soft'/Game/Maps/L_Valley.L_Valley:PersistentLevel.Ground')
  }
```

On the first patch in a Landscape, Unreal creates its native **Patches** edit layer and may
open **Insert New Landscape Edit Layer**. That dialog is not an error: drag `Patches` above
the generated/base layer so the patch evaluates after it, then choose **Complete**. Further
patch actors reuse the typed layer. Read the Landscape afterwards and confirm the layer
order instead of assuming the dialog choice landed.

Transform the actor to move, rotate or resize the stamp. Set the component's
`bIsEnabled=false` to remove its influence without deleting the reusable actor; omitting
the override restores the Blueprint default. `BlendMode` exposes four native choices in
UE 5.7: `Additive` offsets both signs, `AlphaBlend` targets the encoded absolute height,
`Min` only lowers and `Max` only raises. Patch Z is part of the absolute-height modes, so
do not compare their numeric offsets as if they shared additive semantics.

The September 24 fixture placed this source over a 505×505, 100 cm Landscape split into
16 streaming proxies. Across 187 collision probes, additive changed 51 with sampled deltas
from -215.417 to +1,012.549 cm. Min changed only negative samples (-315.412 to -1.174 cm),
and Max changed only positive samples (+0.779 to +912.549 cm). Disable/reenable reproduced
all 187 additive samples exactly. Every tested proxy/component boundary remained joined;
the largest adjacent seam step was 6.903 cm with a 1.865° normal change on the sloped
surface, while flat seams were exact. Two 133-point transects stayed exact outside the mask
and entered through multiple intermediate values rather than a hard rim. Moving, rotating
and non-uniformly scaling the actor restored the old 1,112.549 cm collision point to its
100 cm base while producing the new footprint. A separate hand-authored control mound was
unchanged in every state. A fresh editor process then reproduced all 187 positions, normals
and collision targets exactly, with the saved `Patches` layer above `gc_PatchGround`.

These measurements establish the public authoring, transform, blend, collision and soft-mask
behaviour of this one texture and Landscape size. They do not make its alpha channel a
rainfall, material-resistance or flow-exclusion mask; those need separate process semantics
and outcome tests.

## Historical controls and remaining work

On a 504 m World Partition catchment (505×505 samples, 100 cm spacing, 16 proxies), these
public-tool trials used 49 actual Landscape collision probes:

| Mode / action | Authored controls | Observed result |
|---|---|---|
| Add Noise | Noise Height Meters=10, Noise Scale Meters=100, Noise Seed=24; other native defaults | 44 probes changed by more than 1 cm; maximum rise 612.497 cm. These are noise input controls, not a promise that every point rises 10 m. |
| Detach that brush | `UnbindLandscapeBrush` | All 49 baseline heights restored exactly. |
| Water Levelling, small control | Iterations=64; native water/solubility/evaporation defaults | Completed, but none of the probes changed. |
| Water Levelling, stronger control | Iterations=256, Starting Water Amount=100, Rain Per Iteration=1, Solubility=0.5, Evaporation=0.005, Iterations Per Tick=32 | All probes lowered roughly 0.8–1.6 cm. Reattaching the completed brush and cold-opening the saved map each reproduced all 49 heights exactly. |
| Particle Erosion, original control | Erosion Iterations=128, Starting Water Amount=100, Solubility=0.25, Evaporation=0.01, Velocity Damping=0.2, Resolution Divisor=2 | Completed, but none of the probes changed. Superseded: the September 24 diagnosis found zero particle-mesh instances; see the repaired example above. |

These values are **diagnostic controls, not recommended landscape presets**. Noise Height/Scale
explicitly use metres; other numbers above retain the engine field names and have not been
calibrated into a physical rainfall or sediment budget. Native brush binding has focused engine
tests. The repaired example produces visible drainage in the inspected catchment; physically
credible sediment fans, process masks and packaged erosion playback still require separate
acceptance tests. Reusable native height patches are covered by the measured recipe above.
A visible change alone does not establish the remaining claims.
Cold replay above means loading the saved native texture and Landscape packages; regenerating
an identical simulation from text, or on another GPU, is a separate test.
