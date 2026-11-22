# Use the GraphCode tools

This is a workflow guide, not a parameter manual. Tool names may be exposed with an `unreal_`
prefix by an MCP client. Call the chosen verb with `{ "help": true }`, then ask help about the
parameter or value you need. `gc_describe` gives the installed grammar and coverage for a kind.
The packaged capability export and tool catalog, rather than a remembered kind count, decide what
this installation supports. `.gcw` terrain program grammar is not an asset kind in
`gc_describe`; use the [environment guide](ENVIRONMENTS.md) for working examples and the
installed `gc_apply` dry run for the exact parser answer. `.gcgroom` program grammar is under
`gc_describe kind:"groom"` (its `recipe` blocks); the [Groom guide](GROOM.md) teaches it, and a
`gc_apply` dry run validates a whole recipe.

If this is your first GraphCode task, begin with [AI-COLD-START.md](AI-COLD-START.md). If you know
the task but not which product, document or asset owns it, use
[CAPABILITY-MAP.md](CAPABILITY-MAP.md). For failures that survive the normal loop, use the
[lessons tree](LESSONS.md).

## You want → write

Look here before you say a verb cannot do something. Each row names what to call or read in one
line; the tool's own help (`{ "help": true }`, then `{ "help": "<parameter>" }`) holds the exact
parameters, and it wins if a row disagrees.

| You want | Do | Ask or read |
|---|---|---|
| to know whether a spelling or a block exists | `gc_describe kind:"<kind>"` for the topic index, then `block:"<block>"`; for a verb, `{ "help": "<parameter>" }` | [README.md](README.md#start-here), then the entry guide for the kind |
| to know what is installed and running | `unreal_info` | [CAPABILITY-MAP.md](CAPABILITY-MAP.md) |
| a long build to run while you keep working | `gc_apply` with `wait:0`, then collect it with `unreal_info include:["tasks"]`; do not send the same write again meanwhile | `gc_apply {"help":"wait"}` |
| to try a write without changing anything | `gc_apply` with `dry_run:true` and read the proposed changes | the edit loop below |
| to see a Niagara system or an effect | `gc_render` on the system asset with `at:<seconds>`; it seeks to that age. A still of the open level does not advance its effects, so judge an effect in the asset render or in `gc_play` | [EFFECTS.md](EFFECTS.md) |
| to see hair or cloth move before the game | `gc_render` with `animation:` and `simulate:true` | [ANIMATION-HAIR-AND-CLOTH.md](ANIMATION-HAIR-AND-CLOTH.md) |
| to run an Actor Blueprint's BeginPlay and ticks in a preview | `gc_render` with `simulate:true` and `at:` (0 to 60 s); no PIE is started | `gc_render {"help":true}` |
| to ask what is at a pixel of a picture you took | `gc_render mode:"ask"` on that capture; `capture_stale` means the level changed under it. Pawns, anything with a movement component and anything attached to them no longer cause it; moving a wall still does | `gc_render {"help":"mode"}` |
| to know why a render was slow or failed | read the reply for `gpu_over_budget:` (local video memory is over the budget the OS gave the editor; the manifest carries `over_budget` and `gpu_over_budget_mb`). A capture switches the engine's on-screen messages off, so the engine's own warning is not in the pictures | [VALIDATION.md](VALIDATION.md) |
| to hear a sound | `gc_render` on the sound asset; the reply says SILENT when nothing audible was captured and says the device is muted when it is, and `data` carries `silent` and `muted` | `gc_render {"help":true}` |
| a Blueprint node property that is a struct | the engine's own export text in the node's `[ ]` property channel. An `FKey` is its bare name, `G`, not `(KeyName="G")`; a misspelt member is refused, naming it | `gc_describe kind:"blueprint" block:"graph"` |
| to know why an impure call did nothing | read the write's `diagnostics`: "impure call … nothing wired to its exec input" means the compiler prunes that node. Wire an exec edge into it or delete it | the edit loop below |
| to edit a Blueprint graph again after a write | use the names the reply gives. `aliases_renamed` lists, per graph, each alias the write renamed (`n_new -> n41`); the next edit must use the right-hand names. `orphans_removed` counts leftovers of earlier writes that were swept out of the graph | the `gc_apply` reply |
| to run the game under `gc_play` and run it again | `stop` releases your driver lease; `end` releases it once the game has taken the command; a game ending releases it too, so `attach` again for the next run | [VALIDATION.md](VALIDATION.md) |
| to see how much memory the editor and the GPU have | `gc_play` `status` returns a `memory` block: `editor_private_mb` and `gpu` (`dedicated_mb`, `budget_mb`, `rhi_tracked_mb`, `over_budget`, `over_budget_mb`); `rhi_tracked_mb` counts textures and buffers only, so it is a floor | [VALIDATION.md](VALIDATION.md) |
| to describe a tool from a route that is not the bridge | do not: a native HTTP route given `help` refuses and runs nothing. Help comes from the bridge (`{ "help": true }`) or from `gc_describe` | the refusal |
| to stop a queued task | `unreal_execute mode:"cancel" task_id:`. A task cancelled after its tool had already finished keeps its real result and status, with a warning that the cancel came too late; only a tool that stopped early reads Cancelled | `unreal_info include:["tasks"]` |
| to change one actor of a level, or put one property back to the archetype | edit only that entry in the level's `.gc` and call `gc_apply` with `asset_path` alone. The default `omission:"patch"` keeps every actor you leave out; `delete <Name>` on its own line removes one actor; `reset <Prop>` inside an actor's braces returns that one field to its archetype value | `gc_apply {"help":"omission"}`; `gc_describe kind:"level" block:"actors"` |
| to apply a big or repeated payload without sending it on the command line | write the change in the asset's `.gc` and call `gc_apply` with `asset_path` alone. Every block is compared with a fresh projection and only the blocks whose text changed are written. `gc_read` replies carry the `sidecar_path` | `gc_apply {"help":"ir"}` |
| one settings change on many assets (bulk Nanite) | `gc_apply` with `asset_paths:[...]`, `block:"settings"` and `ir:"settings { NaniteSettings.bEnabled = true }"`. It is a patch, each asset runs as a single apply would, one refusal does not stop the others, and `assets[]` in the reply carries each result | `gc_apply {"help":"asset_paths"}` |
| to repair an asset that logs load errors, or to sweep leftover graph nodes | `gc_apply` with `asset_path` and `resave:true` and nothing else: it loads, compiles, saves the same authored state and replies `orphans_removed`. `sig:` stays and `bin:` moves | `gc_apply {"help":"resave"}` |
| to replace one Blueprint function or graph body, where a node left out is removed | `block:"graph"`, `graph_name:"<name>"` and `omission:"snapshot"`: the braces are the whole graph. A new node can take any free alias such as `n_new`; the reply's `aliases_renamed` says what it became. A payload sharing no alias with the live graph is refused under `snapshot` | `gc_apply {"help":"graph_name"}`, `{"help":"omission"}` |
| a Blueprint variable that holds a class | `TSubclassOf<X>` as the variable's type, as a `gc_read` prints it | `gc_describe kind:"blueprint" block:"properties"` |
| to say where a level actor goes instead of where it is (fifty behind the boss, resting on the floor, watching them) | `@anchor`, `@surface`, `@facing` and `@scale` on the actor in place of `@placement`; they resolve at apply | `gc_describe kind:"level" block:"actors"` |
| one World Partition actor without loading the level | `kind:"external_actor"` | `gc_describe kind:"external_actor"` |
| to read a task whose wait ended, or to find out whether a write reached the editor | the stopped wait names a `task_id` and a state (`not_started`, `still_running`, `cancel_requested`, `completed_result_unreadable`, `completion_unknown`). Read the task with `unreal_info include:["tasks"] task_id:"<id>"`, which answers in any state, and do not send the write again | `unreal_info {"help":"task_id"}` |
| to know whether a newer GraphCode is offered, downloading or ready | `unreal_info include:["updates"]`; it answers with no editor running and says whether this connection must restart. Only the person installs an update | `unreal_info {"help":"include"}` |
| one small example of a kind or a block | `gc_describe kind:"<kind>" include_example:true`; for a new asset `gc_apply {"help":"create"}` carries the smallest legal recipe | `gc_describe {"help":"include_example"}` |
| a route for an agent of a given size (a mouse, a human) | `gcw_ask` with `anchor:`, `reach:` and `mesh:'<agent id>'`. The id is an editor Supported Agent, the same string a `.gcw` writes on the left of `nav(...)`. The reply gives `straight_line_cm` and `path_cm`; what counts as too far round is your call | `gcw_ask {"help":"mesh"}`; [AI-NAVIGATION.md](AI-NAVIGATION.md) |
| N frames of one view in one call, or a short video | `gc_render` with `at:`, `to:` and `fps:` (or `count:`): up to 120 frames per view. `video:"apng"` writes an animated PNG and `video:"mp4"` writes H.264 through the system encoder, on Windows only | `gc_render {"help":"video"}` |
| the game's own audio | `gc_render kind:"audio" source:"live"` records the running Play or Simulate mix at its listener to a WAV with loudness. It takes `seconds`, `output` and `inline_audio`, and no asset | `gc_render {"help":"source"}` |
| the shape without its colours (a clay check) | `gc_render view_mode:"lighting_only"`; `detail_lighting` puts the lighting on a neutral material so a normal map shows apart from painted shading | `gc_render {"help":"view_mode"}` |
| which actor, and what point of the world, is under a pixel | `gc_render mode:"screen_to_world"` with the `capture` id of a level picture you took and `x`, `y` normalized to 0..1 from the top left. It answers location, normal, actor, component and distance | `gc_render {"help":"mode"}` |
| to spawn an actor, call a function, read an actor or run a timed sequence in a running Play | `gc_play`: `mode:"spawn"` (`class:`, `at:`, `rotate:`; needs the lease; the actor goes when the game ends), `mode:"execute" act:"invoke"` with `target:"Actor.Function"` and named `args:`, `mode:"query"` with `what:"actor"`, `"overlaps"` or `"nav_path"` (reads only, no lease), `mode:"set"` with `target:`, `property:` and `value:` (one instance-editable property, with the lease) and `mode:"execute"` with `steps:[...]` for a sequence run inside the game one step per tick. A bridge started before the build of 14 Nov 2026 takes `query` and `spawn` under `gcp_run` only | `gc_play {"help":"mode"}`; [VALIDATION.md](VALIDATION.md#scripted-input-in-a-running-play) |
| a timed sequence in a running Play, such as hold an input for N ticks | `steps:[...]` on `invoke`; each step is `{act, target, args, after, timeout, comment}` and `after` is `{frames:N}`, `{seconds:S}` or `{mark:"name"}`. Every step writes a record as it begins | [VALIDATION.md](VALIDATION.md#scripted-input-in-a-running-play) |
| scripted input in a running Play | a one-node Blueprint helper that calls `InjectInputForAction`, driven by `invoke` or by `steps` | [VALIDATION.md](VALIDATION.md#scripted-input-in-a-running-play) |
| to restart from an instant instead of from the beginning | `gc_play mode:"capture"`, then `mode:"execute" act:"load"` with `target:` the capture and `entry:"Actor.Event"`: the run continues from that instant | `gc_play {"help":"act"}` |
| the props of a running Play with class and position | `gc_play mode:"capture"` with `world:"play"` writes the PIE world; `select` narrows it (`class:`, `name:`, `tag:`, `near:`). `gc_search` does not read a Play | `gc_play {"help":"select"}` |
| to drive or inspect a Play from editor Python | the script context has no world bound; use `gc_play` (`mode:"query"`, `mode:"spawn"`, `mode:"set"`, and `mode:"execute" act:"invoke"`) | [VALIDATION.md](VALIDATION.md#play-from-editor-python-what-does-not-work-and-what-to-use-instead) |

## The normal edit loop

Before this loop, unfamiliar or complex work requires the research and detailed written plan in
[RESEARCH-AND-PLAN.md](RESEARCH-AND-PLAN.md). A tool's dry-run plan checks a particular write;
it does not replace understanding the technique or planning the complete implementation.

1. **Find and identify.** `gc_search` locates a live asset, declaration or callable identity.
   `gc_read` with a small header view identifies the actual kind, blocks, `sig:` and `omits:`.
   `gc_describe` explains only the block or native type you need.
2. **Choose the state.** `saved` means disk; `live` includes unsaved editor changes. Read `live`
   before editing an open asset. Never apply a diagnostic fragment marked `sig:none` or
   `non_canonical`; get a canonical full read first.
3. **Edit the authored file.** Change the relevant `.gc`, `.gcw`, `.gcshape`, `.gcrig`, `.gcgroom`
   or other installed document at its project path. Preserve the current signature, aliases and
   omitted facets. In a graph, put new statements inside its braces. Ask `gc_describe` for that
   block's identity and deletion rules; omission can mean carry, reset or deletion depending on
   the block.
4. **Plan and apply.** Use `gc_apply`'s dry run or impact view when offered, inspect its target and
   proposed changes, then apply. `gc_apply create:true` creates a supported new asset from a whole
   document. An import from an external file goes through `gc_import` instead.
5. **Prove the result.** Read the affected asset back from `live`, check the asset-level compile
   result and errors, save, then read `saved` again. Render visual work and play-test behaviour.
   For a durable claim, unload/reload or reopen in a fresh editor session. A parser pass or clean
   return value alone is not the result the player sees.

## Which verb answers which question

| Question | Tool | Finish by checking |
|---|---|---|
| What is in this asset, on disk or unsaved in the editor? | `gc_read` | The source and `omits:` in the reply; request only the needed block on large assets. |
| How is this kind, block, token or native type written? | `gc_describe` | The installed grammar, identity rule and omission behaviour. |
| Where is an asset, node, declaration or callable identity? | `gc_search` | The full path and owner, not a bare name. Search needs a live editor. |
| Can this document create or change the asset? | `gc_apply` | Dry run, asset-level status, canonical live readback, save and reload. |
| Is a saved sidecar present and current? | `gc_sync` | The reported written/current/suppressed state; it does not author the asset. |
| How does an external file become an Unreal asset? | `gc_import` | Plan first, inspect import settings and destination, then import and read the result. |
| How is the package renamed, moved, copied or explicitly deleted? | `gc_asset` | Exact package, referencers, sidecar path and resulting asset state. |
| What did this asset look like before, and can it be restored? | `gc_versions` | Compare first; restore uses a plan and its signature, then re-read. |
| What local text history or diff is available? | `gc_versions` | The version identities and the document diff, not an assumed binary rollback. |
| What does the asset or level look like? | `gc_render` | The image **and** the layout/geometry report; view a real GPU render for material and hair judgments. |
| What is at a position in the authored world? | `gcw_ask` | The named region or actor and the coordinate frame used. |
| Can I open this level or asset for the person? | `gc_open` | The exact level or asset shown; opening a different level can disrupt unsaved work. |
| Can I activate an authored action on a live object? | `gc_invoke` | The resolved object path and the observed effect. |
| Does the game run and behave as designed? | `gc_play` | A capture, action/result, failure or success assertion, and the final state. |
| Which editor, project and optional products am I connected to? | `unreal_info` | Project identity before any live write. |

`gc_render mode:"layout"` moves graph nodes and dirties an asset; treat it as a write. A render of
appearance is an observation. `gc_play` also includes state-changing modes, so read the selected
mode's help before calling it. `gc_versions` restoration can remove an asset: compare and inspect
the plan before sending its `plan_sig`.

## Lessons that save the most time

- **A new Actor Blueprint is not necessarily an empty graph.** Unreal's factory can seed disabled
  BeginPlay/Tick/overlap events. They are included in the projection: after a header-only create,
  read the canonical live graph before patching it, preserve its aliases and reuse the existing
  event instead of adding a duplicate. A whole-document create can supply its authored graph;
  inspect the dry run's projected result rather than guessing node names such as `n0`.
- **A Blueprint's `parent:` is a creation choice, not a reparent command.** Public GraphCode block
  writes do not change an existing Blueprint's class parent. Native editor reparenting is a separate
  operation; after it, obtain a fresh canonical projection before applying more text. Component
  parenting inside `components {}` changes the attachment tree, not the Blueprint's class.
- **Attachment scale has a canonical alias.** `@attach[RelativeScale=...]` projects Unreal's
  `RelativeScale3D`; the writer maps it back to the engine property. Use the projected spelling
  for subsequent patches. A spelling change alone does not mean the scale value was lost.

- **Create Blueprint functions and their callers in one apply.** New function names, inputs and
  outputs are declared before any function body or event graph is checked or written. A body may
  read its own entry pins (`n1.X`) and call a sibling declared later in `functions { }`; an event
  graph may call those functions in the same document. This works for `create:true`, ordinary
  patches and dry runs. The former two-applies workaround (signatures first, bodies second) is
  superseded. Functions in a different asset still need to exist before their callers are applied.
- **`omits:` defines the boundary of a truthful diff.** If a facet is omitted, an unchanged `.gc`
  says nothing about it. Keep the Unreal binary and source art under version control as appropriate.
- **Names are not always addresses.** Use a full asset path or live object path and verify that the
  tool resolved the intended instance. A class name can have many live instances.
- **A failed write can still change live editor state.** After a refusal or rollback message,
  re-read `live` before trying another edit. Never assume a failed reply restored every graph.
- **A successful round trip can miss the visible outcome.** Render placement, surfaces, joints,
  hair and UI. Run actual gameplay for events, input, animation and runtime nodes.
- **Use a small new fixture to learn a risky operation.** Do not test deletion, identity changes or
  a new grammar on the owner's irreplaceable asset.
- **Keep the instrument matched to the question.** `gc_read` shows authored structure,
  `gc_render` shows appearance, and `gc_play` shows behaviour. A green result on one surface does
  not certify the others.

## Motion previews and resource limits

`gc_render` resolves a camera once per view and keeps it for that view's frames. There is no
per-frame bone-follow option, including when a named level camera supplied the initial view.
Frame the full path, use shorter views of the relevant regions, or record a native moving-camera
sequence through Unreal's cinematic workflow. A Blueprint treadmill is an authored workaround,
not a prerequisite for building a moving clip. See [cinematic camera movement](CINEMATICS-CAMERA-MOVEMENT.md).

A view is limited to 120 rendered frames. A span including both 0 and 4 seconds at 30 fps needs
121 images; a `count` request of 120 at 30 fps ends at 119/30 seconds. Preserve the intended times
by choosing a lower sample rate or splitting spans, rather than assuming the last frame was kept.
Dimension, view and total-pixel limits also apply; frames multiply into the pixel budget.

Those static budgets do not measure the GPU memory currently available to all editors and other
applications. The renderer has no automatic live-VRAM admission check or ray-tracing-off safe-preview
mode. Smaller dimensions, fewer views/frames and fewer concurrent editor sessions reduce demand;
`view_mode:"unlit"` changes shading but does not guarantee a low-memory render. If a render crashes,
retain the engine log/crash evidence before assigning the cause to a renderer feature. A passed
pixel budget is not proof that a loaded scene fits the device's current memory budget.
