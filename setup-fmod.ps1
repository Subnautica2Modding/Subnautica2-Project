# setup-fmod.ps1
# Copies FMOD banks from your local SN2 install. Run this once after cloning.

param(
    [string]$SN2Path = "",
    [string]$FMODPluginZip = ""
)

$ErrorActionPreference = "Stop"
$StubRoot = $PSScriptRoot

# Locate SN2 install
if (-not $SN2Path) {
    $steamLibraries = @(
        "C:\Program Files (x86)\Steam\steamapps\common\Subnautica2",
        "D:\SteamLibrary\steamapps\common\Subnautica2",
        "E:\SteamLibrary\steamapps\common\Subnautica2"
    )
    foreach ($lib in $steamLibraries) {
        if (Test-Path $lib) { $SN2Path = $lib; break }
    }
}

if (-not $SN2Path -or -not (Test-Path $SN2Path)) {
    Write-Error "Could not find Subnautica 2 install. Pass -SN2Path 'C:\...\Subnautica2'"
}

# Copy FMOD banks
$bankSrc = Join-Path $SN2Path "Subnautica2\Content\FMOD\Desktop"
$bankDst = Join-Path $StubRoot "Content\FMOD\Desktop"

if (-not (Test-Path $bankSrc)) {
    Write-Error "FMOD banks not found at: $bankSrc`nIs SN2 fully installed?"
}

New-Item -ItemType Directory -Force $bankDst | Out-Null
Copy-Item "$bankSrc\Master.bank" $bankDst -Force
Copy-Item "$bankSrc\Master.strings.bank" $bankDst -Force
Write-Host "[OK] FMOD banks copied from $bankSrc"

# Check FMOD UE plugin
$pluginDst = Join-Path $StubRoot "Plugins\FMODStudio"

if (Test-Path $pluginDst) {
    Write-Host "[OK] FMODStudio plugin already present"
} elseif ($FMODPluginZip -and (Test-Path $FMODPluginZip)) {
    $tmp = Join-Path $env:TEMP "fmod_ue56_setup"
    Expand-Archive -Path $FMODPluginZip -DestinationPath $tmp -Force
    Copy-Item (Join-Path $tmp "FMODStudio") $pluginDst -Recurse -Force
    Remove-Item $tmp -Recurse -Force
    Write-Host "[OK] FMODStudio plugin extracted from zip"
} else {
    Write-Host ""
    Write-Host "[ACTION REQUIRED] FMODStudio plugin not found."
    Write-Host "  1. Download 'FMOD for Unreal' (UE 5.6, Windows) from https://www.fmod.com/download"
    Write-Host "  2. Re-run: .\setup-fmod.ps1 -FMODPluginZip 'C:\...\fmodstudio20309ue5.6win64.zip'"
    Write-Host "  OR extract the zip and copy the FMODStudio folder into Plugins\FMODStudio"
}

Write-Host ""
Write-Host "Done. Open Subnautica2.uproject and FMOD events will show up under /Game/FMOD/ in the Content Browser."
