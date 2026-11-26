#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonBranchTerraceGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonBranchTerraceGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    void SpawnOptionalThreats() override;
};
