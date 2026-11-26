#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonMortarLeechRules.h"
#include "EidolonMortarLeech.generated.h"

class AEidolonCharacter;
class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonMortarLeech : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonMortarLeech();
    void Tick(float DeltaSeconds) override;

private:
    double ClearLength(const FVector& Origin, const FVector& Direction,
                       double MaxLength, const AEidolonCharacter* Player) const;
    void ResolveStrike(AEidolonCharacter& Player);
    void UpdateLance();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Lance;
    UPROPERTY() TObjectPtr<UPointLightComponent> TelegraphLight;
    EidolonMortarLeech::Attack Attack;
    FVector LockedDirection = FVector::ZeroVector;
};
