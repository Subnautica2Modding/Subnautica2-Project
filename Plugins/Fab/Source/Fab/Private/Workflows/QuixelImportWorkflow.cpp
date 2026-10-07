// Copyright Epic Games, Inc. All Rights Reserved.

#include "QuixelImportWorkflow.h"

#include "FabDownloader.h"
#include "FabLog.h"
#include "NotificationProgressWidget.h"

#include "Engine/StaticMesh.h"

#include "Framework/Notifications/NotificationManager.h"

#include "Importers/QuixelGLTFImporter.h"

#include "HAL/PlatformFileManager.h"

#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "MaterialTypes.h"

#include "Utilities/AssetUtils.h"
#include "Utilities/FabAssetsCache.h"
#include "Utilities/FabLocalAssets.h"
#include "Utilities/QuixelAssetTypes.h"

#include "Widgets/Notifications/SNotificationList.h"

FString ExtractTierNameFromFilename(const FString& FileName)
{
	if (FileName.IsEmpty())
		return "";

	// Get clean filename without extension
	const FString CleanFileName = FPaths::GetBaseFilename(FileName);

	// Split filename by '_'
	TArray<FString> SplitString;
	CleanFileName.ParseIntoArray(SplitString, TEXT("_"), true);

	// Pick last part (tier)
	const FString TierString = SplitString.Last();

	// Convert the extracted string to an integer
	const int32 Tier = TierString.IsNumeric() ? FCString::Atoi(*TierString) : -1;
	if (Tier == 0)
		return "Raw";
	if (Tier == 1)
		return "High";
	if (Tier == 2)
		return "Medium";
	if (Tier == 3)
		return "Low";

	return "";
}

FQuixelImportWorkflow::FQuixelImportWorkflow(const FString& InAssetId, const FString& InAssetName, const FString& InDownloadURL)
	: IFabWorkflow(InAssetId, InAssetName, InDownloadURL)
{}

void FQuixelImportWorkflow::Execute()
{
	DownloadContent();
}

void FQuixelImportWorkflow::DownloadContent()
{
	const FString DownloadLocation = FFabAssetsCache::GetCacheLocation() / AssetId;

	DownloadRequest = MakeShared<FFabDownloadRequest>(AssetId, DownloadUrl, DownloadLocation, EFabDownloadType::HTTP);
	DownloadRequest->OnDownloadProgress().AddRaw(this, &FQuixelImportWorkflow::OnContentDownloadProgress);
	DownloadRequest->OnDownloadComplete().AddRaw(this, &FQuixelImportWorkflow::OnContentDownloadComplete);
	DownloadRequest->ExecuteRequest();

	CreateDownloadNotification();
}

void FQuixelImportWorkflow::OnContentDownloadComplete(const FFabDownloadRequest* Request, const FFabDownloadStats& Stats)
{
	if (!Stats.bIsSuccess)
	{
		FAB_LOG_ERROR("Failed to download Megascans Asset %s", *AssetId);
		ExpireDownloadNotification(false);
		CancelWorkflow();
		return;
	}

	const FString& ZipArchive     = Stats.DownloadedFiles[0];
	const FString ExtractLocation = FPaths::GetBaseFilename(ZipArchive, false) + "_extracted";
	if (!FAssetUtils::Unzip(ZipArchive, ExtractLocation))
	{
		FAB_LOG_ERROR("Failed to unzip Megascans Asset %s", *AssetId);
		ExpireDownloadNotification(false);
		CancelWorkflow();
		return;
	}

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();

	TArray<FString> ImportFiles;
	FileManager.FindFiles(ImportFiles, *ExtractLocation, TEXT(".gltf"));
	FileManager.FindFiles(ImportFiles, *ExtractLocation, TEXT(".json"));

	if (ImportFiles.Num() != 2)
	{
		FAB_LOG_ERROR("Import files not found for %s", *AssetId);
		ExpireDownloadNotification(false);
		CancelWorkflow();
		return;
	}

	ExpireDownloadNotification(true);
	ImportContent(ImportFiles);
}

void FQuixelImportWorkflow::CompleteWorkflow()
{
	FAssetUtils::SyncContentBrowserToFolder(ImportLocation, !bIsDragDropWorkflow);
	IFabWorkflow::CompleteWorkflow();
}

void FQuixelImportWorkflow::OnContentDownloadProgress(const FFabDownloadRequest* Request, const FFabDownloadStats& DownloadStats)
{
	SetDownloadNotificationProgress(DownloadStats.PercentComplete);
}

void FQuixelImportWorkflow::ImportContent(const TArray<FString>& SourceFiles)
{
	const FString SourceFile = SourceFiles[0];
	const FString MetaFile   = SourceFiles[1];

	auto [MegascanId, SubType] = FQuixelAssetTypes::ExtractMeta(MetaFile, SourceFile);
	const FString TierString   = ExtractTierNameFromFilename(SourceFile);

	ImportLocation = "/Game/Fab/Megascans" / SubType / AssetName + '_' + MegascanId / TierString;
	FAssetUtils::SanitizePath(ImportLocation);

	CreateImportNotification();

	auto OnDone = [this, MegascanId](const TArray<UObject*>& Objects)
	{
		if (!Objects.IsEmpty())
		{
			ImportedObjects = Objects;
			ExpireImportNotification(true);
			UFabLocalAssets::AddLocalAsset(FPaths::GetPath(ImportLocation), AssetId);
			CompleteWorkflow();
		}
		else
		{
			FAB_LOG_ERROR("Failed to import Megascan asset: %s [%s]", *MegascanId, *AssetId);
			ExpireImportNotification(false);
			CancelWorkflow();
		}
	};

	if (SubType == "3D")
	{
		FQuixelGltfImporter::ImportGltf3DAsset(SourceFile, ImportLocation, OnDone);
	}
	else if (SubType == "Plants")
	{
		FQuixelGltfImporter::ImportGltfPlantAsset(SourceFile, ImportLocation, TierString == "Raw", OnDone);
	}
	else if (SubType == "Decals")
	{
		FQuixelGltfImporter::ImportGltfDecalAsset(SourceFile, ImportLocation, OnDone);
	}
	else if (SubType == "Imperfections")
	{
		FQuixelGltfImporter::ImportGltfImperfectionAsset(SourceFile, ImportLocation, OnDone);
	}
	else if (SubType == "Surfaces")
	{
		FQuixelGltfImporter::ImportGltfSurfaceAsset(SourceFile, ImportLocation, OnDone);
	}
	else
	{
		FAB_LOG_ERROR("Invalid Quixel asset type: %s", *SubType);
	}
}

void FQuixelImportWorkflow::CreateDownloadNotification()
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

void FQuixelImportWorkflow::SetDownloadNotificationProgress(const float Progress) const
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

void FQuixelImportWorkflow::ExpireDownloadNotification(bool bSuccess) const
{
	if (DownloadProgressNotification.IsValid())
	{
		DownloadProgressNotification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		DownloadProgressNotification->ExpireAndFadeout();
	}
}

void FQuixelImportWorkflow::CreateImportNotification()
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

void FQuixelImportWorkflow::ExpireImportNotification(bool bSuccess) const
{
	if (ImportProgressNotification.IsValid())
	{
		ImportProgressNotification->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
		ImportProgressNotification->ExpireAndFadeout();
	}
}
