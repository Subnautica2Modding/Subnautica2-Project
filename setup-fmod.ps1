# setup-fmod.ps1
# Copies FMOD banks from your local SN2 install and native DLLs from the FMOD download. Run once after cloning.

param(
    [string]$FMODPluginZip = ""
)

$ErrorActionPreference = "Stop"
$StubRoot = $PSScriptRoot

# Extract native FMOD DLLs and plugin source from the official FMOD download
$binDst = Join-Path $StubRoot "Plugins\FMODStudio\Binaries\Win64"
$nativeDlls = @("fmod.dll", "fmodL.dll", "fmodstudio.dll", "fmodstudioL.dll", "resonanceaudio.dll", "fmod_vc.lib", "fmodL_vc.lib", "fmodstudio_vc.lib", "fmodstudioL_vc.lib")
$needDlls = $nativeDlls | Where-Object { -not (Test-Path (Join-Path $binDst $_)) }

if ($needDlls.Count -gt 0) {
    if (-not $FMODPluginZip -or -not (Test-Path $FMODPluginZip)) {
        Write-Host "Error: FMOD plugin zip file or path not valid, please try again."
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
