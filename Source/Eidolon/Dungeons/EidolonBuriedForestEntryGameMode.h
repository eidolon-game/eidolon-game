#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonBuriedForestEntryGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonBuriedForestEntryGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
};
