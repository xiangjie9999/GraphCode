# Make a robot choose and recover from a route

Read this when a customer wants a robot, guard, creature or vehicle to move toward a goal and
respond intelligently when the route changes. Start with the [gameplay guide](GAMEPLAY.md) for
state ownership and with the [validation guide](VALIDATION.md) for evidence. This is a worked
design and acceptance recipe; exact GraphCode grammar comes from the installed tool's help.

## The four jobs

| Job | Engine system | What the robot asks it |
|---|---|---|
| Decide | StateTree, or an existing Behavior Tree | Should I travel, wait, choose another target, recover, or report failure? |
| Choose a place, when there are candidates | EQS | Which reachable work point, cover position or waiting spot best fits this goal? |
| Find and follow a route | Navigation mesh, path following and movement | How do I get to the chosen place with this robot's agent size? |
| Notice a changed world | Navigation relevant collision/modifiers, movement result and optionally Perception | Did the path change, did movement fail, or did I observe a new obstacle? |

StateTree does not draw a route around a box. EQS does not steer the robot along one. When the
goal remains valid and another route exists, updated navigation and path following may take
the detour without a new EQS query. Use EQS when the **destination** needs choosing again: for
example, the loading bay is inaccessible but another dock can accept the delivery. Perception
can help a robot notice actors or hazards, but a route blocker still needs navigation relevant
geometry and a movement response. Unreal's [StateTree guide](https://dev.epicgames.com/documentation/unreal-engine/statetree-quick-start-guide),
[EQS guide](https://dev.epicgames.com/documentation/unreal-engine/environment-query-system-user-guide-in-unreal-engine)
and [runtime navigation guide](https://dev.epicgames.com/documentation/unreal-engine/overview-of-how-to-modify-the-navigation-mesh-in-unreal-engine)
describe those engine roles; use the page for the project's engine version.

## A courier robot that detours

The player asks a courier to deliver a package. A movable barrier can block the direct street.
The robot keeps the delivery goal, follows an alternate street if one exists, and makes failure
visible if there is no complete path.

1. **Define the world.** Give the robot a correctly sized navigation agent, matching bounds and
   walkable collision. Build and inspect the baseline navigation. Confirm that start and goal both
   project onto the **same agent's** mesh and that a complete path exists. A mesh for a different
   agent is not evidence for this robot.
2. **Keep the goal separate from the path.** Store the intended delivery destination and task
   outcome with the actor or controller that owns them. A path is temporary. A new path after a
   blockage must still lead to the original delivery target unless the decision system explicitly
   changes it.
3. **Model the decision.** A practical StateTree can have `Idle`, `ChooseDeliveryPoint`, `Travel`,
   `Recover` and `CannotDeliver` states. `Travel` issues a MoveTo request; success commits the
   delivery once. A failed or invalid request enters `Recover`, which checks the goal, waits for
   a navigation update if relevant and makes a bounded retry. If no complete route exists, it
   enters `CannotDeliver` and informs the player. Use an existing Behavior Tree if the project
   already owns this decision there; do not run two competing controllers for one robot.
4. **Add EQS where it improves the choice.** For several possible delivery or waiting points,
   generate candidates, filter out places the robot cannot reach and score the remaining ones
   for the player's goal. Let the StateTree task request the query, use its result as the MoveTo
   destination, and handle an empty result. A fixed destination with two streets needs no EQS.
5. **Make the barrier count.** For a barrier raised during play, choose and verify the project's
   runtime navigation generation mode and a navigation relevant collider or modifier. A static
   baked mesh cannot automatically account for a new runtime blocker. Check that the affected
   tiles actually change; then observe whether the current move replans, fails, or needs an
   explicit new request. Avoid a loop that issues MoveTo every tick.
6. **Show the outcome.** The robot turns and moves around the obstruction, pauses with a clear
   reason, or reports that it cannot deliver. The package cannot be awarded, consumed or dropped
   twice across interruption and retry.

Ask `gc_describe` for the installed `state_tree` and `env_query` kinds, and ask each write tool for
`{ "help": true }`. In the current GraphCode capability export, StateTree belongs to Move and EQS
to Core Extension (`GraphCodeCoreExtension`); both support creation and whole-block writing. Their
documents are snapshots: omitting a state or query option can delete it. Read the current asset,
inspect `omits:`, dry-run the complete change and read back the compiled asset before playtesting.
The [authoring guide](AUTHORING.md) covers the World and Move authoring paths. A compiled StateTree
or EQS asset alone does not prove that a controller invokes it or that a pawn follows its result.

## Test the actual robot

Use a disposable test state and record the start, goal, agent, barrier position and expected
outcome. Capture the pawn's positions over time, path or route samples where available, move
request and terminal result, visible feedback and whether the package changed hands. A queued
move or a still image of a navmesh earns no arrival pass.

| Case | Action | Pass condition |
|---|---|---|
| Baseline | Send the robot to the goal with the street clear. | It reaches the goal on a complete path and completes delivery once. |
| Blocked before moving | Raise the barrier over the used route, then issue the same goal. | It takes a measured alternate route to the same goal, or reports a clear failure if no route exists. |
| Blocked mid-move | Raise a navigation relevant barrier ahead of a robot already moving. | After the supported navigation update, it replans or terminates clearly; it does not walk through or wait forever. |
| Unreachable | Remove every complete route to a valid destination. | The decision system enters a bounded failure state; it never claims arrival or pays the reward. |
| Recovery | Restore the route and retry the same goal. | The robot can move again without a stale request, duplicate action or editor rebuild. |
| Reopen | Save, leave the level, reopen it and repeat. | Agent selection, collision, decision asset and observed outcomes survive a fresh load. |

Keep these cases distinct in the evidence. A detour with a barrier present **before** movement
proves that an alternate route was available at request time. It does not prove that a robot
already in motion detects a new blockage and recovers. A StateTree or EQS asset that compiles
also needs an observed invocation, result and movement outcome before claiming this behavior.

Once these cases pass in the editor, repeat the required ones in the intended packaged game.
Record performance of navigation updates and EQS queries at the expected robot count; one robot
in an editor session does not establish a crowd budget.
