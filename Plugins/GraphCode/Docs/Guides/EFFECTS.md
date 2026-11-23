# Before you write a Niagara system

Read this page, then every guide it lists for what your effect is, **before the first line**. Then ask
the tool for each block you will use (`gc_describe kind:"niagara_system" block:"<block>"`). Write in
your plan which of them you read.

A Niagara system is written as a `.gc` of kind `niagara_system` through `gc_apply`, and created with
`gc_apply create:true`. GraphCode Move supplies it. There is no separate effects program file: the
document has four blocks (`settings`, `user_parameters`, `system`, `emitters`), and the effect is the
ordered module stacks inside them. Shared values many systems read (wind, weather, time of day) are the
separate kind `niagara_parameter_collection`.

## The order of work, for every effect

**Always build an effect this way** (user-ruled 2026-11-11, after the fireball spell fixture was built
by it and accepted: "always use this approach"). It holds for a spell, an impact, a muzzle flash, a
pickup, weather: anything a player watches. A step is not skipped because the effect is small; a small
effect has a short table.

1. **Write the stack before anything is made.** A table of every layer: what draws it, loop or once,
   when it starts, how long it lasts, its blend, grouped by phase (wind-up, travel, hit, aftermath). The
   section below says what belongs in it. No sheet, material or emitter comes before the table.
2. **One emitter per layer, one system per phase.** The game fires the phases in order (a Blueprint, a
   clip's notify, the hit); Niagara times the layers inside a phase.
3. **Make only the art the stack needs.** Flipbook sheets come from image generation and are repacked
   in code ([FLIPBOOKS.md](FLIPBOOKS.md)); one sheet serves several layers; flashes, sparks, embers and
   rings are small procedural materials with no texture.
4. **Build one phase at a time and look at it alone.** Render its first frames one by one and its whole
   life; fix what is plainly wrong by eye before stacking the next phase.
5. **Have someone else judge the stills, layer by layer.** A second reviewer (another agent or a person)
   names what reads wrong and in which layer; fix that layer and show the same view again. Stills settle
   shape, colour, order and weight. They do not settle motion.
6. **Time the whole effect in one demo Blueprint** over a floor and a wall, and render stills and a
   video of it.
7. **Play it in a lit stage level at a fixed exposure, from where the player stands,** and set
   brightness there ("Before you call it done", below). Fix what daylight shows; it always shows
   something the dark preview hid.
8. **A person judges the motion from video.** That verdict is theirs; an agent reading frames cannot
   give it.
9. **Judge by eye; measure only to settle a dispute.** Hours of measurement on a test effect bought
   nothing the eye and a second reviewer did not find faster.

The worked example is the fireball spell: repository `Content/__MCPTests/FireballSpell/`.

## You want → write

Look here before you write a workaround or ask for new grammar. Each row names the spelling in one
line; the last column is where the tool explains it. If the tool's help and a row disagree, the help
is right.

| You want | Write | Ask |
|---|---|---|
| a new effect (fire, smoke, sparks, magic, an impact) | `gc_apply create:true` with an `emitters { Name = emitter() {…} }` block; a blank emitter, or `[base=<emitter asset path>]` to build from one | `block:"emitters"` |
| particles pushed by gravity, drag or an attractor | a force module in `particle_update`, **then `module SolveForcesAndVelocity()` after it**. A force only adds to `Physics.Force`; without the solve step nothing moves. A write that leaves the dependency unmet still goes in and `write_warnings` names the module Niagara would add | `block:"emitters"` |
| a per-particle value no module offers (a light radius, a glow, a sprite facing) | `set(Particles.LightRadius=12.0)`: a new `set(…)` line is created. A common particle attribute takes its type from Niagara; any other states it, `set(Particles.Glow: NiagaraFloat = 2.0)` | `block:"emitters"` |
| a value the game or a Blueprint changes at runtime | `user_parameters { User.SpawnRate: NiagaraFloat = 20.0 }`, read in a module as `link(User.SpawnRate)` | `block:"user_parameters"` |
| values shared by many systems (wind, weather) | a `niagara_parameter_collection` | `gc_describe kind:"niagara_parameter_collection"` |
| a loop length or an emitter that loops | ``module EmitterState(`Loop Duration`=1.0)`` in `emitter_update`; `module SystemState(…)` in the system's `update` | `block:"system"`, `block:"emitters"` |
| an emitter that fires once, or a layer that starts late (the explosion two frames after the flash) | ``module EmitterState(`Life Cycle Mode`=Self, `Loop Behavior`=Once, `Loop Duration`=0.5)``, and the delay on the burst: ``module SpawnBurst_Instantaneous(Age=link(Emitter.LoopedAge), `Spawn Count`=40, `Spawn Time`=0.067)``. Two bursts are two module lines | `block:"emitters"` |
| particles spawned steadily | `module SpawnRate(SpawnRate=45.0)` in `emitter_update` | `block:"emitters"` |
| a particle's life, size or rotation, fixed or random | on InitializeParticle V2 (by path): `Lifetime`=0.3 alone, or `` `Lifetime Mode`=Random `` with `` `Lifetime Min` `` and `` `Lifetime Max` `` (min and max without the mode line are ignored); `` `Sprite Size Mode`=`Random Uniform` `` with `` `Uniform Sprite Size Min` `` and `Max`; `` `Sprite Rotation Mode`=Random `` with `` `Sprite Rotation Angle Min` `` and `Max` | [FLIPBOOKS.md](FLIPBOOKS.md) step 5 |
| colour or alpha that changes over the particle's life | ``module ScaleColor(`Linear Color Curve`=di(AlphaCurve.Keys=((),(Time=0.06,Value=1.0),(Time=1.0)), BlueCurve.Keys=((Value=1.0),(Time=1.0,Value=1.0)), GreenCurve.Keys=(…), RedCurve.Keys=(…)), `Scale Mode`=2)`` in `particle_update`. Write all four curves, names in alphabetical order; `()` is the key at time 0, value 0; mode 2 is the RGBA curve. An additive material that ignores alpha fades through the RGB curves | `gc_read` a system that has one |
| size that changes over the particle's life | a dynamic input can carry its own inputs now (`dynamic(<Name>, <Input>=<value>, …)`, built 16 Nov); a curve keyed that way is not yet proved. Until it is, keep the quad one size and grow the shape in the material from `ParticleRelativeTime()` | `block:"emitters"`; quote any refusal |
| a number the material reads per particle (a fade power, a cooling rate) | ``module DynamicMaterialParameters(`Index 0 Param 1`=1.5)`` in `particle_spawn`; the material reads it with `DynamicParameter()` | `block:"emitters"` |
| a spark stretched along its motion | `sprite(Alignment=VelocityAligned, …)` with `` `Sprite Size Mode`=Non-Uniform `` and `` `Sprite Size`=(X=6.0,Y=50.0) ``, or `` `Random Non-Uniform` `` with `` `Sprite Size Min` `` and `` `Sprite Size Max` `` for streaks of varied length | `block:"emitters"` |
| a burst thrown outward from a point, or sparks drawn in to it | SphereLocation V2 by its path, then ``module AddVelocityFromPoint(`Velocity Strength`=2000.0)`` in `particle_spawn`; a negative strength flies them inward. Then the forces and the solve step in `particle_update`. To start them on a shell rather than at the centre: ``SphereLocation(`Sphere Radius`=70.0, `Surface Only`=true, `Surface Only Band Thickness`=25.0)`` | `block:"emitters"` |
| one extra particle at the start of an emitter that otherwise spawns at a rate (so a looping ball has a head from frame 0) | a second spawn line, ``module SpawnBurst_Instantaneous(Age=link(Emitter.LoopedAge), UseLoopCountLimit=true)``: it fires on the first loop only | `block:"emitters"` |
| a light that flashes with the effect or travels with it | `light(bUseInverseSquaredFalloff=false)` in `renderers` and `set(Particles.LightRadius=900.0)` in `particle_spawn`; its colour is the particle's colour and its pulse a `ScaleColor` curve. An asset preview has nothing to light: judge it over a floor and a wall | `block:"emitters"` |
| a spell's phases fired in order (cast, then the projectile, then the impact where it hits) | an Actor Blueprint: `KismetSystemLibrary:Delay`, `NiagaraFunctionLibrary:SpawnSystemAtLocation`, `SpawnActorFromClass` for the projectile, and at the hit `MovementComponent:StopMovementImmediately`, `ActorComponent:Deactivate` on its Niagara component (the trail in the air finishes instead of vanishing) and `Actor:SetLifeSpan`. Render that Blueprint with `gc_render`, `simulate:true` and `at:` in each view: delays, spawned actors and spawned systems all run | [AUTHORING.md](AUTHORING.md), [TOOLS.md](TOOLS.md) |
| part of a running effect switched off by the game at one instant (a fireball's head, halo and light at the hit, while its tail finishes) | a user parameter, `User.HeadScale: Vector3f = (X=1.0,Y=1.0,Z=1.0)`, read in `particle_update` of those emitters by ``module ScaleColor(`Color Value To Scale`=link(Particles.Color), `Scale RGB`=link(User.HeadScale))``, and set to zero from the Blueprint with `NiagaraComponent:SetVariableVec3(InVariableName="User.HeadScale", …)` before `Deactivate`. `Scale RGB` is a vector: a float parameter cannot link to it. Deactivate alone lets every particle live out its life, so the head sat under the explosion | `block:"user_parameters"` |
| an emitter switched off without removing it (to judge one layer alone) | `Name = emitter() [enabled=false] {` | `block:"emitters"` |
| particles that follow the emitter instead of staying in the world | `LocalSpace=true` in the emitter's properties, `emitter(LocalSpace=true)`; world space is the default and is not written | `gc_read` an existing emitter |
| a GPU emitter | `emitter(SimTarget=GPUComputeSim)` | `block:"emitters"` |
| something drawn: a sprite, a flipbook, a mesh, a ribbon, a light, a decal, a volume | `renderers { sprite(Material=obj'/Game/…') }`; the type word names the class and the properties are the deviations from its default | `block:"emitters"` |
| a flipbook texture made with image generation (fire, smoke, an explosion's frames) | generate on black, repack onto an exact grid in code, then `SubImageSize` on the sprite and the V2 SubUV module by its path | [FLIPBOOKS.md](FLIPBOOKS.md) |
| a particle to react to an event, or a multi-pass update | `event_handlers { … }` and `simulation_stages { … }` inside the emitter | `block:"emitters"` |
| state a module input at its own default (`Drag=1`, `Spawn Count=1`) | write it: it is accepted, left unset, and noted in `write_warnings` ("reads back as the default"); the file records only values that deviate, so the line will not appear on the next read. Event-handler and simulation-stage stacks take it the same way (built 16 Nov) | the note names the input |
| to see the effect | render the **system asset** with `gc_render` and `at:<seconds>`; the preview seeks to that age (up to 120 s) | [TOOLS.md](TOOLS.md) |
| to see it in a level, played | a still of the open level does not advance its effects unless asked (the level-render row below): judge motion in Play or Simulate with `gc_play` | [VALIDATION.md](VALIDATION.md) |
| an effect with parts (a spell: cast, projectile, impact, fallout) | one emitter per layer, each with its own timing; one system per phase, the next spawned by the game at release or on the hit | the section below |
| edit the fire or smoke of a Fluids template (`Grid3D_Gas_Fire`, `_Smoke`, ...) | duplicate the template system (`gc_asset mode:"duplicate"`), then write the gas emitter's module lines in `emitters { }` like any other emitter; `User.WorldSpaceSize` and `User.ResolutionMaxAxis` size the box and the detail | `block:"emitters"`, `block:"user_parameters"`; "Niagara Fluids" below |
| bake a Fluids sim with Sim Caching | `gc_apply create:true` of a `niagara_sim_cache` with a `capture { System = obj'…' Seconds … FrameRate … }` block; a long one `wait:0`, collected with `unreal_info include:["tasks"]` | `gc_describe kind:"niagara_sim_cache"`, `block:"capture"` |
| to play a cache back | `SetSimCache` on the Niagara component from a graph (BeginPlay), or a Sequencer cache track. A saved level or Blueprint property cannot hold the cache: the component's property is transient | `gc_describe kind:"niagara_sim_cache"`; "Niagara Fluids" below |
| to see Niagara in a level render | `gc_render` of the level with `simulate:true` and `at:<seconds of warm-up>`: the open level's active Niagara components are advanced and the result carries `effects_warmed_up`. Without `simulate` they are not ticked and the render says so | `gc_render {"help":"simulate"}` |
| an emitter removed from a system | `delete <EmitterName>` as its own line in `emitters { }` (leaving it out is still refused; `[enabled=false]` keeps it but off) | `block:"emitters"` |
| a value computed from others (up + wind x k) | ``set(Particles.Color=dynamic(Add_Vector, A=…, B=…))``: `dynamic(<Name>, <Input>=<value>, …)` carries the dynamic input's own inputs, spelled as a module's are, and may nest | `block:"emitters"` |
| a big edit of an effect | edit the `.gc` beside the asset (`sidecar_path` is in every `gc_read`), then `gc_apply` with `asset_path` alone: only the blocks whose text changed are written. Not a large `ir` payload or a private script | `gc_apply {"help":"ir"}` |
| to judge an effect's motion | `gc_render` on a time span with `video:"mp4"`; a person plays it | [TOOLS.md](TOOLS.md) |


## An effect is a stack of timed layers

A good effect is many small effects, each well timed, stacked together (user-ruled 2026-11-11). One
flipbook on one sprite is one layer. **Before the first sheet, write the stack as a table:** every
layer, what draws it, whether it loops or plays once, when it starts, how long it lasts, and its blend.

- **Name a layer by how it plays.** A fireball is a constant glowing ball that loops while it flies.
  An explosion is a one-shot with a start and an end. They are different sheets made differently
  ([FLIPBOOKS.md](FLIPBOOKS.md)); calling one by the other's name makes the wrong one.
- **A fireball spell, for example:** fire gathers at the hand (the cast, short); a flash at release;
  the looping ball with its glow, trail, embers and light while it flies; at the hit a flash, a
  shockwave, the explosion, sparks and a light pulse, a few frames apart, not all on the same frame;
  then the fallout: smoke rising, embers falling, a scorch, lingering flames, seen for seconds.
- **Time it fast in, slow out.** The flash is 2-3 frames and the brightest thing in the effect; the
  shockwave fires first; the explosion follows 2-3 frames after the flash; sparks die soon after the
  ring; smoke starts early and outlives everything. Sources and the full table: repository
  `Docs/Research/spell-vfx-layering-and-timing.md`.
- **Each layer is its own emitter,** so its lifetime, loop, blend and sort are its own: fire is
  additive, smoke is translucent, and each part reads alone before it is stacked.
- **One sheet can serve several layers.** The fireball fixture's one-shot explosion sheet is the
  impact's body (frames 2-15, the opening star left to a separate flash), the projectile's body and
  tail (many short-lived world-space particles playing frames 5-15, born as full fire and breaking
  into wisps, so the tail thins on its own) and the flames at the hand. A looping swirl sheet is only
  the projectile's small hot head. Head alone did not look like flight and tail alone had no head;
  the two together did.
- **What went wrong first, each time:** a dark gap between the flash and a body that grew too slowly
  (skip the body's first frames and shorten its life); a shockwave drawn as a closed even ring, which
  read as a hoop round a pasted-on explosion (break the front into uneven arcs, thin it, fade it as
  it expands); sparks all born at the centre, a white starburst over the fire (start them on a shell
  at the body's edge); a trail whose particles grew with age, a tail as fat as the head. In the lit
  level: a straight line through the fire where the wall behind it changed brightness (additive fire
  is see-through: [FLIPBOOKS.md](FLIPBOOKS.md) step 5.1 gives the body opacity).
- **Place the hit by position, not by a delay.** A projectile stopped "after 1.0 s" lands a whole
  frame's travel late when a frame is slow (a metre at 900 cm/s), sometimes inside the wall. Store
  where it started, or use the real hit, and put the impact at that point, with its centre about a
  body radius in front of the surface so the burst reaches the wall without half of it buried.
  Diagnose a visual fault before fixing it: here the first guess (sprites cut by the wall) was wrong,
  and acting on it made the picture worse.
- The fixture is repository `Content/__MCPTests/FireballSpell/` (three systems, one per phase, and
  `BP_FireballSpellDemo`, which times them over a floor and a wall).

## Niagara Fluids (3D gas: fire, smoke, explosions)

Items marked "built 16 Nov" were built and proved in the editor that day (four test windows on the checkout
build). If a call here disagrees with the tool's help, the help is right.

Niagara Fluids is an engine plugin (beta) that simulates gas on a 3D grid, on the GPU, and draws it as a
volume. `NiagaraFluids` is not enabled by default: enable it in the project first, or the templates are not
there. Sim Caching needs no extra step: the cache asset, its capture and `SetSimCache` live in the Niagara
plugin Move already uses.

**Start from a template copy, never from a blank emitter.** The grid solver is one large inherited emitter
(`Grid3D_Gas_Master_Emitter`); every 3D gas template is a system holding one copy of it. Duplicate the nearest
template, then change values.

| Template system (`/NiagaraFluids/Templates/Gas/3D/Systems/`) | Use it for |
|---|---|
| `Grid3D_Gas_Fire`, `Grid3D_Gas_ColoredFire` | a burning source; the colored one reads a colour grid |
| `Grid3D_Gas_Smoke`, `Grid3D_Gas_ColoredSmoke` | rising smoke |
| `Grid3D_Gas_Explosion`, `Grid3D_Gas_DivergenceExplosion`, `Grid3D_Gas_DustExplosion` | one-shot blasts |
| `Grid3D_Gas_ParticleAdvection`, `Grid3D_Gas_SimpleParticleSource` | gas driven by a particle emitter |
| `Grid3D_MovingGridFire` | fire whose grid follows a moving owner |

Other families (2D gas, liquids, sand, particle rasterisers) are not covered here.

**User parameters.** Ask the copy (`gc_read`, `block:"user_parameters"`) for names, types and defaults. All ten
templates carry `WorldSpaceSize` (the box), `ResolutionMaxAxis` (cells along its longest side), `DrawBounds`,
two directional lights and three `Collide_*` switches; some add `StaticMesh`, `SourceOffset`, `Collide_Meshes`,
`EmitPosition` or `ShowOverlays`. Colour, density gain, temperature range, buoyancy and dissipation are module
inputs on the gas emitter, not user parameters.

**Cost, and why to bake.** A live 3D gas sim runs a grid solve every frame and keeps its grids in video memory;
resolution and box size set the cost. A level with one per instance will not hold. Choose the lowest
`ResolutionMaxAxis` that reads, and bake what a player only watches (a set piece, a cutscene fire).

**Editing the gas emitter of a copy (built 16 Nov).** Write the emitter's module lines in `emitters { }` as for
any emitter: a changed value (`Calculate Wind`, `DensityBuoyancy`, a colour) is accepted with the rest of the
gas emitter unchanged, and reads back. The grids are declared on the inherited base, and a data-interface
binding you restate keeps its variable. Removing a `set()` from the middle of a stack leaves the ones after it
alone: `set()` lines match by their targets, not by position. Read the copy first and change only the lines you
mean to.

**An input the module does not read is refused (built 16 Nov).** A module input that the module never reads
with its switches as set (ShapeLocation `Offset` while `Offset Mode` is at its default) is refused, and the
refusal names the switch value that makes the module read it. Write the switch beside the input or drop the
line. Ask `block:"emitters"`.

**Removing an emitter; values computed from others (built 16 Nov).** `delete <EmitterName>` is its own line in
`emitters { }`; a name the system lacks, a name also written, or an emitter another emitter's event handler
still reads is refused by name. A dynamic input takes its own inputs, `dynamic(<Name>, <Input>=<value>, …)`,
whose values are literals, links or nested `dynamic(…)`; bare `dynamic(<Name>)` means what it always did. A read
of an existing system now prints those inputs, so its sidecar changes on the next sync.

**Bake with Sim Caching (kind `niagara_sim_cache`, built 16 Nov).** A sim cache stores the frames of a running
system; a Niagara component given the cache plays it back without simulating.
- **Capturing is creating an asset:** `gc_apply create:true` with a `capture { System = obj'…' Seconds …
  FrameRate … }` block; the engine's capture settings are written by their own names. A long capture takes
  `wait:0`; collect it with `unreal_info include:["tasks"]`. Keep data-interface caching on, because the gas
  volume lives in a data interface. A grid cache has whole frames and no in-between sampling, so capture at the
  frame rate you will play. A process that cannot render (a `-nullrhi` host) activates no Niagara component, so
  capture refuses there by name.
- **Reading one:** `frames`, `emitters` and `system` are read-only blocks. Ask `gc_describe
  kind:"niagara_sim_cache"` for each block and for `capture`'s fields; do not copy a spelling from here.
- **Play it back with `SetSimCache`** called on the Niagara component from a graph (for example BeginPlay), or
  with a Sequencer Niagara cache track. A saved level or Blueprint property cannot hold the cache: the
  component's cache property is transient.
- **A render that warms up a component with `AdvanceSimulation` shows the LIVE sim, not the recording.** That
  call ticks the live simulation and never reads a cache. Judge playback with `gc_play`, or in a simulated
  Blueprint render, which ticks its world.

**How it is drawn, and how to look at it.** The gas is a volume (Heterogeneous Volumes is the engine's
experimental real-time or path-traced route), and its look depends on exposure: judge it at a fixed one in a
lit level, as below.
- **A level render shows Niagara only when asked (built 16 Nov):** `simulate:true` with `at:<seconds>` warms up
  the open level's active Niagara components to that moment and notes `effects_warmed_up`. Animation and physics
  are not advanced. Ask `gc_render {"help":"simulate"}`.
- **A Fluids asset render uses the Niagara editor viewport's own scene (16 Nov):** a system with a volume
  renderer or a Grid3D data interface is drawn in the same preview scene the Niagara editor viewport builds (its
  sky backdrop, lights and metered exposure, pinned once per call: `pinned_exposure` in the result), so the
  picture is the one the author compares against. An explicit `exposure:` adds on top (`exposure:-3` is darker).
  Sprite, ribbon and mesh effects render exactly as before. A fire template's core can stay white at every
  exposure because its emission is that bright; step `exposure:` down to read the core's colour.
- **Judge motion from video.** `gc_render` on a time span (`at`, `to`, `fps` or `count`) with `video:"mp4"` writes
  a movie a person can play (H.264, Windows; APNG with a warning where the encoder is missing). Frames 0.1 s
  apart of a slow gas look almost the same; the movie shows the flow. The verdict on motion is the person's.

**Big edits go through the file route.** Do not push a large payload through `ir` or a private script. Edit the
`.gc` beside the asset (`sidecar_path` is in every `gc_read`) as you edit any file, then call `gc_apply` with
`asset_path` alone: every block is compared with a fresh projection and only the blocks whose text changed are
written. Creating an asset works the same way: write the `.gc` at the sidecar path, then `gc_apply create:true`.
`ir` is for a block the projection does not carry yet. Ask `gc_apply {"help":"ir"}`.

## Every Niagara system

| Read | For |
|---|---|
| [ASSET-QUALITY.md](ASSET-QUALITY.md) | the bar a shipped effect is held to: references first, one finished effect, then the rest; judge motion, not one frame |
| [AUTHORING.md](AUTHORING.md) | that Move owns Niagara, and the read, describe, dry-run, apply loop |
| [TOOLS.md](TOOLS.md) | the render and play verbs that show an effect |

## What the effect is, and what to read for it

| The effect | Read |
|---|---|
| fire, smoke, sparks, dust, magic | the table above, then the material it draws with: [MATERIALS.md](MATERIALS.md) |
| an effect drawn from a flipbook, its frames made with an image generator or a video model | [FLIPBOOKS.md](FLIPBOOKS.md), then [ARTISTIC-SHAPES.md §2](ARTISTIC-SHAPES.md#2-use-imagegen-for-texture-art-and-svg-design-references) for ImageGen practice |
| rain, wind, weather, a time of day | a `niagara_parameter_collection` shared by the systems; [ENVIRONMENTS.md](ENVIRONMENTS.md) for the world it sits in |
| water, spray, foam | [WATER.md](WATER.md) |
| an effect a character or a hit sets off | the clip's notify or the game's Blueprint spawns the system; prove it in play, not in a still ([GAMEPLAY.md](GAMEPLAY.md), [VALIDATION.md](VALIDATION.md)) |
| an effect in a trailer or cutscene | [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md) |

## Rules the tool enforces that surprise people

- **The `system` stacks are snapshots.** A module has no identity, so the stack you write is the stack:
  leaving a module out removes it, and the order you write is the order it runs. Treat an emitter's four
  stacks the same way: read the asset first, then write the whole stack.
- **A new emitter is appended; one is removed only by `delete <EmitterName>`.** Leaving one out is refused. List the existing emitters first, in order, and
  give a new one all five child blocks (`renderers`, `emitter_spawn`, `emitter_update`, `particle_spawn`,
  `particle_update`).
- **`settings` carries three fields only:** `EffectType`, `WarmupTime`, `FixedBounds`. Any other name is
  refused by name rather than accepted and dropped.
- **A module name is its short name when that is unique;** an ambiguous one is refused with the candidate
  paths, and an exact object path always works.
- **The dry run is real:** every emitter write, including a dry run that builds new emitters, is checked
  on a duplicate before the original is touched.
- **A new system's document needs its `parent: /Script/Niagara.NiagaraSystem` line;** without it the
  create is refused by name.
- **A static switch keeps its value when you delete its line.** `` `Lifetime Mode`=Random `` removed
  from an existing module still reads back as Random; write the other value to change it.
- **The render pins its exposure to its first picture.** A span that starts on an empty frame (age 0,
  before anything has ticked) blows the rest out to white: start at the first frame that shows
  something.

## Before you call it done

Read the write's `write_warnings` and the build's notes first: a force without the solve step, a
deprecated module and an unmet dependency are all named there. Then render the system asset at several
ages (the start, the peak and the tail), from the angle the player sees, and play it in the game with the
real lighting. A particle that sits still and an effect that shows only in the asset render are each a finding to
chase, not a result ([VALIDATION.md](VALIDATION.md)).

**Set brightness in a lit level at a fixed exposure, not in the asset preview.** The preview is dark and
meters itself, so it hides the two faults daylight shows at once (measured on the fireball fixture): the
flashes, tuned to look right in the preview, were about two stops too dim and could not be seen in a daylit
level, and lit smoke with a mid-grey albedo was paler than the wall behind it. Make a small stage level: a
floor, a wall behind the effect, a sun and sky, and a post-process volume whose auto-exposure minimum and
maximum are the same value (EV100 2.0 gave a mid-grey sunlit floor under the default sun). Put a camera where
the player stands (a `CameraActor` with `AutoActivateForPlayer=Player0`), give the demo an event to start it
(`gc_play` `execute` with `{act:"invoke", target:"Demo.StartSpell"}`), and capture with `shot` steps. A shot
stalls the game about a tenth of a second, so shots land 8-11 a second: a layer that lives 0.1 s falls between
them. Cast the effect two or three times in one session to land a shot inside it, rather than lengthening the
layer.
