# Before you write a `.gchuman`

Read this page, then every guide it lists for what your character is, then the nearest fixture,
**before the first line of the file**. Then ask the tool for each block you will use
(`gc_describe kind:"human" block:"entries"`; the built character is `kind:"metahuman"`, a different
document). Write in your plan which of them you read.

A `.gchuman` decides **who the person is**: the preset, the face blended or sculpted, the head's size,
the body measurements, the eyelash and teeth variants, and the build steps that rig and assemble it.
What the character **wears and how it looks** (skin, eyes, makeup, head model, wardrobe) is the built
asset's own `.gc`, kind `metahuman`. The two documents are never peers: a line written in the wrong
one is refused, naming the right one.

## You want → write

Look here before you write a workaround or ask for new grammar. Each row names the spelling in one
line; the last column is where the tool or a guide explains it. If the tool's help and a row
disagree, the help is right.

| You want | Write | Ask or read |
|---|---|---|
| a face that is Ada's with Bruce's nose | `preset('Ada')` then `blend('Bruce', nose=0.4) [features]`; only the named regions blend | `kind:"human" block:"entries"` |
| the whole face moved toward another | `blend('Bruce', 0.45)` or `blend('Bruce', by=0.45)`: the same weight (0 to 1, default 1). A line is the whole face or regions, never both, and `by` is never a region name | `kind:"human" block:"entries"` |
| one part of the face moved | `sculpt(nose, translate=(x,y,z), scale=1.05)`; 21 region names, mirrored unless `[asymmetric]` | `kind:"human" block:"entries"` |
| a larger or smaller head against the body | `scale(0.98)`, between 0.8 and 1.3 | `kind:"human" block:"entries"` |
| a taller person, a wider waist | `measure(Height=182, Waist=82.5)`; the engine's own limits, and an unknown name lists the real ones | `kind:"human" block:"entries"` |
| eyelashes or teeth of another kind | `variant(eyelashes=1, teeth=0)`; an integer is a one-hot weight | `kind:"human" block:"entries"` |
| a crowd of different people | `random(min=165, max=186)` entries read by several characters, one build line each; the draw is seeded, so a rebuild gives the same crowd | `kind:"human" block:"entries"` |
| a likeness of a scan, a drawing or a 3D-AI head | `human(reference='/Game/…') { points { … } }`; at least three fit points | [CHARACTERS.md §6.1](CHARACTERS.md#61-current-grammar) |
| the fit to get a detail right (a lid fold, a lip line, the nose tip) | name the Move handle in `points { }`, for example `nose_tip = (x, y, z)`, measured on the reference; see them with `gc_render highlights:"landmarks"` | `kind:"human" block:"entries"` |
| the head to be a different size than the reference | the fit is life-size whatever units the reference is in; a build with `reference=` prints a note giving the ratio (for example 0.592x). `scale()` moves the head against the body and does not undo that ratio | the build note |
| skin tone, roughness, freckles or redness on a region | in the built character's `.gc`, `skin { }`: `Skin.U`, `Skin.V`, `Skin.Roughness`, and `Accents.<Region> = (Redness=, Saturation=, Lightness=)` for Scalp, Forehead, Nose, UnderEye, Cheeks, Lips, Chin or Ears. Roughness is Epic's multiplier (0.85 to 1.15); Epic's skin settings have no specular | `kind:"metahuman" block:"skin"` |
| eyes, makeup, a head model or clothes | the built character's `.gc`: `eyes`, `makeup`, `head`, `outfit` (and `wardrobe`, read-only) | `kind:"metahuman"` |
| a smile, a blink, an open jaw, any expression | not in `.gchuman`: `expression` is refused by name (the engine has no getter, so it could never read back). Use the FaceExpressions adapter and `drive(…)` in a `.gcmove` | [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md) |
| the playable character | `Person = human() {…}`, then `PersonRig = rig($Person, type=joints)`, then `PersonGame = assemble($PersonRig, pipeline=optimized, quality=high)`, each bound on a build line. Each rig is one paid Epic request, which is why it is an entry a dry run can count and `[rig=]` is refused | [CHARACTERS.md §6.1](CHARACTERS.md#61-current-grammar) |
| to look at the identity before the expensive steps | `gc_apply` with `through:"identity"` and `preview:"update"`, render the preview, then `preview:"discard"` | [CHARACTERS.md §6.2](CHARACTERS.md#62-identity-first-iteration) |
| to animate the body | an adapter `.gcrig` and a `.gcmove` | [METAHUMAN-RIG.md](METAHUMAN-RIG.md), [GCMOVE.md](GCMOVE.md) |
| hair or brows | a `.gcgroom` bound to the character | [GROOM.md](GROOM.md) |
| an eyelid crease or a double lid | `displace(by=<cm> \| $Profile, to=$Line, width=<cm> \| $Profile, slope=$Profile \| linear) [asymmetric]` in the `.gchuman`, along `Line = line(<handle>, <handle> offset=<cm>, ...)` drawn between the face's Move handles. There is no lid word | `kind:"human" block:"entries"` |

## Every `.gchuman`

| Read | For |
|---|---|
| [CHARACTERS.md](CHARACTERS.md) | the route (a MetaHuman, or a clean 3D-AI mesh fitted into one), the dependency order, what Human does not do, and the verification ladder |
| [ASSET-QUALITY.md](ASSET-QUALITY.md) | the bar a shipped character is held to: references first |
| [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md) | only when the reference is a body made from separately generated parts |

## What the character is, and what to read for it

| The character | Read | Fixture in the development checkout |
|---|---|---|
| a MetaHuman preset, varied by blend, sculpt and measure | `gc_describe kind:"human" block:"entries"`; [CHARACTERS.md §6](CHARACTERS.md#6-human-identity-native-rig-and-assembly) | `Content/__MCPTests/Human/Crowd.gchuman`, `Varied.gchuman` |
| a likeness fitted to a reference head | [CHARACTERS.md §6.1](CHARACTERS.md#61-current-grammar) and [§2.1](CHARACTERS.md#21-start-with-a-metahuman-or-fit-an-ai-mesh-into-one); measure the points with `gc_render mode:"ask"` | `Content/Test/FullHuman/MetaHuman.gchuman` (evidence, not a template) |
| a crowd | seeded `random()` entries, one build line each | `Content/__MCPTests/Human/Crowd.gchuman` |
| a character that moves | [METAHUMAN-RIG.md](METAHUMAN-RIG.md), [CHARACTERS.md §9](CHARACTERS.md#9-flagship-recipe-metahuman-face-plus-gcmove-body-and-macro-motion) | |
| a character with hair | [GROOM.md](GROOM.md), including its "Levels of detail" section | |

## Pipelines: what each assembly keeps

`assemble(…, pipeline=optimized|cinematic)` is required, and `quality=high|medium|low` belongs to the
optimised pipeline only. They are not the same character at two sizes:

- **`optimized`** strips face LODs; **`cinematic`** keeps them all. The field report found the optimised
  assembly missing the face's LOD0 that the cinematic one had. Choose the pipeline for the shots you need.
- Every assembly rebuilds its meshes and materials. Sockets added to the previous body or face mesh are gone
  afterwards: put them back with the mesh's `sockets { }` block. An eye value such as `Cornea.Size` may not
  reach the baked eye material: read the built material after the assembly. The generated physics asset's
  joints can carry drives that hold a ragdoll's limbs up until they are turned off.
- Epic's assembly also drops a groom that is never visible in the LODs it keeps (`GetMaxVisibleLOD`). A
  groom with only LOD0 visible vanishes from an optimised assembly; give it a `lods(…)` chain
  ([GROOM.md](GROOM.md#levels-of-detail)).
- A cinematic assembly is very large: the field report measured 60 to 83 GB of memory. Plan the machine
  before you ask for it, and dry run first so the paid and heavy steps are counted.

## Before you call it done

Read the build's notes (what was fitted, how far each handle landed, the head-size ratio, every rig and
assembly request the dry run counted), render the character from the front, three-quarter, side and above,
and check the likeness against the reference with the same camera. A rebuild starts from the preset, so
any sculpting done by hand in the editor since the last build is gone; say so before you rebuild
([VALIDATION.md](VALIDATION.md)).
