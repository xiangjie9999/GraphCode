# Align a standing foot to a slope

[Animation](ANIMATION.md) · [Rig calibration](RIG.md) · [Animation layers](ANIMATION-LAYERS.md)

Use `align=surface` when a declared foot should follow the real collision beneath it.
The foot moves up or down and tilts toward the local support plane, as far as its
ankle limits allow. Full sole alignment is optional: a heel or edge may stay lifted.
Its incoming heading and authored roll guide the requested orientation.

This is a continuous contact correction. It does not create a walk, choose a landing,
infer stance timing or shift the body to balance. Author the motion first. For walking
and running profiles and rolling plants, use [WALK-AND-RUN.md](WALK-AND-RUN.md).

## Declare the contact you want to align

The skeleton needs an IK chain with at least three connected bones and declared
anatomical limits for every participating bone. [RIG.md](RIG.md) owns that setup.
Use contact points calibrated against the actual sole in the rig's rest pose.
They can come from the skeleton's calibration or the `controls:` adapter.

Select points attached to the chain's endpoint. A foot chain ending at the ankle
can use heel, forefoot and outer-edge points rigidly attached to that ankle bone.
A rolling-plant contact attached to a separate ball or toe bone is not interchangeable
with one of these points. Use distinct names when both kinds exist.

The example assumes `HeelPad_L`, `ForefootPad_L` and `OutsidePad_L` already belong
to the same left-foot endpoint. Replace the asset paths and contact names with yours:

```gcmove
gcmove 1 into:/Game/Motion skeleton:/Game/Characters/Hero/SK_Hero_Skeleton ik_rig:/Game/Characters/Hero/IKR_Hero controls:/Game/Characters/Hero/Contacts.gcrig
build {
  CR_StandingFoot = constraints(collision=world, contacts=($HeelPad_L,$ForefootPad_L,$OutsidePad_L), align=surface, slope=45)
}
```

Keep the build declaration on one line. Build through the normal `.gcmove` workflow.
The result is a native Control Rig. Creating it does not connect an Animation Blueprint.

That example selects only the left foot. For a two-foot standing pose, include the
right foot's calibrated endpoint contacts in the same constructor:

```gcmove
build {
  CR_StandingFeet = constraints(collision=world, contacts=($HeelPad_L,$ForefootPad_L,$OutsidePad_L,$HeelPad_R,$ForefootPad_R,$OutsidePad_R), align=surface, slope=45)
}
```

Each contact names its own chain in the rig. A foot that is not selected receives
no terrain correction from this rig; it can remain inside or above the floor.

Use at least three well-spaced points to sample a sole. The solver uses collision
normals, so fewer points can run, but they provide less evidence of support. Points
do not cover the complete skin or sole area. Calibrate each character; do not copy
another skeleton's offsets as human measurements.

Cover the toe tip and the sole boundary over the foot's expected rotations. Three
points from a flat standing support facet can miss a toe that becomes lowest during
push-off. A solved point is only evidence for that point. Inspect the visible sole.

Measure the contact points on the visible sole, not at an assumed `z=0` or at the
ankle bone. A point below the skin can reach the ground while the foot still floats.
Start from a standing pose whose sole support plane is level. A mesh's reference bone
rotation does not guarantee a level sole. Keep the heel, forefoot and outside support
on the intended plane, then inspect the actual skin after solving. This matters when
skin weights spread a contact area across several bones or when the toes are articulated.

## Choose the policy

| Argument | Meaning |
| --- | --- |
| `align=surface` | Signed placement using the ankle's allowed terrain tilt. |
| `align=none` or omitted | Existing upward-only clearance correction. It does not pull a hovering foot down onto support. |
| `slope=45` | Default maximum accepted surface inclination, in degrees relative to `up`. Allowed values are at least 0 and less than 90. This is a collision policy, not an ankle angle. |
| `up=+Z` | Default component-space up axis. Signed X, Y and Z axes are accepted. |
| `channel=Visibility` | Default project trace channel. The support must block it. |
| `tolerance=0.1` | Default distance tolerance in centimetres. Must be positive. Used for plane agreement and residual penetration warnings. It is not a guaranteed final accuracy. |
| `iterations=16` | Default number of bounded joint-correction sweeps. Allowed integers: 1–64. More sweeps can reduce error, but cannot guarantee a solution. |

These are engineering settings. They are not timing or angle recommendations for a person.
The character's rig owns anatomical limits.

Runtime correction uses the rig's recorded hinge directions and angle convention, as the
motion bake does. Tiny residuals within native transform precision do not trigger a reset
to the reference pose. The authored joint ranges remain unchanged.

## What happens during evaluation

1. Enforce the declared joint limits on the incoming pose.
2. Capture the incoming endpoint pose. Trace every selected point along `up`, within
   the chain's rest-length reach on each side of the point.
3. Require all samples to hit an accepted local plane. Mixed tread heights, conflicting
   normals, missing hits and planes nearly parallel to the trace direction are rejected.
4. Request the terrain tilt while preserving horizontal heading. Limit that orientation
   at the ankle, find the lowest calibrated point at the allowed angle, and place it
   onto the plane. Keep the ankle's incoming horizontal position; correct height along `up`.
5. Solve the endpoint position by rotating the declared chain joints. Respect the declared
   hinge axes and reapply each joint's limits. A two-segment hinge chain with no locked root
   axes first calculates the bend needed for its reach, then turns the root toward the goal.
   Other chains use bounded joint-correction sweeps. Segment translations are retained.
6. Recheck the ankle angle and contact as the parent joints move. Report the final errors.

The support plane and requested tilt are captured once per evaluation. Each solve sweep
updates the landing target for the ankle angle allowed by the current parent pose.
The incoming endpoint scale is included when transforming the calibrated offsets.
An authored heel lift remains part of the request. The solver does not force every point
flat, turn the whole leg to supply forbidden ankle twist, or lower the body automatically.

If acquisition is invalid, the chain keeps its incoming pose after joint limiting.
There is no invented floor or retained target from the last evaluation. If acquisition
succeeds but the joints cannot reach the target, the rig reports the remaining error.
It does not stretch the bones or declare that no possible pose exists.

### Why the knee can move too

Rotating at the ankle changes the foot's orientation, but does not move the ankle's
position. Turning the sole about a ground contact can require both. The declared leg
chain then adjusts its hip and knee to reach that ankle position. The incoming knee
bend is part of the authored pose; alignment does not choose a natural standing pose.

The ankle handles tilt. The hip and knee handle the change in ankle position. They do
not swivel to supply forbidden ankle twist. Compare incoming and final joint angles
before judging how much alignment bent the leg. This is not a balance or effort model.
Locked joints remain locked, with a residual if the remaining motion cannot reach the
contact target. An orientation-only correction does not establish supported sole contact.

## Compose it with the performance

Use a native Control Rig node in the host's animation composition. Feed it the intended
upstream pose each evaluation and select the generated rig. Evaluate it after pose contributions that would
otherwise move the corrected foot. [ANIMATION-LAYERS.md](ANIMATION-LAYERS.md) owns
breathing, walking and action-layer composition.

Every selected chain is active whenever this rig runs. Use it for standing, or explicitly
gate its evaluation in the host. A rig containing both feet corrects both feet; it has
no separate inferred left/right stance schedule. Do not run it unconditionally on swing
feet. Native blending can soften activation, but partial blending also weakens contact.
Inspect the pose after the final blend.

Surface correction acts on the declared chains. The existing joint-limit pass still
enforces declared limits elsewhere in the rig. There is no additional pelvis or body solve.
Including a body bone in a chain explicitly allows that bone to participate. Check all
other contacts affected by those joints. The rig does not coordinate a held rifle,
the other foot or a hand gripping a rail unless their requirements are supplied elsewhere.

## Read the result and inspect it

For a chain named `leg_L`, inspect these native rig curves:

| Curve | Meaning |
| --- | --- |
| `GC_GroundValid_leg_L` | 1 means a candidate support plane was acquired. It does not prove the target was reached. |
| `GC_GroundError_leg_L` | Maximum distance from the final calibrated points to the last ankle-limited landing target, in centimetres. |
| `GC_GroundAngle_leg_L` | Difference from the full requested terrain tilt, in degrees. This can remain nonzero when the ankle reaches its limit. |
| `GC_GroundPenetration_leg_L` | Remaining sampled penetration after correction, in centimetres. |
| `GC_GroundHits_leg_L` | Number of accepted final sample hits. |
| `GC_GroundMissing` | Total missing final sample hits across the selected chains. |

`GC_GroundPenetration` is the worst chain penetration. Existing joint-limit diagnostics
remain available. Error and angle curves are zero when acquisition is invalid; always
read validity too. Zero penetration alone can mean a hovering or unsupported foot.

Test level, uphill, downhill, both sideways slopes and an oblique slope. Start above
and below the support. Inspect the skin, heel, forefoot, heading, knee and other held
contacts. Also test missing collision, excessive slope, a foot spanning two tread heights,
an unreachable target and the transition back to the source pose.

The implementation was checked through the public constructor on a native three-bone
humanoid leg and a standing pose with both feet active. The standing fixture uses measured
sole vertices, actual collision and rendered contact close-ups. Those fixtures do not
certify your character's sole calibration, whole-body balance or animation transitions.

A later moving check used authored walking and running clips with separate stance gating.
It found gaps, penetration and added foot sliding on flat ground and 8-degree slopes.
Some fully active contacts found a valid plane but still missed it by several centimetres.
The moving result does not pass contact acceptance. Check final skin contact after joint
limits and blending; a valid trace or a successful rig build is not a successful placement.
An ankle continuity repair reduced measured flat-ground contact jumps, but repeated moving
checks still show floor crossings. Interpolation alone does not keep a sole on its support
or reconcile two planted feet that demand different body motion.

## Current limits

`constraints(from=$Walk, ...)` is not implemented. There is no compiled plant-phase carrier,
persistent moving-platform anchor, automatic body balance, full-sole area proof or future
landing search in this mode. Runtime ladder climbing and hanging need those separate
contact and body-coordination pieces; surface alignment alone does not supply them.
