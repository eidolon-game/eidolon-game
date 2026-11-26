#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EidolonCoreGameInstance.generated.h"

UCLASS()
class EIDOLON_API UEidolonCoreGameInstance : public UGameInstance
{
    GENERATED_BODY()
public:
    virtual void Init() override;
    virtual void Shutdown() override;
};
