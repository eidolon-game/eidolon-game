#pragma once

#include "EidolonAuthoredAbyssGameMode.h"
#include "EidolonQuarryThresholdGameMode.generated.h"

class AController;
class APlayerStart;

UCLASS()
class EIDOLON_API AEidolonQuarryThresholdGameMode : public AEidolonAuthoredAbyssGameMode
{
    GENERATED_BODY()

public:
    void RestartPlayer(AController* Controller) override;
    void Interact(AEidolonCharacter* Player) override;

protected:
    FEidolonAbyssEncounter GetEncounter() const override;
    bool BindCompletionActors(UWorld& World) override;
    bool InteractWithControl(AEidolonCharacter& Player, const ATargetPoint& Control) override;
    void SpawnOptionalThreats() override;

private:
    bool CanUseStart(const APlayerStart& Start) const;
    bool ConfirmArrival(AEidolonCharacter& Player);
    void ReturnToRiftroot();

    TWeakObjectPtr<ATargetPoint> BorerStart;
    TWeakObjectPtr<ATargetPoint> ChargeLimit;
    TWeakObjectPtr<AStaticMeshActor> QuarryGate;
    bool bFallbackQueued = false;
    bool bThreatSpawned = false;
};
