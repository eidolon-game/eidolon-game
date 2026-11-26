#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonRiftrootGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonRiftrootGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool ConfigureEnemy(AEidolonArenaEnemy& Enemy) override;
};
