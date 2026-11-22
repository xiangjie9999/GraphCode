# Connecting your AI to this project

GraphCode writes `.gc` sidecars with no AI involved at all — that half needs nothing on this page.
This page is for the other half: letting an AI assistant **read and write** your project through
the editor.

**The production plugin includes `graphcode-bridge.exe`; Node.js and npm are not required.**
A source checkout runs the JavaScript bridge with [Node.js](https://nodejs.org/) 18 or newer.
Use the command Unreal writes for the installed bridge. The bridge connection itself
does not require an AI-provider-specific plugin. Product entitlement, if required by a later
commercial build, is a separate concern from connecting a local MCP client.

---

## The short version

1. **Open your project in Unreal.** The plugin starts a small local server and writes the
   configuration your AI client needs to:

   ```
   <YourProject>/Saved/UnrealAIBridge/mcp-config.json
   ```

2. **Open that file and copy what is in it.** It looks like this, with your paths and port:

   ```json
   {
     "mcpServers": {
       "unreal": {
         "command": "C:/Games/MyGame/Plugins/GraphCode/Resources/mcp-bridge/graphcode-bridge.exe",
         "args": [],
         "env": {
           "UNREAL_MCP_URL": "http://localhost:3000",
           "UNREAL_PROJECT_PATH": "C:/Games/MyGame/MyGame.uproject"
         }
       }
     }
   }
   ```

3. **Put its `mcpServers.unreal` entry into your AI client's MCP configuration.** Clients use
   different file formats and locations; follow the client's current MCP instructions, keeping
   the command, argument and environment values from the generated file.

4. **Ask it something.** *"Read BP_Player as GraphCode"* is a good first request — it only reads.

That is the whole setup. The file is rewritten every time the editor starts, so it always states
the port this editor actually got.

---

## Any compatible AI client

The generated file gives the values for a standard **stdio MCP server**. Follow your client's
current instructions for adding a stdio server; some accept the JSON directly and others require
equivalent settings in another format. Use the generated command, arguments and `.uproject` path
for **this** installation. Reload or restart the client's MCP connection after changing its config.
Then call a GraphCode verb with `{ "help": true }` and `unreal_info` to confirm the project you
reached before any write.

---

## What talks to what

```
your AI client  ──stdio──▶  the bridge  ──HTTP on localhost──▶  Unreal Editor
```

The editor endpoint is local loopback by default, and the bridge is a local process your client
starts and stops. Keep it on loopback unless you have deliberately configured another secure
network arrangement.

**You do not have to manage the port.** The editor writes `Saved/UnrealAIBridge/endpoint.json`
saying which port it took, and the bridge reads it. If port 3000 was busy — a second editor, or
another program — the editor moves up to 3001 and everything still finds it. Change the preferred
port only if you need to; the generated config remains the source of its actual value.

**`UNREAL_PROJECT_PATH` is the line worth keeping.** It is how the bridge knows which project's
editor to attach to, which matters as soon as you have two open. A client launched from inside the
project folder can work it out on its own; a desktop app cannot.

**Two editors of the same project need an explicit pin.** They share one `endpoint.json` and the
same project path, so automatic discovery cannot tell which editor you intend. Set
`UNREAL_MCP_URL` to the chosen editor's actual port and add `UNREAL_MCP_PIN_URL=1` to that
client's MCP environment. With the pin set, the bridge probes only that URL. If a later editor
of the same project takes the **same port**, an automation that must keep its original editor
can also set `UNREAL_MCP_EXPECT_PID` to the original `processId` from
`Saved/UnrealAIBridge/endpoint.json`. The bridge then refuses the replacement process.
The editor log reports `MCP Server started on http://localhost:<port>`. Give each client
its own pinned port. Remove the optional pins to return to automatic discovery when only
one editor is open.

---

## When it does not connect

**Your client says the server failed to start.**
Check that the executable in the generated `command` exists and that the client uses the current
configuration. After moving a project or updating GraphCode, reopen Unreal and copy the regenerated
configuration. For a source checkout whose command is `node`, also check `node --version`.

**Your client connects but every call says the editor is not reachable.**
The bridge is running and the editor is not. Open the project. The tools that only need the
committed `.gc` files still answer — asking one what it does works with the editor closed, which is
usually when you want to know.

**Two projects open and the wrong one answers.**
Each project writes its own `mcp-config.json` with its own port. Use the one from the project you
mean, and check `UNREAL_PROJECT_PATH` points at that `.uproject`.

**Two editors of one project open and the wrong one answers.**
Use `UNREAL_MCP_PIN_URL=1` with the chosen editor's `UNREAL_MCP_URL`, as described above.

**You changed the port and nothing noticed.**
Restart the editor. The port is read at startup, and both files are rewritten then.

---

## What it can do once connected

For Blender imports, follow [Blender setup](Guides/BLENDER-SETUP.md). It covers the bundled
Integrate exporter/importer, optional live Blender control and Blendkit downloads. Importing a
saved `.blend` does not require a Blender MCP connection. The AI handles the required setup.

Ask the tools themselves rather than reading a list here — the surface is generated, so anything
written down goes stale. Every verb explains itself:

```json
{ "help": true }
```

on any `gc_` tool gives you what it does and one line per parameter, and it answers **with the
editor closed**, which is when you most need it.

For normal work, give the AI [`Guides/AI-COLD-START.md`](Guides/AI-COLD-START.md). If the project
itself has not been configured, ask the AI to *"set GraphCode up for this project"* — the plugin
ships a [`SETUP.md`](../SETUP.md) written for it to read, and it will ask what kind of project this
is and write the project's own documentation from the answers.

Keep this generated connection file under `Saved/`; do not commit its machine-specific paths.
The setup stamp under `Config/GraphCodeSetup.ini` is a different file and should travel with the
project documentation it describes.
