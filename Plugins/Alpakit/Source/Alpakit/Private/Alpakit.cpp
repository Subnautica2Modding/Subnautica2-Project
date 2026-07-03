#include "Alpakit.h"
#include "AlpakitStyle.h"
#include "AlpakitCommands.h"
#include "AlpakitSettings.h"
#include "AlpakitWidget.h"
#include "ISettingsContainer.h"
#include "ISettingsModule.h"
#include "ISettingsSection.h"
#include "LevelEditor.h"

static const FName AlpakitTabName("Alpakit");

#define LOCTEXT_NAMESPACE "FAlpakitModule"

DEFINE_LOG_CATEGORY(LogAlpakit)

void FAlpakitModule::StartupModule() {
    //Register editor settings
    RegisterSettings();

    //Initialize Slate stuff, including commands
    FAlpakitStyle::Initialize();
    FAlpakitStyle::ReloadTextures();
    FAlpakitCommands::Register();

    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FAlpakitCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateLambda([](){
            FGlobalTabmanager::Get()->TryInvokeTab(AlpakitTabName);
        }),
        FCanExecuteAction());
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

    //Register Alpakit in Editor's Project Menu
    TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
    MenuExtender->AddMenuExtension(TEXT("FileProject"), EExtensionHook::After, PluginCommands,
        FMenuExtensionDelegate::CreateLambda([](FMenuBuilder& Builder){
            Builder.AddMenuEntry(FAlpakitCommands::Get().OpenPluginWindow);
        }));
    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

    //Register Alpakit in Editor's Toolbar
    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
    ToolbarExtender->AddToolBarExtension(TEXT("Play"), EExtensionHook::After, PluginCommands,
        FToolBarExtensionDelegate::CreateLambda([](FToolBarBuilder& Builder) {
            Builder.AddToolBarButton(FAlpakitCommands::Get().OpenPluginWindow);
        }));
    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);

    //Register the Alpakit window tab spawner
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AlpakitTabName,
        FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&){
            return SNew(SDockTab)
                .TabRole(NomadTab)
                [ SNew(SAlpakitWidget) ];
        }))
        .SetDisplayName(LOCTEXT("FAlpakitTabTitle", "Alpakit"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FAlpakitModule::ShutdownModule() {
    if (UObjectInitialized()) {
        UnregisterSettings();
    }

    FAlpakitStyle::Shutdown();
    FAlpakitCommands::Unregister();

    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(AlpakitTabName);
}

void FAlpakitModule::RegisterSettings() const {
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings"))) {
        ISettingsContainerPtr SettingsContainer = SettingsModule->GetContainer(TEXT("Project"));
        SettingsContainer->DescribeCategory(TEXT("Alpakit Settings"),
            LOCTEXT("RuntimeWDCategoryName", "Alpakit Settings"),
            LOCTEXT("RuntimeWDCategoryDescription", "Alpakit Settings. Can also be edited in the Alpakit menu"));

        ISettingsSectionPtr SettingsSection = SettingsModule->RegisterSettings(TEXT("Project"), TEXT("Alpakit"), TEXT("General"),
            LOCTEXT("RuntimeGeneralSettingsName", "General"),
            LOCTEXT("RuntimeGeneralSettingsDescription", "Alpakit Settings. Can also be edited in the Alpakit menu"),
            UAlpakitSettings::Get()
        );
        if (SettingsSection.IsValid()) {
            SettingsSection->OnModified().BindLambda([](){
                UAlpakitSettings::Get()->SaveSettings();
                return true;
            });
        }
    }
}

void FAlpakitModule::UnregisterSettings() const {
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>(TEXT("Settings"))) {
        SettingsModule->UnregisterSettings(TEXT("Project"), TEXT("Alpakit"), TEXT("General"));
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAlpakitModule, Alpakit)
