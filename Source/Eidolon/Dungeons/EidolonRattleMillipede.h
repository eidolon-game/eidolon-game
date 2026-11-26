#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonRattleMillipedeRules.h"
#include "EidolonRattleMillipede.generated.h"

class AEidolonCharacter;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonRattleMillipede : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonRattleMillipede();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

    // World-space actor-center points, starting at the spawn anchor.
    bool SetPatrolRoute(const TArray<FVector>& Points);

private:
    void AdvancePatrol(double DeltaSeconds);
    void ReversePatrol();
    bool HasGroundSupport(const FVector& Location) const;
    bool HasSightOf(const AEidolonCharacter& Player) const;
    void UpdateTelegraph();

    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Plates;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Telegraph;
    EidolonRattleMillipede::Attack Attack;
    TArray<FVector> Patrol;
    int32 LastWaypoint = 0;
    int32 NextWaypoint = 0;
    bool bForward = true;
    double PatrolPause = 0.0;
};
