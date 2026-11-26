#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonVaultPincerRules.h"
#include "EidolonVaultPincer.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonVaultPincer : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonVaultPincer();
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

private:
    bool HasClearSweep(const AActor& Target) const;
    void UpdatePresentation();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Claw;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftMarker;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> CenterMarker;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightMarker;
    UPROPERTY() TObjectPtr<UPointLightComponent> PhaseLight;
    EidolonVaultPincer::Attack Attack;
};
