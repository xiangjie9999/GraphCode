# Refine vegetation presets with PVE

Use this guide to learn native Procedural Vegetation Editor authoring through GraphCode.
A preset is the starting plant data. The work is to understand that data, choose the correct
operation, control its effect and inspect the result until the species and intended variant read.
Use [VEGETATION.md](VEGETATION.md) for setup, first-asset creation and the complete production
path through materials, export, wind, biome placement and performance.

**Exact behaviors below were audited on UE5.7.4.** They provide a baseline for UE5.8 preset
work; recheck changed nodes and their values on that engine. The complete study recipe is
source-checked teaching material, not a newly evaluated or visually accepted plant.
Ask the installed tools for current help and reflected types before authoring:

```text
gc_describe {"kind":"procedural_vegetation","block":"graph"}
gc_describe {"type_path":"/Script/ProceduralVegetation.PVFoliageDistributorSettings"}
```

Use the actual tool names exposed by your client, including an `unreal_` prefix where present.
Follow returned type paths into nested structs rather than guessing fields from a tutorial.

## Understand what a preset supplies

A preset contains already generated plant topology and attributes, plus references to reusable
foliage meshes, bark materials and optional radial profiles. Its valid variations become the
Preset Loader's output pins. It is richer than a finished static mesh and more specific than a
random seed.

The structure includes points, branch relationships, radii, orientation, lengths, generations and
stored growth information. Foliage data includes attachment transforms and component descriptors.
Modifiers and the distributor consume that information; they do not infer botanical meaning from
a screenshot or query the level's climate.

Inspect the species' supplied variants and component kit before editing. Compare branching habit,
twig size, leaf arrangement and crown gaps. Identify which parts already fit the brief and which
need changes. Keep the source preset intact and make project-owned graph variants.

A graph referencing a preset still depends on the native preset package and its component assets.
The `.gc` does not embed their binary plant data or textures. Do not rebuild the supplied preset
from a guessed JSON schema or reuse Epic's original workstation paths as project dependencies.
The native importer is a specialized plant-data route, not a general FBX-to-PVE converter.

## Learn the data flow before adding nodes

```text
Preset variation [Growth]
  → Scale / Gravity / Carve / Remove Branches / Slope [Growth]
  → Mesh Builder [Mesh]
  → Foliage Palette / Bone Reduction [Mesh]
  → optional Foliage Distributor [FoliageMesh]
  → Output
```

| Stage | What it changes | What it does not establish |
|---|---|---|
| Source | Plant structure, stored attributes and species resources | Live level light, weather or terrain response |
| Structure modifiers | Existing positions, shape or retained branches | A new from-scratch growth simulation in UE5.7 |
| Mesh Builder | Trunk/branch surface and its material mapping | Foliage material quality or downstream collision |
| Foliage Palette | The reusable components and their matching descriptors | New attachment positions |
| Foliage Distributor | Kept or regenerated foliage placements and component selection | New woody branch topology or world scattering |
| Bone Reduction | Meshed skeleton detail | Smaller texture/material cost or accepted motion |
| Output | The data to preview and native export configuration | A mesh package created by graph apply |

These are native PVE collection types, not interchangeable PCG point data. Structure modifiers
belong before Mesh Builder. Palette and bone reduction belong before a distributor that emits
FoliageMesh. Output can also consume Mesh directly because a preset already supplies placements;
adding a distributor is optional.

Keep the hidden `GraphInputOutput()` terminals in the complete graph. Use the preset's actual
variation pin, such as `Common_Hazel_01`; do not rename it to `Out`. Graph branching can feed
several outputs, but GraphCode's static preview uses the first `PVOutput` in graph order.
Compare alternative outputs through deliberate study copies or the native editor's inspection.

## Establish a baseline and change one cause

Create the smallest complete tree using [the Hazel recipe](VEGETATION.md#a-complete-tree-with-inline-settings).
Retain its native materials, palette and placements. Wait for a ready capture and save an
unchanged baseline from fixed front, side and underside cameras.

1. Record the species variation, dimensions, graph values and references. Keep an accepted
   baseline separate from the graph being studied.
2. Name the visible change you want: fewer short twigs, a broader crown, drooping secondary
   branches, smaller tip leaves or different leaf selection in exposed parts.
3. Choose the node that actually controls that change. Verify its activation gate, input data,
   units and ramp coordinate.
4. Change one cause, dry-run, apply, read live and inspect the same cameras. Preserve the complete
   graph and the current identity/signature.
5. Record the observation. Keep a useful change or restore the baseline before testing another.
   Do not stack unrelated sliders until the result happens to look different.

Native PVE inspection views can isolate structure, mesh, foliage and bones where the installed
editor offers them. Use them to locate the first bad stage. Debug selection and preview modes
are inspection state, not extra authored `.gc` blocks.

## Shape existing structure deliberately

### Scale changes plant data

`PVScale.Scale` scales the structure about its trunk root, including radii, branch lengths and
existing foliage. It is different from an actor transform applied after export.

Compare a small scale change with the baseline's proportions, bark scale and foliage size.
If the goal is only larger leaves, use the appropriate foliage control instead. Under regenerated
distribution, changing branch length can also change the number of spacing sites, so a later
distributor may not preserve the original scaled leaf layout.

### Gravity bends while preserving the source hierarchy

`PVGravity` uses `GravitySettings.Mode`, `Gravity`, `Direction` and `AngleCorrection`.
Ordinary Gravity mode biases bending toward the supplied direction. Phototropic mode consumes
stored light-direction attributes from the source; moving a level light does not update them.

Start with a gentle change and inspect the trunk, heavy branches and fine twigs separately.
Check branch junctions, surface stretching and foliage attachment after the bend. More force
does not create a more mature tree or recalculate its growth history.

AngleCorrection changes the direction-dependent response. Treat it as a separate experiment
after the broad bend works. Preserve hidden authored values returned by GraphCode, but do not
invent public phototropism controls from internal fields.

### Separate pruning whole branches from carving

`PVRemoveBranches` selects whole branches using `BranchRemoveBasis` and `Threshold`.
The native bases are Length, Radius, Light, Age and Generation. Their scores derive from stored
data; the node skips trunks and removes positive scores below the threshold.

The threshold is a native score, normalized by the per-plant maximum when needed. It is not
metres, a literal age or an instruction such as “remove generation three.” Light-based removal
does not sample current scene shadows. Inspect which branches disappeared before increasing it.

`PVCarve` instead trims according to `CarveSettings.CarveBasis` and a normalized `Carve` amount.
LengthFromRoot, FromBottom, ZPosition and Radius use different mappings. ZPosition is not an
absolute authored world-height cutoff.

**FromBottom can remove lower trunk points as well as their branches and foliage.** Inspect
the root and ground contact after using it. To make a clean lower trunk, begin with branch pruning
and a suitable source variation; do not assume this carving mode only removes low branches.

Compare prune-then-bend with bend-then-prune only when order matters to the intended result.
Operations change the data consumed downstream; normalized cutoffs or stored coordinate fields
may produce different results after scale, removal or deformation.

### Slope is a structural bend

`PVSlope` uses `SlopeParams.SlopeAngle`, `SlopeDirection`, `BendStrength` and `TrunkPivotPoint`.
It bends segments with a spatial falloff and updates attached foliage transforms.
Choose the intended root/pivot behavior and inspect the whole plant.

Zero BendStrength produces a uniform tilt; it does not disable the node. Use a zero angle or
remove the operation deliberately when the study needs an unchanged structure.
This node does not align the exported tree to terrain or plant its roots. World placement handles
those decisions separately.

## Build the branch surface without losing its shape

Inspect bark geometry before covering it with leaves. A smooth canopy render can hide broken
forks, collapsed twigs or a poor trunk base.

| Mesh Builder group | Controls to inspect | Observable result |
|---|---|---|
| Cross-section and surface sampling | `MinMeshDivisions`, `MaxMeshDivisions`, `Accuracy` | Roundness, silhouette, fork quality and shading |
| Longitudinal simplification | `PointRemoval`, `SegmentReduction`, `LongestSegmentLength`, `ShortestSegmentLength` | Retained bends and branch continuity |
| Retention | Hull, MainTrunk, Ground and Scale retention fields/gradients; `SegmentRetentionImpact` | Where detail survives simplification |
| Radial profile | `SelectedPlantProfile`, `PlantProfileScale`, `PlantProfileFallOff`, `bApplyProfileToBranches` | Cross-section shape and its falloff |
| Displacement | Texture, strength, bias, UV scale and generation limit | Bark relief on the generated surface |

All these fields are under `MesherSettings`. Read native ranges and values instead of treating
the table as a set of recommended defaults. Segment-length settings in the inspected mesher
use metres, unlike world placement coordinates in centimetres.

First inspect the unsimplified source shape with displacement off. Reduce detail gradually,
then use retention to protect the trunk base, crown silhouette or important bends that lost form.
Do not raise every quality control together. Check several camera distances and inspect the
exported result before calling a cheaper setting equivalent.

Retention curves consume distinct source attributes. “Ground retention” does not mean that
the mesher queried the level landscape, and every retention curve is not a tree-height ramp.
Use the ramp-domain section below before changing a curve.

### Use available profiles

Native profile choices are generated from the incoming preset. On the inspected engine they
are None and numeric index strings for available profiles. Select an option that actually exists;
a new word such as `OakFlare` does not create a radial profile.

Inspect profile scale and falloff at the base, forks and tips. Applying a trunk-shaped profile to
small branches can give them an inappropriate section. Profile falloff uses BranchGradient;
it does not share the distributor's complemented coordinate.

If the source lacks the required shape, use an appropriate component/preset or a deliberate
external/Shape asset route. Mesh sampling cannot reconstruct botanical structure that is absent.

### Finish bark mapping as well as the shader

With `MesherSettings.bOverrideMaterial=false`, native evaluation supplies preset bark materials
and U ranges. Enable the override for a custom mapping, then inspect the complete ordered
`MesherSettings.MaterialSettings.MaterialSetups` array.

MaterialMode selects Generation, Age or Radius from source data. DistributionMethod and
GenerationOffset map those values into material setups. Confirm which material reaches trunk,
primary branches and twigs; the array is not a random list of equally likely bark swatches.
The inspected Repeat mapping clamps its offset selection; it does not cycle randomly through
the array. Fit remaps the source range across the available setups.

Each setup's U range selects part of the texture layout. YTextureMode, YScale and offsets control
longitudinal mapping. A U range does not define a physical moss band on the tree.
Maintain bark size and direction through forks; test repeat seams and stretched small branches.

Native material override toggles can retain previous custom choices. Read live again after
opening/evaluating in PVE before editing the sidecar, even if the package still looks clean.
Preserve the authored state rather than replacing it with whichever defaults were visible earlier.

### Diagnose displacement

The inspected mesher samples the red channel and applies strength to **sample minus bias**.
Increasing positive bias reduces outward displacement. A neutral source value matching the bias
produces no displacement at that sample.

Check the imported texture's source format, dimensions, red data and native warnings first.
The native implementation requires supported source formats and power-of-two dimensions; a
compression setting or a successful texture import alone does not establish that requirement.
Setting the texture must also trigger the native data refresh, not merely save a reference.

Tune displacement after accepting sampling and UV mapping. Inspect moving light, silhouette,
branch junctions and smaller generations. Relief that looks good on the trunk can destroy twigs;
use the supported generation limit and inspect its transition.
Displacement does not repair a disconnected fork or replace good bark normal/roughness maps.

## Separate foliage components from their placement

`PVFoliagePalette` chooses what reusable meshes are available. `PVFoliageDistributor` controls
placements and their component selection. Learn them separately.

With `bOverrideFoliage=false`, the palette retains the source component set. In UE5.7, an enabled
override replaces existing preset slots: a short nonempty list repeats its last entry, excess
entries are ignored and an empty list clears the palette. It is not an unbounded component list.

Keep slot order and intended descriptors when replacing meshes. Inspect a component's pivot,
size, attachment, orientation and material under real transforms. A beautifully modeled leaf
with the wrong attachment axes can make the whole crown look wrong.

Palette Attributes describe how suitable a mesh is for matching conditions. They are not
per-component spawn weights:

| Descriptor | How to think about it |
|---|---|
| Light | Suitability relative to the source attachment's stored light value |
| UpAlignment | Suitability relative to stored orientation data |
| Health | Suitability relative to stored health data |
| Tip | A tip-oriented component choice |

Preserve other authored descriptor state returned by the installed reader. Give the component
set meaningful differences before expecting condition matching to produce meaningful variation.
Duplicating one mesh and identical descriptors across slots does not make a richer plant.

## Choose preserved or regenerated distribution

| Mode | Native behavior in UE5.7 |
|---|---|
| `OverrideDistribution=false` | Keep preset attachment positions; ethylene filtering can remove foliage |
| `OverrideDistribution=true` | Replace placements with the node's sampling, size, orientation, phyllotaxy and matching rules |

Start with preserved placements to evaluate a new palette or material. Regenerate only when
the brief needs a different attachment layout. Expect that transition to change more than leaf
count: the preset's carefully authored distribution is being replaced.

EthyleneThreshold filters stored hormone data in either mode. Raising it admits more samples
for unchanged input. It does not regrow branches, simulate a new season or diagnose plant health
from the level. For a seasonal variant, also author the appropriate organs and material state.

### Control density without confusing the causes

| Control | Effect or constraint |
|---|---|
| `InstanceSpacing` | Distance between sampling sites along the branch's native length coordinate |
| `InstanceSpacingRamp` and `InstanceSpacingRampEffect` | Modify spacing locally when the effect is active |
| `MaxPerBranch` | Limit sampling sites; `-1` uses branch-point count |
| Phyllotaxy bud count | Can place several components at a sampling site |
| EthyleneThreshold | Reject sampled or retained placements based on stored data |
| Component/clump size | Changes visible coverage even when attachment count stays the same |

Reduce spacing while keeping the other causes fixed, then inspect petiole intersections and
negative space. A larger clump can make a crown dense without more attachments; smaller leaves
can make unchanged placements seem sparse.

The native sampler walks from tip toward base. Its cap is not necessarily the final number of
leaves, especially with multi-bud phyllotaxy. A low cap can exhaust the permitted sites before
sampling the branch base. The sentinel does not mean unlimited. Keep spacing positive: zero
spacing gives the native sampler no advance and can repeat the same attachment position.

Check source length units before interpreting spacing physically. The inspected import contract
keeps length-from-root in source units while converting point positions; it is not the World
centimetre coordinate system. Compare actual attachments on the accepted species.

### Size has several multipliers

The distributor samples ScaleRamp, blends branch-radius influence, clamps the resulting
normalized value and maps it into MinScale/MaxScale. It then multiplies by BaseScale and a random
scale factor. **MinScale/MaxScale are not final size clamps.**

For example, a mapped scale of 1 multiplied by BaseScale 1.2 and random factor 1.1 becomes 1.32.
This is a formula illustration, not a recommended leaf size. Inspect the physical dimensions of
the source component as well as those multipliers.

Use a constant scale ramp and fixed random range to learn BaseScale first. Then tune the
branch-local size trend, BranchScaleImpact and bounded random variation one at a time.
Preserve the intended species proportions and avoid visibly identical leaf sizes throughout.

### Learn phyllotaxy and attachment orientation

Set `OverridePhyllotaxy=true` only when replacing the preset pattern is intended; custom
distribution must also be active. The native patterns are Alternate, Opposite, Decussate,
Whorled and Spiral. Match the arrangement to botanical reference.

Whorled uses MinimumNodeBuds/MaximumNodeBuds. Those values do not become a universal bud-count
control for other patterns. Spiral's formation choice describes divergence/ranks, not several
leaves at every sampling site. In the inspected Parastichous implementation, AdditionalAngle
supplies the divergence; do not assume a built-in golden-angle value.

AxilAngle controls tilt relative to the parent branch direction when `OverrideAxilAngle` is active.
The native ramp target is its clamped value multiplied by AxilAngleRampUpperValue; Effect blends
from AxilAngle toward that target. It is not simply a min/max remap.

Consequently a base angle of 45 degrees, full ramp effect and a zero ramp value can produce zero
degrees. The base angle is not a guaranteed lower bound. Use a constant ramp to establish the
attachment angle before adding a branch-local gradient.

Inspect petiole contact, leaf facing and component axes together. Arrangement around a stem,
tilt away from it and the source component's own orientation are different causes of a bad leaf.
Randomness cannot correct a systematically wrong pivot.

### Conditions choose the component

Enable a condition and give it positive Weight for it to participate. Light, UpAlignment, Tip
and Health compare stored source attachment attributes with palette descriptors.
They do not query current lighting or define exclusion zones for where foliage may grow.

Native matching sums descriptor mismatch weighted by condition importance, normalizes the
candidate scores and creates a shortlist using CutoffThreshold and MinimumCandidates.
It then chooses uniformly within that shortlist. **Weight is not a mesh frequency.**

Begin with one active condition and clearly distinct descriptors. Inspect which component
appears where. Add another condition only once the first selection makes sense.
MinimumCandidates keeps alternatives available when the cutoff is strict; it can also admit a
less suitable component, so a larger value is not automatically a better match.

If selection looks random, inspect the actual palette descriptors, activation flags, weights,
offsets and shortlist behavior. Offset changes the source target before comparison.
A copied label such as “shade leaf” has no effect unless the relevant values describe that choice.

## Read ramps in their actual coordinate

A ramp's Time is the consumer's coordinate and Value is its output. It is not an animation curve
or automatically normalized height from the ground to the crown.

| Ramp | Native coordinate | Activation or interpretation |
|---|---|---|
| InstanceSpacingRamp | Normalized position along each sampled branch | Effect blends its contribution into spacing |
| ScaleRamp | `1-BranchGradient` | Custom distribution; output participates in scale mapping |
| AxilAngleRamp | `1-BranchGradient` | Custom distribution, axil override and ramp effect |
| PlantProfileFallOff | `BranchGradient` | A valid selected source profile |
| Mesh retention gradients | Their corresponding source LOD attributes, complemented by the consumer | Matching retention and simplification settings |

In the audited Hazel source, BranchGradient runs from one at the base to zero at the tip.
Distributor scale/axil coordinates therefore run base-to-tip, while profile falloff runs in the
opposite direction. The spacing ramp uses base-to-tip position while the sampler traverses it
from tip toward base. Check another preset's data before transferring a curve.

Start with a constant ramp, then change one end to reveal direction. Keep influence and other
scales fixed. Inspect one representative branch and the whole crown: a branch-local effect can
repeat across every height of the tree.

GraphCode ramps belong in the node's `curves` body. Preserve full RichCurve interpolation,
tangents and extrapolation from an existing graph. Do not flatten authored cubic/weighted curves
to a pair of linear keys during an unrelated edit.

## Use a complete distribution study graph

This recipe uses the installed Hazel dependency and native UE5.7 fields. It deliberately
regenerates foliage with explicit ramps so their effects can be studied. Values are illustrative;
they are not a recommended finished Hazel profile or a claim that its appearance was inspected.

Write `Content/Vegetation/PVE_HazelStudy.gc` in the target project:

```gc
gc 1 kind:procedural_vegetation asset:/Game/Vegetation/PVE_HazelStudy

graph {
  input = GraphInputOutput()
  n0 = PVPresetLoader() [Preset=obj'/ProceduralVegetationEditor/SampleAssets/Tree_Common_Hazel_01/PVE_CommonHazel_Data.PVE_CommonHazel_Data']
  n1 = PVScale(In=n0.Common_Hazel_01) [Scale=0.9]
  n2 = PVGravity(In=n1.Out) [GravitySettings.Gravity=0.35]
  n3 = PVMeshBuilder(In=n2.Out)
  n4 = PVFoliagePalette(In=n3.Out)
  n5 = PVBoneReduction(In=n4.Out) [Strength=0]
  n6 = PVFoliageDistributor(In=n5.Out) [OverrideDistribution=true, EthyleneThreshold=0.85, InstanceSpacing=0.3, InstanceSpacingRampEffect=0.15, MaxPerBranch=24, BaseScale=1, MinScale=0.8, MaxScale=1, RandomScaleMin=0.9, RandomScaleMax=1.1, OverrideAxilAngle=true, AxilAngle=45, AxilAngleRampUpperValue=55, AxilAngleRampEffect=0.25, OverridePhyllotaxy=false, RandomSeed=9231, Conditions.MinimumCandidates=2, Conditions.bActivateLight=true, Conditions.Light=(Weight=0.8,Offset=0)] {
    curves {
      InstanceSpacingRamp = RichCurve(keys=((Time=0,Value=0.15,InterpMode=RCIM_Linear),(Time=1,Value=0.65,InterpMode=RCIM_Linear)))
      ScaleRamp = RichCurve(keys=((Time=0,Value=0.8,InterpMode=RCIM_Linear),(Time=1,Value=1,InterpMode=RCIM_Linear)))
      AxilAngleRamp = RichCurve(keys=((Time=0,Value=1,InterpMode=RCIM_Linear),(Time=1,Value=0.65,InterpMode=RCIM_Linear)))
    }
  }
  n7 = PVOutput(In=n6.Out)
  output = GraphInputOutput()
}
```

Use [the main guide's create/read workflow](VEGETATION.md#a-complete-tree-with-inline-settings):
dry-run creation, inspect diagnostics, create and read live. Do not invent signatures or add
`compile:true`. Use [its scene render workflow](VEGETATION.md#render-and-inspect) for a ready
static image of this asset, with fixed camera and persistent captures.

The graph keeps source phyllotaxy because its override is false. It replaces placement positions,
uses a capped branch sampler and modifies scale/orientation locally. The light condition selects
from source palette descriptors. None of those actions prune woody branches or sample level lights.

After establishing this study's baseline, change only InstanceSpacing, ramp effect, BaseScale,
an angle or a condition at a time. For preserved source placements, set OverrideDistribution
false and treat that result as a separate comparison, not as a way to keep tuning custom controls.
Read live after native evaluation before the next edit.

### Replace a palette slot through GraphCode

For this study graph, replace `n4` with the following declaration while retaining the complete
surrounding graph. It uses a real sample component to demonstrate the ordered array syntax:

```gc
n4 = PVFoliagePalette(In=n3.Out) [bOverrideFoliage=true] {
  FoliageInfos {
    (Mesh=soft'/ProceduralVegetationEditor/SampleAssets/Tree_Common_Hazel_01/Instances/BrLeaf_000.BrLeaf_000', Attributes.Light=0.5, Attributes.UpAlignment=0.5, Attributes.Health=1)
  }
}
```

This single entry repeats into the existing source slots on UE5.7. It demonstrates replacement,
not a rich production palette. For production, inspect and preserve the required slot count,
then provide meaningful component and descriptor differences.
A custom Shape/DCC leaf or twig can occupy a slot after its compatible mesh has been created
and its attachment, scale and material have been verified.

### Override bark mapping through GraphCode

Replace `n3` in the same study graph with a complete mesher declaration carrying the intended
settings. This fragment shows a real bark dependency and an ordered material array:

```gc
n3 = PVMeshBuilder(In=n2.Out) [MesherSettings.bOverrideMaterial=true, MesherSettings.MaterialSettings.MaterialMode=Generation, MesherSettings.MaterialSettings.DistributionMethod=Repeat] {
  MesherSettings.MaterialSettings.MaterialSetups {
    (Material=obj'/ProceduralVegetationEditor/SampleAssets/Tree_Common_Hazel_01/Materials/MI_Broadleaf_Common_Hazel_Bark_01.MI_Broadleaf_Common_Hazel_Bark_01', YScale=1.5)
  }
}
```

Retain any accepted mesher fields and curves when editing a populated graph. An omitted setting
can return to its native default. Inspect material assignment, UV scale and forks after applying;
an ordered array accepted by the reader is not evidence that the bark looks correct.

## Learn bones and native export separately

BoneReduction consumes Mesh and belongs before the distributor. Increasing Strength reduces
connecting-segment bones toward one. Compare the source skeleton with a modest reduction,
then inspect actual wind playback at the intended distance.

Fewer bones can change where branches bend. Inspect anchored roots, forks, slender twigs and
attached clumps; triangle count or a static image cannot validate that tradeoff.
Bone reduction does not simplify the bark material or leaf textures.

| Output decision | Fields or native setup to inspect |
|---|---|
| Static versus skeletal mesh | `ExportSettings.ExportMeshType` |
| Assembly foliage versus combined geometry | `ExportSettings.bCreateNaniteFoliage` |
| Names, destinations and existing packages | ContentBrowserFolder, MeshName and ReplacePolicy |
| Trunk/foliage inclusion | Native export enable fields returned by the current reader |
| Collision | Native static collision or skeletal CollisionGeneration, then the baked result |
| Animated output | WindSettings, exported skeleton/groups and compatible runtime placement/provider |

**Disabling bCreateNaniteFoliage does not necessarily disable Nanite.** It changes the assembly
foliage route; the exporter configures Nanite separately where supported. Inspect the saved mesh.
Choose shape preservation from the installed native options and evaluate its distant coverage.

GraphCode apply saves the graph and its output configuration. Use the separate native export
operation to produce mesh packages, then inspect those packages and required companions.
Follow [the production export steps](VEGETATION.md#bake-usable-meshes-and-inspect-the-result).
Do not infer export success from a tree preview.

The PVE wind profile describes simulation groups, not level wind speed/direction.
Group choices need to match the exported species. An existing mesh's wind data can win when
the profile's overwrite setting is false; inspect the actual data after replacement export.
Verify native DynamicWind placement/provider and playback through
[the wind workflow](VEGETATION.md#make-wind-coherent-and-verify-playback).

## Preserve native state during iteration

A supplied PVE graph block is the complete graph. Keep existing nodes, terminals, aliases,
connections, fields, curves and references that remain wanted. Omitting a node removes it.
Read current live state and retain the projection's identity/signature.

Native evaluation may populate palette entries, source bark defaults, conditions and profile
options. Native override toggles also maintain previous custom choices. Re-read after using the
PVE editor; do not assume an older sidecar describes that current live state.

Shared `pcg_settings` can centralize an accepted complete typed value, such as a mesher setting
asset, when GraphCode World is installed. A referencing graph does not edit that asset inline.
Finish and inspect one plant before sharing a control across a family: changing it can affect
every referencing plant.

Use the native graph organiser for readability without treating layout as plant data.
Save/reload and repeat an unchanged apply to check retained state. Review `omits:` before relying
on unfamiliar values. Existing graph fidelity, visible quality, export and runtime behavior need
their respective checks.

## Diagnose the first stage that went wrong

| Symptom | First useful check |
|---|---|
| Empty plant | Preset dependency, actual variation pin, typed connections and selected Output |
| Branches vanished unexpectedly | RemoveBranches score basis/threshold or normalized Carve amount |
| Trunk base floats after pruning | FromBottom carving, source pivot and the exported ground contact |
| Sliders change but leaf layout does not | OverrideDistribution; its custom controls are inactive when false |
| Ramp edits appear inert | Coordinate, influence/override gate and the input attribute |
| Leaves pile at tips | Positive spacing, tip-to-base sampling and the sampling-site cap |
| Added palette meshes disappear | UE5.7 source slot limit; inspect the evaluated palette |
| One component fills the entire crown | Short repeated palette, descriptor differences and condition shortlist |
| Light-based selection ignores a moved lamp | Stored source attributes; it is not a live light query |
| Leaf orientation is consistently wrong | Mesh pivot/axes, phyllotaxy, axil override and actual ramp target |
| Foliage exceeds the configured scale range | Source dimensions, BaseScale and random multiplier after the mapped range |
| Bark becomes flat or noisy | Actual displacement source data/refresh, bias, UVs and surface sampling |
| Forks or twigs collapse | Input structure versus point/segment simplification and local retention |
| Edits change after opening PVE | Defaults/toggle history populated during evaluation; read live again |
| Skeletal output remains still | Native export, real wind data, instanced skinned placement and provider |
| Mesh packages are absent | Graph apply is complete; native export has not produced them |

Restore a known baseline and isolate the responsible stage. Inspect structure before meshing,
meshing before foliage, and exported output before runtime. Do not hide a geometry error with
more foliage or fix a placement gate by changing every seed.

## Learn through a species family

Work through these exercises on one study asset before batching:

1. Keep preset placements and accept an unchanged render, saved source and dimensions.
2. Improve one structural feature with a single modifier; inspect bark and attachments.
3. Tune mesher sampling/retention without losing the accepted silhouette.
4. Replace one component while keeping placements; prove its pivot, material and contact.
5. Regenerate foliage with a fixed RandomSeed and study spacing versus visible coverage.
6. Use distinct descriptors and one active condition to demonstrate deliberate component selection.
7. Use constant then endpoint-varied ramps to demonstrate their local coordinate and activation.
8. Export the chosen mesh type, inspect its packages and test the required motion/placement.

Choose a small family whose differences have reasons in the species brief: different source
habits, exposure or damage, rather than renamed copies with random colors. Use appropriate source
structure for a life stage; scaling a mature tree down does not establish sapling anatomy.

Keep fixed-camera comparisons, exact graph values, component sources, accepted seed and native
output paths with each variant. Record which visible feature justified each retained operation.
Remove unnecessary modifiers deliberately after comparing their result.

Finish the family in a representative patch using
[the biome and performance workflow](VEGETATION.md#place-an-ecological-vegetation-family).
Native PVE proficiency includes knowing when to stop changing the tree and fix its material,
placement, motion or rendering cost instead.

## Sources and version checks

The artist workflow is introduced by
[Epic's UE5.7 PVE guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/procedural-vegetation-editor-pve-in-unreal-engine?application_version=5.7)
and the [Nanite/PVE presentation, 17:56–27:28](https://www.youtube.com/watch?v=aZr-mWAzoTg&t=1076s).
Exact node behavior above was checked against installed native source and bundled Hazel data on
2026-10-07; some simplified tooltips do not describe the consumer's units or formula precisely.

In another engine version, inspect reflected settings and compare a minimal native result before
reusing an exact rule. UE5.8's enhanced distribution, growth, extraction and grafting need their
own installed-engine checks. See [the version split](VEGETATION.md#ue57-and-its-two-production-routes)
and [the new growth route](VEGETATION.md#way-2-grow-or-extract-a-new-species-in-ue58).
