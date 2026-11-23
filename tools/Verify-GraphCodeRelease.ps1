<#
.SYNOPSIS
  Prove a GraphCode download is the release we signed, before you put it in a project.

.DESCRIPTION
  Three checks, each of which must pass:
    1. release.json carries a valid ES256 (ECDSA P-256 / SHA-256) signature from one of the public
       keys in release/release-keys.json. The signature covers the exact bytes of release.json.
    2. The archive's size and SHA-256 equal the values inside that signed release.json.
    3. Optional (-Folder): every file of the unpacked folder matches BUNDLE-SHA256.txt.
  Nothing is sent anywhere and nothing is changed. PowerShell 7 or Windows PowerShell 5.1.

.EXAMPLE
  ./tools/Verify-GraphCodeRelease.ps1 -Archive ~/Downloads/graphcode-0.1.14-win64.tar.xz `
      -Manifest ~/Downloads/release.json -Signature ~/Downloads/release.sig.json

.EXAMPLE
  ./tools/Verify-GraphCodeRelease.ps1 -Archive .\graphcode-0.1.14-win64.tar.xz -Folder .\GraphCode-0.1.14
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $Archive,
    [string] $Manifest,
    [string] $Signature,
    [string] $Keys,
    [string] $Folder
)
$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$archiveDir = Split-Path -Parent $archivePath
if (-not $Manifest)  { $Manifest  = Join-Path $archiveDir 'release.json' }
if (-not $Signature) { $Signature = Join-Path $archiveDir 'release.sig.json' }
if (-not $Keys)      { $Keys      = Join-Path $here '..\release\release-keys.json' }

function ConvertFrom-Base64Url([string] $Text) {
    $s = $Text.Replace('-', '+').Replace('_', '/')
    switch ($s.Length % 4) { 2 { $s += '==' } 3 { $s += '=' } }
    [Convert]::FromBase64String($s)
}
# Windows PowerShell 5.1 (.NET Framework) stops at 260-character paths unless they carry the \\?\
# prefix; some files in the package are deeper than that once unpacked under a long folder.
function Get-LongPath([string] $Path) {
    # \\?\ turns off path normalisation, so resolve '..' and relative parts first.
    if ($PSVersionTable.PSVersion.Major -lt 6 -and -not $Path.StartsWith('\\?\')) { return '\\?\' + [IO.Path]::GetFullPath($Path) }
    $Path
}
function Get-FileSha256([string] $Path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead((Get-LongPath $Path))
    try { -join ($sha.ComputeHash($stream) | ForEach-Object { $_.ToString('x2') }) }
    finally { $stream.Dispose(); $sha.Dispose() }
}
function Write-Check([bool] $Ok, [string] $Text) {
    if ($Ok) { Write-Host "  PASS  $Text" -ForegroundColor Green }
    else { Write-Host "  FAIL  $Text" -ForegroundColor Red; $script:failed = $true }
}
$failed = $false

# 1. Signature over the exact manifest bytes.
$manifestBytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $Manifest).Path)
$sig = Get-Content -Raw -LiteralPath $Signature | ConvertFrom-Json
$keySet = Get-Content -Raw -LiteralPath $Keys | ConvertFrom-Json
$jwk = $keySet.keys | Where-Object { $_.kid -eq $sig.kid } | Select-Object -First 1
Write-Host "GraphCode release check"
if (-not $jwk -or $sig.alg -ne 'ES256' -or $jwk.crv -ne 'P-256') {
    Write-Check $false "signature key '$($sig.kid)' is one of the published release keys"
} else {
    $params = New-Object Security.Cryptography.ECParameters
    $params.Curve = [Security.Cryptography.ECCurve]::CreateFromValue('1.2.840.10045.3.1.7')
    $point = New-Object Security.Cryptography.ECPoint
    $point.X = ConvertFrom-Base64Url $jwk.x
    $point.Y = ConvertFrom-Base64Url $jwk.y
    $params.Q = $point
    $ecdsa = [Security.Cryptography.ECDsa]::Create($params)
    try {
        $raw = ConvertFrom-Base64Url $sig.sig
        $ok = $raw.Length -eq 64 -and $ecdsa.VerifyData($manifestBytes, $raw, [Security.Cryptography.HashAlgorithmName]::SHA256)
        Write-Check $ok "release.json is signed by $($sig.kid) ($($jwk.role) key, created $($jwk.created))"
        # Fingerprint = SHA-256 of the raw public point X||Y. Compare it with the one published on
        # graphcode.dev/trust, so a key swapped in this repository would not go unnoticed.
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $fp = -join ($sha.ComputeHash([byte[]]($point.X + $point.Y)) | ForEach-Object { $_.ToString('x2') }) }
        finally { $sha.Dispose() }
        Write-Host "        key fingerprint (SHA-256 of X||Y): $fp"
    } finally { $ecdsa.Dispose() }
}

# 2. Archive equals the signed manifest.
$release = [Text.Encoding]::UTF8.GetString($manifestBytes) | ConvertFrom-Json
$size = (Get-Item -LiteralPath $archivePath).Length
Write-Check ($size -eq [int64]$release.size) "archive size $size bytes matches the signed manifest"
$hash = Get-FileSha256 $archivePath
Write-Check ($hash -eq $release.sha256) "archive SHA-256 $hash matches the signed manifest"
Write-Host "  version $($release.version), Unreal Engine $($release.engine) (build $($release.engineBuildId)), $($release.platform)"

# 3. Optional: the unpacked folder.
if ($Folder) {
    $root = (Resolve-Path -LiteralPath $Folder).Path
    $list = Join-Path $root 'BUNDLE-SHA256.txt'
    if (-not (Test-Path -LiteralPath $list)) {
        Write-Check $false "BUNDLE-SHA256.txt exists in $root"
    } else {
        # The list inside the folder could itself have been edited, so compare it with the copy this
        # repository publishes for the signed version before trusting it.
        $published = Join-Path $here "..\release\$($release.version)\BUNDLE-SHA256.txt"
        if (Test-Path -LiteralPath $published) {
            Write-Check ((Get-FileSha256 $list) -eq (Get-FileSha256 $published)) "the folder's BUNDLE-SHA256.txt equals the one published for $($release.version)"
        } else {
            Write-Host "        no published BUNDLE-SHA256.txt for $($release.version) beside this script; the folder's own list is used" -ForegroundColor Yellow
        }
        $bad = 0; $count = 0
        $listed = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
        [void]$listed.Add('BUNDLE-SHA256.txt')   # the list does not hash itself
        foreach ($line in [IO.File]::ReadAllLines($list)) {
            if ($line -notmatch '^([0-9a-f]{64})  (.+)$') { continue }
            $count++
            $rel = $Matches[2]; $want = $Matches[1]
            if ($rel -match '(^|/)\.\.(/|$)' -or [IO.Path]::IsPathRooted($rel)) {
                $bad++; Write-Host "        refused path: $rel" -ForegroundColor Yellow; continue
            }
            [void]$listed.Add($rel)
            $file = Join-Path $root ($rel -replace '/', [IO.Path]::DirectorySeparatorChar)
            if (-not [IO.File]::Exists((Get-LongPath $file)) -or (Get-FileSha256 $file) -ne $want) {
                $bad++; Write-Host "        changed or missing: $rel" -ForegroundColor Yellow
            }
        }
        Write-Check ($bad -eq 0) "$count listed files match BUNDLE-SHA256.txt ($bad differ)"
        $scanRoot = Get-LongPath $root
        $extra = @([IO.Directory]::EnumerateFiles($scanRoot, '*', [IO.SearchOption]::AllDirectories) | ForEach-Object {
            $_.Substring($scanRoot.Length).TrimStart('\', '/') -replace '\\', '/'
        } | Where-Object { -not $listed.Contains($_) })
        foreach ($e in $extra | Select-Object -First 20) { Write-Host "        not in the release: $e" -ForegroundColor Yellow }
        Write-Check ($extra.Count -eq 0) "no files that are not part of the release ($($extra.Count) extra)"
    }
}

if ($failed) { Write-Host "Do not install this download. Report it to support@graphcode.dev." -ForegroundColor Red; exit 1 }
Write-Host "All checks passed: this is the GraphCode release we signed." -ForegroundColor Green
