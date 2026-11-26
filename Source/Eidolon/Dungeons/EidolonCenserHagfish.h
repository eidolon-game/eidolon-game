#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonCenserHagfishRules.h"
#include "EidolonCenserHagfish.generated.h"

class UPointLightComponent;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonCenserHagfish : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonCenserHagfish();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

private:
    bool HasAvailableWorldClaim() const;
    void UpdateStrip();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Strip;
    UPROPERTY() TObjectPtr<UPointLightComponent> StripLight;
    EidolonCenserHagfish::Cast Attack;
    FVector LockedOrigin = FVector::ZeroVector;
    FVector LockedDirection = FVector::ZeroVector;
};
