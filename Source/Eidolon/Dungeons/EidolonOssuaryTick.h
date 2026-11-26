#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonOssuaryTickRules.h"
#include "EidolonOssuaryTick.generated.h"

UCLASS()
class EIDOLON_API AEidolonOssuaryTick : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonOssuaryTick();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

private:
    EidolonOssuaryTick::Attack Attack;
    FVector LockedDirection = FVector::ZeroVector;
};
