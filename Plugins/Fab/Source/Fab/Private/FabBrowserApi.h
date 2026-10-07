// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "FabBrowserApi.generated.h"

class IFabWorkflow;

USTRUCT()
struct FFabAssetMetadata
{
	GENERATED_BODY()

	UPROPERTY()
	FString AssetId;

	UPROPERTY()
	FString AssetName;

	UPROPERTY()
	FString AssetType;

	UPROPERTY()
	FString ListingType;

	UPROPERTY()
	bool IsQuixel = false;

	UPROPERTY()
	FString AssetNamespace;

	UPROPERTY()
	TArray<FString> DistributionPointBaseUrls;
};

USTRUCT()
struct FFabApiVersion
{
	GENERATED_BODY()

	UPROPERTY()
	FString Ue;

	UPROPERTY()
	FString Api;

	UPROPERTY()
	FString PluginVersion;
	
	UPROPERTY()
	FString Platform;
};

USTRUCT()
struct FFabFrontendSettings
{
	GENERATED_BODY()

	UPROPERTY()
	FString PreferredFormat;

	UPROPERTY()
	FString PreferredQuality;
};

UCLASS()
class UFabBrowserApi : public UObject
{
	GENERATED_BODY()
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSignedUrlGenerated, const FString& /*DownloadUrl*/, FFabAssetMetadata /*Metadata*/);

private:
	FOnSignedUrlGenerated OnSignedUrlGeneratedDelegate;
	void CompleteWorkflow(const FString& Id);

public:
	TArray<TSharedPtr<IFabWorkflow>> ActiveWorkflows;

public:
	UFUNCTION()
	void AddToProject(const FString& DownloadUrl, const FFabAssetMetadata& AssetMetadata);

	UFUNCTION()
	void DragStart(const FFabAssetMetadata& AssetMetadata);

	UFUNCTION()
	void OnDragInfoSuccess(const FString& DownloadUrl, const FFabAssetMetadata& AssetMetadata);

	UFUNCTION()
	void OnDragInfoFailure(const FString& AssetId);

	UFUNCTION()
	void Login();

	UFUNCTION()
	void Logout();

	UFUNCTION()
	FString GetAuthToken();

	UFUNCTION()
	FString GetRefreshToken();

	UFUNCTION()
	void OpenPluginSettings();

	UFUNCTION()
	FFabFrontendSettings GetSettings();

	UFUNCTION()
	void SetPreferredQualityTier(const FString& PreferredQuality);

	UFUNCTION()
	static FFabApiVersion GetApiVersion();

	UFUNCTION()
	void OpenUrlInBrowser(const FString& Url);

	UFUNCTION()
	void CopyToClipboard(const FString& Content);

	UFUNCTION()
	void PluginOpened();
	
	UFUNCTION()
	FString GetUrl();

	FDelegateHandle AddSignedUrlCallback(TFunction<void(const FString&, const FFabAssetMetadata&)> Callback);
	FOnSignedUrlGenerated& OnSignedUrlGenerated() { return OnSignedUrlGeneratedDelegate; }
	void RemoveSignedUrlHandle(const FDelegateHandle& Handle) { OnSignedUrlGeneratedDelegate.Remove(Handle); }
};
