# GraphCode tools reference

Everything your AI (or a script) can call, what each call does, and the command-line, editor and
settings surface around it. Exact parameters and values always come from the tool itself:
call any tool with `{ "help": true }`, then `{ "help": "<parameter>" }`, then
`{ "help": "<parameter>", "<parameter>": "<value>" }`, or `{ "help": true, "expanded": true }` for the
whole tree. Help is generated from the code you are calling and needs no running editor. Retired
tool names still answer, naming their successor.

- [The 15 MCP tools](#the-15-mcp-tools)
- [Command-line tools](#command-line-tools-commandlets)
- [The GraphCode panel in the editor](#the-graphcode-panel-in-the-editor)
- [Settings](#settings)
- [Helper scripts in the package](#helper-scripts-in-the-package)
- [How the AI is set up](#how-the-ai-is-set-up)

## The 15 MCP tools

Their definitions total 26 KB, roughly 6,500 tokens of your AI's context (measured from the shipped
bridge's `tools/list`). MCP clients show them with an `unreal_` prefix.

### `gc_read`: read an asset as its document

- Reads any supported asset, a level, or one World Partition actor as its single `.gc` document.
- `source:"saved"` reads the file on disk and works with the editor closed. `source:"live"` reads the
  editor with your unsaved edits. The running game is captured with `gc_play` instead.
- Part reads: `blocks` (only the blocks you name), `only` and `exclude` (declarations or nodes), and
  `around` (one node and everything wired to it).
- `organise:true` lays a graph out in the order it runs and lists statements that are not on the graph;
  `clean` trims them from the view.
- `include_inherited`, `include_defaults` and `verify_values` add inherited members, default values and
  a check of the values against the engine.
- `kind:"regions"` reads a level's `.gcw` regions; `kind:"rig"` reads a rig document.

### `gc_apply`: the one write

- Applies a document (or one block of it) back to its asset, compiled and saved in one call.
- `create:true` builds the asset if nothing exists at the path: the same call makes and edits.
- `dry_run:true` validates everything against the live asset and touches nothing, including the `.gc` on disk.
- `base_sig` carries the `sig:` the edit was computed against. A stale one is refused, never merged.
- `omission:"patch"` (default): entries you leave out are kept, and a removal is an explicit `delete` line.
  `omission:"snapshot"`: the payload is the block's whole state, so an omitted entry is deleted.
- `mode:"differential"` (default) keeps unchanged graph nodes in place with the same GUID and position;
  `mode:"replace"` rebuilds the graph. `graph_name` patches one graph and leaves the others;
  `on_failure:"refuse"|"skip"` decides what happens to the other graphs when one does not validate.
- File route: edit the asset's `.gc` on disk and call with `asset_path` alone; every block is compared
  with a fresh projection and only the changed blocks are written.
- `asset_paths` applies one settings change to many assets in one call, with a result per asset.
- `compile:true` compiles and reports without writing. `resave:true` re-serialises the same authored
  state to clear load errors and sweep leftover graph nodes.
- `organise` re-lays-out the graphs after the write in the same save; `prune` removes unreachable or
  unwired statements.
- `allow_broken` saves a Blueprint the write breaks (by default it is not saved). `allow_breaking_changes`
  proceeds with a schema or widget edit whose impact scan found references, and still reports them.
- `save:false` leaves the asset dirty. `wait:0` queues a long build and returns at once; collect it with
  `unreal_info include:["tasks"]`. `budget` caps how long the editor may spend on the job.
- Also used for program files (`.gcshape`, `.gcmove`, `.gcrig`, `.gcgroom`, `.gcgarment`, `.gchuman`,
  `.gcw`); MetaHuman identities add `through:"identity"` and `preview:"update"|"discard"`.

### `gc_describe`: what may I write here

- `kind:` gives the grammar of a kind, block or program (`shape`, `move`, `rig`, `groom`, `garment`,
  `human`) with the editor closed; `include_example` adds the smallest legal example.
- `type_path:` describes a native `UENUM`, `USTRUCT` or `UCLASS`. `token:` looks up any spelled token.
  `preset:` describes a `.gcshape` library entry.
- Against a real asset it reflects one block, widget, graph node, function, component or class default.

### `gc_sync`: make a sidecar exist and be current

Reports whether the sidecar was created, was already current, or was regenerated, with a fast `bin:` proof.

### `gc_search`: one live search

Kinds: `reference` (where a name is cited across `.gc` text), `entity`, `description`, `node`,
`identity`, `widget`, `asset` and `preset`. It also answers **dependencies and referencers** of an asset
to a depth you choose, including soft references, so you know what a change or a delete touches.

### `gc_render`: see or hear the result

- `kind:"scene"` renders materials, material instances and functions, static and skeletal meshes,
  animation sequences, Niagara systems (seeked to an age with `at:`), Actor Blueprints, Procedural
  Vegetation graphs and the open level, from 1 to 8 named views with camera orbit and presets.
- `kind:"widget"` paints a Widget Blueprint at up to eight viewport sizes and returns a layout report:
  overlap, clipping and contrast (`design_time` for the designer state).
- `kind:"audio"` renders a playable WAV from a Sound Wave, Sound Cue or MetaSound, or records the live
  Play mix at its listener, with peak and RMS loudness and a silent/muted report.
- `kind:"thumbnail"` draws any asset (texture, font, sprite and more) with the editor's own renderer.
- Frames: `at`, `to`, `fps`, `count` (up to 120 frames per view) as a contact sheet, `video:"mp4"`
  (H.264 through the system encoder) or `video:"apng"`.
- `view_mode`: lit, unlit, lighting only, detail lighting, checker. `highlights` marks things that draw no
  pixels: bones, motion trails, MetaHuman landmarks, navmesh, collision, volumes, splines, regions and
  cameras; `graph:true` adds velocity and acceleration graphs for motion. A section `cut`,
  `ignore` lists, and a `reference` picture beside the render.
- `simulate:true` runs an Actor Blueprint's BeginPlay, Tick and timers in the preview (0 to 60 s) without
  starting Play; with `animation:` it previews hair and cloth moving on a clip; `physics` adds physics.
- `source:"live"` or `"viewport"` renders an open instance or the editor viewport.
- `mode:"ask"` answers what is at a pixel or mesh triangle (with vertex and edge snapping);
  `mode:"screen_to_world"` gives the actor, component, location, normal and distance under a pixel;
  `mode:"legend"` returns an identification image (`by:"uv_island"` for a UV-island map);
  `mode:"layout"` tidies a Blueprint or material graph (all of it or chosen nodes) and saves once.
- Reports local GPU memory over budget (`gpu_over_budget`) with the render.

### `gc_versions`: the version axis

- `history` lists the versions the project can speak for, for one asset or the whole project.
- `diff` compares any two versions (unified or split, `changed_only`, per block). Versions can be named
  the way you say them: `HEAD`, a Git ref, `saved`, `live`, `1am`, `yesterday 3pm`.
- `restore` goes back to an earlier version through a plan you confirm by echoing its `plan_sig`. The
  plan lists every asset it would change, create or remove. Project-wide restores take
  `include_removals` and `include_creations`; `accept_*` flags confirm breaking references, partial
  restores and replaced backups.
- `resolve` settles a merge conflict in a `.gc`: ours, theirs, or your own value.

### `gc_asset`: the asset as a package

Rename and move (the sidecar follows), duplicate, export through Unreal's native exporter (the
extension picks it, for example `.fbx`; `overwrite:"refuse"|"replace"`), and delete: a recoverable move
to `/Game/__MCPTrash`, which GraphCode never empties. Referenced assets refuse unless the exact impact is accepted.

### `gc_import`: bring an outside file in

Plan first, then import. FBX, glTF, OBJ, images and audio, with Unreal's Interchange pipeline settings
written as text; an archive (`.zip`, `.7z`, `.tar*`) imports as one batch with one settings document.
Textures take a `settings {}` block.

### `gc_invoke`: act on a live object

Call a function on a live object with named arguments (no `call_in_editor` flag needed), or press a named
widget (its `OnClicked` or another parameterless event). `dry_run` lists the handlers that would run.

### `gc_open`: show it to the person

Opens an asset or level for you; opens, creates (`new`, from `list_templates`) and saves levels
(`save_as`); loads, lists and unloads World Partition regions by bounds; returns a sound as playable
audio; opens an Editor Utility Widget for `gc_invoke`.

### `gc_play`: the running game

Start and stop Play-in-Editor; a driver lease (attach, renew, detach); `execute` an act or an ordered list
of `steps` run one per tick (waits by frames, seconds or marks); `invoke` an actor function; `spawn` and
`set`; `query` actors, overlaps and navigation paths; `shot` screenshots including the HUD; `activate` a
HUD control the way a player would; `capture` the world to a `.gcp` text snapshot; `diff` two captures;
`load` from a captured instant; `apply` a capture back; `continue` and `end`; `follow` a run's log;
`collect` and `compare` runs; `kill` a wedged run through a confirmed plan; `status` with editor and GPU
memory. Every run writes a `.gcplog` with the frame and time of each step. Full list in
[PRODUCTS.md](PRODUCTS.md#graphcode-play).

### `gc_market`: ready-made packs

`search`, `describe`, `list`, `install` (with `dry_run`), `update` and `remove`. See [MARKET.md](MARKET.md).

### `gcw_ask`: questions about space

- Named form: a thing in the open level; its bounds, forward direction and containing zones, and whether
  an agent of a given size can walk to another place (`reach`, `mesh`), with straight-line and path length.
- `$Region` form: answered from the level's `.gcw` with no editor running: a wall's bow depth, radius,
  swept angle and length, and the gaps between regions in centimetres.
- Pixel form: a ray from a pixel of a saved `gc_render` image, returning the hit, distances, the regions
  it lies in and a paste-ready `@anchor` for the level document.

### `unreal_info`: the environment

Sections: `status`, `log`, `project`, `api` (engine API documentation lookup), `storage` (every GraphCode
store), `tasks` (the editor's queue, including background builds and other agents' work) and `updates`
(offered, downloading or ready, and whether a restart is needed). It answers with no editor running.

## Command-line tools (commandlets)

They run headless through `UnrealEditor-Cmd.exe <Project>.uproject "-run=GraphCode.<Name>" ...`.

| Command | What it does | Licence |
|---|---|---|
| `GcCheck` | Checks every sidecar is present and current; exit code 1 on anything absent, stale, invalid or orphaned. Your pre-commit hook and CI gate. | Not needed, no network |
| `GcRegenerate` | Rewrites every eligible sidecar; `-Prune` removes orphans. | Not needed, no network |
| `GcApply -Asset=<path>` | Applies an edited `.gc`; `-DryRun` validates without writing. | `-DryRun` not needed; writes need a licensed PC |
| `GcMerge` | The engine behind the three-way `.gc` merge driver (`-Base -Ours -Theirs`). | Licensed PC |
| `GcResolve` | Settles recorded merge conflicts from the command line. | |
| `GcStamp` | Writes alias identity across the project. | |
| `GcShadowReport` | Reports the local merge records kept under `Saved/GraphCode/Shadow`. | No network |
| `GcCapabilities` | Prints the capability registry of the installed build. | No network |

**Git integration.** Recommended `.gitattributes`: `*.gc text eol=lf merge=binary`. An optional three-way
`.gc` merge driver ships in `Plugins/GraphCode/Tools` (`setup-merge-driver.ps1`, `gc-merge.ps1`): you
register it per clone, it combines two people's edits to one `.gc`, and it writes a real clash as a
`conflict { }` block that `gc_versions resolve` settles.

## The GraphCode panel in the editor

A tab for people, not just the AI: the project's saved versions, a comparison of any two, restore, merge
conflicts in their own zone, and **Sync all**. The update notice (Update, Later, Skip this version) and the
Market tab with pack confirmations live in the editor too. Move adds an **Animation Pose** panel with
sliders for each key pose, effort and timing (Apply to try, Save to keep).

## Settings

**Project Settings > Plugins > GraphCode** turns sidecars on or off and excludes folders; sidecars removed
this way are moved to `Saved/GraphCode/RemovedSidecars`, never deleted. **GraphCode Updates > Check
Automatically** switches the update check off (per user and project). The editor's tool server port is
chosen automatically and published in `Saved/UnrealAIBridge/endpoint.json`; two editors of one project are
told apart by `UNREAL_PROJECT_PATH`. Details: [SETTINGS.md](../Plugins/GraphCode/Docs/Guides/SETTINGS.md).

## Helper scripts in the package

In [`Plugins/GraphCode/Tools`](../Plugins/GraphCode/Tools):

| Script | What it does |
|---|---|
| `ImageToSvg/img2svg.py` | Turns artwork into SVG paths that a Shape recipe can fill, shell or round. |
| `MakeSeamless/make_seamless.py` | Makes a texture set tile and reports how well it wraps. |
| `Inspection/gc_check_assembly.py`, `gc_clearance.py`, `gc_coplanar.py` | Check placed meshes for penetration, z-fighting (coplanar faces), gaps and clearances. |
| `setup-merge-driver.ps1`, `gc-merge.ps1` | Register and run the optional `.gc` merge driver. |
| `Update/graphcode_update.py` | The program that swaps in an update after the editor closes. |

## How the AI is set up

- The editor writes `Saved/UnrealAIBridge/mcp-config.json` for any stdio MCP client; the bundled
  `graphcode-bridge.exe` needs no Node.js.
- The MCP server tells every client to read [`GRAPHCODE.md`](../Plugins/GraphCode/GRAPHCODE.md) first: the
  rules, the order of authority (the tool's own answer, then the registry, then the guides), and the map
  of 96 guides layered from routers down to one entry guide per file kind.
- [`SETUP.md`](../Plugins/GraphCode/SETUP.md) sets up a project: it writes a checklist, asks how involved
  you want to be, activates the PC, configures settings, writes the project's own notes (including the
  `GRAPHCODE.md` pointer in `CLAUDE.md`/`AGENTS.md`), then builds, looks at and plays one thing.
- Guides by purpose: AI cold start, capability map, tools, authoring, lessons; game making (UI/UX, an
  HTML-prototype-then-widget workflow, gameplay systems, designing, production targets, research and
  planning, validation by play runs, asset quality, team work, AI navigation); and one entry guide per file
  kind. Browse them in [`Plugins/GraphCode/Docs/Guides`](../Plugins/GraphCode/Docs/Guides/README.md).
