Fab plugin for Subnautica 2 custom Unreal editor
================================================
Built by: Gam3rGoon
Built:    2026-09-05

Required Engine BuildId (MUST MATCH):
  a0f35de5-25c7-44b8-87b3-4fe5671dcf5c

Target custom engine release:
  Subnautica 2 Custom Engine V4
  https://github.com/Buckminsterfullerene02/UnrealEngine/releases/tag/sn2-v0.1.2.0

Project fork (this repo):
  https://github.com/whagan1310-droid/Subnautica2-Project


COMPATIBILITY (READ THIS FIRST)
-------------------------------
This plugin matches the engine **BuildId**, NOT the folder name.

  - Your engine folder can be named anything
    (UE_5.6_sub, UE_5.6_sn2, MySn2Editor, etc.). Name does not matter.
  - It MUST be the SN2 custom editor build whose
    UnrealEditor.modules BuildId is:
      a0f35de5-25c7-44b8-87b3-4fe5671dcf5c

Check your BuildId here:
  <YourEngine>\Engine\Binaries\Win64\UnrealEditor.modules
  Look for: "BuildId": "a0f35de5-25c7-44b8-87b3-4fe5671dcf5c"

If BuildId matches -> use this plugin.
If BuildId differs  -> this package will NOT load (module skipped / not found).
Stock Epic UE 5.6 Fab DLLs will NOT work on this custom BuildId.


WHAT THIS IS
------------
Prebuilt Epic Fab Content Browser plugin (toolbar), compiled for the SN2
custom editor BuildId above. Browse/download Fab assets in-editor.


INSTALL (this fork already includes Plugins/Fab)
------------------------------------------------
If cloning this fork:
  1. Use Custom Engine V4 / BuildId above.
  2. Fab is Enabled in Subnautica2.uproject.
  3. Open the project — Fab button should appear in Content Browser.

Manual / other projects on the same engine:

  A) Engine install (recommended for all projects on this editor)
     1. Close Unreal Editor.
     2. Remove/rename if present: <YourEngine>\Engine\Plugins\Fab
     3. Copy this Fab folder to: <YourEngine>\Engine\Plugins\Fab
     4. Enable in .uproject:
          { "Name": "Fab", "Enabled": true }
     5. Open with the matching-BuildId custom engine.

  B) Project install
     1. Close Unreal Editor.
     2. Copy Fab folder to: <YourProject>\Plugins\Fab
     3. Enable Fab in .uproject as above.
     4. Open with the matching-BuildId custom engine.


VERIFY
------
  - Content Browser toolbar shows the Fab button
  - Log: Mounting ... plugin Fab  /  LogFab: Plugin initialized
  - No: "module 'Fab' could not be found"
  - No: "Skipping out-of-date modules" for Fab


DO NOT
------
  - Install on stock Epic Launcher UE 5.6 (different BuildId)
  - Mix with stock Fab binaries under Engine\Plugins\Fab
  - Assume the engine folder must be named UE_5.6_sub


CREDITS
-------
  Plugin package built by Gam3rGoon
  Sources: Epic Games Fab plugin (UE 5.6), rebuilt for SN2 custom editor
  Engine: Buckminsterfullerene02 UnrealEngine sn2-v0.1.2.0 (Custom Engine V4)
