# GraphCode for studios

The questions a technical director or security reviewer asks before letting a closed plugin into a
team's editor, answered as plainly as we can. If something you need is missing, open an issue or
write to support@graphcode.dev.

- [Decision facts](#decision-facts)
- [Threat model: who can talk to the editor](#threat-model-who-can-talk-to-the-editor)
- [Release signing and keys](#release-signing-and-keys)
- [CI and pre-commit checks](#ci-and-pre-commit-checks)
- [Version control: Git, Perforce and others](#version-control-git-perforce-and-others)
- [Packaged games](#packaged-games)
- [Pinning, updates and engine upgrades](#pinning-updates-and-engine-upgrades)
- [If the licence server is down, or we disappear](#if-the-licence-server-is-down-or-we-disappear)
- [Seats, onboarding and uninstalling](#seats-onboarding-and-uninstalling)

## Decision facts

| | |
|---|---|
| Platform | Windows 64-bit, Unreal Engine 5.7.4 (build 47537391) only. A new engine version needs a matching release. |
| What you install | Precompiled editor plugins with public C++ headers and build rules. No implementation source. |
| Code signing | Releases are signed with our own ES256 keys (see below). The DLLs and the bridge `.exe` are **not Authenticode-signed**, so SmartScreen or endpoint tools may ask about them. |
| Licence | Per user, per product or all products, one PC at a time; a 14-day trial of everything first. [Pricing](https://graphcode.dev/pricing). |
| Network | Licence tokens before writes, an update check (can be switched off), Market downloads when used. No project content, no telemetry. [Full table](../README.md#trust-and-privacy). |
| AI | GraphCode calls no AI provider. Your AI client does, with the provider you chose. MetaHuman rigging and assembly use Epic's services. |
| Version control | Plain-text sidecars beside each asset, with any VCS. Git has a recommended setup and an optional three-way `.gc` merge driver; in Perforce, check files out before a write. |
| CI | `GcCheck` runs headless with no licence and no network. |
| Legal | [EULA](../LICENSE.md) and [privacy policy](../PRIVACY.md) are drafts under legal review. |

## Threat model: who can talk to the editor

```
your AI client --stdio--> graphcode-bridge.exe --HTTP, loopback only--> Unreal Editor (GraphCode plugins)
                               |
                               +--HTTPS--> GraphCode licence service (tokens for writes)
```

- **Other computers:** the editor's tool server refuses every request whose peer address is not
  this machine.
- **Web pages:** a tool request that carries a browser `Origin` header is refused, so a page cannot
  drive the editor through `localhost` (DNS rebinding, cross-site requests).
- **Other programs running as you** can call the editor's tools, including its script and console
  fallback (`unreal_execute`, not advertised to the AI by default), just as they could run editor
  scripts or open your project files themselves. Writes through `gc_apply` need this PC's licence.
  GraphCode trusts your local user account, the same as Unreal Editor does; on a shared or
  multi-user machine, treat the editor's local port like any other local developer service.
- **Your AI client** can do whatever the tools allow: read, write through `gc_apply` (with dry runs,
  signature checks and recoverable deletes), call functions and press widgets (`gc_invoke`), and
  drive Play-in-Editor. Review what you let it do the
  way you would review a teammate. Every write is a diff.
- **Licence tokens** last 15 minutes and are bound to the licence, the PC's key and the product.
  The PC proves itself with a P-256 key (held in the TPM where available) by signing a server
  challenge. Customer packages contain no private keys.

## Release signing and keys

- Each `release.json` is signed with ES256 over its exact bytes. The archive's SHA-256 and size are
  inside it, and `BUNDLE-SHA256.txt` lists the SHA-256 of every file in the package (1,235 files in 0.1.14;
  the manifest's count of 1,236 includes the list itself).
- Public keys: [release/release-keys.json](../release/release-keys.json). `release-tpm-1`,
  `release-backup-1` and `release-tpm-2` are compiled into 0.1.14, so its updater accepts only releases they signed.
- **Why releases since 0.1.13 are signed by `release-backup-1`:** our primary key lived in this PC's TPM. A Windows
  Secure Boot update cleared the TPM on 18 November 2026, so that key can never sign again. Releases
  are signed with the backup key (a software key, not in the TPM) until the new hardware key, `release-tpm-2` (listed as
  `primary-pending`), reaches users inside a release signed by a key they already trust. 0.1.14 is that release; the switch
  to `release-tpm-2` comes once it is widely installed.
- `tools/Verify-GraphCodeRelease.ps1` prints the fingerprint of the signing key. Compare it with the
  one published on [graphcode.dev/trust](https://graphcode.dev/trust): that way a key swapped in
  this repository would not go unnoticed.
- The verify script also flags files in an unpacked folder that are not part of the release.

## CI and pre-commit checks

The sidecar check runs headless through the editor's command-line binary. It needs no activation,
no licence and no network, so a build agent can run it without a seat:

```powershell
# Is every sidecar present and current? Exit code 1 if any is missing, stale or orphaned.
UnrealEditor-Cmd.exe MyGame.uproject "-run=GraphCode.GcCheck"
# Rewrite every sidecar (exits 1 while orphans remain, unless you add -Prune).
UnrealEditor-Cmd.exe MyGame.uproject "-run=GraphCode.GcRegenerate"
# Validate an edited .gc against its asset without writing anything.
UnrealEditor-Cmd.exe MyGame.uproject "-run=GraphCode.GcApply" -Asset=/Game/Path/BP_Name -DryRun
```

A command-line write (`GcApply` without `-DryRun`, merges) needs a licensed PC, like a write from the
AI. A dry run through the AI's `gc_apply` also uses a licence token; the command-line `-DryRun` above
does not need one. The update check never runs in commandlets, `-unattended` sessions or CI.

## Version control: Git, Perforce and others

- Sidecars are plain UTF-8 text files beside the `.uasset`. Nothing in the authoring path needs Git.
- Recommended `.gitattributes`: `*.gc text eol=lf merge=binary`. If a `.gc` conflicts, resolve the
  binary asset and save, and the text regenerates.
- **Three-way `.gc` merge driver** (optional): register it per clone with
  `Plugins/GraphCode/Tools/setup-merge-driver.ps1`. It combines two people's edits to one `.gc` through
  the `GcMerge` commandlet and writes a real clash as a `conflict { }` block, which `gc_versions resolve`
  or the GraphCode panel settles (ours, theirs or your own value).
- The GraphCode panel in the editor lists versions, compares and restores them, and shows conflicts.
- **Perforce and other VCS:** sidecars are ordinary files. GraphCode does not check files out, so check
  out an asset and its sidecar (or make them writable) before a write.

## Packaged games

GraphCode writes ordinary Unreal assets: Blueprints, materials, meshes, animations, Niagara
systems. Almost all of its modules are editor-only. A few small runtime modules are packaged while their
plugins are enabled (`GraphCodeRuntime` and `AuthoringCore` in Core, `GraphCodePlay` in Play). Only Groom
feature sockets and the optional Play test nodes reference them from your assets. Each release is
checked by packaging and launching Development and Shipping games with GraphCode installed.

## Pinning, updates and engine upgrades

- Updates never install without a person clicking. To stop the check, turn off
  **Project Settings > Plugins > GraphCode Updates > Check Automatically** (per user and project).
- To pin a version, keep the installed plugin folders (or this release's archive) and leave the
  check off. Every release stays downloadable here with its signature and checksums.
- Each release targets one exact engine build; a new engine version comes with a matching GraphCode
  release, published here with its signature and checksums.

## If the licence server is down, or we disappear

- Everything you already made is ordinary Unreal content and stays yours. It opens, builds and ships
  without contacting us.
- Reading, searching, rendering, sidecars, `GcCheck` and all normal editor work continue. AI-driven
  writes need a token from the licence server and pause until it is reachable again, with a message saying so.
- Your `.gcshape`, `.gcmove`, `.gcrig` and other recipe files are plain text in your repository.

## Seats, onboarding and uninstalling

- Studio licences are per user ([pricing](https://graphcode.dev/pricing)). Each licence writes from
  one PC at a time, and moving to another PC can take up to 15 minutes to free the old one.
- Commit the project's `GRAPHCODE.md` pointer and the notes `SETUP.md` writes, so every teammate's AI
  starts from the same rules. Do not commit `Saved/UnrealAIBridge/mcp-config.json` (it is per machine).
- **Uninstall:** close the editor and delete the `Plugins/GraphCode*` folders. Then remove the
  per-user data in `%USERPROFILE%\.graphcode` and `%LOCALAPPDATA%\GraphCode`, and the
  `HKCU\Software\Classes\graphcode` key (the Market's "Get in Unreal" link). To move your licence to
  another PC, sign this PC out first; `SETUP.md` step 2a describes how.
