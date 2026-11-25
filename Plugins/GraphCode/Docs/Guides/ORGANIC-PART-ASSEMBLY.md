# Assemble supplied parts with Shape Organic Join

Assemble meshes in GraphCode `.gcshape` recipes. Do not join, union or assemble the character in
Blender as a substitute for its GraphCode recipe. Blender may inspect or prepare individual source
assets; import those parts and preserve the assembly's sockets, bands and operations in GraphCode.
Generated sources below include meshes made with an AI image- or text-to-3D model, the recommended route for organic
forms (see [CAPABILITY-MAP.md](CAPABILITY-MAP.md)), as well as assets the user already supplied.

Use this guide when a model is generated or supplied as separate meshes and the intended result is
one repairable organic source. The common character route is:

```text
3D-AI part sources
├─ core body or torso/pelvis receiver
├─ head
├─ left/right arms and legs
├─ hands and feet, when separate generation adds value
└─ optional anatomical or creature parts
     ↓
normalize, inspect and preserve every source
     ↓
import as static meshes
     ↓
.gcshape receiver sockets + transition bands
     + incoming root sockets
     ↓
add(..., join=organic, into=..., using=..., fit=frame)
     ↓
clean combined body/head source
├─ keep custom → .gcrig → .gcmove
└─ fit identity → .gchuman reference → native MetaHuman body + face
     ↓
hair, brows, clothing and accessories remain separate
```

The result is a construction source, not necessarily the final runtime character. Organic assembly
solves prepared connections. It does not turn unrelated meshes into a coherent person without a
shared scale, pose, proportions, connection plan and visual review.

## 1. Decide what the combined mesh will own

Choose this before generating parts:

```text
Must the generated topology, silhouette or non-human anatomy survive?
├─ yes → combined mesh remains authoritative
│        └─ build a custom .gcrig after assembly
└─ no; runtime should be a native MetaHuman
         └─ combined mesh becomes a preserved identity reference
            └─ fit through .gchuman; MetaHuman topology becomes authoritative
```

A `.gchuman` reference fit does not preserve source vertices, UVs, skeleton, materials, hair or
clothing. It uses the assembled source to fit a native identity. Use the custom route when those
source properties are part of the design.

## 2. Generate by part, but generate one family

Parts from independent prompts often look plausible alone and fail together. Give every generation
the same character sheet or reference set and the same construction contract.

Prepare [dedicated part reference images](ASSET-QUALITY.md#dedicated-part-references-for-assembly)
before generating an unclear production part. Include its mating collar/rim and neighbouring body
region, using the shared character references to preserve identity. Resolve the interface and join
choice in the plan before spending generation effort on the mesh.

### 2.1 Shared contract

Record these once and repeat them for every part:

- character identity, age range, build, proportions and style;
- neutral expression and neutral body pose;
- real-world target height and unit convention;
- forward axis, up axis, handedness and origin convention;
- connection locations and approximate cross-sections;
- surface style and skin/material intent;
- required symmetry or deliberate asymmetry;
- topology target, polygon budget and subdivision expectations;
- whether the part should be closed or expose an open mating rim;
- source service/model, prompt, seed when available, licence and date.

Use orthographic front, side and back references when supported. Reuse the same references for every
pass. A good-looking arm from a different person is not a compatible arm.

### 2.2 Start with useful part boundaries

Prefer boundaries that are easy to inspect and have enough surrounding surface for a transition:

```text
first practical split
├─ core body ........ torso + pelvis, optionally neck base
├─ head ............. head + a useful neck collar
├─ arm .............. shoulder/upper-arm collar through wrist
├─ leg .............. hip/upper-leg collar through ankle
├─ hand ............. hand + wrist collar, only when it improves quality
└─ foot ............. foot + ankle collar, only when it improves quality
```

Do not split every finger, tooth or eyelid merely because generation by part is possible. Every split
creates another identity, scale, topology, material and seam decision. Split where quality,
replaceability or specialist generation improves enough to pay for the join.

### 2.3 Keep presentation layers out of the base

Generate the construction body/head without baked hair, eyebrows, facial hair, clothing, footwear
or accessories. Here, **bare** means unobstructed for fitting and deformation review; a modest
underbody or mannequin surface is acceptable. Add presentation layers after the body is accepted.

```text
bad wrist seam ........ repair arm/hand connection
wrong brow ............ replace brow layer
collar clipping ....... refit garment
hair penetration ...... restyle or rebind groom
identity mismatch ..... refit body/head source
```

## 3. Preserve and inventory the sources

Keep original files outside generated Unreal outputs. Use one manifest or asset-log entry per source
family:

| Field | Record |
|---|---|
| Part | Stable role, such as `BodyCore`, `Head`, `LeftArm` |
| Source | Service, model/version, source file and original export |
| Prompt | Full prompt and negative prompt; reference-image identities when permitted |
| Reproduction | Seed, settings and generation date when exposed |
| Rights | Licence, account terms, consent and commercial-use status |
| Geometry | Units, axes, pose, dimensions, polygon count and open/closed state |
| Appearance | Material slots, textures, UV channels and color-space assumptions |
| Connection | Intended receiver, root name, socket type and transition region |
| Status | raw, normalized, imported, prepared, joined, accepted or rejected |

Never rely on an Unreal asset path as the only provenance record. The `.uasset` consumes the source;
it does not replace the source and its rights history.

## 4. Normalize before joining

Inspect every part alone before composing it. Correct large disagreements upstream when possible;
an organic join should not hide a different character, pose or scale.

### 4.1 Geometry checks

- Confirm scale against a named measurement, not only a bounding box.
- Put every part in the same neutral pose and axis convention.
- Make the intended connection axis unambiguous.
- Remove accidental duplicate shells, internal fragments and presentation accessories.
- Inspect normals, winding, nonmanifold edges, holes and self-intersections.
- Keep enough clean surface around each mating region for a transition band.
- Prefer compatible edge density near a seam.
- Preserve deliberate openings; do not cap a mating rim just to silence a diagnostic.

### 4.2 Appearance checks

- Inventory every UV channel and material slot.
- Compare skin hue, roughness, normal strength and texel density at intended seams.
- Decide whether a generated transition gets a dedicated material for later blending.
- Preserve source textures even when the first proof uses a diagnostic material.

### 4.3 Preview placement first

First place parts with a non-merging preview and inspect front, side, back and close-up views. The
silhouette and connection frames should already agree. If a hand is rotated, an arm is too short or
a head belongs to a different neck, fix the input instead of asking the seam solver to redesign it.

A non-merging preview tests placement; it does not choose `join=none` for the finished connection.
Apply the planned final join only after the inputs and interface pass their checks.

## 5. Import the parts

Use `gc_import` in plan mode before committing external files. Verify archive members, importer,
destination paths and proposed asset names. Then read and render the imported assets.

Shape accepts an existing `UStaticMesh` as an operand. The source asset is read, not edited. Its
selected source-model LOD, geometry, UV layers, triangle material IDs and material slots enter the
new recipe dependency. A skeletal mesh is not this operand route.

Illustrative current structure:

```gcshape
Part = mesh() {
  add(mesh('/Game/Characters/Source/SM_LeftArm', lod=0))
}
```

Use the installed Shape description for exact current grammar before authoring. This guide owns the
workflow and decisions; installed help owns parameter spellings and accepted values.

### 5.1 Worked imported torso

The shipped [ImportedTorsoNeck.gcshape](Examples/ImportedTorsoNeck.gcshape) uses the measured
Bigfoot torso: local dimensions approximately 72.2 × 61 × 100 cm, Z up, centred in X/Y, base at Z=0.
It connects a diagnostic cylindrical collar to the imported receiver. The collar is a small
repeatable connection test, not a finished head or character.

1. Plan `gc_import` from the original torso FBX with
   `destination:/Game/OrganicImported/Source/SM_Torso.SM_Torso`, inspect the plan, then execute it.
   A plain destination such as `/Game/OrganicImported/Source` always means a folder, whether or
   not it exists. A chosen asset name uses the explicit `Name.Name` object path. Update older
   plans that used a plain path as an asset name before executing them. An ambiguous older
   `importir/1` plan refuses with a migration message; append `/` for a folder, append `.Name`
   for a named asset, or regenerate the plan. New plans use `importir/2`.
2. Read the mesh's `settings` block: the read-only bounds comment gives local min, max and size
   in centimetres, including bounds extensions. Render it before choosing socket positions.
3. Copy the example to a project `.gcshape` and apply it. `keep(largest)` first drops loose shells
   (the downloaded torso carries a separate inner pelvis piece), then `remesh(1)` closes source
   tears and carries existing UV charts and material IDs, and a last `keep(largest)` drops any
   specks the rebuild left. **The order matters: a voxel remesh welds every shell it is given into
   one surface**, so remeshing first turned that inner piece into a deep dent across both hips
   that no leg could join into (Bigfoot, 2026-10-21); `keep(largest)` after it cannot separate
   what is already welded. The remesh note names the shells it fused. All three explicitly permit
   topology changes. Fine detail below 1 cm can be lost; inspect the result.
4. The saved upper band begins at Z=85 cm; its socket is at (0,0,94), facing +Z. The collar's
   root faces -Z. Matching `type='example/neck'` and `fit=frame` seat these exact frames.
5. Inspect front, back and close seam views. To substitute an imported head, wrap its static mesh
   operand in the Collar entry, measure its root frame and declare its own collar band. Reusing
   the example's coordinates on an unmeasured head is not a valid fit.

For placement outside socket fitting, `rotate=(roll,pitch,yaw)` uses Unreal degrees: positive
pitch turns +X toward +Z, yaw turns +X toward +Y, roll turns +Y toward -Z. Thus `(0,-75,0)`
points +X down. Local scale/mirror precede rotation (roll, pitch, yaw), then translation/alignment.
`add` shapes about the part origin; `place` shapes about the current mesh centre.

## 6. Prepare a semantic organic connection

An organic connection has two sides:

```text
receiver part
├─ named socket
├─ position
├─ facing and up axes
├─ semantic type
└─ transition region the receiver permits to move

incoming part
├─ named root socket
├─ position
├─ facing and up axes
├─ the same semantic type
└─ optional incoming transition band for an open-rim/two-band solve
```

The socket frame owns placement. The current route aligns the incoming root to the receiver frame;
it does not silently choose a guide-average rotation, bury a cap, or auto-scale the part.

### 6.1 Structural pattern

This is a small pattern, not a paste-ready promise for every mesh:

```gcshape
Body = mesh() {
  add(mesh('/Game/Characters/Source/SM_BodyCore'))
  save($ShoulderBand) [/* selection allowed to participate */]
  socket(LeftArm,
         at=(-39,-6,53.5), facing=-Z, up=+X,
         type='human/arm', transition=$ShoulderBand)
}

LeftArm = mesh() {
  add(mesh('/Game/Characters/Source/SM_LeftArm'))
  socket(Root, at=(0,0,0), facing=+Z, up=+X, type='human/arm')
}

Combined = mesh() {
  body = add($Body)
  arm = add($LeftArm,
            join=organic,
            into=$body.LeftArm,
            using=Root,
            fit=frame)
}
```

The selection that becomes `$ShoulderBand`, the measured frame and the mating topology are the real
work. Ask the installed Shape description about `socket`, selections and `add(join=organic)` before
adapting this structure.

### 6.2 Receiver transition

The transition is permission, not decoration. It defines the receiver skin that may participate in
the reconstruction. Make it:

- wide enough to blend the incoming collar without a pinched seam;
- narrow enough to protect anatomy and detail that must not move;
- connected to the intended mating region;
- free of unrelated clothing or accessories;
- named by anatomical or functional intent, not a transient vertex index.

When several consecutive connections share one transition, Shape can solve the maximal run as one
batch. This matters for nearby roots such as fingers: receiver skin between roots can participate
instead of each seam being rounded alone. An intervening instruction or different transition ends
that batch, so connection order is part of the recipe's meaning.

### 6.3 Incoming root and collar

The incoming root is the exact frame placed onto the receiver socket. Keep a useful collar around
it. For an open mating rim, the connected transition band must be valid and unambiguous. A two-band
connection can use receiver and incoming bands so protected outer boundaries remain fixed while the
mating rim becomes the interior of the solve.

Do not put the band across unrelated facial surfaces, clothing shells or disconnected fragments.
An organic join is local surface reconstruction, not general face recovery.

The incoming skin must supply a closed cross-section around the socket axis near the root, with
correspondence on every side of the receiver's mating rim. A deliberate open mating rim is allowed;
a tear through a sampled side section is not. Missing-side errors report the nearest open edge's
endpoints and distance to the missed ray; that edge is a locator, not proof that it caused the miss.
Close the incoming mesh with `remesh` or move the root to an intact section. Whole-mesh closure is
not required for unrelated retained openings elsewhere on the part.

`add(...,join=organic,tolerance=.5)` overrides the header's geometric tolerance in centimetres.
All connections in one shared transition batch use the same tolerance. Work-limit errors include
the socket, sample location, last refinement error/location, current-pass error and tolerance. Coarser tolerance accepts a coarser
approximation; it cannot make an invalid correspondence or crossing acceptable. Independent bands
resolve against the completed prior join, so stale triangle IDs cannot turn a shoulder band into a
neck band. Overlapping permissions and shared batches still carry their declared sequencing.

To interrupt a running apply, read `unreal_info include:"tasks"` and call
`unreal_execute mode:"cancel" task_id:"..."` using the same client identity. Shape checks at safe
geometry boundaries and before publication; poll until terminal status. Closing the HTTP client or
reaching its wait timeout does not mean cancellation. Earlier completed outputs of a multi-output
recipe remain saved. Native operations without internal checkpoints finish their current phase
before cancellation is observed.

## 7. What organic join does—and does not do

It can:

- place a prepared incoming part through matching semantic socket frames;
- reconstruct a transition within declared receiver/incoming regions;
- retain unrelated openings and detail outside the solve;
- preserve source geometry and appearance layers outside the transition;
- give the transition a named material slot for diagnostic or blending work;
- solve consecutive nearby connections together when they share a transition.
- measure how stretched the band is and refuse a streaky one (since 2026-10-21). The solver moves the
  receiver's own cells onto the incoming collar; the further the band travels, the more they stretch
  into needles, which shade as streaks and fold where the band turns (the Bigfoot's hips and
  shoulders, 36-47% needles, with the legs cut flush at the thigh and the arms just outside the
  shoulder ball). The build note *"the transition band's edges run ... cm"* gives the band's needle
  cells and its bend beside the kept skin at its border. A band with more needle cells than that skin
  is refused as *"organic transition is stretched"*, naming the thinnest cell. **Author the parts so
  the band travels little:** extend the incoming part into the body (keep the leg's own hip, the
  arm's own shoulder) so its collar sits close to the receiver's surface. A layout of the band's
  cells inside the solver was built and measured on 2026-10-21: clean on a travelling band, but
  5-10 times slower and it broke a five-finger hand, so it was reverted.

It does not automatically:

- decide which anatomy belongs together;
- make parts share identity, scale, pose or proportions;
- scale a part to fit;
- repair arbitrary self-intersections or every hole;
- harmonize skin color, roughness, normals or texel density;
- bake a common texture atlas or blend tangent-space normal maps;
- make a layered character watertight when unrelated openings remain;
- preserve a source mesh literally when the next step is MetaHuman fitting.

Organic, blend, fillet, hard, clip, full and none are different join decisions. Use the
[join decision guide](ASSET-QUALITY.md#choose-the-join-for-each-add) for every connection. Use
`organic` only when the socket/transition contract exists. Use `none` for shells that should remain
separate, such as pants over a body; use separate assets with attachments when independent motion
or removal is required. Neither `organic` nor `none` is a universal assembly default.

## 8. Assemble in deliberate stages

```text
stage 1 — establish core
└─ torso/pelvis or whole central body

stage 2 — identity silhouette
├─ head/neck
├─ arms/shoulders
└─ legs/hips

stage 3 — distal detail when separately generated
├─ hands/wrists
└─ feet/ankles

stage 4 — inspect the clean body/head
├─ front/side/back silhouette
├─ all joins close-up
├─ normals and material boundaries
└─ deformation-ready proportions

stage 5 — choose downstream ownership
├─ custom topology → .gcrig
└─ native MetaHuman → .gchuman reference fit

stage 6 — presentation layers
├─ brows/lashes/facial hair
├─ scalp hair/groom
├─ clothing/footwear
└─ accessories/equipment
```

Build and judge one connection family before multiplying it. A good left-arm join proves more than
four simultaneous joins that hide which preparation failed. Mirror or repeat only where symmetry is
a real design decision.

## 9. Appearance across the transition

Source UVs and material slots can remain independent. A transition material can expose a place to
blend receiver and incoming appearance, but it does not make their maps or colors identical.

1. Inspect the transition with a high-contrast diagnostic material.
2. Inspect normals and shading under a moving light.
3. Restore source materials and compare both sides.
4. Decide whether to reproject, repaint, rebake or blend the transition.
5. Check tangent-space normals after any UV or topology change.
6. Repeat at intended LODs and camera distances.

A seam hidden by flat lighting is not fixed. A smooth silhouette with a color or normal break is
still a failed visible join.

## 10. Continue to custom Rig or MetaHuman

### 10.1 Keep the combined mesh custom

Choose this when generated topology, stylization, non-human anatomy or material layout must remain.
Preserve the combined static source, then author a `.gcrig` for joints, chains, regions, skinning,
controls and IK. Exercise representative bends; a neutral seam can fail under shoulder, wrist, hip
or neck motion.

### 10.2 Use the combined mesh as a MetaHuman reference

Choose this when the runtime result should be a native MetaHuman:

```text
accepted combined body/head static mesh
  → .gchuman human(reference=...)
    → measured face landmarks and body proportions
      → identity preview
        → reviewed native rig request
          → native assembly
            → native Body + Face + DNA/RigLogic/post-process
```

The combined mesh remains a preserved source and identity reference. The native assembly is the
runtime owner. Rebuild hair, brows, clothing and accessories against that result rather than
expecting the Human fit to transfer them.

For body motion plus facial performance, continue with [CHARACTERS.md](CHARACTERS.md). For clothing,
use [GARMENT.md](GARMENT.md); for hair, use [GROOM.md](GROOM.md).

## 11. Verification ladder

Do not accept the whole model because one `add` parsed.

```text
1. Source proof
   └─ source, licence, prompt/settings and original files preserved
2. Part proof
   └─ each part has correct scale, pose, axes, surface and materials
3. Placement proof
   └─ coarse non-merging assembly has the intended silhouette and frames
4. Connection proof
   └─ socket types, root frames and transition ownership are explicit
5. Organic build proof
   └─ expected mesh builds without unintended loss or new intersections
6. Surface proof
   ├─ silhouette is continuous
   ├─ shading/normals are continuous where intended
   └─ UV/material transition is accepted or intentionally separate
7. Downstream proof
   ├─ custom route: rig and representative deformation pass
   └─ Human route: identity preview, native assembly and preservation pass
8. Presentation proof
   └─ groom, brows, clothing and accessories fit the accepted body
9. Durability proof
   └─ saved assets reopen and the recipe rebuilds without losing sources
10. Delivery proof
    └─ intended LODs, camera distances, motion and packaged runtime pass
```

Minimum humanoid review views are front, back, both profiles, three-quarter face, shoulder, neck,
wrist, hip and ankle close-ups, plus representative extreme poses after rigging.

## 12. Troubleshooting

| Symptom | Check first | Corrective direction |
|---|---|---|
| Part lands rotated or rolled | Receiver and root `facing`/`up` frames | Fix the frames; do not rotate the final output by eye |
| Part is the wrong size | Shared generation scale and measurements | Normalize or regenerate; organic fit does not auto-scale |
| Seam pinches | Transition width, collar density and proportion mismatch | Widen/reselect the local band or repair the collar |
| Nearby joins form separate lumps | Connections are not consecutive or do not share a transition | Batch the run only when one shared transition is correct |
| Unrelated clothing moves | Transition reaches a layered/disconnected shell | Keep clothing separate and restrict the connected skin region |
| Smooth join has a color break | Material/UV/texture disagreement | Reproject, repaint, rebake or use a transition material |
| Normal seam appears in motion | Tangents or normal maps disagree | Repair the tangent basis and rebake if needed |
| New self-intersection appears | Collar placement and transition volume | Repair source or frame; do not enlarge the solve blindly |
| "organic transition is stretched" | The thinnest cell's place in the message: where the band travels furthest from its kept edge to the incoming collar | Extend the incoming part into the body so its collar sits close to the receiver, or move the root; a wider band spreads the same travel over more cells |
| Result is not watertight | Unrelated source openings remain | Decide which openings are intentional; the join is local |
| MetaHuman loses source topology | Wrong expectation of identity fitting | Use custom `.gcrig` if topology must survive |
| Hair or clothes block review | Presentation layers arrived too early | Defer them until the clean body/head passes |

## 13. Cold-agent handoff

Leave these facts in the project's own asset log or character doc:

```text
organic assembly handoff
├─ published workflow ........ Plugins/GraphCode/Docs/Guides/ORGANIC-PART-ASSEMBLY.md
├─ source manifest ........... rights, prompts, settings and originals
├─ imported parts ............ exact Unreal paths and chosen LODs
├─ common contract ........... scale, pose, axes, target measurements
├─ connection map
│  ├─ receiver socket
│  ├─ incoming root
│  ├─ semantic type
│  └─ transition selection/ownership
├─ evidence .................. accepted/rejected renders and reasons
├─ downstream owner .......... custom .gcrig or native MetaHuman fit
└─ remaining layers .......... groom, brows, garment and accessories
```

Do not paste changing grammar into that handoff. Link this shipped guide and ask the installed Shape
description for exact syntax. Record project-specific decisions and evidence only.
