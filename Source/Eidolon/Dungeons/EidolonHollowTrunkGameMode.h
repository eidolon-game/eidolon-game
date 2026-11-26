#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonHollowTrunkGameMode.generated.h"

UCLASS()
class EIDOLON_API AEidolonHollowTrunkGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

public:
    AEidolonHollowTrunkGameMode();
    void Tick(float DeltaSeconds) override;

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool BindCompletionActors(UWorld& World) override;
    void UpdateCompletionActors(bool bCompleted) override;
    bool InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control) override;
    FString GetControlPrompt() const override;
    void SpawnOptionalThreats() override;

private:
    void PlacePin(const FVector& Location, const FQuat& Rotation);
    void ReturnPinToAlcove();
    void DropPin(AEidolonCharacter& Player);

    TWeakObjectPtr<ATargetPoint> Alcove;
    TWeakObjectPtr<ATargetPoint> Socket;
    TWeakObjectPtr<AStaticMeshActor> Pin;
    TWeakObjectPtr<AStaticMeshActor> Walkway;
    TWeakObjectPtr<AEidolonCharacter> Carrier;
    FTransform AlcovePose;
    bool bRecovered = false;
};
