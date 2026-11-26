#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonLintelGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonLintelGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
};
