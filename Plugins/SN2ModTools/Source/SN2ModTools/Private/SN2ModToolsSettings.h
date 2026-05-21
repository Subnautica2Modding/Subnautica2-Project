#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SN2ModToolsSettings.generated.h"

UCLASS(Config=EditorPerProjectUserSettings, meta=(DisplayName="SN2 Mod Tools"))
class USN2ModToolsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	USN2ModToolsSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FName GetSectionName() const override { return TEXT("SN2ModTools"); }

	UPROPERTY(Config, EditAnywhere, Category="Installation",
		meta=(DisplayName="SN2 Game Directory",
			ToolTip="Folder containing Subnautica2.exe."))
	FDirectoryPath GameDir;

	UPROPERTY(Config, EditAnywhere, Category="Build",
		meta=(DisplayName="RunUAT.bat Path (override)",
			ToolTip="Leave blank to use the engine this project was opened with."))
	FFilePath RunUATOverride;

	static FString DetectGameDir();
};
