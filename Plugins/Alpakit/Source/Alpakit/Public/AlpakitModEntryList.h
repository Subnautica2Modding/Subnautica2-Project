#pragma once
#include "Interfaces/IPluginManager.h"

/**
 * Widget that lists the project's content plugins that can be cooked & installed as DLC mods
 */
class SAlpakitModEntryList : public SCompoundWidget {
	SLATE_BEGIN_ARGS(SAlpakitModEntryList) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	/** Updates the plugin list. */
	void LoadMods();

	/** Filters the plugins for the given string */
	void Filter(const FString& InFilter);

	/** Returns the string previously used to filter */
	FString GetLastFilter() const;

	void OnNewPluginCreated(IPlugin& Plugin);
private:
	TSharedPtr<SListView<TSharedRef<IPlugin>>> ModList;
	TArray<TSharedRef<IPlugin>> Mods;
	TArray<TSharedRef<IPlugin>> FilteredMods;
	FString LastFilter;

	FReply PackageAllMods();
	FReply CheckAllMods();
	FReply UncheckAllMods();
};
