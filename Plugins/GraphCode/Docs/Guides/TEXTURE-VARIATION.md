# Reduce visible texture repetition

*A GraphCode production guide for landscapes and other large textured surfaces. Read after
[ASSET-QUALITY.md](ASSET-QUALITY.md); use [ENVIRONMENTS.md](ENVIRONMENTS.md) for terrain composition.
Build the material and its seamless texture set first with [MATERIALS.md](MATERIALS.md); this guide
starts once one tile repeats.*

**Consider texture variation whenever a repeated surface becomes visible at the game camera.**
A seamless texture can still form an obvious grid of identical stones, stains or grass clumps.
Inspect close, middle and distant views before choosing a remedy. Preserve deliberate repetition
such as brick courses, floor tiles and woven patterns; vary their finish without destroying their
construction. These techniques reduce repetition, but no node guarantees an acceptable result.

When sourcing rocks or surface materials, use the
[Blendkit-to-Integrate workflow](CONTENT.md#use-blendkit-generators-and-materials-through-integrate):
download a suitable generator or material, produce/tune it in Blender, then translate and verify
it in Unreal. Keep these repetition and imperfection checks on the imported result too.

This guide combines the owner's tutorial summary supplied on 2026-10-03 with the linked primary
references and GraphCode's production checks. The original video URL was not supplied; its timestamps
are provenance for the requested technique, not independently verified footage. References include
older UE tutorials and vendor-specific implementations: check the project's engine version and
installed material functions before copying inputs, paths or defaults.

## Start with the three-part technique

1. **Vary the sampling coordinates** (owner's reference: 0:52–1:47). Feed landscape coordinates
   into `TextureVariation`, then its varied UV output into the surface's texture samples. Consider
   enabling its `Random Rotation and Scale` input through a static bool parameter. Keep the
   coordinate variation consistent across corresponding base color, normal, roughness, AO and
   height samples. First verify the function's actual inputs and normal handling; rotation is a
   choice to test, not a universal default.
2. **Make a reusable Material Function** (3:12–4:10). Put the surface sampling and texture logic
   in one function with texture and scale inputs, returning Material Attributes. Evaluate it at
   a fine near scale and a broader far scale. Expose meaningful per-surface controls through a
   Material Instance. Unreal's [Material Attributes reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/material-attributes-expressions-in-unreal-engine)
   describes bundling attributes inside functions; use `Set Material Attributes` for the required
   channels and enable `Use Material Attributes` on a parent that consumes that output.
3. **Blend the results with camera depth** (4:49–5:17). Feed `CameraDepthFade` into the alpha of
   `Blend Material Attributes`, with near attributes at A and far attributes at B. Confirm alpha
   is 0 nearby and 1 far away; expose the fade start and a positive transition width. Blend the
   sampled results rather than interpolating UV scale, which can make features appear to expand
   as the camera moves. Camera depth is camera-relative, not distance from the character. Test
   camera turns and third-person offsets; if radial distance is required, use an explicitly
   camera-to-surface distance mask instead. Do not substitute the translucent intersection
   `DepthFade` node. See the worked [landscape material concepts](https://www.unrealcode.net/NaniteLandscapeMaterials/)
   and Epic's [depth expressions](https://dev.epicgames.com/documentation/en-us/unreal-engine/depth-material-expressions-in-unreal-engine).

This is a conceptual material graph, **not GraphCode grammar**:

```text
landscape coordinates -> surface function(near scale) -> attributes A
landscape coordinates -> surface function(far scale)  -> attributes B
CameraDepthFade(start, transition width)              -> alpha
A, B, alpha -> Blend Material Attributes -> material output
```

Before authoring, discover the installed material and material-function kinds through `gc_describe`
and current tool help. Inspect an existing function's pins and implementation where useful. Use
`gc_apply` for writes, read back the result, and prove it on the actual receiving surface. A tutorial
node name does not establish a supported GraphCode token or an installed asset path.

## Choose additional techniques by the visible problem

| What is visible | Technique to consider | Preserve or check |
|---|---|---|
| Repeated distinctive marks within each tile | UV variation or a supported texture-bombing/stochastic sampling function | Stable spatial variation, consistent channels, blend seams and normal direction. |
| The same light/dark pattern across a whole valley | Low-frequency macro variation independent of the fine texture | Restrained contrast and meaningful weather, moisture or geological zones. |
| Detail looks good nearby but forms a grid far away | Near/far sampling and a gradual distance blend | Believable feature size and no visible transition band. |
| A cliff stretches horizontal landscape UVs | Triplanar/world-aligned projection with matching normal handling | Projection boundaries, orientation and the extra sampling cost. |
| Repeated modular surfaces still look identical | Authored masks, vertex paint or localized decals/stamps | Construction logic, controlled placement and an acceptable underlying material. |

**Add macro variation at a separate scale.** A low-contrast tileable mask sampled at several
different sizes can break up broad uniformity while fine detail remains legible. Tune its influence
instead of multiplying strong dark patches over every material. Macro variation and distance
tiling address different scales and can work together; [World of Level Design's original tutorial](https://www.worldofleveldesign.com/categories/ue4/landscape-macro-tiling-variation.php)
demonstrates that combination. In production, relate broad masks to the environment's drainage,
exposure and surface palette rather than adding uniform noise everywhere.

**Protect physical scale and direction.** Record how much real surface each source texture covers;
rectangular scans need separate axis scales. Keep stones, wood grain and strata credible. If enlarging
the far texture creates giant pebbles, reduce the scale difference or use a compatible far surface
with fewer identifiable features. Arbitrary rotations suit directionless soil better than brickwork,
planks or layered rock. Source textures with a conspicuous landmark may need artwork repair as well
as shader variation. Extra resolution alone does not remove that repeated landmark.

**Treat normals as directions.** Rotating texture coordinates alone does not guarantee correctly
reoriented tangent-space normals. Use a function that handles the normal transformation, or retain
an offset-only variation path until rotation is verified. Inspect side lighting and normal response
across every variation boundary. SHADERSOURCE's [legacy Landscape Material Tool documentation](https://docs.shadersource.io/assets-and-plugins/landscape-material-tool/how-to-use-legacy-4.26-5.3/advanced-features/advanced-landscape-layers)
specifically reports normal problems with random rotation and pixelated dither seams with RVT.
These are reasons to test the installed implementation, not claims that every current version
has the same defect. Test any dither path using the game's actual anti-aliasing and RVT settings.

**Projection fixes stretching, not repetition.** Epic's [texturing functions](https://dev.epicgames.com/documentation/en-us/unreal-engine/texturing-material-functions-in-unreal-engine)
describe world-sized mapping, `WorldAlignedTexture`, `WorldAlignedNormal` and detail texturing.
Use matching projection for the surface channels. World-space mapping can swim over a moving object;
choose object-local mapping or authored UVs when the texture must move with it. High-frequency detail
can enrich a close view but does not replace macro variation. Keep proper mipmaps and filtering;
check grazing views and motion before increasing sharpness or detail-normal strength.

## Plan macro and micro imperfections beyond terrain

Apply the same scale discipline to rock, masonry, timber, metals, painted surfaces, leather,
fabric, glass and other materials. Follow the [realistic-surface quality rule](ASSET-QUALITY.md#realistic-surfaces-carry-believable-imperfections):
imperfection should explain manufacture, handling and weather, while preserving precise interfaces.
Clean, new and stylized surfaces still follow their references; dirt is not mandatory decoration.

| Scale | Examples to consider | What should drive it |
|---|---|---|
| Macro: reads across the whole object or terrain patch | Broad fading, damp zones, oxidation, dust accumulation, soil changes | Exposure, drainage, shelter, age and maintenance. |
| Meso: reads as local events | Scratches around a latch, chipped paint, muddy splashes, leaks below a joint, worn paths | Contact, handling, gravity, assembly and repeated use. |
| Micro: reads close up or in highlights | Pores, grain, fine scratches, weave, tiny pits and finish roughness | The actual material and manufacturing process, at credible physical scale. |

Keep these scales independently controllable. Avoid placing the same scratch or dirt island on every
copy, and leave quieter regions so local wear remains readable. Curvature and cavity masks can help
place wear and deposits, but review them against the object's use; every edge need not be chipped.
For terrain, connect fine grit and small stones to broader soil, erosion, wetness and vegetation zones.

Dirt and wear should affect the appropriate surface channels, not just darken base color. Give a
deposit its own roughness and, where visible, height/normal response. Use a shared coverage mask to
keep channels aligned, with channel-specific strengths. Adobe's [Dirt filter reference](https://experienceleague.adobe.com/en/docs/substance-3d-sampler/using/filters/wear-and-finish/dirt)
separates coverage, contrast, color, roughness, height and normal intensity; this is a useful authoring
model even when the implementation uses Unreal materials, decals or a bake. It does not require
Substance. For reflection behaviour, consult Adobe's [PBR guide](https://www.adobe.com/learn/substance-3d-designer/web/the-pbr-guide-part-1).

Use geometry for visible silhouette/contact changes, material relief for shallow features and
roughness for finer reflection variation. Keep permanent baked wear editable at its source; leave
runtime changes such as rain or fresh footprints in a suitable dynamic system. Compare clean and
weathered versions under changing lights, inspect seams and mip levels, and verify the final bake
where used. More noise is not evidence of better material work.

## Consider IHP and RVT for terrain relief and integration

**Research conclusion: worth a representative terrain trial when surface depth or integration is
the visible problem; not an automatic upgrade for every landscape.** This is an assessment from
the sources below, not an in-project benchmark. Solve texture repetition with the earlier techniques
whether or not this optional path is used.

The [IHP vendor listing](https://www.fab.com/listings/f5613a9e-69d4-42d9-9995-5ae4bc992128), checked
2026-10-03, describes **Implicit Heightfield Projection** as a proprietary UE 5.8 material plugin.
It offers apparent heightfield depth without moving vertices; terrain collision remains unchanged.
Its RVT node consumes a compatible **Displacement RVT** with world-space mapping. Landscape layers,
writers and volumes must still be configured. Detail depends on RVT resolution and residency.
The included demonstration covers the conventional texture workflow, not a complete RVT landscape.
Read the installed `IHPHeightProjection/Docs/IHP_User_Guide.txt` before implementation.

The vendor makes no universal speed claim. Its self-shadow output needs material integration,
and version 1.0.0 does not enable silhouette masking on the RVT node. Check current compatibility,
licensing, rendering paths and GraphCode node authoring support before depending on it. A UE 5.7
project cannot assume compatibility from this UE 5.8 listing. Do not change the engine or acquire
a plugin just because this guide names it.

| Terrain need | Candidate to compare |
|---|---|
| Fine gravel/cracks need visible depth near the camera | Normal/detail mapping versus POM or IHP at matched visual quality. |
| Roads, rocks and ground need a shared surface transition | RVT blending/caching, independently of whether IHP adds enough value. |
| Large silhouette changes, deep banks or playable ledges | Authored terrain/mesh geometry and collision; assess visual relief separately. |
| Repeated stones, stains or broad color bands | UV/macro variation and authored distribution; projection alone is insufficient. |

### Keep view-dependent shading out of the RVT cache

Epic's [RVT documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/runtime-virtual-texturing-in-unreal-engine?application_version=5.7)
describes a camera-independent shading cache. Keep `CameraDepthFade` and view-dependent projection
in the final consuming material, rather than caching one camera's result. For variation inside the
RVT output, Epic describes mip-dependent shading via virtual-texture output information and
`Runtime Virtual Texture Replace`. Validate the actual plugin's writer/reader separation.

Choose the correct RVT data type: World Height and Displacement have different encodings and are
not interchangeable by name. Keep normal spaces consistent; Epic recommends world-space normals
for RVT sharing. Static surfaces suit the cache better than animated writers. Verify source-texture
streaming before cache population, tile residency and memory pools; for large worlds, consider
streamed low mips and rebuild them after relevant changes. Use current engine documentation for
virtual-texture sampling restrictions on RVT writers and a fallback on unsupported targets.

### Prove the benefit on a small terrain route

Compare the existing material, RVT alone, and RVT plus relief where supported. Include gravel,
a slope, a rock/ground join and a road edge; use the same lighting and cameras. Inspect grazing
angles, fast travel, newly streamed areas and near/far transitions. Check both warm-cache and
first-entry performance, GPU frame time and memory at target resolution. Verify that apparent
height remains plausible against feet, wheels and collision, and that shadow/normal transitions
hold up in motion. Keep IHP only if the visible improvement justifies measured cost and integration.
Record unsupported or untested cases explicitly, and verify a cooked build before release claims.

## Budget the complete material

Evaluate one representative patch before applying the setup to every landscape layer. Near/far
sampling, texture bombing and triplanar projection can multiply texture work when combined. A blend
does not automatically stop evaluating the hidden branch, and a reusable Material Function improves
maintenance without automatically reducing GPU cost. Profile the actual composition.

Expose scale, fade and variation strength as ordinary parameters where possible. Use static switches
for purposeful compiled options; static bools are not runtime toggles. Limit combinations actually
used by instances, because each used static configuration can require a shader variant. Epic's
[Material Instances documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/instanced-materials-in-unreal-engine?application_version=5.7)
explains this distinction and its compilation cost.

Inspect texture sample/sampler counts, shader complexity, GPU frame time and texture residency on
the target platform. Shared samplers, where supported and appropriate, address sampler limits;
they do not remove texture fetches. Test the landscape component with the most overlapping layers,
not only an isolated layer. Consult Epic's [Landscape Materials guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/landscape-materials-in-unreal-engine)
for component-level layer behaviour. Keep only techniques that measurably improve the result within
budget, and verify each supported quality tier.

## Acceptance before expanding production

- Capture matching before/after views at close, middle and far distances on a representative patch
  containing a flat area, slope, cliff and layer boundary where applicable.
- Walk and turn the real game camera through the entire fade range, including fast traversal.
  Reject popping, swimming, noisy dithering, shimmer, changing apparent feature size and visible bands.
- Inspect component/UV/projection boundaries and every relevant channel under neutral and grazing
  lighting. Confirm that reducing color repetition did not introduce incorrect normals or roughness.
- Check bare ground as well as the populated environment. Foliage and decals must not merely hide
  an unacceptable base surface. Preserve designed regular patterns where they belong.
- Compare performance with variation disabled and enabled at identical settings. Record source
  texture dimensions, chosen scales, fade parameters, engine/function versions, captures and cost
  in the project's material note. Verify the cooked result before claiming release readiness.

Finish when the intended views and movement pass, not when a material compiles or one screenshot
looks less repetitive. Keep failed views in the repair list and repeat them after changes.
