#include "SN2ModToolsSettings.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

USN2ModToolsSettings::USN2ModToolsSettings()
{
	if (GameDir.Path.IsEmpty())
	{
		GameDir.Path = DetectGameDir();
	}
}

FString USN2ModToolsSettings::DetectGameDir()
{
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	const FString Contents = ProjectPath + TEXT("GameInstallDirectory.txt");
	if (FPaths::FileExists(Contents))
	{
		FString GameDirPath;
		if (FFileHelper::LoadFileToString(GameDirPath, *Contents))
		{
			TArray<FString> Lines;
			GameDirPath.ParseIntoArrayLines(Lines);
			for (FString& Line : Lines)
			{
				Line.TrimStartAndEndInline();
				if (!Line.IsEmpty() && !Line.StartsWith(TEXT("#")) && !Line.StartsWith(TEXT(";")))
				{
					Line = FPaths::GetPath(Line);
					if (FPaths::FileExists(Line / TEXT("Subnautica2.exe")))
					{
						return Line;
					}
				}
			}
		}
	}
	
	return FString();
}
