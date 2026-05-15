# Subnautica 2 Project

A template UE 5.6 project for creating SN2 mods.

## If you are brand new to modding

Please get familiar with the basics of modding with this excellent set of guides:

https://github.com/Dmgvol/UE_Modding/

Also start with a basic mod idea, such as changing a data asset value (e.g. those that start with `DA_` in `Content/Data`)

## Tools

Get started with [basic mod tooling](./Docs/Tools.md) as outlined in the above UE Modding guides.

## How to open the project

If you haven't already, install Unreal Engine 5.6.

You also need to install Visual Studio 2022 and select the MSVC `v14.38` toolchain to be able to open the project.

[Helpful guide](https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.6)
[Reddit post in case you get stuck](https://www.reddit.com/r/unrealengine/comments/1i0bopv/detected_compiler_newer_than_visual_studio_2022/)

To use, [clone](https://docs.github.com/en/desktop/contributing-and-collaborating-using-github-desktop/adding-and-cloning-repositories/cloning-and-forking-repositories-from-github-desktop), fork or download the repository as .zip.

Then double click on `Subnautica2.uproject` to open the project!

**(Optional):** If you want to generate a `.sln` file to build the project from its source, right click the `Subnautica2.uproject` file and click `Generate Visual Studio project files` (requires the correct build tools, there are plenty of docs online on how to build UE projects).

### If you want to use audio in your mod

Subnautica uses FMOD for its audio system. You must do additional setup for the project to work with it first:

1. Download 'FMOD for Unreal' (UE 5.6, Windows) from https://www.fmod.com/download
2. Run `Automation/setup-fmod.ps1 -FMODPluginZip '<downloads>\fmodstudio20309ue5.6win64.zip'` to copy banks and extract native DLLs
3. Open `Subnautica2.uproject`. It will spend some time compiling first.

This setup must be done for legal reasons:

- Raw FMOD bank files are not included (Unknown Worlds copyright). Copy from your local SN2 install via `setup-fmod.ps1`.
- Native FMOD DLLs are not included (Firelight copyright). Extract from the official FMOD download via `setup-fmod.ps1`.
