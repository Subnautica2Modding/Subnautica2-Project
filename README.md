# Subnautica 2 Modding Stub

A stub UE 5.6 project for creating SN2 mods. Gives you SN2 asset references, FMOD audio events, and class definitions in the editor without needing the game's source.

## Setup

1. Clone the repo
2. Download 'FMOD for Unreal' (UE 5.6, Windows) from https://www.fmod.com/download
3. Run `setup-fmod.ps1 -FMODPluginZip 'C:\...\fmodstudio20309ue5.6win64.zip'` to copy banks and extract native DLLs
4. Open `Subnautica2.uproject` -- the plugin will compile on first open (~2 min)

## What's included

- `AssetRegistry.bin` -- makes SN2 assets browseable in the Content Browser
- Suzie plugin -- generates live UClass definitions from SN2's class maps
- FMODStudio 2.03.09 plugin source -- browse and reference SN2 audio events from Blueprints

## Notes

- FMOD bank files are not included (Unknown Worlds copyright). Copy from your local SN2 install via `setup-fmod.ps1`.
- Native FMOD DLLs are not included (Firelight copyright). Extract from the official FMOD download via `setup-fmod.ps1`.
