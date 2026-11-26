#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EidolonArenaHUD.generated.h"
UCLASS()
class EIDOLON_API AEidolonArenaHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
