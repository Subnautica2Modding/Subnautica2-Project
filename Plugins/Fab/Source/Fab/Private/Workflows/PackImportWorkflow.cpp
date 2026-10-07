// Copyright Epic Games, Inc. All Rights Reserved.

#include "PackImportWorkflow.h"

#include "FabDownloader.h"
#include "FabLog.h"
#include "NotificationProgressWidget.h"

#include "Framework/Notifications/NotificationManager.h"

#include "HAL/FileManager.h"

#include "Misc/FileHelper.h"

#include "Utilities/AssetUtils.h"
#include "Utilities/FabLocalAssets.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SNotificationList.h"

FPackImportWorkflow::FPackImportWorkflow(const FString& InAssetId, const FString& InAssetName, const FString& InManifestDownloadUrl, const FString& InBaseUrls)
	: IFabWorkflow(InAssetId, InAssetName, InManifestDownloadUrl)
	, BaseUrls(InBaseUrls)
{}

void FPackImportWorkflow::Execute()
{
	DownloadContent();
}

void FPackImportWorkflow::DownloadContent()
{
	const FString DownloadURL      = DownloadUrl + ',' + BaseUrls;
	const FString DownloadLocation = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	DownloadRequest = MakeShared<FFabDownloadRequest>(AssetId, DownloadURL, DownloadLocation, EFabDownloadType::BuildPatchRequest);
	DownloadRequest->OnDownloadComplete().AddRaw(this, &FPackImportWorkflow::OnContentDownloadComplete);
	DownloadRequest->OnDownloadProgress().AddRaw(this, &FPackImportWorkflow::OnContentDownloadProgress);
	DownloadRequest->ExecuteRequest();

	CreateDownloadNotification();
}

void FPackImportWorkflow::OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats)
{
	SetDownloadNotificationProgress(DownloadStats.PercentComplete);
}

void FPackImportWorkflow::OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats)
{
	if (!DownloadStats.bIsSuccess || DownloadStats.DownloadedFiles.IsEmpty())
	{
		ExpireDownloadNotification(false);
		CancelWorkflow();
		return;
	}

	ExpireDownloadNotification(true);

	TArray<FString> PathParts;
	DownloadStats.DownloadedFiles[0].ParseIntoArray(PathParts, TEXT("/"));

	if (PathParts.Num() >= 2)
	{
		ImportLocation = "/Game" / PathParts[1];
		UFabLocalAssets::AddLocalAsset(ImportLocation, AssetId);
		FAssetUtils::ScanForAssets(ImportLocation);
		FAssetUtils::SyncContentBrowserToFolder(ImportLocation);
	}

	CompleteWorkflow();
}

void FPackImportWorkflow::CreateDownloadNotification()
{
	// Create the notification info
	FNotificationInfo Info(FText::FromString("Downloading..."));

	ProgressWidget = SNew(SNotificationProgressWidget)
		.ProgressText(FText::FromString("Downloading " + AssetName))
		.HasButton(true)
		.ButtonText(FText::FromString("Cancel"))
		.ButtonToolTip(FText::FromString("Cancel Pack Import"))
		.OnButtonClicked(
			FOnClicked::CreateLambda(
				[this]()
				{
					FAB_LOG("Import Cancelled");
					if (ProgressWidget.IsValid())
					{
						FAB_LOG("Disabling cancel button");
						ProgressWidget->DisableCancel();
					}	
					if (DownloadRequest)
					{
						DownloadRequest->Cancel();
					}
					return FReply::Handled();
				}
			)
		);

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

void FPackImportWorkflow::SetDownloadNotificationProgress(const float Progress) const
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

void FPackImportWorkflow::ExpireDownloadNotification(bool bSuccess) const
{
	if (DownloadProgressNotification.IsValid())
	{
		DownloadProgressNotification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		DownloadProgressNotification->ExpireAndFadeout();
	}
}
