#pragma once

#include "CoreMinimal.h"
#include "AlpakitSettings.generated.h"

UENUM()
enum class EAlpakitStartGameType : uint8 {
    NONE UMETA(DisplayName = "Do not launch"),
    STEAM UMETA(DisplayName = "Steam"),
};

UCLASS(config=Game)
class ALPAKIT_API UAlpakitSettings : public UObject {
    GENERATED_BODY()
public:
    /** Retrieves global instance of alpakit settings */
    static UAlpakitSettings* Get();

    /** Saves alpakit settings to configuration file */
    void SaveSettings();

    /** Whether (and how) to launch the game after a plugin is cooked & installed */
    UPROPERTY(EditAnywhere, config, Category = Config)
    EAlpakitStartGameType LaunchGameAfterPacking = EAlpakitStartGameType::NONE;

    /** Persisted per-plugin checkbox state in the Alpakit window */
    UPROPERTY(BlueprintReadOnly, config, Category = Config)
    TMap<FString, bool> ModSelection;
};
