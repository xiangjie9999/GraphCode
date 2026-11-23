# The git merge driver for `.gc` and `.gcw` files. GC-VCS-MERGE-DESIGN.md M1.11.
#
# Git invokes this with %O %A %B; the RESULT is written over -Ours and the EXIT CODE is the only
# thing that keeps the path unmerged. Everything below exists to make sure both of those are true
# even when something goes wrong, because the golden rule is that the driver must NEVER be able to
# corrupt a file: on any failure -Ours is left exactly as it was and the path stays unmerged.
#
# Not invoked by hand. `setup-merge-driver.ps1` (next to this file) points git at it.
#
# This ships INSIDE the plugin, so it does not know where it was installed. It anchors on the git
# repository root (git invoked it, so there is always one), never on its own depth in the tree.
#
# WINDOWS / PowerShell. The one platform-specific line is the UnrealEditor-Cmd path below; a Mac or
# Linux checkout points it at Engine/Binaries/Mac|Linux/UnrealEditor-Cmd instead.

[CmdletBinding()]
param(
    [string] $Base,
    [Parameter(Mandatory = $true)] [string] $Ours,
    [Parameter(Mandatory = $true)] [string] $Theirs,
    [string] $EngineDir,
    [string] $Project
)

$ErrorActionPreference = 'Stop'

try {
    # The repository root is the reliable anchor regardless of how deep the plugin is installed.
    $repo = & git -C "$PSScriptRoot" rev-parse --show-toplevel 2>$null
    if ($LASTEXITCODE -ne 0 -or -not $repo) { throw "gc-merge must run inside a git repository" }

    if (-not $Project) {
        $found = Get-ChildItem -Path $repo -Filter *.uproject -File | Select-Object -First 1
        if (-not $found) { throw "no .uproject at the repository root '$repo'; pass -Project explicitly" }
        $Project = $found.FullName
    }
    if (-not $EngineDir) {
        # Written by setup-merge-driver.ps1 into LOCAL config, because the engine path is per
        # machine and this file is committed.
        $EngineDir = (& git config --get graphcode.enginedir)
    }
    if (-not $EngineDir) { throw "graphcode.enginedir is not set; run the setup-merge-driver.ps1 beside this file" }

    $cmd = Join-Path $EngineDir 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if (-not (Test-Path $cmd)) { throw "no UnrealEditor-Cmd.exe at '$cmd'" }

    $arguments = @(
        "`"$Project`""
        '-run=GraphCode.GcMerge'
        "-Ours=`"$Ours`""
        "-Theirs=`"$Theirs`""
    )
    # An absent base is M1.7's squash-merge case and is passed through as absent, not as an empty
    # file: the commandlet distinguishes them and makes the no-ancestor merge blocking.
    if ($Base -and (Test-Path $Base)) { $arguments += "-Base=`"$Base`"" }
    $arguments += @('-unattended', '-nopause', '-nosplash', '-nullrhi', '-NoLiveCoding')

    & $cmd @arguments | Out-Null
    exit $LASTEXITCODE
}
catch {
    # THE GOLDEN RULE'S LAST RUNG. Anything this script itself got wrong -- a missing engine, a bad
    # path, a broken install -- must leave `-Ours` untouched and the path unmerged, never merged
    # badly. Exit 1 is what git reads; the message is for the human who has to fix the setup.
    Write-Error "gc-merge: $($_.Exception.Message). '$Ours' was left exactly as it was."
    exit 1
}
