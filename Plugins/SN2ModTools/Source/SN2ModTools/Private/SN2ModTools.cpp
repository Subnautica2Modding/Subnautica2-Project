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

#include "NewModDialog.h"
#include "ModActions.h"

class FSN2ModToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
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
	}

	void BuildModdingMenu(FMenuBuilder& Builder)
	{
		Builder.AddMenuEntry(
			FText::FromString(TEXT("New Mod...")),
			FText::FromString(TEXT("Create a new mod folder with ModActor, WBP, and PrimaryAssetLabel.")),
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
			FText::FromString(TEXT("Open SN2 Mod Tools settings (game directory, UAT path).")),
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
				Builder.BeginSection(TEXT("SN2ModActions"), FText::FromString(TEXT("SN2 Mod")));

				Builder.AddMenuEntry(
					FText::FromString(TEXT("Cook & Install")),
					FText::FromString(FString::Printf(
						TEXT("Package '%s' and copy to your SN2 install."), *ModName)),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([ModName]()
					{
						ModActions::CookAndInstallMod(ModName);
					})));

				Builder.AddMenuEntry(
					FText::FromString(TEXT("Uninstall")),
					FText::FromString(FString::Printf(
						TEXT("Remove '%s' pak files from your SN2 install."), *ModName)),
					FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([ModName]()
					{
						ModActions::UninstallMod(ModName);
					})));

				Builder.EndSection();
			}));

		return Extender;
	}

	FDelegateHandle PathExtenderHandle;
};

IMPLEMENT_MODULE(FSN2ModToolsModule, SN2ModTools)
