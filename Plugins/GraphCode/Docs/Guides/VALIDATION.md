# Prove the game is ready for its milestone

Read this before declaring a slice complete, increasing content density or handing over a build.
The [production guide](PRODUCTION.md) defines the target; this guide explains the evidence.
Use the installed tool's help for commands. Packaging, profiling and platform checks may require
Unreal's native tools or a human run; GraphCode availability is not proof those checks happened.

## Match the check to the claim

| Claim | Required observation | What it does not establish |
|---|---|---|
| The edit landed | Canonical readback, relevant compile result and saved asset | That the game uses this asset or behaves correctly. |
| It looks right | Viewed image at the intended camera, lighting and quality setting | Input, events, dynamic materials or animation that the preview never executed. |
| The mechanic works | Runtime actions, resulting state and visible feedback, including failure/retry | Durability after reload or behaviour in a package. |
| The authored work is durable | Saved assets reopened in a fresh load and exercised again | The game's own save/load system. |
| Progress persists | Save through the game, quit, relaunch and load | Migration from older saves unless tested. |
| It meets its budget | Measured target configuration, scenario and interval | Other hardware, resolutions, quality settings or content densities. |
| The release works | The exact packaged candidate launched and played in the intended environment | Platforms or distribution requirements not exercised. |

Keep the evidence location, date, source revision, engine/plugin versions, map, build configuration
and device with the result. Record pass, fail, not run, or not applicable with a reason. A tool
return code, an empty log excerpt or an unviewed screenshot is insufficient evidence for a player
outcome. A refused or interrupted test is not a pass.

## A repeatable scenario

For visual assets, follow [the seam and detail render loop](ASSET-QUALITY.md#repeated-render-validation-of-every-seam-and-detail).
Account for each seam, connection and authored detail at a view where it can be judged, repair
findings and re-render affected areas. Check the whole asset in its thematic context as well as
isolated close-ups. Evidence must show the current asset; later edits reopen affected checks.
A beauty shot alone does not establish this coverage. Explicitly simple assets still need sound
construction, finish and validation.

For assemblies, run [mesh connection checks](ASSEMBLY-VALIDATION.md) at the placed Blueprint
and level scope, with expected contacts and explicit coverage. For streets, verify the
[complete road-to-building route](ROADS-AND-BUILDINGS.md), including actual collision and
navigation; separate clean mesh builds do not establish either result.

The [same quality bar applies to every medium](ASSET-QUALITY.md#apply-the-quality-bar-to-every-medium).
Use continuous playback for animation and VFX, actual state transitions and input for UI, listening
for audio, and the complete combined experience for cinematics or gameplay feedback. Cover every
relevant authored phase and boundary, inspect interactions between layers, and repeat affected
checks after repairs. Static poses, screenshots or a waveform cannot prove temporal or audio quality.

For cinematics, record the [director's preproduction readiness](CINEMATICS-PREPRODUCTION.md#check-readiness-before-production)
before dependent production work. Use the [filmmaking review gates](CINEMATICS.md#review-gates-before-calling-it-finished)
and [GraphCode delivery/integration checks](CINEMATICS-GRAPHCODE.md). Review the complete edit and
mix, every cut and shot start/tail, then the actual export or in-game entry, skip, interruption
and recovery. Asset readback, a preview span and full-sequence output prove different things.

Describe the initial state and its safe reset; the actions a player takes; expected state and
visible feedback; failure or interruption; and the evidence to capture. Use test profiles and
disposable saves rather than the owner's progress. Bound waits and check the resulting condition
so a stalled scenario fails clearly.

For the resource slice: start with a known inventory, collect once, verify quantity and feedback,
attempt to collect the consumed resource again, fill the inventory and attempt another pickup,
then save, quit and reload. Check that neither failure nor reload duplicated the reward. Adapt
this pattern to the feature, rather than testing only the graph nodes that implement it.

Use `gc_play` when installed and when its selected mode supports the scenario. Otherwise run the
game through an available Unreal workflow and record the actions and observations. Mark checks
that cannot run as not run and name the missing dependency. Do not turn absent automation into
a claim that behaviour was verified.

How a `gc_play` run begins and ends (ask `gc_play` with `{ "help": true }` for the modes):

- **The driver lease is one per editor.** `attach` takes it; a second `attach` is refused while another
  driver holds it. `stop` releases it at once, `end` releases it once the game has taken the command, and a
  game that ends by any route releases it too, so `attach` again for the next run rather than reusing the
  old one. `detach` gives it back without ending a game.
- **`status` reports memory.** Its `memory` block has `editor_private_mb` (the editor's committed memory,
  which is what ran out in long sessions) and a `gpu` block: `dedicated_mb`, `budget_mb`, `rhi_tracked_mb`
  (textures and buffers only, so a floor and not the whole use), `over_budget` and `over_budget_mb`. Read it
  before a long capture and again when a run slows down.
- **A cancelled task may already have finished.** A task cancelled after its tool had finished keeps its real
  result and status, with a warning that the cancel came too late; only a tool that stopped early reads
  Cancelled. Check what the tool actually did before you repeat it.

For UI, use the state and display matrix in [UI-UX.md](UI-UX.md). Keep external prototype evidence
separate from in-game evidence: a Figma or browser review cannot close Unreal input, data or
packaging checks. For comprehension and feel, use the observed playtest method in
[DESIGNING.md](DESIGNING.md); technical correctness does not establish enjoyment.

## Scripted input in a running Play

There is no input verb, and none is needed. Unreal's Enhanced Input functions `InjectInputForAction` and
`InjectInputVectorForAction` are Blueprint-callable, so the game can be given a one-node helper that calls
one of them, and `gc_play` calls the helper like any other game function. Owner ruling 2026-08-16: if the
thing you want is reachable from the game, the answer is authoring, and no verb is added.

1. **Write the helper.** Add a function to any Blueprint in the game whose one node calls the injection
   function on the player's Enhanced Input subsystem, with the action and the value as its parameters. Find
   the callable with `gc_search`, write the Blueprint with `gc_apply` and read it back
   (`gc_describe kind:"blueprint" block:"graph"` for the grammar).
2. **Call it in the running game.** `gc_play mode:"execute" act:"invoke"` with `target:"Actor.Function"` and
   named `args:` in the same value grammar a `.gc` uses. Ask `gc_play {"help":"args"}` for the exact form;
   whatever the function returns is written to the log as `observed=`.
3. **Hold it for a number of ticks.** Pass `steps:[...]` in place of `target:`. The steps run inside the
   game, one per tick, in order. A step with no `act` and an `after` is the wait, and `after` takes
   `{frames:N}`, `{seconds:S}` or `{mark:"name"}`. Frames are counted in game ticks, so a hitch in the editor
   does not shorten the hold. Write the call that ends the input as the next step. Ask
   `gc_play {"help":"steps"}` for the step fields.

What this covers and what it does not. Injection starts after the key has been mapped to an action, so it
puts modifiers, triggers (hold, tap, chord) and value shaping under test. It does not test which key is
mapped to which action, which Mapping Context has priority, a menu context that still shadows movement, or an
input mode left on UI only. Those need a person or a key-level node that does not exist yet. Calling the game
function directly is the other first-class route: use injection when the claim is that the button works, and
the direct call when the test only needs the situation, such as the door already open.

The step act `inject` is not input. It puts a captured state back with no entry event.

## Play from editor Python: what does not work and what to use instead

Field sessions that drove a Play from editor Python through `execute_script` lost days to silent failures.
What they found:

- **The script context has no world bound.** The current-world accessors returned `None`, so every trace
  looked like a miss and a probe over 305 spots reported no floor at all. The bridge runs the script as a
  plain `py` file with no world binding and no switch to the Play world.
- **A hit result is read with `to_tuple()`.**
- **A rotator's positional order is roll, pitch, yaw.** Pass keywords.
- **An editor spawn does not reach a Play world.** A harness that spawned a camera from Python saw no error,
  and the view stayed the hero's.
- **A helper that catches every exception turns each of these into a silent miss.** Make a probe fail when
  its data is missing.
- **Log the frame rate beside every measurement.** A slide measured at 11 fps read as a curve drag and was
  the body's size.

Use the supported route, which runs inside the game and refuses what it cannot do:

- `gc_play mode:"query"` with `what:"actor"` (where `target:` is, its rotation, velocity and tags),
  `what:"overlaps"` (what a sphere touches) or `what:"nav_path"`. It reads only and needs no lease, and a
  missing actor or property is an error that names it.
- `gc_play mode:"spawn"` with `class:`, `at:` and `rotate:` puts an actor into a game GraphCode started, with
  the lease; it goes when the game ends.
- `gc_play mode:"set"` with `target:`, `property:` and `value:` writes one instance-editable property of an
  actor in a game GraphCode started, with the lease; the reply carries `old_value` and `new_value`, and the
  change lives in the running game only.
- A bridge started before the build of 14 Nov 2026 answers "not a mode of gc_play" for these three: there the
  same calls go to `gcp_run` (`query` and `spawn`; `set` needs the new build), or restart the client.
- `invoke` for the rest, a teleport or an aim being a function the game already has
  (`gc_play mode:"execute" act:"invoke"`).
- `gc_play mode:"capture"` with `world:"play"` writes the actors of the Play world with class and location.

There is no fixed time step for a Play yet, so a result taken at a low frame rate says so in the log and
should not be trusted for a timing claim.

## Judge a clip on its trails and its graph

Goal: decide whether a built `.gcmove` is good, and find its jitter, from two pictures instead of
from frames. [GCMOVE.md](GCMOVE.md) step 6 says to render it that way; this section is how to read
what comes back. The numbers below were measured on 2026-10-23 on the clips named with them.

**One render shows the whole clip.** `gc_render` the BUILT clip with
`highlights:[{type:"trails", of:[...]}]`: every named bone's whole path is in that one picture, its
spacing the speed. Add `graph:true` to the SAME entry and `views[].graph` is a second PNG beside the
frames — each bone's velocity on the left, its acceleration on the right, and a RED mark at every
velocity step. `graph:true` gives the bone's place and its joint; `"place"` (cm/s), `"turn"`
(deg/s), `"joint"` (its three angles in the rig's words) or `"centre"` (the centre of mass) gives
one of them, and a rig chain in `of:` is each of its bones.

**The legend is where the numbers are.** Per trace it reads `<trace> fastest N <unit> (frame F),
sharpest N <unit> (frame F), K velocity step(s) at frame ...`, and for the view as a whole
`K velocity step(s) in all (red), most of them N samples apart (every T s)`. Read it before looking
at the shapes: it names the frames to go to.

**Judge on the trails and the graph, never on frames** (rule and sources: [GCMOVE.md](GCMOVE.md) step 6). A contact
sheet hides the arc, the spacing and a flip between two frames.

### What a clean trace looks like

`Content/__MCPTests/BubbleKeyAnims_20261023/A_GuardShove`:

| Trace | Fastest | Velocity steps |
|---|---|---|
| `hand_l place` | 397 cm/s | **0** |
| `blade_r place` | 264 cm/s | **0** |

Both are hand-authored by a `profile()` curve, and both are clean. **A clean velocity trace rises
from zero, has ONE peak per beat, and returns to zero**; its acceleration is a smooth hump, not
grass. A 397 cm/s hand with no steps is the proof that fast is not the same as jittery — speed on
its own never condemns a clip.

### What jitter looks like, in the same picture

The same `A_GuardShove`, read on its `joint` rows:

| Trace | Measured on the graph | The build's kinematics note |
|---|---|---|
| `thigh_l joint` twist | fastest **1875 deg/s**, sharpest **196928 deg/s²**, velocity steps at frames 16, 96, 100 and 132 | **788%** of its 25,000 deg/s² reference and **216%** of its 850 deg/s speed reference |

**27 velocity steps in all, and the legend said most of them 4 samples apart (every 0.033 s)** on a
clip baked at 120 fps. That spacing is the finding, not the count: 0.033 s is one authored 30 fps
frame, so the steps land at exactly the authored frame period. **Steps spaced at the authored frame
period mean the solve is being redone once per authored frame and its answers do not agree between
frames — not that the author's curve is wrong.** Re-tuning the curve will not remove them. Read the
gap before deciding: steps with no regular spacing are a different problem.

The same clip's `already moving at the clip's start: thigh_l 626 deg/s` says it does not begin at
rest — a jerk on the very first frame.

### Plants and landings: read the turn, not the place

`Content/__MCPTests/BubbleKeyAnims_20261023/A_Switch`:

| Trace | What it did |
|---|---|
| `foot_l joint side` | flat at zero, then a single spike to **+608 deg/s** and straight back to **-608** (±**18049 deg/s²**) — twice in the clip |
| `foot_r joint side` | the same shape at **309 deg/s** |
| `hand_l place` | a clean smooth curve, 0 steps |

**The shape to recognise: a planted foot's axis sits flat, then snaps one way and immediately back
within a frame or two.** That there-and-back is the signature. Real movement goes somewhere and
stays there; a solve disagreeing with itself goes and comes straight back.

**Read the `*_joint` rows of the planted foot, not its `*_place` rows. This is the trap.** On
A_Switch the foot's POINT travelled 1.3–1.5 cm over the whole clip — by its place trace the foot is
planted and fine — while its TURN snapped 608 deg/s. A plant holds where the foot IS; nothing in a
place trace shows what the ankle did underneath it. A clean hand in the same clip says nothing about
the feet either: judge each planted part on its own rows.

Read the build's plant notes at the same time, every time:

- `plant '<name>' on <plane>: sole certified at frame N (...)` — the sole was actually on the floor
  at that frame, with its uncovered area, margin and clearances.
- `line N: '<name>' could not be put where the file asked — N cm short at frame F` — the plant did
  not hold. Go to frame F on the graph before reading anything there as a curve problem.

Balance is the other thing to read at a landing: `balance` corrects each frame on its own, so a
one-frame correction of a fraction of a degree can read as a large acceleration, and the working
diagnosis for a rough landing in [NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md) is `balance` changing
its solution as the contacts change (not isolated there). The verb itself is
[MOVE-BALANCE.md](MOVE-BALANCE.md).

### Loops: the seam must agree in speed, not only in pose

A loop's first and last frame must agree in POSE **and in VELOCITY**. Matching poses still pop every
cycle if the body is moving at one speed when the clip ends and a different speed when it starts.

The note that shows it is the boundary note:

```text
already moving at the clip's start: <bone> N deg/s; at its end: <bone> N deg/s
(one-sided over the first/last step; the entry and exit states are unknown)
```

Read both halves and compare them. The caveat is part of the note: it is measured over one step at
each end, so it tells you this clip's own first and last step and nothing about what plays before or
after it.

What to check at a seam:

- both halves of that note name the same bone at about the same speed;
- each trace's first and last sample sit at the same HEIGHT on the velocity graph, not just in the
  same pose;
- if the seam is meant to be a hold, both ends sit at zero.

**Not measured.** No looping clip was measured on 2026-10-23, so there are no loop numbers here: the
three checks above are what to read, and the boundary note is where to read it. The rest of loop
acceptance (weight zero, independent release, repeated activation, loop seams) stays in
[ANIMATION.md](ANIMATION.md#acceptance-before-delivery).

### Read these notes every time

The build prints them, they are part of the result, and they are quoted rather than summarised away
([GCMOVE.md](GCMOVE.md), "Before you call it done").

| Note | What to take from it |
|---|---|
| peak speed: `..., component space, actor trajectory unknown ...; no declared limit. Measured on native compressed playback every N key(s) (T s).` | the clip's fastest motion, where it is, and the sampling it was measured at — compare like with like, because a finer sampling reads a sharp turn higher |
| peak acceleration, and the sharpest turn `relative to its parent` | the sharpest change, and which joint made it |
| `past its reference: <bone> ... N% of its <reference> (a reference is this character's 100%, not a limit)` | the joint furthest past this character's own reference. 788% is a finding; a little over 100% is not |
| `a candidate velocity step at frame N (T s): ... over one step and ... over four (xN). A narrow smooth peak can read the same; the step is not proven.` | a candidate, so go to those frames. A narrow smooth peak reads the same, and `balance` corrects frame by frame ([NATURAL-MOVEMENT.md](NATURAL-MOVEMENT.md)) |
| `already moving at the clip's start: ...; at its end: ...` | the clip does not begin or end at rest: the first-frame jerk, and the loop seam |
| `native compression adds no spike over the raw keys that shows (...)` | the jitter you are reading is in the authored keys, not the codec. Without that note, read `compression amplifies <bone> ... at frame N ... (xN)` and check the clip's compression settings before trusting playback speed |
| `plant ...: sole certified at frame N (...)` and `could not be put where the file asked — N cm short at frame F` | whether each plant actually held |
| the collision notes: the refusal's first failing pair, `collision inventory $Clip` under it, and anything reported as unchecked or incomplete coverage | what was checked and what was not; incomplete coverage is not a collision pass ([ANIMATION.md](ANIMATION.md#body-collision-coverage)) |
| `WARNING joint limit: ... clamped` | a joint stopped by its limit. That is a pose problem, not a style ([ANIMATION.md](ANIMATION.md)) |
| `kinematics $Clip: peak jerk ...` | the sharpest change of acceleration and where, beside the peak speed and acceleration lines; a spike here is the snap a viewer sees |
| `kinematics $Clip: slide while planted: '<bone>' N cm (plant line L, frames A-B)`, with `WARNING` above 0.5 cm | a plant at full weight that moves is not holding; the plant's own notes say whether its chain could reach |
| `kinematics $Clip: plant(chain), line L, lets go at frame N ... moving N cm/s`, with `WARNING` above 300 cm/s | a limb still moving that fast when its plant gives it up snaps: ease the plant's `weight=$Profile` out over more frames, or slow the limb first |
| `WARNING barely moves: $Clip moves no bone more than 0.5 cm or 1 degree(s) ...` | the clip plays as a still pose; its amounts are too small (a swing is degrees, a shift is centimetres, a profile's last value is the amount). Not said for curve, face, physics or speech clips |
| `WARNING held prop: $Clip holds $Prop in '<hand>' ... none of that hand's N finger joints leaves its reference pose` | the prop sits in an open hand: close the fingers with a `grab()` or a hand pose, for example `pose(source=<built hands clip>, bones=<fingers>)` ([HANDS.md](HANDS.md)) |

## Measure performance before multiplying content

Set budgets for frame time, memory and loading with the target device, resolution, quality level
and scenario. For example, 60 frames per second allows about 16.67 milliseconds per frame; that is
an illustrative frame budget, not a measured result or a universal target.

Use a packaged Development build for investigation and verify the intended Shipping candidate
as well. Record a representative traversal plus expensive cases such as combat, crowds, streaming
boundaries or dense effects. Measure an interval and report slow frames and hitches as well as an
average. Separate a first encounter from a warmed repeat; a warm cache can hide loading problems.

Locate the limiting work before changing fidelity: game thread, render thread/GPU, allocation,
streaming or another observed cause. Change one plausible cause, repeat the same scenario, and
compare both performance and appearance. Do not lower global quality settings to hide one costly
asset without checking the agreed visual bar. Epic's
[profiling guide](https://dev.epicgames.com/documentation/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)
routes to Unreal Insights and the engine's profiling tools; select the project's engine version.

## Package early, then test the actual candidate

Create the first package while the representative slice is still small. Record the engine and
SDK prerequisites, target, configuration, startup map, cook/package settings and reproducible
build command or Launcher profile. Confirm the runtime dependencies and dynamically loaded
content are included; editor success can conceal missing cooked assets or editor-only dependencies.
Use the version-matched Epic [packaging guide](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project)
and [build operations reference](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine)
for the native process instead of guessing platform flags.

Run the exact artifact that will be handed over, from a fresh test location and profile, without
depending on the editor or loose development content. Preserve real user saves. Capture build and
runtime diagnostics available in that configuration. Test both first launch and a subsequent
launch; if the release supports an upgrade, test it against a copy of an older supported profile.

| Release gate | Exercise in the candidate |
|---|---|
| Start and finish | Correct startup map, new game, the critical journey, success/failure, retry, return to menu and exit through game UI. |
| Input and UI | Every supported device; focus, pause/resume, settings, readability at supported display sizes and agreed accessibility options. |
| Content | Representative art, animation, effects and audio; map travel, dynamic loads and no missing assets along the route. |
| Persistence | Save/load, settings persistence, clean profile and supported upgrade/recovery paths. |
| Performance and stability | Target configuration, representative and expensive scenarios, repeated cycles and a soak duration chosen for the game's risks. |
| Networking, if used | Intended server/client configuration, competing actions, supported join/recovery and degraded network conditions. |
| Distribution | Required runtime components, asset provenance/credits and the chosen platform's current submission requirements. |

Keep platform and storefront requirements in the project's release note with links to their
current official guidance. These are project-specific checks, not a promise that GraphCode
certifies a game. A missing target device leaves that target unverified.

## Handoff and regression

Give the owner a build identity and location, launch instructions, tested configurations, evidence
links and known issues. Distinguish a technical pass from a pending art or feel review. A failed
required gate keeps the release open; an accepted scope change belongs in the owning design note.

After a change, rerun the affected scenario and the shared player paths it can break. Repeat the
broader release gates when cutting a new candidate or when dependencies, saves, engine version or
packaging settings change. Tie results to that candidate; do not carry an old green status across
an untested build. Keep remaining actions on the board and completed evidence in the project note.
