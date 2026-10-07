// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ContentBrowserDelegates.h"
#include "FabBrowserApi.h"
#include "FabBrowser.generated.h"

class SDockTab;
class SWebBrowser;
class FSpawnTabArgs;
class IWebBrowserWindow;
class UFabBrowserApi;
class UFabSettings;
class FSlateStyleSet;
class FExtender;

USTRUCT()
struct FFabAnalyticsEventValue
{
	GENERATED_BODY()

	UPROPERTY()
	FString Platform;
	
	UPROPERTY()
	FFabApiVersion ApiVersion;
};

USTRUCT()
struct FFabAnalyticsPayload
{
	GENERATED_BODY()

	UPROPERTY()
	FString InteractionType;

	UPROPERTY()
	FString EventCategory;

	UPROPERTY()
	FString EventAction;

	UPROPERTY()
	FString EventLabel;

	UPROPERTY()
	FString EventType;

	UPROPERTY()
	FFabAnalyticsEventValue EventValue;
};

class FFabBrowser
{
private:
	static TSharedPtr<SWebBrowser> WebBrowserInstance;
	static TObjectPtr<UFabBrowserApi> JavascriptApi;
	static TSharedPtr<SDockTab> DockTab;
	static TUniquePtr<FSlateStyleSet> SlateStyleSet;
	static TSharedPtr<IWebBrowserWindow> WebBrowserWindow;
	static TObjectPtr<const UFabSettings> FabPluginSettings;

	static const FName TabId;
	static const FText FabLabel;
	static const FText FabTooltip;
	static const FName FabMenuIconName;
	static const FName FabAssetIconName;
	static const FName FabToolbarIconName;

	static void RegisterSlateStyle();
	static void RegisterNomadTab();
	static void ExtendContextMenuInContentBrowser();
	static void SetupEntryPoints();
	static void ExecuteJavascript(const FString& InSrcScript);
	static TSharedRef<SDockTab> OpenTab(const FSpawnTabArgs& InArgs);
	static void OnPluginTabClosed(TSharedRef<class SDockTab> InParentTab);
	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets);
	static TSharedRef<SWidget> OnFabAssetIconGenerate(const FAssetData& AssetData);

public:
	static void Init();
	static void Shutdown();

	static void LogEvent(const FFabAnalyticsPayload& Payload);
	static void LoggedIn(const FString& InAccessToken);
	static void GetSignedUrl(const FString& AssetId, const int32 Tier);
	static TObjectPtr<UFabBrowserApi> GetBrowserApi() { return JavascriptApi; }
	static void ShowSettings();
	static void OpenURL(const FString& InURL = GetUrl());
	static FString GetUrl();
	static const ISlateStyle& GetStyleSet();
};
