#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonVeinOverlookGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonVeinOverlookGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
};
