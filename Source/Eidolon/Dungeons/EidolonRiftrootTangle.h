#pragma once

#include "../EidolonArenaEnemy.h"
#include "EidolonRiftrootRules.h"
#include "EidolonRiftrootTangle.generated.h"

class AEidolonCharacter;
class UStaticMeshComponent;

UCLASS()
class EIDOLON_API AEidolonRiftrootTangle : public AEidolonArenaEnemy
{
    GENERATED_BODY()

public:
    AEidolonRiftrootTangle();
    void BeginPlay() override;
    void Tick(float DeltaSeconds) override;
    float TakeDamage(float Amount, const FDamageEvent& Event,
                     AController* Instigator, AActor* Causer) override;

    // Coordinates are the feet of the authored center anchors.
    bool ConfigureFaultAdvance(const FVector& SpawnFeet, const FVector& DestinationFeet);

private:
    bool HasSightOf(const AEidolonCharacter& Player) const;
    bool HasGroundSupport(const FVector& Center) const;
    bool IsFaultRouteClear() const;
    void AimAt(const FVector& Target);
    void ClipRockWedge(const AEidolonCharacter* Player);
    void UpdatePose();

    UPROPERTY() TObjectPtr<UStaticMeshComponent> Boulder;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Cavity;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GroundMarker;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Roots;
    EidolonRiftroot::Attack Attack;
    FVector FaultDestination = FVector::ZeroVector;
    bool bFaultConfigured = false;
    bool bFaultMoving = false;
    bool bNextRootDraw = false;
    int32 LastRootSide = -1;
    double StartingHealth = 0.0;
};
