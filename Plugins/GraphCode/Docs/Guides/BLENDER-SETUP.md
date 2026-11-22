# Set up Blender imports as an AI

Use this guide when working from the GraphCode Production folder, including on a new machine.
The AI performs the setup needed for an authorized import: discover existing installations,
download missing dependencies, configure the connection, prepare the source, import and inspect
the result. An add-on ZIP or a menu-click checklist handed to the user is not a completed import.
Preserve existing scenes, preferences, client configuration and project work. Follow the current
session's permissions; ask only for a specific unavailable access, login or decision.

For the general project setup, start with [SETUP.md](../../SETUP.md). For source selection and
art quality, follow [CONTENT.md](CONTENT.md) and [ASSET-QUALITY.md](ASSET-QUALITY.md). This guide
owns installation and connections; the installed `Plugins/GraphCodeIntegrate/README.md` owns
the adapter's current conversion modes and limits. Learn live tool schemas from their help.

## Choose the smallest route that completes the task

| Task | Needed | Not needed for this route |
|---|---|---|
| Import named objects from a saved `.blend` | Blender executable, Core and Integrate in the target Unreal project, running Unreal | Blender MCP, sidebar add-on installation, Blendkit |
| Work with the current open Blender scene | A working Blender control connection and Integrate's Python exporter | A separate asset-library account unless sourcing assets |
| Find/download Blendkit generators or materials | Blendkit add-on or supported connected provider, access to the chosen asset, one of the import routes above | A paid subscription for assets that do not require one |

Reuse a working connection. Do not install another MCP server merely because a guide mentions
one. A saved-file export runs Blender in the background and cannot see unsaved changes in an
open scene. Inspect the live scene first when the user means what is currently open.

## What to obtain, and from where

| Component | Source | Configuration |
|---|---|---|
| GraphCode Core and Integrate | The supplied Production folder; copy its `Plugins` payload together | Enable the required products in the target project; retain their dependencies |
| Matching Unreal Engine | Use the engine identity/platform in `BUNDLE-MANIFEST.json` and the bundle's `START-HERE.md` | Open the intended `.uproject`; a binary mismatch needs a matching release |
| Blender, when absent | [Official Blender download](https://www.blender.org/download/) or [LTS releases](https://www.blender.org/download/lts/) | Discover the actual executable; use a version compatible with this Integrate release |
| Integrate companion, when needed | `Plugins/GraphCodeIntegrate/GraphCode-Integrate-Blender.zip` in the installed project | Enable `graphcode_integrate`; set its `project` preference to the project directory |
| Live Blender control, when absent | [MCP for Blender installation](https://www.mcp-for-blender.com/docs/installation), or the user's already supported Blender provider | Configure only the intended AI client and Blender installation; verify both ends |
| `uv`/`uvx`, only for that MCP provider | [Astral's official installation guide](https://docs.astral.sh/uv/getting-started/installation/) | Use the resolved `uvx` executable in the client configuration |
| Blendkit, only for library sourcing | [Official Blendkit download](https://www.blendkit.com/get-blendkit/) | Enable the add-on; verify account access when required and a writable asset cache |

The Production bridge and importer are native executables. Do not install Node.js, run npm,
compile private plugin source or download a replacement Unreal bridge to use this package.
Blender supplies the Python interpreter for the exporters; separate system Python is not required.
The three Blender components above have different jobs: Integrate converts/exports, a Blender
MCP provider controls a live scene, and Blendkit supplies library assets.

Integrate 0.3.0 was tested with Blender 5.2.1 and Unreal 5.7.4 on Windows. This is a tested
combination, not a promise that any newer or older Blender works. Inspect the installed companion's
version and the bundle manifest; the suite's version and Blender companion's version are distinct.
Follow the current official download instructions rather than a remembered version-specific URL.

## Discover before configuring

Record the target `.uproject`, its parent directory, installed plugin paths, Blender executable
and version, active Blender scene/file, and any existing AI connections. Inspect the user's current
configuration before merging into it. Never replace the complete client settings with an example.

Check these installed files exist:

```text
<project>/Plugins/GraphCode/Resources/mcp-bridge/graphcode-bridge.exe
<project>/Plugins/GraphCodeIntegrate/Resources/graphcode-integrate.exe
<project>/Plugins/GraphCodeIntegrate/Resources/export-mesh.py
<project>/Plugins/GraphCodeIntegrate/Resources/export-material.py
<project>/Plugins/GraphCodeIntegrate/GraphCode-Integrate-Blender.zip
```

Use the paths in this installation, not a developer checkout or another project's binaries.
On Windows, discover Blender through existing configuration, registered installations or the
installed program directory; do not assume `blender` is on PATH. If several versions exist,
identify the one owning the live scene or deliberately choose the compatible background version.

## Connect the target Unreal project

1. Install the Production plugins according to `START-HERE.md`. Before replacing an existing
   installation, preserve it and close its editor after handling unsaved work. Keep the package
   versions together. Do not copy plugin folders over an editor using their DLLs.
2. Enable Core and Integrate and open the target project with the matching engine. Follow the
   project's coordination rules before launching or closing an editor; reuse the correct one.
3. Read the newly generated `<project>/Saved/UnrealAIBridge/mcp-config.json`. Merge its Unreal
   entry into the active AI client's current supported configuration and reload that connection.
   [CONNECTING.md](../CONNECTING.md) explains the generated fields and multi-editor routing.
4. Call `unreal_info` and confirm the expected project. A tool's offline `help` response only
   proves the bridge can answer help; it does not prove Unreal is connected.

`UNREAL_PROJECT_PATH` in the MCP entry identifies the **`.uproject` file**. Integrate's `--project`
argument and Blender preference identify its **parent directory**. The actual generated endpoint
can change; use `mcp-config.json`/`endpoint.json`, not an assumed port 3000 or another machine's
paths. Do not hand-author connection secrets or commit machine-specific configuration.

If the AI client cannot reload its own tool configuration, finish the installation and config
merge, then report the exact remaining reload action. Resume connection verification afterward.
That is a specific host limitation, not a reason to delegate the whole setup to the user.

## Route A: export a saved Blender file

This route needs no installed sidebar companion or live Blender MCP connection. The shipped
script loads the bundled exporter directly. First inspect the source in Blender and identify
the exact mesh object names, current frame, dimensions, modifiers, UVs and material slots. Keep
the editable source and its external textures. Prepare an export copy if changes are needed.

Set these variables to discovered paths and the chosen destination; the values below are examples:

```powershell
$projectDir = 'D:/Projects/MyGame'
$blenderExe = 'D:/Apps/Blender/blender.exe'
$sourceBlend = 'D:/Art/Furniture.blend'
$integrateDir = Join-Path $projectDir 'Plugins/GraphCodeIntegrate'
$exportScript = Join-Path $integrateDir 'Resources/export-mesh.py'
$importerExe = Join-Path $integrateDir 'Resources/graphcode-integrate.exe'
$bundleDir = Join-Path $projectDir 'Saved/GraphCodeIntegrate/exports/furniture-001'
$manifestPath = Join-Path $bundleDir 'integrate.json'

& $blenderExe $sourceBlend --background --python-exit-code 1 --python $exportScript -- --object 'Chair' --object 'Table' --output $bundleDir --destination '/Game/Integrate/Furniture' --mode hybrid --resolution 2048 --surface-policy strict
if ($LASTEXITCODE -ne 0) { throw 'Blender export failed; inspect its report before continuing.' }

& $importerExe --project $projectDir --bundle $manifestPath
if ($LASTEXITCODE -ne 0) { throw 'Integrate plan failed; resolve the reported issue.' }
```

Read the root and material conversion reports and the import plan. Resolve refusals and assess
reported approximations against the requested result. Planning can stage work under `Saved` but
does not apply assets in Unreal; some validation completes after textures are created. When the
plan is appropriate for the authorized import, apply and render:

```powershell
& $importerExe --project $projectDir --bundle $manifestPath --apply --render
if ($LASTEXITCODE -ne 0) { throw 'Import or verification failed; inspect state before retrying.' }
```

Pass the `.blend` before `--python`, and script arguments after `--`. Use explicit `--object`
names instead of relying on a saved selection. Send `integrate.json` through Integrate; importing
the FBX separately would skip its material delivery and managed ownership. Core `gc_import` is
not a Blender shader converter and does not open a `.blend` as this workflow.

## Route B: control an open Blender scene

If a Blender tool connection already works, inspect its current capabilities and reuse it.
For MCP for Blender, query `get_addon_status` and `get_scene_info` to establish versions, connection
and scene identity before executing Python. Other providers may expose different tool names.

When installing MCP for Blender, use its [current installation guide](https://www.mcp-for-blender.com/docs/installation).
With `uvx` available, `uvx mcp-for-blender setup --dry-run` previews the proposed setup;
`uvx mcp-for-blender install-addon` installs the Blender side. Inspect the proposal and configure
only the intended client. Avoid an unattended setup that changes every detected AI client.
The [client configuration guide](https://www.mcp-for-blender.com/docs/mcp-clients) uses a stdio
entry whose command is the resolved `uvx` executable and arguments are `['mcp-for-blender']`.
Merge it in the client's actual format, retaining existing entries. GUI clients may need the
absolute executable path. Match the server and add-on versions and the actual Blender port.

Reload the client connection and enable the Blender add-on in the intended installation. Its
live server requires a GUI Blender process; it does not serve a background `blender --background`
process. Route A works independently in the background. Do not start a duplicate MCP server
manually when the client already launches one. Preserve the provider's configured safety policy.
Use the provider's [troubleshooting guide](https://www.mcp-for-blender.com/docs/reference/troubleshooting)
for connection failures, then repeat a scene read to establish readiness.

For Integrate in that scene, install/enable the bundled companion if absent or outdated (next
section), then call its Python API through the working Blender connection. Adapt object names
and paths to the inspected scene:

```python
from pathlib import Path
import bpy
from graphcode_integrate.mesh_export import export_meshes

objects = [bpy.data.objects[name] for name in ("Chair", "Table")]
bundle = export_meshes(
    objects,
    Path("D:/Projects/MyGame/Saved/GraphCodeIntegrate/exports/furniture-001"),
    "/Game/Integrate/Furniture",
    mode="hybrid", resolution=2048, surface_policy="strict",
)
```

Then run the same plan/apply/render importer steps as Route A. An unsaved live scene can be
exported through this API, but preserve an editable source for later reimport. Do not reopen
another file over the user's current scene or pretend its disk copy contains unsaved changes.

## Install and configure the Integrate companion when needed

The ZIP is already in Production; do not send the user elsewhere to download it. Check the
loaded module's `__file__`, `bl_info['version']`, enabled state and preferences against this ZIP.
A running Blender process can retain old Python code after files on disk have been replaced.
Handle scene preservation and restart/reload deliberately before asserting the update is active.

Use Blender's native add-on installation API. The following core steps were verified on Blender
5.2; inspect the installed Blender API if its operator properties differ. Before executing them,
back up existing `userpref.blend` and the existing `graphcode_integrate` add-on directory under
the project's `Saved/GraphCodeIntegrate/setup/<timestamp>`. Locate them through
`bpy.utils.user_resource('CONFIG')` and the installed module's `__file__`, not a guessed user path.
Back up before disabling or replacing the old add-on. Preserve unrelated preferences.

```python
from pathlib import Path
import addon_utils
import bpy

project_dir = Path("D:/Projects/MyGame").resolve()
assert list(project_dir.glob("*.uproject")), "Expected an Unreal project directory"
companion_zip = project_dir / "Plugins/GraphCodeIntegrate/GraphCode-Integrate-Blender.zip"
assert companion_zip.is_file(), companion_zip

if "graphcode_integrate" in bpy.context.preferences.addons:
    addon_utils.disable("graphcode_integrate", default_set=True)
bpy.ops.preferences.addon_install(
    filepath=str(companion_zip), overwrite=True, enable_on_install=True,
)
addon_utils.enable("graphcode_integrate", default_set=True, persistent=True)
addon = bpy.context.preferences.addons["graphcode_integrate"]
addon.preferences.project = str(project_dir)
bpy.ops.wm.save_userpref()
```

For an unattended install, execute the saved setup script with the discovered Blender executable
and `--background --python-exit-code 1 --python <script>`, after ensuring another process will
not overwrite those same preferences. **Do not use `--factory-startup` when saving user preferences**:
that would save factory defaults over unrelated settings. Do not close an active scene merely to
install an optional companion when Route A already completes the task.

In a fresh process using normal preferences, verify the module path and version, enabled add-on,
saved `project` value and importability of `graphcode_integrate.mesh_export.export_meshes`.
Match the expected version to the shipped ZIP, not a permanently hardcoded number. An install
operator returning success alone is insufficient. No Node executable preference is needed for
the shipped native importer. The sidebar remains available for users who choose manual controls.

## Optional: configure Blendkit for asset sourcing

Blendkit is the current name of BlenderKit. Reuse an installed working provider/add-on; otherwise
obtain its ZIP from the [official download page](https://www.blendkit.com/get-blendkit/), install it
with the supported Blender installation API and enable it. Inspect the downloaded package and
installed Blender version to choose the correct legacy add-on or extension installation method.
Do not guess a module identifier from the display name. Follow its
[official troubleshooting guidance](https://www.blendkit.com/docs/add-on-troubleshooting/) if needed.

Verify a writable cache/download directory, network access, existing authentication and the
chosen asset's access/licence. Keep credentials out of chat and project documentation. Complete
all available setup before requesting an actual unavailable login or paid entitlement; do not
buy or subscribe merely to finish setup. Test the required search/download capability, not just
whether the add-on appears enabled. Some generation or premium assets have separate requirements.

Then follow the [Blendkit sourcing workflow](CONTENT.md#use-blendkit-generators-and-materials-through-integrate):
record the asset URL/ID, licence and editable source, tune the generator/material, prepare the
evaluated result and deliver it through Integrate. The AI does these routine steps for the user.

## Prepare and verify the result

- **Physical scale:** the current mesh exporter requires Unit Scale 1, where one Blender unit
  becomes 100 cm. Check actual object dimensions. Changing only the unit setting is not a
  physical-scale conversion; prepare a copy while preserving geometry and material mapping.
- **Geometry:** use mesh objects in Object Mode. Modifiers are evaluated at the current frame;
  realize generated instances on an export copy first. Armature-deformed meshes are refused.
  This route sends static meshes, not skeletons, animation or level placement.
- **Materials and UVs:** verify slot assignments and source images. Additional mesh UV channels
  survive, but translated materials use UV0. Geometry-dependent bakes need non-overlapping UV0
  within 0–1. Read every conversion loss/refusal; a compiled material is not proof of appearance.
- **Unreal:** check saved assets, expected dimensions, UV channels and material slots, and inspect
  actual renders against the Blender source. Confirm the surfaces are not missing/grey fallbacks.
  Mesh import leaves Nanite off. If requested displacement needs Nanite/project settings, follow
  Integrate's README deliberately; do not change global rendering defaults for ordinary imports.
- **Reimport:** retain stable object names, destinations and `Saved/GraphCodeIntegrate/imports`
  ownership records and sources. Where verifying idempotence, resend the exact same bundle;
  a fresh FBX export can differ in bytes. A shorter selection does not delete previous assets.

## Recover without hiding a failure

| Symptom | Next action |
|---|---|
| Missing packaged exporter/importer/ZIP | Check this installation and bundle identity; obtain the complete matching package |
| Tool help works but Unreal cannot be reached | Confirm the editor/project with `unreal_info` and regenerate its connection config |
| Blender controls see the wrong scene or fail | Inspect provider/add-on versions, live GUI process and configured port; do not mutate another scene |
| Updated companion still lacks mesh export | Inspect the loaded module path/version and restart or reload safely; verify in a fresh process |
| Export refuses units, instances, UVs or materials | Repair an export copy and retry the export; do not bypass the reported requirement |
| Integrate reports dirty or externally edited assets | Inspect changes; choose a new destination or reconcile deliberately. Do not erase the ownership ledger |
| An apply call times out or loses connection | Read the destination and operation evidence before retrying; it may already have completed |
| Render differs from source | Inspect conversion reports, units, UVs, texture interpretation and lighting; fix and render again |

Record the discovered versions and local paths, chosen route, source/destination, configuration
changes and verification evidence in the project's existing local work record. Keep machine paths
and credentials out of shared templates. Leave a pointer to this guide in the project's agent
instructions so the next AI can verify the current installation without repeating setup blindly.
