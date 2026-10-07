// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FAssetUtils
{
public:
	static void SanitizeFolderName(FString& AssetID);
	static void SanitizePath(FString& Path);
	static bool Unzip(const FString& Path, const FString& TargetPath);
	static void ScanForAssets(const FString& FolderPath);
	static void SyncContentBrowserToFolder(const FString& FolderPath, const bool bFocusContentBrowser = true);
	static void SyncContentBrowserToFolders(const TArray<FString>& Folders, const bool bFocusContentBrowser = true);
};
