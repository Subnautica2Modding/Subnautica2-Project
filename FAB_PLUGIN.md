# Fab Plugin (SN2 Custom Editor)

**Built by:** Gam3rGoon

## Where to get it

1. **Already in this project**  
   Clone/use this fork and open it with the matching custom engine — Fab is included at [`Plugins/Fab`](Plugins/Fab) and enabled in `Subnautica2.uproject`.

   - Fork: https://github.com/whagan1310-droid/Subnautica2-Project  
   - Plugin path: `Plugins/Fab`  
   - Full install notes: [`Plugins/Fab/README_INSTALL.txt`](Plugins/Fab/README_INSTALL.txt)

2. **Or use it on any project with the same SN2 editor BuildId**  
   Copy `Plugins/Fab` into your engine or project (see directions below). It is **not** tied to a folder name like `UE_5.6_sub`.

## Required engine BuildId (must match)

```
a0f35de5-25c7-44b8-87b3-4fe5671dcf5c
```

- Engine folder **name does not matter**.
- BuildId **must** match.

Check:

`<YourEngine>/Engine/Binaries/Win64/UnrealEditor.modules`

Look for:

`"BuildId": "a0f35de5-25c7-44b8-87b3-4fe5671dcf5c"`

**Custom Engine V4 release:**  
https://github.com/Buckminsterfullerene02/UnrealEngine/releases/tag/sn2-v0.1.2.0

If the BuildId differs, this Fab build will not load. Stock Epic UE 5.6 Fab will also not work on this BuildId.

## Quick install (other projects / engine-wide)

1. Close the editor.
2. **Engine install (recommended):** copy `Plugins/Fab` → `<YourEngine>/Engine/Plugins/Fab`  
   (remove any old `Engine/Plugins/Fab` first)
3. Or **project install:** copy to `<YourProject>/Plugins/Fab`
4. Enable in `.uproject`:

```json
{
  "Name": "Fab",
  "Enabled": true,
  "TargetAllowList": [ "Editor" ]
}
```

5. Open the project with the engine that has the BuildId above.

## Verify

- Content Browser shows the **Fab** toolbar button  
- Log: `Mounting ... Fab` / `LogFab: Plugin initialized`  
- No “module Fab could not be found” / no “Skipping out-of-date modules” for Fab

## Not related

`Alterra_Transceiver_ClockBuildable` is a separate Habitat Builder pack. It is **not** required for Fab and Fab is **not** required for ClockBuildable.
