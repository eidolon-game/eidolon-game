#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonRootclawRules.h"
#include "EidolonRootclaw.generated.h"

class AEidolonCharacter;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonRootclaw : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonRootclaw();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

    // Three adjacent actor-center positions at one authored arena cross-link.
    bool SetLanePositions(const TArray<FVector>& Positions);

private:
    bool TryBeginLaneStep(const AEidolonCharacter& Player);
    bool HasSightOf(const AEidolonCharacter& Player) const;
    void AimAt(const AEidolonCharacter& Player);
    void UpdatePose();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> HeavyClaw;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LightClaw;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GroundMarker;
    EidolonRootclaw::Attack Attack;
    TArray<FVector> Lanes;
    FVector LaneDestination = FVector::ZeroVector;
    int32 AttacksSinceLane = 0;
    int32 NextSweepSign = 1;
    double StartingHealth = 0.0;
};
