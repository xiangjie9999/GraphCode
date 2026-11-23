# Characters: Human, Rig, Move, Body, and Face

Use this guide when one character crosses more than one GraphCode product: a generated or fitted Human, a custom rig, authored body motion, native MetaHuman facial animation, or an Unreal runtime Blueprint that must play them together.

The detailed MetaHuman body adapter setup lives in [METAHUMAN-RIG.md](METAHUMAN-RIG.md); the section 7.4 link remains here for existing references.

For motion planning, all current Move verbs, joint limits, sword grips and rigid or articulated
objects, follow [ANIMATION.md](ANIMATION.md).

For custom procedural behavior on a `.gcrig`-prepared character, follow
[Control Rig authoring](CONTROL-RIG.md). It covers the native starter, member variables and graph
edits, plus source regeneration that preserves unrelated functions and authored solve wiring.

The shortest correct mental model is:

```text
construction source
├─ identity and native MetaHuman assembly ........ .gchuman
├─ custom skeleton, skin, controls and IK ........ .gcrig
├─ one-skeleton motion ........................... .gcmove
├─ animation settings and float curves ........... anim_sequence .gc
└─ multi-component playback ...................... Unreal Blueprint / AnimBP / Sequencer
```

A file owns one layer. Do not make one layer silently impersonate another.

Keep multi-part mesh assembly in GraphCode `.gcshape`; do not assemble the character in Blender.
Blender may inspect or prepare individual source assets before import. Use
[organic part assembly](ORGANIC-PART-ASSEMBLY.md) for the imported-mesh example and connection contract.

## 1. Evidence labels and the cold-start rule

This guide uses four labels deliberately:

- **Shipped** means the current repository implements the path.
- **Measured** means a saved fixture was built, reopened, and observed through the public path named here.
- **Prototype** means the fixture proves a useful composition, but its orchestration is not yet a general GraphCode abstraction.
- **Proposed** or **open** means design or missing coverage, not a capability to promise.

For exact parameters, use this order:

```text
tool(help:true)
  → gc_describe for the exact kind/block
    → the narrow guide or grammar linked here
      → current parser/writer source when the two disagree
```

Tool help describes the installed build. This guide describes the current checkout and records evidence. A historical design document can preserve an older spelling; the parser is authoritative for what the checkout accepts. For example, the current Human parser accepts `rig()`, not the older proposed name `autorig()`.

Never infer a capability from a file extension alone. Dry-run the actual program against the actual asset before mutating or starting an external service request.

## 2. Choose the route before authoring

```text
What must remain the character's source of truth?
│
├─ A custom/stylized mesh, skeleton, skin or bone layout
│  ├─ build or rebuild that custom rig ............ .gcrig
│  ├─ author motion for that one skeleton ......... .gcmove
│  └─ send body motion to MetaHuman ............... explicit IK retargeter
│
├─ An existing native MetaHuman identity and assembly
│  ├─ edit supported appearance/outfit facets ...... kind:metahuman .gc
│  ├─ keep native body and face meshes ............. do not rebuild through .gcrig
│  └─ animate
│     ├─ body ...................................... .gcmove through an adapter .gcrig (§7.4), retargeted, or native
│     └─ face ...................................... native face sequence / curves / face systems
│
├─ A clean 3D-AI mesh that should become a MetaHuman
│  ├─ one mesh: preserve and import the clean source  Core / gc_import
│  ├─ separate parts: assemble the clean source ..... Shape organic join
│  ├─ fit a MetaHuman identity from it .............. .gchuman reference + landmarks
│  ├─ preview before native rig/assembly work ........ identity preview
│  └─ animate the native result ...................... body + face composition below
│
└─ Both a custom character and MetaHuman performance
   ├─ preserve the custom source and its .gcrig
   ├─ fit a separate MetaHuman identity from the source
   ├─ retarget body motion by explicit chains
   └─ compose native body + native face at Unreal playback level
```

The key ownership rule is simple: **Human owns MetaHuman identity and native assembly; Rig owns custom skeletal construction; Move owns motion for one resolved skeleton; Unreal owns playback composition across components.**

### 2.1 Start with a MetaHuman, or fit an AI mesh into one

Both are valid Human entry paths, but they preserve different things.

If the 3D-AI source is generated as head, body or limb parts, first follow
[ORGANIC-PART-ASSEMBLY.md](ORGANIC-PART-ASSEMBLY.md). That guide owns the shared generation
contract, imported-mesh preparation, socket/transition model, Shape organic join and seam
verification. This guide begins once the clean combined source is ready for custom Rig or Human.

| Starting point | Use this route | What remains authoritative |
|---|---|---|
| Existing MetaHuman | Keep its native Body, Face, DNA/RigLogic and post-process setup; use `kind:metahuman` for supported appearance/outfit changes | The existing native MetaHuman assembly |
| Clean 3D-AI body/head mesh | Import it, reference it from `.gchuman`, declare the measured landmarks and proportions, preview the fitted identity, then request native rig and assembly | The AI mesh remains source/reference; the fitted native MetaHuman becomes the runtime character |
| Custom or non-human mesh whose topology/skeleton must survive | Keep it custom and use `.gcrig`; only retarget its motion to a MetaHuman when a separate MetaHuman performance is wanted | The custom mesh and `.gcrig` |

For the AI-mesh route, generate or prepare a neutral construction source: complete body and head,
neutral expression and pose, known scale and axes, and no baked hair, brows, facial hair, clothing or
accessories. Keep the generation prompt, model/service, licence and original source. Import the mesh,
inspect its topology and landmarks, then use it as the `reference=` of the Human entry. The measured
route has this shape:

```text
clean 3D-AI mesh
  -> import and inspect
    -> .gchuman human(reference=...) + points/measure/preset
      -> transient identity preview
        -> rig(...)
          -> assemble(...)
            -> native MetaHuman Body + Face + DNA/RigLogic
```

This is a **fit into MetaHuman**, not a byte-for-byte mesh conversion. The MetaHuman result uses its
native topology, body/face assets, DNA, rigging and assembly. It does not automatically preserve the
source mesh's topology, skeleton, UVs, materials, hair, eyebrows, clothing or accessories. Keep those
as separate sources and rebuild or reassign each layer deliberately. If preserving the source
topology or skeleton is the requirement, stop before the Human fit and take the custom `.gcrig`
route instead.

## 3. Start from a modular construction source

Plan every layer before building any of them: body, under-layer, armour, straps, hair, carried items
and the motion test. [ASSET-QUALITY.md](ASSET-QUALITY.md#10-characters-plan-the-layers-from-the-skin-out)
has the layer plan; armour is [Garment and Shape together](GARMENT.md#armour-is-a-garment); hair
starts from [the proven coat](GROOM.md).

Default to a clean base body and head before attaching presentation layers:

```text
character source
├─ base body/head
│  ├─ stable topology
│  ├─ neutral expression
│  ├─ known neutral body pose
│  └─ agreed scale, origin and forward/up axes
├─ hair and grooms
├─ eyebrows, eyelashes and facial hair
├─ clothing and footwear
└─ accessories and carried props
```

Here, **bare** means an uncluttered construction source. It does not require a nude final presentation. A modest base layer, mannequin surface, or hidden-body proxy is fine when it exposes the geometry needed for fitting and deformation.

This separation is useful because it lets an author:

- inspect skinning and facial deformation without hair or clothing hiding the result;
- fit garments against one stable body and swap them without rebuilding identity;
- decide deliberately which covered body polygons remain, are masked, or are removed;
- give face skin, brows, lashes, beards and hair different deformation and material policies;
- change LOD, groom, outfit or accessory assets independently;
- repair a seam, penetration, bad weight, or material without replacing the whole character;
- compare source, fitted identity, rigged result, and final presentation using the same landmarks.

Keep all layers on the same scale, origin, axis convention, neutral pose and intended skeleton. Before accepting a layer, check:

- neck, wrist, ankle, waist and garment seams;
- overlap during the neutral pose and representative extreme poses;
- hidden-body policy at each outfit and LOD;
- normals, tangents, material slots and UV continuity where a seam is visible;
- whether the layer follows the body, the face, a socket, cloth simulation, or its own rig;
- whether a supposedly separate layer accidentally carries a second conflicting skeleton.

There are deliberate exceptions. A fused stylized character, a scan whose fused surface is essential, or a final performance bake can remain fused. Preserve a layered construction source beside that delivery asset whenever possible; the fused result should be an output, not the only repairable copy.

## 4. Asset roles and source of truth

| Layer | Canonical authoring surface | Owns | Does not own |
|---|---|---|---|
| Human recipe | `.gchuman` | Human identity recipe, reference landmarks, preset/sculpt/measure/variant operations, native rig request, native assembly request | Body keyframes, arbitrary face expressions, raw DNA editing, custom skeleton construction |
| MetaHuman asset document | `kind:metahuman` `.gc` | Supported saved appearance and outfit facets such as face, skin, eyes, makeup, head and outfit | Raw face DNA, a whole performance timeline |
| Custom rig recipe | `.gcrig` | Custom bones, joints, chains, regions, skinning, controls, contacts, grips, IK outputs | Safe reconstruction of native MetaHuman body/face assets, animation keyframes |
| Motion recipe | `.gcmove` | Clips for one resolved skeleton, semantic-axis motion, IK operations, generic control drives, body-attached contacts and relative fixation | Two-skeleton performance composition, automatic MetaHuman facial semantics |
| Animation asset document | `kind:anim_sequence` `.gc` | Sequence settings, supported float curves, notifies, sync markers and related metadata | Per-bone animation keyframes; those remain import- or Move-produced |
| Native MetaHuman assembly | generated Blueprint and its components | Native body/face meshes, native post-process graphs, DNA/RigLogic, grooms/outfit components | Custom source history unless separately preserved |
| Playback layer | Blueprint, AnimBP, montage or Sequencer as appropriate | When and how body and face assets play, layer and synchronize | Identity fitting or rig construction |

Do not edit a derived asset and assume the recipe learned the change. If the recipe should remain reproducible, put the durable decision in the owning source file.

## 5. Dependency order

Build upstream to downstream:

```text
modular reference source
├─ custom route
│  ├─ .gcrig → custom skeletal mesh / skeleton / IK assets
│  └─ .gcmove → custom-body AnimSequence
│                  └─ explicit IK retarget → MetaHuman-body AnimSequence
│
└─ Human route
   └─ .gchuman identity
      └─ native rig
         └─ native assembly Blueprint
            ├─ Body component ← body sequence
            ├─ Face component ← face sequence / face curves
            ├─ native face post-process + DNA/RigLogic
            └─ grooms, outfit and accessories

Body component + Face component
└─ one Unreal playback owner
   └─ preview, PIE, render and packaged-runtime verification
```

Changing identity can invalidate native rigging and assembly. Changing animation should not request a new Human rig. Changing a custom `.gcrig` can invalidate every `.gcmove` that addresses its bones, axes, controls or contacts. Plan and dry-run at each boundary.

## 6. Human: identity, native rig, and assembly

### 6.1 Current grammar

The current `.gchuman` header is `gchuman 1`. Its top-level entry kinds are:

```text
number()   random()   human()   import()   rig()   assemble()
```

The current measured clothed-reference recipe is structurally:

```gchuman
gchuman 1 into:/Game/Test/FullHuman/MetaHuman seed:42

build {
  MH_ClothedReference = $AnimeRig
  /Game/Test/FullHuman/MetaHuman/ClothedReference/BP_ClothedReference = $AnimeGame
}

Anime = human(reference='/Game/Test/FullHuman/ClothedAssembly/SM_Human') {
  points {
    # Eight source-surface face landmarks are declared in the measured fixture.
  }
  preset('Bruce')
  measure(Height=180)
}

AnimeRig = rig($Anime, type=joints)
AnimeGame = assemble($AnimeRig, pipeline=optimized, quality=high)
```

Do not copy the placeholder landmark block. Measure the actual source surface and use the installed
Human description for the complete current block.

**What the fit does.** The reference's whole surface goes to Epic's solver; the points tell it which
spot on the reference is which spot on the MetaHuman. Every vertex of the fitted head is carried into
the character, so the shape between the points comes from the reference, smoothed, not from an average
face. UE 5.7's solver offers 28 fit points (eye corners, eyelid ends, brow ends, mouth corners, lip
midpoints, philtrum, chin fold ends, ears and the Frankfurt plane) and there are 79 Move handles. Give
at least three fit points; a name the build does not know makes it list them all.

**Where the fit leaves the likeness short, name the detail.** The same `points { }` block takes
MetaHuman's fine Move handles by name (`nose_tip`, `nostril_l`, `upper_lid_r`, `mouth_corner_l`, ...):
after the fit, each named handle is moved onto the point measured on the reference, and the build note
says how far it moved and how close it landed. To see which name is which, render a MetaHuman with
`gc_render highlights:"landmarks"`: every handle (a dot) and fit point (a ring) is drawn on its face,
numbered and coloured by region, with a legend at the picture's right naming each number. Then measure
the matching spot on the reference with `gc_render mode:"ask"`. Declare
only the handles where the likeness is off; the rest stay where the fit put them. The development checkout's measured fixture is
named `Content/Test/FullHuman/MetaHuman.gchuman`; it is evidence, not a shipped template.

`rig()` accepts `type=joints` or `type=joints_and_blendshapes`. `assemble()` accepts `pipeline=optimized|cinematic`; `quality=high|medium|low` belongs to the optimized pipeline. **The optimized pipeline drops the face's highest-detail mesh (LOD0)**, so a lid fold or fine crease seen in the identity preview is gone in the optimized character and back with `pipeline=cinematic`: judge fine facial detail on a cinematic assembly. Every identity build also prints the face measured as a portrait is (widths, mouth, chin, eye openings, in cm and as % of the inner-eye spacing), and the same names in `measure()` move each to its number: `measure(jaw_side_width=335%, chin_drop=9.2, eye_opening_l=32%)`. A crease or fold of the skin is Shape's `displace` along a `line()` through the face's handles: `LidCrease = line(eye_inner_l offset=0.06, upper_lid_l offset=0.22, eye_outer_l)` above the entries and `displace(by=-0.07, to=$LidCrease, width=0.24)` after every other face instruction makes a double eyelid (a second line pushed out by `by=0.04` is its fold); the face note counts where the skin around each eye passes through itself. `GC-HUMAN-GRAMMAR.md` has both in full. An optional assembly animation name is supported by the parser, but it does not turn the Human recipe into the owner of a multi-component performance.

The declared assembly output must match the native naming rule exactly:

```text
<BuildRoot>/<Name>/BP_<Name>
```

Do not use an inline `[rig=...]` shortcut. The rig stage is an explicit entry so planning can expose expensive or external work.

### 6.2 Identity-first iteration

Use `gc_apply(help:true)` for the installed parameter shape, then iterate only through identity:

```text
gc_apply
├─ target:<selected Human output>
├─ through:"identity"
├─ preview:"update"  → returns preview_asset and preview_id
└─ preview:"discard" + preview_id
```

**Shipped:** the preview/update/discard path evaluates a transient native identity without changing the saved destination and without starting rigging, texture-source, or assembly requests.

**Open:** accepting a preview with stale-input protection is not implemented. Treat `preview_id` as a temporary review handle, not a durable revision.

Render the returned `preview_asset`, check the head fit and declared measurements, then discard it. Only run the full chain after the identity is accepted. Always dry-run first; the plan reports whether service requests are needed. An unchanged measured fixture, reopened in a fresh process, planned with zero new service requests.

### 6.3 What Human does not do

Do not place arbitrary expression animation, runtime randomization, raw DNA edits, or custom rig construction inside a `human {}` body. These are separate ownership layers. The public Human recipe fits identity and requests native outputs; native MetaHuman face behavior remains native.

The reference-based fit is not a promise that every clothed body dimension is reconstructed automatically. The measured fixture used explicit face landmarks and an explicit height. Validate body proportion, garment fit and occlusion independently.

## 7. Rig: custom skeletons and the MetaHuman boundary

### 7.1 Read before authoring

The step-by-step guide to writing a `.gcrig`, for an imported hand and for an animal from a bare mesh, is [RIG.md](RIG.md).

For a custom skeletal mesh or skeleton, use `gc_read` with `kind:"rig"` to obtain a `.gcrig` starting declaration containing its bone hierarchy and orientations. `kind:"skeleton"` intentionally omits the reference skeleton and is not the right readback for this task.

Then author only what the custom rig owns:

```text
.gcrig
├─ source mesh / skeleton
├─ points and regions
├─ joints and parentage
├─ chains and semantic axes
├─ skin policy
├─ controls, including curve.Name bindings
├─ contacts and grips
└─ build outputs such as skeletal mesh, skeleton, IK rig or retargeter
```

Use the rig's declared semantic axes in Move. Do not translate `forward`, `side`, `fold`, `twist`, or `points_to` into guessed world axes. A rig is the authority for what those words mean on that skeleton.

### 7.2 Native MetaHuman assets are a preservation boundary

Do **not** feed the assembled MetaHuman body or face back through the GC Rig mesh builder. The current custom rig builder reconstructs mesh/skin outputs; it is not a preservation decorator for native MetaHuman body weights, face DNA, RigLogic or post-process setup. An adapter `.gcrig` (§7.4) is not that builder: it keeps the body's skin and skeleton and builds only an IK rig.

Use this split:

```text
custom source .gcrig
├─ may build a custom skeletal mesh and IK Rig
└─ may define the source side of a retargeter

native MetaHuman assembly
├─ keep its Body mesh and native target IK Rig
├─ keep its Face mesh, DNA/RigLogic and post-process graph
└─ receive motion through supported native animation paths
```

**Measured:** the FullHuman trial used a custom source IK Rig plus ten explicit chain mappings to `/MetaHumanCharacter/Animation/Retargeting/IK_MH_IKRig`. The native MetaHuman meshes were never rebuilt or reweighted through GC Rig.

Fuzzy chain guesses are not an acceptance test. In the measured trial, an automatic mapping incorrectly assigned finger chains to limbs. Name each source-to-target chain explicitly; declare rest-pose fallback for unmapped chains and verify the result visually and numerically.

### 7.3 Regions must follow surface connectivity

A spatial volume near a character can include skin from a neighboring limb, garment or folded surface. When a region is supposed to describe one anatomical surface, verify connectedness on the surface rather than trusting proximity alone. This matters for joint placement, weight regions, cloth attachment and any automated landmark recovery.

### 7.4 Animate a MetaHuman body with `.gcmove`: an adapter `.gcrig`

Follow [the MetaHuman rig quick start](METAHUMAN-RIG.md): inherit the shared 84-joint template, name the character body mesh and build its IK rig. That guide owns the setup example, verified directions, coverage and migration of existing adapters.

## 8. Move: motion for one skeleton

### 8.1 One program, one resolved skeleton

A `.gcmove` program resolves one skeleton in its header. It can use either a direct skeleton or a character component binding:

```gcmove
gcmove 1 skeleton:/Game/Characters/Rig/SK_Custom into:/Game/Characters/Animation fps:24
```

or:

```gcmove
gcmove 1 character:/Game/Characters/BP_Character part:Body into:/Game/Characters/Animation fps:24
```

`character:` plus `part:` resolves the actual component, including inherited Blueprint component overrides. A direct skeletal mesh can be bound as a character with `part:mesh`. One part still resolves one skeleton; this is not a multi-component performance declaration.

Current motion verbs include:

```text
swing()   shift()   drive()   plant()   reach()   aim()   grab()
```

Build modifiers include `[loop]`, `[root_motion]` and `[fps=N]`. Use live Move help for the exact parameter set of each verb.

### 8.2 Semantic motion and controls

Prefer semantic axes from the rig over raw component assumptions. The measured custom-body greeting uses fold/side/twist semantics declared by its rig. Preserve the single aim convention, `points_to=`, instead of inventing a second facing axis.

`drive()` is **shipped** for a control declared in `.gcrig controls {}`. A control can bind bone channels or a generic `curve.Name`, and the built result is an ordinary sequence with bone tracks and/or float curves.

For a MetaHuman, the shipped [FaceExpressions adapter](FACE-EXPRESSIONS.md) supplies named `smile`, `blink`
and `jaw_open` controls over seven native Face curves. Use it through `controls:` on a `part:Face` clip and
`drive(smile, by=$Profile)`; no new expression verb or curve-name discovery is needed for that starting set.
Other expressions need calibrated controls of their own. A generic curve name is useful only when the selected
face system consumes it; this example does not define arbitrary-face transfer or speech/expression blending.

### 8.3 IK and contact limits

`aim()` and the facing part of `reach(frame)` distribute rotation through the declared chain. With `ik_rig:`, Move bounds both sampled FK and final IK rotations by that rig's configured joint limits and reports remaining target error. A grab-only solve preserves the authored palm/ancestors outside its active digit chains. `grab()` still bakes points against a static target; runtime carrying, automatic contact selection and whole-finger collision remain open. See [animation authoring](ANIMATION.md) for grip setup and the held-object workflow.

## 9. Flagship recipe: MetaHuman face plus `.gcmove` body and macro motion

This is the supported adjacent workflow today. It combines a Move-authored or Move-originated body performance with a native MetaHuman facial performance at the Unreal component/playback layer.

### 9.1 Ownership tree

```text
performance intent
├─ body and macro movement
│  ├─ root, pelvis, spine, arms, legs .............. .gcmove on source/body skeleton
│  ├─ head and neck macro turns .................... body track, by declared policy
│  └─ source → MetaHuman body ...................... explicit IK retargeter
│
├─ facial performance
│  ├─ eyelids, eye expression, lips, cheeks ........ MetaHuman face curves/systems
│  ├─ jaw expression ............................... MetaHuman face owner
│  └─ native deformation ........................... face post-process + DNA/RigLogic
│
└─ composition
   ├─ Body component plays body sequence
   ├─ Face component plays face sequence
   ├─ one Blueprint/event starts both in prototype
   └─ production clock/layer policy belongs to Unreal playback architecture
```

The measured fixture is:

```text
custom A_Human_Greeting
  → explicit IK retarget
    → A_BodyGreeting on native MetaHuman body

FacePerformance.gcmove
  → A_FaceGreeting carrier sequence
    → anim_sequence .gc writes exact native float curves

BP_ClothedPerformance
  → plays A_BodyGreeting on Body
  → plays A_FaceGreeting on Face
  → native face post-process copies the required body pose and runs RigLogic
```

**Measured:** body/head motion, blinking, mouth-corner smile and jaw opening were observed together; the saved result reopened and played in a fresh Unreal process.

**Prototype:** the Blueprint starts two independent sequences from one event. GraphCode does not yet provide a single synchronized body-plus-face performance asset or player.

**The face's keys on the body's timing (2026-11-19):** a face file can name the body's built clip
(`body:`) and land its keys on the body's poses (`[lands_at=$Pose]`), so the face is timed by the
body's build when it builds: [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md). That keeps the two clips
in step; it is still two clips, started together by your playback.

### 9.2 Author the body track

The body track can also be written straight onto the MetaHuman body through an adapter `.gcrig`
([§7.4](#74-animate-a-metahuman-body-with-gcmove-an-adapter-gcrig)); then steps 4–6 below fall away. The
retarget route stays for motion that already exists on another skeleton.

1. Build and verify the custom source rig, or select a source skeleton with known chains.
2. Author body and macro motion in `.gcmove`.
3. Build the source AnimSequence.
4. Create or read the source IK Rig and the native MetaHuman target IK Rig.
5. Map chains explicitly. Never accept fuzzy mappings without inspection.
6. Retarget through Unreal's native retarget operation to a MetaHuman-body AnimSequence (below).
7. Read back the destination sequence metadata and render it on the actual assembly Body component.

**Running the native retarget.** GraphCode has no retarget verb; Unreal's batch operation is run from a
small editor utility you write once as a `.gc` (`gc_apply create:true`), then call with `gc_invoke`.
It needs the IK Rig plugin and, for the move, Editor Scripting Utilities:

```gc
gc 1  kind:blueprint  asset:/Game/Tools/EU_RetargetBody
parent: /Script/Blutility.AssetActionUtility

functions {
  function RetargetBody {
    input {
      SourceAnimation: AnimSequence*
      SourceMesh: SkeletalMesh*
      TargetMesh: SkeletalMesh*
      Retargeter: IKRetargeter*
    }
    n1 = FunctionEntry() [bIsEditable=True] : ->n2
    n2 = CallFunction IKRetargetBatchOperation:DuplicateAndRetarget(AssetsToRetarget=n3.Array, SourceMesh=n1.SourceMesh, TargetMesh=n1.TargetMesh, IKRetargetAsset=n1.Retargeter, Prefix="MH_", bIncludeReferencedAssets=false) : ->_
    n3 = MakeArray(`[0]`=n4.ReturnValue)  !types[>Array="TArray<AssetData>", <`[0]`="AssetData"]
    n4 = CallFunction AssetRegistryHelpers:CreateAssetData(InAsset=n1.SourceAnimation)
  }

  function MoveAnimation {
    input {
      SourcePath: string
      DestinationPath: string
    }
    output {
      Success: bool = false
    }
    n1 = FunctionEntry() [bIsEditable=True] : ->n2
    n2 = CallFunction EditorAssetLibrary:RenameAsset(SourceAssetPath=n1.SourcePath, DestinationAssetPath=n1.DestinationPath) : ->n3
    n3 = FunctionResult(Success=n2.ReturnValue) [bIsEditable=True]
  }
}
```

Call it with `gc_invoke asset_path:"/Game/Tools/EU_RetargetBody" function:"RetargetBody"
args:"SourceAnimation=obj'<source clip>', SourceMesh=obj'<source mesh>', TargetMesh=obj'<the MetaHuman's
Body mesh>', Retargeter=obj'<your retargeter>'"`. The result is written at the content root as
`/Game/MH_<source clip name>`; move it into place with `MoveAnimation`. A destination that already
exists refuses the move: move the older one aside first (a rename keeps it), never over it.

**Then play it on the character through `.gcmove`**, `Jump = clip(source=/Game/.../MH_A_Jump) { }` on a
file whose `character:` is the MetaHuman Blueprint, so `[settle=]`, `[bake]` and Move's checks apply.
Move audits every frame against the MetaHuman's own Physics Asset and refuses a new or deeper overlap
beyond 0.05 cm, naming both bodies and the frame. Fix the source motion, never the check: measured on a
standing jump (2026-10-06), the thighs met as soon as the hips flexed (a hip-width stance and knees
tracking out over the toes fixed it) and the hands met overhead at a 100-degree arm swing (75 fixed it).
A joint's `side` sign belongs to its own rig, so read the direction from the audit's numbers after one
try rather than carrying it over from another rig.

The measured development fixture uses `HumanMotion.gcmove`, `IK_AnimeSource.gc` and
`RTG_AnimeToMetaHuman.gc`. Those names record the evidence chain; packaged users should read their
own source rig and target assembly rather than depending on repository-only fixture paths.

### 9.3 Author the face track without implying semantic reach

The original raw-curve proof uses two layers:

1. `FacePerformance.gcmove` creates a face-skeleton carrier sequence. Its Move statement is a no-op root shift; it does not semantically author the expressions.
2. `A_FaceGreeting.gc` authors exact float curves on that sequence, including native controls for blinking, mouth-corner motion, cheek raise and jaw opening.

For new authored expressions, reuse [FACE-EXPRESSIONS.md](FACE-EXPRESSIONS.md) and its portable adapter/example
instead of rebuilding that raw-curve route by hand. `drive()` emits its native curves directly; the adapter
already names `smile`, `blink` and `jaw_open`. The two-layer proof above remains a valid earlier example.
The remaining boundaries are a broader calibrated expression library, arbitrary custom-face/capture transfer
and explicit ownership/blending when speech or another source already controls the same curves. These three
presets do not supply a shared body/face clock or a new performance player.

Preserve the face sequence's native skeleton and the assembly's Face post-process. Do not replace RigLogic with a custom `.gcrig` build.

### 9.4 Resolve overlap once

Head, jaw and eyes can be animated by more than one layer. Write down one owner for each channel family before building:

| Channel family | Recommended owner in this workflow | Conflict to avoid |
|---|---|---|
| Root, pelvis, torso, limbs | Body sequence | A face or additive layer moving body bones |
| Neck and head macro orientation | Body sequence | A face/head control also rotating the same hierarchy |
| Eyelids, lips, cheeks, expression | Face sequence/native face system | Body-bone approximations of facial deformation |
| Jaw expression | Face sequence/native face system | Simultaneous jaw-bone animation from body and face |
| Eye gaze | One explicitly chosen face/gaze layer | Independent body aim and face gaze fighting each other |

The measured face asset also writes `HeadControlSwitch=0`. Treat that as part of this fixture's explicit control policy; do not assume every MetaHuman or facial source needs the same value without reading its control setup.

Independent gaze was not proved by the measured trial. If gaze is required, create a separate acceptance clip and verify both eyes, lids, head compensation and extreme angles.

### 9.5 Time base and synchronization

Use one declared time contract:

```text
start time       same logical event / sequence origin
display rate     same authored fps where possible
duration         explicit and checked on both assets
loop policy      same or deliberately different
root motion      body owner only
facial pre-roll  explicit if the face solver needs it
```

The measured body and face clips are both 24 fps and eight seconds. The Blueprint starts both on `BeginPlay`. That is enough for the prototype observation; it is not a general one-clock guarantee under pausing, seeking, networking, looping or long playback.

For production, choose one Unreal composition owner—an AnimBP/linked setup, Sequencer, or a purpose-built performance player—and make it responsible for time, looping, state changes and recovery. Do not put two unrelated timers in two components and call them synchronized.

### 9.6 Layer and additive ordering

The measured proof is parallel component playback, not an additive facial layer baked onto the body sequence:

```text
Body sequence → Body component
Face sequence → Face component → native face post-process/RigLogic
```

If additional body layers are needed, resolve them in the chosen Unreal body animation graph or Sequencer before the face copies/consumes body pose. If a facial source is additive, define its reference pose and application order explicitly. GraphCode does not currently infer additive order across the Body and Face components.

### 9.7 Playback prototype

`BP_ClothedPerformance.gc` is the measured development orchestration example. It:

- derives from the assembled character;
- hides selected preset grooms for this review character;
- sets Body and Face to tick poses for reliable preview;
- starts the saved body and face AnimSequences at `BeginPlay`.

Use it as evidence and a learning fixture, not as a universal production AnimBP.

## 10. Preview, render, runtime, and reload

These are different tests:

| Surface | What it proves | What it does not prove |
|---|---|---|
| Human identity preview | Transient fitted identity and measurements | Saved rig, assembly, animation or runtime behavior |
| Asset/sequence render at a time | Deterministic pose/curve evaluation at that time | Stateful history, gameplay state or long-run synchronization |
| `gc_render` with `simulate:true` | Time advances through Blueprint/component behavior | Packaged runtime, networking or every LOD |
| PIE/runtime play | Live component and animation-graph behavior | Fresh-process reload unless restarted |
| Fresh Unreal restart and replay | Saved dependencies and reload stability | Packaged build behavior |
| Packaged-runtime test | Cooked asset and runtime behavior | Editor-only authoring paths |

`gc_render` seeking evaluates a requested time; it does not replay all prior state. Use `simulate:true` when Blueprint events, animation state or other stateful systems must advance continuously. For native world collision, CharacterMovement and rigid bodies, also set `physics:true` on an Actor Blueprint preview and include its floor/colliders in that assembly. This uses the components' authored physics settings; it does not automatically make a character ragdoll. The measured performance README recommends rendering the Blueprint with `simulate:true`, `at:0`, `to:8`, and a review capture rate. The capture rate may differ from the clips' authored 24 fps; do not confuse capture sampling with clip time base.

For every accepted combined performance:

1. Read the saved assembly and resolve its actual Body and Face components; do not guess generated paths.
2. Read back each animation with the exact `gc_read` kind returned by live help (`animation` aliases to `anim_sequence` in the current help catalog).
3. Render representative frames: neutral, body extreme, blink, smile, jaw open and recovery.
4. Simulate the whole interval from time zero.
5. Reopen the project in a fresh process and repeat playback.
6. Re-run the upstream Human program as a dry run and confirm whether it reuses identity/rig/assembly without new service requests.
7. If delivery is packaged, test a packaged build separately.

The measured fixture passed combined observation, fresh-process saved playback, and zero-request unchanged Human planning. It did **not** prove packaged runtime.

## 11. Current limits and refusals

Do not promise these as shipped:

- one `.gcmove` program that authors coordinated body and face tracks on two skeletons;
- a single GraphCode performance asset/player with a shared clock for MetaHuman Body and Face;
- automatic mapping of arbitrary expression words, phonemes or custom-face data; the shipped
  [three-control face adapter](FACE-EXPRESSIONS.md) supplies explicit `smile`, `blink` and `jaw_open` mappings;
- calibrated transfer of arbitrary custom facial blendshapes or capture data;
- realtime speech/phoneme solving, lip-sync transfer to an arbitrary custom face, or independent gaze generation;
- safe reconstruction of native MetaHuman body or face meshes through `.gcrig`;
- preservation of an already-rigged arbitrary custom mesh through the current rebuild path;
- fuzzy retarget-chain acceptance without explicit review;
- automatic measurement extraction from every clothed reference;
- stale-safe acceptance of a transient Human identity preview;
- packaged-runtime, networking, LOD-transition, root/contact or long-run sync acceptance based only on the current greeting fixture;
- collision-free or anatomically correct motion based only on `reach(frame)` meeting its endpoint;
- runtime carrying or automatic contact selection from the static-target `grab()` proof.

Offline MetaHuman speech is implemented: a Face-part `.gcmove` can build `clip(speech=<SoundWave>)` through Epic's
MetaHuman Animator. Follow [Speech in the hair-and-cloth guide](ANIMATION-HAIR-AND-CLOTH.md#speech) for the required products, mouth/full
face modes, native playback and current timing limits. Its demonstrated face bake does not establish a measured
audio/face clock in a packaged game. The coordinated Level Sequence audio-writing extension and stricter facial
curve ownership/consumer checks remain separate work; do not infer them from this speech capability.

Some operations are refused by design because they hide ownership or cost:

- inline `[rig=...]` in Human instead of an explicit `rig()` entry;
- raw DNA or arbitrary expression animation inside `human {}`;
- rebuilding native MetaHuman Body/Face through the custom rig mesh builder;
- guessing chains, components or generated assembly paths when they can be read.

## 12. Troubleshooting by boundary

### Identity or assembly

| Symptom | Check first | Corrective action |
|---|---|---|
| Human plan starts unexpected service work | `gc_apply` dry-run result and changed upstream identity inputs | Stop; inspect target, recipe digest and declared rig/assembly stages before applying |
| Assembly output is rejected | Exact `<BuildRoot>/<Name>/BP_<Name>` declaration | Fix the build map; do not invent a parallel path |
| Fit matches face but not clothed body | Declared landmarks and measurements | Add measured inputs and validate garment/body separately; reference fitting is not hidden-body reconstruction |
| Preview looks right but cannot be accepted safely | Preview is transient and acceptance staleness is open | Preserve the recipe edits, discard the preview, then run a reviewed saved build |
| Grooms obscure fit | Presentation layer is enabled during construction review | Temporarily hide the groom; do not delete it or fuse it into the base |

### Rig or retarget

| Symptom | Check first | Corrective action |
|---|---|---|
| `kind:skeleton` lacks enough bone data to author | Wrong readback surface | Use `gc_read kind:"rig"` for a `.gcrig` starting point |
| Native face behavior disappears | MetaHuman mesh was rebuilt or post-process replaced | Restore the native assembly meshes, face post-process and DNA/RigLogic path |
| Fingers drive limbs after retarget | Automatically guessed chain mappings | Replace them with explicit source/target chain mappings |
| Motion bends on the wrong axis | Raw world-axis assumption | Read the rig's semantic axes and `points_to` declaration |
| A region captures nearby skin | Region chosen only by volume | Check surface connectivity and split neighboring surfaces/layers |
| Garment penetrates only in motion | Neutral-pose-only fit | Test representative extremes, cloth owner, hidden-body policy and LODs |

### Move or face

| Symptom | Check first | Corrective action |
|---|---|---|
| `character:` animation targets the wrong mesh | `part:` and actual Blueprint component overrides | Read the character binding; bind the exact resolved part |
| `drive()` builds a curve but nothing moves | Consumer does not use that exact curve | Inspect the target face/control system; a curve name alone is not semantics |
| Head snaps or doubles | Both body and face own head orientation | Assign head/neck macro motion to one layer and disable the competing control |
| Jaw doubles | Body jaw bone and facial jaw curve both animate | Keep jaw expression in the face owner for this workflow |
| Eyes and lids disagree | Gaze and expression layers use different owners/calibration | Test a dedicated gaze clip and make one layer authoritative |
| Body plays but face is frozen | Face sequence, tick policy, Face component or post-process is missing | Compare against the measured Blueprint and native assembly components |
| Face plays but body pose does not reach face | Native face post-process/body-copy dependency is absent | Restore the assembly's native component/post-process arrangement |
| A still render differs from playback | Stateful animation was only sought/evaluated | Render with `simulate:true` from time zero |
| Two clips start together but drift/loop apart | Two independent players have no shared clock contract | Move ownership to one AnimBP, Sequencer or performance player |

## 13. Verification ladder

Do not jump from “the file parsed” to “the character pipeline works.” Climb the ladder:

```text
1. Parse
   └─ exact current grammar accepted
2. Plan
   └─ dry run names outputs, replacements and external requests
3. Build
   └─ expected asset classes and exact paths exist
4. Read back
   └─ rig hierarchy, bindings, curves and metadata match intent
5. Static render
   └─ materials, seams, pose and expression frames are plausible
6. Simulated playback
   └─ body, head and face move together through the full interval
7. Fresh-process reload
   └─ saved dependencies reopen and play
8. Regression plan
   └─ unchanged inputs cause no accidental rebuild/service request
9. Delivery gates
   ├─ root/contact error when relevant
   ├─ gaze and facial extremes when relevant
   ├─ cloth, hidden-body and LOD transitions
   ├─ long-run/loop synchronization
   └─ packaged runtime when shipping
```

A useful acceptance matrix for a new character is:

| Gate | Custom source | MetaHuman body | MetaHuman face | Combined playback |
|---|---:|---:|---:|---:|
| Neutral pose and seams | Required | Required | Required | Required |
| Extreme body pose | Required | Required | Observe copy/follow | Required |
| Blink/smile/jaw | N/A unless custom face | Observe head stability | Required | Required |
| Retarget chain review | Source side | Target side | N/A | Required |
| Static seek render | Required | Required | Required | Required |
| Continuous simulation | Required | Required | Required | Required |
| Fresh restart | Required | Required | Required | Required |
| LOD and packaged runtime | As delivery requires | As delivery requires | As delivery requires | As delivery requires |

## 14. Published routes and exact current help

Everything required to choose and verify this workflow is shipped inside the plugin:

```text
Characters
├─ generated source parts + organic assembly ... ORGANIC-PART-ASSEMBLY.md
├─ identity, Rig, body and face composition .... this guide
├─ clothing .................................... GARMENT.md
├─ hair ........................................ GROOM.md
├─ exact current grammar ....................... gc_describe / tool(help:true)
├─ current product/kind coverage ............... CAPABILITY-MAP.md
└─ validation and delivery gates ............... VALIDATION.md
```

Use these published routes:

- [Organic part assembly](ORGANIC-PART-ASSEMBLY.md)
- [Garment workflow](GARMENT.md)
- [Groom workflow](GROOM.md)
- [Capability and product map](CAPABILITY-MAP.md)
- [Tool choice and help](TOOLS.md)
- [Validation ladder](VALIDATION.md)
- [Generated capability manifest](../../Resources/mcp-bridge/graphcode-capabilities.json)

Repository fixtures and internal implementation plans may provide development evidence, but this
published guide does not require them. When an exact spelling changes, the installed description is
authoritative; update the published workflow only when the durable decision or boundary changes.

## 15. Compact glossary

- **Identity** — the editable/generated Human character state before native rigging and assembly.
- **Assembly** — the generated native MetaHuman Blueprint and its body, face, groom and outfit components.
- **Custom rig** — the skeleton/skin/control system owned by a `.gcrig` recipe, not the native MetaHuman face stack.
- **Carrier sequence** — an AnimSequence created so another supported authoring surface can add exact sequence metadata or curves; it does not imply semantic facial generation.
- **Macro movement** — body-scale motion such as root, pelvis, spine, limbs, neck and head turns.
- **Facial performance** — eye, lid, brow, cheek, lip and jaw expression evaluated by the face system.
- **Retargeting** — transferring body motion between known source and target skeletons through explicit chain mapping.
- **Composition owner** — the Unreal system that controls time and layering across Body, Face and other components.
- **Measured fixture** — saved repository evidence that passed the observations stated here; it is not broader proof than those observations.

The durable rule is: keep construction modular, preserve each layer's source of truth, cross skeletons only through explicit retargeting, and combine body plus face where Unreal owns time and components.
