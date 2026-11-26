#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonLintelMantisRules.h"
#include "EidolonLintelMantis.generated.h"

class AEidolonCharacter;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonLintelMantis : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonLintelMantis();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

private:
    void UpdateTelegraph();
    bool SweepDive(const FVector& Movement);

    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftBlade;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightBlade;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LandingMarker;
    EidolonLintelMantis::Attack Attack;
    TWeakObjectPtr<AEidolonCharacter> LockedPlayer;
    FVector LockedDirection = FVector::ZeroVector;
    double DiveTravel = 0.0;
    double LedgeClearance = 0.0;
    double DropHeight = 0.0;
};
