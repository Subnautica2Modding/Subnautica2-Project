// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FabWorkflow.h"

#include "AssetRegistry/AssetData.h"

#include "NotificationProgressWidget.h"

struct FFabDownloadStats;
class FFabDownloadRequest;

class FQuixelImportWorkflow : public IFabWorkflow
{
public:
	FQuixelImportWorkflow(const FString& InAssetId, const FString& InAssetName, const FString& InDownloadURL);

	virtual void Execute() override;

protected:
	virtual void ImportContent(const TArray<FString>& SourceFiles) override;
	virtual void DownloadContent() override;

	virtual void OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) override;
	virtual void OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats) override;

	virtual void CompleteWorkflow() override;

private:
	void CreateDownloadNotification();
	void SetDownloadNotificationProgress(const float Progress) const;
	void ExpireDownloadNotification(bool bSuccess) const;

	void CreateImportNotification();
	void ExpireImportNotification(bool bSuccess) const;

protected:
	bool bIsDragDropWorkflow = false;

protected:
	TSharedPtr<FFabDownloadRequest> DownloadRequest;

	TSharedPtr<SNotificationItem> DownloadProgressNotification;
	TSharedPtr<SNotificationItem> ImportProgressNotification;
	TSharedPtr<SNotificationProgressWidget> ProgressWidget;
};
