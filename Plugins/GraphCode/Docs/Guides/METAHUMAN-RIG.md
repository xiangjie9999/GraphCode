# A MetaHuman body rig from the shared mapping

Read [GCRIG.md](GCRIG.md) first. This is the MetaHuman adapter quick start; [CHARACTERS.md §7.2](CHARACTERS.md#72-native-metahuman-assets-are-a-preservation-boundary) covers preservation of native Body, Face and skeleton assets.

A MetaHuman body can take `.gcmove` clips directly, with no retarget. You write them with the same `axis=fold|side|twist` words used for a custom rig. The skeleton stays Epic's: every bone, place and frame comes from the body mesh. A `.gcrig` over that mesh, the *adapter*, says only what each core joint does: which way it closes, how far it bends, sides and twists, which bones make each limb, and where the hands hold things.

**Start from the shared mapping:** the shipped template already declares 84 body/placement joints:
70 movable controls and 14 deliberate rigid carriers. It includes the relaxed animation rest, palm
cupping, toe-off and distal toe curl. A new character inherits this work; its adapter normally needs
only `parent:`, `mesh:`, `into:` and an `ik_rig()` output. Read the inherited comments when choosing a
motion, rather than recreating the joint ranges or calibrating raw X/Y/Z axes. Face animation remains
native and separate (§9).

**Set up one character:**

1. Copy [Examples/MetaHumanBody.gcrig](Examples/MetaHumanBody.gcrig) into your project, for example to `Content/Rig/MetaHumanBody.gcrig`. It contains no mesh or positions. In OraStudio, reuse the installed `/Game/GraphCode/Rig/MetaHumanBody` as `parent:` instead of making another copy. Keep character-specific changes in the child so updates to the shared map carry through.
2. Next to it, write the character's file, `Content/Rig/AvaBody.gcrig`, with your own body mesh and folder:

   ```gcrig
   gcrig 1  parent:/Game/Rig/MetaHumanBody
            mesh:/Game/Characters/Ava/Body/SKM_Ava_BodyMesh
            into:/Game/Rig
   build {
     IKR_AvaBody = ik_rig()
   }
   ```

3. Plan with `gc_apply asset_path:"/Game/Rig/AvaBody" dry_run:true`, then apply without `dry_run`. Read the notes for this body's rest and limits. An unmodified standard body map produces 84 joint settings, including 14 rigid settings, and 30 chain goals. The adapter keeps the imported skin and bone frames and builds the IK rig; it does not rebuild the native body mesh. Render a short pose on the actual character to check its proportions and clothing.
4. Animate the character with that file and its IK rig in the header:

   ```gcmove
   gcmove 1 character:/Game/Characters/Ava/BP_Ava part:Body
            controls:/Game/Rig/AvaBody  ik_rig:/Game/Rig/IKR_AvaBody
            into:/Game/Characters/Ava/Anim fps:30
   build {
     A_AvaRaise = $Raise
   }
   Out = profile(0=0, 1=-70)
   Bend = profile(0=0, 1=90)
   Raise = clip(frames=30) {
     swing(upperarm_r, axis=side, by=$Out)
     swing(lowerarm_r, axis=fold, by=$Bend)
   }
   ```

Over one second the right arm rises out to the side and the elbow bends. Bone names, joint ranges and grip sockets all come from the copied file, so nothing else is typed. On the example's right arm `side=` is negative outward.

```text
the character's body mesh ...... Epic's bones, places and frames, fitted to this character
└─ MetaHumanBody.gcrig ......... shared: chains, joint ranges from anatomy studies, grip sockets; no mesh
   └─ <Character>Body.gcrig .... parent: the shared file, mesh: this body, build { IKR_… = ik_rig() }
      └─ .gcmove ............... character: + part:Body + controls: the adapter (+ ik_rig: for limits and IK)
```

**What the shared file holds:**

- **Chains** along each limb's real parents. Arms start at the clavicle, so a reach does not bend the spine. The arm goal is on the hand, not the metacarpal. Since 2026-10-14 also:
  - `neck` (`neck_01 -> neck_02 -> head`) for a look: `aim(neck, from=$Face, at=$Target)` with
    `Face = frame(bone=head, toward=+Y, up=+X)`, because the head bone's +X is not where the face looks (its +Y is). The
    only other chain to the head, `spine`, starts at the pelvis, and an aim on it turns the hips and the whole back; a
    chain from `spine_05` moves both shoulders. KNOWN FAULT (2026-10-14, board row MOVE-INTENT1 item 12): with an `aim`
    in a clip the solve also turns spine_01-05 and the look stops short (15 deg on BlueEdge key 1); until it is fixed,
    turn the head with `swing` and check the face's direction in the readings.
  - `stand_l/r` (thigh to ball) and `heel_l/r` (thigh to foot) to put feet on footholds while the hips stay as posed:
    `leg_l/r` start at the pelvis, and a reach turns its whole chain. Reach the ball to a foothold frame with `stand_`,
    and for a foot on its ball also the ankle to a point with `heel_`, which fixes the knee and the heel's height.
- **Speeds** for effort timing on the pelvis (the hip's references), the collarbones (half the shoulder's) and the toe
  joints (the ankle's), derived and marked so in the file; without them a timed travel that turned them was refused.
- **A weight-bearing ankle:** dorsiflexion 39 (Baumbach 2014, weight-bearing lunge), not CDC's 13.8, which is measured
  knee straight and unloaded; with 13.8 a crouch or lunge hit the stop and the IK bent the toes into the foot.
- **84 intentional joint declarations:** root/pelvis, spine, neck/head, clavicles, arms/hands, finger metacarpals, fingers/thumbs, hips, knees, ankles, balls and toes. The ranges follow [HUMAN-JOINT-RANGES.md](HUMAN-JOINT-RANGES.md) and the template's cited studies; explicitly labelled modelling allowances cover gaps. Root/pelvis are placement envelopes, not clinical ROM. At the ankle, spine, neck and collarbone a study's pair is shifted by the measured rest bend; the template also covers the two measured bodies' palm/ball offsets. A substantially different body still needs its build notes and pose checked.
- **Palm and foot controls:** ring/pinky metacarpals cup the palm; index/middle metacarpals are rigid. Each ball controls the toes' common MTP motion, and each distal `_02` toe curls independently. Proximal `_01` toes are rigid carriers so the same MTP motion is not counted twice. This model does not provide independent MTP motion for each toe or separate PIP/DIP joints for lesser toes. Corrective, slide and twist helpers remain owned by the native MetaHuman post-process.
- **Grip sockets**, `palm_grip_r` and `palm_grip_l`, stated in each hand bone's own frame. The build writes them onto each character's body mesh, at that body's palm, and never onto Epic's shared skeleton. Attach a prop to one in the Blueprint with no offset ([HANDS.md](HANDS.md)). A socket name the shared skeleton already uses is refused, because it would shadow that socket. When holding requires a skinless palm carrier, follow HANDS.md's measured route on separate `/Game` mesh and skeleton copies; never add bones to Epic's shared skeleton.

**To differ for one character**, restate just that joint in the character's file, for example a stiffer knee. A chain
declared with the same name as the shared file's replaces it.

An inherited finger rest is adjustable: `rest=(fold=0)` explicitly replaces its fold; omission
inherits, and a bare joint name makes it rigid. Choose a legal rest in this body's range and
read [RIG.md](RIG.md#main-declarations-each-holding-one-kind-of-line) before changing the defaults.

**Reuse motion on another MetaHuman:** build that body's adapter/IK rig, then rebuild the authored
motion with its `character:`, `controls:` and `ik_rig:` paths. Semantic axes resolve against the new
body, but body proportions, foot contacts, reach targets and collision clearance still need inspection.
To transfer an already-baked clip across skeletons, use the [explicit native retarget workflow](CHARACTERS.md#9-flagship-recipe-metahuman-face-plus-gcmove-body-and-macro-motion).
GraphCode can author the retargeter's mappings and poses; its guides use Unreal's native batch operation
to bake the result. There is no single automatic transfer-and-repair command.

For named `smile`, `blink` and `jaw_open` controls, use the separate
[FaceExpressions adapter](FACE-EXPRESSIONS.md) on `part:Face`; it does not rebuild the native face.

**What choosing a MetaHuman does not give you yet** (each is in the OraStudio project's
`Content/GraphCodeExamples/BlueEdge_20261013/BUILD.md`, "THE WORK", with its fix):

- **A palm carrier bone** for a held thing to ride (HANDS.md): a bone is a mesh change, so it goes in the character's file
  on `/Game` copies of the mesh and skeleton, never on Epic's. A chain ending at `hand_r` is needed with it to AIM a held
  thing (`aim` turns a chain's last bone; `arm_r`'s is the metacarpal): `MetaHumanSwordRig.gcrig`'s `sword_r`.
- **A fighter's or thrower's ranges.** The shared ranges are an ordinary adult's, measured in a clinic. A body that cocks
  a sword or a ball over the shoulder needs its anatomical frame and arm-position-dependent allowance checked.
  BlueEdge withdrew the old universal110deg override after it also allowed a bad low-arm pose; do not copy it.
  The template's±73.8deg is a symmetric modelling choice from active arm-at-side external-rotation data, not measured
  MetaHuman capability. See `Docs/Research/fighter-shoulder-external-rotation.md` for current provenance, sign and
  elevation limits. Read requested versus kept joint values and resolve solver/roll ownership before widening a range.
  A range that widens only with the arm raised is `twists_raised=` on the child's `upperarm_r`, never a wider `twists=`
  ([RIG.md](RIG.md#a-twist-range-that-follows-the-bones-elevation-twists_raised)); a `.gcmove` reads it and holds the joint's twist at the arm's own elevation (a rig without the key is unchanged).
- **Sole outlines for `balance`.** Balance stands a body on planted feet with measured sole outlines; this template has
  none, so they are measured per body until the tool measures them from the skin.
- **A look that moves the eyes.** The eyes are the Face's: MetaHuman's face system (RigLogic) turns them from the face
  curves `CTRL_expressions_eyeLook{Left,Right,Up,Down}{L,R}`, and a `.gcmove` with `character:<BP> part:Face` writes a
  face clip (the lip-sync clip does). Not there yet: how many degrees a curve value turns an eye (gaze was never
  calibrated or proved, CHARACTERS.md 9.4), and one look target that turns the head on the body clip and the eyes on the
  face clip together (Docs/GC-MOVE-METAHUMAN-PERFORMANCE-DESIGN.md section 7, designed, not built).

**Verified directions for the added controls** (the body faces +Y, its left is +X):

| Joint / control | Positive | Negative |
|---|---|---|
| `root` / `pelvis`, `fold` | Tip forward | Tip backward |
| `root` / `pelvis`, `side` | Lean to the character's left | Lean right |
| `root` / `pelvis`, `twist` | Turn right: left shoulder advances | Turn left: right shoulder advances |
| `ring_metacarpal_l/r`, `pinky_metacarpal_l/r`, `fold` | Cup inward toward the palm | Uncup within the range |
| `ball_l/r`, `fold` | Curl toes downward | Extend toes for toe-off |
| `bigtoe_02_l/r` and other toe `_02` bones, `fold` | Curl the distal toe | Return toward straight within the range |

These directions were checked on native clips; palm cupping and toe-off/curl were also viewed on both
sides. The original 52 declarations retain their existing values and comments. Their complete bilateral
side/twist sign audit is still in progress; use a documented direction where present and a small semantic
pose check where a comment marks a sign unverified. Do not infer one side from the other.

**Updating an existing adapter:** replace its shared template copy, review local overrides and rebuild
the child IK rig and affected clips. The new central toe chains change the ball's geometric rest reading;
remove old ball overrides if they were temporary mapping work. Final shared ranges are
`ball_l (-159.8, -57.2)` and `ball_r (-159.8, -59.2)`. Keep only intentional character-specific restrictions,
documenting their purpose. A local pelvis declaration identical to the parent is redundant. Existing baked
animations and IK assets do not change merely because their parent text changed.

1. **Point the `.gcmove` at the character** with `character:<BP> part:Body controls:<the character's file>`. Add `ik_rig:<the built IK rig>` for `reach`, `plant`, `aim` and `grab`, and to apply the joint limits. A clip without `ik_rig:` gets the bend directions but no limit check.
   - Move reads the joint axes from the adapter and solves on this character's own fitted body; the shared skeleton is only a template. The clip records that body as the one it was authored on (its retarget source), so playback does not apply the body's difference from the template a second time. A clip built before 2026-10-09 needs one rebuild.
   - An adapter built for a different body mesh is refused.
2. **Use the inherited directions, then compose.** The table above and template comments carry the verified mapping. On the example's right upper arm, outward `side` is negative. Where an older control's sign remains explicitly unverified, check one small semantic pose and record the result in the shared mapping so the next author can reuse it. Where study endpoints differ and the sign remains unchecked, the shared file allows the larger endpoint both ways.
3. **Judge every pose on the character itself**, with its head and clothing, not on the template body. The template has no head, so a hand in the face goes unseen there.
4. **For a hand that stops on what it holds**, the hand needs collision bodies of its own skin. A MetaHuman's physics asset gives the hand one coarse body and the fingers none. Build `physics_asset() [bodies=hull, gives=0.35]` in a separate file over the character, because it takes minutes. Then name it as the Body's `PhysicsAssetOverride` in the Blueprint that holds something.

Worked example, `Content/GraphCodeExamples/MetaHumanBody_20261009/`:

- `ClothedReferenceBody.gcrig`: one character's file, four lines over the shared one.
- `ClothedReferenceBodyHulls.gcrig`: hull bodies for that skin. The build takes about 6 minutes.
- `Wave.gcmove`: the right arm out, rolled up and waving, made from side, twist and elbow fold only.
- `Reach.gcmove`: the left hand to a point in front of the chest, landing within 0.1 cm as the engine plays it on the character.
- `BP_MetaHumanSword` and `SwordGrip.gcmove`: the character holds a sword at `palm_grip_r`. The sword is attached to the inherited `Body` with `@attach[Parent=Body, SocketName="palm_grip_r"]`. The thumb is lifted off the handle first; the fingers and thumb then close with `collision=(self, $Blade)` and end with `collision($Blade)`.
- `BP_MetaHumanWaveViewer` and `BP_MetaHumanReachViewer`: the clothed character playing each clip.

The same shared file, unedited, also built over Epic's template body, which has different proportions. All its limits held that body's rest pose, and its grip sockets landed on that body's own palm.

These files drive a MetaHuman that must exist in your project; point `mesh:` and `character:` at your own.

The face stays native; [CHARACTERS.md §9](CHARACTERS.md#9-flagship-recipe-metahuman-face-plus-gcmove-body-and-macro-motion) composes body and face.
