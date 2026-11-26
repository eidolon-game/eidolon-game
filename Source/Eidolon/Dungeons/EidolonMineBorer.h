#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonMineBorerRules.h"
#include "EidolonMineBorer.generated.h"

class AEidolonCharacter;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonMineBorer : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonMineBorer();

    bool ConfigureChargeLane(const FVector& End);
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

private:
    void UpdatePose();
    void ResolveDebris(AEidolonCharacter* Player);
    bool CanSee(const AEidolonCharacter& Player) const;

    UPROPERTY() TObjectPtr<UBoxComponent> CollisionBody;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Head;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftPlate;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightPlate;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> DangerMarker;
    EidolonMineBorer::Attack Attack;
    FVector Home = FVector::ZeroVector;
    FVector Direction = FVector::ZeroVector;
    double LaneLength = 0.0;
    bool bHitPlayer = false;
};
