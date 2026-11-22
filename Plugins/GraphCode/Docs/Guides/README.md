# GraphCode guides

These guides explain how to work. The installed tools explain their exact current parameters,
values and grammar. Read one route at a time; do not load this whole tree into every task.

## Start here

**Before the first line of a `.gcmove`, `.gcrig` or `.gcshape`, open its entry guide —
[GCMOVE.md](GCMOVE.md), [GCRIG.md](GCRIG.md), [GCSHAPE.md](GCSHAPE.md) — and read every guide it
lists for what your file does, and the shipped example it names.** Each lists, per situation (a grip,
a strike, planted feet, a held prop, a missing bone), the guides that hold the method. A step a guide
states is never skipped because the rig or tool lacks something: fix the rig or tool instead.

The same holds for the other kinds: [GROOM.md](GROOM.md) for a `.gcgroom`, [GARMENT.md](GARMENT.md) for a
`.gcgarment`, **[GCHUMAN.md](GCHUMAN.md) for a `.gchuman`** (a MetaHuman identity, likeness, skin, face) and
**[EFFECTS.md](EFFECTS.md) for a Niagara system**. Each opens with a **"You want → write"** table: look there
before you say GraphCode cannot do something, because most reported gaps were a spelling that already existed.

**Before unfamiliar or complex work in any area, read
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md).** Research thoroughly, verify the actual capabilities
and write a detailed implementation plan before building. Use its generic template for each
substantial unfamiliar part; keep the task-specific findings and plan in the project.

**Before making game content or presentation, read [ASSET-QUALITY.md](ASSET-QUALITY.md).** Models,
animation, VFX, UI, audio and other media share its bar: exceptional craft within the whole theme,
unless simplicity is explicitly requested. References and planning come first; build one finished
representative result, then expand with repeated render/playback/listening/interaction checks.

**Before designing a game or mechanic, read [DESIGNING.md](DESIGNING.md), then
[GAMEPLAY.md](GAMEPLAY.md). Before making game UI, read [UI-UX.md](UI-UX.md).** The player loop,
controls, presentation and recovery must work together. Use [UI-TOOLS.md](UI-TOOLS.md) when an
external tool helps achieve the quality bar: generate and refine a complete UI mockup and palette
from the game's references, then generate its transparent elements, test interaction in HTML/CSS or the existing workflow,
then verify the implementation in the game.

```text
I am a fresh AI in a GraphCode project
|
+-- I am about to make game content/presentation ... ASSET-QUALITY.md, first, every time
|
+-- The work is unfamiliar or complex ......... RESEARCH-AND-PLAN.md, before building
|
+-- I do not know the product or tools yet .... AI-COLD-START.md
|   `-- I need the installed capability map ... CAPABILITY-MAP.md
|
+-- The person asked me to set up a project ... ../../SETUP.md
|
+-- I know the task, but not the method
|   +-- choose a tool and edit safely .......... TOOLS.md
|   +-- choose the owning product .............. AUTHORING.md
|   `-- avoid known failure patterns ........... LESSONS.md
|
`-- I need to prove a result
    +-- appearance, behaviour, reload .......... VALIDATION.md
    `-- production or release readiness ........ PRODUCTION.md
```

If a tool is reachable, call it with `{ "help": true }`. Use `gc_describe` for an installed kind,
block, token or native type. A remembered example or guide never overrides those answers.

## Route by task

### Build or change content

| Need | Read |
|---|---|
| research any unfamiliar or complex work and write a detailed implementation plan first | [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md) |
| make game content or presentation at full quality, including models, animation, VFX, UI and audio | [ASSET-QUALITY.md](ASSET-QUALITY.md) |
| make artistic shapes and detailed surfaces with SVG, decals and ImageGen; bake the finished material | [ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md) after the quality guide |
| finish tables, building trim, sofas and other Shape parts with curved bevels and protected mating faces | [SHAPE-EDGE-FINISH.md](SHAPE-EDGE-FINISH.md) |
| check separate meshes, sockets, seams, support and possible z-fighting; paste an automatic editor check | [ASSEMBLY-VALIDATION.md](ASSEMBLY-VALIDATION.md) |
| plan roads, junctions, sidewalks, plots, terrain grades and complete building entrances | [ROADS-AND-BUILDINGS.md](ROADS-AND-BUILDINGS.md) |
| pick the right verb, edit safely and prove the write landed | [TOOLS.md](TOOLS.md) |
| find, install and use a ready-made pack (damage, kits, pose libraries, guides, skills) | [MARKET.md](MARKET.md) |
| understand Core, Shape, World, Move, Play, Rig, Groom, Garment, Human or Integrate ownership | [AUTHORING.md](AUTHORING.md) |
| set up Blender imports from Production: downloads, add-ons, AI connections, project paths and verification | [BLENDER-SETUP.md](BLENDER-SETUP.md) |
| import Blender or Blendkit assets: static meshes, materials and managed reimport through Integrate | [Blender import workflow](AUTHORING.md#integrate-bring-an-external-source-across-a-boundary), then the installed `Plugins/GraphCodeIntegrate/README.md` |
| build a realistic environment, terrain, foliage, water, caves or a playable World Partition route | [ENVIRONMENTS.md](ENVIRONMENTS.md) |
| measure scene mesh points for Shape cuts, or show GCW regions and courses in renders | [SCENE-INSPECTION.md](SCENE-INSPECTION.md) |
| make convincing water materials, flow, foam, shorelines and interaction; diagnose poor water appearance | [WATER.md](WATER.md), then the environment guide for terrain and mesh fitting |
| make good materials that tile properly: material job, texel density, seamless texture sets, import settings, master materials and instances, world-scale UVs, verification | [MATERIALS.md](MATERIALS.md) after the quality guide |
| reduce texture repetition; plan macro/micro imperfections, dirt and wear; evaluate terrain IHP with RVT | [TEXTURE-VARIATION.md](TEXTURE-VARIATION.md) |
| make professional UE5.7/UE5.8 vegetation: customize PVE presets, import custom plants or use UE5.8 growth/extraction; finish materials, wind, biomes and performance | [VEGETATION.md](VEGETATION.md) |
| learn PVE preset authoring in depth: native data flow, structural modifiers, meshing, palettes, distribution, phyllotaxy, ramps and troubleshooting | [PVE-PRESETS.md](PVE-PRESETS.md) |
| use native Landscape brushes and their measured boundaries | [NATIVE-TERRAIN.md](NATIVE-TERRAIN.md) |
| author hair or animal fur; use the British shorthair coat, whiskers and eye-material example | [GROOM.md](GROOM.md), including [the British shorthair example](GROOM.md#british-shorthair-example-coat-whiskers-and-eyes) |
| style a groom: the research, construction and combing behind a hairstyle, read with the groom guide | [GROOM-STYLING.md](GROOM-STYLING.md) |
| make anime or toon hair from solid card locks (a mesh with a toon look, outline and scalp cap, not strands) | [ANIME-HAIR.md](ANIME-HAIR.md), after [GROOM.md](GROOM.md) |
| make a wing of feathers on an arm mesh: stylised (rigid solids, toon look, outline) or realistic (flat cut-out cards with painted tiles, lit material; no folding or animation) | [FEATHERS.md](FEATHERS.md), after [ANIME-HAIR.md](ANIME-HAIR.md) |
| generate a 3D-AI model by part and combine prepared connections with Shape organic join | [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md) |
| (retired 2026-10-08, kept for the record) generate a model with Tripo Smart Mesh and compose it in GraphCode | [TRIPO3D.md](TRIPO3D.md): superseded; do not start from it |
| **write a `.gchuman`**: blend, sculpt, measure and scale a MetaHuman identity, fit a likeness to a reference head, skin and roughness, rig and assemble (optimised or cinematic) | **[GCHUMAN.md](GCHUMAN.md), the Human index** |
| use an existing MetaHuman or fit a clean 3D-AI mesh into one, then combine body and face | [CHARACTERS.md](CHARACTERS.md) |
| **make a Niagara effect**: fire, smoke, sparks, magic, an impact; modules, forces, `set(…)`, user parameters, how to see it | **[EFFECTS.md](EFFECTS.md), the Niagara index**. It opens with the order of work every effect follows: the stack as a table first, one emitter per layer, one system per phase, a second reviewer on stills, then Play in a lit level |
| make a flipbook texture for an effect with image generation: the sheet, the cleanup, the import and the SubUV wiring | [FLIPBOOKS.md](FLIPBOOKS.md) after EFFECTS.md |
| organize a professional cinematic from intent/animatic through performance, finishing and delivery | [CINEMATICS.md](CINEMATICS.md): workflow overview and review gates |
| plan a cinematic as a director and document necessary decisions before production | [CINEMATICS-PREPRODUCTION.md](CINEMATICS-PREPRODUCTION.md): required director's packet, decisions, planned studies, dependencies and readiness |
| define presentation intent, audience, viewpoint and information progression for a scene, environment, product, trailer or gameplay handoff | [CINEMATICS-INTENT.md](CINEMATICS-INTENT.md) |
| direct cinematic emotion, subtext, motifs, composition, color/light and sound perspective | [CINEMATICS-DIRECTION.md](CINEMATICS-DIRECTION.md) |
| choose camera, framing, edit and sound together for intimacy, suspense, comedy, scale, product explanation or other expressive presentations | [CINEMATICS-TECHNIQUE-SELECTION.md](CINEMATICS-TECHNIQUE-SELECTION.md): contextual alternatives, prerequisites, failure conditions and worked decisions |
| make storyboards, compare simple views/key frames, keep a frame ledger and review an animatic/rough layout before final recording | [CINEMATICS-STORYBOARDS.md](CINEMATICS-STORYBOARDS.md), including [frame-first review](CINEMATICS-STORYBOARDS.md#review-key-frames-before-final-recording) |
| direct audience focus and attention across beats/cuts; coordinate dramatic priority, visual hierarchy and optical focus | [CINEMATICS-ATTENTION.md](CINEMATICS-ATTENTION.md) |
| choose shot sizes/viewpoints and design staging, geography, coverage, lens/distance and continuity | [CINEMATICS-SHOTS.md](CINEMATICS-SHOTS.md) |
| choose and execute pans, tilts, push/pull, tracking, orbit, crane, zoom, focus pulls or operator behavior | [CINEMATICS-CAMERA-MOVEMENT.md](CINEMATICS-CAMERA-MOVEMENT.md), including [pan rehearsal](CINEMATICS-CAMERA-MOVEMENT.md#rehearse-and-finish-a-pan) |
| cut scenes; choose/construct match cuts, J/L cuts, dissolves, fades, wipes, hidden cuts and whip transitions | [CINEMATICS-EDITING-AND-TRANSITIONS.md](CINEMATICS-EDITING-AND-TRANSITIONS.md) |
| author cinematic shots/masters with GraphCode; verify bindings, timing, native handoffs, MRQ output and gameplay skip/recovery | [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md) after the filmmaking guide and saved director's packet |
| **animate anything (`.gcmove`)**: a grip or held prop, a strike, a reach, planted feet and balance, a walk or run, stairs, jumps, turning and gaze, physics, hair and clothing, runtime composition | **[GCMOVE.md](GCMOVE.md), the animation index**: every guide and shipped example per situation, read before the first line |
| move hair, clothing and swinging parts on an animated character, play a baked cloth or hair clip, and give a MetaHuman a spoken line | [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md), including [play a baked clip](ANIMATION-HAIR-AND-CLOTH.md#play-a-baked-clip) and [speech](ANIMATION-HAIR-AND-CLOTH.md#speech) |
| write or time a `.gcmove`: every instruction, profiles, effort timing, the body joining an action, reusable poses, travel between built poses, and a worked front kick | [ANIMATION.md](ANIMATION.md), [POSES.md](POSES.md), [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) |
| combine clips in a game: Blend Spaces, Sync Groups, additive layers, breathing while walking | [ANIMATION-LAYERS.md](ANIMATION-LAYERS.md) |
| close a hand, hold a handle, pistol or rifle (and the `grab()` route), keep the hold in later clips, walk with it | [HANDS.md](HANDS.md) |
| walk, run or climb: rolling feet, a body the feet carry, a four-legged walk as key poses timed by `travel()`; stairs and slopes | [WALK-AND-RUN.md](WALK-AND-RUN.md) (its closed sections: [WALK-AND-RUN-DONE.md](WALK-AND-RUN-DONE.md)), [STAIRS.md](STAIRS.md), [STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md), [SURFACE-ALIGNMENT.md](SURFACE-ALIGNMENT.md) |
| balance a planted body; gravity, collision and body contact | [MOVE-BALANCE.md](MOVE-BALANCE.md), [MOVE-PHYSICS.md](MOVE-PHYSICS.md) |
| how real bodies move, with numbers: joint ranges, walking, running, body size, grasping, throwing and striking, lifting, sitting, stairs, jumping, turning, gaze, doors, weapons | [HUMAN-MOVEMENT.md](HUMAN-MOVEMENT.md) (the index), [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md), [HUMAN-WALKING.md](HUMAN-WALKING.md), [HUMAN-RUNNING.md](HUMAN-RUNNING.md), [HUMAN-BODY-SIZE-AND-BALANCE.md](HUMAN-BODY-SIZE-AND-BALANCE.md), [HUMAN-GRASP-AND-REACH.md](HUMAN-GRASP-AND-REACH.md), [HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md), [HUMAN-LIFT-CARRY-PUSH.md](HUMAN-LIFT-CARRY-PUSH.md), [HUMAN-SIT-STAND-FLOOR.md](HUMAN-SIT-STAND-FLOOR.md), [HUMAN-STAIRS-AND-LADDERS.md](HUMAN-STAIRS-AND-LADDERS.md), [HUMAN-JUMP-AND-LAND.md](HUMAN-JUMP-AND-LAND.md), [HUMAN-TURNING.md](HUMAN-TURNING.md), [HUMAN-GAZE-AND-HEAD.md](HUMAN-GAZE-AND-HEAD.md), [HUMAN-DOORS-AND-DEVICES.md](HUMAN-DOORS-AND-DEVICES.md), [HUMAN-WEAPON-HANDLING.md](HUMAN-WEAPON-HANDLING.md) |
| animation measured from a video of a rigged character | [MOTION-FROM-VIDEO.md](MOTION-FROM-VIDEO.md) |
| **rig a mesh (`.gcrig`)**: an imported hand or MetaHuman (limits, contacts, sockets, carrier bones, hull bodies), an animal from a bare mesh, a native Control Rig | **[GCRIG.md](GCRIG.md), the rig index** |
| author a native Control Rig on a prepared character; rig a MetaHuman body from the shared mapping | [CONTROL-RIG.md](CONTROL-RIG.md), [METAHUMAN-RIG.md](METAHUMAN-RIG.md) |
| reuse MetaHuman smile, blink and jaw controls; add calibrated native face expressions | [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md), [FaceExpressions example](../Examples/README.md#faceexpressions-smile-blink-and-jaw-opening) |
| open working example files: a rigged hand holding a pistol, and an elephant rig | [../Examples/README.md](../Examples/README.md) |
| fit clothing or armour (Garment and Shape together), decide skinning versus simulation and verify it on the body | [GARMENT.md](GARMENT.md) |
| sewn-on pieces (pockets, belt loops), declared edge finishes and which way a pressed seam stands | [GARMENT-PIECES-AND-EDGES.md](GARMENT-PIECES-AND-EDGES.md) |
| the game cloth's solver words on `hang()`, a hem held up while a garment hangs, curtains | [GARMENT-HANG-AND-CURTAINS.md](GARMENT-HANG-AND-CURTAINS.md) |
| connect input, state, AI, UI, saving and networking | [GAMEPLAY.md](GAMEPLAY.md) |
| design and finish a HUD, menu, inventory, settings or onboarding flow | [UI-UX.md](UI-UX.md) |
| prototype UI in HTML/CSS, generate its artwork, optionally use other design tools, and bring it into Unreal | [UI-TOOLS.md](UI-TOOLS.md) |
| combine StateTree or Behavior Tree, EQS, navigation and route recovery | [AI-NAVIGATION.md](AI-NAVIGATION.md) |

### Design and produce a game

| Need | Read |
|---|---|
| design the player experience, choices, progression, levels and playtests | [DESIGNING.md](DESIGNING.md) |
| follow one connected example from game design through mechanics, UI, feedback and testing | [GAME-SLICE-EXAMPLE.md](GAME-SLICE-EXAMPLE.md) |
| choose where content comes from and record provenance | [CONTENT.md](CONTENT.md) |
| find/download Blendkit generators or materials, produce assets in Blender and send through Integrate | [Blendkit sourcing workflow](CONTENT.md#use-blendkit-generators-and-materials-through-integrate) |
| generate custom sound through the preferred ElevenLabs route and preserve its source | [CONTENT.md](CONTENT.md) |
| start a production game, choose a representative slice and expand through gates | [PRODUCTION.md](PRODUCTION.md) |
| prove appearance, behaviour, persistence, performance and a packaged release | [VALIDATION.md](VALIDATION.md) |

### Operate and recover safely

| Need | Read |
|---|---|
| learn from recurring real-world failures without loading project history | [LESSONS.md](LESSONS.md) |
| configure sidecars, exclusions and project settings | [SETTINGS.md](SETTINGS.md) |
| keep binaries, projections and merges coherent in Git | [GIT.md](GIT.md) |
| work beside another person or agent | [TEAM.md](TEAM.md) |

## What stays where

- **Tool help** owns current parameter names, values, side effects and exact grammar branches.
- **The capability registry** owns current product, kind, block and lifecycle coverage.
- **These guides** own workflows, composition, verification and durable lessons.
- **Project documents** own the player's goal, chosen content, local paths, decisions, open work and
  evidence for that project.

These guides ship with GraphCode and are not copied into a customer's project. Project docs should
link here and record only what is locally true, so a plugin update can improve the shared guidance
without leaving stale copies behind.
