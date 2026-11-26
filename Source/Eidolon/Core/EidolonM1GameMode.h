#pragma once
#include "EidolonCoreGameMode.h"
#include "EidolonM1GameMode.generated.h"

// Opt-in legacy campaign smoke adapter, not a standalone sandbox yet.
UCLASS()
class EIDOLON_API AEidolonM1GameMode : public AEidolonCoreGameMode
{
    GENERATED_BODY()
public:
    AEidolonM1GameMode();
};
