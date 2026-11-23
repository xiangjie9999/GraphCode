# Make game assets and presentation at full quality

*A GraphCode guide. Read it before making game content or presentation: models, environments,
characters, animation, VFX, materials, lighting, UI, audio, cinematics and interactive feedback.
Tool help gives the exact words; this guide gives the order of work and the quality bar.*

This is not optional. The only exception is a blockout the owner explicitly asked for, to test a
mechanic. Label it as a blockout and add a task to replace it. A blockout never closes an art task.

For unfamiliar or complex work, the plan also follows
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md): research the technique and pipeline thoroughly,
check installed capabilities, then save detailed implementation and verification steps before
building. A visual reference alone does not explain how to construct the result.

## The bar

The owner of GraphCode set it on 2026-09-28, after an AI rushed a warrior character:

> "each asset need to be as detailed and as flawless as possible without any trace of ai"
>
> "reject anything quick and sloppy that only subtlely resembles the shape"

The owner strengthened the brief later that day:

> "make the assets more detailed than usually necessary, surprise the user. basically, along with
> repeated render validation across each seam and detail, produce a formidable asset"
>
> "and fit to the whole aesthetic of the theme, unless explicitly told to do it simple"
>
> "and never straight edges or simple geometry like straight lines unless it's explicitly designed as so"
>
> "or a simple sphere"
>
> "and not just 3d asset, but animations, etc. also need"

**Default to an exceptional finished asset with richer, purposeful detail than the minimum needed
to satisfy the brief.** Take initiative on craftsmanship and distinctive features that belong to
the established theme. Aim to reward both the first impression and close inspection. Treat the
reference as a governing design, then resolve and enrich its construction rather than stopping
at the easiest recognizable approximation.

An explicit request for something simple changes the amount of detail. Deliver a deliberately
simple, well-finished asset with clean proportions, connections and materials; keep the validation
requirements. A request for simplicity is not automatically a request for a blockout. The owner's
chosen aesthetic, silhouette, functional requirements and performance constraints still govern.

A result that looks roughly like the reference fails. Reject your own work when you see any of these:

- **Primitive stacks and undesigned contours.** Boxes, cylinders, bare spheres and straight strips
  standing in for designed parts. Straight edges or simple forms are acceptable only where the
  governing design explicitly calls for them; tool convenience is not a design reason.
- **Smooth, unbroken surfaces.** Big flat panels and plain curves with no construction on them.
  Curvature is fine; an uninterrupted primitive is not a finished part.
- **Detail as noise.** Scattered bolts, random greebles, the same rivet everywhere, or a texture
  where geometry should be. Every detail must explain how the thing is built.
- **Parts that float, pierce or flicker.** Parts that do not touch what holds them, pass through their
  neighbours, or share a surface with them (see [overlap and flicker](#7-parts-must-not-share-a-surface)).
- **Parts with no job.** A pipe that goes nowhere, a gear that drives nothing, a strap that holds nothing.
- **Numbers nobody chose.** `0.784377367`, `.403853`, or a coordinate copied off a render into five
  places (see [numbers](#numbers)).
- **One even treatment everywhere.** Everything symmetric and identical, or the opposite: random
  distortion on every surface. Real things have regular construction and local variation.
- **One flat material.** Glossy plastic metal, even dirt, no difference between steel, bronze,
  leather and cloth.
- **Hair and clothing tells.** Hair that stands out like quills or flies off in strays, bald partings,
  armour that is a single shell, clothing that clips the body.
- **"Done" because it built.** A successful build, a green check or a triangle count proves nothing
  about how it looks.

The bar applies to **each part when it is built**, not to a polish pass at the end. Detail that is
postponed rarely arrives.

If time is short, make fewer assets. Never lower the bar to make more of them.
### Use an independent visual critic on every revision

Owner, 22 November 2026, after rejecting an untextured broken stump: **"Always have a critic"**.
Have an independent critic inspect the actual built result before presenting or publishing each revision.
Give the critic the owner's brief, relevant references and current engine views, including close and
opposite views of the repaired areas. A successful build, texture file or numerical check cannot replace
this review. Resolve the critic's material and geometry findings, render again, and return the revised
result to the critic. The owner remains the final judge; the author cannot approve their own appearance.

### Validate the controls, not just the default

Owner,22 November2026: **"No brute forcing (everything should be parametric and with a reason)"** and
**"I don't want something that looked okay on a single parameter value and failed when change any other 9 parameters."**
For a parameterized asset, inspect relevant working precedents before designing its construction. State
what each control means, its valid domain and the relationships that keep dimensions, attachments,
materials and detail valid when controls change. Derive dependent features from those relationships;
do not search arbitrary variants for a lucky default. Verify limits, interacting controls and multiple
seeds, with actual views of representative combinations. An attractive default or one successful build
cannot establish that the asset's controls work. Give the independent critic those varied results.
### Make the detail belong to the theme

Inspect the project's accepted assets, environment, palette and references before designing the
extras. Record its shape language, proportions, material combinations, edge treatment, construction
methods, ornament, age/wear and level of stylization in the build guide. Carry the same visual
language through the whole asset, including secondary and rear-facing parts. Render it beside
representative assets in the intended scene to check that it belongs.

Plan the extra craft before building: stronger layered construction, convincing attachment details,
designed transitions, material-specific finish, or a distinctive feature grounded in the asset's
purpose and world. Choose additions that make the object feel authored and worth inspecting.
Surprise the owner through quality and thoughtful invention within their brief. Preserve established
identity and gameplay; a flourish must not obscure an affordance or change what the object does.

Give every addition a reason. Balance detailed focal areas with deliberate quiet surfaces so the
silhouette and visual hierarchy stay clear. A restrained theme can gain depth through precise
profiles, subtle layering and finish. Random greebles, uniform dirt, excessive ornament and increased
polygon count do not establish quality. Choose geometry, materials or other supported techniques
according to the feature's silhouette, camera distance and rendering needs. Verify cost against the
real budget while preserving the intended appearance.

### Choose controlled construction for artistic shapes

**Updated user direction, 8 October 2026:** allow iterative render-and-brush sculpting and painting
alongside SVG and authored texture brushes. Neither input is compulsory for every form. The loop
must support actual depth with bounded displacement and slope controls, followed by front, side
and grazing inspection. Follow [the brush workflow and query limits](ARTISTIC-SHAPES.md#iterate-with-render-and-brush).
Tripo and other generative 3D model approaches are retired for unreliability; 2D reference/artwork
generation remains available. Earlier instructions to choose generated 3D models are superseded.

Earlier direction, broadened 2026-10-01 and extended above: **for contour-led shapes that are hard
to describe procedurally, prefer an SVG-based approach; use decals for surface detail, with ImageGen for
textures and artwork to turn into SVG.** This supersedes the earlier "complex artistic contours"
threshold. Keep naturally parametric forms procedural when they remain easy to express.
Plan around editable vector design for contours, profiles and openings, construction for
form and interfaces, and painted material detail with a verified production bake. Use physical
geometry where silhouette, thickness, parallax or contact requires it.

**SVG source route, extended 6 October 2026:** when choosing SVG for unique contours, create or reuse texture/motif artwork first,
then convert its flat regions or deliberate silhouette mask into named SVG paths. Inspect the
source/trace preview, holes and part IDs before importing and giving it physical form. Follow
[the texture-to-SVG conversion loop](ARTISTIC-SHAPES.md#convert-texture-artwork-to-svg), using
the shipped `Plugins/GraphCode/Tools/ImageToSvg`. Keep material shading and grain in the texture
workflow; isolate the intended contour for vector conversion.

**Extended 1 October 2026:** actively use texture height/opacity brushes with weighted `displace`
for continuous organic form, including a nose's bridge, tip and wings. Give the field meaningful
depth, footprint and strength controls; SVG can supply the contour or openings. This is a
construction method as well as a surface-detail technique. Follow
[sculpt natural forms with texture brushes](ARTISTIC-SHAPES.md#sculpt-natural-forms-with-texture-brushes)
and inspect the actual displaced mesh from multiple views.

Read [ARTISTIC-SHAPES.md](ARTISTIC-SHAPES.md) before choosing the representation. It covers
ImageGen artwork, raster-to-vector cleanup, SVG extrusion/loft/conform, iterative brush sculpting, weighted painting and
shared atlases. Preserve the same craft and inspection bar; verify the final runtime cost.

## Apply the quality bar to every medium

**This rule covers all authored game content and presentation, including animation.** The examples
below are not the limit of its scope. Aim for a distinctive, carefully finished result beyond the
minimum implementation, coherent with the whole game's aesthetic, unless simplicity is explicitly
requested. In motion or sound, richness comes from designed timing, response and layering as well
as detail. Extra particles, keyframes, ornaments or audio layers alone do not establish quality.

Use the same cycle in every medium:

1. **Study appropriate references and the surrounding game.** A motion clip can explain timing
   that a still cannot; sound needs listening references. Identify what makes the work belong to
   the theme, and what the player must perceive or do.
2. **Plan the craft beyond the minimum.** Describe the distinctive features, transitions, secondary
   details and finish that will make this result memorable. Resolve ambiguous steps using
   [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md). Select supported tools and evidence methods before
   authoring; use external tools when they improve the result and fit the project's pipeline.
3. **Finish one representative passage or interaction.** Choose a sample that includes the hardest
   quality and integration requirements. Judge it in context before expanding to the full work.
4. **Inspect every authored element and boundary.** Cover each part, phase, state and transition.
   Render, play, listen or interact as the medium requires. Repair findings and repeat the same
   observation, including adjacent behaviour affected by the change.
5. **Prove the whole experience.** Run the complete result in its intended context, including
   applicable interruption, repetition and recovery. Preserve evidence from the current revision.

| Medium | Craft to plan beyond the minimum | Repeated observation required |
|---|---|---|
| Models, environments, characters, clothing and hair | Designed contours, layered construction, believable connections and material-specific finish. | Whole-object views plus every seam/detail; assembly, grazing angles and relevant poses. |
| Animation and camera motion | Clear key poses, deliberate timing/spacing, weight, arcs, anticipation, overlap, follow-through and character-specific expression; living characters also need appropriate breathing and moving holds. | Continuous playback at the intended speed, then slow/frame inspection of critical passages; contacts, holds, transitions, loop boundaries and final pose. |
| VFX and animated materials | A readable event with designed onset, peak, sustain and decay; purposeful layers, shapes, rhythm and interaction with scene lighting and motion. | Full temporal sequence in context, repeated triggers, interruption, overlapping instances and relevant viewpoints; inspect frame cost as well as appearance. |
| UI and interaction feedback | Theme-specific composition, hierarchy, typography, iconography, complete states and deliberate motion, sound and response to input. | Every screen/state and transition at target sizes and input methods, including focus, failure/recovery and repeated interaction. |
| Materials and lighting | A coherent palette, surface response, scale of detail, focal lighting and controlled variation suited to the world. | Representative surfaces and scenes under intended conditions, different views/distances and any time-varying states; inspect seams and transitions. |
| Audio and music | Thematic timbre, timing, spatial character, dynamics, variation and transitions; clean layering with dialogue and gameplay cues. | Actually listen in isolation and in the game mix; check onset/tail, loop joins, transitions, repeated/overlapping triggers, distance and interruption where applicable. |
| Cinematics and combined gameplay presentation | Dramatic intent, readable staging, an edited animatic, specific performance, deliberate camera/lighting and a coherent sound mix; follow [CINEMATICS.md](CINEMATICS.md). | Play the complete sequence; inspect every cut, synchronization and state boundary, then verify actual delivery and interrupted paths through [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md). |

For animation, verify the source rig and actual runtime composition. Check contacts and weight
transfer, joint deformation, foot/hand sliding, unwanted pops, clipping and prop relationships.
Test relevant blends and entry/exit transitions, not only an isolated clip. Inspect each temporal
boundary on both sides and through the transition. Static key poses cannot prove convincing
movement. See [ANIMATION.md](ANIMATION.md) for the motion workflow and [CHARACTERS.md](CHARACTERS.md)
when several character systems must play together.

For production living characters, **include a natural-motion pass without waiting for a separate
request for polish**: appropriate breathing, moving holds, weight shifts, attention and secondary
response. Inspect existing motion first, then author what the character's state, reference and
camera require. Preserve contacts and purposeful stillness; document intentional restraint and
report missing rig support or unverified behavior. Follow the
[natural-movement workflow](ANIMATION.md#natural-movement-is-part-of-the-performance), including
its checks on combined playback. A static solver fixture can isolate a problem, but cannot by
itself establish a finished character performance.

For water, follow [WATER.md](WATER.md): finish the optical response, flowing normals, foam and
shoreline treatment together, then verify the actual motion and interactions. A blue surface or
an attractive still image does not establish a convincing water material.

Translate the seam/detail coverage list into the medium's own parts: joints and transitions in
motion, emitters and lifecycle phases in effects, UI states, audio layers and loop joins. Give each
item a planned observation, current evidence, findings and pass/repair/unverified status in the
owning build guide. Judge the final combined experience too: good isolated components can still
produce incoherent timing, density or style when assembled. Never claim playback, listening or
interaction validation from an unviewed capture, waveform, screenshot or successful build.

The straight-edge/simple-form rule concerns intentionally designed visual form. Apply the same
principle to motion and other media: choose paths, easing, holds, rhythms and transitions for the
result, instead of accepting defaults without inspection. A deliberately rigid machine, still pose,
linear travel or quiet moment may be exactly right. Do not add wobble, easing or ornament everywhere
merely to increase apparent complexity.

## The workflow for constructed 3D assets

The sections below expand the physical construction workflow. For another medium, use the common
cycle above and its specific authoring guide; carry across the quality, planning, theme and evidence
requirements without inventing geometry or socket steps that do not apply.

```text
1 brief ............ what it is, who sees it, from where, what it does
2 references ....... images first; no geometry without them
3 plan big ......... a build guide: reference-to-construction table, part map, dimensions, order
4 scope down ....... whole -> sections -> parts -> details; one representative part first
5 build ............ each part at final quality, compared with the reference after every revision
6 assemble ......... sockets, clearances, overlap and support checks
7 finish ........... materials and wear at two scales
8 prove ............ renders against the reference, motion, reload, hand-over
```

Each stage ends at a gate. Do not start the next stage until the gate is met. Show the owner the
results at the cadence the project agreed in setup. How the thing looks is the owner's call.

## 1. Brief

Write down, before anything else:

- what the asset is and what it is for in the game;
- who sees it, from where, and how close: player height, camera distance, close-up or background;
- what moves, what opens, what the player touches, what it attaches to;
- the style (realistic, stylised, a named art direction) and the materials;
- the governing theme references and how this asset will exceed the minimum brief in craft and
  purposeful detail, or the owner's explicit request for simplicity;
- rough size, and the performance budget if one exists;
- which GraphCode product owns each kind of part (Shape, Garment, Groom, Rig, Human, Move, a material).

Keep it in the asset's folder under the project's design docs, for example
`<docs>/Design/Assets/<Name>/BUILD-GUIDE.md`. Images and prompts go in the same folder, never in
`Content/`.

## 2. References first: no geometry without them

Get the images before you write a single shape.

- **Where they come from:** the owner supplies them, or you generate them with the project's chosen
  2D service or the host's built-in image tool. Use an already authorized service and budget;
  obtain authorization for new paid usage when needed. See [content sourcing](CONTENT.md).
- **What to get:**
  - a main three-quarter concept;
  - front, side and back views for proportion;
  - close-ups of every distinctive part: joints, edges, fasteners, the face, hands, materials;
  - for a character, a turnaround of the whole figure and a view of each layer: body, under-clothing,
    armour, hair, carried items.
- **Change the design in the picture, not the mesh.** It is much cheaper. Number every version
  (`v01`, `v02`, ...). Save a `PROMPT-vNN.md` beside each image with the exact prompt and what
  changed, and why. The owner picks the version that governs; write down which one it is.
- **A generated image is a concept, not a drawing.** Do not copy its perspective errors or unreadable
  details. Resolve how things really connect in the written plan.
- **Open the actual image file every time you compare.** Never compare from memory.

Gate: a governing reference set exists and the owner has seen it.

### Dedicated part references for assembly

A whole-object concept often hides the very surfaces needed to build and attach a part. Prepare
dedicated reference images for distinctive, complex or unclear parts **before modelling them**.
Do not force every part out of a crop of the overall picture. Reuse a proven sheet for repeated
parts; a separate generation for every identical fastener adds no information.

First name the part and write what its reference must resolve. Generate or draw a part sheet using
the governing whole-object image and shared style/proportion references as inputs. ImageGen,
Imagen or an equivalent tool or authorized image-generation API can supply these images; follow
[the image workflow](UI-TOOLS.md#make-generated-artwork-serve-the-interface) for reference inputs
and provenance, and [the API route](UI-TOOLS.md#api-based-image-generation-is-a-supported-route)
for service handling.
Use supported image-conditioned editing when available; an unrelated text-only prompt can drift
from the design. Keep the source prompt, reference inputs, model/settings and chosen version.

Include the views needed for its construction and assembly:

- **In context and isolated:** identify the part on the whole object, then show its own silhouette,
  profile and distinctive details at a useful size. Keep front/side/back or orthographic views
  consistent; add a three-quarter view when it clarifies the form.
- **The hidden interface:** show the underside, mating face, opening, collar or mounting features
  that the main concept conceals. Include the neighbouring part and an exploded or sectional view
  when needed to explain how the connection works.
- **Assembly intent:** label the intended root/pivot, socket direction, contact region, clearance,
  motion and which surfaces remain separate or become continuous. Add these annotations from the
  written construction plan; generated labels are not measurements or authoritative geometry.
- **Shared proportions and finish:** preserve the master design's scale relationships, axes,
  material language and neighbouring interfaces across every part sheet.

A useful generation brief names: the governing reference, exact part, purpose of each requested
view, features to preserve, mating neighbours and open questions to answer. Inspect the actual
output. Correct inconsistent views or impossible connections before construction. An image is
visual guidance; derive dimensions, socket frames and clearances from the shared dimension source,
and record any intentional design decision needed to interpret it. Do not infer exact dimensions
or hidden topology from plausible shading.

Link the selected sheet from the part map. A part is ready to build when its appearance **and its
interface to the assembly** are understood. If a connection remains ambiguous, follow the
[per-step planning check](RESEARCH-AND-PLAN.md#4-check-each-step-for-ambiguity-before-doing-it)
before modelling or generating the production part.

## 3. Plan big

Write the build guide before building. It holds four things.

**A reference-to-construction table.** For every feature you can see in the reference, write the
construction that answers it and what would *not* be good enough. This is where "resembles the shape"
gets caught before it is built. From a real steampunk tower:

| Visible in the reference | Construction that answers it | Not acceptable |
|---|---|---|
| Paired pressure vessels | Separate domed heads and barrels, raised seams with seated rivets, straps, sight gauges, connected pipes | Smooth enlarged cylinders |
| Heavy pipe elbows | Continuous bends with collars, paired flanges, bolt heads, supports | Plain tubes with one ring at each end |
| Curved terrace | Layered slab edge, riveted fascia, repeating brackets, railing, a visible load path | A flat disc with a thin rail |
| Arched openings | Deep openings, several framing layers, sills, mullions, glass set back | Flat coloured rectangles |

**A part map.** Every part, which product owns it, whether it is its own asset or part of another,
its selected whole/part reference, what moves, and where it attaches (a named socket, a ring on a
body, a bone). For each connection, record the intended topology, attachment or `add` join choice,
why it fits, and how the fit will be checked. Resolve ambiguous interfaces before building either
side; see [join selection](#choose-the-join-for-each-add).

**One owner for the dimensions.** One file of named measurements: sizes, stations, clearances and
datums. Every other file imports it and derives from it. Human-scale sizes stay real whatever the
object's size: a 1.1 m railing, a 0.16 m stair rise, a door a person fits through, a seat a body fits in.

**The order of work, and what proves each stage.** For example:

| Stage | Build | Proof before moving on |
|---|---|---|
| 1 | A representative part at final quality | Close and whole-object renders beside the reference |
| 2 | The part that moves or fits, with its neighbours | Poses checked, no contact where there should be clearance |
| 3 | The main body or sections | Front, side, back and three-quarter renders against the reference |
| 4 | Visible systems and connections | Every connection traced end to end, supported, no floating parts |
| 5 | Materials and wear | Distance and close renders under the game's lighting |
| 6 | Hand-over | Reload, bounds, pivots, sockets, collision; what is unfinished listed |

Also list the precedents you will reuse ([section 6](#precedents-first)) and any open questions for
the owner.

Include the planned enhancements and a render coverage list in this same guide. Name each part,
seam, connection and distinctive detail, its purpose in the theme, the intended construction,
and the views needed to judge it. Include rear faces, undersides and occluded interfaces. Use this
list to prevent an impressive front view from concealing unfinished construction elsewhere.

Gate: the owner has seen the references and the plan, at the review cadence setup agreed.

## 4. Scope down, and build one representative part first

Go from the whole to the small: the whole object, its sections, each section's parts, each part's
features, then the fasteners and finish. Big assets are built as sections with a defined interface
between them, and assembled at the end.

Before building everything, pick the **hardest or most repeated part** and build it to final quality:

- a vehicle: one wheel and one riveted curved panel;
- a building: one bay with its window, frame, trim and fittings;
- a warrior: one shoulder, meaning the under-layer, the shoulder plate, its strap and the rivets;
- hair: one side of the head, from parting to ends.

This part sets the standard for the rest: rivet size and spacing, edge treatment, how plates overlap,
how materials respond to light. Show it. Fix it. Only then repeat it.

## 5. Build each part at final quality

### Design the form, including every contour

**Never use a straight edge, simple line or simple sphere as the finished answer unless it is
explicitly designed that way.** The same applies to other elementary geometry. A primitive can be
the starting construction operation; develop the final form from the reference and the part's
purpose. A sphere standing in for a head, a ball for a hair bun, a plain tube for a complete pipe
system, or a straight slab for an ornate bracket does not meet the bar.

Plan the silhouette, cross-section, curvature changes, thickness and transitions before modelling.
Where the design calls for them, use shaped profiles, taper, shoulders, recesses, layered rims,
purposeful bends, designed edge treatments and connections that explain how the part is made.
Organic forms need their characteristic volumes and landmarks. Repeated parts should retain
the same intentional form language. A bevel on an otherwise undesigned primitive is not enough.

A simple sphere, straight rail, flat panel or crisp edge is valid when the user, governing reference
or documented design specifically calls for it. Record that choice and its visual or functional
reason in the part plan. Verify it in the reference and render; do not label every shortcut
"intentional" after the fact. Preserve straightness or flatness where a designed mating surface,
mechanism or theme needs it. Arbitrary curves and random distortion can damage the design just
as much as unfinished primitives.

### Detail at three scales

Before finishing a Shape part, choose its material-appropriate
[curved bevel, round-over or deliberate chamfer](SHAPE-EDGE-FINISH.md). Tables,
building trim and upholstered furniture must not inherit unfinished primitive edges.
Protect mating faces and recheck the assembled seam after rounding.

- **Large: the silhouette.** Unequal masses, setbacks, projections, parts that break the outline.
  This is what reads from far away. A dark line painted on a flat wall does not break a silhouette.
- **Middle: the construction.** Panels, straps, ribs, flanges, frames, seams with backing, rolled
  edges, layered plates. This is the scale most often missing, and it is what makes a thing look made.
- **Small: the finish.** Fasteners, stamped marks, stitching, wear, chips.

Rules that came from real rejections:

- **Break smooth shapes with construction.** A large surface needs seams, straps, ribs, access
  covers, layered plates or connected machinery that read at whole-object distance. A row of rivets
  round the edge is not enough.
- **Keep quiet areas.** Rich does not mean covered. A few large purposeful features beat many small
  ones; "too many small gears read as messy" was a real rejection.
- **Every fastener sits on a real joint.** Rivets join plates along seams and straps; bolts hold
  covers that come off. Size them by job, not all the same.
- **Give members a real section.** A shoulder, a recess, a rolled or stepped edge, a cap. Not a box
  made wider.
- **Variation is controlled.** Use seeded, bounded variation where the material has it, such as cast
  metal, stone or wear. Keep mating faces, axles and clearances exact.

### Repeated render validation of every seam and detail

Render your result at the same angle and similar framing as the reference. Open both and compare,
in this order: silhouette, proportions, distinctive shapes, construction, materials, close-ups. Write
the differences down and fix them. Only the owner can call it reference quality.

**Build, render, inspect, repair and re-render throughout production.** Run the loop on the
representative part, each new part or detail group, each join or attachment, the assembled sections,
and the whole asset. Repeat it after changes that affect appearance, fit, materials or motion.
Do not postpone seam inspection until a final beauty shot, or count a generated image as inspected
before actually opening it.

Use the build guide's coverage list to account for **every seam, connection and authored detail**.
One frame can cover several items only when each can actually be judged at that framing. Repeated
instances may share a recipe, but their placement and contact still need inspection throughout the
assembly, including corners, endpoints, transitions and places where the receiving surface changes.
An occluded seam needs an isolated, sectioned or exploded diagnostic view when supported, followed
by a render of the restored final assembly. Do not remove an obstructing part and then claim the
intact assembly was verified.

| View or pass | Inspect |
|---|---|
| Whole asset at the game camera | Theme fit, silhouette, proportions, focal hierarchy, readability and whether added detail improves the result. |
| Each part and detail group, close enough to judge | Actual profiles, thickness, edge treatment, fasteners, ornament, readable intended markings and material finish. |
| Silhouette and contour review | No bare primitive, straight strip, simple sphere or unshaped edge remains without an explicit design reason; the chosen profiles and transitions read in the final asset. |
| Every seam and attachment, from both useful sides | Intended continuity or separation, gaps, penetration, floating parts, support, exposed caps, join shape and neighbouring clearance. |
| Grazing views and a moving camera | Coincident surfaces, flicker, shading discontinuities, unexpected facets and unstable fine detail. |
| Neutral diagnostic lighting plus the game's lighting | Geometry/shading defects, UV or texture discontinuities, material transitions and consistency with nearby assets. |
| Motion through rest, extremes and intermediate poses | Opening seams, clipping, stretching, sliding attachments and lost clearance. |

Keep a compact evidence table beside the part map:

| Part / seam / detail | Required view or pose | Latest inspected capture and source revision | Finding / repair | Status |
|---|---|---|---|---|
| Named item or explicitly enumerated group | What makes this feature judgeable | Actual file/link and the asset state it shows | Observed issue and corrective action, or why it passes | Pass / needs repair / not inspected |

After a repair, re-render the same view and check the affected neighbours and whole-object read.
After a later change, reopen any check that depended on the old geometry, placement or material;
an older successful render cannot certify the changed asset. Continue until the applicable checks
pass. If a render or required inspection is unavailable, retain it as unverified and finish useful
independent work; do not claim that seam or detail is finished. Once the current result passes,
keep the evidence and proceed; repeated validation should answer changes or unresolved findings.

## 6. Shape: precedents and high-level intent, never low-level execution

The owner's rule for every `.gcshape`, 2026-09-28:

> "make all gc shape uses precedents and high level intent, never low level execution"

In practice:

- **Reuse before writing.** Build from existing recipes and presets.
- **Say what you mean.** State the relationship ("sits on", "twelve round the rim", "as wide as the
  shaft's top") and let Shape work out the numbers.
- **No hand-worked numbers.** Never compute coordinates by hand, type out vertex lists for regular
  features, repeat near-identical lines, or use numbers nobody chose.

### Precedents first

Look in this order, and use the first that fits:

1. **GraphCode Marketplace.** Find the closest precedent, inspect its construction, and adapt
   the recipe or assets where the design needs a change. [Market guide](MARKET.md).
2. **This project's own recipe families.** A part the project already built is the house style. Find
   them with `gc_search`.
3. **The shipped preset library.** Search it with `gc_search kind:"preset" target:"<word>"`. It
   returns each entry's parameters, import line and call line. It holds more than 500 entries,
   including:
   - `Mechanical`: Rivet, Flange, Collar, Boss, Gusset, Bracket, PipeRun, PipeElbow, PipeTee,
     Spring, ChainLink, Handle, Grille;
   - `Gears`: spur gears, pinions, racks, pulleys, sprockets, ratchets;
   - `Masonry`: bricks, quoins, paving, kerbs, roof tiles, slate, voussoirs, corbels;
   - `Fixings`, `Profiles`, `Solids`;
   - `Wear`, `Surface` and `Damage`: erosion, chips, dents, grooves, cracks, spalls;
   - `Organic` and the anatomy presets.
4. **A new shared family.** If nothing fits, write the part as its own parametric recipe with named
   inputs and sockets, so the next part reuses it rather than redrawing it.

An import line can override a preset's values, so one preset becomes a family of sizes:
`Fixings = shape('/GraphCodeShape/Presets/Fixings', Thread_R = 0.5)`. Read a preset's own comments
before relying on it, and check its built result: presets are code too, and can have faults.

### Say what you mean

| You mean | Write | Never |
|---|---|---|
| sits on / against / at the corner of | `at=on()`, `at=against($Handle, +X, gap=)`, `at=edge(...)`, `at=corner(...)` | a hand-computed `at=(x, y, z)` |
| N evenly round, along, across or over | `repeat=` with `spread=`, `step=`, `around=`, `along=$Path` or `onto=` | N nearly identical `add` lines |
| as wide as that part, or that part at a height | `span($Part, X)`, `section($Part, X, along=Z, at=80%)` | the width typed again |
| this attaches to that | `socket(Name, ...)` on the real mounting face, zero offset in the assembly | a transform copied into a Blueprint |
| the same thing in another size | one parameterised entry called with values, or an import override | a copied entry with edited numbers |
| this depends on that | an expression from the named owner (`$Radius * 1.04`) | the resolved number |

`gap=` is signed: positive stands a part off, negative sinks it in. Placement relations measure
bounding boxes, so on a curved, tapered or roughened surface a part can float over the real surface.
The build note says so ("NOT TOUCHING"). Sink the part in with a negative `gap=`.

### Choose the operation for the form

`rect`/`circle` + `extrude` + `add` is the blockout vocabulary. A finished part made only from those
fails the bar, unless the real object is that simple. Ask `gc_describe kind:"shape" block:"<word>"`
for each word before using it.

| Form | Reach for |
|---|---|
| Turned parts: knobs, vessels, domes, lamp housings | a profile `curve`, then `revolve` |
| A thin open curved plate: pauldron cap, mudguard, vessel head | an open profile `curve`, then `revolve(capped=false)` (a surface), then `shell(<thickness>)`; a rolled edge is a bead welded at the rim |
| Curved panels, mudguards, plates following a curve | a profile `curve` swept along a path (`sweep`), or `loft` between sections |
| Pipes, rails, straps, cables | `sweep` along a 3D `curve`, or the `Mechanical` pipe presets |
| Tapering members, horns, blades | `taper`, `loft` |
| Bent arms and strips | `bend`, or `sweep` along the bent path |
| Rounded or rolled edges | `round` on the mesh edges; `round` on outline corners |
| Openings, recesses, grooves, pockets | `subtract`, `cut`, `trim` |
| Walls with thickness, hollow bodies | `shell` (negative hollows inward and keeps the outside) |
| A band or fin from a line | `widen(by=, about=)` |
| Cast, worn or natural surfaces | `roughen`, `displace`, `raise` and the Wear/Surface/Damage presets, after the construction is right |
| Liquid run down a surface: glaze, wax, paint, slime | `drip(repeat=, length=, width=, variation=) [<the liquid>]` — runs follow the surface from the liquid's lower edge; a drop hanging off an edge is still an `add` along that edge |
| Softened joins between parts | `add(join=fillet, radius=)`, `add(join=blend, falloff=)` |
| Organic parts meeting, such as a limb into a torso | `add(join=organic, ...)`: see [organic part assembly](ORGANIC-PART-ASSEMBLY.md) |
| Fine grilles and meshes | geometry for the frame, rolled lip and bars; a masked material for the fine mesh |

Traps measured on the current build, 2026-09-28:

- `revolve` closes an open profile to the axis unless you write `capped=false`. `shell` on the
  closed result gives a sealed hollow body (`parts 2`), not an open plate. The plate is
  `curve(...) revolve(capped=false) shell(0.25)`: measured at 9 cm across, 5 cm high and 0.25 cm
  thick, it builds as one closed part with nothing folded or crossing, and stays clean with a
  0.4 cm rolled bead welded round its rim.
- A profile widened into a thin band, then revolved, gives a real open plate in one piece.
  ~~With square ends it came out inside-out (negative volume).~~ Fixed 2026-09-28: `revolve` now
  turns either winding outward, as `extrude` and `sweep` already did. ~~With `ends=round` it still
  reports 45–53 real folds in one ring of the plate at a fine `smooth:`; use the route above.~~
  Fixed 2026-09-28 (MB54): `ends=round`, `square` and `joined` bands are now as wide as `by=` asks
  (they were half), a round end is round, and the revolved band folds nowhere at any `smooth:`.
- In `cut`, `at=` is measured along the plane's own direction. With `plane=-Z`, `at=1%` is near the
  *top*.
- ~~The shipped `Solids.Dome` sits its base at z = (r − h) / 2 instead of 0 whenever `h` is not `r`,
  so `Mechanical.Rivet` builds as two loose pieces.~~ Fixed 2026-09-28: the dome keeps its flat face
  on z = 0 at every `h`, and the rivet builds as one part.

Read every build line: `parts`, `open`, `folds`, `crossings` and `coincident` appear only when they
find something, and `volume` is always there (negative means inside-out). Then check the rendered
result.

### Numbers

- Every number is either **an authored choice** or **derived from one**. An authored choice has a
  name and a value a person would pick: `Wheelbase = 290`, `Plate = 0.25`, `Radius = 7.1`.
- A long decimal like `$Width * .784377367` means someone worked backwards from an absolute
  measurement. Name the measurement instead, and derive the rest from it.
- Measured points (a ring on a body, a hairline) are written once, named, and reused through their
  names.
- Shape control points and deliberate proportions (`$Radius * 0.8`) are fine as they are. The test is
  whether the value should follow another fact or be adjustable, not whether a number appears.
- The shipped `HandAnatomy` and `AnimeBody` presets are unfinished studies full of back-computed
  ratios. Use them as parts if they fit; do not copy their style.

### Repeat with patterns, never by listing

A regular array is one line: `add($Bar, repeat=24, around=Z, spread=360)`,
`add($Brick, repeat=(20, 12), spread=(820, 480), stagger=50%)`,
`add($Rivet, along=$Seam, repeat=18)`.

When copies differ, use `loop (...) { }` with `output.index`, or per-copy parameters. Writing 82
separate grille bars was a real rejection, and unioning 82 solid bars stalled the editor for five
minutes.

## 7. Parts must not share a surface

### Choose the join for each add

**Choose `add(join=...)` from the intended connection; never apply `join=none` everywhere by habit.**
The same is true of blindly applying `hard` or `organic`. Decide whether the geometry must stay
separate, merge with a crease, soften at a seam, blend across a volume, or fit against another
surface. Use separate assets and socket/component attachments when parts must move independently;
`join=none` alone only preserves separate shells inside one output mesh.

| Intended result | Candidate join | What must be understood before using it |
|---|---|---|
| Separate shells, such as layered or individually seated details | `none` | Appends geometry without merging. Check contact, support, exposed overlaps and whether separate assets are actually needed. |
| One solid with a sharp junction | `hard` | Boolean union of suitable closed solids; inspect the junction. Explicit `hard` refuses an open-surface case instead of silently appending it. |
| A rounded edge along a merged junction | `fillet, radius=...` | A seam fillet with radius in cm; the local geometry must have room for the rounding. |
| A soft continuous volume | `blend, falloff=...` | Distance-field reconstruction with falloff in cm. The whole result is retessellated; fine detail/topology can be lost and a small falloff can be expensive. |
| Trim the incoming solid against an unchanged receiver | `clip` | Directional: cuts only the incoming closed mesh, retaining the larger portion, open at the contact. A tie or mere touch is refused. |
| Extend an incoming end to fit an unchanged receiver | `full` | Directional fitted contact: needs one supported connection direction and a planar incoming end; the opposite end stays fixed. The result is open at contact. |
| A reconstructed transition between prepared organic parts | `organic` | Requires the typed socket/transition contract and compatible input geometry. Follow [ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md), including its current validation limits. |

This is a choice guide, not a replacement for current tool help. Load the installed `add` grammar
for permitted placement keys, operand requirements and parameter combinations before writing it.
For example, `clip`/`full` accept an open receiver only when it is planar; `full` does not support
arrays, and neither produces a welded seam. `blend` does not support seam capture. Do not transfer
options from one join kind to another or replace a refused join with `none` just to make it build.

Record the choice once per connection or repeated connection family in the part map, then use it
consistently in the recipe. Write the join explicitly where the intended topology matters. Prove
one representative connection, checking silhouette, contact, topology findings, UVs/materials and
the game camera, before repeating it across the asset.

### Check the resulting surfaces

Nearly coplanar visible surfaces can compete in the depth buffer and flicker as the camera moves.
This can happen in any orientation, including across separate components and actors. Opposing
faces making a legitimate contact are different from two overlapping outward-facing skins.

- **Inside one mesh, `add` defaults to a hard union for suitable closed solids.** An omitted join
  can fall back to appending open surfaces; write `join=hard` when a weld is required and check the
  result. Measured: a plate set flush into a wall, welded, builds as `parts 1, crossings 0`.
- **`join=none` keeps both surfaces.** Use it only for parts that must stay separate shells, and only
  where no visible face of one lies in or right beside a visible face of the other.
- **Design the seat.** A supported part can meet opposing faces, seat into a designed recess or
  stand proud by a deliberate step. An inlay needs its pocket. Record intended insertion; hiding
  one problem by forcing arbitrary penetration can create another.
- **Separate assets follow the same surface-ownership rule.** Do not open a visible gap in every
  contact to avoid flicker. No universal offset guarantees all cameras and materials; inspect the
  actual visible overlap, depth, backing and intended contact.

What the build tells you, and what it does not:

- **A build line names a problem only when it finds one.** `open`, `seam`, `parts`, `folds`,
  `crossings` and `coincident` are absent from a clean line, and the result's first line lists every
  build line that found something: `2 of 3 with findings: SM_Flush (parts 2, crossings 7,
  coincident 1); SM_Proud (parts 2, crossings 7)`.
- **`coincident N` flags possible flicker.** It counts places where two separate shells lie on each other,
  facing the same way, within 0.01 cm, where a camera can see them. The note names both parts, the
  area, the facing and where: *160 cm2 facing +Z at (0, 0, 4), flush, between '$Nameplate' and ...*.
  Visibility is sampled; it is not a complete guarantee for all views, materials and deformation.
  A welded part, a part standing 0.1 cm proud, a part seated face-down on another, and a flush pair
  sealed inside a third solid all read nothing. Measured 2026-09-28 on a cover plate with a nameplate.
- **`crossings` cannot tell a flickering overlap from a harmless one.** Measured 2026-09-28: a plate
  buried flush in a wall (flickers) reported 19, and a plate standing 1 cm proud (does not flicker)
  reported 22. Read it as a list of places where solids pass through each other, and read
  `coincident` for flicker. The note gives each place as `N at (x,y,z) measuring a x b x c` (the
  largest four, with the rest counted), not one box over the whole shape, so each can be looked at.
- **`folds` separates real folds from creases.** The note lists the triangles that pass through the
  surface (the real folds) and the creases in two cluster lists, real folds first, so a small
  pass-through is never lost at the tail of a list the creases fill. Creases are sound: two faces meeting
  at less than a right angle look like a fold to a local test.
- **A `round` that skips corners says so.** When some outline corners round and others cut in by less than
  the header's `smooth:`, the note reads `skipped K of M corners too small to round at smooth: X`;
  lower `smooth:` below the shallowest depth it shows to round them too (every arc gets finer). A `round`
  that skips every corner is refused, naming the same lever.
- **A Blueprint write compares its components.** Any `gc_apply` that writes or creates a Blueprint
  measures its static-mesh components where the class places them, and ends its message with the
  places two of them coincide: `coincident 1: 800 cm2 facing -Y at (0, -50, 12), flush, between Crate
  and Nameplate`. It does not yet see components added in C++, an inherited component a child
  Blueprint overrides, or attachment to a named socket. For those, and for anything placed in a
  level, render close-up at a grazing angle and move the camera: flicker shows as shimmering stripes.

Every assembly also needs two checks. **Intersection**: no unintended solid penetration. **Support**:
no part or island floats without a physical connection to what holds it. Passing one does not prove
the other.

Run [assembly validation](ASSEMBLY-VALIDATION.md), including its **paste-ready editor checker**,
on actual placed components and neighbors. Declare intentional solid seats, inspect the full
contact region and retain incomplete coverage. Blueprint hierarchy and sockets alone prove no fit.

## 8. Assemble

Attach parts at named sockets on their real mounting faces, with zero rest offsets. Pose anything that
moves: rest, the extremes, and a few points between. Check clearance in every pose. Then render the
whole object against the reference from the front, side, back and three-quarter view, from the
player's distance and close up.

## 9. Finish: materials and wear

- Give each material its role: distinct metals, satin broad surfaces with selectively polished
  fittings, leather, cloth, glass, rubber.
- Material noise does not replace missing geometry or hide a poor joint.
- Wear works at two scales. **Macro**: broad, uneven variation that reads from far away. **Micro**:
  grain, scratches, chips. Put it where use and weather would: contact edges, under overhangs, around
  serviced parts. Leave clean stretches. Even dirt everywhere was a real rejection.
- Bake surface attributes without baking a fixed scene's highlights and shadows into base color.
  Material/texture baking and light baking are different operations; generated painted lighting
  is not a substitute for a surface that responds correctly to the game's lights.
- Judge the result under the game's own lighting.

To build tiling materials, master materials and instances, texel density and world-scale UVs,
follow [MATERIALS.md](MATERIALS.md).

For landscapes and other large repeated surfaces, follow [TEXTURE-VARIATION.md](TEXTURE-VARIATION.md).
Consider UV variation, reusable near/far material functions, camera-depth blending and macro masks;
verify physical scale, normal direction, seams, motion and cost before expanding production.

### Surfaces come from texture art, never from an algorithm

Owner, 22 November 2026: **"Stop implementing algorithm based texture. Never do that."** A surface is
real texture art: scanned or generated maps released for the project, in the material packs. Do not
compute its look in a shader: no procedural growth rings, noise stains, cell patterns or Custom-node
"wood" or "stone".

- The shape author's part is geometry, material slots and UVs. Lay the UVs in the layout the material
  was made for, such as a centred planar cap or a tile along the grain.
- If a needed surface does not exist, or its material needs a projection Shape cannot write, ask the
  texture lane for the art, or add the projection to the grammar. Do not stand in for the art with math.
- Material parameters that tint, blend or mask released maps stay allowed. Use them for wear and age
  where a mask needs them.

### Realistic surfaces carry believable imperfections

Standing user direction, 2026-09-28:

> "adknowledge any realistic and non stylized shapes are uneven, imperfect etc. in design so
> things should be, either use geometry to represent or material + bake or decal + bake"

**For realistic work, account for irregularity in the form and surface rather than leaving an
ideal primitive with a uniform finish.** Natural growth, fabrication, assembly, handling and
weathering leave different kinds of variation. Use references to choose their scale, direction,
amplitude and distribution. Stone can have uneven strata and chipped edges; timber can have grain,
small warps and tool marks; cast metal can have subtle waviness and pits; skin has asymmetric
volumes and pores. A new painted part can have slight finish variation without being damaged.

Realism does not mean every surface is visibly bent, dirty or worn out. Precision-machined parts
can read as straight, round or flat at the game camera, with imperfection primarily in their finish.
Preserve mounting faces, socket frames, seals, contact areas and functional clearances. Scale
irregularity to the material, manufacturing process, age, maintenance and reference. Keep quiet
areas and distinct wear zones; uniform noise or identical damage across every copy looks artificial.
Stylized work follows its chosen form language instead of inheriting photorealistic noise by default.
Use the [macro, meso and micro surface workflow](TEXTURE-VARIATION.md#plan-macro-and-micro-imperfections-beyond-terrain)
for material-specific dirt, wear and finish across terrain, props and other textured assets.

Plan broad form variation, local damage or manufacture marks, and fine surface response together.
Use bounded, reproducible variation where appropriate and protect the assembly interfaces. Decide
the representation for each feature before authoring:

| What the imperfection must affect | Representation to consider | What to verify |
|---|---|---|
| Silhouette, visible thickness, a deep chip/dent, contact or a physically raised edge | Geometry, with any required collision updated separately | It reads from relevant angles, preserves support/clearance and does not break shading or deformation. |
| Fine relief that need not alter silhouette or contact | A detailed source baked to normal/height maps, using height only through a supported shader/displacement path | Correct normals, scale and viewing-angle behaviour. A normal map alone does not move the silhouette or collision. |
| Grain, pores, fine scratches, subtle waviness, roughness/color variation or layered finish | Material layers/procedural masks, then bake the needed surface channels where the pipeline calls for it | Physical feature scale, appropriate channel behaviour, tiling and a faithful result on the final mesh. |
| Local stains, scuffs, paint loss, repair marks or other placed surface details | Decal/stamp or masked material layer, then bake into the asset's supported texture/material outputs | Placement, mask edges, channel blending and preservation of the mark in the actual bake. |

These routes can be combined on one asset: a chipped silhouette in geometry, shallow surface relief
in a normal map, and localized wear authored as layers or decals and baked into the final finish.
Choose by the feature's visible effect and target cost, rather than making every imperfection a
mesh or flattening all of them into base color. Epic's [PBR guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/physically-based-materials-in-unreal-engine?application_version=5.7)
explains using roughness variation for scuffs and smudges; its [modeling tools reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/modeling-tools-in-unreal-engine?application_version=5.7)
describes transferring source-mesh details into baked textures. Read the version that matches the project.

### Prove the material or decal bake

Before choosing a bake route, check installed capabilities and define the source layers, receiving
mesh, UVs, output channels, resolution and expected visible result. Material flattening, baking
high-resolution geometry and capturing a projected decal are different operations; do not assume
one baker performs all three. Unreal's [decal system](https://dev.epicgames.com/documentation/en-us/unreal-engine/decal-materials-in-unreal-engine?application_version=5.7)
has its own material and receiving-surface behaviour. A decal visible in the scene is not proof it
will be included in a material bake, and an Unreal decal is not proof of support for a Shape
`decal()` recipe. Read current tool help. If the intended route is unavailable, research and plan
a supported external bake or equivalent material-layer workflow that preserves the result.

Keep editable geometry, source materials, masks and decal placements. Bake one representative
patch first and inspect the generated maps on the actual destination mesh. Check channel/color-space
settings, normal orientation and tangent consistency, UV seams, padding, projection leaks, repeated
patterns and loss of detail at the intended viewing distances. Then compare source and baked results
under the same camera and lighting, followed by changed lighting and grazing views. Confirm roughness
and relief survive the bake rather than becoming painted shadows. Fix and rebake affected outputs;
record their current evidence in the seam/detail coverage list before treating the finish as done.

## 10. Characters: plan the layers from the skin out

A character is several assets from several products. Plan all of them before building any of them.
For a warrior:

```text
warrior
├─ body and head ........ a MetaHuman (.gchuman) or a custom mesh with .gcrig   -> CHARACTERS.md
├─ under-layer .......... shirt, gambeson, trousers: .gcgarment with thickness  -> GARMENT.md
├─ mail ................. a .gcgarment wearing a mail material                  -> GARMENT.md
├─ plate and hard parts . .gcshape parts placed on the garment's rings          -> GARMENT.md, armour
├─ straps, belts, buckles .gcgarment straps + .gcshape buckles and rivets      -> GARMENT.md
├─ tabard, cape, skirts . .gcgarment (skinned, or simulated with [as=cloth])   -> GARMENT.md
├─ hair, brows, beard ... .gcgroom                                              -> GROOM.md
├─ weapons and gear ..... .gcshape assets attached at sockets or bones
└─ motion test .......... idle, walk, run, attack, and the extremes            -> CHARACTERS.md
```

Decide early what the helmet does to the hair, which body regions the armour hides, and what must
bend: shoulders, elbows, hips and knees decide how plates are split.

Build the clean body first. Then build the representative piece: one shoulder with its under-layer,
plate, strap and rivets, posed with the arm raised. **Armour is a garment**: follow
[Garment's armour section](GARMENT.md#armour-is-a-garment).

## 11. Prove it and hand over

Before you call a part or an asset finished:

- renders beside the governing reference: front, side, back, three-quarter, and close-ups of every
  distinctive part;
- views from the player's distance and from the closest camera;
- a grazing-angle flicker check, the intersection check and the support check;
- moving and deforming parts in their poses, and a character in its animations;
- the seam/detail coverage list complete for the current asset, with repairs re-rendered and no
  uninspected item silently marked as passed;
- the whole asset judged beside the project's other assets, with the planned enhancements clearly
  visible and consistent with its theme, or its explicitly requested simplicity;
- saved, reloaded and read again;
- the build guide's status updated: what matches the reference, what does not yet, and what is not
  built.

Present the whole asset and selected close-ups that show the extra craft, along with the evidence
for its connections and finish. Report differences plainly. "Built" and "matches the reference"
are different claims, and only the owner accepts the second. Describe the work and observed quality;
the owner's reaction determines whether it surprised them.
