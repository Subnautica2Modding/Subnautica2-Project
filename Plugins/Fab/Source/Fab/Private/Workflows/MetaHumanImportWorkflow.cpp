// Copyright Epic Games, Inc. All Rights Reserved.

#include "MetaHumanImportWorkflow.h"

#include "FabLog.h"
#include "IAssetTools.h"

#include "Engine/StaticMesh.h"

#include "Framework/Notifications/NotificationManager.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"

#include "Importers/GenericAssetImporter.h"

#include "Misc/MessageDialog.h"

#include "Utilities/AssetUtils.h"
#include "Utilities/FabAssetsCache.h"
#include "Utilities/FabLocalAssets.h"

#include "Widgets/Notifications/SNotificationList.h"

FMetaHumanImportWorkflow::FMetaHumanImportWorkflow(const FString& InAssetId, const FString& InAssetName, const FString& InDownloadURL)
	: IFabWorkflow(InAssetId, InAssetName, InDownloadURL)
{}

void FMetaHumanImportWorkflow::Execute()
{
	DownloadContent();
}

void FMetaHumanImportWorkflow::DownloadContent()
{
	CreateDownloadNotification();

	const FString DownloadLocation = FFabAssetsCache::GetCacheLocation() / AssetId;

	DownloadRequest = MakeShared<FFabDownloadRequest>(AssetId, DownloadUrl, DownloadLocation, EFabDownloadType::HTTP);
	DownloadRequest->OnDownloadProgress().AddRaw(this, &FMetaHumanImportWorkflow::OnContentDownloadProgress);
	DownloadRequest->OnDownloadComplete().AddRaw(this, &FMetaHumanImportWorkflow::OnContentDownloadComplete);
	DownloadRequest->ExecuteRequest();
}

void FMetaHumanImportWorkflow::OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats)
{
	SetDownloadNotificationProgress(DownloadStats.PercentComplete);
}

void FMetaHumanImportWorkflow::OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats)
{
	if (!DownloadStats.bIsSuccess)
	{
		FAB_LOG_ERROR("Failed to download FAB Asset %s", *AssetName);
		ExpireDownloadNotification(false);
		CancelWorkflow();
		return;
	}
	ExpireDownloadNotification(true);

	ImportContent(DownloadStats.DownloadedFiles);
}

void FMetaHumanImportWorkflow::CreateDownloadNotification()
{
	// Create the notification info
	FNotificationInfo Info(FText::FromString("Downloading..."));

	ProgressWidget = SNew(SNotificationProgressWidget)
		.ProgressText(FText::FromString("Downloading " + AssetName));

	// Set up the notification properties
	Info.bFireAndForget                   = false; // We want to control when it disappears
	Info.FadeOutDuration                  = 1.0f;  // Duration of the fade-out
	Info.ExpireDuration                   = 0.0f;  // How long it stays on the screen
	Info.bUseThrobber                     = true;  // Adds a spinning throbber to the notification
	Info.bUseSuccessFailIcons             = true;  // Adds success/failure icons
	Info.bAllowThrottleWhenFrameRateIsLow = false; // Ensures it updates even if the frame rate is low
	Info.bUseLargeFont                    = false; // Uses the default font size
	Info.ContentWidget                    = ProgressWidget;

	DownloadProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);

	if (DownloadProgressNotification.IsValid() && ProgressWidget)
	{
		DownloadProgressNotification->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FMetaHumanImportWorkflow::SetDownloadNotificationProgress(const float Progress)
{
	if (Progress > 100.0f || Progress < 0.0f)
	{
		return;
	}
	if (DownloadProgressNotification.IsValid() && ProgressWidget)
	{
		ProgressWidget->SetProgressPercent(Progress);
	}
}

void FMetaHumanImportWorkflow::ExpireDownloadNotification(bool bSuccess)
{
	if (DownloadProgressNotification.IsValid())
	{
		DownloadProgressNotification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		DownloadProgressNotification->ExpireAndFadeout();
	}
}

void FMetaHumanImportWorkflow::CreateImportNotification()
{
	// Create the notification info
	FNotificationInfo Info(FText::FromString("Importing..."));

	// Set up the notification properties
	Info.bFireAndForget                   = false; // We want to control when it disappears
	Info.FadeOutDuration                  = 1.0f;  // Duration of the fade-out
	Info.ExpireDuration                   = 0.0f;  // How long it stays on the screen
	Info.bUseThrobber                     = true;  // Adds a spinning throbber to the notification
	Info.bUseSuccessFailIcons             = true;  // Adds success/failure icons
	Info.bAllowThrottleWhenFrameRateIsLow = false; // Ensures it updates even if the frame rate is low
	Info.bUseLargeFont                    = false; // Uses the default font size

	ImportProgressNotification = FSlateNotificationManager::Get().AddNotification(Info);

	if (ImportProgressNotification.IsValid())
	{
		ImportProgressNotification->SetCompletionState(SNotificationItem::CS_Pending);
	}
}

void FMetaHumanImportWorkflow::ExpireImportNotification(bool bSuccess)
{
	if (ImportProgressNotification.IsValid())
	{
		ImportProgressNotification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		ImportProgressNotification->ExpireAndFadeout();
	}
}

void FMetaHumanImportWorkflow::ImportContent(const TArray<FString>& ImportFiles)
{
	const TArray<UObject*> ImportedAssets = IAssetTools::Get().ImportAssets(ImportFiles, "/Game/Fab/MetaHuman");
	if (ImportedAssets.Num())
	{
		CompleteWorkflow();
	}
	else
	{
		CancelWorkflow();
	}
}
