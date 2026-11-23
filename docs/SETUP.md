# Setting up GraphCode

From download to your AI building in your project: about 15 minutes. You do steps 1 to 5 once per
PC (step 2 once per project); after that you just open Unreal and your AI client and ask.

**At the end you will have:** your Unreal project open with the GraphCode plugins enabled, your AI
client connected to that editor, the 14-day trial active, and your AI having looked at your project
and built a first thing in it.

**You need:**

| | |
|---|---|
| Windows | 64-bit (Windows 10 or 11). |
| Unreal Engine | Exactly **5.7.4** (check **Help > About Unreal Editor**). Any project: Blueprint-only works. |
| Not needed | A C++ compiler, Visual Studio, Node.js or Python. Everything ships precompiled. |
| An AI client | One that supports MCP: Claude Code, Cursor, Claude Desktop, Codex, or any other MCP client, with your own subscription. |
| An email | For the trial. No card. |

---

## 1. Download

Download [`graphcode-0.1.14-win64.tar.xz`](https://github.com/xiangjie9999/GraphCode/releases/download/v0.1.14/graphcode-0.1.14-win64.tar.xz) (132 MB), from the
[latest release](https://github.com/xiangjie9999/GraphCode/releases/latest). Optionally, [check it is ours](STUDIOS.md#release-signing-and-keys)
with `release.json` and `release.sig.json` from the same page.

Unpack it (Windows has `tar` built in):

```powershell
mkdir GraphCode; tar -xf graphcode-0.1.14-win64.tar.xz -C GraphCode
```

**You get:**

```
GraphCode\
  START-HERE.md            the short version of this page
  BUNDLE-MANIFEST.json     what is in the package
  BUNDLE-SHA256.txt        a checksum for every file
  Plugins\
    GraphCode\             Core: required, and holds the AI's guides and the bridge
    GraphCodeWorld\        terrain, water, planting, navigation
    GraphCodeShape\        props, buildings, plants, damage
    GraphCodeMove\         animation, effects, cutscenes, AI behaviour
    GraphCodeRig\          skeletons and skinning
    GraphCodeGroom\        hair, fur, feathers
    GraphCodeGarment\      clothing and armour
    GraphCodeCoreExtension\ MetaHumans, crowds, sound
    GraphCodePlay\         playing and testing the game
    GraphCodeIntegrate\    Blender
```

## 2. Put the plugins in your project

Close Unreal. Copy the folders inside `Plugins` into your project's own `Plugins` folder, next to your
`.uproject` (create it if there is none):

```powershell
Copy-Item GraphCode\Plugins\* "C:\Path\To\YourProject\Plugins\" -Recurse
```

- **GraphCode** (Core) is required; copy the others you want to try. The trial covers all of them.
- Keep all the folders from one release together. When you update later, delete the old `GraphCode*`
  folders first rather than copying over them.

## 3. Enable them in Unreal

1. Open your project. Go to **Edit > Plugins**, search **GraphCode**, and tick each GraphCode plugin you copied.
2. Unreal also turns on the engine plugins they need (for example IK Rig, Control Rig and Niagara for
   Move, Geometry Scripting for Shape). Accept, then click **Restart Now**.

**It worked if** the project reopens without errors and **Tools > GraphCode Market** is in the menu.
If Unreal says modules are missing or were built for another version, your engine is not 5.7.4.

Opening the project with GraphCode enabled also writes the file your AI client needs (step 5):
`YourProject\Saved\UnrealAIBridge\mcp-config.json`. The **Output Log** shows its path in the line
`MCP client config for this project:`.

## 4. Start the trial

1. Go to [graphcode.dev/start](https://graphcode.dev/start) and enter your email.
2. Save the file it gives you as **`%USERPROFILE%\.graphcode\activation.txt`** (for example
   `C:\Users\you\.graphcode\activation.txt`). It has three lines, `email=`, `code=` and `server=`; keep
   all three as they are.

The trial is checked the first time your AI changes something, and the 14 days start then. Until then
your AI can already read, search and render. There is no sign-in window: if anything is missing, the
AI's first change is refused with a message that says what to do.

## 5. Connect your AI client

Keep Unreal open. Your AI client starts GraphCode's bridge (`graphcode-bridge.exe`, inside
`Plugins\GraphCode`) by itself; you only tell it where the bridge is, using the `unreal` entry of
`Saved\UnrealAIBridge\mcp-config.json`:

```json
{ "mcpServers": { "unreal": {
  "command": "C:/Path/To/YourProject/Plugins/GraphCode/Resources/mcp-bridge/graphcode-bridge.exe",
  "args": [],
  "env": { "UNREAL_MCP_URL": "http://localhost:3000",
           "UNREAL_PROJECT_PATH": "C:/Path/To/YourProject/YourProject.uproject" } } } }
```

| Client | What to do |
|---|---|
| **Claude Code** | Copy the file to `.mcp.json` in your project folder and start `claude` there. Approve the `unreal` server when it asks. |
| **Cursor** | Copy it to `.cursor\mcp.json` in your project folder, then reload the window. |
| **Claude Desktop** | Add the `unreal` entry under `mcpServers` in `%APPDATA%\Claude\claude_desktop_config.json`, then quit Claude Desktop fully and reopen it. |
| **Codex** | Codex uses TOML: add the entry to `~/.codex/config.toml` as below, then start a new session. |
| **Any other** | Add a local (stdio) MCP server with that command and those two environment values. |

Codex:

```toml
[mcp_servers.unreal]
command = "C:/Path/To/YourProject/Plugins/GraphCode/Resources/mcp-bridge/graphcode-bridge.exe"
args = []
env = { UNREAL_MCP_URL = "http://localhost:3000", UNREAL_PROJECT_PATH = "C:/Path/To/YourProject/YourProject.uproject" }
```

**It worked if** you ask your AI *"Run unreal_info"* and it answers **CONNECTED to YourProject** with
your engine version. If it says NOT CONNECTED, Unreal is closed or opened another project.

## 6. Let your AI set up the project

Ask:

> *Read Plugins/GraphCode/SETUP.md and set up this project for GraphCode.*

What happens, in about ten minutes:

1. Your AI looks at your project: its content, plugins, version control and any notes.
2. It asks **how involved you want to be**: approve everything, approve its drafts, let it decide and
   build, or do it all and report.
3. It asks which parts you own (the game design, the art, the mechanics, the sound), then a few questions
   about what is missing, one at a time.
4. It writes short project notes it will read in every later session, so you don't repeat yourself.
5. It **builds, renders and plays one first thing** in your project, so you see it working.

## 7. Ask for what you want

From now on, open Unreal and your AI client, and ask in plain words. Some first asks:

- *"Put a wooden door on the cottage that opens when the player walks up."*
- *"Show me the level from above and list what is in it."*
- *"Find a stone wall pack in the Market and build a ruined wall along the path."*
- *"Make the character's walk heavier, then show me the motion from the side."*
- *"Undo the last change to the HUD."*

What to expect each time: your AI says what it will change, makes the change, renders or plays it to
check, and shows you the picture. Every change is saved as a version you can compare and restore
(the **GraphCode** button in the status bar).

---

## If something goes wrong

| What you see | What to do |
|---|---|
| Unreal: modules missing or built for another engine version | Use exactly Unreal Engine 5.7.4. |
| No `Saved\UnrealAIBridge\mcp-config.json` | Open the project once with the GraphCode plugin enabled; the Output Log shows the path. |
| The AI client cannot start the server | Check the `command` path points at `graphcode-bridge.exe` in your project's `Plugins\GraphCode`. If you moved the project, open it again and copy the new file. |
| *NOT CONNECTED* | Open the project in Unreal and keep it open; then ask again. |
| The wrong project answers | Two editors are open. Close the other one, or add `"UNREAL_MCP_PIN_URL": "1"` to `env`. |
| The port changed (another program had 3000) | Unreal picks the next free port and rewrites the file; copy it again and restart the client. |
| *no licence server to ask* | Your activation file is missing the `server=` line: save the file from step 4 again. |
| A licence refusal about time | Set Windows' clock to sync automatically; it must be within 5 minutes. |
| A licence refusal naming another PC | A licence runs on one PC at a time; the message says how to move it. |
| *trial used* or *trial ended* | One trial per email and per PC. Get a licence at [graphcode.dev/pricing](https://graphcode.dev/pricing). |

Still stuck: [open an issue](https://github.com/xiangjie9999/GraphCode/issues) or email support@graphcode.dev.
Your AI can also read `Plugins/GraphCode/Docs/CONNECTING.md` and fix the connection itself.
