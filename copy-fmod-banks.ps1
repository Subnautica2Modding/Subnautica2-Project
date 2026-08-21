# Copies the FMOD banks from your local SN2 install into the project
# Run this when you first download the project and again on any subsequent game updates to make sure you have the most up to date list

$ErrorActionPreference = "Stop"
$StubRoot = $PSScriptRoot

$gameInstallFile = Join-Path $StubRoot "GameInstallDirectory.txt"
if (Test-Path $gameInstallFile) {
    $SN2Path = (Get-Content $gameInstallFile | Where-Object { $_ -and -not $_.StartsWith(";") } | Select-Object -First 1).Trim()
}

if (-not $SN2Path -or -not (Test-Path $SN2Path)) {
    Write-Error "Could not find Subnautica 2 install. Please provide the path in GameInstallDirectory.txt"
}

$bankSrc = Join-Path $SN2Path "Content\FMOD\Desktop"
$bankDst = Join-Path $StubRoot "Content\FMOD\Desktop"

if (-not (Test-Path $bankSrc)) {
    Write-Error "FMOD banks not found at: $bankSrc. Is SN2 fully installed?"
    exit 0
}

New-Item -ItemType Directory -Force $bankDst | Out-Null
Get-ChildItem -Path $bankSrc -File | ForEach-Object {
    Copy-Item $_.FullName (Join-Path $bankDst $_.Name) -Force
}
Write-Host "[OK] FMOD banks copied from $bankSrc"