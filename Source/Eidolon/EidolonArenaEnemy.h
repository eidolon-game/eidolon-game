#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EidolonArenaEnemy.generated.h"
class UStaticMeshComponent;
UCLASS()
class EIDOLON_API AEidolonArenaEnemy : public AActor
{
    GENERATED_BODY()
public:
    AEidolonArenaEnemy();
    virtual void Tick(float DeltaSeconds) override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    float GetHealth() const { return Health; }
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    float Health = 60.f;
    float AttackCooldown = 0.f;
};
