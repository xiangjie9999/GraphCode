# Edit cinematic scenes and construct transitions

Choose what the audience carries from one image into the next. This chapter covers rhythm,
continuity, split edits, time/space relationships, dissolves/fades/wipes and hidden/whip seams.
Start with [artistic direction](CINEMATICS-DIRECTION.md) and [coverage](CINEMATICS-SHOTS.md).
Use [camera movement](CINEMATICS-CAMERA-MOVEMENT.md) to stage the moves and
[GraphCode execution](CINEMATICS-GRAPHCODE.md#author-camera-moves-and-editorial-transitions)
for the supported authoring route. Examples and suggested exercises are original recommendations.

Use [technique selection](CINEMATICS-TECHNIQUE-SELECTION.md) for the expressive relationship
between camera, cut and sound. Keep picture boundaries, cue frames and source/master ranges
in the [frame ledger](CINEMATICS-STORYBOARDS.md#keep-an-explicit-frame-ledger) before final recording.

## Cut for meaning, performance and rhythm

Make the edit while developing layout. A cut can reveal an answer, emphasize a reaction, advance
time, preserve an action or create a deliberate collision of ideas. Give it a reason. Equally,
recognize when staying in the shot makes the performance stronger.

Try entering later or leaving earlier; also try holding longer when comprehension or emotion
needs it. Compare versions in context rather than optimizing each shot's duration independently.
Do not cut away simply because the previous shot has reached an arbitrary length. An animation
clip's endpoint is not automatically the right editorial endpoint.

When matching action, check the phase on both sides of the cut so the event neither repeats
unintentionally nor loses its decisive moment. When omitting time, make the omission readable.
Use listening and reactions to express subtext. An incoming sound can lead the picture into a
new shot; an outgoing sound can continue over the next image. Judge these sound bridges as part
of the scene's perspective and rhythm.

Music may drive a montage, but do not force every cut onto a beat. Listen for whether the scene's
own tension, silence and performance still work. Professional editors discuss these choices in
specific contexts in [BAFTA's editing session](https://www.bafta.org/media-centre/press-releases/bafta-film-sessions-2022-editing/);
there is no universal duration formula to copy.

**Review:** watch the whole edit, then each cut with the outgoing and incoming passages. Log the
frame/shot, what the viewer misses, the likely cause and the change to try. "Make it cinematic"
is not an actionable review note.

## Choose an editorial transition

A transition determines what the audience carries from one image into the next. State whether
time, place, viewpoint or emotional state changes, what remains connected, and what the viewer
should understand immediately. Start with a straight cut and compare a more conspicuous choice
when its extra meaning or rhythm serves the passage.

| Edit | Choose it when | Construct it | Watch for |
|---|---|---|---|
| Straight cut | The next viewpoint, event or response should arrive directly. | Choose the exact attention/performance change; compare outgoing and incoming subject position, movement and sound. | A cut with no new purpose, missing geography or a reaction removed too early. |
| Match on action | One continuous action should survive a viewpoint change. | Cover the action with usable overlap, then choose compatible phases of reach/contact/release and screen direction. Trim in context. | Repeated impact, missing contact, hand/prop changes or a distracting motion jump. |
| Eyeline / look-to-object cut | A look motivates the next information. | Establish a gaze and show a target that occupies a believable direction/height; include the response when it changes meaning. | A target that seems to be in a different place, or a view that accidentally claims impossible POV. |
| Shot/reverse shot | An exchange develops through contrasting viewpoints. | Preserve the intended axis/eyelines and choose moments around listening, thought and interruption. Vary size only when its function changes. | Cutting mechanically at every line; incorrect gaze or hiding the relationship through isolated coverage. |
| Reaction / cutaway / insert | A response or detail changes interpretation, supplies evidence or gives useful editorial time. | Establish its relevance, match continuity and return at a readable point if returning. | Padding, an unrelated detail or using a cutaway to conceal an unresolved causal action. |
| Graphic / conceptual match | Similar form, movement or sound should connect different moments/ideas. | Design the outgoing and incoming anchor together: shape, position, scale, direction and the intended meaning. | A clever shape resemblance without a useful connection, or confusing a new place/time with continuous action. |
| J cut | Incoming sound should prepare or pull us toward the next image. | Begin that sound before the picture cut; choose how long its source remains unseen. | Revealing a surprise too early, unintelligible overlap or a false assumption about where the sound originates. |
| L cut | Outgoing sound should continue while we see a response/new place. | Keep the sound after the picture cut, with a designed end/tail. | Losing the listener's thought under dialogue, repeated sound or a tail cut off at the next edit. |
| Jump cut / deliberate ellipsis | Missing time or discontinuity should be felt. | Remove a chosen interval, retaining enough action/context to read the change; establish a consistent pattern if repeated. | An accidental-looking body/camera jump or concealing the decisive action. |
| Cross-cutting | Interleaved actions should create anticipation or a relationship, often suggesting simultaneity. | Establish each strand's geography and progression, then choose the moments when returning changes expectation. | Suggesting concurrency or a rescue deadline the events do not support; relentless equal alternation. |
| Montage / parallel construction | Selection and juxtaposition should compress development or compare ideas. | Give the sequence its own progression and variations in image, rhythm and sound. | A collection of interchangeable beauty shots with no development. |
| Smash cut | A sharp contrast should deliver surprise, rupture or comic punctuation. | Design the contrast in image/sound and the incoming beat's readability. | Using an arbitrary loud spike or brightness change without a meaningful payoff. |

Adobe describes [jump cuts](https://www.adobe.com/creativecloud/video/post-production/cuts-in-film/jump-cut.html)
and [cross-cutting](https://www.adobe.com/creativecloud/video/discover/cross-cutting-film) as distinct
time/space choices. The primary [match-cut research](https://arxiv.org/abs/2210.05766) identifies
framing, composition and action correspondences. The construction/review suggestions here apply
those ideas to authored scenes; matching on a continuous action and a conceptual graphic match
have different storytelling jobs even when their terminology overlaps.

### Finish a continuity cut

Rehearse the whole action before selecting the cut. Inspect the outgoing frame, incoming frame
and several beats on both sides. Match support, gaze, hand, prop state, trajectory and physical
phase where the action is continuous. Choose a visibly useful new view; a tiny angle/size change
may read as a bump. A conventional angle guideline cannot guarantee a good cut.

Then watch at speed. A literal pose match can feel slow if it repeats perceived action, while
an apparent small timing mismatch may read better in motion. Adjust by viewing the event, not by
forcing equal frame offsets. Preserve the decisive contact/cause. Use sound deliberately across
the boundary; do not double an impact because both source shots contain it.

### Separate picture time from sound time

Adobe's [J/L-cut guide](https://www.adobe.com/uk/creativecloud/video/discover/j-cut-and-l-cut.html)
distinguishes incoming audio leading the picture from outgoing audio continuing over the next
image. They can occur within a scene or between scenes; neither requires a visual blend.

**Original examples at 24 fps, with exclusive end frames:** picture A occupies `[0,48)` and B
occupies `[48,96)`. For a J cut, start B's bell at frame 36: the viewer hears it 12 frames
(0.5 seconds) before B appears. For a separate L-cut version, let A's final words continue until
frame 60: B's listener is visible for 12 frames before those words finish. These are illustrative
placements; choose the overlap by the information/performance it creates.

```text
Picture in either version:  A A A A A A | B B B B B B
J-cut version:              A sound  [B bell starts] --->
L-cut version:              A dialogue -------> ends over B
                                      ^ picture cut
```

Keep any visible speech synchronized to its performance. A split edit changes which picture is
shown; it does not justify shifting audible syllables against a visible speaking mouth. Build
room tone, ambience and reverberant tails around the transition, then listen to the exported mix.

### Construct a graphic match or cross-cut passage

For a graphic match, write the connection in words before matching shapes. A clenched hand
releasing a ticket → an open hand offering help can develop a character idea; a round lamp →
a round wheel may be only visual resemblance unless their relationship matters. Choose a
dominant anchor and align its screen position, scale, orientation or motion deliberately. Compare
direct cuts and brief overlaps, then establish the incoming time/place when the story needs it.
The viewer should perceive the intended connection without mistaking separate actions for one.

For cross-cutting, first edit each strand so its cause and progression work independently. Mark
the discoveries, deadlines and consequences that make a return useful. Interleave at those
changes rather than alternate for equal durations. Establish enough geography to recognize the
strand quickly and make the implied time relationship honest. Sound can bridge the strands or
separate them; choose which experience serves the scene. Review whether the final convergence,
missed encounter or contrast is earned by what the audience was allowed to know.

## Construct image transitions

| Transition | Choose it when | Construction and failure check |
|---|---|---|
| Cross dissolve | Overlapping impressions, a soft time/place passage or an established associative style serves the scene. | Keep valid moving images on both sides, design the overlap and its duration, and inspect the composite. Faces and bright shapes can form distracting double images. |
| Fade in / fade out | An image should emerge from or give way to a solid field, often framing a beginning/ending. | Design fade duration, color, hold and sound independently. Verify the last state clears or persists as intended. |
| Dip through black/white/color | A deliberate punctuation or interval belongs between the images. | Fade A into the chosen field, define any hold, then reveal B. Check whether it implies an ending, loss of time or impact that the story supports. |
| Wipe / iris / designed graphic transition | A conspicuous formal device belongs to the film's style or chapter structure. | Choose direction/shape in relation to composition; preserve the important outgoing/incoming action and a consistent visual vocabulary. |
| Object wipe / occlusion | A foreground crossing can connect views or conceal a change. | Stage full coverage or a designed mask and match motion/depth at the seam. A translucent edge, shadow or uncovered corner can disclose the cut. |

Adobe's [dissolve reference](https://helpx.adobe.com/premiere/desktop/add-video-effects/effects-and-transitions-library/list-of-video-dissolve-transitions.html)
distinguishes cross dissolves and dips through a color. A dissolve is not automatically more
professional or smoother in meaning than a cut. Test what the overlap adds; use a direct cut if
it already expresses the relationship with better timing.

### Build a dissolve with usable source handles

1. Approve the relationship and approximate edit first. Choose what the audience should retain
   from A while discovering B. Select complementary composition and motion for that overlap.
2. Define the transition interval and alignment. Reserve genuine animation/render source beyond
   A's chosen out point and before B's chosen in point. Both images must exist wherever needed.
3. Composite the actual images in the agreed color pipeline. Evaluate the entire overlap,
   especially its middle, for brightness, double exposure, faces and competing attention.
4. Refine timing with sound and neighboring shots. Re-render source handles when necessary;
   extending an edit beyond valid action cannot create missing performance or simulation history.
5. Review the exported result at the delivery rate. Check repeated/frozen frames, cropping,
   exposure/color shifts and the start/end of sound bridges.

**Original handle calculation:** at 24 fps, a 24-frame centered dissolve around cut frame 100
occupies output `[88,112)`, lasting one second. A must continue 12 frames beyond its original
out at 100, and B must provide 12 frames before its original in at 100. Those are extra usable
handles; the entire 24-frame overlap needs both images. Different alignment changes each side's
requirement. Specify source-to-output mapping after retimes rather than count unretimed handles.

Adobe's [historical Premiere reference](https://helpx.adobe.com/archive/en/premiere-pro/cc/2015/premiere_pro_reference.pdf)
describes the underlying handle requirement and repeated-frame fallback. The principle applies
to source planning here; use current instructions for the chosen finishing application.
Editorial handles and simulation/render pre-roll solve different problems. A handle can still
render incorrectly if cloth, particles or temporal history was not prepared before it.

### Build a fade, dip or designed wipe

For a fade out, choose the last meaningful action/image and decide whether it remains perceptible
while the field takes over. Set the fade's start, progression, fully covered state and any hold.
For a fade in, decide what becomes recognizable first and whether sound has already introduced
the space. For a dip between scenes, design both halves and the interval between them; the black
or color hold has its own duration and meaning. Compare with a direct cut before settling it.

For a wipe or iris, design the moving boundary against both compositions. Decide which important
element disappears last and appears first. Rehearse direction, speed, shape and any match with
on-screen movement; inspect the entire composite for cut-off gestures and distracting edges.
Keep a deliberate vocabulary across the film. A formal chapter transition may repeat reliably;
a one-off conspicuous wipe needs to fit the surrounding style. Sound follows the intended
relationship, rather than automatically fading with the image.

### Distinguish a camera blend from a dissolve

Unreal's [Camera Cut Track](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-camera-cut-track-in-unreal-engine?application_version=5.7)
can interpolate camera transforms and component properties. This produces intermediate views
through the world. It does not overlap two independently rendered images as a cross dissolve.
Use it to join compatible camera behaviors or hand a view between gameplay and Sequencer when
that path is appropriate. Inspect the entire intermediate path, framing, focal length and focus;
two good endpoints do not guarantee a useful transition.

For a genuine dissolve, use the production's image-compositing path. For a solid-color fade,
Unreal provides a [Fade Track](https://dev.epicgames.com/documentation/en-us/unreal-engine/cinematic-color-fade-track-in-unreal-engine?application_version=5.7).
Choose whether sound follows the fade and test its completion/skip state. A fade that leaves
the game black or unintentionally mutes the world has failed its handoff even if it looks good.
The [GraphCode companion](CINEMATICS-GRAPHCODE.md#author-camera-moves-and-editorial-transitions)
explains current authoring coverage for these controls.

## Design a hidden cut or whip transition

A hidden cut aims to maintain an apparent continuous experience while joining separately made
shots. A whip transition may instead be an obvious stylistic scene change. Decide which reading
you need before designing the seam; blur or occlusion alone does not establish spatial continuity.

### Occlusion / invisible-cut workflow

Choose a moment when a near object, darkness or a planned mask can cover the image. Stage the
camera approach and departure so the viewer's perceived route stays coherent. Match the incoming
camera height, movement, scene scale, lighting, exposure, focus and dominant shapes wherever they
remain visible. Keep sound continuous when the experience should remain continuous.

Build a short test across the seam before extending either shot. Inspect the leading and trailing
edges of the covering object, uncovered corners, reflections, shadows, transparency and motion
blur. A composite may require an authored mask/VFX treatment; do not assume an opaque cut point
exists just because the center of the frame is dark. Review at speed and frame by frame, then
check the final encoded output, where compression can reveal the edge differently.

### Whip-pan transition workflow

1. Give the outgoing turn a cue and direction: a thrown object, a sudden look or a deliberately
   stylized scene change. Compose the incoming subject and its landing before making the blur.
2. Author a usable outgoing acceleration and incoming arrival. For a seamless-flow intention,
   match the **visible image motion** through the join, not just the sign of world-space yaw.
   Different lenses and scene depths can make equal angular speeds look different.
3. Place the seam in the rapid passage and compare nearby edit points. Match dominant color/value,
   blur character and exposure, or make their contrast clearly intentional for a scene change.
4. Finish the arrival: readable subject, controlled settling, focus and sufficient time for the
   new beat. An energetic departure without a clear destination is an unfinished shot.
5. Add sound when it serves the action/style. A generic whoosh at every cut can overwhelm cues
   and make unrelated transitions sound identical. Preserve intended direction and perspective.
6. Render/play through the production path. Inspect smearing, judder, temporal history, masked
   edges and the incoming first frames. Compare the full edit; a seamless seam can still interrupt
   the emotion or misrepresent where the character went.

If the change is to a different time/place, provide enough new evidence to establish it. If the
intention is continuous space, verify the implied route. Do not use a whip to avoid deciding the
geography. A straight cut can deliver an abrupt thought more cleanly than a turn through empty space.

## Review and practice

Record outgoing/incoming intent, the picture cut or overlap, independent sound boundaries,
source-time mapping, handles and native handoffs in [the shot record](CINEMATICS.md#keep-a-useful-shot-record).
Watch the whole scene, then the affected boundary with enough lead-in and tail to judge it.
Review meaning and performance first, then attention, geography, action phase, sound and output.
A clean seam can still make the wrong artistic connection.

- **Listening edit:** cut an exchange on speech, then around the listener's thought with J/L
  overlap. Compare the relationship conveyed while preserving visible speech synchronization.
- **Transition study:** join two meaningful moments with a straight cut, graphic match and
  dissolve. Explain the different time/idea reading and inspect the required handles/composite.
- **Seam study:** make a short occlusion or whip join with a clear destination. Inspect the
  exported seam and the landing before adding more elaborate choreography.

Use [VALIDATION.md](VALIDATION.md) to label evidence. Approve the exported sequence and mix; a
valid edit point, attractive still or seamless transition cannot alone prove a well-directed scene.
