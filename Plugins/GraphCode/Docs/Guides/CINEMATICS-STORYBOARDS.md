# Make storyboards, view studies and an animatic

A storyboard works out the sequence through readable images of action, viewpoint and change.
Build it from [presentation intent](CINEMATICS-INTENT.md) and
[artistic direction](CINEMATICS-DIRECTION.md); keep it in the
[director's packet](CINEMATICS-PREPRODUCTION.md). Use drawings, simple vector panels, reference
staging or rough 3D captures as appropriate. The necessary skill is communicating the view and
its development clearly enough to evaluate it.

Disney's [story process](https://www.disneyanimation.com/process/story/) develops rough thumbnails
into clearer boards. Its [layout process](https://www.disneyanimation.com/process/layout/) takes
edited boards into rough 3D staging while allowing creative revisions. Use that relationship:
boards guide spatial tests, and evidence from those tests can improve the boards.

## Know what each artifact proves

| Artifact | Purpose | Limit |
|---|---|---|
| Reference or key image | Establish treatment, design or an emotional moment | Does not establish the sequence's action, geography or timing. |
| Beat sheet | Specify changes in action, knowledge and intention | Does not show the actual composition. |
| Staging diagram | Map actors, objects, axes, paths and cameras in space | Does not show what overlaps or reads from the final view. |
| Storyboard | Show the important compositions and changes within/between shots | Still panels cannot establish exact performance or temporal response. |
| View study | Compare plausible framings or camera behavior against a named question | Proves only the tested conditions; a still does not prove a move or cut. |
| Shot list/record | Organize production and its editorial relationships | A table of shot names does not replace visual panels. |
| Animatic/story reel | Test a continuous timed image/sound progression | Rough motion/assets and temporary sound still need production and integration proof. |
| 3D layout/previs | Test space, camera, rough performance and edit together | Does not close final-art, final-sound or runtime-delivery review. |

Use **scene** for a dramatic or presentation unit, **shot** for a continuous intended camera view
between edits, and **panel** for a significant illustrated moment. One shot may need several
panels. Software may use these terms differently; preserve the project's IDs through export.

## 1. Turn the brief into drawable beats

For each beat, write the audience's before/after state, the necessary visible/audible evidence,
and the viewpoint. Describe observable action: "the courier's foot redirects toward the parcel"
can be boarded; "a meaningful feeling" still needs a behavior or relationship.

Decide what is primary, secondary and withheld using [attention planning](CINEMATICS-ATTENTION.md).
Identify opening orientation, reveals, reactions, decisions, consequences and the ending image.
Do not assign a new shot to every line or beat automatically. Try keeping related changes within
one view when their relationship is the point.

## 2. Map the action before drawing camera views

Draw an overhead staging diagram. Label performers, important props, entrances/exits, action paths,
gaze targets, relevant heights and approximate scale. Mark the action/interaction axis and intended
camera positions or paths. Give objects stable names. For a product, map the moving mechanism and
the viewer-facing surfaces; for a landscape, map landmarks and the intended route of discovery.

Check feasibility: can a performer reach the object, can the camera travel there, and does the
intended view contain the needed relationships? Test uncertain scale or sightlines with simple
3D staging. An overhead diagram and final camera panel must agree; an attractive panel can hide
an impossible angle or contradictory eyeline.

## 3. Thumbnail consequential alternatives

Use the intended output aspect ratio. Start small with large masses, silhouette, horizon, subject
size and foreground/background separation. Put the essential action inside a clearly readable
space. Draw a camera's view, not an overhead diagram pretending to be a view.

For an important reveal or decision, compare meaningful alternatives: shared view versus isolated
reaction, audience-ahead versus shared discovery, held frame versus motivated pan. Keep enough of
the surrounding beat to judge the difference. Select against the brief and treatment, then record
the reason. Do not fill a page with small arbitrary angle changes.

Use consistent perspective and scale cues: eye height/horizon, ground contact, relative object size,
and overlap in depth. Preserve character identity, costume, prop hand and location across panels.
Simple shapes are useful here if they communicate those facts. Detailed rendering is unnecessary
when the unresolved question is where the performer stands.

### Let art direction shape the boards

Show the planned hierarchy of value groups, density, negative space and depth. Use rough tone
when light/shadow separation or a reveal depends on it; use a color key when color relationships
carry the beat. A colored thumbnail should clarify the treatment, not obscure silhouette and action.

For a restrained courier treatment, test modest gestures and the surrounding space that gives the
hesitation weight. For a comic treatment, show the established route and the interruption clearly
enough to time the reversal. Apply the treatment to the sequence, not just a decorative border or
palette on otherwise identical panels. See [contrasting treatments](CINEMATICS-DIRECTION.md#the-same-action-three-artistic-treatments).

## 4. Expand each shot into its significant changes

Draw the entry composition, decisive action/change and exit composition. Add intermediate panels
where a reveal, camera passage, contact or focus transfer would otherwise be ambiguous. Keep a
stable shot ID and panel suffix, such as `SH020-A`, `SH020-B`; a suffix change is not a picture cut.

For a pan, show the start, trigger, important passage and landing. For translation, show how depth
relationships and occlusion change. For a reaction, show the stimulus and the behavioral change,
including a held thought when needed. For contact, show approach, support/contact and release where
they carry meaning. Do not draw only a glamorous start and finish around an unreadable middle.

Use a clear notation legend. One workable project convention is solid arrows for subject travel,
dashed arrows for camera travel, and a labeled curved arrow for pan/tilt. Label start/end and the
cue; an arrow alone cannot say whether camera or character moves. Show optical-focus changes with
named near/far targets and their trigger. These symbols are a suggested convention, not a standard
every application shares.

Mark transitions between shots separately from action inside a shot. For a dissolve or hidden seam,
board the outgoing image, incoming image and the important overlap/occlusion condition. Mark sound
bridges independently: an earlier sound entrance does not move the picture cut. Use
[editing/transitions](CINEMATICS-EDITING-AND-TRANSITIONS.md) for construction and handles.

## 5. Annotate panels so another author can stage them

Keep captions concise and place essential meaning in the image whenever it is meant to be seen.
Use an accompanying record for technical detail that would crowd the panel:

```text
Sequence / beat / shot / panel / revision:
Image: entry, change or exit; viewpoint and framing.
Audience: primary attention, secondary information, what remains withheld.
Action: intention, visible behavior, prop/contact and continuity state.
Camera: hold/move, cue, direction, passage/landing, lens/filmback study if relevant.
Focus: sharpness target and transfer cue, if it affects the beat.
Sound: dialogue, action cue, off-screen source, music/silence and any bridge.
Timing: intended duration/pace and action/cue relationship; provisional until reviewed.
Connection: incoming/outgoing cut or transition; gaze/action/sound relationship.
Dependencies/questions: what needs a view study, performance test or native handoff?
```

Review the panel strip without reading captions first. Can the viewer identify important actors,
actions, relationships and changes? Then use captions to check cues and intentions. A caption
that explains unseen decisive action signals a board that needs another panel or clearer staging.
Sound-led information should be supplied and judged in the animatic.

## Run simple view studies before selecting final shots

Simple shots can help discover useful views before committing to final boards and cameras. ASC's
[game-engine visualization account](https://theasc.com/article/game-on-game-engine-technology/)
describes virtual scouting and early visualization. The procedure below makes that exploration
purposeful; it is an original recommended method rather than a mandated studio test.

1. **Write the question first.** For example: can the courier's voluntary turn and closing gate
   remain readable in one shared view? Name the intended treatment and evidence.
2. **Prepare only the necessary scene.** Use proxy geometry or existing assets with credible scale,
   silhouettes and action. Include rough light/value separation when readability depends on it.
   Include essential cue timing for a temporal question. Record missing conditions.
3. **Choose a small meaningful comparison.** Try a shared profile view, a diagonal relationship
   in depth, and closer connected coverage if those answer the question. Hold action, filmback,
   output aspect and light comparable; note intentional changes. Use the fewest candidates that
   distinguish the alternatives, not a fixed quota of cameras.
4. **Capture the relevant evidence.** Stills can compare composition and sightlines. A short pass
   tests movement, reveal timing and landing. An edited excerpt tests the cut and sound relationship.
   Label camera position/height, lens/filmback, action state, variant and revision; preserve useful
   frame/time references so the selected view can be reconstructed.
5. **Compare against declared criteria.** Look for audience knowledge, gesture/silhouette,
   geography, focus hierarchy, treatment, continuity and technical feasibility. Judge motion and
   cuts at speed. Inspect first/middle/last and occluded moments when diagnosing a problem.
6. **Select and explain.** Record what each candidate answered, why one fits the brief, and why
   the others fail or remain useful elsewhere. If none meets the criteria, revise the hypothesis
   or staging before another comparison. Stop once the question has a defensible answer.
7. **Update the packet.** Put the chosen framing and significant changes into boards; revise
   timing, shot records, staging and dependencies. Archive rejected captures with short reasons.

An illustrative comparison for the courier, not completed capture evidence:

| Candidate | What to inspect | Possible reason to keep or revise it |
|---|---|---|
| Shared profile | Helping gesture and departing tram remain separate/readable | Keep if the decision and cost read; change staging if the helper hides the parcel/contact. |
| Diagonal depth | Courier foreground and gate background connect without overlap | Keep if scale and background action read at delivery size; revise if depth makes the cost too small. |
| Closer reaction + connected wider consequence | Hesitation is legible and the cut preserves time/geography | Keep if the edit communicates one voluntary action; revise if the departure appears unrelated. |

Do not infer final lighting, material quality, performance, exact duration or audience preference
from a proxy still. If a candidate works only because a missing prop or person is absent, the study
has not resolved the actual shot. Revisit important views after final performance and set changes.

For GraphCode, [scene previews](CINEMATICS-GRAPHCODE.md#choose-the-right-review-and-render-route)
can inspect framing and poses. The inspected preview route does not play a master Level Sequence.
Use real sequence playback for a timed camera/cut study; record which route produced each result.

## Example: panel a changing decision

Use [the courier scene](CINEMATICS.md#worked-scene-a-courier-chooses-to-help). This original text
breakdown tells an author what to draw; it is not itself a completed visual storyboard. The shot
IDs describe one candidate construction and do not prescribe how many shots the final scene needs.

| Shot/panel | Image to draw | Change and annotation |
|---|---|---|
| SH010-A | Courier approaching, gate visible ahead on the established route | Goal and geography; indicate travel direction and warning source. |
| SH010-B | Same moving view, courier reacts to the parcel drop outside the initial emphasis | Attention interrupts urgency; mark sound/gaze trigger. |
| SH020-A | Pan starts with the courier's redirected look | Shared discovery; mark start composition, keeping the landing withheld. |
| SH020-B | Pan passage connects the look to the fallen parcel and person | Preserve the spatial link and direction; show any important occlusion. |
| SH020-C | Pan lands on the need, in a composition that can be read | Arrival and reading hold; keep the parcel silhouette clear. |
| SH030-A | Courier between the tram route and the person, looking back toward the gate | Competing choices; body still contains forward urgency. |
| SH030-B | Same view, weight redirects and the courier commits to helping | Decision; mark gaze/weight lead, camera response if any, and warning continuation. |
| SH040-A | Helping action in relation to the gate or connected departure view | Contact and cost; establish the sound/picture relationship deliberately. |
| SH040-B | Completed assistance and recipient's response as departure recedes | Payoff and release; indicate the held ending and sound tail. |

Several panels in `SH020` describe one pan, not three cuts. If the pan passage loses geography,
test another camera/staging choice. If the decision reads in `SH030` without a camera move, do not
add movement solely to make the shot busier. Carry any changed choice into the timing study.

## Review key frames before final recording

Use a frame-first review to settle the important images inexpensively, then prove their timing
in continuous playback. The [director's plan](CINEMATICS-PREPRODUCTION.md) explains why they belong;
[technique selection](CINEMATICS-TECHNIQUE-SELECTION.md) compares the constructions. Key frames
can be drawn boards or rough 3D captures. They are selected significant moments, not a requirement
to render every production frame before the sequence has been planned.

1. **Select the meaningful moments.** Include the opening relationship, cue, first discovery,
   decisive behavior/contact, important camera passage, landing and outgoing/incoming edit images.
   Add occluded or risky moments where the shot could fail. Choose by the action, not a fixed
   sampling interval; use a full pass later to find problems between the samples.
2. **Make the frames comparable.** Use the intended aspect, credible scale/staging and the planned
   treatment's important value/color relationships. Keep the action state comparable across view
   alternatives. Use rough light or effects when they determine readability, rather than polish
   irrelevant surfaces. Record temporary/missing conditions.
3. **Label and arrange a contact sheet.** Include sequence/shot, variant, revision, frame or panel
   ID, frame rate and master/source identity where applicable. Put frames in intended order and
   compare adjacent cuts as pairs. Preserve camera/lens/filmback and scene state in the linked record.
4. **Review the images as a passage.** Check attention order, silhouette, gaze, geography, scale,
   continuity and fit with the treatment. Inspect the important middle, not just the endpoints.
   Record selected/rejected views and the evidence behind that selection.
5. **Time the selected images and test motion.** Build the animatic below with essential sound;
   substitute a rough continuous 3D pass for questions about parallax, occlusion or performance.
   Revise the frame plan if the action cannot read within its proposed timing.

Do not infer the correct state of cloth, hair, particles or exposure from an arbitrary scrubbed
frame. Relevant captured states need the sequence's evaluation history; see
[pre-roll/output review](CINEMATICS-GRAPHCODE.md#choose-the-right-review-and-render-route).
Record whether a frame is drawn, posed, scrubbed or captured from continuous playback/output.

### Keep an explicit frame ledger

Declare frame rate, numbering origin and end convention before timing the sequence. Record actual
rate rather than casually treating a fractional rate as its nearest integer. Use a project display
grid consistently; it is distinct from the engine's tick resolution or measured runtime frame rate.

Keep shot ID/revision, master cut interval, source interval, rate/offset, handles and cue frames.
Name picture cut, action cue, sound entrance/exit, focus transfer and camera landing independently.
A picture boundary does not automatically trim sound or reset simulation. Preserve the
[technical time semantics](CINEMATICS-GRAPHCODE.md#keep-time-and-editing-semantics-explicit).

**Original illustrative plan:** at exactly 24 fps, use zero-based numbering and exclusive ends.
Shot `SH020` occupies master `[240,336)` and source `[12,108)` at 1× with both grids at 24 fps.
It contains 96 output frames, or four seconds. Available source `[0,120)` supplies 12 usable
handle frames before and after the cut range; handles are separate from simulation pre-roll.

| Planned event / useful frame to inspect | Master frame | Source frame | Directing question |
|---|---|---|---|
| Opening composition | 240 | 12 | Is the initial subject and geography clear? |
| Sound/look cue | 252 | 24 | Is the attention change motivated before the pan? |
| Pan begins | 264 | 36 | Does departure preserve the source of the look? |
| Destination intended to become readable | 300 | 72 | Is this the right disclosure moment, and was it withheld earlier? |
| Pan lands | 312 | 84 | Does the composition expose the important detail without a correction? |
| Last displayed frame of this shot | 335 | 107 | Is the exit state suitable for the next view? |
| Picture cut / next shot begins | 336 | Next shot's selected source | Does the incoming image preserve the intended relationship? |

This candidate gives an initial hold `[240,264)`, movement `[264,312)` and landing hold `[312,336)`:
one second, two seconds and one second. The cue precedes movement by 12 frames (0.5 seconds).
Those values are proposed timing to test, not recommended pan durations or observed captures.
Inspect extra frames around the cue, disclosure, landing and cut when diagnosing their timing.

For this unretimed same-rate example, `source = master - 240 + 12` within the shot. Do not reuse
that mapping after a trim, retime, loop or rate change without recalculating it. A panel ID is not
a frame number; several timed panels may describe one shot, and an animatic hold may display the
same drawing over many output frames. A key on the exclusive end does not add an output frame.

Use stable evidence labels, such as `SEQ01_SH020_r03_M000300_S000072.png`, and retain the capture
manifest. That is an illustrative naming convention, not a native render-token or GraphCode command.
Annotate planned timing as planned until continuous review establishes what actually reads.

## Build and review the animatic

Assemble panels in editorial order at the intended project frame rate. Time the actions, thoughts,
reveals and reading holds individually; do not accept equal default panel durations. Add necessary
temporary dialogue, action cues, ambience and sound bridges. Mark provisional music and the pacing
that currently depends on it. Record cue and picture timing separately.

For a movement-dependent beat, use extra panels, a rough camera simulation or a short layout pass
to clarify development. Sliding a flat board can suggest timing but cannot prove 3D parallax,
occlusion or perspective. Substitute a relevant spatial test when those decide the shot.

Watch the passage continuously with sound. Review what changes, whether the cause of each reaction
is available, where attention goes, and whether the ending earns the intended experience. Try a
muted pass for visual-action diagnosis and a sound-only pass for cue/rhythm diagnosis; return to
the combined sequence for the decision. Do not narrate over unclear action to make it seem to work.

Ask a fresh viewer what happened, what mattered and where they became uncertain when available.
Distinguish their observations from the author's self-review. Revise the cause of confusion, then
watch the complete new cut. Check adjacent passages so a locally effective change does not break
the presentation's rhythm or information order.

## Distinguish preproduction frames from final image-sequence output

| Review material | What to establish before moving on |
|---|---|
| Selected board/capture frames and contact sheet | The meaningful compositions, attention and continuity are plausible; selections are documented. |
| Timed animatic with essential sound | Information, thought, reading time, rhythm and transitions work as a complete passage. |
| Continuous rough 3D layout | Actual camera movement, perspective/occlusion, rough performance and neighboring cuts work together. |
| Representative finished passage | Difficult performance/contact, camera, light/FX, simulation history and sound meet the intended quality. |
| Final sequence output and assembled review copy | Every required frame and sound cue survives the actual delivery and full-sequence review. |

Use these stages to resolve different questions; return to an earlier decision when evidence changes.
Disney's [layout](https://www.disneyanimation.com/process/layout/) and
[editorial/sound](https://disneyanimation.com/process/editorial--sound/) accounts support developing
rough staging and the cut before finishing. This staged review is a recommended workflow informed
by those practices, not a claim that every professional production follows one rigid order.

For offline delivery, an image sequence contains contiguous rendered output frames, which can be
assembled at the declared rate with the intended sound. It is different from a selection of board
frames. Use [native output and finishing](CINEMATICS-GRAPHCODE.md#choose-the-right-review-and-render-route)
to choose the format, color pipeline and saved render settings. Missing-frame checks, continuous
picture/sound review and appropriate simulation history still apply. For an in-game cinematic,
verify the real player/runtime as well; an offline image sequence cannot prove that lifecycle.

## Turn the selected plan over to production

Save the selected board/animatic revision and decisions. Link shot IDs to the
[shot records](CINEMATICS.md#keep-a-useful-shot-record), master/source time mapping, camera
start/passage/landing, performance/prop states, sound cues, cut/transition handles and review notes.
Map them to [GraphCode assets and native owners](CINEMATICS-GRAPHCODE.md); board IDs and captions
are planning records, not invented GraphCode syntax or automatic asset bindings.

Resolve the [production readiness review](CINEMATICS-PREPRODUCTION.md#check-readiness-before-production).
Keep boards editable: final performance, contact, cloth, lighting or set composition may expose a
new problem. Record the reason and revise the related shot, timing and dependency records together.
