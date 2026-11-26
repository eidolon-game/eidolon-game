#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonRootletCanalGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonRootletCanalGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool BindCompletionActors(UWorld& World) override;
    void UpdateCompletionActors(bool bCompleted) override;
    bool InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control) override;
    void SpawnOptionalThreats() override;

private:
    TWeakObjectPtr<AStaticMeshActor> Portcullis;
    TWeakObjectPtr<AStaticMeshActor> Counterweight;
    TWeakObjectPtr<ATargetPoint> RaisedGate;
    TWeakObjectPtr<ATargetPoint> RestingWeight;
    FVector ClosedGatePosition = FVector::ZeroVector;
    FVector HeldWeightPosition = FVector::ZeroVector;
};
