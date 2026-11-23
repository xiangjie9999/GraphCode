# Build gameplay systems that hold up

Read this when a feature crosses input, player state, AI, UI, saving or networking. Use the
[production guide](PRODUCTION.md) to choose the slice and [tool help](TOOLS.md) to author it.
The patterns below are design checks, not claims that every framework has a GraphCode writer.
Inspect the installed kind, supported blocks and `omits:` before choosing an implementation.

## Describe the player outcome before the graph

Write the trigger, preconditions, state change, feedback, failure and reset in the mechanics note.
For “collect a resource,” answer: how the target is selected, when it is reachable, what happens
when inventory is full, when the resource disappears, what the player sees and hears, and whether
it returns after reload. These answers prevent several assets from implementing different rules.

Make one object work through the whole path, then add a second object with different data. If that
requires copying the first graph and changing constants, reconsider the shared behaviour before
making fifty objects.

## Specify a mechanic so another AI can implement the same game

Extend the project's existing mechanics note with this compact contract, not a new design bible
for every button. [DESIGNING.md](DESIGNING.md) decides the experience; this makes the rule precise.

```text
Name and player purpose:
Input action and target selection:
Legal starting states and preconditions:
State transitions, interruption and simultaneous-event precedence:
Commit point: when the cost/result becomes real:
Costs, effects, limits, cooldowns and units:
Authority/owner and lifetime:
Visible, audible and tactile feedback where supported:
Unavailable/failure reasons and player recovery:
Reset, respawn, travel and save behaviour:
Tunable data, chosen baseline and why:
Expected success, failure, repeat and interruption scenarios:
Observed evidence and remaining questions:
```

Distinguish a design decision from an engine fact and from an untested assumption. "This attack
may be cancelled during wind-up" is a design decision. Whether the current animation path
supports it is a capability question. Whether it feels responsive requires play evidence.

Write states and transitions before accumulating booleans. A charged action might be Ready ->
Charging -> Committed -> Recovery -> Ready, with Cancelled returning from Charging. Specify what
happens on death, stun, pause, target loss and a second press in each state. Resolve competing
terminal events deliberately, such as lethal damage and objective completion in the same update.

## Choose systems by responsibility and the installed project

| Need | Starting point | Decision that still belongs to the game |
|---|---|---|
| Player intent | Existing input actions; Enhanced Input where the project uses it | Press/hold/release, buffering, remapping and contextual priority. |
| Locomotion | Existing Pawn/Character movement and camera | Acceleration, stopping, turning, collision, jump rules and comfort. |
| Abilities/effects | Existing ability architecture or a suitable component/framework | Costs, cooldowns, cancellation, stacking, authority and feedback. |
| Enemy decisions | Existing StateTree or Behavior Tree with navigation/EQS as needed | Goals, perception, telegraphs, commitment and recovery. |
| Repeated item/enemy variation | Shared behaviour with suitable data assets/tables | Which differences are data and which require a new behaviour. |
| Presentation | UI, animation, audio and effects observing gameplay | What is shown when, how quickly, and how interruption is handled. |

Do not add a large framework solely because it is available. Do not reimplement a suitable engine
system solely because one tool's help page is short. Check installed capability, read the narrow
guide, and preserve an architecture that already works.

Epic's [Enhanced Input guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/enhanced-input-in-unreal-engine?application_version=5.7)
describes actions, mapping contexts, modifiers and trigger states. Match the trigger to the
mechanic: a recurring Triggered callback must not accidentally grant a one-time reward every
frame. Define what consumes an input when gameplay and a menu both could respond. Use the
project's engine version and current help for the exact authoring form.

## Make controls feel deliberate

Separate input sampling, gameplay permission, movement/physics and presentation. A delayed visual
can make a correct action feel unresponsive; an immediate animation can hide a rejected action.
Measure input -> accepted action -> visible response in the actual runtime instead of guessing
from a node graph. Use real elapsed time where required and respect the engine's simulation model;
check at differing frame rates so speed, drains and cooldowns do not depend on rendered frames.

Tune related values as a group with a clear question. For movement, inspect acceleration, braking,
turning, collision and the camera together. For a jump, inspect height, airtime, horizontal control,
landing and any deliberate input buffering or brief off-edge allowance. These assists are choices,
not mandatory defaults. For attacks, inspect anticipation, active hit interval and recovery against
range, opponent warnings and camera readability. For vehicles, inspect steering, traction and
camera response at several speeds rather than only at rest.

Keep named timing and distance values with their units. Prefer relationships over repeated magic
numbers, but keep an explicit authoring override when the design needs one. Test the extremes the
game actually supports: low/high speed, thin obstacles, slopes, corners, rapid direction changes,
and interrupted animation. Do not repair a state-ordering bug with an unexplained Delay.

For each important action, deliberately schedule these contributions:

| Phase | Player-facing job | Proof |
|---|---|---|
| Anticipation | Show what is about to happen when the action needs a warning | The cue appears early enough to be used at the real camera distance. |
| Acceptance | Acknowledge a valid input immediately | Rejected and accepted actions are distinguishable. |
| Commitment | Apply the actual cost, hit or transfer once | A valid event changes the owner's state, including under repeat callbacks. |
| Consequence | Communicate magnitude and meaning | World, sound, animation and UI agree with the result. |
| Recovery | Return control or expose the next choice | Cancellation, death, retry and menu transitions leave no stuck state. |

Not every action needs all five phases or a separate animation for each. Choose them for the
intended feel. Stronger shake, more particles and louder sounds cannot substitute for good timing,
readable collision or consistent rules. Preserve accessibility settings through every effect.

## Give each fact an owner and a lifetime

| Question | Design decision to record |
|---|---|
| Who can change health, inventory, quest state or currency? | One authoritative owner for each fact; other systems request a change or observe it. |
| How long must it live? | Per interaction, spawned actor, player life, level, session or durable save. Prove the relevant respawn and travel paths. |
| How does UI learn it changed? | An explicit notification or binding path; rebuild the view from current state when opening it. Avoid a second independent value in the widget. |
| How is content identified later? | A stable project identifier and a defined missing-content policy; do not depend on a transient actor instance surviving a reload. |
| What happens if the request arrives twice? | Reject or safely handle repeats; prevent duplicated rewards and repeated completion. |
| What happens when the owner disappears? | Cancel or finish pending work, remove callbacks and restore input or camera state as appropriate. |

Choose the Unreal class or subsystem after answering those questions. Keep an existing design
that already answers them. Introduce reusable components where responsibilities or reuse justify
them; do not create a framework for an imagined future game.

## Implement the whole interaction

1. Resolve a real target and validate preconditions at the point the state changes. A prompt shown
   earlier is not proof that the object is still present, reachable or available.
2. Apply the gameplay change once through its owner. Make costs and rewards consistent when an
   action fails partway through. Define which changes are reversible and which are committed.
3. Let animation, effects, sound and UI reflect that state. Decide when an animation authorizes a
   hit or transfer, and what interruption means; a montage playing is not proof of damage.
4. Handle cancellation, destruction, death and retry. Verify that a held input, camera lock,
   interaction prompt, timer or looping sound cannot remain stuck after the action ends.
5. Test again from a clean initial state, then repeat without restarting the game. The second run
   catches stale references, duplicate subscriptions and state that never reset.

For input and menus, test every supported device, focus entering and leaving a menu, pause/resume,
and the return to gameplay. For remapping, verify that saved bindings and displayed prompts agree.
Use the project's installed input and UI framework rather than maintaining unrelated bindings.
Read [UI-UX.md](UI-UX.md) for the complete screen/state design and
[UI-TOOLS.md](UI-TOOLS.md) when an external prototype can improve the interface.

## Make AI and traversal observable

For a robot that chooses a destination, detours around a barrier and recovers when its route
changes, use the [AI navigation worked example](AI-NAVIGATION.md). StateTree or an existing
Behavior Tree owns the decision, EQS can choose among destinations, and navigation/path following
owns the route to the selected one.

Check actual collision, agent dimensions, navigation reachability and arrival conditions with the
intended character. A route drawn in an editor does not demonstrate traversal by a moving pawn.
Test an unreachable target, a blocked route, target loss and recovery. Give waiting and failure
states visible evidence so a stationary enemy cannot be mistaken for a successful behaviour tree.

Pair movement and animation checks: starts, stops, turns, slopes, landings, interruptions and the
expected speed range. Inspect deformations on the real mesh. A correct transition variable does
not prove the displayed pose or foot contact.

## Treat persistence as a gameplay feature

Decide what a save contains, when it commits and which version of that data it represents. Store
durable state with a defined way to reconstruct the world. Keep temporary effects and runtime
object references out unless their reconstruction is explicitly designed.

Verify a new profile, a save after a meaningful change, an application restart, a reload and a
second save. Test the missing or unreadable save path on disposable data. Preserve the player's
last usable save on failure according to the project's recovery policy. When save structure
changes, test a copy of a supported older save and define migration or an explicit incompatibility
message. A Play snapshot is test evidence; it does not automatically implement the game's save
format or prove it survives application shutdown.

## If the game is networked

Set authority, ownership and the supported session model before growing the mechanic. Treat client
requests as requests: validate their preconditions on the authoritative side, including reach,
cost, cooldown and current state. Define what is predicted locally and how disagreement is repaired.

Exercise two players competing for the same resource, an observer, the supported join/rejoin path,
disconnect and latency or loss. Check that state and feedback agree after recovery. Include the
intended server configuration in validation; one local player in the editor proves no network
claim. For engine-specific implementation, consult Epic's
[networking overview](https://dev.epicgames.com/documentation/unreal-engine/networking-overview-for-unreal-engine)
for the project's engine version.

## Keep failures useful

A useful failure record says the initial state, actions, expected and observed result, affected
assets and evidence location. Fix the first incorrect state transition, then repeat that path and
its nearest relevant regression. Do not hide the symptom with an extra delay or a copied value
without understanding which owner or lifecycle was wrong.

## Test rules and experience with different evidence

Assert useful invariants: a consumed pickup cannot reward twice, currency cannot go negative
through a purchase, a failed attempt cannot also complete, and a menu cannot leave hidden input
capture behind. Exercise success, rejection, interruption, repetition and restart through the
public player path. Include save and network cases only when the feature promises them.

Then play for feel and comprehension: is the target obvious, is the action responsive, is the
consequence readable, and does the next choice remain interesting? Record actual observations.
A repeatable automated scenario can prove correctness; it cannot declare the mechanic enjoyable.
Use the tuning and playtest method in [DESIGNING.md](DESIGNING.md), the connected
[delivery example](GAME-SLICE-EXAMPLE.md), and the evidence gates in [VALIDATION.md](VALIDATION.md).
