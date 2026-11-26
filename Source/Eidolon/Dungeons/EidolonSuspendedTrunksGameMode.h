#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonSuspendedTrunksGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonSuspendedTrunksGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool BindCompletionActors(UWorld& World) override;
    bool InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control) override;
    void UpdateCompletionActors(bool bCompleted) override;
    FString GetControlPrompt() const override;
    void SpawnOptionalThreats() override;

private:
    bool IsRotationClear(const AStaticMeshActor& Deck) const;

    TWeakObjectPtr<AStaticMeshActor> Bridge;
    FVector Pivot = FVector::ZeroVector;
    FRotator RestPose = FRotator::ZeroRotator;
    FRotator AlignedPose = FRotator::ZeroRotator;
    double AlignmentDelta = 0.0;
    bool bBridgeOccupied = false;
};
