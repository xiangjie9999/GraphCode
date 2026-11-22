# Smile, blink and open the jaw on a MetaHuman

Use the shipped [FaceExpressions adapter](../Examples/FaceExpressions/FaceExpressions.gcrig) with
[`Expressions.gcmove`](../Examples/FaceExpressions/Expressions.gcmove). The named controls are
`smile`, `blink` and `jaw_open`; `drive(smile, by=$Smile)` writes the native facial curves.
You do not need to discover these seven curves again or add a new expression verb.

Read [GCMOVE.md](GCMOVE.md), [GCRIG.md](GCRIG.md) and the
[native MetaHuman preservation boundary](CHARACTERS.md#72-native-metahuman-assets-are-a-preservation-boundary)
before adapting the example. Ask `gc_describe kind:"rig" block:"controls"` and
`gc_describe kind:"move" block:"drive"` for the installed grammar.

## What the adapter contains

| Control | Native curves | Output at amount 1 |
|---|---|---|
| `smile` | `CTRL_expressions_mouthCornerPullL/R`, `CTRL_expressions_eyeCheekRaiseL/R` | Mouth corners 1; cheek raise 0.6 |
| `blink` | `CTRL_expressions_eyeBlinkL/R` | Both eyelids 1 |
| `jaw_open` | `CTRL_expressions_jawOpen` | Jaw 1 |

Amount 0 sets these channels to zero; amount 1 selects the adapter's authored maximum. Cheek 0.6
is the example's chosen smile strength, not a limit imposed by MetaHuman. The left and right
channels are listed explicitly. This controls-only `.gcrig` supplies mappings; it does not build
a face mesh, skeleton or IK rig. Keep the native Face mesh, DNA, RigLogic and post-process intact.

## Use it on your character

1. Copy both files to your project's `Content/FaceExpressions/`. The adapter needs no build output.
2. In `Expressions.gcmove`, replace `/Game/Characters/BP_Character` with your assembled MetaHuman
   Blueprint. Keep `part:Face`; the body's skeleton is a different target. If you choose a different
   folder, update both `controls:` and `into:` to match it.
3. Plan with `gc_apply asset_path:"/Game/FaceExpressions/Expressions" dry_run:true`. The result
   should describe 90 frame intervals at 30 fps, zero bone tracks and seven control curves.
   Then apply the same source without `dry_run` to create `A_Expressions`.
4. Play `A_Expressions` on that assembly's Face component using its native face playback setup;
   [CHARACTERS §9](CHARACTERS.md#9-flagship-recipe-metahuman-face-plus-gcmove-body-and-macro-motion)
   explains body/face ownership and synchronization. A body clip does not automatically play this
   face sequence. Inspect the face's actual post-process/RigLogic configuration if curves change
   but the face does not.
5. Inspect neutral, each blink, the smile's onset and hold, and jaw opening, then play the whole
   sequence. The sample has blink peaks at 1.5 s and 2.6 s, and holds an 80% smile from 1.8 s;
   the jaw stays at 15%. Frame-aligned blink peaks avoid losing closure between baked samples.
   Check the face from the front and below the fringe, with its brows, lashes and facial hair.

The channels and timing come from the project's existing native-tested three-second face example:
the recorded trial showed closed eyelids, raised cheeks and a held smile through RigLogic.
Retargeting the example to a different character still requires checking that character's face;
this is an expression-control example, not a claim that every face or groom binding is validated.

## Face presets by region, and a face timed by the body

Move ships face presets beside its body presets: `$Mouth(smile, frown, snarl, pucker, open)`,
`$Eyes(blink, wide, squint)`, `$Brows(raise, down)` and `$Cheeks(raise, puff)`, each value an amount
0..1 of its native curves, `none` (not written) when left out. A part:Face file names them and their
controls in its header:

    presets:/GraphCodeMove/Presets/Human/HumanFace  controls:/GraphCodeMove/Presets/Human/HumanFace

The controls (`HumanFace.gcrig` beside the presets) give every native curve one control, so this
smile is the mouth corners only and the cheeks are `$Cheeks(raise=)`. Ask `gc_describe kind:"move"
block:"header"` and `block:"drive"` for the exact grammar.

A face's keys are a `pose(steps)` of poses written `pose(from=reference)`, each driving the controls
it sets; each key takes the control over as it comes in. A key that leaves a value out keeps what the
key before it set (as in the body presets, a value not written is not asked), so a neutral key writes
its zeros: `$Mouth(snarl=0)`. Two drives of one control anywhere else in a clip are refused. Under effort timing the body's build decides
when each body key lands, so a face key typed at a frame drifts when the body's effort changes. Name
the body's built clip in the face's header, `body:/Game/.../A_Slash`, and land a key on a body pose:

    Calm = pose(from=reference) { $Mouth(snarl=0) }
    Snarl = pose(from=reference) { $Mouth(snarl=1) }
    Keys = pose(steps) { $Calm, $Snarl [lands_at=$Contact], $Calm }
    Look = clip() { $Keys }

The body's build stores when it reached each of its poses on its clip: a list timed by effort, or a
`pose(steps)` played over the clip with no clock (equal shares, `[at=]`, a cycle at its `[phase=]`).
Where it cannot know them (a list on a travel's clock, `repeat=`), the face's build says why. The face
reads them when it builds and takes the body's length, counted in its own frames: a face at a coarser
rate than the body lands within one of its own frames. Build the body first, and build the face again
after every body build: its build note names the body build it was timed against.

In the Pose tab, a part:Face file opens on the character's Face, with its preset values as sliders;
a drag puts the pose's curves on the face. A file with `body:` opens with that body clip on the same
character, and Play plays both on the face's clock.

## Extend and combine deliberately

- To add surprise, anger or another expression, add a named control in a project-owned adapter
  using curves actually consumed by the selected Face. These three controls are the supplied
  starting set, not a complete emotion, phoneme or arbitrary-face library.
- A child adapter replaces a same-name control's whole channel list. Reuse `parent:` for shared
  controls and calibrate the child's outputs for a character instead of redefining every clip.
- A control's `curve.` name is checked against the selected Face's DNA (its raw controls, the names
  RigLogic reads). A misspelling inside that family is refused before anything is written, with
  the nearest real names, e.g. `CTRL_expressions_jawOpn` offers `CTRL_expressions_jawOpen`. Names
  outside the raw controls (neural masks, custom curves), or a part with no DNA, build with one
  note saying their spelling was not checked. The check needs the RigLogic plugin enabled.
- One owner for each facial curve, enforced: two controls writing one curve, or a `drive()` over a
  curve the clip's source already writes (speech, or a clip it is made `from=`), are refused before
  anything is written. Layering an expression over speech is not built; use a separate clip or
  another curve, and inspect any native blending policy you add.
- Curve units are native values. Joint acceleration/velocity percentages describe bones, not a
  facial float curve; time these controls with profiles rather than inventing joint effort data.
- A groom artifact needs its own inspection: compare the same expression with facial hair hidden,
  then inspect the groom's binding, materials and simulated playback. A white speck alone does
  not establish that the expression curves or the groom binding are at fault.
