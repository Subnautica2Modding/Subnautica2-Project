#include "AlpakitModEntry.h"
#include "Alpakit.h"
#include "AlpakitSettings.h"
#include "AlpakitStyle.h"
#include "Slate.h"
#include "Async/Async.h"
#include "IUATHelperModule.h"

#define LOCTEXT_NAMESPACE "AlpakitModListEntry"

void SAlpakitModEntry::Construct(const FArguments& Args, TSharedRef<IPlugin> InMod, TSharedRef<SAlpakitModEntryList> InOwner) {
    Mod = InMod;
	Owner = InOwner;

    UAlpakitSettings* Settings = UAlpakitSettings::Get();
    const FString PluginName = Mod->GetName();

    Checkbox = SNew(SCheckBox)
        .OnCheckStateChanged(this, &SAlpakitModEntry::OnEnableCheckboxChanged)
        .IsChecked(Settings->ModSelection.FindOrAdd(PluginName, false));

    ChildSlot[
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0).VAlign(VAlign_Center)[
            Checkbox.ToSharedRef()
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0).VAlign(VAlign_Center)[
            SNew(SButton)
            .Text(LOCTEXT("PackageModAlpakit", "Cook & Install"))
            .OnClicked_Lambda([this](){
                PackageMod(TArray<TSharedPtr<SAlpakitModEntry>>());
                return FReply::Handled();
            })
            .ToolTipText_Lambda([this](){
                return FText::FromString(FString::Printf(TEXT("Cook & install %s into the game"), *this->Mod->GetName()));
            })
        ]
        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)[
            SNew(STextBlock)
            .Text_Lambda([InMod]() {
                const FString DisplayText = FString::Printf(TEXT("%s (%s)"), *InMod->GetDescriptor().FriendlyName, *InMod->GetName());
                return FText::FromString(DisplayText);
            })
            .HighlightText_Lambda([InOwner]() {
                return FText::FromString(InOwner->GetLastFilter());
            })
        ]
    ];
}

FText GetCurrentPlatformName() {
#if PLATFORM_WINDOWS
    return LOCTEXT("PlatformName_Windows", "Windows");
#elif PLATFORM_MAC
    return LOCTEXT("PlatformName_Mac", "Mac");
#elif PLATFORM_LINUX
    return LOCTEXT("PlatformName_Linux", "Linux");
#else
    return LOCTEXT("PlatformName_Other", "Other OS");
#endif
}

void SAlpakitModEntry::PackageMod(const TArray<TSharedPtr<SAlpakitModEntry>>& NextEntries) const {
    UAlpakitSettings* Settings = UAlpakitSettings::Get();
    const FString PluginName = Mod->GetName();

    const FString ProjectPath = FPaths::IsProjectFilePathSet()
        ? FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath())
        : FPaths::RootDir() / FApp::GetProjectName() / FApp::GetProjectName() + TEXT(".uproject");

    // The game install dir is read from GameInstallDirectory.txt
    FString AdditionalUATArguments;
    if (Settings->LaunchGameAfterPacking == EAlpakitStartGameType::STEAM && NextEntries.Num() == 0) {
        AdditionalUATArguments.Append(TEXT("-LaunchGame "));
    }

    UE_LOG(LogAlpakit, Display, TEXT("Cook & install plugin \"%s\". %d remaining"), *PluginName, NextEntries.Num());

    // On source engine builds UAT does not search project/plugin folders for automation script modules
    // makes UAT compile Alpakit.Automation when its build record is missing or stale
    FString ScriptDirArgument;
    if (const TSharedPtr<IPlugin> AlpakitPlugin = IPluginManager::Get().FindPlugin(TEXT("Alpakit"))) {
        const FString ScriptDir = FPaths::ConvertRelativePathToFull(AlpakitPlugin->GetBaseDir() / TEXT("Source") / TEXT("Alpakit.Automation"));
        if (FPaths::DirectoryExists(ScriptDir)) {
            ScriptDirArgument = FString::Printf(TEXT("-ScriptDir=\"%s\" "), *ScriptDir);
        } else {
            UE_LOG(LogAlpakit, Warning, TEXT("Alpakit.Automation source directory not found at \"%s\"; UAT may fail to find the PackagePlugin command"), *ScriptDir);
        }
    }

    const FString CommandLine = FString::Printf(TEXT("-ScriptsForProject=\"%s\" %sPackagePlugin -Project=\"%s\" -PluginName=\"%s\" %s"),
                                                *ProjectPath, *ScriptDirArgument, *ProjectPath, *PluginName, *AdditionalUATArguments);

    const FText PlatformName = GetCurrentPlatformName();
    IUATHelperModule::Get().CreateUatTask(
        CommandLine,
        PlatformName,
        LOCTEXT("PackageModTaskName", "Cooking & Installing Mod"),
        LOCTEXT("PackageModTaskShortName", "Cook & Install Mod"),
        FAlpakitStyle::Get().GetBrush("Alpakit.OpenPluginWindow"),
        nullptr,
        NextEntries.Num() == 0 ? (IUATHelperModule::UatTaskResultCallack)nullptr : [NextEntries](FString resultType, double runTime) {
            AsyncTask(ENamedThreads::GameThread, [NextEntries]() {
                TSharedPtr<SAlpakitModEntry> NextMod = NextEntries[0];

                TArray<TSharedPtr<SAlpakitModEntry>> RemainingEntries = NextEntries.FilterByPredicate([NextMod](const TSharedPtr<SAlpakitModEntry>& X) {
                    return X != NextMod;
                });

                NextMod->PackageMod(RemainingEntries);
            });
        }
    );
}

void SAlpakitModEntry::OnEnableCheckboxChanged(ECheckBoxState NewState) {
    UAlpakitSettings* Settings = UAlpakitSettings::Get();
    const FString PluginName = Mod->GetName();

    Settings->ModSelection.Add(PluginName, NewState == ECheckBoxState::Checked);

    Settings->SaveSettings();
}

#undef LOCTEXT_NAMESPACE
