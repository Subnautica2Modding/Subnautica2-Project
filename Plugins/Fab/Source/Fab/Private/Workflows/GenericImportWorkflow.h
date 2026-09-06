// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FabDownloader.h"
#include "FabWorkflow.h"
#include "NotificationProgressWidget.h"
#include "AssetRegistry/AssetData.h"

class FGenericImportWorkflow : public IFabWorkflow
{
public:
	FGenericImportWorkflow(const FString& InAssetId, const FString& InAssetName, const FString& InDownloadURL);

	virtual void Execute() override;

protected:
	virtual void ImportContent(const TArray<FString>& SourceFiles) override;
	virtual void DownloadContent() override;

	virtual void OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) override;
	virtual void OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) override;

	virtual void CompleteWorkflow() override;

private:
	void CreateDownloadNotification();
	void SetDownloadNotificationProgress(const float Progress);
	void ExpireDownloadNotification(bool bSuccess);

	void CreateImportNotification();
	void ExpireImportNotification(bool bSuccess);

protected:
	bool bIsDragDropWorkflow = false;

private:
	TSharedPtr<FFabDownloadRequest> DownloadRequest;

	TSharedPtr<SNotificationItem> DownloadProgressNotification;
	TSharedPtr<SNotificationItem> ImportProgressNotification;
	TSharedPtr<SNotificationProgressWidget> ProgressWidget;
};
