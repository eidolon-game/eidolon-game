#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonMarrowRamRules.h"
#include "EidolonMarrowRam.generated.h"

class AEidolonMarrowRam;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMarrowRamChargeObstructed, AEidolonMarrowRam&, AActor&);

UCLASS()
class EIDOLON_API AEidolonMarrowRam : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonMarrowRam();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

    FOnMarrowRamChargeObstructed OnChargeObstructed;

private:
    void UpdatePose();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> HeadPlate;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> LeftHoof;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> RightHoof;
    EidolonMarrowRam::Attack Attack;
    FVector Home = FVector::ZeroVector;
    FVector LockedDirection = FVector::ZeroVector;
};
