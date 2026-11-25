# Walk, breathe and act together

[Back to animation](ANIMATION.md)

Use Unreal's native Animation Blueprint to combine motions during gameplay. Walking with
breathing is a normal example: the legs keep their walking rhythm while the chest follows
its own breath cycle. Changing walk speed should change breathing only when the authored
performance calls for that response.

A correct result keeps the base action readable. Breath does not reset at each footstep,
pull the feet off the floor or break a two-handed grip. An upper-body action can fade in
while walking and breathing continue. Inspect the combined character with its props.

This guide describes the UE 5.7 native workflow and existing GraphCode authoring route.
The graph diagrams show connections, not executable `.gc` text. Read the target graph
and current tool grammar before writing its node statements.

## Choose what each asset owns

| Part | Responsibility |
|---|---|
| Walk/run source sequences | The gait and its authored contacts. Build with `.gcmove`, import or retarget. See [WALK-AND-RUN.md](WALK-AND-RUN.md). |
| Breath source sequence | A repeatable breath around a deliberately chosen neutral pose. See [ANIMATION.md](ANIMATION.md#natural-movement-is-part-of-the-performance) for authoring and judging body detail. |
| Native animation sequence settings | How the breath becomes a difference from its neutral pose: additive type and reference. |
| AnimGraph | Separate playback clocks, addition, masks, weights and order. |
| Final native constraints | Restore any foot, hand or prop contacts disturbed by the combined pose. |

Reuse the character's existing AnimBP and movement setup. This composition uses native
graph nodes; it needs no new `.gcmove` instruction. A Move build produces animation assets.
It does not automatically connect them into the character's runtime graph.

## 1. Prepare the base and the breath

Start with a working walk or locomotion graph. Check whether it already contains visible
breathing. Add only the missing contribution; two complete breath cycles can fight or
exaggerate each other.

Prepare a separate breath sequence on the intended skeleton. Author the chest and other
chosen joints around a known neutral pose. Keep unintended local channels at that neutral
pose. Keep the root fixed relative to the reference and exclude travel from this layer.
Disabling root-motion extraction alone does not remove a moving root track.

Judge the breath on the actual character. The selected torso motion also moves descendant
bones, including the shoulders and hands. A local channel with no keys changing does not
mean that bone stays fixed in world space. Use [RIG.md](RIG.md) for the character's axes and
controls, and [HANDS.md](HANDS.md) for grips and prop relationships.

Choose amplitude, inhale/exhale timing and transitions from the intended performance.
There is no universal chest angle or breathing rate in this recipe. A graph weight is an
animation control, not a physiological measurement or an automatic fatigue model.

## 2. Give the breath the correct additive reference

An additive sequence describes a change from a reference pose. Applying that change to
the walking pose lets walking continue underneath. The reference is the neutral pose
used to author the breath, not simply whichever pose is easiest to select.

The native AnimSequence exposes these fields through GraphCode's `settings` block:

| Native property | Choice for this workflow |
|---|---|
| `AdditiveAnimType` | `AAT_LocalSpaceBase` for local-space additive breathing, used with **Apply Additive**. |
| `RefPoseType` | Choose one of the reference modes below to match the source. |
| `RefFrameIndex` | The chosen neutral frame when using a frame reference. |
| `RefPoseSeq` | The reference animation when using a different animation as the base. Use the intended skeleton. |

| Reference mode | When it is correct |
|---|---|
| `ABPT_RefPose` | The breath was authored around the skeleton's reference pose. A standing neutral is not automatically the skeleton reference pose. |
| `ABPT_LocalAnimFrame` | A known frame in the breath sequence is the intended neutral. Set `RefFrameIndex`. |
| `ABPT_AnimFrame` | A known frame in another animation supplies the neutral. Set `RefPoseSeq` and `RefFrameIndex`. |
| `ABPT_AnimScaled` | A whole reference animation supplies the changing base over normalized time. This needs an intentionally matched reference performance. |

For example, **only if frame 0 is the authored neutral**, this is the settings fragment
for the breath asset's existing `kind:anim_sequence` document:

```gc
settings {
  AdditiveAnimType = AAT_LocalSpaceBase
  RefPoseType = ABPT_LocalAnimFrame
  RefFrameIndex = 0
}
```

This fragment is not a complete create request. Read the asset, retain its current identity
and signature, and patch the settings through `gc_apply`. Read the fields back and inspect
the evaluated animation after saving and reloading. If `.gcmove` rebuilds the sequence,
check the settings and evaluated result again.

Use **Apply Mesh Space Additive** only for a source configured as
`AAT_RotationOffsetMeshSpace`. Its rotation space differs from local-space addition;
changing the node without matching the source changes the result. The **Mesh Space
Rotation Blend** option on Layered Blend per Bone is a separate blending choice.

Do not mark a complete walking performance additive and put it over another walk unless
its deliberately matched reference removes that base performance. A mismatched neutral
can add a permanent crouch, arm offset or root movement along with the breath.

## 3. Connect two players

Keep the existing locomotion output. Add a looping Sequence Player for the additive
breath. Connect them through **Apply Additive**:

```text
Locomotion output --------------------> Apply Additive: Base
Breath Sequence Player ---------------> Apply Additive: Additive
Breath amount ------------------------> Apply Additive: Alpha

Apply Additive output -> required contact/IK correction -> final pose
```

| Control | What to set or connect |
|---|---|
| Breath player's `Sequence` | The prepared additive breath asset. |
| Breath player's `bLoopAnimation` | Enable for sustained breathing. The authored cycle itself must also join cleanly. |
| Breath player's `PlayRate` | Its own rate input. At `1`, normal player scaling uses the asset's rate; inspect asset `RateScale`, `PlayRateBasis` and any scale/bias settings too. |
| Breath player's `StartPosition` | An authored starting phase. Stable variation between characters can prevent a crowd inhaling together. |
| Breath player's synchronization | Use `Method=DoNotSync` and no locomotion `GroupName` for an independent cycle. |
| Apply Additive's `Alpha` | Breath amount. With the normal float input and unchanged scale/bias, `0` leaves the base pose and `1` applies the full authored contribution. |

Keep the breath player outside the individual idle, walk and run states when its phase
should survive their transitions. Keep locomotion's foot synchronization within the
locomotion branch. Do not connect movement speed directly to breath rate by accident.

Expose separate controls for breath amount and rate. Changing amount changes how much
motion is added. Changing rate changes how quickly its cycle runs. Blend these controls
over time for exertion or recovery; the author chooses that response.

**A separate player is not an always-running clock.** Apply Additive stops updating its
additive branch when its effective alpha is no longer relevant, and its LOD threshold
can also disable that branch. Graph relevance and state reinitialization affect playback.
For a visible continuous breath, keep its branch active across the required states.
If breath must continue advancing while invisible, explicitly design and verify that
timing path. Do not assume that setting alpha to zero preserves a continuously advancing phase.

## 4. Mask only when needed

If the breath has zero additive change outside the intended local channels, Apply Additive
may be enough. Add a native mask when the same source must affect different body regions,
or when the source contains contributions that this performance should exclude.

**Layered Blend per Bone blends ordinary poses.** For a masked additive breath, first
apply the additive to the base, then blend the resulting ordinary pose with the same base:

```text
Cached locomotion --------------------> Apply Additive: Base
Breath player ------------------------> Apply Additive: Additive

Cached locomotion --------------------> Layered Blend per Bone: Base Pose
Apply Additive output ----------------> Layered Blend per Bone: Blend Pose 0

Layered Blend per Bone output -> required contact/IK correction -> final pose
```

Use native **Save Cached Pose / Use Cached Pose** so both paths use the same evaluated
base. Do not wire the raw additive difference into an ordinary pose blend input.

The node's `BlendMode` selects a branch filter (`LayerSetup`) or a skeleton blend mask
(`BlendMasks`). A branch filter starts from a chosen bone and affects its descendants
according to its depth settings. A blend mask supplies per-bone influence. Read the
actual skeleton; there is no universal chest bone name or correct filter depth.

Keep root, pelvis and leg influence out of a chest-only breath unless their movement is
intentional. Check the shoulders, neck and hands too. Masking their local channels does
not cancel movement inherited from a turning chest.

Choose one main control for breath amount. For example, drive Apply Additive's alpha and
leave the masked blend's layer weight at full strength. Fading both reduces the contribution
at both stages. For multiple overlapping layers, inspect their order and resulting pose.

## 5. Add actions and preserve contacts

The same graph can support a wave, a reload, recoil or aiming. Choose the native mechanism
for the asset you actually have:

| Contribution | Typical native route |
|---|---|
| Full upper-body action over walking | An ordinary action pose, often from a montage slot, with Layered Blend per Bone selecting its region. |
| A small difference from a known base | Apply Additive with a matching additive source and reference. |
| Directional aiming | An authored Aim Offset with compatible samples and additive settings. |

Order is part of the performance. If breath should remain visible during a reload, add
it to the result of the action blend. If an action needs a held torso, fade or restrict
breath for that interval. Test interruptions as well as the complete action.

Keep one owner of actor/root travel. Inspect `CurveBlendOption` and
`bBlendRootMotionBasedOnRootBone` when using Layered Blend per Bone. A bone mask alone
does not establish ownership of all curves, events or root motion. Keep foot phase and
contact-enable curves under the intended owner; avoid unrelated curves in the breath asset.

Apply required contact correction after every contribution that could break those
contacts. Use the character's verified native IK or Control Rig path, with the spaces
and conversions that path requires. A torso layer can disturb a two-handed weapon hold
even when the feet remain correct. See [HANDS.md](HANDS.md) and the runtime example in
[STAIRS-IN-A-GAME.md](STAIRS-IN-A-GAME.md).

Neither Apply Additive nor a bone mask supplies automatic balance, collision avoidance
or the rig's authored joint-limit enforcement. Inspect the final pose after corrections.
If keeping a contact requires an impossible reach, adjust the layer, base or contact plan.

## 6. Attacks and combos: one clip per action

A game's attack chain is not one long clip. Author each action as its own `.gcmove` clip that starts and ends
at rest on a key pose (a guard, the top of a rise, a landing), and let the engine sequence and blend them:

| Part | Owns |
|---|---|
| One clip per action (`A_Cut_Rise`, `A_Cut_Down`, ...) | The action's own timing: its own effort clock and ramp, its own key poses. |
| An `anim_montage` with one section per action | The order, which section follows which, blend in/out times, notify windows (the live/hit interval, the trail, the combo input window). |
| The AnimBP: guard or locomotion -> `Slot` -> output | What plays between attacks; the montage blends out back to it, so no clip has to author the return. |
| A Blend Space | Locomotion only (speed, direction): it mixes clips by a parameter, not in sequence. |
| The character's input logic | Which section plays next: a press inside the window jumps to the next section; no press lets the montage blend out. |

Measured on BlueEdge (2026-10-15, `BlueEdge_20261013/BUILD.md` "Motion round 4"): one joined clip of a rise, a
turnover and a downcut cost most of a day in its JOINS, not its poses. One clock for both cuts softened the
landing; a free-arm swing across both cuts became the slow joint; the return to the guard swept the blade
through a leg and had to be authored as a fourth path. Split per action, each clip only has to be right on its
own, the engine's blend covers the seam, and the combo becomes a player choice (press, press) instead of a
fixed take. Keep the realism where the player sees it (where the blade points, where the feet stand); a short
engine blend between two at-rest poses is accepted game practice.

## Author through GraphCode

Use existing native asset and AnimGraph authoring:

1. Read the intended AnimBP and sequence assets with `gc_read`. Use live state when
   preserving unsaved editor work. Inspect their existing players, references and contacts.
2. Ask `gc_describe` for `kind:anim_sequence` and the target AnimBP's graph grammar.
   Use the tools' own help for current arguments. Resolve the native node classes,
   exposed pins and properties before writing a graph payload.
3. Patch the breath asset settings and the existing AnimGraph through `gc_apply`, using
   the current signatures and aliases. The diagrams above define the required wiring.
   Reuse existing variables or author the needed controls through the supported grammar.
4. Compile, save, read back and play the actual character. Stored settings and a successful
   graph compile do not prove that the additive reference or final contacts are correct.

The current graph writer edits existing AnimGraphs. Do not assume this recipe also
creates linked animation-layer interfaces or a new graph lifecycle.

`clip(source=...)` currently accepts a non-additive sequence on the selected skeleton.
Play the configured additive asset through the native graph; do not feed it back through
that Move source path. Copying a source clip's motion also does not copy its notifies or
Sync Markers. Deliver those on the intended native output and verify their timing.

## Check the combined performance

| Check | Required result |
|---|---|
| Breath amount at `0`, then at the intended maximum | The base is preserved at zero; the maximum has the intended pose without reference offsets or excessive combined angles. |
| Slow walk, faster walk, then run | Gait changes correctly. Breath rate changes only through its own authored controls. |
| Idle/walk/run transitions and action interruptions | No unintended breath restart, pose pop or abrupt amplitude change. |
| Loop seam and long playback | Breath joins cleanly and does not accidentally synchronize to footfalls. |
| Feet and held prop throughout the cycle | Contacts survive the final composition, including inhale, exhale and action extremes. |
| Weight-zero intervals and LOD changes | Phase and reactivation behave as intended; verify any required invisible-time continuity. |
| Save/reload and source rebuild | The additive reference, settings and evaluated motion remain correct. Recheck markers and events. |

A single flattened AnimSequence has one playback clock. Speeding it up speeds up all
motion baked into it. Keep separate native players when runtime timing must remain independent.

## Sources

- Epic Games, Unreal Engine 5.7, *Animation Blueprint Blend Nodes*:
  https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-blueprint-blend-nodes-in-unreal-engine?application_version=5.7
- Epic Games, Unreal Engine 5.7, *Using Layered Animations*:
  https://dev.epicgames.com/documentation/en-us/unreal-engine/using-layered-animations-in-unreal-engine?application_version=5.7
- Epic Games, Unreal Engine 5.7, *Animation Sync Groups*:
  https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-sync-groups-in-unreal-engine?application_version=5.7
- Epic Games, installed UE 5.7 source: `Engine/Source/Runtime/Engine/Classes/Animation/AnimSequence.h`,
  `AnimEnums.h` and `AnimNode_SequencePlayer.h`; `Engine/Source/Runtime/AnimGraphRuntime/Public/AnimNodes/AnimNode_ApplyAdditive.h`
  and `AnimNode_LayeredBoneBlend.h`; `Engine/Source/Runtime/AnimGraphRuntime/Private/AnimNodes/AnimNode_ApplyAdditive.cpp`.
  These define the native fields, enum choices, pose inputs and branch-update behavior used here.
