# setup-fmod.ps1
# Copies FMOD banks from your local SN2 install and native DLLs from the FMOD download. Run once after cloning.

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

# Extract native FMOD DLLs and plugin source from the official FMOD download
$binDst = Join-Path $StubRoot "Plugins\FMODStudio\Binaries\Win64"
$nativeDlls = @("fmod.dll", "fmodL.dll", "fmodstudio.dll", "fmodstudioL.dll", "resonanceaudio.dll", "fmod_vc.lib", "fmodL_vc.lib", "fmodstudio_vc.lib", "fmodstudioL_vc.lib")
$needDlls = $nativeDlls | Where-Object { -not (Test-Path (Join-Path $binDst $_)) }

if ($needDlls.Count -gt 0) {
    if (-not $FMODPluginZip -or -not (Test-Path $FMODPluginZip)) {
        Write-Host ""
        Write-Host "[ACTION REQUIRED] Native FMOD DLLs are not included in this repo (Firelight copyright)."
        Write-Host "  1. Download 'FMOD for Unreal' (UE 5.6, Windows) from https://www.fmod.com/download"
        Write-Host "  2. Re-run: .\setup-fmod.ps1 -FMODPluginZip 'C:\...\fmodstudio20309ue5.6win64.zip'"
        Write-Host ""
        Write-Host "Done (banks copied, but FMOD DLLs still needed before opening the project)."
        exit 0
    }

    $tmp = Join-Path $env:TEMP "fmod_ue56_setup"
    Expand-Archive -Path $FMODPluginZip -DestinationPath $tmp -Force

    New-Item -ItemType Directory -Force $binDst | Out-Null
    foreach ($dll in $nativeDlls) {
        $src = Join-Path $tmp "FMODStudio\Binaries\Win64\$dll"
        if (Test-Path $src) {
            Copy-Item $src $binDst -Force
        }
    }

    Remove-Item $tmp -Recurse -Force
    Write-Host "[OK] Native FMOD DLLs extracted from zip"
} else {
    Write-Host "[OK] Native FMOD DLLs already present"
}

Write-Host ""
Write-Host "Done. Open Subnautica2.uproject and FMOD events will show up under /Game/FMOD/ in the Content Browser."
