#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserDelegates.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISettingsModule.h"
#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Brushes/SlateImageBrush.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"

#include "NewModDialog.h"
#include "ModActions.h"

class FSN2ModToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		RegisterStyle();

		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(
				this, &FSN2ModToolsModule::RegisterMenus));

		FContentBrowserModule& CBModule =
			FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		// No `this` capture, so the delegate survives module unload.
		auto Extender = FContentBrowserMenuExtender_SelectedPaths::CreateStatic(
			&FSN2ModToolsModule::ExtendPathContextMenu);
		PathExtenderHandle = Extender.GetHandle();
		CBModule.GetAllPathViewContextMenuExtenders().Add(MoveTemp(Extender));
	}

	virtual void ShutdownModule() override
	{
		UToolMenus::UnregisterOwner(this);

		if (FContentBrowserModule* CB =
			FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser")))
		{
			CB->GetAllPathViewContextMenuExtenders().RemoveAll(
				[H = PathExtenderHandle](const FContentBrowserMenuExtender_SelectedPaths& D)
				{
					return D.GetHandle() == H;
				});
		}

		UnregisterStyle();
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);

		UToolMenu* MenuBar = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& Section = MenuBar->FindOrAddSection(TEXT("ModdingSection"));
		Section.AddSubMenu(
			TEXT("ModdingMenu"),
			FText::FromString(TEXT("Modding")),
			FText::FromString(TEXT("Subnautica 2 mod tools")),
			FNewMenuDelegate::CreateRaw(this, &FSN2ModToolsModule::BuildModdingMenu),
			false,
			FSlateIcon());

		UToolMenu* PlayToolbar =
			UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
		FToolMenuSection& PlaySection = PlayToolbar->FindOrAddSection(TEXT("Play"));

		FToolMenuEntry ToolbarEntry = FToolMenuEntry::InitComboButton(
			TEXT("SN2ModToolsToolbar"),
			FUIAction(),
			FOnGetContent::CreateRaw(this, &FSN2ModToolsModule::BuildToolbarMenuWidget),
			FText::FromString(TEXT("Mod Tools")),
			FText::FromString(TEXT("Subnautica 2 mod tools")),
			FSlateIcon(TEXT("SN2ModToolsStyle"), TEXT("SN2ModTools.ToolbarIcon")),
			false,
			TEXT("SN2ModToolsToolbar"));
		ToolbarEntry.StyleNameOverride = TEXT("CalloutToolbar");
		PlaySection.AddEntry(ToolbarEntry);
	}

	void RegisterStyle()
	{
		if (StyleSet.IsValid())
		{
			return;
		}

		const TSharedPtr<IPlugin> Plugin =
			IPluginManager::Get().FindPlugin(TEXT("SN2ModTools"));
		if (!Plugin.IsValid())
		{
			return;
		}

		StyleSet = MakeShared<FSlateStyleSet>(TEXT("SN2ModToolsStyle"));
		StyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));
		StyleSet->Set(
			TEXT("SN2ModTools.ToolbarIcon"),
			new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Icon200.png")), FVector2D(20.f, 20.f)));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
	}

	void UnregisterStyle()
	{
		if (!StyleSet.IsValid())
		{
			return;
		}

		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		StyleSet.Reset();
	}

	static void LaunchSubnautica2()
	{
		const FString Args = TEXT("/C start \"\" \"steam://rungameid/1962700\"");
		FPlatformProcess::CreateProc(
			TEXT("cmd.exe"),
			*Args,
			true,
			false,
			false,
			nullptr,
			0,
			nullptr,
			nullptr,
			nullptr);
	}

	TSharedRef<SWidget> BuildToolbarMenuWidget()
	{
		FMenuBuilder MenuBuilder(true, nullptr);
		BuildModdingMenu(MenuBuilder);
		return MenuBuilder.MakeWidget();
	}

	void BuildModdingMenu(FMenuBuilder& Builder)
	{
		Builder.AddMenuEntry(
			FText::FromString(TEXT("Launch Subnautica 2 (steam only)")),
			FText::FromString(TEXT("Run Steam game id 1962700")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateStatic(&FSN2ModToolsModule::LaunchSubnautica2)));

		Builder.AddSeparator();

		Builder.AddMenuEntry(
			FText::FromString(TEXT("New Mod...")),
			FText::FromString(TEXT("Create a new mod folder with ModActor and Primary Asset Label")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				const FString Name = SNewModDialog::ShowModal();
				if (!Name.IsEmpty())
				{
					ModActions::CreateMod(Name);
				}
			})));

		Builder.AddMenuEntry(
			FText::FromString(TEXT("Plugin Settings...")),
			FText::FromString(TEXT("Open SN2 Mod Tools settings (game directory, UAT path)")),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				FModuleManager::LoadModuleChecked<ISettingsModule>(TEXT("Settings"))
					.ShowViewer(TEXT("Editor"), TEXT("Plugins"), TEXT("SN2ModTools"));
			})));
	}

	static TSharedRef<FExtender> ExtendPathContextMenu(const TArray<FString>& SelectedPaths)
	{
		TSharedRef<FExtender> Extender = MakeShared<FExtender>();

		if (SelectedPaths.Num() != 1) return Extender;

		const FString ModName = ModActions::ModNameFromFolderPath(SelectedPaths[0]);
		if (ModName.IsEmpty()) return Extender;

		Extender->AddMenuExtension(
			TEXT("PathViewFolderOptions"),
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateLambda([ModName](FMenuBuilder& Builder)
			{
				Builder.BeginSection(TEXT("SN2ModActions"), FText::FromString(TEXT("SN2 Mod Tools")));

				Builder.AddMenuEntry(
					FText::FromString(TEXT("Cook & Install")),
					FText::FromString(FString::Printf(
						TEXT("Package '%s' and copy to your SN2 install"), *ModName)),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("MainFrame.PackageProject")),
					FUIAction(FExecuteAction::CreateLambda([ModName]()
					{
						ModActions::CookAndInstallMod(ModName);
					})));

				Builder.AddMenuEntry(
					FText::FromString(TEXT("Uninstall")),
					FText::FromString(FString::Printf(
						TEXT("Remove '%s' pak files from your SN2 install"), *ModName)),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Delete")),
					FUIAction(FExecuteAction::CreateLambda([ModName]()
					{
						ModActions::UninstallMod(ModName);
					})));

				Builder.EndSection();
			}));

		return Extender;
	}

	FDelegateHandle PathExtenderHandle;
	TSharedPtr<FSlateStyleSet> StyleSet;
};

IMPLEMENT_MODULE(FSN2ModToolsModule, SN2ModTools)
