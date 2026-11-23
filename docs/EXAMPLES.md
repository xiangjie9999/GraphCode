# Examples, presets and case studies

Everything you can open and learn from: the worked examples that ship in the package (all in this
repository), the downloadable sample files and case studies on graphcode.dev, and the preset libraries.

## Shipped examples

In [`Plugins/GraphCode/Docs/Examples`](../Plugins/GraphCode/Docs/Examples/README.md). The examples index gives
each one's files and build order (the garments also have a README per folder); your AI builds them with `gc_apply`.

| Example | What it shows | Products |
|---|---|---|
| [StairsInAGame](../Plugins/GraphCode/Docs/Examples/StairsInAGame) | A walk fitted to stairs at runtime: an IK Rig with one limb solver per leg, an Animation Blueprint with foot traces, touch-down hip height and a rest settle, a Character that walks to `StopX`, and a course of five 16 × 30 cm steps. All built on Unreal's tutorial character. | Core, Move |
| [WalkAndRun](../Plugins/GraphCode/Docs/Examples/WalkAndRun) | A rolling heel-ball-toe walk (game loop with root motion, a four-stride render version and a legs-as-drawn version), a measured human run at 3.96 m/s, and a run-up into an instep kick and its recovery. The built clips ship in the release download as `RunKick-BuiltContent.zip`. | Move, Rig |
| [HandAndPistol](../Plugins/GraphCode/Docs/Examples/HandAndPistol) | Epic's VR hand with joint ranges, thumb hinges, fingertip contacts, grip sockets and hull bodies; a pistol recipe with its own sockets; a fist clip and a pistol grip seated by touch with the finger on the trigger. | Rig, Shape, Move |
| [FaceExpressions](../Plugins/GraphCode/Docs/Examples/FaceExpressions) | A controls-only face adapter (smile, blink, jaw open) and a clip with two blinks, a held smile and a jaw opening on a MetaHuman. | Rig, Move |
| [ElephantRig](../Plugins/GraphCode/Docs/Examples/ElephantRig) | A whole elephant rigged from a bare mesh: measured points, spine, trunk, ears, tail, four legs, skin regions and sockets. | Rig |
| [TShirt](../Plugins/GraphCode/Docs/Examples/TShirt) | A crew-neck jersey T-shirt step by step: rib neckband, hems turned as real cloth with coverstitch rows, pressed armhole seams, with pictures. | Garment |
| [SkirtFabrics](../Plugins/GraphCode/Docs/Examples/SkirtFabrics) | One A-line skirt simulated twice: silk at 80 g/m² in many narrow folds, and denim at 400 g/m² in broad folds with the hem standing further out. | Garment |
| [GarmentBodies](../Plugins/GraphCode/Docs/Examples/GarmentBodies) | The T-pose and arms-down test bodies the garment examples are cut to, as Shape recipes. | Shape, Garment |

### Garments

21 garment folders, each a `.gcgarment` recipe (with a README, and a parts recipe where a garment has buttons,
zips or plates): button-up shirt, cargo trousers, dress, quilted gambeson, hooded cloak (on a MetaHuman body),
hoodie (with pictures), jeans, joggers, leather armour, linen tunic, long coat (with pictures), robe, scarf,
shorts, suit trousers, sweater, T-shirt, tactical vest, tank top, zip jacket (with pictures), and the skirt in
two fabrics.

### Terrain

[`Plugins/GraphCode/Resources/Examples/NativeTerrain`](../Plugins/GraphCode/Resources/Examples/NativeTerrain):
`Catchment.gcw`, `TextureBrush.gcw`, `TextureMountain.gcw` and `TerrainOperations.gcw` (terrain instructions,
PNG height brushes, erosion), `BP_HeightStamp.gc` and `BP_LandmassErosion.gc` (native Landscape brushes), and
the 16-bit height PNGs they use.

### Inside the guides

Complete recipes inside the guides: a human eyebrow groom (GROOM), five anime hairstyles and a feather wing
(ANIME-HAIR, FEATHERS), a complete procedural tree (VEGETATION), an imported torso and neck assembly
(`Docs/Guides/Examples`), the 84-joint MetaHuman body rig (METAHUMAN-RIG), a car body loft (GCSHAPE), and a
courier-robot game slice designed screen by screen (GAME-SLICE-EXAMPLE).

## Presets

35 Shape preset libraries, the human body and face pose libraries, a starter motion program, a quadruped body
library and a braids library ship with the plugins. Every one is listed in
[MARKET.md](MARKET.md#the-35-shape-presets-that-ship-in-the-plugin) and readable in this repository.

## Downloadable samples on graphcode.dev

| Sample | What is in it | Case study |
|---|---|---|
| [GraphCode-Airship.zip](https://graphcode.dev/media/GraphCode-Airship.zip) | The 18 m airship's recipe files: one dimensions file drives every part. | [The airship, written as code](https://graphcode.dev/features/assets#geometry) |
| [GraphCode-CatCoat.zip](https://graphcode.dev/media/GraphCode-CatCoat.zip) | The cat's 64-line coat recipe and its three zone libraries. | [Fur grown from a recipe](https://graphcode.dev/features/assets#grooming) |
| [GraphCode-Portal.zip](https://graphcode.dev/media/GraphCode-Portal.zip) | The cloud portal: one Niagara system, four materials and one Blueprint. | [Effects in code](https://graphcode.dev/features/assets#vfx) |
| [GraphCode-Systems.zip](https://graphcode.dev/media/GraphCode-Systems.zip) | The belt-fed machine gun Blueprint and the HUD that reads it. | [Game systems](https://graphcode.dev/features/gameplay#systems) |

## Case studies on graphcode.dev

| Case study | What it shows |
|---|---|
| [Materials: velvet, rugs and a rotting town](https://graphcode.dev/features/assets#materials) | A manor hall in peacock velvet, antique rugs and aged wallpaper, and a town weathered house by house; every surface a material written as GraphCode. |
| [Geometry: the airship and the Aether rifle](https://graphcode.dev/features/assets#geometry) | An 18 m airship built part by part, its deck lowered 70 cm with one edit and its cabin replaced by a glazed bridge in a few lines; a rifle in steel, brass, walnut, leather and glass, its engraving baked to a normal map. |
| [Grooming: a cat's coat](https://graphcode.dev/features/assets#grooming) | Over a million strands in four layers from one recipe. |
| [VFX: fire, ice and lightning](https://graphcode.dev/features/assets#vfx) | A cloud portal, a seven-emitter flamethrower, ice and thunder moves, houses burning at night. |
| [Animation: a violinist driven by the music](https://graphcode.dev/features/characters#animation) | Bow from loudness, vibrato from pitch, fingering, face and sway from one recording; a hero climbing through real windows. |
| [Rigging and skinning a cat](https://graphcode.dev/features/characters#rigging) | 36 joints placed in measured bands on a bare mesh, skin weights on a voxel grid. |
| [Faces and lip sync: Hana](https://graphcode.dev/features/characters#faces) | Lips solved from her voice, 23 lines each with a feeling, glances, blinks and head beats, shot as a cinematic. |
| [MetaHumans: a cast of the dead](https://graphcode.dev/features/characters#metahumans) | A varied zombie crowd on one skeleton and one skin material. |
| [Terrain: a cavern at real scale](https://graphcode.dev/features/worlds#terrain) | A 160 × 120 m pond in a 10 m basin with one continuous layered limestone wall. |
| [Levels: the steampunk town](https://graphcode.dev/features/worlds#levels) | A walled canal town with a domed lift and a ferris wheel; its layout is two text files and its level holds 2,648 actors. |
| [Lighting: moods as numbers](https://graphcode.dev/features/worlds#lighting) | Sun angle, fog, exposure and glow as named values; a jellyfish grotto; night as one switch. |
| [PCG and foliage: the jungle](https://graphcode.dev/features/worlds#pcg) | A valley about 1 km square planted from about 130 lines. |
| [Game systems, interactions, AI, damage](https://graphcode.dev/features/gameplay) | The machine gun, a playable piano, horde AI and damage and wear. |
| [Testing and debugging](https://graphcode.dev/features/testing) | Scripted Play sessions, captures and their logs. |
| [Collaboration and version control](https://graphcode.dev/features/collaboration) | Save history, diffs and restore on real project assets. |
| [Showcase](https://graphcode.dev/showcase/) | 16 collections and 94 renders of recipe-built props and buildings. |
