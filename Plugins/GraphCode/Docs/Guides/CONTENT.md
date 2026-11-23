# Where content comes from

*A GraphCode guide. Read it when the task needs it; [`SETUP.md`](../../SETUP.md) routes here.*

Choose the source that fits the object and the owner's rights. GraphCode can author many Unreal
assets, build geometry, rigs, grooms and worlds from intent, and import supplied files. It does not
turn someone else's source mesh, texture, audio or identity into a thing the AI created.

| Source | Good use | What to keep |
|---|---|---|
| A GraphCode recipe | Revisable shapes, rigs, hair, regions and other supported intent | The authored file, dependencies and a render or runtime result. |
| Content already in the project | A legitimate asset that fits the design | Its package path, scale and any reuse limits. |
| An external file or archive | Art or audio made elsewhere | Original source, licence, import plan/settings and resulting package path. |
| A 2D artwork or audio generation service the owner selected | Reference images, textures, motifs or audio needing outside generation | Service, model, prompt, tier, terms and the accepted output. |
| A placeholder | A quick way to test the game loop | What it represents and when to replace it. |

**Use what is already available before spending money or importing a pack.** Shape is a strong
starting point for structured geometry whose dimensions and sockets will move during design.
Rig and Groom can express later character work over a Shape result or imported mesh. A sculpted
surface, source recording or licensed identity may still need an external source. Choose by the
actual design and inspect the result; do not assume one product can author every kind of source art.

For artistic shapes and surface detail, follow [ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md): SVG for
designed contours, iterative render-and-brush sculpting for depth and local corrections, decals for much of the detail,
and ImageGen for textures and motif artwork. Keep simple or naturally parametric forms procedural.
Trace or redraw generated raster motifs into clean SVG paths before using them as Shape outlines.
Keep editable sources and verify the final material bake and texture budget.

**User direction, 8 October 2026: Tripo and other generative 3D model approaches are retired for
unreliability.** Use native construction, SVG and controlled brush operations for authored forms.
The [earlier Tripo guide](TRIPO3D.md) is retained as superseded history.

For game UI, use [UI-UX.md](UI-UX.md) for the design and [UI-TOOLS.md](UI-TOOLS.md) for external
prototypes, image generation through available tools or service APIs, editable vector/raster
sources, exports and the Unreal handoff. The default is HTML/CSS first with artwork generated
where useful. Keep text, interaction and layout live; a generated image of a menu is not a
functioning interface.

To inspect a supported asset supplied by Unreal or an enabled plugin, use `gc_read` with
`source:"live"` and its mounted package path, such as `/Engine/EngineSky/BP_Sky_Sphere`.
The editor must have that content mounted. These library reads return text in memory;
`sidecar:none` is expected where GraphCode does not maintain a `.gc` file. Use
`organise:true` when reading Blueprint logic. Reading a library asset does not make its
installation directory a GraphCode write destination.

Match the source to the agreed fidelity. For production work, approve one representative asset
in the actual level before importing or generating a batch. Inspect scale and pivots, collision,
material slots, texture density, normal/roughness response and performance at the intended camera
distance. Exercise animation and deformation for characters. A temporary proxy needs an explicit
replacement task; its presence does not satisfy the production art bar. See the
[production workflow](PRODUCTION.md) for the slice that proves the choice.

## Build layered models from a clean base

For a character or another model made from replaceable layers, generate or author the base first,
then add the parts. A character base should normally have no hair, eyebrows, facial hair, clothing
or accessories baked into it. Here, **bare** means an uncluttered construction source; it does not
require a nude final presentation. Use an appropriate neutral underbody or coverage layer when the
project needs one, while keeping the replaceable pieces independent.

```text
clean body and head
  +-- skeleton, skin and facial deformation
  +-- scalp hair, brows and facial hair
  +-- clothing and footwear
  +-- accessories and equipment
  `-- final combined LOD or performance bake, when measured and needed
```

This order lets an AI inspect topology and skin weights, fit clothes against the actual body, hide
body regions under garments, repair clipping, swap hairstyles or outfits, give each layer its own
materials and LODs, and decide which garments need simulation. Keep every part on the same scale,
origin, neutral pose and skeleton convention. Check seams, intentional overlap, hidden-body regions
and deformation together before accepting the set.

When the base itself is generated as separate head, torso, limb, hand or foot meshes, treat them as
one source family and combine prepared anatomical connections through Shape rather than flattening
them blindly in an external editor. The generation contract, socket/transition model, organic join
workflow and validation ladder are in
[ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md).

A fused stylized character or a supplied scan can be a deliberate exception. A final performance
bake can also merge layers after the representative poses and camera distances pass. Preserve the
layered source before merging; otherwise one eyebrow, shoe or collar correction becomes a whole
character rebuild. For the downstream character and clothing paths, use
[ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md), [CHARACTERS.md](CHARACTERS.md) and
[GARMENT.md](GARMENT.md).

If the goal is a MetaHuman, the generated clean body/head is a **reference source**, not the final
runtime topology. Import and inspect it, then fit a native identity through `.gchuman`; the resulting
MetaHuman keeps its native Body, Face, DNA/RigLogic and post-process path. Hair, brows, clothing,
materials and accessories are reassigned as separate layers. Use `.gcrig` instead when the generated
mesh's own topology, skeleton or non-human proportions must remain authoritative. The complete
choice and verification route is in [CHARACTERS.md](CHARACTERS.md).

## Import and provenance

### Use Blendkit generators and materials through Integrate

For downloads, add-on installation, live Blender connections and target-project configuration,
follow [Blender setup](BLENDER-SETUP.md). The AI completes routine setup and verifies it.

**Actively use [Blendkit (formerly BlenderKit)](https://github.com/BlenderKit/Blendkit) when a
suitable generator, model or material can supply the production result.** For a rock family,
search for a rock generator, download it into Blender, produce the rocks, then send the finished
meshes and materials through GraphCode Integrate. Apply the same approach to material sourcing:
find the material, download and tune it in Blender, then translate it through Integrate. Do not
stop at suggesting the library or make the owner perform routine search/download/export steps
when available tools and existing account access can do them. Reuse a suitable existing download.
This route complements GraphCode authoring; choose by the required result and editability.

1. **Find a source that fits the brief.** Search the library's models, scenes, node groups and
   materials as appropriate. Search terms such as `rock generator`, `cliff generator`, `gravel`,
   `weathered stone` or `wet soil` describe different needs. Inspect the listing, required Blender
   version, dependencies, exposed controls, licence and account access. Examples checked on
   2026-10-03 include [Rock Generator - Procedural](https://www.blendkit.com/get-blendkit/872f699d-937a-4b53-9462-67b5ba8f69d9/)
   and [STONED - Mesh to Rock Generator](https://www.blendkit.com/asset-gallery-detail/26d0e85a-8a01-4812-ba59-776ef1830dcf/).
   These are discovery examples, not tested GraphCode presets; inspect current listings before use.
2. **Download and keep an editable source.** Use the installed Blendkit add-on or a supported
   automation interface with existing account access. Keep the library cache intact and save a
   project-owned working `.blend`, preserving the generator, node groups, textures and dependencies.
   Record the source URL/asset ID, author, licence, Blender version and generator settings. Use
   existing rights and spending decisions; only request missing account access or new paid usage
   when needed. Do not invent a download API or ask the owner to fetch an accessible free asset.
3. **Produce one finished representative asset.** For rocks, tune broad silhouette, strata,
   fracture, erosion and surface scale against the biome's references. Inspect several angles,
   underside/ground contact and close relief before generating the family. Make deliberate seed
   and proportion variants; preserve the selected seeds and controls. A generator's default sphere
   with random noise is not automatically a finished rock. Apply the same craft and theme checks
   as an asset authored from scratch.
4. **Prepare the evaluated static result.** Preserve the live generator in the source. Verify that
   its output contains the intended mesh faces and material slots; realize instances or convert
   non-mesh output on an export copy when required. A successful Geometry Nodes evaluation does
   not prove an exporter included all instances. Check UV0, normals, pivot and real dimensions.
   Use the installed Integrate workflow's unit convention; currently Unit Scale 1 means one Blender
   unit becomes 100 cm. Mesh-dependent baking needs a suitable non-overlapping UV0 in the 0–1 tile
   and must accompany that same evaluated mesh.
5. **Deliver with GraphCode Integrate.** Follow the [Integrate authoring route](AUTHORING.md#integrate-bring-an-external-source-across-a-boundary)
   and the installed `Plugins/GraphCodeIntegrate/README.md`. Use its selected-mesh
   workflow for geometry plus assigned materials, or its material-only workflow for a downloaded
   surface. Inspect the conversion report and import plan before applying the bundle. Prefer
   translated editable nodes with selective bake fallback for unsupported branches; read every
   frozen control, approximation and unmet requirement. A raw `.blend` import or FBX auto-material
   is not a substitute for the shader translation. Verify installed mesh-send support rather than
   assuming a material-only companion has it; preserve a prepared source/bundle and identify the
   missing capability if the installed release cannot deliver it yet.
6. **Verify in Unreal and expand.** Compare Blender and Unreal under comparable lighting, then
   inspect the actual level. Check silhouette, dimensions, pivot, UV density, all material slots,
   normal direction, roughness and relief. Author and verify needed collision, LOD/Nanite choices
   and placement separately; mesh delivery does not establish these. Prove one asset before a batch.
   Keep stable object names/destinations and Integrate ownership records for managed reimport;
   revise the Blender source and resend when fixes are needed.

**Materials follow the same source-to-result loop even when no mesh is being imported.** Apply a
downloaded stone, metal, wood, fabric or dirt material to a representative receiving object in
Blender, tune its physical scale and finish, then send it through Integrate. Check whether Object
or Generated coordinates, AO or other branches depend on that object. A bake on one rock is not
automatically a reusable tiling landscape material: choose an object-independent UV source for
reusable surfaces, or keep the bake bound to its intended mesh. Preserve supported source maps
instead of replacing per-pixel roughness or normals with convenient constants.

Use [texture variation and surface imperfections](TEXTURE-VARIATION.md) after transfer to inspect
repetition, macro/micro detail, dirt, wear and near/far appearance. A high-quality library material
can still repeat visibly over terrain. Keep durable `.blend` sources with the project's source art
and temporary interchange/bake outputs under `Saved/GraphCodeIntegrate`; retain its managed import
history as directed by Integrate. Record the resulting Unreal paths in the asset log.

For Blender and BlenderKit sources, start with the [Integrate workflow](AUTHORING.md#integrate-bring-an-external-source-across-a-boundary)
for static meshes, converted materials and managed reimport. The companion can send selected
meshes and their translated materials together; an exchange file's automatic material import
does not replace Integrate's shader translation.

Use `gc_import` to **plan** an external file or archive first. Inspect the members, destination,
importer settings and proposed asset paths before committing it. Read the resulting Unreal asset
and render or listen to it. Keep the original source available: a `.gc` usually describes its
authored Unreal settings, not every pixel, sample or vertex in the input file. Log where the source
came from at import time, while the answer is still known. The
[`ASSET-LOG.md`](../Seeds/ASSET-LOG.md) seed gives the project a place for this.

### A texture set is several maps for one surface

Import and connect each map by its role. A folder full of normal maps may be correct when it
contains several plants, bark variants and ground surfaces. Check the actual texture settings
through `gc_read`; a thumbnail colour or a `bNormalizeNormals` flag alone does not establish
which compression mode the texture uses.

| Map | Material input | Import check |
|---|---|---|
| Base colour / albedo / diffuse | Base Color | sRGB enabled for ordinary colour images. |
| Normal | Normal | Normal-map compression, sRGB disabled; verify the source's green-channel convention. |
| Roughness | Roughness | sRGB disabled; read the intended channel. |
| Opacity / alpha | Opacity Mask for cutout foliage | sRGB disabled; check that leaves retain their silhouette at distance. |
| Ambient occlusion | Ambient Occlusion where appropriate | sRGB disabled; follow any packed-channel specification. |

For a known OpenGL normal source imported into Unreal's DirectX convention, flip green once.
Do not also invert it in the material. A DirectX source needs no such conversion. Confirm the
result under directional light; do not infer the convention solely from the image's colour.

Check source encoding as well as the texture's `SRGB` flag. Some 16-bit PNG colour images arrive
as linear source data even though their pixels were authored in sRGB. In that case, read the
texture's `settings` and set `SourceColorSettings.EncodingOverride` to `TSE_sRGB` through
`gc_apply`, preserving the other source settings. Verify against the original image. Do not apply
this to normal, roughness or other data maps, or assume every 16-bit image needs it.

If the original, texture thumbnail and scene disagree, isolate those stages before changing
material brightness or lighting. A September 2026 Wilderness trial exposed an extra sRGB
conversion in GraphCode's capture target. The corrected target uses Unreal's display gamma on a
linear render target, matching the native thumbnail renderer. A correctly set `SRGB` flag alone
does not prove either import fidelity or capture fidelity.

An FBX importing successfully does not prove its materials survived. Read its material slots,
assign the intended Unreal materials, and inspect both a close view and the playing view.
Foliage also needs correct cutouts, two-sided lighting where appropriate, restrained wind,
ground contact, collision and a measured distance strategy. Preserve matching pivots across LODs.

**Free to download is not a shipping licence.** Check the current terms for the exact item and
account tier before adding it to a release. Record any attribution in the game's credits when the
asset enters the project. If the rights are unclear, keep a placeholder while the owner resolves
them. A source list is a set of places to investigate, never a blanket licence for everything on
that site.

For generated content, record the service, model and prompt as well as the output and terms. One
generation, then look at it and show the owner; do not fan out paid variants because you are
unsure what they meant. Use existing service and spending decisions; ask only for missing choices.
If they supply files instead of a connected service, agree on the source format and maps before
they export.

### Custom sound uses the ElevenLabs API

For sound that must be made specifically for the project, use the ElevenLabs API as the preferred
generation route. Choose the current official capability that matches the job—sound effect,
speech/dialogue or music—and read its current model, format, duration, looping, licence and cost
rules from the [ElevenLabs API documentation](https://elevenlabs.io/docs/api-reference). The
[sound-effects quickstart](https://elevenlabs.io/docs/eleven-api/guides/cookbooks/sound-effects)
is the starting route for custom Foley, ambience and game effects. Do not freeze an endpoint or
model ID into a project guide; those are checked again when the sound is generated.

Use the documented `ELEVENLABS_API_KEY` environment variable and keep it out of project history.
Generate one deliberate candidate first, listen to the actual file, then show it to the owner before
spending credits on variants. Keep the prompt, capability, model, duration/loop intent, output format,
generation date, accepted source file and usage terms in the asset log. Import the accepted source
through the normal Unreal path, then listen to the Unreal asset as well as the original: compression,
loop points, attenuation and runtime mixing can change a correct source.

For voices, use only a voice the owner is entitled and authorized to use. A request to imitate or
clone a real person is not ordinary sound generation; verify consent and the service's current rules
before creating it.

A mood board is a design reference, not an imported game asset. Keep its images under project
design docs rather than `Content/` unless the game actually uses them. Use it to settle the look
with the owner before commissioning or generating finished art.

## Keys stay out of the project history

If a chosen service needs an API key, prepare one `.env` at the project root with only that
service's **documented variable name** and an empty value. Add `.env` to `.gitignore` before anyone
fills it in. The account owner supplies the value; never paste, read back, log or commit it. A
keyless `.env.example` may be committed. If no service was selected, create no secrets file.
