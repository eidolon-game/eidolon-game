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
    bool ConfigurePerch(const FVector& PerchFeet, const FVector& LandingFeet);

private:
    enum class PerchStage { Disabled, Ground, Climb, Waiting, Telegraph, Drop, ReturnToGround };

    bool CanSee(const AEidolonCharacter& Player) const;
    bool CanStandAt(const FVector& Position) const;
    bool SweepGrounded(const FVector& Movement, FHitResult& Hit);
    void StalkToward(const FVector& Destination, double DeltaSeconds, double Speed);
    void UpdatePose();
    bool TickPerch(AEidolonCharacter* Player, bool bVisible, double DeltaSeconds);
    bool IsPerchPathClear(const FVector& Lower, const AActor* PreviewTarget = nullptr) const;
    FVector GetPerchRoutePoint(const FVector& Lower, double Alpha) const;
    void BeginPerchTravel(PerchStage Stage, const FVector& Lower, bool bAscending);
    void TickPerchTravel(AEidolonCharacter* Player, double DeltaSeconds);
    void ShowPerchLanding(bool bShow);

    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Hooks;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Crest;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Telegraph;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> PerchMarker;
    EidolonForestStalker::Search Search;
    EidolonForestStalker::Attack Attack;
    FVector Home = FVector::ZeroVector;
    FVector LastSeen = FVector::ZeroVector;
    FVector LockedDirection = FVector::ZeroVector;
    TWeakObjectPtr<AEidolonCharacter> LockedPlayer;
    PerchStage Perch = PerchStage::Disabled;
    EidolonForestStalker::PerchTravel Flight;
    FVector PerchCenter = FVector::ZeroVector;
    FVector PerchGround = FVector::ZeroVector;
    FVector LandingCenter = FVector::ZeroVector;
    FVector FlightLower = FVector::ZeroVector;
    double PerchRemaining = 0.0;
};
