#pragma once

#include "CoreMinimal.h"

namespace ModActions
{
	bool CreateMod(const FString& ModName);

	void CookAndInstallMod(const FString& ModName);

	void UninstallMod(const FString& ModName);

	// "/Game/Mods/MyMod" -> "MyMod"; empty for anything else.
	FString ModNameFromFolderPath(const FString& FolderPath);
}
