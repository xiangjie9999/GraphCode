# Plugin tools

`Update/graphcode_update.py` is GraphCode's own updater helper. The editor runs it with the engine's Python
when you click Update (it checks and unpacks the download) and when you restart to finish (it swaps the
GraphCode folders after Unreal closes, and puts the previous version back if the new one does not start).
You never run it yourself; Project Settings > Plugins > GraphCode Updates is where updates live.

For optional read-only mesh assembly checks, use `Inspection/gc_check_assembly.py` through
the [paste-ready inspection guide](../Docs/Guides/ASSEMBLY-VALIDATION.md). It reports placed
static-mesh penetration, potential z-fighting, expected connection gaps, seam samples and
optional island support. It requires Unreal editor Python and Geometry Script; it adds no
runtime dependency to the game.

To make a texture or a whole texture set tile, use `MakeSeamless/make_seamless.py` — a standalone
Python 3 CLI (NumPy, OpenCV; `pip install -r MakeSeamless/requirements.txt`). It cuts every map
identically, previews the result as a repeated field and reports the wrap; `--help` lists every
option. Workflow, layouts and reading the report: the
[materials guide](../Docs/Guides/MATERIALS.md#make-the-set-tile-and-prove-it).

To turn flat artwork into SVG paths for Shape parts, use `ImageToSvg/img2svg.py` — a standalone
Python 3 CLI (NumPy, OpenCV; `pip install -r ImageToSvg/requirements.txt`). It traces colour
regions, silhouettes or stroke centre lines into named paths and writes a preview and report;
`--help` lists every option. Usage, prompts, line art and fixes: the
[texture-to-SVG loop](../Docs/Guides/ARTISTIC-SHAPES.md#convert-texture-artwork-to-svg).

## The `.gc` merge driver

Two scripts, and the git guide behind them is [`../Docs/Guides/GIT.md`](../Docs/Guides/GIT.md). They
let git merge two people's edits to the same `.gc` / `.gcw` **as a three-way over the document**
instead of line-by-line — so *one dev moved a wall, another rotated it* merges cleanly, and a real
clash becomes a `conflict { }` construct a person resolves, never a silently interleaved file.

The merge engine itself is already in the plugin's C++ (`GraphCode.GcMerge` commandlet, proven on the
whole committed corpus). These scripts are only the wiring git needs.

| Script | What it does |
|---|---|
| `setup-merge-driver.ps1` | run **once per clone, by each person** — registers the driver command in *local* git config and records the engine path. Git refuses a driver command named in a committed file, so this cannot be done for you. |
| `gc-merge.ps1` | the driver git invokes; not run by hand. On any failure it leaves *ours* untouched and the path unmerged — it can never corrupt a file. |

## The safe order — this is the whole point

The driver is **off by default and must stay off until every clone has registered it.** The danger is
not the driver; it is an attribute that *names* a driver a clone has not registered — git then falls
back to its line merge and interleaves both sides **at exit 0, with no warning**, which is worse than
a plain conflict. So:

1. **Ship / clone as-is.** `.gitattributes` says `*.gc merge=binary` (a real clash stays a conflict a
   human resolves — the safe floor). Nothing here is required to use GraphCode.
2. **Everyone runs `setup-merge-driver.ps1`.** Dormant: it changes nothing while the attribute is
   `binary`. Skipping it just leaves that person on the safe floor.
3. **Only then flip the attribute** to `merge=graphcode` for `*.gc` and `*.gcw`, in one commit, once
   the whole team has step 2 done.
4. **Teams: turn on the one repository setting** the setup script prints — GitHub → Branches → protect
   `main` → *require branches to be up to date before merging*. Without it a server-side merge (which
   has no driver) resolves `.gc` ours-wins and silent. This forces every real merge onto a machine
   that has the driver.

Solo, or not ready? Do nothing. `merge=binary` is correct and costs you a conflict marker at worst,
never a wrong merge.

## Notes

- **Windows / PowerShell.** The one platform-specific line is the `UnrealEditor-Cmd.exe` path; a Mac
  or Linux checkout points it at `Engine/Binaries/Mac|Linux/UnrealEditor-Cmd`.
- **Engine path.** `setup-merge-driver.ps1` defaults to `C:\Program Files\Epic Games\UE_5.7`; pass
  `-EngineDir <path>` if yours differs. It is stored in local git config, never committed.
- Both scripts anchor on the git repository root, so they work wherever the plugin is installed.
