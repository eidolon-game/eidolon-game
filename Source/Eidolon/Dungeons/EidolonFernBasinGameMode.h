#pragma once

#include "EidolonSluiceGameMode.h"
#include "EidolonFernBasinGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonFernBasinGameMode : public AEidolonSluiceGameMode
{
    GENERATED_BODY()

protected:
    FEidolonDrainLayout GetDrainLayout() const override;
    AActor* SpawnThreat(UWorld& World) override;
};
