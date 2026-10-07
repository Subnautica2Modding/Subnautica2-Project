// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FabSettings.h"

class FFabAssetsCache
{
private:
	static int64 CacheExpirationTimeoutInDays;

public:
	static FString GetCacheLocation() { return GetDefault<UFabSettings>()->CacheDirectoryPath.Path; }
	static TArray<FString> GetCachedAssets();
	static int64 GetCacheSize();
	static FText GetCacheSizeString();
	static void ClearCache();
	static bool IsCached(const FString& AssetId, int64 DownloadSize);
	static FString GetCachedFile(const FString AssetId);
	static FString CacheAsset(const FString& DownloadedAssetPath);
};
