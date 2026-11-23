# Make a cinematic that tells a story

A cinematic directs the audience's attention through an experience. Its staging, performance,
camera, cut, light and sound must express the same intent. Decide what changes for the audience
and characters before choosing how to photograph it. Artistic direction leads the work: develop
the emotional experience, visual language and sound perspective, then use the tools to realize it.

Read this guide for the filmmaking method, then [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md)
for authoring and verification in Unreal. Use [ASSET-QUALITY.md](ASSET-QUALITY.md) throughout;
unfamiliar work also follows [RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md). An internal rough
animatic is useful production work, but it does not satisfy a request for a finished cinematic.

**For AI cinematic work, act as a director and save the necessary plan before production.** Use
[CINEMATICS-PREPRODUCTION.md](CINEMATICS-PREPRODUCTION.md) for the director's packet and readiness
review. Rough framings or short camera studies may inform the plan after their question and
criteria are written; record their results before choosing final shots. Keep intent, boards,
timing, shot rationale and technical dependencies connected throughout production.

## Learn from complete scenes

Study a passage with an opening, development and ending, including its soundtrack. Record what
the audience knows before and after each beat, whose experience it follows, and how performance,
framing and sound direct attention. Separate observable choices from guesses about the setup.
Watch at speed, then examine cuts and important actions frame by frame. Keep references specific
to the project's tone; an intimate conversation and a mechanical reveal need different evidence.

Professional examples support a connected workflow: Disney's
[story process](https://www.disneyanimation.com/process/story/) develops emotion, timing, staging
and framing in boards, while [layout](https://www.disneyanimation.com/process/layout/) turns
edited boards into rough 3D filmmaking and revisits cameras after animation. Pixar's
[Incredibles 2 account](https://renderman.pixar.com/stories/incredible-cinematography) shows camera,
lighting and effects collaborating early and reviewing in context. The stages below are a
recommended GraphCode workflow informed by those practices; adapt their scale to the project.

## Read the focused chapters

| Working question | Guide |
|---|---|
| What should this presentation accomplish for its audience? | [Presentation intent](CINEMATICS-INTENT.md): brief, use case, viewpoint, knowledge and evidence. |
| What must the AI decide and document before production? | [Director-led preproduction](CINEMATICS-PREPRODUCTION.md): packet, research, planned exploration, decisions, dependencies and readiness. |
| How should the experience be expressed artistically? | [Artistic direction](CINEMATICS-DIRECTION.md): subtext, emotional progression, motifs, composition, color/light and sound perspective. |
| Which technique serves this expressive presentation, and why? | [Technique selection](CINEMATICS-TECHNIQUE-SELECTION.md): intent-to-evidence reasoning, contextual alternatives and coordinated camera/edit/sound choices. |
| How do we make boards and choose useful views? | [Storyboards and view studies](CINEMATICS-STORYBOARDS.md): staging diagrams, thumbnails, significant panels, simple camera comparisons, animatic and turnover. |
| What should the viewer notice, and when? | [Audience attention](CINEMATICS-ATTENTION.md): dramatic priority, visual hierarchy, cues, attention transfer and optical focus. |
| What should each view reveal, and how should the action be staged? | [Shot design and coverage](CINEMATICS-SHOTS.md): framing, viewpoints, blocking, geography, lens/distance and continuity. |
| When and how should the camera move? | [Camera movement](CINEMATICS-CAMERA-MOVEMENT.md): pans, tilts, tracking, push/pull, orbit, crane, zoom, focus and operator behavior. |
| How should shots and scenes connect? | [Editing and transitions](CINEMATICS-EDITING-AND-TRANSITIONS.md): rhythm, cut types, J/L edits, dissolves/fades/wipes, handles and hidden/whip seams. |
| How do these choices become a verified Unreal sequence? | [GraphCode execution](CINEMATICS-GRAPHCODE.md): assets, bindings, channels, native handoffs, output and gameplay recovery. |

## 1. Write the purpose and artistic treatment

Define the audience experience, starting state, meaningful change, ending state, viewpoint and
delivery with [CINEMATICS-INTENT.md](CINEMATICS-INTENT.md). Develop key images, performance
references and a visual/sound progression around that intent. Start the
[director's packet](CINEMATICS-PREPRODUCTION.md#save-the-directors-packet) and record each
consequential choice, its evidence and its effect on the production plan.

## Develop the artistic treatment

The [direction chapter](CINEMATICS-DIRECTION.md#develop-the-artistic-treatment) gives the detailed
method for emotion, subtext, composition, motifs, light/color and listening perspective. It also
compares three artistic treatments of the same action. These choices lead the production below.

## 2. Build a beat sheet and edit an animatic

A beat is a meaningful change of attention, information, intention or pressure. One beat can
span several shots; one sustained shot can contain several beats. Write the beats first, then
try the coverage that makes them read.

| Beat | Audience knows/feels before → after | Visible action or evidence | Sound/contrast | Why the next beat follows |
|---|---|---|---|---|
| A named change | The specific change, including withheld information | Something the camera can show | A cue, silence or transition | Cause, reaction, discovery or deliberate association |

Use [CINEMATICS-STORYBOARDS.md](CINEMATICS-STORYBOARDS.md) to make readable visual panels and
turn them into a timed story reel with temporary dialogue, ambience and key cues. A page of
attractive stills cannot establish pacing. Use rights-cleared temporary material and track what
must be replaced. Reference images guide treatment; they do not prove performance or cuts.

Where the space or framing is uncertain, make
[simple view studies](CINEMATICS-STORYBOARDS.md#run-simple-view-studies-before-selecting-final-shots)
with written questions and comparison criteria. Stills, short passes and edited excerpts answer
different questions. Use their results to revise the boards and shot plan before final authoring;
storyboarding and rough layout can inform each other.

Review [key frames and a frame ledger](CINEMATICS-STORYBOARDS.md#review-key-frames-before-final-recording)
before final recording: compare important compositions, time the selected boards with sound,
then test continuous rough layout. Stills and a contact sheet cannot establish motion or pacing.

Try alternatives while revision is inexpensive: reveal the threat before the character sees it,
or share the discovery with them; stay with the listener, or show the speaker; hold the action,
or omit time. Compare the effect on the audience. Remove redundant exposition and allow time
to read unfamiliar objects, relationships or expressions.

Sound belongs here. Disney's [Editorial & Sound](https://disneyanimation.com/process/editorial--sound/)
describes editors shaping rhythm and pacing throughout production with dialogue, temporary
effects and music. Establish important sound timing before polishing the pictures.

**Review:** watch the whole animatic without pausing or narrating its intent. Ask a fresh viewer
what happened, what mattered and where they became confused. For visual action, a muted pass
helps diagnose staging; it does not replace the sound review or require dialogue scenes to
communicate every fact silently. Revise the cause of confusion before finishing more shots.

## 3. Stage the complete action in 3D

Block performers, props and environmental events through the whole passage. Rehearse intention,
relationships, gaze, support and contact before dressing the set. Draw the relevant paths and
geography. Use [the staging procedure](CINEMATICS-SHOTS.md#stage-the-complete-action-in-3d) and
verify that rough playback communicates the action.

## 4. Design coverage and attention

Give every view a function: what we attend to, what changes, what is revealed/withheld and why
we enter/leave now. Plan [attention and focus](CINEMATICS-ATTENTION.md) across each beat and cut.
Choose shot sizes and viewpoints from [CINEMATICS-SHOTS.md](CINEMATICS-SHOTS.md),
then test their attention transfer, continuity and performance in an edit.

## 5. Design the camera behavior

Choose position/lens with the staging. Hold, move or reframe for an intended audience effect.
Use [CINEMATICS-CAMERA-MOVEMENT.md](CINEMATICS-CAMERA-MOVEMENT.md) for when/how to execute each
move, including a detailed pan rehearsal. Check the entire path, landing, focus and adjacent cut.

## 6. Develop the edit and transitions

Cut for meaning, thought and rhythm while layout is still inexpensive to revise. Choose how
time, place, viewpoint and sound carry across each boundary. Follow
[CINEMATICS-EDITING-AND-TRANSITIONS.md](CINEMATICS-EDITING-AND-TRANSITIONS.md) for cut selection,
split edits, image transitions, handles and seam construction. Review the full scene with sound.

Before finished production, record the
[director's readiness review](CINEMATICS-PREPRODUCTION.md#check-readiness-before-production)
and save the chosen boards, timing, shot decisions and resolved dependencies as the baseline.

## 7. Finish the performance and its physical relationships

Develop acting around thought and intention: noticing, processing, deciding, acting and responding.
Avoid distributing gestures evenly through a line. Listening, hesitation and suppression may carry
the scene. Use reference to judge weight and timing while preserving the character's personality.
Disney's [animation practice](https://www.disneyanimation.com/process/animation/) emphasizes staging,
anticipation, weight and secondary action as parts of believable performance.

Follow [ANIMATION.md](ANIMATION.md) for construction and native composition. Check body and face
together, including gaze, blinks, breath and moving holds appropriate to the character. Inspect
feet on the actual ground, hands on the actual prop, changes of support, contact/release and
root travel. Decide whether the source clip or actor/sequence transform owns translation; two
independent journeys can double the movement.

Evaluate hair, clothing, accessories and effects through the movement and cuts. Secondary motion
should respond to the action without obscuring it. Disney's
[technical animation](https://disneyanimation.com/process/technical-animation/) describes this as
intentional, physically believable work combining rigging and simulation. Revisit framing after
the final performance; rough and finished animation may occupy different space.

**Review:** performance reads at speed in the cut; then inspect critical contacts and changes
frame by frame. A strong key pose does not prove the transition into or out of it.

## 8. Develop lighting, color, effects and sound as one scene

Design the scene's lighting logic and color progression early. Identify the focal subject and
what makes it readable against its surroundings. Check faces, silhouettes, materials and depth
in motion; keep practical sources and the environment coherent across coverage. Shot-specific
adjustments can support the composition, but review their continuity in the cut.

Build effects around the story event: anticipation, onset, peak, consequence and decay. Place
important action before adding obscuring dust, bloom or particles. Design background activity so
it supports the world without competing with the decisive gesture or reveal. Finishing detail
should reward attention at the intended screen size.

Create a sound map with dialogue, breaths/effort, Foley, world ambience, designed effects, music
and intentional silence. Name which cues are on-screen, off-screen, spatial or subjective, and
what starts/stops them. Dialogue intelligibility and meaningful effects need space in the mix.
Carry ambience and reverberant tails deliberately through edits, avoiding accidental resets.

Listen to the full mix on the intended playback system and useful secondary systems. Inspect
sync at decisive gestures, impacts and speech; listen for masking, clipping, abrupt tails and
fatiguing constant loudness. Keep editable sources/stems and record the delivery requirements.
Disney's [post-production overview](https://disneyanimation.com/process/post-production/) treats
sound and color finishing as part of the film through final delivery. Follow
[CONTENT.md](CONTENT.md) for project sourcing and provenance.

**Review:** compare neighboring shots for subject readability, exposure, palette and sound space.
A dramatic change may be correct; an unplanned brightness or ambience jump needs a cause and repair.

## Worked scene: a courier chooses to help

This is an original teaching example, not a mandatory shot count or a production asset. A courier
is about to catch the last tram. Someone drops a parcel; helping will cost the courier the ride.
The audience should see urgency become a choice, then understand its cost.

Stage the tram gate and fallen parcel on different paths within one readable space. Establish
the departing tram with sight and sound before the decision. Keep the courier's route and gaze
targets consistent. The following coverage is one option to test in boards and rough layout:

| Passage | Change for the audience | Staging, camera and edit purpose | Sound |
|---|---|---|---|
| Approach | The courier is nearly in time | Follow the purposeful approach; show the gate and route clearly enough to understand the goal. | Footsteps and a departure warning establish pressure. |
| Interruption | Another person needs help | A stumble or falling parcel enters the courier's attention; preserve geography through the look and reveal. | The drop interrupts the established rhythm. |
| Recognition | Helping will cost something | Hold the courier between the two possibilities; an insert is useful only if a small detail is necessary to understand the need. | The warning continues; avoid music explaining the decision too early. |
| Choice | The courier commits to helping | Show the change in gaze, weight and direction, then the handoff/contact. Stay with enough of the action to make agency clear. | Effort, handling and the gate beginning to close. |
| Consequence | The ride is lost, but help mattered | A wider relationship can show the closed gate and completed assistance; finish on the response that earns the intended emotion. | Departure recedes; the remaining ambience gives the ending space. |

Try a second edit where the audience sees the need before the courier does. It creates anticipation
instead of shared discovery. Compare where viewers notice the dilemma and whether the choice is
clear. If they cannot tell why the courier stopped, repair evidence and staging. Adding a crane
move does not resolve that missing information.

Compare [three artistic treatments](CINEMATICS-DIRECTION.md#the-same-action-three-artistic-treatments)
and [specific coverage/move/edit choices](CINEMATICS-SHOTS.md#example-choose-coverage-for-a-dilemma).
Map the approved beats to GraphCode assets and native handoffs using the companion. First finish
a representative passage containing a reaction, contact, camera/focus change and cut; expand
after the passage proves the intended craft and actual authoring/playback connections.

## Keep a useful shot record

Use stable shot IDs so editorial changes do not rename every source. Keep the beat and editorial
record in the project's production note; use GraphCode for the actual supported asset authoring.

```text
Shot ID / revision / owner:
Story function and viewpoint:
Emotional turn, subtext and visual/sound motif if relevant:
Entry state → action/turn → exit state:
Master cut range / source range / working handles and required pre-roll:
Camera framing, behavior, cue, passage/landing, lens/filmback, focus targets:
Selected technique, meaningful alternative, reason and linked study/review evidence:
Performance sources; root travel, gaze, prop and contact ownership:
Lighting/FX purpose and dependencies:
Picture cut/transition range; independent dialogue/sound cue and tail ranges:
Preceding/following bridges; source-time mapping after retimes:
Continuity with neighbors:
GraphCode assets and native handoffs:
Latest review evidence, unresolved notes and next change:
```

Handles are extra usable source time around the chosen cut for retiming and finishing. Choose
them for the production; do not confuse them with simulation pre-roll or assume extra frames
exist in a source that was never animated there.

## Review gates before calling it finished

| Stage | Required output | Exit condition |
|---|---|---|
| Intent and references | Brief, beats, visual/sound treatment and study plan | The intended audience change and delivery are clear; each consequential unknown has a distinguishing check. |
| Story reel | Edited boards with temporary sound | The scene's meaning and progression read without the author explaining them. |
| Layout/previs | Rough 3D performance, cameras and cut | Staging, geography, coverage and timing work together; authoring dependencies are proven. |
| Production baseline | [Saved director's packet](CINEMATICS-PREPRODUCTION.md#check-readiness-before-production) with selected views, shot/transition rationale and asset/technical owners | Necessary decisions are recorded, timing reviewed and blocking unknowns resolved before dependent production work. |
| Representative finish | A difficult passage at the intended quality | Performance, physical contacts, camera, look and sound work in the real composition. |
| Sequence finish | Complete revised cut and mix | Every shot and transition is reviewed, including first/last frames and simulation history. |
| Delivery/integration | Exported film or packaged in-game sequence | Actual output, synchronization and applicable skip/replay/control recovery pass. |

Watch dailies at the correct speed and with an appropriate color transform; Disney's
[review workflow](https://disneyanimation.com/process/collaboration--rendering-technologies/)
emphasizes accurate frame rate and color. Review intent/comprehension first, then performance,
continuity, look/sound and technical defects. Keep notes tied to observed moments. Repair the
earliest cause, then replay the changed passage and affected neighboring shots.

For a rendered film, approve the assembled export and mix, not just selected stills. For an
in-game cinematic, also exercise entry, completion, early/late skip, repetition and return of
player control. [CINEMATICS-GRAPHCODE.md](CINEMATICS-GRAPHCODE.md) defines those technical checks;
[VALIDATION.md](VALIDATION.md) records what each kind of evidence actually proves.
