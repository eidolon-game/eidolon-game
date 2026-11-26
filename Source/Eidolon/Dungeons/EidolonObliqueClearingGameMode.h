#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonObliqueClearingGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonObliqueClearingGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
};
