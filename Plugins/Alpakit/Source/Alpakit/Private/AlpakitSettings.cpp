#include "AlpakitSettings.h"
#include "Alpakit.h"

UAlpakitSettings* UAlpakitSettings::Get() {
    return GetMutableDefault<UAlpakitSettings>();
}

void UAlpakitSettings::SaveSettings() {
    SaveConfig();

    UE_LOG(LogAlpakit, Verbose, TEXT("Saved Alpakit settings to %s"), *GetClass()->GetConfigName());
}

#if WITH_EDITOR
void UAlpakitSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);

    SaveSettings();
}
#endif
