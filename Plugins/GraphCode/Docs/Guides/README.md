# GraphCode guides

This is the index of every guide: find your area, then open the one guide that answers your question.

New to a project? Read [../../SETUP.md](../../SETUP.md) (setting a project up) and
[CAPABILITY-MAP.md](CAPABILITY-MAP.md) (what each product, kind and block covers). The installed tools
explain their exact current parameters, values and grammar; the guides explain how to work. Read one
route at a time; do not load this whole tree into every task.

## Start here

**Before the first line of a `.gcmove`, `.gcrig` or `.gcshape`, open its entry guide —
[GCMOVE.md](GCMOVE.md), [GCRIG.md](GCRIG.md), [GCSHAPE.md](GCSHAPE.md) — and read every guide it
lists for what your file does, and the shipped example it names.** Each lists, per situation (a grip,
a strike, planted feet, a held prop, a missing bone), the guides that hold the method. A step a guide
states is never skipped because the rig or tool lacks something: fix the rig or tool instead.

The same holds for the other kinds: [GROOM.md](GROOM.md) for a `.gcgroom`, [GARMENT.md](GARMENT.md) for a
`.gcgarment`, [GCHUMAN.md](GCHUMAN.md) for a `.gchuman` and [EFFECTS.md](EFFECTS.md) for a Niagara system.
Each opens with a **"You want → write"** table: look there before you say GraphCode cannot do something,
because most reported gaps were a spelling that already existed.

If a tool is reachable, call it with `{ "help": true }`. Use `gc_describe` for an installed kind, block,
token or native type. A remembered example or guide never overrides those answers.

```text
I am a fresh AI in a GraphCode project
|
+-- I am about to make game content/presentation ... ASSET-QUALITY.md, first, every time
+-- The work is unfamiliar or complex ......... RESEARCH-AND-PLAN.md, before building
+-- I do not know the product or tools yet .... AI-COLD-START.md, then CAPABILITY-MAP.md
+-- The person asked me to set up a project ... ../../SETUP.md
+-- I know the task, but not the method ....... TOOLS.md, AUTHORING.md, LESSONS.md
`-- I need to prove a result .................. VALIDATION.md, PRODUCTION.md
```

## Getting started

| Guide | Answers |
|---|---|
| [../../SETUP.md](../../SETUP.md) | How is a project set up, licensed and connected, step by step? |
| [../CONNECTING.md](../CONNECTING.md) | How do I let an AI assistant read and write this project through the editor? |
| [AI-COLD-START.md](AI-COLD-START.md) | I have no conversation history: what do I read and do first? |
| [CAPABILITY-MAP.md](CAPABILITY-MAP.md) | Which product, kind and source document owns what? |
| [TOOLS.md](TOOLS.md) | Which verb do I call, how do I edit safely, and how do I prove the write landed? |
| [AUTHORING.md](AUTHORING.md) | Which product owns this work (Core, Shape, World, Move, Play, Rig, Groom, Garment, Human, Integrate) and how do I verify it? |
| [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md) | How do I research unfamiliar work and write the plan before building? |
| [ASSET-QUALITY.md](ASSET-QUALITY.md) | What quality bar do models, animation, VFX, UI and audio meet before I show them? |
| [LESSONS.md](LESSONS.md) | Which failures keep recurring on real projects, and how do I avoid them? |
| [Examples](../Examples/README.md) | Which working example files can I copy (hand and pistol, elephant rig, garments, stairs, walk and run)? |
| [Seeds](../Seeds/README.md) | Which starter documents do I copy into a new project? |

## Game design, gameplay and UI

| Guide | Answers |
|---|---|
| [DESIGNING.md](DESIGNING.md) | How do I design the player experience, choices, progression, levels and playtests? |
| [GAME-SLICE-EXAMPLE.md](GAME-SLICE-EXAMPLE.md) | What does one connected example look like from design through mechanics, UI, feedback and testing? |
| [GAMEPLAY.md](GAMEPLAY.md) | How do input, state, AI, UI, saving and networking connect? |
| [AI-NAVIGATION.md](AI-NAVIGATION.md) | How do StateTree or Behavior Tree, EQS, navigation and route recovery combine? |
| [UI-UX.md](UI-UX.md) | How do I design and finish a HUD, menu, inventory, settings or onboarding flow? |
| [UI-TOOLS.md](UI-TOOLS.md) | How do I prototype UI in HTML/CSS, generate its artwork and bring it into Unreal? |
| [PRODUCTION.md](PRODUCTION.md) | How do I start a production game, choose a representative slice and expand through gates? |
| [VALIDATION.md](VALIDATION.md) | How do I prove appearance, behaviour, persistence, performance and a packaged release? |
| [CONTENT.md](CONTENT.md) | Where does content come from, and how do I record its provenance (Blendkit, ElevenLabs sound, supplied files)? |

## Blueprints and assets

| Guide | Answers |
|---|---|
| [SETTINGS.md](SETTINGS.md) | How do I configure sidecars, exclusions and project settings? |
| [ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md) | Do separate meshes, sockets, seams and supports connect, with no z-fighting? |
| [CONTROL-RIG.md](CONTROL-RIG.md) | How do I author a native Control Rig on a prepared character? |

## Levels, World and PCG

| Guide | Answers |
|---|---|
| [ENVIRONMENTS.md](ENVIRONMENTS.md) | How do I build a realistic walkable environment: terrain, foliage, water, caves, World Partition route? |
| [ROADS-AND-BUILDINGS.md](ROADS-AND-BUILDINGS.md) | How do I plan connected roads, junctions, plots, terrain grades and building entrances? |
| [NATIVE-TERRAIN.md](NATIVE-TERRAIN.md) | How do I use native Landscape brushes, and where are their measured limits? |
| [WATER.md](WATER.md) | How do I make convincing water: material, flow, foam, shorelines, interaction? |
| [VEGETATION.md](VEGETATION.md) | How do I make professional UE5.7 and UE5.8 vegetation with PVE presets, custom plants, wind and biomes? |
| [PVE-PRESETS.md](PVE-PRESETS.md) | How does PVE preset authoring work in depth (data flow, modifiers, meshing, palettes, distribution)? |
| [SCENE-INSPECTION.md](SCENE-INSPECTION.md) | How do I measure scene mesh points for Shape cuts, or show GCW regions and courses in renders? |
| [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md) | How does the capsule climb stairs smoothly while the feet stand on the steps? |

## Shape

| Guide | Answers |
|---|---|
| [GCSHAPE.md](GCSHAPE.md) | The Shape entry guide: spellings, crisp lines, lofts and joins for a `.gcshape`. |
| [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md) | Which edge finish (rounded, chamfered, crisp) suits which material, and in what order? |
| [ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md) | How do I make contour and ornament shapes from SVG outlines, texture brushes and render-and-brush work? |
| [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md) | How do I join supplied or generated parts with Shape organic join? |

## Animation and Move

Start at [GCMOVE.md](GCMOVE.md), the animation index; it lists every guide and shipped example per situation.

| Guide | Answers |
|---|---|
| [GCMOVE.md](GCMOVE.md) | Which guide and example do I read before the first line of a `.gcmove`? |
| [ANIMATION.md](ANIMATION.md) | What does every `.gcmove` instruction do, and how do profiles, timing and runtime composition work? |
| [ANIMATION-LAYERS.md](ANIMATION-LAYERS.md) | How do I combine clips in a game: Blend Spaces, sync groups, additive layers, breathing while walking? |
| [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md) | How do hair, clothing and swinging parts move on an animated character, and how do I play a baked clip or speech? |
| [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) | How do I make a movement look like a person did it (a worked front kick)? |
| [POSES.md](POSES.md) | How do I reuse poses, travel between built poses and fit collisions? |
| [HUMAN-PRESET-ANIMATION.md](HUMAN-PRESET-ANIMATION.md) | How do I make a pose-based human animation from the body-part presets? |
| [MOVE-PARAMETRIC-POSES.md](MOVE-PARAMETRIC-POSES.md) | Which presets and values exist, and how do I time a list of poses by effort? |
| [QUADRUPED-PRESETS.md](QUADRUPED-PRESETS.md) | Which pose presets exist for four-legged bodies? |
| [WALK-AND-RUN.md](WALK-AND-RUN.md) | How do I make a walk or run with rolling feet, a body the feet carry, and a four-legged walk? |
| [STAIRS.md](STAIRS.md), [SURFACE-ALIGNMENT.md](SURFACE-ALIGNMENT.md) | How does a body climb stairs, and how does a standing foot align to a slope? |
| [HANDS.md](HANDS.md) | How do I close a hand, hold a handle, pistol or rifle, and walk with it? |
| [HOLDING-A-SWORD.md](HOLDING-A-SWORD.md) | How do I make a hand hold a sword or any handle firmly and fast? |
| [MOVE-BALANCE.md](MOVE-BALANCE.md) | How do I keep a planted body balanced while an arm reaches or a leg lifts? |
| [MOVE-PHYSICS.md](MOVE-PHYSICS.md) | How do gravity, collision and body contact work in Move? |
| [MOTION-FROM-VIDEO.md](MOTION-FROM-VIDEO.md) | How do I measure a clip of a rigged character and use it like any other motion? |

Human movement reference, with numbers ([HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) is the index):

| Guide | Answers |
|---|---|
| [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) | How do real bodies move, and which page holds which measurement? |
| [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) | How far does each joint of a healthy adult go, and how do I type it into a rig? |
| [HUMAN-WALKING.md](HUMAN-WALKING.md), [HUMAN-RUNNING.md](HUMAN-RUNNING.md) | What does a normal walk or run look like, in numbers? |
| [HUMAN-TURNING.md](HUMAN-TURNING.md) | How does a person turn while walking? |
| [HUMAN-BODY-SIZE-AND-BALANCE.md](HUMAN-BODY-SIZE-AND-BALANCE.md) | How does a gait scale to body size, where is the mass, how does a person stand? |
| [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md) | How does a hand grasp and an arm reach? |
| [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md) | How does a person throw overarm and punch? |
| [HUMAN-LIFT-CARRY-PUSH.md](HUMAN-LIFT-CARRY-PUSH.md) | How does a person lift, carry, push and pull? |
| [HUMAN-SIT-STAND-FLOOR.md](HUMAN-SIT-STAND-FLOOR.md) | How does a person sit, stand, crouch, kneel and get up from the floor? |
| [HUMAN-STAIRS-AND-LADDERS.md](HUMAN-STAIRS-AND-LADDERS.md) | How does a person climb and descend stairs and ladders? |
| [HUMAN-JUMP-AND-LAND.md](HUMAN-JUMP-AND-LAND.md) | How does a person jump and land? |
| [HUMAN-GAZE-AND-HEAD.md](HUMAN-GAZE-AND-HEAD.md) | How do the eyes and head look, move and coordinate with action? |
| [HUMAN-DOORS-AND-DEVICES.md](HUMAN-DOORS-AND-DEVICES.md) | How does a person use doors, phones, keyboards and screens? |
| [HUMAN-WEAPON-HANDLING.md](HUMAN-WEAPON-HANDLING.md) | How does a person hold and handle a rifle or pistol? |

## Cinematics

Start at [CINEMATICS.md](CINEMATICS.md), the workflow overview and review gates.

| Guide | Answers |
|---|---|
| [CINEMATICS.md](CINEMATICS.md) | How do I organise a cinematic from intent and animatic through performance, finishing and delivery? |
| [CINEMATICS-PREPRODUCTION.md](CINEMATICS-PREPRODUCTION.md) | What goes in the director's packet before production starts? |
| [CINEMATICS-INTENT.md](CINEMATICS-INTENT.md) | What should the audience experience, from which viewpoint, with what information progression? |
| [CINEMATICS-DIRECTION.md](CINEMATICS-DIRECTION.md) | How do I direct emotion, subtext, motifs, composition, colour, light and sound? |
| [CINEMATICS-TECHNIQUE-SELECTION.md](CINEMATICS-TECHNIQUE-SELECTION.md) | Which camera, framing, edit and sound techniques suit intimacy, suspense, comedy, scale or explanation? |
| [CINEMATICS-STORYBOARDS.md](CINEMATICS-STORYBOARDS.md) | How do I make storyboards, view studies and an animatic, and review key frames first? |
| [CINEMATICS-ATTENTION.md](CINEMATICS-ATTENTION.md) | How do I direct audience focus across beats and cuts? |
| [CINEMATICS-SHOTS.md](CINEMATICS-SHOTS.md) | How do I choose shot sizes and viewpoints and design staging, coverage and continuity? |
| [CINEMATICS-CAMERA-MOVEMENT.md](CINEMATICS-CAMERA-MOVEMENT.md) | How do I choose and execute pans, tilts, tracking, orbit, crane, zoom and focus pulls? |
| [CINEMATICS-EDITING-AND-TRANSITIONS.md](CINEMATICS-EDITING-AND-TRANSITIONS.md) | How do I cut scenes and build match cuts, dissolves, fades, wipes and whip transitions? |
| [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md) | How do I author and verify shots with GraphCode: bindings, timing, MRQ output, gameplay skip and recovery? |

## Characters, Rig, Hair and Garment

| Guide | Answers |
|---|---|
| [CHARACTERS.md](CHARACTERS.md) | How do Human, Rig, Move, Body and Face work together on one character, including fitting a clean 3D-AI mesh into a MetaHuman? |
| [GCHUMAN.md](GCHUMAN.md) | The Human entry guide: how do I blend, sculpt, measure and scale a MetaHuman identity? |
| [GCRIG.md](GCRIG.md) | The Rig entry guide: which guide do I read before a `.gcrig`? |
| [RIG.md](RIG.md) | How do I rig an imported hand or an animal from a bare mesh, and calibrate it? |
| [METAHUMAN-RIG.md](METAHUMAN-RIG.md) | How do I rig a MetaHuman body from the shared mapping? |
| [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md) | How do I reuse MetaHuman smile, blink and jaw controls and add calibrated face expressions? |
| [GROOM.md](GROOM.md) | How do I author hair or animal fur, including the British shorthair example? |
| [GROOM-STYLING.md](GROOM-STYLING.md) | What research, construction and combing sit behind a hairstyle? |
| [ANIME-HAIR.md](ANIME-HAIR.md) | How do I make anime or toon hair from solid card locks? |
| [FEATHERS.md](FEATHERS.md) | How do I make a wing of stylised solid or realistic card feathers? |
| [GARMENT.md](GARMENT.md) | How do I author and fit clothing or armour, and decide skinning versus simulation? |
| [GARMENT-PIECES-AND-EDGES.md](GARMENT-PIECES-AND-EDGES.md) | How do I add pockets and belt loops, declare edge finishes and set which way a pressed seam stands? |
| [GARMENT-HANG-AND-CURTAINS.md](GARMENT-HANG-AND-CURTAINS.md) | How do I use the game cloth's solver words, lift a hem, and make curtains? |

## Effects and materials

| Guide | Answers |
|---|---|
| [EFFECTS.md](EFFECTS.md) | The Niagara entry guide: what is the order of work for fire, smoke, sparks, magic and impacts? |
| [FLIPBOOKS.md](FLIPBOOKS.md) | How do I make a flipbook texture with image generation and wire it into SubUV? |
| [MATERIALS.md](MATERIALS.md) | How do I make good tiling materials: texel density, seamless sets, master materials, world-scale UVs? |
| [TEXTURE-VARIATION.md](TEXTURE-VARIATION.md) | How do I reduce visible texture repetition and plan dirt, wear and terrain RVT? |

## Integrate and Blender

| Guide | Answers |
|---|---|
| [BLENDER-SETUP.md](BLENDER-SETUP.md) | How do I set up Blender imports: downloads, add-ons, AI connections, project paths, verification? |
| [Blender import workflow](AUTHORING.md#integrate-bring-an-external-source-across-a-boundary) | How do static meshes and materials come across through Integrate, and how does managed reimport work? |
| GraphCode Integrate: the installed `Plugins/GraphCodeIntegrate/README.md` | What does the Integrate plugin send, and how do I operate it? |
| [Blendkit sourcing](CONTENT.md#use-blendkit-generators-and-materials-through-integrate) | How do I find Blendkit generators and materials and send them through Integrate? |

## Market

| Guide | Answers |
|---|---|
| [MARKET.md](MARKET.md) | How do I find, install and use a ready-made pack (damage, kits, pose libraries, guides, skills)? |

## Team and Git

| Guide | Answers |
|---|---|
| [GIT.md](GIT.md) | How do I keep binaries, projections and merges coherent in Git? |
| [TEAM.md](TEAM.md) | How do I work beside another person or agent? |

## Reference

| Guide | Answers |
|---|---|
| [SETTINGS.md](SETTINGS.md) | Configuration of sidecars, exclusions and project settings. |
| [Historical](../Historical/) | Closed and superseded records, kept for the record: [ANIMATION-DONE.md](../Historical/ANIMATION-DONE.md), [WALK-AND-RUN-DONE.md](../Historical/WALK-AND-RUN-DONE.md), [WALK-AND-RUN-DONE-2026-11.md](../Historical/WALK-AND-RUN-DONE-2026-11.md), [TRIPO3D.md](../Historical/TRIPO3D.md) (retired Tripo route), [README-LONG-FORM-2026-07.md](../Historical/README-LONG-FORM-2026-07.md). |

## What stays where

- **Tool help** owns current parameter names, values, side effects and exact grammar branches.
- **The capability registry** owns current product, kind, block and lifecycle coverage.
- **These guides** own workflows, composition, verification and durable lessons.
- **Project documents** own the player's goal, chosen content, local paths, decisions, open work and
  evidence for that project.

These guides ship with GraphCode and are not copied into a customer's project. Project docs should
link here and record only what is locally true, so a plugin update can improve the shared guidance
without leaving stale copies behind.

Contact: admin@graphcode.dev.
