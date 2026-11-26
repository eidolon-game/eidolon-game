#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonOssuaryGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonOssuaryGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
};
