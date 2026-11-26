#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EidolonArenaEnemy.generated.h"
class AEidolonArenaEnemy;
class UStaticMeshComponent;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnArenaEnemyDefeated, AEidolonArenaEnemy&);
UCLASS()
class EIDOLON_API AEidolonArenaEnemy : public AActor
{
    GENERATED_BODY()
public:
    AEidolonArenaEnemy();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    void Configure(FName Type, int32 Difficulty);
    float GetHealth() const { return Health; }
    FOnArenaEnemyDefeated OnEncounterDefeated;
private:
    void OnLethalDamage();
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    float Health = 60.f;
    float AttackCooldown = 0.f;
    float Windup = 0.f;
    float Damage = 12.f;
    float Speed = 180.f;
    float Reach = 140.f;
    float Recovery = 1.25f;
    float WindupDuration = .55f;
    bool bWindingUp = false;
    FName ObjectiveType;
    int32 OriginChapter = 0;
};
