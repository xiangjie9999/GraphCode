# Hair, clothing and parts that swing

[Animation guide](ANIMATION.md) · this page: hair, clothing and parts that swing on an animated character (moved here
verbatim from ANIMATION.md on 2026-10-13, when that page passed its length; its old heading there points here).

A `.gcmove` animates the body. Hair and clothing are layers worn on that body, each moved by its own
system; you author their shape and how they are worn, then judge them on the moving character. What
moves each one, and what is and is not built:

| Layer | What moves it | What you author | How to see it moving |
|---|---|---|---|
| Hair, fur, beards (Groom) | Unreal's groom physics on the hair's guides, at runtime | The style in a `.gcgroom` ([GROOM.md](GROOM.md)); its `settle()` is a styling step at build time, not motion. The binding goes on the character: `outfit { Hair = <the binding> }` | `gc_render kind:"scene"` on the Groom with `animation:` a clip for its target's skeleton: `at:` poses one moment; `simulate:true` plays from zero under the hair's own physics, colliding with the target's physics asset. On a MetaHuman the clip plays on the body and the head and what it wears follow. Which groups simulate, and their drag, is the Groom's `physics` block (every group starts off) |
| Clothing that follows the body | The skeleton: a skinned garment takes the body's bone weights | A `.gcgarment` built `[as=skeletal]`, or a render-only MetaHuman outfit built `[as=outfit]` ([GARMENT.md](GARMENT.md)) | Play the body's clips on the dressed character; pose the extremes GARMENT.md lists (arms raised, forward and crossed, torso bent and twisted, walk, run, attack) |
| Rigid parts on skinned clothing (plates, buckles) | The part's weights carry the cloth under it; the weight blend fades across a spatial border `hold=` centimetres wide | `add($Parts.Plate, at=..., hold=6cm)` in the `.gcgarment` | As above; plates must not cut each other at shoulders, elbows, hips and knees |
| Cloth that drapes or swings (cape, skirt hem, tabard) | Chaos cloth simulation | A `.gcgarment` built `[as=cloth]`, with `hang(from=$Ring)` below `construct()` to define its held region. This requires the optional ChaosClothAsset plugin; on a MetaHuman it creates a simulated outfit | Play the body's clips with cloth simulation enabled; inspect the loose edges, settling and body/self-contact throughout the motion |
| A swinging bone chain (tail, ear, trunk, a ponytail bone) | `[dynamic=]` on a `.gcrig` chain is read but **reaches no asset yet** | Today: key the follow-through yourself in the `.gcmove`, lagging its driver, or use Unreal's own secondary-motion tools in the Animation Blueprint | Render the clip |

Garment's `settle(time=...)` runs Chaos during the build to author a hanging pose, including tied
tails. Runtime cloth motion is selected separately by `[as=cloth]`; `[as=skeletal]` and `[as=outfit]`
follow the bones. The part's `hold=` describes distance across the cloth, not an easing duration.

Two lines of the `.gcmove` serve both simulations. `Breeze = wind(toward=+X, speed=3, gust=30%)`, used on
a clip as `$Breeze@$Profile`, is the air the clip is played in (metres a second; the level supplies its own
in game), and `gc_render simulate:true` blows it on the hair and the cloth. A build line's `[settle=1]` holds
the clip's first pose for that many seconds before frame 0, so hair and cloth start hanging rather than
falling. Neither changes the clip's frames.

**Live or baked: the author decides, per clip.** With no word, cloth simulates live: it answers any
animation, wind or collision in the game, and costs time every frame. A build line's `[bake]` runs the
character's cloth once over that clip, after its `[settle=]` and in its `wind()`, and writes beside it:

- `<clip>_<component>`, a **Geometry Cache of each drawn garment** — what you play, with no cloth and no
  solver: in your own Blueprint (below), or on a Sequencer Geometry Cache track beside the clip.
- `<clip>_Cloth`, a Chaos cache collection of the simulation itself, for Unreal's own Chaos Cache Manager
  and its Sequencer track.

#### Play a baked clip

The bake does not write a Blueprint: you write the one your game needs — an emote on a button, an idle
loop placed in a level, a character that switches between live and baked. Its build note gives every value
below (`parent:`, root, body, clip, and each cloth with its cache), named as a graph reads them. The four
things that make it right, and why:

- **The cache hangs on the character's root at zero offset.** The bake records the drawn garment relative
  to the actor, not to the body or a bone.
- **The actor ticks after the body** (`AddTickPrerequisiteComponent(Body)`), so the time you read is this
  frame's pose; without it the garment can trail the body by a frame.
- **The live cloth is hidden, stops ticking and has its simulation switched off**, so nothing draws it and nothing
  solves it. It is still registered, so it keeps its memory; only the per-frame cost goes.
- **The time comes from the body** (`GetPosition`, which needs the body playing the clip itself, not an
  Animation Blueprint), set every frame on a cache with `bManualTick=true`. The cache component's own
  `bLooping` must match the clip: it defaults to true, and a play-once clip that leaves it true has its
  garment's bounds taken from a wrapped frame.

A subclass of the character, playing the clip from BeginPlay (seen working, 2026-10-06, against the live
skirt at three moments):

```gc
gc 1  kind:blueprint  asset:/Game/Characters/BP_DancerCurtsy
parent: /Game/Characters/BP_Dancer.BP_Dancer_C

class_defaults {
  PrimaryActorTick.bCanEverTick = true
}

components {
  SkirtBaked = GeometryCacheComponent(bLooping=false, bManualTick=true, GeometryCache=obj'/Game/Characters/A_Curtsy_Skirt.A_Curtsy_Skirt')
    @attach[Parent=Root]
}

graph {
  eventgraph EventGraph {
    n1 = Event ReceiveBeginPlay() [bOverrideFunction=True] : ->n2
    n2 = CallParentFunction /Game/Characters/BP_Dancer.BP_Dancer_C:ReceiveBeginPlay() : ->n3
    n3 = CallFunction Actor:AddTickPrerequisiteComponent(PrerequisiteComponent=n4.Body) : ->n5
    n4 = VariableGet Body()
    n5 = CallFunction SkeletalMeshComponent:PlayAnimation(self=n4.Body, NewAnimToPlay=obj'/Game/Characters/A_Curtsy.A_Curtsy') : ->n6
    n6 = CallFunction SceneComponent:SetVisibility(self=n7.Skirt) : ->n8
    n7 = VariableGet Skirt()
    n8 = CallFunction ActorComponent:SetComponentTickEnabled(self=n7.Skirt) : ->n9
    n9 = CallFunction ChaosClothComponent:SetEnableSimulation(self=n7.Skirt) : ->_
    n10 = Event ReceiveTick() [bOverrideFunction=True] : ->n11
    n11 = CallParentFunction /Game/Characters/BP_Dancer.BP_Dancer_C:ReceiveTick(DeltaSeconds=n10.DeltaSeconds) : ->n12
    n12 = CallFunction GeometryCacheComponent:TickAtThisTime(self=n13.SkirtBaked, Time=n14.ReturnValue, bInIsRunning=true) : ->_
    n13 = VariableGet SkirtBaked()
    n14 = CallFunction SkeletalMeshComponent:GetPosition(self=n4.Body)
  }
}
```

One cache component, one hide-stop-switch-off and one `TickAtThisTime` per baked garment. For a `[loop]`
clip set the component's `bLooping=true`, and add `bLooping=true` to `PlayAnimation` and `bInIsLooping=true`
to `TickAtThisTime`; the cloth's last frame does not meet its first, so a loop shows a seam where it wraps. Create it with `gc_apply create:true`, then
render it beside the live character at the same moments with `simulate:true` before you call it done.

The build says what it recorded and checks that the cache replays the recording (on the skirt fixture, to
0.001 cm); a frame of the live simulation cost about 10 ms in the editor's preview world, where the Geometry
Cache plays with no solver at all — measure your own target before quoting a number.
A bake replays against a clock, not a blend: use it for a clip that plays as it is — a cutscene, an
idle loop, an emote, a menu turn. A character that blends clips in play wears its cloth live.

```gcmove
build {
  A_Walk = $Walk [loop]                       # live: the cloth simulates in the game
  A_Curtsy = $Curtsy [settle=1] [bake]        # baked: the skirt recorded once, played back with the clip
}
```

#### Baked hair

`[bake]` records the hair too: each Groom component of the character that simulates is recorded over the clip into a
Guide Groom Cache `<clip>_<component>`, and the note names it as `<variable> -> obj'<cache>'`. A Guide cache plays only on
a Groom asset that allows it, so the bake turns on that asset's **Enable Guide-Cache Support** and saves it, and says so.

A hairstyle has to simulate to be baked. Its physics left at the defaults does not — the engine's `EnableSimulation` is
off — and the hair then only follows the head through its binding; the bake says "no Hair Group simulates at LOD 0" and
records nothing. Turn it on in the Groom's `.gc`:

```gc
physics {
  Hair = group(SolverSettings=(EnableSimulation=True))
}
```

Playing it takes two things beside the cloth's:

- **The Groom's template keeps no cache and has its simulation switched off**: `SimulationSettings.bOverrideSettings=true`
  (with the override on and nothing else set, the component's own setting — simulation off — applies), and `bLooping` the
  clip's.
- **At BeginPlay, after the clip starts, `SetGroomCache(<cache>)`.** Assigning a cache starts its time at zero; from there it
  advances with each frame, as the body's clip does, so the two stay in step for a clip played from its start. A Groom
  cache's time cannot be set from a Blueprint, so a clip that seeks, pauses or changes rate plays its hair from a Level
  Sequence with a Groom Cache track (author that in Sequencer; GraphCode does not write one yet).

```gc
components {
  Hair = GroomComponent(bLooping=false, SimulationSettings.bOverrideSettings=true)
}
graph {
  eventgraph EventGraph {
    // ... after PlayAnimation and the cloth's lines:
    n10 = CallFunction GroomComponent:SetGroomCache(self=n11.Hair, InGroomCache=obj'/Game/Characters/A_Curtsy_Hair.A_Curtsy_Hair') : ->_
    n11 = VariableGet Hair()
  }
}
```

Two things about previewing it with `gc_render simulate:true`. Its `[settle=]` hold pauses the clip but not a Groom Cache, so
a baked clip with `[settle=]` shows its hair a second ahead: preview with a copy of the clip built without `[settle=]` (the
same motion), or in Play-In-Editor. And live, simulating hair in that preview can be caught unplaced at a moment (a bald
frame) — the preview ticks many times between pictures and hair solves only when drawn; the bake itself draws every step.
Seen working 2026-10-06: a 2 s jump, the cached hair on the head and moving with it at three moments, against live.

#### Speech

A MetaHuman says a line from a sound: `speech=` on a clip solves the face from the SoundWave with Epic's MetaHuman Animator
(offline, in the editor, when you build; it needs GraphCode Advanced and the `MetaHuman` plugin, which MetaHumanCharacter
already brings). The file's `part:` is the character's **Face**:

```gcmove
gcmove 1 character:/Game/Characters/BP_Ana part:Face into:/Game/Characters/Ana/Speech fps:30
build {
  A_Hello_Face = $Hello
}
Hello = clip(speech=/Game/VO/S_Hello) {      # face=mouth keeps it to the mouth; full is the default
}
```

What you get is an ordinary AnimSequence of the face's control curves, as long as the sound. The solve runs at 30 frames a
second and ends up to a frame short of the sound; the clip keeps the solve's own seconds and holds the last pose to the
sound's end rather than stretching it, and the build checks every curve against the solve at every frame and says so. Head
motion is left to the body. The sound decides the length, so `frames=`, `repeat=` and `[loop]` are refused on it.

Playing it from the start, in a subclass of the character: at BeginPlay, `PlayAnimation` the clip on the **Face** — the
Face's post-process Anim Blueprint still runs, so RigLogic moves the face from the clip's curves — and `SpawnSoundAttached`
the sound to the Face:

```gc
n3 = CallFunction SkeletalMeshComponent:PlayAnimation(self=n4.Face, NewAnimToPlay=obj'/Game/Characters/Ana/Speech/A_Hello_Face.A_Hello_Face') : ->n5
n4 = VariableGet Face()
n5 = CallFunction GameplayStatics:SpawnSoundAttached(Sound=obj'/Game/VO/S_Hello.S_Hello', AttachToComponent=n4.Face) : ->_
```

The face and the sound start together but run on two clocks; for a cutscene that must stay locked, seeks or pauses, put the
body clip, the face clip and the sound on one Level Sequence. When you preview with `gc_render simulate:true`, a body clip
with `[settle=]` is held for its settle while the face runs on — stand the body on a clip without one.
Seen working 2026-10-06: a 5.7 s line on a MetaHuman, the mouth open on the vowels and closed on the "m" at their times
(251 face curves, every frame equal to the solve). Not yet measured: how far the sound and the face drift apart in a game.

**Work in this order.** Judge the body's motion first, with hair and clothing hidden, so they do not
hide a bad pose. Then clothing: right at rest, then skinned, then the extreme poses. Then hair: the
style at rest, the binding on the character, then a render with `animation:` and `simulate:true`.
Last, the whole character together at its real speed. When the body's timing changes, look at the hair
again: its motion follows the body's. Check simulated clothing in the fastest motion the character will
make (a jump, a landing, a spin), as [GARMENT.md](GARMENT.md#check-simulated-cloth-in-the-fastest-motion-it-will-wear)
describes; judge a limb against the cloth from a section through it, not from a low view.

**What Move does not do with them.** Move's collision checks the body's physics-asset bodies and
static-mesh props (`surface(part=)`); it does not see hair strands or garment cloth. A hand that
touches hair or pulls a collar is not built. Do not stand a prop in for the hair to make one look built.
