# Balance a planted body

`balance()` adds a body-level constraint to `.gcmove`. It compensates for a reaching arm or lifted leg while retaining the authored contacts and joint limits. Use `swing`, `reach`, poses and profiles to describe the action, and balance to keep its mass projection over the supporting soles.

```text
Posture = bones(pelvis,spine_01,spine_02,spine_03)
Forward = point(at=(0,5,0))

// Inside a clip with measured finite sole plants on a horizontal floor:
balance(pelvis,over=(support_L,support_R),bones=$Posture,at=$Forward,margin=.5,max=8)
```

The point above is in component coordinates. Choose the forward direction and coordinates for your character; the verb does not assume that +Y means forward on every rig. Existing `point(bone=foot_r,at=(...))` can express a foot-relative preference. Project the intended component-space point onto the support plane before converting it into that bone's local coordinates. The full local point is transformed before its horizontal projection is taken, so an arbitrary local Z can affect X/Y when the foot rotates.

## Each parameter

| Parameter | Meaning | Why it is needed |
|---|---|---|
| Positional bone, e.g. `pelvis` | The declared native full-body IK root, retarget root or skeleton root whose translation may be corrected. Required. | Identifies the movable body anchor on different rigs. Translating an anatomical endpoint is refused because it would change bone length. The anchor is not a declaration of actual COM. |
| `over=(support_L,support_R)` | The planted chains allowed to carry the body's weight. Required, with distinct bare chain names. | A contact elsewhere on the body need not be load bearing. Released or partially weighted plants do not supply full support area. |
| `max=8` | At most 8 cm of additional anchor translation from this sample's ordinary solved pose. Required; finite and nonnegative. | Bounds how far balance may change the action. The budget restarts at each sample; it never accumulates drift. `max=0` allows rotation-only correction. |
| `bones=$Posture` | Additional bones permitted to rotate. Optional. Active support and active reach paths are already permitted. An active zero-offset positional reach grants rotation to its path joints, excluding endpoint rotation unless facing or explicit posture permission needs it. | Gives deliberate permission for torso compensation. Released plant paths need explicit permission here to receive further correction. Ordinary `swing` remains a pose preference; positive movement-block `at(angle=...)` ownership locks that bone's whole local rotation, including an explicit zero angle. |
| `at=$Forward` | A preferred horizontal mass projection. Optional; component-fixed or relative to a currently supporting endpoint. | Expresses front/back/side weight placement. Without it, a safe, valid pose stays unchanged. The preference is clamped to safe support and can remain unmet; inspect the reported shortfall. |
| `margin=.5` | Insets the actual touching support hull by 0.5 cm. Optional; default 0. | Reserves space from the edge. A margin that consumes the whole support area refuses the build. |
| `weight=$Steady` | Influence from 0 to 1, or a profile. Optional; default 1. | Times preparation and recovery. Zero disables balance. Intermediate influence fades the requested COM task while preserving contacts, limits and the translation budget. Only full influence certifies full static balance. |

There is one balance owner per skeleton. A `clip(from=$Other)` variant may replace its inherited owner. Composing independent owners refuses instead of choosing one silently. Time balance with `weight`; the line has no modifiers.

## What the solver actually measures

The selected `physics_asset:` supplies each native body's configured mass and local centre. At a candidate pose, the solver computes:

```text
posed_COM = sum(body_mass * posed_body_centre) / sum(body_mass)
```

This includes the native local COM offset and reference scale. Missing bone mappings and animated mass-body scale changes refuse. Snapshotting uses disposable native actors and does not alter asset defaults or a live simulation. These values describe the selected collider model. They are not automatically anatomically calibrated human segment masses. Configure and retain the native Physics Asset you want to use; rebuilding a generated rig can regenerate its Physics Asset defaults.

For full influence, the horizontal projection of that COM must lie inside the inset convex hull of the **actually touching skinned sole points**. Holding an ankle transform alone is insufficient: skin influenced by neighbouring joints can still tilt away from the floor. Each fully planted finite sole is therefore independently certified, including a plant outside `over`.

Correction starts from the ordinary native IK/FK result. It searches permitted legal joint rotations and, when needed, the bounded anchor translation. Bone lengths and scales remain unchanged. Contacts remain binding; a reach retains its ordinary best-effort residual bound and may improve when the permitted body motion makes its target more accessible. A small preference for the original pose and previous correction helps select between feasible solutions. The local search does not promise a globally optimal pose. Where a sample cannot be balanced within `max=` with its soles seated, it keeps the correction the last balanced sample made, all of it or as much as its soles still seat, so the body does not drop back to its uncorrected pose for one sample; the note says `not attained; the correction frame N made is kept` and no balance certificate is claimed for it (MOVE-BALSMOOTH1). On a standing clip the correction is then planned over the whole clip: each sample's own correction is found first, and where those jump (a rolling foot's support changing in steps) they are smoothed into one that moves as the body does, each sample still held by its soles and, where it was balanced, by the balance audit. The note `balance's correction planned over the clip` says how far its centre of mass steps against the body it was handed; `gc.Move.BalancePlan 0` corrects each sample alone, for comparing.

For a weighted positional reach, the FK end of its blend follows the chain-parent body frame through the additional balance correction. At the ordinary baseline this is exactly the original FK point; at full reach the target is unchanged. This lets a nearly released arm follow the moving torso instead of pinning its old world-space FK position. Repeated identical task inputs reuse an independently recertified ordinary/contact pose, preventing a constant source stance from slowly changing through repeated optimisation.

Candidate selection also checks interpolation from the preceding actual pose. Individually valid endpoints can otherwise switch to a different joint solution and lose contact between them. Contact objectives fade toward the ordinary contact pose with influence, while the contact certificates stay binding. These checks preserve the trajectory's constraints; they do not impose a joint-speed or acceleration limit.

Permission matters: allowing only the spine can leave a forward preference unmet. Add the pelvis to `bones` when whole posture rotation is intended; its translation permission alone does not grant rotation. The build reports preference shortfall and missing native angular limits, so a free body anchor must not be mistaken for an anatomically calibrated joint.

Final keys, quarter intervals and native compressed playback are independently rechecked before saving. Balanced clips use the engine's ACL Safe codec with full quaternion rotations, 0.0001 cm positional precision and frame stripping disabled to preserve precise contacts. Every balanced target's native compressed representation is checked on a disposable sequence before any persistent output changes. A correction invalidated by interpolation or compression is refused. Those checks apply to the baked clip; later runtime blending or warping needs its own validation.

## Kicks, reaching, and falling

Balance constrains horizontal support, **not COM height**. During a high kick the torso might lean back, sideways, bend, or combine these changes. Keeping the projection over one foot does not require the torso to move downward or keep the whole-body centre at the same height. Add a specific `swing` or pose as a form preference; use existing `at(angle=...)` ownership when an angle must remain binding.

### A reach and the weight: the later line wins

When a `reach` and `balance` cannot both be met, the line written later wins, and the build says how far the other gave way (the order rule; user-ruled 2026-10-26). Where both can be met, both are, whatever the order.

- **`reach` then `balance`:** the weight stays over the feet and the limb gives way: it ends short of its place, and the build says by how much (`'heel_r' could not be put where the file asked - 5.8 cm short`). The balance is certified. A controlled strike.
- **`balance` then `reach`:** the limb gets to its place (as far as `max=` lets the body move for it) and the weight goes where that takes it. The build says `a reach written after balance wins: the weight ends N cm outside its support` and claims no balance certificate. An over-committed strike, a lunge, a fall.

For a strike at full extension you rarely need either: the bubble's fullest reach, asked with `key:`, already counts where balance leaves the hips with the limb out, so its line is met with the weight over the feet ([HUMAN-THROW-AND-STRIKE.md](HUMAN-THROW-AND-STRIKE.md), "A strike at full extension"). Write the order for what the body should keep when you ask for more than that.

### Lifting a foot: `lead=`

With no `weight=` and no `at=`, balance moves the weight before a foot in `over=` lets go:

```gcmove
plant(support_L, on=$Floor, contact=(...), weight=$LeftPlant)   # the left foot lets go at 0.32
plant(support_R, on=$Floor, contact=(...))
balance(pelvis, over=(support_L,support_R), bones=$Posture, margin=.5, max=20)
```

- The weight starts moving `lead=` before the release: 0.3 s by default, `lead=0.5s`, `lead=frame(12)`, or a
  fraction of the clip. It eases to the middle of the support that stays and is there on the last frame before
  the release.
- It stays over the standing foot while the other is up, and eases back over the same span once every `over=`
  foot bears full weight again.
- `lead=0` turns it off. A release then has to put the weight over the standing foot in a single frame, which the
  support audit refuses ("balance not attained at frame N … outside safe support"): the reason the lead exists.
- `lead=` beside an authored `weight=` or `at=` is refused, because your shift owns the timing.
- The build notes each lead, for example "(support_L) lets go at frame 39; the weight eases onto (support_R) from
  frame 29, there at frame 38, held to frame 108".
- A release with no support remaining (both feet leave) gets no lead: flight is outside static balance.

To time it yourself instead: keep balance's influence active over the supporting foot, release the kicking plant with its existing profile, and give the leg its authored swing. Release explicit leg-angle ownership before replanting so the contact solve can restore the sole. Return the balance profile to the starting influence when the clip must close its seam.

A forward preference moves body weight within the declared support. A fully planted foot stays planted. A released foot follows its authored pose and ancestor motion; this does not invent a recovery step. There is no falling simulation or automatic foot-placement policy in this verb. Runs and run-ups are balanced by the moving rule above (the body's carried speed and the next written foothold); fast punches, pushes and slips still need dynamic reasoning about momentum, contact forces and friction beyond it.

## Balance while moving: runs, run-ups and stops

Where plants drive the root (`plant(..., drive=Root)`), the body carries its own speed between the footholds you wrote
([WALK-AND-RUN.md](WALK-AND-RUN.md), "The body carries its speed"). `balance()` reads that speed. There is no new word: the
same line balances a stand and a run.

**What it keeps over the support.** Standing, the centre of mass (CoM). Moving, the *extrapolated* centre of mass, the point a
body moving at speed `v` is heading for (Hof's XCoM, the capture point):

```text
XCoM = CoM + v / w0        w0 = sqrt(g / l)
g = the project's gravity (Project Settings > Physics), l = the CoM's height over the floor
```

A body at 4 m/s with its CoM 95 cm up leads by about 1.25 m: far past the stance foot. Judged against the stance foot alone,
every running step would look like a fall, and balance would brake the run. So the XCoM is judged against **the support now
united with the next foothold you wrote**, moved back to the present by when that foot lands and half its stance (Raibert's
neutral point for running). A run whose footholds keep its pace already has its XCoM inside: balance changes nothing and the run
loses no speed. A step that lands short of where the speed is carrying the body is what balance corrects, by turning the
posture bones you allowed in `bones=`.

| Situation | What balance does |
|---|---|
| Standing, or a moving clip at zero speed | exactly the standing rule above: the CoM over the touching sole |
| Moving, a later foothold written | the XCoM over the support now or the next foothold, timed by the schedule |
| Moving, no later foothold (the last step) | the XCoM over the support it stands on: a stop. A clip that ends mid-run is judged as a stop at its end, so write the stop, or read the note |
| In the air (no driving plant weighs) | nothing: there is nothing to stand over, and the arc is gravity's |
| `at=` written | `at=` states where the CoM goes, so the speed is not read; the target is the CoM, in the correction and the final audit |

**The driven root is the body's, not balance's.** The root your plants drive follows the body's carried speed. Balance never
translates it (as the subject it gets `max=0`); it turns the posture bones and, when the subject is the pelvis, moves it within
`max=` over the held feet.

**Rolling feet are supports.** A rolling plant (`plant(leg_L, on=$Floor, roll=(Heel_L, Ball_L, Toe_L), drive=Root, ...)`)
supports with the skin that actually touches the floor, within 0.1 cm, about the pivot it is on (heel, ball or toe). The skin is
measured once from the character's mesh around the roll bones; no radius is invented. A rolling foot often touches along a line
(the heel at strike, the ball at push-off); while the body moves toward a next foothold that is fine, because the region judged is
the hull of what touches now and where the next foot lands, inset by `margin=` once. A *stop* has no next foothold, so it is
judged on the touching patch alone, and a stop on a heel line refuses by name: nobody stands on a line. Skin pushed more than
0.1 cm into the floor refuses the build, and the rolling foot's attained frame is kept exactly. A root-driving plant without
`roll=` is still refused with balance.

**A standing foot rolls too** (MOVE-ROLLSTAND1, 2026-10-25). A rolling plant with no `drive=` that a balance stands on -- a
cross's rear foot on its ball, `plant(leg_r, on=$Floor, roll=(Heel_R, Ball_R, Toe_R))` -- is a support the same way: its touching
skin is bound from the mesh and seated by the leg above the paw (never the pelvis, which carries the other leg), and no flatness
is asked of it. Planted flat instead, such a foot is refused (every flat sole must stand within 0.1 degree). A foot that stays down
whole, a cross's lead foot, is planted flat: `plant(leg_l, on=$Floor)` stands it flat through the leg above it (MOVE-PAWFLAT1, fixed
2026-10-26: before, only its ankle could turn and it stood 9-12 degrees off) and its whole sole supports. Fixtures:
`MetaHumanStrike_20261025/StrikeCross.gcmove`, `A_StrikeCrossBalanced` (both feet rolling), and the spec
`GraphCode.Move.Program.ABalancedCrossMovesItsWeightSmoothly` (the lead foot flat).

**What the build says.** One note per stance: *"balance frames 211-250: largest incoming XCoM clearance shortfall 3.2 cm at
frame 216; next scheduled foothold kept it inside: yes"*. A shortfall is how far the body arrived outside before the correction;
"next scheduled foothold kept it inside: yes" says the run was carried by where the next foot lands, as a run should be.

**Limits.** The root stands in for the whole body's speed, and the next-foothold rule is the constant-pace running approximation,
not a physical stability certificate. Sudden changes of direction, slips and pushes are outside it.

**Known in this build (2026-10-12).** Balance on a run can still refuse at a frame where a rolling foot's skin presses more than
0.1 cm into the floor, which the support audit rejects: the measured Runner's run refuses at its first frame (*"balance not attained
at frame 0 … rolling skin penetrates the floor by 0.29 cm"*), where it also asks a sideways correction of about 7 cm that is still
being checked. The carried speed itself does not need balance; build the run without `balance(...)` until this is fixed.

### A balanced run-up into a kick

```gcmove
Posture = bones(pelvis, spine_01, spine_02, spine_03)
RunKick = clip(frames=357) {
  ...                                                 # the strides, the plant, the kicking leg, the trunk: drawn
  travel(Root, velocity=$Pace)                        # the approach and the braking you intend: it moves the body
  plant(leg_R, on=$Floor, roll=(Heel_R, Ball_R, Toe_R), weight=$OnR)
  plant(leg_L, on=$Floor, roll=(Heel_L, Ball_L, Toe_L), weight=$OnL)
  balance(pelvis, over=(leg_L, leg_R), bones=$Posture, margin=.5, max=20)
}
```

Balance keeps the body over the support foot through the strike because the support foot is planted where the speed carries
the body, not because it pulls the body back. If the notes show large shortfalls at the plant, move the plant (or slow the pace
before it), not balance: [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md), "One clock for the whole body".

## Current accepted scope and examples

Active balance uses a skeletal mesh, native IK rig and configured Physics Asset, with stationary horizontal coplanar finite soles, and (MOVE-ROOT1) rolling plants that drive the root, judged as above. A `character:` file's selected body is balanced with the parts attached to it at a socket (a held sword at its Mass (kg) override or its collision's density; MOVE-INTENT1 item 10); a part attached with no socket rides no bone and is named in a note as not weighed. `collision(self)` runs beside balance (2026-10-14, MOVE-INTENT1): balance picks a posture already clear by the final self-collision audit, the fit then runs once on it, and a fitted pose that lost its support is balanced again on the fitted pose, with a build note saying so (`Balance_20261009/BalanceWithCollision.gcmove`: a stance, a 120-frame reach and the high kick, each certified with it). Aims stand beside balance (2026-10-14): a part's lean is a direction aim (`aim(clavicle_r, from=$Collar, toward=(right, up=20), relative=spine_05)`), balance keeps every aimed bone fixed relative to its parent while it moves the body, and a bone both aimed and given to balance in `bones=` is refused by name. Bones given in `bones=` and not aimed are still balance's to bend, and they carry the aimed parts above them: a catwalk whose chest turn and look are aims, balanced with `bones=` on the lower back, bent the lower back to move the weight and left the chest 10 and the eyes 33 degrees off their aims. When aims shape the trunk, leave `bones=` out and balance moves the hips (`balance(pelvis, over=(leg_l, leg_r), max=8)`; `Content/GraphCodeExamples/MetaHumanBody_20261009/Catwalk.gcmove`). On a body whose capsules already overlap at rest (the thighs), name the body's skin-checked Physics Asset in the header (`physics_asset:`), or every balanced posture that presses them is refused. Other moving or driving supports, simulation, staged grip and body-contact owners remain refused with active balance. A reach whose target follows another constrained chain also requires a staged solve and is refused with balance; otherwise the stages could silently discard its body-level task. An always-disabled reach does not create that dependency. This restriction concerns a reach target, not balance's own support-relative `at=` preference. Normal final joint-limit and self-collision checks remain binding.

The public fixture lives in `Content/GraphCodeExamples/Balance_20261009`: `BalanceRig.gcrig`, measured `BalanceContacts.gcrig`, `StandingBalance.gcmove`, three reach variants, three leg-extension variants, a facing interaction and an explicit zero-angle interaction. `HighKick*` names denote a higher authored hip angle than `LowKick*`; these are waist-height test extensions, not a polished head-height martial-arts performance. Preview actors include the actual floor so contact can be judged against geometry.

The standing controls compare neutral, forward, half, zero and safe no-target motion. The longer controls exercise preparation, support release, extension, return and replant. Numerical audits establish the declared kinematic contracts; render inspection assesses visible compensation and motion continuity. Neither proves human anatomy or a finished performance. Current measured receipts and parameter decisions are recorded in `Docs/Research/balance-native-animation-validation.md` and `Docs/GC-MOVE-BALANCE-PLAN.md`.

The far-reach control reduces peak hand miss from 14.54 cm to 0.19 cm using permitted body compensation, with contacts retained. Its brisk arm recovery still needs performance refinement. The facing control retains attained orientation, but the existing ordinary frame-facing IK has a sharp transition also seen without balance. Balance is worthwhile for coordinating posture and support; natural fighting technique still needs authored form and timing and better IK continuity in those cases. The tested parameters each serve a distinct purpose; no extra height, global-COM or optimizer parameter is needed for this scope.
