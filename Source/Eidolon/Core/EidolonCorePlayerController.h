#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EidolonCorePlayerController.generated.h"

UCLASS()
class EIDOLON_API AEidolonCorePlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Targeting") bool ToggleLockOn();
protected:
    virtual void OnUnPossess() override;
};
