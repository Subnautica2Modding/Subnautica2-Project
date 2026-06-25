@echo off
setlocal enabledelayedexpansion

set "INSTALL_DIR=C:\Program Files (x86)\Steam\steamapps\common\Subnautica2"
set "PAKS_DIR=%INSTALL_DIR%\Subnautica2\Content\Paks"
set "GAME_ID=1962700"
set "PROCESS_NAME=Subnautica2-Win64-Shipping"
set "JMAP_DUMPER_PATH=jmap_dumper\jmap_dumper.exe"
set "WAIT_TIME=40"
set "UE4SS_PROXY_NAME=dwmapi.dll"
set "UE4SS_PROXY_PATH=%INSTALL_DIR%\Subnautica2\Binaries\Win64\%UE4SS_PROXY_NAME%"
set "UE4SS_DISABLED=0"

echo Starting Subnautica2 automation script...
echo.

echo Reading game version from version.json...
set "VERSION_JSON_PATH=%INSTALL_DIR%\version.json"
if not exist "%VERSION_JSON_PATH%" (
    echo ERROR: version.json not found at: "%VERSION_JSON_PATH%"
    pause
    exit /b 1
)
for /f "usebackq delims=" %%i in (`powershell -NoProfile -Command "(Get-Content '%VERSION_JSON_PATH%' -Raw | ConvertFrom-Json).changelist"`) do set "GAME_VERSION=%%i"
if "%GAME_VERSION%"=="" (
    echo ERROR: Could not read changelist from version.json
    pause
    exit /b 1
)

echo Game version detected: %GAME_VERSION%
set PATTERNSLEUTH_RES_EngineVersion=5.6
set "OUTPUT_JMAP_PATH=../Content/DynamicClasses/Subnautica2-%GAME_VERSION%.jmap.gz"
set "OUTPUT_USMAP_PATH=Subnautica2-%GAME_VERSION%.usmap"
set "OUTPUT_HEADERS_PATH=Headers.hpp"
set "INDEX_PATH=DataIndex.json"
set "ASSET_SNAPSHOT_PATH=%~dp0AssetSnapshot.txt"
set "AR_PATH=%~dp0../AssetRegistry.bin"
set "LOCRES_PATH=%~dp0Localisations.json"
echo Output files will be: 
echo   - %OUTPUT_JMAP_PATH%
echo   - %OUTPUT_USMAP_PATH%
echo   - %OUTPUT_HEADERS_PATH%
echo   - %INDEX_PATH%
echo   - %ASSET_SNAPSHOT_PATH%
echo   - %AR_PATH%
echo   - %LOCRES_PATH%

echo Checking for UE4SS proxy DLL at: "%UE4SS_PROXY_PATH%"
if exist "%UE4SS_PROXY_PATH%" (
    echo SUCCESS: UE4SS detected, disabling it temporarily...
    echo Renaming "%UE4SS_PROXY_NAME%" to "%UE4SS_PROXY_NAME%.bak"
    ren "%UE4SS_PROXY_PATH%" "%UE4SS_PROXY_NAME%.bak"
    if errorlevel 1 (
        echo ERROR: Failed to rename UE4SS proxy DLL
    ) else (
        echo SUCCESS: UE4SS proxy DLL renamed successfully
        set "UE4SS_DISABLED=1"
    )
) else (
    echo INFO: UE4SS not detected at expected location, continuing as normal...
)
echo.

echo Deleting existing Subnautica2*.jmap.gz files...
dir "..\Content\DynamicClasses\Subnautica2*.jmap.gz" /b 2>nul
if errorlevel 1 (
    echo No Subnautica2*.jmap.gz files found in DynamicClasses folder
) else (
    echo Deleting Subnautica2*.jmap.gz files from DynamicClasses folder...
    @REM this utter baboonery is needed to handle this relative path for some reason, otherwise it fails silently
    for %%f in ("..\Content\DynamicClasses\Subnautica2*.jmap.gz") do (
        echo Deleting: %%f
        del "%%f"
        if errorlevel 1 (
            echo ERROR: Failed to delete %%f
        ) else (
            echo SUCCESS: Deleted %%f
        )
    )
)
echo.

echo Deleting existing Subnautica2*.usmap files...
dir "Subnautica2*.usmap" /b 2>nul
if errorlevel 1 (
    echo No Subnautica2*.usmap files found in current directory
) else (
    echo Deleting Subnautica2*.usmap files from current directory...
    for %%f in ("Subnautica2*.usmap") do (
        echo Deleting: %%f
        del "%%f"
        if errorlevel 1 (
            echo ERROR: Failed to delete %%f
        ) else (
            echo SUCCESS: Deleted %%f
        )
    )
)
echo.

echo Steam command: steam://rungameid/%GAME_ID%
echo Launching Subnautica2 via Steam...
start "" "steam://rungameid/%GAME_ID%"
echo Game launch command sent to Steam
echo.

echo Waiting %WAIT_TIME% seconds for game to load...
powershell -Command "Start-Sleep -Seconds %WAIT_TIME%"
echo Wait period completed
echo.

echo Searching for game process: %PROCESS_NAME%
set "PID="
for /f "tokens=2" %%i in ('tasklist /fi "imagename eq %PROCESS_NAME%.exe" /fo table /nh 2^>nul') do (
    set "PID=%%i"
    goto :found_process
)

:not_found
echo ERROR: Game process '%PROCESS_NAME%.exe' not found!
echo Make sure the game has fully loaded and try again.
pause
exit /b 1

:found_process
echo Game process found with PID: %PID%
echo.

echo Current working directory: "%CD%"
echo Target .jmap.gz path: "%OUTPUT_JMAP_PATH%"
echo Target .usmap path: "%OUTPUT_USMAP_PATH%"
echo.

echo Running jmap_dumper for .jmap.gz output...
echo Command: %JMAP_DUMPER_PATH% --pid %PID% --suzie "%OUTPUT_JMAP_PATH%"
%JMAP_DUMPER_PATH% --pid %PID% --suzie "%OUTPUT_JMAP_PATH%"
if errorlevel 1 (
    echo WARNING: jmap_dumper for .jmap.gz file failed or returned an error
) else (
    echo SUCCESS: .jmap.gz dump completed successfully
    if exist "%OUTPUT_JMAP_PATH%" (
        echo File created: "%OUTPUT_JMAP_PATH%"
    ) else (
        echo ERROR: .jmap.gz file not found after dump!
    )
)
echo.

echo Closing game process (PID: %PID%)...
taskkill /pid %PID% /f >nul 2>&1
if errorlevel 1 (
    echo WARNING: Failed to close game process. You may need to close it manually.
) else (
    echo Game closed successfully
)
echo.

if "%UE4SS_DISABLED%"=="1" (
    echo Restoring UE4SS proxy DLL from backup...
    echo Waiting 2 seconds before restoration...
    powershell -Command "Start-Sleep -Seconds 2"
    echo Renaming "%UE4SS_PROXY_NAME%.bak" back to "%UE4SS_PROXY_NAME%"
    ren "%UE4SS_PROXY_PATH%.bak" "%UE4SS_PROXY_NAME%"
    if errorlevel 1 (
        echo ERROR: Failed to restore UE4SS proxy DLL
    ) else (
        echo SUCCESS: UE4SS restored successfully
    )
    echo.
)
echo.

echo Running jmap_dumper for .usmap output...
echo Command: %JMAP_DUMPER_PATH% --jmap "%OUTPUT_JMAP_PATH%" "%OUTPUT_USMAP_PATH%"
%JMAP_DUMPER_PATH% --jmap "%OUTPUT_JMAP_PATH%" "%OUTPUT_USMAP_PATH%"
if errorlevel 1 (
    echo WARNING: jmap_dumper for .usmap file failed or returned an error
) else (
    echo SUCCESS: .usmap dump completed successfully
    if exist "%OUTPUT_USMAP_PATH%" (
        echo File created: "%OUTPUT_USMAP_PATH%"
    ) else (
        echo ERROR: .usmap file not found after dump!
    )
)
echo.

echo Running jmap_dumper for Headers.hpp output...
echo Command: %JMAP_DUMPER_PATH% --jmap "%OUTPUT_JMAP_PATH%" --no-offsets "%OUTPUT_HEADERS_PATH%"
%JMAP_DUMPER_PATH% --jmap "%OUTPUT_JMAP_PATH%" --no-offsets "%OUTPUT_HEADERS_PATH%"
if errorlevel 1 (
    echo WARNING: jmap_dumper for Headers.hpp file failed or returned an error
) else (
    echo SUCCESS: Headers.hpp dump completed successfully
    if exist "%OUTPUT_HEADERS_PATH%" (
        echo File created: "%OUTPUT_HEADERS_PATH%"
    ) else (
        echo ERROR: Headers.hpp file not found after dump!
    )
)
echo.

set "TABLE_DUMPER_PATH=TableGraph/TableGraph.exe"
echo Running TableGraph.exe for DataTable/DataAsset dumps...
echo Command: "%TABLE_DUMPER_PATH%" --pak-dir "%PAKS_DIR%" --mappings "%OUTPUT_USMAP_PATH%" --version GAME_UE5_6 --export "%INDEX_PATH%"
"%TABLE_DUMPER_PATH%" --pak-dir "%PAKS_DIR%" --mappings "%OUTPUT_USMAP_PATH%" --version GAME_UE5_6 --export "%INDEX_PATH%"
if errorlevel 1 (
    echo WARNING: TableGraph.exe failed or returned an error
) else (
    echo SUCCESS: TableGraph.exe completed successfully
    if exist "%INDEX_PATH%" (
        echo File created: "%INDEX_PATH%"
    ) else (
        echo ERROR: "%INDEX_PATH%" file not found after export!
    )
)
echo.

set "COOKED_EXPORT_PATH=CookedExport/CookedExport.exe"
echo Running CookedExport.exe for asset list snapshot...
echo Command: "%COOKED_EXPORT_PATH%" -p "%PAKS_DIR%" -m "%OUTPUT_USMAP_PATH%" -dra -ipp "Engine/" -ipp "Subnautica2/Content/Maps/Main/L_Main/_Generated_/"  -ro "%ASSET_SNAPSHOT_PATH%"
"%COOKED_EXPORT_PATH%" -p "%PAKS_DIR%" -m "%OUTPUT_USMAP_PATH%" -dra -ipp "Engine/" -ipp "Subnautica2/Content/Maps/Main/L_Main/_Generated_/" -ro "%ASSET_SNAPSHOT_PATH%"
if errorlevel 1 (
    echo WARNING: CookedExport.exe failed or returned an error
) else (
    echo SUCCESS: CookedExport.exe completed successfully
    if exist "%ASSET_SNAPSHOT_PATH%" (
        echo File created: "%ASSET_SNAPSHOT_PATH%"
    ) else (
        echo ERROR: "%ASSET_SNAPSHOT_PATH%" file not found after export!
    )
)
echo.

echo Running CookedExport.exe for asset registry...
echo Command: "%COOKED_EXPORT_PATH%" -p "%PAKS_DIR%" -erb -arbo "%AR_PATH%"
"%COOKED_EXPORT_PATH%" -p "%PAKS_DIR%" -erb -arbo "%AR_PATH%"
if errorlevel 1 (
    echo WARNING: CookedExport.exe failed or returned an error
) else (
    echo SUCCESS: CookedExport.exe completed successfully
    if exist "%AR_PATH%" (
        echo File created: "%AR_PATH%"
    ) else (
        echo ERROR: "%AR_PATH%" file not found after export!
    )
)
echo.

echo Running CookedExport.exe for localisations file...
echo Command: "%COOKED_EXPORT_PATH%" -p "%PAKS_DIR%" -dl "Subnautica2/Content/Localization/Game/en/Game.locres" "%LOCRES_PATH%"
"%COOKED_EXPORT_PATH%" -p "%PAKS_DIR%" -dl "Subnautica2/Content/Localization/Game/en/Game.locres" "%LOCRES_PATH%"
if errorlevel 1 (
    echo WARNING: CookedExport.exe failed or returned an error
) else (
    echo SUCCESS: CookedExport.exe completed successfully
    if exist "%LOCRES_PATH%" (
        echo File created: "%LOCRES_PATH%"
    ) else (
        echo ERROR: "%LOCRES_PATH%" file not found after export!
    )
)
echo.

echo Done
pause