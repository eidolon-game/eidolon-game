#pragma once
#include "../EidolonGameMode.h"
#include "EidolonCoreGameMode.generated.h"

// Opt-in adapter preserving the existing campaign prototype.
UCLASS()
class EIDOLON_API AEidolonCoreGameMode : public AEidolonGameMode
{
    GENERATED_BODY()
public:
    AEidolonCoreGameMode();
};
