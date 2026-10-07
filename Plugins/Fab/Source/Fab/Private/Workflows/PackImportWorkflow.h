// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FabWorkflow.h"

class SNotificationItem;
class SNotificationProgressWidget;

#include "AssetRegistry/AssetData.h"

class FPackImportWorkflow : public IFabWorkflow
{
public:
	FPackImportWorkflow(const FString& InAssetId, const FString& InAssetName, const FString& InManifestDownloadUrl, const FString& InBaseUrls);

	virtual void Execute() override;

protected:
	virtual void DownloadContent() override;

	virtual void OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) override;
	virtual void OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) override;

private:
	void CreateDownloadNotification();
	void SetDownloadNotificationProgress(const float Progress) const;
	void ExpireDownloadNotification(bool bSuccess) const;

private:
	FString BaseUrls;

	TSharedPtr<FFabDownloadRequest> DownloadRequest;

	TSharedPtr<SNotificationItem> DownloadProgressNotification;
	TSharedPtr<SNotificationItem> ImportProgressNotification;
	TSharedPtr<SNotificationProgressWidget> ProgressWidget;
};
