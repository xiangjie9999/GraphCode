# GraphCode Market and the shipped presets

The Market is a free, signed catalogue of ready-made GraphCode text: mesh recipes, operations you
apply to your own shapes, material packs, pose libraries, guides and AI skills. Packs are text, not
meshes: installing writes the recipe into your project, and you build it when you want it.

- **Browse:** [graphcode.dev/marketplace](https://graphcode.dev/marketplace), or let your AI call `gc_market`.
- **Search** by words, kind, tag, state and sort order, and see which packs need a given pack. **Describe** a pack to see every
  entry, its settings, a ready-to-paste import line, its build choices, what it needs and what uses it.
- **Install** resolves dependencies first, checks the catalogue signature and every file hash, and writes
  the pack whole into `/Game/GraphCodeMarket/<Publisher>/<Name>/` with a lock file. `dry_run` shows what
  would be written. Installing needs no licence and never builds anything.
- **Update** keeps files you edited (it reports them as `edited_files`). **Remove** moves the pack to
  the project trash and refuses while another pack still needs it. One version per project; pin with `version:`.
- **Build** a pack's example by applying its main file with `gc_apply`. Building is a GraphCode write.
- **Operation packs** (Damage, Form, Surface, Wear, Weather) act on *your* shape: chip the edge of your
  own brick, sag your own beam. Mesh packs build their own subjects.
- **Material packs** come as flat colours or with optional photo texture sets (the Textures tick).
- **Skill packs** are copied to `.claude/skills/<Name>/` so your AI can use them directly.
- **Refusals are named:** a bad signature, a file hash mismatch or a version conflict stops the install
  before anything is written, with the reason.
- **Get in Unreal** on a pack page opens the GraphCode tab in your editor with the install confirmation.
- **For agents:** the catalogue is public read-only JSON at `https://graphcode.dev/marketplace/catalog.json`,
  with `agent.json`, `/llms.txt`, a `pack.json` per pack, and a read-only MCP endpoint
  `https://graphcode.dev/mcp` with `search` and `fetch`.

## All 39 packs (catalogue serial 61)

| Pack | Form | What it makes | Version | Needs |
|---|---|---|---|---|
| [Clean a Generated Mesh](https://graphcode.dev/marketplace/GraphCode/CleanGeneratedMesh/) | Guide | Turn an AI-generated 3D model (Tripo, Meshy and the like) into a clean, life-size asset with named material slots, keeping its painted texture: find what each piece is, keep the eyes, drop the whisker cards and hidden shells, close the holes, map the patches, assign and reassign materials. Worked example: a generated cat. | 1.1.0 | - |
| [Damage Guide](https://graphcode.dev/marketplace/GraphCode/DamageGuide/) | Guide | Which damage, wear and weathering packs do what, the order to use them in, a weathered-walkway example, and how to make a build look real. | 1.1.0 | - |
| [Bark Materials](https://graphcode.dev/marketplace/GraphCode/BarkMaterials/) | Materials | Tileable bark materials (furrowed oak, mossy oak, smooth beech), as flat colours or with optional photo textures. | 1.0.0 | - |
| [Foliage Materials](https://graphcode.dev/marketplace/GraphCode/FoliageMaterials/) | Materials | Leaf, ivy and grass materials (beech, oak, birch, ivy, young ivy, meadow grass): two-sided foliage that greens, dries and browns with age, as flat colours or with optional photo textures. | 1.0.0 | - |
| [Broken Concrete](https://graphcode.dev/marketplace/GraphCode/BrokenConcrete/) | Mesh | A reinforced concrete floor slab, intact, broken and wrecked, with its debris. | 1.0.0 | GraphCodeShape |
| [Broken Timber](https://graphcode.dev/marketplace/GraphCode/BrokenTimber/) | Mesh | Broken boards, beams, posts and slats with torn, splintered ends, ready to place. | 1.0.0 | GraphCodeShape; Damage |
| [Concrete Kit](https://graphcode.dev/marketplace/GraphCode/ConcreteKit/) | Mesh | Cast concrete, new: a board-formed wall with tie holes, lift lines and chamfered arrises, and a ledge, in game and high detail. | 1.0.0 | GraphCodeShape |
| [Concrete Parts](https://graphcode.dev/marketplace/GraphCode/ConcreteParts/) | Mesh | Loose concrete chunks and plates with a cast skin and rough broken faces. | 1.0.0 | GraphCodeShape |
| [Debris](https://graphcode.dev/marketplace/GraphCode/Debris/) | Mesh | Splinters, shards, chips, offcuts and smashed-board piles left behind by broken wood. | 1.0.0 | GraphCodeShape |
| [Debris Assemblies](https://graphcode.dev/marketplace/GraphCode/DebrisAssemblies/) | Mesh | Concrete chunks with steel bars poking through the break, as in a real ruin. | 1.0.0 | GraphCodeShape; ConcreteParts, Rebar |
| [Fixings](https://graphcode.dev/marketplace/GraphCode/Fixings/) | Mesh | Metric hex nuts, bolts and washers that fit each other. | 1.0.0 | GraphCodeShape |
| [Gears](https://graphcode.dev/marketplace/GraphCode/Gears/) | Mesh | Spur gears whose teeth mesh when you set them the right distance apart. | 1.0.1 | GraphCodeShape |
| [Masonry](https://graphcode.dev/marketplace/GraphCode/Masonry/) | Mesh | Bricks, roof tiles, slates, paving stones, kerbs, quoins and other building units. | 1.0.0 | GraphCodeShape |
| [Mechanical](https://graphcode.dev/marketplace/GraphCode/Mechanical/) | Mesh | Rivets, flanges, brackets, pipes, springs, chain links, handles and grilles. | 1.0.0 | GraphCodeShape; Solids |
| [Profiles](https://graphcode.dev/marketplace/GraphCode/Profiles/) | Mesh | Named curves such as ease, bell, taper and wave that shape how a form changes. | 1.0.0 | GraphCodeShape |
| [Rebar](https://graphcode.dev/marketplace/GraphCode/Rebar/) | Mesh | Steel reinforcing bars, whole, bent or snapped stubs, in three detail levels. | 1.0.0 | GraphCodeShape |
| [Rocks & Cliffs Kit](https://graphcode.dev/marketplace/GraphCode/RocksCliffsKit/) | Mesh | Cliffs cut by beds and joint sets (oblique joints, sway, dip, taper, splits), jointed outcrops, boulders and six scree sizes, with PCG graphs that scatter scree and lay cliff modules along a line. Built on the Stone preset; stone textures are pending release. | 1.0.0 | GraphCodeShape; Stone |
| [Roof Kit](https://graphcode.dev/marketplace/GraphCode/RoofKit/) | Mesh | Old pitched roofs: clay tile, slate, ruined, and a weathered East Asian pan-and-roll (kawara) roof, plus a gate roof. | 1.1.0 | GraphCodeShape |
| [Rope](https://graphcode.dev/marketplace/GraphCode/Rope/) | Mesh | Laid rope and cord, new to frayed: spans that sag, hanging ends, coils, lashings, hitches and knots. | 1.0.2 | GraphCodeShape |
| [Rubble](https://graphcode.dev/marketplace/GraphCode/Rubble/) | Mesh | A heap of concrete rubble with plates, chunks, fines and loose bent steel bars. | 1.0.0 | GraphCodeShape; ConcreteParts, DebrisAssemblies, Rebar |
| [Solids](https://graphcode.dev/marketplace/GraphCode/Solids/) | Mesh | Basic solids: box, cylinder, cone, ball, torus, wedge and more, ready to place. | 1.0.0 | GraphCodeShape |
| [Stone](https://graphcode.dev/marketplace/GraphCode/Stone/) | Mesh | STONE - one parametric stone. Every kind (field stone / boulder, split stone, river cobble, slab) is a NAMED VALUE SET of the same continuous settings; nothing switches method. | 1.0.0 | GraphCodeShape |
| [Stone Kit](https://graphcode.dev/marketplace/GraphCode/StoneKit/) | Mesh | Aged stone stairs and blocks, intact or broken, in several detail levels. | 1.0.0 | GraphCodeShape; ConcreteParts |
| [Stone Wall](https://graphcode.dev/marketplace/GraphCode/StoneWall/) | Mesh | Dry-stone, mortared, ruined, coursed and boulder stone walls and a 90 degree corner; a detailed boulder wall with soil and moss in the joints, in game and high detail. | 1.2.1 | GraphCodeShape; Stone, Weather |
| [Understory Kit](https://graphcode.dev/marketplace/GraphCode/UnderstoryKit/) | Mesh | Plants under the trees, every feature a knob: four ferns, hazel, boxwood and bramble, daisy, poppy, bluebell and foxglove, conifer sprays and cones, clover, moss and four fungi, plus PCG graphs for a forest floor and a flowering meadow. Wears the Foliage and Bark material packs; species textures are pending release. | 1.0.0 | GraphCodeShape; BarkMaterials, FoliageMaterials, VegetationKit |
| [Vegetation Kit](https://graphcode.dev/marketplace/GraphCode/VegetationKit/) | Mesh | Leaves of five species at any age, twigs and sprays for Unreal's Procedural Vegetation Editor, grass from lawn to hay meadow, rush and sedge, and ivy on walls and leaf litter grown by PCG graphs from small parts, wearing the Foliage and Bark material packs. | 1.1.0 | GraphCodeShape; BarkMaterials, FoliageMaterials |
| [Wood Ruin](https://graphcode.dev/marketplace/GraphCode/WoodRuin/) | Mesh | Wooden decks and board walls: a weathered plank walkway on lashed beams, and intact, broken and wrecked decks with the debris left. | 1.8.2 | GraphCodeShape; ConcreteParts, Damage, Debris, TimberKit |
| [Damage](https://graphcode.dev/marketplace/GraphCode/Damage/) | Mesh + Operation | Ways to damage a solid: chips, cracks, gouges, punctures, snapped and split wood. | 1.0.0 | GraphCodeShape |
| [Surface](https://graphcode.dev/marketplace/GraphCode/Surface/) | Mesh + Operation | Surface finishes: erode, pit, stipple, dent, groove and ridge. | 1.0.0 | GraphCodeShape |
| [Timber Kit](https://graphcode.dev/marketplace/GraphCode/TimberKit/) | Mesh + Operation | Weathered boards, plank walkways, board walls and ladders, plus an old-wood treatment (checks, end cracks, erosion, rot) for any wooden shape, in game and high detail. | 1.6.1 | GraphCodeShape; Damage, Debris, Rope |
| [Wear](https://graphcode.dev/marketplace/GraphCode/Wear/) | Mesh + Operation | Weathering and erosion that make an object look as if it has really existed. | 1.0.0 | GraphCodeShape |
| [Wheels](https://graphcode.dev/marketplace/GraphCode/Wheels/) | Mesh + Operation | A tyre whose tread is pressed into the rubber, not cut out of it. | 1.0.0 | GraphCodeShape |
| [Form](https://graphcode.dev/marketplace/GraphCode/Form/) | Operation | Reshape a model's surface by bending, bulging, sagging, twisting or curving it in. | 1.0.0 | GraphCodeShape |
| [Weather](https://graphcode.dev/marketplace/GraphCode/Weather/) | Operation | WEATHER: snow and moss on the shape being built, as Form's recipes are. Coverage is a soft vertex-colour mask; volume only on flat tops. | 1.0.0 | GraphCodeShape |
| [Human Body](https://graphcode.dev/marketplace/GraphCode/HumanBody/) | Poses | Human body poses (back, head, arms, hips, legs, feet, hand), measured on a MetaHuman body. | 1.0.0 | GraphCodeMove |
| [Human Face](https://graphcode.dev/marketplace/GraphCode/HumanFace/) | Poses | Human face poses by region (mouth, eyes, brows, cheeks), for a MetaHuman face. | 1.0.0 | GraphCodeMove |
| [Quadruped Body](https://graphcode.dev/marketplace/GraphCode/QuadrupedBody/) | Poses | Four-legged body poses (back, head, tail, legs, paws), measured on one cat sculpt. | 1.0.0 | GraphCodeMove |
| [Damage a Prop](https://graphcode.dev/marketplace/GraphCode/DamageAProp/) | Skill | A skill that helps Claude damage, age or ruin a prop using the damage packs. | 1.0.0 | - |
| [Match a Reference](https://graphcode.dev/marketplace/GraphCode/MatchAReference/) | Skill | A skill that helps Claude build a Shape asset that matches a reference photo: measure it, build from the numbers, compare renders from the photo's camera. | 1.0.0 | - |

## The 35 Shape presets that ship in the plugin

These recipe libraries are installed with GraphCode Shape at `GraphCodeShape/Content/Presets` and are in
this repository at [`Plugins/GraphCodeShape/Content/Presets`](../Plugins/GraphCodeShape/Content/Presets).
A preset is a family, not a fixed size: import it with your own values, for example
`shape('/GraphCodeShape/Presets/Fixings', Steel=$Brass)`, and call its entries with arguments.

| Preset | What it gives you |
|---|---|
| [`Solids`](../Plugins/GraphCodeShape/Content/Presets/Solids.gcshape) | Named basic solids (box, cylinder, cone, ball, torus, wedge and more), one geometric act each. |
| [`Footprints`](../Plugins/GraphCodeShape/Content/Presets/Footprints.gcshape) | The outlines everything else is stamped, cut or grown from. |
| [`Profiles`](../Plugins/GraphCodeShape/Content/Presets/Profiles.gcshape) | Named curves (ease, bell, taper, wave) that shape how a form changes along its length. |
| [`Surface`](../Plugins/GraphCodeShape/Content/Presets/Surface.gcshape) | Finishing operations: erode, pit, stipple, dent, groove, ridge. |
| [`Form`](../Plugins/GraphCodeShape/Content/Presets/Form.gcshape) | Reshape a surface: bend, bulge, sag, twist, curve in. |
| [`Wear`](../Plugins/GraphCodeShape/Content/Presets/Wear.gcshape) | Weathering and erosion qualities that make an object look as if it has existed. |
| [`Damage`](../Plugins/GraphCodeShape/Content/Presets/Damage.gcshape) | Chips, cracks, gouges, punctures, snapped and split wood. |
| [`Liquid`](../Plugins/GraphCodeShape/Content/Presets/Liquid.gcshape) | What a liquid leaves on a surface: drips, runs and pools. |
| [`Masonry`](../Plugins/GraphCodeShape/Content/Presets/Masonry.gcshape) | Bricks, roof tiles, slates, paving, kerbs, quoins: one building unit per entry. |
| [`Mechanical`](../Plugins/GraphCodeShape/Content/Presets/Mechanical.gcshape) | Rivets, flanges, brackets, pipes, springs, chain links, handles, grilles. |
| [`Fixings`](../Plugins/GraphCodeShape/Content/Presets/Fixings.gcshape) | ISO metric M8 hex nuts, bolts and washers that fit each other. |
| [`Gears`](../Plugins/GraphCodeShape/Content/Presets/Gears.gcshape) | Spur gears whose teeth mesh at the right spacing. |
| [`Wheels`](../Plugins/GraphCodeShape/Content/Presets/Wheels.gcshape) | A tyre whose tread is pressed into the rubber. |
| [`Rebar`](../Plugins/GraphCodeShape/Content/Presets/Rebar.gcshape) | Steel reinforcing bars: whole, bent or snapped, three detail levels. |
| [`ConcreteParts`](../Plugins/GraphCodeShape/Content/Presets/ConcreteParts.gcshape) | Loose concrete chunks and plates with cast skin and rough broken faces. |
| [`DebrisAssemblies`](../Plugins/GraphCodeShape/Content/Presets/DebrisAssemblies.gcshape) | Concrete chunks with steel bars poking through the break. |
| [`Debris`](../Plugins/GraphCodeShape/Content/Presets/Debris.gcshape) | Splinters, shards, chips, offcuts and smashed-board piles. |
| [`Rubble`](../Plugins/GraphCodeShape/Content/Presets/Rubble.gcshape) | A heap of concrete rubble composed from plates, chunks, fines and bent bars. |
| [`BrokenConcrete`](../Plugins/GraphCodeShape/Content/Presets/BrokenConcrete.gcshape) | A reinforced concrete slab, intact, broken or wrecked, with its debris. |
| [`BrokenTimber`](../Plugins/GraphCodeShape/Content/Presets/BrokenTimber.gcshape) | Broken boards, beams, posts and slats with torn ends. |
| [`BrokenTimberMaps`](../Plugins/GraphCodeShape/Content/Presets/BrokenTimberMaps.gcshape) | The texture layer that goes with BrokenTimber. |
| [`TimberKit`](../Plugins/GraphCodeShape/Content/Presets/TimberKit.gcshape) | Weathered boards, plank walkways, board walls, ladders and old-wood treatment. |
| [`TimberKitUltra`](../Plugins/GraphCodeShape/Content/Presets/TimberKitUltra.gcshape) | Each TimberKit subject at its heaviest detail tier. |
| [`WoodRuin`](../Plugins/GraphCodeShape/Content/Presets/WoodRuin.gcshape) | Wooden decks and board walls that swap in place for intact ones: intact, broken, wrecked. |
| [`StoneKit`](../Plugins/GraphCodeShape/Content/Presets/StoneKit.gcshape) | Aged stone stairs and blocks, intact or broken, several detail levels. |
| [`StoneWall`](../Plugins/GraphCodeShape/Content/Presets/StoneWall.gcshape) | Dry-stone, mortared, ruined and coursed walls, and a 90 degree corner on a quoin pier. |
| [`RoofKit`](../Plugins/GraphCodeShape/Content/Presets/RoofKit.gcshape) | Gable roofs in clay tile or slate, ruined roofs, a weathered kawara roof and a gate roof. |
| [`Organic`](../Plugins/GraphCodeShape/Content/Presets/Organic.gcshape) | Organic parts in centimetres for creatures and soft forms. |
| [`OrganicBrushes`](../Plugins/GraphCodeShape/Content/Presets/OrganicBrushes.gcshape) | Reusable projected sculpting controls. |
| [`AnimeBody`](../Plugins/GraphCodeShape/Content/Presets/AnimeBody.gcshape) | An unclothed adult anime base body. |
| [`AnimeHand`](../Plugins/GraphCodeShape/Content/Presets/AnimeHand.gcshape) | An anime hand in its rest pose. |
| [`Hands`](../Plugins/GraphCodeShape/Content/Presets/Hands.gcshape) | Hand forms built from interchangeable digit recipes. |
| [`HandAnatomy`](../Plugins/GraphCodeShape/Content/Presets/HandAnatomy.gcshape) | A parametric stylised hand. |
| [`Digits`](../Plugins/GraphCodeShape/Content/Presets/Digits.gcshape) | Finger and thumb building blocks. |
| [`FingerAnatomy`](../Plugins/GraphCodeShape/Content/Presets/FingerAnatomy.gcshape) | A detailed finger built from anatomy. |

## Pose and hair presets

| Preset | What it gives you |
|---|---|
| [`HumanBody.gcmove`](../Plugins/GraphCodeMove/Content/Presets/Human/HumanBody.gcmove) | Human body-part poses (stance, back, head, upper arm, forearm, reach, hang, hips, foot placement, thigh, shin, leg reach, foot, hand), each with values you pass in. |
| [`HumanFace.gcmove`](../Plugins/GraphCodeMove/Content/Presets/Human/HumanFace.gcmove), [`HumanFace.gcrig`](../Plugins/GraphCodeMove/Content/Presets/Human/HumanFace.gcrig) | Face poses by region for a MetaHuman face, and the face controls they drive. |
| [`Starter.gcmove`](../Plugins/GraphCodeMove/Content/Presets/Human/Starter.gcmove), [`Starter_Keys.gcmove`](../Plugins/GraphCodeMove/Content/Presets/Human/Starter_Keys.gcmove) | A starting motion program to copy and adapt. |
| [`QuadrupedBody.gcmove`](../Plugins/GraphCodeMove/Content/Presets/Quadruped/QuadrupedBody.gcmove) | Four-legged body poses. |
| [`Braids.gcgroom`](../Plugins/GraphCodeGroom/Content/Presets/Braids.gcgroom) | Braid and plait features you call on a region of the scalp. |
