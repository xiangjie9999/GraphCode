# Stairs in a game: the capsule climbs smoothly, the feet stand on the steps

The usual game technique, built only from Unreal's own nodes, every one written as `.gc` text. GraphCode
gives you the pieces and this guide gives a set of rules that works; the rules are yours to change. A
working copy is in [Examples/StairsInAGame](../Examples/StairsInAGame/), on Unreal's tutorial character,
idle and walk, which every install has.

For an AUTHORED climb (a `.gcmove` clip with planted feet) read [STAIRS.md](STAIRS.md) instead. This guide
plays an ordinary walk and idle and fixes the legs at runtime, on any staircase.

**Measured on the example** (2026-10-07; 16 cm risers, 30 cm treads, walking at 100 cm/s, side view at
0.5 cm per pixel; the example's files created exactly as shipped measure the same to the pixel): the
head rises 4.5–6.5 cm every 0.1 s from the first step to the last, as evenly as the capsule, with no stall
and no surge; each landing foot is within 0–3 cm of its step; after a stop the back foot comes down onto
its step over about 1.5 s and both soles stay within 0.5 cm. The cost: late in each step the back foot
floats up to about 20 cm before it lifts (see "What it does not do").

## How it works

1. **The capsule rides a ramp.** An invisible ramp through the step noses gives Character Movement a
   smooth slope, so the capsule, and a camera on it, rise steadily instead of jumping a riser per step.
2. **Each foot finds its step.** Every frame, a trace goes down under each ankle and each ball on a channel
   the ramp ignores, and the foot is moved up or down by how far its step is from the clip's floor.
3. **The foot that touches down carries the body.** At the moment a foot comes down, the hips take its
   step's height and keep it until the next foot comes down. The capsule keeps climbing meanwhile, so the
   body rises with it evenly. (Following the planted foot for its whole stance instead drags the hips down
   as the body passes over it and springs them up at the next step: a normal walk step covers about two
   treads, so the capsule climbs about 32 cm while one foot is down.)
4. **At rest the hips sink slowly to the lower foot**, so the back foot comes down onto its step.

## Step 1: collision — a ramp for the capsule, steps for the feet

Read [STAIRS.md, "Common runtime collision setup"](STAIRS.md#common-runtime-collision-setup-a-ramp-for-travel-treads-for-feet).
Two ways to make the ramp:

- **In the stair mesh** (GC Shape): `SM_Stairs = $Stairs [collision=$Ramp]` makes the ramp the mesh's
  simple collision, which Character Movement uses, while the drawn steps stay its complex collision,
  which the foot traces use.
- **As a hidden component** beside any stair mesh: a box tilted to the stair's slope whose top face runs
  through the step noses. It blocks the character (Pawn) and ignores the foot channel (here
  `Visibility`); the steps do the opposite. The example course writes it as:

  ```gc
  Ramp = StaticMeshComponent(BodyInstance.CollisionProfileName=Custom, BodyInstance.CollisionResponses.ResponseArray=((Channel=Visibility,Response=ECR_Ignore),(Channel=Camera,Response=ECR_Ignore)), BodyInstance.ObjectType=ECC_WorldStatic, bVisible=false, CastShadow=false, Mobility=Static, StaticMesh=obj'/Engine/BasicShapes/Cube.Cube')
    @attach[RelativeLocation=(X=210.941,Y=0.0,Z=46.235), RelativeRotation=(Pitch=28.0725,Yaw=0.0,Roll=0.0), RelativeScale=(X=2.04,Y=1.18,Z=0.04)]
  ```

  For your stairs (rise `r`, run `d`, first nose at height `r`, climbing toward +X):
  - pitch `θ = atan(r / d)` — 16 / 30 gives 28.07°;
  - the nose line meets the lower floor one run before the first nose and the landing at the last nose;
    the box's length is that horizontal span divided by `cos θ` (180 cm / cos 28.07° = 204 cm, scale X
    2.04 on the 100 cm cube);
  - the box's centre is any point on the nose line moved down along the slope's normal by half the box's
    thickness `t`: `P + (t/2)·(sin θ, 0, −cos θ)`.

The steps' own collision must block the foot channel and ignore Pawn (the example's treads:
`BodyInstance.CollisionResponses.ResponseArray=((Channel=Pawn,Response=ECR_Ignore))`).

## Step 2: the character

An ordinary Character. `MaxStepHeight` at least one riser (the ramp does the climbing, but the ends of a
flight are steps). The skeletal mesh's origin at the capsule's bottom (the usual `Z = −half height`): the
recipe measures "the clip's floor" from the mesh component's location.

## Step 3: a target bone per foot

The legs are solved to a bone that follows each foot. Unreal's mannequin skeletons have them (`ik_foot_l`,
`ik_foot_r`). A skeleton without them gets virtual bones from its root to each foot in its `kind:skeleton`
`.gc` `virtual_bones { }` block (`gc_describe kind:"skeleton" block:"virtual_bones"`); use those names
wherever the recipe says `ik_foot_l` / `ik_foot_r`.

## Step 4: an IK Rig that bends only the legs

One Limb solver per leg: it bends hip and knee to reach its goal and never moves the pelvis, so the hips
move only by the rule in step 5. `IKR_StairLegs.gc`, one `gc_apply create:true`:

```gc
gc 1  kind:ik_rig  asset:/Game/StairsInAGame/IKR_StairLegs
parent: /Script/IKRig.IKRigDefinition

settings {
  PreviewSkeletalMesh = soft'/Engine/Tutorial/SubEditors/TutorialAssets/Character/TutorialTPP.TutorialTPP'
}

goals {
  FootL = (bExposePosition=true, bExposeRotation=true, BoneName=foot_l)
  FootR = (bExposePosition=true, bExposeRotation=true, BoneName=foot_r)
}

retarget_definition {
  $pelvis = pelvis
  leg_L = (Start=thigh_l, End=foot_l, Goal=FootL)
  leg_R = (Start=thigh_r, End=foot_r, Goal=FootR)
}

solvers {
  IKRigLimbSolver(Settings.EndBone=foot_l, Settings.GoalName=FootL, Settings.HingeRotationAxis=Z, Settings.StartBone=thigh_l)
  IKRigLimbSolver(Settings.EndBone=foot_r, Settings.GoalName=FootR, Settings.HingeRotationAxis=Z, Settings.StartBone=thigh_r)
}
```

`HingeRotationAxis` is the knee's bend axis in your skeleton (`Z` on the mannequin).

## Step 5: the Animation Blueprint

`ABP_StairLegs.gc` in the example — apply it with `gc_apply create:true` after the IK Rig. The file is
long because each foot's query is written out in full; its settings, all instance-editable:

```gc
properties {
  SourceSpeed: double = 192.343097 [instance_editable]   # the walk clip's own speed, cm/s
  DownMargin: double = 3.0 [instance_editable]
  FootSpeed: double = 15.0 [instance_editable]
  PelvisSpeed: double = 10.0 [instance_editable]
  RestSpeed: double = 2.0 [instance_editable]
  TraceReach: double = 60.0 [instance_editable]
  LowerFootShare: double = 0.0 [instance_editable]
  ...the working variables: Speed, Moving, OffsetL/R, Pelvis, LowestL/R, DownL/R, Carry
}
```

and the anim graph, from the pose out:

```gc
n1 = AnimGraphNode_BlendListByBool(BlendPose_0=<walk>.Pose, BlendPose_1=<idle>.Pose, bActiveValue=<Moving>)
n14 = AnimGraphNode_LocalToComponentSpace(LocalPose=n1.Pose)
n13 = AnimGraphNode_CopyBone(...)     [Node=(SourceBone=(BoneName="foot_l"),TargetBone=(BoneName="ik_foot_l"),...)]
n12 = AnimGraphNode_CopyBone(...)     [Node=(SourceBone=(BoneName="foot_r"),TargetBone=(BoneName="ik_foot_r"),...)]
n11 = AnimGraphNode_ModifyBone(..., Translation=MakeVector(Z=OffsetL)) [Node=(BoneToModify=(BoneName="ik_foot_l"),TranslationMode=BMM_Additive,TranslationSpace=BCS_ComponentSpace)]
n10 = AnimGraphNode_ModifyBone(..., Translation=MakeVector(Z=OffsetR)) [... ik_foot_r ...]
n9  = AnimGraphNode_ModifyBone(..., Translation=MakeVector(Z=Pelvis))  [... pelvis ...]
n8  = AnimGraphNode_ComponentToLocalSpace(ComponentPose=n9.Pose)
n21 = AnimGraphNode_IKRig(Source=n8.Pose, ...) [Node=(RigDefinitionAsset=IKR_StairLegs, Goals=((Name="FootL",...,TransformSource=Bone,SourceBone=(BoneName="ik_foot_l")),(Name="FootR",...)))]
n22 = AnimGraphNode_Root(Result=n21.Pose)
```

(`...` marks what the example file spells out; apply the file, not this outline.)

What each part does:

- **Event graph** (`BlueprintUpdateAnimation`), for each foot:
  1. trace down under the ankle and the ball (`GetSocketLocation`, `LineTraceSingle` on the foot channel)
     from `TraceReach` above to `TraceReach` below the clip's floor; the foot's step is the higher hit;
  2. the foot's **offset** is that step's height above the clip's floor, smoothed with `FInterpTo`
     (`FootSpeed`) so it does not pop when a trace crosses a nose;
  3. the foot is **down** when its animated height above the floor is within `DownMargin` of the lowest
     it has been — the recipe learns that, so no ankle height is typed;
  4. when a foot comes down, the hips' carry height (`Carry`) becomes its step's offset.
  Then the hips: moving, `Carry` (blended toward the lower foot by `LowerFootShare`); at rest, the lower
  foot's offset; never above the clip. Smoothed by `PelvisSpeed` moving and the slower `RestSpeed` at rest.
- **Anim graph**: walk or idle (`Blend Poses by bool` on `Moving`) → copy each foot into its target bone →
  move each target by its foot's offset and the pelvis by the hips' (`Transform (Modify) Bone`, additive,
  component space) → the IK Rig with goals from the target bones.

## Step 6 (recommended): joint limits and sole contact after the IK

Put a GraphCode-generated Control Rig after the IK Rig node. In a `.gcmove` whose `ik_rig:` declares
joint limits (the character's full IK Rig, not the two-leg one above) and whose `controls:` calibrate
sole contacts, `CR_Feet = constraints(collision=world, contacts=(...))` keeps every joint inside its
range and lifts a sole that has sunk into a step ([MOVE-PHYSICS.md](MOVE-PHYSICS.md), "Animation floor and
live world"). Wire it as `AnimGraphNode_ControlRig(Source=<the IK Rig node>.Pose)` before the Root node.
The example leaves it out because it needs a rig of your own character.

## Step 7: check it

Render your course running, from the side, orthographic, and look at it:

```json
{ "asset": "/Game/StairsInAGame/BP_StairCourse", "simulate": true, "physics": true,
  "at": 2.0, "to": 6.5, "fps": 10, "sheet": true, "width": 800, "height": 800,
  "camera": { "location": [220, -700, 110], "look_at": [220, 0, 110],
              "projection": "orthographic", "ortho_width": 400 } }
```

- each landing: the front sole on its step, not on the ramp line above it and not inside the step;
- the head: the same rise every frame while climbing (a pause then a jump is the hips following the
  planted foot through its stance);
- after the stop: both soles on their steps within a second or two, with no pop.

## Tuning

| Variable | Default | Raise it when | Lower it when |
|---|---|---|---|
| `LowerFootShare` | 0 | the back foot floats too long before it lifts | the body dips and rises with each step |
| `DownMargin` | 3 cm | a planted foot flickers between down and up | a lifting foot still counts as down |
| `FootSpeed` | 15 | a foot lags behind its step | a foot pops at a nose |
| `PelvisSpeed` | 10 | the body settles late after a landing | the body bobs more than the walk |
| `RestSpeed` | 2 | the back foot takes too long to settle | the drop at a stop is noticeable |
| `TraceReach` | 60 cm | a step further above or below the floor is missed | traces hit things above the legs |

## What it does not do

- **The back foot floats late in its stance.** A walk made for flat ground steps about two treads at a
  time on 30 cm treads; holding the hips at the touch-down height keeps the body smooth, so the trailing
  leg cannot reach its lower step for the last part of the stance (measured up to 20.8 cm at
  `LowerFootShare` 0, just before it lifts). `LowerFootShare` lets the hips reach part of the way toward
  the lower foot while moving: at 0.3 the float fell to 12.8 cm and the head's rise per 0.1 s went from an
  even 4.5–6.5 cm to an uneven 3.0–7.5 cm. Pick by which your camera shows more. Only shorter steps
  remove it at the source (walking slower does not: the walk's play rate follows the speed, so its step
  length stays the same): Unreal's Stride Warping node, or a stair clip authored for the treads
  ([STAIRS.md](STAIRS.md)).
- **It does not plan the swing.** A swinging toe can still brush a nose; the ball trace raises a foot as
  soon as its ball is over the next step, and the generated contact rig in step 6 lifts a toe out of a step.
- **Descending, turning on stairs and running** use the same rules but were not measured here.
- **Slopes** work the same way with no ramp (the ground is its own ramp); not measured here.
