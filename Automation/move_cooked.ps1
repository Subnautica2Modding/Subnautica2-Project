param(
    [string]$InstallDir = "",
    [string]$LegacyOutputDir = ""
)

$ErrorActionPreference = "Stop"

# Locate SN2 install
if (-not $InstallDir) {
    $steamLibraries = @(
        "C:\Program Files (x86)\Steam\steamapps\common\Subnautica2",
        "D:\SteamLibrary\steamapps\common\Subnautica2",
        "E:\SteamLibrary\steamapps\common\Subnautica2"
    )
    foreach ($lib in $steamLibraries) {
        if (Test-Path $lib) { $InstallDir = $lib; break }
    }
}

if (-not $InstallDir -or -not (Test-Path $InstallDir)) {
    Write-Error "Could not find Subnautica 2 install. Pass -InstallDir 'C:\...\Subnautica2'"
}

if (-not $LegacyOutputDir) {
    # $LegacyOutputDir = Join-Path $InstallDir 'Subnautica2\Content\Paks\Cooked'
    $LegacyOutputDir = Join-Path $PSScriptRoot 'Cooked'
}

$PaksDir = Join-Path $InstallDir 'Subnautica2\Content\Paks'
$ProjectDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$CookedSourceDir = Join-Path $LegacyOutputDir 'Subnautica2'
$BlacklistFile = Join-Path $PSScriptRoot 'CookedExport\BlacklistAssetTypes.txt'
$SpaceRequired = [int64]30000000000
$SpaceRequiredText = '30GB'

function Get-FreeSpaceBytes {
	param(
		[Parameter(Mandatory = $true)]
		[string]$Path
	)

	$resolved = (Resolve-Path $Path).Path
	$root = [System.IO.Path]::GetPathRoot($resolved)
	$drive = [System.IO.DriveInfo]::new($root)
	return [int64]$drive.AvailableFreeSpace
}

function Pause-AndExit {
	param(
		[int]$Code
	)

	Read-Host 'Press Enter to exit'
	exit $Code
}

try {
	$freeSpace = Get-FreeSpaceBytes -Path $InstallDir
	if ($freeSpace -lt $SpaceRequired) {
		Write-Host "ERROR: Not enough free space on drive. At least $SpaceRequiredText of free space is required to export all cooked content. Detected free space: $freeSpace bytes"
		Pause-AndExit -Code 1
	}
	else {
		Write-Host "Detected free space on drive: $freeSpace bytes - sufficient to proceed with exporting cooked content"
	}

	$projectRoot = [System.IO.Path]::GetPathRoot((Resolve-Path $ProjectDir).Path)
	$installRoot = [System.IO.Path]::GetPathRoot((Resolve-Path $InstallDir).Path)

	if ($projectRoot -ine $installRoot) {
		$projectFreeSpace = Get-FreeSpaceBytes -Path $ProjectDir
		if ($projectFreeSpace -lt $SpaceRequired) {
			Write-Host "ERROR: Not enough free space on project drive. At least $SpaceRequiredText of free space is required to export all cooked content. Detected free space: $projectFreeSpace bytes"
			Pause-AndExit -Code 1
		}
		else {
			Write-Host "Detected free space on project drive: $projectFreeSpace bytes - sufficient to proceed with exporting cooked content"
		}
	}
	else {
		Write-Host 'Install directory and project directory are on the same drive, no need to check project drive for free space.'
	}

	Write-Host ''
	Write-Host 'Reading game version from version.json...'

	$versionJsonPath = Join-Path $InstallDir 'version.json'
	if (-not (Test-Path $versionJsonPath)) {
		Write-Host "ERROR: version.json not found at: $versionJsonPath"
		Pause-AndExit -Code 1
	}

	$versionJson = Get-Content -Raw -Path $versionJsonPath | ConvertFrom-Json
	$gameVersion = [string]$versionJson.changelist

	if ([string]::IsNullOrWhiteSpace($gameVersion)) {
		Write-Host 'ERROR: Could not read changelist from version.json'
		Pause-AndExit -Code 1
	}

	$usmapPath = "Subnautica2-$gameVersion.usmap"
	Write-Host "Game version detected: $gameVersion"
	Write-Host "Cooked content will first be exported to '$LegacyOutputDir' and then moved from '$CookedSourceDir' to project folder '$ProjectDir'"
	Write-Host "Make sure that you do not have any mods directly within the $PaksDir folder, otherwise it will incorrectly pick up mod files."
	$confirm = Read-Host 'Have you checked the above information and are happy to continue? (y/n)'
	if ($confirm -notmatch '^(?i:y)$') {
		Write-Host 'Operation cancelled by user.'
		Pause-AndExit -Code 0
	}

	Write-Host ''
	Write-Host 'Running retoc to generate legacy cooked assets...'
	Write-Host 'Note: make sure that you do not have any mods in the Content\Paks folder, otherwise it will incorrectly pick up mod files'

	$retocExe = Join-Path $PSScriptRoot 'retoc\retoc.exe'
	if (-not (Test-Path $retocExe)) {
		Write-Host "ERROR: retoc.exe not found at: $retocExe"
		Pause-AndExit -Code 1
	}

	Write-Host "Command: $retocExe to-legacy --no-shaders '$PaksDir' '$LegacyOutputDir'"
	& $retocExe to-legacy --no-shaders $PaksDir $LegacyOutputDir
	if ($LASTEXITCODE -ne 0) {
		Write-Host 'WARNING: retoc.exe failed or returned an error'
	}
	else {
		Write-Host 'SUCCESS: retoc.exe completed successfully'
		if (Test-Path $LegacyOutputDir) {
			Write-Host "Folder of cooked content created: '$LegacyOutputDir'"
		}
		else {
			Write-Host "ERROR: '$LegacyOutputDir' folder not found after export"
		}
	}

	Write-Host ''

	$cookedExportExe = Join-Path $PSScriptRoot 'CookedExport\CookedExport.exe'
	if (-not (Test-Path $cookedExportExe)) {
		Write-Host "ERROR: CookedExport.exe not found at: $cookedExportExe"
		Pause-AndExit -Code 1
	}

	Write-Host "Command: $cookedExportExe -p '$PaksDir' -m '$usmapPath' -mb -dr -s '$CookedSourceDir' -c '$ProjectDir' -bat '$BlacklistFile' --print-skipped"
	& $cookedExportExe -p $PaksDir -m $usmapPath -mb -dr -s $CookedSourceDir -c $ProjectDir -bat $BlacklistFile

	Write-Host ''
	if ($LASTEXITCODE -ne 0) {
		Write-Host "CookedExport.exe exited with code $LASTEXITCODE"
		Pause-AndExit -Code $LASTEXITCODE
	}

    Write-Host "Cooked content has been exported to project folder. The temporary cooked export folder at '$LegacyOutputDir' can now be deleted to free up space (approx $SpaceRequiredText)."
    $confirmDelete = Read-Host 'Do you want to delete the temporary cooked export folder now? (y/n)'
    if ($confirmDelete -notmatch '^(?i:y)$') {
        Write-Host "You chose not to delete the temporary cooked export folder. Please remember to check and delete the folder at '$LegacyOutputDir' manually when you have time, to free up space."
        Pause-AndExit -Code 0
    }

    if (Test-Path $LegacyOutputDir) {
        Write-Host "Deleting temporary cooked export folder: '$LegacyOutputDir'"
        Remove-Item -Path $LegacyOutputDir -Recurse -Force
        if (Test-Path $LegacyOutputDir) {
            Write-Host "WARNING: Failed to delete '$LegacyOutputDir', please check and delete manually if needed"
        }
        else {
            Write-Host "Successfully deleted '$LegacyOutputDir'"
        }
    }
    else {
        Write-Host "Warning: '$LegacyOutputDir' folder not found, expected to clean up after export"
    }


	Pause-AndExit -Code 0
}
catch {
	Write-Host "ERROR: $($_.Exception.Message)"
	Pause-AndExit -Code 1
}
