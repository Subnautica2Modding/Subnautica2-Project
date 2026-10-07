// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Queue.h"

#include "Containers/Ticker.h"

#include "Importers/BuildPatchInstallerLibHelper.h"

#include "Interfaces/IHttpRequest.h"

enum class EFabDownloadType
{
	// Download asset using HTTP
	HTTP,

	// Download asset using BuildPatchServices (for Unreal Engine Marketplace Assets)
	BuildPatchRequest
};

struct FFabDownloadStats
{
	float PercentComplete = 0.0f;

	uint64 CompletedBytes = 0;
	uint64 TotalBytes     = 0;

	uint64 DownloadStartedAt   = 0;
	uint64 DownloadCompletedAt = 0;

	float DownloadSpeed = 0.0f;

	bool bIsSuccess = false;
	bool bIsCached = false;
	
	TArray<FString> DownloadedFiles;
};

class FFabDownloadRequest
{
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDownloadProgress, const FFabDownloadRequest*, const FFabDownloadStats&);
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDownloadComplete, const FFabDownloadRequest*, const FFabDownloadStats&);

private:
	FString GetFilenameFromURL(const FString& URL);

	void ExecuteHTTPRequest();

	static bool LoadBuildPatchServices();
	void ExecuteBuildPatchRequest();
	void OnManifestDownloaded(const FString& BaseURL);

	void StartDownload();

public:
	FFabDownloadRequest(const FString& AssetID, const FString& InDownloadURL, const FString& InDownloadLocation, EFabDownloadType InDownloadType = EFabDownloadType::HTTP);

	~FFabDownloadRequest() = default;

	void ExecuteRequest();
	void Cancel();

	static void ShutdownBpsModule();

	const FFabDownloadStats& GetDownloadStats() { return DownloadStats; }

	FOnDownloadProgress& OnDownloadProgress() { return OnDownloadProgressDelegate; }
	FOnDownloadComplete& OnDownloadComplete() { return OnDownloadCompleteDelegate; }

private:
	FString AssetID;

	FString DownloadURL;

	FString DownloadLocation;

	EFabDownloadType DownloadType;

	FFabDownloadStats DownloadStats;

	FOnDownloadProgress OnDownloadProgressDelegate;
	FOnDownloadComplete OnDownloadCompleteDelegate;

	FHttpRequestPtr DownloadRequest;
	IBuildInstallerPtr BpsInstaller;

	bool bPendingCancel = false;

	TArray<uint8> ManifestData;
	FTSTicker::FDelegateHandle BpsProgressTickerHandle;
	static FTSTicker::FDelegateHandle BpsTickerHandle;
	static TUniquePtr<BpiLib::IBpiLib> BuildPatchServices;

	friend class FFabDownloadQueue;
};

class FFabDownloadQueue
{
private:
	static int32 DownloadQueueLimit;
	static TSet<FFabDownloadRequest*> DownloadQueue;
	static TQueue<FFabDownloadRequest*> WaitingQueue;

public:
	static void AddDownloadToQueue(FFabDownloadRequest* DownloadRequest);
};
