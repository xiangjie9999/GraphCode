# The GraphCode Market: find, install and use a pack

A pack is a **recipe**: a small versioned bundle of GraphCode text (a `.gcshape`, `.gcmove`, `.gcrig`,
`.gcgroom`, `.gcgarment` or `.gc` file, a guide, or an agent skill). It never contains meshes, textures
or binaries. The meshes come from running the recipe on your machine. The Market is free, and the one
verb for it is `unreal_gc_market`. Ask the verb for its parameters (`{ "help": true }`, then
`{ "help": "mode" }`); this page gives the order of work, not the schema.

## The order of work

1. **Search.** `mode:"search"`, plain words in `target` (every word must match, over title, entry
   names, tags and summary: "chip" finds the Damage pack because it has `EdgeChip`). Filters are their
   own parameters, never words inside `target`: `kind` (`shape`, `move`, `rig`, `groom`, `garment`, `gc`,
   `guide`, `skill`), `tag`, `state` (`installed`, `updates`, `available`), `needs` (packs that need a
   given pack id), `sort` (`relevance`, `updated`, `name`), `max_results`. A word with a colon in
   `target` is refused. The answer says `total_matches`, `returned` and `truncated`: when `truncated`
   is true, raise `max_results` or narrow the query.
2. **Describe** a candidate: `mode:"describe"`, `pack:"<Publisher>/<Name>"`. You get its entries (each
   with an import line and a call example, ready to paste), what it needs, what uses it, the products
   it requires (`met` true or false), its description, and its build choices. Take names and
   parameters from here, never from memory.
3. **Install**: `mode:"install"`, `pack:`. Add `dry_run:true` first to see what would be written.
4. **Use it** from your own file with the answer's import line.
5. **Build** only if you want the pack's own example assets (below).

## Install writes text, nothing else

- Install writes the recipe text. It never builds anything and needs no authoring licence.
- Dependencies come along: every pack it needs is installed first, and the answer lists each step.
- Files land in `/Game/GraphCodeMarket/<Publisher>/<Name>/` (`Content/GraphCodeMarket/...` on disk),
  inside your project, so they are committed with it. One version of a pack per project. `version:`
  asks for an exact one; omit it for the newest.
- The answer carries, per pack, `folder`, `main` (the main file's asset path, without its extension: the
  `asset_path` gc_apply takes) and each entry's import line.
- Everything is checked before anything is written (signature, every file's hash, dependencies,
  required products), and a pack is written whole or not at all.
- Installed shape presets are found by `gc_search kind:"preset"` (hits carry `source:"market"`) and
  read by `gc_describe preset:"<Name>"`.
- A **skill** pack is also copied to `.claude/skills/<Name>/` in the project, and the answer says so.
  A **guide** pack's `.md` files stay in its folder; `describe` lists their paths once installed.

## Building is a separate step

Installing never builds. To make a pack's own example assets, run its main file through the ordinary
write path: `gc_apply` with `asset_path` set to the install answer's `main`. That is an authoring write
and needs a GraphCode authoring licence; the Market tab's **Build** button does the same thing. Skip it
when you only want to use the pack from your own file, which is the usual case.

**Build choices.** One recipe can offer several things to build: a tier, a state, a map layer. `describe`
lists them under `choices` (label, file, `asset_path`, `default`, `time`, `builds`). The default is the
main file. Any other choice is a file of its own that imports the main file: `gc_apply` it by its own
`asset_path` to build just that choice. They are not separate packs, and none needs another built first.

**A library pack** (its `build {}` is empty, as with Move pose libraries) has nothing to build. Use it
from your own file by importing it: `presets:/Game/GraphCodeMarket/<Publisher>/<Name>/<Name>` in a
`.gcmove`, or `X = shape('<its path>')` in a `.gcshape`. Install it and go straight to your file; never
try to build it.

## Operation packs and mesh packs

A pack's entries are either **operations** (change a mesh you already have: `Damage`'s `EdgeChip`) or
**meshes** (make a finished mesh: `TimberKit`'s `Walkway`), or both. Describe tells you which each entry
is; call an operation inside your own mesh, and `add(...)` a mesh entry. The Market tab shows the same
distinction in plain words (what you can change, and its main settings); `describe` gives you the full
parameter list and the pack's header notes.

## Worked example: chip the edge of your own brick

```text
unreal_gc_market  mode:"search", target:"chip", kind:"shape"        -> GraphCode/Damage first
unreal_gc_market  mode:"describe", pack:"GraphCode/Damage"          -> entries EdgeChip, Spall, ...; no needs
unreal_gc_market  mode:"install", pack:"GraphCode/Damage", dry_run:true
unreal_gc_market  mode:"install", pack:"GraphCode/Damage"
```

Then write your own file, using the import line and call example from the answer (read the
[shape guide](GCSHAPE.md) first; the parameters below are the preset's own, check them with
`gc_describe preset:"EdgeChip"`):

```text
Dmg   = shape('/Game/GraphCodeMarket/GraphCode/Damage/Damage')
Brick = mesh() { ...your brick...  $Dmg.EdgeChip(origin=(10, 5, 6), rotation=(0, 0, 20)) }
```

Finish and bevel the part first and break it second; never `round()` after a break. Then `gc_apply` your
own file (read first, dry run if offered) and `gc_render` it. Looking at the picture is the check: the
chip must be visible (place it into the edge, not at its origin point). That `gc_apply` builds *your*
brick. The pack's own examples are not built, and you did not need them. To also get them,
`gc_apply` the install answer's `main`.

## Installed, update, remove

- `mode:"list"`: what is installed, with folder, needs, `newest` and `state` (`installed` or `updates`).
- `mode:"update"`, `pack:` (or none for every installed pack with a newer version), `dry_run` allowed.
  An update never overwrites a file you edited: it refuses with `edited_files` naming them. Copy your
  edits into your own file, or leave that pack alone.
- `mode:"remove"`, `pack:`. The pack's text is moved to `Content/__MCPTrash` (never emptied for you).
  It refuses while another installed pack needs it, and it leaves any assets you built.
- The lock file `Content/GraphCodeMarket/market.lock.json` records each pack's version and every
  file's hash. Do not edit it; a lock that cannot be read refuses with `bad_lock` and is never rewritten.

## Refusals, and what to do

Each refusal names its code, the pack, the file and the reason, and nothing was written.

| Code | Do this |
|---|---|
| `missing_pack`, `bad_id`, `not_installed` | the id is `<Publisher>/<Name>`; find it with `search`; `list` shows what is installed |
| `missing_product` | the pack needs a GraphCode product this project lacks; `describe` shows `requires` |
| `version_conflict`, `dependency_cycle` | an installed pack is below a needed minimum or on another major; update it, or install a version that fits |
| `edited_files` | you changed the pack's files; keep your copy elsewhere, then update |
| `needed_by` | another installed pack needs it; remove that one first (the reason names it) |
| `skill_exists` | `.claude/skills/<Name>` exists and the Market did not write it; rename or move yours |
| `bad_query` | `target` takes plain words; put filters in their own parameters |
| `no_content_folder` | the project has no `Content` folder |
| `unreachable` | the hosted catalog could not be read; retry. `list` still works |
| `bad_signature`, `bad_catalog`, `hash_mismatch`, `path_escape` | the catalog or a file failed its checks. Do not work around it; report it |
| `write_failed`, `bad_lock`, `cancelled` | nothing changed (the old version is whole); read the reason |

On a hosted catalog `install` and `update` download in the background: `mode:"list"` shows the pack
once it has arrived. The first ever search may ask you to try again in a few seconds.
