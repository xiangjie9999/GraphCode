# Flipbook textures from image generation

A flipbook (a sub-UV sheet) is one texture that holds an effect's frames in a grid; a Niagara
sprite renderer plays them in order. This page covers how to make one with an image generator,
clean it into a sheet Unreal plays, and wire it into a system. The generator can be ImageGen or an
authorized API route ([UI-TOOLS.md](UI-TOOLS.md#api-based-image-generation-is-a-supported-route)).
Read [EFFECTS.md](EFFECTS.md) first.
Its rules hold here, and the effect is judged in motion, never on one frame.

**A flipbook is one layer, not the effect.** A fireball is a looping ball; an explosion is a one-shot
with a start and an end; a fireball spell stacks the cast at the hand, the looping ball, the explosion
where it hits and the fallout, each timed and each on its own emitter. Plan the stack before the first
sheet ([EFFECTS.md](EFFECTS.md#an-effect-is-a-stack-of-timed-layers)), and follow that page's order of
work every time ([EFFECTS.md](EFFECTS.md#the-order-of-work-for-every-effect)): a sheet is step 3 of it.

A generated sheet is a rough draft, not the finished texture. Its grid is uneven, its frames drift and it has
no usable alpha. Steps 3 and 4 below are not optional cleanup; they are what makes it a flipbook.

## Choose the route by the effect

| The effect | Route | Why |
|---|---|---|
| a loop: a fireball (a constant glowing ball), a flame, steam, a glow, a slow smoke column | a 4×4 sheet or one still, with the motion supplied by the material (panning noise, erosion, distortion) and Sub UV blending | few frames hide the drift between generated frames |
| a one-shot whose shape changes: explosion, impact puff, splash, muzzle flash | **one generation for the whole sheet**, 4×4 to 8×8 | the generator sees every frame at once, so colour and style hold; frames generated one at a time drift |
| a long or detailed sequence | a short clip from a video model, frames extracted and packed | the most coherent motion; you still make the alpha and the loop |
| sparks, embers, debris | no flipbook: particles with a small still sprite | the motion comes from the simulation |

**An effect with both light and smoke is two sheets on two emitters,** for example an explosion's fire
and the soot it leaves. The bright part is additive; the smoke is translucent. One sheet cannot be both,
because additive blending cannot darken anything.

## 1. Fix the sheet before you generate

- **The grid is exact.** Columns × rows = frame count: 4×4 is 16, 8×8 is 64. Make the texture
  square and a power of two (1024 or 2048). Unreal makes mips only for power-of-two textures, and
  a power-of-two grid keeps every cell whole pixels at every mip. A 6×6 grid does not divide 2048.
- **One frame is the texture divided by the columns:** 2048 at 8×8 is 256 px a frame. Size it to how
  large the effect is on screen, not larger.
- **Frames run row by row from the top left,** 0 to N−1.
- **Decide how it plays:** once over the particle's life (a one-shot) or looping at a frame rate
  (a flame). That decides whether step 3 makes a loop and how step 5 sets the SubUV module.
- **Every frame stays inside its cell with a margin.** Anything touching a cell edge bleeds into the
  next frame as the mips average.

## 2. Ask for something you can key

- **Generate on flat, pure black.** Never ask for "transparent": generators paint a checkerboard or
  return an alpha that is not the effect's density.
- **An additive sheet** (fire, glow, sparks, magic) uses the black plate as it is. Black is invisible
  when added, and brightness is visibility. It cannot hold dark smoke: that goes on its own sheet.
- **A translucent sheet** (smoke, dust, soot): ask for grayscale density, white smoke on black under
  even neutral light, with no shadow cast on the background. Step 3 turns the gray into alpha. Colour and
  lighting come from the material.
- **For a whole sheet,** state the grid, the frame count and the order. Each frame is centred in its
  cell with empty margin, under the same camera and scale, showing one event from start to end, with no
  labels, borders or grid lines. A route that edits an image can take a black template with the
  cells marked, which keeps them even. Step 3 crops inside the marks.
- Generate at the largest square size the route offers, and **look at the result** before step 3.
  A wrong frame count or a frame drawn in another style means generate again. After two failed
  tries, drop to a smaller grid (8×8 to 4×4); fewer cells come back right more often.

## 3. Repack onto an exact grid, in code

A generated grid is never exact: cells differ in size, a frame crosses a border, the count is off.
Write a short script (Python with Pillow is enough) and keep it beside the sheet:

1. **Measure the background's black and subtract it.** A generator's black is rarely zero. Left in,
   it adds a faint haze square over every additive sprite.
2. **Find the frames the generator actually drew.** Split on the dark gaps (the row and column sums
   of brightness), not on an even division of the image. Count the frames.
   - **Put each border on the grid line you asked for when that line falls inside the gap,** not at the
     gap's middle. The middle moves as the effect grows or rises, and the frames then jump in playback.
   - **Faint frames break into pieces** (an explosion's last embers): join pieces that sit closer
     together than the frames do, and take a faint row's borders from the whole sheet.
   - **No clear gaps** (bright fire often has none): divide evenly by the grid you asked for (or the
     template's marks) and check every crop by eye.
3. **Crop every frame with one box of the same size, anchored at the same point of the effect** (an
   explosion's base, the smoke's source). Never fit each frame to its own bounds. That erases the
   growth and drift that are the animation.
4. **Scale every frame by one factor,** so the largest fits its cell inside the margin. The
   generator's own size is the ceiling on detail. A 1254 px sheet of 4×4 holds about 300 px a frame,
   so pack it to 1024 (256 px frames); packing it to 2048 only upscales, at four times the memory.
5. **Paste into a new sheet** of exactly the planned grid and size, black everywhere else.
6. **Fade each frame to black** over the last few pixels before its cell edge.
7. **Write the channels the blend mode needs, as a PNG:**
   - additive: RGB as it is. No alpha is needed.
   - translucent: RGBA, with **A = the gray's brightness** and RGB set to white. The material tints
     it. Do not premultiply. A premultiplied sheet is only right when its material divides RGB by
     alpha or blends as premultiplied, so mixing the two darkens every edge.
8. **A loop only:** the loop comes out K frames shorter than the generated run. Blend the last K
   frames into the first K and drop the last K: of N frames, the loop plays N − K. Output frame i
   (for i < K) is a mix of frame N − K + i and frame i, moving from the first toward the second as
   i grows. The last played frame, N − K − 1, then flows into a frame 0 that is mostly frame N − K,
   its true successor. Start with K = 2 to 4 on 16 frames, watch it, and stop once the jump is gone.
   Set the SubUV module's last frame to N − K − 1 and leave the spare cells black, or ask the
   generator for K more frames than the loop needs. A last frame that copies the first one
   stutters. The crossfade hides the wrap only for content that cycles (a flame's flicker). An effect
   that drifts one way across the frames, like a rising column, still jumps: let the material supply
   that drift instead. A one-shot needs none of this.
9. **Play it before importing.** Write the frames out as an animated GIF at the effect's speed and
   watch it several times (loop it if it loops). Flicker, a jump at the loop, a frame from another
   hand: fix them here.

Keep the original generation, its prompt and the script with the sheet
([ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md) on provenance).

## 4. Import it

Plan the import with `gc_import`, then import. The plan's `settings { }` lists only what differs from
the engine's defaults, so an empty one means sRGB on and mips kept, which is right for a colour sheet.
To change a setting, ask `gc_describe kind:"texture_2d"` for its name and add the line to the plan.

- sRGB **on** for a colour sheet. Turn it **off** for a sheet whose only meaning is density, and for any
  packed data.
- **Keep the mips;** the margins from steps 1 and 3 protect them. No mipmaps is a deliberate fix
  for blur, not a default: it shimmers in motion. If the first plays look blurry and then sharpen,
  the texture is streaming in. Stop the sheet from streaming rather than dropping its mips: a
  settings-only write of `NeverStream = true` (no reimport; the dry run shows that one change). A
  short effect needs it: a 0.65 s explosion was over before its sheet arrived, and every render
  taken just after an editor restart showed soft blobs where the fire's detail should be. Set it on
  the motion-vector sheet too.
- A motion-vector texture is uncompressed with sRGB off.

## 5. Wire the emitter

Do these in order. The spellings below were accepted by the tool on UE 5.7 while a 4×4 explosion was
built this way. If the tool's help disagrees, the help is right.

1. **The material.** A particle material is not covered elsewhere in these guides, so here it is.
   Create it with `gc_apply create:true`:
   - settings: `BlendMode = BLEND_Additive` (fire; `BLEND_Translucent` for smoke),
     `ShadingModel = MSM_Unlit`, `bUsedWithNiagaraSprites = true`;
   - graph: `ParticleSubUV() [Texture=obj'/Game/…']`, multiplied by `ParticleColor()`'s `.rgb` and by
     a `ScalarParameter` glow, into `MaterialResult(EmissiveColor=…)`. Smoke also wires the sheet's alpha
     times the particle's alpha into `Opacity`.
   The Particle SubUV node reads the sprite's frame and blends by default. A sprite that shows the
   default material, or one frame that never changes, is missing one of these.
   **Fire with blending needs a colour ramp.** A blend halfway between a yellow frame and black is
   half-bright yellow, which reads as olive. Real fire dims through red. Colour the sheet in the
   material instead of passing its RGB through: temperature is the sample's luminance and runs deep
   red → red-orange → orange → pale yellow. Brightness is its brightest channel. **Measure the ramp
   from the sheet, do not pick it:** per luminance bin, the median of each lit pixel's rgb divided by
   its brightest channel, in linear values, written as a `lerp` chain in a `Custom` node (the
   explosion needed 8 knots). Anchor both ends on the outermost 1% of brightness: a knot at the top
   bin's median leaves the rare hottest pixels the bin's colour, and the explosion's white-hot core came
   out cream. A hand-picked three-`lerp` ramp drifted 10° in hue on the explosion:
   ochre embers, a pale peak. Every dim or blended pixel then comes out red-orange (the explosion:
   olive before, fire after).
   **Additive fire is see-through: give a dense fire body opacity.** Pure additive only adds light, so
   every hard edge behind the flames (a wall's lit face against its shadowed one, a wall against the sky)
   reads straight through them as a line, and the fire looks pasted on. Hiding the wall proved it on the
   explosion: the line moved to the next edge, 9 m behind. Use premultiplied alpha instead:
   `BlendMode = BLEND_AlphaComposite`, EmissiveColor as before, and Opacity = saturate(brightness ×
   a `Solidity` parameter, about 1.5) × the particle colour's strength and alpha, so the hot body hides
   what is behind it, the wisps stay see-through, a fading particle fades, and a trail of many
   particles does not turn into dark blobs. At Solidity 0 it is plain additive again. A small hot core
   that never shows the scene through it can stay additive.
   **Soften every sprite where it meets geometry:** multiply the output (emissive, or opacity for
   smoke) by `DepthFade(in1=<distance>.out0) [link1.name="FadeDistance"]`, about 60 cm for large
   sprites and a fraction of the sprite's own size for sparks and embers, so small ones near the floor
   are not eaten.
2. **Write the emitter out in full,** with all five child blocks (EFFECTS.md). Do not start from
   `[base=…]` on Niagara's SubUV example: that template is copied and then replaced by the stacks you
   write, so it brings nothing, and its module values leak into lines you reuse. The explosion's stacks:
   - `emitter_update`: ``EmitterState(`Life Cycle Mode`=Self, `Loop Duration`=2.0)`` and
     `SpawnBurst_Instantaneous(Age=link(Emitter.LoopedAge))`: one particle every 2 s.
   - `particle_spawn`: InitializeParticle `V2` by path, with its lifetime: `Lifetime`=0.3 alone for
     one fixed life (the default mode is Direct Set), or `` `Lifetime Mode`=Random `` with
     `` `Lifetime Min` `` and `` `Lifetime Max` `` for a range. **Min and max without the mode line
     are ignored,** and every particle lives the default 1 s. ~~A lone `Lifetime` does nothing, because
     the default lifetime mode is Random.~~ *(Superseded 2026-11-11: measured the other way round. The
     explosion below only played right because its min and max were both the default 1.0.)* Add
     `` `Sprite Size Mode`=Uniform `` and `` `Uniform Sprite Size` `` in cm; for a size range,
     `` `Sprite Size Mode`=`Random Uniform` `` with `` `Uniform Sprite Size Min` `` and `Max`.
   - `particle_update`: `ParticleState()`, then the SubUV module by its path,
     `/Niagara/Modules/Update/SubUV/V2/SubUVAnimation.SubUVAnimation`. Two engine modules share the
     short name, and an ambiguous short name is refused.
3. **Set the SubUV module to the sheet.** A read shows only the inputs that differ from their
   defaults, so the useful ones are named here: `SubUV Animation Mode` (Linear by default; also
   Random, Infinite Loop, Direct Index, Curve), `UseStartFrame` with `Start Frame Range Override`,
   `UseEndFrame` with `End Frame Range Override`, `Playback Mode`, `Frames Per Second`, `Play Rate`.
   No tool lists a module's inputs yet. Until one does, a write with a deliberately wrong input name is
   refused with the full list. For a one-shot: ``UseStartFrame=true, UseEndFrame=true, `End Frame Range Override`=15``
   (columns × rows − 1). Linear then plays frame (N − 1) × the particle's normalized age, measured at
   15 × age over a 1 s life. To skip a one-shot's first frames (an explosion's tiny opening star,
   when a separate flash layer already plays that beat), raise `` `Start Frame Range Override` ``.
   **A loop at a frame rate:** ``SubUVAnimation(`SubUV Animation Mode`=`Infinite Loop`, `Playback Mode`=`Frames Per Second`, `Frames Per Second`=18, `Random Start Frame`=true)``.
   One looping sprite shows its wrap and its pattern. Overlap two or three instead: a spawn rate of
   about 3 a second, a life under a second, random rotation, a random start frame, and each one
   faded in and out (the alpha curve below, or the RGB curves for an additive material that ignores
   alpha). On the fireball's core this gave a steady ball in stills with no wrap crossfade made
   (K = 0); it has not been judged in motion yet.
4. **Point the renderer at the sheet:** `sprite(Material=obj'/Game/…', SubImageSize=(X=4.0,Y=4.0))`,
   where X is columns and Y is rows. "Sub UV Blending Enabled" (`bSubImageBlend`) is already on by
   default, so leave it out.
5. **Smoke, the translucent layer** (proved on a 4×4 smoke puff, UE 5.7). The sheet is RGBA with
   density in alpha: import it with `SRGB = false` and check the alpha survived
   (`CompressionSettings = TC_BC7` kept it). The material's graph:
   **Make it a lit material.** Unlit smoke with its tint in `EmissiveColor` lights itself: it glowed
   pale grey in a dark scene. Lit, it takes the scene's light, warm beside the fire and grey after.
   Settings: `BlendMode = BLEND_Translucent`, `bUsedWithNiagaraSprites = true`,
   `TranslucencyLightingMode = TLM_VolumetricDirectional`, and no `ShadingModel` line (default lit).
   ```gc
   n0 = ParticleSubUV() [SamplerType=SAMPLERTYPE_LinearColor, Texture=obj'/Game/…/T_SmokePuff.T_SmokePuff']
   n1 = ParticleColor()
   n2 = Multiply(in0=n0.out4, in1=n1.out0.a)
   n5 = Multiply(in0=n0.out0, in1=n1.out0.rgb)
   n6 = Constant()
   n7 = Constant() [R=1.0]
   result = MaterialResult(Opacity=n2.out0, BaseColor=n5.out0, Specular=n6.out0, Roughness=n7.out0)
   ```
   A result pin takes a node's output, never a literal: `Roughness=1.0` is refused, so a constant is
   its own node. Changing an unlit material to lit needs `ShadingModel = MSM_DefaultLit` stated once:
   under the default patch a settings line you leave out is kept, not reset.
   The particle's colour is the smoke's albedo (mid grey, about 0.3) and its alpha the density. On
   the renderer add `SortOrderHint=-1` so the smoke draws behind the fire. Fade it over its life
   with ``module ScaleColor(`Linear Color Curve`=di(AlphaCurve.Keys=((),(Time=0.05,Value=1.0),(Time=0.65,Value=0.9),(Time=1.0)), …), `Scale Mode`=2)``
   ([EFFECTS.md](EFFECTS.md) has the full line). The sheet's last frames already thin out, so a
   curve that also falls early removes the smoke long before its life ends: hold it near 1 for the
   first two thirds. Judge its tone in a scene with a floor and a light, not on a black preview.
6. **Cutout only pays for many particles.** It builds tighter geometry from the sheet's alpha. A sheet
   with no alpha needs `OpacitySourceMode=OSM_ColorBrightness`, and its threshold can clip faint
   embers. On one large sprite every 2 s the saving is negligible.

## Fewer frames: blending and motion vectors

- **Sub UV blending** crossfades neighbouring frames. It suits soft effects; on fast shapes it ghosts,
  and on fire it turns in-betweens olive unless the material colours by a ramp (step 5.1).
- **Motion vectors** warp each frame toward the next, so fewer frames read as smooth. One artist
  went from an 11×11 sheet to 6×6 this way. The material function is
  `/Engine/Functions/Engine_MaterialFunctions02/Texturing/FlipBook_MotionVectors`. Give it a square
  grid, because uneven grids such as 4×2 are reported to break it. Vectors come from a simulation tool
  or from optical flow run on the frames. ~~Optical flow on generated frames is untried: it can work only
  where consecutive frames overlap and agree.~~ *(Superseded 2026-11-11: it worked on the generated
  explosion, below.)* Try it on your sheet, and keep it only if it holds up in motion.
- **On a Niagara sprite, motion vectors from optical flow** (proved on the 4×4 explosion, UE 5.7):
  1. **Vectors:** a short script (the fixture's is repository `SourceArt/FireballFlipbook/motion_vectors.py`)
     runs DIS optical flow (OpenCV) on each pair's linear brightness both ways and solves the *midpoint* flow, the motion through each pixel
     halfway between the frames, so one vector serves both warps. Cell N holds N → N+1, in the sheet's
     own layout: R = U, G = V (down), 128 = zero, ±0.25 cell over the codes, each vector kept inside
     its cell. 512 px is enough for a 1024 sheet.
  2. **Import** with `gc_import`; its plan's `settings { }` gets `CompressionSettings = TC_VectorDisplacementmap`
     (uncompressed 8-bit) and `SRGB = false`.
  3. **Material:** keep the renderer's Sub UV blending on; it fills UV1 and Blend. These lines replace
     the `ParticleSubUV` node, and the colour ramp then reads `n6`:
     ```gc
     n0 = ParticleSubUVProperties()
     n1 = TextureSample(in0=n0.out0) [SamplerType=SAMPLERTYPE_LinearColor, Texture=obj'/Game/…/T_FireballFlipbook_MV.T_FireballFlipbook_MV', link0.name="Coordinates"]
     n2 = ScalarParameter() [DefaultValue=1.0, ParameterName=MotionVectorStrength]
     n3 = Custom(in0=n0.out0, in1=n0.out1, in2=n0.out2, in3=n1.out0.rg, in4=n2.out0) [Code="float2 w = (MV * 255.0 - 128.0) * (0.25 / 127.0 / 4.0) * Strength; float e = 0.5 / 1024.0; float2 c0 = floor(UV0 * 4.0) * 0.25; float2 c1 = floor(UV1 * 4.0) * 0.25; float2 a = clamp(UV0 - Blend * w, c0 + e, c0 + 0.25 - e); float2 b = clamp(UV1 + (1.0 - Blend) * w, c1 + e, c1 + 0.25 - e); return float4(a, b);", Inputs=((InputName=UV0),(InputName=UV1),(InputName=Blend),(InputName=MV),(InputName=Strength)), OutputType=CMOT_Float4, link0.name="UV0", link1.name="UV1", link2.name="Blend", link3.name="MV", link4.name="Strength"]
     n4 = TextureSample(in0=n3.out0.rg) [Texture=obj'/Game/…/T_FireballFlipbook.T_FireballFlipbook', link0.name="Coordinates"]
     n5 = TextureSample(in0=n3.out0.ba) [Texture=obj'/Game/…/T_FireballFlipbook.T_FireballFlipbook', link0.name="Coordinates"]
     n6 = LinearInterpolate(in0=n4.out0, in1=n5.out0, in2=n0.out2) [link0.name="A", link1.name="B", link2.name="Alpha"]
     ```
     `ParticleSubUVProperties` gives UV0 (frame N), UV1 (frame N+1) and Blend. Frame N moves against the
     vector by Blend and frame N+1 along it by 1 − Blend. Both read the vector once, from cell N.
     4 is the grid and 1024 the colour sheet's size. The clamps keep each sample in its own cell.
  4. **Measured:** in the render, light outside the in-between silhouette over 0.03–0.17 s fell from
     6.3% to 1.2% (the exact frames' own halo is 0.3–1.5%), and the largest growth step fell from 4.0 to
     1.2 times the median. Exact frames are unchanged. A sharpened blend weight (smoothstep) changed
     nothing at 30 fps: with 15 frames over 1 s, Blend is 0 or 0.5 at every sample.

## Before you call it done

- The GIF from step 3 plays cleanly, and the frame count matches the grid and the module's range.
- Render the system at the start, the peak and the tail (`gc_render` with `at:`), then play it in the
  game's lighting ([VALIDATION.md](VALIDATION.md)). Give the render a fixed camera. A fitted camera is
  fitted to the first age, where a flash is tiny, and can end up inside the grown sprite. Look at it from the nearest and the farthest
  distance the player sees it. Each of these is a finding to chase: a frame that pops, a hard line at
  a cell edge (bleed), dark fringes (premultiplied alpha in a plain translucent material), a jump at
  the loop, a blur at distance (mips) or a blur that sharpens (streaming).
