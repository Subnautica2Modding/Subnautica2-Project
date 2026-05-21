#include "SN2ModToolsSettings.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

static const TCHAR* SteamLibraryRoots[] = {
	TEXT("C:/Program Files (x86)/Steam/steamapps/common/Subnautica2"),
	TEXT("C:/Program Files/Steam/steamapps/common/Subnautica2"),
	TEXT("D:/SteamLibrary/steamapps/common/Subnautica2"),
	TEXT("E:/SteamLibrary/steamapps/common/Subnautica2"),
};

USN2ModToolsSettings::USN2ModToolsSettings()
{
	if (GameDir.Path.IsEmpty())
	{
		GameDir.Path = DetectGameDir();
	}
}

FString USN2ModToolsSettings::DetectGameDir()
{
	for (const TCHAR* Root : SteamLibraryRoots)
	{
		if (FPaths::FileExists(FString(Root) / TEXT("Subnautica2.exe")))
		{
			return Root;
		}
	}
	return FString();
}
