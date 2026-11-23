# Opt this checkout in to the GraphCode `.gc` / `.gcw` merge driver. GC-VCS-MERGE-DESIGN.md M1.11.
#
#   pwsh -File Plugins/GraphCode/Tools/setup-merge-driver.ps1
#
# WHY A SETUP STEP EXISTS AT ALL, rather than the repository just configuring itself: git executes
# the driver COMMAND, so it deliberately refuses to take one from a versioned `.gitattributes` -- a
# repository that could ship an executable command would be a repository that could run code on
# clone. `.gitattributes` may only NAME a driver; the command has to come from local config, which
# is per checkout and cannot be committed. That is why this script exists and must be run once per
# clone, by each person on the project.
#
# A DEVELOPER WHO SKIPS THIS GETS TODAY'S PLAIN CONFLICT, which is the correct failure: git falls
# back to `.gitattributes`, the file is left for a human, and nothing silently degrades to a line
# merge. That is why this is safe to ship before every developer has run it.
#
# NOTE that a fresh project's `.gitattributes` still says `merge=binary` for `*.gc` / `*.gcw`.
# Running this configures the driver and changes NOTHING until that line is flipped to
# `merge=graphcode`. Flip it only after EVERY collaborator has run this script -- an attribute that
# names a driver a clone has not registered falls back to git's line merge at exit 0, which
# interleaves both sides into a file that parses and describes neither. See the README beside this
# file for the safe order.
#
# This ships INSIDE the plugin, so it does not know where it was installed. It anchors on the git
# repository root, never on its own depth in the tree.
#
# ---------------------------------------------------------------------------------------------
# THE REPOSITORY SETTING THIS DEPENDS ON, for a team. M4.4, settled item 5. One-time, by a repo
# admin, and it is not a nicety -- it is what makes "the driver always runs" true.
#
#   GitHub -> Settings -> Branches -> protect `main` -> REQUIRE BRANCHES TO BE UP TO DATE BEFORE
#   MERGING (under "Require status checks to pass").
#
# WHY. This driver is local config. GitHub's servers do not have it and never will -- see the note
# above about why git refuses an executable command from a committed file. So if the server is ever
# allowed to perform a REAL merge, it performs it with git's default rule for a `merge=binary`
# path: ours wins, whole file, silently. No proposal, no `conflict { }` construct, no `merged-from:`
# line, no non-zero exit. Theirs' changes are gone and nothing in the history says a merge decision
# was made.
#
# That is the precise failure this whole system exists to prevent, arriving through the one path
# the system does not control. "Require branches to be up to date" forces every real merge to happen
# on a developer's machine, where the driver is, and leaves the server merge always a fast-forward --
# which has nothing to decide and so cannot decide it wrongly.
# ---------------------------------------------------------------------------------------------

[CmdletBinding()]
param(
    [string] $EngineDir = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

# The repository root is the reliable anchor regardless of how deep the plugin is installed.
$repo = & git -C "$PSScriptRoot" rev-parse --show-toplevel 2>$null
if ($LASTEXITCODE -ne 0 -or -not $repo) { throw "setup-merge-driver must run inside a git repository" }

$script = Join-Path $PSScriptRoot 'gc-merge.ps1'
if (-not (Test-Path $script)) { throw "gc-merge.ps1 is missing from '$PSScriptRoot'" }

$cmd = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (-not (Test-Path $cmd)) {
    throw "no UnrealEditor-Cmd.exe at '$cmd'. Pass -EngineDir <path to your UE install>."
}

Push-Location $repo
try {
    git config --local graphcode.enginedir $EngineDir
    git config --local merge.graphcode.name 'GraphCode .gc three-way merge'
    # %O base, %A ours and the file the result is written over, %B theirs. %L -- git's conflict
    # marker size -- is deliberately unused: this driver never writes `<<<<<<<`, because those
    # markers make a `.gc` unparseable and that is the entire reason it exists.
    git config --local merge.graphcode.driver "pwsh -NoProfile -ExecutionPolicy Bypass -File `"$script`" -Base %O -Ours %A -Theirs %B"

    Write-Host "Configured merge.graphcode for $repo"
    Write-Host "  engine: $EngineDir"
    Write-Host ''
    Write-Host "'.gitattributes' still has *.gc merge=binary, so nothing changes yet. Flip it to"
    Write-Host 'merge=graphcode only after every collaborator has run this script (see README).'
    Write-Host ''
    # Printed on every run rather than only the first, because it is a REPOSITORY setting and this
    # script configures a CHECKOUT - so the person who needs to act on it is usually not the person
    # running this. Saying it once to whoever happened to set up first would be saying it to nobody.
    Write-Host 'FOR A TEAM, ONE REPOSITORY SETTING IS REQUIRED for any of this to hold (M4.4):'
    Write-Host '  Settings -> Branches -> protect main -> "Require branches to be up to date before merging"'
    Write-Host 'Without it the SERVER can perform a real merge, and the server does not have this'
    Write-Host 'driver - it would resolve a .gc by the merge=binary rule, ours-wins and silent. See'
    Write-Host 'this file header for why a server-side bot was rejected rather than overlooked.'
}
finally {
    Pop-Location
}
