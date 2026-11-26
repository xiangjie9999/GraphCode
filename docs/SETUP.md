# Setting up GraphCode

From download to your AI building in your project: about 15 minutes. You do steps 1 to 5 once per
PC (step 2 once per project); after that you just open Unreal and your AI client and ask.

**At the end you will have:** your Unreal project open with the GraphCode plugins enabled, your AI
client connected to that editor, the 21-day trial active, and your AI having looked at your project
and built a first thing in it.

**You need:**

| | |
|---|---|
| Windows | 64-bit (Windows 10 or 11). |
| Unreal Engine | **5.7 or 5.8** (check **Help > About Unreal Editor**). Each engine has its own download; take the one that matches. Any project: Blueprint-only works. |
| Not needed | A C++ compiler, Visual Studio, Node.js or Python. Everything ships precompiled. |
| An AI client | One that supports MCP: Claude Code, Cursor, Claude Desktop, Codex, or any other MCP client, with your own subscription. |
| An email | For the trial. No card. |

---

## 1. Download

From the [latest release](https://github.com/xiangjie9999/GraphCode/releases/latest) download the
`graphcode-0.1.16` package for your engine (5.7 or 5.8). Optionally, [check it is ours](STUDIOS.md#release-signing-and-keys)
with `release.json` and `release.sig.json` from the same page.

Unpack it (Windows has `tar` built in; use the file name you downloaded):

```powershell
mkdir GraphCode; tar -xf <the-file-you-downloaded>.tar.xz -C GraphCode
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
If Unreal says modules are missing or were built for another version, you took the download for the other engine: use the one for your 5.7 or 5.8.

Opening the project with GraphCode enabled also writes the file your AI client needs (step 5):
`YourProject\Saved\UnrealAIBridge\mcp-config.json`. The **Output Log** shows its path in the line
`MCP client config for this project:`.

## 4. Start the trial

The trial needs only your email: no card, no account. GraphCode emails you a code to confirm the email is yours,
and the 21 days start when you use it.

**Easiest:** once your AI is connected (step 5), send it this, with your own email:

> My email is you@example.com. Start my GraphCode trial: the licence server is
> https://rlzbzhcrwvdxoriwgajn.supabase.co/functions/v1/gc-license-device

It writes the activation file for you. Its first change is held while GraphCode emails you a code: the email,
from `noreply@graphcode.dev`, holds a code like `7F3A-9C21-4B8E-D054`.
Give the code to your AI (it goes in the file's `code =` line); the trial starts, and the AI carries on. The code works once,
only on this PC, for 15 minutes; if it ran out, ask the AI to try again for a new one.

**Or make the file yourself.** Save this file as **`%USERPROFILE%\.graphcode\activation.txt`** (for example
`C:\Users\you\.graphcode\activation.txt`), with your own email on the first line:

```
email = you@example.com
code =
server = https://rlzbzhcrwvdxoriwgajn.supabase.co/functions/v1/gc-license-device
```

Leave `code =` empty for now: the first change GraphCode is asked to make emails you a code, which goes there.
`server =` is GraphCode's licence service; copy it exactly.

Or make the file with one PowerShell command (put your email in the first line):

```powershell
$email = "you@example.com"
New-Item -ItemType Directory -Force "$env:USERPROFILE\.graphcode" | Out-Null
[IO.File]::WriteAllText("$env:USERPROFILE\.graphcode\activation.txt", "email = $email`ncode =`nserver = https://rlzbzhcrwvdxoriwgajn.supabase.co/functions/v1/gc-license-device`n")
```

Your AI can also make it for you: give it your email and this page.

One trial per email and one per PC. A new email on a PC that already had a trial gets no trial, only the free products
(it confirms its email with an emailed code the same way). If you later move to another PC, GraphCode emails you a one-time
code for that move.

The trial is checked the first time your AI changes something, and the 21 days start then. Until then
your AI can already read, search and render. There is no sign-in window: if anything is missing, the
AI's first change is refused with a message that says what to do. When the 21 days end, Core, Shape and
Integrate keep working, free for personal use, for ever; the other products need a licence.

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

## 8. What it can and cannot do

[What GraphCode can build](PRODUCTS.md) lists each product's range. Every product also has limits, and
each limit has a workaround. The page for that is `Plugins/GraphCode/Docs/Guides/CAPABILITY-MAP.md` in your
project; your AI reads it during setup and applies it, and you can ask it: *"What can GraphCode not do for
this game, and what do we do instead?"* The common ones:

- **Creatures, characters and other sculpted organic forms** are not Shape's strength. Your AI generates the
  mesh with an AI image or text-to-3D model, cleans it with the Market guide `CleanGeneratedMesh`, rigs it
  with GraphCode Rig and animates it with Move.
- **Textures, voices and music** are made with an image generator or audio service (or recorded), then
  imported and wired by GraphCode.
- **Blender** brings in static meshes and materials; skeletons and animation come another way.
- **Caves and overhangs** are built as geometry; terrain height cannot form them.
- **Whether the game is fun** is your call; GraphCode plays and records it for you to judge.

---

## If something goes wrong

| What you see | What to do |
|---|---|
| Unreal: modules missing or built for another engine version | Use the download that matches your engine, 5.7 or 5.8. |
| No `Saved\UnrealAIBridge\mcp-config.json` | Open the project once with the GraphCode plugin enabled; the Output Log shows the path. |
| The AI client cannot start the server | Check the `command` path points at `graphcode-bridge.exe` in your project's `Plugins\GraphCode`. If you moved the project, open it again and copy the new file. |
| *NOT CONNECTED* | Open the project in Unreal and keep it open; then ask again. |
| The wrong project answers | Two editors are open. Close the other one, or add `"UNREAL_MCP_PIN_URL": "1"` to `env`. |
| The port changed (another program had 3000) | Unreal picks the next free port and rewrites the file; copy it again and restart the client. |
| *no licence server to ask* | Your activation file is missing the `server=` line: save the file from step 4 again. |
| A licence refusal about time | Set Windows' clock to sync automatically; it must be within 5 minutes. |
| A licence refusal naming another PC | A licence runs on one PC at a time; the message says how to move it. |
| *trial used* or *trial ended* | One trial per email and per PC. Core, Shape and Integrate keep working for free; for the other products get a licence at [graphcode.dev/pricing](https://graphcode.dev/pricing). |

Still stuck: [open an issue](https://github.com/xiangjie9999/GraphCode/issues) or email admin@graphcode.dev.
Your AI can also read `Plugins/GraphCode/Docs/CONNECTING.md` and fix the connection itself.
