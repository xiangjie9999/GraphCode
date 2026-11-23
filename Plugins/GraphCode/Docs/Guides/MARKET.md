# The GraphCode Market: find, install and use a pack

A pack is a **recipe**: a small versioned bundle of GraphCode text (a `.gcshape`, `.gcmove`, `.gcrig`,
`.gcgroom`, `.gcgarment` or `.gc` file, a guide, or an agent skill). It never contains meshes or
binaries; the one exception is a pack's optional texture set (PNG images, below), which you choose to
download. The meshes come from running the recipe on your machine. The Market is free, and the one
verb for it is `unreal_gc_market`. Ask the verb for its parameters (`{ "help": true }`, then
`{ "help": "mode" }`); this page gives the order of work, not the schema.

## The order of work

Start here for every asset, recipe or workflow. A precedent can supply the whole result or
the construction you need for one part. Inspect the closest pack and adapt it as needed.

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
4. **Use and adapt it.** Import the pack from your own file. Change exposed values where they
   cover the design. When the recipe itself needs changes, start from a project-owned copy and
   verify the changed part in context.
5. **Build** only if you want the pack's own example assets (below).

## "Get in Unreal" from GraphCode.dev

A pack page on GraphCode.dev/marketplace has a **Get in Unreal** button. It opens
`graphcode://market/install/<Publisher>/<Name>` and Window > GraphCode Market comes up on that pack with
its install confirmation already showing. **One click on Install there installs it; the link alone never
installs anything.** If the pack is already installed, unknown to the catalog, or cannot be installed in
this project, the tab says so instead.

- The link works once Unreal has opened a project with GraphCode at least once: the editor registers
  `graphcode://` for your Windows account (per user, no admin) to the handler GraphCode ships,
  `Plugins/GraphCode/Resources/mcp-bridge/graphcode-bridge.exe --market-link`. Moving the folder is fine:
  the next editor start points it at the new place.
- With no editor open, the link is kept for the next editor you start (for a day) and a message says so.
  With several editors open, each shows it; install in the one you want.
- Anything but that one form (another address, a query, `..`, an encoded `/`) is refused with a message,
  and nothing happens. The request store is `%LOCALAPPDATA%\GraphCode\MarketLinks` (`handler.log` there
  records what each link did); nothing is written into a project until you press Install.
- To open a link without a browser: the console command `GraphCode.Market.Link <link>`.

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

**Materials and textures.** A pack may bring its own materials (`describe` says `materials: true`): they
are `.gc` text and install with the pack. It may also offer a texture set (`describe`'s `textures`: how many
files, how many bytes, with every source and licence in `Textures/ATTRIBUTION.txt`). The textures download
only when you ask: `install` with `textures:true`, or the **Textures** tick beside Install in the tab.
Installing again with it adds them to a pack you installed without; an update keeps the ones you have.
Building the pack makes its materials first, before the first mesh: with the textures, they are imported
and bound; without them, the same material instances are flat colours measured from those textures. Add
the textures later and build again: the same instances take the maps in place, and every mesh already
wearing them shows the change. To use your own material instead, bind it at import, as any slot:
`shape('<the pack>', LeafBeech = $MyLeaf)`.

**Build choices.** One recipe can offer several things to build: a tier, a state, a map layer. `describe`
lists them under `choices` (label, file, `asset_path`, `default`, `time`, `builds`). The default is the
main file. Any other choice is a file of its own that imports the main file: `gc_apply` it by its own
`asset_path` to build just that choice. They are not separate packs, and none needs another built first.

On GraphCode.dev, each product has a scrollable showcase strip with a photograph of every mesh and its
name at the bottom left, including tiers and states. Scroll or drag to browse, and open a picture to
inspect it at a larger size.

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
