# Make good materials and tile them properly

*A GraphCode production guide for surface materials: choose the material's job, prepare a tiling
texture set, import it, build master materials and instances, map them onto geometry and prove the
result. Read after [ASSET-QUALITY.md](ASSET-QUALITY.md). Installed tool help owns exact grammar; a
fragment here is illustrative until `gc_describe` and a dry run agree with it.*

A good material reads as the right substance at the right size from every distance the player sees
it. Most material failures are not shader failures. Typical causes are a texture that does not
tile, a tile whose size nobody recorded, or a data map imported as colour. Others are UVs at a
different scale on every object, or one tile repeated across a wall until it reads as a grid. Work
through the steps in order and prove each before the next. A material that compiles is not yet a
finished material.

```text
1 decide the job -> 2 source and prepare the set -> 3 import -> 4 master + instances
  -> 5 map onto geometry -> 6 break repetition -> 7 verify, then expand
```

This guide owns the job choice, physical scale and texel density, tiling preparation, the
master/instance structure and the world-scale UV convention. Other guides own the neighbouring
topics; it links to them rather than repeating them:

- texture-set map roles: [CONTENT.md](CONTENT.md#a-texture-set-is-several-maps-for-one-surface);
- ImageGen swatches, decals and bakes: [ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md);
- wear and imperfection: [ASSET-QUALITY.md §9](ASSET-QUALITY.md#9-finish-materials-and-wear);
- landscape layers: [ENVIRONMENTS.md](ENVIRONMENTS.md#build-a-complete-pbr-landscape-surface);
- runtime repetition: [TEXTURE-VARIATION.md](TEXTURE-VARIATION.md), the canonical home.

## 1. Decide the material's job

| Job | Use it for | UVs it needs | Texture | GraphCode route |
|---|---|---|---|---|
| **Tiling material** | One substance at any size: walls, floors, planks, plaster, painted metal, ground | One physical scale on every mesh ([§5](#5-map-it-onto-geometry)) | A seamless tile of known physical size | One master material; one instance per surface |
| **Trim sheet** | Repeated linear detail: edge trims, mouldings, panel seams, pipes | Each face placed on its strip, repeating along the strip only | Horizontal strips, each tiling in U | An instance of a tiling master; the mesh's UVs choose the strip |
| **Unique bake** | Detail that belongs to one object: engraved relief, edge wear, contact blends, AO | A non-overlapping 0–1 layout (`uv(auto)` or an imported unwrap) | A per-asset atlas | Shape `bake()` targets ([ARTISTIC-SHAPES §5](ARTISTIC-SHAPES.md#5-bake-permanent-detail-and-budget-the-shared-result)) |
| **Decal** | One placed mark over a tiling base: stain, crack, leak, sign, repair | Projected | An isolated motif with a clean mask | Shape `decal()` painted into a bake, or a runtime Unreal decal ([ARTISTIC-SHAPES §4](ARTISTIC-SHAPES.md#4-paint-most-surface-detail-through-decals-and-masks)) |

**Default to a tiling material plus a unique layer.** In the common production pipeline, reusable
surfaces are Unreal master materials and instances shared across many assets. Bake only the layer
unique to one asset: its wear, contact blends and placed marks. Bake too much and every asset ends
up with its own atlas and nothing is reused. (Source audit: repository
`Docs/Research/blender-mcp-vs-graphcode-ai-authoring.md` §6.)
A hero prop may be a unique bake. A forty-metre wall should be a tiling material with a few decals.

### Record physical scale and texel density

Every tiling texture covers a real area. Record it as **tile size in centimetres**, per axis when
the tile is not square. Write it when the texture is chosen, and again after any crop, resample or
seamless pass changes it. Without it, nobody can set the instance's tiling or judge whether a brick
is the right size.

**Texel density** is texture pixels per metre of surface. A 2048-pixel tile covering 200 cm has
2048 / 2 = 1024 px/m. Choose the project's target density from the closest view the player gets,
then check it: render that view at the game's resolution. If individual texels or blur show, the
density is too low for that view. If the surface is never seen that close, a smaller texture or a
larger tile may serve. Keep neighbouring surfaces at similar density. A sharp floor beside a
blurred wall reads as a mistake even when each is acceptable alone.

Shape's `material('/Game/…', texel_density=…)` and `bake(…, texel_density=…)` set the **bake**
resolution in texels per metre. They do not change texture tiling
(`gc_describe kind:"shape" block:"surface"`).

### Choose which surfaces share one master

Group materials by what the shader must do, not by the substance's name. Opaque tiling surfaces
with the same channels and the same variation features can share one master: stone, plaster, wood,
painted or bare metal. A different blend mode or shading model needs its own master and its own
guide:

- masked foliage: [VEGETATION.md](VEGETATION.md);
- water: [WATER.md](WATER.md);
- hair and fur: [GROOM.md](GROOM.md);
- landscape layer blending: [ENVIRONMENTS.md](ENVIRONMENTS.md).

Translucent glass also needs its own master. Each static-switch combination that instances use can
need its own compiled shader variant, so keep only the combinations instances actually use
([budget the complete material](TEXTURE-VARIATION.md#budget-the-complete-material)).

## 2. Source and prepare the textures

A texture set is several registered maps for one surface, each imported and connected by its role
([CONTENT.md](CONTENT.md#a-texture-set-is-several-maps-for-one-surface)). For a tiling surface,
plan at least these maps:

- base colour;
- normal;
- roughness;
- metallic, where raw metal shows;
- height, where layers blend by height or the seam should follow crevices;
- AO, only where the material uses it.

| Source | Good for | Watch for |
|---|---|---|
| Library scan or material: Blendkit through Integrate, or the project's library | Real substances with every map and a declared physical size | Licence; the declared tile size; lighting baked into base colour; repetition over large areas ([Blendkit workflow](CONTENT.md#use-blendkit-generators-and-materials-through-integrate)) |
| ImageGen swatch | Original or stylized surfaces that belong to the project's theme | A colour image only: author or derive the data maps. A request for seamlessness is not proof ([ARTISTIC-SHAPES §2](ARTISTIC-SHAPES.md#2-use-imagegen-for-texture-art-and-svg-design-references)) |
| Photograph | One specific real surface | Perspective, vignetting, directional light and conspicuous features |

**Check the source before processing it.** It should be orthographic and evenly lit, with no cast
shadows or highlights. You should know its physical size. It needs enough pixels for the target
density. It should hold no single feature that a viewer would spot in every repeat. A tool can fix
edges and broad lighting; it cannot invent a better source. When the source is the problem, say so
and replace it.

### Make the set tile and prove it

GraphCode ships `make_seamless.py` in `Plugins/GraphCode/Tools/MakeSeamless`. It turns a texture
that does not tile into one that does: an ImageGen swatch, a photo crop or a library map. It
removes what would repeat, cuts the tile for a layout, and verifies the result at the distance a
player sees it. Every map of a set gets **identical cuts, donor patches and turns**, so the maps
stay registered. It runs outside the editor and needs no engine: Python 3.9+, NumPy and OpenCV 4.x,
with PyMaxflow optional. `--help` lists every option with its default; this section names the ones
each step uses.

```powershell
pip install -r Plugins/GraphCode/Tools/MakeSeamless/requirements.txt
py -3 Plugins/GraphCode/Tools/MakeSeamless/make_seamless.py T_Rock_BC.png T_Rock_N.png T_Rock_H.png T_Rock_R.png -o tiled/ --physical-size 200cm
py -3 Plugins/GraphCode/Tools/MakeSeamless/make_seamless.py T_Rock_BC.png T_Rock_N.png T_Rock_H.png T_Rock_R.png -o tiled/ --auto-remove 2 --variants 3 --layout rotate
py -3 Plugins/GraphCode/Tools/MakeSeamless/make_seamless.py swatch.png --check
```

Run the whole set in one call. The first map is the **guide**, the one the preview and report
analyse, so put base colour first. Use `python` in place of `py -3` on hosts without the Windows
launcher. Every run prints one line per map and a summary, and exits 1 when a written map still
has a seam. `--check` only measures and writes nothing. Run it on anything that only *claims* to
tile, generator output included: generator "seamless" tiles have measured 1.5–2.5 at their wrap,
where a seamless tile reads about 1.

The professional order is **prep → seamless → landmarks → variants and layout → verify at scale →
runtime breakup**. One call runs the first five; the sixth is the material's job
([§6](#6-break-repetition-at-runtime)).

1. **Prep: tone only, no geometry.** `--flatten auto`, the default, divides out large-scale
   luminance on colour maps: shading, vignetting, a lit centre. It uses a blur of a quarter of the
   shorter side and leaves normal, height and data maps alone. Edge equalization (default on;
   Moisan's periodic-plus-smooth decomposition) removes the low-frequency mismatch between opposite
   edges. It measures the jumps between 1/16-tile bands, so a mortar line along one edge is not
   smeared through the image. Both change large-scale tone on purpose; `--flatten off` and
   `--no-equalize` keep a deliberate gradient. The report's `prep` lists each step with before and
   after numbers.
2. **Make it seamless.** The default method, `overlap`, duplicates nothing. The right band (1/8 of
   the width) is laid over the left band and joined along a minimum-cost cut. Rows are joined the
   same way, with a cut that closes on itself across the wrap.
   - The cut is steered by the differences of every map in the set, feathered and tone-levelled
     across the band. `--seam graphcut` (Kwatra 2003) allows a cut of any shape. It needs
     PyMaxflow and falls back to the default path with a warning.
   - With a height map in the set (named `_H`, `_Height`, `_Disp`, or given as `--height FILE`),
     the cut prefers low ground and the taller side wins near it, so stones and bricks are not
     sliced at an arbitrary line.
   - A regular texture's overlap snaps to a whole number of its repeats, so bricks and stripes meet
     in phase. `--align auto` morphs offset structure (ripples, grain, cracks) together before the
     cut.
   - The tile is resampled back to the input size. That **magnifies the content by
     W / (W − overlap), about 1.14×** at the default band.

   | Method | Keeps | Duplicates | Use when |
   |---|---|---|---|
   | `overlap` (default) | all detail; size through resampling | nothing | almost always |
   | `offset` | size, pixel-exact | the middle half appears twice (warned) | the pixel size must not change |
   | `periodic` | size and every pixel | nothing | only colour or lighting differs across the edges |
3. **Remove landmarks.** A landmark is a patch unlike anything else in the tile: the bright pebble,
   the dark hollow, the stain a player finds again in every repeat. The report scores every 1/16-tile
   patch by its rarity and brightness: 1 is ordinary, ordinary textures top out at 2–4, and above 5
   is warned. It lists the top five with boxes. `<guide>.seamless.landmarks.png` numbers them on a
   heat map.
   - `--auto-remove N` replaces the N worst.
   - `--remove-mask FILE` replaces painted regions (white = replace, at the size of the inputs).

   Both work on the finished, wrapped tile. A donor patch whose surroundings match is joined by a
   graph cut through low ground, and bright or dark objects are covered whole. Every map gets the
   same donor and weights, and the overlay outlines replaced regions in green. For a large landmark,
   painting it out of the source and rerunning is still the cleanest fix.
4. **Choose the layout and make variants.** Pick how the tiles meet
   ([below](#choose-how-the-tiles-meet)) with `--layout grid|brick|half-drop|rotate` and
   `--offset`. Add `--variants 2–4` for tiles that share their border, so the material can mix
   them. Otherwise plan the variation at runtime ([§6](#6-break-repetition-at-runtime)).
5. **Verify at scale.** Judge repetition across many tiles, not nine.
   - `<guide>.seamless.preview.png` shows the tile before and after, tiled 2×2 and 3×3 in the
     layout. It adds the corner where four tiles meet at 1:1 and the landmark heat map. Margin ticks
     mark tile boundaries, and nothing is drawn over a seam. The 2×2 and the 1:1 corner show seams;
     the 3×3 shows repetition.
   - `<guide>.seamless.field.png` is 10×10 tiles at 200 px each, like a distant mip, drawn in the
     layout. With variants, `.variants.field.png` picks one at random per cell. **Judge repetition
     here.**
   - For rotate with a normal map, `.normal-field.png` lights the normals of the same cells from the
     upper left. A cell lit from another side than its neighbours means the normals were not turned.
   - The **grid read** (`verify.grid_read.mip16`) is the contrast of the pattern every cell shares at
     a 16 px mip. Above 0.08 is warned. It is a hint, not proof.

   **Look at the images.** A metric is a hint. Run every set, read the summary, then open the
   preview and the field. Pass the tile only if it has all of these: no seam in the 2×2 or at 1:1, no
   smear along a cut, no broken structure, and nothing distinctive repeating in the field. For
   rotate, also check for no mirror line along an edge and no pinwheel at a corner. Then check a
   large surface in the engine ([§7](#7-verify-like-a-professional)).

**Record the size the tool reports, not the source's.** Give `--physical-size` (`200cm`, `2m`, or
`200cm,150cm` per axis), and use the reported `physical_size.output_tile` as the instance's tile
size. `--no-keep-size` keeps the cut size instead of resampling.

**Carry the structure class into the material.** The report classifies the tile:

- *stochastic* (isotropic): gravel, soil or plaster can take random rotation and stochastic or hex
  tiling.
- *directional*: grain can take offsets but not rotation.
- *regular* (structured): bricks, planks and tiles keep their grid.

The report's `structure` block adds `stochastic_tiling_safe`, `random_rotation_safe` and
`offset_blend_safe`, and `next_steps` names the runtime techniques that are safe for this texture.
A seamless tile can still repeat visibly. The report tells you what the material must still hide.

#### Read what the tool wrote

| File | Written | What it is |
|---|---|---|
| `<name>.seamless.<ext>` | every input map | the tile, in the input's format, bit depth, channels and size |
| `<name>.seamless.v2.<ext>` … `v4` | `--variants N` | variants that share the border (or the rotate band) |
| `<name>.seamless.super.<ext>` | `--layout brick` or `half-drop` | the rectangle of whole rows that repeats on plain UVs; metrics are measured on it |
| `<guide>.seamless.preview.png`, `.field.png`, `.landmarks.png` | unless `--preview none` | the views in step 5 |
| `<guide>.seamless.variants.field.png`, `.normal-field.png` | with variants; rotate with a normal map | the field with a random variant per cell; the lit normal field |
| `<guide>.seamless.json` | always | the report |
| `<name>.check.preview.png`, `.check.field.png`, `.check.json` | `--check` | the same views and metrics for the input as it is |

The report carries sizes, `overlap_px`, `content_scale` and `physical_size`. It has `prep`, `seams`
(per cut: band, cost, `ratio`, `visibility`, overlap choice, alignment), `structure`, `landmarks`,
`removal`, `variants`, `verify` and `layout`. Per map it gives `before`/`after` metrics, `turns`
(rotate), `normal_length` and warnings, then `warnings` and `ok`. `--json` prints it instead of the
summary. The numbers to read:

- **`wrap_x`**: the mean difference between the first and last column, over the mean step between
  neighbouring columns inside. About 1 is natural. `wrap_x_rank` is the share of interior column
  pairs with a smaller step. A **seam** is `wrap > 1.15` *and* `rank ≥ 0.995`; interior pairs alone
  reach 1.2–1.45× their mean. `low_x` does the same for 16 px band means (tone): a tone seam is
  `low > 1.5` with rank ≥ 0.995. The same holds for y.
- **`turns`** (rotate): the same ratios for all 16 pairings of a tile turned *a* beside one turned
  *b*, with `worst_wrap`, `worst_turns` and `seams`.
- **Cut `visibility`**: the RMS difference on the cut over the RMS difference of pixels one blend
  width apart inside. Above 1.3 is warned: the cut joins things that never sit side by side.
- **`macro`**: the spread of heavily blurred luminance over its mean. Above 0.05, large blotches
  repeat visibly.

| The report or preview shows | Do this |
|---|---|
| `--check`: `NOT seamless` | run without `--check` |
| cut `visibility` above 1.3, structure broken along a line | `--seam graphcut`; `--overlap 0.2`; add the height map; `--align on` |
| bricks or stones sliced by the cut | give the height map; `--seam graphcut --height-route 3` |
| one landmark repeats in the field | `--auto-remove 2`, or paint a `--remove-mask`; then `--variants 3` |
| a grid read in the field, `macro` warned | `--variants 3`; a brick or half-drop layout; rotate when isotropic; runtime macro variation |
| `--layout rotate refused` | the texture is directional or regular: brick or half-drop along the grain |
| rotate: a rosette at corners, a line along every edge | read `layout.cuts` and `joins`; try variants instead |
| content scale 1.14 | restate the tile size from `physical_size.output_tile`, or `--no-keep-size` |

**Formats.** It reads and writes 8- and 16-bit PNG and TIF, JPG, TGA (uncompressed or RLE) and EXR.
Bit depth, channel count and alpha are preserved, and half EXR stays half. Normal maps are decoded to
vectors, blended, renormalised and re-encoded. Alpha is cut and resampled, never levelled. EXR needs
the OpenCV 4.x wheel, because 5.0 was built without OpenEXR. Output is deterministic: the same input
gives the same bytes.

**Limits.**

- Meandering ripples or grain that cross a cut can still break along it. `--align` and a wider
  overlap reduce that, and cut visibility reports it. Exact repeats are snapped in phase; irregular
  ones are not.
- Seamless is not repetition-free. Removal, variants and layouts reduce repetition, but broad
  blotches (`macro`) stay and need runtime macro variation.
- The landmark, macro, structure and grid-read figures are hints calibrated on a fixture set:
  real CC0 rock, gravel and sand scans, project wood and bronze, and generated bricks and stripes.
  The pictures decide.

### Choose how the tiles meet

Seamless only promises that edges join. The **layout** decides what the repeat looks like at the
game camera. On a plain grid, every copy of a stain lines up in rows and columns, and the eye
locks onto that. Shifting rows puts the repeats on diagonals, turning cells makes copies look
different, and variants make them actually different. Every layout has two halves: the tool
prepares a tile whose edges join *in that layout*, and the game lays it out *the same way*.
**The two must agree.** A tile cut for brick and laid on a plain grid shows a seam at every row,
and a grid tile turned per cell shows a seam on every turned side.

| Layout | What the eye sees | Texture side (tool) | Game side (Unreal) | Suits | Never for |
|---|---|---|---|---|---|
| **Grid** (default) | Repeats in rows and columns | default | plain UVs at `TileSizeCm` | regular textures, whose grid is their construction; anything the field preview passes | — |
| **Brick / half-drop** (diagonal) | Repeats on diagonals; the row/column read is gone | `--layout brick` (rows shift sideways) or `half-drop` (columns shift down), `--offset 0.5`, `1/3` or `1/4`. The cut pairs the shifted edges. | Import `<name>.seamless.super.<ext>`, the rectangle of whole rows that repeats on plain UVs. It is *q* tiles long for an offset *p/q*, so its scale along that axis is *q* × the tile size. Alternatively, shift the plain tile per row in the material. | most textures. For a directional one, shift **along** the grain: brick for grain along U, half-drop for grain along V. | regular textures, unless the offset is a whole number of their own units (bricks, planks) |
| **Rotate** (90° per cell) | Each copy turned; the same stain points four ways | `--layout rotate`: border strips and corners rebuilt so any side joins any side ([how](#how-the-rotate-tile-is-built)). The symmetric zones it reports must not read as a mirror line or pinwheel in the field. | Per cell: pick a quarter turn, turn the UV about the cell centre, **turn the normal map's XY the same way** the content turned, and sample with the turned derivatives (below) | isotropic textures only: gravel, rough rock, sand without ripples | directional or regular textures, or anything with baked lighting (shadows reveal the turn; de-light first) |
| **Variants** (with any layout above) | Copies actually differ | `--variants 2–4`: tiles that keep the base tile's border (1/16 of the tile) exactly and take their interior from the best-matching offset, so any variant fits beside any other; with rotate they share the rotate band instead | Per cell: pick a variant, from a texture array slice or by selecting among samples. An atlas needs padding between cells, or distant mips bleed. | landmarks that survive source repair | — |
| **Stochastic / hex tiling** (Heitz–Neyret, Mikkelsen) | No cells at all; blended, randomly offset and rotated copies. No band or corner patch shows, but structure blurs where copies blend | a plain seamless tile; no symmetric edges needed | runtime function ([TEXTURE-VARIATION.md](TEXTURE-VARIATION.md#choose-additional-techniques-by-the-visible-problem)); costs more samples | isotropic textures only | directional or regular textures |

#### How the rotate tile is built

With a quarter turn per cell, any side of a tile can meet any side of its neighbour. So every side
must carry the same band in its own frame, the side turned to the top.

- **Edges.** Across every edge line, the field then shows that band beside its own 180° turn. The
  result is one strip, point-symmetric about the edge midpoint, whatever the turns. The strip is
  natural content on one side of a point-symmetric minimum-cost cut and its 180° turn on the other.
  The cut stays off the edge line except at its midpoint, so the line runs through natural content.
- **Corners.** The four corner squares round every field corner form one patch with exact 4-fold
  symmetry: a patch's four quarter turns, split by a minimum-cost cut from the centre outwards.
- **Narrow zones.** The exact band is 1/32 of the tile per side, and the corner patch twice that. Each
  zone joins the interior through the usual height-routed cut within the next 1/16. The report gives
  `band_px`, `strip_px`, `corner_px`, `join_px` and `arm_px`.
- **Sources.** Strip and patch come from places that are level, far from landmarks and as busy as the
  rest. Four turns of a slope make a pyramid, and a calm strip outlines every cell.
- **Tone.** Colour and data maps give up half of their large-scale tone in the frame and regain it
  over 1/8 of the tile. Keeping all of it repeats a tone pattern along every edge and a rosette at
  every corner. Keeping none leaves a calm corridor round every cell. Normal maps keep theirs.
- **Normals.** Normal vectors turn with the image. The tool assumes DirectX (green down, Unreal's
  convention) unless the file name says GL (`NormalGL`, `_gl`, `OpenGL`).
- **Guards.** Textures the structure check reads as directional or regular are refused, and mildly
  oriented ones are warned. `--allow-structured` turns the refusal into a loud warning. The tile must
  be square.

Rotate shares one strip on every edge and one patch at every corner, so whatever they contain recurs
there. Its frame is narrow but not invisible at 1:1. A normal map with large facets shows its turns
under light, because a quarter turn changes how a facet faces the light. For strong macro relief,
prefer brick or half-drop.

**Choose in this order.**
1. Read the structure class from the report.
2. *Regular*: keep the grid, and vary the finish ([§6](#6-break-repetition-at-runtime)).
3. *Directional*: brick or half-drop along the grain, plus variants; never rotate.
4. *Stochastic*: start with brick or half-drop, which costs nothing at runtime with the super-tile.
   Add variants if landmarks still repeat. Rotate or hex only if the field preview still reads as a
   grid.
5. Make the tile with the tool in the chosen layout, and judge the field preview, which is drawn
   in that layout.
6. Lay it out the same way in the material, and check it in the engine at gameplay distance
   ([§7](#7-verify-like-a-professional)).

**Per-cell choices in the material.** Brick, rotate and variants all start from the same few
steps. This is a conceptual graph, **not GraphCode grammar**; build it as a material function
([§4](#share-logic-through-material-functions)) and verify every node name with `gc_describe`:

```text
uv     = world-scale coordinate / TileSizeCm             (tile units; §5)
row    = floor(uv.y);  uv.x += row * Offset              (brick; half-drop swaps x and y)
cell   = floor(uv);    local = frac(uv)
r      = hash(cell)                                      (one value per cell, shared by every map)
slice  = floor(r.x * VariantCount)                       (variants)
turn   = floor(r.y * 4)                                  (rotate)
local' = local turned `turn` quarter turns about (0.5, 0.5)
sample every map at cell + local', with ddx/ddy of uv turned the same way
normal.xy turned with the content; then the usual normal handling
```

- **Sample with explicit derivatives.** Without them, the jump in UV at each cell border picks the
  smallest mip and draws a thin line along every border. Check that the installed texture-sample
  node offers a derivative mip mode before relying on it.
- **Use one hash for every map** of the set, or the colour and normal fall out of step.
- **Check normal turning on an asymmetric bump:** under one light, every cell's bump must light
  the same way as its neighbours. The tool's lit normal field is the same check before import.
- The report's `layout.runtime` states the material side for the layout the tile was cut for.

**Wire the report into the instance:**
- `TileSizeCm` comes from the reported physical size, multiplied by *q* along the super-tile's long
  axis.
- The layout and offset are the ones the tool cut for.
- `VariantCount` is the number of variants written.
- Rotate is enabled only when the report marks the texture random-rotation safe.

## 3. Import settings that matter

Plan the import with `gc_import`, then read every texture's `settings{}` through `gc_read`
(`gc_describe kind:"texture_2d" block:"settings"`). That block lists only **deviations from the
engine default**, so a line that should not be there stands out. The spellings below are engine
values read from real sidecars. Ask `gc_describe type_path:"/Script/Engine.Texture2D"` for every other
setting name; GraphCode keeps no table of them.

| Map | Read back | Sample it as |
|---|---|---|
| Base colour | No `SRGB` line: sRGB stays on | Colour, the default sampler |
| Normal | `CompressionSettings = TC_Normalmap`, `SRGB = false`; green convention checked | `SamplerType=SAMPLERTYPE_Normal` |
| Roughness, metallic, AO, height, masks | `SRGB = false`, with a data compression such as `TC_Masks` or `TC_Grayscale` | A linear sampler that matches the compression, such as `SAMPLERTYPE_Masks` |
| Packed data (several masks in one image) | One data texture with `SRGB = false`; record which channel holds which map | Each channel by its role, as the source's packing specification states |

- **Normal convention.** Unreal uses the DirectX convention. Flip the green channel of an OpenGL
  source exactly once, at import or in the source, and never again in the material
  ([CONTENT.md](CONTENT.md#a-texture-set-is-several-maps-for-one-surface)). To find the texture's
  flip-green setting, read its reflected name through the `type_path` call above. A file name
  such as `NormalGL` is only a hint. Confirm the result under a directional light: raised features
  should catch light on the side facing the light.
- **Addressing.** A tiling texture must wrap. An `AddressX` or `AddressY` line such as `TA_Clamp` on
  a tiling map is a defect: the texture smears or seams at every tile edge.
- **Mips.** A tiling texture needs its mip chain. `MipGenSettings = TMGS_NoMipmaps` on a tiling map
  makes it shimmer and alias at distance.
- **Writing settings.** Change them with `gc_apply` on `settings{}`. Omission refuses rather than
  resets, so state `reset <Path>` to return one field to its default. The pixels are not in the
  document (`source_pixels` is on `omits:`). A new texture comes from `gc_import`; a reimport is
  `import{}` with a `source_hash` that matches the file on disk.
- **Proof.** Draw the imported texture with `gc_render kind:"thumbnail"` and compare it with the
  original. Follow CONTENT.md's notes on 16-bit encoding and capture fidelity before changing
  brightness in the material.

## 4. Build the master material and its instances

Follow [the normal edit loop](TOOLS.md#the-normal-edit-loop):

1. Write the `.gc` at the asset's sidecar path.
2. Run `gc_apply create:true` with `dry_run:true`. It returns the whole projected document.
3. Apply.
4. Read the result back from `live` and check the compile result.
5. Save, read `saved`, and render.

Before writing each block, ask `gc_describe kind:"material"` (or `material_instance`,
`material_function`) with `block:"<block>"` for its grammar.

### Build the master around physical size

This is a conceptual dataflow, **not GraphCode grammar**:

```text
mesh UV0 (or world position) x (project UV scale in cm / TileSizeCm) -> one shared coordinate
shared coordinate -> base colour, normal and data samples      (one coordinate for every map)
base colour x Tint;  roughness remapped to RoughnessMin..Max;  normal strength restrained
+ macro variation at its own scale, detail by distance         (TEXTURE-VARIATION.md)
-> MaterialResult: BaseColor, Metallic, Roughness, Normal, AO where used
```

**Feed every map of a set from the same coordinate**, so the maps stay registered. In GraphCode
spelling, the tiling maths is a few expression lines. The aliases, link names and texture path
below are illustrative; take yours from your own read:

```gc
graph {
  n0 = TextureCoordinate()
  n1 = Constant() [R=100.0]
  n2 = ScalarParameter() [DefaultValue=200.0, ParameterName=TileSizeCm]
  n3 = Divide(in0=n1.out0, in1=n2.out0) [link0.name="A", link1.name="B"]
  n4 = Multiply(in0=n0.out0, in1=n3.out0) [link0.name="A", link1.name="B"]
  n5 = TextureSampleParameter2D(in0=n4.out0) [ParameterName=BaseColorMap, Texture=obj'/Game/Materials/Surfaces/T_Plaster_BC.T_Plaster_BC', link0.name="Coordinates"]
  result = MaterialResult(BaseColor=n5.out0.rgb)
}
```

- `n1` is the project's UV scale in centimetres per UV unit ([§5](#5-map-it-onto-geometry)).
- A data map's sample adds `SamplerType=SAMPLERTYPE_Masks`; a normal map's adds
  `SamplerType=SAMPLERTYPE_Normal`.
- The grammar requires `link<N>.name` wherever a class names its inputs. Copy each name from an
  existing line for that class.
- In a material graph, **an alias you leave out deletes that expression**. Deleting a parameter
  expression also requires `allow_breaking_changes:true`.

### Expose the controls an instance needs

| Parameter | Instance type | Why |
|---|---|---|
| `BaseColorMap`, `NormalMap`, the data map(s) | `texture` | One master serves every surface of its family |
| `TileSizeCm` (per axis where needed) | `float` | The tile's physical size, from the source or the seamless report |
| `Tint` | `linear_color` | Small colour corrections without repainting the texture |
| `RoughnessMin`, `RoughnessMax` (or a scale) | `float` | Fit the measured roughness to the reference finish |
| `NormalStrength` | `float` | Restrain relief that reads too strong at grazing light |
| `MacroSizeCm`, `MacroStrength`, `DetailSizeCm`, `DetailStrength` | `float` | Repetition break-up at separate scales ([§6](#6-break-repetition-at-runtime)) |
| World-aligned, detail on or off | static switch in the master | Compiled options, chosen in the parent (see below) |

Name each parameter for what it means, with sizes in centimetres, and keep the names stable.
Instances bind their overrides to a parameter's identity. Before changing a master, list everything
that depends on it with `unreal_gc_search kind:"referencers" target:"<master path>"`.

### Share logic through material functions

Put sampling logic several masters need in a `kind:"material_function"` asset: the world-scale
coordinate, macro variation, a world-aligned path. Its writer keeps the `FunctionInput` and
`FunctionOutput` ids, so callers stay bound, and it updates loaded dependent materials. In a graph,
a function is a `MaterialFunctionCall` line that names the function's path. Engine functions can
be read too, with `gc_read source:"live"`. Read the function for its exact input names before
wiring it.

### Make one instance per surface

```gc
gc 1  kind:material_instance  asset:/Game/Materials/Surfaces/MI_Plaster_Warm
parent: /Game/Materials/Master/M_TilingSurface

parameters {
  BaseColorMap: texture = obj'/Game/Materials/Surfaces/T_Plaster_BC.T_Plaster_BC'
  TileSizeCm: float = 175.0
  Tint: linear_color = (R=1.0,G=0.96,B=0.9,A=1.0)
}
```

- The instance writer refuses static-switch and component-mask changes, because they select
  shader permutations. Choose those options in the parent, or make a separate parent.
- Dynamic material instances are runtime objects and get no sidecar. Values that change in play,
  such as wetness or damage, belong to a runtime system.
- Assign the instance in the static mesh's `materials{}` block, by slot name. A path in a Shape
  `material('/Game/…')` entry only fills the slot the first time the mesh is built.

### Keep values physically plausible

These are checks against sources the project already cites, not numbers to copy:

- **Base colour holds the surface's colour, not its lighting**
  ([Adobe PBR guide](https://www.adobe.com/learn/substance-3d-designer/web/the-pbr-guide-part-2)).
  View the material `unlit`: shading you can still see there is painted into base colour. Measure
  the mean and spread of a flat region. Compare them with the reference values in
  [Epic's PBR guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/physically-based-materials-in-unreal-engine?application_version=5.7)
  and the Adobe guide, and with a real reference of the substance, before trusting a library or
  generated map.
- **Metallic separates raw metal from everything else.** Paint, oxide, dirt and coatings over metal
  are non-metal. Change base colour and metallic together where they change (Adobe). Do not lower
  metallic across a whole surface to make it darker.
- **Roughness carries the finish.** Epic's guidance uses roughness variation for scuffs and smudges,
  and leaves Specular at its default for ordinary surfaces. Remap measured roughness towards the
  reference finish, but keep its per-pixel variation
  ([CONTENT.md](CONTENT.md#use-blendkit-generators-and-materials-through-integrate)).
- **Normal strength is restrained.** A normal map changes shading but never the silhouette or
  collision ([ASSET-QUALITY.md](ASSET-QUALITY.md#realistic-surfaces-carry-believable-imperfections)).
  Check it under grazing light.
- **Judge everything under the game's own lighting**, not only a preview scene.

## 5. Map it onto geometry

### Use one UV scale for the whole project

**Every mesh that wears a tiling material gets UV0 at the same physical scale, and each instance
states its own tile's real size.** Use Shape's projection default: 100 cm per UV unit. The master
then multiplies UVs by 100 / `TileSizeCm`. A 30 cm crate and a 3 m wall show bricks of the same
size, and any surface material can be swapped onto any mesh without re-unwrapping. Do not set each
mesh's `scale=` to one texture's size instead: the next material put on that mesh would be wrong.

### In Shape, write the projection explicitly and last

Ask `gc_describe kind:"shape" block:"uv"` for the current rules. The ones that decide tiling:

- `uv(box|cylinder|plane, scale=)` writes UV0 at `scale=` centimetres per tile, default 100. The
  scale is per axis on `box`, `(u,v)` on `plane`, and `(around,along)` on `cylinder`, whose U is
  corrected to measured arc length.
- **An entry that never says `uv()` gets the header's policy:** `keep` where no boolean ran, and
  `auto` where one did. `auto` packs the whole shape into one square. That is the layout a unique
  bake needs, not a tiling scale, so a carved wall left without `uv(box)` shows a different brick
  size from its neighbour.
- **Surface instructions go last.** A boolean after a `uv()` throws the projection away.
- `at=`, `facing=` and `up=` frame a projection, and a `[ ]` region gives a face family its own
  frame. `offset=(u,v)` shifts the pattern in whole or fractional tiles, for example to start a
  brick course at a floor line.
- `uv()` writes channel 0 only. Lightmap UVs are a build setting in the static mesh's own `.gc`.

These are instruction fragments for a suitable entry, not a complete recipe:

```gcshape
uv(box)
uv(cylinder, facing=+Z, up=+X)
uv(plane, facing=+Z, offset=(0,0.5)) [top]
```

Verify on the built mesh with `gc_render mode:"legend" by:"uv_island"`. It draws the UV atlas and
lists the islands and any overlap. Tiled coordinates stay unwrapped there, so a 300 cm wall at the
default scale spans three units.

### Imported meshes

An imported mesh's UV0 is whatever its source made. An automatic unwrap usually packs each object
into 0–1. That is right for a unique bake and wrong for a shared tiling material, because every
object then shows a different tile size. Check the UV island legend. Fix it by scaling the islands
to world units in the source before export, or sample world-aligned. If the mesh enters a Shape
recipe as a `mesh('/Game/…')` operand, check whether a `uv()` projection there gives the result
you need.

### Placed scale

UVs are measured in the mesh's own centimetres, so scaling an actor in the level scales its texture
with it. Shape's bake notes say the same of density: it is measured at asset scale and does not
follow component scale. Build tiling meshes at their real size, or sample world-aligned for surfaces
that must scale freely.

### When to sample in world space instead

Rocks, cliffs, terrain and organic sculpts rarely have UVs that tile cleanly. Give them world-aligned
or triplanar sampling in the material. Epic's [texturing functions](https://dev.epicgames.com/documentation/en-us/unreal-engine/texturing-material-functions-in-unreal-engine)
include `WorldAlignedTexture` and `WorldAlignedNormal`. Divide world position by the tile size in
centimetres. Weigh the costs before choosing it:

- each map needs several samples;
- the projection blend zones show;
- normals need the matching function;
- world-space mapping swims over a moving object.

[TEXTURE-VARIATION.md](TEXTURE-VARIATION.md#choose-additional-techniques-by-the-visible-problem)
covers these trade-offs. These are material approaches, not GraphCode verbs. The GraphCode work is
the material graph that calls the engine functions.

### Trim sheets

Lay the strips out at known heights in centimetres. Place each face's UVs on its strip and let the
face run along U. In Shape, check whether a projection with `scale=` and `offset=` on a selected
region lands the face on its strip, and confirm it in the UV island legend. Otherwise, unwrap the
mesh in a DCC tool and import it. Keep each strip at the texel density of the tiling surfaces
beside it.

## 6. Break repetition at runtime

One seamless tile repeated is still a grid. [TEXTURE-VARIATION.md](TEXTURE-VARIATION.md) is the
canonical home for the techniques, their checks and their cost. In short:

| Technique | Use it when | Note |
|---|---|---|
| Macro variation at a non-matching scale | Broad light/dark uniformity across a large surface | Choose a macro size that is not a whole multiple of the tile, so the combined pattern does not repeat at a short period. Drive it by exposure, moisture or wear. |
| Detail texture by distance | Good near, grid-like far | Near and far samples blended by camera depth ([three-part technique](TEXTURE-VARIATION.md#start-with-the-three-part-technique)). |
| Layer, vertex and contact blending | Where surfaces meet or weather differs | Shape `weightage` with `rule(condition=contact, …)`, then `decal()`, paints a blend into the bake around a contact (`gc_describe kind:"shape" block:"weightage"`). Shape `colour()` writes the same weights, or one value per tile, as vertex colour ([below](#vary-each-piece-with-a-shape-vertex-colour)). Otherwise use runtime vertex paint or landscape layers. |
| Decals | Unique features and local events | A few placed marks beat dirt everywhere ([ASSET-QUALITY §9](ASSET-QUALITY.md#9-finish-materials-and-wear)). |
| Stochastic or hex tiling, random rotation | Isotropic textures only | Follow the seamless report's structure class or `stochastic_tiling_safe`. Never use it on bricks, planks or weaves. |
| Brick or half-drop layout, quarter turns, two to four edge-sharing variants | A grid read, or landmarks that still repeat after source repair | The tool cuts the tile for the layout (`--layout`, `--variants`), and the material must lay it out the same way ([choose how the tiles meet](#choose-how-the-tiles-meet)). |

Add the techniques one at a time, look again after each, and measure cost with each enabled
([budget](TEXTURE-VARIATION.md#budget-the-complete-material)).

### Vary each piece with a Shape vertex colour

World-space noise gives every tile of a roof one shared wash, and the wash slides when the house is
moved or placed twice. A per-piece value belongs to the mesh. In a `.gcshape`, `colour()` writes the
vertex colour after the pieces are added: `random` with `seed=` gives each placed copy its own value,
and a weighted selection gives an area with a soft edge (`gc_describe kind:"shape" block:"colour"`).
Keep one job per channel, for example R for the per-tile tint, G for grime and A for wetness.

In the master material, a `Vertex Color` node reads the number the line wrote, to 1/255:

- **Tint per tile:** `Lerp(TintLow, TintHigh, VertexColor.R)` multiplied into Base Color. Expose the
  two tints and keep their difference small (a few percent of value and hue), so the roof reads as
  one material with variation rather than a patchwork.
- **Grime by area:** `Lerp(GrimeColour, BaseColour, VertexColor.G)` and raise roughness where G is low.
- An unwritten channel reads 1, so a material that multiplies by it is unchanged on meshes without colour.

Vertex colour is interpolated between vertices, so a fade needs vertices to vary at. A weighted
`colour()` refines the faces as `displace` does; `per=copy` writes one value per piece and adds none.
Use a baked `decal()` instead where the variation must be finer than the mesh.

Limits to expect: write `colour()` after any `remesh` or `join=blend`, which drop vertex colour;
inserting a line above the `add` that places the copies redraws their `random` values (growing
`repeat=` does not); and Nanite has not been confirmed to keep the colour.

## 7. Verify like a professional

Look at the material where and how the player will:

- under neutral light and under the game's lighting;
- at the closest camera, at a grazing angle, and at the player's usual distance;
- on a large surface at gameplay distance;
- in motion.

The large surface should be shown at the size the game shows it, from the player's eye height, at
the game's resolution, so the real mip levels are in use. A close crop never exercises the small
mips, which is where a grid appears.

| Question | Evidence |
|---|---|
| Is lighting painted into base colour? | `gc_render` with `view_mode:"unlit"` |
| Does the relief come from the normal map, and is it the right way up? | `view_mode:"detail_lighting"`, with a low camera `elevation` for grazing light |
| Did the import match the original image? | `gc_render kind:"thumbnail"` on the texture, beside the source |
| Are the UVs at one scale, without unintended overlap? | `gc_render mode:"legend" by:"uv_island"` on the static mesh |
| Does the surface read as a grid at distance? | The open level from the player's position, at game resolution; `source:"viewport"` during play for streamed mips |

**Checklist before reusing the material:**

- the tile size in centimetres is recorded, and `TileSizeCm` matches the seamless report;
- texel density is checked at the closest view and matches the neighbouring surfaces;
- each map's settings are read back: sRGB, compression, addressing, mips;
- every map samples one shared coordinate;
- the UV legend shows one scale, and placed scale is 1 or the material samples world-aligned;
- no seam shows in the 2×2, at the 1:1 corner, or on the mesh;
- no grid or landmark repeat shows at gameplay distance;
- `unlit` shows no painted lighting, and `detail_lighting` shows the relief the right way up;
- the material passes under game lighting, at a grazing angle and in motion;
- the cost is measured with every variation technique enabled;
- the project's material note is written up (below).

| You see | Likely cause | Do |
|---|---|---|
| A line at every tile edge | The texture does not tile, or addressing is clamped | `--check` the texture; make it seamless; remove any `AddressX`/`AddressY` clamp |
| A line only where faces meet | A UV island or projection boundary | Re-frame the projection, or add a region with its own frame; check the UV legend |
| A grid at gameplay distance | Broad blotches or landmarks repeating | Field preview; macro variation; fix the source; variants ([§6](#6-break-repetition-at-runtime)) |
| The same rock or stain in every tile | A landmark in the source | `--auto-remove`, a `--remove-mask`, or paint it out of the source; then variants or decals |
| Texture stretched or smeared on some faces | The wrong projection for those faces | Use `box` or a per-region frame; on rock or cliffs, sample world-aligned |
| Bricks the wrong size, or a different size on each object | UVs not at the project scale (`auto`, `keep` or a 0–1 import), a stale `TileSizeCm`, or a scaled actor | Project `uv()` explicitly; restate `TileSizeCm` from the report; place at scale 1 or sample world-aligned |
| Too uniform, clean or CG-looking | No macro or roughness variation, and no wear driven by use | Macro variation; roughness variation; wear where use and weather would put it ([ASSET-QUALITY §9](ASSET-QUALITY.md#9-finish-materials-and-wear)) |
| Bumps read as dents, or lit from the wrong side | The normal map's green channel is inverted | Flip green once, at import; confirm with `detail_lighting` under a directional light |
| Flat or washed-out normals; milky roughness or metal | A data map imported or sampled as colour | `SRGB = false`; the right compression and sampler type |
| Shadows that do not move with the light | Lighting painted into base colour | De-light the source; check the `unlit` view |
| Shimmer or sparkle in motion at distance | Missing mips, or detail normal too strong | Restore the mip chain; reduce detail strength or blend detail out by distance |
| A grey checker on the mesh | An empty material slot, not a UV fault | Assign the slot in the static mesh's `materials{}` block |
| A band where near and far detail meet | The blend range or a scale mismatch | [TEXTURE-VARIATION acceptance](TEXTURE-VARIATION.md#acceptance-before-expanding-production) |

**Keep the record with the project**, in its material note or [asset log](../Seeds/ASSET-LOG.md):
the source and licence, the physical tile size after preparation, the texel density, each map's
import settings, the master and instance paths, the structure class, the runtime variation chosen,
and the captures that passed.

Prove one representative surface in the actual level before expanding to the whole set.
