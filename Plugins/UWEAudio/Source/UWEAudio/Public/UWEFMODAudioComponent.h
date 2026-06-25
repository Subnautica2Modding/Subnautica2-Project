#pragma once

#include "CoreMinimal.h"
#include "FMODAudioComponent.h"
#include "UWEFMODAudioComponent.generated.h"

UCLASS()
class UWEAUDIO_API UUWEFMODAudioComponent : public UFMODAudioComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FMOD Audio")
    int32 PlayingCountWarningThreshold = 0;
};
