# Artistic shapes: SVG outlines, texture brushes and render-and-brush work

## What this guide is for

Use it when a shape's value is in its contour, ornament or soft surface detail rather than in hard-surface
construction: an emblem, a scroll, a cartouche, a nose or cheek, bark, worn stone. It helps you decide which method
to use for each feature, then walks through each method in the order you will meet them:

1. choose a method (below);
2. plan the artwork and the surface that receives it;
3. make artwork with ImageGen and convert it to SVG;
4. give the SVG physical form in Shape;
5. paint surface detail with decals and masks, and sculpt natural forms with texture brushes;
6. iterate with render and brush;
7. bake and budget the result.

Read it with [ASSET-QUALITY.md](ASSET-QUALITY.md). For the grammar of structure, joins and crisp lines see
[GCSHAPE.md](GCSHAPE.md); for rounding and edge treatment see [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md).
Installed tool help owns exact grammar and available capabilities (`gc_describe kind:"shape"`).

## Choose a controllable authoring method

Neither an SVG nor a prepainted texture is required for every artistic form. Pick whichever expresses the feature
most clearly and stays editable.

- **SVG for contour-led work that is hard to describe procedurally.** Use an editable contour, profile or footprint
  as soon as it says the form more clearly than a long run of primitive shapes or hand-computed coordinates. The SVG
  supplies the outline; Shape's operations supply depth, fitting, scale and precise relationships. Keep simple or
  naturally parametric forms procedural.
- **Texture brushes for continuous organic volume.** A nose, cheek, lip mound or other soft feature can be shaped by
  a height or opacity field on the receiving mesh, with controllable depth and strength. SVG remains useful for the
  footprint, silhouette or openings.
- **Render and brush for forms judged by eye.** The loop is render, measure the target, author one local brush
  operation, rebuild, render and repair. It includes real depth through displacement as well as material painting.
- **Decals and masks for surface detail.** Use decals for much of the painted detail, and ImageGen for texture
  artwork and design references from which SVG can be made.

Combine these with reusable parametric construction for the underlying forms and interfaces. Build recipes around
authored contours and continuous fields with meaningful controls, rather than hard-coding every crack or irregularity
into a rigid construction sequence. Keep parametric construction for dimensions, connections and relationships that
must stay precise. Where a feature's visible or physical requirements need another representation, use it and note
why in the asset's build guide.

This workflow builds with native construction, editable contours and controlled brush operations. For an organic
form (a creature, a character, a sculpted shape) use a generated model instead: see the organic route in
[CAPABILITY-MAP.md](CAPABILITY-MAP.md). ImageGen is available for 2D reference and artwork; image brightness is not a measured
depth field.

For unique contours, start from texture artwork, then convert it to named paths ([below](#convert-texture-artwork-to-svg)).
The artwork carries the unique contour while Shape supplies physical size, depth, fitting and interfaces.

This keeps the design editable and puts geometry where it earns its cost. Keep the full quality bar: distinctive
silhouettes, purposeful detail and repeated visual inspection. Measure runtime cost on the finished asset.


Match each feature to a starting point:

| Feature to author | Preferred starting point | Use geometry when |
|---|---|---|
| An emblem, leaf, scroll, ornamental plate, cutout or other precise artistic contour | Texture/motif artwork or a silhouette mask → inspected named SVG filled paths → Shape extrusion, loft or surface fitting | The contour needs thickness, a real opening, a raised border or a separate fitted piece. |
| A nose, cheek, lip mound or other smoothly varying organic form | An authored height/opacity texture brush on a declared receiving surface, with weighted `displace` | Its volume and silhouette need real shape; control the form through the field rather than assembling many rigid bumps. |
| A form whose depth or local transitions are easiest to judge and correct in views | Iterative render-and-brush authoring on a protected receiving region; use a procedural profile or an optional texture brush | The correction changes volume, silhouette, contact or a visible recess; measure and inspect actual displacement. |
| Painted motifs, lettering, insignia, patina, stains, scuffs or fine surface ornament | Decals or masked material painting, with ImageGen artwork where useful | The feature must affect silhouette, readable depth/parallax, contact or collision. |
| Rock grain, moss coverage, pores, fine scratches and shallow relief | Texture/material channels and painted masks; use a normal or supported height workflow for relief | Large chips, deep cracks or moss clumps must read as actual volume. |
| A housing, bracket, support, joint or repeating structural part | Existing parametric recipes, dimensions and sockets | Its construction already calls for a physical part; SVG and decals can supply its artistic finish. |

For example, make a rock's broad broken form in geometry, paint moss and mineral variation through
controlled masks, and add actual clumps only where their volume reads. For an ornate metal fitting,
use an SVG silhouette with real openings and rounded thickness, then decals for engraved-looking
lines, enamel, patina and fine wear. Give a deep engraving real geometry when the intended view
needs the recess.

## 1. Plan the artwork and the receiving surface together

Start from the governing reference and the intended camera distance. In the reference-to-construction
table, assign each feature to SVG-derived geometry, parametric geometry, iterative brush sculpting,
or painted surface detail.
Record physical scale, required thickness, texture density, attachment and the intended bake target.
Design one representative motif and finish it on the actual receiving part before repeating it.

Reuse accepted project artwork and recipes. Keep editable SVG, generated originals, cleaned textures,
masks and construction recipes alongside their provenance in the project's appropriate source-art
and design locations. Save prompts, reference inputs, generation settings and selected versions.
Project dependencies must not live only in a tool's temporary or personal output directory.

## 2. Use ImageGen for texture art and SVG design references

Use the available ImageGen tool and its current instructions for new raster artwork or edits.
Provide the established reference so the texture or motif belongs to the asset's theme. Generate
one useful source at a time, inspect it, then revise the specific defect. A production source needs
different framing from a beauty render:

- **Tileable surface artwork:** request a flat, orthographic material swatch at a stated physical
  scale, even neutral illumination, no cast shadows, perspective or highlights, and matching
  opposite edges. Check the actual output in a repeated grid; a request for seamlessness is not
  proof that the seams or repeating landmarks are gone. The shipped `make_seamless.py` makes a
  tile or a whole texture set seamless with identical cuts, removes landmarks, cuts it for a
  layout and verifies it in a 10×10 field. `--check` tests a generator's seamless claim
  ([MATERIALS.md, make the set tile](MATERIALS.md#make-the-set-tile-and-prove-it)).
- **Decal artwork:** request an isolated motif with the needed transparent background or explicit
  mask, enough margin, and no receiver or scene lighting painted into it. Inspect alpha edges and
  remove halos before import. Preserve deliberate painted shading when the art style calls for it.
- **Brush artwork and footprints:** ImageGen can provide a visual reference or a proposed pattern
  to shape into a brush. It does not reliably recover depth: a generated image's brightness or opacity is
  not a height, even when the prompt asks for a height field. Author the height values deliberately or derive them from geometry; keep generated
  grayscale artwork marked as a concept until its displaced form has been accepted. Image range,
  neutral color and a black background do not validate depth.
- **Flipbook frames for an effect:** request the whole sheet in one generation on flat black, with
  the grid, frame count and order stated. Then repack it onto an exact grid in code and make its
  alpha and loop yourself: [FLIPBOOKS.md](FLIPBOOKS.md).
- **Artwork to become SVG:** request a front-on, flat silhouette or clean high-contrast motif,
  intentional negative space, crisp filled shapes and no bevel, shading, texture or perspective.
  Keep separate parts and cutouts easy to distinguish. Inspect its design before tracing it. The
  prompt matters more than any converter setting. Append to the subject:

  > flat vector logo style, solid fills only, 2 to 4 colours, no gradients, no shading, no shadows,
  > no texture, no outlines thinner than the shapes, bold simple shapes, centred, plain white
  > background, nothing touching the edges

  - Ask for a real transparent background when the generator supports it. A painted grey-and-white
    checkerboard is detected and removed, but inside a hole only where both squares show.
  - Use one colour per part you want to address separately. Two shades of one colour become two
    shapes.
  - Separate parts with background, not dark outlines; a 3 px outline becomes its own shape.
  - Generate at 1024 px or larger. Anything thinner than about 2 px at that size is treated as edge
    noise.
  - Keep lettering large. Letters under about 40 px tall come out as rounded estimates; use
    `--detail high`, or leave the text out and add it another way.
  - For line art ([below](#trace-line-art-as-centre-lines)), ask instead for "clean line drawing,
    uniform dark strokes, no fill, no shading, plain white background", with lines at least 2 px wide.

ImageGen supplies raster artwork. Convert the selected motif with the image-to-SVG loop below,
then inspect and clean its paths. Embedding a PNG in an SVG container does not supply Shape outlines.
For an existing editable SVG, edit its vector source directly; preserve the designed curves and IDs.

A generated color image is not a complete PBR material. Author or derive the required roughness,
normal, height and masks through a suitable material workflow and inspect them under changing
lights. Brightness alone does not identify surface height or roughness. Keep color and data maps
in their appropriate import/color-space settings; follow [CONTENT.md](CONTENT.md#a-texture-set-is-several-maps-for-one-surface).

### Convert texture artwork to SVG

When SVG is the chosen representation, use this source-art route for a unique contour that is
awkward to describe with procedural operations:

1. **Create the artwork before the geometry.** Generate or reuse a front-on texture/motif image
   with the intended silhouette, deliberate asymmetry and negative space. For conversion, use
   solid contrasting fills, a real transparent background or a plain background, margin around
   the design and one colour per intended part. Inspect and revise the artwork itself first.
2. **Choose what the image should contribute.** Trace its outer silhouette, explicit mask or
   distinct flat regions. A shaded material texture can supply a design reference; isolate its
   intended contour in a mask or a flat art variant before tracing. Keep grain, lighting, colour
   variation and the original full texture for their material/decal use. Image brightness alone
   does not establish physical height.
3. **Run the converter outside Unreal.** GraphCode ships `img2svg.py` in
   `Plugins/GraphCode/Tools/ImageToSvg`. It runs without the engine on Python 3.9+, NumPy and
   OpenCV. It turns the image into an SVG of **closed, filled, named shapes**. Each colour, or each
   piece with `--split`, becomes one `<path id="...">` with absolute coordinates. There are no
   transforms, groups, strokes or gradients, and every hole is wound against its outline so the
   non-zero fill rule cuts it. That is the shape the SVG route wants, where a `.gcshape` names a
   shape by its `id` (ask `gc_describe kind:` for the current spelling). From the project
   root:

   ```powershell
   pip install -r Plugins/GraphCode/Tools/ImageToSvg/requirements.txt
   py -3 Plugins/GraphCode/Tools/ImageToSvg/img2svg.py SourceArt/Emblem/motif.png --split -o SourceArt/Emblem/motif.svg
   py -3 Plugins/GraphCode/Tools/ImageToSvg/img2svg.py crest.png --colors 3 --names shield,ring,star
   py -3 Plugins/GraphCode/Tools/ImageToSvg/img2svg.py logo.png --mode silhouette
   py -3 Plugins/GraphCode/Tools/ImageToSvg/img2svg.py "refs/*.png" -o svg/
   ```

   Use `python` in place of `py -3` on hosts without the Windows Python launcher. `--help` lists
   every option. **Outlines use few points by default.** Each outline is the fewest lines and cubic
   curves that stay within `--tolerance` (3 source px by default) of the traced edge. A circle
   becomes three curves, a rectangle four lines and a five-point star ten lines. A test crest takes
   26 segments where a plain tracer spends about 4,000. A 1024 px image converts in 1–2 s on one
   CPU core, and the same input always gives the same output.
4. **Inspect all outputs and iterate.** Every run writes three files and prints one line per shape:

   ```text
   crest.png -> crest.svg  924 x 839  26 segments  fidelity 0.977
     ring             #d1593c  area 203750     outlines 1  holes 1  segments 6    iou 0.976
     star             #efa91f  area 65351      outlines 1  holes 1  segments 12   iou 0.981
   ```

   - `motif.preview.png` shows the source, the traced SVG in flat colours and the disagreements in
     red. Look at it first; it is the fastest check there is.
   - `motif.svg.json` holds the same numbers plus each shape's `bbox`, the crop, the units and any
     warnings. `segments` counts lines and curves, the point count that matters. `fidelity` is the
     area-weighted IoU of the re-rasterised paths against the cleaned colour regions. A looser
     `--tolerance` lowers it on purpose, and below 0.94 a warning says so. `--json` prints the report
     instead of the summary.
   - **Units are source pixels**, cropped to the shapes. SVG user units are not centimetres; state
     the real size where the shape is used. `--fit 100` scales the longer side to 100 units if a
     round number helps.

   Check the design's thin tips, holes, joins and each needed part. Fidelity measures agreement
   with the converter's cleaned regions; the original artwork and the visible design remain the
   acceptance reference. A high score cannot excuse a missing feature. Adjust flags and rerun;
   never hand-edit path coordinates. Stop when all of these hold: the set of shapes is right, no
   warning remains that you have not deliberately accepted, the preview's red is a thin line along
   edges, and the segment count looks like what a person would draw. If the image itself is the
   problem (shading, texture, fine lines), say so and get a better image rather than tuning around
   it.
5. **Make the paths usable by Shape.** Keep `--split` for independently addressable disconnected
   pieces: Shape expects one connected outer contour plus holes per selected SVG element. Once
   the parts are correct, rerun with semantic `--names` IDs, assigned largest-area first, and verify
   which part each ID names. Keep source art, SVG, report, preview and accepted conversion settings
   together. Import the accepted SVG as SVG Data and continue with the physical-form workflow below.

| Conversion issue or intent | Adjustment and check |
|---|---|
| Only the outer design matters | `--mode silhouette`; preserve intentional holes. Add `--fill-holes` only when those openings should actually close. |
| Shading became unintended parts; several shades of one colour, banding across a shape | Flatten the source or use a deliberate mask; `--colors N` with the intended flat-region count (one fewer than the banded result), or `--mode silhouette`. |
| A part you do not want (a white fill, a shadow) | `--drop white` drops `white`, `white_2`, … |
| Enclosed white details became holes instead of shapes | `--bg-connected`: background only where it reaches the border. Check the preview's holes. |
| Wrong or no background detected | `--bg "#ffffff"` (two colours for a checkerboard), or `--bg none`. |
| Two pieces of one colour should be addressable separately | `--split`. |
| Too many fitted segments | Increase `--tolerance` gradually (6, 10, …) and compare the silhouette; its units are source pixels. |
| Important small detail or corners were lost | Lower `--tolerance`, try `--detail high`, or improve the source; inspect the result rather than relying on the flag. |
| A rounded corner should be sharp, or the reverse | `--corner` lower for more corners, higher for fewer. |
| Specks, too many pieces | `--detail low`, or a cleaner image. |
| Thin lines vanished, or came out as slivers | `--mode lines` ([below](#trace-line-art-as-centre-lines)). |
| Straight lines only | `--tracer opencv`: a polygon within `--tolerance`. `--tracer vtracer` (MIT) remains for dense splines. |

IDs come from colour names (`red`, `blue_2`); `--names a,b,c` renames them largest first, after any
`--drop`.

#### Trace line art as centre lines

For drawings made of strokes (a plan, a route, a wire, a zigzag trim, a sketch), `--mode lines`
traces the middle of each stroke instead of its outline. A stroke becomes one path that stays open
unless the drawing closes it, with `fill="none"` and the stroke's measured `stroke-width`. So an
open polygon stays an open polygon, not a thin filled sliver:

```text
plan.png -> plan.svg  889 x 785  10 segments  fidelity 1.000
  blue             #234f8d  width 6.24  (6.23-6.25)  strokes 2  closed 2  segments 6    match 1.000
  red              #bd2c2d  width 6.04  (5.92-6.23)  strokes 2  closed 0  segments 4    match 1.000
```

- A line carries straight on through a crossing, so a crosshair is two lines and its rings stay
  closed. Corners stay sharp, and free ends reach where the ink stops.
- Each ink colour becomes one path, with each stroke a subpath; `--split` gives one path per stroke.
- Filled areas are not lines. An ink mostly wider than 4% of the image is skipped with a warning,
  so an outlined icon gives its outlines. Trace the fills with `--mode colors`, or move the limit
  with `--max-width` (source px). Bold filled text is not line art either: use `--mode colors`.
- `match` is the share of ink the strokes cover and of stroke that lies on ink, within
  `--tolerance` either way. `--tolerance`, `--corner`, `--names`, `--drop` and `--fit` work as in
  the other modes.

**Width profile, for lines that are not uniform.** An SVG path has one `stroke-width`, so the drawn
line uses the stroke's average. Its width along its length goes in two places:

- The JSON report, per stroke: `profiles` as `[fraction of length, width]` knots, plus
  `width_range` per shape. Plain numbers are the reliable carrier for an agent or GraphCode.
- The SVG, as `data-width-profile="0 0.77, 0.08 6.2, 0.24 12.33, …"` on the path, with one group per
  subpath separated by ` | `. Renderers and importers ignore `data-` attributes, so the profile
  rides along without changing the drawing.

Positions run from each subpath's `M` point. The width at a point is the ink measured across the
line. The profile is cut to the fewest knots within a tenth of the width. A uniform line is two
knots; a brush stroke that swells to 17 px and tapers to a point is about seven.

**`--expand`** draws each stroke out to its profile and outputs the filled outline instead of the
centre line, fitted like any shape: a leaf from its midrib, a brush stroke as a solid. The centre
lines and profiles stay in the JSON.

The converter fits closed filled paths (or measured centre lines) and hole winding; it does not
complete the geometry or material work. Explicitly author centimetres at the Shape outline, then inspect the imported and
rendered contour as well as the raster preview. Preserve an already accepted editable SVG when
it is the better source; avoid repeated rasterisation and tracing of its existing curves.

## 3. Give the SVG physical form

Clean tracing artifacts, excessive control points, accidental islands and self-intersections.
Convert lettering and strokes to filled paths; preserve deliberate holes, asymmetry and thin tips.
Give each connected part a stable element ID and author its game size explicitly.

Check the installed SVG import and Shape support before choosing exact syntax. In the SVG-enabled
Shape workflow, import the source to SVG Data and consume a selected element with `outline()`.
The drawing supplies contours; Shape supplies size, construction and materials. SVG fill colors,
filters and embedded images are not the material definition.

- **Flat solid ornament:** SVG outline -> extrude -> round the solid edges.
- **Changing section:** compatible SVG profiles -> loft -> edge treatment. Check the installed
  loft contract; current Shape SVG loft requires single-ring profiles without holes.
- **Thin ornament fitted to a curved host:** SVG outline -> fill -> conform -> shell -> round.
  Fit the surface first, then add thickness. Inspect projection misses, stretching, clearance,
  folds and narrow openings; carrying an already thick bevelled solid can fold on tight curvature.

Compare source and result from matching views. Round radii must fit the narrowest designed feature.
Check both the web thickness measured normal to the contour and the smallest convex curve radius.
A cubic refit can stay very close to its source outline while introducing a tighter bend, so repeat
these checks after vector cleanup. If only one bend fails, adjust its SVG handles locally while
preserving neighbouring endpoints, tangents and mounting clearances. Inspect the native rounded
result; positional fitting tolerance alone does not prove that its edge round is sound.
SVG import sampling and deformation refinement both affect triangle count: control them by the
required shape accuracy, inspect the resulting mesh, and verify the shipping geometry budget.

## 4. Paint most surface detail through decals and masks

Build a convincing base material first. Layer the designed motifs, local variation and wear over
it with decals or masked material painting. Let references determine placement and scale; preserve
quiet regions and functional interfaces. Combine broad coverage, local motifs and fine breakup.

In Shape, keep the selection's hard region separate from its weight rules. Compose point-to-point,
coordinate, angle, image-mask or procedural-noise rules to control coverage, and add an explicit
edge-falloff rule when the edge should soften. Use each rule's profile to shape the transition.
Painting should carry the relevant surface channels as well as color so moss, bare metal and paint
respond differently to light. Read current help for the supported rules and channel blending.

For a directed motif, combine that selection with `at`, `project`, `size` and optional `turn` on
the decal. Projection positions the artwork; the weight field still controls its strength across
the receiving surface. Choose the direction into the surface and inspect the footprint at grazing
angles. A projected width is measured across the projector, so it stretches over sloped surfaces.

Shape's `decal()` painting is a bake input evaluated through the selection. It creates ordinary
baked material assets; it does not require a runtime Unreal decal actor for every authored mark.
An independently placed runtime decal is a different route, useful where placement or appearance
must change in play. Choose its receiving-surface and material behavior deliberately.

### Compose natural surfaces from contours and weighted brushes

Give each authored input a clear job:

| Input | What it controls |
|---|---|
| SVG contour | The designed silhouette, outline, opening or irregular brush footprint. |
| Declared receiving surface or saved region | Where the effect is allowed; preserve interfaces and neighbouring parts through this hard boundary. |
| Texture opacity | The influence at each sample, including any falloff painted into the image. |
| Weight rules | How that influence combines with position, slope, edge distance, noise and other stamps. |
| Reusable instruction | The operation applied through the resulting selection, with controls such as depth, roughness scale and material. |

For an opacity brush, explicitly sample `channel=a`. Alpha 0 leaves the surface alone, alpha 1
gives full brush influence, and intermediate values give partial influence. Spatial variation
in alpha supplies the falloff; multiplying every alpha by the same amount only changes strength.
Use an identity profile to preserve the painted falloff, or an explicit profile to reshape it.
An extra edge rule is optional. Do not automatically multiply in another fade when the artwork
already contains the intended transition. A grayscale mask stored in RGB instead uses a colour
channel such as `r`; an opaque image's alpha cannot recover its grayscale pattern.

For cracks, start with a declared receiving region and stack several placed opacity fields.
Vary position, independent width/height, rotation, UV tiling/offset and strength. Choose the
combination deliberately: `add` strengthens overlaps up to 100% weight; `merge` takes the maximum
without increasing overlap depth; `multiply` applies an overall weathering or falloff field.
Compose the fields first and displace once when one overall depth limit is intended. Repeated
displacement passes instead accumulate physical travel and re-evaluate rules on the changed mesh.
Weights capped at 100% do not cap the total depth of several separate passes.

Use seeded variation when scattering is part of the recipe, so rebuilding retains the design.
Keep the actual placements reproducible and tied to the receiving surface. **Stacking supported
texture rules is available; automatic random scattering into a weight-rule stack is an authoring
pattern to implement or verify, not a new built-in operation supplied by the texture brush.**
Do not invent `seed` or `repeat` arguments on a texture rule. Stacked stamps make a composite
pattern; they do not by themselves simulate fracture propagation or guarantee connected cracks.

The same approach supports bark grooves, worn edges, dents, rippled or uneven surfaces, and
painted wear. SVG contributes the deliberate broad contour; texture fields supply variation
inside it; parameterized instructions make that combination reusable. Choose geometry where
depth or silhouette matters and baked material detail where it does not. A texture-controlled
indentation does not create a split or an open hole. Inspect the combined result, including
overlaps, protected boundaries, projection through other facing surfaces and source resolution.
Fine brush images can demand dense geometry; measure the finished result before reusing it widely.

### Sculpt natural forms with texture brushes

**Use a texture brush with authored height data as a first-choice method for smoothly varying organic volume.** On an
existing face surface, a nose field can describe the bridge, rounded tip and alar wings together,
with a gradual transition into the forehead and cheeks. The image controls the local amount of
protrusion; it need not be broken into separate rigid solids or hand-written vertex coordinates.
This uses the existing texture-weighted displacement mechanism; it is a recommended
approach, and the shipped texture-brush example demonstrates cracks, not a finished anatomical nose.

**Image generation does not supply the height.** A generated picture of a nose is not a height map:
image generation cannot infer depth from brightness or opacity. This limits the image-generation step,
not Shape's ability to apply a scalar brush you supply. Brightness or opacity can encode a height that has been authored or measured; neither establishes
that height by itself. Asking for a normal map instead does not establish valid geometry either.

For a dependable source, paint explicit height values against the reference and inspect the
resulting relief, or sculpt/model the feature and render its displacement from the receiving
surface into a linear height map. Keep the brush's coverage mask separate when coverage and
height differ. ImageGen can supply the design reference and footprint; automatic anatomical
depth recovery from its artwork is not a verified part of this workflow.

1. Start with the character or object reference and a suitable receiving mesh. Declare the face
   region that may change and preserve the eyes, mouth and required interfaces. Choose a footprint
   and outward direction from that surface, with physical dimensions taken from the reference.
2. Deliberately author a height field, or derive one from the modeled feature. For a nose, let the bridge rise gradually, the tip
   form a broader crest, and the wings have lower, softer lobes. Fade the field to zero where it
   meets the surrounding face. Use smaller or lower-valued regions for local depressions.
   Lighting and color in a portrait do not directly specify anatomical height. If opacity is used
   to carry height, encode those established values into alpha; do not infer depth from transparency.
3. Import the brush as a Texture2D with source data. Sample `channel=a` for authored opacity, or
   `channel=r` for a grayscale height map. Keep grayscale height data linear (`sRGB=false`); alpha
   is already sampled linearly. A uniformly opaque alpha channel cannot supply the RGB pattern.
4. Declare `weightage()` with `rule(condition=texture, texture=..., channel=...)`, and apply it
   through `select($ReceivingRegion, weight=$Brush)`. Use source UV0, or place the local brush with
   `at`, `project` and `size`, optionally `turn`. Keep placement tied to the named receiving surface.
5. Use positive `displace(by=...)` for protrusion and negative displacement for recesses. Expose
   footprint width/height, physical depth, strength and profile as meaningful recipe controls.
   With an identity profile, displacement is the authored depth multiplied by the sampled image
   value and any strength multiplier. Changing strength should retain the designed proportions.
6. Inspect front, side, underside and grazing views of the actual mesh. Check bridge-to-forehead
   continuity, tip shape, wing transitions, nostril depth, silhouette, folds and protected regions.
   Choose refinement from the visible form and budget; sharpen or smooth the field deliberately.

Compose named fields for broad anatomy and local corrections. A positive brush can build the
bridge and tip, while a separate negative pass forms a groove or depression. `merge` takes the
largest weight and `add` strengthens overlaps up to 100%; separate displacement passes instead
accumulate physical travel. Preserve the editable fields and reusable instruction alongside the
reference so the form can be revised without rebuilding a stack of primitive solids.

Use **`displace` for image-driven relief**, including a sharp mask when its mesh result is adequate.
Texture-weighted `raise` is not supported; `raise` serves a selected region that needs explicit
connecting walls or an inset. A height brush alone does not create an open nostril, an undercut
or every side of a free-standing feature: combine it with the receiving form and appropriate
opening/shape operations where those are required. Projection is through-facing, not an
occlusion-aware or arc-length wrap; the declared region must limit where the brush is allowed.

For a height image that already contains the intended shape, use a constant slope to preserve
that height field: `displace(by=0.16,along=+Z,slope=profile(0=100%,1=100%)) [$Brush]`.
The default slope adds a mound from the receiving region's border; it changes the image's heights.
Keep the image's black margin inside the receiving surface, and inspect the projection's `turn`
so the pattern lands in the intended orientation before wrapping it.

A single projected, clamp-addressed texture with an affine two-stop `0..1` weight profile
refines from its local height variation and the declared `smooth:` tolerance when `displace`
has a constant `by` and explicit `along`. Flat image areas and linear ramps can remain coarse;
hidden interior peaks still trigger refinement. The source image stays at full resolution.
Combined weight recipes, UV sampling, tiled/offset textures, varying crests and strokes retain
the existing feature-based refinement. Explicit `[smooth=]` can still demand more geometry, and
the construction safety limit remains in force. This bounds the texture contribution to the
displacement, not every subsequent deformation or the whole slope/fade product.

Exact syntax and supported geometry verbs are available through
`gc_describe kind:"shape"`.

### Iterate with render and brush

Use this approach when looking at the evolving surface is the clearest way to decide the next
correction. A brush can be a local procedural footprint and slope profile; it does not require
an external SVG or texture. Keep broad construction and precise interfaces parametric, and use
the loop for controlled local form and finish.

Some shapes carry real depth, and the choice of operation matters. **A sculpting stroke changes the receiving
mesh through `displace`; a material-painting stroke changes baked surface channels through the
existing weighted `decal`/`bake` path.** Choose the operation by the intended visible effect.
Color or a normal-map mark cannot establish the physical depth, opening or contact of a shape.
The same weighted selection can also write per-vertex colour with `colour()` for a material to
read ([vary each piece](MATERIALS.md#vary-each-piece-with-a-shape-vertex-colour)).

#### What `gc_render` supplies

The existing public tool supports the observation and placement part of this loop:

| Need | Current surface | Boundary to respect |
|---|---|---|
| Inspect shape, silhouette and finish repeatedly | Scene render with named views, explicit cameras and `view_mode`; `details:full` plus the retained manifest supply camera, projection, resolution and subject frame | Keep comparable cameras and inspect side/grazing views; shaded brightness does not measure height. |
| Locate a visible brush center and footprint samples | `mode:ask`, retained `capture`, `view`, and up to 64 named integer `pixel:[x,y]` points | Use the actual image dimensions, top-left origin, `hit:true`, `space`, and a current capture. |
| Measure depth through a closed part | Ordinary mesh-preview `ask` returns entry, paired exit, ray thickness, position and entry normal; depth accepts a fraction or `{value,unit:"cm"}` | Thickness is along the camera ray, not normal thickness. The returned normal is the entry normal, even for an interior/exit query. Open surfaces may have no exit. |
| Identify a material-painting region or UV anchor | `mode:legend, by:uv_island, uv_channel:0` makes a new source-LOD0 capture, atlas and legend; ask its `main` view at depth 0 | Use the NEW capture ID. Overlapping UVs are reported as ambiguous; a UV selector identifies an island, not a brush-sized patch. |
| Locate a static mesh in an assembly | A level capture's `ask` with explicit `mesh:{actor,component?,instance?}`; optionally `relative_to` | Ordinary level picks use collision. Explicit mesh queries report local/world frames and geometry fidelity; they can select a mesh behind another surface. |

**Geometry fidelity matters for depth.** Ordinary static-mesh preview queries currently intersect
render LOD0. A Nanite beauty preview can draw higher-detail source geometry. Do not treat those
two surfaces as interchangeable for a small sculpting correction. The UV-island diagnostic draws
and queries source LOD0 together, but supplies surface/UV queries rather than entry/exit thickness.
Explicit selected scene-mesh queries report their geometry source. Use a matching diagnostic or
verify the discrepancy is below the required accuracy before using an ordinary preview depth.

`gc_render` is the observer here. It does not write a brush stroke, supply a dense depth/normal
image, infer a desired 3D form from a picture, or perform continuous screen-space/geodesic brush
painting. The current authoring loop uses existing Shape operations; a new interactive brush
writer is not available. Ask the installed tool for help before choosing call syntax.

#### A repeatable sculpting or painting pass

1. **Start with one receiving part.** Keep its baseline recipe, named receiving region, physical
   scale and protected seats/borders. Record the intended silhouette, depth range and reference.
   Complete that part before repeating the method across the asset.
2. **Render a baseline.** Retain the capture/manifest and fixed front, side and grazing cameras.
   Use detail lighting to inspect geometry and lit views to inspect the final materials. Avoid
   auto-refitting comparisons when changing bounds would also change the apparent scale.
3. **Measure the target.** Ask the chosen surface pixel at depth 0 and a few footprint-edge
   samples. Inspect misses, frame, normals and fidelity. Use entry/exit depth only when a matching
   closed mesh supports it. A requested query depth locates a point inside existing geometry;
   it does not prescribe the next stroke's displacement.
4. **Author one bounded stroke.** Keep center, footprint size, frame/direction, signed depth or
   material channel, strength and falloff/profile in the editable recipe. Define its hard receiving
   region independently of its influence field. Use existing geometric/point/edge weight rules
   and ordinary displacement slope controls; add an authored texture only when it helps.
   The axis-slope extension can shape the two cross-sections independently, multiplying their
   influences; check its installed grammar/status before use. It is not a general freehand height
   field, and it moves existing surfaces rather than making overhangs.
5. **Carry coordinates into the correct recipe frame.** Preview coordinates are mesh-local;
   scene world coordinates require the receiving mesh's inverse placement. Account for build
   pivot and later recipe transforms. Preserve a deliberate `[pivot=(0,0,0)]` frame where useful,
   or record the mapping; do not paste a post-build point into an earlier construction frame.
6. **Build through `gc_apply`, then render fresh.** Rebuild from the baseline plus authored
   operations, and check the same views and actual depth/clearance. Change the stroke parameters
   when correcting that stroke. Appending another displacement accumulates travel; a bounded
   weight does not bound a sequence's total depth. Old pixels and island labels expire after
   geometry/UV changes, so re-query the new capture before placing the next correction.
7. **Repair and accept the saved result.** Check silhouette, local depth, seams, folds, protected
   regions, back-face/nearby-surface influence, material transitions and the actual production
   bake. Record construction time, triangle count and final runtime cost at the intended view.

An opacity/point field is influence, `by` is physical travel, and `slope` controls how that travel
varies within the selected region. Keep these controls separate. A local projector is not an
occlusion-aware brush: protect the intended surface so a stroke cannot reach another facing layer.

Prove the loop on one isolated shallow dish or raised crest and one painted patch before using it across an
asset. Place a correction from a render query, rebuild, inspect fixed front, side and grazing views, and
confirm that protected borders and depth limits hold. Also try an open surface and an overlapping UV island,
and expect a query on a stale capture to be rejected.

## 5. Bake permanent detail and budget the shared result

For static finish, prefer baking the authored layers into the supported material outputs. Group
compatible related parts into a shared atlas where that preserves texture density and suits reuse
and streaming. Shape's named bake target can collect several meshes into one atlas and generated
material. Keep surfaces requiring unsupported or dynamic material behavior separate as needed.

An atlas can consolidate material sections and reuse one texture set, but sharing it does not
automatically merge separate mesh draws or save memory. Check achieved density, packing/padding,
texture dimensions, material slots, mip behavior and actual runtime cost. Avoid a giant atlas that
must stay resident for one small visible part. Keep editable layers and sources for future changes.

Read [Prove the material or decal bake](ASSET-QUALITY.md#prove-the-material-or-decal-bake) before
delivery. Inspect the baked mesh under neutral and game lighting, close up, at grazing angles and
at the playing distance. Check source-to-SVG fidelity, holes, fitted edges, texture repetition,
alpha halos, UV seams and mip bleeding, normal direction and the final painted transitions.
Record evidence from the saved final asset; a successful generation, import or bake is only one
step toward a finished result.

## Common mistakes

- **Treating image brightness as height.** A generated or photographed image does not say how tall anything is.
  Author or measure the height values, and keep generated grayscale marked as a concept until the displaced form
  is accepted.
- **Converting a shaded image to SVG.** Shading becomes unwanted shapes. Use a flat, high-contrast source, a
  mask, or `--mode silhouette`, and give each part its own colour.
- **Hand-editing path coordinates.** Change the converter flags or the source and rerun. Edit handles locally only
  when one bend fails a radius check, and keep neighbouring endpoints and tangents.
- **Forcing a contour through primitives.** If a long run of shapes or computed coordinates is needed, use SVG.
- **Round radii larger than the narrowest feature.** Check web thickness and the smallest convex radius after every
  vector cleanup.
- **Stacking displacement passes and expecting a depth cap.** Each pass adds travel. Compose weights first and
  displace once.
- **Using texture-weighted `raise`.** It is not supported; use `displace` for image-driven relief.
- **Reusing an old capture.** Pixels and island labels expire after geometry or UV changes. Query the new capture.
- **Giving a decal or paint what depth must do.** Colour and normal-map marks do not change silhouette, openings
  or contact. Use geometry for those.
- **Baking before the receiving part is finished.** Finish one representative motif on its real part first.

## Reference

| You want | Method | Key spelling or tool |
|---|---|---|
| a contour from artwork | ImageGen art, then `img2svg.py`, then `outline('/Game/…', id=, width=)` | `--split`, `--names`, `--mode silhouette\|colors\|lines` |
| centre lines from line art | `img2svg.py --mode lines` (`--expand` for a filled outline) | `data-width-profile` on the path |
| soft organic volume | texture brush on a declared receiving region | `weightage()` with `rule(condition=texture, channel=)`, `displace(by=)` |
| an opacity brush | sample the alpha channel | `channel=a` |
| a grayscale height map | linear texture, constant slope | `channel=r`, `sRGB=false`, `displace(…, slope=profile(0=100%,1=100%))` |
| combined weight fields | `add` strengthens up to 100%, `merge` takes the maximum, `multiply` falls off | `weightage()` |
| painted marks and wear | decals and masked material painting | `decal()` with `at`, `project`, `size`, `turn` |
| a form judged by eye | render and brush loop | `gc_render` to observe, `displace` to sculpt, `gc_apply` to rebuild |
| a baked finish | named bake target, shared atlas | [ASSET-QUALITY.md](ASSET-QUALITY.md#prove-the-material-or-decal-bake) |

See also [GCSHAPE.md](GCSHAPE.md) for the full spelling list and [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md) for
edge treatment of the finished form.
