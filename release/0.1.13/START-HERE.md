# GraphCode

This is the production folder. Use it locally or send this entire folder unchanged.
Version: 0.1.13. Built for Unreal Engine 5.7.4, Win64.
The bundle manifest records the engine identity and last native build.

## Install

Close Unreal before updating. Put every folder inside **Plugins** in your project's
**Plugins** directory, beside its .uproject file. Replace the previous GraphCode
folders together; do not merge versions. Open the project, enable the GraphCode
products you need in Edit > Plugins, and accept their dependencies and restart.

Included: GraphCode, GraphCodePlay, GraphCodeMove, GraphCodeWorld, GraphCodeShape, GraphCodeRig, GraphCodeGroom, GraphCodeCoreExtension, GraphCodeGarment, GraphCodeIntegrate.

Optional engine frameworks are enabled only when their features are needed.
GraphCode's help explains unavailable features and the required plugins.

## Connect your AI

Open **Saved/UnrealAIBridge/mcp-config.json** in your project and use its generated
MCP configuration in your AI client. The bundled bridge is an executable; installing
Node.js or npm is unnecessary. Ask your AI to read
[GraphCode setup](Plugins/GraphCode/SETUP.md) and set up this project.
The [guides](Plugins/GraphCode/Docs/Guides/README.md) remain inside this installation.
Every session, your AI reads [GRAPHCODE.md](Plugins/GraphCode/GRAPHCODE.md) before any GraphCode work
instead of relying on memory; the GraphCode MCP server reminds it. Before it writes any GraphCode file,
it reads that kind's entry guide (the table in GRAPHCODE.md) and every guide it lists for the task.

Writes (Apply, restore) go through that bridge, which fetches the licence token each write needs.
A request sent straight to the editor's HTTP port carries no token, so a write there is refused.
Keep this PC's clock set automatically: writes are refused while it is more than five minutes
from the licence server's time.

For Blender or Blendkit imports, have the AI follow
[Blender setup](Plugins/GraphCode/Docs/Guides/BLENDER-SETUP.md). It handles required downloads,
add-on installation, project configuration and verification; Production already includes
the Integrate exporter, native importer and Blender companion ZIP.

## Updates and compatibility

Use the engine build recorded in BUNDLE-MANIFEST.json. A plugin module/version error
requires the matching binary release; this folder does not contain private C++ source.
Public headers, Unreal build rules and precompiled objects are intentional. Integrate
includes its Blender Python add-on because Blender loads that code at runtime.

Share this folder as-is. There is no separate personal or friend edition.
