#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EidolonGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AEidolonGameMode();
    virtual void StartPlay() override;
};
