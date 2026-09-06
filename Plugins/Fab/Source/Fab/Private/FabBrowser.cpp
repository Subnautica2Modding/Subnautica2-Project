// Copyright Epic Games, Inc. All Rights Reserved.

#include "FabBrowser.h"

#include "ContentBrowserModule.h"
#include "FabAuthentication.h"
#include "FabLog.h"
#include "FabSettings.h"
#include "FabSettingsWindow.h"
#include "IWebBrowserPopupFeatures.h"
#include "IWebBrowserWindow.h"
#include "JsonObjectConverter.h"
#include "LevelEditor.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"
#include "WebBrowserModule.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxExtender.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"

#include "Interfaces/IMainFrameModule.h"
#include "Interfaces/IPluginManager.h"

#include "Math/Vector2D.h"

#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"

#include "Styling/CoreStyle.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

#include "UObject/GCObject.h"

#include "Utilities/FabLocalAssets.h"

#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Docking/SDockTab.h"

#define LOCTEXT_NAMESPACE "Fab"

TSharedPtr<SWebBrowser> FFabBrowser::WebBrowserInstance = nullptr;

TObjectPtr<UFabBrowserApi> FFabBrowser::JavascriptApi = nullptr;

TSharedPtr<SDockTab> FFabBrowser::DockTab                     = nullptr;
TUniquePtr<FSlateStyleSet> FFabBrowser::SlateStyleSet         = nullptr;
TSharedPtr<IWebBrowserWindow> FFabBrowser::WebBrowserWindow   = nullptr;
TObjectPtr<const UFabSettings> FFabBrowser::FabPluginSettings = nullptr;

const FName FFabBrowser::TabId = TEXT("FabTab");

const FText FFabBrowser::FabLabel           = LOCTEXT("Fab.Label", "Fab");
const FText FFabBrowser::FabTooltip         = LOCTEXT("Fab.Tooltip", "Get content from Fab");
const FName FFabBrowser::FabMenuIconName    = TEXT("Fab.MenuIcon");
const FName FFabBrowser::FabAssetIconName   = TEXT("Fab.AssetIcon");
const FName FFabBrowser::FabToolbarIconName = TEXT("Fab.ToolbarIcon");

void FFabBrowser::Init()
{
	RegisterSlateStyle();
	RegisterNomadTab();
	SetupEntryPoints();
	ExtendContextMenuInContentBrowser();
}

void FFabBrowser::ExtendContextMenuInContentBrowser()
{
	FContentBrowserModule& ContentBrowserModule                       = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FContentBrowserMenuExtender_SelectedAssets>& MenuExtenders = ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	FAssetViewExtraStateGenerator StateGenerator(
		FOnGenerateAssetViewExtraStateIndicators::CreateStatic(&FFabBrowser::OnFabAssetIconGenerate),
		FOnGenerateAssetViewExtraStateIndicators()
	);
	ContentBrowserModule.AddAssetViewExtraStateGenerator(StateGenerator);
	MenuExtenders.Add(FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FFabBrowser::OnExtendContentBrowserAssetSelectionMenu));
}

void FFabBrowser::RegisterSlateStyle()
{
	SlateStyleSet = MakeUnique<FSlateStyleSet>(TEXT("FabStyle"));
	SlateStyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("Fab"))->GetBaseDir() / TEXT("Resources"));

	const FString IconPath          = SlateStyleSet->RootToContentDir(TEXT("FabLogo.svg"));
	const FString AlternateIconPath = SlateStyleSet->RootToContentDir(TEXT("FabLogoAlternate.svg"));
	SlateStyleSet->Set(FabMenuIconName, new FSlateVectorImageBrush(IconPath, CoreStyleConstants::Icon16x16));
	SlateStyleSet->Set(FabAssetIconName, new FSlateVectorImageBrush(AlternateIconPath, CoreStyleConstants::Icon20x20));
	SlateStyleSet->Set(FabToolbarIconName, new FSlateVectorImageBrush(IconPath, CoreStyleConstants::Icon20x20));

	FSlateStyleRegistry::RegisterSlateStyle(*SlateStyleSet);

	FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

void FFabBrowser::SetupEntryPoints()
{
	const FUIAction InvokeTabAction = FUIAction(
		FExecuteAction::CreateLambda(
			[LevelEditorModuleName = FName("LevelEditor")]()
			{
				FModuleManager::GetModuleChecked<FLevelEditorModule>(LevelEditorModuleName).GetLevelEditorTabManager()->TryInvokeTab(TabId);
			}
		),
		FCanExecuteAction()
	);

	{
		FToolMenuSection& SaveSection = UToolMenus::Get()->ExtendMenu("ContentBrowser.Toolbar")->FindOrAddSection("Save");
		FToolMenuEntry& ToolMenuEntry = SaveSection.AddEntry(
			FToolMenuEntry::InitToolBarButton(
				"OpenFabWindow",
				InvokeTabAction,
				FabLabel,
				FabTooltip,
				FSlateIcon(SlateStyleSet->GetStyleSetName(), FabToolbarIconName),
				EUserInterfaceActionType::Button
			)
		);
		ToolMenuEntry.InsertPosition.Position = EToolMenuInsertType::Last;
		ToolMenuEntry.StyleNameOverride = "ContentBrowser.ToolBar.Buttons";
	}

	FToolMenuEntry FabMenuEntry = FToolMenuEntry::InitMenuEntry(
		"OpenFabTab",
		FabLabel,
		FabTooltip,
		FSlateIcon(SlateStyleSet->GetStyleSetName(), FabMenuIconName),
		InvokeTabAction
	);

	{
		UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
		FToolMenuSection& ContentSection = WindowMenu->FindOrAddSection("GetContent", NSLOCTEXT("MainAppMenu", "GetContentHeader", "Get Content"));
		FToolMenuEntry& FabEntry = ContentSection.AddEntry(FabMenuEntry);
		FabEntry.InsertPosition.Position = EToolMenuInsertType::First;
	}
	
	// Add a Fab entry to the Content Browser's Add popup menu
	UToolMenus::Get()->ExtendMenu(TEXT("ContentBrowser.AddNewContextMenu"))->AddSection(TEXT("ContentBrowserGetContent"), LOCTEXT("GetContentText", "Get Content")).AddEntry(
		FToolMenuEntry::InitMenuEntry(TEXT("OpenFabWindow"), FabLabel, FabTooltip, FSlateIcon(SlateStyleSet->GetStyleSetName(), FabMenuIconName), InvokeTabAction)
	);
	
	{
		FToolMenuEntry& FabEntry = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.AddQuickMenu")->FindOrAddSection("Content").AddEntry(FabMenuEntry);
		FabEntry.InsertPosition.Name = "ImportContent";
		FabEntry.InsertPosition.Position = EToolMenuInsertType::After;
	}
}

// Extend the context menu to view listings in Fab
TSharedRef<FExtender> FFabBrowser::OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
{
	TSharedRef<FExtender> Extender = MakeShared<FExtender>();

	if (SelectedAssets.Num() != 1)
	{
		return Extender;
	}
	const FAssetData AssetData = SelectedAssets[0];
	const FString ObjectPath   = AssetData.GetObjectPathString();
	FString FabListingId;
	UFabLocalAssets::GetListingID(ObjectPath, FabListingId);

	if (FabListingId.IsEmpty())
	{
		return Extender;
	}

	Extender->AddMenuExtension(
		"CommonAssetActions",
		EExtensionHook::After,
		nullptr,
		FMenuExtensionDelegate::CreateLambda(
			[FabListingId](FMenuBuilder& MenuBuilder)
			{
				MenuBuilder.AddMenuEntry(
					FText::FromString("View in Fab"),
					FText::FromString("View the asset in Fab plugin"),
					FSlateIcon(SlateStyleSet->GetStyleSetName(), FabMenuIconName),
					FUIAction(
						FExecuteAction::CreateLambda(
							[FabListingId]()
							{
								FFabBrowser::OpenURL(GetUrl() / "listings" / FabListingId);
							}
						)
					)
				);
			}
		)
	);
	return Extender;
}

TSharedRef<SWidget> FFabBrowser::OnFabAssetIconGenerate(const FAssetData& AssetData)
{
	const FSlateBrush* FabImage = nullptr;

	const FString ObjectPath = AssetData.GetObjectPathString();
	FString FabListingId;
	UFabLocalAssets::GetListingID(ObjectPath, FabListingId);

	if (!FabListingId.IsEmpty())
	{
		FabImage = SlateStyleSet->GetBrush(FabAssetIconName);
	}

	return SNew(SBox)
		.Padding(4.0f, 4.0f, 0.0f, 0.0f)
		.IsEnabled(FabImage != nullptr)
		[
			SNew(SImage)
			.Image(FabImage)
			.ToolTipText(FText::FromString("Imported from FAB"))
		];
}

void FFabBrowser::RegisterNomadTab()
{
	auto RegisterSpawner = [](TSharedPtr<ILevelEditor>)
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabId, FOnSpawnTab::CreateStatic(OpenTab)).SetAutoGenerateMenuEntry(false).SetDisplayName(FabLabel).
		                          SetTooltipTextAttribute(FabTooltip).SetIcon(FSlateIcon(SlateStyleSet->GetStyleSetName(), FabMenuIconName));
	};

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	if (LevelEditorModule.GetLevelEditorInstance().IsValid())
	{
		RegisterSpawner(LevelEditorModule.GetLevelEditorInstance().Pin());
	}
	else
	{
		LevelEditorModule.OnLevelEditorCreated().AddLambda(RegisterSpawner);
	}
}

FString FFabBrowser::GetUrl()
{
	if (FabPluginSettings == nullptr)
	{
		return TEXT("https://www.fab.com/plugins/ue5");
	}

	switch (FabPluginSettings->Environment)
	{
		default: // fall through
		case EFabEnvironment::Prod:  // fall through
		case EFabEnvironment::Gamedev: // fall through
		case EFabEnvironment::Test:  // fall through
		{
			FString Url = FabPluginSettings->GetUrlFromEnvironment();
			Url += TEXT("/plugins/ue5");
			return Url;
		}
		case EFabEnvironment::CustomUrl:
		{
			return FabPluginSettings->CustomUrl;
		}
	}
}

const ISlateStyle& FFabBrowser::GetStyleSet()
{
	return *(SlateStyleSet.Get());
}

TSharedRef<SDockTab> FFabBrowser::OpenTab(const FSpawnTabArgs&)
{
	FabAuthentication::LoginUsingPersist();
	
	FabPluginSettings = GetDefault<UFabSettings>();

	JavascriptApi = NewObject<UFabBrowserApi>();
	JavascriptApi->AddToRoot(); //Don't garbage collect

	IWebBrowserModule& WebBrowserModule = IWebBrowserModule::Get();
	if (!IWebBrowserModule::IsAvailable() || !WebBrowserModule.IsWebModuleAvailable())
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to load the plugin. Please enable Web WebBrowserWindow in the plugin manager to use Emporium."));
		return SNew(SDockTab).TabRole(ETabRole::NomadTab);
	}

	// Override the user agent to avoid issues with the project name
	// If the project name has the word "agent" in it, it'll cause the captcha to fail
	FWebBrowserInitSettings BrowserInitSettings = FWebBrowserInitSettings();
	BrowserInitSettings.ProductVersion = TEXT("UE_5.6");
	IWebBrowserModule::Get().CustomInitialize(BrowserInitSettings);

	FCreateBrowserWindowSettings WindowSettings;

	FString PluginPath              = IPluginManager::Get().FindPlugin(TEXT("Fab"))->GetBaseDir();
	FString IndexUrl                = FPaths::ConvertRelativePathToFull(FPaths::Combine(PluginPath, TEXT("ThirdParty"), TEXT("index.html")));
	FString FinalUrl                = FPaths::Combine(TEXT("file:///"), IndexUrl);
	WindowSettings.InitialURL       = FinalUrl;
	WindowSettings.BrowserFrameRate = 60;

	IWebBrowserSingleton* WebBrowserSingleton = WebBrowserModule.GetSingleton();
	WebBrowserSingleton->SetDevToolsShortcutEnabled(true);

	WebBrowserWindow = WebBrowserSingleton->CreateBrowserWindow(WindowSettings);
	WebBrowserWindow->OnUnhandledKeyUp().BindLambda([](const FKeyEvent&) { return true; });
	WebBrowserWindow->OnUnhandledKeyDown().BindLambda([](const FKeyEvent&) { return true; });
	WebBrowserWindow->OnUrlChanged().AddLambda(
		[](const FString& Url)
		{
			if (FabPluginSettings->Environment != EFabEnvironment::Prod)
				return;

			FString Domain, Protocol;
			Url.Split("://", &Protocol, &Domain);
			if (!Protocol.Contains("http"))
			{
				return;
			}

			if (int32 PathIndex; Domain.FindChar('/', PathIndex))
			{
				Domain = Domain.Left(PathIndex);
			}

			Domain = Domain.Replace(TEXT("www."), TEXT(""));

			if (!Domain.Contains("fab.com"))
			{
				FAB_LOG_ERROR("Trying to access thirdparty url [%s] in plugin browser. Redirecting back to fab.com", *Url);
				WebBrowserWindow->LoadURL(GetUrl());
				FPlatformProcess::LaunchURL(*Url, nullptr, nullptr);
			}
		}
	);

	if (FabPluginSettings->bEnableDebugOptions)
	{
		WebBrowserWindow.Get()->OnCreateWindow().BindLambda(
			[](const TWeakPtr<IWebBrowserWindow>& NewBrowserWindow, const TWeakPtr<IWebBrowserPopupFeatures>& PopupFeatures)
			{
				const TSharedRef<SWindow> DialogMainWindow = SNew(SWindow)
					.ClientSize(FVector2D(700, 700))
					.SupportsMaximize(true)
					.SupportsMinimize(true)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().HAlign(HAlign_Fill).VAlign(VAlign_Fill)
						[
							SNew(SWebBrowser, NewBrowserWindow.Pin())
						]
					];
				FSlateApplication::Get().AddWindow(DialogMainWindow);
				return true;
			}
		);
	}

	bool bShowAddressBar = FabPluginSettings->Environment == EFabEnvironment::CustomUrl;

	SAssignNew(WebBrowserInstance, SWebBrowser, WebBrowserWindow).ShowAddressBar(bShowAddressBar).ShowControls(bShowAddressBar);

	WebBrowserInstance->BindUObject(TEXT("fab"), Cast<UObject>(JavascriptApi), true);
	WebBrowserWindow->Reload();

	SAssignNew(DockTab, SDockTab).TabRole(ETabRole::NomadTab).OnTabClosed_Static(&FFabBrowser::OnPluginTabClosed)
	[
		WebBrowserInstance.ToSharedRef()
	];

	return DockTab.ToSharedRef();
}

void FFabBrowser::OnPluginTabClosed(TSharedRef<SDockTab> InParentTab)
{
	LogEvent(
		{
			"click",
			"button",
			"terminatePlugin",
			"closeFabPlugin",
			"interaction",
			{
				"Fab_UE5_Plugin",
				UFabBrowserApi::GetApiVersion()
			}
		}
	);

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, []()
	{
		// add a small delay so that the frontend can register the close event
		FPlatformProcess::Sleep(1.5f);

		AsyncTask(ENamedThreads::GameThread, []()
		{
			WebBrowserInstance->UnbindUObject(TEXT("fab"), Cast<UObject>(JavascriptApi), true);
			WebBrowserInstance.Reset();
			WebBrowserWindow.Reset();
			DockTab.Reset();
		});
	});
}

void FFabBrowser::ExecuteJavascript(const FString& InSrcScript)
{
	if (WebBrowserInstance == nullptr)
	{
		return;
	}

	WebBrowserInstance->ExecuteJavascript(InSrcScript);
}

void FFabBrowser::Shutdown()
{
	WebBrowserInstance.Reset();
	WebBrowserWindow.Reset();
	DockTab.Reset();
	FSlateStyleRegistry::UnRegisterSlateStyle(*SlateStyleSet);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabId);
}

void FFabBrowser::LoggedIn(const FString& InAccessToken)
{
	ExecuteJavascript(FString::Printf(TEXT("window.ue.fab.onLoginSuccessful('%s');"), *InAccessToken));
}

void FFabBrowser::GetSignedUrl(const FString& AssetId, const int32 Tier)
{
	ExecuteJavascript(FString::Printf(TEXT("window.ue.fab.getSignedUrl('%s', %d)"), *AssetId, Tier));
}

void FFabBrowser::LogEvent(const FFabAnalyticsPayload& Payload)
{
	FString JSONPayload;
	FJsonObjectConverter::UStructToJsonObjectString(Payload, JSONPayload, 0, 0, 0, nullptr, false);

	ExecuteJavascript(FString::Printf(TEXT("window.ue.fab.logevent('%s')"), *JSONPayload));
}

void FFabBrowser::ShowSettings()
{
	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("FabSettingsLabel", "Fab Settings"))
		.ClientSize(FVector2D(600.f, 300.f))
		.SizingRule(ESizingRule::UserSized);

	TSharedPtr<SFabSettingsWindow> SettingsWindow;
	Window->SetContent(SAssignNew(SettingsWindow, SFabSettingsWindow).WidgetWindow(Window));
	TSharedPtr<SWindow> ParentWindow;
	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		const IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow                      = MainFrame.GetParentWindow();
	}

	FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);
}

void FFabBrowser::OpenURL(const FString& InURL)
{
	FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor").GetLevelEditorTabManager()->TryInvokeTab(TabId);
	if (WebBrowserWindow.Get()->GetUrl() != InURL)
	{
		WebBrowserWindow.Get()->LoadURL(InURL);
	}
}

#undef LOCTEXT_NAMESPACE
