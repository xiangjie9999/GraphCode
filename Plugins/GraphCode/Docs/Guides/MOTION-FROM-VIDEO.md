# Motion from video: measure a clip, then use it like any other

Video is another source of animation, the way motion capture is. A video of a rigged character is measured frame by
frame (where every joint is, how every bone is turned) and the result becomes an ordinary AnimSequence on the
character's own skeleton. Authoring does not change: a `.gcmove` uses the clip as it uses any built clip, or the clip
plays as it is. Still pictures of key poses are read the same way, one picture a key ([below](#from-still-pictures-key-poses-an-image-model-made)).
Read [GCMOVE.md](GCMOVE.md) first; this page is one row of it.

**Status in this release.** The GraphCode side ships: drawing every bone on a render, exporting a clip as glTF,
importing a glTF clip onto a skeleton, and using the imported clip. The measuring scripts (finding the joints in the
frames, rebuilding them in 3D, fitting the rig) are **not in this release**. They are being proven in GraphCode's own
repository on one character, a cat's walk that GraphCode rendered itself, so the true motion is known, then on the same
walk made to look like a generated video (graded colour, new background, softness, compressed). Pictures from an image
model (Codex's, of the cat) have been read into key poses; no AI-generated video has been measured yet. The still-picture route
derives its joints, feet and pairs from any character's own files (see "Pictures to animation in four commands"). The measurements and the steps to
run them: `Docs/GC-MOVE-VIDEO-CAPTURE-PLAN.md`.

**When to use which (user 2026-10-26: "simple movement, use normal approach, more complex ones use video or image based").** Pictures
and video are main ways to make a motion, beside authoring: author a simple movement (the words say it exactly); take a complex POSE
from still pictures (a sit, a stretch, a crouch: the whole body at once, GraphCode drawing the motion between); take a complex MOTION
from video (when the path and timing between poses are the point). The table and the rule for asking the user: [GCMOVE.md](GCMOVE.md).

## The rules

1. **Measured motion enters as an asset, never as numbers typed into a `.gcmove`.** Never compute a place, a facing or
   a joint angle outside the tool and paste it in ([GCMOVE.md](GCMOVE.md)). A take is imported as a clip on the
   skeleton, like a motion-capture take.
2. **A take of another body carries its path and its contacts, not its angles.** Where the body is (height, pitch) and
   when and where each foot touches (as a share of leg length) carry over, and the legs are solved to reach them. A real
   cat's joint angles put on a sculpt with other proportions curled its legs and dropped its chest 7 cm. A take of the
   character itself (a video made from its own render) applies as it is.
3. **Takes that are not one motion are lined up by phase, then combined.** Two generated videos of "a cat walking" are
   two walks, not one walk seen twice. Each is cut into cycles at its footfalls, each cycle rescaled to one length, the
   takes lined up by the gait and combined phase by phase. Several views of ONE recording are triangulated frame by frame
   instead.
4. **Prove each step against a known answer before the next.** Start from a video of a clip you already have, so every
   number can be checked against the truth.

## How it works

Two ways read the joints out of the pictures, and they can be combined:

- **Drawn bones (rings).** The video shows a coloured ball on every joint, and each ball is followed by its colour. Exact
  on our own renders, but a video model does not keep the drawing: it shifts the colours, softens the balls and loses
  the rays, and the rings are then lost or confused.
- **The character itself (a joint reader).** A small model reads each joint off the character's own look. It is built on
  DINOv2 (Meta's image describer, Apache-2.0), which describes every patch of a picture the same way across frames and
  lighting, and is taught on the character's OWN renders, where every joint is known, under the changes a video model
  makes (colour, light, background, softness, compression). It needs no drawing, so the video can be made from a plain
  frame. It is taught once per character, and again whenever its rig moves a joint point: it learns where each joint sits in
  the character's look, so after a joint moves it still reads the old place (a cat's left hind paw joint moved 1 cm: the
  reader then put that ankle 68 px off on pictures of the fixed rig, 10 px on the old).
- **A take stands on its planted paws.** A fit from video leaves the body a little high or low (a cat walk whose paws were on the
  floor came back with every paw 0.8-1.0 cm up in stance), and a plant holds its paw ON the floor, so each leg was pressed that far
  into its drawing and sprang back as the paw rolled (a wrist 25 degrees in one frame). Each take is lowered by the smallest of its
  paws' median heights in stance before it is keyed (2026-10-30): its worst one-frame snaps 32.8 -> 26.2 and 27.7 -> 12.3 degrees,
  1.88 cm off the walk it was made from (2.07).
- **Both.** For a video made from a frame with the bones drawn, a ring counts where the reader agrees with it; where the
  video garbled a ring, the reader and the rig outvote it.

1. **The first frame is known.** For a generated video, render the frame to give the video model: plain and lit for the
   reader, every bone drawn for rings or both. `gc_render` with `highlights:[{type:"bones", of:[<bones or rig chains>],
   draw:["ray"]}]` draws a coloured ball at each joint and a pyramid toward its child; its manifest names each joint's
   place, so a tiny second view with the bones drawn gives the places of a plain frame. Keep the camera still; the first
   frame's pose and the camera are then known.
2. **Find each joint in 2D, with the rig in the loop.** Each frame, the rig carried on from the frames before says
   where each joint should be. Rings: each ball is looked for there by its colour and size, and named by the rays that
   meet it (many bones share a colour). Reader: each joint takes its strongest nearby peak (the reader can mix up a left
   and a right paw that look alike; the rig knows which is which). A point the fitted rig cannot reach is dropped, and
   the clip is tracked forward and backward.
3. **Rebuild 3D.** Two cameras watching one motion triangulate each joint. One camera cannot see depth: the bone lengths
   and the rig's joints carry it, much less well.
4. **Fit the rig.** Each bone's turn is solved from where it and its child are, inside the rig's own free axes and
   limits (the IK rig names them). A twist that the joints' places cannot show (a forearm's) is read off the drawn
   pyramid's pale and dark sides, on sharp frames only. Twist helper bones are set from their drivers, as the rig drives
   them. The travel is read off the planted feet: a foot on the ground does not move, so in a picture played in place it
   slides back at the body's speed.
5. **Import it as a clip.** The fitted frames are written into a copy of the character's own glTF export
   (`gc_asset mode:"export"` of any of its clips to a `.gltf` gives the same bones and the same rest pose) and imported
   with `gc_import` onto the character's skeleton. Plan the import, then in the plan's document import only animations,
   name the skeleton, and turn materials and textures off. **Import under a new name:** an import over an existing clip
   stops on a modal "override asset?" question that holds the editor until someone answers it. The import does not carry
   root motion or looping: set `bEnableRootMotion` and `bLoop` under `settings` in the clip's `.gc` as the source has
   them, and apply it.
6. **Check it against the original.** Render the imported clip from the same cameras and compare it with the original,
   side by side and overlaid.

## How close it gets (the cat walk)

| Video | Read by | Joints off the truth, mean / worst | Bone turns off, typical worst | Pixels that differ from the original |
| --- | --- | --- | --- | --- |
| One motion seen from two cameras, every bone drawn | rings | 0.1 cm / 3.4 cm | 2 degrees | 1-2 % |
| The same, made to look generated (colour, background, softness, compression) | rings | 0.9 cm / 21 cm | 4 degrees, one forearm 72 | |
| The same | both | 0.2 cm / 2 cm | 3.5 degrees | 1-3 % |
| Two cameras, no bones drawn, made to look generated | the reader | 0.4 cm / 2.6 cm | 10 degrees, the forearm twist worst | 3-6 % |
| One camera, no bones drawn, made to look generated | the reader | 2 cm / 15 cm | 20 degrees | |
| Four separate videos, one camera each, blurred and compressed, combined | rings | 0.6 cm / 10 cm | 9-10 degrees | 4-11 % |

Every take imported walking, with root motion and looping on. Still weak: with one camera per video, the far forepaw
mid-swing ends 5-15 cm off; a twist read off a soft frame misleads; one camera lets the tracker follow the wrong ball
of a similar colour for several frames, the rig bending in depth to reach it; and rings alone do not survive what a video
model does to colour, which is what the reader is for. Read with both where the video keeps some of the drawing.

## The joint reader is a separate, optional download

The joint reader runs on its own Python with PyTorch, on a PC with an NVIDIA graphics card. It is not in the plugin package (it is
several gigabytes); the Train window's **Set up...** downloads and installs it once per PC, after saying what it fetches and asking
(`picture_route.py setup`: a virtual environment made by the editor's own Python in `%LOCALAPPDATA%/GraphCode/PictureRoute/venv`,
the DINOv2 cache beside it in `torchhub`). It is needed only to read videos with no bones drawn (or both), and to read pictures:

| What | Where it comes from | Size |
|---|---|---|
| Python 3.12 | python.org, or `uv python install 3.12` | about 70 MB |
| PyTorch with CUDA, NumPy, SciPy, OpenCV, Pillow | `pip install torch torchvision --index-url https://download.pytorch.org/whl/cu128`, then the others from PyPI | about 4.5 GB, almost all PyTorch |
| DINOv2 (Meta, Apache-2.0), with registers, Base | downloaded by PyTorch on first use (`torch.hub`, `facebookresearch/dinov2`) | 346 MB (Small: 88 MB) |
| The reader taught for your character | made on your PC from your character's own renders | 9-120 MB |

Keep it in its own environment, away from the Python the editor uses. DINOv3 (2025) would also work but comes under Meta's
own licence and a sign-up; DINOv2's Apache-2.0 needs only its notice kept.

## What to ask a video model for

- **One motion seen from two cameras**, where the tool can make it. That is the biggest difference in the table above.
- Otherwise several takes, each from its own camera, to be combined.
- Start from a frame GraphCode rendered: plain and lit for the reader, or with the bones drawn for rings or both.
- Keep the camera locked off (no pan, zoom or cut), the same shape as the first frame (16:9, not cropped), and the
  frames sharp. The character may move across the picture; its travel is measured.

## Using the clip

- Play it as it is, through an AnimBP, a montage or Sequencer.
- **Or key a `.gcmove` from it, and let GraphCode finish it.** `pose(source=<the take>, at=frame(7))` is the take's pose at
  that moment (`at=` takes a fraction of the clip, `frame(n)` or seconds, `0.9s`; without it, the clip's last frame), and
  `bones=` takes only some bones (`gc_describe kind:"move" block:"poses"`). Keyed this way, a take gets everything a
  `.gcmove` key gets: each foot down at that moment planted (held on its print, rolling, no slide), the body carried by
  the planted feet at a `travel(root, velocity=)` speed, the rig's joint limits held between keys, and, on a character
  with hull bodies (`physics_asset() [bodies=hull]` in its rig's build), `collision(self)`; the floor is the plants'
  (a paw's hull stands proud of its skin, so a floor listed in `collision()` meets every planted paw). `balance()` suits a
  take of a stance, a reach or a landing; a walk is held up by moving, and balance refuses the moments only two feet on
  one side are down. Take the take in place (its root at rest) as the source of keys: the travel moves the body.
- **The moments come from the take, not from typing.** The take's report says when each foot lands, which feet are down
  then (about to lift first, the one carrying the body last), and the travel speed that keeps its timing. Key the
  landings and GraphCode draws the motion between (cleaner and editable; the swing paths are GraphCode's), or key a
  moment every few frames and the take's own motion is kept, its contacts held by GraphCode.
- Measured on the cat (a take read off an AI-like video with no bones drawn): four keys at the landings held three paws
  exactly on their prints and the fourth within 1.1 cm (the take's left elbow arrives at its stop, and the build names
  it), the walk 1.3 cm from the original on average (the motion between keys is GraphCode's); a key every two frames
  stayed 0.7 cm from it with every paw held. The take's paws slid 2.1-3.0 cm while down, the keyed walk's 1.1-1.6, as
  the original walk's own rolling paws do.

## Pictures to animation in four commands (any character)

The still-picture route below is **not tied to the cat**. It reads the character's own GraphCode files, so a character with a
`.gcrig` (its feet declared as `contacts`), an IK rig and a skeletal mesh runs it without editing a constant. One script,
`picture_route.py` (GraphCode Move ships it in `Resources/PictureRoute`; a project's own `Tools/VideoCapture` wins), prints the next command after every step; the ones that need the editor or the graphics
card run only with `--run`.

```
python picture_route.py init  MyHero --gcrig /Game/.../MyHero --ikrig /Game/.../IKR_MyHero --mesh /Game/.../SK_MyHero
          [--character-bp /Game/.../BP_Stage --part Body --worn SkeletalMesh --physics-asset /Game/...]   # or --from-gcmove <a .gcmove of it>
python picture_route.py check MyHero                  # what is missing, and the command that fixes it
python picture_route.py template   MyHero /Game/.../AnyClip --run          # the rig template (no older export needed)
python picture_route.py references MyHero /Game/.../AnyClip --run          # size, framing and the two pictures for the image model
python picture_route.py teach MyHero /Game/.../ClipA /Game/.../ClipB --run  # once per character: poses, renders, the reader
python picture_route.py read  MyHero <stills_dir>                           # your pictures -> joints
python picture_route.py fit   MyHero <stills_dir> /Game/.../A_NewName       # rig fitted to each picture (offline)
python picture_route.py build MyHero <stills_dir> /Game/.../A_NewName out.gcmove --run   # new clip + the .gcmove
```

Run these from PowerShell (Git Bash rewrites a leading `/Game/` into its own install folder).

The four commands you live in are `check`, `references`, `read` and `build` (`init`, `template` and `teach` run once per character).
What is **derived, not typed**, from the character's `.gcrig` (parent files followed) and IK rig: the joints the reader reads
(every chain's bones, no twist or corrective helpers, each hand's first and last finger base); the planted legs (every chain the rig
declares `contacts` for: two for a person, four for a cat) and the pads each rolls on; left/right from the `_l/_r` or `_L/_R` names;
the hips and shoulders (the mid-plane) and the chest from the chains; whether a hidden leg copies its partner (four legs only);
head levelling (only where a skin file names what to level by; off otherwise); the floor is fitted from **one or more** feet down;
the `.gcmove` header from what you gave `init`; the reader's path (`models/<name>.pt`) and its joint count; the cameras and the
random-camera framing from a render of the character's own bounds (`references`). A value the rig cannot give goes in an optional
`characters/<name>.override.json` (the shipped cat's holds its reader's joint order and its tuning); a character without one is fully derived.

**Where things are:** `picture_route.py` and the scripts it runs ship with GraphCode Move (`Resources/PictureRoute`); run from a
shell, give `--project <the folder of the .uproject>` (the editor always does). The joint reader's Python (PyTorch, CUDA, DINOv2) is
installed per PC by `setup` (above); each character's taught reader is the project's own, `Saved/GraphCode/PictureRoute/models/<name>.pt`,
and the characters added with `init` are in `Saved/Research/VideoCapture/characters`. The cat is the only character measured end to end; the clothed MetaHuman is the next (derived, offline-checked,
not yet taught or run through the editor).

### In the editor: the Train window and the Pose tab

The same route runs from the editor, as external processes (nothing runs inside the editor). The steps that need only the
standard library (`list`, `init`, `setup`) run on the editor's own Python (`Engine/Binaries/ThirdParty/Python3`), so the characters
show and one can be added before anything is installed. The reader's steps run on the reader's Python: `GRAPHCODE_PICTURE_PYTHON` as
the editor started with it, else the Train window's **Choose...** setting (it takes effect at the next run: each process is given it
on its command line), else the one **Set up...** makes; when it is missing, both places say so and offer Set up.... Nothing is
downloaded silently: Set up... says what it fetches and asks first, and when the DINOv2 backbone is not in the reader's torch hub
cache (its code and `dinov2_vitb14_reg4_pretrain.pth`, 346 MB), the route stops and says so, and the Train window says so before Start.

- **Add** (beside Refresh): the character selected in the Content Browser. Select its skeletal mesh and a few of its animations;
  its IK rig and its control rig with a `.gcrig` are found in the mesh's folder when there is one of each, else select them too.
  The IK rig needs its GraphCode text (`<IK rig>.gc`, from gc_sync). It runs `init` and lists the animations as the motions its
  practice pictures are made from (`teach/<name>/assets.txt`); with only animations selected, they are added to the chosen character.
  Its first **Start** reads its rig (`template`) and frames it (`references`) from its first motion before the pictures.

- **GraphCode Train** (the Pose tab's **Train...**, or `GraphCode.Move.Train <character>`): the characters `picture_route.py list`
  finds, each taught or not (its reader on disk) with its last result; **Pictures** (an even spread of the rendered teach
  pictures, the full set by default) and **Passes**, each with a time estimate from the measured 0.013 s a picture a pass;
  **Start** and **Stop**, with progress and the time left read from the route's own lines; the result: the reader on the
  character's held-out pictures (`teach/<name>/heldout`) in px and the worst joints, the pose fitted from two held-out
  pictures in cm (camera withheld), and a few held-out pictures with the reader's points and the fitted rig drawn, beside the
  result before it on the same held-out pictures. Closing the window does not stop a run: it goes on, a notice says so, and
  Train... opens the window onto it again; the editor's exit stops it.
  - A taught character's teach GOES ON from its reader (15 passes by default; 30 from scratch, only when it has none). The
    reader is copied aside as `<name>_before_<time>.pt` before anything runs and stays as it was unless the teach ends well:
    every pass, its file written, and its held-out check run. A Stop, a stop for memory or a teach that wrote no file leaves it
    as it was; a reader taught part way is kept as `<name>_train_stopped.pt`, one taught in full whose check failed as
    `<name>_train_unchecked.pt`, for the person to judge. To start clean, move the reader file aside.
  - Pictures above what is rendered renders the shortfall first, in this editor (its viewport is busy meanwhile): new random
    poses with a seed of their own, imported into a fresh `/Game/__PictureRoute/<name>/r<time>`, and the authored clips the
    first render listed, into a fresh `teach/<name>/data_<time>`, with cameras of a seed of their own, in the look the character's
    reader was taught on: its config's `"render_look"`, `"preview"` (the clip's preview body; the MetaHuman's, whose reader was taught
    on `data` and `data_v`) or `"stage"` (its stage Blueprint, `GC_STAGE`, as `data_stage` was). Nothing earlier is overwritten (an import over a clip
    stops the editor on a modal question). A character never rendered starts at 2,800 pictures, made from the motions Add listed.
- **The Pose tab's row**: **Character** (whose reader reads the pictures), **Retarget...** (opens Unreal's IK Retargeter that
  GraphCode wrote for the file's IK rig, `ik_retargeter(onto=, map=)` in its `.gcrig`; nothing else), **Reference photo** (the
  character rendered for an image model: a person whose config says `"reference_views": "high_three_quarters"`, the MetaHuman,
  from the two high front three-quarter views, about 45 degrees each side with the camera above the chest, since a person's side
  view reads a left joint on the right limb half the time; any other character, the cat among them, from its framing's own two
  views as before; it renders the FILE's character, the one on the clip's skeleton; one with no taught reader says "Pictures aren't
  set up for this character yet" and offers **Set up...**, which opens Train on it), and **Upload pictures...**:
  one or two pictures of one pose -> `read` -> `fit` -> `overlay` -> the fitted pose imported as a clip in this editor -> the
  chosen key written again as preset values by the conversion `[as_presets]` runs (MOVE-PARAMETRIC-POSES.md part 6). What the
  pictures cannot see keeps the key's own value, never zero: a bone with no read joint below it keeps its whole turn, and a
  bone whose read joints lie down one child (an upper arm, a thigh) keeps its roll about that child. The line says which bones
  the pictures set and which kept their own values. The fit drawn over each picture shows under the row: it is a TRIAL to judge
  (a person fits to about 7.5 cm mean), and Apply builds it, Save writes it, Undo takes it back. Its plants, reaches and
  balance stay, and a limb they hold stays theirs. Each key has ONE scratch clip (`/Game/__GcScratch/PictureUpload/A_Picture_<file>_<key>`):
  a later upload for the same key reimports it from the key's own source file, so nothing piles up and nothing is deleted. The
  key is the one chosen when the upload starts (another chosen meanwhile: nothing is written), an upload that sets no bone writes
  nothing and says why, and **Stop** ends an upload or a reference photo.

The route steps the editor uses, for the command line too: `list`, `cuda`, `teach <name> --steps reader [--pictures N] [--data
<dir>,<dir>] [--out <model.pt>] [--test <heldout dir>] [--init <model.pt>]`, `teach <name> <clips> --steps render --tag <time>
--count N --times T --run`, `heldout <name> <dir> [--reader] [--fits] [--pictures]`, `overlay <name> <stills_dir>`,
`references <name> <clip> --run --keep-framing`.

## From still pictures: key poses an image model made

A video is not the only source. An image model (or an artist) can make the KEY poses as still pictures, one picture a key: a
crouch, a landing, the four contacts of a walk. Each picture (or two of the same pose from two sides) becomes one pose of the
character's own skeleton; GraphCode draws everything between them, exactly as for keys taken from a video. The in-betweens are never
the image model's.

- **What the author gives:** the pictures, in order, and for each the paws (feet) that are down in it; then either a travel speed (a
  walk, a run: the keys are timed by `travel(root, velocity=)`, the paws written about-to-lift first and the one staying down longest
  last) or each picture's moment in seconds (anything else: each key takes over from the one before over the time between their
  moments, the per-still form of [POSES.md](POSES.md#travel-between-two-built-poses); a paw down in every picture is planted, and one
  whose print moves between two pictures slides along the floor from the one to the other, held on it all the way, at the pace the
  pictures move it: a sitting cat's hind paws come forward, a stretching cat's forepaws reach out. While it slides, the body stands as
  high as its planted legs carry it, so a leg is never stretched past its pictures' drawing to reach the floor. The pictures show the paw down at both ends and
  nothing lifting it, so GraphCode does not invent a step; to step a paw over, give it a picture with the paw lifted, or write the step
  as a key or a reach. A paw the drawing between them lifts clearly (a walking toe's clearance over where it stands at the two pictures)
  keeps that drawn swing).
- **No camera is needed.** A picture from an image model has no camera to read, so each is fitted with its own: a level lens (a
  picture's horizon has no roll) tipped down by a pitch found with the pose, the character's root standing upright on the floor and
  leaning by its own joints within the rig's limits, the paws said to be down held on one floor. One picture cannot see how far the body
  or a leg leans toward or away from the lens: from one picture those stay near the character's rest (each leg under its hip or
  shoulder), and a hidden leg in a four-paw stance copies its partner mirrored: when no picture reads it surely (a sitting cat's far
  hind leg, hidden in every picture taken from one side), it is posed after the fit, its own joints alone, onto the seen leg mirrored
  across the upright plane through the middle of the hips or shoulders, the seen leg untouched (2026-10-29). The copy is only as good
  as the rig's symmetry: a joint point placed off its partner's mirror turns the copied paw (the cat's left hind paw point lay 31
  degrees off and its copied sole 38 off flat). **Two pictures of a pose from two sides settle it**: they
  are fitted as one pose, each with its own camera. For a stance they are needed, not a refinement (measured 2026-10-29, a cat looking
  around on four planted paws, three key poses): from one picture each, the hind paws came out 6-10 cm apart in depth from key to key
  and the move slid them where the cat kept them still, 2.15 cm mean off the original; from two, 1.31 cm, the paws within 1.1 cm and
  no joint snapping more than the original does. In a stance each paw said to be down stands its lowest contact (heel, pad or toes,
  as the rig's `contacts` name them) on the floor; a key that the pictures put inside the body is pushed out of it before the move is
  made, and a key clear of the body is used as fitted. The focal length is not measured (it barely shows at a character's distance) and
  is not needed. Each picture's camera is solved and kept with the clip, so the built move can be rendered from it beside the picture.
- **The pictures give the whole pose; what they cannot show takes the natural pose; the author writes the detail** (user, 2026-10-27:
  "macro and micro"). The joint reader marks where joints are, not which way a bone is turned along its length, so a turn no picture
  shows (a forearm's or a thigh's twist) stays at the character's rest. For a person the reader reads two knuckles (index and pinky)
  beside the wrist, so the fit does turn the palm, but **from pictures the forearm's roll is a guess: an author states it or takes the
  preset's** (measured 2026-11-26 on 17 held-out pairs of our MetaHuman: the palm's turn about the forearm 13-21 degrees off at the median,
  up to 70; the knuckle points land 5-6 cm off for knuckles 7 cm apart). **Pictures do not tell a closed hand either: say it** in
  `stills.json`, `"hands": "fist"` (or `"fist_left"`, `"fist_right"`), and the route writes the hand's body word over that key
  (`$Hand(curl=1)`, with the presets line it needs). A paw said to be down stands flat, as `plant(leg, on=$Floor)`
  stands it, and the `.gcmove` plants it so (`roll=` keeps the drawn turn instead: a walk's rolling paws). Where the built move still
  reads wrong (a paw, where the head looks), edit the `.gcmove`'s keys and plants as for any authored move; the pictures stay as they are.
- **The result** is one clip, a frame a picture, and a `.gcmove` keying each frame with `pose(source=<clip>, at=frame(n),
  from=reference)`, its plants and its timing written, and, timed by seconds, `collision(self)`: each key is clear of itself, but the
  blend between two can pass a hip through the spine or an elbow into the chest (Codex's sit and stretch were refused that way). An
  ordinary `.gcmove` to edit, with balance and effort added as for any key.

Measured on the cat (pictures the reader was not taught on, made to look generated, every camera withheld):

| | Joints off the truth, mean / 95 % / worst | |
| --- | --- | --- |
| One picture, the pose alone (21 pictures of a walk) | 1.6 / 4.2 / 16.8 cm | the floor's up 1.9 degrees off on average, 5.9 worst; about 7 s a picture |
| Two pictures of each moment from two sides, fitted together | 0.7 / 1.9 / 7.9 cm | the floor 0.9 degrees, 3.1 worst: as close as two KNOWN cameras (0.68 cm) |
| A cat that looks down then up to its left, from three pictures (three framings), timed by their seconds | 1.1 / 3.1 / 5.3 cm from the original over the whole clip | the head 1.3 cm; each key reached at its moment |
| A walk from four pictures at its landings, two from the side and two from a high three-quarter view | 2.2 / 5.7 / 10.1 cm from the original walk | every paw held on its print within 0.33 cm; two paws dipped 1.0-1.1 cm into the floor for a few frames (the build names them) |
| The same four moments keyed from the video | 1.3 / 4.1 / 13.2 cm | |

Four pictures were read and fitted in about a minute; the walk built in 65 s.

**How many pictures: only the poses the motion really passes through** (2026-10-29, the user on a three-picture sit: *"It still feels
two different speed? Is it intentional?"*). A motion that is one continuous phase (sitting down, standing up, lying down, a turn of the
head) takes its first and last pictures and nothing between: GraphCode's travel between two poses is one rise and one ease. Give a
middle picture only where the body truly takes a shape the ends do not (a stretch's bow, a pounce's gather), and check that it lies
between its neighbours: every joint that turns back at a picture stops there. The image model's middle picture of a sit was a low
crouch (the front down, then back up), and through it the whole body slowed to a third, two motions joined at a stop; a dog's stand to
sit "could not be divided into multiple phases" (8 beagles, Yoshikawa et al. 2023). From its two ends the same sit is one motion.

**What to ask an image model for:** the whole character in the picture, not cropped; the character as GraphCode renders it (give the
model a GraphCode render as its reference: the joint reader was taught on that look); one pose a picture, sharp; any background; and
**each pose from two sides** (the side and a front three-quarter). Pictures an image model draws separately are not one moment seen
twice: Codex's two views of a pose disagreed by 5-14 px once fitted together, the most on a play bow, which the fit absorbs.

Tried with Codex (2026-10-26): from two reference renders it drew a sit-down and a play-bow stretch, each pose from the side and from
three-quarters (12 pictures). The reader found the joints on every one (the rig within 1.7-6.6 px of each picture alone); from one side
view a standing cat's foreleg came out crossed into its chest in depth (the build's self-collision check refused it), which the two
views together put back under the shoulder; a sitting cat's far hind leg is hidden from the side. ~~**Not built yet:**~~ (both BUILT
2026-10-27: the seat's give from the rig's `skin { rump [gives=] }`, a planted leg sliding as a grip's fingers do) the sit (its
fitted lower legs lie inside the haunch, 3.7-4 cm, which no coat gives; the joint reader is weakest on sitting cats) and the play bow
(the cat rig's body pitch and chest stop 30 degrees short of it). Standing, walking and looking poses build.

## Not yet

- The measuring scripts themselves (see the status above). The joint reader also needs its own Python environment with
  PyTorch and a CUDA graphics card, and its taught weights per character.
- A take of another body, retargeted by rule 2: only takes of the character itself have been measured.
- ~~The joint reader for characters other than the cat: each needs its own renders and teaching (`picture_route.py teach`, an hour on one graphics
  card; none taught yet). For PEOPLE, ready-made joint readers already exist and come first (ViTPose and RTMPose are Apache-2.0).~~
  Superseded 18 Nov (the owner: DINO for every character): the clothed MetaHuman is taught (`teach/metahuman`, 4,053 pictures,
  about 13 minutes); on 13 held-out poses from the two high three-quarter views it fits to about 7.5 cm mean, short of 3 cm.
- Cameras solved from the bone lengths when they are not known (a moving camera). Still pictures already are (above); a video's
  camera is still the first frame's.
- Pictures from a real image model: only renders made to look generated have been measured.
- A walk keyed from a take with `collision(self)` builds (the cat, 2026-10-26: joints within 0.02 cm of the same keys
  without it, paws unchanged, the forelimb kept off the chest), but slowly: about 5 minutes for a one-second walk, most
  of it keeping the joints' limits between keys while fitting each added key against the body.
