# Build and verify cinematics with GraphCode

Start with [CINEMATICS.md](CINEMATICS.md) for the workflow. The focused
[direction](CINEMATICS-DIRECTION.md), [shots](CINEMATICS-SHOTS.md),
[movement](CINEMATICS-CAMERA-MOVEMENT.md) and [editing/transition](CINEMATICS-EDITING-AND-TRANSITIONS.md)
chapters develop the artistic choices. This companion connects them to Unreal assets,
GraphCode authoring and the final output.
A camera asset and some keys are one dependency of a cinematic, not its completion criterion.

The capability details below were checked against the UE5.7 GraphCode source/help on 7 October
2026. They describe that inspection, not a playback certification. The installed tools and the
actual asset's canonical read decide current syntax and coverage; recheck after updates.

## Start from documented directing decisions

Before production authoring, save and review the
[director's packet](CINEMATICS-PREPRODUCTION.md#check-readiness-before-production). Carry the
selected [intent](CINEMATICS-INTENT.md), [boards/animatic](CINEMATICS-STORYBOARDS.md) and
[attention plan](CINEMATICS-ATTENTION.md) into the asset plan. Each shot needs a purpose,
viewpoint, entry/turn/exit, move/focus cues, cut relationship and sound dependencies before keys
become production work. Keep stable beat/shot IDs in the production records and asset mapping.

Record [technique-selection rationale](CINEMATICS-TECHNIQUE-SELECTION.md) and its alternative
beside the asset plan. Use the [frame-first review](CINEMATICS-STORYBOARDS.md#review-key-frames-before-final-recording)
for composition and timing decisions before final sequence recording.

Technical discovery and small representative tests can resolve the packet's unknowns. Record
their question, result and limits; distinguish a framing preview, canonical source inspection,
successful apply and actual playback. Do not use a camera recipe below as a substitute for
directing the sequence. Once the documented baseline and dependencies are ready, build from it
without an extra permission checkpoint when implementation is already authorized.

## Discover the actual authoring path

Use [TOOLS.md](TOOLS.md) for the normal edit loop. MCP clients may expose an `unreal_` prefix.
Ask each verb for help, then request only the grammar needed for this stage:

```text
unreal_gc_describe(help:true)
unreal_gc_describe(kind:"level_sequence", block:"sequence")
unreal_gc_describe(kind:"level_sequence", include_example:"full")
unreal_gc_apply(help:true)
unreal_gc_apply(help:"omission")
unreal_gc_read(help:true)
unreal_gc_render(help:true)
```

These are discovery calls, not an asset-building script. Check the connected project/products
through `unreal_info` before live authoring. Use a current canonical `gc_read` of the intended
asset to learn its actual declarations, track classes, sections, channel names and warnings.
Ask `gc_describe type_path:` when a native property's spelling, units or enum is uncertain.
Never invent a track or section payload from its name.

**Separate four questions:** can Unreal do it; is the necessary plugin installed; can GraphCode
author the needed fields; did this particular sequence actually play correctly? An affirmative
answer to one does not establish the next. Keep unsupported facets in `omits:` and heed warnings.
An unchanged sidecar cannot prove an omitted binding target, cache or director implementation.

## Give each part an owner

Carry the artistic treatment into the asset plan before writing keys. GraphCode exposes authoring
controls; the author decides what they mean in this scene. Keep the reason beside the shot record
so a later adjustment preserves the artistic intent.

| Artistic choice | Authoring decision | Review the intended effect |
|---|---|---|
| A relationship changes from separation to connection | Stage the performers, then choose the camera position/framing and performance that bring them into a shared image. | Play the change; can viewers perceive the new relationship at the intended moment? |
| Empty space anticipates an arrival | Author blocking and camera composition together; preserve the reveal through the move and cut. | Does the audience attend to the right space without seeing the arrival too soon? |
| A repeated hand gesture changes meaning | Develop the source performance, prop/contact and coverage; retain the motif across shot revisions. | Compare both occurrences in the edit, including their different context and sound. |
| Color/light changes with the scene's emotional turn | Coordinate material/light assets and supported sequence properties; use native handoffs for omitted controls. | Compare key moments and adjacent shots through the actual display/output transform. |
| Rhythm changes from urgency to hesitation | Revise performance timing, camera response, shot boundaries and sound cues as one passage. | Watch and listen at speed; does the decision read before the consequence? |
| A sound becomes subjectively important | Use the proven native sound/mix composition and current supported audio authoring fields. | Listen to the transition in the full sequence; a source preview cannot establish its dramatic perspective. |

| Production decision | Owning artifact or Unreal system | GraphCode route and verification |
|---|---|---|
| Purpose, beats, continuity, review notes | Project's cinematic/production note and edited boards | Keep decisions in the project; link to these guides instead of copying them. |
| Set, placed performers, props and practical lights | Level and actor/component assets | Core `level`/`blueprint` authoring; World where needed for spatial construction. Verify the actual scene and actor identities. |
| Body, face, contacts and root travel | Rig, source performance and native animation composition | Rig/Move/Human or imported/retargeted sources as appropriate; follow [ANIMATION.md](ANIMATION.md) and [CHARACTERS.md](CHARACTERS.md). |
| Camera, lens, cut and shot timing | Move `level_sequence`, `sequence { }` | Supported binding templates, tracks, sections and channels; verify the camera that actually wins during playback. |
| Editorial shot order | Master Level Sequence containing child-shot sections | Supported shot `plays=` and trim/rate fields; review from the master in the destination level. |
| Sound source and behavior | Sound Wave, Sound Cue or MetaSound; native sequence/game mix | Import/author supported sound kinds; separately verify the sequence's audio payload and the heard output. |
| Hair/cloth/FX history | Native simulation, supported animation/cache track or prepared bake | Check each track's payload coverage and dependencies; a generic section is not evidence that a cache was assigned. |
| Start, skip, consequence and control recovery | Project Blueprint/gameplay owner and Level Sequence player | Author supported Blueprint behavior through Core; exercise it in play. |
| Render job, color transform, output and finishing | Native Movie Render Queue/Graph, editor/compositor and sound workflow | Use a verified native path. Do not invent a GraphCode MRQ kind or sequence render mode. |

Choose one owner for each animated property at a given moment. Resolve overlaps among AnimBP,
Control Rig, clip, actor transform and gameplay logic deliberately. For a multi-component
character, one native composition must coordinate body, face, props and audio. Independent
start timers do not establish synchronization. A baked clip does not choose its own runtime actor.

## Organize the sequence for revision

For a substantial cinematic, use a master for editorial order and child sequences for shot work.
Epic's [UE5.7 Camera Cut Track guidance](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-camera-cut-track-in-unreal-engine?application_version=5.7)
recommends a camera and cut within each shot for most full cinematics. A very short continuous
scene can stay in one sequence; do not add hierarchy without a revision or ownership need.

An illustrative project structure is:

```text
/Game/Cinematics/Help/LS_Help_Master
/Game/Cinematics/Help/Shots/LS_Help_010_Approach
/Game/Cinematics/Help/Shots/LS_Help_020_Interruption
/Game/Cinematics/Help/Shots/LS_Help_030_Choice
/Game/Cinematics/Help/Shots/LS_Help_040_Consequence
```

Shot IDs remain stable when reordered or trimmed. The master uses a cinematic shot track whose
sections name children with `plays=`; current grammar also describes shot names, clipping,
looping, bias and constant rate where supported. Copy the exact form from help/readback. Each
child's camera cut should reference its own declared camera. This supported arrangement differs
from a camera-cut binding ID reaching into another sequence, which remains an omitted facet.

Decide where continuous performance, world changes and sound live. A character can be a persistent
level actor while cameras spawn per shot. A performance subsequence can maintain one passage
across editorial cuts when the native setup supports it. Document ownership and test the actual
hierarchy: duplicated actor instances, repeated cues or competing property tracks can otherwise
appear only when the master plays. Review a child both in isolation and in master context.

## Understand binding before animating

Current sequence grammar declares bindings before the tracks that cite them:

- **Spawnable:** the sequence owns a template and creates an instance for its lifetime. Its
  `@spawn` declaration can carry placement and component deviations. This fits a shot camera or
  other shot-owned object. Check its spawn/despawn behavior at trims, pre-roll and completion.
- **Possessable:** the sequence addresses an existing object. Its name/class/parent declaration
  is carried, but a newly minted binding does **not** automatically find a level actor with a
  similar name. The inspected implementation declares `possessable_targets` omitted. Assign
  the target through native Sequencer, then verify it in the actual destination map.
- **Component under a spawnable:** a same-named component binding parented to the spawned actor
  can resolve to that template's component. The actor's transform track moves the camera; the
  camera component's property tracks animate the lens/focus.

Use exact actor/component names and full asset paths. With multiple cameras whose components
share a name, retain the parent identity from the projection. Changing spawn/possess identity or
class is not an ordinary property edit; use the current declared replacement procedure only
when explicitly intended. Binding deletion takes its tracks with it and must be deliberate.

**Minimum proof:** move or change a clearly identifiable property, play the sequence and observe
that the intended runtime instance changes. A declared binding, colored track or nonempty channel
does not prove an object was resolved. Reopen the saved map/sequence and repeat the relevant check.

## A small camera recipe

This source-checked teaching recipe shows a spawned camera, its component binding, a lens track
and a camera cut. It was not applied or rendered for this guide. Its four-second range, placement
and lens values are illustrative; solve them from the scene rather than reuse them as a style.
The camera looks along its local forward axis; put the intended subject in its view and set
focus/filmback from measured staging and current reflected properties.

```gc
gc 1  kind:level_sequence  asset:/Game/Cinematics/LS_CameraStudy
parent: /Script/LevelSequence.LevelSequence
baseline: default
omits: custom_bindings possessable_targets cross_sequence_cuts section_timewarp director_graph

sequence {
  sequence LS_CameraStudy [range=0.0s..4.0s, tick=24000, rate=24]
  {
    binding ShotCam = /Script/CinematicCamera.CineCameraActor()
      @spawn
      @placement[Location=(X=0.0,Y=0.0,Z=160.0), Rotation=(Pitch=0.0,Yaw=0.0,Roll=0.0), Scale=(X=1.0,Y=1.0,Z=1.0)]
    {
      CameraComponent = CineCameraComponent(CurrentAperture=4.0, CurrentFocalLength=50.0)
    }
    binding CameraComponent = /Script/CinematicCamera.CineCameraComponent()
      @binding[Parent=ShotCam]
    track CameraComponent.CurrentFocalLength [class=/Script/MovieSceneTracks.MovieSceneFloatTrack]
    {
      section [range=0.0s..4.0s, blend=Absolute]
      {
        channel : float
        {
          0.0s = 50.0 [linear]
          4.0s = 65.0 [linear]
        }
      }
    }
    track `Camera Cuts` [class=/Script/MovieSceneTracks.MovieSceneCameraCutTrack]
    {
      section [range=0.0s..4.0s, camera=ShotCam]
      {
      }
    }
  }
}
```

Create a supported new Level Sequence with `gc_apply create:true` from a whole document, using
the installed help's input route. A creation header does not invent existing `sig:`/`bin:` values.
For subsequent edits, obtain fresh signed text and retain its actual coverage. The lens change
above is a zoom, not camera travel; keep it only when it serves the intended beat. To author
travel, read/describe a real transform track and preserve its native channel layout rather than
guessing its channel names or rotation convention.

Finish the exercise by checking the cut selects ShotCam, the component lens changes, the framing
remains useful and the result survives save/reopen. That proves this camera setup, not a complete
performance, sound mix, master sequence or game integration.

## Author camera moves and editorial transitions

Choose the technique through the craft chapters first. GraphCode writes the supporting native
tracks/channels; it has no `pan=`, `dolly=` or cinematic transition generator. Keep the movement's
cue, start, passage and landing in [the shot record](CINEMATICS.md#keep-a-useful-shot-record).

| Craft choice | Authoring path | Required observation |
|---|---|---|
| Pan/tilt | Animate the camera actor's supported transform rotation channels; keep position/lens fixed when a pure turn is intended. | Subject/cue leads or follows as designed, useful passage/landing, no unexpected travel or angular wrap. |
| Dolly/truck/pedestal/orbit | Animate position and the required orientation together, or use the production's proven native rig. | Correct parallax/route and attention, no collision/occlusion error, consistent focus and useful arrival. |
| Zoom | Animate the bound Cine Camera component's focal-length property; solve filmback/focus with it. | Optical reframing reads as intended; it is not being mistaken for travel. |
| Rack focus | Use the component's actual reflected focus-mode/settings and supported property channels. | Focus moves between the intended depth planes at the dramatic cue; another focus controller does not override it. |
| Straight/action/reaction/graphic cut | Set the camera-cut or master-shot boundary, with compatible performance/source-time mapping. | Correct active camera and the intended time, action phase, eyeline and attention across the cut. |
| J/L cut | Keep picture and sound boundaries independent in the verified native sequence/mix. | Incoming audio leads, or outgoing audio continues, by the intended interval; visible speech stays synchronized. |
| Camera interpolation | Use native Camera Cut Track Can Blend and section overlap/easing where required. | Intermediate viewpoint, focal/focus response and gameplay ownership behave correctly. |
| Cross dissolve / graphic wipe / masked seam | Use the project's image-compositing workflow; supply both images and usable handles. | The actual composite reads correctly, with no missing source, freeze, exposure jump or visible unintended seam. |
| Fade to/from a solid color | Use native Fade Track or the project's proven presentation system. | Intended color/range, sound policy and exit/skip state; no persistent accidental black screen. |

### Author and refine a pan

1. Set the camera's initial transform and lens from measured staging. Bind the actual actor and
   component correctly; the [small recipe](#a-small-camera-recipe) shows that ownership setup.
2. Obtain the installed native transform track's canonical shape through `gc_read`/current help.
   If the asset has no transform section to inspect, establish a native section as a scaffold,
   read it, and preserve its actual channel count, order, names, types, defaults and empty curves.
   A section may carry a weight curve as well as translation/rotation/scale. A hand-written
   single yaw curve is not a replacement for the complete native section.
3. Identify the rotation channel that turns the camera in its actual parent/reference frame.
   Confirm that with a bounded change and playback; do not infer it from a channel index or a
   display label copied from another asset. Account for attachments/rig transforms.
4. Put keys at the intended start, move cue, arrival and end/working handles. Keep other
   position/rotation/scale defaults and keys deliberate. For a follow pan, match performance and
   screen-space composition; for a reveal, control when the destination becomes readable.
5. Choose supported interpolation/tangents from the canonical form. Compare the curve with the
   intended operator behavior: holds, acceleration, following, settling or a carried exit. Check
   overshoot, discontinuities and turns across angle boundaries through the actual player.
6. Dry-run/apply under the [safe authoring loop](#apply-safely-and-prove-each-connection), read
   back and play through the active cut. Recheck the full scene and exported movement after
   timing, lens, focus or performance changes. Save/reopen for the durable claim.

The [pan rehearsal](CINEMATICS-CAMERA-MOVEMENT.md#rehearse-and-finish-a-pan) determines the artistic
timing. The source supports native numeric curves; this task has not executed a new pan recipe.

### State an intentional cut

This source-checked fragment belongs **inside** a complete `sequence` entry with already-declared
`CameraA` and `CameraB` bindings. It is not a standalone apply payload and was not run. The
two-second boundary is illustrative; choose it from performance and editorial review.

```gc
track `Camera Cuts` [class=/Script/MovieSceneTracks.MovieSceneCameraCutTrack]
{
  section [range=0.0s..2.0s, camera=CameraA]
  {
  }
  section [range=2.0s..4.0s, camera=CameraB]
  {
  }
}
```

Use this arrangement for a small two-view study. For substantial work, keep each shot's own
camera/cut in its child sequence and edit the master's shot sections, as explained above. Moving
the cut and slipping a child's source are different edits: one changes its output boundary,
the other changes the source action shown. Compare contact/action and sound at the new boundary.

### Preserve the native transition controls

Epic's [UE5.7 camera-cut documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-camera-cut-track-in-unreal-engine?application_version=5.7)
uses **Can Blend** and section overlap/easing to interpolate views. The inspected GraphCode
sequence representation does not carry that track flag or manual section easing controls.
Use native Sequencer for them and preserve a recoverable source asset; do not rebuild a blended
track from a shortened projection and assume those settings were reconstructed. Check them after
nearby edits and save/reopen. Section `blend=Absolute` is a native MovieScene blend-type setting,
not a cross-dissolve or a duration, and invented `dissolve=`/`ease_in=` tokens are not a solution.

A real image dissolve needs the production's compositing route. Reserve/render valid handles
and the required prior simulation history for both sources. A native
[Fade Track](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-color-fade-track-in-unreal-engine?application_version=5.7)
has additional color, sound and completion settings: do not assume projecting its scalar curve
also authors all of those controls. Inspect the actual coverage; use native controls as needed.
Exercise completion, early/late skip and return to gameplay with both picture and sound.

For a hidden/whip cut, author each camera passage, then use the actual rendered/native output
to align and inspect the seam. A viewport still, blur key or pair of good endpoints does not
prove a continuous-looking transition. Follow
[the editorial construction guide](CINEMATICS-EDITING-AND-TRANSITIONS.md#design-a-hidden-cut-or-whip-transition).

## Keep time and editing semantics explicit

Carry the [frame ledger](CINEMATICS-STORYBOARDS.md#keep-an-explicit-frame-ledger) into authoring:
display rate, numbering/end convention, master/source mapping and independent action/picture/sound
cues. Its frame examples are planning records; apply supported time syntax from current help.

- **Display rate** defines the editorial frame grid. **Tick resolution** defines sequence time
  precision. In the example, 24,000 ticks/second and 24 frames/second give 1,000 ticks/frame by
  division; this is an example configuration, not a universal standard.
- Use explicit seconds/time spelling from current grammar. Record master cut time separately
  from child/source time; offsets, trim, loops and rate changes alter the mapping. Verify a
  recognizable gesture/cue after every retime. A curve time warp is different from a scalar rate.
- Treat output as a frame interval with a defined start/end convention. For a four-second,
  24 fps output with an exclusive end, 96 frames are expected. A key at the end may shape approach
  to that boundary without producing an extra output frame. Check the actual exporter settings.
- Reserve usable handles for editorial adjustment and required evaluation outside the chosen
  cut. Simulation pre-roll is history needed to reach the first visible state; it is not merely
  spare editorial footage. Both need valid source data.
- Do not change tick resolution on an already keyed asset as a casual retime. The inspected
  writer refuses that change; creation is where the initial resolution is set.

Recheck camera, body, face, prop, effect and audio timing together. Retiming only the body can
move the contact away from its effect and sound; retiming a child can expose missing handles.

## Apply safely and prove each connection

1. Read the intended saved/live state and current coverage. Preserve existing aliases, binding
   identity, sections, warnings and the current signature. Avoid taking a filtered diagnostic view
   as a complete document.
2. Edit the supported authored document. For `sequence { }`, carry existing binding/track entries
   when applying under `omission:"patch"`: the inspected writer refuses accidental omissions.
   Use explicit deletion only for an intended removal. Under `omission:"snapshot"`, omission
   removes entries; it requires the complete intended set. These are **not** `mode` values.
3. Dry-run the exact target/payload; inspect changes, removals and warnings. Apply within the
   authorized scope, then read back from `live` and `saved`. Re-read after a failed write too.
4. Play the affected passage through its actual sequence/player. Check the camera, performer,
   component, clip and event that should respond. For a durable claim, reopen the saved result.
5. Replay the neighboring cuts and full scene after a consequential timing or ownership change.
   Preserve current evidence and unresolved notes in the shot/production record.

Patch does not mean every missing interior field is preserved. A carried binding template can
reset omitted component deviations; section/channel rules also have their own replacement and
default behavior. Read those rules before shortening a declaration or rebuilding its interior.
See [TOOLS.md](TOOLS.md) and current `gc_describe kind:"omission"`.

## Handle capability gaps explicitly

The inspected Level Sequence read declares omitted custom/dynamic bindings, possessed-object
targets, camera cuts into another sequence, non-scalar section time warps and director graphs.
Use native Sequencer for a required omitted feature and verify that it survives nearby GraphCode
edits. Never remove an omission token simply to make a document look complete.

The current inspected sequence writer does not carry the sound asset/trim/loop payload of a
native audio section; `sound=` work was staged, not installed at this guide's audit. Recheck
current help/build before using it. Until supported and verified, place audio through native
Sequencer or the project's proven composition, then listen to the whole result. Audio channel
support or Sound Wave import does not establish audio-section authoring.

An event key naming an endpoint does not author the director graph that implements it. Check
the director through its supported Blueprint/native route and exercise event timing, repeated
evaluation and skip behavior. Similarly, verify actual Control Rig, Groom Cache, Geometry Cache,
Niagara and other specialized section data before relying on a generic track projection. Use a
supported native workflow where necessary; keep source binaries and dependencies recoverable.

## Choose the right review and render route

`gc_render` scene views help inspect a set, camera framing, asset motion and critical poses.
Asset spans/contact sheets are useful diagnostics. Isolated Blueprint simulation has a different
world/lifecycle from the actual level. The inspected bridge has no Level Sequence render route:
`at`, `to` or `video` on a scene preview does not play a master cinematic through Sequencer/MRQ.
Use the real sequence/player for timing and cut approval, and native output for delivery.

For a preproduction view study, label the question, variant, camera/scene state and evidence
route. A still can resolve sightlines; a short real sequence pass can resolve a pan's timing or
an adjacent cut. Save the selection and update boards/shot records before promoting a candidate
into final production. The [study procedure](CINEMATICS-STORYBOARDS.md#run-simple-view-studies-before-selecting-final-shots)
defines the comparison and its stopping condition.

For a rendered film, use Unreal's
[Movie Render Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/movie-render-pipeline-in-unreal-engine?application_version=5.7).
Record the exact map, sequence, saved revision, output range/rate, resolution, quality preset and
destination. Save dependencies before an external render process reads them. Test a difficult
passage and cut before queuing the full output; keep that preset with the production record.

Native UE5.7 [render settings/formats](https://dev.epicgames.com/documentation/unreal-engine/cinematic-render-settings-and-formats-in-unreal-engine?application_version=5.7)
include image-sequence output, separate WAV audio and shot/render burn-ins. Selected preproduction
stills are different from contiguous output frames. For offline image-sequence delivery, record
frame naming, rate, ranges and sound synchronization, inspect completeness and assemble a timed
review copy through the chosen finishing workflow. Choose format/color with that workflow; do not
assume a frame folder or separately exported sound proves the assembled result.

Specify the color pipeline and delivery format before finishing: working/render space, display
transform, tone mapping, compositing needs and output encoding. A grading master and a viewing
copy can have different purposes. Use native
[render settings and formats](https://dev.epicgames.com/documentation/unreal-engine/cinematic-render-settings-and-formats-in-unreal-engine?application_version=5.7)
to configure the agreed result. Check an actual exported image in the intended viewer so a
missing or doubled display transform cannot be mistaken for a lighting problem.

Prepare history for cloth, hair, particles, exposure and temporal rendering. Epic's
[UE5.7 pre-roll guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-pre-roll?application_version=5.7)
distinguishes system-only warm-up from world-aware evaluation and rendered warm-up needed by
depth/GBuffer-dependent effects. Give moving systems valid preceding animation/transforms where
needed; settling at a frozen initial pose may not reproduce a trail or moving garment. Determine
counts by rendering the actual content. Inspect the first frames of **each shot**, not only the
start of the master, and compare a shot rendered alone with its full-sequence state.

Assemble the output at its intended frame rate with the intended sound. Check missing/duplicate
frames, cut boundaries, focus, motion blur, temporal smearing/flicker, streaming/LOD changes,
simulation resets, exposure jumps, audio startup/drift and the final tail. Listen to the exported
mix; a waveform or separate audio preview cannot prove synchronization. Measure render cost and
storage for offline delivery; measure frame time, loading and memory on target hardware for games.
Offline sampling quality cannot certify real-time performance.

## Integrate an in-game cinematic as a lifecycle

Before production expansion, define entry, normal completion, skip and interruption. The gameplay
owner—not a collection of incidental timeline events—must know which persistent consequences
the cinematic is meant to commit. Reuse the project's state, input, camera and save patterns.

```text
Entry: validate actors/content; acquire needed camera/input/AI ownership; start one player.
Playing: coordinate body/face/props/audio/FX; allow the specified skip/pause behavior.
Completion or skip: reach the agreed story/game state once; resolve animated state explicitly.
Exit: release ownership; restore/blend camera, input, HUD, AI and animation; stop transient cues.
Interruption: cancel or resume by policy; leave a known valid game state and a safe re-entry path.
```

Choose what persists and what restores: a door opening may be permanent while camera/lens, input
suppression and temporary light changes should end. Check section completion settings together
with player-level restoration in the installed engine. Keeping a transform does not move a
gameplay capsule, award a quest or provide cleanup. Skipping to the end and calling Stop can
evaluate events/state differently; implement and test the project's desired consequence policy.

| Scenario | Observe in the actual game |
|---|---|
| Enter and finish normally | Correct actor instances, camera, animation, sync and exactly-once consequence; expected control/camera at exit. |
| Skip early, near an event and near the end | Agreed final gameplay state, no missing/duplicate reward, leaked input lock, orphan audio or lingering spawnable. |
| Pause/resume if supported | All required layers follow the chosen clock/policy and recover without drift or a jump. |
| Replay/retrigger | Re-entry is intentional; transient state resets and persistent changes do not apply twice. |
| Unload, death, scene change or other supported interruption | Camera/input/AI and effects release safely; no stranded player or invalid actor reference. |
| Fresh load and packaged candidate | Saved bindings, cooked assets, streaming and runtime dependencies work without editor-only assistance. |

Only claim multiplayer behavior if the design needs it and server/client paths were exercised.
For prerendered movie playback in a game, additionally test decode, audio/subtitles, skip and
state handoff through the actual media player; an offline film export is only its source.

## Diagnose the earliest failed connection

| Observation | First discriminating check |
|---|---|
| Keys exist, intended actor stays still | Resolve the binding in the destination map; identify the runtime instance and competing property owner. |
| Camera moves but final view differs | Check the active camera cut/player, camera component, filmback/aspect and output settings. |
| Child looks right, master differs | Compare source-time mapping, hierarchy overlap, bindings, spawn lifetime, bias and shared tracks. |
| Foot/prop jumps at a cut | Inspect action phase, actor versus clip root travel, contact continuity and section completion. |
| First frames look wrong | Render the same shot with valid prior world/animation history; inspect streaming, simulation and temporal accumulation. |
| Speech looks plausible but is out of sync | Capture body/face/audio together through the real player and output; compare start and later sync landmarks. |
| Sequence ends but game cannot continue | Trace the exit/skip cleanup and ownership release, not just the final timeline frame. |

Record the exact observation and repair that cause. Re-test the changed passage plus affected
neighbors. Follow [VALIDATION.md](VALIDATION.md) for evidence labels, save/reload, performance and
package checks. A source-checked recipe, successful apply, beautiful still, playable preview and
finished delivered cinematic are separate claims.
