#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonForestStalkerRules.h"
#include "EidolonForestStalker.generated.h"

class AEidolonCharacter;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonForestStalker : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonForestStalker();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

private:
    bool CanSee(const AEidolonCharacter& Player) const;
    bool CanStandAt(const FVector& Position) const;
    bool SweepGrounded(const FVector& Movement, FHitResult& Hit);
    void StalkToward(const FVector& Destination, double DeltaSeconds, double Speed);
    void UpdatePose();

    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Hooks;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Crest;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Telegraph;
    EidolonForestStalker::Search Search;
    EidolonForestStalker::Attack Attack;
    FVector Home = FVector::ZeroVector;
    FVector LastSeen = FVector::ZeroVector;
    FVector LockedDirection = FVector::ZeroVector;
    TWeakObjectPtr<AEidolonCharacter> LockedPlayer;
};
