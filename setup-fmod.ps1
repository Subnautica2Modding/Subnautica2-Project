# setup-fmod.ps1
# Copies FMOD banks from your local SN2 install and native DLLs from the FMOD download. Run once after cloning.

param(
    [string]$FMODPluginZip = ""
)

$ErrorActionPreference = "Stop"
$StubRoot = $PSScriptRoot

$gameInstallFile = Join-Path $StubRoot "GameInstallDirectory.txt"
if (Test-Path $gameInstallFile) {
    $SN2Path = (Get-Content $gameInstallFile | Where-Object { $_ -and -not $_.StartsWith(";") } | Select-Object -First 1).Trim()
}

if (-not $SN2Path -or -not (Test-Path $SN2Path)) {
    Write-Error "Could not find Subnautica 2 install. Please provide the path in GameInstallDirectory.txt"
}

# Copy FMOD banks
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

Write-Host "Done."
