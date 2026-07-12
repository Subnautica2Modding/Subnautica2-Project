#include "AlpakitModEntryList.h"
#include "Alpakit.h"
#include "AlpakitModEntry.h"
#include "Interfaces/IPluginManager.h"
#include "Slate.h"

#define LOCTEXT_NAMESPACE "AlpakitModListEntry"

void SAlpakitModEntryList::Construct(const FArguments& Args) {
    ChildSlot[
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth()[
                SNew(SButton)
                .Text(LOCTEXT("PackageModAlpakitAll", "Cook & Install Selected"))
                .OnClicked(this,& SAlpakitModEntryList::PackageAllMods)
            ]
            + SHorizontalBox::Slot().AutoWidth()[
                SNew(SButton)
                .Text(LOCTEXT("PackageModCheckAll", "Check All"))
                .OnClicked(this,& SAlpakitModEntryList::CheckAllMods)
            ]
            + SHorizontalBox::Slot().AutoWidth()[
                SNew(SButton)
                .Text(LOCTEXT("PackageModUncheckAll", "Check None"))
                .OnClicked(this,& SAlpakitModEntryList::UncheckAllMods)
            ]
        ]
        + SVerticalBox::Slot().FillHeight(1.0f)[
            SNew(SScrollBox)
            .Orientation(Orient_Vertical)
            .ScrollBarAlwaysVisible(true)
            + SScrollBox::Slot()[
                SAssignNew(ModList, SListView<TSharedRef<IPlugin>>)
                .SelectionMode(ESelectionMode::None)
                .ListItemsSource(&FilteredMods)
                .OnGenerateRow_Lambda(
                    [this](TSharedRef<IPlugin> Mod, const TSharedRef<STableViewBase>& List) {
                        return SNew(STableRow<TSharedRef<IPlugin>>, List)[
                            SNew(SAlpakitModEntry, Mod, SharedThis(this))
                        ];
                    })
            ]
        ]
    ];

    LoadMods();
    IPluginManager::Get().OnNewPluginCreated().AddSP(this, &SAlpakitModEntryList::OnNewPluginCreated);
}

void SAlpakitModEntryList::LoadMods() {
    Mods.Empty();
    const TArray<TSharedRef<IPlugin>> EnabledPlugins = IPluginManager::Get().GetEnabledPlugins();
    for (TSharedRef<IPlugin> Plugin : EnabledPlugins) {
        // Only project plugins that can contain content are candidates for content-only DLC mods.
        // (Engine plugins and pure C++ plugins are intentionally excluded.)
        if (Plugin->GetType() == EPluginType::Project && Plugin->GetDescriptor().bCanContainContent) {
            Mods.Add(Plugin);
        }
    }
    Mods.Sort([](const TSharedRef<IPlugin> Plugin1, const TSharedRef<IPlugin> Plugin2) {
        return Plugin1->GetName() < Plugin2->GetName();
    });
    Filter(LastFilter);
}

bool PluginMatchesSearchTokens(const IPlugin& Plugin, const TArray<FString>& Tokens) {
    const FString PluginName = Plugin.GetName();
    const FString FriendlyName = Plugin.GetDescriptor().FriendlyName;
    const FString Description = Plugin.GetDescriptor().Description;

    for (const FString& Token : Tokens) {
        if (PluginName.Contains(Token) ||
            FriendlyName.Contains(Token) ||
            Description.Contains(Token)) {
            return true;
        }
    }
    return false;
}

void SAlpakitModEntryList::Filter(const FString& InFilter) {
    LastFilter = InFilter;
    FilteredMods.Empty();

    if (InFilter.IsEmpty()) {
        FilteredMods = Mods;
    } else {
        TArray<FString> FilterTokens;
        InFilter.ParseIntoArray(FilterTokens, TEXT(" "), true);

        for (TSharedRef<IPlugin> Mod : Mods) {
            if (PluginMatchesSearchTokens(Mod.Get(), FilterTokens)) {
                FilteredMods.Add(Mod);
            }
        }
    }

    ModList->RequestListRefresh();
}

FString SAlpakitModEntryList::GetLastFilter() const {
    return LastFilter;
}

void SAlpakitModEntryList::OnNewPluginCreated(IPlugin& Plugin)
{
    LoadMods();
}

FReply SAlpakitModEntryList::PackageAllMods() {
    TSharedPtr<SAlpakitModEntry> First;
    TArray<TSharedPtr<SAlpakitModEntry>> NextEntries;

    UE_LOG(LogAlpakit, Display, TEXT("Cook & Install Selected"));

    for (TSharedRef<IPlugin> Mod : FilteredMods) {
        TSharedPtr<ITableRow> TableRow = ModList->WidgetFromItem(Mod);
        if (!TableRow.IsValid()) {
            continue;
        }

        TSharedPtr<SAlpakitModEntry> ModEntry = StaticCastSharedPtr<SAlpakitModEntry>(TableRow->GetContent());
        if (!ModEntry.IsValid()) {
            continue;
        }

        if(!ModEntry->IsSelected()) {
            continue;
        }

        if (!First) {
            First = ModEntry.ToSharedRef();
        } else {
            NextEntries.Add(ModEntry.ToSharedRef());
        }
    }

    if (First) {
        First->PackageMod(NextEntries);
    }

    return FReply::Handled();
}

FReply SAlpakitModEntryList::CheckAllMods() {
    for (TSharedRef<IPlugin> Mod : FilteredMods) {
        TSharedPtr<ITableRow> TableRow = ModList->WidgetFromItem(Mod);
        if (!TableRow.IsValid()) {
            continue;
        }

        TSharedPtr<SAlpakitModEntry> ModEntry = StaticCastSharedPtr<SAlpakitModEntry>(TableRow->GetContent());
        if (!ModEntry.IsValid()) {
            continue;
        }
        ModEntry->SetSelected(true);
    }
    return FReply::Handled();
}

FReply SAlpakitModEntryList::UncheckAllMods() {
    for (TSharedRef<IPlugin> Mod : FilteredMods) {
        TSharedPtr<ITableRow> TableRow = ModList->WidgetFromItem(Mod);
        if (!TableRow.IsValid()) {
            continue;
        }

        TSharedPtr<SAlpakitModEntry> ModEntry = StaticCastSharedPtr<SAlpakitModEntry>(TableRow->GetContent());
        if (!ModEntry.IsValid()) {
            continue;
        }
        ModEntry->SetSelected(false);
    }
    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
